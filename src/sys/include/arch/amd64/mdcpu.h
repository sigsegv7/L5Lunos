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

#ifndef _MACHINE_MDCPU_H_
#define _MACHINE_MDCPU_H_ 1

#include <sys/types.h>
#include <sys/cdefs.h>
#include <machine/tss.h>
#include <machine/gdt.h>

#define HALT_VECTOR 0x90

#define md_spinwait() __ASMV("pause")
#define md_intoff()   __ASMV("cli")
#define md_inton()    __ASMV("sti")
#define md_halt()     __ASMV("hlt")

#define CPU_VENDOR_OTHER  0x00
#define CPU_VENDOR_AMD    0x01
#define CPU_VENDOR_INTEL  0x02

/*
 * Represents the machine dependent information
 * of a processor core on the machine.
 *
 * @apic_id: Local APIC ID
 * @cr3: CR3 register value (PML<n> phys)
 * @vendor: Processor vendor (CPU_VENDOR_*)
 * @family: Processor family ID
 * @lapic_base: LAPIC register interface base
 * @x2apic: Has the x2APIC? Is 1 if true
 * @tss: Task state segment for this core
 * @lapic_tmr_freq: Local APIC timer frequency
 * @gdt: Global descriptor table instance
 * @gdtr: GDT descriptor
 */
struct mdcore {
    uint32_t apic_id;
    uint64_t cr3;
    uint8_t vendor;
    uint32_t family;
    void *lapic_base;
    uint8_t x2apic : 1;
    struct tss_entry tss;
    size_t lapic_tmr_freq;
    struct gdt_entry gdt[GDT_ENTRY_COUNT];
    struct gdtr gdtr;
};

#endif  /* !_MACHINE_MDCPU_H_ */
