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

#ifndef _MACHINE_ACPI_H_
#define _MACHINE_ACPI_H_

#include <sys/types.h>
#include <acpi/tables.h>

/*
 * Perform an ACPI checksum on a specific ACPI
 * header
 *
 * @hdr: Header to be verified
 *
 * Returns zero on success, otherwise a less than zero
 * value if the checksum is invalid
 */
int acpi_checksum(struct acpi_header *hdr);

/*
 * Get the number of entries within the root system
 * descriptor table
 */
size_t acpi_get_root_sdt_len(void);

/*
 * Get the root system descriptor table base
 * address.
 */
void *acpi_get_root_sdt(void);

/*
 * Query for an ACPI table
 *
 * @query: Signature of table to query
 *
 * Returns virtual base address of table on success,
 * otherwise NULL on failure.
 */
void *acpi_query(const char *query);

/*
 * Initialize the ACPI subsystem
 *
 * Returns zero on success, otherwise a less than zero
 * value on failure.
 */
int acpi_init(void);

#endif  /* !_MACHINE_ACPI_H_ */
