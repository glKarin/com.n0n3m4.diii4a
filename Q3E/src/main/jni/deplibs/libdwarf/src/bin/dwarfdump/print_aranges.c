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

/* SGI has moved from the Crittenden Lane address.  */

#include <config.h>
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

static int
do_checking(Dwarf_Debug dbg, Dwarf_Arange *arange_buf,Dwarf_Signed i,
    Dwarf_Off cu_die_offset,Dwarf_Bool first_cu,
    Dwarf_Off cu_die_offset_prev, Dwarf_Die cu_die,
    Dwarf_Error *err )
{
    int dres = 0;
    Dwarf_Off cuhdroff = 0;
    Dwarf_Off cudieoff3 = 0;
    /*  .debug_types has no address ranges, only .debug_info[.dwo]
        has them.*/
    int is_info = 1;

    dres = dwarf_get_arange_cu_header_offset(
        arange_buf[i],&cuhdroff,err);
    if (dres == DW_DLV_OK) {
        Dwarf_Off cudieoff2 = 0;

        /* Get the CU offset for easy error reporting */
        if (first_cu || cu_die_offset != cu_die_offset_prev) {
            dres = dwarf_die_offsets(cu_die,&glflags.
                DIE_section_offset,
                &glflags.DIE_offset,err);
            glflags.DIE_CU_overall_offset =
                glflags.DIE_section_offset;
            glflags.DIE_CU_offset = glflags.DIE_offset;
            if (dres != DW_DLV_OK) {
                print_error_and_continue(
                    "ERROR: reading dwarf_die_offsets", dres, *err);
                return dres;
            }
        }
        dres = dwarf_get_cu_die_offset_given_cu_header_offset_b(
            dbg,cuhdroff,is_info,&cudieoff2,err);
        if (dres == DW_DLV_OK) {
            /* Get the CU offset for easy error reporting */
            int res2=0;

            res2 = dwarf_die_offsets(cu_die,
                &glflags.DIE_section_offset,
                &glflags.DIE_offset,err);
            if (res2 == DW_DLV_OK) {
                glflags.DIE_CU_overall_offset =
                    glflags.DIE_section_offset;
                glflags.DIE_CU_offset = glflags.DIE_offset;
                DWARF_CHECK_COUNT(aranges_result,1);
                if (cu_die_offset != cudieoff2) {
                    printf("Error, cu_die offsets mismatch,  0x%"
                        DW_PR_DUx " != 0x%" DW_PR_DUx
                        " from arange data",
                        cu_die_offset,cudieoff2);
                    DWARF_CHECK_ERROR(aranges_result,
                        " dwarf_get_cu_die_offset_given_cu..."
                        " gets wrong offset");
                }
            } else {
                /* DW_DLV_ERROR or DW_DLV_NO_ENTRY */
                print_error_and_continue(
                    "ERROR from arange checking offsets "
                    "dwarf_die_offsets... fails",
                    dres, *err);
            }
        } else {
            /* DW_DLV_ERROR or DW_DLV_NO_ENTRY */
            print_error_and_continue(
                "ERROR from arange checking "
                "dwarf_get_cu_die_offset_given... fails",
                dres, *err);
            return dres;
        }
    } else {
        /* DW_DLV_ERROR or DW_DLV_NO_ENTRY */
        print_error_and_continue(
            "ERROR: from arange checking "
            "dwarf_get_arange_cu_header_offset fails",
                dres, *err);
        return dres;
    }
    dres = dwarf_get_cu_die_offset(arange_buf[i],&cudieoff3,
        err);
    if (dres == DW_DLV_OK) {
        DWARF_CHECK_COUNT(aranges_result,1);
        if (cudieoff3 != cu_die_offset) {
            printf(
                "Error, cu_die offsets (b) mismatch ,  0x%"
                DW_PR_DUx
                " != 0x%" DW_PR_DUx " from arange data",
                cu_die_offset,cudieoff3);
            DWARF_CHECK_ERROR(aranges_result,
                " dwarf_get_cu_die_offset "
                " gets wrong offset");
        }
    } else {
        /* DW_DLV_ERROR or DW_DLV_NO_ENTRY */
        print_error_and_continue(
            "ERROR: from arange checking "
            "dwarf_get_cu_die_offset fails",
            dres,*err);
        return dres;
    }
    return DW_DLV_OK;
}

static void
aranges_dealloc_now(Dwarf_Debug dbg,
    Dwarf_Signed count,
    Dwarf_Arange *arange_buf)
{
    Dwarf_Signed i = 0;

    for ( ; i < count;  ++i) {
        dwarf_dealloc(dbg,arange_buf[i],DW_DLA_ARANGE);
    }
    dwarf_dealloc(dbg,arange_buf,DW_DLA_LIST);
}

