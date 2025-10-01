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

#include <sys/errno.h>
#include <sys/syslog.h>
#include <os/kalloc.h>
#include <lib/ptrbox.h>
#include <string.h>

/*
 * Allocate memory and add it to an existing
 * box
 */
void *
ptrbox_alloc(size_t len, struct ptrbox *box)
{
    struct ptrbox_entry *ent;

    if (len == 0 || box == NULL) {
        return NULL;
    }

    ent = kalloc(sizeof(*ent));
    if (ent == NULL) {
        return NULL;
    }

    /* Allocate the actual data */
    ent->data = kalloc(len);
    if (ent->data == NULL) {
        kfree(ent);
        return NULL;
    }

    ent->len = len;
    TAILQ_INSERT_TAIL(&box->q, ent, link);
    return ent->data;
}

/*
 * Terminate a box
 */
int
ptrbox_terminate(struct ptrbox *box)
{
    struct ptrbox_entry *ent;

    if (box == NULL) {
        return -EINVAL;
    }

    /*
     * Go through each entry first, reap one
     * by one.
     */
    TAILQ_FOREACH(ent, &box->q, link) {
        if (ent == NULL) {
            continue;
        }

        kfree(ent->data);
        printf("reaped %p\n", ent->data);
        ent->data = NULL;
    }

    /* Reap ourselves */
    kfree(ent);
    return 0;
}

/*
 * Initialize a box
 */
int
ptrbox_init(struct ptrbox **box_res)
{
    struct ptrbox *box;

    if (box_res == NULL) {
        return -EINVAL;
    }

    box = kalloc(sizeof(*box));
    if (box == NULL) {
        return -ENOMEM;
    }

    box->count = 0;
    TAILQ_INIT(&box->q);
    *box_res = box;
    return 0;
}
