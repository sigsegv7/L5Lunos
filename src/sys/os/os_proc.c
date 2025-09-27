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
#include <sys/atomic.h>
#include <sys/errno.h>
#include <sys/cdefs.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <vm/vm.h>
#include <vm/physseg.h>
#include <os/signal.h>
#include <os/kalloc.h>
#include <os/filedesc.h>
#include <string.h>

static pid_t next_pid = 0;

/*
 * Deallocate saved memory ranges
 *
 * @proc: Process to target
 */
static void
proc_clear_ranges(struct proc *proc)
{
    const size_t PSIZE = DEFAULT_PAGESIZE;
    struct vm_range *range;
    size_t n_pages;

    TAILQ_FOREACH(range, &proc->maplist, link) {
        if (range == NULL) {
            continue;
        }

        n_pages = ALIGN_UP(range->len, PSIZE) / PSIZE;
        vm_free_frame(range->pa_base, n_pages);
    }
}

/*
 * MI proc init code
 */
int
proc_init(struct proc *procp, int flags)
{
    struct syscall_domain *scdp;
    struct syscall_win *winp;
    int error;

    if (procp == NULL) {
        return -EINVAL;
    }

    /* Put the process in a known state */
    scdp = &procp->scdom;
    memset(procp, 0, sizeof(*procp));
    TAILQ_INIT(&procp->maplist);

    /*
     * Initialize each platform latch
     */
    for (platch_t p = 0; p < __SC_PLATCH_MAX; ++p) {
        winp = &scdp->slots[p];
        switch (p) {
        case SC_PLATCH_UNIX:
            winp->p = 1;
            winp->sctab = &g_unix_sctab[0];
            winp->nimpl = UNIX_SCTAB_LEN;
            break;
        }
    }

    procp->pid = atomic_inc_int(&next_pid);
    error = md_proc_init(procp, flags);
    if (error < 0) {
        return error;
    }

    error = fdtab_init(procp);
    return error;
}

/*
 * Add range to process
 */
int
proc_add_range(struct proc *procp, vaddr_t va, paddr_t pa, size_t len)
{
    const size_t PSIZE = DEFAULT_PAGESIZE;
    struct vm_range *range;

    if (procp == NULL) {
        return -EINVAL;
    }

    range = kalloc(sizeof(*range));
    if (range == NULL) {
        return -ENOMEM;
    }

    range->pa_base = pa;
    range->va_base = va;
    range->len = ALIGN_UP(len, PSIZE);

    spinlock_acquire(&procp->maplist_lock);
    TAILQ_INSERT_TAIL(&procp->maplist, range, link);
    spinlock_release(&procp->maplist_lock);
    return 0;
}

/*
 * Kill a specific process
 */
int
proc_kill(struct proc *procp, int status)
{
    if (procp == NULL) {
        return -EINVAL;
    }

    procp->flags |= PROC_EXITING;
    proc_clear_ranges(procp);
    return md_proc_kill(procp, 0);
}


/*
 * Check that an address is within the bounds of a
 * process.
 */
int
proc_check_addr(struct proc *proc, uintptr_t addr, size_t len)
{
    uintptr_t stack_base;
    uintptr_t stack_end;

    /* Within the bounds of the stack? */
    stack_base = STACK_TOP - STACK_LEN;
    if (addr >= stack_base && addr <= STACK_TOP) {
        return 0;
    }
    if ((stack_base + len) < stack_end) {
        return 0;
    }

    return -EFAULT;
}
