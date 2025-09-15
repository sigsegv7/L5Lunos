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
 * Description: Local register interface definitions
 * Author: Ian Marco Moffett
 */

#ifndef _MACHINE_LAPICREGS_H_
#define _MACHINE_LAPICREGS_H_ 1

#include <sys/param.h>
#include <sys/cdefs.h>
#include <vm/vm.h>

#define _LAPIC_MMIO_BASE 0xFEE00000

/* LAPIC register offsets */
#define LAPIC_ID            0x0020U     /* ID Register */
#define LAPIC_VERSION       0x0030U     /* Version Register */
#define LAPIC_TPR           0x0080U     /* Task Priority Register */
#define LAPIC_APR           0x0090U     /* Arbitration Priority Register */
#define LAPIC_PPR           0x00A0U     /* Processor Priority Register */
#define LAPIC_EOI           0x00B0U     /* End Of Interrupt Register */
#define LAPIC_RRD           0x00C0U     /* Remote Read Register */
#define LAPIC_LDR           0x00D0U     /* Logical Destination Register */
#define LAPIC_DFR           0x00E0U     /* Destination Format Register */
#define LAPIC_SVR           0x00F0U     /* Spurious Vector Register */
#define LAPIC_ISR           0x0100U     /* In service register (max=0x0220) */
#define LAPIC_TMR           0x0180U     /* Trigger Mode Register (max=0x0220) */
#define LAPIC_IRR           0x0200U     /* Interrupt Request Register (max=0x0270) */
#define LAPIC_ERR           0x0280U     /* Error Status Register */
#define LAPIC_ICRLO         0x0300U     /* Interrupt Command Low Register */
#define LAPIC_ICRHI         0x0310U     /* Interrupt Command High Register */
#define LAPIC_LVT_TMR       0x0320U     /* LVT Timer Register */
#define LAPIC_DCR           0x03E0U     /* Divide Configuration Register (for timer) */
#define LAPIC_INIT_CNT      0x0380U     /* Initial Count Register (for timer) */
#define LAPIC_CUR_CNT       0x0390U     /* Current Count Register (for timer) */

/*
 * The x2APIC register space is accessed via
 * RDMSR/WRMSR instructions. The below defines
 * the base MSR address for the register space.
 */
#define x2APIC_MSR_BASE 0x00000800

/*
 * To hardware enable, OR the value of the IA32_APIC_BASE
 * MSR with LAPIC_HW_ENABLE and rewrite it.
 *
 * To software enable, OR the value of the SVR with
 * LAPIC_SW_ENABLE and rewrite it.
 *
 * LAPIC_SW_ENABLE has the low 8 bits set as some hardware
 * requires the spurious vector to be hardwired to 1s so
 * we'll go with that to be safe.
 */
#define LAPIC_HW_ENABLE     BIT(11)
#define LAPIC_SW_ENABLE     (BIT(8) | 0xFF)
#define x2APIC_ENABLE_SHIFT 10

/* LVT bits */
#define LAPIC_LVT_MASK            BIT(16)
#define LVT_TMR_ONESHOT           0x00
#define LVT_TMR_PERIODIC          0x01
#define LVT_TMR_TSC_DEADLINE      0x02

/*
 * Convert a APIC destination ID into a MMIO
 * base address for the Local APIC register
 * space of that target processor
 */
__always_inline static void *
lapic_base(uint8_t apic_id)
{
    uint64_t base = _LAPIC_MMIO_BASE;

    base |= (apic_id & 0xFF) << 12;
    return PHYS_TO_VIRT(base);
}

#endif  /* !_MACHINE_LAPICREGS_H_ */
