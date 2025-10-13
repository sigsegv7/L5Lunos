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
#include <sys/syslog.h>
#include <sys/panic.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/cpuvar.h>
#include <os/systm.h>
#include <vm/vm.h>
#include <vm/physseg.h>
#include <os/elfload.h>
#include <os/signal.h>
#include <os/kalloc.h>
#include <os/filedesc.h>
#include <string.h>
#include <stdbool.h>

/*
 * System-wide process state
 *
 * XXX: The process queue here is different from the runqueues,
 *      these keep track of the processes present whether they
 *      are running or not (unless terminated)
 */
static bool is_procq_init = false;
static TAILQ_HEAD(, proc) procq;
static pid_t next_pid = 0;

/*
 * Copy a process environment block from userland
 */
static struct penv_blk *
penv_blk_cpy(struct proc *procp, struct penv_blk *u_blk)
{
    struct penv_blk *blk;
    int error;
    char argbuf[ARG_LEN];
    char **u_argv;
    char *arg;
    uint16_t i;

    if (u_blk == NULL) {
        return NULL;
    }

    /* Allocate a kernel copy */
    blk = kalloc(sizeof(*blk));
    if (blk == NULL) {
        return NULL;
    }

    /* Copy in the user side */
    memset(blk, 0, sizeof(*blk));
    error = copyin(u_blk, blk, sizeof(*blk));
    if (error < 0) {
        printf("penv_blk_cpy: bad u_blk\n");
        kfree(blk);
        return NULL;
    }

    /* Too many args? */
    if (blk->argc > NARG_MAX) {
        printf("penv_blk_cpy: argc > ARG_MAX!!\n");
        kfree(blk);
        return NULL;
    }

    /* Allocate a pointer box for args */
    if (ptrbox_init(&procp->envblk_box) < 0) {
        return NULL;
    }

    /* Allocate a new string store */
    u_argv = blk->argv;
    blk->argv = ptrbox_alloc(
        sizeof(char *) * blk->argc,
        procp->envblk_box
    );
    if (blk->argv == NULL) {
        kfree(blk);
        return NULL;
    }

    /* Dup each arg */
    for (i = 0; i < blk->argc; ++i) {
        error = proc_check_addr(
            procp,
            (uintptr_t)&u_argv[i],
            sizeof(char *)
        );

        /* Is the address valid? */
        if (error != 0) {
            printf("penv_blk_cpy: bad arg pointer (%d)\n", i);
            break;
        }

        error = copyinstr(u_argv[i], argbuf, ARG_LEN);
        if (error < 0) {
            printf("penv_blk_cpy: bad arg pointer (%d)\n", i);
            break;
        }

        blk->argv[i] = ptrbox_strdup(argbuf, procp->envblk_box);
    }

    /* Cleanup on error */
    if (error != 0) {
        ptrbox_terminate(procp->envblk_box);
        kfree(blk);
        blk = NULL;
    }

    return blk;
}

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
 * Put a process to sleep
 */
int
proc_sleep(struct proc *proc)
{
    struct pcore *core = this_core();

    if (proc == NULL || core == NULL) {
        return -EINVAL;
    }

    proc->flags |= PROC_SLEEPING;
    md_proc_sleep();
    return 0;
}

/*
 * Wake up a process
 */
