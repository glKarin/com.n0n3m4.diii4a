/*
  Copyright (C) 2005 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2013-2019 David Anderson. All Rights Reserved.
  This program is free software; you can redistribute it and/or
  modify it under the terms of version 2 of the GNU General
  Public License as published by the Free Software Foundation.

  This program is distributed in the hope that it would be
  useful, but WITHOUT ANY WARRANTY; without even the implied
  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  Further, this software is distributed without any warranty
  that it is free of the rightful claim of any third person
  regarding infringement or the like.  Any license provided
  herein, whether implied or otherwise, applies only to this
  software file.  Patent licenses, if any, provided herein
  do not apply to combinations of this program with other
  software, or any other product whatsoever.

  You should have received a copy of the GNU General Public
  License along with this program; if not, write the Free
  Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
  Boston MA 02110-1301, USA.
*/

/*  esb.c
    extensible string buffer.

    A simple means (vaguely like a C++ class) that
    enables safely saving strings of arbitrary length built up
    in small pieces.

    We really do allow only C strings here. NUL bytes
    in a string result in adding only up to the NUL.
    Errors get an ERROR line printed.

    The functions assume that
    pointer arguments of all kinds are not NULL.
*/

#include <config.h>
#include <stdarg.h> /* va_end() va_start() */
#include <stdlib.h> /* exit() free() malloc() realloc() */
#include <string.h> /* memcpy() memset() strlen() */
#include <stdio.h> /* FILE decl for dd_esb.h */
#include "dd_esb.h"
#include "libdwarf_private.h" /* For malloc/calloc debug */
#include "dd_minimal.h" /* For dwarfdump error count */

/*  INITIAL_ALLOC value takes no account of space for a trailing NUL,
    the NUL is accounted for in init_esb_string
    and in later tests against esb_allocated_size. */
#ifdef TESTING
#define INITIAL_ALLOC 1  /* TESTING */
#define MALLOC_COUNT 1
#else
/*  There is nothing magic about this size.
    It is just big enough to avoid most resizing. */
#define INITIAL_ALLOC 100
#endif
/*  Allow for final NUL */
static size_t alloc_size = INITIAL_ALLOC;

/* NULL device used when printing formatted strings */
static FILE *null_device_handle = 0;
#ifdef _WIN32
#define NULL_DEVICE_NAME "NUL"
#else
#define NULL_DEVICE_NAME "/dev/null"
#endif /* _WIN32 */

#ifdef MALLOC_COUNT
size_t malloc_count = 0;
size_t malloc_size = 0;
#endif

/*  m must be a string, like  "ESBERR..."  for this to work */
#define ESBERR(m) esb_appendn_internal(data,m,sizeof(m)-1)

#define INCRVALIDATENEXT(n) do { \
    ++n ;                       \
    if (!format[n]) {           \
        ESBERR("ESBERR_next_followedby_end"); \
        return;                 \
    }                           \
    } while (0)
FILE *esb_open_null_device(void)
{
    if (!null_device_handle) {
        null_device_handle = fopen(NULL_DEVICE_NAME,"w");
    }
    return null_device_handle;
}

/* Close the null device used during formatting printing */
void esb_close_null_device(void)
{
    if (null_device_handle) {
        fclose(null_device_handle);
        null_device_handle = 0;
    }
}

/*  min_len is overall space wanted for initial alloc.
    ASSERT: esb_allocated_size == 0
    Return FALSE if unable to init. Else return TRUE */
static int
init_esb_string(struct esb_s *data, size_t min_len)
{
    char* d;
    static int notederror = FALSE;

    if (data->esb_allocated_size > 0) {
        return TRUE;
    }
    /*  Only esb_constructor applied so far.
        Now Allow for string space. */
    if (min_len < alloc_size) {
        min_len = alloc_size;
    } else {
        min_len++ ; /* Allow for NUL at end */
    }
    d = (char *)malloc(min_len);
#ifdef MALLOC_COUNT
    ++malloc_count;
    malloc_size += min_len;
#endif
    data->esb_used_bytes = 0;
    if (!d) {
        if (!notederror) {
            printf("ERROR: dwarfdump is out of memory"
                " allocating"
                " %lu esb string bytes.  Unable to set up esb_string"
                "strings will be incomplete.\n",
                (unsigned long) min_len);
            notederror = TRUE;
            dd_minimal_count_global_error();
        }
        data->esb_allocated_size = 0;
        data->esb_string = 0;
        return FALSE;
    }
    data->esb_string = d;
    data->esb_string[0] = 0;
    data->esb_allocated_size = min_len;
    return TRUE;
}

