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
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <os/vfs.h>
#include <string.h>

/*
 * The filesystem table
 */
static struct fs_info fstab[] = {
    { MOUNT_INITRD, &g_omar_vfsops, 0 }
};

/*
 * Get entry by name
 */
int
vfs_by_name(const char *name, struct fs_info **resp)
{
    size_t nelem;
    int retval = -ENOENT;

    if (name == NULL || resp == NULL) {
        return -EINVAL;
    }

    nelem = NELEM(fstab);
    for (int i = 0; i < nelem; ++i) {
        if (strcmp(fstab[i].name, name) == 0) {
            *resp = &fstab[i];
            retval = 0;
            break;
        }
    }

    return retval;
}

/*
 * Get entry by index
 */
int
vfs_by_index(uint16_t index, struct fs_info **resp)
{
    if (resp == NULL) {
        return -EINVAL;
    }

    if (index >= NELEM(fstab)) {
        return -ENOENT;
    }

    *resp = &fstab[index];
    return 0;
}

int
vfs_init(void)
{
    const struct vfsops *vfsops;
    struct fs_info *fip;
    size_t nelem = NELEM(fstab);

    /*
     * Initialize each filesystem
     */
    for (size_t i = 0; i < nelem; ++i) {
        fip = &fstab[i];
        vfsops = fip->vfsops;
        if (vfsops->init != NULL) {
            vfsops->init(fip);
        }
    }
    return 0;
}
