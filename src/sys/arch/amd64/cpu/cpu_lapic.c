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
#include <acpi/acpi.h>
#include <acpi/tables.h>
#include <machine/mdcpu.h>
#include <machine/lapicregs.h>
#include <machine/lapic.h>
#include <machine/idt.h>
#include <machine/cpuid.h>
#include <machine/i8254.h>
#include <machine/msr.h>

#define pr_trace(fmt, ...) printf("lapic: " fmt, ##__VA_ARGS__)
#define bsp_trace(fmt, ...) if (lapic_is_bsp()) pr_trace(fmt, ##__VA_ARGS__)

extern void lapic_tmr_isr(void);

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

/*
 * Put the local APIC timer in a halted state
 *
 * @core: Current processor core
 */
static void
lapic_timer_stop(const struct mdcore *core)
{
    lapic_writel(core, LAPIC_LVT_TMR, LAPIC_LVT_MASK);
    lapic_writel(core, LAPIC_INIT_CNT, 0);
}

/*
 * Start the timer with a specific mode, mask and count
 *
 * @core: Current processor with the target LAPIC
 * @mask: If true, mask it so that no interrupts will fire
 * @mode: Timer operation mode (see LVT_TMR_*)
 * @cnt:  Counter value to start decrementing at
 */
static void
lapic_timer_start(const struct mdcore *core, bool mask, uint8_t mode, uint32_t cnt)
{
    uint32_t tmp;

    tmp = (mode << 17) | (mask << 16) | LAPIC_TIMER_VEC;
    lapic_writel(core, LAPIC_LVT_TMR, tmp);
    lapic_writel(core, LAPIC_DCR, 0x00);
    lapic_writel(core, LAPIC_INIT_CNT, cnt);
}

/*
 * Init the Local APIC timer and return
 * the frequency.
 *
 * @ci: Current processor
 */
static size_t
lapic_timer_init(struct mdcore *core)
{
    uint16_t ticks_start, ticks_end;
    size_t ticks_total, freq;
    const uint16_t MAX_SAMPLES = 0xFFFF;

    lapic_timer_stop(core);
    i8254_set_reload(MAX_SAMPLES);
    ticks_start = i8254_get_count();

    lapic_writel(core, LAPIC_INIT_CNT, MAX_SAMPLES);
    while (lapic_readl(core, LAPIC_CUR_CNT) != 0);

    ticks_end = i8254_get_count();
    ticks_total = ticks_start - ticks_end;

    freq = (MAX_SAMPLES / ticks_total) * I8254_DIVIDEND;
    lapic_timer_stop(core);
    return freq;
}

/*
 * Start Local APIC timer oneshot with number
 * of ticks to count down from.
 *
 * @mask: If `true', timer will be masked, `count' should be 0.
 * @count: Number of ticks.
 */
static void
lapic_timer_oneshot(bool mask, uint32_t count)
{
    struct pcore *core;

    if ((core = this_core()) == NULL) {
        return;
    }

    lapic_timer_start(&core->md, mask, LVT_TMR_ONESHOT, count);
}

/*
 * Start Local APIC timer oneshot in usec
 */
void
lapic_timer_oneshot_us(size_t usec)
{
    uint64_t ticks;
    struct pcore *core;
    struct mdcore *md;

    if ((core = this_core()) == NULL) {
        return;
    }

    md = &core->md;
    ticks = usec * (md->lapic_tmr_freq / 1000000);
    lapic_timer_oneshot(false, ticks);
}

/*
 * Send an end-of-interrupt to the current
 * core's Local APIC
 */
void
lapic_eoi(void)
{
    struct pcore *core = this_core();

    if (core == NULL) {
        return;
    }

    lapic_writel(&core->md, LAPIC_EOI, 0);
}

void
lapic_init(void)
{
    union tss_stack tmr_stack;
    struct pcore *core = this_core();
    struct mdcore *mdcore;
    struct acpi_madt *madt;
    bool is_relocated;

    if (__unlikely(core == NULL)) {
        panic("lapic_init: unable to get current core\n");
    }

    /* Try to allocate LAPIC timer interrupt stack */
    if (tss_alloc_stack(&tmr_stack, DEFAULT_PAGESIZE) != 0) {
        panic("failed to allocate LAPIC TMR stack!\n");
    }

    /* We need the MADT */
    madt = acpi_query("APIC");
    if (madt == NULL) {
        panic("lapic_init: failed to fetch MADT\n");
    }

    /*
     * Usually the Local APIC register interface starts
     * at MMIO address 0xFEE00000. However, as we are
     * making this assumption, we'll need to check in
     * case some weird firmware moved it.
     */
    is_relocated = madt->lapic_addr != _LAPIC_MMIO_BASE;
    if (__unlikely(is_relocated)) {
        panic("lapic_init: MMIO base not at %p\n", _LAPIC_MMIO_BASE);
    }

    /* Set up the timer interrupt */
    tss_update_ist(core, tmr_stack, IST_SCHED);
    idt_set_desc(
        LAPIC_TIMER_VEC, IDT_INT_GATE,
        ISR(lapic_tmr_isr), IST_SCHED
    );

    mdcore = &core->md;
    bsp_trace("detected lapic0 @ core %d\n", core->id);
    mdcore->x2apic = lapic_has_x2apic();

    /* If we can, put the chip in x2APIC mode on startup */
    lapic_enable(mdcore);
    bsp_trace("lapic0 enabled in %sapic mode\n", mdcore->x2apic ? "x2" : "x");

    /* Calibrate the timer */
    mdcore->lapic_tmr_freq = lapic_timer_init(mdcore);
}
