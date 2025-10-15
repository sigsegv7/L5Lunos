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

#ifndef _DMS_DMS_H_
#define _DMS_DMS_H_

#include <sys/queue.h>
#include <sys/cdefs.h>
#include <sys/types.h>

/*
 * Maximum attached disks, from kconf
 */
#if defined(__DMS_MAX_DISKS)
#define DMS_MAX_DISKS __DMS_MAX_DISKS
#else
#define DMS_MAX_DISKS 32
#endif  /* __DMS_MAX_DISKS */

__static_assert(
    DMS_MAX_DISKS > 0,
    "maxiumum disks cannot be zro!"
);

/* Parameters */
#define DISKNAME_MAX 128    /* Disk name maxlen */

/* ID of the disk */
typedef uint16_t disk_id_t;

/* Forward declarations */
struct dms_disk;

/*
 * Operations that may be performed on a
 * disk drive
 *
 * XXX: `off' and `len' are byte relative
 */
struct dms_ops {
    ssize_t(*write)(struct dms_disk *dp, void *p, off_t off, size_t len);
    ssize_t(*read)(struct dms_disk *dp, void *p, off_t off, size_t len);
};

/*
 * Represents a storage device associated
 * with the system
 *
 * @name: Name of this device
 * @ops: Operations that can be performed
 * @data: Driver specific data  [set by driver]
 * @bsize: Disk block size      [set by driver]
 * @id: ID of the disk
 * @link: Internal queue link
 *
 * XXX: The `data' and `bsize' fields must be set by
 *      the driver
 */
struct dms_disk {
    char name[DISKNAME_MAX];
    struct dms_ops *ops;
    void *data;
    uint16_t bsize;
    disk_id_t id;
    TAILQ_ENTRY(dms_disk) link;
};

/*
 * Write to a disk
 *
 * @dp: Disk to write to
 * @p: Input buffer to write
 * @off: Byte relative offset
 * @len: Byte relative length
 *
 * Returns the length written on success, otherwise a less
 * than zero errno on failure
 */
ssize_t dms_write(struct dms_disk *dp, void *p, off_t off, size_t len);

/*
 * Read from a disk
 *
 * @dp: Disk to read from
 * @p: Buffer to read into
 * @off: Byte relative offset
 * @len: Byte relative length
 *
 * Returns the length read on success, otherwise a less
 * than zero errno on failure
 */
ssize_t dms_read(struct dms_disk *dp, void *p, off_t off, size_t len);

/*
 * Register a device to the DMS core
 *
 * @name: Name of device to register
 * @ops: DMS operations associated with device
 * @res: Result is written here [NULLable]
 *
 * Returns zero on success, otherwise a less than zero
 * value on failure
 */
int dms_register(const char *name, struct dms_ops *ops, struct dms_disk **res);

/*
 * Retrieve a disk from the DMS core
 *
 * @disk_id: ID of disk to retrieve
 *
 * Returns the disk descriptor when found, otherwise
 * NULL on failure or when not found
 */
struct dms_disk *dms_get(disk_id_t disk_id);

#endif  /* !_DMS_DMS_H_ */
