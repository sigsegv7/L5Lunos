/*
 * Copyright (c) 2023-2025 Ian Marco Moffett and the Osmora Team.
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
 * 3. Neither the name of Hyra nor the names of its
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

#ifndef _MACHINE_INTR_H_
#define _MACHINE_INTR_H_ 1

#include <sys/types.h>

#define IPL_NONE    0x00

/* Upper 4 bits of an interrupt vector */
#define IPL_SHIFT 4

/*
 * Represents an interrupt handler, this is split up into
 * two parts. One side is only meant for the driver and C
 * interrupt code to see, and the other shall be visible
 * for the lower level interrupt handling code in vector.S
 *
 * @hand: Driver-specific handler to invoke [D]
 * --
 * @name: Name of interrupt         : strdup()'d  [D]
 * @ipl: Interrupt priority level [D]
 * @irq: IRQ number to use [D]
 * @vector: Interrupt vector result [I]
 * @count: Interrupt count [I]
 *
 * [D]: Set by driver
 * [I]: Set internally
 */
struct intr_hand {
    /* -- shared between vector.S -- */
    int(*hand)(struct intr_hand *hp);

    /* -- private to driver and C -- */
    char *name;
    int8_t ipl;
    uint8_t irq;
    uint8_t vector;
    uint32_t count;
};

/*
 * Register an interrupt with a specific priority
 *
 * @ih: Interrupt handler to register
 *
 * Returns new interrupt handler on success,
 * otherwise NULL on failure
 */
struct intr_hand *intr_register(const struct intr_hand *ih);

#endif  /* !_MACHINE_INTR_H_ */
