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
#include <machine/gdt.h>
#include <machine/mdcpu.h>
#include <machine/cpuid.h>
#include <string.h>

/* Valid vendor strings */
#define VENDSTR_INTEL "GenuineIntel"
#define VENDSTR_INTEL1 "GenuineIotel"
#define VENDSTR_AMD    "AuthenticAMD"

extern void syscall_isr(void);
extern void core_halt_isr(void);
void core_halt_handler(void);

void
core_halt_handler(void)
{
    for (;;) {
        __ASMV("cli; hlt");
    }
}

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
    idt_set_desc(0x80, IDT_USER_GATE, ISR(syscall_isr), 0);
    idt_set_desc(HALT_VECTOR, IDT_USER_GATE, ISR(core_halt_isr), 0);
}

/*
 * Identify the CPU vendor - used by cpu_identify()
 */
static void
cpu_vendor(struct mdcore *mdcore)
{
    uint32_t ebx, edx, ecx, dmmy;
    char vendstr[13];

    if (mdcore == NULL) {
        return;
    }

    CPUID(0x00, dmmy, ebx, ecx, edx);

    /* DWORD 0 */
    vendstr[0] = (ebx & 0xFF);
    vendstr[1] = (ebx >> 8) & 0xFF;
    vendstr[2] = (ebx >> 16) & 0xFF;
    vendstr[3] = (ebx >> 24) & 0xFF;

    /* DWORD 1 */
    vendstr[4] = (edx & 0xFF);
    vendstr[5] = (edx >> 8) & 0xFF;
    vendstr[6] = (edx >> 16) & 0xFF;
    vendstr[7] = (edx >> 24) & 0xFF;

    /* DWORD 2 */
    vendstr[8] = (ecx & 0xFF);
    vendstr[9] = (ecx >> 8) & 0xFF;
    vendstr[10] = (ecx >> 16) & 0xFF;
    vendstr[11] = (ecx >> 24) & 0xFF;
    vendstr[12] = '\0';

    if (strcmp(vendstr, VENDSTR_INTEL) == 0) {
        mdcore->vendor = CPU_VENDOR_INTEL;
        return;
    }

    /* Some buggy Intel CPUs have this rare one */
    if (strcmp(vendstr, VENDSTR_INTEL1) == 0) {
        mdcore->vendor = CPU_VENDOR_INTEL;
        return;
    }

    if (strcmp(vendstr, VENDSTR_AMD) == 0) {
        mdcore->vendor = CPU_VENDOR_AMD;
        return;
    }

    /* Unknown CPU vendor */
    mdcore->vendor = CPU_VENDOR_OTHER;
}

/*
 * Acquire the CPU family ID - used by cpu_identify()
 */
static void
cpu_family(struct mdcore *mdcore)
{
    uint32_t eax, dmmy;
    uint32_t family, ext_family;

    if (mdcore == NULL) {
        return;
    }

    CPUID(0x01, eax, dmmy, dmmy, dmmy);

    /*
     * If the family ID is 15 then the actual result
     * is the sum of the family ID and extended family
     * ID.
     */
    family = (eax >> 8) & 0xF;
    if (family == 15) {
        ext_family = (eax >> 20) & 0xFF;
        family += ext_family;
    }

    mdcore->family = family;
}

/*
 * Identify the CPU via a CPUID
 */
static void
cpu_identify(struct mdcore *mdcore)
{
    if (mdcore == NULL) {
        return;
    }

    cpu_vendor(mdcore);
    cpu_family(mdcore);
}

void
cpu_conf(struct pcore *pcore)
{
    struct mdcore *mdcore;
    struct gdtr *gdtr;

    /* Copy the template GDT */
    mdcore = &pcore->md;
    memcpy(mdcore->gdt, &g_gdt_data, sizeof(g_gdt_data));

    /* Set up the GDTR */
    gdtr = &mdcore->gdtr;
    gdtr->offset = (uintptr_t)&mdcore->gdt[0];
    gdtr->limit = sizeof(g_gdt_data) - 1;

    /* We use %GS to store the processor */
    gdt_load(&mdcore->gdtr);
    pcore->self = pcore;
    wrmsr(IA32_GS_BASE, (uintptr_t)pcore);

    init_vectors();
    idt_load();
    cpu_identify(mdcore);
    __ASMV("sti");
}

void
cpu_init(struct pcore *pcore)
{
    platform_boot();
    lapic_init();
}