/*  Make more room. Leaving  contents unchanged, effectively.
    The NUL byte at end has room and this preserves that room.
    Return TRUE if allocated, else return FALSE
*/
static int
esb_allocate_more(struct esb_s *data, size_t len)
{
    size_t new_size = 0;
    char* newd = 0;
    int ires = FALSE;
    int notederror = FALSE;

    if (data->esb_rigid) {
        return TRUE;
    }
    if (data->esb_allocated_size == 0) {
        ires = init_esb_string(data, alloc_size);
        if (ires == FALSE) {
            return FALSE;
        }
    }
    new_size = data->esb_allocated_size + len;
    if (new_size < alloc_size) {
        new_size = alloc_size;
    }
    if (data->esb_fixed) {
        newd = malloc(new_size);
#ifdef MALLOC_COUNT
        ++malloc_count;
        malloc_size += len;
#endif
        if (newd) {
            memcpy(newd,data->esb_string,data->esb_used_bytes+1);
        } else {
            return FALSE;
        }
    } else {
        newd = realloc(data->esb_string, new_size);
#ifdef MALLOC_COUNT
        ++malloc_count;
        malloc_size += len;
#endif
    }
    if (!newd) {
        if ( !notederror) {
            printf("ERROR: dwarfdump is out of memory "
                "allocating "
                "%lu esb string bytes, "
                "strings will print incorrectly\n",
                (unsigned long)new_size);
            notederror=TRUE;
            dd_minimal_count_global_error();
        }
        return FALSE;
    }
    /*  If the area was reallocated by realloc() the earlier
        space was free()d by realloc(). */
    data->esb_string = newd;
    data->esb_allocated_size = new_size;
    data->esb_fixed = 0;
    return TRUE;
}

/*  Ensure that the total buffer length is large enough that
    at least minlen bytes are available, unused,
    in the allocation. */
void
esb_force_allocation(struct esb_s *data, size_t minlen)
{
    size_t target_len = 0;

    if (data->esb_rigid) {
        return;
    }
    if (data->esb_allocated_size == 0) {
        int ires = init_esb_string(data, alloc_size);
        if (ires == FALSE) {
            return;
        }
    }
    target_len = data->esb_used_bytes + minlen;
    if (data->esb_allocated_size < target_len) {
        size_t needed = target_len - data->esb_allocated_size;
        esb_allocate_more(data,needed);
    }
}

/*  The 'len' is believed. Do not pass in strings < len bytes long.
    For strlen(in_string) > len bytes we take the initial len bytes.
    len does not include the trailing NUL. */
static void
esb_appendn_internal(struct esb_s *data,
    const char * in_string, size_t len)
{
    size_t remaining = 0;
    size_t needed = len;

    if (data->esb_allocated_size == 0) {
        size_t maxlen = (len >= alloc_size)? len:alloc_size;
        int ires = init_esb_string(data, maxlen);

        if (ires == FALSE) {
            return;
        }
    }
    /*  ASSERT: data->esb_allocated_size > data->esb_used_bytes  */
    remaining = data->esb_allocated_size - data->esb_used_bytes - 1;
    if (remaining <= needed) {
        if (data->esb_rigid && len > remaining) {
            len = remaining;
        } else {
            size_t alloc_amt = needed - remaining;
            int ires = esb_allocate_more(data,alloc_amt);

            if (ires == FALSE) {
                return;
            }
        }
    }
    if (len ==  0) {
        /* No room for anything more, or no more requested. */
        return;
    }
    /*  Might be that len < string len, so do not assume len+1 byte
        is a NUL byte. */
    memcpy(&data->esb_string[data->esb_used_bytes],in_string,len);
    data->esb_used_bytes += len;
    /* Insist on explicit NUL terminator */
    data->esb_string[data->esb_used_bytes] = 0;
}

/* len >= strlen(in_string) */
void
esb_appendn(struct esb_s *data,
    const char * in_string, size_t len)
{
    size_t full_len = strlen(in_string);

    if (full_len < len) {
        ESBERR("ESBERR_appendn bad call");
        return;
    }
    esb_appendn_internal(data, in_string, len);
}

/*  The length is gotten from the in_string itself, this
    is the usual way to add string data.. */
