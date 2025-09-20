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
 * Description: PCI base address register helpers
 * Author: Ian Marco Moffett
 */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/panic.h>
#include <io/pci/bar.h>

/*
 * Get the size of a BAR
 */
ssize_t
pci_bar_size(struct pci_device *dev, uint8_t bar)
{
    uint32_t reg_tmp, reg;
    uint8_t barreg = pci_get_barreg(bar);

    if (dev == NULL) {
        return -EINVAL;
    }

    /*
     * A trick we can do to find the length of the BAR defined
     * by the device is to see how many bits it is capable of
     * decoding for a BAR then invert the mask as log2(length)
     * bits must be unset.
     */
    reg_tmp = pci_readl(dev, barreg);
    pci_writel(dev, barreg, 0xFFFFFFFF);
    reg = pci_readl(dev, barreg);
    reg = ~reg + 1;

    /* Restore and return the length */
    pci_writel(dev, barreg, reg_tmp);
    return reg;
}
