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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/syslog.h>
#include <sys/panic.h>
#include <sys/atomic.h>
#include <sys/cpuvar.h>
#include <sys/limits.h>
#include <os/kalloc.h>
#include <os/spinlock.h>
#include <machine/mdcpu.h>
#include <string.h>
#include <limine.h>

extern struct pcore g_bsp;

static size_t ncores_up = 1;
static struct pcore *corelist[CPU_MAX];
static struct spinlock lock;

/*
 * We'll use the bootloader to keep the kernel small
 *
 * XXX: Maybe move this somewhere better?
 */
static volatile struct limine_smp_request g_smp_req = {
    .id = LIMINE_SMP_REQUEST,
    .revision = 0
};

__dead static void
ap_entry(struct limine_smp_info *)
{
    struct pcore *pcore;

    spinlock_acquire(&lock);
    pcore = kalloc(sizeof(*pcore));
    if (pcore == NULL) {
        panic("mp: could not allocate pcore\n");
    }

    /* Initialize the core */
    memset(pcore, 0, sizeof(*pcore));
    pcore->id = ncores_up;
    cpu_conf(pcore);
    cpu_init(pcore);

    corelist[ncores_up - 1] = pcore;
    atomic_inc_64(&ncores_up);
    spinlock_release(&lock);
    for (;;);
}

/*
 * Get a specific core descriptor
 */
struct pcore *
cpu_get(uint16_t index)
{
    if (index == 0) {
        return &g_bsp;
    }

    if ((index - 1) >= (ncores_up - 1)) {
        return NULL;
    }

    return corelist[index - 1];
}

void
bsp_ap_startup(void)
{
    struct limine_smp_response *resp = g_smp_req.response;
    struct limine_smp_info **cpus;
    struct mdcore *mdcore;
    uint32_t ncores, tmp;

    /* Sanity check */
    if (__unlikely(resp == NULL)) {
        panic("mp: could not get SMP response\n");
    }

    /* Put the AP list in a known state */
    memset(corelist, 0, sizeof(*corelist));
    corelist[0] = &g_bsp;

    /*
     * If the number of cores total exceeds the cap we have
     * set in sys/limits.h, truncate what we start up and
     * log it so any inconsistencies have a definitive
     * source.
     */
    cpus = resp->cpus;
    ncores = MIN(resp->cpu_count, CPU_MAX);
    if (resp->cpu_count >= CPU_MAX) {
        tmp = (resp->cpu_count - ncores - 1);
        printf("mp: not starting %d cores\n", tmp);
    }

    /* Don't continue if we have only one core */
    if (ncores == 1) {
        printf("mp: single cored CPU - no APs to bring up\n");
        return;
    }

    printf("mp: bringing APs online...\n");
    mdcore = &g_bsp.md;

    for (int i = 0; i < ncores; ++i) {
        if (mdcore->apic_id == cpus[i]->lapic_id) {
            continue;
        }

        cpus[i]->goto_address = ap_entry;
    }

    while (ncores_up < ncores);
    printf("mp: %d cores [up]\n", ncores - 1);
}
