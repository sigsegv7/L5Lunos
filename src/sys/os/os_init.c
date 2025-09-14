#include <sys/cdefs.h>
#include <sys/panic.h>
#include <sys/cpuvar.h>

struct pcore g_bsp;

/*
 * Kernel entrypoint
 */
__dead void
main(void)
{
    cpu_conf(&g_bsp);
    panic("end of kernel reached\n");
    for (;;);
}
