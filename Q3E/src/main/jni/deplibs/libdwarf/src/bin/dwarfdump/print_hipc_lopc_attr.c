/*
Copyright (C) 2000-2006 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright 2007-2010 Sun Microsystems, Inc. All rights reserved.
Portions Copyright 2009-2018 SN Systems Ltd. All rights reserved.
Portions Copyright 2007-2020 David Anderson. All rights reserved.

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
#include "dd_macrocheck.h"
#include "dd_helpertree.h"
#include "dd_tag_common.h"
#include "print_frames.h"  /* for print_expression_operations() . */

/* Is this a PU has been invalidated by the SN Systems linker? */
#define IsInvalidCode(low,high) \
    (((low) == max_address) || ((low) == 0 && (high) == 0))

/*  Most types of CU can have highpc and/or lowpc.
    DW_TAG_type_unit will not. */
static Dwarf_Bool
cu_tag_type_may_have_lopc_hipc(int tag)
{
    if (tag == DW_TAG_compile_unit) {
        return TRUE;
    }
    if (tag == DW_TAG_partial_unit) {
        return TRUE;
    }
    if (tag == DW_TAG_skeleton_unit) {
        return TRUE;
    }
    return FALSE;
}

/*  This updates values in glflags used for reporting
    in error cases. */
static void
update_cu_base_addresses(Dwarf_Debug dbg,
    Dwarf_Attribute attrib,
    Dwarf_Half attr,
    Dwarf_Half tag,
    LoHiPc     *lohipc,
    Dwarf_Error *err)
{
    /* Update base address for CU */
    if (attr == DW_AT_low_pc) {
        if (glflags.need_CU_base_address &&
            cu_tag_type_may_have_lopc_hipc(tag)) {
            int lres = dwarf_formaddr(attrib,
                &glflags.CU_base_address, err);
            DROP_ERROR_INSTANCE(dbg,lres,*err);
            if (lres == DW_DLV_OK) {
                glflags.need_CU_base_address = FALSE;
                glflags.CU_low_address =
                    glflags.CU_base_address;
            }
        } else if (!glflags.CU_low_address) {
            /*  We take the first non-zero address
                as meaningful. Even if no such in CU DIE. */
            int fres = dwarf_formaddr(attrib,
                &glflags.CU_low_address, err);
            DROP_ERROR_INSTANCE(dbg,fres,*err);
            if (fres == DW_DLV_OK) {
                /*  Stop looking for base. Bogus, but
                    there is none available, so stop. */
                glflags.need_CU_base_address = FALSE;
            }
        }
    }

    /* Update high address for CU */
    if (attr == DW_AT_high_pc) {
        if (glflags.need_CU_high_address ) {
            /*  This is bogus in that it accepts the first
                high address in the CU, from any TAG */
            if (lohipc->sawhi_flag == LOHIPC_SAWADDR) {
                glflags.need_CU_high_address = FALSE;
                glflags.CU_high_address = lohipc->hival;
            } else if (lohipc->havefinal_flag) {
                glflags.CU_high_address = lohipc->hifinal;
                glflags.need_CU_high_address = FALSE;
            }
        }
    }
}

#if 0 /* debugging only */
static void
dumplohipc(LoHiPc *hilopc,char *msg, int line)
{
    printf("LoHiPc  %s line %d "
        "sawlo:    %d  0x%" DW_PR_XZEROS DW_PR_DUx "\n",
        msg,line,
        hilopc->sawlo_flag,hilopc->lopc);
    printf( "    sawhi:    %d  0x%" DW_PR_XZEROS DW_PR_DUx "\n",
        hilopc->sawhi_flag,hilopc->hival);
    printf( "    havefinal:%d  0x%" DW_PR_XZEROS DW_PR_DUx "\n",
        hilopc->havefinal_flag,hilopc->hifinal);
}
#endif

