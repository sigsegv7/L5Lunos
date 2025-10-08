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
#include <sys/atomic.h>
#include <os/kalloc.h>
#include <os/iotap.h>
#include <os/nsvar.h>
#include <os/ns.h>
#include <stdbool.h>
#include <string.h>

static size_t next_id = 0;

iotap_t
iotap_register(const struct iotap_desc *iotap)
{
    struct iotap_desc *tap;
    struct ns_obj *obj;
    int error;

    if (iotap->name == NULL) {
        return -EINVAL;
    }

    tap = kalloc(sizeof(*tap));
    if (tap == NULL) {
        return -EINVAL;
    }

    obj = kalloc(sizeof(*obj));
    if (obj == NULL) {
        kfree(tap);
        return -EINVAL;
    }

    memcpy(tap, iotap, sizeof(*tap));
    tap->id = atomic_inc_64(&next_id);

    /* Copy the name */
    tap->name = strdup(iotap->name);

    /* Initialize and store */
    ns_obj_init(obj);
    obj->data = tap;
    error = ns_obj_enter(NS_IOTAP, obj, iotap->name);

    if (error < 0) {
        kfree(tap);
        kfree(obj);
        return error;
    }

    return tap->id;
}

int
iotap_lookup(const char *name, struct iotap_desc *dp_res)
{
    struct iotap_desc *tap;
    int error;

    if (name == NULL || dp_res == NULL) {
        return -EINVAL;
    }

    /* Look up the object in the namespace */
    error = ns_obj_lookup(NS_IOTAP, name, (void *)&tap);
    if (error < 0) {
        return error;
    }

    /* Copy it out to the result store */
    *dp_res = *tap;
    return 0;
}
