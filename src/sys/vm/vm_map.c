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
#include <sys/param.h>
#include <sys/syslog.h>
#include <vm/physseg.h>
#include <vm/mmu.h>
#include <vm/map.h>
#include <vm/vm.h>

#define MMAP_START 0x6F3C8E0C0000
#define MMAP_END   0x6F3C90000000

/*
 * Create a virtual to physical memory
 * mapping
 */
static int
__vm_map(struct vm_vas *vas, struct mmu_map *spec, size_t len, int prot)
{
    const size_t PSIZE = DEFAULT_PAGESIZE;
    struct mmu_map tmp_spec;
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

    /*
     * If we encounter any address that is zero, we
     * must assign our own.
     */
    if (spec->pa == 0 || spec->va == 0) {
        spec->pa = vm_alloc_frame(len / PSIZE);
        if (spec->va == 0)
            spec->va = spec->pa;
    }

    if (spec->pa == 0) {
        return -ENOMEM;
    }

    /* Must be on a 4K boundary */
    tmp_spec.va = ALIGN_DOWN(spec->va, PSIZE);
    tmp_spec.pa = ALIGN_DOWN(spec->pa, PSIZE);

    /*
     * start: always the mapping base
     * cur: current pointer sliding around
     * end: region end
     */
    va_start = spec->va;
    va_cur = spec->va;
    va_end = spec->va + len;

    while (va_cur < va_end) {
        error = mmu_map_single(vas, &tmp_spec, prot);
        if (error < 0) {
            return spec->va;
        }

        va_cur += PSIZE;
        tmp_spec.va += PSIZE;
        tmp_spec.pa += PSIZE;
    }

    return 0;
}

int
vm_map(struct vm_vas *vas, struct mmu_map *spec, size_t len, int prot)
{
    const size_t PSIZE = DEFAULT_PAGESIZE;
    int retval;
    size_t unmap_len;
    struct mmu_map spec_cpy;
    struct proc *self = proc_self();

    if (spec != NULL) {
        spec_cpy = *spec;
    }

    /* If this fails, unmap the partial region */
    len = ALIGN_UP(len, PSIZE);
    retval = __vm_map(vas, spec, len, prot);
    if (retval != 0) {
        printf("vm_map: could not map <%p>\n", spec_cpy.va);
        unmap_len = retval - spec_cpy.va;

        __vm_map(vas, &spec_cpy, unmap_len, 0);
        return -1;
    }

    /* Add the range if we can */
    if (self != NULL) {
        spinlock_acquire(&self->maplist_lock);
        proc_add_range(self, spec->va, spec->pa, len);
        spinlock_release(&self->maplist_lock);
    }

    /* Place a guard page at the end */
    spec_cpy.va = spec_cpy.va + len;
    __vm_map(vas, &spec_cpy, DEFAULT_PAGESIZE, 0);
    return 0;
}

void *
mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
    struct mmu_map spec;
    struct vm_vas vas;
    paddr_t pa;
    int error;

    /* TODO: Support address allocation  */
    if (len == 0 || addr == NULL) {
        return NULL;
    }

    spec.va = (uintptr_t)addr;
    spec.pa = 0;
    error = mmu_this_vas(&vas);
    if (error < 0) {
        return NULL;
    }

    /* Create the mapping */
    error = vm_map(&vas, &spec, len, prot);
    if (error < 0) {
        return NULL;
    }

    return (void *)spec.va;
}

/*
 * ARG0: Address
 * ARG1: Length
 * ARG2: Protection flags
 * ARG3: Flags
 * ARG4: Fildes
 * ARG5: Offset
 */
scret_t
sys_mmap(struct syscall_args *scargs)
{
    void *address = SCARG(scargs, void *, 0);
    size_t length = SCARG(scargs, size_t, 1);
    int prot = SCARG(scargs, int, 2);

    prot |= PROT_USER;
    if (address != NULL) {
        address = PTR_OFFSET(address, MMAP_START);
    }

    /* XXX: Should use rest of the args */
    return (uintptr_t)mmap(address, length, prot, 0, 0, 0);
}
