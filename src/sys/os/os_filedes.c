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
#include <sys/syslog.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/limits.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <os/filedesc.h>
#include <os/kalloc.h>
#include <os/systm.h>
#include <io/cons/cons.h>
#include <compat/unix/syscall.h>
#include <string.h>

#define STDOUT_FILENO 1

/*
 * Allocate a file descriptor from a specific process's
 * file descriptor table
 *
 * @procp: Process to allocate fd from
 * @fd_res: Result pointer is written here
 *
 * Returns zero on success, otherwise a less than
 * zero value upon failure
 */
static int
fd_alloc(struct proc *procp, struct filedesc **fd_res)
{
    struct filedesc *fd;

    if (procp == NULL) {
        return -EINVAL;
    }

    /*
     * Find an entry that is free and allocate a new
     * file descriptor for it.
     */
    for (int i = 0; i < FD_MAX; ++i) {
        if (procp->fdtab[i] != NULL) {
            continue;
        }

        fd = kalloc(sizeof(*fd));
        if (fd == NULL) {
            return -EINVAL;
        }

        /* Zero and assign */
        memset(fd, 0, sizeof(*fd));
        procp->fdtab[i] = fd;
        fd->fdno = i;
        if (fd_res != NULL) {
            *fd_res = fd;
        }

        return 0;
    }

    return -EMFILE;
}

/*
 * Look up a file descriptor that belongs to a specific
 * process by using its fd number
 *
 * @procp: Process to look up
 * @fd: File descriptor number
 *
 * Returns the file descriptor pointer on success,
 * otherwise a less than zero value on failure
 */
static struct filedesc *
fd_get(struct proc *procp, int fd)
{
    struct filedesc *fdp;

    if (procp == NULL) {
        return NULL;
    }

    for (int i = 0; i < FD_MAX; ++i) {
        if ((fdp = procp->fdtab[i]) == NULL) {
            continue;
        }

        break;
    }

    return fdp;
}


/*
 * Duplicate a file descriptor
 */
struct filedesc *
fd_dup(struct proc *procp, int fd)
{
    struct filedesc *new, *old;
    int error;

    old = fd_get(procp, fd);
    if (new == NULL) {
        return NULL;
    }

    /* Allocate an even newer file descriptor */
    error = fd_alloc(procp, &new);
    if (error != 0) {
        return NULL;
    }

    /* Grab a ref if we can */
    if (old->vp != NULL) {
        vnode_ref(old->vp);
    }

    new->mode = old->mode;
    new->vp = old->vp;
    return new;
}

int
fd_open(const char *path, mode_t mode)
{
    struct filedesc *fd;
    struct nameidata nd;
    struct proc *self = proc_self();
    struct vnode *vp;
    int error;

    /* We need the current proc */
    if (self == NULL) {
        return -ESRCH;
    }

    if (path == NULL) {
        return -ESRCH;
    }

    /* Allocate a new file descriptor */
    error = fd_alloc(self, &fd);
    if (error < 0) {
        return error;
    }

    /*
     * Now we try to do the lookup, we'll need
     * the vnode for file references to be
     * useful
     */
    nd.path = path;
    nd.flags = 0;
    nd.vp_res = &vp;
    error = namei(&nd);
    if (error < 0) {
        return error;
    }

    fd->vp = vp;
    fd->mode = mode;
    return fd->fdno;
}

/*
 * Initialize file descriptor table
 */
int
fdtab_init(struct proc *procp)
{
    struct filedesc *fd;

    if (procp == NULL) {
        return -EINVAL;
    }

    /* Don't do it twice */
    if (procp->fdtab[0] != NULL) {
        printf("fdtab: fd table already initialized\n");
        return -1;
    }

    fd_alloc(procp, &fd);  /* stdin */
    fd->mode = O_RDWR;     /* all can read */
    fd_dup(procp, 0);      /* stdout */
    fd_dup(procp, 0);      /* stderr */
    return 0;
}

/*
 * XXX: STUB
 */
ssize_t
write(int fd, const void *buf, size_t count)
{
    struct proc *self = proc_self();
    struct filedesc *fdp;
    int error;
    char kbuf[1024];

    if (self == NULL) {
        return -ESRCH;
    }

    /* Must be valid */
    error = proc_check_addr(proc_self(), (uintptr_t)buf, count);
    if (error < 0) {
        return error;
    }

    /* Get the file descriptor structure */
    fdp = fd_get(self, fd);
    if (fdp == NULL) {
        return -EBADF;
    }

    /* We need to be able to write it */
    if ((fdp->mode & (O_WRONLY | O_RDWR)) == 0) {
        return -EPERM;
    }

    memcpy(kbuf, buf, count);
    switch (fd) {
    case STDOUT_FILENO:
        cons_putstr(
            &g_root_scr, kbuf,
            count
        );
        break;
    default:
        return -EBADF;
    }

    return count;
}

/*
 * ARG0: Path
 * ARG1: Mode
 */
scret_t
sys_open(struct syscall_args *scargs)
{
    const char *u_path = SCARG(scargs, const char *, 0);
    mode_t mode = SCARG(scargs, mode_t, 1);
    char pathbuf[PATH_MAX];
    int error;

    error = copyinstr(u_path, pathbuf, PATH_MAX);
    if (error < 0) {
        return error;
    }

    return fd_open(pathbuf, mode);
}
