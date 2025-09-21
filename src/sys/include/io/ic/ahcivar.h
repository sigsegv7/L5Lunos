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

#ifndef _IC_AHCIVAR_H_
#define _IC_AHCIVAR_H_  1

#include <sys/queue.h>
#include <io/ic/ahciregs.h>

/*
 * Represents an AHCI host bus adapter (HBA)
 *
 * @io: HBA register space
 * @pi: Ports implemented
 * @nport: Number of ports supported
 *
 * XXX: Just because 'n' ports are _supported_ by the HBA does
 *      not mean the host will implement exactly 'n' ports.
 */
struct ahci_hba {
    volatile struct hba_memspace *io;
    uint32_t pi;
    uint32_t nport;
};

/*
 * Represents a port on the host bus adapter
 *
 * @parent: Parent HBA this port belongs to
 * @io: Port register space
 * @portno: Port number
 */
struct ahci_port {
    volatile struct ahci_hba *parent;
    volatile struct hba_port *io;
    uint32_t portno;
    TAILQ_ENTRY(ahci_port) link;
};

#define AHCI_TIMEOUT 500

#endif  /* !_IC_AHCIVAR_H_ */