int
proc_wake(struct proc *proc)
{
    struct pcore *core = cpu_sched();

    if (core == NULL || proc == NULL) {
        return -1;
    }

    if (!ISSET(proc->flags, PROC_SLEEPING)) {
        return -1;
    }

    proc->flags &= ~PROC_SLEEPING;
    return 0;
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

    /* Initialize the process queue once */
    if (!is_procq_init) {
        TAILQ_INIT(&procq);
        is_procq_init = true;
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
    if (error != 0) {
        return error;
    }

    return 0;
}

/*
 * Lookup a process by PID
 */
struct proc *
proc_lookup(pid_t pid)
{
    struct proc *curproc;

    TAILQ_FOREACH(curproc, &procq, lup_link) {
        if (curproc == NULL) {
            continue;
        }

        if (curproc->pid == pid) {
            return curproc;
        }
    }

    return NULL;
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
    TAILQ_INSERT_TAIL(&procp->maplist, range, link);
    return 0;
}

/*
 * Kill a specific process
 */
int
proc_kill(struct proc *procp, int status)
{
    struct proc *self = proc_self();

    if (procp == NULL) {
        return -EINVAL;
    }

    /*
     * Try to wake up our parent if they are sleeping
     * at all.
     */
    if (self->pid == procp->pid) {
        if (self->parent != NULL)
            proc_wake(self->parent);
    }

    procp->flags |= PROC_EXITING;
    proc_clear_ranges(procp);
    TAILQ_REMOVE(&procq, procp, lup_link);
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

int
proc_spawn(const char *path, struct penv_blk *envbp)
{
    struct pcore *core;
    struct loaded_elf elf;
    struct proc *proc;
    int error;

    if (path == NULL) {
        return -EINVAL;
    }

    /* Allocate a new process */
    proc = kalloc(sizeof(*proc));
    if (proc == NULL) {
        return -ENOMEM;
    }

    proc_init(proc, 0);
    error = elf_load(path, proc, &elf);
    if (error < 0) {
        kfree(proc);
        return error;
    }

    core = cpu_sched();
    if (__unlikely(core == NULL)) {
        panic("spawn: failed to arbitrate core\n");
    }

    proc->envblk = envbp;
    proc->parent = proc_self();
    md_set_ip(proc, elf.entrypoint);
    sched_enq(&core->scq, proc);

    TAILQ_INSERT_TAIL(&procq, proc, lup_link);
    return proc->pid;
}

/*
 * ARG0: Pathname to spawn
 * ARG1: Process environment block
 */
scret_t
sys_spawn(struct syscall_args *scargs)
{
    const char *u_path = SCARG(scargs, const char *, 0);
    struct penv_blk *u_blk = SCARG(scargs, struct penv_blk *, 1);
    struct penv_blk *envblk;
    char buf[PATH_MAX];
    int error;

    error = copyinstr(u_path, buf, sizeof(buf));
    if (error < 0) {
        return error;
    }

    envblk = penv_blk_cpy(proc_self(), u_blk);
    return proc_spawn(buf, envblk);
}

/*
 * Get argument number number
 *
 * ARG0: Argument number
 * ARG1: Buffer result
 * ARG2: Max length
 */
scret_t
sys_getargv(struct syscall_args *scargs)
{
    uint16_t argno = SCARG(scargs, uint16_t, 0);
    char *u_buf = SCARG(scargs, char *, 1);
    size_t maxlen = SCARG(scargs, size_t, 2);
    struct proc *self = proc_self();
    struct penv_blk *envblk;
    char *arg;

    if (argno >= envblk->argc) {
        return -EINVAL;
    }

    if ((envblk = self->envblk) == NULL) {
        return -EIO;
    }

    arg = envblk->argv[argno];
    return copyoutstr(arg, u_buf, maxlen);
}

/*
 * Wait for a child to complete
 *
 * ARG0: PID
 * ARG1: Status
 * ARG2: Options
 */
scret_t
sys_waitpid(struct syscall_args *scargs)
{
    int pid = SCARG(scargs, int, 0);
    int *u_status = SCARG(scargs, int *, 1);
    int status = 0;
    int error = 0;
    struct proc *proc, *self;

    if ((proc = proc_lookup(pid)) == NULL) {
        return -ESRCH;
    }

    if (u_status != NULL) {
        error = copyout(&status, u_status, sizeof(*u_status));
    }
    if (error < 0) {
        return error;
    }

    self = proc_self();
    proc_sleep(self);
    return 0;
}
