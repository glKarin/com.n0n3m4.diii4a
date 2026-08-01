/*
Copyright (C) 2000-2006 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright 2007-2010 Sun Microsystems, Inc. All rights reserved.
Portions Copyright 2009-2018 SN Systems Ltd. All rights reserved.
Portions Copyright 2007-2021 David Anderson. All rights reserved.

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

/*  The address of the Free Software Foundation is
    Free Software Foundation, Inc., 51 Franklin St, Fifth
    Floor, Boston, MA 02110-1301, USA.  SGI has moved from
    the Crittenden Lane address.  */

#include <config.h>
#include <stdio.h> /* FILE decl for dd_esb.h, printf etc */
#include <stdlib.h> /* calloc() free() */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_naming.h"
#include "dd_esb.h"                /* For flexible string buffer. */
#include "dd_esb_using_functions.h"
#include "dd_sanitized.h"
#include "print_frames.h"  /* for print_expression_operations() . */
#include "dd_macrocheck.h"
#include "dd_helpertree.h"
#include "dd_tag_common.h"
#include "dd_attr_form.h"
#include "dd_tsearchbal.h"

/*  Borrow the definition from pro_encode_nm.h */
/*  Bytes needed to encode a number.
    Not a tight bound, just a reasonable bound.
*/
#ifndef ENCODE_SPACE_NEEDED
#define ENCODE_SPACE_NEEDED   (2*sizeof(Dwarf_Unsigned))
#endif /* ENCODE_SPACE_NEEDED */

typedef struct attr_encoding {
    Dwarf_Unsigned entries; /* Attribute occurrences */
    Dwarf_Unsigned formx;   /* Space used by current encoding */
    Dwarf_Unsigned leb128;  /* Space used with LEB128 encoding */
} a_attr_encoding;

/*  The other DW_FORM_datan are lower form values than data16,
    so the following is safe for the unchanging  static table. */
static int attributes_encoding_factor[DW_FORM_data16 + 1];
/*  These must be reset for each object if we are processing
    an archive! see print_attributes_encoding(). */
static a_attr_encoding *attributes_encoding_table = NULL;
static Dwarf_Bool attributes_encoding_do_init = TRUE;

/*  Check the potential amount of space wasted by
    attributes values that can
    be represented as an unsigned LEB128.
    Only attributes with forms:
    DW_FORM_data1, DW_FORM_data2, DW_FORM_data4 and
    DW_FORM_data are checked
    See print_tag_attributes_usage.c for examples of
    a better way to do this.
*/
void
check_attributes_encoding(Dwarf_Half attr,Dwarf_Half theform,
    Dwarf_Unsigned value)
{

    if (attributes_encoding_do_init) {
        /* Create table on first call */
        attributes_encoding_table = (a_attr_encoding *)calloc(
            DW_AT_lo_user,
            sizeof(a_attr_encoding));
        if (!attributes_encoding_table) {
            printf("\nERROR: Unable the check attributes "
                "encoding as calloc failed. Trying to continue\n");
            glflags.gf_count_major_errors++;
            return;
        }
        /* We use only 5 slots in the table, for quick access */
        /* index 0x0b */
        attributes_encoding_factor[DW_FORM_data1]=1; /* index 0x0b */
        attributes_encoding_factor[DW_FORM_data2]=2; /* index 0x05 */
        attributes_encoding_factor[DW_FORM_data4]=4; /* index 0x06 */
        attributes_encoding_factor[DW_FORM_data8]=8; /* index 0x07 */

        /* index 0x1e */
        attributes_encoding_factor[DW_FORM_data16] = 16;
        attributes_encoding_do_init = FALSE;
    }

    /* Regardless of the encoding form, count the checks. */
    DWARF_CHECK_COUNT(attr_encoding_result,1);

    /*  For 'DW_AT_stmt_list', due to the way is generated,
        the value can be unknown at compile time and only
        the assembler can decide how to represent the offset;
        ignore this attribute. */
    if (DW_AT_stmt_list == attr ||
        DW_AT_macros == attr ||
        DW_AT_GNU_macros == attr) {
        if (theform == DW_FORM_addr) {
            struct esb_s lesb;
            esb_constructor(&lesb);
            esb_append_printf_s(&lesb,
                "Attribute %s has form ",
                get_AT_name(attr));
            esb_append_printf_s(&lesb,
                " %s, an error",
                get_FORM_name(theform));
            DWARF_CHECK_ERROR(attr_encoding_result,
                esb_get_string(&lesb));
            esb_destructor(&lesb);
        }
        return;
    }

    /*  Only checks those attributes that have DW_FORM_dataX:
        DW_FORM_data1, DW_FORM_data2, DW_FORM_data4 and DW_FORM_data8
        DWARF5 adds DW_FORM_data16, but we ignore data16 here
        as it makes no sense as a uleb. */
    if (theform == DW_FORM_data1 || theform == DW_FORM_data2 ||
        theform == DW_FORM_data4 || theform == DW_FORM_data8 ) {
        int res = 0;
        /*  Size of the byte stream buffer that needs to be
            memcpy-ed. */
        int leb128_size = 0;
        /* To encode the attribute value */
        char encode_buffer[ENCODE_SPACE_NEEDED];

        res = dwarf_encode_leb128(value,&leb128_size,
            encode_buffer,sizeof(encode_buffer));
        if (res == DW_DLV_OK) {
            if (attributes_encoding_factor[theform] > leb128_size) {
                if (glflags.gf_check_verbose_mode > 1) {
                    int wasted_bytes = attributes_encoding_factor[
                        theform] - leb128_size;
                    struct esb_s lesb;
                    esb_constructor(&lesb);

                    esb_append_printf_i(&lesb,
                        "%" DW_PR_DSd
                        " wasted ",wasted_bytes);
                    esb_append_printf_s(&lesb,
                        "%s",(wasted_bytes ==1)?"byte":"bytes");
                    esb_append_printf_s(&lesb,
                        " using form %s ",get_FORM_name(theform));
                    DWARF_CHECK_ERROR2(attr_encoding_result,
                        get_AT_name(attr),
                        esb_get_string(&lesb));
                    esb_destructor(&lesb);
                }
                if (attr < DW_AT_lo_user) {
                    attributes_encoding_table[attr].entries += 1;
                    attributes_encoding_table[attr].formx   +=
                        attributes_encoding_factor[theform];
                    attributes_encoding_table[attr].leb128  +=
                        leb128_size;
                }
            }
        }
        /* ignoring error, it should be impossible. */
    }
}

