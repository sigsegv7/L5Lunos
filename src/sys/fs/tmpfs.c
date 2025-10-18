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
#include <sys/mount.h>
#include <sys/syslog.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/param.h>
#include <os/vnode.h>
#include <os/kalloc.h>
#include <string.h>

#define TMPFS_NAMEMAX 128
#define TMPFS_INIT_SIZE 8

/*
 * Represents a single tmpfs node
 */
struct tmpfs_node {
    char name[TMPFS_NAMEMAX];
    char *data;
    size_t len;
    size_t real_len;
    int ref;
    vtype_t vtype;  /* vnode vtype mapping */
    TAILQ_ENTRY(tmpfs_node) link;
};

#define tmpfs_ref(TMP_NODEP) \
    atomic_inc_int(&(TMP_NODEP)->ref)

static TAILQ_HEAD(, tmpfs_node) tmpfs;
static struct vop tmpfs_vops;

/*
 * Create a new tmpfs node
 *
 * @name: Name of node to create
 * @type: Type of vnode file should be associated with
 * @np_res: Result pointer is written here
 *
 * Returns zero on success, otherwise a less than
 * zero value on failure
 */
static int
tmpfs_new(const char *name, vtype_t type, struct tmpfs_node **np_res)
{
    struct tmpfs_node *np;
    size_t name_len;

    if (name == NULL) {
        return -EINVAL;
    }

    name_len = strlen(name);
    if (name_len > sizeof(np->name) - 1) {
        return -ENAMETOOLONG;
    }

    np = kalloc(sizeof(*np));
    if (np == NULL) {
        return -ENOMEM;
    }

    memset(np, 0, sizeof(*np));
    np->data = kalloc(TMPFS_INIT_SIZE);
    if (np->data == NULL) {
        kfree(np);
        return -ENOMEM;
    }

    np->real_len = 0;
    np->len = TMPFS_INIT_SIZE;
    np->ref = 1;
    np->vtype = type;
    memset(np->data, 0, TMPFS_INIT_SIZE);
    memcpy(np->name, name, name_len);

    if (np_res != NULL) {
        *np_res = np;
    }

    TAILQ_INSERT_TAIL(&tmpfs, np, link);
    return 0;
}

/*
 * Lookup a tmpfs node by name
 *
 * @name: Name to lookup
 * @np_res: Result is written here
 */
static int
tmpfs_byname(const char *name, struct tmpfs_node **np_res)
{
    struct tmpfs_node *npcur;
    int retval = -ENOENT;

    if (name == NULL || np_res == NULL) {
        return -EINVAL;
    }

    TAILQ_FOREACH(npcur, &tmpfs, link) {
        if (npcur == NULL) {
            continue;
        }

        if (strcmp(npcur->name, name) == 0) {
            retval = 0;
            *np_res = npcur;
            break;
        }
    }

    return retval;
}

/*
 * VFS tmpfs lookup callback
 */
static int
tmpfs_lookup(struct vop_lookup_args *args)
{
    struct tmpfs_node *np;
    struct vnode *vp;
    int error;

    /* Sanity checks */
    if (args == NULL)
        return -EINVAL;
    if (args->name == NULL)
        return -EINVAL;
    if (args->vpp == NULL)
        return -EINVAL;

    error = tmpfs_byname(args->name, &np);
    if (error < 0) {
        return error;
    }

    error = vfs_valloc(&vp, np->vtype, 0);
    if (error < 0) {
        return error;
    }

    /* Ref that node and give it */
    tmpfs_ref(np);
    vp->data = np;
    vp->vops = &tmpfs_vops;
    *args->vpp = vp;
    return 0;
}

/*
 * Create a tmpfs node
 */
static int
tmpfs_create(struct vop_create_args *args)
{
    struct nameidata *ndp;
    struct tmpfs_node *np = NULL;

    if (args == NULL) {
        return -EINVAL;
    }

    if ((ndp = args->ndp) == NULL) {
        return -EINVAL;
    }

    /* Does it already exist? */
    if (tmpfs_byname(ndp->path, &np) == 0) {
        return -EEXIST;
    }

    /* Only accept the types we support */
    switch (args->vtype) {
    case VTYPE_FILE:
        break;
    default:
        return -ENOTSUP;
    }

    return tmpfs_new(ndp->path, args->vtype, NULL);
}

/*
 * Initialize the temporary filesystem
 */
static int
tmpfs_init(struct fs_info *fip)
{
    TAILQ_INIT(&tmpfs);
    return 0;
}

/*
 * Mount the filesystem
 */
static int
tmpfs_mount(struct fs_info *fip, struct mount_args *margs)
{
    int error;
    struct vnode *vp;

    if (fip == NULL || margs == NULL) {
        return -1;
    }

    /* Allocate a vnode for this mountpoint */
    error = vfs_valloc(&margs->vp_res, VTYPE_DIR, 0);
    if (error < 0) {
        return error;
    }

    vp = margs->vp_res;
    vp->vops = &tmpfs_vops;
    return 0;
}

/*
 * Write to the filesystem
 */
static ssize_t
tmpfs_write(struct vop_rw_data *data)
{
    struct vnode *vp;
    struct tmpfs_node *np;
    char *dest;
    void *p;
    size_t node_len, len;
    size_t overflow_window;

    if (data == NULL) {
        return -EINVAL;
    }

    if ((vp = data->vp) == NULL) {
        return -EIO;
    }

    /* We need the vnode for lengths */
    if ((np = vp->data) == NULL) {
        return -EIO;
    }

    /*
     * Check if there is going to be any overflows
     * and if so, get the overflow window and expand
     * the buffer by it.
     */
    len = data->len;
    if ((len + data->off) > np->len) {
        overflow_window = (len + data->off) - np->len;
        np->len += overflow_window + 1;

        p = np->data;
        np->data = krealloc(np->data, np->len);
        if (np->data == NULL) {
            np->data = p;
            return -ENOMEM;
        }
    }

    np->real_len += len;
    node_len = np->len;
    dest = np->data + data->off;
    memcpy(dest, data->data, len);
    return len;
}

static ssize_t
tmpfs_read(struct vop_rw_data *data)
{
    struct vnode *vp;
    struct tmpfs_node *np;
    size_t len;
    char *dest, *src;

    if (data == NULL) {
        return -1;
    }

    if ((vp = data->vp) == NULL) {
        return -EIO;
    }

    if ((np = vp->data) == NULL) {
        return -EIO;
    }

    /* Return EOF if the offset is too far */
    len = data->len;
    if (data->off >= np->real_len) {
        return 0;   /* EOF */
    }

    /* Is there any data to read? */
    if ((len + data->off) > np->len) {
        len = np->len;
    }
    if (len == 0) {
        return 0;
    }

    dest = data->data;
    src = np->data + data->off;
    memcpy(dest, src, len);
    return len;
}

static int
tmpfs_getattr(struct vnode *vp, struct vattr *res)
{
    struct tmpfs_node *np;

    if (vp == NULL || res == NULL) {
        return -EINVAL;
    }

    if ((np = vp->data) == NULL) {
        return -EIO;
    }

    res->size = np->real_len;
    return 0;
}

static int
tmpfs_reclaim(struct vnode *vp, int flags)
{
    return 0;
}

static struct vop tmpfs_vops = {
    .lookup = tmpfs_lookup,
    .create = tmpfs_create,
    .reclaim = tmpfs_reclaim,
    .write = tmpfs_write,
    .read = tmpfs_read,
    .getattr = tmpfs_getattr
};

struct vfsops g_tmpfs_vfsops = {
    .init = tmpfs_init,
    .mount = tmpfs_mount,
};
