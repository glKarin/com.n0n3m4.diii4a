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

/*  macrocheck.c, .h  create a map of macro import
    operators to establish that they are resolved and
    that their offsets refer to actual macro groups.
    This is across all CUs in the object file,
    not per-cu..

    DEFECTS: There are some things done here that are
    not correct and need fixing at some point.
    See macfile_stack, macro_import_stack,
    macro_check_tree and macinfo_check_tree here
    and in print_macro.c and in print_die.c
    print_macro.c, print_die.c, and macrocheck.c
    work together in checking DWARF macro data.

    A)  macro_import_stack and macfile_stack are global
        to dwarfdump and therefore these can only be safely
        used with DWARF5-style .debug_macro data. Not
        with .debug_macinfo.
    B)  To support .debug_macro_sup in a separate object
        will likely require moving all the macro data
        out of global and into something per-open-dbg.
*/

#include <config.h>

#include <stdlib.h> /* calloc() free() */
#include <stdio.h> /* FILE decl for dd_esb.h */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_tsearchbal.h"
#include "dd_macrocheck.h"
#include "dd_esb.h"

/*  WARNING: the tree walk functions will, if presented **tree
    when *tree is wanted, simply find nothing. No error,
    just bad results. So when a walk produces nothing
    suspect a code mistake here.
    The basic problem is void* is a terrible way to
    pass in a pointer. But it's how tsearch was defined
    long ago.
*/

Dwarf_Unsigned macro_import_stack[MACRO_IMPORT_STACK_DEPTH +1];
unsigned macro_import_stack_next_to_use;
unsigned macro_import_stack_max_seen;

unsigned macfile_stack_next_to_use = 0;
unsigned macfile_stack[MACFILE_STACK_DEPTH_MAX+1];
unsigned macfile_stack_max_seen;

void *  macro_check_tree;   /* DWARF5 macros */
void *  macinfo_check_tree; /* DWARF 2,3,4 macros */
void *  macdefundeftree;    /* DWARF5 macros */

static struct Macrocheck_Map_Entry_s * macrocheck_map_insert
(
    Dwarf_Unsigned off,
    Dwarf_Unsigned prim,
    Dwarf_Unsigned sec,
    Dwarf_Unsigned linenum,
    unsigned src_file_num,
    void **map);
static void macrocheck_map_destroy(void *map);
static Dwarf_Unsigned macro_count_recs(void **base);

#ifdef TESTING
int failcount = 0;
struct glflags_s glflags;
#endif /* TESTING */

static struct Macrocheck_Map_Entry_s *
macrocheck_map_create_entry(Dwarf_Unsigned offset,
    Dwarf_Unsigned add_primary, Dwarf_Unsigned add_secondary)
{
    struct Macrocheck_Map_Entry_s *mp =
        (struct Macrocheck_Map_Entry_s *)
        calloc(1,sizeof(struct Macrocheck_Map_Entry_s));
    if (!mp) {
        return 0;
    }
    mp->mp_key = offset;
    mp->mp_len = 0;
    mp->mp_printed = 0;
    mp->mp_refcount_primary = add_primary;
    mp->mp_refcount_secondary = add_secondary;
    return mp;
}
static void
macrocheck_map_free_func(void *mx)
{
    struct Macrocheck_Map_Entry_s *m = mx;
    free(m);
}

static int
macrocheck_map_compare_func(const void *l, const void *r)
{
    const struct Macrocheck_Map_Entry_s *ml = l;
    const struct Macrocheck_Map_Entry_s *mr = r;
    if (ml->mp_key < mr->mp_key) {
        return -1;
    }
    if (ml->mp_key > mr->mp_key) {
        return 1;
    }
    return 0;
}

static struct Macrocheck_Map_Entry_s *
macrocheck_map_insert(Dwarf_Unsigned offset,
    Dwarf_Unsigned add_prim,Dwarf_Unsigned add_sec,
    Dwarf_Unsigned line_num,
    unsigned src_file_num,
    void **tree1)
{
    void *retval = 0;
    struct Macrocheck_Map_Entry_s *re = 0;
    struct Macrocheck_Map_Entry_s *e = 0;

    e  = macrocheck_map_create_entry(offset,add_prim,add_sec);
    /*  tsearch records e's contents unless e
        is already present . We must not free it till
        destroy time if it got added to tree1.  */
    retval = dwarf_tsearch(e,tree1, macrocheck_map_compare_func);
    if (retval) {
        re = *(struct Macrocheck_Map_Entry_s **)retval;
        if (re != e) {
            /*  We returned an existing record, e not needed.
                Increment refcounts. Lets update line, filenum
                to latest. So later reports show latest...?  */
            re->mp_import_linenum = line_num;
            re->mp_import_from_filenum = src_file_num;
            re->mp_refcount_primary += add_prim;
            re->mp_refcount_secondary += add_sec;
            macrocheck_map_free_func(e);
        } else {
            /* New record */
            e->mp_import_linenum = line_num;
            e->mp_import_from_filenum = src_file_num;
            /* Record e got added to tree1, do not free record e. */
        }
    }
    return NULL;
}

