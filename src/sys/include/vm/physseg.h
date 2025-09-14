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

#ifndef _VM_PHYSSEG_H_
#define _VM_PHYSSEG_H_ 1

#include <sys/types.h>

/*
 * Represents physical memory stats gathered
 * during initialization or during refresh.
 */
struct physmem_stat {
    size_t pages_free;
    size_t pages_used;
};

/*
 * Initialize physical memory and get initial physical
 * memory stats.
 *
 * @stat: Stats are written here
 *
 * Returns zero on success, otherwise a less than zero
 * value on failure.
 */
int vm_seg_init(struct physmem_stat *stat);

/*
 * Allocate one or more physical frames and
 * get the [physical] address.
 *
 * @count: Number of frames to allocate
 *
 * Returns zero on failure (e.g., out of memory)
 */
uintptr_t vm_alloc_frame(size_t count);

/*
 * Free one or more physical frames
 *
 * @base: Base address to start freeing at
 * @count: Number of frames to free
 */
void vm_free_frame(uintptr_t base, size_t count);

#endif  /* !_VM_PHYSSEG_H_ */
