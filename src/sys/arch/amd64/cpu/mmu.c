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
#include <sys/cdefs.h>
#include <sys/panic.h>
#include <machine/mmu.h>
#include <vm/vm.h>
#include <vm/physseg.h>
#include <string.h>
#include <stdbool.h>

/*
 * Page-Table Entry (PTE) flags
 *
 * See Intel SDM Vol 3A, Section 4.5, Table 4-19
 */
#define PTE_ADDR_MASK   0x000FFFFFFFFFF000
#define PTE_P           BIT(0)        /* Present */
#define PTE_RW          BIT(1)        /* Writable */
#define PTE_US          BIT(2)        /* User r/w allowed */
#define PTE_PWT         BIT(3)        /* Page-level write-through */
#define PTE_PCD         BIT(4)        /* Page-level cache disable */
#define PTE_ACC         BIT(5)        /* Accessed */
#define PTE_DIRTY       BIT(6)        /* Dirty (written-to page) */
#define PTE_PS          BIT(7)        /* Page size */
#define PTE_GLOBAL      BIT(8)        /* Global / sticky map */
#define PTE_NX          BIT(63)       /* Execute-disable */

/*
 * Used to enable/disable 57-bit paging which expands
 * the paging levels to MMU_L5
 */
#define CR4_L5_PAGING  BIT(12)

/*
 * Describes each paging level
 *
 * @MMU_OFF: Offset
 * @MMU_TBL: Page table
 * @MMU_L<n>: Page table level n
 */
typedef enum {
    MMU_OFF,
    MMU_TBL,
    MMU_L2,
    MMU_L3,
    MMU_L4,
    MMU_L5
} pglvl_t;

/*
 * Convert machine independent protection flags
 * to machine dependent flags.
 *
 * @prot: MI flags
 */
static uint64_t
prot_to_pte(int prot)
{
    uint64_t pte_flags = PTE_NX;

    if (ISSET(prot, MMU_PROT_READ))
        pte_flags |= PTE_P;
    if (ISSET(prot, MMU_PROT_WRITE))
        pte_flags |= PTE_RW;
    if (ISSET(prot, MMU_PROT_EXEC))
        pte_flags &= ~PTE_NX;

    return pte_flags;
}

/*
 * Invalidate a page in the TLB. We use this to prevent
 * stale entries when remapping or changing attributes
 * of pages.
 *
 * @ptr: Page base to invalidate from the TLB
 */
static inline void
__invlpg(void *ptr)
{
    uintptr_t v = (uintptr_t)ptr;

    v = ALIGN_UP(v, DEFAULT_PAGESIZE);
    __ASMV(
        "invlpg (%0)"
        :
        : "r" (v)
        : "memory"
    );
}

/*
 * Get the current value of CR3 which holds
 * the physical address of the current virtual
 * address space.
 */
static inline uint64_t
__mmu_read_cr3(void)
{
    uint64_t cr3;

    __ASMV(
        "mov %%cr3, %0"
        : "=r" (cr3)
        :
        : "memory"
    );

    return cr3;
}

/*
 * Acquire the paging level used by the
 * current processing element (pcore)
 */
static inline pglvl_t
mmu_pg_level(void)
{
    uint64_t cr4;

    __ASMV(
        "mov %%cr4, %0"
        : "=r" (cr4)
        :
        : "memory"
    );

    if (ISSET(cr4, CR4_L5_PAGING)) {
        return MMU_L5;
    }

    return MMU_L4;
}

/*
 * Get the table index of a specific level by
 * using a specific virtual address as a key.
 *
 * @vaddr: Virtual address to use in lookup
 * @level: The index of the desired level
 */
static inline size_t
mmu_get_level(vaddr_t vaddr, pglvl_t level)
{
    switch (level) {
    case MMU_L5:
        return (vaddr >> 48) & 0x1FF;
    case MMU_L4:
        return (vaddr >> 39) & 0x1FF;
    case MMU_L3:
        return (vaddr >> 30) & 0x1FF;
    case MMU_L2:
        return (vaddr >> 21) & 0x1FF;
    case MMU_TBL:
        return (vaddr >> 12) & 0x1FF;
    case MMU_OFF:
        return vaddr & 0x1FF;
    }

    panic("mmu_get_level: bad level index\n");
    __builtin_unreachable();
}

