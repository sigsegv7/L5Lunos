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
 * Description: Module interface
 * Author: Ian Marco Moffett
 */

#ifndef _OS_MODULE_H_
#define _OS_MODULE_H_ 1

#include <sys/types.h>

/* Early (high priority)modules */
extern char __modules_init_start[];
extern char __modules_init_end[];

/*
 * Module type enumeration
 *
 * @MODTYPE_GENERIC: Generic module/driver
 * @MODTYPE_PCI: PCI driver
 */
typedef enum {
    MODTYPE_GENERIC,
    MODTYPE_PCI,
} modtype_t;

struct module {
    const char *name;
    modtype_t type;
    int(*init)(struct module *modp);
};

/*
 * Export a module to be started up upon
 * early boot.
 */
#define MODULE_EXPORT(NAME, TYPE, INIT_FN) \
    __attribute__((used, section(".modules")))       \
    struct module __module_##INIT_FN  = {          \
        .name = (NAME),                             \
        .type = (TYPE),                             \
        .init = (INIT_FN)                           \
    };

/*
 * XXX: Guarding this in case ever exported
 */
#if defined(_KERNEL)
/*
 * Used internally to initializemodules
 */
#define __MODULES_INIT(TYPE) \
    for (struct module *__m = (struct module *)__modules_init_start;    \
         (uintptr_t)__m < (uintptr_t)__modules_init_end; ++__m)         \
    {                                                                   \
        if (__m->type == ((TYPE)))                                      \
            __m->init(__m);                                             \
    }

#endif  /* _KERNEL */
#endif  /* !_OS_MODULE_H_ */
