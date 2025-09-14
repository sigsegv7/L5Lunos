/*
 * Copyright (c) 2025 Ian Marco Moffett and Ethos0 engineers
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
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/panic.h>
#include <vm/vm.h>
#include <limine.h>
#include <string.h>
#include <stdbool.h>

#define FRAMEBUFFER framebuffer_req.response->framebuffers[0]

static off_t __hhdm_offset = 0;
static volatile struct limine_hhdm_request hhdm_req = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

/* Request from bootloader */
static volatile struct limine_framebuffer_request framebuffer_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

static volatile struct limine_module_request mod_req = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

static volatile struct limine_rsdp_request rsdp_req = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

/*
 * Get a module from the bootloader, this can be used for
 * things like configuration or initramfs.
 *
 * @path: Path to lookup
 * @size: Size of found data written here
 */
static char *
get_module(const char *path, uint64_t *size) {
    for (uint64_t i = 0; i < mod_req.response->module_count; ++i) {
        if (strcmp(mod_req.response->modules[i]->path, path) == 0) {
            *size = mod_req.response->modules[i]->size;
            return mod_req.response->modules[i]->address;
        }
    }

    return NULL;
}

static void
read_fbvars(struct bootvar_fb *fbvars)
{
    fbvars->io = FRAMEBUFFER->address;
    fbvars->width = FRAMEBUFFER->width;
    fbvars->height = FRAMEBUFFER->height;
    fbvars->pitch = FRAMEBUFFER->pitch;
    fbvars->bpp = FRAMEBUFFER->bpp;
}

static void
read_iovars(struct bootvar_io *iovp)
{
    iovp->get_module = get_module;
}

/*
 * Return the kernel base offset which is used to convert
 * certain lower half physical addresses to higher half
 * physical addresses.
 */
off_t
get_kernel_base(void)
{
    static struct limine_hhdm_response *hhdm_resp = NULL;

    if (hhdm_resp == NULL) {
        if ((hhdm_resp = hhdm_req.response) == NULL) {
            panic("bootvars: could not get HHDM\n");
        }
    }

    if (__hhdm_offset == 0) {
        __hhdm_offset = hhdm_resp->offset;
    }

    return __hhdm_offset;
}

/*
 * Read bootvars given to us on boot that we will
 * use to set up the system.
 *
 * @bvp: Bootvars result
 * @flags: Optional flags (see BV_*)
 *
 * XXX: BV_BYPASS_CACHE calls are still cached however entries are
 *      guaranteed to not be stale.
 */
int
bootvars_read(struct bootvars *bvp, int flags)
{
    static struct bootvars cache;
    static short cached = false;
    struct limine_rsdp_response *rsdp_resp;

    if (bvp == NULL) {
        return -EINVAL;
    }

    /*
     * If BV_BYPASS_CACHE is not set and we have a cache
     * entry, use it.
     */
    if (cached && !ISSET(flags, BV_BYPASS_CACHE)) {
        *bvp = cache;
        return 0;
    }

    cache.magic = BOOTVARS_MAGIC;
    read_fbvars(&cache.fbvars);
    read_iovars(&cache.iovars);

    /* We need this for proper operation */
    if ((rsdp_resp = rsdp_req.response) == NULL) {
        panic("bootvars: could not get ACPI RSDP\n");
    }

    cache.rsdp = rsdp_resp->address;
    cached = true;
    *bvp = cache;
    return 0;
}
