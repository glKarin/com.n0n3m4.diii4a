/*
  Copyright 2015-2016 David Anderson. All rights reserved.

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
#ifndef HELPERTREE_H
#define HELPERTREE_H

/*  This is a tsearch tree  interface we may use in various ways
    where each different sort of use is a different
    Helpertree_Base_s instance. */

/*  We create Helpertree_Base_s so we can use type-checked calls,
    not showing the tsearch void* outside of helpertree.c. */
struct Helpertree_Base_s {
    void * hb_base;
};

/* For .debug_info  */
extern struct Helpertree_Base_s helpertree_offsets_base_info;
/* For .debug_types. */
extern struct Helpertree_Base_s helpertree_offsets_base_types;

struct Helpertree_Map_Entry_s {
    /*  Key is offset. It will be a section-global offset so
        applicable across an entire executable/object section. */
    Dwarf_Unsigned hm_key;

    /*  val is something defined differently in different uses.
        for integer type it is
        0 means unknown
        -1 known signed
        1 known unsigned. */
    int            hm_val;
    /* Add fields here as needed. */
};

/*  Add entry or set to known-signed or known-unsigned. */
struct Helpertree_Map_Entry_s *
helpertree_add_entry( Dwarf_Unsigned offset, int  val,
    struct Helpertree_Base_s *helper);

/*  Look for entry. Use hm_val (if non-null return)
    to determine signedness. */
struct Helpertree_Map_Entry_s *
helpertree_find(Dwarf_Unsigned offset,
    struct Helpertree_Base_s *helper);

void helpertree_clear_statistics(struct Helpertree_Base_s *helper);
#endif /* HELPERTREE_H */
