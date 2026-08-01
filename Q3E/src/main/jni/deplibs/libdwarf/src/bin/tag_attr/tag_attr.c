/*
Copyright (C) 2000-2005 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright (C) 2009-2012 SN Systems Ltd. All Rights Reserved.
Portions Copyright (C) 2009-2017 David Anderson. All Rights Reserved.

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

#include <stdlib.h> /* exit() */
#include <string.h> /* memset() */
#include <stdio.h> /* FILE decl for dd_esb.h, printf etc */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_common.h"
#include "dd_attr_form.h" /* for struct threekey_s */

#include "dd_esb.h"
#include "dd_tag_common.h"
#include "dd_getopt.h"
#include "dd_safe_strcpy.h"
#include "dd_minimal.h"

void dd_minimal_count_global_error(void) {}

Dwarf_Bool ellipsis = FALSE; /* So we can use dwarf_names.c */

/* Expected input format

0xffffffff
value of a tag
value of a standard attribute that follows that tag
...
0xffffffff
value of a tag
value of a standard attribute that follows that tag
...
0xffffffff
...

The generated tag_attr_combination_table
is used and generated quite differently
for standard than for extended tags.

For standard tags the generated table is indexed by
tag number. All the columns are bit flags.

For extended tags the generated table is indexed
(call it j) by 0 - N-1
and  [j][0] is the tag number and the rest of
the columns (1 - N-1) are
allowed attribute numbers.

*/

static const char *usage[] = {
    "Usage: tag_attr_build <options>",
    "    -i input-table-path",
    "    -o output-table-path",
    "    -s (Generate standard attribute table)",
    "    -e (Generate extended attribute table (common extensions))",
    ""
};

const char *program_name = 0;
char *input_name = 0;
char *output_name = 0;
int standard_flag = FALSE;
int extended_flag = FALSE;

/* process arguments */
static void
process_args(int argc, char *argv[])
{
    int c = 0;
    Dwarf_Bool usage_error = FALSE;

    program_name = argv[0];

    while ((c = dwgetopt(argc, argv, "i:o:se")) != EOF) {
        switch (c) {
        case 'i':
            input_name = dwoptarg;
            break;
        case 'o':
            output_name = dwoptarg;
            break;
        case 'e':
            extended_flag = TRUE;
            break;
        case 's':
            standard_flag = TRUE;
            break;
        default:
            usage_error = TRUE;
            break;
        }
    }

    if (usage_error || 1 == dwoptind || dwoptind != argc) {
        print_usage_message(usage);
        exit(EXIT_FAILURE);
    }
}
#if 0
/*  Two new naming routines May 2016.
    Instead of directly calling dwarf_get_*()
    When bad tag/attr numbers are presented we return
    a warning string through the pointer.
    The thought is that eventually someone will notice the error.
    It might, of course, be better to emit an error message
    and stop. */
static void
ta_get_AT_name(unsigned int attrnum,const char **nameout)
{
    int res = 0;

    res = dwarf_get_AT_name(attrnum,nameout);
    if (res == DW_DLV_OK) {
        return;
    }
    if (res == DW_DLV_NO_ENTRY) {
        *nameout = "<no name known for the attribute>";
        return;
    }
    *nameout = "<ERROR so no name known for the attribute>";
    return;
}
#endif

static void
ta_get_TAG_name(unsigned int tag,const char **nameout)
{
    int res = 0;

    res = dwarf_get_TAG_name(tag,nameout);
    if (res == DW_DLV_OK) {
        return;
    }
    if (res == DW_DLV_NO_ENTRY) {
        *nameout = "<no name known for the tag>";
        return;
    }
    *nameout = "<ERROR so no name known for the tag>";
    return;
}

#if 0
static unsigned maxrowused = 0;
static unsigned maxcolused = 0;

/*  The argument sizes are declared in tag_common.h, so
    the max usable is toprow-1 and topcol-1. */
static void
check_unused_combo(unsigned toprow,unsigned topcol)
{
    if ((toprow-1) !=  maxrowused) {
        printf("Providing for %u rows but used 0-%u\n",
            toprow,maxrowused);
        printf("Giving up\n");
        exit(EXIT_FAILURE);
    }
    if ((topcol-1) != maxcolused) {
        printf("Providing for %u cols but used 0-%u\n",
            topcol,maxcolused);
        printf("Giving up\n");
        exit(EXIT_FAILURE);
    }
}

