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
#include <sys/atomic.h>
#include <os/vnode.h>
#include <os/kalloc.h>
#include <os/vfs.h>
#include <string.h>

/*
 * Returns a value of zero if a character value
 * within a path is valid, otherwise a less than
 * zero value on error.
 */
static int
vfs_pathc_valid(char c)
{
    /* Handle [A-Za-z] */
    if (c >= 'a' && c <= 'z')
        return 0;
    if (c >= 'A' && c <= 'Z')
        return 0;

    /* Handle [0-9] and '/' */
    if (c >= '0' && c <= '9')
        return 0;
    if (c == '/')
        return 0;

    return -1;

}

/*
 * Allocate a new vnode
 */
int
vfs_valloc(struct vnode **resp, vtype_t type, int flags)
{
    struct vnode *vp;

    if (resp == NULL) {
        return -EINVAL;
    }

    /* Attempt to allocate a vnode */
    vp = kalloc(sizeof(*vp));
    if (vp == NULL) {
        return -ENOMEM;
    }

    memset(vp, 0, sizeof(*vp));
    vp->refcount = 1;
    vp->type = type;
    *resp = vp;
    return 0;
}

int
vfs_vrel(struct vnode *vp, int flags)
{
    if (vp == NULL) {
        return -EINVAL;
    }

    if (atomic_dec_int(&vp->refcount) > 1) {
        return 0;
    }
    kfree(vp);
    return 0;
}


/*
 * Count number of components
 */
int
vfs_cmp_cnt(const char *path)
{
    const char *p;
    int error, cnt = -1;

    if (path == NULL) {
        return -EINVAL;
    }

    if (*path != '/') {
        return -ENOENT;
    }

    /* Is pointing to root? */
    if (path[0] == '/' && path[1] == '\0') {
        return 1;
    }

    p = path;
    cnt = 0;

    while (*p != '\0') {
        while (*p == '/')
            ++p;
        while (*p != '/' && *p != '\0')
            error = vfs_pathc_valid(*p++);

        /* Invalid character? */
        if (error < 0)
            return -EINVAL;

        /* Break if we got NUL */
        if (*p == '\0')
            break;

        ++p, ++cnt;
    }

    return cnt + 1;
}

ssize_t
vop_write(struct vnode *vp, char *data, off_t off, size_t len)
{
    struct vop_rw_data rwdata;
    struct vop *vops;

    if (vp == NULL || data == NULL) {
        return -EINVAL;
    }

    if (len == 0) {
        return -EINVAL;
    }

    /* Grab the virtual operations */
    if ((vops = vp->vops) == NULL) {
        return -EIO;
    }

    if (vops->write == NULL) {
        return -ENOTSUP;
    }

    rwdata.data = data;
    rwdata.len = len;
    rwdata.vp = vp;
    rwdata.off = off;
    return vops->write(&rwdata);
}

ssize_t
vop_read(struct vnode *vp, char *data, off_t off, size_t len)
{
    struct vop_rw_data rwdata;
    struct vop *vops;

    if (vp == NULL || data == NULL) {
        return -EINVAL;
    }

    if (len == 0) {
        return -EINVAL;
    }

    /* Grab the virtual operations */
    if ((vops = vp->vops) == NULL) {
        return -EIO;
    }

    if (vops->read == NULL) {
        return -ENOTSUP;
    }

    rwdata.data = data;
    rwdata.len = len;
    rwdata.vp = vp;
    rwdata.off = off;
    return vops->read(&rwdata);
}

int
vop_reclaim(struct vnode *vp, int flags)
{
    struct vop *vops;

    if (vp == NULL) {
        return -EINVAL;
    }

    /* Grab the virtual operations */
    if ((vops = vp->vops) == NULL) {
        return -EIO;
    }

    if (vops->reclaim == NULL) {
        return -ENOTSUP;
    }

    return vops->reclaim(vp, flags);
}

int
vop_create(struct vnode *vp, struct nameidata *ndp)
{
    struct vop *vops;
    struct vop_create_args args;

    if (vp == NULL || ndp == NULL) {
        return -EINVAL;
    }

    if ((vops = vp->vops) == NULL) {
        return -EIO;
    }

    if (vops->create == NULL) {
        return -EIO;
    }

    args.ndp = ndp;
    return vops->create(&args);
}
