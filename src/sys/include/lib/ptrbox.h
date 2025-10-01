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

#ifndef _PTRBOX_H_
#define _PTRBOX_H_  1

#include <sys/queue.h>

struct ptrbox_entry;
struct ptrbox;

/*
 * Represents an allocate entry within a
 * pointer box
 *
 * @data: Allocated data
 * @len: Length of allocated data
 * @link: Queue link
 */
struct ptrbox_entry {
    void *data;
    size_t len;
    TAILQ_ENTRY(ptrbox_entry) link;
};

/*
 * Represents a RAII styled pointer box
 *
 * @count: Number of entries
 * @q: Entry queue
 *
 * Memory is allocated and placed into a pointer
 * box
 *
 * Why?: One call to free them all
 */
struct ptrbox {
    size_t count;
    TAILQ_HEAD(, ptrbox_entry) q;
};

/*
 * Initialize a new pointer box
 *
 * @box_res: Pointer box is written here
 *
 * Returns zero on success, otherwise a less than
 * zero value on failure
 */
int ptrbox_init(struct ptrbox **box_res);

/*
 * Terminate the box from its life and kill its
 * children. Used for RAII-styled cleanups.
 *
 * Returns zero on success, otherwise a less than
 * zero value on failure.
 */
int ptrbox_terminate(struct ptrbox *box);

/*
 * Allocate memory and put it into a pointer
 * box for RAII style handling
 *
 * @len: Length of memory to allocate
 * @box: Pointer box to use
 *
 * Returns the allocated memory base on success, otherwise
 * a value of NULL on failure.
 */
void *ptrbox_alloc(size_t len, struct ptrbox *box);

#endif  /* !_PTRBOX_H_ */
