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
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/dms.h>
#include <os/systm.h>
#include <os/kalloc.h>
#include <dms/dms.h>
#include <string.h>

static ssize_t
dms_io(struct dms_frame *dfp)
{
    struct dms_disk *dp;
    struct dms_diskinfo info;
    size_t len;
    ssize_t retval = -ENXIO;
    int error;
    void *kbuf;

    if (dfp == NULL) {
        return -EINVAL;
    }

    if ((len = dfp->len) == 0) {
        return -EINVAL;
    }

    if ((dp = dms_get(dfp->id)) == NULL) {
        return -ENODEV;
    }

    if ((kbuf = kalloc(dfp->len)) == NULL) {
        return -ENOMEM;
    }

    switch (dfp->opcode) {
    case DMS_OPC_READ:
        retval = dms_read(dp, kbuf, dfp->offset, dfp->len);
        if (retval < 0) {
            break;
        }
        error = copyout(kbuf, dfp->buf, dfp->len);
        if (error < 0) {
            retval = error;
            break;
        }
        break;
    case DMS_OPC_WRITE:
        error = copyin(dfp->buf, kbuf, dfp->len);
        if (error < 0) {
            retval = error;
            break;
        }

        retval = dms_write(dp, kbuf, dfp->offset, dfp->len);
        break;
    case DMS_OPC_QUERY:
        memcpy(info.name, dp->name, sizeof(info.name));
        info.bsize = dp->bsize;
        info.id = dp->id;
        if (dfp->len > sizeof(info))
            dfp->len = sizeof(info);
        retval = copyout(&info, dfp->buf, dfp->len);
        break;
    }

    kfree(kbuf);
    return retval;
}

/*
 * ARG0: DMS frame pointer
 */
scret_t
sys_dmsio(struct syscall_args *scargs)
{
    struct dms_frame *u_dfp = SCARG(scargs, struct dms_frame *, 0);
    struct dms_frame df;
    int error;

    error = copyin(u_dfp, &df, sizeof(df));
    if (error < 0) {
        return error;
    }

    return dms_io(&df);
}
