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

#include <sys/bootvars.h>
#include <sys/panic.h>
#include <sys/syslog.h>
#include <acpi/acpi.h>
#include <acpi/tables.h>
#include <vm/vm.h>

static size_t root_sdt_len = 0;
static struct acpi_root_sdt *root_sdt = NULL;
static uintptr_t rsdp_pa = 0;   /* [physical address] */

size_t
acpi_get_root_sdt_len(void)
{
    return root_sdt_len;
}

void *
acpi_get_root_sdt(void)
{
    return root_sdt;
}

/*
 * ACPI initialization
 */
int
acpi_early_init(void)
{
    struct bootvars bootvars;
    struct acpi_rsdp *rsdp;
    int error;

    error = bootvars_read(&bootvars, 0);
    if (error < 0) {
        panic("acpi: failed to read bootvars\n");
    }

    /* Fetch the RSDP */
    rsdp = bootvars.rsdp;
    rsdp_pa = VIRT_TO_PHYS(rsdp);

    /* Fetch the root SDT */
    if (rsdp->revision >= 2) {
        root_sdt = PHYS_TO_VIRT(rsdp->xsdt_addr);
        printf("acpi: using XSDT as root SDT\n");
    } else {
        root_sdt = PHYS_TO_VIRT(rsdp->rsdt_addr);
        printf("acpi: using RSDT as root SDT\n");
    }

    if (acpi_checksum(&root_sdt->hdr) != 0) {
        panic("root SDT checksum is invalid!\n");
    }

    root_sdt_len = (root_sdt->hdr.length - sizeof(root_sdt->hdr)) / 4;
    return 0;
}
