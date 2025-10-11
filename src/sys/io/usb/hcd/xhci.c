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

#include <sys/types.h>
#include <sys/syslog.h>
#include <sys/errno.h>
#include <os/bus.h>
#include <os/mmio.h>
#include <io/usb/xhcivar.h>
#include <io/usb/xhciregs.h>
#include <io/pci/bar.h>
#include <io/pci/pci.h>
#include <os/module.h>

#define pr_trace(fmt, ...) printf("xhci: " fmt, ##__VA_ARGS__)
#if defined(XHCI_DEBUG)
#define dtrace(fmt, ...) printf("xhci: " fmt, ##__VA_ARGS__)
#else
#define dtrace(...) __nothing
#endif  /* AHCI_DEBUG */

static struct pci_adv adv;
static struct pci_device dev;
static struct xhci_hcd hcd;

static int
xhci_init(struct module *modp)
{
    int error;

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

    hcd.io = bs.va_base;
    return 0;
}

static struct pci_adv adv = {
    .lookup = PCI_CSI_ID(0x0C, 0x03, 0x30),
    .idtype = PCI_LU_ICLASSREV,
    .attach = xhci_attach
};

MODULE_EXPORT("xhci", MODTYPE_PCI, xhci_init);
