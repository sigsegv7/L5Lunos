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

#ifndef _SYS_CPUVAR_H_
#define _SYS_CPUVAR_H_ 1

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/proc.h>
#if defined(_KERNEL)
#include <os/sched.h>
#include <machine/mdcpu.h>
#endif  /* _KERNEL */

/*
 * Logically describes a processor core on the
 * system. This structure contains machine
 * independent.
 *
 * @id: Monotonic logical ID
 * @curproc: Current process running
 * @scq: Scheduler queue
 * @md: Machine dependent processor information
 * @self: Chain pointer to self
 */
struct pcore {
    uint32_t id;
    struct proc *curproc;
#if defined(_KERNEL)
    struct sched_queue scq;
    struct mdcore md;
#endif  /* _KERNEL */
    struct pcore *self;
};

#if defined(_KERNEL)
/*
 * Configure a processor core on the system
 *
 * [MD]
 *
 * @pcore: Core to configure
 */
void cpu_conf(struct pcore *pcore);

/*
 * Initialize a processor core on the system, second
 * stage initialization hook.
 *
 * [MD]
 *
 * @pcore: Processor core to init
 */
void cpu_init(struct pcore *pcore);

/*
 * Get a specific CPU descriptor using a logical
 * ID number (`index') as a key
 *
 * @index: Index / logical ID of desired processor
 *
 * [MD]
 *
 * Returns a pointer to the core descriptor on success,
 * otherwise NULL to represent failure.
 */
struct pcore *cpu_get(uint16_t index);

/*
 * Get the current processing element (core) as
 * a 'pcore' descriptor.
 *
 * [MD]
 *
 * Returns NULL on failure.
 */
struct pcore *this_core(void);

/*
 * Start up the application processes from the
 * bootstrap processor.
 *
 * [MD]
 */
void bsp_ap_startup(void);
#endif  /* _KERNEL */
#endif  /* !_SYS_CPUVAR_H_ */
