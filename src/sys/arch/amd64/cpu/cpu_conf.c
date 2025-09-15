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

#include <sys/cpuvar.h>
#include <machine/boot.h>
#include <machine/msr.h>
#include <machine/idt.h>
#include <machine/trap.h>
#include <machine/lapic.h>

/*
 * Initialize interrupt vectors
 */
static void
init_vectors(void)
{
    idt_set_desc(0x0, IDT_TRAP_GATE, ISR(arith_err), 0);
    idt_set_desc(0x2, IDT_TRAP_GATE, ISR(nmi), 0);
    idt_set_desc(0x3, IDT_TRAP_GATE, ISR(breakpoint_handler), 0);
    idt_set_desc(0x4, IDT_TRAP_GATE, ISR(overflow), 0);
    idt_set_desc(0x5, IDT_TRAP_GATE, ISR(bound_range), 0);
    idt_set_desc(0x6, IDT_TRAP_GATE, ISR(invl_op), 0);
    idt_set_desc(0x8, IDT_TRAP_GATE, ISR(double_fault), 0);
    idt_set_desc(0xA, IDT_TRAP_GATE, ISR(invl_tss), 0);
    idt_set_desc(0xB, IDT_TRAP_GATE, ISR(segnp), 0);
    idt_set_desc(0xC, IDT_TRAP_GATE, ISR(ss_fault), 0);
    idt_set_desc(0xD, IDT_TRAP_GATE, ISR(general_prot), 0);
    idt_set_desc(0xE, IDT_TRAP_GATE, ISR(page_fault), 0);
}

void
cpu_conf(struct pcore *pcore)
{
    pcore->self = pcore;
    init_vectors();
    idt_load();
    platform_boot();

    /* We use %GS to store the processor */
    wrmsr(IA32_GS_BASE, (uintptr_t)pcore);
    lapic_init();
}
