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
#include <sys/bootvars.h>
#include <sys/cdefs.h>
#include <sys/panic.h>
#include <sys/syslog.h>
#include <os/omar.h>
#include <string.h>
#include <stdbool.h>

#define INITRD_PATH "/boot/initrd.omar"

#define OMAR_EOF "RAMO"
#define OMAR_REG    0
#define OMAR_DIR    1
#define BLOCK_SIZE 512

static const char *__initrd_root = NULL;
static size_t initrd_size = 0;

/*
 * File or directory.
 */
struct initrd_node {
    const char *path;       /* Path */
    void *data;             /* File data */
    size_t size;            /* File size */
    mode_t mode;            /* Perms and type */
};

/*
 * The OMAR file header, describes the basics
 * of a file.
 *
 * @magic: Header magic ("OMAR")
 * @len: Length of the file
 * @namelen: Length of the filename
 * @rev: OMAR revision
 * @mode: File permissions
 */
struct __packed omar_hdr {
    char magic[4];
    uint8_t type;
    uint8_t namelen;
    uint32_t len;
    uint8_t rev;
    uint32_t mode;
};

/*
 * Get a file from initrd
 *
 * @path: Path of file to get.
 * @res: Pointer to new resulting node.
 */
static int
initrd_get_file(const char *path, struct initrd_node *res)
{
    struct initrd_node node;
    const struct omar_hdr *hdr;
    const char *p, *name;
    char namebuf[256];
    off_t off;

    p = __initrd_root;
    for (;;) {
        hdr = (struct omar_hdr *)p;
        if (strncmp(hdr->magic, OMAR_EOF, sizeof(OMAR_EOF)) == 0) {
            break;
        }

        /* Ensure the file is valid */
        if (strncmp(hdr->magic, "OMAR", 4) != 0) {
            /* Bad magic */
            return -EINVAL;
        }
        if (hdr->namelen > sizeof(namebuf) - 1) {
            return -EINVAL;
        }

        name = (char *)p + sizeof(struct omar_hdr);
        memcpy(namebuf, name, hdr->namelen);
        namebuf[hdr->namelen] = '\0';

        /* Compute offset to next block */
        if (hdr->type == OMAR_DIR) {
            off = 512;
        } else {
            off = ALIGN_UP(sizeof(*hdr) + hdr->namelen + hdr->len, BLOCK_SIZE);
        }

        /* Skip header and name, right to the data */
        p = (char *)hdr + sizeof(struct omar_hdr);
        p += hdr->namelen;

        if (strcmp(namebuf, path) == 0) {
            node.mode = hdr->mode;
            node.size = hdr->len;
            node.data = (void *)p;
            *res = node;
            return 0;
        }

        hdr = (struct omar_hdr *)((char *)hdr + off);
        p = (char *)hdr;
        memset(namebuf, 0, sizeof(namebuf));
    }

    return -ENOENT;
}

/*
 * Initialize the initrd
 */
static int
initrd_init(void)
{
    struct bootvars bootvars;
    struct bootvar_io *bvio = NULL;
    int error;

    if (__initrd_root != NULL) {
        panic("initrd: cannot re-init initrd\n");
    }

    error = bootvars_read(&bootvars, 0);
    if (error < 0) {
        return error;
    }

    bvio = &bootvars.iovars;
    __initrd_root = bvio->get_module(INITRD_PATH, &initrd_size);
    if (__initrd_root == NULL) {
        panic("initrd: could not find '%s'\n", INITRD_PATH);
    }

    return 0;
}

/*
 * Open an entry within the OMAR initrd
 * image.
 */
ssize_t
initrd_open(const char *path, char **res)
{
    struct initrd_node node;
    int error = 0;

    if (path == NULL || res == NULL) {
        return -EINVAL;
    }

    /* Path must start with a '/' */
    if (*path++ != '/') {
        printf("initrd: bad path '%s'\n", path);
        return -ENOENT;
    }

    if (__initrd_root == NULL) {
        error = initrd_init();
        if (error < 0) {
            panic("initrd: failed to setup initrd\n");
        }
    }

    error = initrd_get_file(path, &node);
    if (error < 0) {
        printf("initrd: failed to open '%s'\n", path);
        return error;
    }

    *res = node.data;
    return node.size;
}
