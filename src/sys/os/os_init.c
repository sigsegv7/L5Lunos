#include <sys/cdefs.h>
#include <sys/panic.h>
#include <sys/syslog.h>
#include <sys/cpuvar.h>
#include <io/cons/cons.h>
#include <vm/vm.h>

struct pcore g_bsp;

/*
 * Kernel entrypoint
 */
__dead void
main(void)
{
    cpu_conf(&g_bsp);
    cons_init();
    syslog_toggle(true);

    printf("booting l5 lunos v0.0.1...\n");
    vm_init();

    panic("end of kernel reached\n");
    for (;;);
}
