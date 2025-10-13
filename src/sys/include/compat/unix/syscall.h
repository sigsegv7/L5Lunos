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

#ifndef _UNIX_SYSCALL_H_
#define _UNIX_SYSCALL_H_ 1

#include <sys/proc.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <os/iotap.h>
#include <os/reboot.h>

/*
 * Exit the current process - exit(2) syscall
 */
scret_t sys_exit(struct syscall_args *scargs);

/*
 * Write to a file descriptor - write(2) syscall
 */
scret_t sys_write(struct syscall_args *scargs);

/*
 * Cross a resource border - L5 mandatory
 */
scret_t sys_cross(struct syscall_args *scargs);

/*
 * Query a syscall border - L5 mandatory
 */
scret_t sys_query(struct syscall_args *scargs);

/*
 * Open a file
 */
scret_t sys_open(struct syscall_args *scargs);

#ifdef _NEED_UNIX_SCTAB
scret_t(*g_unix_sctab[])(struct syscall_args *) = {
    [SYS_none]   = NULL,
    [SYS_exit]   = sys_exit,
    [SYS_write]  = sys_write,
    [SYS_cross]  = sys_cross,
    [SYS_query]  = sys_query,
    [SYS_spawn]  = sys_spawn,
    [SYS_mount]  = sys_mount,
    [SYS_open]   = sys_open,
    [SYS_muxtap] = sys_muxtap,
    [SYS_getargv] = sys_getargv,
    [SYS_reboot]  = sys_reboot,
    [SYS_waitpid] = sys_waitpid
};

#endif  /* !_NEED_UNIX_SCTAB */
#endif  /* !_UNIX_SYSCALL_H_ */
