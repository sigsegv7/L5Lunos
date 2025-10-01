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
#include <sys/param.h>
#include <np/lex.h>
#include <np/parse.h>
#include <np/ast.h>
#include <os/np.h>

#define pr_trace(fmt, ...) printf("pirho.parse: " fmt, ##__VA_ARGS__)
#define pr_error(fmt, ...) printf("pirho.parse: error: " fmt, ##__VA_ARGS__)

/* Token to string conversion table */
static const char *stoktab[] = {
    /* Reserved */
    [TT_NONE]   = "<TT_NONE>",

    /* Symbols */
    [TT_LPAREN] = "<TT_LPAREN>",
    [TT_RPAREN] = "<TT_RPAREN>",
    [TT_IDENT]  = "<IDENTIFIER>",
    [TT_COMMA]  = "<TT_COMMA>",
    [TT_STAR]   = "<TT_STAR>",
    [TT_MINUS]  = "<TT_MINUS>",
    [TT_PLUS]   = "<TT_PLUS>",
    [TT_SLASH]  = "<TT_SLASH>",
    [TT_EQUALS] = "<TT_EQUALS>",
    [TT_GT]     = "<TT_GREATER>",
    [TT_LT]     = "<TT_LESSTHAN>",
    [TT_LBRACK] = "<TT_LBRACK>",
    [TT_RBRACK] = "<TT_RBRACK>",
    [TT_COLON]  = "<TT_COLON>",

    /* Types */
    [TT_U8]     = "<TT_U8>",

    /* Values */
    [TT_NUMBER] = "<TT_NUMBER>",

    /* Keywords */
    [TT_BEGIN]  = "<TT_BEGIN>",
    [TT_PROC]   = "<TT_PROC>",
    [TT_END]    = "<TT_END>"
};

/*
 * Scan the next token
 *
 * @work: Work input
 * @token: Last token
 *
 * Returns the token type on success (TT_NONE on error)
 */
static inline tt_t
parse_scan(struct np_work *work, struct lex_token *tok)
{
    if (lex_nom(work, tok) < 0) {
        return TT_NONE;
    }

    return tok->token;
}

/*
 * Scan and expect the next token to be a specific
 * value
 *
 * @work: Work input
 * @cur: The value before the value we expect
 * @what: What we expect
 * @tok: Last token
 *
 * Returns the token type on success (TT_NONE on error)
 */
static inline tt_t
parse_expect(struct np_work *work, char *cur, tt_t what, struct lex_token *tok)
{
    tt_t tt = parse_scan(work, tok);

    if (tt != what) {
        pr_error(
            "line %d: expected %s after '%s', got %s\n",
            work->line_no,
            stoktab[what],
            cur,
            stoktab[tok->token]
        );

        return TT_NONE;
    }

    return tt;
}

/*
 * Parse a token
 *
 * @work: Input work
 * @tok: Current token
 */
static int
parse_token(struct np_work *work, struct lex_token *tok)
{
    tt_t tt;
    int error;

    /*
     * XXX: wrapped in "[]" indicates optional
     *
     * TT_PROC => proc <TT_IDENT>(..., ...) [ -> <TYPE> ]
     */
    switch (tok->token) {
    case TT_PROC:
        /* We need the identifier */
        tt = parse_expect(work, "proc", TT_IDENT, tok);
        if (tt == TT_NONE) {
            return -1;
        }

        /* Expect the left paren */
        tt = parse_expect(work, "<TT_IDENT>", TT_LPAREN, tok);
        if (tt == TT_NONE) {
            return -1;
        }

        /* TODO: ARGS LATER */
        tt = parse_expect(work, "<TT_LPAREN>", TT_RPAREN, tok);
        if (tt == TT_NONE) {
            return -1;
        }
        break;
    }

    return 0;
}

int
parse_work(struct np_work *work)
{
    struct lex_token tok;
    int error = 0;

    if (work == NULL) {
        pr_error("bad work argument\n");
        return -EINVAL;
    }

    /* Get the AST root node */
    work->ast_root = ast_alloc(work);
    if (work->ast_root == NULL) {
        pr_error("failed to alloc root AST|n");
        return -ENOMEM;
    }

    while (error == 0) {
        error = lex_nom(work, &tok);
        if (error < 0) {
            return error;
        }

        /* Don't overflow the table */
        if (tok.token > NELEM(stoktab)) {
            pr_error("bad token %d\n", tok.token);
            return -1;
        }

        if (parse_token(work, &tok) < 0) {
            return -1;
        }
    }

    return 0;
}
