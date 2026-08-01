/*
  Copyright 2010-2012 David Anderson. All rights reserved.
  Portions Copyright 2012 SN Systems Ltd. All rights reserved.

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

/*  If memory full  we do not exit, we just keep going as if
    all were well. */

#include <config.h>

#include <stddef.h> /* NULL */
#include <stdlib.h> /* free() malloc() */
#include <string.h> /* strdup() */

/* Windows specific header files */
#if defined(_WIN32) && defined(HAVE_STDAFX_H)
#include "stdafx.h"
#endif /* HAVE_STDAFX_H */

#include "dwarf.h"
#include "libdwarf.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_addrmap.h"
#include "dd_tsearchbal.h"
#include "libdwarf_private.h" /* For malloc/calloc debug */

static struct Addr_Map_Entry *
addr_map_create_entry(Dwarf_Unsigned k,char *name)
{
    struct Addr_Map_Entry *mp =
        (struct Addr_Map_Entry *)malloc(
        sizeof(struct Addr_Map_Entry));
    if (!mp) {
        return 0;
    }
    mp->mp_key = k;
    if (name) {
        /* Might be zero if malloc fails. Ok. */
        mp->mp_name = (char *)strdup(name);
    } else {
        mp->mp_name = 0;
    }
    return mp;
}
static void
addr_map_free_func(void *mx)
{
    struct Addr_Map_Entry *m = mx;
    if (!m) {
        return;
    }
    free(m->mp_name);
    m->mp_name = 0;
    free(m);
    return;
}

static int
addr_map_compare_func(const void *l, const void *r)
{
    const struct Addr_Map_Entry *ml = l;
    const struct Addr_Map_Entry *mr = r;
    if (ml->mp_key < mr->mp_key) {
        return -1;
    }
    if (ml->mp_key > mr->mp_key) {
        return 1;
    }
    return 0;
}
struct Addr_Map_Entry *
addr_map_insert( Dwarf_Unsigned addr,char *name,void **tree1)
{
    void *retval = 0;
    struct Addr_Map_Entry *re = 0;
    struct Addr_Map_Entry *e;
    e  = addr_map_create_entry(addr,name);
    /*  tsearch records e's contents unless e
        is already present . We must not free it till
        destroy time if it got added to tree1.  */
    retval = dwarf_tsearch(e,tree1, addr_map_compare_func);
    if (retval) {
        re = *(struct Addr_Map_Entry **)retval;
        if (re != e) {
            /* We returned an existing record, e not needed. */
            addr_map_free_func(e);
        } else {
            /* Record e got added to tree1, do not free record e. */
        }
    }
    return re;
}
struct Addr_Map_Entry *
addr_map_find(Dwarf_Unsigned addr,void **tree1)
{
    void *retval = 0;
    struct Addr_Map_Entry *re = 0;
    struct Addr_Map_Entry *e = 0;

    e = addr_map_create_entry(addr,NULL);
    retval = dwarf_tfind(e,tree1, addr_map_compare_func);
    if (retval) {
        re = *(struct Addr_Map_Entry **)retval;
    }
    /*  The one we created here must be deleted, it is dead.
        We look at the returned one instead. */
    addr_map_free_func(e);
    return re;
}

void
addr_map_destroy(void *map)
{
    /* tdestroy is not part of Posix, it is a GNU libc function. */
    if (map) {
        dwarf_tdestroy(map,addr_map_free_func);
    }
}
