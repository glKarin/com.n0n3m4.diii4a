/*
  Copyright 2015-2016 David Anderson. All rights reserved.

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

#include <stdio.h>  /* printf() */

#ifdef HAVE_STDINT_H
#include <stdint.h> /* uintptr_t */
#endif /* HAVE_STDINT_H */

#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dwarf_tsearch.h"
#include "dd_helpertree.h"

/*  WARNING: the tree walk functions will, if presented **tree
    when *tree is wanted, simply find nothing. No error,
    just bad results. So when a walk produces nothing
    suspect a code mistake here.
    The basic problem is void* is a terrible way to
    pass in a pointer. But it's how tsearch was defined
    long ago.
*/

struct Helpertree_Base_s testbase;
int main(int argc, char *argv[])
{
    struct  Helpertree_Map_Entry_s *re = 0;
    int failcount = 0;

    /* Test 1 */
    re = helpertree_add_entry(0x1000,0,&testbase);
    if (!re) {
        printf("FAIL test1\n");
        failcount++;
    }
    re = helpertree_add_entry(0x2000,0,&testbase);
    if (!re) {
        printf("FAIL test2\n");
        failcount++;
    }

    re = helpertree_find(0x1000,&testbase);
    if (!re) {
        printf("FAIL test3\n");
        failcount++;
    }
    re = helpertree_find(0x2000,&testbase);
    if (!re) {
        printf("FAIL test4\n");
        failcount++;
    }
    re = helpertree_find(0x2004,&testbase);
    if (re) {
        printf("FAIL test5\n");
        failcount++;
    }
    helpertree_clear_statistics(&testbase);
    if (failcount) {
        return 1;
    }
    printf("PASS helpertree\n");
    return 0;

    (void)argc;
    (void)argv;
}
