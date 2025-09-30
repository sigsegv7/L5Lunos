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

#include <stdint.h>
#include <errno.h>
#include <stddef.h>
#include <libwidget/window.h>
#include <libwidget/core.h>

#define WINDOW_MAX 2

static struct window windows[WINDOW_MAX];
static uint8_t next_window = 0;

/*
 * Initialize a window
 *
 * TODO: We do not have a malloc so we are doing a hack
 *       and using a window array, change this when we can
 */
static int
window_init(struct libwidget_state *lws, struct widget *wp)
{
    struct window *win;

    if (next_window >= sizeof(WINDOW_MAX)) {
        return -1;
    }

    win = &windows[next_window++];
    wp->data = win;
    return 0;
}

static int
window_draw(struct libwidget_state *lws, struct widget *wp)
{
    struct fb_info *fbinfo;
    struct blueprint *bp;
    const struct bp_color *color;
    uint32_t x, y, idx;

    if (lws == NULL || wp == NULL) {
        return -EINVAL;
    }

    bp = &wp->bp;
    color = &bp->color;
    fbinfo = &lws->fbinfo;

    /* Draw a square */
    for (uint32_t cy = 0; cy < bp->height; ++cy) {
        for (uint32_t cx = 0; cx < bp->width; ++cx) {
            x = bp->x + cx;
            y = bp->y + cy;
            idx = get_pix_index(&lws->fbinfo, x, y);
            lws->fbdev[idx] = color->bg;
        }
    }

    return 0;
}

struct widget_ops g_winops = {
    .init = window_init,
    .draw = window_draw
};
