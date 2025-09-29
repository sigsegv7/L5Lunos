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
 * Description: System object namespace
 * Author: Ian Marco Moffett
 */

#include <sys/errno.h>
#include <sys/param.h>
#include <os/nsvar.h>
#include <os/ns.h>
#include <string.h>

#define NS_HM_ENTRIES 16

/* Name space object hashmap entry */
struct hashmap_entry {
    void *data;
    char *name;
    uint32_t key;
};

/* Namespace object hashmap */
struct ns_hashmap {
    struct hashmap_entry entries[NS_HM_ENTRIES];
    size_t entry_count;
    struct ns_hashmap *next;
};

/* Global namespace */
static struct ns_hashmap namespace;

/*
 * Fowler–Noll–Vo hash function
 *
 * @s: String to hash
 */
static uint32_t
fnv1_hash(const char *s)
{
    uint32_t hash = 2166136261UL;
    const uint8_t *p = (uint8_t *)s;

    while (*p != '\0') {
        hash ^= *p;
        hash = hash * 0x01000193;
        ++p;
    }

    return hash;
}

/*
 * Initialize an object
 */
int
ns_obj_init(struct ns_obj *nsop)
{
    if (nsop == NULL) {
        return -EINVAL;
    }

    memset(nsop, 0, sizeof(*nsop));
    nsop->refcount = 1;
    return 0;
}

/*
 * Read data from an object
 */
ssize_t
ns_obj_read(struct ns_obj *nsop, void *buf, off_t off, size_t len)
{
    if (nsop == NULL || buf == NULL) {
        return -EINVAL;
    }

    if (len == 0) {
        return -EINVAL;
    }

    /*
     * Prefer read() but fallback to data if we
     * need to
     */
    if (nsop->read == NULL && nsop->data != NULL) {
        memcpy(buf, nsop->data, len);
    }

    return len;
}

/*
 * Place an object into the namespace
 */
int
ns_obj_enter(ns_t ns, void *obj, const char *name)
{
    uint32_t hash, key;
    struct ns_hashmap *hm;
    struct hashmap_entry *entry;

    if (obj == NULL || name == NULL) {
        return -EINVAL;
    }

    /* Get the index */
    hash = fnv1_hash(name);
    key = hash % NS_HM_ENTRIES;

    /* Find a free slot */
    hm = &namespace;
    while (hm != NULL) {
        entry = &hm->entries[key];

        if (entry->data == NULL) {
            entry->data = obj;
            entry->name = strdup(name);
            return 0;
        }

        hm = hm->next;
    }

    return 0;
}

/*
 * Look up and object within the namespace
 */
int
ns_obj_lookup(ns_t ns, const char *name, void *res_p)
{
    uint32_t hash, key;
    struct ns_hashmap *hm;
    struct hashmap_entry *entry;

    if (name == NULL || res_p == NULL) {
        return -EINVAL;
    }

    /* Get the index */
    hash = fnv1_hash(name);
    key = hash % NS_HM_ENTRIES;

    hm = &namespace;
    while (hm != NULL) {
        entry = &hm->entries[key];
        if (entry->name == NULL) {
            hm = hm->next;
            continue;
        }

        if (strcmp(entry->name, name) == 0) {
            *((void **)res_p) = entry->data;
            return 0;
        }

        hm = hm->next;
    }

    return -ENOENT;
}

/*
 * Initialize the namespace
 */
int
ns_init(void)
{
    memset(&namespace, 0, sizeof(namespace));
    return 0;
}
