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
#include <sys/syslog.h>
#include <sys/errno.h>
#include <sys/cdefs.h>
#include <sys/mount.h>
#include <os/vfs.h>
#include <string.h>

/*
 * Represents the root mountlist, every other
 * mountpoint is put here.
 */
static struct mountlist root;

/*
 * Mount a filesystem
 */
int
mount(struct mount_args *margs, uint32_t flags)
{
    const struct vfsops *vfsops;
    struct fs_info *fip = NULL;
    int error;

    if (margs == NULL) {
        return -EINVAL;
    }

    /* XXX: Unused as of now */
    (void)margs->source;
    (void)margs->target;
    (void)margs->data;

    /* XXX: Requires fstype for now */
    if (margs->fstype == NULL) {
        return -ENOENT;
    }

    /*
     * Now we go through each entry of the filesystem
     * table and if it matches the filesystem type,
     * we're good.
     */
    for (uint8_t i = 0;; ++i) {
        error = vfs_by_index(i, &fip);
        if (error != 0) {
            return -ENOENT;
        }

        if (strcmp(fip->name, margs->fstype) == 0) {
            break;
        }
    }


    /* Sanity check */
    if (__unlikely(fip == NULL)) {
        printf("mount: got NULL filesystem info\n");
        return -ENOENT;
    }

    vfsops = fip->vfsops;
    if (__unlikely(vfsops->mount == NULL)) {
        printf("mount: fs does not implement mount!\n");
        return -EIO;
    }

    vfsops->mount(fip, margs);
    return 0;
}

/*
 * Initialize the mountlist
 */
int
mountlist_init(struct mountlist *mlp)
{
    if (mlp == NULL) {
        mlp = &root;
    }

    /* Don't initialize twice */
    if (mlp->i) {
        return -EPERM;
    }

    TAILQ_INIT(&mlp->list);
    mlp->i = 1;
    return 0;
}
