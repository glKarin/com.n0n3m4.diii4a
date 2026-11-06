/*
ffs
Copyright (C) 2000-2006 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright 2007-2010 Sun Microsystems, Inc. All rights reserved.
Portions Copyright 2009-2018 SN Systems Ltd. All rights reserved.
Portions Copyright 2008-2020 David Anderson. All rights reserved.

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

#include <stdlib.h> /* calloc() free() realloc() */
#include <string.h> /* memset() */
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

static struct esb_s esb_string;

void
ranges_esb_string_destructor(void)
{
    esb_destructor(&esb_string);
}
/*  Because we do not know what DIE is involved, if the
    object being printed has different address sizes
    in different compilation units this will not work
    properly: anything could happen.
    This applies to .debug_ranges, something only used
    in DWARF3 and DWARF4.
*/
extern int
print_ranges(Dwarf_Debug dbg)
{
    Dwarf_Unsigned off = 0;
    int group_number = 0;
    int wasdense = 0;
    struct esb_s truename;
    char buf[DWARF_SECNAME_BUFFER_SIZE];
    unsigned loopct = 0;

    glflags.current_section_id = DEBUG_RANGES;
    if (!glflags.gf_do_print_dwarf) {
        return DW_DLV_OK;
    }
    esb_constructor_fixed(&truename,buf,sizeof(buf));
    /*  Turn off dense, we do not want  print_ranges_list_to_extra
        to use dense form here. */
    wasdense = glflags.dense;
    glflags.dense = 0;
    for (;;++loopct) {
        Dwarf_Ranges *rangeset = 0;
        Dwarf_Signed rangecount = 0;
        Dwarf_Unsigned bytecount = 0;
        Dwarf_Off  actual_offset = 0;
        Dwarf_Error pr_err;

        int rres = dwarf_get_ranges_b(dbg,off,
            0 /*No DIE available here, we will do
                the best we can. */,
            &actual_offset,&rangeset,
            &rangecount,&bytecount,&pr_err);
        if (!loopct) {
            get_true_section_name(dbg,".debug_ranges",
                &truename,TRUE);
            printf("\n%s\n",sanitized(esb_get_string(&truename)));
        }
        if (rres == DW_DLV_OK) {
            char *val = 0;
            printf(" Ranges group %d:\n",group_number);
            esb_empty_string(&esb_string);
            print_ranges_list_to_extra(dbg,0 /* no DIE */
                ,off,off, rangeset,rangecount,
                bytecount, &esb_string);
            dwarf_dealloc_ranges(dbg,rangeset,rangecount);
            val = esb_get_string(&esb_string);
            printf("%s",sanitized(val));
            ++group_number;
        } else if (rres == DW_DLV_NO_ENTRY) {
            printf("End of %s.\n",
                sanitized(esb_get_string(&truename)));
            break;
        } else {
            /*  ERROR, which does not quite mean a real error,
                as we might just be misaligned reading things without
                a DW_AT_ranges offset.*/
            struct esb_s m;

            esb_constructor(&m);
            esb_append_printf_u(&m,"ERROR: at offset 0x%lx in ",off);
            esb_append_printf_s(&m,"section %s. Stopping ranges"
                " output.",sanitized(esb_get_string(&truename)));
            print_error_and_continue(esb_get_string(&m),
                rres,pr_err);
            dwarf_dealloc(dbg,pr_err,DW_DLA_ERROR);
            pr_err = 0;
            esb_destructor(&m);
            break;
        }
        off += bytecount;
    }
    glflags.dense = wasdense;
    esb_destructor(&truename);
    return DW_DLV_OK;
}

