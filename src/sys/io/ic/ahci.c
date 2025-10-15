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
#include <io/dma/alloc.h>
#include <vm/physseg.h>
#include <dms/dms.h>
#include <os/kalloc.h>
#include <os/module.h>
#include <os/clkdev.h>
#include <os/mmio.h>
#include <string.h>

#define pr_trace(fmt, ...) printf("ahci: " fmt, ##__VA_ARGS__)
#if defined(AHCI_DEBUG)
#define dtrace(fmt, ...) printf("ahci: " fmt, ##__VA_ARGS__)
#else
#define dtrace(...) __nothing
#endif  /* AHCI_DEBUG */

struct bufargs {
    void *buf;
    size_t nblocks;
    off_t lba;
    uint8_t write : 1;
};

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

/* Disk management operations */
static struct dms_ops dms_ops;

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
 * Allocate a new command list for an HBA port
 */
static int
ahci_alloc_cmdslot(struct ahci_hba *hba, struct ahci_port *port)
{
    volatile struct hba_port *io = port->io;
    uint32_t slotlist;

    slotlist = mmio_read32(&io->ci);
    slotlist |= mmio_read32(&io->sact);

    for (int i = 0; i < hba->nslots; ++i) {
        if (!ISSET(slotlist, i)) {
            return i;
        }
    }
    return -EAGAIN;
}

/*
 * Submit a command to a specific port
 */
static int
ahci_submit_cmd(struct ahci_hba *hba, struct ahci_port *port, uint8_t slot)
{
    const uint32_t BUSY_BITS = (AHCI_PXTFD_BSY | AHCI_PXTFD_DRQ);
    volatile struct hba_port *io = port->io;
    uint32_t ci;
    uint8_t attempts = 0;
    int status = 0;

    if (hba == NULL || port == NULL) {
        return -1;
    }

    /*
     * Wait until the port is not busy so that we can
     * send any commands
     */
    if (ahci_poll32(&io->tfd, BUSY_BITS, false) < 0) {
        pr_trace("cmd failed, port busy ((slot=%d)\n", slot);
        return -EBUSY;
    }

    /* Submit and wait for it to be done */
    ci = mmio_read32(&io->ci);
    mmio_write32(&io->ci, ci | BIT(slot));
    while ((attempts++) < 10) {
        status = ahci_poll32(&io->ci, BIT(slot), false);
        if (status == 0) {
            break;
        }
    }

    if (status != 0) {
        return status;
    }

    return 0;
}


/*
 * Log information and register the disk
 * to the DMS subsystem
 */
static int
ahci_register(struct ata_identity *identity, struct ahci_port *port)
{
    struct dms_disk *dp;
    char tmp;
    char serial[SERIAL_LEN];
    char model[MODEL_LEN];
    int error;

    if (identity == NULL) {
        return -EINVAL;
    }

    memcpy(
        serial,
        identity->serial_number,
        SERIAL_LEN
    );
    memcpy(
        model,
        identity->model_number,
        sizeof(model)
    );

    serial[SERIAL_LEN - 1] = '\0';
    model[MODEL_LEN - 1] = '\0';

    /* Fixup endianess for serial number */
    for (size_t i = 0; i < SERIAL_LEN; i += 2) {
        tmp = serial[i];
        serial[i] = serial[i + 1];
        serial[i + 1] = tmp;
    }

    /* Fixup endianess for model number */
    for (size_t i = 0; i < MODEL_LEN; i += 2) {
        tmp = model[i];
        model[i] = model[i + 1];
        model[i + 1] = tmp;
    }

    pr_trace("detected %s\n", model);
    pr_trace("serial number: %s\n", serial);

    error = dms_register(model, &dms_ops, &dp);
    if (error < 0) {
        pr_trace("could not register drive!\n");
        return error;
    }

    dp->bsize = port->bsize;
    dp->data = (void *)port;
    return 0;
}

/*
 * Send an ATA identify command to a port
 */
