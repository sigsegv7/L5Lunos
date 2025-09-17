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

#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/queue.h>
#include <machine/pcb.h>    /* standard */

/*
 * The stack starts here and grows down
 */
#define STACK_TOP   0xBFFFFFFF
#define STACK_LEN   4096

/*
 * A process describes a running program image
 * on the system.
 *
 * @pid: Process ID
 */
struct proc {
    pid_t pid;
    struct md_pcb pcb;
    TAILQ_ENTRY(proc) link;
};

/*
 * Initialize a process into a basic minimal
 * state
 *
 * @procp: New process data is written here
 * @flags: Optional flags
 *
 * Returns zero on success, otherwise a less than
 * zero value to indicate failure.
 */
int proc_init(struct proc *procp, int flags);

/*
 * Initialize machine dependent state of a process
 *
 * @procp: New process data is written here
 * @flags: Optional flags
 *
 * Returns zero on success, otherwise a less than
 * zero value to indicate failure.
 */
int md_proc_init(struct proc *procp, int flags);

/*
 * Set the instruction pointer field of a specific
 * process
 *
 * @procp: Process to update
 * @ip: Instruction pointer to write
 *
 * Returns zero on success, otherwise a less than
 * zero value to indicate failure.
 */
int md_set_ip(struct proc *procp, uintptr_t ip);

/*
 * Put the current process into a halt loop
 * until the next one runs.
 */
__dead void md_proc_yield(void);

/*
 * Kick a process into a user context
 *
 * @procp: Process pointer
 */
__dead void md_proc_kick(struct proc *procp);

#endif  /* !_SYS_PROC_H_ */
