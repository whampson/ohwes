/*============================================================================*
 * Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                  *
 *                                                                            *
 * This file is part of the Niobium Operating System.                         *
 * Niobium is free software; you may redistribute it and/or modify it under   *
 * the terms of the license agreement provided with this software.            *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: include/nb/acpi.h                                                 *
 * Created: December 20, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __ACPI_H
#define __ACPI_H

#include <stdint.h>

/**
 * System Address Map Types
 */
enum smap_type
{
    SMAP_TYPE_INVALID,  /* (invalid entry) */
    SMAP_TYPE_FREE,     /* Free to use */
    SMAP_TYPE_RESERVED, /* Reserved, do not use */
    SMAP_TYPE_ACPI,     /* ACPI tables, do not use */
    SMAP_TYPE_NV,       /* Non-volatile, do not use */
    SMAP_TYPE_BAD,      /* Bad RAM, do not use */
    SMAP_TYPE_DISABLED  /* Disabled, do not use */
};

/**
 * System Address Map Entry
 */
struct smap_entry
{
    union {
        struct {
            uint32_t addr_lo;
            uint32_t addr_hi;
        };
        uint64_t addr;
    };
    union {
        struct {
            uint32_t limit_lo;
            uint32_t limit_hi;
        };
        uint64_t limit;
    };
    uint32_t type;
    uint32_t extra;
};
_Static_assert(sizeof(struct smap_entry) == 0x18, "sizeof(struct smap_entry)");

#endif /* __ACPI_H */
