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
#include <sys/syslog.h>
#include <acpi/tables.h>
#include <acpi/acpi.h>
#include <os/mmio.h>
#include <machine/ioapic.h>
#include <machine/ioapicvar.h>

static struct ioapic *ioapic = NULL;
static struct acpi_madt *madt = NULL;

/*
 * Write a single 32-bit value to a specific
 * I/O APIC register via MMIO.
 *
 * @ioapic: Target I/O APIC unit
 * @reg: Register to write to
 * @val: Value to write to register
 */
static void
ioapic_writel(uint32_t reg, uint32_t val)
{
    uint32_t *ioregsel;
    uint32_t *iowin;

    if (ioapic == NULL) {
        printf("ioapic_writel: error: got NULL `ioapic'\n");
        return;
    }

    ioregsel = PTR_OFFSET(ioapic->ioapic_addr, IOREGSEL);
    iowin = PTR_OFFSET(ioapic->ioapic_addr, IOWIN);

    /* Latch register and write value */
    mmio_write32(ioregsel, reg);
    mmio_write32(iowin, val);
}

/*
 * Read a single 32-bit value from a specific I/O
 * APIC register via MMIO
 *
 * @ioapic: I/O APIC unit to target
 * @reg: Register to read
 *
 * Returns the register value
 */
static uint32_t
ioapic_readl(uint32_t reg)
{
    uint32_t *ioregsel;
    uint32_t *iowin;

    if (ioapic == NULL) {
        printf("ioapic_readl: error: got NULL `ioapic'\n");
        return 0;
    }

    ioregsel = PTR_OFFSET(ioapic->ioapic_addr, IOREGSEL);
    iowin = PTR_OFFSET(ioapic->ioapic_addr, IOWIN);

    /* Latch register and read value */
    mmio_write32(ioregsel, reg);
    return mmio_read32(iowin);
    return 0;
}

/*
 * Reads an I/O APIC redirection entry.
 *
 * @entry: Entry variable to read into.
 * @index: Index to read.
 */
static void
ioapic_read_redentry(union ioapic_redentry *entry, uint8_t index)
{
    uint32_t lo, hi;

    lo = ioapic_readl(IOREDTBL + index * 2);
    hi = ioapic_readl(IOREDTBL + index * 2 + 1);
    entry->value = ((uint64_t)hi << 32) | lo;
}

/*
 * Writes an I/O APIC redirection entry.
 *
 * @entry: Entry variable to write.
 * @index: Index to write to.
 */
static void
ioapic_write_redentry(const union ioapic_redentry *entry, uint8_t index)
{
    ioapic_writel(IOREDTBL + index * 2, (uint32_t)entry->value);
    ioapic_writel(IOREDTBL + index * 2 + 1, (uint32_t)(entry->value >> 32));
}

/*
 * Read a MADT entry
 *
 * @type: Type that we should scan for
 * @cb: Callback (returns 0 if entry is found)
 * @arg: Optional argument
 *
 * Returns the callback return value, on success,
 * otherwise a less than zero value on failure.
 */
static int
ioapic_read_madt(uint32_t type, int(*cb)(struct apic_header *, size_t arg),
    size_t arg)
{
    uint8_t *cur, *end;
    int retval = -1;
    struct apic_header *apichdr;

    /* Try to read the MADT table */
    madt = acpi_query("APIC");
    if (madt == NULL) {
        panic("ioapic_read_madt: failed to get MADT\n");
    }

    cur = (uint8_t *)(madt + 1);
    end = (uint8_t *)madt + madt->hdr.length;
    while (cur < end) {
        apichdr = (void *)cur;
        if (apichdr->type == type) {
            retval = cb(apichdr, arg);
        }

        /* If the entry was found, stop */
        if (retval >= 0) {
            return retval;
        }

        cur += apichdr->length;
    }

    return -1;
}

/*
 * Set the I/O APIC MMIO base address
 */
static int
__ioapic_callback(struct apic_header *hdr, size_t arg)
{
    if (ioapic != NULL) {
        return 0;
    }

    ioapic = (struct ioapic *)hdr;
    return 0;
}

}

/*
 * Mask or unmask a GSI
 */
void
ioapic_gsi_mask(uint8_t gsi, uint8_t mask)
{
    union ioapic_redentry redent;

    ioapic_read_redentry(&redent, gsi);
    redent.interrupt_mask = gsi & 1;
    ioapic_write_redentry(&redent, gsi);
}

/*
 * Initialize the I/O APIC
 */
void
ioapic_init(void)
{
    union ioapic_redentry redent;
    uint32_t ioapicver;
    uint8_t ver, nredir;

    if (ioapic == NULL) {
        ioapic_read_madt(
            APIC_TYPE_IO_APIC,
            __ioapic_callback,
            0
        );
    }

    /* Read the IOAPIC version register */
    ioapicver = ioapic_readl(IOAPICVER);
    ver = ioapicver & 0xFF;
    nredir = (ioapicver >> 16) & 0xFF;

    printf("ioapic: ioapic @ mainbus:ver=%x,nredir=%d\n"
           "ioapic: masking all %d pins...\n",
           ver, nredir, nredir
    );

    /* Mask each and every entry */
    for (int i = 0; i < nredir; ++i) {
        ioapic_gsi_mask(i, IOAPIC_PIN_MASK);
    }
}
