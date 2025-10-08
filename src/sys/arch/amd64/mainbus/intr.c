/*
 * Copyright (c) 2023-2025 Ian Marco Moffett and the Osmora Team.
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
 * 3. Neither the name of Hyra nor the names of its
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
 * Description: Interrupt registration core
 * Author: Ian Marco Moffett
 */

#include <sys/types.h>
#include <machine/intr.h>
#include <machine/ioapic.h>
#include <os/kalloc.h>
#include <string.h>

/* List of interrupt handlers */
struct intr_hand *g_intrs[256] = {0};

struct intr_hand *
intr_register(const struct intr_hand *ih)
{
    struct intr_hand *ih_new = NULL;
    int gsi;
    uint8_t vec;

    if (ih == NULL) {
        return NULL;
    }

    /* Allocate a new interrupt handler */
    ih_new = kalloc(sizeof(*ih_new));
    if (ih_new == NULL) {
        return NULL;
    }

    /*
     * The first 0x20 to 0x5F interrupt vectors are
     * reserved for I/O APIC input pins
     */
    vec = MAX(ih->ipl << IPL_SHIFT, 0x60);

    /*
     * We can have up to 15 interrupt vectors per
     * priority level as only 4 bits encode the IPL
     */
    for (int i = vec; i < vec + 16; ++i) {
        if (g_intrs[i] != NULL) {
            continue;
        }

        ih_new->name = strdup(ih->name);
        ih_new->ipl = ih->ipl;
        ih_new->irq = ih->irq;
        ih_new->count = ih->count;
        ih_new->vector = i;
        g_intrs[i] = ih_new;

        if (ih->irq >= 0) {
            ioapic_route_vec(ih->irq, ih_new->vector);
            gsi = ioapic_get_gsi(ih->irq);
            ioapic_gsi_mask(gsi, 0);
        }
        return ih_new;

    }

    return NULL;
}
