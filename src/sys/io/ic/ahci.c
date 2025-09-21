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
#include <io/pci/pci.h>
#include <io/pci/bar.h>
#include <io/ic/ahciregs.h>
#include <io/ic/ahcivar.h>
#include <os/module.h>
#include <os/clkdev.h>
#include <os/mmio.h>

/*
 * Represents the PCI advocation descriptor for this
 * device so we can advertise ourselves as its driver.
 */
static struct pci_adv driver;

/* Various other state */
static struct pci_device dev;
static struct clkdev *clkdev;
static struct ahci_hba root_hba;

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
        printf("ahci: HBA reset timed out\n");
        return error;
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
    uint32_t ghc;
    int error;

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
    return 0;
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
        printf("ahci_init: could not get clkdev\n");
        return error;
    }

    if ((error = pci_advoc(&driver)) < 0) {
        printf("ahci_init: failed to advocate for HBA\n");
        return error;
    }

    root_hba.io = NULL;
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
    printf("ahci: detected AHCI controller\n");

    /* Map ABAR */
    error = pci_map_bar(&dev, 5, &bs);
    if (error < 0) {
        printf("ahci: failed to map bar 5 (error=%d)\n", error);
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
