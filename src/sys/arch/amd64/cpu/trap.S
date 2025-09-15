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

#include <machine/frameasm.h>
#include <machine/trap.h>

    .globl breakpoint_handler
TRAP_ENTRY(breakpoint_handler, $TRAP_BREAKPOINT)
    mov %rsp, %rdi
    call trap_handler
TRAP_EXIT(breakpoint_handler)

    .globl arith_err
TRAP_ENTRY(arith_err, $TRAP_ARITH_ERR)
    mov %rsp, %rdi
    call trap_handler
TRAP_EXIT(arith_err)

    .globl overflow
TRAP_ENTRY(overflow, $TRAP_OVERFLOW)
    mov %rsp, %rdi
    call trap_handler
TRAP_EXIT(overflow)

    .globl bound_range
TRAP_ENTRY(bound_range, $TRAP_BOUND_RANGE)
    mov %rsp, %rdi
    call trap_handler
TRAP_EXIT(bound_range)

    .globl invl_op
TRAP_ENTRY(invl_op, $TRAP_INVLOP)
    mov %rsp, %rdi
    call trap_handler
TRAP_EXIT(invl_op)

    .globl double_fault
TRAP_ENTRY_EC(double_fault, $TRAP_DOUBLE_FAULT)
    mov %rsp, %rdi
    call trap_handler
TRAP_EXIT_EC(double_fault)

    .globl invl_tss
TRAP_ENTRY_EC(invl_tss, $TRAP_INVLTSS)
    mov %rsp, %rdi
    call trap_handler
TRAP_EXIT_EC(invl_tss)

    .globl segnp
TRAP_ENTRY_EC(segnp, $TRAP_SEGNP)
    mov %rsp, %rdi
    call trap_handler
TRAP_EXIT_EC(segnp)

    .globl general_prot
TRAP_ENTRY_EC(general_prot, $TRAP_PROTFLT)
    mov %rsp, %rdi
    call trap_handler
TRAP_EXIT_EC(general_prot)

    .globl page_fault
TRAP_ENTRY_EC(page_fault, $TRAP_PAGEFLT)
    mov %rsp, %rdi
    call trap_handler
TRAP_EXIT_EC(page_fault)

    .globl nmi
TRAP_ENTRY(nmi, $TRAP_NMI)
    mov %rsp, %rdi
    call trap_handler
TRAP_EXIT(nmi)

    .globl ss_fault
TRAP_ENTRY_EC(ss_fault, $TRAP_SS)
    mov %rsp, %rdi
    call trap_handler
TRAP_EXIT_EC(ss_fault)
