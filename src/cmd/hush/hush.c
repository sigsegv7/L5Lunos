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

#include <sys/wait.h>
#include <sys/spawn.h>
#include <sys/limits.h>
#include <sys/iotap.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

/* XXX: Hardcoded for now, will change */
#define PROMPT "[sv@l5]: "

/*
 * Read input from an input device
 *
 * @buf: Buffer to fill with input
 * @maxlen: Max length of buffer
 */
static void
read_input(char *buf, size_t maxlen)
{
    ssize_t retval;
    size_t i = 0;
    char c = '\0';
    struct iotap_msg msg = {
        .opcode = IOTAP_OPC_READ,
        .buf = &c,
        .len = 1
    };

    if (buf == NULL) {
        return;
    }

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
            buf[i++] = c;
            write(STDOUT_FILENO, &c, 1);
        }

    } while (c != '\n');
}

static int
parse_args(char *input, char *argv[], int max_args)
{
    int argc = 0;

    while (*input != '\0') {
        /* Skip leading spaces */
        while (*input == ' ')
            input++;

        if (*input == '\0')
            break;
        if (argc < max_args)
            argv[argc++] = input;

        /* Keep going until space or NUL */
        while (*input != '\0' && *input != ' ') {
            input++;
        }

        /* end */
        if (*input != '\0') {
            *input = '\0';
            input++;
        }
    }

    return argc;
}

int
main(void)
{
    char *argv[16];
    char buf[128];
    char binpath[256];
    int pid, argc;

    memset(argv, 0, sizeof(argv));
    argv[0] = binpath;

    for (;;) {
        write(STDOUT_FILENO, PROMPT, sizeof(PROMPT) - 1);
        read_input(buf, sizeof(buf));
        write(STDOUT_FILENO, "\n", 1);

        if (buf[0] == '\0') {
            continue;
        }

        argc = parse_args(buf, argv, sizeof(argv));
        snprintf(binpath, sizeof(binpath), "/usr/bin/%s", argv[0]);

        if ((pid = spawn(binpath, argv)) < 0) {
            printf("unknown command \"%s\"\n", buf);
        }

        waitpid(pid, NULL, 0);
        buf[0] = '\0';
        memset(buf, 0, sizeof(buf));
    }

    return 0;
}
