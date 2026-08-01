/*
Copyright (C) 2020 David Anderson. All Rights Reserved.

    This program is free software; you can redistribute it
    and/or modify it under the terms of version 2 of the GNU
    General Public License as published by the Free Software
    Foundation.

    This program is distributed in the hope that it would
    be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A
    PARTICULAR PURPOSE.

    Further, this software is distributed without any warranty
    that it is free of the rightful claim of any third person
    regarding infringement or the like.  Any license provided
    herein, whether implied or otherwise, applies only to
    this software file.  Patent licenses, if any, provided
    herein do not apply to combinations of this program with
    other software, or any other product whatsoever.

    You should have received a copy of the GNU General Public
    License along with this program; if not, write the Free
    Software Foundation, Inc., 51 Franklin Street - Fifth
    Floor, Boston MA 02110-1301, USA.
*/

/*  DWARF5 has the new .debug_rnglists section.
    Here we print that data.
    The raw printing covers all the content of the
    section but without relating it to any
    compilation unit.

    Printing the actual address means printing
    with the actual DIEs on hand.
*/

#include <config.h>
#include <stdio.h> /* FILE decl for dd_esb.h, printf etc */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_esb.h"
#include "dd_esb_using_functions.h"
#include "dd_sanitized.h"

static void
print_sec_name(Dwarf_Debug dbg)
{
    struct esb_s truename;
    char buf[DWARF_SECNAME_BUFFER_SIZE];

    esb_constructor_fixed(&truename,buf,sizeof(buf));
    get_true_section_name(dbg,".debug_rnglists",
        &truename,TRUE);
    printf("\n%s\n\n",sanitized(esb_get_string(&truename)));
    esb_destructor(&truename);
}

static int
print_offset_entry_table(Dwarf_Debug dbg,
    Dwarf_Unsigned contextnum,
    Dwarf_Unsigned offset_entry_count,
    Dwarf_Unsigned offset_of_offset_array,
    Dwarf_Unsigned offset_of_header,
    Dwarf_Unsigned offset_size,
    Dwarf_Error *error)
{
    Dwarf_Unsigned e = 0;
    unsigned colmax = 2;
    unsigned col = 0;
    int res = 0;
    int hasnewline = TRUE;
    Dwarf_Unsigned goff = 0;
    Dwarf_Unsigned loff = 0;

    (void)offset_of_header;

    loff = 0;
    goff = offset_of_offset_array;
    for ( ; e < offset_entry_count; ++e) {
        Dwarf_Unsigned value = 0;

        if (e == 0) {

            printf("\n   Location Offset Table at 0x%" DW_PR_XZEROS
                DW_PR_DUx  "\n",offset_of_offset_array);
            printf("   [goff][loff][index]\n");
        }
        hasnewline = FALSE;
        res = dwarf_get_rnglist_offset_index_value(dbg,
            contextnum,e,&value,0,error);
        if (res != DW_DLV_OK) {
            return res;
        }
        if (col == 0) {
            printf("   [0x%" DW_PR_XZEROS DW_PR_DUx "]",
                goff);
            printf("[0x%" DW_PR_XZEROS DW_PR_DUx "]",
                loff);
            printf("[%2" DW_PR_DUu "]",e);
        }
        loff += offset_size;
        goff += offset_size;
        printf(" 0x%" DW_PR_XZEROS DW_PR_DUx, value);
        printf("(0x%" DW_PR_XZEROS DW_PR_DUx ")", value +
            offset_of_offset_array);
        col++;
        if (col == colmax) {
            printf("\n");
            hasnewline = TRUE;
            col = 0;
        }
    }
    if (!hasnewline) {
        printf("\n");
    }
    return DW_DLV_OK;
}

