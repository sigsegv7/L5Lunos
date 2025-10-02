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

#include <sys/types.h>
#include <sys/errno.h>
#include <os/np.h>
#include <np/piir.h>
#include <lib/ptrbox.h>
#include <string.h>

/*
 * Allocate a new stack
 */
int
piir_stack_new(struct np_work *work, struct piir_stack **resp)
{
    struct piir_stack *stack;

    if (work == NULL || resp == NULL) {
        return -EINVAL;
    }

    stack = ptrbox_alloc(sizeof(*stack), work->work_mem);
    if (stack == NULL) {
        return -ENOMEM;
    }

    memset(stack, 0, sizeof(*stack));
    *resp = stack;
    return 0;
}

/*
 * Push to the PIIR stack
 */
int
piir_push(struct piir_stack *stack, ir_byte_t byte)
{
    if (stack == NULL) {
        return -EINVAL;
    }

    if (stack->op_head >= PIIR_STACK_SIZE - 1) {
        return -1;
    }

    spinlock_acquire(&stack->lock);
    stack->opstore[stack->op_head++] = byte;
    spinlock_release(&stack->lock);
    return 0;
}

/*
 * Pop from the PIIR stack
 */
int
piir_pop(struct piir_stack *stack)
{
    ir_byte_t byte;

    if (stack == NULL) {
        return -EINVAL;
    }

    /* Don't read past what we can */
    if (stack->op_tail == stack->op_head) {
        stack->op_tail = 0;
        stack->op_head = 0;
        return -1;
    }

    spinlock_acquire(&stack->lock);

    /* Grab a byte, reset pointers if empty */
    byte = stack->opstore[stack->op_tail++];
    if (stack->op_tail == stack->op_head) {
        stack->op_tail = 0;
        stack->op_head = 0;
    }

    spinlock_release(&stack->lock);
    return byte;
}
