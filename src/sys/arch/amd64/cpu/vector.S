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
  * Description: Interrupt vector handling core
  * Author: Ian Marco Moffett
  */

#include <machine/frameasm.h>
#include <machine/idt.h>

.macro IDT_SET_VEC vec, isr
    mov $\vec, %rdi
    mov $IDT_INT_GATE, %rsi
    lea \isr(%rip), %rdx
    xor %rcx, %rcx
    call idt_set_desc
.endm

  .globl irq_init_pins
  .text
irq_init_pins:
    IDT_SET_VEC 37, ioapic_edge_0
    IDT_SET_VEC 38, ioapic_edge_1
    IDT_SET_VEC 39, ioapic_edge_2
    IDT_SET_VEC 40, ioapic_edge_3
    IDT_SET_VEC 41, ioapic_edge_4
    IDT_SET_VEC 42, ioapic_edge_5
    IDT_SET_VEC 43, ioapic_edge_6
    IDT_SET_VEC 44, ioapic_edge_7
    IDT_SET_VEC 45, ioapic_edge_8
    IDT_SET_VEC 46, ioapic_edge_9
    IDT_SET_VEC 47, ioapic_edge_10
    IDT_SET_VEC 48, ioapic_edge_11
    IDT_SET_VEC 49, ioapic_edge_12
    IDT_SET_VEC 50, ioapic_edge_13
    IDT_SET_VEC 51, ioapic_edge_14
    IDT_SET_VEC 52, ioapic_edge_15
    IDT_SET_VEC 53, ioapic_edge_16
    IDT_SET_VEC 54, ioapic_edge_17
    IDT_SET_VEC 55, ioapic_edge_18
    IDT_SET_VEC 56, ioapic_edge_19
    IDT_SET_VEC 57, ioapic_edge_20
    IDT_SET_VEC 58, ioapic_edge_21
    IDT_SET_VEC 59, ioapic_edge_22
    IDT_SET_VEC 60, ioapic_edge_23
    IDT_SET_VEC 61, ioapic_edge_24
    IDT_SET_VEC 62, ioapic_edge_25
    IDT_SET_VEC 63, ioapic_edge_26
    IDT_SET_VEC 64, ioapic_edge_27
    IDT_SET_VEC 65, ioapic_edge_28
    IDT_SET_VEC 66, ioapic_edge_29
    IDT_SET_VEC 67, ioapic_edge_30
    IDT_SET_VEC 68, ioapic_edge_31
    IDT_SET_VEC 69, ioapic_edge_32
    IDT_SET_VEC 70, ioapic_edge_33
    IDT_SET_VEC 71, ioapic_edge_34
    IDT_SET_VEC 72, ioapic_edge_35
    IDT_SET_VEC 73, ioapic_edge_36
    IDT_SET_VEC 74, ioapic_edge_37
    IDT_SET_VEC 75, ioapic_edge_38
    IDT_SET_VEC 76, ioapic_edge_39
    IDT_SET_VEC 77, ioapic_edge_40
    IDT_SET_VEC 78, ioapic_edge_41
    IDT_SET_VEC 79, ioapic_edge_42
    IDT_SET_VEC 80, ioapic_edge_43
    IDT_SET_VEC 81, ioapic_edge_44
    IDT_SET_VEC 82, ioapic_edge_45
    IDT_SET_VEC 83, ioapic_edge_46
    IDT_SET_VEC 84, ioapic_edge_47
    IDT_SET_VEC 85, ioapic_edge_48
    IDT_SET_VEC 86, ioapic_edge_49
    IDT_SET_VEC 87, ioapic_edge_50
    IDT_SET_VEC 88, ioapic_edge_51
    IDT_SET_VEC 89, ioapic_edge_52
    IDT_SET_VEC 90, ioapic_edge_53
    IDT_SET_VEC 91, ioapic_edge_54
    IDT_SET_VEC 92, ioapic_edge_55
    IDT_SET_VEC 93, ioapic_edge_56
    IDT_SET_VEC 94, ioapic_edge_57
    IDT_SET_VEC 95, ioapic_edge_58
    IDT_SET_VEC 96, ioapic_edge_59
    IDT_SET_VEC 97, ioapic_edge_60
    IDT_SET_VEC 98, ioapic_edge_61
    IDT_SET_VEC 99, ioapic_edge_62
    IDT_SET_VEC 100, ioapic_edge_63
    retq

ioapic_common:
    xor %rcx, %rcx                      // Start at zero