/* get all the data in .debug_aranges */
int
print_aranges(Dwarf_Debug dbg,Dwarf_Error *ga_err)
{
    Dwarf_Signed count = 0;
    Dwarf_Signed i = 0;
    Dwarf_Arange *arange_buf = NULL;
    int ares = 0;
    int aires = 0;
    Dwarf_Off prev_off = 0; /* Holds previous CU offset */
    Dwarf_Bool first_cu = TRUE;
    Dwarf_Off cu_die_offset_prev = 0;

    /* Reset the global state, so we can traverse the debug_info */
    glflags.seen_CU = FALSE;
    glflags.need_CU_name = TRUE;
    glflags.need_CU_base_address = TRUE;
    glflags.need_CU_high_address = TRUE;
    glflags.current_section_id = DEBUG_ARANGES;
    ares = dwarf_get_aranges(dbg, &arange_buf, &count, ga_err);
    if (glflags.gf_do_print_dwarf) {
        /*  Now we know the section is loaded (if any) so
            lets get the true name with any compression info. */
        struct esb_s truename;
        char buf[DWARF_SECNAME_BUFFER_SIZE];

        esb_constructor_fixed(&truename,buf,sizeof(buf));
        get_true_section_name(dbg,".debug_aranges",
            &truename,TRUE);
        printf("\n%s\n",sanitized(esb_get_string(&truename)));
        esb_destructor(&truename);
    }
    if (ares == DW_DLV_ERROR) {
        print_error_and_continue(
            "Unable to load the .debug_aranges section.",
            ares,*ga_err);
        return ares;
    } else if (ares == DW_DLV_NO_ENTRY) {
        return ares;
    } else {
        for (i = 0; i < count; i++) {
            Dwarf_Unsigned segment = 0;
            Dwarf_Unsigned segment_entry_size = 0;
            Dwarf_Addr start = 0;
            Dwarf_Unsigned length = 0;
            Dwarf_Off cu_die_offset = 0;
            Dwarf_Die cu_die = NULL;
            Dwarf_Bool is_info = TRUE; /* has to be debug_info
                as this involves addresses. */

            aires = dwarf_get_arange_info_b(arange_buf[i],
                &segment,
                &segment_entry_size,
                &start, &length,
                &cu_die_offset, ga_err);
            if (aires != DW_DLV_OK) {
                struct esb_s m;

                esb_constructor(&m);
                esb_append_printf_i(&m,
                    "\nERROR: attempt to read arange %d",
                    i);
                esb_append_printf_i(&m,
                    " of  %d aranges failed.",
                    count);
                simple_err_return_msg_either_action(aires,
                    esb_get_string(&m));
                esb_destructor(&m);
                aranges_dealloc_now(dbg,count,arange_buf);
                return aires;
            } else {
                int dres;

                /*  Get basic locations for error reporting */
                dres = dwarf_offdie_b(dbg, cu_die_offset,
                    is_info,
                    &cu_die, ga_err);
                if (dres != DW_DLV_OK) {
                    struct esb_s m;
                    const char *failtype = "no-entry";
                    if (dres == DW_DLV_ERROR) {
                        failtype = "error";
                    }
                    esb_constructor(&m);
                    esb_append_printf_s(&m,
                        "\nERROR: dwarf_offdie_b() gets a "
                        "return of %s ",
                        failtype);
                    esb_append_printf_i(&m," finding the "
                        "compilation-unit DIE for "
                        "arange number %d and that "
                        "should never happen.",
                        i);
                    simple_err_return_msg_either_action(dres,
                        esb_get_string(&m));
                    esb_destructor(&m);
                    aranges_dealloc_now(dbg,count,arange_buf);
                    arange_buf = 0;
                    return dres;
                }
                if (glflags.gf_cu_name_flag) {
                    Dwarf_Bool should_skip = FALSE;
                    /*  Always sets should_skip */
                    should_skip_this_cu(dbg,
                        &should_skip,cu_die);
                    if (should_skip) {
                        dwarf_dealloc(dbg,cu_die,DW_DLA_DIE);
                        continue;
                    }
                }
                /*  Get producer name for this CU and update
                    compiler list */
                {
                    struct esb_s producer_name;
                    int pres = 0;

                    esb_constructor(&producer_name);
                    pres = get_producer_name(dbg,cu_die,cu_die_offset,
                        &producer_name,ga_err);
                    if (pres == DW_DLV_ERROR) {
                        dwarf_dealloc(dbg,cu_die,DW_DLA_DIE);
                        aranges_dealloc_now(dbg,count,arange_buf);
                        return pres;
                    }
                    update_compiler_target(
                        esb_get_string(&producer_name));
                    esb_destructor(&producer_name);
                }
                if (!checking_this_compiler()) {
                    dwarf_dealloc(dbg,cu_die,DW_DLA_DIE);
                    cu_die = 0;
                    continue;
                }

                if (glflags.gf_check_aranges) {
                    int cres = 0;

                    cres = do_checking(dbg,arange_buf,i,
                        cu_die_offset,first_cu,
                        cu_die_offset_prev,cu_die,ga_err);
                    if (cres == DW_DLV_ERROR) {
                        aranges_dealloc_now(dbg,count,arange_buf);
                        return cres;
                    }
                }
                /*  Get the offset of the cu header itself in the
                    section, but not for end-entries. */
                if (start || length) {
                    Dwarf_Off off = 0;
                    int cures3 = dwarf_get_arange_cu_header_offset(
                        arange_buf[i], &off, ga_err);
                    if (cures3 != DW_DLV_OK) {
                        struct esb_s m;
                        const char *failtype = "no-entry";
                        if (cures3 == DW_DLV_ERROR) {
                            failtype = "error";
                        }
                        esb_constructor(&m);
                        esb_append_printf_s(&m,
                            "\nERROR: dwarf_get_arange_cu_"
                            "header_offset() "
                            "gets a return of %s ",
                            failtype);
                        esb_append_printf_i(&m,"finding the "
                            "compilation-unit DIE offset for "
                            "arange number %d and that should "
                            "never happen.",
                            i);
                        simple_err_return_msg_either_action(cures3,
                            esb_get_string(&m));
                        esb_destructor(&m);
                        dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
                        aranges_dealloc_now(dbg,count,arange_buf);
                        return cures3;
                    }

                    /* Print the CU information if different.  */
                    if (prev_off != off || first_cu) {
                        first_cu = FALSE;
                        prev_off = off;
                        /*  We are faking the indent level.
                            We do not know
                            what level it is, really.

                            If do_check_dwarf we do not want to do
                            the die print call as it will do
                            check/print we may not have asked for.
                            And if we did ask for debug_info checks
                            this will do the checks a second time!
                            So only call print_one_die if printing.
                        */
                        if (glflags.gf_do_print_dwarf){
                            /*  There is no die if its a
                                set-end entry */
                            int pres = 0;
                            Dwarf_Bool attr_duplicated = FALSE;

                            pres = print_one_die(dbg, cu_die,
                                cu_die_offset,
                                /* print_information= */
                                (Dwarf_Bool) TRUE,
                                /* indent_level = */0,
                                /* srcfiles= */ 0,
                                /* cnt= */ 0,
                                &attr_duplicated,
                                /* ignore_die_stack= */TRUE,
                                ga_err);
                            if (pres == DW_DLV_ERROR) {
                                dwarf_dealloc(dbg,cu_die,DW_DLA_DIE);
                                aranges_dealloc_now(dbg,
                                    count,arange_buf);
                                return pres;
                            }
                        }
                        /*  Reset the state, so we can traverse
                            the debug_info */
                        glflags.seen_CU = FALSE;
                        glflags.need_CU_name = TRUE;
                        if (glflags.gf_do_print_dwarf) {
                            printf("\n");
                        }
                    }

                    if (glflags.gf_do_print_dwarf) {
                        /* Print current aranges record */
                        if (segment_entry_size) {
                            printf(
                                "\narange starts at seg,off 0x%"
                                DW_PR_XZEROS DW_PR_DUx
                                ",0x%" DW_PR_XZEROS DW_PR_DUx
                                ", ",
                                segment,
                                (Dwarf_Unsigned)start);
                        } else {
                            printf("\narange starts at 0x%"
                                DW_PR_XZEROS DW_PR_DUx ", ",
                                (Dwarf_Unsigned)start);
                        }
                        printf("length of 0x%" DW_PR_XZEROS DW_PR_DUx
                            ", cu_die_offset = 0x%"
                            DW_PR_XZEROS DW_PR_DUx,
                            length,
                            (Dwarf_Unsigned)cu_die_offset);
                    }
                    if (glflags.verbose &&
                        glflags.gf_do_print_dwarf) {
                        printf(" cuhdr 0x%" DW_PR_XZEROS DW_PR_DUx
                            "\n",
                            (Dwarf_Unsigned)off);
                    }
                } else {
                    /*  Must be a range end.
                        We really do want to print
                        this as there is a real record here, an
                        'arange end' record. */
                    if (glflags.gf_do_print_dwarf) {
                        printf("\narange end\n");
                    }
                }/* end start||length test */
            }  /* end aires DW_DLV_OK test */
            dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
            cu_die = 0;
            /* print associated die too? */
        } /* end loop on arange_buf  */
        aranges_dealloc_now(dbg,count,arange_buf);
        arange_buf = 0;
    } /* end DW_DLV_OK */
    return DW_DLV_OK;
}