/*  Extracted this from print_range_attribute()
    to isolate the check of the range list.
*/
static int
check_ranges_list(Dwarf_Debug dbg,
    Dwarf_Die cu_die,
    Dwarf_Unsigned original_off,
    Dwarf_Unsigned finaloff,
    Dwarf_Ranges *rangeset,
    Dwarf_Signed rangecount,
    Dwarf_Unsigned bytecount,
    Dwarf_Error *err)
{
    Dwarf_Unsigned off = original_off;
    Dwarf_Signed index = 0;
    Dwarf_Addr base_address = glflags.CU_base_address;
    Dwarf_Addr lopc = 0;
    Dwarf_Addr hipc = 0;
    Dwarf_Bool bError = FALSE;
    Dwarf_Half elf_address_size = 0;
    Dwarf_Addr elf_max_address = 0;
    static Dwarf_Bool do_print = TRUE;
    const char *sec_name = 0;
    struct esb_s truename;
    char buf[DWARF_SECNAME_BUFFER_SIZE];

    esb_constructor_fixed(&truename,buf,sizeof(buf));
    get_true_section_name(dbg,".debug_ranges",
        &truename,FALSE);
    sec_name = esb_get_string(&truename);
    get_address_size_and_max(dbg,&elf_address_size,
        &elf_max_address,err);

    /* Ignore last entry, is the end-of-list */
    for (index = 0; index < rangecount - 1; index++) {
        Dwarf_Ranges *r = rangeset + index;

        if (r->dwr_addr1 == elf_max_address) {
            /*  (0xffffffff,addr), use specific address
                (current PU address) */
            base_address = r->dwr_addr2;
            continue;
        }
        {
            /* (offset,offset), update using CU address */
            lopc = r->dwr_addr1 + base_address;
            hipc = r->dwr_addr2 + base_address;
            DWARF_CHECK_COUNT(ranges_result,1);

            if (lopc == hipc) {
                /* An empty range. Ignore the addresses */
                continue;
            }
            /*  Check the low_pc and high_pc
                are within a valid range in
                the .text section */
            if (IsValidInBucketGroup(glflags.pRangesInfo,lopc) &&
                IsValidInBucketGroup(glflags.pRangesInfo,hipc)) {
                /* Valid values; do nothing */
                continue;
            }
            /*  At this point may be
                dealing with a
                linkonce symbol */
            if (IsValidInLinkonce(glflags.pLinkonceInfo,
                glflags.PU_name,lopc,hipc)) {
                /* Valid values; do nothing */
                continue;
            }
            {
                struct esb_s errbuf;

                bError = TRUE;
                esb_constructor(&errbuf);
                esb_append_printf_s(&errbuf,
                    "%s: Address outside a "
                    "valid .text range",sanitized(sec_name));
                DWARF_CHECK_ERROR(ranges_result,
                    esb_get_string(&errbuf));
                esb_destructor(&errbuf);
                if (glflags.gf_check_verbose_mode && do_print) {
                    /*  Update DIEs offset just for printing */
                    int dioff_res = dwarf_die_offsets(cu_die,
                        &glflags.DIE_section_offset,
                        &glflags.DIE_offset,err);
                    if (dioff_res != DW_DLV_OK) {
                        simple_err_return_msg_either_action(
                            dioff_res,
                            "Call to dwarf_die_offsets failed "
                            "for the CU DIE."
                            "in printing ranges");
                        return dioff_res;
                    }
                    printf(
                        "Offset = 0x%" DW_PR_XZEROS DW_PR_DUx
                        ", Base = 0x%" DW_PR_XZEROS DW_PR_DUx
                        ", "
                        "Low = 0x%" DW_PR_XZEROS DW_PR_DUx
                        " (0x%" DW_PR_XZEROS  DW_PR_DUx
                        "), High = 0x%"
                        DW_PR_XZEROS  DW_PR_DUx
                        " (0x%" DW_PR_XZEROS DW_PR_DUx
                        ")\n",
                        off,base_address,lopc,
                        r->dwr_addr1,hipc,
                        r->dwr_addr2);
                }
            }
        }
        /*  Each entry holds 2 addresses (offsets) */
        off += elf_address_size * 2;
    }

    /*  In the case of errors, we have to print the range
        records that caused the error. */
    if (bError && glflags.gf_check_verbose_mode && do_print) {
        struct esb_s rangesstr;
        esb_constructor(&rangesstr);

        printf("\n");
        print_ranges_list_to_extra(dbg,cu_die,original_off,
            finaloff,
            rangeset,rangecount,bytecount,
            &rangesstr);
        printf("%s\n", sanitized(esb_get_string(&rangesstr)));
        esb_destructor(&rangesstr);
    }

    /*  In the case of printing unique errors, stop the printing
        of any subsequent errors, which have the same text. */
    if (bError && glflags.gf_check_verbose_mode &&
        glflags.gf_print_unique_errors) {
        do_print = FALSE;
    }
    esb_destructor(&truename);
    return DW_DLV_OK;
}

/*  Records information about compilers (producers) found in the
    debug information, including the check results for several
    categories (see -k option). */
typedef struct {
    Dwarf_Off die_off;
    Dwarf_Off range_off;
} Range_Array_Entry;

/*  Array to record the DW_AT_range attribute DIE, to be used
    at the end of the CU, to check the range values; DWARF4
    allows an offset relative to the low_pc as the high_pc
    value. Also, LLVM generates for the CU the
    pair (low_pc, at_ranges) instead of the traditional
    (low_pc, high_pc).
*/
static Range_Array_Entry *range_array = NULL;
static Dwarf_Unsigned range_array_size = 0;
static Dwarf_Unsigned range_array_count = 0;
#define RANGE_ARRAY_INITIAL_SIZE 64

