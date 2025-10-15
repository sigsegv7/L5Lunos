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

#ifndef _SYS_DMS_H_
#define _SYS_DMS_H_ 1

#include <sys/types.h>
#if !defined(_KERNEL)
#include <stdint.h>
#include <stddef.h>
#endif  /* !_KERNEL */

#define DISKNAME_MAX 128    /* Disk name maxlen */

#define DMS_OPC_READ    0x00    /* Read from drive */
#define DMS_OPC_WRITE   0x01    /* Write to drive */
#define DMS_OPC_QUERY   0x02    /* Query a drive */

/* ID of the disk */
typedef uint16_t disk_id_t;

/*
 * Represents disk information that can be requested
 * from a device with a query
 *
 * @name: Name of disk
 * @bsize: Block size
 * @id: ID of disk
 */
struct dms_diskinfo {
    char name[DISKNAME_MAX];
    uint16_t bsize;
    disk_id_t id;
};

/*
 * Represents data that can be sent between the
 * DMS framework and certain user applications
 *
 * @id: ID of disk to operate on
 * @opcode: Operation code
 * @buf: Data buffer (direction depends on opcode)
 * @offset: Offset to perform operation at (depends on opcode)
 * @len: Length of buffer
 */
struct dms_frame {
    uint16_t id;
    uint8_t opcode;
    void *buf;
    off_t offset;
    size_t len;
};

/*
 * Perform I/O on a specific disk
 *
 * @dfp: DMS frame
 */
ssize_t dms_mux(struct dms_frame *dfp);

#endif  /* !_SYS_DMS_H_ */