.walk:                                  // Begin the table walk
    lea g_intrs(%rip), %rbx             // BASE -> RBX
    lea (%rbx, %rcx, 8), %rbx           // RBX+RCX*8 -> RBX
    mov (%rbx), %rdx                    // *RBX -> RDX
    or %rdx, %rdx                       // Is it NULL?
    jz 1f                               // Yes

    mov (%rdx), %rbx                    // Get the handler
    or %rbx, %rbx                       // Can we use this handler?
    jz 1f                               // No, continue on
    push %rcx                           // Save RCX between CALLs
    call *%rbx                          // Indirect call
    pop %rcx                            // We need RCX back now

    cmp $0, %rax                        // Do we continue? (not handled)
    jg done                             // Nope, handled

1:  inc %rcx                            // Bump up RCX, next idx
    cmp $256, %rcx                      // Are we through every?
    jl .walk                            // Nope, continue
done:                                   // End of loop
    call lapic_eoi                      // Signal end of interrupt
    retq


INTR_ENTRY(ioapic_edge_0)
    call ioapic_common
INTR_EXIT(ioapic_edge_0)

INTR_ENTRY(ioapic_edge_1)
    call ioapic_common
INTR_EXIT(ioapic_edge_1)

INTR_ENTRY(ioapic_edge_2)
    call ioapic_common
INTR_EXIT(ioapic_edge_2)

INTR_ENTRY(ioapic_edge_3)
    call ioapic_common
INTR_EXIT(ioapic_edge_3)

INTR_ENTRY(ioapic_edge_4)
    call ioapic_common
INTR_EXIT(ioapic_edge_4)

INTR_ENTRY(ioapic_edge_5)
    call ioapic_common
INTR_EXIT(ioapic_edge_5)

INTR_ENTRY(ioapic_edge_6)
    call ioapic_common
INTR_EXIT(ioapic_edge_6)

INTR_ENTRY(ioapic_edge_7)
    call ioapic_common
INTR_EXIT(ioapic_edge_7)

INTR_ENTRY(ioapic_edge_8)
    call ioapic_common
INTR_EXIT(ioapic_edge_8)

INTR_ENTRY(ioapic_edge_9)
    call ioapic_common
INTR_EXIT(ioapic_edge_9)

INTR_ENTRY(ioapic_edge_10)
    call ioapic_common
INTR_EXIT(ioapic_edge_10)

INTR_ENTRY(ioapic_edge_11)
    call ioapic_common
INTR_EXIT(ioapic_edge_11)

INTR_ENTRY(ioapic_edge_12)
    call ioapic_common
INTR_EXIT(ioapic_edge_12)

INTR_ENTRY(ioapic_edge_13)
    call ioapic_common
INTR_EXIT(ioapic_edge_13)

INTR_ENTRY(ioapic_edge_14)
    call ioapic_common
INTR_EXIT(ioapic_edge_14)

INTR_ENTRY(ioapic_edge_15)
    call ioapic_common
INTR_EXIT(ioapic_edge_15)

INTR_ENTRY(ioapic_edge_16)
    call ioapic_common
INTR_EXIT(ioapic_edge_16)

INTR_ENTRY(ioapic_edge_17)
    call ioapic_common
INTR_EXIT(ioapic_edge_17)

INTR_ENTRY(ioapic_edge_18)
    call ioapic_common
INTR_EXIT(ioapic_edge_18)

INTR_ENTRY(ioapic_edge_19)
    call ioapic_common
INTR_EXIT(ioapic_edge_19)

INTR_ENTRY(ioapic_edge_20)
    call ioapic_common
INTR_EXIT(ioapic_edge_20)

INTR_ENTRY(ioapic_edge_21)
    call ioapic_common
INTR_EXIT(ioapic_edge_21)

INTR_ENTRY(ioapic_edge_22)
    call ioapic_common
INTR_EXIT(ioapic_edge_22)

INTR_ENTRY(ioapic_edge_23)
    call ioapic_common
INTR_EXIT(ioapic_edge_23)

INTR_ENTRY(ioapic_edge_24)
    call ioapic_common
INTR_EXIT(ioapic_edge_24)

INTR_ENTRY(ioapic_edge_25)
    call ioapic_common
INTR_EXIT(ioapic_edge_25)

INTR_ENTRY(ioapic_edge_26)
    call ioapic_common
INTR_EXIT(ioapic_edge_26)

INTR_ENTRY(ioapic_edge_27)
    call ioapic_common
INTR_EXIT(ioapic_edge_27)