static void
validate_row_col(const char *position,
    unsigned crow,
    unsigned ccol,
    unsigned maxrow,
    unsigned maxcol)
{
    if (crow >= ATTR_TABLE_ROW_MAXIMUM) {
        printf("error generating row in tag-attr array, %s "
            "current row: %u  size of static array decl: %u\n",
            position,crow, ATTR_TABLE_ROW_MAXIMUM);
        exit(EXIT_FAILURE);
    }
    if (crow >= maxrow) {
        printf("error generating row in tag-attr array, %s "
            "current row: %u  max allowed: %u\n",
            position,crow,maxrow-1);
        exit(EXIT_FAILURE);
    }
    if (ccol >= ATTR_TABLE_COLUMN_MAXIMUM) {
        printf("error generating column in tag-attr array, %s "
            "current col: %u  size of static array decl: %u\n",
            position,ccol, ATTR_TABLE_COLUMN_MAXIMUM);
        exit(EXIT_FAILURE);
    }
    if (ccol >= maxcol) {
        printf("error generating column in tag-attr array, %s "
            "current row: %u  max allowed: %u\n",
            position,ccol,maxcol-1);
        exit(EXIT_FAILURE);
    }
    if (crow > maxrowused) {
        maxrowused = crow;
    }
    if (ccol > maxcolused) {
        maxcolused = ccol;
    }
    return;
}
#endif

