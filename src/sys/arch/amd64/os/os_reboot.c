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
#include <os/reboot.h>
#include <machine/pio.h>

/*
 * Actual reboot core
 */
__dead static void
__reboot(void)
{
    void *dmmy_null = NULL;

    /*
     * Try to be gentle and simply put the CPU into a
     * reboot cycle through the i8042 reset line, though
     * some chipsets may not like this.
     */
    outb(0x64, 0xFE);

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
