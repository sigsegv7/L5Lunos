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

#ifndef _NP_LEX_H_
#define _NP_LEX_H_ 1

#include <sys/types.h>

/* End-of-file */
#define LEX_EOF (-2)

struct np_work;

/* Keywords */
#define TOKEN_BEGIN "begin"
#define TOKEN_PROC "proc"
#define TOKEN_END "end"

/* Types */
#define TOKEN_U8    "u8"
#define TOKEN_U16   "u16"
#define TOKEN_U32   "u32"
#define TOKEN_U64   "u64"
#define TOKEN_I8    "i8"
#define TOKEN_I16   "i16"
#define TOKEN_I32   "i32"
#define TOKEN_I64   "i64"

/*
 * Represents the various token types that are
 * possible
 */
typedef enum {
    /* Reserved */
    TT_NONE,            /* Always invalid */

    /* Symbols */
    TT_LPAREN,          /* '(' */
    TT_RPAREN,          /* ')' */
    TT_IDENT,           /* '<IDENTIFIER>' */
    TT_COMMA,           /* ',' */
    TT_STAR,            /* '*' */
    TT_MINUS,           /* '-' */
    TT_PLUS,            /* '+' */
    TT_SLASH,           /* '/' */
    TT_EQUALS,          /* '=' */
    TT_GT,              /* '>' */
    TT_LT,              /* '<' */
    TT_LBRACK,          /* '[' */
    TT_RBRACK,          /* ']' */
    TT_COLON,           /* ':' */

    /* Types */
    TT_U8,              /* 'u8' */
    TT_U16,             /* 'u16' */
    TT_U32,             /* 'u32' */
    TT_U64,             /* 'u64' */
    TT_I8,              /* 'i8'  */
    TT_I16,             /* 'i16' */
    TT_I32,             /* 'i32' */
    TT_I64,             /* 'i64' */

    /* Values */
    TT_NUMBER,          /* <numbers> */

    /* Keywords */
    TT_BEGIN,           /* 'begin' */
    TT_PROC,            /* 'proc' */
    TT_END,             /* 'end' */
} tt_t;

typedef uint64_t tokval_t;

/*
 * Represents a lexer token
 *
 * @token: Token type
 * @val: Integer value
 */
struct lex_token {
    tt_t token;
    union {
        tokval_t val;
        char *val_str;
    };
};

/*
 * Represents the lexer state machine
 *
 * @work: Current compiler work unit
 * @tok: Current token
 * @source_idx: Byte index into source stream
 */
struct lexer_state {
    struct np_work *work;
    struct lex_token tok;
    size_t source_idx;
};

/*
 * Initialize the lexer state machine into
 * a known state
 *
 * @state: Lexer state machine to initialize
 * @work: Work to initialize state machine with
 *
 * Returns zero on success, otherwise a less than
 * zero value on failure
 */
int lex_init(struct lexer_state *state, struct np_work *work);

/*
 * "Nom" a token from the source input and advance
 * to the next.
 *
 * @work: Current work
 * @res: Result is written here
 *
 * Returns zero on success, otherwise a less than
 * zero value on failure
 */
int lex_nom(struct np_work *work, struct lex_token *res);

#endif  /* !_NP_LEX_H_ */