INTR_ENTRY(ioapic_edge_28)
    call ioapic_common
INTR_EXIT(ioapic_edge_28)

INTR_ENTRY(ioapic_edge_29)
    call ioapic_common
INTR_EXIT(ioapic_edge_29)

INTR_ENTRY(ioapic_edge_30)
    call ioapic_common
INTR_EXIT(ioapic_edge_30)

INTR_ENTRY(ioapic_edge_31)
    call ioapic_common
INTR_EXIT(ioapic_edge_31)

INTR_ENTRY(ioapic_edge_32)
    call ioapic_common
INTR_EXIT(ioapic_edge_32)

INTR_ENTRY(ioapic_edge_33)
    call ioapic_common
INTR_EXIT(ioapic_edge_33)

INTR_ENTRY(ioapic_edge_34)
    call ioapic_common
INTR_EXIT(ioapic_edge_34)

INTR_ENTRY(ioapic_edge_35)
    call ioapic_common
INTR_EXIT(ioapic_edge_35)

INTR_ENTRY(ioapic_edge_36)
    call ioapic_common
INTR_EXIT(ioapic_edge_36)

INTR_ENTRY(ioapic_edge_37)
    call ioapic_common
INTR_EXIT(ioapic_edge_37)

INTR_ENTRY(ioapic_edge_38)
    call ioapic_common
INTR_EXIT(ioapic_edge_38)

INTR_ENTRY(ioapic_edge_39)
    call ioapic_common
INTR_EXIT(ioapic_edge_39)

INTR_ENTRY(ioapic_edge_40)
    call ioapic_common
INTR_EXIT(ioapic_edge_40)

INTR_ENTRY(ioapic_edge_41)
    call ioapic_common
INTR_EXIT(ioapic_edge_41)

INTR_ENTRY(ioapic_edge_42)
    call ioapic_common
INTR_EXIT(ioapic_edge_42)

INTR_ENTRY(ioapic_edge_43)
    call ioapic_common
INTR_EXIT(ioapic_edge_43)

INTR_ENTRY(ioapic_edge_44)
    call ioapic_common
INTR_EXIT(ioapic_edge_44)

INTR_ENTRY(ioapic_edge_45)
    call ioapic_common
INTR_EXIT(ioapic_edge_45)

INTR_ENTRY(ioapic_edge_46)
    call ioapic_common
INTR_EXIT(ioapic_edge_46)

INTR_ENTRY(ioapic_edge_47)
    call ioapic_common
INTR_EXIT(ioapic_edge_47)

INTR_ENTRY(ioapic_edge_48)
    call ioapic_common
INTR_EXIT(ioapic_edge_48)

INTR_ENTRY(ioapic_edge_49)
    call ioapic_common
INTR_EXIT(ioapic_edge_49)

INTR_ENTRY(ioapic_edge_50)
    call ioapic_common
INTR_EXIT(ioapic_edge_50)

INTR_ENTRY(ioapic_edge_51)
    call ioapic_common
INTR_EXIT(ioapic_edge_51)

INTR_ENTRY(ioapic_edge_52)
    call ioapic_common
INTR_EXIT(ioapic_edge_52)

INTR_ENTRY(ioapic_edge_53)
    call ioapic_common
INTR_EXIT(ioapic_edge_53)

INTR_ENTRY(ioapic_edge_54)
    call ioapic_common
INTR_EXIT(ioapic_edge_54)

INTR_ENTRY(ioapic_edge_55)
    call ioapic_common
INTR_EXIT(ioapic_edge_55)

INTR_ENTRY(ioapic_edge_56)
    call ioapic_common
INTR_EXIT(ioapic_edge_56)

INTR_ENTRY(ioapic_edge_57)
    call ioapic_common
INTR_EXIT(ioapic_edge_57)

INTR_ENTRY(ioapic_edge_58)
    call ioapic_common
INTR_EXIT(ioapic_edge_58)

INTR_ENTRY(ioapic_edge_59)
    call ioapic_common
INTR_EXIT(ioapic_edge_59)

INTR_ENTRY(ioapic_edge_60)
    call ioapic_common
INTR_EXIT(ioapic_edge_60)

INTR_ENTRY(ioapic_edge_61)
    call ioapic_common
INTR_EXIT(ioapic_edge_61)

INTR_ENTRY(ioapic_edge_62)
    call ioapic_common
INTR_EXIT(ioapic_edge_62)

INTR_ENTRY(ioapic_edge_63)
    call ioapic_common
INTR_EXIT(ioapic_edge_63)
