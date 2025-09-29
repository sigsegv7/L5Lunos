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

#ifndef _OS_NS_H_
#define _OS_NS_H_

#include <sys/types.h>
#include <os/nsvar.h>

typedef uint8_t ns_t;

/*
 * Initializes an object into a known state so that
 * further operations can be performed
 *
 * @nsop: Newly created object to be initialized
 *
 * Returns a value of zero on success, otherwise a less
 * than zero value upon failure.
 */
int ns_obj_init(struct ns_obj *nsop);

/*
 * Place an object into the namespace
 *
 * @ns: ID of the namespace to enter object
 * @obj: Object that we want to enter
 * @name: Name of the object
 *
 * Returns zero on success, otherwise a less than
 * zero value on failure
 */
int ns_obj_enter(ns_t ns, void *obj, const char *name);

/*
 * Find an object within a namespace
 *
 * @ns: ID of namespace to lookup
 * @name: Name of object we want to find
 * @res_p: Pointer of result pointer to store
 *
 * Returns zero on success, otherwise a less than
 * zero value on failure.
 */
int ns_obj_lookup(ns_t ns, const char *name, void *res_p);

/*
 * Unifies the buffer / callback interface into a single
 * function, used to read an object's data into a buffer
 * by length.
 *
 * @nsop: Object to read from
 * @buf: Buffer to read into
 * @off: Offset to read at
 * @len: Length of bytes to read
 *
 * Returns the length of the data read on success, otherwise
 * a less than zero value upon failure.
 */
ssize_t ns_obj_read(struct ns_obj *nsop, void *buf, off_t off, size_t len);

#endif  /* !_OS_NS_H_ */