/* For printing the raw rangelist data from .debug_rnglists */
static int
print_single_rle(Dwarf_Unsigned lineoffset,
    Dwarf_Unsigned code,
    Dwarf_Unsigned v1,
    Dwarf_Unsigned v2,
    Dwarf_Unsigned entrylen,
    Dwarf_Unsigned goff)
{
    int res = DW_DLV_OK;

    const char *name = "";
    struct esb_s m;

    esb_constructor(&m);
    res = dwarf_get_RLE_name((unsigned int)code,&name);
    if (res != DW_DLV_OK) {
        /* ASSERT: res == DW_DLV_NO_ENTRY, see dwarf_names.c */
        esb_append_printf_u(&m, "<ERROR: rle code 0x%" DW_PR_DUx
            "unknown>",code);
    } else {
        esb_append(&m,name);
    }
    printf("   ");
    printf("[0x%" DW_PR_XZEROS DW_PR_DUx "]",goff);
    printf("[0x%" DW_PR_XZEROS DW_PR_DUx "] %-20s",
        lineoffset,esb_get_string(&m));
    switch(code) {
    case DW_RLE_end_of_list:
        printf("           ");
        printf("           ");
        break;

    case DW_RLE_base_addressx:{
        printf(" 0x%" DW_PR_XZEROS DW_PR_DUx ,v1);
        printf("           ");
        }
        break;
    case DW_RLE_startx_endx: {
        printf(
            " 0x%" DW_PR_XZEROS DW_PR_DUx
            " 0x%" DW_PR_XZEROS DW_PR_DUx ,v1,v2);
        }
        break;
    case DW_RLE_startx_length: {
        printf(
            " 0x%" DW_PR_XZEROS DW_PR_DUx
            " 0x%" DW_PR_XZEROS DW_PR_DUx ,v1,v2);
        }
        break;

    case DW_RLE_offset_pair: {
        printf(
            " 0x%" DW_PR_XZEROS DW_PR_DUx
            " 0x%" DW_PR_XZEROS DW_PR_DUx ,v1,v2);
        }
        break;
    case DW_RLE_base_address: {
        printf(
            " 0x%" DW_PR_XZEROS DW_PR_DUx ,v1);
        printf("           ");
        }
        break;
    case DW_RLE_start_end: {
        printf(
            " 0x%" DW_PR_XZEROS DW_PR_DUx
            " 0x%" DW_PR_XZEROS DW_PR_DUx ,v1,v2);
        }
        break;
    case DW_RLE_start_length: {
        printf(
            " 0x%" DW_PR_XZEROS DW_PR_DUx
            " 0x%" DW_PR_XZEROS DW_PR_DUx ,v1,v2);
        }
        break;
    default:
        printf(" ERROR: Unknown RLE code in .debug_rnglists. %s\n",
            esb_get_string(&m));
        simple_err_return_msg_either_action(res,
            esb_get_string(&m));
        break;
    }
    esb_destructor(&m);
    printf(" %" DW_PR_DUu,entrylen);
    printf("\n");
    return res;
}

/* For printing raw rangelist data as found in .debug_rnglists */
static int
print_entire_rangeslist(Dwarf_Debug dbg,
    Dwarf_Unsigned contextnumber,
    Dwarf_Unsigned offset_of_first_range,
    Dwarf_Unsigned offset_past_last_rangeentry,
    Dwarf_Error *error)
{
    /*  These offsets are rnglists section global offsets,
        not rnglist context local offsets. */
    Dwarf_Unsigned curoffset = offset_of_first_range;
    Dwarf_Unsigned endoffset = offset_past_last_rangeentry;
    int res = 0;
    Dwarf_Unsigned ct = 0;
    int title_printed = FALSE;
    Dwarf_Unsigned goff = offset_of_first_range;

    for ( ; curoffset < endoffset; ++ct ) {
        unsigned entrylen = 0;
        unsigned code = 0;
        Dwarf_Unsigned v1 = 0;
        Dwarf_Unsigned v2 = 0;
        if (!ct) {
            printf("   RangeEntries (raw)\n");
        }
        res = dwarf_get_rnglist_rle(dbg,contextnumber,
            curoffset,endoffset,
            &entrylen,
            &code,&v1,&v2,error);
        if (res != DW_DLV_OK) {
            return res;
        }

        if (!title_printed) {
            title_printed = TRUE;
            printf("   [goff      ][loff      ] "
                "entryname            "
                "val1       val2 entrylen\n");
        }
        print_single_rle(curoffset,
            code,v1,v2,entrylen,goff);
        curoffset += entrylen;
        goff += entrylen;
        if (curoffset > endoffset) {
            struct esb_s m;

            esb_constructor(&m);
            esb_append_printf_u(&m, "DW_DLE_USER_DECLARED_ERROR: "
                "final RLE in "
                ".debug_rnglists runs past end of its area "
                "so current offset 0x%" DW_PR_DUx,curoffset);

            esb_append_printf_u(&m," exceeds context 1-past-end"
                " offset of 0x%" DW_PR_DUx ".",endoffset);
            dwarf_error_creation(dbg,error,
                esb_get_string(&m));
            esb_destructor(&m);
            return DW_DLV_ERROR;
        }
    }
    return DW_DLV_OK;
}

