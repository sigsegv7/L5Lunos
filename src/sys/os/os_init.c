#include <sys/cdefs.h>
#include <sys/panic.h>
#include <sys/syslog.h>
#include <sys/cpuvar.h>
#include <os/sched.h>
#include <acpi/acpi.h>
#include <io/cons/cons.h>
#include <vm/vm.h>
#include <logo.h>

struct pcore g_bsp;

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

    acpi_early_init();

    cons_init();
    syslog_toggle(true);
    boot_print();

    cpu_conf(&g_bsp);
    vm_init();

    cpu_init(&g_bsp);
    bsp_ap_startup();

    sched_init();
    panic("end of kernel reached\n");
    for (;;);
}
