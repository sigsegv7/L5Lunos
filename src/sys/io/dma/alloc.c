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
#include <sys/param.h>
#include <sys/syslog.h>
#include <io/dma/alloc.h>
#include <vm/physseg.h>
#include <vm/mmu.h>
#include <vm/map.h>
#include <vm/vm.h>
#include <string.h>

/*
 * Allocate DMA pages
 */
void *
dma_alloc_pg(size_t npages)
{
    const size_t PSIZE = DEFAULT_PAGESIZE;
    struct vm_vas vas;
    struct mmu_map spec;
    size_t length;
    int error, prot;
    void *buf = NULL;
    paddr_t pa;

    if ((pa = vm_alloc_frame(npages)) == 0) {
        return NULL;
    }

    spec.pa = pa;
    spec.va = pa;
    length = DEFAULT_PAGESIZE * npages;

    /* Try to get the current VAS */
    error = mmu_this_vas(&vas);
    if (error < 0) {
        goto done;
    }

    /* Map the region */
    prot = PROT_READ | PROT_WRITE;
    error = vm_map(&vas, &spec, length, prot);
    if (error < 0) {
        goto done;
    }

    buf = (void *)spec.va;
    memset(buf, 0, npages * PSIZE);
done:
    return buf;
}
