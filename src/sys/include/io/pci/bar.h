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

#ifndef _PCI_BAR_H_
#define _PCI_BAR_H_ 1

#include <sys/types.h>
#include <sys/cdefs.h>
#include <io/pci/pci.h>
#include <io/pci/cam.h>

/*
 * Convert a BAR number to BAR register offset
 *
 * @bar: Bar number
 *
 * Returns a register offset of the desired BAR on success,
 * otherwise a value of 0 to indicate failure
 */
__always_inline static inline uint8_t
pci_get_barreg(uint8_t bar)
{
    switch (bar) {
    case 0: return PCIREG_BAR0;
    case 1: return PCIREG_BAR1;
    case 2: return PCIREG_BAR2;
    case 3: return PCIREG_BAR3;
    case 4: return PCIREG_BAR4;
    case 5: return PCIREG_BAR5;
    default: return 0;
    }
}

/*
 * Get the number of bytes a BAR region covers
 *
 * @dev: Device of BAR to check
 * @bar: BAR number of BAR to check
 *
 * Returns the number of bytes the BAR region spans on success,
 * otherwise a less than zero value on failure.
 */
ssize_t pci_bar_size(struct pci_device *dev, uint8_t bar);

#endif  /* !_PCI_BAR_H_ */
