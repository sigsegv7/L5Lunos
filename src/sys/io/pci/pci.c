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
 * Description: MI PCI(e) bus driver
 * Author: Ian Marco Moffett
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/cdefs.h>
#include <sys/queue.h>
#include <sys/panic.h>
#include <sys/syslog.h>
#include <os/kalloc.h>
#include <os/module.h>
#include <io/pci/pci.h>
#include <io/pci/cam.h>
#include <string.h>

#if defined(__PCI_MAX_BUS)
#define PCI_MAX_BUS  __PCI_MAX_BUS
#if PCI_MAX_BUS > 256
#error "PCI_MAX_BUS must be <= 256"
#endif  /* PCI_MAX_BUS */
#else
#define PCI_MAX_BUS 1
#endif  /* __PCI_MAX_BUS */

static TAILQ_HEAD(, pci_device) devlist;
static TAILQ_HEAD(, pci_adv) advlist;
static struct cam_hook cam;

/*
 * Lookup hook for matching vendor and device IDs
 *
 * @vda: Vendor device (A)
 * @vdb: Vendor device (B)
 *
 * Returns zero on match
 */
static int
pci_vd_match(struct pci_device *vda, struct pci_device *vdb)
{
    if (vda == NULL || vdb == NULL)
        return -EINVAL;

    /* The actual match */
    if (vda->device != vdb->device)
        return -1;
    if (vda->vendor != vdb->vendor)
        return -1;

    return 0;
}

/*
 * Lookup hook for matching class and subclass IDs
 *
 * @csa: Class / subclass (A)
 * @csb: Class / subclass (B)
 *
 * Returns zero on match
 */
static int
pci_cs_match(struct pci_device *csa, struct pci_device *csb)
{
    if (csa == NULL || csb == NULL)
        return -EINVAL;

    /* Do the actual match */
    if (csa->class != csb->class)
        return -1;
    if (csa->subclass != csb->subclass)
        return -1;

    return 0;
}

/*
 * Attempt to register a PCI device and bail
 * if it doesn't exist on the bus.
 */
static void
pci_register_dev(struct pci_device *dev)
{
    struct pci_device *devp;
    pcireg_t vend_dev;
    uint32_t classrev;
    uint8_t class, subclass;
    uint8_t prog_if;
    uint16_t device_id;
    uint16_t vendor_id;

    if (dev == NULL) {
        return;
    }

    /* Get the vendor and device ID */
    vend_dev = pci_readl(dev, PCIREG_VENDOR_ID);
    vendor_id = vend_dev & 0xFFFF;
    device_id = (vend_dev >> 16) & 0xFFFF;

    /* Get the class and revision */
    classrev = pci_readl(dev, PCIREG_CLASSREV);
    class = PCIREG_CLASS(classrev);
    subclass = PCIREG_SUBCLASS(classrev);
    prog_if = PCIREG_PROGIF(classrev);

    /* Does this device exist? */
    if (vendor_id == 0xFFFF) {
        return;
    }

    dev->vendor = vendor_id;
    dev->device = device_id;
    dev->class = class;
    dev->subclass = subclass;
    dev->prog_if = prog_if;

    /* Set up base address registers */
    dev->bar[0] = pci_readl(dev, PCIREG_BAR0);
    dev->bar[1] = pci_readl(dev, PCIREG_BAR1);
    dev->bar[2] = pci_readl(dev, PCIREG_BAR2);
    dev->bar[3] = pci_readl(dev, PCIREG_BAR3);
    dev->bar[4] = pci_readl(dev, PCIREG_BAR4);
    dev->bar[5] = pci_readl(dev, PCIREG_BAR5);

    /*
     * Log out the BDF notation as well as vendor,
     * and logical slot ID.
     *
     * XXX: The device and vendor id are in the format of
     *      "[V:D]" where 'V' is the vendor ID and 'D' is the
     *      device ID.
     */
    printf(
        "bridge: device [%x:%x] @ pci <%x.%x.%x>\n",
        dev->device, dev->vendor,
        dev->bus, dev->slot,
        dev->func
    );

    /* Allocate a seperate one to queue */
    devp = kalloc(sizeof(*devp));
    if (devp == NULL) {
        printf("pci_register_dev: failed to alloc devp\n");
        return;
    }

    /* Queue a copy */
    memcpy(devp, dev, sizeof(*devp));
    TAILQ_INSERT_TAIL(&devlist, devp, link);
}

