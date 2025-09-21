/*
 * Copyright (c) 2025 Ian Marco Moffett and L5 engineers
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Description: AHCI HBA driver
 * Author: Ian Marco Moffett
 */

#include <sys/types.h>
#include <sys/syslog.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <io/pci/pci.h>
#include <io/pci/bar.h>
#include <io/ic/ahciregs.h>
#include <io/ic/ahcivar.h>
#include <os/kalloc.h>
#include <os/module.h>
#include <os/clkdev.h>
#include <os/mmio.h>

#define pr_trace(fmt, ...) printf("ahci: " fmt, ##__VA_ARGS__)
#if defined(AHCI_DEBUG)
#define dtrace(fmt, ...) printf("ahci: " fmt ##__VA_ARGS__)
#else
#define dtrace(...) __nothing
#endif  /* AHCI_DEBUG */

/*
 * Represents the PCI advocation descriptor for this
 * device so we can advertise ourselves as its driver.
 */
static struct pci_adv driver;

/* Various other state */
static struct pci_device dev;
static struct clkdev *clkdev;
static struct ahci_hba root_hba;
static TAILQ_HEAD(, ahci_port) portlist;

/*
 * Poll register to have 'bits' set/unset.
 *
 * @reg: Register to poll.
 * @bits: Bits to be checked.
 * @pollset: True to poll as set.
 *
 * Returns zero if the register updated in time, otherwise a less
 * than zero value upon failure.
 */
static int
ahci_poll32(volatile uint32_t *reg, uint32_t bits, bool pollset)
{
    size_t usec_start, usec;
    size_t elapsed_msec;
    uint32_t val;
    bool tmp;

    usec_start = clkdev->get_time_usec();
    for (;;) {
        val = mmio_read32(reg);
        tmp = (pollset) ? ISSET(val, bits) : !ISSET(val, bits);

        usec = clkdev->get_time_usec();
        elapsed_msec = (usec - usec_start) / 1000;

        /* If tmp is set, the register updated in time */
        if (tmp) {
            break;
        }

        /* Exit with an error if we timeout */
        if (elapsed_msec > AHCI_TIMEOUT) {
            return -ETIME;
        }
    }

    return 0;
}

/*
 * Stop a running port, turn off the command list engine,
 * as well as its FIS receive engine
 *
 * @port: Port to stop
 *
 * Returns zero if the port has been stopped successfully,
 * otherwise a less than zero value
 */
static int
ahci_port_stop(struct ahci_port *port)
{
    volatile struct hba_port *io = port->io;
    uint32_t cmd, mask;
    int error;

    /*
     * If the port is already stopped, then don't try to do
     * it again.
     */
    mask = AHCI_PXCMD_FR | AHCI_PXCMD_CR;
    cmd = mmio_read32(&io->cmd);
    if (!ISSET(cmd, mask)) {
        dtrace("port %d already stopped\n", port->portno);
        return 0;
    }

    /* Stop everything */
    dtrace("stopping port %d...\n", port->portno);
    cmd &= ~(AHCI_PXCMD_FRE | AHCI_PXCMD_ST);
    mmio_write32(&io->cmd, cmd);

    /* Wait until everything has truly stopped */
    error = ahci_poll32(&io->cmd, mask, false);
    if (error < 0) {
        pr_trace("timed out stopping port %d\n", port->portno);
        return error;
    }

    return 0;
}

/*
 * Perform a full HBA reset by using the HR bit
 * within the GHC register.
 *
 * See section 10.4.3 of the AHCI spec for more
 * information.
 */
static int
ahci_hba_reset(struct ahci_hba *hba)
{
    volatile struct hba_memspace *io = hba->io;
    uint32_t ghc;
    int error;

    dtrace("resetting HBA...\n");

    /*
     * The HBA must be in an AHCI aware state before
     * we can follow through with a reset
     */
    ghc = mmio_read32(&io->ghc);
    if (!ISSET(ghc, AHCI_GHC_AE)) {
        ghc |= AHCI_GHC_AE;
        mmio_write32(&io->ghc, ghc);
    }

    /* Now perform the actual reset */
    ghc |= AHCI_GHC_HR;
    mmio_write32(&io->ghc, ghc);

    /* Wait until it is done */
    error = ahci_poll32(&io->ghc, AHCI_GHC_HR, false);
    if (error < 0) {
        pr_trace("HBA reset timed out\n");
        return error;
    }

    dtrace("HBA reset success\n");
    return 0;
}

