/*
  Copyright (C) 2025 David Anderson. All Rights Reserved.

  Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that the
  following conditions are met:

    Redistributions of source code must retain the above
    copyright notice, this list of conditions and the following
    disclaimer.

    Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <config.h>

#include <stdlib.h> /* calloc() free() */
#include <string.h> /* memcmp() memset() strchr() strcmp()
    strlen() strncmp() */
#include <stdio.h> /* FILE decl for dd_esb.h, printf etc */

#ifdef HAVE_STDINT_H
#include <stdint.h> /* uintptr_t */
#endif /* HAVE_STDINT_H */

#ifdef HAVE_FCNTL_H
#include <fcntl.h> /* O_RDONLY open() */
#endif /* HAVE_FCNTL_H */
#ifdef HAVE_UTF8
/*  locale.h is guaranteed in C90 and later,
    but langinfo.h might not be present. */
#include "locale.h"
#include "langinfo.h"
#endif /* HAVE_UTF8 */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_sanitized.h"
#include "dd_safe_strcpy.h"
#include "dd_all_srcfiles.h"
#include "dd_tsearchbal.h"
#include "dd_esb.h"
#ifdef HAVE_UTF8
#include "dd_utf8.h"
#endif /* HAVE_UTF8 */

static void * srcfiles_tree = 0;

#if 0 /* debugging only */
static void
dump_bytes(const char *msg,Dwarf_Small * start, long len)
{
    Dwarf_Small *end = start + len;
    Dwarf_Small *cur = start;
    printf("%s (0x%lx) ",msg,(unsigned long)start);
    for (; cur < end; cur++) {
        printf("%02x", *cur);
    }
    printf("\n");
}
static char *
keyprint(const void *k)
{
    struct All_Srcfiles_Entry * m =
        (struct All_Srcfiles_Entry *)k;
    char * key = m->ase_srcfilename;
    printf("key %s\n",key);
    return key;

}
#endif /* 0 */

static struct All_Srcfiles_Entry *
all_srcfiles_create_entry(char *key )
{
    struct All_Srcfiles_Entry *mp = 0;

    if (!key) {
        /* mistake somewhere */
        return 0;
    }
    mp = (struct All_Srcfiles_Entry *)malloc(
        sizeof(struct All_Srcfiles_Entry));
    if (!mp) {
        return 0;
    }
    mp->ase_srcfilename = (char *)strdup(key);
    mp->ase_dupcount = 1;
    return mp;
}
static void
all_srcfiles_free_func(void *mx)
{
    struct All_Srcfiles_Entry *m = mx;
    if (!m) {
        return;
    }
    free(m->ase_srcfilename);
    free(m);
    return;
}

static int
all_srcfiles_compare_func(const void *l, const void *r)
{
    const struct All_Srcfiles_Entry *ml = l;
    const struct All_Srcfiles_Entry *mr = r;
    int res = strcmp(ml->ase_srcfilename,mr->ase_srcfilename);

    return res;
}

static struct All_Srcfiles_Entry *
dd_all_srcfiles_insert(char *name)
{
    void *retval = 0;
    struct All_Srcfiles_Entry *re = 0;
    struct All_Srcfiles_Entry *e = 0;

    e  = all_srcfiles_create_entry(name);
    /*  tsearch records e's contents unless e
        is already present . We must not free it till
        destroy time if it got added to tree.  */
    retval = dwarf_tsearch(e,&srcfiles_tree,
        all_srcfiles_compare_func);
    if (retval) {
        re = *(struct All_Srcfiles_Entry **)retval;
        if (re != e) {
            ++re->ase_dupcount;
            /*  We returned an name already in
                tree, e not needed. */
            all_srcfiles_free_func(e);
        } else {
            /* Record e got added to tree1, do not free record e. */
        }
    }
    return re;
}

void
dd_all_srcfiles_insert_new(Dwarf_Debug dbg, Dwarf_Die cu_die)
{
    int res = 0;
    Dwarf_Signed srcfiles_cnt = 0;
    char       **srcfiles = 0;
    Dwarf_Error srcerr = 0;
    Dwarf_Signed i = 0;

    res = dwarf_srcfiles(cu_die,
        &srcfiles, &srcfiles_cnt, &srcerr);
    if (res == DW_DLV_OK) {
        for ( ; i < srcfiles_cnt; ++i) {
            char *filepath = srcfiles[i];

            dd_all_srcfiles_insert(filepath);
            dwarf_dealloc(dbg,filepath,DW_DLA_STRING);
        }
        dwarf_dealloc(dbg,srcfiles,DW_DLA_LIST);
    }
    if (res == DW_DLV_ERROR) {
        DROP_ERROR_INSTANCE(dbg,res,srcerr);
        srcfiles = 0;
        srcfiles_cnt = 0;
        return;
    }
    return;
}
static unsigned long count = 0;
static unsigned long countdups = 0;
static void
all_srcfiles_walk_print(const void *nodep,
    const DW_VISIT which,
    const int depth)
{
    struct All_Srcfiles_Entry *ase =
        *(struct All_Srcfiles_Entry**)nodep;

    (void)depth;
    if (which == dwarf_preorder || which == dwarf_endorder) {
        return;
    }
    /* dwarf_postorder */
    count += 1;
    /* not double-counting, 1 is original. */
    countdups += ase->ase_dupcount - 1;
    /*  Could print the count too.  */
    printf("%s\n",sanitized(ase->ase_srcfilename));
}

/*  Since the balanced tree is in sorted order already
    just walk the tree and print */
void
dd_print_all_srcfiles(void)
{
    count = 0;
    countdups = 0;
    printf("\nAll unique source files, sorted by name\n");
    dwarf_twalk(srcfiles_tree,all_srcfiles_walk_print);
    printf("\n Number of unique names    %lu\n",count);
    printf("\n Number of duplicate names %lu\n",countdups);
}

void dd_destroy_all_srcfiles(void)
{
    if (srcfiles_tree) {
        dwarf_tdestroy(srcfiles_tree,all_srcfiles_free_func);
        srcfiles_tree = 0;
    }
}
