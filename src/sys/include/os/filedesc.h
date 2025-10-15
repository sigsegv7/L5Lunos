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

#ifndef _OS_FILEDESC_H_
#define _OS_FILEDESC_H_ 1

#include <sys/types.h>
#include <os/vnode.h>

struct proc;

/*
 * Represents a file descriptor
 *
 * @fdno: File descriptor index
 * @vp: Vnode this fd is linked with
 * @mode: File attributes
 */
struct filedesc {
    int fdno;
    struct vnode *vp;
    mode_t mode;
};

/*
 * Duplicate a file descriptor
 *
 * @procp: Process to duplicate from
 * @fd: File descriptor to duplicate
 *
 * Returns the new file descriptor on success, otherwise
 * a value of NULL on failure
 */
struct filedesc *fd_dup(struct proc *procp, int fd);

/*
 * Initialize a process file descriptor table
 * and set up standard streams
 *
 * @procp: Process of fd table to init
 *
 * Returns zero on success, less than zero values
 * indicate failure.
 */
int fdtab_init(struct proc *procp);

/*
 * Open a file
 *
 * @path: Path to file in which we wish to open
 * @mode: Mode of our desired file
 *
 * Returns the file descriptor on success,
 * otherwise a less than zero value on failure.
 */
int fd_open(const char *path, mode_t mode);

/*
 * Write to a file descriptor
 *
 * @fd: File descriptor to write to
 * @buf: Buffer to write
 * @count: Buffer byte count
 *
 * Returns the number of bytes written on success, otherwise
 * a less than zero value on error.
 */
ssize_t write(int fd, const void *buf, size_t count);

/*
 * Read a file descriptor
 *
 * @fd: File descriptor to read from
 * @buf: Buffer to read into
 * @count: Number of bytes to read
 *
 * Returns the number of bytes read on success, otherwise a less
 * than zero value on error.
 */
ssize_t read(int fd, void *buf, size_t count);

#endif  /* !_OS_FILEDESC_H_ */
