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

#ifndef _VM_H_
#define _VM_H_ 1

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/bootvars.h>

#define VM_HIGHER_HALF (get_kernel_base())
#define PHYS_TO_VIRT(PHYS) (void *)((uintptr_t)(PHYS) + VM_HIGHER_HALF)
#define VIRT_TO_PHYS(VIRT) ((uintptr_t)(VIRT) - VM_HIGHER_HALF)
#define DEFAULT_PAGESIZE 4096

/* Physical/virtual address */
typedef uintptr_t vaddr_t;
typedef uintptr_t paddr_t;

/*
 * Describes a virtual memory range
 *
 * @pa_base: Physical memory base
 * @va_base: Virtual memory base
 * @len: Length of region
 * @link: Queue link
 */
struct vm_range {
    paddr_t pa_base;
    vaddr_t va_base;
    size_t len;
    TAILQ_ENTRY(vm_range) link;
};

void vm_init(void);

#endif  /* !_VM_H_ */
