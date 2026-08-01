/*
Copyright (C) 2000,2004 Silicon Graphics, Inc. All Rights Reserved.
Portions Copyright(C) David Anderson 2016-2019. All Rights reserved.

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

   makename.c
   $Revision: 1.4 $
   $Date: 2005/11/08 21:48:42 $

   This used to be elaborate stuff.
   Now it is trivial, as duplicating names is
   unimportant in dwarfdump (in general).

   And in fact, this is only called for attributes and
   tags etc whose true name is unknown. Not for
   any normal case.

*/

#include <config.h>

#include <stdio.h> /* free() */
#include <stdlib.h> /* free() */
#include <string.h> /* strcmp() strdup() */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_tsearchbal.h"
#include "dd_makename.h"
#include "dd_minimal.h"

static void * makename_data;
#define VALTYPE char *

static int
value_compare_func(const void *l, const void *r)
{
    VALTYPE ml = (VALTYPE)l;
    VALTYPE mr = (VALTYPE)r;
    return strcmp(ml,mr);
}
/*  Nothing to free for the 'value' example but
    the key itself. */
static void
value_node_free(void *valp)
{
    VALTYPE v = (VALTYPE)valp;
    free(v);
}

void
makename_destructor(void)
{
    /*  Pass in root, not pointer to root */
    dwarf_tdestroy(makename_data,value_node_free);
    makename_data = 0;
}

/*  WARNING: the tree walk functions will, if presented **tree
    when *tree is wanted, simply find nothing. No error,
    just bad results. So when a walk produces nothing
    suspect a code mistake here.
    The basic problem is void* is a terrible way to
    pass in a pointer. But it's how tsearch was defined
    long ago.

    This function can return NULL if memory is exhausted.
*/

char *
makename(const char *s)
{
    char *newstr = 0;
    VALTYPE re = 0;
    void *retval = 0;
    static int mnfailed = FALSE;

    if (!s) {
        return "";
    }
    retval = dwarf_tfind(s,&makename_data, value_compare_func);
    if (retval) {
        /* We found our string, it existed already. */
        re = *(VALTYPE *)retval;
        return re;
    }
    newstr = (char *)strdup(s);
    if (!newstr) {
        if (!mnfailed) {
            printf("ERROR: Out of memory to record a string"
                "dwarfdump will not work correctly."
                " Future errors of this type are not shown.\n");
            dd_minimal_count_global_error();
            mnfailed = TRUE;
        }
        return "";
    }
    retval = dwarf_tsearch(newstr,&makename_data, value_compare_func);
    if (!retval) {
        if (!mnfailed) {
            printf("ERROR: Out of memory to record a string"
                " in a search table. "
                "dwarfdump will not work correctly."
                " Future errors of this type are not shown.\n");
            dd_minimal_count_global_error();
            mnfailed = TRUE;
        }
        /*  Out of memory, */
        free(newstr);
        return "";
    }
    re = *(VALTYPE *)retval;
    return re;
}
