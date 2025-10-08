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

#ifndef _OS_IOTAP_H_
#define _OS_IOTAP_H_ 1

#include <sys/types.h>
#include <sys/queue.h>

struct iotap_desc;
typedef int16_t iotap_t;

struct iotap_ops {
    ssize_t(*read)(struct iotap_desc *desc, void *p, size_t len);
};

/*
 * Represents an I/O tap descriptor used to interface
 * with a device
 *
 * @name: I/O tap name
 * @id: ID of I/O tap
 * @ops: I/O tap operations
 *
 * XXX: When registering an I/O tap, leave `id' unset as it will
 *      be filled by the registration logic.
 */
struct iotap_desc {
    char *name;
    iotap_t id;
    struct iotap_ops *ops;
};

/*
 * Register an I/O tap interface
 *
 * @iotap: I/O tap interface descriptor to register
 *
 * Returns an I/O table ID as a handle on success,
 * otherwise a less than zero value on failure.
 */
iotap_t iotap_register(const struct iotap_desc *iotap);

/*
 * Look up an I/O tap by name
 *
 * @name: Name of I/O tap to look up
 * @dp_res: I/O tap descriptor result
 *
 * Returns zero on success with a result
 * assigned to `dp`, otherwise a less than
 * zero value on failure.
 */
int iotap_lookup(const char *name, struct iotap_desc *dp_res);

#endif  /* !_OS_IOTAP_H_ */
