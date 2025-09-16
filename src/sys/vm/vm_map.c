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
#include <sys/errno.h>
#include <sys/syslog.h>
#include <vm/mmu.h>
#include <vm/map.h>
#include <vm/vm.h>

/*
 * Create a virtual to physical memory
 * mapping
 */
static int
__vm_map(struct vm_vas *vas, struct mmu_map *spec, size_t len, int prot)
{
    const size_t PSIZE = DEFAULT_PAGESIZE;
    vaddr_t va_cur, va_end;
    vaddr_t va_start;
    int error;

    if (vas == NULL || spec == NULL) {
        return -EINVAL;
    }

    if (len == 0) {
        return -EINVAL;
    }

    /* Must be 4K aligned */
    len = ALIGN_UP(len, PSIZE);
    spec->va = ALIGN_DOWN(spec->va, PSIZE);
    spec->pa = ALIGN_DOWN(spec->pa, PSIZE);

    /*
     * start: always the mapping base
     * cur: current pointer sliding around
     * end: region end
     */
    va_start = spec->va;
    va_cur = spec->va;
    va_end = spec->va + len;

    while (va_cur < va_end) {
        error = mmu_map_single(vas, spec, prot);
        if (error < 0) {
            return spec->va;
        }

        va_cur += PSIZE;
        spec->va += PSIZE;
        spec->pa += PSIZE;
    }

    return 0;
}

int
vm_map(struct vm_vas *vas, struct mmu_map *spec, size_t len, int prot)
{
    int retval;
    size_t unmap_len;
    struct mmu_map spec_cpy;

    if (spec != NULL) {
        spec_cpy = *spec;
    }

    /* If this fails, unmap the partial region */
    retval = __vm_map(vas, spec, len, prot);
    if (retval != 0) {
        printf("vm_map: could not map <%p>\n", spec_cpy.va);
        unmap_len = retval - spec_cpy.va;

        __vm_map(vas, &spec_cpy, unmap_len, 0);
        return -1;
    }

    return 0;
}
