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

/*  DWARF5 has the new .debug_loclists section.
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

#if 0 /* debugging */
static void
dump_bytes(const char *msg,Dwarf_Small * start, long len)
{
    Dwarf_Small *end = start + len;
    Dwarf_Small *cur = start;
    printf("%s (0x%lx) ",msg,(unsigned long)start);
    for (; cur < end; cur++) {
        printf("%02x", *cur);
    }
    printf("\n");
}
#endif /* 0 */

static void
print_sec_name(Dwarf_Debug dbg)
{
    struct esb_s truename;
    char buf[DWARF_SECNAME_BUFFER_SIZE];

    esb_constructor_fixed(&truename,buf,sizeof(buf));
    get_true_section_name(dbg,".debug_loclists",
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
    Dwarf_Unsigned loff = offset_of_offset_array;
    Dwarf_Unsigned goff = offset_of_header + loff;

    for ( ; e < offset_entry_count; ++e) {
        Dwarf_Unsigned value = 0;

        if (e == 0) {
            printf("   Location Offset Table :\n");
            printf("   Location Offset Table at 0x%" DW_PR_XZEROS
                DW_PR_DUx  "\n",offset_of_offset_array);
            printf("   (Added 0x%"
                DW_PR_DUx  " to value for actual offsets)\n",
                offset_of_offset_array);
            printf("   [goff][loff][index]\n");
        }
        hasnewline = FALSE;
        res = dwarf_get_loclist_offset_index_value(dbg,
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
        printf(" 0x%" DW_PR_XZEROS DW_PR_DUx, value);
        printf("(0x%" DW_PR_XZEROS DW_PR_DUx ")",
            value+offset_of_offset_array);
        loff += offset_size;
        goff += offset_size;
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

static void
print_opsbytes(Dwarf_Unsigned expr_ops_blocklen,
    Dwarf_Small* expr_ops)
{
    Dwarf_Unsigned i = 0;

    if (!expr_ops_blocklen) {
        return;
    }
    printf(" opsbytes:");
    for ( ; i < expr_ops_blocklen; ++i ) {
        Dwarf_Small *b =  expr_ops+i;
        printf(" %02x", *b);
    }
    printf(" ");
}

/*  Print single raw lle */
static int
print_single_lle(Dwarf_Unsigned lineoffset,
    Dwarf_Unsigned code,
    Dwarf_Unsigned v1,
    Dwarf_Unsigned v2,
    Dwarf_Unsigned expr_ops_blocklen,
    Dwarf_Small    *expr_ops,
    Dwarf_Unsigned entrylen,
    Dwarf_Unsigned goff)
{
    int res = DW_DLV_OK;

    const char *name = "";
    struct esb_s m;

    esb_constructor(&m);
    res = dwarf_get_LLE_name((unsigned int)code,&name);
    if (res != DW_DLV_OK) {
        /* ASSERT: res == DW_DLV_NO_ENTRY, see dwarf_names.c */
        esb_append_printf_u(&m, "<ERROR: lle code 0x%" DW_PR_DUx
            "unknown>",code);
    } else {
        esb_append(&m,name);
    }
    printf("   ");
    printf("[0x%" DW_PR_XZEROS DW_PR_DUx "]",goff);
    printf("[0x%" DW_PR_XZEROS DW_PR_DUx "] %-20s",
        lineoffset,esb_get_string(&m));
    switch(code) {
    case DW_LLE_end_of_list:
        printf("           ");
        printf("           ");
        break;
    case DW_LLE_base_addressx:
        printf(" 0x%" DW_PR_XZEROS DW_PR_DUx ,v1);
        printf("           ");
        break;
    case DW_LLE_startx_endx:
        printf(
            " 0x%" DW_PR_XZEROS DW_PR_DUx
            " 0x%" DW_PR_XZEROS DW_PR_DUx ,v1,v2);
        break;
    case DW_LLE_startx_length:
        printf(
            " 0x%" DW_PR_XZEROS DW_PR_DUx
            " 0x%" DW_PR_XZEROS DW_PR_DUx ,v1,v2);
        break;
    case DW_LLE_offset_pair:
        printf(
            " 0x%" DW_PR_XZEROS DW_PR_DUx
            " 0x%" DW_PR_XZEROS DW_PR_DUx ,v1,v2);
        break;
    case DW_LLE_default_location:
        printf(
            " 0x%" DW_PR_XZEROS DW_PR_DUx
            " 0x%" DW_PR_XZEROS DW_PR_DUx ,v1,v2);
        break;
    case DW_LLE_base_address:
        printf(
            " 0x%" DW_PR_XZEROS DW_PR_DUx ,v1);
        printf("           ");
        break;
    case DW_LLE_start_end:
        printf(
            " 0x%" DW_PR_XZEROS DW_PR_DUx
            " 0x%" DW_PR_XZEROS DW_PR_DUx ,v1,v2);
        break;
    case DW_LLE_start_length:
        printf(
            " 0x%" DW_PR_XZEROS DW_PR_DUx
            " 0x%" DW_PR_XZEROS DW_PR_DUx ,v1,v2);
        break;
    default:
        printf(" ERROR: Unknown LLE code in .debug_loclists. %s\n",
            esb_get_string(&m));
        simple_err_return_msg_either_action(res,
            esb_get_string(&m));
        break;
    }
    printf( " %" DW_PR_DUu,entrylen);
    esb_destructor(&m);
    if (glflags.verbose && expr_ops_blocklen > 0) {
        printf("\n");
        printf("    ");
        printf(" opslen %" DW_PR_DUu,expr_ops_blocklen);
        print_opsbytes(expr_ops_blocklen,expr_ops);
    }
    printf("\n");
    return res;
}

/*  Prints the raw content. Exactly as in .debug_loclists
    Similar to .debug_rnglists
    Here we print, but do not stop, on seeing
    DW_LLE_end_of_list. Only the end of list
    bytes stop the iteration.  */
static int
print_entire_loclist(Dwarf_Debug dbg,
    Dwarf_Unsigned contextnumber,
    Dwarf_Unsigned header_offset,
    Dwarf_Unsigned offset_of_first_loc,
    Dwarf_Unsigned offset_past_last_locentry,
    Dwarf_Error *error)
{
    /*  These offsets are rnglists section global offsets,
        not rnglist context local offsets. */
    Dwarf_Unsigned curoffset = offset_of_first_loc;
    Dwarf_Unsigned endoffset = offset_past_last_locentry;
    int res = 0;
    Dwarf_Unsigned ct = 0;
    Dwarf_Unsigned loff = 0;
    Dwarf_Unsigned goff = 0;

    loff = offset_of_first_loc;
    goff = loff +header_offset;
    for ( ; curoffset < endoffset; ++ct ) {
        unsigned entrylen = 0;
        unsigned code = 0;
        Dwarf_Unsigned v1 = 0;
        Dwarf_Unsigned v2 = 0;
        Dwarf_Unsigned expr_ops_blocksize = 0;
        Dwarf_Unsigned expr_ops_offset = 0;
        Dwarf_Small   *expr_ops_data = 0;

        if (!ct) {
            printf("   LocationList (raw)\n");
            printf("   [goff      ][loff      ] "
                "entryname           "
                "val1     val2   entrylen\n");
        }
        /*  This returns ops data as in DWARF. No
            application of base addresses or anything. */
        res = dwarf_get_loclist_lle(dbg,contextnumber,
            curoffset,endoffset,
            &entrylen,
            &code,&v1,&v2,
            &expr_ops_blocksize,&expr_ops_offset,&expr_ops_data,
            error);
        if (res != DW_DLV_OK) {
            return res;
        }
        print_single_lle(curoffset,
            code,v1,v2,expr_ops_blocksize,
            expr_ops_data,entrylen,goff);
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

int
print_raw_all_loclists(Dwarf_Debug dbg,
    Dwarf_Error *error)
{
    int res = 0;
    Dwarf_Unsigned count = 0;
    Dwarf_Unsigned i = 0;
    res = dwarf_load_loclists(dbg,&count,error);
    if (res != DW_DLV_OK) {
        return res;
    }
    print_sec_name(dbg);

    printf(" Number of loclists contexts:  %" DW_PR_DUu "\n",
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
        Dwarf_Unsigned offset_of_first_locentry = 0;
        Dwarf_Unsigned offset_past_last_locentry = 0;

        res = dwarf_get_loclist_context_basics(dbg,i,
            &header_offset,
            &offset_size,
            &extension_size,
            &version,&address_size,&segment_selector_size,
            &offset_entry_count,&offset_of_offset_array,
            &offset_of_first_locentry,
            &offset_past_last_locentry,
            error);
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
        printf("   loclist size in bytes : %3" DW_PR_DUu "\n",
            offset_past_last_locentry - header_offset);
        printf("   Offset in section     : "
            "0x%"  DW_PR_XZEROS DW_PR_DUx"\n",
            header_offset);
        printf("   Offset  of offsets    : "
            "0x%" DW_PR_XZEROS DW_PR_DUx"\n",
            offset_of_offset_array);
        printf("   Offsetof first loc    : "
            "0x%" DW_PR_XZEROS DW_PR_DUx"\n",
            offset_of_first_locentry);
        printf("   Offset past locations : "
            "0x%" DW_PR_XZEROS DW_PR_DUx"\n",
            offset_past_last_locentry);
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
        if ((offset_of_first_locentry+1) <
            offset_past_last_locentry) {
            res = print_entire_loclist(dbg,i,
                header_offset,
                offset_of_first_locentry,
                offset_past_last_locentry,
                error);
            if (res != DW_DLV_OK) {
                return res;
            }
        }
    }
    return DW_DLV_OK;
}
