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
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/panic.h>
#include <sys/cdefs.h>
#include <sys/cpuvar.h>
#include <vm/physseg.h>
#include <vm/vm.h>
#include <machine/tss.h>
#include <machine/mdcpu.h>
#include <string.h>

/*
 * Allocates memory for TSS and kernel
 * stack.
 *
 * XXX: Kernel stack is allocated from
 *      vm_alloc_frame()
 */
static void
alloc_resources(struct mdcore *mdcore)
{
    const size_t STACK_SIZE = 0x1000;
    size_t tss_pages = sizeof(struct tss_entry);
    static uintptr_t rsp0_base, rsp0;
    struct tss_entry *tss = &mdcore->tss;

    tss_pages = ALIGN_UP(tss_pages, DEFAULT_PAGESIZE) / DEFAULT_PAGESIZE;
    memset(tss, 0, sizeof(*tss));
    rsp0_base = vm_alloc_frame(tss_pages) + VM_HIGHER_HALF;

    if (rsp0_base == 0) {
        panic("could not allocate RSP0 base\n");
    }

    rsp0 = rsp0_base + STACK_SIZE;
    tss->rsp0_lo = rsp0 & 0xFFFFFFFF;
    tss->rsp0_hi = (rsp0 >> 32) & 0xFFFFFFFF;
}

int
tss_alloc_stack(union tss_stack *entry_out, size_t size)
{
    uintptr_t base;

    base = vm_alloc_frame(1);
    if (base == 0) {
        panic("tss_alloc_stack: failed to allocate stack\n");
    }

    base = (uintptr_t)PHYS_TO_VIRT(base);
    entry_out->top = base + size;
    return 0;
}

int
tss_update_ist(struct pcore *pcore, union tss_stack stack, uint8_t istno)
{
    struct mdcore *mdcore = &pcore->md;
    struct tss_entry *tss = &mdcore->tss;

    switch (istno) {
    case 1:
        tss->ist1_lo = stack.top_lo;
        tss->ist1_hi = stack.top_hi;
        break;
    case 2:
        tss->ist2_lo = stack.top_lo;
        tss->ist2_hi = stack.top_hi;
        break;
    case 3:
        tss->ist3_lo = stack.top_lo;
        tss->ist3_hi = stack.top_hi;
        break;
    case 4:
        tss->ist4_lo = stack.top_lo;
        tss->ist4_hi = stack.top_hi;
        break;
    case 5:
        tss->ist5_lo = stack.top_lo;
        tss->ist5_hi = stack.top_hi;
        break;
    case 6:
        tss->ist6_lo = stack.top_lo;
        tss->ist6_hi = stack.top_hi;
        break;
    case 7:
        tss->ist7_lo = stack.top_lo;
        tss->ist7_hi = stack.top_hi;
        break;
    default:
        return -EINVAL;
    };

    return 0;
}

void
write_tss(struct pcore *pcore, struct tss_desc *desc)
{
    volatile struct tss_entry *tss;
    struct mdcore *mdcore = &pcore->md;
    uintptr_t tss_base;
    uint8_t io_base;

    alloc_resources(mdcore);
    tss_base = (uintptr_t)&mdcore->tss;

    desc->seglimit = sizeof(struct tss_entry) - 1;
    desc->p = 1;        /* Must be present to be valid! */
    desc->g = 0;        /* Granularity -> 0 */
    desc->avl = 0;      /* Not used */
    desc->dpl = 0;      /* Descriptor Privilege Level -> 0 */
    desc->type = 0x9;   /* For TSS -> 0x9 (0b1001) */

    desc->base_lo16 = tss_base & 0xFFFF;
    desc->base_mid8 = (tss_base >> 16) & 0xFF;
    desc->base_hi_mid8 = (tss_base >> 24) & 0xFF;
    desc->base_hi32 = (tss_base >> 32) & 0xFFFFFFFF;

    /*
     * XXX: By default, each process is not allowed to have any I/O
     *      port access. This should be configurable for certain servers
     *      running in CPL 3.
     */
    tss = &mdcore->tss;
    memset((void *)tss->iomap, 0xFF, sizeof(tss->iomap));
    tss->io_base = offsetof(struct tss_entry, iomap);
}
