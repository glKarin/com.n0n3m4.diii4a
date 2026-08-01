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

#include <config.h>

#include <stddef.h> /* NULL */
#include <stdlib.h> /* calloc() free() */

#include "dwarf.h"
#include "libdwarf.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_tsearchbal.h"
#include "dd_helpertree.h"
#include "libdwarf_private.h" /* For malloc/calloc debug */

#define TRUE 1
#define FALSE 0
/*  WARNING: the tree walk functions will, if presented **tree
    when *tree is wanted, simply find nothing. No error,
    just bad results. So when a walk produces nothing
    suspect a code mistake here.
    The basic problem is void* is a terrible way to
    pass in a pointer. But it's how tsearch was defined
    long ago.
*/
/*  For .debug_info (not for tied file)  */
struct Helpertree_Base_s helpertree_offsets_base_info;
/*  For .debug_types (not for tied file)  */
struct Helpertree_Base_s helpertree_offsets_base_types;

static struct Helpertree_Map_Entry_s *
helpertree_map_create_entry(Dwarf_Unsigned offset,
    int val)
{
    struct  Helpertree_Map_Entry_s *mp =
        (struct  Helpertree_Map_Entry_s *)
        calloc(1,sizeof(struct  Helpertree_Map_Entry_s));
    if (!mp) {
        return 0;
    }
    mp->hm_key = offset;
    mp->hm_val = val;
    return mp;
}
static void
helpertree_map_free_func(void *mx)
{
    struct  Helpertree_Map_Entry_s *m = mx;
    free(m);
}

static int
helpertree_map_compare_func(const void *l, const void *r)
{
    const struct  Helpertree_Map_Entry_s *ml = l;
    const struct  Helpertree_Map_Entry_s *mr = r;
    if (ml->hm_key < mr->hm_key) {
        return -1;
    }
    if (ml->hm_key > mr->hm_key) {
        return 1;
    }
    return 0;
}
static void
helpertree_map_destroy(void *map)
{
    /* tdestroy is not part of Posix. */
    dwarf_tdestroy(map,helpertree_map_free_func);
}

/* Globally-visible functions follow this line. */

struct Helpertree_Map_Entry_s *
helpertree_add_entry(Dwarf_Unsigned offset,
    int val,struct Helpertree_Base_s *base)
{
    void *retval = 0;
    struct  Helpertree_Map_Entry_s *re = 0;
    struct  Helpertree_Map_Entry_s *e;
    void **tree1 = 0;

    tree1 = &base->hb_base;
    e  = helpertree_map_create_entry(offset,val);
    /*  tsearch records e's contents unless e
        is already present . We must not free it till
        destroy time if it got added to tree1.  */
    retval = dwarf_tsearch(e,tree1, helpertree_map_compare_func);
    if (retval) {
        re = *(struct Helpertree_Map_Entry_s **)retval;
        if (re != e) {
            /*  We returned an existing record, e not needed.
                Set val. */
            re->hm_val = val;
            helpertree_map_free_func(e);
        } else {
            /* Record e got added to tree1, do not free record e. */
        }
        return retval;
    }
    return NULL;
}

struct  Helpertree_Map_Entry_s *
helpertree_find(Dwarf_Unsigned offset,struct Helpertree_Base_s *base)
{
    void *retval = 0;
    struct  Helpertree_Map_Entry_s *re = 0;
    struct  Helpertree_Map_Entry_s *e = 0;

    e = helpertree_map_create_entry(offset,0);
    retval = dwarf_tfind(e,&base->hb_base,
        helpertree_map_compare_func);
    if (retval) {
        re = *(struct  Helpertree_Map_Entry_s **)retval;
    }
    /*  The one we created here must be deleted, it is dead.
        We look at the returned one instead. */
    helpertree_map_free_func(e);
    return re;
}

void
helpertree_clear_statistics(struct Helpertree_Base_s *base)
{
    if (!base) {
        return;
    }
    if (!base->hb_base) {
        return;
    }
    helpertree_map_destroy(base->hb_base);
    base->hb_base = 0;
}