/*  Print a detailed encoding standard usage per attribute -kE
    This would be better done using tree setup so non-standard
    attributes with the problem are noted.
    See print_tag_attributes_usage.c for code supporting
    and showing tree setups.
*/
int
print_attributes_encoding(Dwarf_Debug dbg,
    Dwarf_Error* attr_error)
{
    if (attributes_encoding_table) {
        Dwarf_Unsigned total_entries = 0;
        Dwarf_Unsigned total_bytes_formx = 0;
        Dwarf_Unsigned total_bytes_leb128 = 0;
        Dwarf_Unsigned entries = 0;
        Dwarf_Unsigned bytes_formx = 0;
        Dwarf_Unsigned bytes_leb128 = 0;
        int index;
        int count = 0;
        float saved_rate = 0.0;
        Dwarf_Bool intro_printed = FALSE;

        for (index = 0; index < DW_AT_lo_user; ++index) {
            if (!intro_printed &&
                attributes_encoding_table[index].leb128) {
                {
                    intro_printed = TRUE;
                    printf("\n*** SPACE USED AND WASTED BY ATTRIBUTE "
                        "ENCODINGS of DW_FORM_data<n> ***\n");
                    if (glflags.gf_check_verbose_mode == 1){
                        printf("    To see each instance"
                        " individually rerun "
                        "dwarfdump adding option -v\n" );
                    }
                    printf("    Instances of linker relocated fields"
                        " cannot be LEB\n");
                    printf("    because few linkers know how to"
                        " relocate LEB values\n");
                    printf("    So some 'waste' listed may not "
                        "really be wasted space.\n");
                    printf("Nro Attribute Name                 "
                        "   Entries     Data_x     leb128-Better\n");
                }
                entries = attributes_encoding_table[index].entries;
                bytes_formx = attributes_encoding_table[index].formx;
                bytes_leb128 =
                    attributes_encoding_table[index].leb128;
                total_entries += entries;
                total_bytes_formx += bytes_formx;
                total_bytes_leb128 += bytes_leb128;
                saved_rate = 0.0;
                if (bytes_formx) {
                    saved_rate = (((float)bytes_formx
                        -(float)bytes_leb128)/
                        (float)bytes_formx) * 100;
                }
                printf("%3d %-25s "
                    "  %10" DW_PR_DUu /* Entries */
                    "  %10" DW_PR_DUu /* FORMx */
                    "  %10" DW_PR_DUu /* LEB128 */
                    "  %3.0f%%"
                    "\n",
                    ++count,
                    get_AT_name(index),
                    entries,
                    bytes_formx,
                    bytes_leb128,
                    saved_rate);
            }
        }
        {
            /*  If we have an entry, print summary and percentage */
            Dwarf_Addr lower = 0;
            Dwarf_Unsigned size = 0;
            int infoerr = 0;

            Dwarf_Signed saved =
                (Dwarf_Signed)total_bytes_formx -
                (Dwarf_Signed)total_bytes_leb128;
            if (total_bytes_formx) {
                saved_rate = ((float)saved/(float)total_bytes_formx)
                    *100;
            }

            if (total_entries) {
                const char * truename = "<missing>";
                printf("** Summary **\n");
                printf("  Entry Count           :"
                    " %lu\n",
                    (unsigned long)total_entries);
                printf("  Data Bytes FORM datax :"
                    " %lu\n",
                    (unsigned long)total_bytes_formx);
                printf("  Data Bytes as uleb    :"
                    " %lu\n",
                    (unsigned long)total_bytes_leb128);
                printf("  Saving with uleb      :"
                    " %3.0f percent\n",
                    saved_rate);
                /*  Get .debug_info size  */
                infoerr = dwarf_get_die_section_name(dbg,
                    TRUE,&truename,attr_error);
                if (infoerr != DW_DLV_OK) {
                    truename = ".debug_info";
                    if (infoerr == DW_DLV_ERROR) {
                        return infoerr;
                    }
                }
                infoerr = dwarf_get_section_info_by_name(dbg,
                    truename,&lower,
                    &size,attr_error);
                if (infoerr == DW_DLV_ERROR) {
                    free(attributes_encoding_table);
                    attributes_encoding_table = 0;
                    attributes_encoding_do_init = TRUE;
                    return infoerr;
                }
                saved_rate = 0.0;
                if (size) {
                    saved_rate = (float)(((total_bytes_formx -
                        (float)total_bytes_leb128) * 100.0) /
                        (float)size);
                }
                if (saved_rate > 0.0) {
                    printf("** .debug_info size can be reduced "
                        "by %.1f percent using leb **\n",
                        saved_rate);
                }
            }
        }
        free(attributes_encoding_table);
        attributes_encoding_table = 0;
        attributes_encoding_do_init = TRUE;
    }
    return DW_DLV_OK;
}
