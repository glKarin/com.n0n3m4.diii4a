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

#include "dd_macrocheck.c"
#include "dd_minimal.h"
#include <stdlib.h> /* for exit() */
void dd_minimal_count_global_error(void) {}

int
main(void)
{
    void * base = 0;
    Dwarf_Unsigned count = 0;
    int basefailcount = 0;
    int isdwarf5=FALSE;

    /* Test 1 */
    add_macro_import(&base,TRUE,200,0,0);
    count = macro_count_recs(&base);
    if (count != 1) {
        printf("FAIL: expect count 1, got %" DW_PR_DUu "\n",count);
        ++failcount;
    }
    print_macrocheck_statistics("test1",&base,isdwarf5,2000);

    /* Test two */
    add_macro_area_len(&base,200,100);
    add_macro_import(&base,FALSE,350,0,0);
    add_macro_area_len(&base,350,100);
    count = macro_count_recs(&base);
    if (count != 2) {
        printf("FAIL: expect count 2, got %" DW_PR_DUu "\n",count);
        ++failcount;
    }
    print_macrocheck_statistics("test 2",&base,isdwarf5,2000);
    clear_macrocheck_statistics(&base);

    /* Test three */
    basefailcount = failcount;
    add_macro_import(&base,TRUE,0,0,0);
    add_macro_area_len(&base,0,1000);
    add_macro_import(&base,FALSE,2000,0,0);
    add_macro_area_len(&base,2000,100);
    mark_macro_offset_printed(&base,2000);
    add_macro_import(&base,FALSE,1000,0,0);
    add_macro_area_len(&base,1000,900);
    add_macro_import(&base,FALSE,1000,0,0);
    add_macro_area_len(&base,1000,900);
    count = macro_count_recs(&base);
    if (count != 3) {
        printf("FAIL: expect count 3, got %" DW_PR_DUu "\n",count);
        ++failcount;
    }
    printf("\n  Expect an ERROR about overlap with "
        "the end of section\n");
    print_macrocheck_statistics("test 3",&base,isdwarf5,2000);
    clear_macrocheck_statistics(&base);
    if ((basefailcount+1) != failcount) {
        printf("FAIL: Found no error in test 3 checking!\n");
        ++failcount;
    } else {
        failcount = basefailcount;
    }

    /* Test Four */
    basefailcount = failcount;
    add_macro_import(&base,TRUE,50,0,0);
    add_macro_import(&base,TRUE,50,0,0);
    add_macro_area_len(&base,50,50);
    add_macro_import(&base,FALSE,200,0,0);
    add_macro_import(&base,FALSE,50,0,0);
    add_macro_import(&base,FALSE,60,0,0);
    add_macro_area_len(&base,60,10);
    printf( "\n  Expect an ERROR about offset 50 having "
        "2 primaries\n");
    printf( "  and Expect an ERROR about offset 50 having 2\n"
        "  primaries"
        " and a secondary\n");
    printf( "  and Expect an ERROR about crazy overlap 60\n\n");
    print_macrocheck_statistics("test 4",&base,isdwarf5,2000);
    clear_macrocheck_statistics(&base);
    if ((basefailcount + 3) != failcount) {
        printf("FAIL: Found wrong errors in test 4 checking!\n");
    } else {
        failcount = basefailcount;
    }
    if (failcount > 0) {
        printf("FAIL macrocheck selftest\n");
        exit(EXIT_FAILURE);
    }
    printf("PASS macrocheck selftest\n");
    return 0;
}
