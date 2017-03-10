/* Copyright (C) 1991,92,93,94,96,97,98,2000,2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.
*/
#pragma once

#include <string.h>

#ifdef WIN32

#ifndef _LIBC
# define __builtin_expect(expr, val)   (expr)
#endif

#undef memmem

// @see https://raw.githubusercontent.com/jaysi/jlib/3f7a190e1e80b270c9a07cac46aeab960a802cd1/src/memmem.c
// @see http://sourceware.org/ml/libc-alpha/2007-12/msg00000.html

/* Return the first occurrence of NEEDLE in HAYSTACK. */
inline void*
memmem(const void* haystack,
       size_t haystack_len,
       const void* needle,
       size_t needle_len) {
    /* not really Rabin-Karp, just using additive hashing */
    char* haystack_ = (char*)haystack;
    char* needle_ = (char*)needle;
    int hash = 0;       /* this is the static hash value of the needle */
    int hay_hash = 0;   /* rolling hash over the haystack */
    char* last;
    size_t i;

    if (haystack_len < needle_len) {
        return nullptr;
    }

    if (!needle_len) {
        return haystack_;
    }

    /* initialize hashes */
    for (i = needle_len; i; --i) {
        hash += *needle_++;
        hay_hash += *haystack_++;
    }

    /* iterate over the haystack */
    haystack_ = (char*)haystack;
    needle_ = (char*)needle;
    last = haystack_ + (haystack_len - needle_len + 1);
    for (; haystack_ < last; ++haystack_) {
        if (__builtin_expect(hash == hay_hash, 0) &&
                *haystack_ == *needle_ &&   /* prevent calling memcmp, was a optimization from existing glibc */
                !memcmp(haystack_, needle_, needle_len)) {
            return haystack_;
        }

        /* roll the hash */
        hay_hash -= *haystack_;
        hay_hash += *(haystack_ + needle_len);
    }

    return nullptr;
}

#endif