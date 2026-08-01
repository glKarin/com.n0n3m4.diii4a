/*
Copyright (c) 2023, David Anderson
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

/* Invoke as
   test_setupsections [--count <loopiterations>]
*/

#include <config.h>

#include <stdio.h>  /* printf() */
#include <stdlib.h> /* exit() */
#include <string.h> /* strcmp() strlen() */
#include <stddef.h> /* offsetof */

#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dwarf_base_types.h"
#include "dwarf_die_deliv.h"
#include "dwarf_opaque.h"
#include "dwarf_secname_ck.h"
#include "dwarf_setup_sections.h"

static int errcount;

struct Dwarf_Debug_s dbgs;
Dwarf_Debug dbg = &dbgs;
struct basis_s {
    const char *name;
    unsigned    group_number;
    unsigned    secdataoff;
    int         duperr;
    int         emptyerr;
    int         havedwarf;
};
struct basis_s basis[] = {
{".debug_info",
DW_GROUPNUMBER_BASE,
offsetof(struct Dwarf_Debug_s,de_debug_info),
DW_DLE_DEBUG_INFO_DUPLICATE,DW_DLE_DEBUG_INFO_NULL,TRUE
},

{".debug_line",
DW_GROUPNUMBER_BASE,
offsetof(struct Dwarf_Debug_s,de_debug_line),
DW_DLE_DEBUG_LINE_DUPLICATE,0, FALSE
},
#if 0
{".debug_line.dwo",
DW_GROUPNUMBER_DWO,
offsetof(struct Dwarf_Debug_s,de_debug_line),
DW_DLE_DEBUG_LINE_DUPLICATE,0, TRUE
},
#endif

{".debug_pubtypes",
DW_GROUPNUMBER_BASE,
offsetof(struct Dwarf_Debug_s,de_debug_pubtypes),
/*13*/ DW_DLE_DEBUG_PUBTYPES_DUPLICATE,0, FALSE
},

{".debug_gnu_pubnames",
DW_GROUPNUMBER_BASE,
offsetof(struct Dwarf_Debug_s,de_debug_pubnames),
DW_DLE_DUPLICATE_GNU_DEBUG_PUBNAMES,0, FALSE
}
};

struct basis_s fails[] = {
{".debug_infox",
DW_GROUPNUMBER_BASE,
offsetof(struct Dwarf_Debug_s,de_debug_info),
DW_DLE_DEBUG_INFO_DUPLICATE,DW_DLE_DEBUG_INFO_NULL,TRUE
},

{".debug_",
DW_GROUPNUMBER_BASE,
offsetof(struct Dwarf_Debug_s,de_debug_line),
DW_DLE_DEBUG_LINE_DUPLICATE,0, FALSE
},
{".rel",
DW_GROUPNUMBER_BASE,
offsetof(struct Dwarf_Debug_s,de_debug_line),
DW_DLE_DEBUG_LINE_DUPLICATE,0, FALSE
},
{".rela",
DW_GROUPNUMBER_BASE,
offsetof(struct Dwarf_Debug_s,de_debug_line),
DW_DLE_DEBUG_LINE_DUPLICATE,0, FALSE
},
};

static void
test_adds(void)
{
    unsigned i = 0;
    int err = 0;
    unsigned testcount = sizeof(basis)/sizeof(struct basis_s);

    memset(dbg,0,sizeof(*dbg));
    for ( ; i < testcount; ++i) {
        struct basis_s *b = &basis[i];
        int res = 0;

        err = 0;
        res = _dwarf_enter_section_in_de_debug_sections_array(
            dbg,b->name,i,b->group_number,&err);
        if (res != DW_DLV_OK) {
            ++errcount;
            printf("FAIL enter index %u name %s\n",
                i,
                b->name);
        }
    }
}
static void
test_fails(void)
{
    unsigned i = 0;
    int err = 0;
    unsigned testcount = sizeof(fails)/sizeof(struct basis_s);

    memset(dbg,0,sizeof(*dbg));
    for ( ; i < testcount; ++i) {
        struct basis_s *b = &fails[i];
        int res = 0;

        err = 0;
        res = _dwarf_enter_section_in_de_debug_sections_array(
            dbg,b->name,i,b->group_number,&err);
        if (res == DW_DLV_OK) {
            ++errcount;
            printf("FAIL passed expected fail "
                "enter index %u name %s\n",
                i,
                b->name);
        }
    }
}

int main(int argc,char *argv[])
{
    int lim = 1;
    int i = 0;
    int op = 1;

    for ( ; op < argc; ++op) {
        char *opname = 0;
        opname = argv[op];
        if (!strcmp(opname,"--count")) {
            ++op;
            if (op < argc) {
                opname = argv[op];
                lim = atoi(opname);
            }
        }
    }

    for ( ; i < lim; ++i) {
        test_adds();
    }
    for ( ; i < lim; ++i) {
        test_fails();
    }
    if (errcount) {
        exit(EXIT_FAILURE);
    }
    exit(0);
}
