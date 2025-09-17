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
#include <sys/panic.h>
#include <sys/cpuvar.h>
#include <machine/uart.h>
#include <machine/boot.h>
#include <machine/i8259.h>
#include <machine/ioapic.h>
#include <machine/tss.h>
#include <machine/gdt.h>
#include <stdbool.h>

static void
chipset_init(void)
{
    static bool once = false;

    if (once) {
        return;
    }

    once = true;
    ioapic_init();

    uart_init();
    i8259_disable();
}

/*
 * Initialize and load the task state segment
 */
static void
init_tss(struct pcore *pcore)
{
    struct tss_desc *desc;
    struct mdcore *mdcore;

    mdcore = &pcore->md;
    desc = (struct tss_desc *)&mdcore->gdt[GDT_TSS_INDEX];
    write_tss(pcore, desc);
    tss_load();
}

void
platform_boot(void)
{
    struct pcore *core;

    /* Try to get the current core */
    if ((core = this_core()) == NULL) {
        panic("platform_boot: could not get core\n");
    }

    init_tss(core);
    chipset_init();
}
