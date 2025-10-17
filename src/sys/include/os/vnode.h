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

#ifndef _OS_VNODE_H_
#define _OS_VNODE_H_ 1

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/namei.h>

/* Forward declarations */
struct vnode;
struct vop;

/*
 * Valid vnode types
 *
 * @VTYPE_NONE: Vnode is yet to be assigned a type
 * @VTYPE_FILE: Vnode references a file
 * @VTYPE_DIR:  Vnode references a directory
 * @__N_VTYPE:  Number of valid nodes on the system
 */
typedef enum {
    VTYPE_NONE,
    VTYPE_FILE,
    VTYPE_DIR,
    __N_VTYPE
} vtype_t;

/*
 * Represents the arguments used in a vnode lookup
 * operation
 *
 * @name: Path componenet to look up
 * @dirvp: Current directory
 * @vpp: Resulting vnode pointer is written here
 */
struct vop_lookup_args {
    const char *name;
    struct vnode *dirvp;
    struct vnode **vpp;
};

/*
 * Represents VOP data that can be used to read
 * or write a file, etc
 *
 * @data: Buffer containing I/O data
 * @len: Length of buffer
 * @off: Offset of operation
 * @vp: Current vnode
 */
struct vop_rw_data {
    void *data;
    size_t len;
    off_t off;
    struct vnode *vp;
};

/*
 * Arguments to create an entry within a
 * filesystem
 *
 * @ndp: Path component to create
 */
struct vop_create_args {
    struct nameidata *ndp;
};

/*
 * Represents attributes of a vnode
 *
 * @size: File size in bytes
 */
struct vattr {
    size_t size;
};

/*
 * Represents operations that can be performed on
 * a specific vnode. These are implemented as callbacks
 */
struct vop {
    int(*getattr)(struct vnode *vp, struct vattr *res);
    int(*lookup)(struct vop_lookup_args *args);
    int(*reclaim)(struct vnode *vp, int flags);
    int(*create)(struct vop_create_args *args);
    ssize_t(*write)(struct vop_rw_data *data);
    ssize_t(*read)(struct vop_rw_data *data);
};

/*
 * Abstract representation of a referenced filesystem object
 * such as a file, directory, network file object,
 * etc.
 *
 * [F]: Set up by filesystem
 * [V]  Set up by VFS
 * [F/V]: Both F and V
 *
 * @refcount: How many objects have a reference
 * @type: Vnode type  [F/V]
 * @vops:  Vnode operations hooks [F]
 * @data: Filesystem specific data [F]
 */
struct vnode {
    int refcount;
    vtype_t type;
    struct vop *vops;
    void *data;
};

#define vnode_ref(VP) (atomic_inc_int(&(VP)->refcount))

/*
 * Allocate a new vnode
 *
 * @resp: Result pointer is written here
 * @type: Vnode type
 * @flags: Optional flags
 *
 * Return zero on success, otherwise a less than zero
 * on failure.
 */
int vfs_valloc(struct vnode **resp, vtype_t type, int flags);

/*
 * Deallocate / release a vnode
 *
 * @vp: Vnode to release
 * @flags: Optional flags
 *
 * Returns zero on success, otherwise less than zero
 * on failure.
 */
int vfs_vrel(struct vnode *vp, int flags);

/*
 * Wrapper for the vnode write callback
 *
 * @vp: Vnode to write to
 * @data: Data to write
 * @off: Offset to write at
 * @len: Length of bytes to write
 *
 * Returns the number of bytes written on success, otherwise
 * a less than zero value on failure.
 */
ssize_t vop_write(struct vnode *vp, char *data, off_t off, size_t len);

/*
 * Wrapper for the read write callback
 *
 * @vp: Vnode to read from
 * @data: Read data written here
 * @off: Offset to read at
 * @len: Length of bytes to read
 *
 * Returns the number of bytes read on success, otherwise
 * a less than zero value on failure.
 */
ssize_t vop_read(struct vnode *vp, char *data, off_t off, size_t len);

/*
 * Reclaim the resources tied to a specific vnode
 *
 * @vp: Vnode to reclaim
 * @flags: Optional flags
 *
 * Returns zero on success, otherwise a less than zero value
 * on failure.
 */
int vop_reclaim(struct vnode *vp, int flags);

/*
 * Create a node within a specific filesystem or
 * directory
 *
 * @vp: Vnode of parent directory
 * @ndp: Namei descriptor of path component
 *
 * Returns zero on success, otherwise a less than zero
 * value on failure.
 */
int vop_create(struct vnode *vp, struct nameidata *ndp);

/*
 * Get the attributes of a file
 *
 * @vp: Vnode of file to get attributes of
 * @res: Result of file to get attr of
 *
 * Returns zero on success, otherwise a less than
 * zero value on failure
 */
int vop_getattr(struct vnode *vp, struct vattr *res);

#endif  /* !_OS_VNODE_H_ */
