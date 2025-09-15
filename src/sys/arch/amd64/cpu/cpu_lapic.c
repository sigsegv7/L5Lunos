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
 * Description: Local APIC driver
 * Author: Ian Marco Moffett
 */

#include <sys/syslog.h>
#include <sys/cpuvar.h>
#include <sys/panic.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/types.h>
#include <os/mmio.h>
#include <machine/mdcpu.h>
#include <machine/lapicregs.h>
#include <machine/lapic.h>
#include <machine/cpuid.h>
#include <machine/msr.h>

#define pr_trace(fmt, ...) printf("lapic: " fmt, ##__VA_ARGS__)
#define bsp_trace(fmt, ...) if (lapic_is_bsp()) pr_trace(fmt, ##__VA_ARGS__)

/*
 * Returns true if the current processor is the
 * bootstrap processor.
 */
__always_inline static inline bool
lapic_is_bsp(void)
{
    uint64_t apic_base;

    apic_base = rdmsr(IA32_APIC_BASE_MSR);
    return ISSET(apic_base, BIT(8)) != 0;
}

/*
 * Returns true if the processor supports the
 * x2APIC.
 */
__always_inline static inline bool
lapic_has_x2apic(void)
{
    uint32_t unused, ecx;

    CPUID(1, unused, unused, ecx, unused);
    return ISSET(ecx, BIT(21)) != 0;
}

/*
 * Reads a 32 bit value from Local APIC
 * register space.
 *
 * @ci: Target processor
 * @reg: Register to read from.
 */
static inline uint64_t
lapic_readl(const struct mdcore *core, uint32_t reg)
{
    void *addr;

    if (!core->x2apic) {
        addr = PTR_OFFSET(core->lapic_base, reg);
        return mmio_read32(addr);
    } else {
        reg >>= 4;
        return rdmsr(x2APIC_MSR_BASE + reg);
    }
}

/*
 * Reads the Local APIC ID of the current
 * processor.
 *
 * @ci: Current processor
 */
static inline uint32_t
lapic_read_id(const struct mdcore *core)
{
    if (!core->x2apic) {
        return (lapic_readl(core, LAPIC_ID) >> 24) & 0xF;
    } else {
        return lapic_readl(core, LAPIC_ID);
    }
}

/*
 * Writes a 32 bit value to Local APIC
 * register space.
 *
 * @ci: Target processor
 * @reg: Register to write to.
 * @val: Value to write
 */
static inline void
lapic_writel(const struct mdcore *core, uint32_t reg, uint64_t val)
{
    void *addr;

    if (!core->x2apic) {
        addr = PTR_OFFSET(core->lapic_base, reg);
        mmio_write32(addr, val);
    } else {
        reg >>= 4;
        wrmsr(x2APIC_MSR_BASE + reg, val);
    }
}

/*
 * Hardware and software enable the Local APIC
 * through IA32_APIC_BASE_MSR and the SVR.
 *
 * @ci: Current processor
 */
static inline void
lapic_enable(const struct mdcore *core)
{
    uint64_t tmp;

    /* Hardware enable the Local APIC */
    tmp = rdmsr(IA32_APIC_BASE_MSR);
    tmp |= core->x2apic << x2APIC_ENABLE_SHIFT;
    wrmsr(IA32_APIC_BASE_MSR, tmp | LAPIC_HW_ENABLE);

    /* Software enable the Local APIC */
    tmp = lapic_readl(core, LAPIC_SVR);
    lapic_writel(core, LAPIC_SVR, tmp | LAPIC_SW_ENABLE);
}

void
lapic_init(void)
{
    struct pcore *core = this_core();
    struct mdcore *mdcore;

    if (__unlikely(core == NULL)) {
        panic("lapic_init: unable to get current core\n");
    }

    mdcore = &core->md;
    bsp_trace("detected lapic0 @ core %d\n", core->id);
    mdcore->x2apic = lapic_has_x2apic();

    /* If we can, put the chip in x2APIC mode on startup */
    lapic_enable(mdcore);
    bsp_trace("lapic0 enabled in %sapic mode\n", mdcore->x2apic ? "x2" : "x");
}
