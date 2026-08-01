/*
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

#include <string.h> /* memset() */
#include <stdio.h> /* FILE decl for dd_esb.h, printf etc */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_naming.h"
#include "dd_macrocheck.h"
#include "dd_esb.h"
#include "dd_esb_using_functions.h"
#include "dd_sanitized.h"

#include "print_sections.h"
#include "print_frames.h"

struct macro_counts_s {
    long mc_start_file;
    long mc_end_file;
    long mc_define;
    long mc_undef;
    long mc_extension;
    long mc_code_zero;
    long mc_unknown;
};

static int
print_one_macro_entry_detail(long i,
    char *type,
    struct Dwarf_Macro_Details_s *mdp)
{
    /* "DW_MACINFO_*: section-offset file-index [line] string\n" */
    if (glflags.gf_do_print_dwarf) {
        if (mdp->dmd_macro) {
            printf("%3ld %s: %6" DW_PR_DUu " %4" DW_PR_DSd " [%4"
                DW_PR_DSd "] \"%s\" \n",
                i,
                type,
                (Dwarf_Unsigned)mdp->dmd_offset,
                mdp->dmd_fileindex, mdp->dmd_lineno,
                sanitized(mdp->dmd_macro));
            print_split_macro_value(mdp->dmd_macro);
        } else {
            printf("%3ld %s: %6" DW_PR_DUu " %4" DW_PR_DSd " [%4"
                DW_PR_DSd "] 0\n",
                i,
                type,
                (Dwarf_Unsigned)mdp->dmd_offset,
                mdp->dmd_fileindex, mdp->dmd_lineno);
        }
    }
    return DW_DLV_OK;
}

/*  Nothing in here can actually fail.
    Returns DW_DLV_OK */
static int
print_one_macro_entry(long i,
    struct Dwarf_Macro_Details_s *mdp,
    struct macro_counts_s *counts,
    char ** srcfiles,
    Dwarf_Signed srcf_count)
{
    int res = 0;

    switch (mdp->dmd_type) {
    case 0:
        counts->mc_code_zero++;
        res = print_one_macro_entry_detail(i,
            "DW_MACINFO_type-code-0", mdp);
        break;

    case DW_MACINFO_start_file: {
        counts->mc_start_file++;
        if (mdp->dmd_fileindex == 0) {
            mdp->dmd_macro = "<zero index, no file specified>";
        } else if (srcf_count > 0  &&
            mdp->dmd_fileindex <= srcf_count) {
            mdp->dmd_macro = srcfiles[mdp->dmd_fileindex-1];
        } else {
            if (srcf_count == 0)  {
                mdp->dmd_macro =
                    "<invalid index, no line table file names exist>";
            } else {
                mdp->dmd_macro = "<invalid index, corrupt data?>";
            }
        }
        res = print_one_macro_entry_detail(i,
            "DW_MACINFO_start_file", mdp);
        break;
    }

    case DW_MACINFO_end_file:
        counts->mc_end_file++;
        res = print_one_macro_entry_detail(i,
            "DW_MACINFO_end_file  ", mdp);
        break;

    case DW_MACINFO_vendor_ext:
        counts->mc_extension++;
        res = print_one_macro_entry_detail(i,
            "DW_MACINFO_vendor_ext", mdp);
        break;

    case DW_MACINFO_define:
        counts->mc_define++;
        res = print_one_macro_entry_detail(i,
            "DW_MACINFO_define    ", mdp);
        break;

    case DW_MACINFO_undef:
        counts->mc_undef++;
        res = print_one_macro_entry_detail(i,
            "DW_MACINFO_undef     ", mdp);
        break;

    default:
        {
            struct esb_s typeb;

            esb_constructor(&typeb);
            counts->mc_unknown++;
            esb_append_printf_u(&typeb,
                "DW_MACINFO_0x%x, of unknown type", mdp->dmd_type);
            print_one_macro_entry_detail(i,
                esb_get_string(&typeb), mdp);
            esb_destructor(&typeb);
        }
        res = DW_DLV_OK;
        break;
    }
    return res;
}

static void
mac_dealloc_srcfiles_data(Dwarf_Debug dbg,
    char **srcfiles,
    Dwarf_Signed srcf_count)
{
    Dwarf_Signed i = 0;
    if (!srcfiles) {
        return;
    }
    for ( ; i < srcf_count; ++i) {
        dwarf_dealloc(dbg,srcfiles[i],DW_DLA_STRING);
    }
    dwarf_dealloc(dbg, srcfiles, DW_DLA_LIST);
}

