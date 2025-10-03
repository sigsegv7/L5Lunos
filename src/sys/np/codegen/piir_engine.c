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
#include <sys/syslog.h>
#include <machine/piir.h>
#include <os/np.h>
#include <np/piir.h>
#include <string.h>

#define pr_trace(fmt, ...) printf("pirho.piir: " fmt, ##__VA_ARGS__)
#define pr_error(fmt, ...) printf("pirho.piir: error: " fmt, ##__VA_ARGS__)

int
piir_inject(struct np_work *work)
{
    struct piir_stack *stack;
    struct piir_vm vm;
    ir_byte_t byte;

    if (work == NULL) {
        return -EINVAL;
    }

    /* Need the stack */
    if ((stack = work->piir_stack) == NULL) {
        return -EIO;
    }

    /* Put the vm in a known state */
    memset(&vm, 0, sizeof(vm));

    /*
     * Go through each byte and decode the IR, new
     * machine code should be in `vm.code' after
     * this loop.
     */
    while ((byte = piir_pop(stack)) >= 0) {
        md_piir_decode(work, &vm, byte);
    }

    return 0;
}
