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

#ifndef _SYS_MOUNT_H_
#define _SYS_MOUNT_H_

#include <sys/queue.h>
#include <sys/types.h>
#include <os/vnode.h>

#if defined(_KERNEL)

/*
 * Number of bytes allowed in a filesystem
 * name including the null termination
 */
#define FSNAME_MAX 16

/*
 * Mount filesystem string names
 */
#define MOUNT_INITRD "initrd"   /* Initial ramdisk */

/* Forward declarations */
struct vfsops;
struct mount;

/* Filesystem vfsops */
extern struct vfsops g_omar_vfsops;

/*
 * Represents a mountpoint
 *
 * @vp: Vnode of mount
 * @name: Mountname
 * @link: TAILQ link
 */
struct mount {
    struct vnode *vp;
    char name[FSNAME_MAX];
    TAILQ_ENTRY(mount) link;
};

/*
 * Represents a list of mountpoints
 *
 * @i: Initialized (set if initialized)
 */
struct mountlist {
    uint8_t i : 1;
    TAILQ_HEAD(, mount) list;
};
/*
 * Arguments for mount()
 *
 * @vp_res: Vnode result
 * @source: Specifies the source filesystem to mount
 * @target: Specifies the location to mount source
 * @fstype: File system type
 * @data: Filesystem specific data
 */
struct mount_args {
    struct vnode *vp_res;
    const char *source;
    const char *target;
    const char *fstype;
    const void *data;
};

/*
 * Represents information about a filesystem
 *
 * @name: Filesystem type name
 * @vfsops: VFS operations vector
 * @refcount: Mount count of this type
 */
struct fs_info {
    char name[FSNAME_MAX];
    const struct vfsops *vfsops;
    int refcount;
};

/*
 * VFS operations vector
 *
 * @init: Initialize the filesystem
 * @mount: Mount a filesystem
 */
struct vfsops {
    int(*init)(struct fs_info *fip);
    int(*mount)(struct fs_info *fip, struct mount_args *margs);
};

/*
 * Mount a specific filesystem
 *
 * @margs: Mount arguments
 * @flags: Mount flags
 *
 * Returns zero on success, otherwise a less than zero
 * failure upon failure.
 */
int mount(struct mount_args *margs, uint32_t flags);

/*
 * Initialize a mountpoint to a known state
 *
 * @mp: Pointer to mountlist [if NULL, use root]
 *
 * Returns zero on success, otherwise a less than
 * zero value on failure.
 */
int mountlist_init(struct mountlist *mlp);

/*
 * Allocate a new mountpoint
 *
 * @name: The name to allocate mountpoint as
 * @mp_res: Result pointer is written here
 *
 * Returns zero on success, otherwise a less than
 * zero value to indicate failure.
 */
int mount_alloc(const char *name, struct mount **mp_res);

#endif  /* !_KERNEL */
#endif  /* !_SYS_MOUNT_H_ */
