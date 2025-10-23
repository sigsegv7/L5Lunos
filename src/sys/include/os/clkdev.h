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

#ifndef _OS_CLKDEV_H_
#define _OS_CLKDEV_H_ 1

#include <sys/types.h>
#include <sys/param.h>

/* Clock attributes */
#define CLKDEV_MSLEEP    BIT(0)  /* Has msleep() */
#define CLKDEV_GET_USEC  BIT(1)  /* Get elapsed usec */

#define MAX_CLKDEV 4

/*
 * Represents a clock device for time
 * keeping
 *
 * @name: Name of clock
 * @msleep: Sleep for 'n' msec
 * @get_time_usec: Get time since init in microseconds
 * @attr: Attribute mask
 */
struct clkdev {
    const char *name;
    int(*msleep)(size_t ms);
    int(*usleep)(size_t usec);
    size_t(*get_time_usec)(void);
    uint16_t attr;
};

/*
 * Registers a clock device
 *
 * @clkdev: Descriptor of clock device to use
 *
 * Returns zero on success, otherwise a less than
 * zero value.
 */
int clkdev_register(struct clkdev *clkdev);

/*
 * Get a specific clock device by index
 *
 * @attr: Attribute mask for clock to include
 * @clkdev_res: Clock device pointer result written here
 *
 * Returns zero on success, otherwise a less than zero
 * value.
 */
int clkdev_get(uint16_t attr, struct clkdev **clkdev_res);

#endif  /* !_OS_CLKDEV_H_ */
