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

/*
    Handles the following from print_die.c print_attribute()
    case DW_AT_specification:
    case DW_AT_abstract_origin:
    case DW_AT_type:
*/
#include <config.h>

#include <stdlib.h> /* calloc() free() */
#include <string.h> /* memcmp() memset() strchr() strcmp()
    strlen() strncmp() */
#include <stdio.h> /* FILE decl for dd_esb.h, printf etc */

#ifdef HAVE_STDINT_H
#include <stdint.h> /* uintptr_t */
#endif /* HAVE_STDINT_H */

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
#include "dd_opscounttab.h"
#include "dd_tag_common.h"
#include "dd_attr_form.h"
#include "dd_regex.h"
#include "dd_safe_strcpy.h"

int
dd_trace_abstract_origin_etc(
    Dwarf_Debug dbg,
    Dwarf_Half  tag,
    Dwarf_Die   die,
    Dwarf_Off   dieprint_cu_goffset,
    Dwarf_Half  theform,
    Dwarf_Half  attrnum,
    Dwarf_Attribute attrib,
    char       **srcfiles,
    Dwarf_Signed srcfiles_count,
    struct esb_s *valname,
    struct esb_s *esb_extra,
    int           die_indent_level,
    Dwarf_Error  *err)
{
    char typebuf[ESB_FIXED_ALLOC_SIZE];
    struct esb_s lesb;

    /*  A local flag to make it easy to tell
        if we should append a target die name */
    Dwarf_Bool standard_messages = TRUE;

    /*  To support finding target DIE, use these helper locals. */
    Dwarf_Unsigned target_goff = 0;
#if 0
    Dwarf_Bool     target_goff_known = FALSE;
    Dwarf_Unsigned target_gofferror = 0;
    Dwarf_Bool     target_refsig8 = FALSE;
#endif
    Dwarf_Bool     target_is_info = FALSE;
    Dwarf_Bool     is_info = dwarf_get_die_infotypes_flag(die);
    int            tres = 0;

    esb_constructor_fixed(&lesb,typebuf,
        sizeof(typebuf));
    tres = get_attr_value(dbg, tag, die,
        dieprint_cu_goffset,attrib, srcfiles,
        srcfiles_count, &lesb,
        glflags.show_form_used,glflags.verbose,err);
    if (tres == DW_DLV_ERROR) {
        struct esb_s m;
        const char *n =
            get_AT_name(attrnum);
        esb_constructor(&m);
        esb_append(&m,
            "Cannot get get value for a ");
        esb_append(&m,n);
        print_error_and_continue(
            esb_get_string(&m),
            tres,*err);
        esb_destructor(&m);
        esb_destructor(valname);
        esb_destructor(esb_extra);
        return tres;
    }
    if (theform == DW_FORM_ref_sig8) {
        /* SIG8 CHECK */
        int res = 0;

#if 0
        target_refsig8 = TRUE;
#endif
        standard_messages = FALSE;
        res = dd_print_sig8_target(dbg,attrib,
            die_indent_level,
            srcfiles, srcfiles_count, &lesb,err);
        if (res == DW_DLV_ERROR) {
            esb_destructor(&lesb);
            return res;
        }
    }

    esb_empty_string(valname);
    esb_append(valname, esb_get_string(&lesb));
    esb_destructor(&lesb);

    if (glflags.gf_check_forward_decl ||
        glflags.gf_check_self_references ||
        glflags.gf_search_is_on) {
        Dwarf_Off die_goff = 0;
        Dwarf_Off ref_goff = 0;
        int frres = 0;
        int suppress_check = 0;
        Dwarf_Bool is_info2 = TRUE;

        /*  SPECIFIC CHECKS A */
        standard_messages = FALSE;
        frres = dwarf_global_formref_b(attrib, &ref_goff,
            &is_info2,err);
        if (frres == DW_DLV_ERROR) {
            /* myerr will be way less than 1000 */
            int myerr = (int)dwarf_errno(*err);
            if (myerr == DW_DLE_REF_SIG8_NOT_HANDLED) {
                /*  DW_DLE_REF_SIG8_NOT_HANDLED */
                /*  No offset available, it makes
                    little sense
                    to delve into this sort of reference
                    unless
                    we think a graph of self-refs *across*
                    type-units is possible. Hmm. FIXME? */
                suppress_check = 1 ;
                DWARF_CHECK_COUNT(self_references_result,1);
                DWARF_CHECK_ERROR(self_references_result,
                    "DW_AT_ref_sig8 not handled so "
                    "self references not fully checked");
                DROP_ERROR_INSTANCE(dbg,frres,*err);
            } else {
                const char *n =
                    get_AT_name(attrnum);
                struct esb_s m;
                esb_constructor(&m);
                esb_append(&m,
                    "Cannot get formref global offset "
                    "for a ");
                esb_append(&m,n);
                print_error_and_continue(
                    esb_get_string(&m),
                    frres,*err);
                esb_destructor(&m);
                esb_destructor(valname);
                esb_destructor(esb_extra);
                return frres;
            }
        } else if (frres == DW_DLV_NO_ENTRY) {
            const char *n =
                get_AT_name(attrnum);
            struct esb_s m;

            esb_constructor(&m);
            esb_append(&m,
                "Cannot get formref global offset for a ");
            esb_append(&m,n);
            print_error_and_continue(
                esb_get_string(&m),
                    frres,*err);
            esb_destructor(&m);
            esb_destructor(valname);
            esb_destructor(esb_extra);
            return frres;
        }
        frres = dwarf_dieoffset(die, &die_goff, err);
        if (frres != DW_DLV_OK) {
            const char *n = get_AT_name(attrnum);
            struct esb_s m;
            esb_constructor(&m);
            esb_append(&m,
                "Cannot get formref dieoffset offset for a ");
            esb_append(&m,n);
            print_error_and_continue(
                esb_get_string(&m),
                frres,*err);
            esb_destructor(&m);
            esb_destructor(valname);
            esb_destructor(esb_extra);
            return frres;
        }

        if (!suppress_check &&
            glflags.gf_check_self_references &&
            dd_form_refers_local_info(theform) ) {
            Dwarf_Die ref_die = 0;
            int  ifres = 0;

            /*  SPECIFIC CHECKS B */
            standard_messages = FALSE;
            ResetBucketGroup(glflags.pVisitedInfo);
            AddEntryIntoBucketGroup(glflags.pVisitedInfo,
                die_goff,0,0,0,
                NULL,FALSE);
            /*  Follow reference chain, looking for
                self references */
            if (glflags.nTrace[KIND_VISITED_INFO]) {
                PrintBucketGroup("Added entry dd_trace A",
                    glflags.pVisitedInfo);
            }
            frres = dwarf_offdie_b(dbg,ref_goff,is_info2,
                &ref_die,err);
            if (frres == DW_DLV_OK) {
                Dwarf_Off ref_die_cu_goff = 0;
                Dwarf_Off die_loff = 0; /* CU-relative. */
                int fresb = 0;

                if (glflags.nTrace[KIND_VISITED_INFO]) {
                    const char *atname = get_AT_name(attrnum);
                    fresb = dwarf_die_CU_offset(die,
                        &die_loff, err);
                    if (fresb == DW_DLV_OK) {
                        dd_do_dump_visited_info(die_indent_level,
                            die_loff,die_goff,
                            dieprint_cu_goffset,
                            atname,esb_get_string(valname));
                    } else {
                        esb_destructor(valname);
                        esb_destructor(esb_extra);
                        dwarf_dealloc_die(ref_die);
                        return fresb;
                    }
                }
                ++die_indent_level;
                fresb = dwarf_CU_dieoffset_given_die(ref_die,
                    &ref_die_cu_goff, err);
                    /*  Check above call return
                        status? FIXME */
                if (fresb != DW_DLV_OK) {
                    const char *n =
                        get_AT_name(attrnum);
                    struct esb_s m;
                    esb_constructor(&m);
                    esb_append(&m,
                        "Cannot get CU dieoffset "
                        "given die for a ");
                    esb_append(&m,n);
                    print_error_and_continue(
                        esb_get_string(&m),
                        frres,*err);
                    dwarf_dealloc_die(ref_die);
                    esb_destructor(&m);
                    esb_destructor(valname);
                    esb_destructor(esb_extra);
                    return frres;
                }

                ifres = dd_traverse_one_die(dbg,attrib,ref_die,
                    ref_die_cu_goff,
                    is_info,srcfiles,srcfiles_count,
                    die_indent_level, err);
                dwarf_dealloc_die(ref_die);
                ref_die = 0;
                --die_indent_level;
                if (ifres != DW_DLV_OK) {
                    esb_destructor(valname);
                    esb_destructor(esb_extra);
                    return ifres;
                }
            }
            DeleteKeyInBucketGroup(glflags.pVisitedInfo,
                die_goff);
            if (glflags.nTrace[KIND_VISITED_INFO]) {
                PrintBucketGroup("Deleted entry dd_trace A",
                    glflags.pVisitedInfo);
            }
            if (frres == DW_DLV_ERROR) {
                esb_destructor(valname);
                esb_destructor(esb_extra);
                return frres;
            }
        }
        if (!suppress_check &&
            glflags.gf_check_forward_decl) {
            /*  Check the DW_AT_specification
                forward references to DIEs.
                DWARF4 specifications, section 2.13.2.
                They are legal, this just reports how many
                are forward decls as 'error' in
                the final checks output. */
            /*  SPECIFIC CHECKS C */
            standard_messages = FALSE;
            if (attrnum == DW_AT_specification) {
                /*  Counting DW_AT_specification */
                DWARF_CHECK_COUNT(forward_decl_result,1);
                if (ref_goff > die_goff) {
                    /*  Not an error. Just counting
                        the number that are forward
                        as if it might be a problem. */
                    DWARF_ERROR_COUNT(forward_decl_result,1);
                }
            }
        }
        /*  When doing search, if the attribute is
            DW_AT_specification or
            DW_AT_abstract_origin, get any name
            associated with the DIE
            referenced in the offset.
            The 2 more typical cases are:
            Member functions, where 2 DIES are generated:
                DIE for the declaration and DIE for
                the definition
                and connected via the DW_AT_specification.
            Inlined functions, where 2 DIES are generated:
                DIE for the concrete instance and
                DIE for the abstract
                instance and connected via the
                DW_AT_abstract_origin.
        */
        if ( glflags.gf_search_is_on &&
            (attrnum == DW_AT_specification ||
            attrnum == DW_AT_abstract_origin)) {
            Dwarf_Die ref_die = 0;
            int srcres = 0;

            /*  SPECIFIC CHECKS D */
            standard_messages = FALSE;
            /*  Follow reference chain, looking for
                the DIE name */
            srcres = dwarf_offdie_b(dbg,ref_goff,is_info2,
                &ref_die,err);
            if (srcres == DW_DLV_OK) {
                /* Get the DIE name */
                char *name = 0;
                srcres = dwarf_diename(ref_die,&name,err);
                if (srcres == DW_DLV_OK) {
                    esb_empty_string(valname);
                    esb_append(valname,name);
                }
                if (srcres == DW_DLV_ERROR) {
                    glflags.gf_count_major_errors++;
                    esb_empty_string(valname);
                    esb_append(valname,
                        "<ERROR: no name for reference");
                        DROP_ERROR_INSTANCE(dbg,srcres,*err);
                    }
                    /* Release the allocated DIE */
                    dwarf_dealloc_die(ref_die);
            } else if (srcres == DW_DLV_ERROR) {
                glflags.gf_count_major_errors++;
                esb_empty_string(valname);
                esb_append(valname,
                    "<ERROR: no referred-to die found ");
                DROP_ERROR_INSTANCE(dbg,srcres,*err);
            }
        }
    }
    /*  If we are in checking mode and we do not
        have a PU name */
    if (( glflags.gf_check_locations ||
        glflags.gf_check_ranges) &&
        glflags.seen_PU && !glflags.PU_name[0]) {
        /*  SPECIFIC CHECKS E */
        standard_messages = FALSE;
        if (tag == DW_TAG_subprogram) {
            /* This gets the DW_AT_name if this
                DIE has one. */
            Dwarf_Addr low_pc =  0;
            struct esb_s pn;
            int found = 0;
            /*  The cu_die_for_print_frames will not
                be changed
                by get_proc_name_by_die().
                Used when printing frames */
            Dwarf_Die cu_die_for_print_frames = 0;

            esb_constructor(&pn);
            /* Only looks in this one DIE's attributes */
            found = get_proc_name_by_die(dbg,die,
                low_pc,&pn,
                &cu_die_for_print_frames,
                /*pcMap=*/0,
                err);
            if (found == DW_DLV_ERROR) {
                struct esb_s m;
                const char *n =
                    get_AT_name(attrnum);
                esb_constructor(&m);
                esb_append(&m,
                    "Cannot get get value for a ");
                esb_append(&m,n);
                print_error_and_continue(
                    esb_get_string(&m),
                    found,*err);
                esb_destructor(&m);
                return found;
            }
            if (found == DW_DLV_OK) {
                dd_safe_strcpy(glflags.PU_name,
                    sizeof(glflags.PU_name),
                    esb_get_string(&pn),
                    esb_string_len(&pn));
            }
            esb_destructor(&pn);
        }
    }
    if (standard_messages) {
        int smres = 0;
        Dwarf_Die target_die = 0;

        smres = dwarf_global_formref_b(attrib, &target_goff,
            &target_is_info,err);
        if (smres != DW_DLV_OK) {
            if (smres == DW_DLV_ERROR) {
                esb_append(valname,
                    " ERROR reading target offset: ");
                dwarf_dealloc_error(dbg,*err);
                *err = 0;
            }
        } else {
            int tares = 0;
            tares = dwarf_offdie_b(dbg,target_goff,target_is_info,
                &target_die,err);
            if (tares == DW_DLV_OK) {
                /* Get the DIE name */
                char *name = 0;
                tares = dwarf_diename(target_die,&name,err);
                if (tares == DW_DLV_OK) {
                    esb_append(valname," Refers to: ");
                    esb_append(valname,name);
                } else if (tares == DW_DLV_ERROR) {
                    dwarf_dealloc_error(dbg,*err);
                    *err = 0;
                } else {
#if 0
                    Be quiet here. Useless message, one thinks.
                    esb_append(valname," No target name available. ");
#endif
                }
            } else {
                if (tares == DW_DLV_ERROR) {
                    /*  Details of locviews are unclear to me. */
                    if (attrnum != DW_AT_GNU_locviews) {
                        esb_append(valname,
                            " Reference fails: ");
                        if (dwarf_errno(*err) ==
                            /*  This means we might never see
                                the error otherwise, just
                                incomplete report. */
                            DW_DLE_ABBREV_ATTR_DUPLICATION) {
                            esb_append(valname,
                                "  \n ERROR: in ref target: ");
                            esb_append(valname,dwarf_errmsg(*err));
                            ++glflags.gf_count_major_errors;
                        }
                    }
                    dwarf_dealloc_error(dbg,*err);
                    *err = 0;
                }
            }
            if (target_die) {
                dwarf_dealloc_die(target_die);
                target_die = 0;
            }
        }
    }
    return DW_DLV_OK;
}
