
/*
 * default memory allocator for libavutil
 * Copyright (c) 2002 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file libavutil/mem.c
 * default memory allocator for libavutil
 */

#include "config.h"
#include <stdlib.h>
#ifndef __KERNEL__
#include <limits.h>
#include <string.h>
#endif
#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "mem.h"

/* here we can use OS-dependent allocation functions */
#undef free
#undef malloc
#undef realloc

#ifdef __KERNEL__
#define free kfree
#define malloc(x) kmalloc(x, GFP_KERNEL)
#define realloc(p, x) krealloc(p, x, GFP_KERNEL)
#endif

/* You can redefine av_malloc and av_free in your project to use your
   memory allocator. You do not need to suppress this file because the
   linker will do it automatically. */

void *av_malloc(unsigned int size)
{
    void *ptr = NULL;
#if CONFIG_MEMALIGN_HACK
    long diff;
#endif

    /* let's disallow possible ambiguous cases */
    if (size > (INT_MAX - 16) )
    { return NULL; }

#if CONFIG_MEMALIGN_HACK
    ptr = malloc(size + 16);

    if (!ptr)
    { return ptr; }

    diff = ((-(long)ptr - 1) & 15) + 1;
    ptr = (char *)ptr + diff;
    ((char *)ptr)[-1] = (char)diff;
#elif HAVE_POSIX_MEMALIGN

    if (posix_memalign(&ptr, 16, size))
    { ptr = NULL; }

#elif HAVE_MEMALIGN
    ptr = memalign(16, size);
    /* Why 64?
       Indeed, we should align it:
    	 on 4 for 386
    	 on 16 for 486
    	 on 32 for 586, PPro - K6-III
    	 on 64 for K7 (maybe for P3 too).
       Because L1 and L2 caches are aligned on those values.
       But I don't want to code such logic here!
     */
    /* Why 16?
    Because some CPUs need alignment, for example SSE2 on P4, & most RISC CPUs
    it will just trigger an exception and the unaligned load will be done in the
    exception handler or it will just segfault (SSE2 on P4).
    Why not larger? Because I did not see a difference in benchmarks ...
    */
    /* benchmarks with P3
    memalign(64)+1          3071,3051,3032
    memalign(64)+2          3051,3032,3041
    memalign(64)+4          2911,2896,2915
    memalign(64)+8          2545,2554,2550
    memalign(64)+16         2543,2572,2563
    memalign(64)+32         2546,2545,2571
    memalign(64)+64         2570,2533,2558

    BTW, malloc seems to do 8-byte alignment by default here.
    */
#else
    ptr = malloc(size);
#endif

    return ptr;
}

/* 2010/30/06 9:12:00 liuxw+00139685 */
/* 增加函数指针av_malloc_hw，用来代替av_malloc */
#if 0
funAVMalloc *av_malloc_hw = av_malloc;
#else
funAVMalloc *av_malloc_hw = NULL;
#endif

void *av_realloc(void *ptr, unsigned int size)
{
#if CONFIG_MEMALIGN_HACK
    int diff;
#endif

    /* let's disallow possible ambiguous cases */
    if (size > (INT_MAX - 16) )
    { return NULL; }

#if CONFIG_MEMALIGN_HACK

    //FIXME this isn't aligned correctly, though it probably isn't needed
    if (!ptr) { return av_malloc(size); }

    diff = ((char *)ptr)[-1];
    return (char *)realloc((char *)ptr - diff, size + diff) + diff;
#else
    return realloc(ptr, size);
#endif
}

void av_free(void *ptr)
{
    /* XXX: this test should not be needed on most libcs */
    if (ptr)
    {
#if CONFIG_MEMALIGN_HACK
        free((char *)ptr - ((char *)ptr)[-1]);
#else
        free(ptr);
#endif
    }
}

/* 2010/30/06 9:12:00 liuxw+00139685 */
/* 增加函数指针av_free_hw，用来代替av_free */
#if 0
funAVFree *av_free_hw = av_free;
#else
funAVFree *av_free_hw = NULL;
#endif

void av_freep(void *arg)
{
    void **ptr = (void **)arg;

    if (!ptr || !*ptr)
    { return; }

    /* 2010/30/06 9:12:00 liuxw+00139685 */
    /* 将av_free改为av_free_hw */
    //av_free(*ptr);
    av_free_hw(*ptr);
    *ptr = NULL;
}

void *av_mallocz(unsigned int size)
{
    /* 2010/30/06 9:12:00 liuxw+00139685 */
    /* 将av_malloc改为av_malloc_hw */
    void *ptr = av_malloc(size);

    //void *ptr = av_malloc_hw(size);
    if (ptr)
    { memset(ptr, 0, size); }

    return ptr;
}

#ifndef __KERNEL__
char *av_strdup(const char *s)
{
    char *ptr = NULL;

    if (s)
    {
        int len = strlen(s) + 1;
        /* 2010/30/06 9:12:00 liuxw+00139685 */
        /* 将av_malloc改为av_malloc_hw */
        ptr = av_malloc(len);

        //ptr = av_malloc_hw(len);
        if (ptr)
        { memcpy(ptr, s, len); }
    }

    return ptr;
}
#endif
