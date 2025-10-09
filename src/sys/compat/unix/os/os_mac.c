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

#include <sys/syscall.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <os/mac.h>
#include <compat/unix/syscall.h>

/*
 * ARG0: Border ID (BORDER_*)
 * ARG1: Length requested
 * ARG2: Offset requested
 * ARG3: Flags
 * ARG4: Result
 */
scret_t
sys_cross(struct syscall_args *scargs)
{
    border_id_t bd = SCARG(scargs, border_id_t, 0);
    size_t len = SCARG(scargs, size_t, 1);
    off_t off = SCARG(scargs, off_t, 2);
    int flags = SCARG(scargs, int, 3);
    void **res = SCARG(scargs, void **, 4);
    struct mac_border *bop;
    struct proc *self = proc_self();
    int error;

    error = proc_check_addr(self, (uintptr_t)res, len);
    if (error < 0) {
        return error;
    }

    bop = mac_get_border(bd);
    if (bop == NULL) {
        return -EIO;
    }

    return mac_map(bop, off, len, res, flags);
}

/*
 * ARG0: Border ID (BORDER_*)
 * ARG1: Data
 * ARG2: Data length
 * ARG3: Optional flags
 *
 * Returns int (0 on success)
 */
scret_t
sys_query(struct syscall_args *scargs)
{
    border_id_t bd = SCARG(scargs, border_id_t, 0);
    void *u_data = SCARG(scargs, void *, 1);
    size_t u_datalen = SCARG(scargs, size_t, 2);
    int flags = SCARG(scargs, int, 3);
    struct mac_border *bop;
    struct mac_ops *ops;
    struct proc *self = proc_self();
    int error;

    bop = mac_get_border(bd);
    if (bop == NULL) {
        return -EIO;
    }

    /* Can we even touch this? */
    error = mac_check_lvl(self, bop->level);
    if (error < 0) {
        return error;
    }

    error = proc_check_addr(self, (uintptr_t)u_data, u_datalen);
    if (error < 0) {
        return error;
    }

    /* We need the operations vector */
    if ((ops = bop->ops) == NULL) {
        return -EIO;
    }

    return ops->getattr(bop, u_data, u_datalen);
}
