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
#include <sys/proc.h>
#include <sys/errno.h>
#include <os/module.h>
#include <os/clkdev.h>
#include <os/iotap.h>
#include <os/spinlock.h>
#include <machine/intr.h>
#include <machine/i8042var.h>
#include <machine/pio.h>
#include <stdbool.h>

#define RING_NENT 16

/*
 * Keybuffer - holds keybuffer entries
 *
 * @ring: Scancode ring
 * @head: Data is pushed here
 * @tail: Data is popped here
 */
struct keybuf {
    uint8_t ring[RING_NENT];
    uint8_t head;
    uint8_t tail;
};

/* I/O tap forward declarations */
static struct iotap_ops tap_port0_ops;
static struct iotap_desc tap_port0;

static struct clkdev *clk;
static struct spinlock lock;
static struct keybuf buf = {0};

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
 * Compute the length of a kbp ring
 */
__always_inline static inline size_t
keybuf_len(const struct keybuf *kbp)
{
    return (kbp->head - kbp->tail);
}

/*
 * Enter a scancode into the key buffer
 *
 * @kbp: Key buffer to place scancode in
 * @scancode: Scancode to enter
 */
static void
keybuf_enter(struct keybuf *kbp, uint8_t scancode)
{
    if (kbp == NULL) {
        return;
    }

    if (kbp->head >= RING_NENT) {
        return;
    }

    spinlock_acquire(&lock);
    kbp->ring[kbp->head++] = scancode;
    spinlock_release(&lock);
}

/*
 * Pop a character from the scancode buffer
 *
 * Returns the scancode on success, otherwise
 * a less than zero errno value.
 */
static int8_t
keybuf_pop(struct keybuf *kbp)
{
    uint8_t tail;

    if (kbp == NULL) {
        return -EINVAL;
    }

    if (kbp->head == kbp->tail) {
        return -EAGAIN;
    }

    tail = kbp->ring[kbp->tail++];
    if (kbp->tail >= RING_NENT) {
        kbp->tail = 0;
        kbp->head = 0;
    }
    return tail;
}

/*
 * Tap the i8042 using I/O tap
 */
static ssize_t
i8042_read_tap(struct iotap_desc *desc, void *p, size_t len)
{
    size_t maxlen, i = 0;
    char *pbuf = p;
    int8_t scancode;

    /* Truncate if needed */
    spinlock_acquire(&lock);
    maxlen = keybuf_len(&buf);
    if (len == 0) {
        goto done;
        return -EAGAIN;
    }

    /*
     * Pop as many characters as we can, if after reading a few
     * bytes, return the length. If we fail before we read anything,
     * return -EAGAIN.
     */
    for (i = 0; i < MIN(len, maxlen); ++i) {
        scancode = keybuf_pop(&buf);
        spinlock_release(&lock);

        /* Failed and haven't read anything? */
        if (scancode < 0) {
            break;
        }

        if (i < len) {
            pbuf[i] = scancode;
        }
    }

done:
    spinlock_release(&lock);
    return (i == 0) ? -EAGAIN : i;
}

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
    uint8_t count = 0;

    while (!ISSET(status, I8042_OBUFF)) {
        status = inb(I8042_STATUS);
        clk->msleep(5);

        /* Timeout? */
        if ((count++) >= 20) {
            return 0xFF;
        }
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

    scancode = i8042_read();
    if (!ISSET(scancode, 0x80)) {
        keybuf_enter(&buf, scancode);
    }
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

/*
 * Initialize an I/O tap for the
 * i8042
 */
static int
i8042_init_tap(void)
{
    int error;

    /* Register the tap */
    error = iotap_register(&tap_port0);
    if (error < 0) {
        return error;
    }

    return 0;
}

static int
i8042_init(struct module *modp)
{
    int error;

    error = clkdev_get(CLKDEV_MSLEEP | CLKDEV_GET_USEC, &clk);
    if (error < 0) {
        printf("i8042: could not get clkdev\n");
        return error;
    }

    /* Disable both ports and flush any data */
    i8042_write(true, I8042_DISABLE_PORT0);
    i8042_write(true, I8042_DISABLE_PORT1);
    i8042_read();

    /* Initialize interrupts and taps */
    i8042_init_intr();
    i8042_init_tap();

    /* Enable the keyboard */
    i8042_write(true, I8042_ENABLE_PORT0);
    return 0;
}

static struct iotap_ops tap_port0_ops = {
    .read = i8042_read_tap
};

static struct iotap_desc tap_port0 = {
    .name = "i8042.port.0",
    .ops = &tap_port0_ops
};

MODULE_EXPORT("i8042", MODTYPE_GENERIC, i8042_init);
