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
 * Description: Standard CPU MMU interface
 * Author: Ian Marco Moffett
 */

#ifndef _MACHINE_MMU_H_
#define _MACHINE_MMU_H_

#include <sys/cpuvar.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <vm/vm.h>

/*
 * Standard memory protection flags
 */
#define MMU_PROT_NONE   0x0         /* Nothing */
#define MMU_PROT_READ   PROT_READ   /* Readable */
#define MMU_PROT_WRITE  PROT_WRITE  /* Writable */
#define MMU_PROT_EXEC   PROT_EXEC   /* Executable */

/*
 * This will represent a virtual to
 * physical address mapping.
 *
 * @va: Virtual address
 * @pa: Physical address
 */
struct mmu_map {
    vaddr_t va;
    paddr_t pa;
};

/*
 * Represents the current virtual address
 *
 * @cr3: The value of CR3 for this VAS
 */
struct vm_vas {
    paddr_t cr3;
};

/*
 * Global early kernel VAS structure used in the
 * creation of new virtual address spaces.
 */
extern struct vm_vas g_kvas;

/*
 * Initialize arch-specific MMU state such as
 * page tables, initial mappings and sanity checks.
 *
 * Returns zero on success, otherwise a less than zero
 * value on failure.
 */
int mmu_init(void);

/*
 * Map a single virtual page into physical address
 * space.
 *
 * @spec: Mapping specifier (virtual/physical address)
 * @prot: Protection flags for the mapping (see MMU_PROT_*)
 */
int mmu_map_single(struct vm_vas *vas, struct mmu_map *spec, int prot);

/*
 * Get a pointer to the current virtual address
 * space.
 *
 * @vasres_p: Resulting VAS is written here
 *
 * Returns zero on success, otherwise a less than zero
 * value on failure.
 */
int mmu_this_vas(struct vm_vas *vasres_p);

#endif  /* !_MACHINE_MMU_H_ */
