#pragma once

#include <string.h>

#ifdef WIN32

#ifndef _LIBC
# define __builtin_expect(expr, val)   (expr)
#endif

#undef memmem

/* Return the first occurrence of NEEDLE in HAYSTACK. */
inline void *
memmem(const void *haystack,
       size_t haystack_len,
       const void *needle,
       size_t needle_len) {
    /* not really Rabin-Karp, just using additive hashing */
    char* haystack_ = (char*)haystack;
    char* needle_ = (char*)needle;
    int hash = 0;       /* this is the static hash value of the needle */
    int hay_hash = 0;   /* rolling hash over the haystack */
    char* last;
    size_t i;

    if (haystack_len < needle_len)
        return NULL;

    if (!needle_len)
        return haystack_;

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
            !memcmp(haystack_, needle_, needle_len))
            return haystack_;

        /* roll the hash */
        hay_hash -= *haystack_;
        hay_hash += *(haystack_ + needle_len);
    }

    return NULL;
}

#endif