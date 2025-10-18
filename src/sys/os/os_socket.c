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

#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/syslog.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <os/vnode.h>
#include <os/kalloc.h>
#include <os/filedesc.h>
#include <string.h>

static size_t next_sockid = 1;

/*
 * Allocate a socket file descriptor
 *
 * @fdp_res: Result is written here
 *
 * XXX: Socket vnodes are never deallocated until explicitly
 *      closed
 *
 * Returns file descriptor on success, otherwise a
 * less than zero value on faulure
 */
static int
get_sock_fd(struct filedesc **fdp_res)
{
    char namebuf[256];
    struct ksocket *sock;
    struct mount *mp;
    struct nameidata nd;
    struct filedesc *fdp;
    struct proc *self = proc_self();
    struct vnode *vp;
    int error;

    if (fdp_res == NULL) {
        return -EINVAL;
    }

    if (self == NULL) {
        return -EIO;
    }

    error = mount_lookup("/tmp", &mp);
    if (error < 0) {
        printf("socket: could not get /tmp mountpoint\n");
        return error;
    }

    error = fd_alloc(self, &fdp);
    if (error < 0) {
        printf("socket: could not allocate fd\n");
        return error;
    }

    sock = kalloc(sizeof(*sock));
    if (sock == NULL) {
        fd_close(fdp->fdno);
        return -EINVAL;
    }

    /* Format the name */
    snprintf(
        namebuf,
        sizeof(namebuf),
        "sock.%d",
        next_sockid++
    );

    /* Allocate a persistent vnode */
    error = vfs_valloc(&vp, VTYPE_SOCK, 0);
    if (error < 0) {
        printf("socket: could not allocate vnode\n");
        fd_close(fdp->fdno);
        kfree(sock);
        return error;
    }

    /* Create the backing file */
    nd.path = namebuf;
    error = vop_create(mp->vp, &nd, VTYPE_SOCK);
    if (error < 0) {
        printf("socket: failed to create /tmp entry\n");
        fd_close(fdp->fdno);
        kfree(sock);
        return error;
    }

    /* Mark the socket as inactive */
    memset(sock, 0, sizeof(*sock));
    sock->backlog = -1;

    fdp->vp = vp;
    vp->data = sock;
    printf("socket: socket created @ /tmp/%s\n", namebuf);
    return fdp->fdno;
}

/*
 * Get a socket descriptor from a fd
 *
 * @fd: File descriptor to look up
 * @sock_res: Result pointer is written here
 *
 * Returns zero on success, otherwise a less
 * than zero value on failure
 */
static int
get_sock(int fd, struct ksocket **sock_res)
{
    struct filedesc *fdp;
    struct proc *self = proc_self();
    struct vnode *vp;

    if (fd < 0) {
        return -EBADF;
    }

    if (sock_res == NULL) {
        return -1;
    }

    if (self == NULL) {
        return -EINVAL;
    }

    /* Get the file descriptor */
    fdp = fd_get(
        self,
        fd
    );

    if (fdp == NULL) {
        return -EBADF;
    }

    /* We need the vnode */
    if ((vp = fdp->vp) == NULL) {
        return -EIO;
    }

    *sock_res = vp->data;
    return 0;
}

/*
 * Any backlog value > 0 marks the socket
 * as accepting connections
 */
int
listen(int socket, int backlog)
{
    struct ksocket *ksock = NULL;
    int error;

    if (socket < 0) {
        return -EBADF;
    }

    if (backlog < 0) {
        backlog = 0;
    }

    error = get_sock(socket, &ksock);
    if (error < 0) {
        printf("listen: failed to get socket descriptor\n");
        return error;
    }

    /* Set the backlog */
    ksock->backlog = backlog;
    return 0;
}

/*
 * Handles the AF_UNIX domain
 */
static int
af_unix(int type, int protocol)
{
    struct filedesc *desc;

    return get_sock_fd(&desc);
}

int
socket(int domain, int type, int protocol)
{
    if (domain < 0 || type < 0) {
        return -EINVAL;
    }

    switch (domain) {
    case AF_UNIX:
        return af_unix(type, protocol);
    }

    return -1;
}

/*
 * ARG0: Domain
 * ARG1: Type
 * ARG2: Protocol
 */
scret_t
sys_socket(struct syscall_args *scargs)
{
    int domain = SCARG(scargs, int, 0);
    int type = SCARG(scargs, int, 1);
    int protocol = SCARG(scargs, int, 2);

    return socket(domain, type, protocol);
}

/*
 * ARG0: Socket fd
 * @backlog: Maximum connections
 */
scret_t
sys_listen(struct syscall_args *scargs)
{
    int sockfd = SCARG(scargs, int, 0);
    int backlog = SCARG(scargs, int, 1);

    return listen(sockfd, backlog);
}
