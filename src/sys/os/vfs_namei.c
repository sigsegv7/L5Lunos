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

/*
 * Description: File lookup algorithm
 * Author: Ian Marco Moffett
 */

#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/mount.h>
#include <sys/limits.h>
#include <sys/namei.h>
#include <string.h>

int
namei(struct nameidata *ndp)
{
    struct mount *mp = NULL;
    struct vnode *vp;
    struct vop *vops;
    struct vop_lookup_args lookup;
    struct fs_info *fip;
    char namebuf[NAME_MAX];
    const char *p, *pcur;
    size_t len, i = 0;
    int error;

    if (ndp == NULL) {
        return -EINVAL;
    }

    if ((p = ndp->path) == NULL) {
        printf("namei: path is NULL\n");
        return -EINVAL;
    }

    error = mount_lookup("/", &mp);
    if (error < 0) {
        printf("namei: failed to get rootfs\n");
        return error;
    }

    vp = mp->vp;
    fip = mp->fs;

    if ((vops = vp->vops) == NULL) {
        printf("namei: failed to get vops\n");
        return -EIO;
    }

    /* We need vops->lookup() */
    if (vops->lookup == NULL) {
        printf("namei: vops does not have lookup op\n");
        return -EIO;
    }

    /*
     * If this is an image we are looking up, then throw
     * a path right at it.
     */
    if (ISSET(fip->attr, FS_ATTR_IMAGE)) {
        lookup.name = ndp->path;
        lookup.dirvp = mp->vp;
        lookup.vpp = &vp;
        return vops->lookup(&lookup);
    }

    printf("namei: f: %s\n", ndp->path);
    printf("namei: d: /\n", ndp->path);

    pcur = p;
    while (*pcur != '\0') {
        /* Get out of the slashes */
        while (*pcur == '/')
            ++pcur;

        p = pcur;

        /* Go back to the start */
        memset(namebuf, 0, sizeof(namebuf));
        while (*pcur != '/' && *pcur != '\0') {
            namebuf[i++] = *pcur++;

            if (i > sizeof(namebuf) - 1) {
                return -ENAMETOOLONG;
            }
        }

        i = 0;
        printf("namei: n %s\n", namebuf);
    }
    return 0;
}
