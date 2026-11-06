/*
Copyright (c) 2025, David Anderson
All rights reserved.
Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the
following conditions are met:

    Redistributions of source code must retain the above
    copyright notice, this list of conditions and the following
    disclaimer.

    Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*  Printing the language name and version details
    for DWARF6. Using information from the
    standard, not information from any object file.
    See dwarfstd.org/languages-v6.html  */

#include <config.h>
#include <stdio.h> /* FILE decl for dd_esb.h */

#include "dwarf.h"
#include "libdwarf.h"
#include "dd_defined_types.h"
#include "libdwarf_private.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"

void
print_language_version_table(void)
{
    Dwarf_Unsigned i = 0;
    int res = 0;
    Dwarf_Unsigned language_name = 0;
    Dwarf_Unsigned language_version = 0;
    const char * v_scheme = 0;
    const char * v_name = 0;
    const char * language_name_string = 0;

    printf("\n");
    printf("DWARF6 Language Version Codes\n");
    printf("\n");
    printf("  []   Language                      Language          "
        "Version\n");
    printf("                                     Version           "
        "Scheme\n");

    for ( ; ; ++i) {
        res = dwarf_lvn_table_entry(i,
            &language_name,&language_version,
            &v_scheme, &v_name);
        if (res == DW_DLV_NO_ENTRY) {
            break;
        }
        res = dwarf_get_LNAME_name(
            (unsigned int)language_name,
            &language_name_string);
        if (res == DW_DLV_NO_ENTRY) {
            language_name_string = "<unknown>";
        }

        printf("  [%2" DW_PR_DUu "] ",i);

        printf("%-20s ",language_name_string);
        printf("(0x%04" DW_PR_DUx ") ",language_name);

        printf("%-8s ",v_name);
        printf("(%6" DW_PR_DUu ") ",language_version);

        printf("%-12s ",v_scheme);
        printf("\n");
    }
}
