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

#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/cdefs.h>
#include <vm/mmu.h>
#include <vm/map.h>
#include <machine/pcb.h>
#include <machine/gdt.h>
#include <machine/frame.h>
#include <string.h>

/*
 * Put the current process into userland for its first
 * run. The catch is that we have never been in userland
 * from this context so we'll have to fake an IRET frame
 * to force the processor to have a CPL of 3.
 *
 * @tfp: Trapframe of context to enter
 */
static inline void
__proc_kick(struct trapframe *tfp)
{
    __ASMV(
        "sti\n"
        "mov %0, %%rax\n"
        "push %1\n"
        "push %2\n"
        "push %3\n"
        "push %%rax\n"
        "push %4\n"
        "lfence\n"
        "swapgs\n"
        "iretq"
        :
        : "r" (tfp->cs),
          "i" (USER_DS | 3),
          "r" (tfp->rsp),
          "m" (tfp->rflags),
          "r" (tfp->rip)
    );
}

/*
 * MD proc init code
 */
int
md_proc_init(struct proc *procp, int flags)
{
    struct md_pcb *pcbp;
    struct trapframe *tfp;
    struct mmu_map spec;
    uint8_t cs, ds;
    int error;

    if (procp == NULL) {
        return -EINVAL;
    }

    pcbp = &procp->pcb;
    if ((error = mmu_new_vas(&pcbp->vas)) < 0) {
        printf("md_proc_init: could not create new vas\n");
        return error;
    }

    ds = USER_DS | 3;
    cs = USER_CS | 3;

    /*
     * Set up the mapping specifier, we'll use zero
     * to allocate new pages.
     */
    spec.pa = 0;
    spec.va = STACK_TOP;

    /* Put the trapframe in a known state */
    tfp = &pcbp->tf;
    memset(tfp, 0, sizeof(*tfp));
    tfp->rflags = 0x202;
    tfp->cs = cs;
    tfp->ss = ds;

    /* Map the stack */
    vm_map(
        &pcbp->vas, &spec,
        STACK_LEN,
        PROT_READ
        | PROT_WRITE
        | PROT_USER
    );

    tfp->rsp = STACK_TOP;
    return 0;
}

/*
 * Set process instruction pointer
 */
int
md_set_ip(struct proc *procp, uintptr_t ip)
{
    struct md_pcb *pcbp;
    struct trapframe *tfp;

    if (procp == NULL) {
        return -EINVAL;
    }

    pcbp = &procp->pcb;
    tfp = &pcbp->tf;
    tfp->rip = ip;
    mmu_write_vas(&procp->pcb.vas);
    __proc_kick(tfp);
    return 0;
}
