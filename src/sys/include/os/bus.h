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

#ifndef _OS_BUS_H_
#define _OS_BUS_H_

#include <sys/types.h>

typedef enum {
    BUS_PCI_NONE,
    BUS_PCI_PCI,
} bus_type_t;

/*
 * Represents a physical or virtual MMIO address that
 * allows tranmission and reception on the bus.
 */
typedef uintptr_t bus_addr_t;

/*
 * Represents a bus space that can be used with certain
 * bus interfaces
 *
 * @va_base: Virtual address base
 * @length: Length at buffer
 * @type: Type of the space space (see BUS_*)
 */
struct bus_space {
    void *va_base;
    size_t length;
    bus_type_t type;
};

/*
 * Initialize a bus space descriptor and map a physical
 * bus address.
 *
 * @bp: Bus space descriptor to use
 * @pa: Physical bus address to map.
 * @len: Length of mapping to use
 *
 * Returns zero on success, otherwise a less than zero
 * value.
 */
int bus_space_map(struct bus_space *bp, bus_addr_t pa, size_t len);

#endif  /* !_OS_BUS_H_ */
