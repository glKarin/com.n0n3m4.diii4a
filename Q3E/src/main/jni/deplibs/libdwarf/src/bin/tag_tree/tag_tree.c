/*
Copyright (C) 2000-2005 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright 2009-2012 SN Systems Ltd. All rights reserved.
Portions Copyright 2009-2017 David Anderson. All rights reserved.

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

#include <ctype.h>  /* isspace() */
#include <stdarg.h> /* va_end() va_list va_start() */
#include <stdio.h>  /* FILE stderr fprintf() vfprintf() */
#include <stdlib.h> /* exit() */
#include <string.h> /* strlen() strncmp() */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_common.h"
#include "dd_tag_common.h"
#include "dd_attr_form.h"  /* threekey struct */
#include "dd_getopt.h"
#include "dd_safe_strcpy.h"
#include "dd_minimal.h"

void dd_minimal_count_global_error(void) {}

const char * program_name;

Dwarf_Bool ellipsis = FALSE; /* So we can use dwarf_names.c */

/* Expected input format

0xffffffff
value of a tag
value of a standard tag that may be a child ofthat tag
...
0xffffffff
value of a tag
value of a standard tag that may be a child ofthat tag
...
0xffffffff
...

No commentary allowed, no symbols, just numbers.
Blank lines are allowed and are dropped.

*/

static const char *usage[] = {
    "Usage: tag_tree_build <options>",
    "options:\t-t\tGenerate Tags table",
    "    -i Input-file-path",
    "    -o Output-table-path",
    "    -e   (Want Extended table (common extensions))",
    "    -s   (Want Standard table)",
    ""
};

static char *input_name = 0;
static char *output_name = 0;
int extended_flag = FALSE;
int standard_flag = FALSE;
const char * structname = 0;

static void
process_args(int argc, char *argv[])
{
    int c = 0;
    Dwarf_Bool usage_error = FALSE;

    program_name = argv[0];

    while ((c = dwgetopt(argc, argv, "i:o:es")) != EOF) {
        switch (c) {
        case 'i':
            input_name = (char *)strdup(dwoptarg);
            break;
        case 'o':
            output_name = (char *)strdup(dwoptarg);
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
/*  New naming routine May 2016.
    Instead of directly calling dwarf_get_*()
    When bad tag/attr numbers are presented we return
    a warning string through the pointer.
    The thought is that eventually someone will notice the error.
    It might, of course, be better to emit an error message
    and stop. */
static void
ta_get_TAG_name(unsigned int tagnum,const char **nameout)
{
    int res = 0;

    res = dwarf_get_TAG_name(tagnum,nameout);
    if (res == DW_DLV_OK) {
        return;
    }
    if (res == DW_DLV_NO_ENTRY) {
        *nameout = "<no name known for the TAG>";
        return;
    }
    *nameout = "<ERROR so no name known for the TAG>";
    return;
}

int
main(int argc, char **argv)
{
    unsigned int num = 0;
    int input_eof = 0;
    FILE *fileInp = 0;
    FILE *fileOut = 0;
    unsigned int table_type = 0;

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
        table_type = AF_STD;
        structname = "dd_threekey_tt_std";
    } else {
        table_type = AF_EXTEN;
        structname = "dd_threekey_tt_ext";
    }
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
        PACKAGE_VERSION );
    fprintf(fileOut,"\n/* BEGIN FILE */\n\n");
    fprintf(fileOut,"struct Three_Key_Entry_s %s [] = {\n",
        structname);

    while (!feof(stdin)) {
        unsigned int tag = 0;
        const char *name = 0;

        input_eof = read_value(&tag,fileInp);
        if (IS_EOF == input_eof) {
            /* Reached normal eof */
            break;
        }
        input_eof = read_value(&num,fileInp);
        if (IS_EOF == input_eof) {
            bad_line_input("Not terminated correctly..");
        }
        ta_get_TAG_name(tag,&name);
        fprintf(fileOut,"/* 0x%02x - %-37s*/\n", tag, name);

        while (num != MAGIC_TOKEN_VALUE) {
            /* print a 3key */
            fprintf(fileOut,"{0x%04x,0x%04x,%u,%d,0,0},\n",
                (Dwarf_Half)tag,(Dwarf_Half)num,(Dwarf_Half)0,
                table_type);
            input_eof = read_value(&num,fileInp);
            if (IS_EOF == input_eof) {
                bad_line_input("Not terminated correctly.");
            }
        }
    }
    fprintf(fileOut,"{0,0,0,0,0,0}};");
    fprintf(fileOut,"\n/* END FILE */\n");
    fclose(fileInp);
    fclose(fileOut);
    return (0);
}

/* A fake so we can use dwarf_names.c */
void print_error (Dwarf_Debug dbg,
    const char *msg,
    int res,
    Dwarf_Error localerr)
{
    (void)dbg;
    (void)msg;
    (void)res;
    (void)localerr;;
}
