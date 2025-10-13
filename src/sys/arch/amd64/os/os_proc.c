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
#include <vm/physseg.h>
#include <machine/pcb.h>
#include <machine/gdt.h>
#include <machine/frame.h>
#include <machine/lapic.h>
#include <os/kalloc.h>
#include <string.h>

extern struct proc g_rootproc;

/*
 * Put the current process into userland for its first
 * run. The catch is that we have never been in userland
 * from this context so we'll have to fake an IRET frame
 * to force the processor to have a CPL of 3.
 */
__dead void
md_proc_kick(struct proc *procp)
{
    struct md_pcb *pcbp = &procp->pcb;
    struct trapframe *tfp = &pcbp->tf;

    mmu_write_vas(&pcbp->vas);
    lapic_timer_oneshot_us(SCHED_QUANTUM);

    __ASMV(
        "sti\n"
        "mov %0, %%rax\n"
        "mov %1, %%rbp\n"
        "push %2\n"
        "push %3\n"
        "push %4\n"
        "push %%rax\n"
        "push %5\n"
        "lfence\n"
        "swapgs\n"
        "iretq"
        :
        : "r" (tfp->cs),
          "r" (tfp->rbp),
          "i" (USER_DS | 3),
          "r" (tfp->rsp),
          "m" (tfp->rflags),
          "r" (tfp->rip)
    );

    __builtin_unreachable();
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
 * Process idle loop
 */
__dead void
md_proc_yield(void)
{
    struct proc *proc;
    struct pcore *core = this_core();
    int error;

    lapic_eoi();

    /*
     * Set the oneshot timer and check if there is
     * any procs in our runqueues. If not, wait for
     * it to fire again.
     */
    for (;;) {
        lapic_timer_oneshot_us(9000);
        error = sched_deq(&core->scq, &proc);
        if (error == 0) {
            core->curproc = proc;
            md_proc_kick(proc);
        }
        __ASMV("sti; hlt");
    }
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
    return 0;
}

void
md_sched_switch(struct trapframe *tf)
{
    struct proc *self, *proc = NULL;
    struct md_pcb *pcbp;
    struct pcore *core;
    int error;

    if ((core = this_core()) == NULL) {
        printf("sched_switch: could not get core\n");
        goto done;
    }

    /* Don't switch if no self */
    if ((self = core->curproc) == NULL) {
        md_proc_yield();
    }

    error = sched_enq(&core->scq, self);
    if (error < 0) {
        goto done;
    }

    /*
     * Save the current trapframe to our process control
     * block as we'll want it later when we're back.
     */
    pcbp = &self->pcb;
    memcpy(&pcbp->tf, tf, sizeof(*tf));

    /*
     * Grab the next process. If we cannot find any, assume
     * we are the only one and continue on...
     */
    error = sched_deq(&core->scq, &proc);
    if (error < 0) {
        goto done;
    }

    /* Load the next trapframe into the live one */
    pcbp = &proc->pcb;
    memcpy(tf, &pcbp->tf, sizeof(*tf));
    core->curproc = proc;

    /* Switch the address space and hope for the best */
    mmu_write_vas(&pcbp->vas);
done:
    lapic_eoi();
    lapic_timer_oneshot_us(SCHED_QUANTUM);
}

/*
 * Commit procedural murder
 */
int
md_proc_kill(struct proc *procp, int flags)
{
    const size_t PSIZE = DEFAULT_PAGESIZE;
    struct proc *self;
    struct penv_blk *envblk;
    struct pcore *core = this_core();
    struct md_pcb *pcbp;
    struct vm_range *range;

    if (core == NULL) {
        return -ENXIO;
    }

    /* Default to ourself */
    if (procp == NULL) {
        procp = core->curproc;
    }

    /* Free every range */
    TAILQ_FOREACH(range, &procp->maplist, link) {
        if (range == NULL) {
            continue;
        }

        vm_free_frame(range->pa_base, range->len / PSIZE);
    }

    if ((envblk = procp->envblk) != NULL) {
        ptrbox_terminate(procp->envblk_box);
        kfree(envblk->argv);
        procp->envblk = NULL;
    }

    /* Release the VAS */
    pcbp = &procp->pcb;
    mmu_free_vas(&pcbp->vas);

    /* Sanity check */
    if ((self = core->curproc) == NULL) {
        printf("kill: could not get self, using rootproc\n");
        self = &g_rootproc;
    }

    /* If this is us, spin time */
    if (self->pid == procp->pid) {
        core->curproc = NULL;
        md_proc_yield();
    }

    return 0;
}

/*
 * Get the current running process
 */
struct proc *
proc_self(void)
{
    struct pcore *core = this_core();

    if (core == NULL) {
        return NULL;
    }

    return core->curproc;
}
