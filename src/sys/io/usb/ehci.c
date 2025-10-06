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
#include <sys/syslog.h>
#include <io/pci/pci.h>
#include <io/pci/bar.h>
#include <os/module.h>

#define pr_trace(fmt, ...) printf("ehci: " fmt, ##__VA_ARGS__)
#if defined(AHCI_DEBUG)
#define dtrace(fmt, ...) printf("ehci: " fmt, ##__VA_ARGS__)
#else
#define dtrace(...) __nothing
#endif  /* AHCI_DEBUG */

static struct pci_device dev;
static struct pci_adv driver;

static int
ehci_init(struct module *modp)
{
    int error;

    if ((error = pci_advoc(&driver)) < 0) {
        pr_trace("failed to advocate for xhc\n");
        return error;
    }

    return 0;
}

/*
 * Called when an EHCI host controller is detected
 * on the machine
 */
static int
ehci_attach(struct pci_adv *adv)
{
    dev = adv->lookup;
    pr_trace("detected EHCI controller\n");
    return 0;
}

static struct pci_adv driver = {
    .lookup = PCI_CS_ID(0x0C, 0x03),
    .attach = ehci_attach,
    .classrev = 1,
};

MODULE_EXPORT("ehci", MODTYPE_PCI, ehci_init);
