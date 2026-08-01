/*
  Copyright (C) 2021 David Anderson. All Rights Reserved.

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

#ifndef ATTR_FORM_H
#define ATTR_FORM_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AF_UNKNOWN   0 /* also means malloc, so free required */
#define AF_STD       1 /* also means static alloc, do not free */
#define AF_EXTEN     2 /* also means static alloc, do not free */

/*  See also: glflags.gf_check_tag_tree
    all while respecting */

#define TK_OK           0
#define TK_SHOW_MESSAGE 1
#define TK_ERROR        2

/*  For a tag->tag tree key1s a tag number, key 2 is a tag number
    and key3  is zero.
    For a tag->attr tree key1 is a tag number, key 2 is attrnum,
    and key3 is zero.
    for attr-> form tree key1 is DW_AT_ number, key2 is
    FORM_CLASS, key3 is a DW_FORM number. */

struct Three_Key_Entry_s {
    Dwarf_Half      key1;
    Dwarf_Half      key2;
    Dwarf_Half      key3;

    /* Not from tables:0. std table:1. extension table:2. */
    Dwarf_Small    from_tables;
    Dwarf_Small    reserved;
    Dwarf_Unsigned count; /* The number actually encountered
        in print_die.c */
};
typedef struct Three_Key_Entry_s Three_Key_Entry;

/* Returns DW_DLV_ERROR if out of memory, else DW_DLV_OK */
int make_3key(Dwarf_Half k1,
    Dwarf_Half k2,
    Dwarf_Half k3,
    Dwarf_Small from_table,
    Dwarf_Small reserved,
    Dwarf_Unsigned count,
    Three_Key_Entry ** out);

Dwarf_Unsigned three_key_entry_count(void *base);
void free_func_3key_entry(void *keystructptr);
int  std_compare_3key_entry(const void *l, const void *r);

/*   First Calls the next five */
int  dd_build_tag_attr_form_base_trees(int*errnum);

void dd_destroy_tag_use_base_tree(void);
void dd_destroy_tag_attr_form_trees(void);
void record_attr_form_use(Dwarf_Debug dbg,
    Dwarf_Half tag, Dwarf_Half attr,
    Dwarf_Half fclass, Dwarf_Half form,
    int die_stack_indent_level);

/*  The standard main tree for attr_form data.
    Starting out as simple global variables. */
extern void * threekey_attr_form_base; /* for attr-form combos */
void print_attr_form_usage(void);

void record_tag_tag_use(Dwarf_Debug dbg,
    Dwarf_Half tagp, Dwarf_Half tagc);
void record_tag_attr_use(Dwarf_Debug dbg,
    Dwarf_Half tag, Dwarf_Half attrnum);

/*  The standard main trees for tag and attr use
    Starting out as simple global variables. */
extern void * threekey_tag_tag_base; /* tag-tree recording */
extern void * threekey_tag_attr_base; /* for tag_attr recording */
extern void * threekey_tag_use_base; /* for tag count recording */
/* The following two print all from the above trees.*/
/* FIXME and printing tag_use_base??? */
void print_tag_tree_usage(void);
void print_tag_attr_usage(void);

void dd_print_tag_tree_results(Dwarf_Unsigned tag_tag_count);
void dd_print_tag_attr_results(Dwarf_Unsigned tag_attr_count);
void dd_print_tag_use_results(Dwarf_Unsigned tag_count);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* ATTR_FORM_H */
