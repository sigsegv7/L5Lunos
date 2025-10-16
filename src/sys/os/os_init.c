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

#include <sys/cdefs.h>
#include <sys/panic.h>
#include <sys/sysvar.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <sys/cpuvar.h>
#include <os/sched.h>
#include <os/elfload.h>
#include <os/vfs.h>
#include <os/nsvar.h>
#include <os/module.h>
#include <acpi/acpi.h>
#include <io/cons/cons.h>
#include <vm/vm.h>
#include <logo.h>

struct pcore g_bsp;
struct proc g_rootproc;

static void
boot_print(void)
{
    printf("%s\n", g_LOGO);
    printf("Copyright (c) 2025 Ian Marco Moffett, et al\n");
    printf("booting l5 lunos %s...\n", _L5_VERSION);
}

/*
 * Kernel entrypoint
 */
__dead void
main(void)
{
    struct loaded_elf elf;
    struct pcore *core;
    int error;

    cons_init();
    syslog_toggle(true);
    boot_print();

    acpi_early_init();
    cpu_conf(&g_bsp);
    vm_init();

    cpu_init(&g_bsp);
    bsp_ap_startup();
    vfs_init();
    ns_init();

    sched_init();

    /* Initialize generic modules */
    __MODULES_INIT(MODTYPE_GENERIC);

    core = this_core();
    proc_init(&g_rootproc, 0);
    core->curproc = &g_rootproc;

    error = elf_load("/usr/bin/init", &g_rootproc, &elf);
    if (error < 0) {
        panic("could not load init\n");
    }

    syslog_toggle(false);
    md_set_ip(&g_rootproc, elf.entrypoint);
    md_proc_kick(&g_rootproc);
    panic("end of kernel reached\n");
    for (;;);
}
