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

#ifndef _SYS_BOOTVARS_H_
#define _SYS_BOOTVARS_H_  1

#include <sys/types.h>
#include <sys/param.h>

#define BOOTVARS_MAGIC 0xDECAFE

/* Flags for bootvars_read() */
#define BV_BYPASS_CACHE BIT(0)   /* Don't read cached entries */

/*
 * Framebuffer information
 *
 * @io: MMIO address for pixel plotting
 * @width: Framebuffer width
 * @height: Framebuffer height
 * @pitch: Framebuffer pitch
 * @bpp: Bytes per pixel
 */
struct bootvar_fb {
    uint32_t *io;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
};

/*
 * Various callbacks that are related in one
 * way or another to the features of the bootloader.
 */
struct bootvar_io {
    char *(*get_module)(const char *path, uint64_t *size);
};

/*
 * Boot variables given to us by the bootloader
 *
 * @magic: Magic number (see BOOTVARS_MAGIC)
 * @fbvars: Framebuffer information
 * @iovars: I/O callbacks
 * @rsdp: ACPI Root System Descriptor Pointer [vaddr]
 */
struct bootvars {
    uint32_t magic;
    struct bootvar_fb fbvars;
    struct bootvar_io iovars;
    void *rsdp;
};

#if defined(_KERNEL)

off_t get_kernel_base(void);
int bootvars_read(struct bootvars *bvp, int flags);

#endif  /* _KERNEL */
#endif  /* !_SYS_BOOTVARS_H_ */
