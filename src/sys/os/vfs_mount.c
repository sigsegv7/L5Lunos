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
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/cdefs.h>
#include <sys/mount.h>
#include <os/vfs.h>
#include <os/kalloc.h>
#include <string.h>

/*
 * Represents the root mountlist, every other
 * mountpoint is put here.
 */
static struct mountlist root;

/*
 * Mount a filesystem to a specific location
 *
 * @margs: Mount arguments to use
 * @mp_res: Mountpoint result pointer written here
 * @flags: Optional flags
 */
static int
mount_to(struct mount_args *margs, struct mount **mp_res, int flags)
{
    int ncmp, error;
    const char *p, *pcur;
    struct mount *mp;
    size_t len;
    char namebuf[FSNAME_MAX];

    if (margs == NULL) {
        return -EINVAL;
    }

    /* XXX: Requires target */
    if (margs->target == NULL) {
        return -EINVAL;
    }

    /* XXX: Requires fstype */
    if (margs->fstype == NULL) {
        return -EINVAL;
    }

    ncmp = vfs_cmp_cnt(margs->target);
    if (ncmp > 1 || ncmp < 0) {
        printf("mount_to: got bad path\n");
        return -EINVAL;
    }

    p = margs->target;
    pcur = p;

    /*
     * Get all the way to the end of the first component
     * with the end pointer.
     */
    while (*pcur == '/')
        ++pcur;
    while (*pcur != '/' && *pcur != '\0')
        ++pcur;

    /* Compute the length of the first component */
    len = (size_t)PTR_NOFFSET(pcur, (uintptr_t)p);
    if (len >= sizeof(namebuf) - 1 || len == 0) {
        printf("mount_to: bad path\n");
        return -EINVAL;
    }

    memcpy(namebuf, p, len);
    namebuf[len] = '\0';

    /* Allocate the actual mountpoint */
    error = mount_alloc(namebuf, &mp);
    if (error < 0) {
        return error;
    }

    TAILQ_INSERT_TAIL(&root.list, mp, link);
    *mp_res = mp;
    return 0;
}

/*
 * Lookup a specific mountpoint
 */
int
mount_lookup(const char *name, struct mount **mp_res)
{
    struct mount *mp;

    if (name == NULL || mp_res == NULL) {
        return -EINVAL;
    }

    TAILQ_FOREACH(mp, &root.list, link) {
        if (strcmp(mp->name, name) == 0) {
            *mp_res = mp;
            return 0;
        }
    }

    return -ENOENT;
}

/*
 * Allocate a new mountpoint
 */
int
mount_alloc(const char *name, struct mount **mp_res)
{
    struct mount *mp;
    size_t len, slen;

    if (mp_res == NULL || name == NULL) {
        return -EINVAL;
    }

    mp = kalloc(sizeof(*mp));
    if (mp == NULL) {
        printf("mount_alloc: allocation failure\n");
        return -ENOMEM;
    }

    slen = strlen(name);
    len = MIN(sizeof(mp->name), slen);
    memcpy(mp->name, name, len);

    *mp_res = mp;
    return 0;
}

/*
 * Mount a filesystem
 */
int
kmount(struct mount_args *margs, uint32_t flags)
{
    const struct vfsops *vfsops;
    struct mount *mpp;
    struct fs_info *fip = NULL;
    int error;

    if (margs == NULL) {
        return -EINVAL;
    }

    /* XXX: Unused as of now */
    (void)margs->source;
    (void)margs->data;

    /* XXX: Requires fstype for now */
    if (margs->fstype == NULL) {
        return -ENOENT;
    }
    if (margs->target == NULL) {
        return -EINVAL;
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

    if ((error = vfsops->mount(fip, margs)) < 0) {
        printf("mount: fs mount failure\n");
        return error;
    }

    /* Do the actual mount */
    error = mount_to(margs, &mpp, flags);
    if (error < 0) {
        printf("mount: mount_to() returned %d\n", error);
        return error;
    }

    mpp->fs = fip;
    mpp->vp = margs->vp_res;
    return 0;
}

/*
 * Initialize the mountlist
 */
int
mountlist_init(struct mountlist *mlp)
{
    struct mount_args margs;

    memset(&margs, 0, sizeof(margs));
    margs.target = "/";
    margs.fstype = MOUNT_INITRD;

    if (mlp == NULL) {
        mlp = &root;
    }

    /* Don't initialize twice */
    if (mlp->i) {
        return -EPERM;
    }

    TAILQ_INIT(&mlp->list);
    mlp->i = 1;
    kmount(&margs, 0);
    return 0;
}