int
main(int argc, char **argv)
{
    unsigned int num = 0;
    int input_eof = 0;
#if 0
    unsigned u = 0;
    unsigned table_rows = 0;
    unsigned table_columns = 0;
    unsigned current_row = 0;
#endif
    FILE * fileInp = 0;
    FILE * fileOut = 0;
    unsigned int table_type = 0;
    const char *name = 0;
    const char *structname = 0;

    print_version_details(argv[0]);
    print_args(argc,argv);
    process_args(argc,argv);

    if (!input_name ) {
        fprintf(stderr,"Input name required, not supplied.\n");
        print_usage_message(usage);
        exit(EXIT_FAILURE);
    }
    fileInp = fopen(input_name,"r");
    if (!fileInp) {
        fprintf(stderr,"Invalid input filename,"
            " could not open '%s'\n",
            input_name);
        print_usage_message(usage);
        exit(EXIT_FAILURE);
    }

    if (!output_name ) {
        fprintf(stderr,"Output name required, not supplied.\n");
        print_usage_message(usage);
        exit(EXIT_FAILURE);
    }
    fileOut = fopen(output_name,"w");
    if (!fileOut) {
        fprintf(stderr,"Invalid output filename,"
            " could not open: '%s'\n",
            output_name);
        print_usage_message(usage);
        exit(EXIT_FAILURE);
    }
    if ((standard_flag && extended_flag) ||
        (!standard_flag && !extended_flag)) {
        fprintf(stderr,"Invalid table type\n");
        fprintf(stderr,"Choose -e  or -s .\n");
        print_usage_message(usage);
        exit(EXIT_FAILURE);
    }

    if (standard_flag) {
        structname = "dd_threekey_ta_std";
        table_type = AF_STD;
    } else {
        structname = "dd_threekey_ta_ext";
        table_type = AF_EXTEN;
    }
#if 0
    if (standard_flag) {
        table_rows = STD_ATTR_TABLE_ROWS;
        table_columns = STD_ATTR_TABLE_COLUMNS;
    } else {
        table_rows = EXT_ATTR_TABLE_ROWS;
        table_columns = EXT_ATTR_TABLE_COLS;
    }
#endif

    input_eof = read_value(&num,fileInp);       /* 0xffffffff */
    if (IS_EOF == input_eof) {
        bad_line_input("Empty input file");
    }
    if (num != MAGIC_TOKEN_VALUE) {
        bad_line_input("Expected 0xffffffff");
    }

    /*  Generate main header, regardless of contents */
    fprintf(fileOut,"/* Generated code, do not edit. */\n");
    fprintf(fileOut,"/* Generated for source version %s */\n",
        PACKAGE_VERSION);
    fprintf(fileOut,"\n/* BEGIN FILE */\n\n");
    fprintf(fileOut,"struct Three_Key_Entry_s %s [] = {\n",
        structname);

    while (!feof(stdin)) {
        unsigned int tag;
#if 0
        unsigned int curcol = 0;
#endif

        input_eof = read_value(&tag,fileInp);
        if (IS_EOF == input_eof) {
            /* Reached normal eof */
            break;
        }
#if 0
        {
            /*  In extended case, the row indexed by 0-N
                and column zero has the tag number. */
            if (current_row >= table_rows) {
                bad_line_input(
                    "too many extended table rows. Have %u max %u",
                    current_row,table_rows);
            }
            validate_row_col("Reading tag",current_row,0,
                table_rows,table_columns);
            tag_attr_combination_table[current_row][0] = tag;
        }
#endif
        input_eof = read_value(&num,fileInp);
        if (IS_EOF == input_eof) {
            bad_line_input("Not terminated correctly..");
        }
#if 0
        curcol = 1;
#endif
        ta_get_TAG_name(tag,&name);
        fprintf(fileOut,"/* 0x%02x - %-37s*/\n", tag, name);

        while (num != MAGIC_TOKEN_VALUE) {
#if 0
            struct esb_s msg_buf;

            esb_constructor(&msg_buf);
            {
                if (curcol >= table_columns) {
                    esb_append_printf_i(&msg_buf,
                        "too many attributes b: table incomplete "
                        "index %d ",curcol);
                    esb_append_printf_i(&msg_buf,
                        "cols %d.",table_columns);
                    bad_line_input(esb_get_string(&msg_buf));
                }
                validate_row_col("Setting attr col",
                    current_row,curcol,
                    table_rows,table_columns);
                tag_attr_combination_table[current_row][curcol] =
                    num;
                curcol++;

            }
            esb_destructor(&msg_buf);
#endif
            /* print a 3key */
            fprintf(fileOut,"{0x%04x,0x%04x,%u,%d,0,0},\n",
                (Dwarf_Half)tag,(Dwarf_Half)num,(Dwarf_Half)0,
                table_type);
            input_eof = read_value(&num,fileInp);
            if (IS_EOF == input_eof) {
                bad_line_input("Not terminated correctly.");
            }
        }
#if 0
        ++current_row;
#endif
    }
    fprintf(fileOut,"{0,0,0,0,0,0}};");
    fprintf(fileOut,"\n/* END FILE */\n");
    fclose(fileInp);
    fclose(fileOut);
    return 0;
}
#if 0
    if (standard_flag) {
        fprintf(fileOut,"#define ATTR_TREE_ROW_COUNT %d\n\n",
            table_rows);
        fprintf(fileOut,"#define ATTR_TREE_COLUMN_COUNT %d\n\n",
            table_columns);
        fprintf(fileOut,
            "static Dwarf_Half tag_attr_combination_table\n");
        fprintf(fileOut,
            "[ATTR_TREE_ROW_COUNT][ATTR_TREE_COLUMN_COUNT] = {\n");
    }
    else {
        fprintf(fileOut,"/* Common extensions */\n");
        fprintf(fileOut,"#define ATTR_TREE_EXT_ROW_COUNT %d\n\n",
            table_rows);
        fprintf(fileOut,"#define ATTR_TREE_EXT_COLUMN_COUNT %d\n\n",
            table_columns);
        fprintf(fileOut,
            "static Dwarf_Half tag_attr_combination_ext_table\n");
        fprintf(fileOut,
            "    [ATTR_TREE_EXT_ROW_COUNT]"
            "[ATTR_TREE_EXT_COLUMN_COUNT]"
            " = {\n");
    }

    for (u = 0; u < table_rows; u++) {
        unsigned j = 0;
        const char *name = 0;
        unsigned k = tag_attr_combination_table[u][0];
        ta_get_TAG_name(k,&name);
        fprintf(fileOut,"/* 0x%02x - %-37s*/\n",k,name);
        fprintf(fileOut,"    { ");
        for (j = 0; j < table_columns; ++j ) {
            if (j && j%5 == 0) {
                fprintf(fileOut,"\n        ");
            }
            fprintf(fileOut,"0x%08x,",
                tag_attr_combination_table[u][j]);
        }
        fprintf(fileOut,"},\n");
    }
    fprintf(fileOut,"{0,0,0,0,0,0}};");
    fprintf(fileOut,"\n/* END FILE */\n");
    fclose(fileInp);
    fclose(fileOut);
    return (0);
}
/* A fake so we can use dwarf_names.c */
void print_error (Dwarf_Debug dbg,
    const char * msg,
    int res,
    Dwarf_Error localerr)
{
    (void)dbg;
    (void)msg;
    (void)res;
    (void)localerr;
}
#endif
