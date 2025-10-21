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
 * Description: Device filesystem
 * Author: Ian Marco Moffett
 */

#include <sys/errno.h>
#include <sys/mount.h>
#include <sys/syslog.h>
#include <os/vnode.h>
#include <os/kalloc.h>
#include <fs/devfs.h>
#include <string.h>

static struct vop devfs_vops;
static TAILQ_HEAD(, devfs_node) nodelist;

/*
 * Find a node within the device filesystem
 */
static int
devfs_lookup(struct vop_lookup_args *args)
{
    int error;
    struct devfs_node *dnp = NULL;
    struct vnode *vp;

    if (args == NULL) {
        return -EINVAL;
    }

    /* Need a pointer to write the result */
    if (args->vpp == NULL) {
        return -EINVAL;
    }

    TAILQ_FOREACH(dnp, &nodelist, link) {
        if (dnp == NULL) {
            continue;
        }

        if (strcmp(dnp->name, args->name) != 0) {
            continue;
        }

        /* Found it! Create a vnode */
        error = vfs_valloc(&vp, VTYPE_CDEV, 0);
        if (error < 0) {
            return error;
        }

        vp->data = dnp;
        vp->vops = &devfs_vops;
        *args->vpp = vp;
        return 0;
    }

    return -ENOENT;
}

/*
 * Register a new device to devfs
 */
int
devfs_register(const char *name, dev_type_t type, void *devsw, int flags)
{
    struct devfs_node *dnp;
    size_t devname_len;
    struct vop_lookup_args args;
    struct vnode *vp;

    if (name == NULL || devsw == NULL) {
        return -EINVAL;
    }

    if (type >= __DEVFS_NTYPE) {
        return -EINVAL;
    }

    dnp = kalloc(sizeof(*dnp));
    if (dnp == NULL) {
        return -ENOMEM;
    }

    switch (type) {
    case DEVFS_NONE:
    case DEVFS_CDEV:
        dnp->type = type;
        break;
    }

    dnp->dev = devsw;
    devname_len = strlen(name);
    memcpy(dnp->name, name, devname_len);
    TAILQ_INSERT_TAIL(&nodelist, dnp, link);
    return 0;
}

/*
 * Read a character device
 */
static int
devfs_cdev_read(struct devfs_node *dnp, struct dev_iobuf *iobuf, int flags)
{
    struct cdevsw *cdev;

    if (dnp == NULL || iobuf == NULL) {
        return -EINVAL;
    }

    if ((cdev = dnp->cdev) == NULL) {
        return -EIO;
    }

    return cdev->read(dnp, iobuf, flags);
}

/*
 * VFS read callback for devfs
 */
static ssize_t
devfs_read(struct vop_rw_data *args)
{
    struct vnode *vp;
    struct devfs_node *dnp;
    struct dev_iobuf iobuf;

    if ((vp = args->vp) == NULL) {
        return -EIO;
    }

    if ((dnp = vp->data) == NULL) {
        return -EIO;
    }

    iobuf.buf = args->data;
    iobuf.count = args->len;
    iobuf.off = args->off;
    if (dnp->type == DEVFS_CDEV) {
        return devfs_cdev_read(dnp, &iobuf, 0);
    }

    return -EIO;
}

/*
 * Initialize the device filesystem
 */
static int
devfs_init(struct fs_info *fip)
{
    (void)fip;
    TAILQ_INIT(&nodelist);
    return 0;
}

/*
 * Mount the device filesystem
 */
static int
devfs_mount(struct fs_info *fip, struct mount_args *margs)
{
    int error;
    struct vnode *vp;

    if (fip == NULL || margs == NULL) {
        return -1;
    }

    error = vfs_valloc(&margs->vp_res, VTYPE_DIR, 0);
    if (error < 0) {
        return error;
    }

    vp = margs->vp_res;
    vp->vops = &devfs_vops;
    return 0;
}

static struct vop devfs_vops = {
    .lookup = devfs_lookup,
    .read = devfs_read
};

struct vfsops g_devfs_vfsops = {
    .init = devfs_init,
    .mount = devfs_mount
};