/*  print data in .debug_macinfo */
/*ARGSUSED*/ int
print_macinfo_by_offset(Dwarf_Debug dbg,
    Dwarf_Die cu_die,
    Dwarf_Unsigned offset,
    Dwarf_Error *error)
{
    Dwarf_Unsigned max = 0;
    Dwarf_Signed count = 0;
    Dwarf_Macro_Details *maclist = NULL;
    int lres = 0;
    long i = 0;
    struct macro_counts_s counts;
    Dwarf_Unsigned totallen = 0;
    Dwarf_Bool is_primary = TRUE;
    char ** srcfiles = 0;
    Dwarf_Signed srcf_count = 0;

    glflags.current_section_id = DEBUG_MACINFO;

    /*  No real need to get the real section name, this
        section not used much in modern compilers
        as this definition of macro data (V2-V4)
        is obsolete -- it takes too much space to be
        much used. */

    lres = dwarf_get_macro_details(dbg, offset,
        max, &count, &maclist, error);
    if (lres == DW_DLV_ERROR) {
        struct esb_s m;

        esb_constructor(&m);
        esb_append_printf_u(&m,
            "\nERROR: dwarf_get_macro_details() fails on"
            " offset 0x%x from print_macinfo_by_offset().",offset);
        simple_err_only_return_action(lres,
            esb_get_string(&m));
        esb_destructor(&m);
        return lres;
    } else if (lres == DW_DLV_NO_ENTRY) {
        return lres;
    }
    lres = dwarf_srcfiles(cu_die,&srcfiles,&srcf_count,error);
    if (lres == DW_DLV_ERROR) {
        /*  This error will get found other places. No need
            to say anything here. */
        dwarf_dealloc_error(dbg,*error);
        *error = 0;
    }

    memset(&counts, 0, sizeof(counts));
    if (glflags.gf_do_print_dwarf) {
        struct esb_s truename;
        char buf[DWARF_SECNAME_BUFFER_SIZE];

        esb_constructor_fixed(&truename,buf,sizeof(buf));
        get_true_section_name(dbg,".debug_macinfo",
            &truename,TRUE);
        printf("\n%s\n",sanitized(esb_get_string(&truename)));
        esb_destructor(&truename);
        printf("\n");
        printf("compilation-unit .debug_macinfo offset "
            "0x%" DW_PR_XZEROS DW_PR_DUx "\n",offset);
        printf("                          sec    file\n");
        printf("num name                  offset index [line] "
            "\"string\"\n");
    }
    for (i = 0; i < count; i++) {
        struct Dwarf_Macro_Details_s *mdp = &maclist[i];
        print_one_macro_entry(i, mdp, &counts,
            srcfiles,srcf_count);
    }

    if (counts.mc_start_file == 0) {
        printf("ERROR: DW_MACINFO file count of zero is "
            "invalid DWARF2/3/4\n");
        glflags.gf_count_major_errors++;
    }
    if (counts.mc_start_file != counts.mc_end_file) {
        glflags.gf_count_major_errors++;
        printf("ERROR: Counts of DW_MACINFO start_file (%ld)"
            " end_file (%ld) "
            "do not match!. Incorrect DWARF2,3,4.\n",
            counts.mc_start_file, counts.mc_end_file);
    }
    if (counts.mc_code_zero < 1) {
        glflags.gf_count_major_errors++;
        printf("ERROR: Count of zeros in macro group "
            "should be non-zero "
            "(1 preferred), count is %ld\n",
            counts.mc_code_zero);
    }
    /* next byte is  maclist[count - 1].dmd_offset + 1; */
    totallen = (maclist[count - 1].dmd_offset + 1) - offset;
    add_macro_import(&macinfo_check_tree,is_primary, offset,0,0);
    add_macro_area_len(&macinfo_check_tree,offset,totallen);
    if (glflags.gf_do_print_dwarf) {
        printf("Macro counts: start file %ld, "
            "end file %ld, "
            "define %ld, "
            "undef %ld, "
            "ext %ld, "
            "code-zero %ld, "
            "unknown %ld\n",
            counts.mc_start_file,
            counts.mc_end_file,
            counts.mc_define,
            counts.mc_undef,
            counts.mc_extension,
            counts.mc_code_zero, counts.mc_unknown);
    }

    /* int type= maclist[count - 1].dmd_type; */
    /* ASSERT: type is zero */
    mac_dealloc_srcfiles_data(dbg,srcfiles,srcf_count);
    dwarf_dealloc(dbg, maclist, DW_DLA_STRING);
    return DW_DLV_OK;
}
