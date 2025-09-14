#include <sys/cdefs.h>
#include <sys/panic.h>
#include <sys/syslog.h>
#include <sys/cpuvar.h>

struct pcore g_bsp;

/*
 * Kernel entrypoint
 */
__dead void
main(void)
{
    cpu_conf(&g_bsp);
    printf("booting l5 lunos v0.0.1...\n");
    panic("end of kernel reached\n");
    for (;;);
}
