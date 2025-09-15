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
#include <io/cons/cons.h>
#include <stdbool.h>
#include <string.h>

#define DEFAULT_BG 0x000000
#define DEFAULT_FG 0xB57614

struct cons_scr g_root_scr;

/*
 * Fill a screen with a desired background
 * color
 *
 * @bg: Background color to fill screen
 */
static void
fill_screen(struct cons_scr *scr, uint32_t bg)
{
    struct bootvar_fb *fbvars;
    size_t len;

    if (scr == NULL) {
        return;
    }

    fbvars = &scr->fbvars;
    len = fbvars->width * fbvars->pitch;
    memset(fbvars->io, bg, len);
}

/*
 * Initialize the console
 */
int
cons_init(void)
{
    static bool is_init = false;
    struct bootvars bv;
    int error;

    /* Only init once */
    if (!is_init) {
        is_init = true;
    } else {
        return -EPERM;
    }

    /*
     * Attempt to acquire the bootvars so we
     * can get the framebuffer
     */
    error = bootvars_read(&bv, 0);
    if (error < 0) {
        printf("cons_init: could not read bootvars\n");
        is_init = false;
        return error;
    }

    memcpy(&g_root_scr.fbvars, &bv.fbvars, sizeof(bv.fbvars));
    g_root_scr.scr_bg = DEFAULT_BG;
    g_root_scr.scr_fg = DEFAULT_FG;
    fill_screen(&g_root_scr,  g_root_scr.scr_bg);
    return 0;
}
