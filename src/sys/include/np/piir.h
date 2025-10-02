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

#ifndef _NP_PIIR_H_
#define _NP_PIIR_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <os/spinlock.h>
#include <os/np.h>

/*
 * Represents two different kinds of bytes, MI IR
 * bytes and MD opcode / instruction bytes, less than
 * zero values are reserved for error indication.
 */
typedef int8_t ir_byte_t;
typedef int8_t md_byte_t;

/*
 * The maxiumum size of the bytecode stack per
 * translation unit
 */
#define PIIR_STACK_SIZE 4096

/* PIIR opcodes */
#define PIIR_NOP      0x00  /* Do nothing */
#define PIIR_LOAD_R8  0x01  /* Load 8-bit register */
#define PIIR_LOAD_R16 0x02  /* Load 16-bit register */
#define PIIR_LOAD_R32 0x03  /* Load 32-bit register */
#define PIIR_LOAD_R64 0x04  /* Load 64-bit register */
#define PIIR_RET_NIL  0x05  /* Return nothing */

/*
 * Represents the PIIR virtual machine for storing
 * state while converting IR to instruction bytes
 *
 * @code: Generated machine code
 * @last_ir: Last IR byte used
 * @code_i: Current index into code buffer
 */
struct piir_vm {
    md_byte_t code[4096];
    ir_byte_t last_ir;
    uint32_t code_i;
};

/*
 * Represents the Pi-IR (PIIR) bytecode stack
 *
 * @opstore: The actual stack
 * @op_head: Where new ops are written
 * @op_tail: Where new ops are read
 */
struct piir_stack {
    ir_byte_t opstore[PIIR_STACK_SIZE];
    uint16_t op_head;
    uint16_t op_tail;
    struct spinlock lock;
};

/*
 * Allocate a new PIIR stack
 *
 * @work: Current work
 * @resp: Result pointer is written here
 *
 * Returns zero on success, otherwise a less than zero
 * value on failure.
 */
int piir_stack_new(struct np_work *work, struct piir_stack **resp);

/*
 * Push a byte to the IR stack
 *
 * @stack: Stack to push to
 * @byte: Byte to push
 *
 * Returns zero on success, otherwise a less than zero
 * value on failure.
 */
int piir_push(struct piir_stack *stack, ir_byte_t byte);

/*
 * Pop a byte from the IR stack
 *
 * @stack: Stack to pop from
 *
 * Returns the byte on success, otherwise a less than
 * zero value if the stack is empty
 */
int piir_pop(struct piir_stack *stack);

/*
 * Inject control flow via stack machine
 *
 * @work: Current work
 *
 * Returns zero on success, otherwise a less than
 * zero value on failure.
 */
int piir_inject(struct np_work *work);

#endif  /* !_NP_PIIR_H_ */
