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

#include <sys/panic.h>
#include <os/kalloc.h>
#include <os/spinlock.h>
#include <vm/tlsf.h>
#include <vm/physseg.h>
#include <vm/vm.h>
#include <stdbool.h>

#define KALLOC_POOL_PAGES     (KALLOC_POOL_SZ / DEFAULT_PAGESIZE)
#define KALLOC_POOL_SZ        0x400000  /* 4 MiB */

void __kalloc_init(void);

static struct spinlock lock;
static tlsf_t tlsf_ctx;
static paddr_t pool = 0;
static void *pool_va = 0;
static bool is_init = false;

/*
 * Memory allocation
 */
void *
kalloc(size_t sz)
{
    void *tmp;

    if (!is_init) {
        return NULL;
    }

    spinlock_acquire(&lock);
    tmp = tlsf_malloc(tlsf_ctx, sz);
    spinlock_release(&lock);
    return tmp;
}

void *
krealloc(void *old_ptr, size_t newsize)
{
    void *tmp;

    spinlock_acquire(&lock);
    tmp = tlsf_realloc(tlsf_ctx, old_ptr, newsize);
    spinlock_release(&lock);
    return tmp;
}

/*
 * Memory deallocation
 */
void
kfree(void *ptr)
{
    spinlock_acquire(&lock);
    tlsf_free(tlsf_ctx, ptr);
    spinlock_release(&lock);
}

void
__kalloc_init(void)
{
    /* Don't do it twice */
    if (is_init) {
        return;
    }

    pool = vm_alloc_frame(KALLOC_POOL_PAGES);
    if (pool == 0) {
        panic("__kalloc_init: could not create pool\n");
    }

    pool_va = PHYS_TO_VIRT(pool);
    tlsf_ctx = tlsf_create_with_pool(pool_va, KALLOC_POOL_SZ);
    is_init = true;
}
