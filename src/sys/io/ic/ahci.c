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
 * Description: AHCI HBA driver
 * Author: Ian Marco Moffett
 */

#include <sys/types.h>
#include <sys/syslog.h>
#include <sys/cdefs.h>
#include <io/pci/pci.h>
#include <os/module.h>

/*
 * Represents the PCI advocation descriptor for this
 * device so we can advertise ourselves as its driver.
 */
static struct pci_adv driver;

/*
 * Initialize only the AHCI driver's state rather than
 * the hardware it covers. This is used so we can advertise
 * ourself to the PCI driver.
 */
static int
ahci_init(struct module *modp)
{
    int error;

    if ((error = pci_advoc(&driver)) < 0) {
        printf("ahci_init: failed to advocate for HBA\n");
        return error;
    }
    return 0;
}

/*
 * Ran when a device is attached, does the actual
 * hardware initialization.
 */
static int
ahci_attach(struct pci_adv *adv)
{
    printf("ahci: detected AHCI controller\n");
    return 0;
}

static struct pci_adv driver = {
    .lookup = PCI_CS_ID(0x1, 0x06),
    .attach = ahci_attach,
    .classrev = 1
};

MODULE_EXPORT("ahci", MODTYPE_PCI, ahci_init);
