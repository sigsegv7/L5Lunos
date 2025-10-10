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
#include <sys/ascii.h>
#include <io/cons/cons.h>
#include <io/cons/font.h>
#include <io/cons/consvar.h>
#include <stdbool.h>
#include <string.h>

#define DEFAULT_BG 0x000000
#define DEFAULT_FG 0xB57614
#define FONT_WIDTH 8
#define FONT_HEIGHT 20

#define CURSOR_WIDTH FONT_WIDTH
#define CURSOR_HEIGHT 4

struct cons_scr g_root_scr;

/* Forward declarations */
static void fill_screen(struct cons_scr *scr, uint32_t bg);

/*
 * Get the index into the framebuffer with an x and y
 * position.
 *
 * @pitch: Framebuffer pitch
 * @x: X position to plot
 * @y: Y position to plot
 *
 * Returns index
 */
__always_inline static inline size_t
fb_get_index(uint32_t pitch, uint32_t x, uint32_t y)
{
    return x + y * (pitch / 4);
}

/*
 * Invert an RGB color
 *
 * @rgb: Color to invert
 *
 * Returns inverted color code (RGB)
 */
__always_inline static inline uint32_t
rgb_invert(uint32_t rgb)
{
    return (0xFFFFFF - rgb);
}

/*
 * Draw the text cursor onto the screen
 *
 * @scr: Screen to draw onto
 * @hide: True if it should be hidden
 */
static void
cons_draw_cursor(struct cons_scr *scr, bool hide)
{
    struct bootvar_fb *fbvars;
    uint32_t *fbio, idx;
    uint32_t color;

    if (scr == NULL) {
        return;
    }

    fbvars = &scr->fbvars;
    fbio = fbvars->io;

    for (uint32_t cy = 0; cy < CURSOR_HEIGHT; ++cy) {
        for (uint32_t cx = 0; cx < CURSOR_WIDTH; ++cx) {
            idx = fb_get_index(
                fbvars->pitch,
                cx + scr->cursor_x,
                (cy + scr->cursor_y) + FONT_HEIGHT / 2
            );

            color = (hide) ? scr->scr_bg : rgb_invert(scr->scr_bg);
            fbio[idx] = color;
        }
    }
}

/*
 * Write a newline onto the console and handle
 * Y overflows
 *
 * @scr: Screen to write a newline onto
 */
static void
cons_newline(struct cons_scr *scr)
{
    scr->text_x = 0;
    scr->text_y += FONT_HEIGHT;

    cons_draw_cursor(scr, true);
    scr->cursor_y += FONT_HEIGHT;
    scr->cursor_x = 0;

    /* Handle console y overflow */
    if (scr->text_y >= scr->max_row - FONT_HEIGHT) {
        scr->text_x = 0;
        scr->text_y = 0;

        scr->cursor_x = 0;
        scr->cursor_y = 0;
        fill_screen(scr,  scr->scr_bg);
    }

    /* Redraw the cursor */
    cons_draw_cursor(scr, false);
}

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
 * Handle special ASCII characters
 *
 * @scr: Screen we are writing on
 * @c: Character to handle
 *
 * Returns the character on success, otherwis a less than
 * zero value on [need retransmit] / error
 */
static int
cons_handle_spec(struct cons_scr *scr, int c)
{
    if (scr == NULL) {
        return -EINVAL;
    }

    switch (c) {
    case ASCII_LF:
        cons_newline(scr);
        return c;
    }

    return -1;
}

/*
 * Plot a single character onto the screen
 *
 * @scr: Screen to plot onto
 * @ch: Character to plot onto screen
 */
static void
cons_putch(struct cons_scr *scr, struct cons_ch *ch)
{
    struct bootvar_fb *fbvars;
    struct font_header *hdr;
    uint32_t x, y, color;
    uint32_t *fbio;
    const uint8_t *glyph;
    size_t idx;

    hdr = (void *)g_CONS_FONT;
    glyph = PTR_OFFSET(hdr, FONT_HDRLEN + ch->c * hdr->csize);
    fbvars = &scr->fbvars;
    fbio = fbvars->io;
    x = ch->x;
    y = ch->y;

    /* Update the cursor */
    cons_draw_cursor(scr, true);
    scr->cursor_x += FONT_WIDTH;

    /* Begin the plotting */
    for (int cy = 0; cy < hdr->csize; ++cy) {
        for (int cx = 0; cx < 8; ++cx) {
            color = ISSET(glyph[cy], BIT(8 - cx)) ? ch->fg : ch->bg;

            /* Plot the pixel */
            idx = fb_get_index(fbvars->pitch, x+cx, y+cy);
            fbio[idx] = color;
        }
    }

    cons_draw_cursor(scr, false);
}

/*
 * Draw a string onto the screen
 */
ssize_t
cons_putstr(struct cons_scr *scr, const char *str, size_t len)
{
    struct cons_ch ch;
    size_t slen;

    if (scr == NULL || str == NULL) {
        return -EINVAL;
    }

    ch.bg = scr->scr_bg;
    ch.fg = scr->scr_fg;

    spinlock_acquire(&scr->lock);
    for (size_t i = 0; i < len; ++i) {
        ch.y = scr->text_y;
        ch.x = scr->text_x;
        ch.c = str[i];

        if (cons_handle_spec(scr, ch.c) == ch.c) {
            continue;
        }

        cons_putch(scr, &ch);
        scr->text_x += FONT_WIDTH;

        /* Handle console x overflow */
        if (scr->text_x >= scr->max_col - FONT_WIDTH) {
            cons_newline(scr);
        }
    }

    spinlock_release(&scr->lock);
    return len;
}

/*
 * Initialize the console
 */
int
cons_init(void)
{
    static bool is_init = false;
    struct bootvars bv;
    struct bootvar_fb *fbvars;
    int error;

    /* Only init once */
    if (!is_init) {
        is_init = true;
    } else {
        return -EPERM;
    }

    memset(&g_root_scr, 0, sizeof(g_root_scr));

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

    /* Save framebuffer information */
    fbvars = &g_root_scr.fbvars;
    memcpy(fbvars, &bv.fbvars, sizeof(*fbvars));

    /* Set up screen state */
    g_root_scr.scr_bg = DEFAULT_BG;
    g_root_scr.scr_fg = DEFAULT_FG;
    g_root_scr.max_col = fbvars->width;
    g_root_scr.max_row = fbvars->height;
    fill_screen(&g_root_scr,  g_root_scr.scr_bg);
    return 0;
}
