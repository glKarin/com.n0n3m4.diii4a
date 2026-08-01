/*
Copyright (C) 2000,2004 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright(C) David Anderson 2016. All Rights reserved.

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
/*
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

#include <stdio.h>  /* printf() */
#include <stdlib.h> /* exit() */

#ifdef HAVE_STDINT_H
#include <stdint.h> /* uintptr_t */
#endif /* HAVE_STDINT_H */

#include "libdwarf.h"
#include "dwarf_tsearch.h"
#include "dd_makename.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_minimal.h"

void dd_minimal_count_global_error(void) {}

#if defined(__WIN32) &&  (!defined(__GNUC__) && !defined(__clang__))
#pragma warning(disable:4996)    /* Warning when migrated to VS2010 */
#endif /* _WIN32 */

#define TRUE 1
#define FALSE 0

#define VALTYPE char *
#define DW_TSHASHTYPE uintptr_t

char *samples[]  = {
"abcd",
"efgh",
"a",
"abcd",
0
};

int main(void)
{
    char *e1 = 0;
    char *e2= 0;
    char *e3= 0;
    char *e4= 0;
    int errct = 0;

    e1 = makename(samples[0]);
    e2 = makename(samples[1]);
    e3 = makename(samples[2]);
    e4 = makename(samples[3]);

    if (e1 != e4) {
        printf(" FAIL. mismatch  pointers\n");
        ++errct;
    }
    if (e1 == e2 ) {
        printf(" FAIL. match  pointers\n");
        ++errct;
    }
    if ( e1 == e3) {
        printf(" FAIL. match  pointers\n");
        ++errct;
    }
    if (errct) {
        exit(EXIT_FAILURE);
    }
    printf("PASS makename test\n");
    return 0;
}
