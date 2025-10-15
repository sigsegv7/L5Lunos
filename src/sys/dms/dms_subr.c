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
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/param.h>
#include <os/kalloc.h>
#include <dms/dms.h>
#include <string.h>
#include <stdbool.h>

static TAILQ_HEAD(, dms_disk) diskq;
static bool diskq_is_init = false;
static disk_id_t next_id = 0;

int
dms_register(const char *name, struct dms_ops *ops, struct dms_disk **res)
{
    struct dms_disk *dp;
    size_t name_len;

    if (name == NULL || ops == NULL) {
        return -EINVAL;
    }

    if (!diskq_is_init) {
        TAILQ_INIT(&diskq);
        diskq_is_init = true;
    }

    dp = kalloc(sizeof(*dp));
    if (dp == NULL) {
        return -ENOMEM;
    }

    name_len = strlen(name);
    if (name_len >= DISKNAME_MAX - 1) {
        kfree(dp);
        return -ENAMETOOLONG;
    }

    /* Copy the name and set the ID */
    memcpy(dp->name, name, name_len + 1);
    dp->id = next_id++;
    dp->ops = ops;
    TAILQ_INSERT_TAIL(&diskq, dp, link);

    if (res != NULL) {
        *res = dp;
    }
    return 0;
}

struct dms_disk *
dms_get(disk_id_t disk_id)
{
    struct dms_disk *dp = NULL;
    struct dms_disk *dp_cur;

    TAILQ_FOREACH(dp_cur, &diskq, link) {
        if (dp_cur == NULL) {
            continue;
        }

        if (dp_cur->id == disk_id) {
            dp = dp_cur;
            break;
        }
    }

    return dp_cur;
}

ssize_t
dms_write(struct dms_disk *dp, void *p, off_t off, size_t len)
{
    void *buf;
    size_t real_len;
    int error;
    struct dms_ops *ops;

    if (dp == NULL || p == NULL) {
        return -EINVAL;
    }

    if ((ops = dp->ops) == NULL) {
        return -EIO;
    }

    if (ops->write == NULL) {
        return -ENOTSUP;
    }

    real_len = ALIGN_UP(len, dp->bsize);
    if ((buf = kalloc(real_len)) == NULL) {
        return -ENOMEM;
    }

    memset(buf, 0, real_len);
    memcpy(buf, p, len);
    error = ops->write(dp, buf, off, real_len);
    kfree(buf);
    return error;
}

ssize_t
dms_read(struct dms_disk *dp, void *p, off_t off, size_t len)
{
    void *buf;
    struct dms_ops *ops;
    size_t real_len;
    int error;

    if (dp == NULL || p == NULL) {
        return -EINVAL;
    }

    if ((ops = dp->ops) == NULL) {
        return -EIO;
    }

    if (ops->read == NULL) {
        return -ENOTSUP;
    }

    /* Allocate a real sized buffer */
    real_len = ALIGN_UP(len, dp->bsize);
    if ((buf = kalloc(real_len)) == NULL) {
        return -ENOMEM;
    }

    memset(buf, 0, real_len);
    error =  ops->read(dp, buf, off, real_len);
    if (error < 0) {
        kfree(buf);
        return error;
    }

    memcpy(p, buf, len);
    kfree(buf);
    return 0;
}
