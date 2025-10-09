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

#include <sys/syslog.h>
#include <os/module.h>
#include <os/clkdev.h>
#include <os/spinlock.h>
#include <machine/intr.h>
#include <machine/i8042var.h>
#include <machine/pio.h>
#include <stdbool.h>


static struct clkdev *clk;
static struct spinlock lock;

/*
 * Character table for scancode conversion
 */
static char keytab[] = {
    '\0', '\x1B', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', '\n', '\0', 'a', 's', 'd', 'f', 'g', 'h',
    'j', 'k', 'l', ';', '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.',  '/', '\0', '\0', '\0', ' '
};

/*
 * Write a byte to the i8042
 */
static void
i8042_write(bool is_cmd, uint8_t v)
{
    uint8_t port, status = 0xFF;

    /* Wait until the input buffer empties */
    while (ISSET(status, I8042_IBUFF)) {
        status = inb(I8042_STATUS);
        clk->msleep(5);
    }

    port = is_cmd ? I8042_CMD : I8042_DATA;
    outb(port, v);
}

/*
 * Read a byte from the i8042
 */
static uint8_t
i8042_read(void)
{
    uint8_t status = 0x00;

    while (!ISSET(status, I8042_OBUFF)) {
        status = inb(I8042_STATUS);
        clk->msleep(5);
    }

    clk->msleep(5);
    return inb(I8042_DATA);
}

/*
 * IRQ 1 handler
 */
static int
i8042_irq(struct intr_hand *hp)
{
    uint8_t scancode;

    spinlock_acquire(&lock);
    scancode = i8042_read();
    if (!ISSET(scancode, 0x80)) {
        printf("%c", keytab[scancode]);
    }
    spinlock_release(&lock);
    return 1;
}

/*
 * Initialize i8042 interrupts (IRQ 1)
 */
static void
i8042_init_intr(void)
{
    struct intr_hand hand = {
        .hand = i8042_irq,
        .name = "i8042-port0",
        .irq = 1
    };

    intr_register(&hand);
}

static int
i8042_init(struct module *modp)
{
    uint8_t byte;
    int error;

    error = clkdev_get(CLKDEV_MSLEEP | CLKDEV_GET_USEC, &clk);
    if (error < 0) {
        printf("i8042: could not get clkdev\n");
        return error;
    }

    i8042_write(true, I8042_DISABLE_PORT0);
    i8042_write(true, I8042_DISABLE_PORT1);
    i8042_init_intr();

    i8042_write(true, I8042_ENABLE_PORT0);
    i8042_read();
    return 0;
}

MODULE_EXPORT("i8042", MODTYPE_GENERIC, i8042_init);
