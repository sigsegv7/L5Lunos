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
 * Description: Pirho compiler parser
 * Author: Ian Marco Moffett
 */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/param.h>
#include <np/lex.h>
#include <np/parse.h>
#include <np/piir.h>
#include <os/np.h>
#include <lib/ptrbox.h>

#define MAX_BEGIN_DEPTH 8

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
    [TT_U16]    = "<TT_U16>",
    [TT_U32]    = "<TT_U32>",
    [TT_U64]    = "<TT_U64>",
    [TT_I8]     = "<TT_I8>",
    [TT_I16]    = "<TT_I16>",
    [TT_I32]    = "<TT_I32>",
    [TT_I64]    = "<TT_I64>",

    /* Values */
    [TT_NUMBER] = "<TT_NUMBER>",

    /* Keywords */
    [TT_BEGIN]  = "<TT_BEGIN>",
    [TT_PROC]   = "<TT_PROC>",
    [TT_END]    = "<TT_END>",
    [TT_RETURN] = "<TT_RETURN>"
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
 * Parse a type token
 *
 * @work: Current work
 * @tok: Token result
 *
 * Returns the ast integer type on success, and NP_BAD_TYPE
 * on failure.
 */
static np_itype_t
parse_type(struct np_work *work, struct lex_token *tok)
{
    tt_t tt;

    if (work == NULL || tok == NULL) {
        return NP_BAD_TYPE;
    }

    tt = parse_scan(work, tok);
    switch (tt) {
    /* Unsigned types */
    case TT_U8: return NP_U8;
    case TT_U16: return NP_U16;
    case TT_U32: return NP_U32;
    case TT_U64: return NP_U64;

    /* Signed types */
    case TT_I8: return NP_I8;
    case TT_I16: return NP_I16;
    case TT_I32: return NP_I32;
    case TT_I64: return NP_I64;
    }

    return NP_BAD_TYPE;
}

/*
 * Parse a return statement
 *
 * @work: Input work
 * @tok: Current token
 *
 * Returns zero on success
 */
static int
parse_return(struct np_work *work, struct lex_token *tok)
{
    tt_t tt;

    /*
     * For now we'll only accept numbers as return values
     * so we must see one after the return statement
     */
    tt = parse_expect(work, "return", TT_NUMBER, tok);
    if (tt == TT_NONE) {
        return -1;
    }

#define PIIR_PUSH(BYTE) piir_push(work->piir_stack, (BYTE))
    PIIR_PUSH(PIIR_RET_NUM);
    PIIR_PUSH(tok->val);
#undef PIIR_PUSH
    return 0;
}

/*
 * Parse a procedure / function
 *
 * @work: Input work
 * @tok: Current token
 *
 * Returns zero on success
 */
static int
parse_proc(struct np_work *work, struct lex_token *tok)
{
    char *ident;
    np_itype_t ret_type = NP_BAD_TYPE;
    tt_t tt;

    if (work == NULL || tok == NULL) {
        return -EINVAL;
    }

    /* We need the identifier */
    tt = parse_expect(work, "proc", TT_IDENT, tok);
    if (tt == TT_NONE) {
        return -1;
    }

    ident = ptrbox_strdup(tok->val_str, work->work_mem);
    if (ident == NULL) {
        return -ENOMEM;
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

    /* We need the first part of '->' */
    tt = parse_expect(work, "<TT_RPAREN>", TT_MINUS, tok);
    if (tok == TT_NONE) {
        return -1;
    }

    /* Now we need to complete the '->' with '>' */
    tt = parse_expect(work, "<TT_MINUS>", TT_GT, tok);
    if (tok == TT_NONE) {
        return -1;
    }

    /* And now the return type */
    ret_type = parse_type(work, tok);
    if (ret_type == NP_BAD_TYPE) {
        pr_error(
            "line %d: expected valid type, got %s\n",
            work->line_no,
            stoktab[tok->token]
        );
        return -1;
    }

    /* Need a 'begin' */
    tt = parse_expect(work, "<TYPENAME>", TT_BEGIN, tok);
    if (tt == TT_NONE) {
        return -1;
    }

    ++work->begin_depth;
    work->in_func = 1;
    return 0;
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
#define PIIR_PUSH(BYTE) piir_push(work->piir_stack, (BYTE))
    /*
     * XXX: wrapped in "[]" indicates optional
     *
     * TT_BEGIN => nil
     * TT_END  => nil
     * TT_PROC => proc <TT_IDENT>(..., ...) [ -> <TYPE> ]
     */
    switch (tok->token) {
    case TT_BEGIN:
        /* Don't exceed the max depth */
        if (work->begin_depth >= MAX_BEGIN_DEPTH) {
            pr_error("line %d: max depth reached\n", work->line_no);
            return -1;
        }

        ++work->begin_depth;
    case TT_END:
        /*
         * Check if the 'begin' statements match, if they do
         * we'll decrease the depth and push a NIL return
         *
         * TODO: We'll need to handle returns better
         */
        if (work->begin_depth > 0) {
            --work->begin_depth;
            PIIR_PUSH(PIIR_RET_NIL);
            break;
        }

        pr_error(
            "line %d: got 'end' statement but no matching 'begin' statements\n",
            work->line_no
        );
        return -1;
    case TT_PROC:
        /* Can't be nested */
        if (work->in_func) {
            pr_error(
                "line %d: nested functions not supported\n",
                work->line_no
            );
            return -1;
        }

        if ((error = parse_proc(work, tok)) != 0) {
            return -1;
        }

        /* XXX: NOP for testing */
        PIIR_PUSH(PIIR_NOP);
        break;
    case TT_RETURN:
        if ((error = parse_return(work, tok)) != 0) {
            return -1;
        }
        break;
    }
#undef PIIR_PUSH
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

    /* Initialize PIIR */
    error = piir_stack_new(work, &work->piir_stack);
    if (error < 0) {
        pr_error("failed to alloc PIIR stack\n");
        return error;
    }

    while (error == 0) {
        error = lex_nom(work, &tok);
        if (error == LEX_EOF) {
            break;
        }
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

    piir_inject(work);

    /*
     * If there are more begin clauses than end
     * clauses, someone mismatched them.
     */
    if (work->begin_depth > 0) {
        pr_error("line %d: expected 'end' statement\n", work->line_no);
        return -1;
    }

    return 0;
}
