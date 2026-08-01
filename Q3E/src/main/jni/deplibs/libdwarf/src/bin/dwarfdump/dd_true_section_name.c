/*
  Copyright 2018-2018 David Anderson. All rights reserved.

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
/*  We've tried stick to C90, but a newish clang on
    MacOS deprecates sprintf. So we use snprintf as
    of 2024 v0.11.1 */
#include <stdio.h> /* FILE decl for dd_esb.h,snprintf  */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_esb.h"
#include "dd_esb_using_functions.h"

int get_true_section_name(Dwarf_Debug dbg,
    const char *standard_name,
    struct esb_s *name_out,
    Dwarf_Bool add_compr)
{
    Dwarf_Small marked_compressed = 0;
    Dwarf_Small marked_zlib_compressed = 0;
    Dwarf_Small marked_shf_compressed = 0;
    Dwarf_Unsigned compressed_length = 0;
    Dwarf_Unsigned uncompressed_length = 0;
    const char *stdname =  standard_name;
    const char *actualname = 0;
    int cres = 0;
    Dwarf_Error tnameerr = 0;

    cres = dwarf_get_real_section_name(dbg,
        stdname, &actualname, &marked_compressed,
        &marked_zlib_compressed,
        &marked_shf_compressed,
        &compressed_length,&uncompressed_length,
        &tnameerr);
    if (cres == DW_DLV_OK) {
        esb_append(name_out,actualname);
        if (add_compr) {
            Dwarf_Bool compr = FALSE;
            if (marked_compressed) {
                esb_append(name_out," .zdebug");
                compr = TRUE;
            }
            if (marked_zlib_compressed) {
                esb_append(name_out," ZLIB-initial-bytes");
                compr = TRUE;
            }
            if (marked_shf_compressed) {
                esb_append(name_out," SHF_COMPRESSED");
                compr = TRUE;
            }
            if (compr) {
                char floatbuf[40];
                double comprfactor = 0.0;
                if (compressed_length > 0) {
                    comprfactor = (double)uncompressed_length /
                        (double)compressed_length;
                }
                esb_append_printf_u(name_out," CompLen=%"
                    DW_PR_DUu, compressed_length);
                esb_append_printf_u(name_out," Uncomp=%"
                    DW_PR_DUu, uncompressed_length);
                snprintf(floatbuf,sizeof(floatbuf),
                    " compression=%.1f",comprfactor);
                esb_append(name_out, floatbuf);
            }
        }
        return DW_DLV_OK;
    }
    if (cres == DW_DLV_NO_ENTRY) {
        esb_append(name_out,stdname);
        return cres;
    }
    /* DW_DLV_ERROR */
    esb_append(name_out,stdname);
    /* avoid error leak */
    dwarf_dealloc(dbg,tnameerr,DW_DLA_ERROR);
    tnameerr = 0;
    esb_append(name_out," (Error accessing section name)");
    return DW_DLV_OK;
}
