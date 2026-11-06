/*
  Copyright (C) 2000,2004 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright 2002-2010 Sun Microsystems, Inc. All rights reserved.
  Portions Copyright 2011-2020 David Anderson. All rights reserved.

  This program is free software; you can redistribute it
  and/or modify it under the terms of version 2.1 of the
  GNU Lesser General Public License as published by the Free
  Software Foundation.

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

  You should have received a copy of the GNU Lesser General
  Public License along with this program; if not, write the
  Free Software Foundation, Inc., 51 Franklin Street - Fifth
  Floor, Boston MA 02110-1301, USA.

*/

#include <config.h>

#include <stdlib.h> /* free() */

#include "dwarf.h"
#include "libdwarf.h"
#include "dwarf_base_types.h"
#include "libdwarfp.h"
#include "dwarf_pro_incl.h"
#include "dwarf_pro_opaque.h"
#include "dwarf_pro_error.h"
#include "dwarf_pro_alloc.h"

/*  This routine deallocates all memory, and does some
    finishing up.  New September 2016. */
int
dwarf_producer_finish_a(Dwarf_P_Debug dbg, Dwarf_Error * error)
{
    if (dbg->de_version_magic_number != PRO_VERSION_MAGIC) {
        DWARF_P_DBG_ERROR(dbg, DW_DLE_IA, DW_DLV_ERROR);
    }

    /* this frees all blocks, then frees dbg. */
    free(dbg->de_debug_sup.ds_filename);
    dbg->de_debug_sup.ds_filename = 0;
    dbg->de_debug_sup.ds_version = 0;
    free(dbg->de_debug_sup.ds_checksum);
    dbg->de_debug_sup.ds_checksum = 0;
    dbg->de_debug_sup.ds_checksum_len = 0;
    _dwarf_p_dealloc_all(dbg);
    return DW_DLV_OK ;
}

/* FIXME: Add stats for debug_line_str. */
int
dwarf_pro_get_string_stats(Dwarf_P_Debug dbg,
    Dwarf_Unsigned * str_count,
    Dwarf_Unsigned * str_total_length,
    Dwarf_Unsigned * strp_count_debug_str,
    Dwarf_Unsigned * strp_len_debug_str,
    Dwarf_Unsigned * strp_reused_count,
    Dwarf_Unsigned * strp_reused_len,
    Dwarf_Error    * error)
{
    struct Dwarf_P_Str_stats_s* ps = 0;
    if (!dbg) {
        _dwarf_p_error(dbg, error, DW_DLE_IA);
        return DW_DLV_ERROR;
    }
    if (dbg->de_version_magic_number !=PRO_VERSION_MAGIC ) {
        _dwarf_p_error(dbg, error, DW_DLE_VMM);
        return DW_DLV_ERROR;
    }
    *str_count        = dbg->de_stats.ps_str_count;
    *str_total_length = dbg->de_stats.ps_str_total_length;
    ps = &dbg->de_stats.ps_strp;
    *strp_count_debug_str = ps->ps_strp_count_debug_str;
    *strp_len_debug_str   = ps->ps_strp_len_debug_str;
    *strp_reused_count    = ps->ps_strp_reused_count;
    *strp_reused_len      = ps->ps_strp_reused_len;
    return DW_DLV_OK;
}
