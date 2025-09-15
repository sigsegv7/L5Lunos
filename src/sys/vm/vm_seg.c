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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/syslog.h>
#include <sys/panic.h>
#include <os/spinlock.h>
#include <vm/physseg.h>
#include <vm/vm.h>
#include <limine.h>
#include <string.h>

#define BYTES_PER_MIB 8388608

static size_t pages_free = 0;
static size_t pages_used = 0;
static size_t pages_total = 0;
static size_t highest_frame_idx = 0;
static size_t bitmap_size = 0;
static size_t bitmap_free_start = 0;
static ssize_t last_idx = 0;

static uint8_t *bitmap;
static struct limine_memmap_response *resp = NULL;
static struct spinlock lock = {0};

static struct limine_memmap_request mmap_req = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

static const char *memstr_tab[] = {
    [LIMINE_MEMMAP_USABLE] = "usable",
    [LIMINE_MEMMAP_RESERVED] = "reserved",
    [LIMINE_MEMMAP_ACPI_RECLAIMABLE] = "ACPI reclaimable",
    [LIMINE_MEMMAP_ACPI_NVS] = "ACPI NVS",
    [LIMINE_MEMMAP_BAD_MEMORY] = "bad",
    [LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE] = "bootloader reclaimable",
    [LIMINE_MEMMAP_KERNEL_AND_MODULES] = "lunos",
    [LIMINE_MEMMAP_FRAMEBUFFER] = "vram"
};

/*
 * Populate physical memory bitmap.
 */
static void
physmem_populate_bitmap(void)
{
    struct limine_memmap_entry *ent;

    for (size_t i = 0; i < resp->entry_count; ++i) {
        ent = resp->entries[i];
        pages_total += ent->length / DEFAULT_PAGESIZE;

        if (ent->type != LIMINE_MEMMAP_USABLE) {
            /* This memory is not usable */
            pages_used += ent->length / DEFAULT_PAGESIZE;
            continue;
        }

        if (bitmap_free_start == 0) {
            bitmap_free_start = ent->base / DEFAULT_PAGESIZE;
        }

        for (size_t j = 0; j < ent->length; j += DEFAULT_PAGESIZE) {
            clrbit(bitmap, (ent->base + j) / DEFAULT_PAGESIZE);
        }

        pages_free += ent->length / DEFAULT_PAGESIZE;
    }
}

/*
 * Allocate physical memory for the bitmap
 * we'll use to keep track of free memory.
 */
static void
physmem_alloc_bitmap(void)
{
    struct limine_memmap_entry *ent;
    uintptr_t highest_addr = 0;

    for (size_t i = 0; i < resp->entry_count; ++i) {
        ent = resp->entries[i];

        if (ent->type != LIMINE_MEMMAP_USABLE) {
            /* This memory is not usable */
            continue;
        }

        if (ent->length >= bitmap_size) {
            bitmap = PHYS_TO_VIRT(ent->base);
            memset(bitmap, 0xFF, bitmap_size);
            ent->length -= bitmap_size;
            ent->base += bitmap_size;
            return;
        }

        highest_addr = MAX(highest_addr, ent->base + ent->length);
    }
}

/*
 * Init the physical memory bitmap.
 */
static void
physmem_init_bitmap(void)
{
    uintptr_t highest_addr = 0;
    struct limine_memmap_entry *ent;
    uintptr_t start, end;
    const char *typestr = "bad";

    for (size_t i = 0; i < resp->entry_count; ++i) {
        ent = resp->entries[i];

        if (ent->type < NELEM(memstr_tab)) {
            typestr = memstr_tab[ent->type];
        }

        start = ent->base;
        end = ent->base + ent->length;
        printf("sysmem: [%p -> %p]: %s\n", start, end, typestr);

        if (ent->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        highest_addr = MAX(highest_addr, ent->base + ent->length);
    }

    highest_frame_idx = highest_addr / DEFAULT_PAGESIZE;
    bitmap_size = ALIGN_UP(highest_frame_idx / 8, DEFAULT_PAGESIZE);

    physmem_alloc_bitmap();
    physmem_populate_bitmap();
}

/*
 * Allocate page frames.
 *
 * @count: Number of frames to allocate.
 */
static uintptr_t
__vm_alloc_frame(size_t count)
{
    size_t frames = 0;
    ssize_t idx = -1;
    uintptr_t ret = 0;

    for (size_t i = last_idx; i < highest_frame_idx; ++i) {
        if (!testbit(bitmap, i)) {
            if (idx < 0)
                idx = i;
            if (++frames >= count)
                break;

            continue;
        }

        idx = -1;
        frames = 0;
    }

    if (idx < 0 || frames != count) {
        ret = 0;
        goto done;
    }

    for (size_t i = idx; i < idx + count; ++i) {
        setbit(bitmap, i);
    }
    ret = idx * DEFAULT_PAGESIZE;
    last_idx = idx;
    memset(PHYS_TO_VIRT(ret), 0, count * DEFAULT_PAGESIZE);
done:
    return ret;
}

/*
 * Central frame allocation routine
 */
uintptr_t
vm_alloc_frame(size_t count)
{
    uintptr_t ret;

    spinlock_acquire(&lock);
    if ((ret = __vm_alloc_frame(count)) == 0) {
        last_idx = 0;
        ret = __vm_alloc_frame(count);
    }

    if (ret == 0) {
        panic("out of memory\n");
    }

    pages_used += count;
    pages_free -= count;
    spinlock_release(&lock);
    return ret;
}

/*
 * Central frame freeing routine
 */
void
vm_free_frame(uintptr_t base, size_t count)
{
    size_t stop_at = base + (count * DEFAULT_PAGESIZE);

    base = ALIGN_UP(base, DEFAULT_PAGESIZE);

    spinlock_acquire(&lock);
    for (uintptr_t p = base; p < stop_at; p += DEFAULT_PAGESIZE) {
        clrbit(bitmap, p / DEFAULT_PAGESIZE);
    }
    pages_used -= count;
    pages_free += count;
    spinlock_release(&lock);
}

int
vm_seg_init(struct physmem_stat *stat)
{
    resp = mmap_req.response;
    physmem_init_bitmap();

    stat->pages_free = pages_free;
    stat->pages_used = pages_used;
    printf(
        "physseg initialized [free: %d, used: %d]\n",
        pages_free,
        pages_used
    );
    return 0;
}
