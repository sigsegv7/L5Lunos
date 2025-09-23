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

#ifndef _MACHINE_SYSCALL_H_
#define _MACHINE_SYSCALL_H_

#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/cdefs.h>

typedef __ssize_t scarg_t;

__always_inline static inline long
syscall0(scarg_t code)
{
    volatile long ret;
    __ASMV("int $0x80" : "=a"(ret) : "a"(code));
    return ret;
}

__always_inline static inline long
syscall1(scarg_t code, scarg_t arg0)
{
    volatile long ret;
    __ASMV("int $0x80" : "=a"(ret) : "a"(code), "D"(arg0) : "memory");
    return ret;
}

__always_inline static long inline
syscall2(scarg_t code, scarg_t arg0, scarg_t arg1)
{
    volatile long ret;
    __ASMV("int $0x80" : "=a"(ret) : "a"(code), "D"(arg0), "S"(arg1) : "memory");
    return ret;
}

__always_inline static inline long
syscall3(scarg_t code, scarg_t arg0, scarg_t arg1, scarg_t arg2)
{
    volatile long ret;
    __ASMV("int $0x80" : "=a"(ret) : "a"(code), "D"(arg0), "S"(arg1), "d"(arg2) : "memory");
    return ret;
}

__always_inline static inline long
syscall4(scarg_t code, scarg_t arg0, scarg_t arg1, scarg_t arg2, scarg_t arg3)
{
    volatile long ret;
    register scarg_t _arg3 asm("r10") = arg3;
    __ASMV("int $0x80" : "=a"(ret) : "a"(code), "D"(arg0), "S"(arg1), "d"(arg2), "r"(_arg3) : "memory");
    return ret;
}

__always_inline static inline long
syscall5(scarg_t code, scarg_t arg0, scarg_t arg1, scarg_t arg2, scarg_t arg3, scarg_t arg4)
{
    volatile long ret;
    register scarg_t _arg3 asm("r10") = arg3;
    register scarg_t _arg4 asm("r9") = arg4;
    __ASMV("int $0x80" : "=a"(ret) : "a"(code), "D"(arg0), "S"(arg1), "d"(arg2), "r"(_arg3), "r"(_arg4) : "memory");
    return ret;
}

__always_inline static inline long
syscall6(scarg_t code, scarg_t arg0, scarg_t arg1, scarg_t arg2, scarg_t arg3, scarg_t arg4, scarg_t arg5)
{
    volatile long ret;
    register scarg_t _arg3 asm("r10") = arg3;
    register scarg_t _arg4 asm("r9") = arg4;
    register scarg_t _arg5 asm("r8") = arg5;
    __ASMV("int $0x80" : "=a"(ret) : "a"(code), "D"(arg0), "S"(arg1), "d"(arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5) : "memory");
    return ret;
}

#define _SYSCALL_N(a0, a1, a2, a3, a4, a5, a6, name, ...) \
    name

#define syscall(...) \
_SYSCALL_N(__VA_ARGS__, syscall6, syscall5, \
            syscall4, syscall3, syscall2, syscall1, \
            syscall0)(__VA_ARGS__)

#endif  /* !_MACHINE_SYSCALL_H_ */