static int
ahci_identify(struct ahci_hba *hba, struct ahci_port *port)
{
    volatile struct hba_port *io = port->io;
    struct ahci_cmd_hdr *cmdhdr;
    struct ahci_cmdtab *cmdtbl;
    struct ahci_fis_h2d *fis;
    paddr_t buf, cmdbase;
    int cmdslot, status;

    buf = vm_alloc_frame(1);
    if (buf == 0) {
        pr_trace("identify: failed to allocate frame\n");
        return -ENOMEM;
    }

    /* Get the command list entry for this slot */
    cmdslot = ahci_alloc_cmdslot(hba, port);
    cmdbase = port->cmdlist;
    cmdbase += cmdslot * sizeof(*cmdhdr);

    cmdhdr = PHYS_TO_VIRT(cmdbase);
    cmdhdr->w = 0;
    cmdhdr->cfl = sizeof(struct ahci_fis_h2d) / 4;
    cmdhdr->prdtl = 1;

    cmdtbl = PHYS_TO_VIRT(cmdhdr->ctba);
    cmdtbl->prdt[0].dba = buf;
    cmdtbl->prdt[0].dbc = 511;
    cmdtbl->prdt[0].i = 0;

    fis = (void *)&cmdtbl->cfis;
    fis->command = ATA_CMD_IDENTIFY;
    fis->c = 1;
    fis->type = FIS_TYPE_H2D;

    status = ahci_submit_cmd(hba, port, cmdslot);
    if (status < 0) {
        vm_free_frame(buf, 1);
        return status;
    }

    ahci_register(PHYS_TO_VIRT(buf), port);
    vm_free_frame(buf, 1);
    return 0;
}
/*
 * Read or write a SATA drive
 */
static int
ahci_rw(struct ahci_hba *hba, struct ahci_port *port, struct bufargs *bufd)
{
    volatile struct hba_port *io;
    struct ahci_cmd_hdr *cmdhdr;
    struct ahci_cmdtab *cmdtbl;
    struct ahci_fis_h2d *fis;
    paddr_t buf, cmdbase;
    void *va;
    int cmdslot, status;
    size_t npgs, bsize;

    if (hba == NULL || port == NULL)
        return -EINVAL;
    if (bufd == NULL)
        return -EINVAL;

    if (bufd->buf == NULL)
        return -EINVAL;
    if (bufd->nblocks == 0)
        return -EINVAL;

    io = port->io;
    bsize = port->bsize;

    npgs = ALIGN_UP((bufd->nblocks * bsize), DEFAULT_PAGESIZE);
    npgs /= DEFAULT_PAGESIZE;

    buf = vm_alloc_frame(npgs);
    if (buf == 0) {
        pr_trace("identify: failed to allocate frame\n");
        return -ENOMEM;
    }

    if (bufd->write) {
        va = PHYS_TO_VIRT(buf);
        memcpy(va, bufd->buf, bufd->nblocks * bsize);
    }

    /* Get the command list entry for this slot */
    cmdslot = ahci_alloc_cmdslot(hba, port);
    cmdbase = port->cmdlist;
    cmdbase += cmdslot * sizeof(*cmdhdr);

    cmdhdr = PHYS_TO_VIRT(cmdbase);
    cmdhdr->w = bufd->write;
    cmdhdr->cfl = sizeof(struct ahci_fis_h2d) / 4;
    cmdhdr->prdtl = 1;

    cmdtbl = PHYS_TO_VIRT(cmdhdr->ctba);
    cmdtbl->prdt[0].dba = buf;
    cmdtbl->prdt[0].dbc = (bufd->nblocks << 9) - 1;
    cmdtbl->prdt[0].i = 0;

    fis = (void *)&cmdtbl->cfis;
    fis->command = bufd->write ? ATA_CMD_WRITE_DMA : ATA_CMD_READ_DMA;
    fis->c = 1;
    fis->type = FIS_TYPE_H2D;
    fis->device = (1 << 6); /* LBA */

    /* Encode the LBA into the FIS */
    fis->lba1 = (bufd->lba >> 8) & 0xFF;
    fis->lba2 = (bufd->lba >> 16) & 0xFF;
    fis->lba3 = (bufd->lba >> 24) & 0xFF;
    fis->lba4 = (bufd->lba >> 32) & 0xFF;
    fis->lba5 = (bufd->lba >> 40) & 0xFF;

    /* Encode the block count into the FIS */
    fis->countl = bufd->nblocks & 0xFF;
    fis->counth = (bufd->nblocks >> 8) & 0xFF;

    status = ahci_submit_cmd(hba, port, cmdslot);
    if (status < 0) {
        vm_free_frame(buf, npgs);
        return status;
    }

    /*
     * Now copy it to the real buffer if
     * reading
     */
    if (!bufd->write) {
        memcpy(
            bufd->buf,
            PHYS_TO_VIRT(buf),
            bufd->nblocks * bsize
        );
    }

    vm_free_frame(buf, npgs);
    return 0;
}

