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

/*
 * Valid 32-bit register IDs
 */
typedef enum {
    R32_EAX,
    R32_ECX,
    R32_EDX,
    R32_EBX,
    R32_ESP,
    R32_EBP,
    R32_ESI,
    R32_RDI,
    __R32_MAX
} r32_t;

/*
 * Valid 64-bit register IDs
 */
typedef enum {
    R64_RAX,
    R64_RCX,
    R64_RDX,
    R64_RBX,
    R64_RSP,
    R64_RBP,
    R64_RSI,
    R64_RDI,
    __R64_MAX
} r64_t;

/* SYS-V ABI specific */
#define R32_RETVAL R32_EAX
#define R64_RETVAL R64_RAX

/* Declare an instruction array */
#define INST_DECL(...) ((inst_t)__VA_ARGS__)

/* Near return [C2] */
#define OP_NRET INST_DECL({0xC2})
#define OP_NRET_LEN 1

/* No-operation */
#define OP_NOP INST_DECL({0x90})
#define OP_NOP_LEN 1

/* MOV R32, IMM32 (B8 + rd) */
#define OP_LOAD32_R32(IMM32, RD) INST_DECL({0xB8 + (RD), (IMM32)})
#define OP_LOAD32_R32_LEN 5

/* MOV R64, IMM64 (REX.W | 0xB8 + rd) */
#define OP_LOAD64_R64(IMM64, RD) INST_DECL({0x48, 0xB8 + (RD), (IMM64)})
#define OP_LOAD64_R64_LEN 10

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

/*
 * Push a 64-bit value
 */
static void
vm_push64(struct piir_vm *vm, uint64_t v)
{
    size_t max_len;
    uint64_t *p;

    max_len = (vm->code_i + sizeof(v));
    if (max_len >= sizeof(vm->code) - 1) {
        return;
    }

    p = (uint64_t *)&vm->code[vm->code_i];
    *p = v;
    vm->code_i += sizeof(v);
}

ssize_t
md_piir_decode(struct np_work *work, struct piir_vm *vm, ir_byte_t input)
{
    struct symbol *sym;
    struct piir_stack *stack;
    ssize_t len;
    int error;

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
            vm, OP_LOAD32_R32(input, R32_RETVAL),
            OP_LOAD32_R32_LEN
        );
    case PIIR_RET_SYMBOL:
        /* Get the symbol ID */
        if ((input = piir_pop(stack)) < 0) {
            pr_error("failed to pop element from stack\n");
            return input;
        }

        /* Lookup the symbol */
        error = symbol_lookup_id(&work->symlist, input, &sym);
        if (error < 0) {
            pr_error("failed to lookup symbol %d\n", input);
            return error;
        }

        /* Load some padding into our register */
        error = vm_push(vm, OP_LOAD64_R64(0, R64_RETVAL), OP_LOAD64_R64_LEN);
        if (error < 0) {
            pr_error("failed to push load op\n");
            return error;
        }

        /*
         * Move back 8 bytes, load the symbol address
         * and overwrite the padding
         */
        vm->code_i -= sizeof(sym->addr);
        vm_push64(vm, (uintptr_t)sym->addr);
        break;
    case PIIR_NOP:
        return vm_push(vm, OP_NOP, OP_NOP_LEN);
    case PIIR_RET_NIL:
        return vm_push(vm, OP_NRET, OP_NRET_LEN);
    }
    return 0;
}

reg_t
md_alloc_reg(struct np_work *work, struct piir_vm *vm, int flags)
{
    if (work == NULL || vm == NULL) {
        return -EINVAL;
    }

    /* If a bit is unset, it is free */
    for (int i = 0; i < __R32_MAX; ++i) {
        if (!ISSET(vm->regset, BIT(i))) {
            vm->regset |= BIT(i);
            return i;
        }
    }

    return -1;
}

void
md_free_reg(struct np_work *work, struct piir_vm *vm, reg_t reg)
{
    if (work == NULL || vm == NULL) {
        return;
    }

    vm->regset &= ~BIT(reg);
}
