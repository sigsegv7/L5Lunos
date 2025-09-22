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
#include <io/dma/alloc.h>

/*
 * Represents an AHCI host bus adapter (HBA)
 *
 * @io: HBA register space
 * @pi: Ports implemented
 * @nport: Number of ports supported
 * @nslots: Number of command slots
 *
 * XXX: Just because 'n' ports are _supported_ by the HBA does
 *      not mean the host will implement exactly 'n' ports.
 */
struct ahci_hba {
    volatile struct hba_memspace *io;
    uint32_t pi;
    uint32_t nport;
    uint8_t nslots;
};

/*
 * Represents a port on the host bus adapter
 *
 * @parent: Parent HBA this port belongs to
 * @io: Port register space
 * @cmdlist: Command list base
 * @fis_rx: FIS recieve area
 * @portno: Port number
 */
struct ahci_port {
    volatile struct ahci_hba *parent;
    volatile struct hba_port *io;
    dma_addr_t cmdlist;
    dma_addr_t fis_rx;
    uint32_t portno;
    TAILQ_ENTRY(ahci_port) link;
};

/*
 * Command header
 *
 * @cfl: Command FIS length
 * @a: ATAPI
 * @w: Write
 * @p: Prefetchable
 * @r: Reset
 * @b: BIST self test
 * @c: Clear busy upon R_OK
 * @rsvd0: Reserved
 * @pmp: Port multiplier port
 * @prdtl: PRDT length (in entries)
 * @prdbc: PRDT bytes transferred count
 * @ctba: Command table descriptor base addr
 * @rsvd1: Reserved
 */
struct ahci_cmd_hdr {
    uint8_t cfl : 5;
    uint8_t a   : 1;
    uint8_t w   : 1;
    uint8_t p   : 1;
    uint8_t r   : 1;
    uint8_t b   : 1;
    uint8_t c   : 1;
    uint8_t rsvd0 : 1;
    uint8_t pmp   : 4;
    uint16_t prdtl;
    volatile uint32_t prdbc;
    uintptr_t ctba;
    uint32_t rsvd1[4];
};

/*
 * Physical region descriptor
 *
 * @dba: Data base address
 * @rsvd0: Reserved
 * @dbc: Count
 * @rsvd1: Reserved
 * @i: Interrupt on completion
 */
struct ahci_prdt_entry {
    uintptr_t dba;
    uint32_t rsvd0;
    uint32_t dbc : 22;
    uint16_t rsvd1 : 9;
    uint8_t i : 1;
};

/*
 * Command table
 *
 * @cfis: Command FIS
 * @acmd: ATAPI command
 * @rsvd: Reserved
 * @prdt: Physical region descriptors
 */
struct ahci_cmdtab {
    uint8_t cfis[64];
    uint8_t acmd[16];
    uint8_t rsvd[48];
    struct ahci_prdt_entry prdt[1];
};

/*
 * Host to device FIS
 *
 * [h]: Set by host
 * [d]: Set by device
 * [srb]: Shadow register block
 *
 * @type: Must be 0x27 for H2D [h]
 * @pmp: Port multiplier port [h]
 * @c: Set to denote command FIS [h]
 * @command: Command type [h/srb]
 * @feature1: Features register (7:0) [h/srb]
 * @lba0: LBA low [h/srb]
 * @lba1: LBA mid [h/srb]
 * @lba2: LBA hi  [h/srb]
 * @device: Set bit 7 for LBA [h/srb]
 * @lba3: LBA (31:24) [h/srb]
 * @lba4: LBA (39:32) [h/srb]
 * @lba5: LBA (47:40) [h/srb]
 * @featureh: Features high [h/srb]
 * @countl: Count low (block aligned) [h/srb]
 * @counth: Count high (block aligned) [h/srb]
 */
struct ahci_fis_h2d {
    uint8_t type;
    uint8_t pmp : 4;
    uint8_t rsvd0 : 3;
    uint8_t c : 1;
    uint8_t command;
    uint8_t featurel;
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t featureh;
    uint8_t countl;
    uint8_t counth;
    uint8_t icc;
    uint8_t control;
    uint8_t rsvd1[4];
};

#define AHCI_TIMEOUT 500    /* In ms */

/* AHCI size constants */
#define AHCI_FIS_SIZE 256
#define AHCI_CMDTAB_SIZE 256
#define AHCI_CMDENTRY_SIZE 32
#define AHCI_SECTOR_SIZE 512

/* AHCI FIS types */
#define FIS_TYPE_H2D 0x27
#define FIS_TYPE_D2H 0x34

/* ATA commands */
#define ATA_CMD_NOP         0x00
#define ATA_CMD_IDENTIFY    0xEC
#define ATA_CMD_READ_DMA    0x25
#define ATA_CMD_WRITE_DMA   0x35

#define AHCI_TIMEOUT 500

#endif  /* !_IC_AHCIVAR_H_ */
