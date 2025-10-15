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

#include <sys/syscall.h>
#include <sys/limits.h>
#include <sys/syslog.h>
#include <sys/mount.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <os/kalloc.h>
#include <os/systm.h>
#include <os/filedesc.h>
#include <compat/unix/syscall.h>

/*
 * Write syscall
 *
 * ARG0: (int)fd
 * ARG1: (const void *)buf
 * ARG2: (size_t)count
 */
scret_t
sys_write(struct syscall_args *scargs)
{
    int fd = SCARG(scargs, int, 0);
    int error;
    const void *buf = SCARG(scargs, const void *, 1);
    size_t count = SCARG(scargs, size_t, 2);
    char kbuf[1024];

    error = copyin(buf, kbuf, MIN(count, sizeof(kbuf)));
    if (error < 0) {
        printf("sys_write: copyin() bad pointer\n");
        return -EFAULT;
    }

    return write(fd, kbuf, count);
}

/*
 * ARG0: Source
 * ARG1: Target
 * ARG2: Filesystem type
 * ARG3: Mountflags
 * ARG4: Data
 */
scret_t
sys_mount(struct syscall_args *scargs)
{
    struct mount_args args;
    char source[NAME_MAX];
    char target[NAME_MAX];
    char fstype[FSNAME_MAX];
    const char *u_source = SCARG(scargs, const char *, 0);
    const char *u_target = SCARG(scargs, const char *, 1);
    const char *u_fstype = SCARG(scargs, const char *, 2);
    unsigned long u_mountflags = SCARG(scargs, unsigned long, 3);
    int error;

    /* Get the filesystem source */
    error = copyinstr(u_source, source, sizeof(source));
    if (error < 0) {
        source[0] = '\0';
    }

    /* Get the filesystem target */
    error = copyinstr(u_target, target, sizeof(target));
    if (error < 0) {
        return error;
    }

    /* Get the filesystem type */
    error = copyinstr(u_fstype, fstype, sizeof(u_fstype));
    if (error < 0) {
        return error;
    }

    args.source = source;
    args.target = target;
    args.fstype = fstype;
    return kmount(&args, u_mountflags);
}

/*
 * ARG0: fd
 * ARG1: buf[]
 * ARG2: count
 */
scret_t
sys_read(struct syscall_args *scargs)
{
    int fd = SCARG(scargs, int, 0);
    char *u_buf = SCARG(scargs, char *, 1);
    size_t count = SCARG(scargs, size_t, 2);
    char *kbuf;
    ssize_t retval;
    int error;

    /* Do we have enough memory to fulfill this */
    kbuf = kalloc(count);
    if (kbuf == NULL) {
        return -ENOMEM;
    }

    retval = read(fd, kbuf, count);
    if (retval < 0) {
        kfree(kbuf);
        return retval;
    }

    error = copyout(kbuf, u_buf, count);
    kfree(kbuf);
    return (error == 0) ? retval : error;
}
