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

#ifndef _PCI_PCI_H_
#define _PCI_PCI_H_ 1

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/cdefs.h>

/*
 * Use for making instances of the pci_adv
 * structure.
 */
#define PCI_CS_ID(CLASS, SUBCLASS)  \
    {                               \
        .class = (CLASS),           \
        .subclass = (SUBCLASS)      \
    }
#define PCI_DV_ID(DEVICE, VENDOR)   \
    {                               \
        .device = (DEVICE),         \
        .vendor = (VENDOR)          \
    }

/*
 * Use for making instances of the pci_adv
 * structure while specifying the programming
 * interface
 */
#define PCI_CSI_ID(CLASS, SUBCLASS, IF)     \
    {                                       \
        .class = (CLASS),                   \
        .subclass = (SUBCLASS),             \
        .prog_if = (IF)                     \
    }
#define PCI_DVI_ID(DEVICE, VENDOR, IF)      \
    {                                       \
        .device = (DEVICE),                 \
        .vendor = (VENDOR),                 \
        .prog_if = (IF)                     \
    }

/* PCI specific types */
typedef uint32_t pcireg_t;
typedef uint32_t pcival_t;

/*
 * Represents a device attached to the PCI
 * bus
 *
 * @bus: Bus number of device
 * @slot: Slot number of device
 * @func: Function number of device
 * @prog_if: Programming interface
 * @vendor: Vendor ID
 * @device: Device ID
 * @bar: Base address registers
 */
struct pci_device {
    uint16_t bus;
    uint8_t slot;
    uint8_t func;
    uint8_t prog_if;
    uint8_t class;
    uint8_t subclass;
    uint16_t vendor;
    uint16_t device;
    uint32_t bar[6];
    TAILQ_ENTRY(pci_device) link;
};

typedef enum {
    PCI_LU_VENDEV,      /* Vendor / device */
    PCI_LU_CLASSREV,    /* Class / subclass */
    PCI_LU_IVENDEV,     /* Interface + vendor / device */
    PCI_LU_ICLASSREV,   /* Interface + class / revision */
} lookup_type_t;

/*
 * Structure that allows a device driver of a PCI
 * bus node to advocate for its workings. In other words,
 * it registers itself to be the driver of the device.
 *
 * @lookup: Lookup arguments
 * @attach: Attach the driver
 * @idtype: How the device will be identified
 *
 * XXX: The `lookup` field is used for both input arguments
 *      as well as output results
 */
struct pci_adv {
    struct pci_device lookup;
    int(*attach)(struct pci_adv *ap);
    lookup_type_t idtype;
    TAILQ_ENTRY(pci_adv) link;
};

/*
 * Lookup a device on the PCI(e) bus by using the pci_descriptor
 * as a lookup key.
 *
 * @lookup: Lookup descriptor that must match a device
 * @type:  Lookup type
 *
 * XXX: The result is written back to 'lookup'
 *
 * Returns zero on success, otherwise a less than zero value
 * on failure.
 */
int pci_bus_lookup(struct pci_device *lookup, lookup_type_t type);

/*
 * Advocate for a specific device as its driver.
 *
 * @advp: Advocation descriptor
 *
 * Returns zero on success, otherwise a less than
 * zero value on failure.
 */
int pci_advoc(struct pci_adv *advp);

/*
 * Read from a specific register on a specific PCI
 * enabled device.
 *
 * @dp: Device to read from
 * @reg: Offset of desired register
 *
 * Returns the 32-bit register value on success
 */
pcireg_t pci_readl(struct pci_device *dp, pcireg_t reg);

/*
 * Write a value to a specific register on a specific
 * PCI enabled device.
 *
 * @dp: Device to write to
 * @reg: Offset of the desired register
 * @v: 32-bit value to be written
 */
void pci_writel(struct pci_device *dp, pcireg_t reg, pcival_t v);

/*
 * Initialize the root bus and enumerate attached
 * devices.
 */
void pci_init_bus(void);

#endif  /* !PCI_PCI_H_ */
