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

#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <io/video/fbdev.h>
#include <os/mac.h>

/*
 * The MAC border table is used for mappable resources
 * under MAC that have common operations associated with
 * them.
 */
static struct mac_border *bortab[__BORDER_MAX] = {
    [BORDER_NONE] = NULL,
    [BORDER_FBDEV] = &g_fbdev_border
};

int
mac_check_lvl(struct proc *procp, mac_level_t lvl)
{
    if (procp == NULL) {
        return -EINVAL;
    }

    if (procp->level < lvl) {
        return -EACCES;
    }

    return 0;
}

/*
 * Map a resource into process address space
 */
ssize_t
mac_map(struct mac_border *mbp, off_t off, size_t len, void **res, int flags)
{
    struct mac_map_args args;
    struct proc *procp = proc_self();
    struct mac_ops *ops;
    int error;

    if (procp == NULL || mbp == NULL) {
        return -EINVAL;
    }

    if (res == NULL) {
        return -EINVAL;
    }

    error = mac_check_lvl(procp, mbp->level);
    if (error < 0) {
        return error;
    }

    ops = mbp->ops;
    if (ops->map == NULL) {
        return -EIO;
    }

    args.off = off;
    args.len = len;
    args.flags = flags;
    args.dp_res = res;
    return ops->map(mbp, &args);
}

/*
 * Grab a specific border using an ID
 */
struct mac_border *
mac_get_border(border_id_t id)
{
    if (id >= NELEM(bortab)) {
        return NULL;
    }

    return bortab[id];
}