/*  ASSERT: attrnum == DW_AT_low_pc or DW_AT_high_pc
    This does not print errors as those checks will be
    redone by other related code.
*/
static void
get_pc_value(Dwarf_Debug dbg,
    Dwarf_Attribute attrib,
    Dwarf_Half attrnum,
    Dwarf_Half theform,
    LoHiPc *lohipc,
    Dwarf_Error *err)
{
    int bres = 0;
    int islowaddr =(attrnum == DW_AT_low_pc)?TRUE:FALSE;
    Dwarf_Unsigned uval = 0;
    Dwarf_Addr addr = 0;

    if (!islowaddr && (theform != DW_FORM_addr &&
        !dwarf_addr_form_is_indexed(theform))) {
        /*  New in DWARF4: other forms
        (of class constant) are not an address
        but are instead offset from pc. */
        bres = dwarf_formudata(attrib,&uval,err);
        if (bres == DW_DLV_OK) {
            if (!lohipc->sawhi_flag) {
                lohipc->sawhi_flag = LOHIPC_SAWOFFSET;
                lohipc->hival = uval;
                if (lohipc->sawlo_flag && !lohipc->havefinal_flag ) {
                    lohipc->havefinal_flag = TRUE;
                    lohipc->hifinal = lohipc->lopc +
                        uval;
                }
            }
        } else {
            if (bres == DW_DLV_ERROR) {
                DROP_ERROR_INSTANCE(dbg,bres,*err);
            }
        }
        return;
    }
    bres = dwarf_formaddr(attrib, &addr, err);
    if (bres == DW_DLV_OK) {
        if (islowaddr) {
            lohipc->lopc = addr;
            lohipc->sawlo_flag = TRUE;
            if (lohipc->sawhi_flag ==
                LOHIPC_SAWOFFSET && !lohipc->havefinal_flag) {
                lohipc->havefinal_flag = TRUE;
                lohipc->hifinal = addr + lohipc->hival;
            }
        } else {
            if (!lohipc->sawhi_flag ) {
                lohipc->hival = addr;
                lohipc->sawhi_flag = TRUE;
                lohipc->hifinal = addr;
                lohipc->havefinal_flag = TRUE;
            }
        }
    } else if (bres == DW_DLV_ERROR) {
        DROP_ERROR_INSTANCE(dbg,bres,*err);
    }
}

static void
checking_valid_code(Dwarf_Half tag,
    Dwarf_Unsigned max_address,
    Dwarf_Addr lowaddr,
    Dwarf_Addr highaddr)
{
    if (glflags.need_PU_valid_code) {
        glflags.need_PU_valid_code = FALSE;
        /*  To ignore a PU as invalid code,
            only consider the lowpc and
            highpc values associated with the
            DW_TAG_subprogram; other
            instances of lowpc and highpc,
            must be ignored (lexical blocks) */
        glflags.in_valid_code = TRUE;
        if (IsInvalidCode(lowaddr,highaddr) &&
            tag == DW_TAG_subprogram) {
            glflags.in_valid_code = FALSE;
        }
    }
    /*  We have a low_pc/high_pc pair;
        check if they are valid */
    if (glflags.in_valid_code) {
        DWARF_CHECK_COUNT(ranges_result,1);
        if (lowaddr != max_address &&
            lowaddr > highaddr ) {
            DWARF_CHECK_ERROR(ranges_result,
                ".debug_info: Incorrect values "
                "for low_pc/high_pc");
            if (glflags.gf_check_verbose_mode &&
                PRINTING_UNIQUE) {
                printf("Low = 0x%" DW_PR_XZEROS DW_PR_DUx
                    ", High = 0x%"
                    DW_PR_XZEROS DW_PR_DUx "\n",
                    lowaddr,highaddr);
            }
        }
        if (glflags.gf_check_decl_file ||
            glflags.gf_check_ranges ||
            glflags.gf_check_locations) {
            AddEntryIntoBucketGroup(glflags.pRangesInfo,0,
                lowaddr, lowaddr,highaddr,NULL,FALSE);
        }
    }
}

