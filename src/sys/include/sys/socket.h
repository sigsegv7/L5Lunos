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

#ifndef _SYS_SOCKET_H_
#define _SYS_SOCKET_H_ 1

#include <sys/types.h>
#include <sys/syscall.h>

/* Address family defines */
#define AF_UNIX     0x00        /* Local comms / IPC */
#define AF_LOCAL    AF_UNIX     /* AF_UNIX alias */

/* Socket type defines */
#define SOCK_STREAM  0x00
#define SOCK_DGRAM   0x01

/*
 * Get a socket as a file descriptor
 *
 * @domain: Socket domain (AF_*)
 * @type: Socket type SOCK_*
 *
 * Returns file descriptor on success, otherwise
 * a less than zero value on failure
 */
int socket(int domain, int type, int protocol);

/*
 * @socket: Socket to listen on
 * @backlog: Max connections
 */
int listen(int socket, int backlog);

#if defined(_KERNEL)

/*
 * Kernel representation of a socket
 *
 * @backlog: Maximum connections (< 0 means socket not active)
 */
struct ksocket {
    int backlog;
};

/*
 * Socket syscall
 */
scret_t sys_socket(struct syscall_args *scargs);

/*
 * Listen syscall
 */
scret_t sys_listen(struct syscall_args *scargs);

#endif  /* _KERNEL */
#endif  /* !_SYS_SOCKET_H_ */
