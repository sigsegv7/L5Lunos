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

#ifndef _OS_SCHED_H_
#define _OS_SCHED_H_ 1

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <os/spinlock.h>

#define SCHED_NQUEUES 4

/*
 * Represents a queue of processes
 *
 * @q; Actual queue
 * @nproc: Number of processes in this queue
 */
struct sched_queue {
    TAILQ_HEAD(, proc) q;
    struct spinlock lock;
    size_t nproc;
};

/*
 * Enqueue a new process to a queue
 *
 * @q: Queue to target
 * @proc: Process to place in the queue
 *
 * Returns zero on success, otherwise a less than
 * zero value to indicate failure.
 */
int sched_enq(struct sched_queue *q, struct proc *proc);

/*
 * Dequeue a process from a queue
 *
 * @q: Queue to dequeue from
 * @procp: Result of new popped process is written here
 *
 * Returns zero on success, otherwise a less than zero
 * value on failure.
 */
int sched_deq(struct sched_queue *q, struct proc **procp);

/*
 * Initialize the scheduler into a basic
 * known state.
 */
void sched_init(void);

#endif  /* !_OS_SCHED_H_ */
