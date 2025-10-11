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

#ifndef _STRING_H
#define _STRING_H 1

#include <stddef.h>

/*
 * Get the length of a string
 *
 * @s: String to get length of
 */
size_t strlen(const char *s);

/*
 * Compare a string against another
 *
 * @s1: First string to compare
 * @s2: Second string to compare
 *
 * Returns zero if strings are equal
 */
int strcmp(const char *s1, const char *s2);

/*
 * Get the length of a string with a maximum
 * length
 *
 * @s: String to check length of
 * @maxlen: Max length of string to check
 */
size_t strnlen(const char *s, size_t maxlen);

/*
 * Copy variable amount of bytes from 'src' to 'dest'
 *
 * @dest: Copy destination
 * @src: Copy source
 * @n: Number of bytes to copy
 *
 * Returns 'dest' on success
 */
void *memcpy(void *dest, const void *src, size_t n);

/*
 * Fill 'n' bytes of memory with 'c'
 *
 * @s: Memory to fill
 * @c: Byte to fill memory with
 * @n: Number of bytes to fill
 *
 * Returns a pointer to 's'
 */
void *memset(void *s, int c, size_t n);

/*
 * Convert an integer base to string form
 *
 * @value: Value to convert to string
 * @buf: Buffer to use
 * @base: Radix to use
 *
 * Returns 'buf' on success, otherwise a value of
 * NULL on failure.
 */
char *itoa(int64_t value, char *buf, int base);

#endif  /* _STRING_H */
