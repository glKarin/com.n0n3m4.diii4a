/*
Copyright (C) 2000-2006 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright 2007-2010 Sun Microsystems, Inc. All rights reserved.
Portions Copyright 2009-2010 SN Systems Ltd. All rights reserved.
Portions Copyright 2008-2011 David Anderson. All rights reserved.

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
#include <stdio.h> /* FILE decl for dd_esb.h, printf etc */

#include "dwarf.h"
#include "libdwarf.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_naming.h"
#include "dd_esb.h"
#include "print_sections.h"
#include "print_frames.h"

/* The April 2005 dwarf_get_section_max_offsets()
   in libdwarf returns all max-offsets, but we only
   want one of those offsets. This function returns
   the one we want from that set,
   making functions needing this offset as readable as possible.
   (avoiding code duplication).
   In case of error or missing section it returns a zero
   size, which seems appropriate.
*/
Dwarf_Unsigned
get_info_max_offset(Dwarf_Debug dbg)
{
    Dwarf_Unsigned debug_info_size = 0;
    int res = 0;

    res = dwarf_get_section_max_offsets_d(dbg,
        &debug_info_size,0,0,0,0, 0,0,0,0,0,
        0,0,0,0,0,  0,0,0,0,0);
    if (res != DW_DLV_OK) {
        return 0;
    }
    return debug_info_size;
}

/* Dumping a dwarf-expression as a byte stream. */
void
dump_block(char *prefix, char *data, Dwarf_Signed len)
{
    char *end_data = data + len;
    char *cur = data;
    int i = 0;

    printf("%s", prefix);
    for (; cur < end_data; ++cur, ++i) {
        if (i > 0 && i % 4 == 0)
            printf(" ");
        printf("%02x", 0xff & *cur);

    }
}
