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

#ifndef _SYS_SYSCALL_H_
#define _SYS_SYSCALL_H_

#include <sys/types.h>
#include <sys/limits.h>
#if !defined(_KERNEL)
#include <machine/syscall.h>
#endif  /* _!KERNEL */

/*
 * Default syscall numbers
 *
 * Defines marked as (mandatory) must be implemented
 * between latches.
 */
#define SYS_none        0x00
#define SYS_exit        0x01
#define SYS_write       0x02
#define SYS_cross       0x03    /* cross a border (mandatory) */
#define SYS_sigaction   0x04
#define SYS_query       0x05    /* query a border (mandatory) */
#define SYS_spawn       0x06    /* spawn a process */
#define SYS_mount       0x07    /* mount a filesystem */
#define SYS_open        0x08    /* open a file */
#define SYS_muxtap      0x09    /* mux an I/O tap */
#define SYS_getargv     0x0A    /* get process argv */
#define SYS_reboot      0x0B    /* reboot the system */
#define SYS_waitpid     0x0C    /* wait for child to exit */
#define SYS_dmsio       0x0D    /* DMS I/O */
#define SYS_read        0x0E    /* read a file descriptor */
#define SYS_close       0x0F    /* close a file */
#define SYS_lseek       0x10    /* seek to end of file */

typedef __ssize_t scret_t;
typedef __ssize_t scarg_t;

#if defined(_KERNEL)
/*
 * Acquire a specific syscall argument
 */
#define SCARG(SCARGS, TYPE, SYSNO) ((TYPE)(SCARGS)->arg[(SYSNO)])

struct syscall_args {
    scarg_t arg[6];
    struct trapframe *tf;
};

/* Syscall callback */
typedef scret_t(*sccb_t)(struct syscall_args *);

/*
 * L5 supports syscall windows in where there can be a
 * given set of syscalls and whatever syscall traps into
 * kernel space will address the syscalls referenced by the
 * current focused window. The window can be slid to change
 * the type of syscall interface used by the running application.
 *
 * @sctab: Syscall table for this window
 * @nimpl: The number of syscalls implemented
 * @p: Present bit to indicate usability
 */
struct syscall_win {
    sccb_t *sctab;
    size_t nimpl;
    uint8_t p : 1;
};

/*
 * Valid platform latch constants
 */
typedef enum {
    SC_PLATCH_UNIX = 0x00,
    SC_PLATCH_L5,
    __SC_PLATCH_MAX
} platch_t;

/*
 * L5 provides the concept of "syscall domains". A syscall domain
 * is a collection of syscall windows along with a sliding index
 * known as the platform latch (`platch') to govern which syscall
 * interface is to be used.
 *
 * @slots: List of windows within this domain
 * @platch: Platform latch
 */
struct syscall_domain {
    struct syscall_win slots[SCWIN_MAX];
    platch_t platch;
};

/* Implemented platforms */
extern scret_t(*g_unix_sctab[])(struct syscall_args *);
extern const size_t UNIX_SCTAB_LEN;
#endif  /* _KERNEL */
#endif  /* !_SYS_SYSCALL_H_ */