static void
dd_check_file_etc(Dwarf_Unsigned max_address,
    Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Half tag,
    LoHiPc *lohipc)
{
    Dwarf_Bool had_low_flag = FALSE;
    Dwarf_Addr low_pc = 0;

    if ((glflags.gf_check_decl_file ||
    glflags.gf_check_ranges ||
    glflags.gf_check_locations)) {
        if (glflags.seen_PU &&
            !glflags.seen_PU_base_address
            && lohipc->sawlo_flag) {
            glflags.seen_PU_base_address = TRUE;
            glflags.PU_base_address =  lohipc->lopc;
        }
        if (glflags.seen_PU &&
            !glflags.seen_PU_high_address &&
            lohipc->havefinal_flag) {
            glflags.seen_PU_high_address = TRUE;
            glflags.PU_high_address = lohipc->hifinal;
        }
        /* We have now both low_pc and high_pc values */
        if (lohipc->havefinal_flag && lohipc->sawlo_flag) {
            /*  We need to decide if this PU is
                valid, as the SN Linker marks a stripped
                function by setting lowpc to -1;
                also for discarded comdat, both lowpc
                and highpc are zero */
            Dwarf_Unsigned lowaddr = lohipc->lopc;
            Dwarf_Unsigned highaddr = lohipc->hifinal;
            checking_valid_code(tag,max_address,lowaddr,highaddr);
        }
    }
    if (glflags.gf_check_functions) {
        DWARF_CHECK_COUNT(check_functions_result,1);

        if (lohipc->sawlo_flag) {
            int res = 0;
            Dwarf_Error loclerr = 0;

            res = dwarf_lowpc(die,&low_pc,&loclerr);
            if (res != DW_DLV_OK) {
                DROP_ERROR_INSTANCE(dbg,res,loclerr);
                DWARF_CHECK_ERROR(check_functions_result,
                    "dwarf_lowpc failed but should not have !");
            } else  {
                had_low_flag = TRUE;
                if ( low_pc != lohipc->lopc) {
                    DWARF_CHECK_ERROR(check_functions_result,
                        " dwarf_lowpc return did not match "
                        "The expected dwarfdump value!");
                }
            }
        }
        if (lohipc->sawhi_flag == LOHIPC_SAWOFFSET &&
            lohipc->havefinal_flag &&
            had_low_flag) {
            int res = 0;
            Dwarf_Addr highpc = 0;
            Dwarf_Half form = 0;
            enum Dwarf_Form_Class formclass =DW_FORM_CLASS_UNKNOWN;
            Dwarf_Error loclerr = 0;

            res = dwarf_highpc_b(die,&highpc,
                &form,&formclass, &loclerr);
            if (res != DW_DLV_OK) {
                DROP_ERROR_INSTANCE(dbg,res,loclerr);
                DWARF_CHECK_ERROR(check_functions_result,
                    " dwarf_highpc_b failed but should not have !");
            } else  {
                if (form != DW_FORM_addr &&
                    !dwarf_addr_form_is_indexed(form)) {
                    highpc += low_pc;
                }
                if ( highpc != lohipc->hifinal) {
                    struct esb_s m;
                    char mbuf[100];

                    esb_constructor_fixed(&m,mbuf,sizeof(mbuf));
                    esb_append_printf_u(&m,
                        " dwarf_highpc_b return 0x%"
                        DW_PR_XZEROS DW_PR_DUx
                        " did not match", highpc);
                    esb_append_printf_u(&m,
                        " the expected dwarfdump value 0x%"
                        DW_PR_XZEROS DW_PR_DUx " ",
                        lohipc->hifinal);
                    DWARF_CHECK_ERROR(check_functions_result,
                        esb_get_string(&m));
                    esb_destructor(&m);
                }
            }
        }
    }
}

