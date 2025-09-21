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

/*
 * Description: IA-PC HPET timer driver
 * Author: Ian Marco Moffett
 */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/syslog.h>
#include <os/mmio.h>
#include <os/clkdev.h>
#include <acpi/acpi.h>
#include <acpi/tables.h>
#include <machine/hpet.h>

#define HPET_REG_CAPS               0x00
#define HPET_GENERAL_CONFIG         0x10
#define HPET_REG_MAIN_COUNTER       0xF0

#define CAP_REV_ID(caps)            (caps & 0xFF)
#define CAP_NUM_TIM(caps)           (caps >> 8) & 0x1F
#define CAP_CLK_PERIOD(caps)        (caps >> 32)

static struct clkdev clkdev;
static void *hpet_base = NULL;

/*
 * Read from HPET register space.
 *
 * @reg: Register to read from.
 */
static inline uint64_t
hpet_read(uint32_t reg)
{
    void *addr;

    addr = PTR_OFFSET(hpet_base, reg);
    return mmio_read64(addr);
}

/*
 * Write to HPET register space.
 *
 * @reg: Register to write to.
 * @val: Value to write.
 */
static inline void
hpet_write(uint32_t reg, uint64_t val)
{
    void *addr;

    addr = PTR_OFFSET(hpet_base, reg);
    mmio_write64(addr, val);
}

/*
 * Sleep for 'n' amount of 'units'
 *
 * @n: Number of 'units' to sleep
 * @units: Units per 'n'
 *
 * Returns zero on success, otherwise a less than zero
 * value.
 */
static int
hpet_sleep(uint64_t n, uint64_t units)
{
    uint64_t caps;
    uint32_t period;
    uint64_t counter_val;
    volatile size_t ticks;

    caps = hpet_read(HPET_REG_CAPS);
    period = CAP_CLK_PERIOD(caps);
    counter_val = hpet_read(HPET_REG_MAIN_COUNTER);

    ticks = counter_val + (n * (units / period));

    while (hpet_read(HPET_REG_MAIN_COUNTER) < ticks) {
        __ASMV("rep; nop");
    }

    return 0;
}

static int
hpet_msleep(size_t ms)
{
    return hpet_sleep(ms, 1000000000000);
}

/*
 * Initialize the HPET
 */
int
hpet_init(void)
{
    struct acpi_gas *gas;
    struct acpi_hpet *hpet;
    uint64_t caps;

    hpet = acpi_query("HPET");
    if (hpet == NULL) {
        return -ENODEV;
    }

    gas = &hpet->gas;
    hpet_base = (void *)gas->address;

    caps = hpet_read(HPET_REG_CAPS);
    if (CAP_REV_ID(caps) == 0) {
        printf("hpet_init: bad revision HPET ID\n");
        return -1;
    }

    /*
     * The spec states this counter clk period must
     * be <= 0x05F5E100. So we'll consider it as bogus
     * if it exceeds this value
     */
    if (CAP_CLK_PERIOD(caps) > 0x05F5E100) {
        printf("hpet_init: bad COUNTER_CLK_PERIOD\n");
        return -1;
    }

    /*
     * Clear the counter and enable the HPET via
     * the ENABLE_CNF bit in general_config
     */
    hpet_write(HPET_REG_MAIN_COUNTER, 0);
    hpet_write(HPET_GENERAL_CONFIG, 1);
    printf("hpet: HPET initialized and enabled\n");

    /* Initialize as clock device */
    clkdev.name = "IA-PC HPET";
    clkdev.attr = CLKDEV_MSLEEP;
    clkdev.msleep = hpet_msleep;
    if (clkdev_register(&clkdev) < 0) {
        printf("hpet_init: could not register clock device\n");
        return -1;
    }

    return 0;
}
