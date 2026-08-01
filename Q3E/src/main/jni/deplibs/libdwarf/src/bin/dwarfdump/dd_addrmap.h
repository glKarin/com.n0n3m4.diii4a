/*
  Copyright 2010 David Anderson. All rights reserved.

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

#ifndef ADDRMAP_H
#define ADDRMAP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct Addr_Map_Entry {
    Dwarf_Unsigned mp_key;
    char * mp_name;
};

struct Addr_Map_Entry * addr_map_insert(Dwarf_Unsigned addr,
    char *name, void **map);
struct Addr_Map_Entry * addr_map_find(Dwarf_Unsigned addr,
    void **map);
void addr_map_destroy(void *map);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADDRMAP_H */
