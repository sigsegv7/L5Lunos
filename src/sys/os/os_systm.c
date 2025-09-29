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

#include <sys/proc.h>
#include <sys/errno.h>
#include <os/systm.h>
#include <string.h>

/*
 * Safe copy in function
 */
int
copyin(const void *uaddr, void *kaddr, size_t len)
{
    struct proc *self = proc_self();
    int error;

    if (kaddr == NULL || uaddr == NULL) {
        return -EINVAL;
    }

    if (len == 0) {
        return -EINVAL;
    }

    if (self == NULL) {
        return -EIO;
    }

    error = proc_check_addr(self, (uintptr_t)uaddr, len);
    if (error < 0) {
        return error;
    }

    memcpy(kaddr, uaddr, len);
    return 0;
}

/*
 * Safe copy out function
 */
int
copyout(const void *kaddr, void *uaddr, size_t len)
{
    struct proc *self = proc_self();
    int error;

    if (kaddr == NULL || uaddr == NULL) {
        return -EINVAL;
    }

    if (len == 0) {
        return -EINVAL;
    }

    if (self == NULL) {
        return -EIO;
    }

    error = proc_check_addr(self, (uintptr_t)uaddr, len);
    if (error < 0) {
        return error;
    }

    memcpy(uaddr, kaddr, len);
    return 0;
}
