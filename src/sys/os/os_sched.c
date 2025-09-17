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
 * Description: Lunos scheduler core
 * Author: Ian Marco Moffett
 */

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/panic.h>
#include <sys/queue.h>
#include <sys/cpuvar.h>
#include <os/sched.h>

__cacheline_aligned
static struct core_arbiter arbiter = {
    .rr_id = 0,
    .type = CORE_ARBITER_RR
};

/*
 * Schedule the next processor core
 */
struct pcore *
cpu_sched(void)
{
    struct pcore *retval;

    spinlock_acquire(&arbiter.lock);
    switch (arbiter.type) {
    case CORE_ARBITER_RR:
        retval = cpu_get(arbiter.rr_id++);

        /*
         * If we made it at the end, wrap to the beginning.
         * XXX: Us getting entry 0 would make the next be 1.
         */
        if (retval == NULL) {
            arbiter.rr_id = 1;
            retval = cpu_get(0);
        }

        break;
    }

    spinlock_release(&arbiter.lock);
    return retval;
}

/*
 * Enqueue a process into a queue
 */
int
sched_enq(struct sched_queue *q, struct proc *proc)
{
    if (q == NULL || proc == NULL) {
        return -EINVAL;
    }

    spinlock_acquire(&q->lock);
    TAILQ_INSERT_TAIL(&q->q, proc, link);
    ++q->nproc;
    spinlock_release(&q->lock);
    return 0;
}

/*
 * Dequeue a process from a queue
 */
int
sched_deq(struct sched_queue *q, struct proc **procp)
{
    struct proc *proc;

    if (q == NULL || procp == NULL) {
        return -EINVAL;
    }

    /* Anything to dequeue? */
    if (q->nproc == 0) {
        return -EAGAIN;
    }

    spinlock_acquire(&q->lock);
    proc = TAILQ_FIRST(&q->q);
    TAILQ_REMOVE(&q->q, proc, link);
    --q->nproc;
    spinlock_release(&q->lock);
    return 0;
}

void
sched_init(void)
{
    struct pcore *core;

    if ((core = this_core()) == NULL) {
        panic("sched_init: could not get core\n");
    }

    TAILQ_INIT(&core->scq.q);
    printf("sched: scheduler is [up]\n");
}
