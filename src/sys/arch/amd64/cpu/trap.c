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
 * Description: AMD64 trap handling module
 * Author: Ian Marco Moffett
 */

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/panic.h>
#include <sys/cpuvar.h>
#include <sys/syslog.h>
#include <sys/syscall.h>
#include <machine/trap.h>

/*
 * Trap type to type string conversion table
 * Indexed by trapno field in trapframe.
 */
static const char *trapstr[] = {
    [TRAP_NONE]         = "bad",
    [TRAP_BREAKPOINT]   = "breakpoint",
    [TRAP_ARITH_ERR]    = "arithmetic error",
    [TRAP_OVERFLOW]     = "overflow",
    [TRAP_BOUND_RANGE]  = "bound range exceeded",
    [TRAP_INVLOP]       = "invalid opcode",
    [TRAP_DOUBLE_FAULT] = "double fault",
    [TRAP_INVLTSS]      = "invalid TSS",
    [TRAP_SEGNP]        = "segment not present",
    [TRAP_PROTFLT]      = "general protection",
    [TRAP_PAGEFLT]      = "page fault",
    [TRAP_NMI]          = "non-maskable interrupt",
    [TRAP_SS]           = "stack-segment fault"
};

/*
 * Page fault flags (bit relative)
 */
static const char pf_flags[] = {
    'p',    /* Present */
    'w',    /* Write */
    'u',    /* User */
    'r',    /* Reserved write */
    'x',    /* Instruction fetch */
    'k',    /* Protection key violation */
    's'     /* Shadow stack access */
};

/*
 * Return the current value of the page fault address
 * register (CR2)
 */
static inline uintptr_t
pf_faultaddr(void)
{
    uintptr_t cr2;

    __ASMV(
        "mov %%cr2, %0\n"
        : "=r" (cr2)
        :
        : "memory"
    );
    return cr2;
}

/*
 * Log the error code for page faults
 *
 * @error_code: Page fault error code to log
 */
static void
pf_code(uint64_t error_code)
{
    char tab[8] = {
        '-', '-', '-',
        '-', '-', '-',
        '-', '\0'
    };

    for (int i = 0; i < 7; ++i) {
        if (ISSET(error_code, BIT(i))) {
            tab[i] = pf_flags[i];
        }
    }

    printf("code=[%s]\n", tab);
}

/*
 * Dump a trapframe
 *
 * @tf: Trapframe to dump
 */
static inline void
trapframe_dump(struct trapframe *tf)
{
    uintptr_t cr3, cr2 = pf_faultaddr();

    __ASMV("mov %%cr3, %0\n"
           : "=r" (cr3)
           :
           : "memory"
    );

    if (tf->trapno == TRAP_PAGEFLT) {
        pf_code(tf->error_code);
    }

    printf("got trap (%s)\n\n"
        "-- DUMPING PROCESSOR STATE --\n"
        "RAX=%p RCX=%p RDX=%p\n"
        "RBX=%p RSI=%p RDI=%p\n"
        "RFL=%p CR2=%p CR3=%p\n"
        "RBP=%p RSP=%p RIP=%p\n\n",
        trapstr[tf->trapno],
        tf->rax, tf->rcx, tf->rdx,
        tf->rbx, tf->rsi, tf->rdi,
        tf->rflags, cr2, cr3,
        tf->rbp, tf->rsp, tf->rip);
}

void
trap_syscall(struct trapframe *tf)
{
    struct pcore *pcore = this_core();
    struct syscall_domain *scdp;
    struct syscall_win *scwp;
    struct proc *self;
    struct syscall_args scargs = {
        .arg[0] = tf->rdi,
        .arg[1] = tf->rsi,
        .arg[2] = tf->rdx,
        .arg[3] = tf->r10,
        .arg[4] = tf->r9,
        .arg[5] = tf->r8,
        .tf = tf
    };

    /* Sanity check */
    if (__unlikely(pcore == NULL)) {
        printf("trap_syscall: pcore is NULL\n");
        return;
    }

    /* Get the current window */
    self = pcore->curproc;
    scdp = &self->scdom;
    scwp = &scdp->slots[scdp->platch];
    if (scwp->sctab == NULL && scwp->p == 0) {
        printf("trap_syscall: no sctab (platch=%x)\n", scdp->platch);
        return;
    }

    if (tf->rax < scwp->nimpl && tf->rax > 0) {
        tf->rax = scwp->sctab[tf->rax](&scargs);
    }
}

void
trap_handler(struct trapframe *tf)
{
    trapframe_dump(tf);
    if (ISSET(tf->cs, 3)) {
        panic("fatal user trap\n");
    }

    panic("fatal trap\n");
}
