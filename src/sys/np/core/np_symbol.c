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

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/syslog.h>
#include <lib/ptrbox.h>
#include <os/np.h>
#include <np/symbol.h>
#include <string.h>

#define pr_trace(fmt, ...) printf("pirho.sym: " fmt, ##__VA_ARGS__)
#define pr_error(fmt, ...) printf("pirho.sym: error: " fmt, ##__VA_ARGS__)

/*
 * Cache a symbol so that future lookups will
 * be faster
 *
 * @slp: Symbol list
 * @s: Symbol to cache
 */
static void
symbol_cache(struct symlist *slp, struct symbol *s)
{
    if (slp == NULL || s == NULL) {
        return;
    }

    /* Wrap if needed */
    if (slp->cache_i >= SYMCACHE_LEN) {
        slp->cache_i = 0;
    }

    slp->cache[slp->cache_i++] = s;
}

int
symbol_lookup(struct symlist *slp, char *name, struct symbol **res_p)
{
    struct symbol *sym = NULL;

    if (slp == NULL || name == NULL) {
        return -EINVAL;
    }

    if (res_p == NULL) {
        return -EINVAL;
    }

    /* Is what we are looking for in the cache? */
    for (uint16_t i = 0; i < slp->cache_i; ++i) {
        sym = slp->cache[i];
        if (strcmp(sym->name, name) == 0) {
            *res_p = sym;
            return 0;
        }
    }

    TAILQ_FOREACH(sym, &slp->symq, link) {
        if (sym == NULL)
            continue;
        if (sym->name == NULL)
            continue;

        if (strcmp(sym->name, name) == 0) {
            symbol_cache(slp, sym);
            *res_p = sym;
            return 0;
        }
    }

    return -1;
}

int
symbol_lookup_id(struct symlist *slp, size_t id, struct symbol **res_p)
{
    struct symbol *sym = NULL;

    if (slp == NULL || res_p == NULL) {
        return -EINVAL;
    }

    /* Is it in the cache? */
    for (uint16_t i = 0; i < slp->cache_i; ++i) {
        sym = slp->cache[i];
        if (sym->id == id) {
            *res_p = sym;
            return 0;
        }
    }

    /* Go through each entry to find it */
    TAILQ_FOREACH(sym, &slp->symq, link) {
        if (sym == NULL)
            continue;

        if (sym->id == id) {
            symbol_cache(slp, sym);
            *res_p = sym;
            return 0;
        }
    }

    return -1;
}

struct symbol *
symbol_alloc(struct symlist *slp, char *name, void *addr)
{
    char namebuf[64];
    struct symbol *sym = NULL;
    struct np_work *work;

    if (slp == NULL || addr == NULL) {
        return NULL;
    }

    /* Grab the work for this symbol list */
    if ((work = slp->work) == NULL) {
        pr_error("bad work pointer");
        return NULL;
    }

    sym = ptrbox_alloc(sizeof(*sym), work->work_mem);
    if (sym == NULL) {
        pr_error("could not allocated symbol");
        return NULL;
    }

    sym->addr = addr;
    sym->id = slp->nsym;

    /*
     * If there is no name to use, we'll make one up for
     * ourselves.
     */
    if (name == NULL) {
        sym->name = ptrbox_strdup(name, work->work_mem);
    } else {
        snprintf(namebuf, sizeof(namebuf), "__internal.%d", sym->id);
        sym->name = ptrbox_strdup(namebuf, work->work_mem);
    }

    TAILQ_INSERT_TAIL(&slp->symq, sym, link);
    ++slp->nsym;
    return sym;
}

int
symlist_init(struct np_work *work, struct symlist *symlist)
{
    if (symlist == NULL) {
        return -EINVAL;
    }

    TAILQ_INIT(&symlist->symq);
    symlist->nsym = 0;
    symlist->cache_i = 0;
    symlist->work = work;
    return 0;
}
