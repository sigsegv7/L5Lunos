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
 * Description: Subsystem to convert PIIR to instruction bytes
 * Author: Ian Marco Moffett
 */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <machine/piir.h>
#include <np/piir.h>
#include <os/np.h>

#define pr_trace(fmt, ...) printf("piir.conv: " fmt, ##__VA_ARGS__)
#define pr_error(fmt, ...) printf("piir.conv: error: " fmt, ##__VA_ARGS__)

typedef md_byte_t inst_t[8];

/* Declare an instruction array */
#define INST_DECL(...) ((inst_t)__VA_ARGS__)

/* Near return [C2] */
#define OP_NRET INST_DECL({0xC2})
#define OP_NRET_LEN 1

/* No-operation */
#define OP_NOP INST_DECL({0x90})
#define OP_NOP_LEN 1

/* MOV R32, IMM32 (B8 + rd) */
#define OP_LOAD32_R32(IMM32) INST_DECL({0xB8, (IMM32)})
#define OP_LOAD32_R32_LEN 5

/*
 * Push an instruction byte into the virtual
 * machine descriptor's code buffer
 */
static ssize_t
vm_push(struct piir_vm *vm, inst_t inst, size_t len)
{
    for (int i = 0; i < len; ++i) {
        if (vm->code_i >= sizeof(vm->code) - 1) {
            return -1;
        }

        vm->code[vm->code_i++] = inst[i];
    }
    return len;
}

ssize_t
md_piir_decode(struct np_work *work, struct piir_vm *vm, ir_byte_t input)
{
    struct piir_stack *stack;
    ssize_t len;

    if (vm == NULL) {
        return -EINVAL;
    }

    vm->last_ir = input;
    stack = work->piir_stack;

    /*
     * Convert the instruction from IR to machine
     * code so that it actually may be executed
     */
    switch (input) {
    case PIIR_RET_NUM:
        if ((input = piir_pop(stack)) < 0) {
            pr_error("bad input on PIIR_RET_NUM\n");
            return input;
        }

        return vm_push(
            vm, OP_LOAD32_R32(input),
            OP_LOAD32_R32_LEN
        );
    case PIIR_NOP:
        return vm_push(vm, OP_NOP, OP_NOP_LEN);
    case PIIR_RET_NIL:
        return vm_push(vm, OP_NRET, OP_NRET_LEN);
    }
    return 0;
}
