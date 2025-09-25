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

#ifndef _OS_MAC_H_
#define _OS_MAC_H_ 1

#include <sys/types.h>

/* Forward declarations */
struct mac_border;
struct proc;

typedef enum {
    BORDER_NONE,
    BORDER_FBDEV,
    __BORDER_MAX
} border_id_t;

/*
 * MAC levels
 *
 * Processes or users with lower levels cannot
 * access higher levels, higher levels can access
 * lower levels.
 */
typedef enum {
    MAC_GLOBAL,
    MAC_RESTRICTED,
    MAC_SECRET
} mac_level_t;

/*
 * Mapping arguments for MAC
 *
 * @off: Mapping offset
 * @len: Mapping length
 * @flags: Optional flags
 * @dp_res: Data pointer result written here
 */
struct mac_map_args {
    off_t off;
    size_t len;
    int flags;
    void **dp_res;
};

/*
 * MAC operation hooks
 *
 * @map: Map a resource, return length
 * @sync: Sync resource with driver
 * @getattr: Get attributes of resource
 */
struct mac_ops {
    ssize_t(*map)(struct mac_border *mbp, struct mac_map_args *args);
    int(*sync)(struct mac_border *mbp, int flags);
    int(*getattr)(struct mac_border *mbp, void *p, size_t len);
};

/*
 * A MAC border sits inbetween a resource and the user
 * and controls if they can access it or not.
 */
struct mac_border {
    mac_level_t level;
    struct mac_ops *ops;
};

/*
 * Check creds with a specific process and a specific
 * resource border.
 *
 * @procp: Process to check with border
 * @mbp: Border to 'procp' is trying to access
 *
 * Returns zero if the check passed, otherwise a less than
 * zero value if the check failed.
 */
int mac_check_creds(struct proc *procp, struct mac_border *mbp);

/*
 * Map a resource into process address space by
 * going through its border
 *
 * @mbp: Border of resource
 * @off: Offset of mapping to make
 * @len: Length of mapping to make
 * @res: Result pointer is written here
 * @flags: Optional flags
 *
 * Returns zero on success, otherwise a less than zero value
 * on failure.
 */
ssize_t mac_map(struct mac_border *mbp, off_t off, size_t len, void **res, int flags);

/*
 * Acquire a specific border using an ID
 *
 * @id: ID to lookup
 *
 * Returns the pointer pointer on success, otherwise a NULL
 * value on failure.
 */
struct mac_border *mac_get_border(border_id_t id);

#endif  /* !_OS_MAC_H_ */
