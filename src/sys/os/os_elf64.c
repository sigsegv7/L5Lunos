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
#include <sys/elf.h>
#include <sys/syslog.h>
#include <sys/errno.h>
#include <os/omar.h>
#include <os/elfload.h>
#include <vm/vm.h>
#include <vm/mmu.h>
#include <vm/map.h>
#include <vm/physseg.h>
#include <string.h>

#if defined(__x86_64__)
#define _EM_MACH  EM_X86_64
#else
#error "ELF loader not ported to platform"
#endif  /* __x86_64__ */

/*
 * Verify an ELF64 using its header
 *
 * @eh: ELF64 header
 *
 * Returns zero on success, otherwise a less than
 * zero value on error or invalid binary.
 */
static int
elf64_verify(Elf64_Ehdr *eh)
{
    int error;

    if (eh == NULL) {
        return -EINVAL;
    }

    /* The magic must match */
    error = memcmp(&eh->e_ident[EI_MAG0], ELFMAG, SELFMAG);
    if (error != 0) {
        printf("elf64_verify: bad ELF magic\n");
        return -ENOEXEC;
    }

    /* Must be for this platform */
    if (eh->e_machine != _EM_MACH) {
        printf("elf64_verify: bad target machine\n");
        return -ENOEXEC;
    }

    /* Version must be valid */
    if (eh->e_ident[EI_VERSION] != EV_CURRENT) {
        printf("elf64_verify: bad version\n");
        return -ENOEXEC;
    }

    return 0;
}

/*
 * Do the actual ELF64 loading
 *
 * @eh: ELF header to load
 * @p: Process to load program as
 *
 * Returns zero on success, otherwise a less
 * than zero value on failure.
 *
 * XXX: We'll need to handler unloading here
 */
static int
elf64_do_load(Elf64_Ehdr *eh, struct proc *proc)
{
    static const size_t PSIZE = DEFAULT_PAGESIZE;
    struct md_pcb *pcbp;
    struct mmu_map spec;
    Elf64_Phdr *phdr, *phdr_base;
    paddr_t frame;
    size_t npgs, len, misalign;
    void *tmp;
    int error, prot;

    if (eh == NULL || proc == NULL) {
        return -EINVAL;
    }

    pcbp = &proc->pcb;
#define PHDR_I(PHDR_BASE, INDEX) \
    PTR_OFFSET(PHDR_BASE, eh->e_phentsize*(INDEX))
    phdr_base = PTR_OFFSET(eh, eh->e_phoff);
    for (int i = 0; i < eh->e_phnum; ++i) {
        prot = PROT_READ | PROT_USER;
        phdr = PHDR_I(phdr_base, i);

        /* What segment type is this? */
        switch (phdr->p_type) {
        case PT_LOAD:
            if (ISSET(phdr->p_flags, PF_W))
                prot |= PROT_WRITE;
            if (ISSET(phdr->p_flags, PF_X))
                prot |= PROT_EXEC;

            if (phdr->p_memsz == 0 && phdr->p_filesz == 0) {
                continue;
            }

            /* Re-align the length */
            misalign = phdr->p_memsz & (PSIZE - 1);
            len = phdr->p_memsz;
            len = ALIGN_UP(len + misalign, PSIZE);
            npgs = len / PSIZE;

            if (npgs == 0) {
                ++npgs;
            }

            frame = vm_alloc_frame(npgs);
            if (frame == 0) {
                printf("elf64_do_load: could not alloc frame\n");
                return -ENOMEM;
            }

            /* Copy the segment data */
            tmp = PTR_OFFSET(eh, phdr->p_offset);
            memcpy(PHYS_TO_VIRT(frame), tmp, phdr->p_filesz);

            /* Map the segment */
            spec.va = phdr->p_vaddr;
            spec.pa = frame;
            error = vm_map(
                &pcbp->vas,
                &spec,
                len,
                prot
            );

            if (error < 0) {
                printf("elf64_do_load: failed to map segment\n");
                return error;
            }
            break;
        }
#undef _PHDR_I
    }

    return 0;
}

/*
 * Load an ELF binary
 */
int
elf_load(const char *path, struct proc *proc, struct loaded_elf *res)
{
    Elf64_Ehdr *eh;
    int error;
    ssize_t len;
    char *data;

    if (path == NULL || proc == NULL) {
        return -EINVAL;
    }

    if (res == NULL) {
        return -EINVAL;
    }

    len = initrd_open(path, &data);
    if (len < 0) {
        printf("elf_load: failed to open \"%s\"\n", path);
        return len;
    }

    eh = (Elf64_Ehdr *)data;
    if ((error = elf64_verify(eh)) != 0) {
        return error;
    }

    error = elf64_do_load(eh, proc);
    if (error < 0) {
        return error;
    }

    res->entrypoint = eh->e_entry;
    return 0;
}