/*
 * Get the table at the desired level
 *
 * @vas: Virtual address space
 * @va: Virtual address to use as key
 * @lvl: Desired level
 * @res: Virtual address result is written here
 * @a: If true, allocate memory for unmapped entries
 *
 * Returns zero on success, otherwise a less than
 * zero value on failure.
 */
static int
mmu_read_level(struct vm_vas *vas, vaddr_t va, pglvl_t lvl, vaddr_t **res, bool a)
{
    uintptr_t *cur, tmp_va, addr;
    size_t index;
    pglvl_t cur_level = MMU_L4;

    if (vas == NULL || lvl > MMU_L5) {
        return -EINVAL;
    }

    if (res == NULL) {
        return -EINVAL;
    }

    /*
     * We'll do a recursive descent style algorithm
     * to get the page table that we want. Keep going
     * down levels [lvl, MMU_TBL) until we hit the
     * bottom.
     */
    cur = PHYS_TO_VIRT(vas->cr3 & PTE_ADDR_MASK);
    while (cur_level > lvl) {
        index = mmu_get_level(va, cur_level);
        addr = cur[index];

        /* Is this present? */
        if (ISSET(addr, PTE_P)) {
            addr = cur[index] & PTE_ADDR_MASK;
            cur = PHYS_TO_VIRT(addr);
            --cur_level;
            continue;
        }

        /* If we can't alloc, bail */
        if (!a) {
            return -EPIPE;
        }

        /* Allocate new frame */
        addr = vm_alloc_frame(1);
        if (__unlikely(addr == 0)) {
            panic("mmu_read_level: out of memory\n");
        }

        /* Write the new entry */
        addr |= (PTE_P | PTE_RW | PTE_US);
        addr = (uintptr_t)PHYS_TO_VIRT(addr);
        cur[index] = addr;

        /*
         * To be certain that we will see every change
         * per every level, we must invalidate its
         * corresponding entry.
         */
        __invlpg(cur);
        --cur_level;
    }

    *res = cur;
    return 0;
}

/*
 * Read the current VAS into 'vasres_p'
 */
int
mmu_this_vas(struct vm_vas *vasres_p)
{
    if (vasres_p == NULL) {
        return -EINVAL;
    }

    vasres_p->cr3 = __mmu_read_cr3();
    return 0;
}

/*
 * Create a virtual to physical mapping
 */
int
mmu_map_single(struct vm_vas *vas, struct mmu_map *spec, int prot)
{
    int error;
    size_t index;
    uint64_t pte_flags;
    vaddr_t *pte;

    if (spec == NULL) {
        return -EINVAL;
    }

    /*
     * First things first, we need to translate these
     * architecture abstracting protection flags to
     * something these Intel MMUs will understand. We
     * just need to start at the PML4, hit the bottom
     * and plop em there.
     */
    pte_flags = prot_to_pte(prot);
    error = mmu_read_level(
        vas, spec->va, MMU_TBL,
        &pte, true
    );

    /* Did this fail? */
    if (error < 0) {
        return error;
    }

    /*
     * Now using the virtual address within the map spec,
     * we'll acquire the index at which we put the physical
     * address, along with its flags. Then of course, flush the
     * TLB entry.
     */
    index = mmu_get_level(spec->va, MMU_TBL);
    pte[index] = pte_flags | spec->pa;
    __invlpg((void *)spec->va);
    return 0;
}

/*
 * Verify that we are in a known state
 */
int
mmu_init(void)
{
    struct pcore *self = this_core();
    struct mdcore *core;

    if (self == NULL) {
        panic("mmu_init: could not get core\n");
    }

    core = &self->md;

    /*
     * It would be foolish to assume the state of the
     * processor we are handed over with. Check first,
     * cry later.
     */
    if (mmu_pg_level() != MMU_L4) {
        panic("mmu_init: processor not using L4 paging\n");
    }

    core->cr3 = __mmu_read_cr3();
    g_kvas.cr3 = core->cr3;
    return 0;
}
