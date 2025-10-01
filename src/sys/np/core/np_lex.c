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
 * Compare a token with existing integer types (used internally)
 */
static void
lex_cmp_itype(const char *tokstr, struct lex_token *res)
{
    switch (*tokstr) {
    case 'u':
        if (strcmp(tokstr, TOKEN_U8) == 0) {
            res->token = TT_U8;
            break;
        }

        if (strcmp(tokstr, TOKEN_U16) == 0) {
            res->token = TT_U16;
            break;
        }

        if (strcmp(tokstr, TOKEN_U32) == 0) {
            res->token = TT_U32;
            break;
        }

        if (strcmp(tokstr, TOKEN_U64) == 0) {
            res->token = TT_U64;
            break;
        }
        break;
    case 'i':
        if (strcmp(tokstr, TOKEN_I8) == 0) {
            res->token = TT_I8;
            break;
        }

        if (strcmp(tokstr, TOKEN_I16) == 0) {
            res->token = TT_I16;
            break;
        }

        if (strcmp(tokstr, TOKEN_I32) == 0) {
            res->token = TT_I32;
            break;
        }

        if (strcmp(tokstr, TOKEN_I64) == 0) {
            res->token = TT_I64;
            break;
        }
        break;
    }
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
    case 'i':
        lex_cmp_itype(tokstr, res);
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
    res->val_str = ptrbox_strdup(id, work->work_mem);

    /* Match the tokens */
    error = lex_cmptok(id, res);
    if (error < 0) {
        pr_error("invalid indentifier '%s'\n", id);
    }
    return error;
}

/*
 * Scan arithmetic operators and return the token type
 * (tt_t) on success
 *
 * @work: Input work
 * @c: Character to check
 *
 * Returns a less than zero value on failure
 */
static int
lex_arithop(struct np_work *work, char c, struct lex_token *res)
{
    switch (c) {
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
    default:
        return -1;
    }

    return res->token;
}

/*
 * Scan compare operators and return the token type
 * (tt_t) on success
 *
 * @work: Input work
 * @c: Character to check
 *
 * Returns a less than zero value on failure
 */
static int
lex_cmpop(struct np_work *work, char c, struct lex_token *res)
{
    switch (c) {
    case '>':
        res->token = TT_GT;
        break;
    case '<':
        res->token = TT_LT;
        break;
    default:
        return -1;
    }

    return res->token;
}

/*
 * Parse a number and get a token value
 *
 * @work: Input work
 * @c: First character [digit]
 * @res: Result
 */
static int
lex_nomnum(struct np_work *work, char c, struct lex_token *res)
{
    uint64_t num = 0;

    if (work == NULL || res == NULL) {
        return -EINVAL;
    }

    while (is_num(c)) {
        num = num * 10 + (c - '0');
        c = lex_pop(work);
    }

    res->token = TT_NUMBER;
    res->val = num;
    return 0;
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
    res->token = TT_NONE;

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
    case '\0':
        return -2;  /* EOF */
    case '(':
        res->token = TT_LPAREN;
        break;
    case ')':
        res->token = TT_RPAREN;
        break;
    case ',':
        res->token = TT_COMMA;
        break;
    case '=':
        res->token = TT_EQUALS;
        break;
    case '[':
        res->token = TT_LBRACK;
        break;
    case ']':
        res->token = TT_RBRACK;
        break;
    case ':':
        res->token = TT_COLON;
        break;
    default:
        if (is_num(c)) {
            lex_nomnum(work, c, res);
            break;
        }

        if (lex_arithop(work, c, res) >= 0)
            break;
        if (lex_cmpop(work, c, res) >= 0)
            break;

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