void
esb_append(struct esb_s *data, const char * in_string)
{
    size_t lenszt = 0;
    if (in_string) {
        lenszt = strlen(in_string);
        if (lenszt) {
            esb_appendn_internal(data, in_string, lenszt);
        }
    }
}

/*  Always returns an empty string or a non-empty string. Never 0. */

char*
esb_get_string(struct esb_s *data)
{
    if (data->esb_allocated_size == 0) {
        int ires = init_esb_string(data, alloc_size);
        if (ires == FALSE) {
            dd_minimal_count_global_error();
            return "<ERROR ESBERROR out of memory >";
        }
    }
    return data->esb_string;
}

/*  Sets esb_used_bytes to zero. The string is not freed and
    esb_allocated_size is unchanged.  */
void
esb_empty_string(struct esb_s *data)
{
    if (data->esb_allocated_size == 0) {
        int ires = init_esb_string(data, alloc_size);
        if (ires == FALSE) {
            return;
        }
    }
    data->esb_used_bytes = 0;
    data->esb_string[0] = 0;
}

/*  Return esb_used_bytes. */
size_t
esb_string_len(struct esb_s *data)
{
    return data->esb_used_bytes;
}

/*  *data is presumed to contain garbage, not values, and
    is properly initialized here. */
void
esb_constructor(struct esb_s *data)
{
    memset(data, 0, sizeof(*data));
}

/*  ASSERT: buflen > 0 */
void
esb_constructor_fixed(struct esb_s *data,char *buf,size_t buflen)
{
    memset(data, 0, sizeof(*data));
    if (buflen < 1) {
        return;
    }
    data->esb_string = buf;
    data->esb_string[0] = 0;
    data->esb_allocated_size = buflen;
    data->esb_used_bytes = 0;
    data->esb_rigid = 0;
    data->esb_fixed = 1;
}

/*  The string is freed, contents of *data set to zeros. */
void
esb_destructor(struct esb_s *data)
{
    if (data->esb_fixed) {
        data->esb_allocated_size = 0;
        data->esb_used_bytes = 0;
        data->esb_string = 0;
        data->esb_rigid = 0;
        data->esb_fixed = 0;
        return;
    }
    if (data->esb_string) {
        free(data->esb_string);
        data->esb_string = 0;
    }
    esb_constructor(data);
}

/*  To get all paths in the code tested, this sets the
    initial allocation/reallocation file-static
    which can be quite small but must not be zero
    The alloc_size variable  is used for initializations. */
void
esb_alloc_size(size_t size)
{
    if (size < 1) {
        size = 1;
    }
    alloc_size = size;
}

size_t
esb_get_allocated_size(struct esb_s *data)
{
    return data->esb_allocated_size;
}

static void
esb_appendn_internal_spaces(struct esb_s *data,size_t l)
{
    static char spacebuf[] =
        {"                                       "};
    size_t charct = sizeof(spacebuf)-1;
    while (l > charct) {
        esb_appendn_internal(data,spacebuf,charct);
        l -= charct;
    }
    /* ASSERT: l > 0 */
    esb_appendn_internal(data,spacebuf,l);
}

static void
esb_appendn_internal_zeros(struct esb_s *data,size_t l)
{
    static char zeros[] =
        {"0000000000000000000000000000000000000000"};
    size_t charct = sizeof(zeros)-1;
    while (l > charct) {
        esb_appendn_internal(data,zeros,charct);
        l -= charct;
    }
    /* ASSERT: l > 0 */
    esb_appendn_internal(data,zeros,l);
}