struct Macrocheck_Map_Entry_s *
macrocheck_map_find(Dwarf_Unsigned offset,void **tree1)
{
    void *retval = 0;
    struct Macrocheck_Map_Entry_s *re = 0;
    struct Macrocheck_Map_Entry_s *e = 0;

    e = macrocheck_map_create_entry(offset,0,0);
    retval = dwarf_tfind(e,tree1, macrocheck_map_compare_func);
    if (retval) {
        re = *(struct Macrocheck_Map_Entry_s **)retval;
    }
    /*  The one we created here must be deleted, it is dead.
        We look at the returned one instead. */
    macrocheck_map_free_func(e);
    return re;
}

static void
macrocheck_map_destroy(void *map)
{
    /* tdestroy is not part of Posix. */
    dwarf_tdestroy(map,macrocheck_map_free_func);
}

void
add_macro_import(void **base,
    Dwarf_Bool is_primary,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned line_num,
    unsigned src_filenum)
{
    Dwarf_Unsigned prim_count  = 0;
    Dwarf_Unsigned sec_count  = 0;

    if (is_primary) {
        prim_count = 1;
    } else {
        sec_count = 1;
    }
    macrocheck_map_insert(offset,prim_count,sec_count,
        line_num,src_filenum,base);
}

void
add_macro_area_len(void **base, Dwarf_Unsigned offset,
    Dwarf_Unsigned len)
{
    struct Macrocheck_Map_Entry_s *re = 0;

    re = macrocheck_map_find(offset,base);
    if (re) {
        re->mp_len = len;
    }
}

static Dwarf_Unsigned reccount = 0;

static void
macro_walk_count_recs(const void *nodep,
    const DW_VISIT which,
    const int depth)
{
    (void)nodep;
    (void)depth;
    if (which == dwarf_postorder || which == dwarf_endorder) {
        return;
    }
    reccount += 1;
}
static Dwarf_Unsigned
macro_count_recs(void **base)
{
    reccount = 0;
    dwarf_twalk(*base,macro_walk_count_recs);
    return reccount;
}

static Dwarf_Unsigned lowestoff = 0xffffff;
static Dwarf_Bool lowestfound = FALSE;
static void
macro_walk_find_lowest(const void *nodep,const DW_VISIT  which,
    const int  depth)
{
    struct Macrocheck_Map_Entry_s * re =
        *(struct Macrocheck_Map_Entry_s**)nodep;

    (void)depth;
    if (which == dwarf_postorder || which == dwarf_endorder) {
        return;
    }
    if (!re->mp_printed) {
        if (!lowestfound) {
            lowestoff = re->mp_key;
            lowestfound = TRUE;
        } else if (re->mp_key <= lowestoff) {
            lowestoff = re->mp_key;
        }
    }
}

/* Never returns DW_DLV_ERROR */
int
get_next_unprinted_macro_offset(void **tree, Dwarf_Unsigned * off)
{
    lowestfound = FALSE;
    lowestoff = 0xffffffff;

    /*  This walks the tree to find one entry.
        Which could get slow if the tree has lots of entries. */
    dwarf_twalk(*tree,macro_walk_find_lowest);
    if (!lowestfound) {
        return DW_DLV_NO_ENTRY;
    }
    *off = lowestoff;
    return DW_DLV_OK;
}

void
mark_macro_offset_printed(void **base, Dwarf_Unsigned offset)
{
    struct Macrocheck_Map_Entry_s *re = 0;

    re = macrocheck_map_find(offset,base);
    if (re) {
        re->mp_printed = TRUE;
    }
}

static struct Macrocheck_Map_Entry_s **mac_as_array = 0;
static unsigned mac_as_array_next = 0;
static void
macro_walk_to_array(const void *nodep,const DW_VISIT  which,
    const int  depth)
{
    struct Macrocheck_Map_Entry_s * re =
        *(struct Macrocheck_Map_Entry_s**)nodep;

    (void)depth;
    if (which == dwarf_postorder || which == dwarf_endorder) {
        return;
    }
    mac_as_array[mac_as_array_next] = re;
    mac_as_array_next++;
}

