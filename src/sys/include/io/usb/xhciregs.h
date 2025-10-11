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

#ifndef _XHCIREGS_H_
#define _XHCIREGS_H_ 1

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/param.h>

/*
 * XHCI capability registers
 *
 * See section 5.3 of the xHCI spec
 */
struct __packed xhci_capregs {
    volatile uint8_t caplength;
    volatile uint8_t reserved;
    volatile uint16_t hciversion;
    volatile uint32_t hcsparams1;
    volatile uint32_t hcsparams2;
    volatile uint32_t hcsparams3;
    volatile uint32_t hccparams1;
    volatile uint32_t dboff;
    volatile uint32_t rtsoff;
    volatile uint32_t hccparams2;
};

/*
 * XHCI operational registers
 *
 * See section 5.4 of the xHCI spec
 */
struct __packed xhci_opregs {
    volatile uint32_t usbcmd;
    volatile uint32_t usbsts;
    volatile uint32_t pagesize;
    volatile uint32_t reserved;
    volatile uint32_t dnctrl;
    volatile uint32_t crcr;
    volatile uint32_t reserved1;
    volatile uint32_t dcbaap;
    volatile uint32_t config;
    volatile uint32_t reserved2;
};

/*
 * USB command register bits
 *
 * See section 5.4.1 of the xHCI spec
 */
#define USBCMD_HCRST BIT(1)     /* Host controller reset */

/*
 * USB status register bits
 *
 * See section 5.4.2 of the xHCI spec
 */
#define USBSTS_CNR  BIT(11)     /* Controller not ready */

/*
 * Macros to get various register spaces
 */
#define XHCI_OPBASE(CAPBASE) \
    PTR_OFFSET(CAPBASE, (CAPBASE)->caplength)

#endif  /* !_XHCIREGS_H_ */
