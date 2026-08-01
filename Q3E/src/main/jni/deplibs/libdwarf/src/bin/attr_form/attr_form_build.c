/*
  Copyright (C) 2021 David Anderson. All Rights Reserved.

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
#include <stdio.h> /* FILE decl for dd_esb.h, printf etc */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_common.h"
#include "dd_esb.h"
#include "dd_tag_common.h"
#include "dd_getopt.h"
#include "dd_tsearchbal.h"
#include "dd_attr_form.h"
#include "dd_safe_strcpy.h"
#include "dd_minimal.h"
/* works around ref to this via dd_getopt */
void dd_minimal_count_global_error(void) {}

Dwarf_Bool ellipsis = FALSE; /* So we can use dwarf_names.c */

/* Expected input format

0xffffffff
DW_AT_something (a number as seen here)
name of a form-class enum entry
...
0xffffffff
DW_AT_something_else (a number as seen here)
name of a form-class enum entry
...
0xffffffff
...

We generate a text list of numbers as a C header file.
The array there is used
by dwarfdump at runtime if attr/form checking
is requested.
The file is named dwarfump/dwarfdump-af-table.h and is intended
to be included in exactly one place in dwarfdump source.

The list is N entries (N is not limited) of
    attribute#  formclass#   std1/extended2 flag
followed by a space and the # and then names.

For example:
{0x02,10, 1},  DW_AT_location, DW_FORM_CLASS_REFERENCE, Std
with the names in a C comment block (which we do not
show quite right here). See dwarfdump/dwarfdump-af-table.h

The Standard vs Extended table indication is a rough indication.
dwarfdump will know what are extension ATtributes and
extension FORMs by the valuing being at or above
the formal AT DW_AT_lo_user and the FORM being above 0x2c
(ie the highest defined by any DWARF standard, lacking
a DW_FORM_lo_user value)

Lines beginning with a # character are ignored
by the code in dwarfdump reading this output.
Any lines commented with C comments are stripped
by the initial C pre-processor invocation.

*/

#define AF_STANDARD 1
#define AF_EXTENDED 2

static const char *usage[] = {
    "Usage: attr_form_build <options>",
    "    -i input-table-path",
    "    -o output-table-path",
    "    -s (Generate standard attr-formclass table)",
    "    -e (Generate extended attr-formclass table ",
    "       common extensions))",
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

int
main(int argc, char **argv)
{
    unsigned int num = 0;
    int         input_eof = 0;
    FILE       *fileInp = 0;
    FILE       *fileOut = 0;
    unsigned    table_type = 0;
    const char *structname = 0;
    const char *macroname = 0;

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

    input_eof = read_value(&num,fileInp);       /* 0xffffffff */
    if (IS_EOF == input_eof) {
        bad_line_input("Empty input file");
    }
    if (num != MAGIC_TOKEN_VALUE) {
        bad_line_input("Expected 0xffffffff");
    }
    if (standard_flag) {
        table_type = AF_STD;
        structname = "dd_threekey_af_table_std";
        macroname = "DWARFDUMP_AF_TABLE_STD_H";
    } else {
        table_type = AF_EXTEN;
        structname = "dd_threekey_af_table_ext";
        macroname = "DWARFDUMP_AF_TABLE_EXT_H";
    }

    fprintf(fileOut,"/* Generated table, do not edit. */\n");
    fprintf(fileOut,"/* Generated for source version %s */\n",
        PACKAGE_VERSION);
    fprintf(fileOut,"\n");
    fprintf(fileOut,"%s%s\n",
        "#ifndef ",macroname);
    fprintf(fileOut,"%s%s\n",
        "#define ",macroname);
    fprintf(fileOut,"\n");

    fprintf(fileOut,"%s\n",
        "#ifdef __cplusplus");
    fprintf(fileOut,"%s\n", "extern \"C\" {");
    fprintf(fileOut,"%s\n",
        "#endif /* __cplusplus */");
    fprintf(fileOut,"struct Three_Key_Entry_s %s [] = {\n",
        structname);

    while (!feof(stdin)) {
        int res = 0;
        unsigned int attr = 0;
        const char * attrname = 0;

        input_eof = read_value(&attr,fileInp);
        if (IS_EOF == input_eof) {
            /* Reached normal eof */
            break;
        }
        res = dwarf_get_AT_name(attr,&attrname);
        if (res != DW_DLV_OK) {
            printf("Unknown attribute number of 0x%x,"
                " Giving up\n",attr);
            exit(EXIT_FAILURE);
        }
        fprintf(fileOut,"/* 0x%04x %s */\n",attr,attrname);
        input_eof = read_value(&num,fileInp);
        if (IS_EOF == input_eof) {
            bad_line_input("Not terminated correctly.");
        }

        while (num != MAGIC_TOKEN_VALUE) {
            fprintf(fileOut,"{0x%04x,0x%04x,%u,%d,0,0},\n",
                (Dwarf_Half)attr,(Dwarf_Half)num,(Dwarf_Half)0,
                table_type);
            input_eof = read_value(&num,fileInp);
            if (IS_EOF == input_eof) {
                bad_line_input("Not terminated correctly.");
            }
        }
    }
    fprintf(fileOut,"{0,0,0,0,0,0}};");
    fprintf(fileOut,"\n/* END FILE */\n");
    fprintf(fileOut,"%s\n",
        "#ifdef __cplusplus");
    fprintf(fileOut,"%s\n", "}");
    fprintf(fileOut,"%s\n",
        "#endif /* __cplusplus */");
    fprintf(fileOut,"%s%s%s\n",
        "#endif /* ",
        macroname," */");
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