static int
qsort_compare(const void *lin, const void *rin)
{
    const struct Macrocheck_Map_Entry_s *l =
        *(const struct Macrocheck_Map_Entry_s **)lin;
    const struct Macrocheck_Map_Entry_s *r =
        *(const struct Macrocheck_Map_Entry_s **)rin;
    if (l->mp_key < r->mp_key) {
        return -1;
    }
    if (l->mp_key > r->mp_key) {
        return 1;
    }
    if (l->mp_len < r->mp_len) {
        return -1;
    }
    if (l->mp_len > r->mp_len) {
        return 1;
    }
    return 0;
}

static void
warnprimeandsecond(struct Macrocheck_Map_Entry_s *r)
{
#ifdef TESTING
        ++failcount;
#endif
    glflags.gf_count_major_errors++;
    printf("\nERROR: For offset "
        "0x%" DW_PR_XZEROS DW_PR_DUx
        " %" DW_PR_DUu
        " there is a nonzero primary count of "
        "0x%"  DW_PR_XZEROS DW_PR_DUx
        " %" DW_PR_DUu
        " with a secondary count of "
        "0x%"  DW_PR_XZEROS DW_PR_DUx
        " %" DW_PR_DUu
        "\n",
        r->mp_key,
        r->mp_key,
        r->mp_refcount_primary,
        r->mp_refcount_primary,
        r->mp_refcount_secondary,
        r->mp_refcount_secondary);
}

int
print_macrocheck_statistics(const char *name,void **tsbase,
    int isdwarf5,
    Dwarf_Unsigned section_size)
{
    Dwarf_Unsigned count = 0;
    Dwarf_Unsigned lowest = (Dwarf_Unsigned)-1ll;
    Dwarf_Unsigned highest = 0;
    Dwarf_Unsigned lastend = 0;
    Dwarf_Unsigned laststart = 0;
    Dwarf_Unsigned internalgap = 0;
    Dwarf_Unsigned wholegap = 0;
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned end = 0;

    if (! *tsbase) {
        return DW_DLV_NO_ENTRY;
    }
    count = macro_count_recs(tsbase);
    if (count < 1) {
        return DW_DLV_NO_ENTRY;
    }
    free(mac_as_array);
    mac_as_array = 0;
    mac_as_array_next = 0;
    mac_as_array = (struct Macrocheck_Map_Entry_s **)calloc(count,
        sizeof(struct Macrocheck_Map_Entry_s *));
    if (!mac_as_array) {
#ifdef TESTING
        ++failcount;
#endif
        glflags.gf_count_major_errors++;
        printf("\nERROR:  Macro checking %s: "
            "unable to allocate %" DW_PR_DUu "pointers\n",
            name,
            count);
        /*  Return OK so dwarfdump.c won't look for Dwarf_Error */
        return DW_DLV_OK;
    }
    dwarf_twalk(*tsbase,macro_walk_to_array);
    printf("  Macro unit count %s: %" DW_PR_DUu "\n",name,count);
    qsort(mac_as_array,
        count,sizeof(struct Macrocheck_Map_Entry_s *),
        qsort_compare);
    for (i = 0; i < count ; ++i) {
        struct Macrocheck_Map_Entry_s *r = mac_as_array[i];
        if (r->mp_key < lowest) {
            lowest = r->mp_key;
        }
        end = r->mp_key + r->mp_len;
        if (end > highest) {
            highest = end;
        }
        if (r->mp_refcount_primary > 1) {
#ifdef TESTING
            ++failcount;
#endif
            glflags.gf_count_major_errors++;
            printf("\nERROR: For offset 0x%" DW_PR_XZEROS DW_PR_DUx
                " %" DW_PR_DUu
                " there is a primary reference count of "
                "0x%"  DW_PR_XZEROS DW_PR_DUx
                " %"  DW_PR_DUu "\n",
                r->mp_key,
                r->mp_key,
                r->mp_refcount_primary,
                r->mp_refcount_primary);
        }
        /*  For DWARF5 style macros (in .debug_macro)
            having both counts
            is normal. Not so for DWARF2 .debug_macinfo.  */
        if (!isdwarf5 && r->mp_refcount_primary &&
            r->mp_refcount_secondary) {
            warnprimeandsecond(r);
        }
    }
    lastend =
        mac_as_array[0]->mp_key +
        mac_as_array[0]->mp_len;
    laststart = mac_as_array[0]->mp_key;
    printf("  Macro Offsets start at 0x%" DW_PR_XZEROS DW_PR_DUx
        " and end at 0x%" DW_PR_XZEROS DW_PR_DUx "\n",
        lowest, highest);
    for (i = 1; i < count ; ++i) {
        struct Macrocheck_Map_Entry_s *r = mac_as_array[i];
        if (r->mp_key > lastend) {
            internalgap += (r->mp_key - lastend);
        } else if (r->mp_key < lastend) {
            /* crazy overlap */
#ifdef TESTING
            ++failcount;
#endif
            glflags.gf_count_major_errors++;
            printf(" ERROR: For offset 0x%" DW_PR_XZEROS DW_PR_DUx
                " %" DW_PR_DUu
                " there is a crazy overlap with the previous "
                "end offset of "
                "0x%"  DW_PR_XZEROS DW_PR_DUx
                " %"  DW_PR_DUu
                " (previous start offset "
                "of 0x%" DW_PR_XZEROS DW_PR_DUx ")"
                " %"  DW_PR_DUu
                "\n",
                r->mp_key,
                r->mp_key,
                lastend,
                lastend,
                laststart,
                laststart);
        }
        laststart = r->mp_key;
        lastend   = laststart + r->mp_len;
    }
    /*  wholegap is a) starting offset > 0 and b)
        space after used area before end of section. */
    wholegap = mac_as_array[0]->mp_key + internalgap;
    if (lastend > section_size) {
        /* Something seriously wrong */
#ifdef TESTING
        ++failcount;
#endif
        printf(" ERROR: For offset 0x%" DW_PR_XZEROS DW_PR_DUx
            " %" DW_PR_DUu
            " there is an overlap with the end of section "
            "0x%"  DW_PR_XZEROS DW_PR_DUx
            " %" DW_PR_DUu
            "\n",laststart,laststart,
            lastend, lastend);
    } else {
        wholegap += (section_size - lastend);
    }
    if (wholegap) {
        printf("  Macro Offsets internal unused space: "
            "0x%" DW_PR_XZEROS DW_PR_DUx
            "\n",
            internalgap);
        printf("  Macro Offsets total    unused space: "
            "0x%" DW_PR_XZEROS DW_PR_DUx
            "\n",
            wholegap);
    }
    if (macfile_stack_max_seen)  {
        printf("Maximum nest depth of DW_MACRO_start_file: %u\n",
            macfile_stack_max_seen-1);
    }
    if (macro_import_stack_max_seen)  {
        printf("Maximum nest depth of DW_MACRO_import    : %u\n",
            macro_import_stack_max_seen-1);
    }
    free (mac_as_array);
    mac_as_array = 0;
    mac_as_array_next = 0;
    return DW_DLV_OK;
}

