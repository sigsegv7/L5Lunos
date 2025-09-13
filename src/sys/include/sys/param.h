/*
 * Copyright (c) 2025 Ian Marco Moffett and Ethos0 engineers
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

#ifndef _SYS_PARAM_H_
#define _SYS_PARAM_H_ 1

/* Assumed cache line size */
#ifndef COHERENCY_UNIT
#define COHERENCY_UNIT 64
#endif

/* Bit related macros */
#define ISSET(v, f)  ((v) & (f))
#define BIT(n) (1ULL << (n))
#define MASK(n) ((1ULL << n) - 1)

/* Min/max macros */
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

/* Align up/down a value */
#define ALIGN_DOWN(value, align)      ((value) & ~((align)-1))
#define ALIGN_UP(value, align)        (((value) + (align)-1) & ~((align)-1))
#define MALIGN(value)                 ALIGN_UP((value), M_WORD_SIZE)

/* Bitmap helper macros */
#define setbit(a, b) ((a)[(b) >> 3] |= BIT(b % 8))
#define clrbit(a, b) ((a)[(b) >> 3] &= ~BIT(b % 8))
#define testbit(a, b) (ISSET((a)[(b) >> 3], BIT(b % 8)))

/* Combine bits */
#define COMBINE8(h, l) ((uint16_t)((uint16_t)(h) << 8) | (l))
#define COMBINE16(h, l) ((uint32_t)((uint32_t)(h) << 16) | (l))
#define COMBINE32(h, l) ((uint64_t)((uint64_t)(h) << 32) | (l))

/*
 * Checks if value, `v' is in range of least, 'l'
 * and max, 'm'.
 */
#define IN_RANGE(V, L, M) (((V) >= (L)) && ((V) <= (M)))

/* Gives 1 if pointer is aligned */
#define PTR_ALIGNED(PTR, ALIGN) (!((uintptr_t)PTR & (ALIGN - 1)))

/*
 * PTR_OFFSET: Adds an offset to the pointer
 * PTR_NOFFSET: Subtracts a negative offset from the pointer
 */
#define PTR_OFFSET(PTR, OFF) ((void *)((uintptr_t)PTR + OFF))
#define PTR_NOFFSET(PTR, NOFF) ((void *)((uintptr_t)PTR - NOFF))

#define NELEM(a) (sizeof(a) / sizeof(a[0]))

#endif  /* _SYS_PARAM_H_ */
