/*
Copyright (C) 2006 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright 2011-2019 David Anderson. All Rights Reserved.
Portions Copyright 2012 SN Systems Ltd. All rights reserved.

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

#include <config.h>

#include <stddef.h> /* NULL size_t */
#include <string.h> /* strlen() */

#include "dd_canonical_append.h"
#include "dd_safe_strcpy.h"
/*  Given path strings, attempt to make a canonical file name:
    that is, avoid superfluous '/' so that no
    '//' (or worse) is created in the output. The path components
    are to be separated so at least one '/'
    is to appear between the two 'input strings' when
    creating the output.
*/
char *
_dwarf_canonical_append(char *target, unsigned int target_size,
    const char *first_string, const char *second_string)
{
    size_t firstlen = strlen(first_string);
    size_t totallen = firstlen + strlen(second_string) + 1 + 1;

    /*  +1 +1: Leave room for added "/" and final NUL, though that is
        overkill, as we drop a NUL byte too. */
    if (totallen >= target_size) {
        /* Not enough space. */
        return NULL;
    }
    for (; *second_string == '/'; ++second_string) {
    }
    for (; firstlen > 0 && first_string[firstlen - 1] == '/';
        --firstlen) {
    }
    target[0] = 0;
    if (firstlen > 0) {
        dd_safe_strcpy(target,target_size,first_string, firstlen);
    }
    target[firstlen] = '/';
    firstlen++;
    target[firstlen] = 0;
    dd_safe_strcpy(target+firstlen, target_size-firstlen,
        second_string,strlen(second_string));
    return target;
}
