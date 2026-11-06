/*

  Copyright (C) 2015-2015 David Anderson. All Rights Reserved.

  This program is free software; you can redistribute it
  and/or modify it under the terms of version 2.1 of the
  GNU Lesser General Public License as published by the Free
  Software Foundation.

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

  You should have received a copy of the GNU Lesser General
  Public License along with this program; if not, write the
  Free Software Foundation, Inc., 51 Franklin Street - Fifth
  Floor, Boston MA 02110-1301, USA.

*/

#include <config.h>

#include <stddef.h> /* size_t */
#include <stdio.h>  /* printf() */
#include <stdlib.h> /* exit() */
#include <string.h> /* memcpy() memset() */

#ifdef HAVE_STDINT_H
#include <stdint.h> /* uintptr_t */
#endif /* HAVE_STDINT_H */

#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dwarf_base_types.h"
#include "dwarf_opaque.h"
#include "dwarf_tsearch.h"
#include "dwarf_tied_decls.h"

struct test_data_s {
    const char action;
    unsigned long val;
}  testdata[] = {
{'a', 0x33c8},
{'a', 0x34d8},
{'a', 0x35c8},
{'a', 0x3640},
{'a', 0x3820},
{'a', 0x38d0},
{'a', 0x3958},
{'a', 0x39e8},
{'a', 0x3a78},
{'a', 0x3b08},
{'a', 0x3b98},
{'a', 0x3c28},
{'a', 0x3cb8},
{'d', 0x3c28},
{'a', 0x3d48},
{'d', 0x3cb8},
{'a', 0x3dd8},
{'d', 0x3d48},
{'a', 0x3e68},
{'d', 0x3dd8},
{'a', 0x3ef8},
{'a', 0x3f88},
{'d', 0x3e68},
{'a', 0x4018},
{'d', 0x3ef8},
{0,0}
};

/* We don't test this here, referenced from dwarf_tied.c. */
int
_dwarf_next_cu_header_internal(
    Dwarf_Debug dbg,
    Dwarf_Bool is_info,
    Dwarf_Die  *cu_die_output,
    Dwarf_Unsigned * cu_header_length,
    Dwarf_Half * version_stamp,
    Dwarf_Unsigned * abbrev_offset,
    Dwarf_Half * address_size,
    Dwarf_Half * offset_size,
    Dwarf_Half * extension_size,
    Dwarf_Sig8 * signature,
    Dwarf_Bool * has_signature,
    Dwarf_Unsigned *typeoffset,
    Dwarf_Unsigned * next_cu_offset,
    Dwarf_Half     * header_cu_type,
    Dwarf_Error * error)
{
    (void)dbg;
    (void)is_info;
    (void)cu_die_output;
    (void)cu_header_length;
    (void)version_stamp;
    (void)abbrev_offset;
    (void)address_size;
    (void)offset_size;
    (void)extension_size;
    (void)signature;
    (void)has_signature;
    (void)typeoffset;
    (void)next_cu_offset;
    (void)header_cu_type;;
    (void)error;
    return DW_DLV_NO_ENTRY;
}

static struct Dwarf_Tied_Entry_s *
makeentry(Dwarf_Unsigned instance, unsigned ct)
{
    Dwarf_Sig8 s8;
    Dwarf_CU_Context context = 0;
    struct Dwarf_Tied_Entry_s * entry = 0;

    memset(&s8,0,sizeof(s8));
    /* Silly, but just a test...*/
    memcpy(&s8,&instance,sizeof(instance));
    context = (Dwarf_CU_Context)(uintptr_t)instance;

    entry = (struct Dwarf_Tied_Entry_s *)
        _dwarf_tied_make_entry(&s8,context);
    if (!entry) {
        printf("Out of memory in test! %u\n",ct);
        exit(EXIT_FAILURE);
    }
    return entry;
}

static int
insone(void**tree,Dwarf_Unsigned instance, unsigned ct)
{
    struct Dwarf_Tied_Entry_s * entry = 0;
    void *retval = 0;

    entry = makeentry(instance, ct);
    retval = dwarf_tsearch(entry,tree, _dwarf_tied_compare_function);

    if (!retval) {
        printf("FAIL ENOMEM in search on rec %u adr  0x%lu,"
            " error in insone\n",
            ct,(unsigned long)instance);
        exit(EXIT_FAILURE);
    } else {
        struct Dwarf_Tied_Entry_s *re = 0;
        re = *(struct Dwarf_Tied_Entry_s **)retval;
        if (re != entry) {
            /* Found existing, error. */
            printf("insertone rec %u addr 0x%lu found record"
                " preexisting, error\n",
                ct,(unsigned long)instance);
            _dwarf_tied_destroy_free_node(entry);
            exit(EXIT_FAILURE);
        } else {
            /* inserted new entry, make sure present. */
            struct Dwarf_Tied_Entry_s * entry2 = 0;
            entry2 = makeentry(instance,ct);
            retval = dwarf_tfind(entry2,tree,
                _dwarf_tied_compare_function);
            _dwarf_tied_destroy_free_node(entry2);
            if (!retval) {
                printf("insertonebypointer record %d addr 0x%lu "
                    "failed to add as desired,"
                    " error\n",
                    ct,(unsigned long)instance);
                exit(EXIT_FAILURE);
            }
        }
    }
    return 0;
}

static int
delone(void**tree,Dwarf_Unsigned instance, unsigned ct)
{
    struct Dwarf_Tied_Entry_s * entry = 0;
    void *r = 0;

    entry = makeentry(instance, ct);
    r = dwarf_tfind(entry,(void *const*)tree,
        _dwarf_tied_compare_function);
    if (r) {
        struct Dwarf_Tied_Entry_s *re3 =
            *(struct Dwarf_Tied_Entry_s **)r;
        re3 = *(struct Dwarf_Tied_Entry_s **)r;
        dwarf_tdelete(entry,tree,_dwarf_tied_compare_function);
        _dwarf_tied_destroy_free_node(entry);
        _dwarf_tied_destroy_free_node(re3);
    } else {
        printf("delone could not find rec %u ! error! addr"
            " 0x%lx\n",
            ct,(unsigned long)instance);
        exit(EXIT_FAILURE) ;
    }
    return 0;

}

int main(int argc, char *argv[])
{
    void *tied_data = 0;
    unsigned u = 0;

    INITTREE(tied_data,_dwarf_tied_data_hashfunc);
    for ( ; testdata[u].action; ++u) {
        char action = testdata[u].action;
        Dwarf_Unsigned v = testdata[u].val;
        if (action == 'a') {
            insone(&tied_data,v,u);
        } else if (action == 'd') {
            delone(&tied_data,v,u);
        } else  {
            printf("FAIL testtied on action %u, "
                "not a or d\n",action);
            exit(EXIT_FAILURE);
        }
    }
    printf("PASS tsearch works for Dwarf_Tied_Entry_s.\n");
    return 0;

    (void)argc;
    (void)argv;
}
