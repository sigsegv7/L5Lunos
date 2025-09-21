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

#include <sys/errno.h>
#include <os/clkdev.h>
#include <os/spinlock.h>

static uint8_t nclk = 0;
static struct clkdev *clklist[MAX_CLKDEV];
static struct spinlock lock;

/*
 * Register a clock device
 */
int
clkdev_register(struct clkdev *clkdev)
{
    if (clkdev == NULL) {
        return -EINVAL;
    }

    if (nclk >= MAX_CLKDEV) {
        return -ENOSPC;
    }

    spinlock_acquire(&lock);
    clklist[nclk++] = clkdev;
    spinlock_release(&lock);
    return 0;
}

/*
 * Get a specific clock device from index
 */
int
clkdev_get(uint16_t attr, struct clkdev **clkdev_res)
{
    if (clkdev_res == NULL) {
        return -EINVAL;
    }

    /* Must find one that matches 'attr' */
    for (int i = 0; i < MAX_CLKDEV; ++i) {
        if (clklist[i]->attr == attr) {
            *clkdev_res = clklist[i];
            return 0;
        }
    }

    return -1;
}
