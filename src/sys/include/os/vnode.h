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
 * Represents operations that can be performed on
 * a specific vnode. These are implemented as callbacks
 */
struct vop {
    int(*lookup)(struct vop_lookup_args *args);
};

/*
 * Abstract representation of a referenced filesystem object
 * such as a file, directory, network file object,
 * etc.
 *
 * [D]: Set up by driver
 * [V]  Set up by VFS
 * [D/V]: Both D and V
 *
 * @vtype: Vnode type  [D/V]
 * @vops:  Vnode operations hooks [D]
 */
struct vnode {
    vtype_t vtype;
    struct vop *vops;
};

#endif  /* !_OS_VNODE_H_ */
