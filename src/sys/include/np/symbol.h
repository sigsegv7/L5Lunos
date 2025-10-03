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

#ifndef _NP_SYMBOL_H_
#define _NP_SYMBOL_H_ 1

#include <sys/types.h>
#include <sys/queue.h>
#include <os/np.h>

/* Number of symbol cache entries */
#define SYMCACHE_LEN 32

/*
 * Represents a single symbol entry within
 * a symbol table
 *
 * @name: Name of the symbol
 * @addr: Data the symbol holds
 * @link: Queue link
 */
struct symbol {
    char *name;
    void *addr;
    TAILQ_ENTRY(symbol) link;
};

/*
 * Represents a symbol list
 *
 * @symq: Symbol queue
 * @cache: Lookup cache
 * @work: Work associated with this symbol list
 * @nsym: Number of total symbols
 * @cache_i: Cache index
 */
struct symlist {
    TAILQ_HEAD(, symbol) symq;
    struct symbol *cache[SYMCACHE_LEN];
    struct np_work *work;
    size_t nsym;
    uint16_t cache_i;
};

/*
 * Initialize a symbol list
 *
 * @work: Current work
 * @symlist: Symbol list to initialize
 *
 * Returns zero on success, otherwise a less
 * than zero value on failure.
 */
int symlist_init(struct np_work *work, struct symlist *symlist);

/*
 * Allocate a new symbol that may be placed
 * in a symbol list
 *
 * @slp: Symbol list pointer
 * @name: Symbol name
 * @addr: Symbol address
 *
 * Returns a pointer to the allocated symbol on success,
 * otherwise returns a value of NULL on failure.
 */
struct symbol *symbol_alloc(struct symlist *slp, char *name, void *addr);

/*
 * Lookup a specific symbol from a symbol
 * table
 *
 * @slp: Symbol list pointer
 * @name: Symbol name
 * @res_p: Result pointer is written here
 *
 * Returns zero on success, otherwise a less than zero
 * value on failure.
 */
int symbol_lookup(struct symlist *slp, char *name, struct symbol **res_p);
#endif  /* !_NP_SYMBOL_H_ */
