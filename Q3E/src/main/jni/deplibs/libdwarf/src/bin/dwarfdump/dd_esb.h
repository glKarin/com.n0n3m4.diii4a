/*
Copyright (C) 2005 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright 2011-2018 David Anderson. All Rights Reserved.
    This program is free software; you can redistribute it
    and/or modify it under the terms of version 2 of the GNU
    General Public License as published by the Free Software
    Foundation.

    This program is distributed in the hope that it would
    be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A
    PARTICULAR PURPOSE.

    Further, this software is distributed without any warranty
    that it is free of the rightful claim of any third person
    regarding infringement or the like.  Any license provided
    herein, whether implied or otherwise, applies only to
    this software file.  Patent licenses, if any, provided
    herein do not apply to combinations of this program with
    other software, or any other product whatsoever.

    You should have received a copy of the GNU General Public
    License along with this program; if not, write the Free
    Software Foundation, Inc., 51 Franklin Street - Fifth
    Floor, Boston MA 02110-1301, USA.
*/

/*  esb.h
    Extensible string buffer.
    A simple vaguely  object oriented extensible string buffer.

    The struct could be opaque here, but it seems ok to expose
    the contents: simplifies debugging.
*/

#ifndef ESB_H
#define ESB_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/*  If esb_allocated_size > 0 then esb_string points to
    esb_allocated_size bytes and
    esb_used bytes <= (esb_allocated_size -1)
    All operations that insert or return strings
    ensure, first, that esb_allocated_size is non-zero
    and the requirements here are met.

    In esb_constructor esb_allocated_size and esb_used_bytes
    and esb_string are set 0. No malloc done.

    Default init alloc  sets  esb_allocated_size=alloc_size
    and mallocs alloc_size bytes.
    and esb_used_bytes = 0 and esb_string[0] = NUL.
*/

struct esb_s {
    char *  esb_string; /* pointer to the data itself, or  NULL. */
    size_t  esb_allocated_size; /* Size of allocated data or 0
        The allocated size must include the trailing NUL
        as we do insert a NUL. */
    size_t  esb_used_bytes; /* Amount of space used  or 0,
        which does not include the trailing NUL.
        Matches what strlen(esb_string) would return. */
    /*  rigid means never do malloc.
        fixed means the size is a user buffer  but
        if we run out of room feel free to malloc space
        (and then unset esb_fixed).
        An esb can be (fixed and rigid), (fixed and not rigid),
        or (not fixed and not rigid).  */
    char    esb_fixed;
    char    esb_rigid;
};

/*  Mirroring the broken code in libdwarf.h.in  */
typedef long long esb_int;
typedef unsigned long long esb_unsigned;

/* Open/close the null device used during formatting printing */
FILE *esb_open_null_device(void);
void esb_close_null_device(void);

/* string length taken from string itself. */
void esb_append(struct esb_s *data, const char * in_string);

/* The 'len' is believed. Do not pass in strings < len bytes long. */
void esb_appendn(struct esb_s *data,
    const char * in_string, size_t len);

/* Always returns an empty string or a non-empty string. Never 0. */
char * esb_get_string(struct esb_s *data);

/*  Sets esb_used_bytes to zero. The string is not freed and
    esb_allocated_size is unchanged.  */
void esb_empty_string(struct esb_s *data);

/*  Return esb_used_bytes. */
size_t esb_string_len(struct esb_s *data);

/*  The following are for testing esb, not use by dwarfdump. */

/*  *data is presumed to contain garbage, not values, and
    is properly initialized. */
void esb_constructor(struct esb_s *data);
void esb_constructor_rigid(struct esb_s *data,
    char *buf,size_t buflen);
void esb_constructor_fixed(struct esb_s *data,
    char *buf,size_t buflen);

void esb_force_allocation(struct esb_s *data, size_t minlen);

/*  The string is freed, contents of *data set to zeroes. */
void esb_destructor(struct esb_s *data);

/*  To get all paths in the code tested, this sets the
    allocation/reallocation to the given value,
    which can be quite small
    but must not be zero. */
void esb_alloc_size(size_t size);
size_t esb_get_allocated_size(struct esb_s *data);

/* Append a formatted string */
void esb_append_printf_s(struct esb_s *data,
    const char *format,const char *s);
void esb_append_printf_i(struct esb_s *data,
    const char *format,esb_int);
void esb_append_printf_u(struct esb_s *data,
    const char *format,esb_unsigned);

/* Get a copy of the internal data buffer */
char * esb_get_copy(struct esb_s *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* ESB_H */
