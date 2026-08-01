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
#include "dd_helpertree.h"
#include "dd_tag_common.h"

/* Prints locentry descriptions for DW_LKIND_loclist */

int
print_original_loclist_linecodes(Dwarf_Bool    checking,
    const char *  tagname,
    const char *  attrname,
    unsigned int  llent,
    Dwarf_Small   lle_value,
    Dwarf_Addr    base_address,
    Dwarf_Addr    rawlopc,
    Dwarf_Addr    rawhipc,
    Dwarf_Bool    debug_addr_unavailable,
    Dwarf_Addr    lopc,
    Dwarf_Addr    hipc,
    Dwarf_Unsigned locdesc_offset,
    struct esb_s * esbp,
    Dwarf_Bool   * bError)
{
    switch(lle_value) {
    case DW_LLE_base_address:
        esb_append_printf_u(esbp,
            "<base address       0x%"
            DW_PR_XZEROS DW_PR_DUx
            ">",
            hipc);
        break;
    case DW_LLE_end_of_list:
        /* Nothing to do. */
        esb_append(esbp,"<end-of-list>");
        break;
    case DW_LLE_offset_pair:
        {
            if (glflags.verbose) {
                esb_append_printf_u(esbp,
                    "<DW_LLE_offset_pair 0x%"
                    DW_PR_XZEROS DW_PR_DUx,rawlopc);
                esb_append_printf_u(esbp,
                    " , 0x%"
                    DW_PR_XZEROS DW_PR_DUx
                    ">",rawhipc);
                esb_append_printf_i(esbp, "\n   [%2d]",llent);
            }
            esb_append_printf_u(esbp,
                "<low,hi addrs       0x%"
                DW_PR_XZEROS DW_PR_DUx, lopc);
            esb_append_printf_u(esbp,
                " , 0x%"
                DW_PR_XZEROS DW_PR_DUx
                ">", hipc);
            if (checking && !debug_addr_unavailable) {
                loc_error_check(tagname,attrname,
                    lopc, hipc, locdesc_offset,
                    base_address,
                    bError);
            }
        }
        break;
    case DW_LLE_start_end:
        /* debug_addr_unavailable does not apply here */
        esb_append_printf_u(esbp,
            "<start,end            0x%"
            DW_PR_XZEROS DW_PR_DUx,lopc);
        esb_append_printf_u(esbp,
            " 0x%"
            DW_PR_XZEROS DW_PR_DUx
            ">",hipc);
        if (checking && !debug_addr_unavailable) {
            loc_error_check(tagname,attrname,
                lopc, hipc, locdesc_offset, base_address,
                bError);
        }
        break;

    default: {
        struct esb_s unexp;

        esb_constructor(&unexp);
        esb_append_printf_u(&unexp,
            "ERROR: Unexpected LLE code 0x%x"
            " in original loclist (synthesized code error)",
            lle_value);
        print_error_and_continue(esb_get_string(&unexp),
            DW_DLV_OK, 0);
        esb_destructor(&unexp);
        }
        break;
    }
    return DW_DLV_OK;
}
