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

#include <sys/syslog.h>
#include <os/module.h>
#include <fs/devfs.h>

static struct cdevsw null_cdev;

/*
 * Lie to the user and make them think they are
 * reading data but give them nothing
 */
static ssize_t
null_read(struct devfs_node *dnp, struct dev_iobuf *io, int flags)
{
    /* We give you a whole lot of nothing! */
    return io->count;
}

/*
 * Take the users carefully crafted bytes made with love
 * and throw it in the trash
 */
static ssize_t
null_write(struct devfs_node *dnp, struct dev_iobuf *io, int flags)
{
    return io->count;
}

static int
init_devnull(struct module *modp)
{
    int error;

    error = devfs_register(
        "null",
        DEVFS_CDEV,
        &null_cdev,
        0
    );

    if (error < 0) {
        printf("null: could not create /dev/null\n");
        return error;
    }

    return 0;
}

static struct cdevsw null_cdev = {
    .read = null_read,
    .write = null_write
};

MODULE_EXPORT("null", MODTYPE_GENERIC, init_devnull);
