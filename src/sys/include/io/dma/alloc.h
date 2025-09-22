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

#ifndef _DMA_ALLOC_H_
#define _DMA_ALLOC_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <vm/vm.h>

/*
 * Represents a physical address that may be used
 * with direct memory access
 */
typedef uintptr_t dma_addr_t;

/*
 * Convert virtual DMA pages to physical DMA
 * addresses.
 */
__always_inline static dma_addr_t
dma_get_pa(void *pgbuf)
{
    return VIRT_TO_PHYS(pgbuf);
}

__always_inline static void *
dma_get_va(dma_addr_t pa)
{
    return PHYS_TO_VIRT(pa);
}

/*
 * Allocate a buffer suitable for Direct Memory
 * Access
 *
 * @npages: Number of pages to allocate
 *
 * Returns a pointer to the buffer base on success, otherwise a value
 * of NULL on failure.
 */
void *dma_alloc_pg(size_t npages);

#endif  /* !_DMA_ALLOC_H_ */
