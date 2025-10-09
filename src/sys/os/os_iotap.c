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
#include <sys/syslog.h>
#include <sys/queue.h>
#include <sys/atomic.h>
#include <sys/limits.h>
#include <os/systm.h>
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

ssize_t
iotap_mux(const char *name, struct iotap_msg *msg)
{
    struct iotap_ops *ops;
    struct iotap_desc desc;
    int error;

    if (msg == NULL) {
        return -EINVAL;
    }

    if (msg->buf == NULL || msg->len == 0) {
        return -EINVAL;
    }

    /* Lookup the tap */
    error = iotap_lookup(name, &desc);
    if (error < 0) {
        return error;
    }

    ops = desc.ops;
    switch (msg->opcode) {
    case IOTAP_OPC_READ:
        return ops->read(&desc, msg->buf, msg->len);
    }

    return -EINVAL;
}

/*
 * Get an I/O TAP:
 *
 * ARG0: Name
 * ARG1: Message
 *
 * RETURNED IN RAX: TAP ID
 */
scret_t
sys_muxtap(struct syscall_args *scargs)
{
    struct iotap_msg msg;
    struct iotap_desc desc;
    char *u_databuf;
    char buf[NAME_MAX], *kbuf;
    const char *u_name = SCARG(scargs, const char *, 0);
    struct iotap_msg *u_msg = SCARG(scargs, struct iotap_msg *, 1);
    int error;

    /* Grab the name */
    error = copyinstr(u_name, buf, sizeof(buf));
    if (error < 0) {
        printf("gettap: bad address for name\n");
        return error;
    }

    /* Get the message */
    error = copyin(u_msg, &msg, sizeof(msg));
    if (error < 0) {
        printf("gettap: bad address for message\n");
        return error;
    }

    /* Grab the actual tap */
    error = iotap_lookup(buf, &desc);
    if (error < 0) {
        printf("gettap: SYS_gettap lookup failure\n");
        return error;
    }

    /* Truncate if needed */
    if (msg.len >= IOTAP_MSG_MAX) {
        msg.len = IOTAP_MSG_MAX;
    }

    /* Allocate a kernel-side buffer */
    kbuf = kalloc(msg.len);
    if (kbuf == NULL) {
        return -ENOMEM;
    }

    /* Perform the operation */
    u_databuf = msg.buf;
    msg.buf = kbuf;
    error = iotap_mux(buf, &msg);

    /*
     * If there are no errors, then we are free to
     * copy the results back
     */
    if (error > 0) {
        copyout(kbuf, u_databuf, msg.len);
    }

    kfree(kbuf);
    return error;
}
