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

#include <config.h>

#include <stdio.h>  /* printf() */
#include <stdlib.h> /* calloc() free() malloc() */

/* Windows specific header files */
#if defined(_WIN32) && defined(HAVE_STDAFX_H)
#include "stdafx.h"
#endif /* HAVE_STDAFX_H */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_tsearchbal.h"
#include "dd_naming.h"
#include "dd_attr_form.h"
#include "dwarfdump-af-table-std.h"
#include "dwarfdump-af-table-ext.h"
#include "dwarfdump-ta-table.h"
#include "dwarfdump-ta-ext-table.h"
#include "dwarfdump-tt-table.h"
#include "dwarfdump-tt-ext-table.h"

/*  Here we have  code to read the generated header files
    with the relationship data so we can print the data.
    This also prints the attr-form tables.
    See also print_tag_attributes_usage.c as that is
    where tag-tag and tag-attr tree is printed. */

#if 0 /* debugging only */
static void
print_3key_record(const char *msg,int num,Three_Key_Entry *e)
{
    printf("3key %s %d 0x%x 0x%x 0x%x st %d  ct %lu\n",
        msg,num,e->key1,e->key2,e->key3,
        e->from_tables,
        (unsigned long)e->count);
}
#endif /* 0 */

void * threekey_tag_tag_base;   /* tag-tree recording */
void * threekey_tag_attr_base;  /* for tag_attr recording */
void * threekey_attr_form_base; /* for attr/class/form recording */
void * threekey_tag_use_base;   /* for simple tag counting */

int
make_3key(Dwarf_Half k1,
    Dwarf_Half k2,
    Dwarf_Half k3,
    Dwarf_Small std_or_exten,
    Dwarf_Small reserved,
    Dwarf_Unsigned count,
    Three_Key_Entry ** out)
{
    Three_Key_Entry *e =
        (Three_Key_Entry *)malloc(sizeof(Three_Key_Entry));
    if (!e) {
        return DW_DLV_ERROR; /* out of memory */
    }
    e->key1         = k1;
    e->key2         = k2;
    e->key3         = k3;
    e->from_tables  = std_or_exten;
    e->reserved     = reserved;
    e->count        = count;
    *out            = e;
    return DW_DLV_OK;
}

void
free_func_3key_entry(void *keystructptr)
{
    Three_Key_Entry *e = keystructptr;
    if (e->from_tables) {
        /*  Points into static AF_STD or
            AF_EXTEN array. do not free */
        return;
    }
    free(e);
}

int
std_compare_3key_entry(const void *l_in, const void *r_in)
{
    const Three_Key_Entry *l = l_in;
    const Three_Key_Entry *r = r_in;
    if (l->key1 < r->key1) {
        return -1;
    }
    if (l->key1 > r->key1) {
        return 1;
    }
    if (l->key2 < r->key2) {
        return -1;
    }
    if (l->key2 > r->key2) {
        return 1;
    }
    if (l->key3 < r->key3) {
        return -1;
    }
    if (l->key3 > r->key3) {
        return 1;
    }
    return 0;

}

static Dwarf_Unsigned counting_global;
static void
count_3key_entry(const void * vptr,
    DW_VISIT x,
    int level)
{
    (void)vptr;
    (void)level;
    if (x == dwarf_preorder || x == dwarf_leaf) {
        ++counting_global;
    }
}

/*  Tree argument expected is threekey_attr_form_base for example */
Dwarf_Unsigned
three_key_entry_count(void *base)
{
    Dwarf_Unsigned count = 0;

    counting_global = 0;
    dwarf_twalk(base,count_3key_entry);
    count = counting_global;
    counting_global = 0;
    return count;
}

