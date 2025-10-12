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
#include <sys/reboot.h>
#include <sys/cdefs.h>
#include <sys/cpuvar.h>
#include <os/reboot.h>
#include <machine/pio.h>

#if defined(__I8042_REBOOT)
#define I8042_REBOOT __I8042_REBOOT
#else
#define I8042_REBOOT 0
#endif  /* __I8042_REBOOT */

static void
intel_reset(struct mdcore *mdcore)
{
    /*
     * Some intel platform controller hubs allow a
     * reset via a special RST_CNT control register,
     * perform a CPU reset with SYS_RST and RST_CPU.
     */
    if (mdcore->family == 0x06) {
        outb(0xCF9, 3 << 1);
    }
}

/*
 * Perform platform specific reset methods
 *
 * @core: Current core for platform identification
 */
static void
platform_reset(struct pcore *core)
{
    struct mdcore *mdcore;

    if (core == NULL) {
        return;
    }

    mdcore = &core->md;
    switch (mdcore->vendor) {
    case CPU_VENDOR_INTEL:
        intel_reset(mdcore);
        break;
    }
}

/*
 * Actual reboot core
 */
__dead static void
__reboot(void)
{
    struct pcore *core;
    void *dmmy_null = NULL;

    /*
     * Use the i8042 to reboot the system if we can, this
     * might be disabled in some L5 kernels.
     */
    if (I8042_REBOOT) {
        outb(0x64, 0xFE);
    }

    /* Try platform specific methods */
    core = this_core();
    if (core != NULL) {
        platform_reset(core);
    }

    /*
     * If somehow nothing we've tried worked, be very
     * violent and force it to reboot via faulty IDT.
     */
    __ASMV(
        "lidt %0\n"
        "int $0\n"
        :
        : "m" (dmmy_null)
        : "memory"
    );

    __builtin_unreachable();
}

void
reboot(int method)
{
    __reboot();
    __builtin_unreachable();
}

/*
 * ARG0: Method
 */
scret_t
sys_reboot(struct syscall_args *scargs)
{
    int method = SCARG(scargs, int, 0);

    reboot(method);
    __builtin_unreachable();
}