void
esb_append_printf_s(struct esb_s *data,
    const char *format,const char *s)
{
    size_t stringlenszt = strlen(s);
    size_t next = 0;
    long val = 0;
    char *endptr = 0;
    const char *numptr = 0;
    /* was %[-]fixedlen.  Zero means no len provided. */
    size_t fixedlen = 0;
    /* was %-, nonzero means left-justify */
    long leftjustify = 0;
    size_t prefixlen = 0;

    while (format[next] && format[next] != '%') {
        ++next;
        ++prefixlen;
    }
    if (prefixlen) {
        esb_appendn_internal(data,format,prefixlen);
    }
    if (format[next] != '%') {
        /*   No % operator found */
        ESBERR("ESBERR..esb_append_printf_s has no percent operator");
        return;
    }
    next++;
    if (format[next] == ' ') {
        ESBERR("ESBERR_pct_followedby_space_in_s");
        return;
    }
    if (!format[next]) {
        ESBERR("ESBERR_pct_followedby_wrong_in_s");
        return;
    }
    if (format[next] == '-') {
        leftjustify++;
        next++;
    }
    numptr = format+next;
    val = strtol(numptr,&endptr,10);
    if ( endptr != numptr) {
        fixedlen = val;
    }
    next = (endptr - format);
    if (format[next] != 's') {
        ESBERR("ESBERR_pct_s_missing_in_s");
        return;
    }
    next++;
    if (fixedlen && (fixedlen <= stringlenszt)) {
        leftjustify = 0;
    }
    if (leftjustify) {
        esb_appendn_internal(data,s,stringlenszt);
        if (fixedlen) {
            size_t trailingspaces = fixedlen - stringlenszt;

            esb_appendn_internal_spaces(data,trailingspaces);
        }
    } else {
        if (fixedlen && (fixedlen < stringlenszt)) {
            /*  This lets us have fixedlen < stringlenszt by
                taking all the chars from s ignoring
                the fixedlen*/
            esb_appendn_internal(data,s,stringlenszt);
        } else {
            if (fixedlen) {
                size_t leadingspaces = fixedlen - stringlenszt;
                size_t k = 0;

                for ( ; k < leadingspaces; ++k) {
                    esb_appendn_internal(data," ",1);
                }
            }
            esb_appendn_internal(data,s,stringlenszt);
        }
    }
    if (!format[next]) {
        return;
    }
    {
        const char * startpt = format+next;
        size_t suffixlen = strlen(startpt);

        esb_appendn_internal(data,startpt,suffixlen);
    }
    return;
}

static char dtable[10] = {
'0','1','2','3','4','5','6','7','8','9'
};
static char xtable[16] = {
'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
};
static char Xtable[16] = {
'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
};

/*  With gcc version 5.4.0 20160609  a version using
    const char *formatp instead of format[next]
    and deleting the 'next' variable
    is a few hundredths of a second slower, repeatably.

    We deal with formats like:
    %u   %5u %05u (and ld and lld too).
    %x   %5x %05x (and ld and lld too).  */
void
esb_append_printf_u(struct esb_s *data,
    const char *format,esb_unsigned v)
{
    size_t next = 0;
    long val = 0;
    char *endptr = 0;
    const char *numptr = 0;
    size_t fixedlen = 0;
    int leadingzero = 0;
    int lcount = 0;
    int ucount = 0;
    int dcount = 0;
    int xcount = 0;
    int Xcount = 0;
    char *ctable = 0;
    size_t divisor = 0;
    size_t prefixlen = 0;

    while (format[next] && format[next] != '%') {
        ++next;
        ++prefixlen;
    }
    esb_appendn_internal(data,format,prefixlen);
    if (format[next] != '%') {
        /*   No % operator found */
        ESBERR("ESBERR..esb_append_printf_u has no percent operator");
        return;
    }
    next++;
    if (format[next] == ' ') {
        ESBERR("ESBERR_pct_followedby_space_in_s");
        return;
    }
    if (!format[next]) {
        ESBERR("ESBERR_pct_followedby_wrong_in_s");
        return;
    }
    if (format[next] == '-') {
        ESBERR("ESBERR_printf_u - format not supported");
        INCRVALIDATENEXT(next);
    }
    if (format[next] == '0') {
        leadingzero = 1;
        INCRVALIDATENEXT(next);
    }
    numptr = format+next;
    val = strtol(numptr,&endptr,10);
    if ( endptr != numptr) {
        fixedlen = val;
    }
    next = (endptr - format);
    /*  Following is lx lu or u or llx llu , we take
        all this to mean 64 bits, */
#ifdef _WIN32
    if (format[next] == 'I') {
        /*lcount++;*/
        next++;
    }
    if (format[next] == '6') {
        /*lcount++;*/
        next++;
    }
    if (format[next] == '4') {
        /*lcount++;*/
        next++;
    }
#endif /* _WIN32 */
    if (format[next] == 'l') {
        lcount++;
        next++;
    }
    if (format[next] == 'l') {
        lcount++;
        next++;
    }
    if (format[next] == 'u') {
        ucount++;
        next++;
    }
    if (format[next] == 'd') {
        dcount++;
        next++;
    }
    if (format[next] == 'x') {
        xcount++;
        next++;
    }
    if (format[next] == 'X') {
        Xcount++;
        next++;
    }
    if (format[next] == 's') {
        ESBERR("ESBERR_pct_scount_in_u");
        return;
    }

    if ( (Xcount +xcount+dcount+ucount) > 1) {
        ESBERR("ESBERR_pct_xcount_etc_u");
        /* error */
        return;
    }
    if (lcount > 2) {
        ESBERR("ESBERR_pct_lcount_error_u");
        /* error */
        return;
    }
    if (dcount > 0) {
        ESBERR("ESBERR_pct_dcount_error_u");
        /* error */
        return;
    }
    if (ucount) {
        divisor = 10;
        ctable = dtable;
    } else {
        divisor = 16;
        if (xcount) {
            ctable = xtable;
        } else {
            ctable = Xtable;
        }
    }
    {
        char digbuf[36];
        char *digptr = 0;
        size_t digcharlen = 0;
        esb_unsigned remaining = v;

        if (divisor == 16) {
            digptr = digbuf+sizeof(digbuf) -1;
            for ( ;; ) {
                esb_unsigned dig;
                dig = remaining & 0xf;
                remaining = remaining >> 4;
                *digptr = ctable[dig];
                ++digcharlen;
                if (!remaining) {
                    break;
                }
                --digptr;
            }
        } else {
            digptr = digbuf+sizeof(digbuf) -1;
            *digptr = 0;
            --digptr;
            for ( ;; ) {
                esb_unsigned dig;
                dig = remaining % divisor;
                remaining /= divisor;
                *digptr = ctable[dig];
                ++digcharlen;
                if (!remaining) {
                    break;
                }
                --digptr;
            }
        }
        if (fixedlen <= digcharlen) {
            esb_appendn_internal(data,digptr,digcharlen);
        } else {
            if (!leadingzero) {
                size_t justcount = fixedlen - digcharlen;
                esb_appendn_internal_spaces(data,justcount);
                esb_appendn_internal(data,digptr,digcharlen);
            } else {
                size_t prefixcount = fixedlen - digcharlen;
                esb_appendn_internal_zeros(data,prefixcount);
                esb_appendn_internal(data,digptr,digcharlen);
            }
        }
    }
    if (format[next]) {
        size_t trailinglen = strlen(format+next);
        esb_appendn_internal(data,format+next,trailinglen);
    }
}

