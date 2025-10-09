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

#ifndef _SYS_IOTAP_H_
#define _SYS_IOTAP_H_ 1

#include <sys/types.h>
#include <sys/cdefs.h>
#if !defined(_KERNEL)
#include <stdint.h>
#include <stddef.h>
#endif  /* _KERNEL */

/* Valid I/O tap opcodes */
#define IOTAP_OPC_READ       0x0000

/*
 * An I/O tap message that can be sent to
 * the kernel to perform an operation on
 * a tap.
 *
 * @opcode: Operation to be performed
 * @buf: I/O buffer read/write operations
 * @len: Length of I/O buffer
 */
struct __packed iotap_msg {
    uint8_t opcode;
    void *buf;
    size_t len;
};

#if !defined(_KERNEL)

/*
 * Perform an operation on a named I/O tap
 *
 * @name: Name of I/O tap
 * @msg: Message to send
 *
 * Returns a less than zero value on error
 */
ssize_t iotap_mux(const char *name, struct iotap_msg *msg);

#endif  /* !_KERNEL */
#endif  /* _SYS_IOTAP_H_ */
