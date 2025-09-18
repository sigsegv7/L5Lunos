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
 * Description: PCI(e) CAM/ECAM driver
 * Author: Ian Marco Moffett
 */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/cdefs.h>
#include <io/pci/cam.h>
#include <io/pci/pci.h>
#include <machine/pio.h>

/*
 * PCI compatibility configuration access
 * method.
 *
 * @PCI_CAM_ADDR: Address window
 * @PCI_CAM_DATA: Data register
 *
 * Writing to the address register results in the host
 * bridge latching the data register until the next write.
 *
 * -- CONFIG ADDRESS FORMAT --
 *
 * bits 1:0   -> zero
 * bits 7:2   -> register offset
 * bits 10:8  -> function
 * bits 15:11 -> device number
 * bits 23:16 -> bus number
 * bits 30:24 -> reserved
 * bit  31    -> ENABLE
 */
#define PCI_CAM_ADDR 0xCF8
#define PCI_CAM_DATA 0xCFC

/*
 * Compute the configuration space address of a specific
 * device at a specific register offset
 *
 * @dev: Device to get address base of
 * @offset: Desired register offset
 *
 * Returns a 32-bit PCI configuration space address
 */
static inline uint32_t
pci_conf_addr(struct pci_device *dev, uint32_t offset)
{
    return BIT(31)             |
           (offset & ~3)       |
           (dev->func << 8)    |
           (dev->slot << 11)   |
           (dev->bus << 16);
}

/*
 * Perform a legacy PCI CAM read which interacts
 * with the lower 256 bytes of the configuration
 * space. Upon receiving the read to the PCI_CAM_ADDR
 * the PCI host bridge latches its PCI_CAM_DATA register
 * at which written data is delegated to the target device.
 *
 * @dp: Target device to read
 * @offset: Register offset to read from
 *
 * Returns a 32-bit value read from the device
 * register.
 */
static pcireg_t
pci_cam_readl(struct pci_device *dp, uint32_t offset)
{
    uint32_t conf_addr;

    if (dp == NULL) {
        return 0;
    }

    /* Latch the data register to this device */
    conf_addr = pci_conf_addr(dp, offset);
    outl(PCI_CAM_ADDR, conf_addr);

    /* Actually read from the device */
    return inl(PCI_CAM_DATA);
}

static void
pci_cam_writel(struct pci_device *dp, uint32_t offset, pcival_t v)
{
    uint32_t conf_addr;

    if (dp == NULL) {
        return;
    }

    /* Latch the data register to this device */
    conf_addr = pci_conf_addr(dp, offset);
    outl(PCI_CAM_ADDR, conf_addr);

    /* Write the data to the target device */
    outl(PCI_CAM_DATA, v);
}

int
pci_cam_init(struct cam_hook *chp)
{
    if (chp == NULL) {
        return -EINVAL;
    }

    chp->cam_writel = pci_cam_writel;
    chp->cam_readl = pci_cam_readl;
    return 0;
}
