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

#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/cdefs.h>
#include <sys/proc.h>
#include <os/signal.h>

/*
 * Handle a POSIX sigaction
 */
static int
do_sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
    struct proc *self;

    if (act == NULL || oact == NULL) {
        return -EINVAL;
    }

    if ((self = proc_self()) == NULL) {
        return -ESRCH;
    }

    /* Don't overflow the signal table */
    if (sig >= NELEM(self->sigtab) || sig < 0) {
        return -EINVAL;
    }

    if (oact != NULL) {
        *oact = self->sigtab[sig];
    }

    if (act != NULL) {
        self->sigtab[sig] = *act;
    }

    return 0;
}

/*
 * ARG0: Signal number
 * ARG1: Action to set (act)
 * ARG2: Old action (old sigaction written here)
 */
scret_t
sys_sigaction(struct syscall_args *scargs)
{
    struct proc *self = proc_self();
    int sig;
    struct sigaction *sap, *sap_old;
    int error;

    sig = SCARG(scargs, int, 0);
    sap = SCARG(scargs, struct sigaction *, 1);
    sap_old = SCARG(scargs, struct sigaction *, 2);

    /* Sigactions at a valid address? */
    error = proc_check_addr(self, (uintptr_t)sap, sizeof(*sap));
    if (error < 0) {
        return error;
    }

    /* Old sigactions at a valid address? */
    error = proc_check_addr(self, (uintptr_t)sap_old, sizeof(sap_old));
    if (error < 0) {
        return error;
    }

    return do_sigaction(sig, sap, sap_old);

}
