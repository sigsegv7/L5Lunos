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
#include <sys/cdefs.h>
#include <sys/panic.h>
#include <machine/mmu.h>

/*
 * Used to enable/disable 57-bit paging which expands
 * the paging levels to MMU_L5
 */
#define CR4_L5_PAGING  BIT(12)

/*
 * Describes each paging level
 *
 * @MMU_PAGE: Page table
 * @MMU_L<n>: Page table level n
 */
typedef enum {
    MMU_PAGE,
    MMU_L2,
    MMU_L3,
    MMU_L4,
    MMU_L5
} pglvl_t;

/*
 * Acquire the paging level used by the
 * current processing element (pcore)
 */
static inline pglvl_t
mmu_pg_level(void)
{
    uint64_t cr0;

    __ASMV(
        "mov %%cr0, %0"
        :
        : "r" (cr0)
        : "memory"
    );

    if (ISSET(cr0, CR4_L5_PAGING)) {
        return MMU_L5;
    }

    return MMU_L4;
}

/*
 * Verify that we are in a known state
 */
int
mmu_init(void)
{
    /*
     * It would be foolish to assume the state of the
     * processor we are handed over with. Check first,
     * cry later.
     */
    if (mmu_pg_level() != MMU_L4) {
        panic("mmu_init: processor not using L4 paging\n");
    }
    return 0;
}
