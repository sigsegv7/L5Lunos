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

#include <sys/iotap.h>
#include <sys/spawn.h>
#include <sys/wait.h>
#include <stddef.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "blake2.h"

#define DEFAULT_SHELL "/usr/bin/hush"

/* Max lengths */
#define USERNAME_MAX 128
#define PASSWORD_MAX 256

/*
 * Read input from an input device
 *
 * @buf: Buffer to fill with input
 * @maxlen: Max length of buffer
 * @bool: True if characters should be plaintext
 */
static void
read_input(char *buf, size_t maxlen, bool show)
{
    ssize_t retval;
    size_t i = 0;
    char c = '\0';
    char vis_c;
    struct iotap_msg msg = {
        .opcode = IOTAP_OPC_READ,
        .buf = &c,
        .len = 1
    };

    if (buf == NULL) {
        return;
    }

    /* Zero for security */
    memset(buf, 0, maxlen);

    /*
     * Read all the input until we hit a '\n'
     */
    do {
        retval = iotap_mux("input.igkbd", &msg);
        if (retval < 0) {
            continue;
        }

        if (!isascii(c)) {
            continue;
        }

        /* Don't overwrite prompt */
        if (c == '\b' && i == 0) {
            continue;
        }

        /* Is this a backspace? */
        if (c == '\b' && i > 0) {
            buf[--i] = '\0';
            write(STDOUT_FILENO, &c, 1);
            continue;
        }

        if (c != '\n' && i < maxlen) {
            vis_c = (show) ? c : '*';
            buf[i++] = c;
            write(STDOUT_FILENO, &vis_c, 1);
        }

    } while (c != '\n');
    printf("\n");
}

static int
auth(const char *username, char *hash)
{
    char passwd_path[128];
    char real_hash[BLAKE2B_OUTBYTES];
    int fd, retval = 0;

    /* Get the real password hash */
    snprintf(
        passwd_path,
        sizeof(passwd_path),
        "/ucred/%s/passwd",
        username
    );

    fd = open(passwd_path, O_RDONLY);
    if (fd < 0) {
        return fd;
    }

    read(fd, real_hash, BLAKE2B_OUTBYTES);
    if (memcmp(hash, real_hash, BLAKE2B_OUTBYTES) != 0) {
        retval = -EPERM;
    }

    close(fd);
    return retval;
}

int
main(void)
{
    char shell_path[] = DEFAULT_SHELL;
    char *argv_dmmy[] = { DEFAULT_SHELL, NULL };
    char login[USERNAME_MAX];
    char password[PASSWORD_MAX];
    char hash[BLAKE2B_OUTBYTES];
    pid_t shell_pid;
    int error;

    printf("- the points have aligned -\n");
    printf("** authenticate yourself **\n");

    for (;;) {
        /* Get the username */
        printf("login: ");
        read_input(login, sizeof(login), true);

        /* Get the password */
        printf("password: ");
        read_input(password, sizeof(password), false);

        /*
         * We need to hash this right away and keep the plaintext
         * password in memory as little as possible
         */
        blake2b(
            hash,
            BLAKE2B_OUTBYTES,
            password,
            strlen(password),
            NULL,
            0
        );

        memset(password, 0, sizeof(password));
        error = auth(login, hash);
        if (error < 0) {
            printf("error: bad login\n");
            continue;
        }

        break;
    }

    shell_pid = spawn(shell_path, argv_dmmy);
    waitpid(shell_pid, NULL, 0);
    return 0;
}
