/*
  Copyright 2020 David Anderson. All Rights Reserved.

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

#include <stdlib.h> /* free() malloc() */
#include <string.h> /* memcpy() strdup() */

#include "dwarf.h"
#include "libdwarf.h"
#include "dwarf_base_types.h"
#include "libdwarfp.h"
#include "dwarf_pro_incl.h"
#include "dwarf_pro_opaque.h"
#include "dwarf_pro_error.h"
#include "dwarf_pro_alloc.h"

int
dwarf_add_debug_sup(Dwarf_P_Debug dbg,
    Dwarf_Half      version,
    Dwarf_Small     is_supplementary,
    char          * filename,
    Dwarf_Unsigned  checksum_len,
    Dwarf_Small   * checksum,
    Dwarf_Error * error)
{
    dbg->de_debug_sup.ds_version = version;
    dbg->de_debug_sup.ds_is_supplementary = is_supplementary;
    dbg->de_debug_sup.ds_filename = strdup(filename);
    dbg->de_debug_sup.ds_checksum_len = checksum_len;
    dbg->de_debug_sup.ds_checksum = malloc(checksum_len);
    if (!dbg->de_debug_sup.ds_checksum) {
        free(dbg->de_debug_sup.ds_filename);
        dbg->de_debug_sup.ds_filename = 0;
        dbg->de_debug_sup.ds_version = 0;
        dbg->de_debug_sup.ds_checksum_len = 0;
        _dwarf_p_error(dbg, error, DW_DLE_ALLOC_FAIL);
        return DW_DLV_ERROR;
    }
    memcpy(dbg->de_debug_sup.ds_checksum,checksum,checksum_len);
    return DW_DLV_OK;
}
