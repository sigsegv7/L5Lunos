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

#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_

#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/queue.h>
#if defined(_KERNEL)
#include <lib/ptrbox.h>
#include <os/mac.h>
#include <os/signal.h>
#include <os/spinlock.h>
#include <os/filedesc.h>
#include <vm/vm.h>
#include <machine/pcb.h>    /* standard */
#endif  /* _KERNEL */

/*
 * The stack starts here and grows down
 */
#define STACK_TOP   0xBFFFFFFF
#define STACK_LEN   4096

/*
 * Process environment block, used to store arguments
 * and other information.
 *
 * @argv: Argument vector
 * @argc: Argument count
 */
struct penv_blk {
    char **argv;
    uint16_t argc;
};

#if defined(_KERNEL)

/*
 * A process describes a running program image
 * on the system.
 *
 * @pid: Process ID
 * @flags: State flags (see PROC_*)
 * @pcb: Process control block
 * @scdom: Syscall domain
 * @fdtab: File descriptor table
 * @envblk: Environment block
 * @envblk_box: Pointer box for envblk
 * @level: Access level
 * @maplist_lock: Protects the maplist
 * @sigtab: Signal table
 * @maplist: List of mapped regions
 * @link: TAILQ link
 */
struct proc {
    pid_t pid;
    uint32_t flags;
    struct md_pcb pcb;
    struct syscall_domain scdom;
    struct filedesc *fdtab[FD_MAX];
    struct penv_blk *envblk;
    struct ptrbox *envblk_box;
    struct proc *parent;
    mac_level_t level;
    struct spinlock maplist_lock;
    sigtab_t sigtab;
    TAILQ_HEAD(, vm_range) maplist;
    TAILQ_ENTRY(proc) lup_link;
    TAILQ_ENTRY(proc) link;
};

#define PROC_EXITING BIT(0)     /* Process is exiting */
#define PROC_SLEEPING BIT(1)    /* Process is sleeping */
#define PROC_KTD BIT(2)         /* Process is kernel thread */

/* Flags for PROC_SPAWN */
#define SPAWN_KTD BIT(0)        /* Spawn kernel thread */

/*
 * Initialize a process into a basic minimal
 * state
 *
 * @procp: New process data is written here
 * @flags: Optional flags
 *
 * Returns zero on success, otherwise a less than
 * zero value to indicate failure.
 */
int proc_init(struct proc *procp, int flags);

/*
 * Get the current running process on the system
 * Returns NULL if none
 */
struct proc *proc_self(void);

/*
 * Allocate a range descriptor and add it to the
 * process's range tracking list for cleanup upon
 * exit.
 *
 * @procp: Process to initialize the range of
 * @va: Virtual address to use
 * @pa: Physical address to use
 * @len: Length to use
 *
 * Returns zero on success, otherwise a less than
 * zero value upon error.
 */
int proc_add_range(struct proc *procp, vaddr_t va, paddr_t pa, size_t len);

/*
 * Spawn a kernel thread
 *
 * @procp_res: Result is written here
 * @fn: Function where kernel thread should end up
 */
int proc_ktd(struct proc **procp_res, void(*fn)(void *));

/*
 * Kill a process with a specific status code
 *
 * @procp: Process to kill
 * @status: Status code to send
 *
 * Returns zero on success, otherwise a less than
 * zero value is returned on failure.
 */
int proc_kill(struct proc *procp, int status);

/*
 * Spawn a process from a binary
 *
 * @path: Path to binary
 * @envp: Environment block pointer
 *
 * Returns the PID of the new process on success,
 * otherwise a less than zero value on error
 */
int proc_spawn(const char *path, struct penv_blk *envbp);

/*
 * Initialize machine dependent state of a process
 *
 * @procp: New process data is written here
 * @flags: Optional flags
 *
 * Returns zero on success, otherwise a less than
 * zero value to indicate failure.
 */
int md_proc_init(struct proc *procp, int flags);

/*
 * Machine dependent kill routine which cleans up
 * things that exist within the process control block
 *
 * @procp: Process to kill
 * @flags: Optional flags to use
 *
 * Returns zero on success, otherwise less than zero
 * value on failure.
 */
int md_proc_kill(struct proc *procp, int flags);

/*
 * Set the instruction pointer field of a specific
 * process
 *
 * @procp: Process to update
 * @ip: Instruction pointer to write
 *
 * Returns zero on success, otherwise a less than
 * zero value to indicate failure.
 */
int md_set_ip(struct proc *procp, uintptr_t ip);

/*
 * Check that a virtual address is within the bounds of
 * a process.
 *
 * @proc: Process the address should be within
 * @addr: Virtual address to check
 * @len: Length of memory referenced by 'addr'
 *
 * Returns zero if the address is within the process bounds,
 * otherwise a less than zero value on failure.
 */
int proc_check_addr(struct proc *proc, uintptr_t addr, size_t len);

/*
 * Put a process to sleep
 *
 * @proc: Process to put to sleep
 *
 * Returns zero on success
 */
int proc_sleep(struct proc *proc);

/*
 * Wake up a process
 *
 * @proc: Process to wake up
 *
 * Returns zero if the address is within the process bounds,
 * otherwise a less than zero value on failure.
 */
int proc_wake(struct proc *proc);

/*
 * Lookup a process using its PID
 *
 * @pid: PID of process to lookup
 *
 * Returns NULL on failure
 */
struct proc *proc_lookup(pid_t pid);

/*
 * Put the current process to sleep until woken
 * up
 */
void md_proc_sleep(void);

/*
 * Put the current process into a halt loop
 * until the next one runs.
 */
__dead void md_proc_idle(void);

/*
 * Kick a process into a user context
 *
 * @procp: Process pointer
 */
__dead void md_proc_kick(struct proc *procp);

/*
 * Spawn a new process
 */
scret_t sys_spawn(struct syscall_args *scargs);

/*
 * Get argument number n
 */
scret_t sys_getargv(struct syscall_args *scargs);

/*
 * Wait for a child to complete
 */
scret_t sys_waitpid(struct syscall_args *scargs);

#endif  /* !_KERNEL */
#endif  /* !_SYS_PROC_H_ */