/*
 * Enumerate a specifc bus of its devices
 *
 * @bus: Bus to enumerate
 */
static void
pci_enum_bus(uint16_t bus)
{
    pcireg_t reg;
    struct pci_device dev;
    if (bus > 256) {
        printf("pci_enum_bus: bad bus number %x\n", bus);
        return;
    }

    dev.bus = bus;
    for (int slot = 0; slot < 32; ++slot) {
        dev.slot = slot;
        dev.func = 0;
        reg = pci_readl(&dev, PCIREG_HDRTYPE);

        /*
         * Section 6.2.1 of the PCI spec states that some
         * devices may only implement a single function.
         * We can check this with bit 7 of the header type
         * register. If we read a 1, it is a multifunction
         * device.
         */
        if (!ISSET(reg, BIT(7))) {
            pci_register_dev(&dev);
            continue;
        }

        for (uint8_t func = 0; func < 8; ++func) {
            pci_register_dev(&dev);
            ++dev.func;
        }
    }
}

/*
 * Lookup devices on the PCI bus
 */
int
pci_bus_lookup(struct pci_device *lookup, lookup_type_t type)
{
    struct pci_device *dp;
    int cmp = -1;

    if (lookup == NULL) {
        return -EINVAL;
    }

    TAILQ_FOREACH(dp, &devlist, link) {
        switch (type) {
        case PCI_LU_CLASSREV:
            cmp = pci_cs_match(lookup, dp);
            break;
        case PCI_LU_VENDEV:
            cmp = pci_vd_match(lookup, dp);
            break;
        }

        /* Does it match? */
        if (cmp == 0) {
            *lookup = *dp;
            return 0;
        }
    }

    return -ENODEV;
}

/*
 * Read from a specific register
 */
pcireg_t
pci_readl(struct pci_device *dp, pcireg_t reg)
{
    if (dp == NULL) {
        return 0;
    }

    return cam.cam_readl(dp, reg);
}

/*
 * Write to a specific register
 */
void
pci_writel(struct pci_device *dp, pcireg_t reg, uint32_t v)
{
    if (dp == NULL) {
        return;
    }

    return cam.cam_writel(dp, reg, v);
}

/*
 * Advocate for a specific device
 */
int
pci_advoc(struct pci_adv *advp)
{
    struct pci_adv *adv_new;

    if (advp == NULL) {
        return -EINVAL;
    }

    adv_new = kalloc(sizeof(*adv_new));
    if (adv_new == NULL) {
        printf("pci_advoc: could not alloc adv\n");
        return -ENOMEM;
    }
    *adv_new = *advp;
    TAILQ_INSERT_TAIL(&advlist, advp, link);
    return 0;
}

void
pci_init_bus(void)
{
    struct pci_adv *advp;
    struct pci_device dev;
    lookup_type_t lup;
    int error;

    error = pci_cam_init(&cam);
    if (error < 0) {
        printf("pci_init_bus: pci_cam_init() returned %d\n", error);
        panic("pci_init_bus: failed to init CAM\n");
    }

    /* Initialize device and advocation list */
    TAILQ_INIT(&devlist);
    TAILQ_INIT(&advlist);

    printf("pci: enumerating %d buses\n", PCI_MAX_BUS);
    for (int i = 0; i < PCI_MAX_BUS; ++i) {
        pci_enum_bus(i);
    }

    __MODULES_INIT(MODTYPE_PCI);
    printf("bridge: detected %d devices\n", devlist.nelem);

    /* Now allocate load the drivers */
    TAILQ_FOREACH(advp, &advlist, link) {
        dev = advp->lookup;
        lup = advp->idtype;

        error = pci_bus_lookup(&dev, lup);
        if (error == 0) {
            memcpy(&advp->lookup, &dev, sizeof(advp->lookup));
            advp->attach(advp);
        }
    }
}
