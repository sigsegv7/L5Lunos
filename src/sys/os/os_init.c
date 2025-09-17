#include <sys/cdefs.h>
#include <sys/panic.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <sys/cpuvar.h>
#include <os/sched.h>
#include <os/elfload.h>
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
    printf("booting l5 lunos v0.0.1...\n");
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

    acpi_early_init();

    cons_init();
    syslog_toggle(true);
    boot_print();

    cpu_conf(&g_bsp);
    vm_init();

    cpu_init(&g_bsp);
    bsp_ap_startup();

    sched_init();
    core = this_core();
    proc_init(&g_rootproc, 0);
    core->curproc = &g_rootproc;

    error = elf_load("/usr/bin/init", &g_rootproc, &elf);
    if (error < 0) {
        panic("could not load init\n");
    }

    md_set_ip(&g_rootproc, elf.entrypoint);
    md_proc_kick(&g_rootproc);
    panic("end of kernel reached\n");
    for (;;);
}
