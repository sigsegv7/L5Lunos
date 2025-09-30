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

#include <sys/bootvars.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/fbdev.h>
#include <os/kalloc.h>
#include <os/module.h>
#include <os/ns.h>
#include <io/video/fbdev.h>
#include <vm/map.h>
#include <vm/mmu.h>

static struct fb_info info;

/*
 * Map the framebuffer, we'll decided how many bytes
 * is mapped.
 */
static ssize_t
fbdev_map(struct mac_border *mbp, struct mac_map_args *args)
{
    int prot = PROT_READ | PROT_WRITE | PROT_USER;
    size_t max_size = 0;
    struct bootvars bv;
    struct proc *self = proc_self();
    struct vm_vas vas;
    struct bootvar_fb *fbvar;
    struct mmu_map spec;
    int error;

    /* Grab the bootvars */
    error = bootvars_read(&bv, 0);
    if (error < 0) {
        return error;
    }

    /* Grab the current VAS for mapping */
    error = mmu_this_vas(&vas);
    if (error < 0) {
        return error;
    }

    fbvar = &bv.fbvars;
    max_size = fbvar->width * fbvar->pitch;
    if (args->len > max_size) {
        args->len = max_size;
    }

    spec.pa = VIRT_TO_PHYS(fbvar->io);
    spec.va = spec.pa;
    error = vm_map(&vas, &spec, args->len, prot);
    if (error < 0) {
        return error;
    }

    *args->dp_res = (void *)VIRT_TO_PHYS(fbvar->io);
    return args->len;
}

static int
fbdev_init(struct module *modp)
{
    struct bootvar_fb *fbvar;
    struct bootvars bv;
    struct ns_obj *obj;
    int error;

    obj = kalloc(sizeof(*obj));
    if (obj == NULL) {
        return -EINVAL;
    }

    /* Grab the bootvars */
    error = bootvars_read(&bv, 0);
    if (error < 0) {
        return error;
    }

    fbvar = &bv.fbvars;
    info.width = fbvar->width;
    info.height = fbvar->height;
    info.pitch = fbvar->pitch;

    /* Initialize the object */
    if ((error = ns_obj_init(obj)) < 0) {
        return error;
    }

    obj->data = &info;
    return ns_obj_enter(0, obj, FBDEV_NSO);
}

static struct mac_ops ops = {
    .map = fbdev_map,
    .sync = NULL,
    .getattr = NULL
};

struct mac_border g_fbdev_border = {
    .level = MAC_RESTRICTED,
    .ops = &ops
};

MODULE_EXPORT("fbdev", MODTYPE_GENERIC, fbdev_init);
