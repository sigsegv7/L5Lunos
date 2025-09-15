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

#ifndef _MACHINE_IDT_H_
#define _MACHINE_IDT_H_ 1

#include <sys/types.h>
#include <sys/cdefs.h>

#define IDT_INT_GATE    0x8E
#define IDT_TRAP_GATE   0x8F
#define IDT_USER_GATE   0xEE

#define ISR(p) ((uintptr_t)p)

/*
 * Represents a single interrupt/trap gate entry
 * within the interrupt descriptor table
 *
 * @off_low: Low 16 bits of ISR offset
 * @segsel: Expected code segment upon ISR entry
 * @ist: Interrupt stack table
 * @zero: [UNUSED] keep zero
 * @type: Gate type
 * @zero1: Same as `zero'
 * @dpl: Descriptor privilege level
 * @p: Present (1=valid)
 * @off_mid: Middle offset bits of ISR entry
 * @off_high: High offset bits of ISR entry
 * @reserved: [RESERVED] keep zero
 */
struct __packed idt_entry {
    uint16_t off_low;
    uint16_t segsel;
    uint8_t ist : 3;
    uint8_t zero : 5;
    uint8_t type : 4;
    uint8_t zero1 : 1;
    uint8_t dpl : 2;
    uint8_t p : 1;
    uint16_t off_mid;
    uint32_t off_high;
    uint32_t reserved;
};

/*
 * Represents the IDT register that points to the
 * base and limit of the Interrupt Descriptor Table.
 *
 * @limit: Limit of the IDT structure
 * @offset: Base offset of IDT structure
 */
struct __packed idtr {
    uint16_t limit;
    uintptr_t offset;
};

/*
 * Set an entry in the interrupt descriptor table
 *
 * @vector: Vector of entry to set
 * @type: Interrupt gate type
 * @isr: Interrupt service routine entrypoint
 * @ist: Interrupt stack table index
 */
void idt_set_desc(uint8_t vector, uint8_t type, uintptr_t isr, uint8_t ist);

/*
 * Load the interrupt descriptor table
 */
void idt_load(void);

#endif  /* !_MACHINE_IDT_H_ */
