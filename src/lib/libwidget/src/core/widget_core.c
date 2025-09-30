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

#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/mac.h>
#include <sys/fbdev.h>
#include <libwidget/window.h>
#include <libwidget/core.h>

static struct widget backends[];
static struct libwidget_state lws;

/*
 * Initialize lib widget
 */
int
libwidget_init(void)
{
    int error;

    /* Grab framebuffer information */
    error = query(BORDER_FBDEV, &lws.fbinfo, sizeof(lws.fbinfo), 0);
    if (error < 0) {
        return error;
    }

    /* Grab the whole framebuffer, directly mapped */
    error = cross(BORDER_FBDEV, -1, 0, 0, &lws.fbdev);
    if (error < 0) {
        return error;
    }

    return 0;
}

/*
 * Put the widget into a known state
 */
int
widget_init(struct widget *wp, widget_type_t type, struct blueprint *bp)
{
    struct blueprint *bp_dest;
    struct bp_color *color;
    struct widget *backend;
    struct widget_ops *ops;

    if (wp == NULL) {
        return -EINVAL;
    }

    if (type >= MAX_WIDGETS) {
        return -EINVAL;
    }

    /* Put in a known state */
    memset(wp, 0, sizeof(*wp));

    /* Set the default color */
    bp_dest = &wp->bp;
    color = &bp_dest->color;
    color->bg = 0x282828;
    color->fg = 0xA89984;

    /* Default height */
    bp_dest->width = 100;
    bp_dest->height = 250;

    if (bp != NULL) {
        bp_dest->x = bp->x;
        bp_dest->y = bp->y;
        memcpy(bp_dest, bp, sizeof(wp->bp));
    }

    /* Get the backend and call init */
    backend = &backends[type];
    ops = backend->ops;
    wp->ops = ops;
    return ops->init(&lws, wp);
}

int
widget_update(struct widget *wp)
{
    struct widget_ops *ops;

    ops = wp->ops;
    return ops->draw(&lws, wp);
}

static struct widget backends[] = {
    [WIDGET_WINDOW] = { .ops = &g_winops }
};
