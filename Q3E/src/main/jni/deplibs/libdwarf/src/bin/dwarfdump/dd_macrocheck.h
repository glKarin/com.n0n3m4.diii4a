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
#ifndef MACROCHECK_H
#define MACROCHECK_H

/* tsearch trees used in macro checking. */
extern void * macro_check_tree; /* DWARF5 macros. */
extern void * macinfo_check_tree; /* DWARF2,3,4 macros */
extern void * macdefundeftree; /* DWARF5 style macros */

struct Macrocheck_Map_Entry_s {
    Dwarf_Unsigned mp_key; /* Key is offset */
    Dwarf_Unsigned mp_len; /* len in bytes off this macro set */

    /*  We count number of uses. More than 1 primary is an error.
        Both primary and secondary is ok or error?  */
    Dwarf_Unsigned mp_refcount_primary;
    Dwarf_Unsigned mp_refcount_secondary;
    Dwarf_Unsigned mp_import_linenum;
    unsigned       mp_import_from_filenum;

    /* So we go through each one just once. */
    Dwarf_Bool     mp_printed;
};
/* for def/undef checking. */
struct macdef_entry_s {
    char * md_key; /* Tacked on this record. Do not free */
    unsigned md_operatornum;
    unsigned int md_defined;
    unsigned int md_undefined;
    unsigned int md_defcount;
    unsigned int md_undefcount;
    unsigned md_operator;
    Dwarf_Unsigned md_line;
    Dwarf_Unsigned md_offset;
    Dwarf_Unsigned md_macro_unit_offset;
    char * md_string; /* see create entry. Do not free */
    unsigned md_file_array_entry;
};
typedef struct macdef_entry_s macdef_entry;

/* for start-file end-file checking. */
struct macfile_entry_s {
    unsigned ms_operatornum;
    unsigned ms_operator;
    Dwarf_Unsigned ms_line;
    Dwarf_Unsigned ms_filenum;
    Dwarf_Unsigned ms_offset;
    Dwarf_Unsigned ms_macro_unit_offset;
    unsigned ms_array_number; /* position in macfile_array */
    char * ms_filename;
};
typedef struct macfile_entry_s macfile_entry;
#define MACFILE_STACK_DEPTH_MAX 50 /* Arbitrary. Make bigger? */
extern unsigned macfile_stack_next_to_use;
extern unsigned macfile_stack[MACFILE_STACK_DEPTH_MAX+1];
extern unsigned macfile_stack_max_seen;

#define MACRO_IMPORT_STACK_DEPTH 20 /* Arbitrary. Make bigger? */
extern Dwarf_Unsigned macro_import_stack[MACRO_IMPORT_STACK_DEPTH +1];
extern unsigned macro_import_stack_next_to_use;
extern unsigned macro_import_stack_max_seen;
/*  Returns DW_DLV_ERROR if the push could not done,
    which would be because full.
    Else returns DW_DLV_OK.  */
int macro_import_stack_push(Dwarf_Unsigned offset);

/*  Returns DW_DLV_ERROR if the pop could not done,
    else returns DW_DLV_OK.  */
int macro_import_stack_pop(void);

/*  Returns DW_DLV_OK if offset is in the stack or
    DW_DLV_NO_ENTRY if it is not in the stack. */
int macro_import_stack_present(Dwarf_Unsigned offset);

void macro_import_stack_cleanout(void);
void print_macro_import_stack(void);

struct Macrocheck_Map_Entry_s * macrocheck_map_find(
    Dwarf_Unsigned offset,
    void **map);
void add_macro_import(void **base,Dwarf_Bool is_primary,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned linenum,
    unsigned mafile_file_number);
void add_macro_area_len(void **base, Dwarf_Unsigned offset,
    Dwarf_Unsigned len);

void print_split_macro_value(const char *name_in);

int get_next_unprinted_macro_offset(void **base,
    Dwarf_Unsigned * off);
void mark_macro_offset_printed(void **base, Dwarf_Unsigned offset);

int print_macrocheck_statistics(const char *name,void **basep,
    int isdwarf5,
    Dwarf_Unsigned section_size);
void clear_macrocheck_statistics(void **basep);

macfile_entry * macfile_from_array_index( unsigned index);

#endif /* MACROCHECK_H */