/*
 * Logically detaches our link with the port, cleans up
 * resources, etc.
 *
 * @port: Port to "detach"
 */
static void
ahci_port_detach(struct ahci_port *port)
{
    if (port == NULL) {
        return;
    }

    kfree(port);
}

/*
 * Initialize a specific port on the HBA as per section
 * 10.1.2 of the AHCI specification
 *
 * @hba: HBA port belongs to
 * @port: Port to initialize
 *
 * XXX: Upon failure, the caller is to clean up resources relating
 *      to 'port'
 */
static int
ahci_init_port(struct ahci_hba *hba, struct ahci_port *port)
{
    volatile struct hba_port *regs;
    uint32_t cmd;

    if (hba == NULL || port == NULL) {
        return -EINVAL;
    }

    ahci_port_stop(port);
    TAILQ_INSERT_TAIL(&portlist, port, link);
    return 0;
}

/*
 * Initialize the ports of an HBA
 */
static int
ahci_init_ports(struct ahci_hba *hba)
{
    volatile struct hba_memspace *io = hba->io;
    struct ahci_port *port;
    uint32_t pi, nbits;
    int error;

    pi = hba->pi;
    for (int i = 0; i < hba->nport; ++i) {
        if (!ISSET(pi, BIT(i))) {
            continue;
        }

        /* Allocate a new port descriptor */
        dtrace("port %d implemented\n", i);
        port = kalloc(sizeof(*port));
        if (port == NULL) {
            printf("failed to allocate port\n");
            continue;
        }

        port->io = &io->ports[i];
        port->portno = i;
        port->parent = hba;

        /* Initialize the port */
        error = ahci_init_port(hba, port);
        if (error < 0) {
            ahci_port_detach(port);
            printf("port init failed (error=%d)\n", error);
            continue;
        }
    }

    return 0;
}

/*
 * Put the HBA as well as its devices in an initialized
 * state so that they may be used for operation.
 */
static int
ahci_hba_init(struct ahci_hba *hba)
{
    volatile struct hba_memspace *io = hba->io;
    uint32_t ghc, cap;
    int error;

    /* Yoink from firmware before reset */
    cap = mmio_read32(&io->cap);
    hba->pi = mmio_read32(&io->pi);
    hba->nport = AHCI_CAP_NP(cap) + 1;

    /*
     * We cannot be so certain what state the BIOS or whatever
     * firmware left the host controller in, therefore the HBA
     * will require a reset so we can have it in a known state.
     */
    if ((error = ahci_hba_reset(hba)) < 0) {
        return error;
    }

    /*
     * Make the HBA AHCI aware, we do this after the reset
     * as the reset logic ensures of this before continuing.
     *
     * We simply do it twice as the reset value is zero and we
     * want to ensure it is still AHCI aware after this.
     */
    ghc = mmio_read32(&io->ghc);
    ghc |= AHCI_GHC_AE;
    mmio_write32(&io->ghc, ghc);
    return ahci_init_ports(hba);
}

/*
 * Initialize only the AHCI driver's state rather than
 * the hardware it covers. This is used so we can advertise
 * ourself to the PCI driver.
 */
static int
ahci_init(struct module *modp)
{
    uint16_t clkmask;
    int error;

    clkmask = CLKDEV_MSLEEP | CLKDEV_GET_USEC;
    error = clkdev_get(clkmask, &clkdev);
    if (error < 0) {
        pr_trace("could not get clkdev\n");
        return error;
    }

    if ((error = pci_advoc(&driver)) < 0) {
        pr_trace("failed to advocate for HBA\n");
        return error;
    }

    root_hba.io = NULL;
    TAILQ_INIT(&portlist);
    return 0;
}

/*
 * Ran when a device is attached, does the actual
 * hardware initialization.
 */
static int
ahci_attach(struct pci_adv *adv)
{
    struct bus_space bs;
    int error;

    /* Only call once */
    if (root_hba.io != NULL) {
        return -EIO;
    }

    dev = adv->lookup;
    pr_trace("detected AHCI controller\n");

    /* Map ABAR */
    error = pci_map_bar(&dev, 5, &bs);
    if (error < 0) {
        pr_trace("failed to map bar 5 (error=%d)\n", error);
        return error;
    }

    root_hba.io = (void *)bs.va_base;
    return ahci_hba_init(&root_hba);
}

static struct pci_adv driver = {
    .lookup = PCI_CS_ID(0x1, 0x06),
    .attach = ahci_attach,
    .classrev = 1
};

MODULE_EXPORT("ahci", MODTYPE_PCI, ahci_init);