/*  Allocate space to store information about the ranges;
    the values are extracted from the DW_AT_ranges attribute.
    The space is reused by all CUs.
*/
void
allocate_range_array_info(void)
{
    if (range_array == NULL) {
        /* Allocate initial range array info */
        range_array = (Range_Array_Entry *)
            calloc(RANGE_ARRAY_INITIAL_SIZE,
                sizeof(Range_Array_Entry));
        range_array_size = RANGE_ARRAY_INITIAL_SIZE;
    }
}

void
release_range_array_info(void)
{
    if (range_array) {
        free(range_array);
        range_array = 0;
        range_array_size = 0;
        range_array_count = 0;
    }
}

/*  Clear out values from previous CU */
static void
reset_range_array_info(void)
{
    if (range_array) {
        memset((void *)range_array,0,
            (range_array_count) * sizeof(Range_Array_Entry));
        range_array_count = 0;
    }
}

void
record_range_array_info_entry(Dwarf_Off die_off,Dwarf_Off range_off)
{
    /* Record a new detected range info. */
    if (range_array_count == range_array_size) {
        /* Resize range array */
        Range_Array_Entry * nrp = 0;
        Dwarf_Signed nsize = range_array_size*2;
        nrp = (Range_Array_Entry *)
            realloc(range_array,
            nsize * sizeof(Range_Array_Entry));
        if (!nrp) {
            printf("\nERROR: realloc fails building "
                "range array entry %"
                DW_PR_DUu ". Attempting "
                "to continue.\n",range_array_size+1);
            glflags.gf_count_major_errors++;
            return;
        }
        range_array = nrp;
        range_array_size = nsize;
        nrp = 0;
    }
    /* The 'die_off' is the Global Die Offset */
    range_array[range_array_count].die_off = die_off;
    range_array[range_array_count].range_off = range_off;
    ++range_array_count;
}

/*  Now that we are at the end of the CU, check the range lists */
int
check_range_array_info(Dwarf_Debug dbg,
    Dwarf_Die cu_die_in,Dwarf_Error * err)
{
    Dwarf_Bool is_info = dwarf_get_die_infotypes_flag(cu_die_in);
    if (range_array && range_array_count) {
        /*  Traverse the range array and for each entry:
            Load the ranges
            Check for any outside conditions */
        Dwarf_Off original_off = 0;
        Dwarf_Off finaloffset = 0;
        Dwarf_Off die_off = 0;
        Dwarf_Unsigned index = 0;
        Dwarf_Die cu_die = 0;
        int res = 0;

        /*  In case of errors, the correct DIE offset should be
            displayed. At this point we are at the end of the PU */
        Dwarf_Off DIE_section_offset_bak = glflags.DIE_section_offset;

        for (index = 0; index < range_array_count; ++index) {
            Dwarf_Ranges *rangeset = 0;
            Dwarf_Signed rangecount = 0;
            Dwarf_Unsigned bytecount = 0;

            /* Get a range info record */
            die_off = range_array[index].die_off;
            original_off = range_array[index].range_off;

            res = dwarf_offdie_b(dbg,die_off,is_info,&cu_die,err);
            if (res != DW_DLV_OK) {
                struct esb_s m;

                esb_constructor(&m);
                esb_append_printf_u(&m,
                    "Call to dwarf_offdie failed "
                    "getting the CU die from offset "
                    " 0x%" DW_PR_XZEROS DW_PR_DUx
                    "in checking ranges",die_off);
                simple_err_return_msg_either_action(
                    res,
                    esb_get_string(&m));
                esb_destructor(&m);
                return res;
            }
            res = dwarf_get_ranges_b(dbg,original_off,
                cu_die,
                &finaloffset,
                &rangeset,&rangecount,&bytecount,err);
            if (res == DW_DLV_OK) {
                res = check_ranges_list(dbg,cu_die,
                    original_off,finaloffset,
                    rangeset,rangecount,bytecount,err);
                if (res != DW_DLV_OK) {
                    dwarf_dealloc(dbg,cu_die,DW_DLA_DIE);
                    reset_range_array_info();
                    return res;
                }
                dwarf_dealloc(dbg,rangeset,DW_DLA_RANGES);
            }
            dwarf_dealloc(dbg,cu_die,DW_DLA_DIE);
        };
        reset_range_array_info();

        /*  Point back to the end of the PU */
        glflags.DIE_section_offset = DIE_section_offset_bak;
    }
    return DW_DLV_OK;
}
