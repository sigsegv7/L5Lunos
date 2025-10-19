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
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <os/ucred.h>
#include <os/ucred.h>

int
ucred_init(struct proc *proc, struct ucred *cred)
{
    struct ucred *curcred;

    if (cred == NULL) {
        return -EINVAL;
    }

    if (proc != NULL) {
        curcred = &proc->cred;
        cred->ruid = curcred->ruid;
    } else {
        cred->ruid = 0;
    }

    cred->euid = cred->ruid;
    cred->suid = cred->ruid;
    return 0;
}

int
seteuid(uid_t euid)
{
    struct proc *self = proc_self();
    struct ucred *cred;
    int retval = -EPERM;

    if (__unlikely(self == NULL)) {
        return -ESRCH;
    }

    /* Verify against current creds */
    cred = &self->cred;
    if (euid == cred->euid || euid == cred->ruid) {
        cred->euid = euid;
        retval = 0;
    } else if (euid == cred->suid || cred->euid == 0) {
        cred->euid = euid;
        retval = 0;
    }

    return retval;
}

/*
 * ARG0: EUID
 */
scret_t
sys_seteuid(struct syscall_args *scargs)
{
    uid_t euid = SCARG(scargs, int, 0);

    return seteuid(euid);
}