void
clear_macrocheck_statistics(void **tsbase)
{
    if (!*tsbase) {
        return;
    }
    macrocheck_map_destroy(*tsbase);
    *tsbase = 0;
}

void
print_macro_import_stack(void)
{
    unsigned i = 0;

    printf("Macro Stack Depth: %u\n",
        macro_import_stack_next_to_use);
    for ( ; i <  macro_import_stack_next_to_use; ++i) {
        printf("Macro Stack[%u] "
            "MOFF=0x%" DW_PR_XZEROS DW_PR_DUx "\n",
            i,macro_import_stack[i]);
    }
}

/*  Returns DW_DLV_ERROR if the push could not done,
    which would be because full.
    Else returns DW_DLV_OK.  */
int
macro_import_stack_push(Dwarf_Unsigned offset)
{
    if (macro_import_stack_next_to_use >=
        MACRO_IMPORT_STACK_DEPTH) {
        printf("ERROR: The macro_import_stack has exceeded "
            "its maximum of %d\n",MACRO_IMPORT_STACK_DEPTH);
        print_macro_import_stack();
        glflags.gf_count_major_errors++;
        return DW_DLV_ERROR;
    }
    macro_import_stack[macro_import_stack_next_to_use] = offset;
    ++macro_import_stack_next_to_use;
    if (macro_import_stack_max_seen <
        macro_import_stack_next_to_use) {
        macro_import_stack_max_seen = macro_import_stack_next_to_use;
    }
    return DW_DLV_OK;
}

/*  Returns DW_DLV_ERROR if the pop could not done,
    else returns DW_DLV_OK.  */
int
macro_import_stack_pop(void)
{
    if (!macro_import_stack_next_to_use) {
        printf("ERROR: The macro_import_stack is"
            " empty and the attempted pop() is "
            "impossible. A dwarfdump bug.\n");
        glflags.gf_count_major_errors++;
        return DW_DLV_ERROR;
    }
    --macro_import_stack_next_to_use;
    return DW_DLV_OK;
}

/*  Returns DW_DLV_OK if offset present
    else DW_DLV_NO_ENTRY. */
int
macro_import_stack_present(Dwarf_Unsigned offset)
{
    unsigned i = 0;

    for ( ; i < macro_import_stack_next_to_use;++i) {
        if (macro_import_stack[i] == offset) {
            return DW_DLV_OK;
        }
    }
    return DW_DLV_NO_ENTRY;
}

void
macro_import_stack_cleanout(void)
{
    macro_import_stack_next_to_use = 0;
}
