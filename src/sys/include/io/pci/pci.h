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
 * @vendor: Vendor ID
 * @device: Device ID
 */
struct pci_device {
    uint16_t bus;
    uint8_t slot;
    uint8_t func;
    uint8_t class;
    uint8_t subclass;
    uint16_t vendor;
    uint16_t device;
    TAILQ_ENTRY(pci_device) link;
};

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