static char v32m[] = {"-2147483648"};
static char v64m[] = {"-9223372036854775808"};

/*  We deal with formats like:
    %d   %5d %05d %+d %+5d (and ld and lld too). */
void
esb_append_printf_i(struct esb_s *data,
    const char *format,esb_int v)
{
    size_t next = 0;
    long val = 0;
    char *endptr = 0;
    const char *numptr = 0;
    size_t fixedlen = 0;
    int leadingzero = 0;
    int pluscount = 0;
    int lcount = 0;
    int ucount = 0;
    int dcount = 0;
    int xcount = 0;
    int Xcount = 0;
    char *ctable = dtable;
    size_t prefixlen = 0;
    int done = 0;

    while (format[next] && format[next] != '%') {
        ++next;
        ++prefixlen;
    }
    esb_appendn_internal(data,format,prefixlen);
    if (format[next] != '%') {
        /*   No % operator found */
        ESBERR("ESBERR..esb_append_printf_i has no percent operator");
        return;
    }
    next++;
    if (format[next] == ' ') {
        ESBERR("ESBERR_pct_followedby_space_in_s");
        return;
    }
    if (!format[next]) {
        ESBERR("ESBERR_pct_followedby_wrong_in_s");
        return;
    }
    if (format[next] == '-') {
        ESBERR("ESBERR_printf_i - format not supported");
        INCRVALIDATENEXT(next);
    }
    if (format[next] == '+') {
        pluscount++;
        INCRVALIDATENEXT(next);
    }
    if (format[next] == '0') {
        leadingzero = 1;
        INCRVALIDATENEXT(next);
    }
    numptr = format+next;
    val = strtol(numptr,&endptr,10);
    if ( endptr != numptr) {
        fixedlen = val;
    }
    next = (endptr - format);
    /*  Following is lx lu or u or llx llu , we take
        all this to mean 64 bits, */
#ifdef _WIN32
    if (format[next] == 'I') {
        /*lcount++;*/
        next++;
    }
    if (format[next] == '6') {
        /*lcount++;*/
        next++;
    }
    if (format[next] == '4') {
        /*lcount++;*/
        next++;
    }
#endif /* _WIN32 */
    if (format[next] == 'l') {
        lcount++;
        next++;
    }
    if (format[next] == 'l') {
        lcount++;
        next++;
    }
    if (format[next] == 'u') {
        ucount++;
        next++;
    }
    if (format[next] == 'd') {
        dcount++;
        next++;
    }
    if (format[next] == 'x') {
        xcount++;
        next++;
    }
    if (format[next] == 'X') {
        Xcount++;
        next++;
    }
    if (format[next] == 's') {
        ESBERR("ESBERR_pct_scount_in_i");
        return;
    }
    if (!dcount || (lcount >2) ||
        (Xcount +xcount+dcount+ucount) > 1) {
        /* error */
        ESBERR("ESBERR_xcount_etc_i");
        return;
    }
    {
        char digbuf[36];
        char *digptr = digbuf+sizeof(digbuf) -1;
        size_t digcharlen = 0;
        esb_int remaining = v;
        int vissigned = 0;
        esb_int divisor = 10;

        *digptr = 0;
        --digptr;
        if (v < 0) {
            vissigned = 1;
            /*  This test is for twos-complement
                machines and would be better done via
                configure with a compile-time check
                so we do not need a size test at runtime. */
            if (sizeof(v) == 8) {
                esb_unsigned vm = 0x7fffffffffffffffULL;
                if (vm == (esb_unsigned)~v) {
                    memcpy(digbuf,v64m,sizeof(v64m));
                    digcharlen = sizeof(v64m)-1;
                    digptr = digbuf;
                    done = 1;
                } else {
                    remaining = -v;
                }
            } else if (sizeof(v) == 4) {
                esb_unsigned vm = 0x7fffffffL;
                if (vm == (esb_unsigned)~v) {
                    memcpy(digbuf,v32m,sizeof(v32m));
                    digcharlen = sizeof(v32m)-1;
                    digptr = digbuf;
                    done = 1;
                } else {
                    remaining = -v;
                }
            }else {
                ESBERR("ESBERR_sizeof_v_i");
                /* error */
                return;
            }
        }
        if (!done) {
            for ( ;; ) {
                esb_unsigned dig = 0;

                dig = remaining % divisor;
                remaining /= divisor;
                *digptr = ctable[dig];
                digcharlen++;
                if (!remaining) {
                    break;
                }
                --digptr;
            }
            if (vissigned) {
                --digptr;
                digcharlen++;
                *digptr = '-';
            } else if (pluscount) {
                --digptr;
                digcharlen++;
                *digptr = '+';
            }
        }
        if (fixedlen > 0) {
            if (fixedlen <= digcharlen) {
                esb_appendn_internal(data,digptr,digcharlen);
            } else {
                size_t prefixcount = fixedlen - digcharlen;
                if (!leadingzero) {
                    esb_appendn_internal_spaces(data,prefixcount);
                    esb_appendn_internal(data,digptr,digcharlen);
                } else {
                    if (*digptr == '-') {
                        esb_appendn_internal(data,"-",1);
                        esb_appendn_internal_zeros(data,prefixcount);
                        digptr++;
                        esb_appendn_internal(data,
                            digptr,digcharlen-1);
                    } else if (*digptr == '+') {
                        esb_appendn_internal(data,"+",1);
                        esb_appendn_internal_zeros(data,prefixcount);
                        digptr++;
                        esb_appendn_internal(data,
                            digptr,digcharlen-1);
                    } else {
                        esb_appendn_internal_zeros(data,prefixcount);
                        esb_appendn_internal(data,digptr,digcharlen);
                    }
                }
            }
        } else {
            esb_appendn_internal(data,digptr,digcharlen);
        }
    }
    if (format[next]) {
        size_t trailinglen = strlen(format+next);
        esb_appendn_internal(data,format+next,trailinglen);
    }
}

/*  Get a copy of the internal data buffer.
    It is up to the code calling this
    to free() the string using the
    pointer returned here.
    Returns NULL if copy not possible. */
char*
esb_get_copy(struct esb_s *data)
{
    char* copy = NULL;

    /* is ok as is if esb_allocated_size is 0 */
    size_t len = data->esb_used_bytes+1;
    if (len) {
        copy = (char*)malloc(len);
        if (!copy) {
            return NULL;
        }
#ifdef MALLOC_COUNT
        ++malloc_count;
        malloc_size += len+1;
#endif
        memcpy(copy,data->esb_string,len);
    }
    return copy;
}
