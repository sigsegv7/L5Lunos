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

#ifndef _SYS_MAC_H_
#define _SYS_MAC_H_ 1

#include <sys/types.h>
#ifndef _KERNEL
#include <stdint.h>
#include <stddef.h>
#endif  /* !_KERNEL */

typedef enum {
    BORDER_NONE,
    BORDER_FBDEV,
    __BORDER_MAX
} border_id_t;

#ifndef _KERNEL

/*
 * Cross a resource border via MAC
 *
 * @id: ID of resource border to cross
 * @length: Length of resource
 * @off: Resource offset to request
 * @flags: Optional flags
 * @res: Result pointer is written here
 *
 * Returns the resource length on success, otherwise a less
 * than zero value on failure.
 */
ssize_t cross(
    border_id_t id, size_t length, off_t off,
    int flags, void *res
);

/*
 * Query a resource border
 *
 * @id: Border ID
 * @buf: Buffer to store data
 * @len: Length of buffer
 * @flags: Optional flags
 */
int query(border_id_t id, void *buf, size_t len, int flags);

#endif  /* _KERNEL */
#endif  /* !_SYS_MAC_H_ */
