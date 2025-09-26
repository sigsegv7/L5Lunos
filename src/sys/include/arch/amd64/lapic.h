
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

#ifndef _MACHINE_LAPIC_H_
#define _MACHINE_LAPIC_H_ 1

#define LAPIC_TIMER_VEC 0x81

/*
 * Represents the possible values for the ICR
 * destination shorthand register for IPIs
 *
 * @IPI_SHAND_NONE: No shorthand
 * @IPI_SHAND_SELF: Send IPI to self
 * @IPI_SHAND_AIS: Send IPI to all including self
 * @IPI_SHAND_AXS: Send IPI to all excluding self
 */
typedef enum {
    IPI_SHAND_NONE,
    IPI_SHAND_SELF,
    IPI_SHAND_AIS,
    IPI_SHAND_AXS
} ipi_shand_t;

/*
 * Represents the possible values for the ICR
 * delivery mode
 */
#define IPI_DELMOD_FIXED    0x0     /* Fixed */
#define IPI_DELMOD_LOWPRI   0x1     /* Lowest priority */
#define IPI_DELMOD_INIT     0x5     /* Init (for bootstrap) */
#define IPI_DELMOD_STARTUP  0x6     /* Startup (for bootstrap) */

/* See above */
typedef uint8_t ipi_delmod_t;

/* Destination mode */
#define IPI_DESTMODE_PHYSICAL 0x0
#define IPI_DESTMODE_LOGICAL  0x01

/*
 * Represents parameters used for generating
 * IPIs on the mainbus
 *
 * @shorthand: Destination shorthand
 * @delmod: Delivery mode
 * @vector: Interrupt vector to trigger
 * @dest_mode: Destination mode
 */
struct lapic_ipi {
    ipi_shand_t shorthand;
    ipi_delmod_t delmod;
    uint8_t vector;
    uint8_t apic_id;
    uint8_t dest_mode : 1;
};

/*
 * Transmit an IPI on the main bus to another processor,
 * self, or both
 *
 * @ipip: IPI descriptor
 *
 * Returns zero on success, otherwise a less than zero value
 * on failure.
 */
int lapic_tx_ipi(const struct lapic_ipi *ipip);

/*
 * Initialize the local APIC on the current
 * processor.
 */
void lapic_init(void);

/*
 * Send an end-of-interrupt message to the current
 * processor's Local APIC
 */
void lapic_eoi(void);

/*
 * Start Local APIC timer oneshot in microseconds.
 *
 * @us: Microseconds.
 */
void lapic_timer_oneshot_us(size_t usec);

#endif  /* !_MACHINE_LAPIC_H_ */
