/*
  Copyright 2014-2020 David Anderson. All rights reserved.

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

#include <string.h> /* memset() strcmp() */
#include <stdio.h> /* FILE decl for dd_esb.h */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_naming.h"
#include "dd_esb.h"
#include "dd_esb_using_functions.h"
#include "dd_sanitized.h"
#include "print_sections.h"

static const char *
dw_dlv_string(int res)
{
    if (res == DW_DLV_ERROR) {
        return "DW_DLV_ERROR";
    }
    if (res == DW_DLV_NO_ENTRY) {
        return "DW_DLV_NO_ENTRY";
    }
    if (res == DW_DLV_OK) {
        return "DW_DLV_OK";
    }
    return "ERROR: Impossible libdwarf DW_DLV code";
}

static int
hashval_zero(Dwarf_Sig8 *val)
{
    unsigned u = 0;

    for (u=0 ; u < sizeof(Dwarf_Sig8);++u) {
        if (val->signature[u]) {
            return FALSE;
        }
    }
    return TRUE;
}

int
print_debugfission_index(Dwarf_Debug dbg,const char *type,
    Dwarf_Error *err)
{
    int res = 0;
    Dwarf_Xu_Index_Header xuhdr = 0;
    Dwarf_Unsigned version_number = 0;
    Dwarf_Unsigned offsets_count = 0;
    Dwarf_Unsigned units_count = 0;
    Dwarf_Unsigned hash_slots_count = 0;
    const char * section_name = 0;
    const char * section_type2 = 0;
    const char * section_name2 = 0;
    int is_cu = !strcmp(type,"cu")?TRUE:FALSE;

    res = dwarf_get_xu_index_header(dbg,
        type,
        &xuhdr,
        &version_number,
        &offsets_count,
        &units_count,
        &hash_slots_count,
        &section_name,
        err);
    if (res == DW_DLV_NO_ENTRY) {
        /* This applies to most object files. */
        return res;
    }
    if (res == DW_DLV_ERROR) {
        simple_err_return_msg_either_action(res,
            "ERROR: Call to dwarf_get_xu_index_header() failed.");
        return res;
    }
    res = dwarf_get_xu_index_section_type(xuhdr,
        &section_type2,
        &section_name2,
        err);
    if (res == DW_DLV_NO_ENTRY) {
        struct esb_s tmsg;

        esb_constructor(&tmsg);
        esb_append(&tmsg,
            "ERROR: dwarf_get_xu_index_section_type() "
            " returned DW_DLV_NO_ENTRY "
            " which should be impossible.");
        esb_append(&tmsg," Something is corrupted.");
        simple_err_return_action(DW_DLV_ERROR,
            esb_get_string(&tmsg));
        dwarf_dealloc_xu_header(xuhdr);
        esb_destructor(&tmsg);
        /* We have no way to return a DW_DLV_ERROR.
            as we cannot manufacture a Dwarf_Error */
        return res;
    }
    if (res == DW_DLV_ERROR) {
        simple_err_return_msg_either_action(res,
            "ERROR: Call to dwarf_get_xu_index_section_type() "
            "failed.");
        dwarf_dealloc_xu_header(xuhdr);
        return res;
    }
    if (strcmp(section_type2,type)) {
        struct esb_s tmsg;

        esb_constructor(&tmsg);
        esb_append_printf_s(&tmsg,
            "ERROR: dwarf_get_xu_index_section_type() "
            " returned section type %s ",
            sanitized(section_type2));
        esb_append_printf_s(&tmsg,
            "whereas the call was for section type %s. ",
            sanitized(type));
        esb_append(&tmsg," Something is corrupted.");
        simple_err_return_action(DW_DLV_ERROR,
            esb_get_string(&tmsg));
        esb_destructor(&tmsg);
        dwarf_dealloc_xu_header(xuhdr);
        /* We have no way to return a DW_DLV_ERROR.
            as we cannot manufacture a Dwarf_Error */
        return DW_DLV_OK;
    }
    if (!section_name || !*section_name) {
        section_name = (is_cu?".debug_cu_index":".debug_tu_index");
    }
    {
        struct esb_s truename;
        char buf[DWARF_SECNAME_BUFFER_SIZE];

        esb_constructor_fixed(&truename,buf,sizeof(buf));
        get_true_section_name(dbg,section_name,
            &truename,TRUE);
        printf("\n%s\n",sanitized(esb_get_string(&truename)));
        esb_destructor(&truename);
    }
    printf("  Version:           : %" DW_PR_DUu "\n",
        version_number);
    printf("  Number of columns N: %" DW_PR_DUu "\n",
        offsets_count);
    printf("  number of entries U: %" DW_PR_DUu  "\n",
        units_count);
    printf("  Number of slots   S: %" DW_PR_DUu "\n",
        hash_slots_count);
    {
        unsigned n = 0;
        Dwarf_Unsigned sect_num;
        const char *name = 0;

        printf("\n");
        printf("Columns index to section id and name \n");
        printf("  [ ] id       name\n");
        for ( ; n < offsets_count; ++n) {
            res = dwarf_get_xu_section_names(xuhdr,
                n,&sect_num,&name,err);
            if (res == DW_DLV_ERROR) {
                dwarf_dealloc_xu_header(xuhdr);
                return res;
            } if (res == DW_DLV_NO_ENTRY) {
                printf("  [%u] unused\n",n);
            } else {
                printf("  [%u] %" DW_PR_DUu "        %s\n",n,
                    sect_num,name);
            }
        }
    }
    if (hash_slots_count > 0) {
        printf("\n");
        printf("   slot      hash                index\n");
    }
    {
        /*  For h < S */
        Dwarf_Unsigned h = 0;
        for ( h = 0; h < hash_slots_count; h++) {
            Dwarf_Sig8 hashval;
            Dwarf_Unsigned index = 0;
            Dwarf_Unsigned col = 0;

            memset(&hashval,0,sizeof(hashval));
            res = dwarf_get_xu_hash_entry(xuhdr,h,
                &hashval,&index,err);
            if (res == DW_DLV_ERROR) {
                struct esb_s hmsg;

                esb_constructor(&hmsg);
                esb_append_printf_u(&hmsg,
                    "ERROR: dwarf_get_xu_hash_entry failed "
                    " on slot number %u ",h);
                esb_append_printf_u(&hmsg," of %u slots.",
                    hash_slots_count);
                simple_err_return_action(res,
                    esb_get_string(&hmsg));
                dwarf_dealloc_xu_header(xuhdr);
                esb_destructor(&hmsg);
                return res;
            } else if (res == DW_DLV_NO_ENTRY) {
                /* Impossible */
                struct esb_s hmsg;

                esb_constructor(&hmsg);
                esb_append_printf_u(&hmsg,
                    "ERROR: dwarf_get_xu_hash_entry got NO_ENTRY "
                    " on slot number %u ",h);
                esb_append_printf_u(&hmsg," of %u slots."
                    " That should be impossible.",
                    hash_slots_count);
                dwarf_dealloc_xu_header(xuhdr);
                esb_destructor(&hmsg);
                return res;
            } else if (!index) {
                if (hashval_zero(&hashval)) {
                    /* An unused hash slot, we do not print them */
                } else {
                    struct esb_s hashhexstring;

                    esb_constructor(&hashhexstring);
                    format_sig8_string(&hashval,&hashhexstring);
                    printf("  [%4" DW_PR_DUu "] %s"
                        " %8" DW_PR_DUu  " %s\n",
                        h,
                        esb_get_string(&hashhexstring),
                        index,
                        "Index 0 means the hash gets ignored");
                    esb_destructor(&hashhexstring);
                }
                continue;
            }
            {
                struct esb_s hashhexstring;

                esb_constructor(&hashhexstring);
                format_sig8_string(&hashval,&hashhexstring);
                printf("  [%4" DW_PR_DUu "] %s"
                    " %8" DW_PR_DUu  "\n",
                    h,
                    esb_get_string(&hashhexstring),
                    index);
                esb_destructor(&hashhexstring);
            }
            printf("    [r,c]              section   "
                "  offset             size\n");
            for (col = 0; col < offsets_count; col++) {
                Dwarf_Unsigned off = 0;
                Dwarf_Unsigned len = 0;
                const char * name = 0;
                Dwarf_Unsigned num = 0;
                res = dwarf_get_xu_section_names(xuhdr,
                    col,&num,&name,err);
                if (res != DW_DLV_OK) {
                    struct esb_s hmsg;
                    const char * et= dw_dlv_string(res);

                    esb_constructor(&hmsg);
                    esb_append_printf_s(&hmsg,
                        "ERROR: dwarf_get_xu_section_names "
                        "got %s ",et);
                    esb_append_printf_u(&hmsg,
                        " on column number %u ",col);
                    esb_append_printf_u(&hmsg," of %u columns.",
                        offsets_count);
                    simple_err_return_action(res,
                        esb_get_string(&hmsg));
                    esb_destructor(&hmsg);
                    dwarf_dealloc_xu_header(xuhdr);
                    return res;
                }
                /*  index is 1-origin. We use it
                    that way.  */
                res = dwarf_get_xu_section_offset(xuhdr,
                    index,col,&off,&len,err);
                if (res != DW_DLV_OK) {
                    struct esb_s hmsg;
                    const char * et= dw_dlv_string(res);

                    esb_constructor(&hmsg);
                    esb_append_printf_s(&hmsg,
                        "ERROR: dwarf_get_xu_section_offset "
                        "got %s ",et);
                    esb_append_printf_u(&hmsg,
                        " on index number %u ",index);
                    esb_append_printf_u(&hmsg,
                        " on column number %u ",col);
                    esb_append_printf_u(&hmsg," of %u columns.",
                        offsets_count);
                    simple_err_return_action(res,
                        esb_get_string(&hmsg));
                    esb_destructor(&hmsg);
                    dwarf_dealloc_xu_header(xuhdr);
                    return res;
                }
                printf("    [%1" DW_PR_DUu ",%1" DW_PR_DUu "] %20s "
                    "0x%" DW_PR_XZEROS DW_PR_DUx
                    " (%8" DW_PR_DUu ") "
                    "0x%" DW_PR_XZEROS DW_PR_DUx
                    " (%8" DW_PR_DUu ")\n",
                    index,
                    col,name,
                    off,off,
                    len,len);
            }
        }
    }
    dwarf_dealloc_xu_header(xuhdr);
    return DW_DLV_OK;
}