/* For printing the raw rangelist data from .debug_rnglists */
int
print_raw_all_rnglists(Dwarf_Debug dbg,
    Dwarf_Error *error)
{
    int res = 0;
    Dwarf_Unsigned count = 0;
    Dwarf_Unsigned i = 0;

    res = dwarf_load_rnglists(dbg,&count,error);
    if (res != DW_DLV_OK) {
        return res;
    }
    print_sec_name(dbg);

    printf(" Number of rnglists contexts:  %" DW_PR_DUu "\n",
        count);
    for (i = 0; i < count ; ++i) {
        Dwarf_Unsigned header_offset = 0;
        Dwarf_Small   offset_size = 0;
        Dwarf_Small   extension_size = 0;
        unsigned      version = 0; /* 5 */
        Dwarf_Small   address_size = 0;
        Dwarf_Small   segment_selector_size = 0;
        Dwarf_Unsigned offset_entry_count = 0;
        Dwarf_Unsigned offset_of_offset_array = 0;
        Dwarf_Unsigned offset_of_first_rangeentry = 0;
        Dwarf_Unsigned offset_past_last_rangeentry = 0;

        res = dwarf_get_rnglist_context_basics(dbg,i,
            &header_offset,&offset_size,&extension_size,
            &version,&address_size,&segment_selector_size,
            &offset_entry_count,&offset_of_offset_array,
            &offset_of_first_rangeentry,
            &offset_past_last_rangeentry,error);
        if (res != DW_DLV_OK) {
            struct esb_s m;

            esb_constructor(&m);
            esb_append_printf_u(&m,"ERROR: Getting debug_rnglists "
                "entry %u we unexpectedly stop early.",i);
            simple_err_return_msg_either_action(res,
                esb_get_string(&m));
            esb_destructor(&m);
            return res;
        }
        printf("  Context number         : %3" DW_PR_DUu "\n",i);
        printf("   Version               : %3u\n",version);
        printf("   address size          : %3u\n",address_size);
        printf("   offset size           : %3u\n",offset_size);
        if (glflags.verbose) {
            printf("   extension size        : %3u\n",extension_size);
        }
        printf("   segment selector size : %3u\n",
            segment_selector_size);
        printf("   offset entry count    : %3" DW_PR_DUu "\n",
            offset_entry_count);
        printf("   context size in bytes : %3" DW_PR_DUu "\n",
            offset_past_last_rangeentry - header_offset);
        {
            printf("   Offset in section     : 0x%"
                DW_PR_XZEROS DW_PR_DUx"\n",
                header_offset);
            if (!offset_entry_count) {
                printf("   Offset of first range : 0x%"
                    DW_PR_XZEROS DW_PR_DUx"\n",
                    offset_of_offset_array);
            } else {
                printf("   Offset of offsettable : 0x%"
                    DW_PR_XZEROS DW_PR_DUx"\n",
                    offset_of_offset_array);
                printf("   Offsetof first range  : 0x%"
                    DW_PR_XZEROS DW_PR_DUx"\n",
                    offset_of_first_rangeentry);
            }
            printf("   Offset past ranges    : 0x%"
                DW_PR_XZEROS DW_PR_DUx"\n",
                offset_past_last_rangeentry);
        }
        if (offset_entry_count) {
            res = print_offset_entry_table(dbg,i,
                offset_entry_count,
                offset_of_offset_array,
                header_offset,
                offset_size,
                error);
            if (res == DW_DLV_ERROR) {
                return res;
            }
        }
        if ((offset_of_first_rangeentry+1) <
            offset_past_last_rangeentry) {
            res = print_entire_rangeslist(dbg,i,
                offset_of_first_rangeentry,
                offset_past_last_rangeentry,
                error);
            if (res != DW_DLV_OK) {
                return res;
            }
        }
    }
    return DW_DLV_OK;
}
