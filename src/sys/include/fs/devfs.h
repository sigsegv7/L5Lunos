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

#ifndef _FS_DEVFS_H_
#define _FS_DEVFS_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/limits.h>
#include <os/vnode.h>

/* Forward declarations */
struct devfs_node;

/*
 * Represents an I/O buffer for device files
 * on the system
 */
struct dev_iobuf {
    void *buf;
    size_t count;
};

/*
 * Character device descriptor and hooks
 *
 * @read: Read n bytes from the char device
 * @write: Write n bytes from the char device
 */
struct cdevsw {
    ssize_t(*read)(struct devfs_node *dnp, struct dev_iobuf *iob, int flags);
    ssize_t(*write)(struct devfs_node *dnp, struct dev_iobuf *iob, int flags);
};

/*
 * Describes the device node types
 *
 * @DEVFS_NONE: No type
 * @DEVFS_CDEV: Char device
 */
typedef enum {
    DEVFS_NONE,
    DEVFS_CDEV,
    __DEVFS_NTYPE
} dev_type_t;

/*
 * Represents a device filesystem node
 *
 * @name: Name of device node
 * @type: Node type
 * @cdev: Character device (if DEVFS_CDEV)
 * @link: Queue link
 */
struct devfs_node {
    char name[NAME_MAX];
    dev_type_t type;
    union {
        struct cdevsw *cdev;
        void *dev;
    };

    TAILQ_ENTRY(devfs_node) link;
};

/*
 * Register a character device
 *
 * @name: Name of character device
 * @type: Device type to use
 * @cdev: Character device operations to use
 * @flags: Optional flags
 *
 * Returns zero on success, otherwise a less than zero
 * value on failure.
 */
int devfs_register(const char *name, dev_type_t type, void *devsw, int flags);

#endif  /* !_FS_DEVFS_H_ */