/*
 * DMS write interface
 */
static ssize_t
sata_write(struct dms_disk *dp, void *p, off_t off, size_t len)
{
    struct bufargs bd;
    struct ahci_port *port;
    size_t real_off, real_len;
    size_t bsize;
    int error;

    if (dp == NULL || p == NULL) {
        return -EINVAL;
    }

    if (len == 0) {
        return -EINVAL;
    }

    if ((port = dp->data) == NULL) {
        return -EIO;
    }

    bsize = port->bsize;
    real_off = ALIGN_DOWN(off, bsize);
    real_len = ALIGN_UP(len, bsize);

    bd.buf = p;
    bd.nblocks = real_len / bsize;
    bd.lba = real_off / bsize;
    bd.write = 1;

    error = ahci_rw((void *)port->parent, port, &bd);
    if (error < 0) {
        return error;
    }

    return real_len;
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
 * Bring up an AHCI port, start the command list engine and
 * FIS receive engine.
 *
 * @port: Port to bring up
 *
 * Returns zero if the port has been brought up successfully,
 * otherwise a less than zero value
 */
static int
ahci_port_start(struct ahci_port *port)
{
    volatile struct hba_port *io = port->io;
    uint32_t cmd, mask;
    int error;

    if (port == NULL) {
        return -EINVAL;
    }

    /* Don't start if already started */
    mask = AHCI_PXCMD_FR | AHCI_PXCMD_CR;
    cmd = mmio_read32(&io->cmd);
    if (ISSET(cmd, mask)) {
        dtrace("port %d already started\n", port->portno);
        return 0;
    }

    /* Bring the port up */
    cmd |= (AHCI_PXCMD_FRE | AHCI_PXCMD_ST);
    mmio_write32(&io->cmd, cmd);

    /* Wait until everything is up and running */
    error = ahci_poll32(&io->cmd, mask, true);
    if (error < 0) {
        pr_trace("timed out starting port %d\n", port->portno);
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
 * Send a COMRESET over the interface to reset a
 * specific port. This is typically used after resetting
 * the controller.
 *
 * @port: The target port to reset
 */
static int
ahci_reset_port(struct ahci_port *port)
{
    const char *spdtab[5] = {
        "0 Gbit/s (inactive)",
        "1.5 Gbit/s",   /* SATA gen 1 */
        "3 Gbit/s",     /* SATA gen 2 */
        "6 Gbit/s",     /* SATA gen 3 */
        "bad"           /* Reserved */
    };
    volatile struct hba_port *io;
    uint32_t sctl, ssts;
    uint8_t ipm, spd;

    if (port == NULL) {
        return -EINVAL;
    }

    io = port->io;

    /*
     * Section 3.3.11 of the AHCI spec states that to transmit a
     * COMRESET on the interface, we'll need to hold a value of 1
     * at PxSCTL.DET for a minimum of 1ms.
     */
    sctl = mmio_read32(&io->sctl);
    sctl &= ~0xF;
    sctl |= 1;
    mmio_write32(&io->sctl, sctl);

    /* Be generous and give it 3ms */
    clkdev->msleep(3);

    /* Stop COMRESET TX */
    sctl &= ~0xF;
    mmio_write32(&io->sctl, sctl);
    ahci_port_start(port);

    /* Give it some time to come up */
    for (int i = 0; i < AHCI_TIMEOUT; ++i) {
        clkdev->msleep(1);

        /* Get the current port status */
        ssts = mmio_read32(&io->ssts);
        ipm = AHCI_PXSSTS_IPM(ssts);
        if (ipm == AHCI_IPM_ACTIVE) {
            break;
        }
    }

    if (ipm != AHCI_IPM_ACTIVE) {
        dtrace("port %d not active after reset\n", port->portno);
        return -EIO;
    }

    spd = AHCI_PXSSTS_SPD(ssts);
    pr_trace("port %d interface online\n", port->portno);
    pr_trace("port %d clocked @ %s\n", port->portno, spdtab[spd]);
    ahci_port_stop(port);
    return 0;
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
    const uint16_t BSIZE = 512;
    volatile struct hba_port *regs;
    struct ahci_cmd_hdr *cmdlist;
    uint32_t cmd, lo, hi;
    size_t clen;
    paddr_t pa;
    int error;

    if (hba == NULL || port == NULL) {
        return -EINVAL;
    }

    regs = port->io;
    if ((error = ahci_reset_port(port)) < 0) {
        return error;
    }

    port->bsize = BSIZE;

    clen = ALIGN_UP(hba->nslots * AHCI_CMDENTRY_SIZE, DEFAULT_PAGESIZE);
    clen /= DEFAULT_PAGESIZE;
    port->cmdlist = vm_alloc_frame(clen);
    if (port->cmdlist == 0) {
        return -ENOMEM;
    }

    /* Program the command list in */
    lo = port->cmdlist & 0xFFFFFFFF;
    hi = (port->cmdlist >> 32) & 0xFFFFFFFF;
    mmio_write32(&regs->clb, lo);
    mmio_write32(&regs->clbu, hi);

    /* Set up each command slot */
    cmdlist = dma_get_va(port->cmdlist);
    for (int i = 0; i < hba->nslots; ++i) {
        /* Allocate H2D FIS area */
        cmdlist[i].prdtl = 1;
        cmdlist[i].ctba = vm_alloc_frame(1);
    }

    /* Allocate FIS recieve area */
    port->fis_rx = vm_alloc_frame(1);

    /* Program FIS recieve area for port */
    lo = port->fis_rx & 0xFFFFFFFF;
    hi = (port->fis_rx >> 32) & 0xFFFFFFFF;
    mmio_write32(&regs->fb, lo);
    mmio_write32(&regs->fbu, hi);

    /* Clear errors and bring up the port */
    mmio_write32(&regs->serr, 0xFFFFFFFF);
    error = ahci_port_start(port);
    if (error < 0) {
        return error;
    }

    TAILQ_INSERT_TAIL(&portlist, port, link);
    ahci_identify(hba, port);
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

    pr_trace("bringing up ports...\n");
    pi = hba->pi;
    for (int i = 0; i < hba->nport; ++i) {
        if (!ISSET(pi, BIT(i))) {
            continue;
        }

        /* Allocate a new port descriptor */
        dtrace("port %d implemented\n", i);
        port = kalloc(sizeof(*port));
        if (port == NULL) {
            dtrace("failed to allocate port\n");
            continue;
        }

        port->io = &io->ports[i];
        port->portno = i;
        port->parent = hba;

        /* Initialize the port */
        error = ahci_init_port(hba, port);
        if (error < 0) {
            ahci_port_detach(port);
            dtrace("port init failed (error=%d)\n", error);
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
    hba->nslots = AHCI_CAP_NCS(cap) + 1;

    /* Only support 64-bit addressing as of now */
    if (AHCI_CAP_S64A(cap) == 0) {
        pr_trace("HBA does not support 64-bit addressing\n");
        pr_trace("aborting..\n");
        return -ENOTSUP;
    }

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
 * Initialize bus mastering and MMIO for the host
 * bus adapter
 */
static void
ahci_pci_init(struct pci_device *devp)
{
    uint32_t config;

    if (devp == NULL) {
        return;
    }

    config = pci_readl(devp, PCIREG_CMDSTATUS);
    config |= PCI_BUS_MASTERING | PCI_MEM_SPACE;
    pci_writel(devp, PCIREG_CMDSTATUS, config);
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

    ahci_pci_init(&dev);
    root_hba.io = (void *)bs.va_base;
    return ahci_hba_init(&root_hba);
}

static struct dms_ops dms_ops = {
    .write = sata_write,
    .read = NULL
};

static struct pci_adv driver = {
    .lookup = PCI_CS_ID(0x1, 0x06),
    .attach = ahci_attach,
    .idtype = PCI_LU_CLASSREV
};

MODULE_EXPORT("ahci", MODTYPE_PCI, ahci_init);
