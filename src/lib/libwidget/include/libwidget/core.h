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

#ifndef LIBWIDGET_CORE_H
#define LIBWIDGET_CORE_H 1

#include <stdint.h>

/* Forward declarations */
struct widget;

typedef enum {
    WIDGET_WINDOW,
    MAX_WIDGETS
} widget_type_t;

/*
 * Represents the color blueprint to get
 * or set color attributes
 *
 * @fg: Border color
 * @bg: Background
 */
struct bp_color {
    uint32_t fg;
    uint32_t bg;
};

/*
 * Represents the blueprint of a widget, describes
 * its attributes and can be used to apply changes
 *
 * @x: Cartesian X position
 * @y: Cartesian Y position
 * @color: Color blueprint
 */
struct blueprint {
    uint32_t x;
    uint32_t y;
    struct bp_color color;
};

/*
 * Widget operations, what can be done on
 * a widget
 *
 * @init: Called upon start up
 * @draw: Draw the widget
 */
struct widget_ops {
    int(*init)(struct widget *wp);
    int(*draw)(struct widget *wp);
};

/*
 * Describes a widget
 *
 * @ops: Widget operations
 * @type: Widget type
 * @bp: Widget blueprint
 * @leaf_count: Number of children
 * @data: Widget specific data
 */
struct widget {
    struct widget_ops *ops;
    widget_type_t type;
    struct blueprint bp;
    uint16_t leaf_count;
    void *data;
};

/*
 * Initialize a widget
 *
 * @wp: Widget to initialize
 * @type: Widget type
 * @bp: Blueprint to apply upon init [NULL for default]
 *
 * Returns zero on success, otherwise a less than zero
 * value on failure.
 */
int widget_init(struct widget *wp, widget_type_t type, struct blueprint *bp);

/*
 * Draw a widget onto the screen
 *
 * @wp: Widget to draw onto the screen
 *
 * Returns zero on success, otherwise a less than zero
 * value on failure.
 */
int widget_update(struct widget *wp);

#endif  /* LIBWIDGET_CORE_H */