/*  Used for base table creation */
static int
dd_insert_table_entry(void *tree,
    Three_Key_Entry *e,
    int *errnum)
{
    Three_Key_Entry *re = 0;
    void *ret = dwarf_tsearch(e,tree,std_compare_3key_entry);
    if (!ret) {
        *errnum = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    re = *(Three_Key_Entry **)ret;
    if (re == e) {
        /* Normal. Added. */
        return DW_DLV_OK;
    }
    /*  A full duplicate in the table. Oops.
        Not a great choice of error code. */
    *errnum = DW_DLE_ATTR_FORM_BAD;
    return DW_DLV_ERROR;
}

static int
dd_common_build_base_tree(void *tree_base,
    Three_Key_Entry *std,
    Three_Key_Entry *ext,
    int *errnum)
{
    struct Three_Key_Entry_s *key = 0;
    int          res = 0;
    int          t = 0;
    void *tree = tree_base;
    struct Three_Key_Entry_s * stdext [3] = {std,ext,0};

    for (key = stdext[t]; key ;  ++t, key = stdext[t]) {
        for ( ; ; ++key) {
            if (!(key->key1 | key->key1 | key->key3)){
                break;
            }
            res =  dd_insert_table_entry(tree,key,errnum);
            if (res != DW_DLV_OK) {
                if (res == DW_DLV_ERROR) {
                    return res;
                }
                *errnum = DW_DLE_ALLOC_FAIL;
                return res;
            }
        }
    }
    return DW_DLV_OK;
}

int
dd_build_tag_attr_form_base_trees(int*errnum)
{
    int res = 0;

    res = dd_common_build_base_tree(&threekey_attr_form_base,
        dd_threekey_af_table_std,dd_threekey_af_table_ext,
        errnum);
    if (res != DW_DLV_OK){
        return res;
    }
    res = dd_common_build_base_tree(&threekey_tag_attr_base,
        dd_threekey_ta_std,dd_threekey_ta_ext,
        errnum);
    if (res != DW_DLV_OK){
        return res;
    }
    res = dd_common_build_base_tree(&threekey_tag_tag_base,
        dd_threekey_tt_std,dd_threekey_tt_ext,
        errnum);
    if (res != DW_DLV_OK){
        return res;
    }
    /*  No need to initialize the tag_use tree, the
        initial run-time dwarf_tsearch call will do that.
        All entries are run-time-only. */
    return DW_DLV_OK;
}

void
dd_destroy_tag_use_base_tree(void)
{
    if (!threekey_tag_use_base) {
        return;
    }
    dwarf_tdestroy(threekey_tag_use_base,
        free_func_3key_entry);
    threekey_tag_use_base = 0;
}
static void
dd_destroy_attr_form_tree(void)
{
    if (!threekey_attr_form_base) {
        return;
    }
    dwarf_tdestroy(threekey_attr_form_base,
        free_func_3key_entry);
    threekey_attr_form_base = 0;
}
static void
dd_destroy_tag_attr_tree(void)
{
    if (!threekey_tag_attr_base) {
        return;
    }
    dwarf_tdestroy(threekey_tag_attr_base,
        free_func_3key_entry);
    threekey_tag_attr_base = 0;
}
static void
dd_destroy_tag_tag_tree(void)
{
    if (!threekey_tag_tag_base) {
        return;
    }
    dwarf_tdestroy(threekey_tag_tag_base,
        free_func_3key_entry);
    threekey_tag_tag_base = 0;
}

void
dd_destroy_tag_attr_form_trees(void)
{
    dd_destroy_attr_form_tree();
    dd_destroy_tag_attr_tree();
    dd_destroy_tag_tag_tree();
    dd_destroy_tag_use_base_tree();
}

/*  SKIP_AF_CHECK defined means this is in scripts/ddbuild.sh
    and this checking makes no sense and will not compile. */
#ifndef SKIP_AF_CHECK
static Dwarf_Bool
legal_attr_formclass_combination(Dwarf_Half attr,
    Dwarf_Half fc)
{
    Three_Key_Entry *e = 0;
    Three_Key_Entry *re = 0;
    void *ret = 0;
    int   res = 0;

    res = make_3key(attr,fc,0,0,0,0,&e);
    if (res!= DW_DLV_OK) {
        /*  Hiding some sort of botch/malloc issue */
        return TRUE;
    }
    ret = dwarf_tfind(e,&threekey_attr_form_base,
        std_compare_3key_entry);
    if (!ret) {
        /*  Surprising combo. */
        free(e);
        return FALSE;
    }
    re = *(Three_Key_Entry **)ret;
    if (!glflags.gf_suppress_check_extensions_tables) {
        free(e);
        return TRUE;
    }
    if (re->from_tables == AF_STD) {
        free(e);
        return TRUE;
    }
    free(e);
    return FALSE;
}

static void
check_attr_formclass_combination(Dwarf_Debug dbg,
    Dwarf_Half tag,
    Dwarf_Half attrnum,
    Dwarf_Half fc,
    int die_stack_indent_level)
{
    const char *tagname = "<AT invalid>";
    const char *formclassname = "<FORM_CLASS invalid>";
    DWARF_CHECK_COUNT(attr_formclass_result,1);
    if (legal_attr_formclass_combination(attrnum,fc)) {
        /* OK */
    } else {
        /* Report errors only if tag-attr check is on */
        if (glflags.gf_check_tag_attr) {
            tagname = get_AT_name(attrnum);
            tag_specific_globals_setup(dbg,tag,
                die_stack_indent_level);
            formclassname = get_FORM_CLASS_name(fc);

            DWARF_CHECK_ERROR3(attr_formclass_result,tagname,
                formclassname,
                "check the attr-formclass combination");
        } else { /* Nothing to do. */ }
    }
}
#endif /* SKIP_AF_CHECK  */

void
record_attr_form_use(
    Dwarf_Debug dbg,
    Dwarf_Half tag,
    Dwarf_Half attr,
    Dwarf_Half fclass,
    Dwarf_Half form,
    int die_stack_indent_level)
{
    /*  SKIP_AF_CHECK defined means this is in scripts/ddbuild.sh
        and checking/recording makes no sense and will not compile. */
#ifdef SKIP_AF_CHECK
    (void)dbg;
    (void)tag;
    (void)attr;
    (void)fclass;
    (void)form;
    (void)die_stack_indent_level;;
    return;
#else
    Three_Key_Entry *e =  0;
    Three_Key_Entry *re =  0;
    void *ret =  0;
    int res = 0;

/*  SKIP_AF_CHECK defined means this is in scripts/ddbuild.sh
    and this checking makes no sense and will not compile. */
    check_attr_formclass_combination(dbg,
        tag,attr,fclass,
        die_stack_indent_level);
    res = make_3key(attr,fclass,form,0,0,1,&e);
    if (res!= DW_DLV_OK) {
        /*  Could print something */
        return;
    }
    ret = dwarf_tsearch(e,&threekey_attr_form_base,
        std_compare_3key_entry);
    if (!ret) {
        free_func_3key_entry(e);
        /*  Could print something */
        return;
    }
    re = *(Three_Key_Entry **)ret;
    if (re == e) {
        /*  Brand new entry. Done.
            local malloc is in the tree. */
        return;
    }
    /* Was already entered.*/
    re->count++;
    /* Clean out the local malloc */
    free_func_3key_entry(e);
    return;
#endif /* SKIP_AF_CHECK */
}

static Dwarf_Unsigned recordcount = 0;
static Dwarf_Unsigned recordmax = 0;
static Three_Key_Entry * tkarray = 0;
static void
extract_3key_entry(const void * vptr,
    DW_VISIT x,
    int level)
{
    (void)level;
    if (x == dwarf_preorder || x == dwarf_leaf) {
        Three_Key_Entry *m = *(Three_Key_Entry **)vptr;
        if (recordcount >= recordmax) {
            /* Should never happen */
            return;
        }
        tkarray[recordcount] = *m;
        ++recordcount;
    }
}

static int
qsortformclass(const void * e1in, const void * e2in)
{
    Three_Key_Entry *e1 = (Three_Key_Entry *)e1in;
    Three_Key_Entry *e2 = (Three_Key_Entry *)e2in;

    if (e1->key2 < e2->key2) {
        return -1;
    }
    if (e1->key2 > e2->key2) {
        return 1;
    }
    return 0;
}

static int
qsortform(const void * e1in, const void * e2in)
{
    Three_Key_Entry *e1 = (Three_Key_Entry *)e1in;
    Three_Key_Entry *e2 = (Three_Key_Entry *)e2in;

    if (e1->key3 < e2->key3) {
        return -1;
    }
    if (e1->key3 > e2->key3) {
        return 1;
    }
    return 0;
}

static int
qsortcountattr(const void * e1in, const void * e2in)
{
    Three_Key_Entry *e1 = (Three_Key_Entry *)e1in;
    Three_Key_Entry *e2 = (Three_Key_Entry *)e2in;

    if (e1->count < e2->count) {
        return 1;
    }
    if (e1->count > e2->count) {
        return -1;
    }
    if (e1->key1 < e2->key1) {
        return -1;
    }
    if (e1->key1 > e2->key1) {
        return 1;
    }
    if (e1->key3 < e2->key3) {
        return -1;
    }
    if (e1->key3 > e2->key3) {
        return 1;
    }
    if (e1->key2 < e2->key2) {
        return -1;
    }
    if (e1->key2 > e2->key2) {
        return 1;
    }
    return 0;
}

static int
qsortk1k2(const void * e1in, const void * e2in)
{
    Three_Key_Entry *e1 = (Three_Key_Entry *)e1in;
    Three_Key_Entry *e2 = (Three_Key_Entry *)e2in;

    if (e1->key1 < e2->key1) {
        return -1;
    }
    if (e1->key1 > e2->key1) {
        return 1;
    }
    if (e1->key2 < e2->key2) {
        return -1;
    }
    if (e1->key2 > e2->key2) {
        return 1;
    }
    return 0;
}

void
print_attr_form_usage(void)
{
    Three_Key_Entry  *tk_l  = 0;
    Dwarf_Unsigned    i     = 0;
    unsigned          curform = 0;
    Dwarf_Unsigned    formtotal = 0;
    unsigned          curattr = 0;
    Dwarf_Unsigned    attrtotal = 0;
    Dwarf_Unsigned    j     = 0;
    float             total = 0.0f;
    float             pct   = 0.0f;
    Dwarf_Bool        startnoted = FALSE;
    const char *      localformat=  NULL;
    Dwarf_Unsigned    localsum = 0;
    /*  These are file static and must be carefully aligned
        with our table reading.
        Dwarf_Unsigned recordcount = 0;
        Dwarf_Unsigned recordmax = 0;
        Three_Key_Entry * tkarray = 0;
    */

    recordmax = three_key_entry_count(threekey_attr_form_base);
    if (!recordmax) {
        return;
    }
    tk_l = (Three_Key_Entry *)calloc(recordmax+1,
        sizeof(Three_Key_Entry));
    tkarray=tk_l;
    if (!tk_l) {
        printf("ERROR: unable to malloc attr/form array "
            " for a summary report \n");
        glflags.gf_count_major_errors++;
        return;
    }
    /* Reset the file-global! */
    recordcount = 0;
    dwarf_twalk(threekey_attr_form_base,extract_3key_entry);
    if (recordcount != recordmax) {
        printf("ERROR: unable to fill in attr/form array "
            " for a summary report, count %lu != walk %lu \n",
            (unsigned long)recordcount,
            (unsigned long)recordmax);
        glflags.gf_count_major_errors++;
        free(tk_l);
        tkarray = 0;
        return;
    }
    for (i = 0; i < recordmax; ++i) {
        Three_Key_Entry * tke = tk_l+i;

        if (!tke->key3) {
            /* Skip table building data */
            continue;
        }
        total += (float)tke->count;
    }
    qsort(tk_l,recordmax,sizeof(Three_Key_Entry),
        qsortcountattr);
    printf("\n*** ATTRIBUTES AND FORMS USAGE ***\n");
    printf("Full record count                    : %8" DW_PR_DUu "\n",
        recordmax);
    printf("Total number of objectfile attributes: %8.0f\n", total);
    printf("[]                                        "
        "                found rate\n");
    localformat="[%3u] %-30s %-20s %7" DW_PR_DUu " %.0f%%\n";
    localsum = 0;
    for (i = 0; i < recordmax; ++i) {
        Three_Key_Entry * tke = tk_l+i;

        if (!tke->key3) {
            /* Skip table building data */
            continue;
        }
        pct = ( (float)tke->count / total)*100.0f;
        printf(localformat,
            (unsigned)i,
            get_AT_name(tke->key1),
            get_FORM_name(tke->key3),
            tke->count,pct);
        localsum += tke->count;
    }
    printf(localformat, (unsigned)(recordmax),
        "Sum found:","",localsum,100.0f);

    qsort(tk_l,recordmax,sizeof(Three_Key_Entry),
        qsortformclass);
    j = 0;
    /* Re-using the following two */
    curform = 0;
    formtotal = 0;
    startnoted = FALSE;
    printf("\n*** COUNT BY FORMCLASS ***\n");
    printf("[]                                 found rate\n");
    localsum = 0;
    localformat="[%2u] %-28s %6" DW_PR_DUu " %.0f%%\n";
    for (i = 0; i < recordmax; ++i) {
        Three_Key_Entry * tke = tk_l+i;

        if (!tke->key3) {
            /* Skip table building data */
            continue;
        }
        if (!startnoted) {
            curform = tke->key2;
            formtotal = tke->count;
            startnoted = TRUE;
            continue;
        }
        if (curform != tke->key2) {
            pct = ( (float)formtotal / total)*100.0f;
            printf(localformat,
                (unsigned)j,
                get_FORM_CLASS_name(curform),
                formtotal,pct);
            localsum += formtotal;
            curform = tke->key2;
            formtotal = tke->count;
            ++j;
            continue;
        }
        formtotal += tke->count;
    }
    if (formtotal) {
        pct = ( (float)formtotal / total)*100.0f;
        printf(localformat,
            (unsigned)j,
            get_FORM_CLASS_name(curform),
            formtotal,pct);
        localsum += formtotal;
    }
    printf(localformat, (unsigned)(j+1),
        "Sum found:",localsum,100.0f);

    /* Re-using the following two */
    curform = 0;
    formtotal = 0;
    startnoted = FALSE;
    qsort(tk_l,recordmax,sizeof(Three_Key_Entry),
        qsortform);
    j = 0;
    printf("\n*** COUNT BY FORM ***\n");
    printf("[]                         found rate\n");
    localformat="[%2u] %-20s %6" DW_PR_DUu " %.0f%%\n";
    localsum = 0;
    for (i = 0; i < recordmax; ++i) {
        Three_Key_Entry * tke = tk_l+i;

        if (!tke->key3) {
            /* Skip table building data */
            continue;
        }
        if (!startnoted) {
            curform = tke->key3;
            formtotal = tke->count;
            startnoted = TRUE;
            continue;
        }
        if (curform != tke->key3) {
            pct = ( (float)formtotal / total)*100.0f;
            printf(localformat,
                (unsigned)j,
                get_FORM_name(curform),
                formtotal,pct);
            localsum += formtotal;
            curform = tke->key3;
            formtotal = tke->count;
            ++j;
            continue;
        }
        formtotal += tke->count;
    }
    if (formtotal) {
        pct = ( (float)formtotal / total)*100.0f;
        printf(localformat,
            (unsigned)j,
            get_FORM_name(curform),
            formtotal,pct);
        localsum += formtotal;
    }
    printf(localformat, (unsigned)(j+1),
        "Sum found:",localsum,100.0f);

    j = 0;
    curattr = 0;
    attrtotal = 0;
    startnoted = FALSE;
    printf("\n*** COUNT BY ATTRIBUTE ***\n");
    printf("[]                                   found rate\n");
    localsum = 0;
    localformat="[%2u] %-30s %6" DW_PR_DUu " %.0f%%\n";
    for (i = 0; i < recordmax; ++i) {
        Three_Key_Entry * tke = tk_l+i;

        if (!tke->key3) {
            /* Skip table building data */
            continue;
        }
        if (!startnoted) {
            curattr = tke->key1;
            attrtotal = tke->count;
            startnoted = TRUE;
            continue;
        }
        if (curattr != tke->key1) {
            pct = ( (float)attrtotal / total)*100.0f;
            printf(localformat,
                (unsigned)j,
                get_AT_name(curattr),
                attrtotal,pct);
            localsum += attrtotal;
            curattr = tke->key1;
            attrtotal = tke->count;
            ++j;
            continue;
        }
        formtotal += tke->count;
    }
    if (attrtotal) {
        pct = ( (float)attrtotal / total)*100.0f;
        printf(localformat,
            (unsigned)j,
            get_AT_name(curattr),
            attrtotal,pct);
        localsum += attrtotal;
    }
    printf(localformat, (unsigned)(j+1),
        "Sum found:",localsum,100.0f);
    free(tk_l);
    tkarray = 0;
}

/*  extract all tag_tree  records
    (also called tag_tag sometimes) to an array,
    sort by tag number and child tag number.

    Loop the list, printing tag (and name)
    and within that a line for each child tag and count.
*/
static const char *
gettablename(unsigned t)
{
    switch(t) {
    case AF_STD:
        return "Standard  ";
    case AF_EXTEN:
        return "Extended  ";
    case AF_UNKNOWN:
        return "Unknown   ";
    default:
        break;
    }
    return     "Impossible";
}

void
dd_print_tag_tree_results(Dwarf_Unsigned tag_tag_count)
{
    Three_Key_Entry  *tk_l  = 0;
    Dwarf_Unsigned    i     = 0;
    Dwarf_Half  curparent = 0;
    /*  These are file static and must be carefully aligned
        with our table reading.
        Dwarf_Unsigned recordcount = 0;
        Dwarf_Unsigned recordmax = 0;
        Three_Key_Entry * tkarray = 0;
    */

    recordmax = tag_tag_count;
    printf("\nNumber of tag-parent/tag-child records %7"
        DW_PR_DUu "\n", tag_tag_count);
    if (!tag_tag_count) {
        return;
    }
    tkarray = 0;
    tk_l = (Three_Key_Entry *)calloc(tag_tag_count+1,
        sizeof(Three_Key_Entry));
    tkarray=tk_l;
    if (!tk_l) {
        printf("ERROR: unable to malloc tag-parent/tag-child array "
            " for a summary report \n");
        glflags.gf_count_major_errors++;
        return;
    }
    /* Reset the file-global! */
    recordcount = 0;
    dwarf_twalk(threekey_tag_tag_base,extract_3key_entry);
    if (recordcount != tag_tag_count) {
        printf("ERROR: unable to fill in tag-paraent/tag-child array "
            " for a summary report, count %lu != walk %lu \n",
            (unsigned long)tag_tag_count,
            (unsigned long)recordcount);
        glflags.gf_count_major_errors++;
        free(tk_l);
        tkarray = 0;
        return;
    }
    qsort(tk_l,recordcount,sizeof(Three_Key_Entry),
        qsortk1k2);

    for (i = 0; i < recordmax; ++i) {
        Three_Key_Entry * tke = tk_l+i;

        /* In checking mode verbose is automatically 1 */
        if (glflags.verbose < 2) {
            if (!tke->count) {
                /* Skip this */
                continue;
            }
        }
        if (tke->key1 != curparent) {
            printf("[ %4" DW_PR_DUu "] 0x%04x %-38s"
                " table         count\n",
                i,tke->key1,get_TAG_name(tke->key1));
            curparent = tke->key1;
        }
        printf("        0x%04x %-38s %s  %7" DW_PR_DUu "\n",
            tke->key2, get_TAG_name(tke->key2),
            gettablename(tke->from_tables),
            tke->count);
    }
    free(tk_l);
}
void
dd_print_tag_attr_results(Dwarf_Unsigned tag_attr_count)
{

    Three_Key_Entry  *tk_l  = 0;
    Dwarf_Unsigned    i     = 0;
    Dwarf_Half  curparent = 0;
    /*  These are file static and must be carefully aligned
        with our table reading.
        Dwarf_Unsigned recordcount = 0;
        Dwarf_Unsigned recordmax = 0;
        Three_Key_Entry * tkarray = 0;
    */
    Dwarf_Unsigned attrs_unknown = 0; /* meaning not in tables */
    Dwarf_Unsigned attrs_extended = 0;
    Dwarf_Unsigned attrs_std = 0;
    Dwarf_Unsigned count_attr_instances = 0;

    recordmax = tag_attr_count;
    printf("\nNumber of tag/attr records             %7"
        DW_PR_DUu "\n",
        tag_attr_count);
    if (!tag_attr_count) {
        return;
    }
    tkarray = 0;
    tk_l = (Three_Key_Entry *)calloc(tag_attr_count+1,
        sizeof(Three_Key_Entry));
    tkarray=tk_l;
    if (!tk_l) {
        printf("ERROR: unable to malloc tag/attr array "
            " for a summary report \n");
        glflags.gf_count_major_errors++;
        return;
    }
    /* Reset the file-global! */
    recordcount = 0;
    dwarf_twalk(threekey_tag_attr_base,extract_3key_entry);
    if (recordcount != tag_attr_count) {
        printf("ERROR: unable to fill in tag/attr array "
            " for a summary report, count %lu != walk %lu \n",
            (unsigned long)tag_attr_count,
            (unsigned long)recordcount);
        glflags.gf_count_major_errors++;
        free(tk_l);
        tkarray = 0;
        return;
    }
    qsort(tk_l,recordcount,sizeof(Three_Key_Entry),
        qsortk1k2);

    for (i = 0; i < recordmax; ++i) {
        Three_Key_Entry * tke = tk_l+i;

        count_attr_instances += tke->count;
    }

    for (i = 0; i < recordmax; ++i) {
        Three_Key_Entry * tke = tk_l+i;
        double pct = 0.0;

        /* In checking mode verbose is automatically 1 */
        if (glflags.verbose < 2) {
            if (!tke->count) {
                /* Skip this */
                continue;
            }
        }
        if (tke->key1 != curparent) {
            printf("[ %4" DW_PR_DUu "] 0x%04x %-38s"
                " table      count percent\n",
                i,tke->key1,get_TAG_name(tke->key1));
            curparent = tke->key1;
        }
        switch(tke->from_tables) {
        case AF_STD:
            attrs_std += tke->count;
            break;
        case AF_EXTEN:
            attrs_extended += tke->count;
            break;
        case AF_UNKNOWN:
        default:
            attrs_unknown += tke->count;
        break;
        }
        if (count_attr_instances) {
            pct = ((double)tke->count/(double)count_attr_instances)*
                100.0;
        }
        printf("        0x%04x %-38s %s  %7" DW_PR_DUu
            " %4.1f\n",
            tke->key2, get_AT_name(tke->key2),
            gettablename(tke->from_tables),
            tke->count,pct);
    }
    printf("Number of attribute instances   : %7" DW_PR_DUu "\n",
        count_attr_instances);
    printf("Number of standard table entries: %7" DW_PR_DUu "\n",
        attrs_std);
    printf("Number of extended table entries: %7" DW_PR_DUu "\n",
        attrs_extended);
    printf("Number of unknown  table entries: %7" DW_PR_DUu "\n",
        attrs_unknown);
    free(tk_l);
}

void
dd_print_tag_use_results(Dwarf_Unsigned tag_count)
{
    Three_Key_Entry  *tk_l  = 0;
    Dwarf_Unsigned    i     = 0;
    Dwarf_Unsigned    sum_of_uses = 0;
    /*  These are file static and must be carefully aligned
        with our table reading.
        Dwarf_Unsigned recordcount = 0;
        Dwarf_Unsigned recordmax = 0;
        Three_Key_Entry * tkarray = 0;
    */

    recordmax = tag_count;
    printf("\nNumber of TAG records               %7" DW_PR_DUu "\n",
        tag_count);
    if (!tag_count) {
        return;
    }
    tkarray = 0;
    tk_l = (Three_Key_Entry *)calloc(tag_count+1,
        sizeof(Three_Key_Entry));
    tkarray=tk_l;
    if (!tk_l) {
        printf("ERROR: unable to malloc tag array "
            " for a summary report \n");
        glflags.gf_count_major_errors++;
        return;
    }
    /* Reset the file-global! */
    recordcount = 0;
    dwarf_twalk(threekey_tag_use_base,extract_3key_entry);
    if (recordcount != tag_count) {
        printf("ERROR: unable to fill in tag/attr array "
            " for a summary report, count %lu != walk %lu \n",
            (unsigned long)tag_count,
            (unsigned long)recordcount);
        glflags.gf_count_major_errors++;
        free(tk_l);
        tkarray = 0;
        return;
    }
    qsort(tk_l,recordcount,sizeof(Three_Key_Entry),
        qsortk1k2);

    for (i = 0; i < recordmax; ++i) {
        Three_Key_Entry * tke = tk_l+i;
        sum_of_uses += tke->count;
    }
    printf("Number of distinct TAGs in object   %7" DW_PR_DUu "\n",
        sum_of_uses);

    printf("[   ]  TAG                                    "
        "    use-count percent\n");

    for (i = 0; i < recordmax; ++i) {
        Three_Key_Entry * tke = tk_l+i;
        double pct = 0.0;

        /* In checking mode verbose is automatically 1 */
        if (glflags.verbose < 2) {
            if (!tke->count) {
                /* Skip this */
                continue;
            }
        }
        if (sum_of_uses) {
            pct = ((double)tke->count/(double)sum_of_uses)* 100.0;
        }
        printf("[ %4" DW_PR_DUu "] 0x%04x %-38s %7"
            DW_PR_DUu " %3.1f\n",
                i,tke->key1,get_TAG_name(tke->key1),
                tke->count,pct);
    }
    free(tk_l);
    tkarray  = 0;
}
