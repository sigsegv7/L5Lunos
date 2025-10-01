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
#include <sys/syslog.h>
#include <os/np.h>
#include <np/lex.h>
#include <string.h>

#define pr_trace(fmt, ...) printf("pirho.lex: " fmt, ##__VA_ARGS__)
#define pr_error(fmt, ...) printf("pirho.lex: error: " fmt, ##__VA_ARGS__)

/* Max identifier length */
#define MAX_ID_LEN 32

/* Just some helpers */
#define to_lower(c) ((c) | 0x20)
#define is_alpha(c) (to_lower(c) >= 'a' && to_lower(c) <= 'z')
#define is_num(c) ((c) >= '0' && (c) <= '9')
#define is_space(c) ((c) == ' ' || (c) == '\t' || (c) == '\f' || (c) == '\n')

/*
 * Pop a character from the input work
 *
 * @work: Input work to pop from
 */
static char
lex_pop(struct np_work *work)
{
    struct lexer_state *lex_st;
    char c;

    if (work == NULL) {
        return '\0';
    }

    /*
     * First we check the cache, if there is a char,
     * grab and clear it.
     */
    if (work->ccache != '\0') {
        c = work->ccache;
        work->ccache = '\0';
        return c;
    }

    /* Don't overflow the source file */
    lex_st = &work->lex_st;
    if (lex_st->source_idx >= work->source_size) {
        return '\0';
    }

    return work->source[lex_st->source_idx++];
}

/*
 * Compare a token with existing tokens (used internally)
 */
static int
lex_cmptok(char *tokstr, struct lex_token *res)
{
    switch (*tokstr) {
    case 'b':
        if (strcmp(tokstr, TOKEN_BEGIN) == 0) {
            res->token = TT_BEGIN;
        }
        return 0;
    case 'e':
        if (strcmp(tokstr, TOKEN_END) == 0) {
            res->token = TT_END;
        }
        return 0;
    case 'p':
        if (strcmp(tokstr, TOKEN_PROC) == 0) {
            res->token = TT_PROC;
        }
        return 0;
    case 'u':
        if (strcmp(tokstr, TOKEN_U8) == 0) {
            res->token = TT_U8;
        }
        return 0;
    }

    res->token = TT_IDENT;
    return 0;
}

/*
 * Match a token with a set of known tokens
 *
 * @work: Input work
 * @c: Current character
 * @res: Token result
 *
 * Returns zero on success
 */
static int
lex_matchstr(struct np_work *work, char c, struct lex_token *res)
{
    char id[MAX_ID_LEN + 1];
    size_t id_idx = 0;
    int error;

    if (work == NULL || res == NULL) {
        return -EINVAL;
    }

    /* Grab the identifier */
    do {
        if (id_idx >= sizeof(id) - 1) {
            pr_error("identifier too long!\n");
            return -1;
        }
        if (!is_alpha(c) && !is_num(c)) {
            work->ccache = c;
            break;
        }
        id[id_idx++] = c;
    } while ((c = lex_pop(work)) != 0);

    id[id_idx] = '\0';

    /* Match the tokens */
    error = lex_cmptok(id, res);
    if (error < 0) {
        pr_error("invalid indentifier '%s'\n", id);
    }
    return error;
}

/*
 * Nom a token
 */
int
lex_nom(struct np_work *work, struct lex_token *res)
{
    struct lexer_state *lex_st;
    int error = 0;
    char c;

    if (work == NULL || res == NULL) {
        return -EINVAL;
    }

    lex_st = &work->lex_st;

    /* Skip all whitespace */
    while ((c = lex_pop(work)) != 0) {
        if (c == '\n') {
            ++work->line_no;
        }
        if (is_space(c)) {
            continue;
        }
        break;
    }

    /* Match the token type */
    switch (c) {
    case '(':
        res->token = TT_LPAREN;
        break;
    case ')':
        res->token = TT_RPAREN;
        break;
    case ',':
        res->token = TT_COMMA;
        break;
    case '*':
        res->token = TT_STAR;
        break;
    case '-':
        res->token = TT_MINUS;
        break;
    case '+':
        res->token = TT_PLUS;
        break;
    case '/':
        res->token = TT_SLASH;
        break;
    case '=':
        res->token = TT_EQUALS;
        break;
    case '>':
        res->token = TT_GT;
        break;
    case '<':
        res->token = TT_LT;
        break;
    default:
        /* Stuff like '1var_name' is invalid */
        if (!is_alpha(c)) {
            pr_error("unexpected token '%c'\n", c);
            return -1;
        }

        error = lex_matchstr(work, c, res);
        if (error == 0) {
            break;
        }

        error = -1;
        break;
    }

    return error;
}

int
lex_init(struct lexer_state *state, struct np_work *work)
{
    if (state == NULL) {
        return -EINVAL;
    }

    memset(state, 0, sizeof(*state));
    state->work = work;
    return 0;
}