int
print_hipc_lopc_attribute(Dwarf_Debug dbg,
    Dwarf_Half tag,
    Dwarf_Die die,
    Dwarf_Unsigned dieprint_cu_goffset,
    char ** srcfiles,
    Dwarf_Signed cnt,
    Dwarf_Attribute attrib,
    Dwarf_Half attr,
    Dwarf_Unsigned max_address,
    LoHiPc *lohipc,
    struct esb_s *valname,
    Dwarf_Error *err)
{
    Dwarf_Half theform =0;
    int rv = 0;
    /* For DWARF4, the high_pc offset from the low_pc */
    Dwarf_Unsigned highpcOff = 0;
    char highpcstrbuf[ESB_FIXED_ALLOC_SIZE];
    struct esb_s highpcstr;

    esb_constructor_fixed(&highpcstr,highpcstrbuf,
        sizeof(highpcstrbuf));
    rv = dwarf_whatform(attrib,&theform,err);
    /*  Depending on the form and the attribute,
        process the form. */
    if (rv == DW_DLV_ERROR) {
        print_error_and_continue("in print_attribute "
            "dwarf_whatform cannot"
            " Find attr form",
            rv, *err);
        esb_destructor(&highpcstr);
        return rv;
    } else if (rv == DW_DLV_NO_ENTRY) {
        esb_destructor(&highpcstr);
        return rv;
    }
    get_pc_value(dbg, attrib, attr, theform,lohipc,err);
    /*  Determine if the high pc is really an offset,
        set offset_detected flag if so. */
    if (theform != DW_FORM_addr &&
        !dwarf_addr_form_is_indexed(theform)) {
        /*  New in DWARF4: other forms
            (of class constant) are not an address
            but are instead offset from pc.
            One could test for DWARF4 here
            before adding this string, but that
            seems unnecessary as this
            could not happen with DWARF3 or earlier.
            A normal consumer would have to
            add this value to
            DW_AT_low_pc to get a true pc. */
        esb_append(&highpcstr,"<offset-from-lowpc> ");
        /*  Update the high_pc value if we
            are checking the ranges */
        if ( glflags.gf_check_ranges && attr == DW_AT_high_pc) {
            /* Get the offset value */
            int show_form_here = 0;
            int ares = dd_get_integer_and_name(dbg,
                attrib,
                &highpcOff,
                /* attrname */ (const char *) NULL,
                /* err_string */ ( struct esb_s *) NULL,
                (encoding_type_func) 0,
                err,show_form_here);
            if (ares != DW_DLV_OK) {
                if (ares == DW_DLV_NO_ENTRY) {
                    print_error_and_continue(
                        "dd_get_integer_and_name"
                        " No Entry for DW_AT_high_pc/DW_AT_low_pc",
                        ares, *err);
                } else {
                    print_error_and_continue(
                        "dd_get_integer_and_name"
                        " Failed for DW_AT_high_pc/DW_AT_low_pc",
                        ares, *err);
                }
                esb_destructor(&highpcstr);
                return ares;
            }
        }
    }
    rv = get_attr_value(dbg, tag, die,
        dieprint_cu_goffset,
        attrib, srcfiles, cnt,
        &highpcstr,glflags.show_form_used,
        glflags.verbose,err);
    if (rv == DW_DLV_ERROR) {
        return rv;
    }
    if (lohipc->sawhi_flag == LOHIPC_SAWOFFSET &&
        lohipc->havefinal_flag) {
        esb_append_printf_u(&highpcstr,
            " <highpc: 0x%" DW_PR_XZEROS DW_PR_DUx ">",
            lohipc->hifinal);
    }
    esb_empty_string(valname);
    esb_append(valname, esb_get_string(&highpcstr));
    esb_destructor(&highpcstr);

    /* Update base and high addresses for CU */
    if (glflags.seen_CU &&
        (glflags.need_CU_base_address
        || glflags.need_CU_high_address)) {
        /* updating glflags data for checking/reporting later. */
        update_cu_base_addresses(dbg,attrib,
            attr, tag, lohipc, err);
    }
    dd_check_file_etc(max_address,dbg,die,tag,lohipc);
    esb_destructor(&highpcstr);
    return DW_DLV_OK;
}
