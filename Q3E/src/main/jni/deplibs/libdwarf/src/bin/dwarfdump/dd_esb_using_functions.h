/*
Copyright (C) 2000,2004,2005 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright (C) 2007-2020 David Anderson. All Rights Reserved.
Portions Copyright 2012-2018 SN Systems Ltd. All rights reserved.

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

#ifndef ESB_USING_FUNCTIONS_H
#define ESB_USING_FUNCTIONS_H
#ifdef __cplusplus
extern "C" {
#endif

/* Always returns DW_DLV_OK */
int print_ranges_list_to_extra(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Unsigned originaloff,
    Dwarf_Unsigned finaloff,
    Dwarf_Ranges *rangeset,
    Dwarf_Signed rangecount,
    Dwarf_Unsigned bytecount,
    struct esb_s *stringbuf);

int get_producer_name(Dwarf_Debug dbg,Dwarf_Die cu_die,
    Dwarf_Off  dieprint_cu_offset,
    struct esb_s *producername,Dwarf_Error *err);

int get_attr_value(Dwarf_Debug dbg, Dwarf_Half tag,
    Dwarf_Die die,
    Dwarf_Off die_cu_offset,
    Dwarf_Attribute attrib,
    char **srcfiles, Dwarf_Signed srcfiles_count,
    struct esb_s *esbp,
    int show_form,int local_verbose,
    Dwarf_Error *err);

void format_sig8_string(Dwarf_Sig8 *data,struct esb_s *out);

int dwarfdump_print_expression_operations(Dwarf_Debug dbg,
    Dwarf_Die die,
    int die_indent_level,
    Dwarf_Locdesc_c  locs, /* 2015 interface */
    Dwarf_Unsigned entrycount, /* count of DW_OP operators */
    struct esb_s *string_out,
    Dwarf_Error *err);

int get_true_section_name(Dwarf_Debug dbg,
    const char *standard_name,
    struct esb_s *name_out,
    Dwarf_Bool add_compr);

#ifdef __cplusplus
}
#endif
#endif /* ESi_USING_FUNCTIONS_H */
