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
 * Description: USB 3.0 controller driver
 * Author: Ian Marco Moffett
 */

#include <sys/types.h>
#include <sys/syslog.h>
#include <sys/errno.h>
#include <os/bus.h>
#include <os/mmio.h>
#include <os/clkdev.h>
#include <io/usb/xhcivar.h>
#include <io/usb/xhciregs.h>
#include <io/pci/cam.h>
#include <io/pci/bar.h>
#include <io/pci/pci.h>
#include <os/module.h>
#include <vm/vm.h>
#include <vm/physseg.h>
#include <string.h>

#define pr_trace(fmt, ...) printf("xhci: " fmt, ##__VA_ARGS__)
#if defined(XHCI_DEBUG)
#define dtrace(fmt, ...) printf("xhci: " fmt, ##__VA_ARGS__)
#else
#define dtrace(...) __nothing
#endif  /* AHCI_DEBUG */

static struct pci_adv adv;
static struct pci_device dev;
static struct xhci_hcd hcd;
static struct clkdev *clk;

/*
 * Poll a register until it is either set or unset
 *
 * @reg: Register pointer to poll
 * @mask: Mask to poll
 * @pollset: If true, poll until set
 */
static int
xhci_poll32(volatile uint32_t *reg, uint32_t mask, bool pollset)
{
    uint16_t msec = 0;
    uint32_t v, tmp;

    if (reg == NULL) {
        return -EINVAL;
    }

    for (;;) {
        if (msec >= XHCI_TIMEOUT_MSEC) {
            break;
        }

        v = mmio_read32(reg);
        v &= mask;

        if (pollset && v == mask)
            return 0;
        if (!pollset && v == 0)
            return 0;

        clk->msleep(1);
        ++msec;
    }

    return -ETIME;
}

/*
 * Perform a hard host controller reset
 */
static int
xhci_reset_hc(struct xhci_hcd *hcd)
{
    struct xhci_opregs *opregs;
    uint32_t usbcmd;
    int error;

    if (hcd == NULL) {
        return -EINVAL;
    }

    opregs = XHCI_OPBASE(hcd->capspace);

    /* Reset the controller */
    usbcmd = mmio_read32(&opregs->usbcmd);
    usbcmd |= USBCMD_HCRST;
    mmio_write32(&opregs->usbcmd, usbcmd);

    /* Wait for it to be done */
    error = xhci_poll32(&opregs->usbcmd, USBCMD_HCRST, false);
    if (error < 0) {
        pr_trace("failed to initialize controller\n");
        return error;
    }

    /*
     * Section 4.2 of the XHCI spec states that we also
     * need to wait for the controller to be ready via
     * the USBSTS.CNR (controller not ready) bit
     */
    error = xhci_poll32(&opregs->usbcmd, USBSTS_CNR, false);
    if (error < 0) {
        pr_trace("hang waiting for controller ready\n");
        return error;
    }

    return 0;
}

/*
 * Initialize the device context base address
 * array pointer register
 */
static int
xhci_init_dcbaap(struct xhci_hcd *hcd)
{
    struct xhci_opregs *opregs;
    uint32_t npages;
    void *va;

    npages = BYTES_TO_PAGES(hcd->max_slots * XHCI_CTX_SIZE);
    hcd->dcbaap_pa = vm_alloc_frame(npages);
    if (hcd->dcbaap_pa == 0) {
        pr_trace("failed to allocate dcbaap\n");
        return -ENOMEM;
    }

    /* Ensure it is zeroed */
    va = PHYS_TO_VIRT(hcd->dcbaap_pa);
    memset(va, 0, npages * DEFAULT_PAGESIZE);

    /* Give it to the controller, fetch! */
    opregs = XHCI_OPBASE(hcd->capspace);
    mmio_write32(&opregs->dcbaa_ptr, hcd->dcbaap_pa);
    return 0;
}

/*
 * Initialize the host controller
 */
static int
xhci_init_hc(struct xhci_hcd *hcd)
{
    struct xhci_opregs *opregs;
    struct xhci_capregs *capspace;
    paddr_t pa;
    uint32_t usbcmd, hcsparams1;
    uint32_t config;
    int error;

    if (hcd == NULL) {
        return -EINVAL;
    }

    if ((error = xhci_reset_hc(hcd)) < 0) {
        return error;
    }

    if ((capspace = hcd->capspace) == NULL) {
        return -EIO;
    }

    /* Get the structural params 1 */
    opregs = XHCI_OPBASE(capspace);
    hcsparams1 = mmio_read32(&capspace->hcsparams1);
    hcd->max_slots = HCSPARAMS1_MAXSLOTS(hcsparams1);
    hcd->max_intrs = HCSPARAMS1_MAXINTRS(hcsparams1);
    hcd->max_ports = HCSPARAMS1_MAXPORTS(hcsparams1);

    /* Enable all the slots */
    config = mmio_read32(&opregs->config);
    config |= hcd->max_slots;
    mmio_write32(&opregs->config, config);

    if ((error = xhci_init_dcbaap(hcd)) < 0) {
        return error;
    }
    return 0;
}

static void
xhci_pci_init(struct pci_device *devp)
{
    uint32_t config;

    if (devp == NULL) {
        return;
    }

    config = pci_readl(devp, PCIREG_CMDSTATUS);
    config |= PCI_BUS_MASTERING | PCI_MEM_SPACE;
    pci_writel(devp, PCIREG_CMDSTATUS, config);
}

static int
xhci_init(struct module *modp)
{
    int error;

    error = clkdev_get(CLKDEV_MSLEEP | CLKDEV_GET_USEC, &clk);
    if (error < 0) {
        return error;
    }

    if ((error = pci_advoc(&adv)) < 0) {
        return error;
    }

    return 0;
}

static int
xhci_attach(struct pci_adv *ap)
{
    static bool once = false;
    struct bus_space bs;
    int error;

    /* Only run once */
    if (once) {
        return -EAGAIN;
    }

    once = true;
    pr_trace("detected xHCI controller\n");

    /* Get the BAR */
    dev = ap->lookup;
    error = pci_map_bar(&dev, 0, &bs);
    if (error < 0) {
        pr_trace("failed to get BAR 0 and 1\n");
        return error;
    }

    xhci_pci_init(&dev);
    hcd.capspace = bs.va_base;
    return xhci_init_hc(&hcd);
}

static struct pci_adv adv = {
    .lookup = PCI_CSI_ID(0x0C, 0x03, 0x30),
    .idtype = PCI_LU_ICLASSREV,
    .attach = xhci_attach
};

MODULE_EXPORT("xhci", MODTYPE_PCI, xhci_init);
