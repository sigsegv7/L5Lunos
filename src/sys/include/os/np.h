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

/*
 * Description: Pirho compiler interface
 * Author: Ian Marco Moffett
 */

#ifndef _OS_NP_H_
#define _OS_NP_H_ 1

#include <sys/types.h>
#include <os/vnode.h>
#include <np/lex.h>
#include <lib/ptrbox.h>

/*
 * Compiler work
 *
 * @source: Source input file
 * @source_size: Source size in bytes
 * @line_no: Current line number
 * @lex_st: Lexer state
 * @ccache: Character cache (temporary store for lexer)
 */
struct np_work {
    char *source;
    size_t source_size;
    size_t line_no;
    struct lexer_state lex_st;
    struct ptrbox *work_mem;
    char ccache;
};

/*
 * Initializes the compiler state into known values
 *
 * @in_path: Input file path of sources to be compiled
 * @workp: Resulting work data is written here
 *
 * Returns zero on success, otherwise a less than zero
 * value on failure.
 */
int np_init(const char *in_path, struct np_work *workp);

/*
 * Complete work by compiling the input file
 *
 * @work: Work that will be compiled
 *
 * Returns zero on success, otherwise a less than
 * zero value on failure
 */
int np_compile(struct np_work *work);

#endif /* _OS_NP_H_ */
