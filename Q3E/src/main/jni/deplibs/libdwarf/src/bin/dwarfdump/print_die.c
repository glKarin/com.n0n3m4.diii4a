/*
Copyright (C) 2000-2006 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright 2007-2010 Sun Microsystems, Inc. All rights reserved.
Portions Copyright 2009-2018 SN Systems Ltd. All rights reserved.
Portions Copyright 2007-2024 David Anderson. All rights reserved.

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

/*  The address of the Free Software Foundation is
    Free Software Foundation, Inc., 51 Franklin St, Fifth
    Floor, Boston, MA 02110-1301, USA.  SGI has moved from
    the Crittenden Lane address.  */

#include <config.h>

#include <stdlib.h> /* calloc() free() */
#include <string.h> /* memcmp() memset() strchr() strcmp()
    strlen() strncmp() */
#include <stdio.h> /* FILE decl for dd_esb.h, printf etc */

#ifdef HAVE_STDINT_H
#include <stdint.h> /* uintptr_t */
#endif /* HAVE_STDINT_H */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_naming.h"
#include "dd_esb.h"                /* For flexible string buffer. */
#include "dd_esb_using_functions.h"
#include "dd_sanitized.h"
#include "print_frames.h"  /* for print_expression_operations() . */
#include "dd_macrocheck.h"
#include "dd_helpertree.h"
#include "dd_opscounttab.h"
#include "dd_tag_common.h"
#include "dd_attr_form.h"
#include "dd_regex.h"
#include "dd_safe_strcpy.h"
#include "dd_all_srcfiles.h"
#ifdef HAVE_UTF8
#include "dd_utf8.h"
#endif /* HAVE_UTF8 */

#define VSFBUFSZ 200
#define DIE_STACK_SIZE 800  /* A hard limit. */
/*  OpBranchHead_s gives us nice type-checking
    in calls. */
struct OpBranchEntry_s {
    Dwarf_Unsigned offset;
    Dwarf_Unsigned target_offset;
    Dwarf_Small    op;
};
struct OpBranchHead_s {
    Dwarf_Half               bh_opcount;
    struct OpBranchEntry_s * bh_ops_array;
};

/*  Defaults to all 0, never changes */
static const LoHiPc  lohipc_zero;

static int traverse_attribute(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Off dieprint_cu_goffset,
    Dwarf_Bool is_info,
    Dwarf_Half attr, Dwarf_Attribute attr_in,
    char **srcfiles, Dwarf_Signed srcfiles_cnt,
    int die_indent_level,
    Dwarf_Error * err);
static int print_die_and_children_internal(Dwarf_Debug dbg,
    Dwarf_Die in_die_in,
    Dwarf_Off dieprint_cu_goffset,
    Dwarf_Bool is_info,
    char **srcfiles, Dwarf_Signed srcfiles_cnt,
    Dwarf_Off       *sibling_off_array,
    Dwarf_Unsigned   sibling_off_count,
    Dwarf_Error *);
static int print_one_die_section(Dwarf_Debug dbg,
    Dwarf_Bool is_info,
    Dwarf_Error *pod_err);
static int handle_rnglists(Dwarf_Attribute attrib,
    Dwarf_Half theform,
    Dwarf_Unsigned value,
    Dwarf_Unsigned *rle_offset_out,
    struct esb_s *  esbp,
    int local_verbose,
    Dwarf_Error *err);
static int _dwarf_print_one_expr_op(Dwarf_Debug dbg,
    Dwarf_Die die,
    int die_indent_level,
    Dwarf_Locdesc_c exprc,
    Dwarf_Unsigned  index,
    Dwarf_Bool      has_skip_or_branch,
    struct OpBranchHead_s *oparray,
    int *stackchange,
    Dwarf_Signed *branchdistance,
    int *zerostackdepth,
    struct esb_s *string_out,
    Dwarf_Error *err);

static int get_form_values(Dwarf_Attribute attrib,
    Dwarf_Half * theform, Dwarf_Half * directform,Dwarf_Error *err);
static void show_form_itself(int show_form,int verbose,
    int theform, int directform, struct esb_s * str_out);
static int print_attribute(Dwarf_Debug dbg, Dwarf_Die die,
    Dwarf_Off dieprint_cu_goffset,
    Dwarf_Half attr,
    Dwarf_Attribute actual_addr,
    Dwarf_Bool print_else_name_match,
    int die_indent_level,
    char **srcfiles, Dwarf_Signed srcfcnt,
    LoHiPc *lohipc,
    Dwarf_Bool *attr_matched,
    Dwarf_Error *err);
static int print_location_list(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Attribute attr,
    Dwarf_Bool checking,
    int die_indent_level,
    int no_end_newline,
    struct esb_s *details,Dwarf_Error *);

static int formxdata_print_value(Dwarf_Debug dbg,
    Dwarf_Die die,Dwarf_Attribute attrib,
    Dwarf_Half theform,
    struct esb_s *esbp, Dwarf_Error * err, Dwarf_Bool hex_format);
static void bracket_hex(const char *s1, Dwarf_Unsigned v,
    const char *s2, struct esb_s * esbp);
static void formx_unsigned(Dwarf_Unsigned u, struct esb_s *esbp,
    Dwarf_Bool hex_format);
static void formx_data16(Dwarf_Form_Data16 * u, struct esb_s *esbp,
    Dwarf_Bool hex_format);

static void formx_signed(Dwarf_Signed s, struct esb_s *esbp);

static int        die_stack_indent_level = 0;
static Dwarf_Bool local_symbols_already_begun = FALSE;
static const Dwarf_Sig8 zerosig;

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
#endif /* 0 */
/* 20 characters */
static char * indentspace =
    "          "
    "          ";
static int  indentspacelen = 20;
static void
appendn( struct esb_s *m, int len)
{
    int remaining = len;
    while (remaining > indentspacelen) {
        esb_append(m,indentspace);
        remaining -= indentspacelen;
    }
    esb_appendn(m,indentspace,remaining);
}
static void
append_indent_prefix(struct esb_s *m,
    int prespaces,
    int indent,
    int postspaces)
{
    if (indent < glflags.gf_max_space_indent) {
        appendn(m,2*indent + prespaces+postspaces);
        return;
    }
    appendn(m,prespaces);
    esb_append_printf_i(m,"...%d...",indent);
    appendn(m,postspaces);
}

static int
standard_indent(void)
{
    /*  Attribute indent. The global offset printed length
        is actually 32. With GOFF the length is
        actually 16. Usually. */
    int nColumn = glflags.gf_show_global_offsets ? 34 : 18;
    if (!glflags.gf_display_offsets) {
        nColumn = 2;
    }
    return nColumn;
}

static void
print_indent_prefix(int prespaces,int indent,int postspaces)
{
    if (indent < glflags.gf_max_space_indent) {
        int len = prespaces+postspaces+ 2*indent;
        printf("%*s",len," ");
        return;
    }
    if (prespaces) {
        printf("%*s",prespaces," ");
    }
    printf("...%d...",indent);
    if (postspaces) {
        printf("%*s",postspaces," ");
    }
}

struct die_stack_data_s {
    Dwarf_Die die_;
    /*  sibling_die_globaloffset_ is set while processing the DIE.
        We do not know the sibling global offset
        when we create the stack entry.
        If the sibling attribute absent we never know. */
    Dwarf_Off sibling_die_globaloffset_;
    /*  We may need is_info here too. */
    Dwarf_Off cu_die_offset_; /* global offset. */
    Dwarf_Bool already_printed_;
};
static struct die_stack_data_s empty_stack_entry;
static struct die_stack_data_s die_stack[DIE_STACK_SIZE];
#define SET_DIE_STACK_ENTRY(i,x,o)                \
    { die_stack[(i)].die_ = (x);                  \
    die_stack[(i)].cu_die_offset_ = o;            \
    die_stack[(i)].sibling_die_globaloffset_ = 0; \
    die_stack[(i)].already_printed_ = FALSE; }
#define EMPTY_DIE_STACK_ENTRY(i)     \
    { die_stack[(i)] = empty_stack_entry; }
#define SET_DIE_STACK_SIBLING(x) {     \
    die_stack[die_stack_indent_level]. \
    sibling_die_globaloffset_ = (x); }

static void
report_die_stack_error(Dwarf_Debug dbg, Dwarf_Error *err)
{
    struct esb_s m;

    esb_constructor(&m);
    esb_append_printf_i(&m,
        "ERROR: compiled in DIE_STACK_SIZE "
        "(the depth of the DIE tree in this CU)"
        " of %d exceeded!"
        " Likely a circular DIE reference."
        ,DIE_STACK_SIZE);
    dwarf_error_creation(dbg,err,
        esb_get_string(&m));
    print_error_and_continue(esb_get_string(&m),
        DW_DLV_OK,*err);
    esb_destructor(&m);
}

/*  Just access the die and print selected fields into
    the string.
    In case of error print an error message
    and return DW_DLV_NO_ENTRY.
    Initially we  just verify the offset is ok.  */
#define NO_SPECIFIC_TAG 0
/*  The following two must differ, but value unimportant.
    We choose each to be a small prime number. */
#define NON_ZERO_OFFSET_REQUIRED 3
#define ZERO_OFFSET_GENERIC_TYPE 19
#define WITHIN_CU                TRUE
static void
check_die_expr_op_basic_data(Dwarf_Debug dbg,Dwarf_Die die,
    const char * op_name,
    int indentprespaces,
    int die_indent_level,
    int indentpostspaces,
    int required_tag,
    int required_offset,
    int within_cu,
    Dwarf_Unsigned offset,
    struct esb_s *string_out)
{
    Dwarf_Error err = 0;
    Dwarf_Off globaloff = 0;
    Dwarf_Bool is_info =0;
    Dwarf_Die other_die = 0;
    int res = 0;
    Dwarf_Half tag = 0;
    const char * required_tag_name = 0;
    const char * actual_tag_name = 0;

    if (! glflags.gf_do_print_dwarf) {
        /*  We are checking so any errors detected
            will not print, so do not check.
            Even though this all seems self-contradictory! */
        return;
    }
    if (within_cu && !offset &&
        required_offset == ZERO_OFFSET_GENERIC_TYPE ) {
        /*  Means the offset zero represents generic type,
            not a DIE DW_OP_convert DW_OP_reinterpret */
        return;
    }
    if (!die) {
        esb_append(string_out," <No DIE, cannot verify die offset>");
        return;
    }
    is_info = dwarf_get_die_infotypes_flag(die);
    if (within_cu) {
        /* Our target DIE is in same CU as die argument */
        /*  DW_OP_const_type */
        Dwarf_Unsigned length = 0;

        res = dwarf_die_CU_offset_range(die,&globaloff,
            &length,&err);
        if (res != DW_DLV_OK) {
            esb_append_printf_s(string_out,
                " ERROR: %s Cannot access CU DIE global offset ",
                    (char *)op_name);
            if (res == DW_DLV_ERROR) {
                esb_append(string_out, dwarf_errmsg(err));
                dwarf_dealloc_error(dbg,err);
                err = 0;
            } else {
                esb_append(string_out, "DW_DLV_NO_ENTRY ");
            }
            glflags.gf_count_major_errors++;
            return;
        }
        /*  Offset passed in is off of CU header, not
            CU DIE. */
        globaloff += offset;
    } else {
        /*  DW_OP_implicit_ptr */
        globaloff = offset;
    }
    if ((required_offset == NON_ZERO_OFFSET_REQUIRED)
        && !globaloff) {
        esb_append_printf_s(string_out,
            "ERROR: %s  DIE global offset 0, but 0 not allowed ",
            op_name);
        glflags.gf_count_major_errors++;
        return;
    }
    if (!globaloff) {
        return;
    }
    res = dwarf_offdie_b(dbg,globaloff,is_info,
        &other_die,&err);
    if (res != DW_DLV_OK) {
        esb_append_printf_s(string_out,
            "ERROR: %s Cannot access DIE via global offset ",
            op_name);
        esb_append_printf_u(string_out,
            "0x%x ",globaloff);
        if (res == DW_DLV_ERROR) {
            esb_append(string_out, dwarf_errmsg(err));
            esb_append(string_out, " ");
            dwarf_dealloc_error(dbg,err);
            err = 0;
        } else {
            esb_append(string_out, "DW_DLV_NO_ENTRY ");
        }
        if (!within_cu) {
            esb_append(string_out,"DW_OP_implicit_ptr offset "
                "might apply to another object file ");
        }
        glflags.gf_count_major_errors++;
        return;
    }
    res = dwarf_tag(other_die,&tag,&err);
    if (res != DW_DLV_OK) {
        esb_append_printf_s(string_out,
            "ERROR: %s Cannot access DIE tag ",
            op_name);
        esb_append_printf_u(string_out,
            "0x%x ",globaloff);
        if (res == DW_DLV_ERROR) {
            esb_append(string_out, dwarf_errmsg(err));
            esb_append(string_out, " ");
            dwarf_dealloc_error(dbg,err);
            err = 0;
        } else {
            esb_append(string_out, "DW_DLV_NO_ENTRY ");
        }
        glflags.gf_count_major_errors++;
        dwarf_dealloc_die(other_die);
        return;
    }
    if (required_tag) {
        required_tag_name = get_TAG_name(required_tag);
    }
    actual_tag_name = get_TAG_name(tag);
    if (required_tag && tag != required_tag) {
        esb_append_printf_s(string_out,
            "ERROR: %s incorrect target die tag ",
            op_name);
        esb_append_printf_s(string_out,
            " Tag required: %s",required_tag_name);
        esb_append_printf_s(string_out,
            " Tag found: %s",actual_tag_name);
        glflags.gf_count_major_errors++;
    }
    if (!glflags.dense && !glflags.gf_expr_ops_joined) {
        char *diename = 0;

        /*  <0x000854ff GOFF=0x00546047> */
        esb_append(string_out,"\n");
        append_indent_prefix(string_out,indentprespaces,
            die_indent_level,indentpostspaces+2);
        esb_append(string_out," Target Die: ");
        if (within_cu) {
            esb_append_printf_u(string_out,
                "<0x%" DW_PR_XZEROS DW_PR_DUx ,offset);
            if (glflags.gf_show_global_offsets) {
                esb_append_printf_u(string_out,
                    " GOFF=0x%" DW_PR_XZEROS DW_PR_DUx,
                    globaloff);
            }
            esb_append(string_out,"> ");
        } else {
            esb_append_printf_u(string_out,
                "<GOFF=0x%" DW_PR_XZEROS DW_PR_DUx
                "> ",globaloff);
        }
        esb_append(string_out,actual_tag_name);
        res = dwarf_diename(other_die,&diename,&err);
        if ( res == DW_DLV_OK)  {
            esb_append_printf_s(string_out,
                " name: %s",diename);
        } else if (res == DW_DLV_ERROR) {
            esb_append_printf_s(string_out,
                "ERROR: gets error reading "
                "DW_AT_diename: %s ",
                dwarf_errmsg(err));
            dwarf_dealloc_error(dbg,err);
            err = 0;
        } else { /* Else no entry */ }
    }
    dwarf_dealloc_die(other_die);
}

/*  The first non-zero sibling offset we can find
    is what we want to return. The lowest sibling
    offset in the stack.  Or 0 if we have none known.
*/
static Dwarf_Off
get_die_stack_sibling(void)
{
    int i = die_stack_indent_level;
    for ( ; i >=0 ; --i)
    {
        Dwarf_Off v = die_stack[i].sibling_die_globaloffset_;
        if (v) {
            return v;
        }
    }
    return 0;
}
static void
possibly_increase_esb_alloc(struct esb_s *esbp,
    Dwarf_Unsigned count,
    Dwarf_Unsigned entrysize)
{
    /*  for bytes of text needed per element */
    Dwarf_Unsigned targetsize = count*entrysize;
    Dwarf_Unsigned used       = esb_string_len(esbp);
    Dwarf_Unsigned cursize    = esb_get_allocated_size(esbp);

    if ((targetsize+used) > cursize) {
        esb_force_allocation(esbp,targetsize+used);
    }
}
static void
dealloc_all_srcfiles(Dwarf_Debug dbg,
    char **srcfiles,
    Dwarf_Signed srcfiles_cnt)
{
    Dwarf_Signed i = 0;

    if (!srcfiles) {
        return;
    }
    for ( ; i < srcfiles_cnt; ++i) {
        dwarf_dealloc(dbg,srcfiles[i],DW_DLA_STRING);
    }
    dwarf_dealloc(dbg,srcfiles, DW_DLA_LIST);
}

/*  Higher stack level numbers must have a smaller sibling
    offset than lower or else the sibling offsets are wrong.
    Stack entries with sibling_die_globaloffset_ 0 must be
    ignored in this, it just means there was no sibling
    attribute at that level.
*/
static void
validate_die_stack_siblings(void)
{
    int i = die_stack_indent_level;
    Dwarf_Off innersiboffset = 0;
    for ( ; i >=0 ; --i)
    {
        Dwarf_Off v = die_stack[i].sibling_die_globaloffset_;
        if (v) {
            innersiboffset = v;
            break;
        }
    }
    if (!innersiboffset) {
        /* no sibling values to check. */
        return;
    }
    for (--i ; i >= 0 ; --i)
    {
        /* outersiboffset is an outer sibling offset. */
        Dwarf_Off outersiboffset =
            die_stack[i].sibling_die_globaloffset_;
        if (outersiboffset ) {
            if (outersiboffset < innersiboffset) {
                char small_buf[ESB_FIXED_ALLOC_SIZE];
                Dwarf_Error ouerr = 0;
                /* safe: all values known length. */
                struct esb_s pm;

                esb_constructor_fixed(&pm,small_buf,
                    sizeof(small_buf));
                esb_append_printf_u(&pm,
                    "ERROR: Die stack sibling error, "
                    "outer global offset "
                    "0x%"  DW_PR_XZEROS DW_PR_DUx,outersiboffset);
                esb_append_printf_u(&pm,
                    " less than inner global offset "
                    "0x%"  DW_PR_XZEROS DW_PR_DUx
                    ", the DIE tree is erroneous.",
                    innersiboffset);
                esb_append_printf_i(&pm,
                    "Die indent level; %d",i);
                print_error_and_continue(esb_get_string(&pm),
                    DW_DLV_OK,ouerr);
                esb_destructor(&pm);
                return;
            }
            /*  We only need check one level with an offset
                at each entry. */
            break;
        }
    }
    return;
}

static void
append_local_prefix(struct esb_s *esbp)
{
    esb_append(esbp,"\n      ");
}
static int
print_as_info_or_by_cuname(void)
{
    return (glflags.gf_info_flag || glflags.gf_types_flag
        || glflags.gf_cu_name_flag);
}

#if 0 /* debugging only */
/*  Only used for debugging. */
static void
dump_die_offsets(Dwarf_Debug dbg, Dwarf_Die die,
    const char *msg)
{
    Dwarf_Error dderr = 0;
    Dwarf_Off goff = 0;
    Dwarf_Off loff = 0;
    Dwarf_Half tag = 0;
    int res = 0;

    res = dwarf_die_offsets(die,&goff, &loff,&dderr);
    DROP_ERROR_INSTANCE(dbg,res,dderr);
    res = dwarf_tag(die, &tag, &dderr);
    DROP_ERROR_INSTANCE(dbg,res,dderr);
    printf("debugonly: Die tag 0x%x GOFF 0x%llx Loff 0x%llx %s\n",
        tag,goff,loff,msg);
}
#endif

/*  These for sure are not to debug_info or debug_types. */
Dwarf_Bool
dd_form_refers_local_info(Dwarf_Half form)
{
    switch(form) {
    case DW_FORM_GNU_ref_alt:
    case DW_FORM_GNU_strp_alt:
    case DW_FORM_strp_sup:
    case DW_FORM_line_strp:
        /*  These do not refer to the current
            section and cannot be checked
            as if they did. */
        return FALSE;
    default: break;
    }
    return TRUE;
}

/* process each compilation unit in .debug_info */
int
print_infos(Dwarf_Debug dbg,Dwarf_Bool is_info,
    Dwarf_Error *pi_err)
{
    int nres = 0;
    nres = print_one_die_section(dbg,is_info,pi_err);
    return nres;
}

static void
print_debug_fission_header(struct Dwarf_Debug_Fission_Per_CU_s *fsd)
{
    const char * fissionsec = ".debug_cu_index";
    unsigned i  = 0;
    struct esb_s hash_str;

    if (!fsd || !fsd->pcu_type) {
        /* No fission data. */
        return;
    }
    esb_constructor(&hash_str);
    printf("\n");
    if (!strcmp(fsd->pcu_type,"tu")) {
        fissionsec = ".debug_tu_index";
    }
    printf("  %-19s = %s\n","Fission section",fissionsec);
    printf("  %-19s = 0x%"  DW_PR_XZEROS DW_PR_DUx "\n",
        "Fission index ",
        fsd->pcu_index);
    format_sig8_string(&fsd->pcu_hash,&hash_str);
    printf("  %-19s = %s\n","Fission hash",esb_get_string(&hash_str));
    /* 0 is always unused. Skip it. */
    esb_destructor(&hash_str);
    printf("  %-19s = %s\n","Fission entries",
        "offset     size        DW_SECTn");
    for ( i = 1; i < DW_FISSION_SECT_COUNT; ++i)  {
        const char *nstring = 0;
        Dwarf_Unsigned off = 0;
        Dwarf_Unsigned size = fsd->pcu_size[i];
        int res = 0;
        if (size == 0) {
            continue;
        }
        res = dwarf_get_SECT_name(i,&nstring);
        if (res != DW_DLV_OK) {
            nstring = "Unknown SECT";
        }
        off = fsd->pcu_offset[i];
        printf("  %-19s = 0x%"  DW_PR_XZEROS DW_PR_DUx " 0x%"
            DW_PR_XZEROS DW_PR_DUx " %2d\n",
            nstring,
            off,size,i);
    }
}

static void
print_cu_hdr_cudie(Dwarf_Unsigned overall_offset,
    Dwarf_Unsigned offset )
{
    struct Dwarf_Debug_Fission_Per_CU_s fission_data;

    if (glflags.dense) {
        printf("\n");
        return;
    }
    memset(&fission_data,0,sizeof(fission_data));
    printf("\nCOMPILE_UNIT<header overall offset = 0x%"
        DW_PR_XZEROS DW_PR_DUx ">",
        (Dwarf_Unsigned)(overall_offset - offset));
    printf(":\n");
}

static  void
print_cu_hdr_std(Dwarf_Unsigned cu_header_length,
    Dwarf_Unsigned abbrev_offset,
    Dwarf_Half version_stamp,
    Dwarf_Half address_size,
    /* offset_size is often called length_size in libdwarf. */
    Dwarf_Half offset_size,
    int debug_fission_res,
    Dwarf_Half cu_type,
    struct Dwarf_Debug_Fission_Per_CU_s *fsd)
{
    int res = 0;
    const char *utname = 0;

    res = dwarf_get_UT_name(cu_type,&utname);
    if (res != DW_DLV_OK) {
        glflags.gf_count_major_errors++;
        utname = "ERROR";
    }
    if (glflags.dense) {
        printf("<%s>", "cu_header");
        printf(" %s<0x%" DW_PR_XZEROS  DW_PR_DUx
            ">", "cu_header_length",
            cu_header_length);
        printf(" %s<0x%04x>", "version_stamp",
            version_stamp);
        printf(" %s<0x%"  DW_PR_XZEROS DW_PR_DUx
            ">", "abbrev_offset", abbrev_offset);
        printf(" %s<0x%02x>", "address_size",
            address_size);
        printf(" %s<0x%02x>", "offset_size",
            offset_size);
        printf(" %s<0x%02x %s>", "cu_type",
            cu_type,utname);
        if (debug_fission_res == DW_DLV_OK) {
            struct esb_s hash_str;
            unsigned i = 0;

            esb_constructor(&hash_str);
            format_sig8_string(&fsd->pcu_hash,&hash_str);
            printf(" %s<0x%" DW_PR_XZEROS  DW_PR_DUx  ">",
                "fissionindex",
                fsd->pcu_index);
            printf(" %s<%s>", "fissionhash",
                esb_get_string(&hash_str));
            esb_destructor(&hash_str);
            for ( i = 1; i < DW_FISSION_SECT_COUNT; ++i)  {
                const char *nstring = 0;
                Dwarf_Unsigned off = 0;
                Dwarf_Unsigned size = fsd->pcu_size[i];
                int fires = 0;

                if (size == 0) {
                    continue;
                }
                fires = dwarf_get_SECT_name(i,&nstring);
                if (fires != DW_DLV_OK) {
                    nstring = "UnknownDW_SECT";
                }
                off = fsd->pcu_offset[i];
                printf(" %s< 0x%"  DW_PR_XZEROS DW_PR_DUx " 0x%"
                    DW_PR_XZEROS DW_PR_DUx ">",
                    nstring,
                    off,size);
            }
        }
    } else {
        printf("\nCU_HEADER:\n");
        printf("  %-16s = 0x%" DW_PR_XZEROS DW_PR_DUx
            " %" DW_PR_DUu
            "\n", "cu_header_length",
            cu_header_length,
            cu_header_length);
        printf("  %-16s = 0x%04x     %u\n", "version_stamp",
            version_stamp,version_stamp);
        printf("  %-16s = 0x%" DW_PR_XZEROS DW_PR_DUx
            " %" DW_PR_DUu
            "\n", "abbrev_offset",
            abbrev_offset,
            abbrev_offset);
        printf("  %-16s = 0x%02x       %u\n", "address_size",
            address_size,address_size);
        printf("  %-16s = 0x%02x       %u\n", "offset_size",
            offset_size,offset_size);
        printf("  %-16s = 0x%02x       %s\n", "cu_type",
            cu_type,utname);
        if (debug_fission_res == DW_DLV_OK) {
            print_debug_fission_header(fsd);
        }
    }
}
static void
print_cu_hdr_signature(Dwarf_Sig8 *signature,
    Dwarf_Unsigned typeoffset)
{
    if (glflags.dense) {
        struct esb_s sig8str;

        esb_constructor(&sig8str);
        format_sig8_string(signature,&sig8str);
        printf(" %s<%s>", "signature",esb_get_string(&sig8str));
        printf(" %s<0x%" DW_PR_XZEROS DW_PR_DUx ">",
            "typeoffset", typeoffset);
        esb_destructor(&sig8str);
    } else {
        struct esb_s sig8str;

        esb_constructor(&sig8str);
        format_sig8_string(signature,&sig8str);
        printf("  %-16s = %s\n", "signature",
            esb_get_string(&sig8str));
        printf("  %-16s = 0x%" DW_PR_XZEROS DW_PR_DUx
            " %" DW_PR_DUu "\n",
            "typeoffset",
            typeoffset,typeoffset);
        esb_destructor(&sig8str);
    }
}

static int
get_macinfo_offset(Dwarf_Die cu_die,
    Dwarf_Unsigned *offset,
    Dwarf_Error *macerr)
{
    Dwarf_Attribute attrib= 0;
    int vres = 0;
    int ares = 0;
    Dwarf_Bool is_info = TRUE;

    ares = dwarf_attr(cu_die, DW_AT_macro_info, &attrib, macerr);
    if (ares == DW_DLV_ERROR) {
        print_error_and_continue(
            "ERROR: getting dwarf_attr for DW_AT_macro_info"
            " failed",
            ares,*macerr);
        return ares;
    }
    if (ares == DW_DLV_NO_ENTRY) {
        return ares;
    }
    vres = dwarf_global_formref_b(attrib,offset,&is_info,macerr);
    if (vres == DW_DLV_ERROR) {
        dwarf_dealloc_attribute(attrib);
        print_error_and_continue(
        "ERROR: dwarf_global_formref_b on DW_AT_macro_info failed",
            vres, *macerr);
        return vres;
    }
    dwarf_dealloc_attribute(attrib);
    return vres;
}

static void
print_die_secname(Dwarf_Debug dbg,int is_info)
{
    if (print_as_info_or_by_cuname() &&
        glflags.gf_do_print_dwarf) {
        const char * section_name = 0;
        struct esb_s truename;
        char buf[ESB_FIXED_ALLOC_SIZE];

        if (is_info) {
            section_name = ".debug_info";
        } else  {
            section_name = ".debug_types";
        }
        esb_constructor_fixed(&truename,buf,sizeof(buf));
        get_true_section_name(dbg,section_name,
            &truename,TRUE);
        printf("\n%s\n",sanitized(esb_get_string(&truename)));
        esb_destructor(&truename);
    }
}

static Dwarf_Bool
empty_signature(const Dwarf_Sig8 *sigp)
{
    if (memcmp(sigp,&zerosig,sizeof(zerosig))) {
        return FALSE ; /* empty */
    }
    return TRUE;
}

static int
print_macinfo_for_cu(
    Dwarf_Debug dbg,
    Dwarf_Die cu_die2,
    Dwarf_Error *err)
{

    int mres = 0;
    Dwarf_Unsigned offset = 0;

    mres = get_macinfo_offset(cu_die2,&offset,err);
    if (mres == DW_DLV_NO_ENTRY) {
        /* By far the most likely result. */
        return mres;
    }else if (mres == DW_DLV_ERROR) {
        print_error_and_continue(
            "ERROR: get_macinfo_offset for "
            "DWARF 2,3,or 4 failed "
            "on a CU die",
            mres,*err);
        return mres;
    } else {
        mres = print_macinfo_by_offset(dbg,
            cu_die2,
            offset,err);
        if (mres==DW_DLV_ERROR) {
            struct esb_s m;

            esb_constructor(&m);
            esb_append_printf_u(&m,
                "\nERROR: printing macros for "
                " a CU at macinfo offset 0x%x "
                " failed ",offset);
            print_error_and_continue(esb_get_string(&m),
                mres,*err);
            esb_destructor(&m);
        }
    }
    return DW_DLV_OK;
}

#if 0 /* debugging only */
static void
dump_offset_list(Dwarf_Off *array,
    Dwarf_Unsigned count,
    int line)
{
    Dwarf_Unsigned i =0;
    printf("From line %d: count %lu\n",line,(unsigned long)count);
    for ( ; i < count; ++i) {
        printf("%3lu: 0x%08lx\n",(unsigned long)i,
            (unsigned long)array[i]);
    }
}
#endif

static void
dd_setup_die_check_functions(Dwarf_Debug dbg,
    Dwarf_Die       in_die_in,
    Dwarf_Bool      is_info,
    Dwarf_Off     **offset_array,
    Dwarf_Unsigned *offset_count,
    Dwarf_Error    *err)
{
    Dwarf_Off section_offset = 0;
    Dwarf_Off local_offset = 0;
    int       res = 0;

    if (!(glflags.gf_check_functions && checking_this_compiler())) {
        return;
    }
    DWARF_CHECK_COUNT(check_functions_result,1);
    res = dwarf_die_offsets(in_die_in,&section_offset,
        &local_offset,err);
    if (res != DW_DLV_OK) {
        DWARF_CHECK_ERROR(check_functions_result,
            "Unable to get die offsets");
        if (res == DW_DLV_ERROR) {
            DROP_ERROR_INSTANCE(dbg,res,*err);
        }
    } else {
        /*  This is the real intent. To check that
            this works. */
        res = DW_DLV_OK;
        res = dwarf_offset_list(dbg,section_offset,is_info,
            offset_array,offset_count,err);
        if (res == DW_DLV_ERROR) {
            DWARF_CHECK_ERROR(check_functions_result,
                "Unable to get dwarf_offset_list offsets");
            if (res == DW_DLV_ERROR) {
                DROP_ERROR_INSTANCE(dbg,res,*err);
            }
        } else if (res == DW_DLV_OK) {
            Dwarf_Unsigned i = 0;
            Dwarf_Unsigned count = *offset_count;
            Dwarf_Off     *off_array = *offset_array;
            Dwarf_Off      lastoffset = section_offset;

            for ( ; i < count; ++i) {
                Dwarf_Off off = off_array[i];

                if (off <= lastoffset) {
                    struct esb_s m;
                    char buf[100];

                    esb_constructor_fixed(&m,buf,sizeof(buf));
                    esb_append_printf_u(&m,
                        "dwarf_offset_list offset [%u]", i);
                    esb_append_printf_u(&m,
                        " 0x%" DW_PR_XZEROS DW_PR_DUx,
                        off);
                    esb_append_printf_u(&m,
                        "is below the previous of 0x%"
                        DW_PR_XZEROS DW_PR_DUx,
                        lastoffset);
                    DWARF_CHECK_ERROR(check_functions_result,
                        esb_get_string(&m));
                    esb_destructor(&m);
                    /* Skip further checks in this list */
                    break;
                }
            }
        }
    }
    return;
}

/* Called with a CU_Die as in_die_in. */
static int
print_die_and_children(Dwarf_Debug dbg,
    Dwarf_Die in_die_in,
    Dwarf_Off dieprint_cu_goffset,
    Dwarf_Bool is_info,
    char **srcfiles, Dwarf_Signed srcfiles_count,
    Dwarf_Error *err)
{
    int             res = 0;
    Dwarf_Off     *offset_array = 0;
    Dwarf_Unsigned offset_count = 0;

    /* A CU_die has a single child */
    local_symbols_already_begun = FALSE;
    res  = print_die_and_children_internal(dbg,
        in_die_in, dieprint_cu_goffset, is_info,
        srcfiles,srcfiles_count,
        offset_array,offset_count,err);
    return res;
}

static int
print_cu_hdr_abbrev_data(Dwarf_Debug dbg,
    Dwarf_Off aboffset,
    Dwarf_Error *error)
{
    Dwarf_Unsigned initial_abbnum = 1;
    Dwarf_Unsigned final_abbnum = 0;
    Dwarf_Unsigned length = 0;
    int ores = 0;

    if (glflags.gf_do_print_dwarf) {
        printf("\nAbbreviation table this CU, offset "
            "0x%" DW_PR_XZEROS DW_PR_DUx ":\n",
            aboffset);
    }
    ores = print_all_abbrevs_for_cu(dbg,
        aboffset,
        initial_abbnum,
        &length,&final_abbnum,error);
    return ores;

}

static void
fill_in_producer_name(Dwarf_Debug dbg,Dwarf_Die cu_die,
    Dwarf_Unsigned dieprint_cu_goffset)

{
    /*  Get producer name for this CU and
        update compiler list */
    int cures = 0;
    struct esb_s producername;
    Dwarf_Error pod_err = 0;

    esb_constructor(&producername);
    /*  Fills in some producername no matter
        what status returned. */
    cures  = get_producer_name(dbg,cu_die,
        dieprint_cu_goffset,&producername,&pod_err);
    if (cures == DW_DLV_OK) {
        update_compiler_target(esb_get_string(&producername));
    } else if (cures == DW_DLV_ERROR) {
        DROP_ERROR_INSTANCE(dbg,cures,pod_err);
    }
    esb_destructor(&producername);
}

static int
fill_in_compiler_target(Dwarf_Debug dbg,Dwarf_Die cu_die,
    Dwarf_Unsigned dieprint_cu_goffset,
    Dwarf_Error*pod_err)
{
    char * cu_short_name = NULL;
    char * cu_long_name = NULL;
    int chres = 0;

    chres = get_cu_name(dbg,cu_die,
        dieprint_cu_goffset,
        &cu_short_name,&cu_long_name,
        pod_err);
    if (chres == DW_DLV_ERROR ) {
        return chres;
    }
    if (chres == DW_DLV_OK) {
        /* Add CU name to current compiler entry */
        add_cu_name_compiler_target(cu_long_name);
    }
    return chres;
}

static void
suppress_irrelevant_checking(void)
{
    /*  In a .dwp file some checks get all sorts
        of spurious errors.  */
    glflags.gf_suppress_checking_on_dwp = TRUE;
    glflags.gf_check_ranges = FALSE;
    glflags.gf_check_aranges = FALSE;
    glflags.gf_check_decl_file = FALSE;
    glflags.gf_check_lines = FALSE;
    glflags.gf_check_pubname_attr = FALSE;
    glflags.gf_check_fdes = FALSE;
}

static void
reset_error_reporting_globals(void)
{
    /*  We have not seen the compile unit  yet, reset these
        error-reporting  globals. */
    glflags.seen_CU = FALSE;
    glflags.need_CU_name = TRUE;
    glflags.need_CU_base_address = TRUE;
    glflags.need_CU_high_address = TRUE;
    /*  Some prerelease gcc versions used ranges but seemingly
        assumed the lack of a base address in the CU was
        defined to be a zero base.
        Assuming a base address (and low and high) is sensible. */
    glflags.CU_base_address = 0;
    glflags.CU_high_address = 0;
    glflags.CU_low_address = 0;
}

static void
print_cu_header_data_or_signature(
    Dwarf_Unsigned cu_header_length,
    Dwarf_Unsigned abbrev_offset,
    Dwarf_Half version_stamp,
    Dwarf_Half address_size,
    Dwarf_Half length_size,
    int fission_data_result,
    Dwarf_Half cu_type,
    struct Dwarf_Debug_Fission_Per_CU_s  *fission_data,
    Dwarf_Sig8 *signature,
    Dwarf_Unsigned typeoffset)
{
    if (glflags.verbose) {
        print_cu_hdr_std(cu_header_length,abbrev_offset,
            version_stamp,address_size,length_size,
            fission_data_result,cu_type,fission_data);
        if (!empty_signature(signature)) {
            print_cu_hdr_signature(signature,typeoffset);
        }
        if (glflags.dense) {
            printf("\n");
        }
    } else {
        if (!empty_signature(signature)) {
            if (glflags.dense) {
                printf("<%s>", "cu_header");
            } else {
                printf("\nCU_HEADER:\n");
            }
            print_cu_hdr_signature(signature,typeoffset);
            if (glflags.dense) {
                printf("\n");
            }
        }
    }
}

/*   */
static int
print_one_die_section(Dwarf_Debug dbg,Dwarf_Bool is_info,
    Dwarf_Error *pod_err)
{
    Dwarf_Unsigned cu_header_length = 0;
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half version_stamp = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Half extension_size = 0;
    Dwarf_Half length_size = 0;
    Dwarf_Unsigned typeoffset = 0;
    Dwarf_Unsigned next_cu_offset = 0;
    unsigned loop_count = 0;
    int nres = DW_DLV_OK;
    int   cu_count = 0;
    int res = 0;
    Dwarf_Off dieprint_cu_goffset = 0;

    glflags.current_section_id = is_info?DEBUG_INFO:
        DEBUG_TYPES;
    {
        const char * test_section_name = 0;
        res = dwarf_get_die_section_name(dbg,is_info,
            &test_section_name,pod_err);
        if (res == DW_DLV_NO_ENTRY) {
            if (!is_info) {
                /*  No .debug_types. Do not print
                    .debug_types name */
                return res;
            }
        }
    }
    /*  Start loop on loop_count (CUs) */
    for (;;++loop_count) {
        Dwarf_Die  cu_die = 0;
        Dwarf_Die  cu_die2 = 0;
        struct Dwarf_Debug_Fission_Per_CU_s fission_data;
        int        fission_data_result = 0;
        Dwarf_Half cu_type = 0;
        Dwarf_Sig8 signature;
        int        offres = 0;

        signature = zerosig;
        /*  glflags.DIE_section_offset: in case
            dwarf_next_cu_header_d fails due
            to corrupt dwarf. */
        glflags.DIE_section_offset = dieprint_cu_goffset;
        memset(&fission_data,0,sizeof(fission_data));
#ifdef  ORIGINAL_HEADER_API
        /* a sample. Other places changed with no ifdef */
        nres = dwarf_next_cu_header_d(dbg,
            is_info,
            &cu_header_length, &version_stamp,
            &abbrev_offset, &address_size,
            &length_size,&extension_size,
            &signature, &typeoffset,
            &next_cu_offset,
            &cu_type, pod_err);
#else
        nres = dwarf_next_cu_header_e(dbg,
            is_info,
            &cu_die,
            &cu_header_length, &version_stamp,
            &abbrev_offset, &address_size,
            &length_size,&extension_size,
            &signature, &typeoffset,
            &next_cu_offset,
            &cu_type, pod_err);
#endif /* ORIGINAL_HEADER_API */

        if (nres == DW_DLV_OK && glflags.gf_print_all_srcfiles) {
            dd_all_srcfiles_insert_new(dbg,cu_die);
        }

        if (!loop_count) {
            /*  So compress flags show, we waited till
                section loaded to do this. */
            print_die_secname(dbg,is_info);
        }
        if (nres == DW_DLV_NO_ENTRY) {
            return nres;
        }
        if (nres == DW_DLV_ERROR) {
            /*  With corrupt DWARF due to a bad CU die
                we won't know much. */
            print_error_and_continue(
                "ERROR: Failure reading CU header"
                " or DIE, corrupt DWARF", nres, *pod_err);
            return nres;
        }
        if (cu_count >= glflags.break_after_n_units) {
            const char *m = "CUs";
            if (cu_count == 1) {
                m="CU";
            }
            printf("Break at %d %s\n",cu_count,m);
            dwarf_dealloc_die(cu_die);
            cu_die = 0;
            break;
        }
        /*  Get the CU offset  (when we can)
            for easy error reporting. Ignore errors. */
        offres = dwarf_die_offsets(cu_die,
            &glflags.DIE_section_offset,
            &glflags.DIE_offset,pod_err);
        DROP_ERROR_INSTANCE(dbg,offres,*pod_err);
        glflags.DIE_CU_overall_offset = glflags.DIE_section_offset;
        glflags.DIE_CU_offset = glflags.DIE_offset;
        dieprint_cu_goffset = glflags.DIE_section_offset;

        if (glflags.gf_cu_name_flag) {
            Dwarf_Bool should_skip = FALSE;

            /* always sets should_skip, even if error */
            should_skip_this_cu(dbg, &should_skip,cu_die);
            if (should_skip) {
                dwarf_dealloc_die(cu_die);
                cu_die = 0;
                ++cu_count;
                continue;
            }
        }
        fill_in_producer_name(dbg,cu_die, dieprint_cu_goffset);
        /*  Once the compiler table has been updated, see
            if we need to generate the list of CU compiled
            by all the producers contained in the elf file */
        if (glflags.gf_producer_children_flag) {
            int chres = 0;
            chres = fill_in_compiler_target(dbg,cu_die,
                dieprint_cu_goffset,pod_err);
            if (chres == DW_DLV_ERROR ) {
                dwarf_dealloc_die(cu_die);
                cu_die = 0;
                return chres;
            }
        }

        /*  If the current compiler is not requested by the
            user, then move to the next CU */
        if (!checking_this_compiler()) {
            dwarf_dealloc_die(cu_die);
            ++cu_count;
            cu_die = 0;
            continue;
        }
        fission_data_result = dwarf_get_debugfission_for_die(
            cu_die,
            &fission_data,pod_err);
        if (fission_data_result == DW_DLV_ERROR) {
            dwarf_dealloc_die(cu_die);
            cu_die = 0;
            print_error_and_continue(
                "ERROR: Failure looking for Debug Fission data",
                fission_data_result, *pod_err);
            return fission_data_result;
        }
        if (fission_data_result == DW_DLV_OK) {
            suppress_irrelevant_checking();
        }
        reset_error_reporting_globals();

        if ((glflags.gf_info_flag || glflags.gf_types_flag) &&
            glflags.gf_do_print_dwarf) {
            print_cu_header_data_or_signature(cu_header_length,
                abbrev_offset, version_stamp,address_size,
                length_size, fission_data_result,cu_type,
                &fission_data, &signature,typeoffset);
        }
        if (glflags.gf_check_abbreviations ||
            (glflags.verbose > 3 &&
            (glflags.gf_info_flag || glflags.gf_types_flag) &&
            glflags.gf_do_print_dwarf)) {
            int hares = 0;
            hares = print_cu_hdr_abbrev_data(dbg,
                abbrev_offset,
                pod_err);
            if (hares == DW_DLV_ERROR) {
                /*  Will be reported later, no doubt. */
                DROP_ERROR_INSTANCE(dbg,hares,*pod_err);
                *pod_err = 0;
            }
        }

        /*  Get abbreviation info for this CU, given
            the abbrev offset */
        get_abbrev_array_info(dbg,abbrev_offset);

        /*  Process a single compilation unit in .debug_info or
            .debug_types. */
        cu_die2 = cu_die;
        cu_die = 0;
        {
            int pres = 0;
            Dwarf_Signed srcfiles_cnt = 0;
            char **srcfiles = 0;
            int srcf =  0;
            Dwarf_Error srcerr = 0;

            srcf = dwarf_srcfiles(cu_die2,
                &srcfiles, &srcfiles_cnt, &srcerr);
            if (srcf == DW_DLV_ERROR) {
                print_error_and_continue(
                    "ERROR: dwarf_srcfiles problem ",
                    srcf,srcerr);
                DROP_ERROR_INSTANCE(dbg,srcf,srcerr);
                srcfiles = 0;
                srcfiles_cnt = 0;
            } else if (srcf == DW_DLV_NO_ENTRY) {
                /*  DW_DLV_NO_ENTRY generally means there
                    there is no DW_AT_stmt_list attribute.
                    and we do not want to print anything
                    about statements in that case */
            }
            if (print_as_info_or_by_cuname() ||
                glflags.gf_search_is_on) {
                    /*  Do regardless if dwarf_srcfiles
                        was successful to print die
                        and children as best we can
                        even with errors . */
                    int podres2 = 0;
                    Dwarf_Error lperr = 0;
                    /* Get the CU offset for easy error reporting */
                    podres2 = dwarf_die_offsets(cu_die2,
                        &glflags.DIE_section_offset,
                        &glflags.DIE_offset,&lperr);
                    DROP_ERROR_INSTANCE(dbg,podres2,lperr);
                    glflags.DIE_CU_overall_offset =
                        glflags.DIE_section_offset;
                    glflags.DIE_CU_offset = glflags.DIE_offset;
                    dieprint_cu_goffset = glflags.DIE_section_offset;

                    pres = print_die_and_children(dbg, cu_die2,
                        dieprint_cu_goffset,is_info,
                            srcfiles, srcfiles_cnt,pod_err);
                    if (pres == DW_DLV_ERROR) {
                        if (srcfiles) {
                            dealloc_all_srcfiles(dbg,srcfiles,
                                srcfiles_cnt);
                            srcfiles = 0;
                            srcfiles_cnt = 0;
                        }
                        dwarf_dealloc_die(cu_die2);
                        return pres;
                    }
                }
                if (glflags.nTrace[KIND_RANGES_INFO]) {
                    PrintBucketGroup(
                        "print_one_die_info_section PD A",
                        glflags.pRangesInfo);
                }

                /* Check the range array if in check mode */
                if ( glflags.gf_check_ranges) {
                    int rares = 0;
                    Dwarf_Error raerr = 0;

                    rares = check_range_array_info(dbg,
                        cu_die2,&raerr);
                    if (rares == DW_DLV_ERROR) {
                        print_error_and_continue(
                            "ERROR: range array checks for "
                            "the current CU failed. ",
                            rares,raerr);
                        DROP_ERROR_INSTANCE(dbg,rares,raerr);
                    }
                }

                /*  Traverse the line section if in check mode
                    or if line-printing requested */
                if (glflags.gf_line_flag ||
                    glflags.gf_check_decl_file) {
                    int plnres = 0;

                    int oldsection = glflags.current_section_id;
                    plnres = print_line_numbers_this_cu(dbg,
                        cu_die2,
                        srcfiles,srcfiles_cnt,pod_err);
                    if (plnres == DW_DLV_ERROR) {
                        print_error_and_continue(
                            "ERROR: Printing line numbers for "
                            "the current CU failed. ",
                            plnres,*pod_err);
                        /*  Suppress the error so we print
                            whatever we can */
                        DROP_ERROR_INSTANCE(dbg,plnres,*pod_err);
                    }
                    glflags.current_section_id = oldsection;
                }
                /*  We are not currently checking the macro
                    import trees for (infinite) loops.
                    We do not follow the import tree directly
                    so we will not loop forever here. */
                if (glflags.gf_macro_flag ||
                    glflags.gf_check_macros) {
                    int mres = 0;
                    Dwarf_Bool in_import_list = FALSE;
                    Dwarf_Unsigned import_offset = 0;
                    int oldsection = glflags.current_section_id;
                    /*  DWARF5 .debug_macro (version 5
                        in the macro header) or GNU extension
                        of DWARF4 .debug_macro, with version 4
                        in the macro header. */

                    macro_import_stack_cleanout();
                    mres = print_macros_5style_this_cu(dbg, cu_die2,
                        srcfiles,srcfiles_cnt,
                        glflags.gf_do_print_dwarf,
                        TRUE /* descend_into_imports */,
                        in_import_list,
                        import_offset,pod_err);
                    if (mres == DW_DLV_ERROR) {
                        print_error_and_continue(
                            "ERROR: Printing DWARF5 macros "
                            "for the current CU failed. ",
                            mres,*pod_err);
                        /*  Suppress the error so we print
                            whatever we can */
                        DROP_ERROR_INSTANCE(dbg,mres,*pod_err);
                    }

                    macro_import_stack_cleanout();
                    in_import_list = TRUE;
                    /*  The above would have checked all reachable
                        macro units, so no need to check here. */
                    if (!glflags.gf_do_check_dwarf &&
                        mres == DW_DLV_OK) {
                        for (;;) {
                            /* Never returns DW_DLV_ERROR */
                            mres = get_next_unprinted_macro_offset(
                                &macro_check_tree, &import_offset);
                            if (mres != DW_DLV_OK) {
                                break;
                            }
                            macro_import_stack_cleanout();
                            mres = print_macros_5style_this_cu(dbg,
                                cu_die2,
                                srcfiles,srcfiles_cnt,
                                glflags.gf_do_print_dwarf,
                                FALSE /* no descend_into_imports */,
                                in_import_list,
                                import_offset,
                                pod_err);
                            if (mres == DW_DLV_ERROR) {
                                struct esb_s m;

                                esb_constructor(&m);
                                esb_append_printf_u(&m,
                                    "ERROR: Printing DWARF5 macros "
                                    " at offset 0x%x "
                                    "for the current macro import"
                                    " in the macros  failed. ",
                                    import_offset);
                                print_error_and_continue(
                                    esb_get_string(&m),
                                    mres,*pod_err);
                                DROP_ERROR_INSTANCE(dbg,mres,
                                    *pod_err);
                                esb_destructor(&m);
                                break;
                            }
                            macro_import_stack_cleanout();
                        }
                    }
                    glflags.current_section_id = oldsection;
                }
                if ( glflags.gf_macinfo_flag ||
                    glflags.gf_check_macros) {
                    int mres = 0;
                    /*  Macros have no version number before
                        DWARF 5. */

                    mres = print_macinfo_for_cu(dbg,cu_die2,
                        pod_err);
                    if (mres == DW_DLV_ERROR) {
                        if (srcfiles) {
                            dealloc_all_srcfiles(dbg,srcfiles,
                                srcfiles_cnt);
                            srcfiles = 0;
                            srcfiles_cnt = 0;
                        }
                        dwarf_dealloc_die(cu_die2);
                        return mres;
                    }
                }
                if (srcfiles) {
                    dealloc_all_srcfiles(dbg,srcfiles,srcfiles_cnt);
                    srcfiles = 0;
                    srcfiles_cnt = 0;
                }
            }
            if (cu_die2) {
                dwarf_dealloc_die(cu_die2);
                cu_die2 = 0;
            }
            ++cu_count;
        } /*  End loop on loop_count (CUs) */
        return nres;
    }

static int
print_a_die_stack(Dwarf_Debug dbg,
    char **srcfiles,
    Dwarf_Signed srcfiles_cnt,
    int lev,
    Dwarf_Error *err)
{
    Dwarf_Bool print_else_name_match = TRUE;
    Dwarf_Bool ignore_die_stack = FALSE;
    Dwarf_Bool attribute_matched = FALSE;
    int res = 0;

    res = print_one_die(dbg,
        die_stack[lev].die_,
        die_stack[lev].cu_die_offset_,
        print_else_name_match,lev,srcfiles,srcfiles_cnt,
        &attribute_matched,
        ignore_die_stack,
        err);
    return res;
}

static int
print_die_stack(Dwarf_Debug dbg,
    char **srcfiles,
    Dwarf_Signed srcfiles_cnt,
    Dwarf_Error*err)
{
    int lev = 0;
    /*  Print_information TRUE means attribute_matched
        will NOT be set by attribute name match.
        Just print the dies in the stack.*/

    Dwarf_Bool print_else_name_match = TRUE;
    Dwarf_Bool ignore_die_stack = FALSE;
    Dwarf_Bool attribute_matched = FALSE;

    for (lev = 0; lev <= die_stack_indent_level; ++lev)
    {
        int res = 0;
        res = print_one_die(dbg,die_stack[lev].die_,
            die_stack[lev].cu_die_offset_,
            print_else_name_match,
            lev,srcfiles,srcfiles_cnt,
            &attribute_matched,
            ignore_die_stack,err);
        if (res == DW_DLV_ERROR) {
            return res;
        }
    }
    return DW_DLV_OK;
}

static int
dd_check_tag_tree(Dwarf_Debug dbg,
    Dwarf_Die in_die,
    Dwarf_Error *err)
{
    if ( glflags.gf_check_tag_tree ||
        glflags.gf_print_usage_tag_attr) {
        DWARF_CHECK_COUNT(tag_tree_result,1);
        if (die_stack_indent_level == 0) {
            Dwarf_Half tag = 0;
            int dtres = 0;

            dtres = dwarf_tag(in_die, &tag, err);
            if (dtres != DW_DLV_OK) {
                DROP_ERROR_INSTANCE(dbg,dtres,*err);
                DWARF_CHECK_ERROR(tag_tree_result,
                    "Tag-tree root tag unavailable: "
                    "is not DW_TAG_compile_unit");
            } else if (tag == DW_TAG_skeleton_unit) {
                /* OK */
            } else if (tag == DW_TAG_compile_unit) {
                /* OK */
            } else if (tag == DW_TAG_partial_unit) {
                /* OK */
            } else if (tag == DW_TAG_type_unit) {
                /* OK */
            } else {
                DWARF_CHECK_ERROR(tag_tree_result,
                    "tag-tree root is not "
                    "DW_TAG_compile_unit "
                    "or DW_TAG_partial_unit or "
                    "DW_TAG_type_unit");
            }
            return DW_DLV_OK;
        }
        {
            Dwarf_Half tag_parent = 0;
            Dwarf_Half tag_child = 0;
            int pres = 0;
            int cres = 0;

            pres = dwarf_tag(die_stack[
                die_stack_indent_level - 1].die_,
                &tag_parent, err);
            if (pres != DW_DLV_OK) {
                return pres;
            }
            cres = dwarf_tag(in_die, &tag_child, err);
            if (cres != DW_DLV_OK) {
                return cres;
            }

            /* Check for specific compiler */
            if (checking_this_compiler()) {
                unsigned char ttresult = 0;
                /* Process specific TAGs. */
                tag_specific_globals_setup(dbg,tag_child,
                    die_stack_indent_level);
                ttresult = legal_tag_tree_combination(
                    tag_parent, tag_child);
                switch(ttresult) {
                case TK_OK:
                    break;
                case TK_SHOW_MESSAGE:
                    if (glflags.gf_check_tag_tree) {
                        DWARF_CHECK_ERROR3(tag_tree_result,
                        get_TAG_name(tag_parent),
                            get_TAG_name(tag_child),
                            "tag-tree relation is "
                            "not standard.");
                    }
                    break;
                case TK_ERROR: /* fall thru */
                default:
                    {
                        static unsigned long errcount = 0;
                        if (!errcount) {
                            printf("ERROR: Tag parent 0x%x "
                            "tag child 0x%x fails for unknown"
                            " reason, possibly out of memory"
                            " building large search trees."
                            " This message will not repeat\n",
                            tag_parent,tag_child);
                            glflags.gf_count_major_errors++;
                        }
                        ++errcount;
                    }
                    break;
                }
            }
        }
    }
    return DW_DLV_OK;
}

static void
check_sibling_off(Dwarf_Unsigned loop_iteration,
    Dwarf_Off     *sibling_off_array,
    Dwarf_Unsigned sibling_off_count)
{
    if (!sibling_off_count) {
        return;
    }
    DWARF_CHECK_COUNT(check_functions_result,1);
    if ((loop_iteration+1) > sibling_off_count) {
        DWARF_CHECK_ERROR(check_functions_result,
            "The offset count from dwarf_offset_list"
            " is smaller than the actual sibling count");
    } else {
        Dwarf_Unsigned i = 0;
        Dwarf_Off      off = 0;
        struct esb_s m;
        Dwarf_Off check_off = glflags.DIE_section_offset;

        /*  The following could use binary search as
            the array is strictly ascending values. */
        for ( ; i < sibling_off_count; ++i) {
            off = sibling_off_array[i];
            if (check_off == off) {
                /* Good. Found */
                return;
            }
        }
        esb_constructor(&m);
        esb_append_printf_u(&m,
            "An offset  from dwarf_offset_list"
            " of 0x%x"
            " is different than any actual"
            " sibling offset",glflags.DIE_section_offset);
        DWARF_CHECK_ERROR(check_functions_result,
            esb_get_string(&m));
        esb_destructor(&m);
    }
}

static void
check_sibling_offset_list_count(Dwarf_Unsigned loop_iteration,
    Dwarf_Unsigned sibling_off_count)
{
    if (!sibling_off_count) {
        return;
    }
    if ((loop_iteration+1) != sibling_off_count) {
        DWARF_CHECK_COUNT(check_functions_result,1);
        DWARF_CHECK_ERROR(check_functions_result,
        "The offset count from dwarf_offset_list"
        " does not match the actual sibling count");
    }
}

/*  Here do pre-descent printing of the die
    The die stack is a global static array
    of a  struct. See DIE_STACK
*/
static int
dd_print_die_and_die_stack(Dwarf_Debug dbg,
    Dwarf_Die      in_die,
    Dwarf_Unsigned dieprint_cu_goffset,
    char         **srcfiles,
    Dwarf_Signed   srcfilescount,
    Dwarf_Error *  err)
{
    int pdres = 0;
    Dwarf_Bool an_attribute_match_local = FALSE;
    Dwarf_Bool ignore_die_stack = FALSE;

    pdres = print_one_die(dbg, in_die,
        dieprint_cu_goffset,
        print_as_info_or_by_cuname(),
        die_stack_indent_level, srcfiles, srcfilescount,
        &an_attribute_match_local,
        ignore_die_stack,
        err);
    if (pdres != DW_DLV_OK) {
        return pdres;
    }
    validate_die_stack_siblings();
    if (!print_as_info_or_by_cuname() &&
        an_attribute_match_local) {
        if (glflags.gf_display_parent_tree) {
            pdres = print_die_stack(dbg,srcfiles,
                srcfilescount, err);
            if (pdres == DW_DLV_ERROR) {
                return pdres;
            }
        } else {
            if (glflags.gf_display_children_tree) {
                pdres = print_a_die_stack(
                    dbg,srcfiles,srcfilescount,
                    die_stack_indent_level,err);
                if (pdres == DW_DLV_ERROR) {
                    return pdres;
                }
            }
        }
        if (glflags.gf_display_children_tree) {
            glflags.gf_stop_indent_level = die_stack_indent_level;
            glflags.gf_info_flag = TRUE;
            glflags.gf_types_flag = TRUE;
        }
    }
    return DW_DLV_OK;
}

static int
dd_check_die_abbrevs(Dwarf_Debug dbg, Dwarf_Die in_die,
    int childres)
{
    Dwarf_Half ab_has_child;
    Dwarf_Bool bError = FALSE;
    Dwarf_Half tag = 0;
    int abtres = 0;

    /* This does not return a Dwarf_Error value! */
    abtres = dwarf_die_abbrev_children_flag(in_die,
        &ab_has_child);
    if (abtres == DW_DLV_OK) {
        Dwarf_Error tagerr = 0;
        int tagres = 0;

        DWARF_CHECK_COUNT(abbreviations_result,1);
        tagres = dwarf_tag(in_die, &tag, &tagerr);
        if (tagres == DW_DLV_OK) {
            switch (tag) {
            case DW_TAG_array_type:
            case DW_TAG_class_type:
            case DW_TAG_compile_unit:
            case DW_TAG_type_unit:
            case DW_TAG_partial_unit:
            case DW_TAG_enumeration_type:
            case DW_TAG_lexical_block:
            case DW_TAG_namespace:
            case DW_TAG_structure_type:
            case DW_TAG_subprogram:
            case DW_TAG_subroutine_type:
            case DW_TAG_union_type:
            case DW_TAG_entry_point:
            case DW_TAG_inlined_subroutine:
                break;
            default:
                /*  Once in a while Go compilers say DW_TAG_typedef
                    has children but the abbreviation list does
                    not reflect that properly.
                    We don't think a child of that TAG
                    is appropriate: we think it is a compiler bug. */
                bError = (childres == DW_DLV_OK && !ab_has_child) ||
                    (childres == DW_DLV_NO_ENTRY && ab_has_child);
                if (bError) {
                    struct esb_s pm;
                    const char *tagname = "<unknown DW_TAG>";
                    const char *chres = (childres == DW_DLV_OK)?
                        "yes":"no";
                    const char *hasch = (ab_has_child)?"yes":"no";

                    esb_constructor(&pm);
                    tagname = get_TAG_name(tag);
                    esb_append_printf_s(&pm,
                        "check 'dw_children'."
                        " abbrev says has child? %s.",
                        chres);
                    esb_append_printf_s(&pm,
                        " While the abbreviation list"
                        " shows a child? %s.",
                        hasch);
                    esb_append_printf_s(&pm,
                        " Check flag combination"
                        " on %s. \n",tagname);
                    DWARF_CHECK_ERROR(
                        abbreviations_result,
                        esb_get_string(&pm));
                    esb_destructor(&pm);

                }
                break;
            }
        } else if (tagres == DW_DLV_ERROR) {
            print_error_and_continue(
                "Unable to read die tag!",
                tagres,tagerr);
            dwarf_dealloc_error(dbg,tagerr);
            return DW_DLV_NO_ENTRY;
        }
    } else if (abtres == DW_DLV_ERROR) {
        glflags.gf_count_major_errors++;
        printf("ERROR: Unable to read die children "
            "flag.\n");
        /*  return is unrelated to *error
            but leaving *error NULL should be ok. */
        return abtres;
    }
    return DW_DLV_OK;
}

static int
dwarf_check_child_offset(Dwarf_Debug dbg,
    Dwarf_Die child,
    Dwarf_Error *err)
{
    /*  If the global offset of the (first) child is
        <= the parent DW_AT_sibling global-offset-value
        then the compiler has made a mistake, and
        the DIE tree is corrupt.  */
    int       childoffres = 0;
    Dwarf_Off child_overall_offset = 0;

    childoffres = dwarf_dieoffset(child,
        &child_overall_offset, err);
    /*  ASSERT: never returns DW_DLV_NO_ENTRY */
    if (childoffres == DW_DLV_ERROR) {
        print_error_and_continue(
            "Finding a DIE offset (dwarf_dieoffset())"
            "failed.",childoffres,*err);
        return DW_DLV_ERROR;
    }
    if (childoffres == DW_DLV_OK) {
        Dwarf_Off parent_sib_val =
            get_die_stack_sibling();
        if (parent_sib_val &&
            (parent_sib_val <= child_overall_offset )) {
            char small_buf[ESB_FIXED_ALLOC_SIZE];
            struct esb_s pm;

            esb_constructor_fixed(&pm,small_buf,
                sizeof(small_buf));
            esb_append_printf_u(&pm,
                "ERROR: A parent DW_AT_sibling of "
                "0x%" DW_PR_XZEROS  DW_PR_DUx,
                parent_sib_val);
            esb_append_printf_s(&pm,
                " points %s the first child ",
                (parent_sib_val == child_overall_offset)?
                    "at":"before");
            esb_append_printf_u(&pm,
                "0x%"  DW_PR_XZEROS  DW_PR_DUx
                " so the die tree is corrupt "
                "(showing section, not CU, offsets). ",
                child_overall_offset);
            dwarf_error_creation(dbg,err,
                esb_get_string(&pm));
            print_error_and_continue(
                esb_get_string(&pm),
                DW_DLV_ERROR,*err);
            /*   Original test did a print_error()
                here, which did exit().
                We would like to return ERROR all the way
                back, but have no way at present
                to generate a Dwarf_Error record.
                Because these sorts of errors
                are not really recoverable.
            */
            esb_destructor(&pm);
            return DW_DLV_ERROR;
        }
    }
    return DW_DLV_OK;
}

static void
dd_validate_die_sibling(Dwarf_Debug dbg, Dwarf_Die sibling)
{
    Dwarf_Off glb_off = 0;
    DWARF_CHECK_COUNT(di_gaps_result,1);
    if (!sibling) {
        return;
    }
    if (dwarf_validate_die_sibling(sibling,&glb_off) ==
        DW_DLV_ERROR) {
        Dwarf_Off    sib_off;
        struct esb_s msg;
        Dwarf_Error  err = 0;
        int          dores = 0;

        esb_constructor(&msg);
        dores = dwarf_dieoffset(sibling,&sib_off,&err);
        if (dores == DW_DLV_OK) {
            esb_append_printf_u(&msg,
                "GSIB = 0x%" DW_PR_XZEROS  DW_PR_DUx,
                sib_off);
            esb_append_printf_u(&msg,
                " GOFF = 0x%" DW_PR_XZEROS DW_PR_DUx,
                glb_off);
            esb_append_printf_u(&msg,
                " Gap = %" DW_PR_DUu " bytes",
                sib_off-glb_off);
            DWARF_CHECK_ERROR2(di_gaps_result,
                "Incorrect sibling chain",
                esb_get_string(&msg));
            esb_destructor(&msg);
            return;
        }
        glflags.gf_count_major_errors++;
        if (dores == DW_DLV_ERROR) {
            printf("ERROR: Offset of sibling is unavailable "
                "(traversing die tree) "
                "which implies corrupt DWARF %s\n",
                dwarf_errmsg(err));
            dwarf_dealloc_error(dbg,err);
            err = 0;
            return;
        }
        printf("ERROR: Offset of sibling is unavailable "
            "(traversing die tree) "
            "which implies corrupt DWARF is corrupt\n");
    }
}

/*  Recursively follow the die tree.
    For in_die and its siblings we loop here,
    touching eash sibling of in_die.
    For children of in_die we recurse on this function.
    We use a DIE_STACK to keep track of where
    we are in the current loop and in the recursion.
*/
static int
print_die_and_children_internal(Dwarf_Debug dbg,
    Dwarf_Die        in_die_in,
    Dwarf_Off        dieprint_cu_goffset,
    Dwarf_Bool       is_info,
    char           **srcfiles,
    Dwarf_Signed     srcfilescount,
    Dwarf_Off       *sibling_off_array,
    Dwarf_Unsigned   sibling_off_count,
    Dwarf_Error     *err)
{
    Dwarf_Die sibling = 0;
    Dwarf_Die in_die = in_die_in;
    Dwarf_Unsigned loop_iteration = 0;

    /*  Loop through siblings of in_die_in */
    for (;;++loop_iteration) {
        int       offres = 0;
        int       res = 0;
        int       childres = 0;
        int       siblingres = 0;
        Dwarf_Die child = 0;
        int       pdacires = 0;

        /* Get the CU offset for easy error reporting */
        offres = dwarf_die_offsets(in_die,
            &glflags.DIE_section_offset,
            &glflags.DIE_offset,err);
        DROP_ERROR_INSTANCE(dbg,offres,*err);
        SET_DIE_STACK_ENTRY(die_stack_indent_level,in_die,
            dieprint_cu_goffset);
        check_sibling_off(loop_iteration, sibling_off_array,
            sibling_off_count);
        res = dd_check_tag_tree(dbg,in_die,err);
        if (res != DW_DLV_OK){
            if (in_die != in_die_in) {
                dwarf_dealloc_die(in_die);
                return res;
            }
        }
        if (glflags.gf_record_dwarf_error &&
            glflags.gf_check_verbose_mode) {
            glflags.gf_record_dwarf_error = FALSE;
        }
        res = dd_print_die_and_die_stack(dbg,in_die,
            dieprint_cu_goffset, srcfiles,srcfilescount,
            err);
        if (res != DW_DLV_OK) {
            if (in_die != in_die_in) {
                dwarf_dealloc_die(in_die);
            }
            return res;
        }
        childres = dwarf_child(in_die, &child, err);
        if (childres == DW_DLV_ERROR) {
            print_error_and_continue(
                "Call to dwarf_child failed printing die tree",
                childres,*err);
            if (in_die != in_die_in) {
                dwarf_dealloc_die(in_die);
            }
            return childres;
        }
        /* Check for specific compiler, gf_check_abbreviations */
        if (glflags.gf_check_abbreviations &&
            checking_this_compiler()) {
            int cdares = 0;

            cdares = dd_check_die_abbrevs(dbg, in_die,childres);
            if (cdares != DW_DLV_OK) {
                dwarf_dealloc_die(child);
                if (in_die != in_die_in) {
                    dwarf_dealloc_die(in_die);
                }
                return cdares;
            }
        }
        /* child first: we are doing depth-first walk */
        if (childres == DW_DLV_OK) {
            int chkoffres = 0;

            chkoffres = dwarf_check_child_offset(dbg,child,
                err);
            if (chkoffres == DW_DLV_ERROR) {
                dwarf_dealloc_die(child);
                if (in_die != in_die_in) {
                    dwarf_dealloc_die(in_die);
                }
                return chkoffres;
            }
            if ((1+die_stack_indent_level) >= DIE_STACK_SIZE ) {
                report_die_stack_error(dbg,err);
                if (in_die != in_die_in) {
                    dwarf_dealloc_die(in_die);
                }
                dwarf_dealloc_die(child);
                return DW_DLV_ERROR;
            }
            /*  Use DIE_STACK to process children
                of the current DIE. Push current DIE to stack.
                Recursing would take too much stack space. */
            die_stack_indent_level++;
            SET_DIE_STACK_ENTRY(die_stack_indent_level,0,
                dieprint_cu_goffset);
            {   /*  Recurse here to traverse child */
                Dwarf_Off     *inner_offset_array = 0;
                Dwarf_Unsigned inner_offset_count = 0;

                dd_setup_die_check_functions(dbg,in_die,is_info,
                    &inner_offset_array, &inner_offset_count,err);
                pdacires = print_die_and_children_internal(dbg, child,
                    dieprint_cu_goffset,
                    is_info, srcfiles, srcfilescount,
                    inner_offset_array, inner_offset_count,
                    err);
                if (inner_offset_array) {
                    dwarf_dealloc(dbg,
                        inner_offset_array,DW_DLA_UARRAY);
                }
            }

            EMPTY_DIE_STACK_ENTRY(die_stack_indent_level);
            dwarf_dealloc_die(child);
            child = 0;
            /*  Unwind DIE_STACK one level to get
                back to our sibling list to process the
                next sibling at our level. */
            die_stack_indent_level--;
            if (pdacires == DW_DLV_ERROR) {
                if (in_die != in_die_in) {
                    dwarf_dealloc_die(in_die);
                }
                return pdacires;
            }
        } /* End processing child */
        if (glflags.gf_display_children_tree &&
            (glflags.gf_info_flag || glflags.gf_types_flag) &&
            glflags.gf_stop_indent_level ==
            die_stack_indent_level) {
            glflags.gf_info_flag = FALSE;
            glflags.gf_types_flag = FALSE;
        }
        sibling = 0;
        dwarf_dealloc_die(child);
        child = 0;
        /*  Find the next sibling or get DW_DLV_NO_ENTRY */
        siblingres = dwarf_siblingof_c(in_die, &sibling, err);
        if (siblingres == DW_DLV_ERROR) {
            print_error_and_continue(
                "ERROR: dwarf_siblingof fails"
                " tracing siblings of a DIE.",
                siblingres, *err);
            if (in_die != in_die_in) {
                dwarf_dealloc_die(in_die);
            }
            return siblingres;
        }
        /*  If we have a sibling, verify that its offset
            is next to the last processed DIE;
            An incorrect sibling chain is a nasty bug.  */
        if (siblingres == DW_DLV_OK &&
            glflags.gf_check_di_gaps &&
            checking_this_compiler()) {
            dd_validate_die_sibling(dbg,sibling);
        }

        /*  Here do any post-descent (ie post-dwarf_child)
            processing of the in_die (we prepare
            to loop again). */
        EMPTY_DIE_STACK_ENTRY(die_stack_indent_level);
        if (in_die != in_die_in) {
            /*  Dealloc our in_die, but not the
                argument die, it belongs
                to our caller. Whether the siblingof
                call worked or not. */
            dwarf_dealloc_die(in_die);
            in_die = 0;
        }
        if (siblingres == DW_DLV_OK) {
            /*  Set to process the sibling, loop again. */
            in_die = sibling;
            sibling = 0;
        } else {
            /* ASSERT: siblingres is DW_DLV_NO_ENTRY  */
            sibling = 0;
            in_die = 0;
            /*  We are done, no more siblings at this level. */
            check_sibling_offset_list_count(loop_iteration,
                sibling_off_count);
            break;
        }
    }  /* end for loop on siblings */
    return DW_DLV_OK;
}

static void
dealloc_local_atlist(Dwarf_Debug dbg,
    Dwarf_Attribute *atlist,
    Dwarf_Signed    atcnt)
{
    Dwarf_Signed i = 0;

    for (i = 0; i < atcnt; i++) {
        dwarf_dealloc_attribute(atlist[i]);
        atlist[i] = 0;
    }
    dwarf_dealloc(dbg, atlist, DW_DLA_LIST);
}

/* Print one die on error and verbose or non check mode */
#define PRINTING_DIES (glflags.gf_do_print_dwarf || \
    (glflags.gf_record_dwarf_error && \
    glflags.gf_check_verbose_mode))

static void
print_srcfiles( char **srcfiles,Dwarf_Signed srcfcnt)
{
    Dwarf_Signed i = 0;
    const char * cntstr = "  [%" DW_PR_DSd "]";

    printf("  dwarf_srcfiles() returned strings. Count = %"
        DW_PR_DSd ".\n",srcfcnt);
    if (srcfcnt < 0) {
        glflags.gf_count_major_errors++;
        printf("ERROR: dwarf_srcfiles count less than zero "
            "which should be impossible. Ignoring srcfiles.");
        return;
    }
    if (srcfcnt > 9) {
        if (srcfcnt > 99) {
            cntstr = "  [%3" DW_PR_DSd "]";
        } else {
            cntstr = "  [%2" DW_PR_DSd "]";
        }
    }
    for ( ; i < srcfcnt; ++i) {
        printf(cntstr,i);
        printf(" %s\n",sanitized(srcfiles[i]));
    }
}

static int
check_duplicated_attributes(Dwarf_Debug dbg,
    Dwarf_Signed     i,
    Dwarf_Half       attr,
    Dwarf_Attribute *atlist,
    Dwarf_Signed     atcnt,
    Dwarf_Error     *err)
{
    if (glflags.gf_check_duplicated_attributes) {
        Dwarf_Half   attr_next = 0;
        Dwarf_Signed j = 0;

        /* DWARFDUMP does the checking */
        DWARF_CHECK_COUNT(duplicated_attributes_result,1);
        for (j = i + 1; j < atcnt; ++j) {
            int ares = 0;

            ares = dwarf_whatattr(atlist[j],
                &attr_next,err);
            if (ares == DW_DLV_OK) {
                if (attr == attr_next) {
                    DWARF_CHECK_ERROR2(
                        duplicated_attributes_result,
                        "Duplicated attribute ",
                        get_AT_name(attr));
                }
            } else {
                struct esb_s m;
                esb_constructor(&m);
                esb_append_printf_i(&m,
                    "ERROR: dwarf_whatattr entry missing "
                    " when checking for duplicated "
                    "attributes"
                    " reading attribute ",j);
                print_error_and_continue(esb_get_string(&m),
                    ares, *err);
                esb_destructor(&m);
                dealloc_local_atlist(dbg,atlist,atcnt);
                return ares;
            }
        }
    }
    return DW_DLV_OK;
}
static void
dd_check_attrlist_sensible(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Attribute *attrlist, Dwarf_Signed atcnt)
{
    Dwarf_Signed i = 0;
    Dwarf_Error error = 0;
    static int check_limiter;

    if (!glflags.gf_check_functions) {
        return;
    }
    if (check_limiter > 1000) {
        return;
    }
    ++check_limiter;

    DWARF_CHECK_COUNT(check_functions_result,1);
    for (i=0; i < atcnt; ++i) {
        int res = 0;
        Dwarf_Off current_off = 0;
        Dwarf_Half form = 0;
        Dwarf_Bool present = FALSE;

        res = dwarf_attr_offset(die,attrlist[i],
            &current_off,&error);
        if (res == DW_DLV_ERROR) {
            DWARF_CHECK_ERROR(check_functions_result,
                "attr offsets unavailable");
            DROP_ERROR_INSTANCE(dbg,res,error);
            return;
        }
        if (res == DW_DLV_NO_ENTRY) {
            DWARF_CHECK_ERROR(check_functions_result,
                "attr offsets no_entry (impossible)");
            DROP_ERROR_INSTANCE(dbg,res,error);
            /* impossible */
            continue;
        }

        res = dwarf_whatform(attrlist[i], &form,&error);
        if (res == DW_DLV_ERROR) {
            DWARF_CHECK_ERROR(check_functions_result,
                "dwarf_whatform fails");
            DROP_ERROR_INSTANCE(dbg,res,error);
            return;
        } if (res == DW_DLV_NO_ENTRY) {
            DWARF_CHECK_ERROR(check_functions_result,
                "dwarf_whatform gets NO_ENTRY");
            DROP_ERROR_INSTANCE(dbg,res,error);
            /* impossible */
            continue;
        }
        res = dwarf_hasform(attrlist[i],form,&present,&error);
        if (res == DW_DLV_ERROR) {
            DWARF_CHECK_ERROR(check_functions_result,
                "dwarf_hasform fails");
            DROP_ERROR_INSTANCE(dbg,res,error);
            return;
        } else if (res == DW_DLV_NO_ENTRY) {
            DWARF_CHECK_ERROR(check_functions_result,
                "dwarf_hasform gets NO_ENTRY");
            DROP_ERROR_INSTANCE(dbg,res,error);
            /* impossible */
            continue;
        } else if (!present) {
            struct esb_s m;
            char buf[40];

            esb_constructor_fixed(&m,buf,sizeof(buf));
            esb_append_printf_u(&m,
                "dwarf_hasform shows not present"
                " on form 0x%" DW_PR_DUx,
                (Dwarf_Unsigned)form);
            esb_append_printf_i(&m,
                "on attribute number %d.",
                i);

            DWARF_CHECK_ERROR(check_functions_result,
                esb_get_string(&m));
            DROP_ERROR_INSTANCE(dbg,res,error);
            esb_destructor(&m);
        }
        if (form == DW_FORM_data16) {
            Dwarf_Form_Data16  data16;

            memset(&data16,0,sizeof(data16));
            res = dwarf_formdata16(attrlist[i],&data16,&error);
            if (res == DW_DLV_ERROR) {
                DWARF_CHECK_ERROR(check_functions_result,
                    "dwarf_formdata16 fails");
                DROP_ERROR_INSTANCE(dbg,res,error);
                return;
            } else if (res == DW_DLV_NO_ENTRY) {
                DWARF_CHECK_ERROR(check_functions_result,
                    "dwarf_formdata16 gets NO_ENTRY");
                DROP_ERROR_INSTANCE(dbg,res,error);
                /* impossible */
            }
            /* Unclear how to check the value */
        }
        /*  Currently unsure how to check more than this. */
    }
}

static void
dd_check_if_legal_offset( Dwarf_Debug dbg,
    Dwarf_Off offset,
    Dwarf_Bool is_info)
{
    int res = 0;
    Dwarf_Error error = 0;
    Dwarf_Die dietwo = 0;

    DWARF_CHECK_COUNT(check_functions_result,1);
    res = dwarf_offdie_b(dbg,offset,is_info,&dietwo,&error);
    if (res == DW_DLV_OK) {
        dwarf_dealloc_die(dietwo);
        return; /* ok */
    }
    if (res == DW_DLV_NO_ENTRY) {
        struct esb_s m;
        char ebuf[120];

        ebuf[0] = 0;
        esb_constructor_fixed(&m,ebuf,sizeof(ebuf));
        esb_append_printf_u(&m,
            "dwarf_offdie_b  ok but "
            "given offset 0x%"
            DW_PR_DUx ,offset);
        esb_append_printf_u(&m,
            " and is_info %u"
            " got DW_DLV_NO_ENTRY.",
            (Dwarf_Small)is_info);
        DWARF_CHECK_ERROR(check_functions_result,
            esb_get_string(&m));
        esb_destructor(&m);
        return;
    }
    {
        struct esb_s m;
        char ebuf[200];

        esb_constructor_fixed(&m,ebuf,sizeof(ebuf));
        esb_append_printf_u(&m,
            "dwarf_offdie_b given offset 0x%"
            DW_PR_DUx, offset);
        esb_append_printf_u(&m,
            "and is_info %u"
            " is not a local reference.",
            (Dwarf_Small)is_info);
        esb_append_printf_s(&m," Underlying error: %s",
            dwarf_errmsg(error));
        DWARF_CHECK_ERROR(check_functions_result,
            esb_get_string(&m));
        esb_destructor(&m);
        DROP_ERROR_INSTANCE(dbg,res,error);
    }
}

static void
check_functions_simple_fail(const char *name,Dwarf_Error error)
{
    struct esb_s m;
    char ebuf[60];

    esb_constructor_fixed(&m,ebuf,sizeof(ebuf));
    esb_append_printf_s(&m,"fail %s ",name);
    esb_append(&m,
        dwarf_errmsg(error));
    DWARF_CHECK_ERROR(check_functions_result,
        esb_get_string(&m));
    esb_destructor(&m);
}

static void
dd_check_die_functions( Dwarf_Debug dbg,
    Dwarf_Die die)
{
    Dwarf_Error    error = 0;
    Dwarf_Off      off = 0;
    Dwarf_Half     attrnum = 0;
    Dwarf_Unsigned unsign = 0;
    int            res = 0;
    Dwarf_Bool     is_info = FALSE;

    if (!glflags.gf_check_functions) {
        return;
    }
    DWARF_CHECK_COUNT(check_functions_result,1);
    res = dwarf_dietype_offset(die,&off,&is_info,&error);
    if (res == DW_DLV_OK) {
        dd_check_if_legal_offset(dbg,off,is_info);
    } else if (res == DW_DLV_ERROR) {
        /* errno > 0 , < 1000 */
        int err = (int)dwarf_errno(error);
        if (err == DW_DLE_MISSING_NEEDED_DEBUG_ADDR_SECTION) {
            /* OK */
        } else {
            struct esb_s m;
            char ebuf[60];

            esb_constructor_fixed(&m,ebuf,sizeof(ebuf));
            esb_append_printf_s(&m,
                "dwarf_dietype_offset error %s"
                " Not a CU die or not a local reference",
                dwarf_errmsg(error));
            DWARF_CHECK_ERROR(check_functions_result,
                esb_get_string(&m));
            esb_destructor(&m);
        }
        DROP_ERROR_INSTANCE(dbg,res,error);
    } /* else DW_DLV_NO_ENTRY, we assume ok */
    off = 0;

    res = dwarf_bytesize(die,&unsign,&error);
    if (res == DW_DLV_OK) {
        /* Ok */
    } else if (res == DW_DLV_ERROR) {
        check_functions_simple_fail("dwarf_bytesize",error);
        DROP_ERROR_INSTANCE(dbg,res,error);
    } /* else DW_DLV_NO_ENTRY, we assume ok */
    unsign = 0;
    res = dwarf_bitsize(die,&unsign,&error);
    if (res == DW_DLV_OK) {
        /* Ok */
    } else if (res == DW_DLV_ERROR) {
        check_functions_simple_fail("dwarf_bitsize",error);
        DROP_ERROR_INSTANCE(dbg,res,error);
    } /* else DW_DLV_NO_ENTRY, we assume ok */
    unsign = 0;

    res = dwarf_bitoffset(die,&attrnum,&unsign,&error);
    if (res == DW_DLV_OK) {
        /* ok */
    } else if (res == DW_DLV_ERROR) {
        check_functions_simple_fail("dwarf_bitoffset",error);
        DROP_ERROR_INSTANCE(dbg,res,error);
    } /* else DW_DLV_NO_ENTRY, we assume ok */
    attrnum = 0;
    unsign = 0;

    res = dwarf_srclang(die,&unsign,&error);
    if (res == DW_DLV_OK) {
        if ((unsign >=1) && (unsign <= DW_LANG_Ada2012)) {
            /* standard */
        } else  {
            switch(unsign) {
            case DW_LANG_Mips_Assembler:
            case DW_LANG_GOOGLE_RenderScript:
            case DW_LANG_Upc:
            case DW_LANG_ALTIUM_Assembler:
            case DW_LANG_BORLAND_Delphi:
            case DW_LANG_SUN_Assembler:
                break;
            default: {

            struct esb_s m;
            char ebuf[60];

            esb_constructor_fixed(&m,ebuf,sizeof(ebuf));
            esb_append_printf_u(&m,
                "dwarf_srclang fail "
                "as the source language 0x%x "
                "returned is unknown",unsign);
            DWARF_CHECK_ERROR(check_functions_result,
                esb_get_string(&m));
            esb_destructor(&m);
            }
            }
        }
    } else if (res == DW_DLV_ERROR) {
        check_functions_simple_fail("dwarf_srclang",error);
        DROP_ERROR_INSTANCE(dbg,res,error);
    } /* else DW_DLV_NO_ENTRY, we assume ok */

    unsign = 0;
    res = dwarf_srclanglname(die,&unsign,&error);
    if (res == DW_DLV_OK) {
        if ((unsign >=1) && (unsign <= DW_LNAME_Nim)) {
            /* standard */
        } else  {
            switch(unsign) {
                break;
            default: {

            struct esb_s m;
            char ebuf[60];

            esb_constructor_fixed(&m,ebuf,sizeof(ebuf));
            esb_append_printf_u(&m,
                "dwarf_srclanglname fail "
                "as the source language 0x%x "
                "returned is unknown",unsign);
            DWARF_CHECK_ERROR(check_functions_result,
                esb_get_string(&m));
            esb_destructor(&m);
            }
            }
        }
    } else if (res == DW_DLV_ERROR) {
        check_functions_simple_fail("dwarf_srclanglname",error);
        DROP_ERROR_INSTANCE(dbg,res,error);
    } /* else DW_DLV_NO_ENTRY, we assume ok */
    unsign = 0;

    res = dwarf_arrayorder(die,&unsign,&error);
    if (res == DW_DLV_OK) {
        if (unsign != DW_ORD_col_major &&
            unsign != DW_ORD_row_major) {
            DWARF_CHECK_ERROR(check_functions_result,
                "array ordering value not valid DW_ORD_");
        }
    } else if (res == DW_DLV_ERROR) {
        check_functions_simple_fail("dwarf_arrayorder",error);
        DROP_ERROR_INSTANCE(dbg,res,error);
    } /* else DW_DLV_NO_ENTRY, we assume ok */
}

/*  If print_else_name_match is FALSE,
    check for attribute  matches with -S
    inr print_attribute, and if found,
    print the information anyway.

    if print_else_name_match is true, do not check
    for attribute name matches. Just print.

    Sets *an_attr_matched TRUE if there is
    attribute name or value that matches
    a -S option (the long form option starts with --search)

    Returns DW_DLV_OK DW_DLV_ERROR or DW_DLV_NO_ENTRY
*/
int
print_one_die(Dwarf_Debug dbg, Dwarf_Die die,
    Dwarf_Off dieprint_cu_goffset,
    Dwarf_Bool print_else_name_match,
    int die_indent_level,
    char **srcfiles, Dwarf_Signed srcfcnt,
    Dwarf_Bool *an_attr_matched_io,
    Dwarf_Bool ignore_die_stack,
    Dwarf_Error *err)
{
    Dwarf_Signed i = 0;
    Dwarf_Off offset = 0;
    Dwarf_Off overall_offset = 0;
    const char * tagname = 0;
    Dwarf_Half tag = 0;
    Dwarf_Signed atcnt = 0;
    Dwarf_Attribute *atlist = 0;
    int tres = 0;
    int ores = 0;
    Dwarf_Bool attribute_matchedpod = FALSE;
    int atres = 0;
    Dwarf_Unsigned abbrev_code = dwarf_die_abbrev_code(die);
    LoHiPc  lohipc;
    int indentprespaces = 0;

    lohipc = lohipc_zero;
    /* Print using indentation see standard_indent() above.
    < 1><0x000854ff GOFF=0x00546047>    DW_TAG_pointer_type -> 34
    < 1><0x000854ff>    DW_TAG_pointer_type                 -> 18
        DW_TAG_pointer_type                                 ->  2
    */
    if (glflags.gf_check_abbreviations &&
        checking_this_compiler()) {
        validate_abbrev_code(abbrev_code);
    }
    if (!ignore_die_stack &&
        die_stack[die_indent_level].already_printed_) {
        /* FALSE seems safe . */
        *an_attr_matched_io = FALSE;
        return DW_DLV_OK;
    }
    indentprespaces = standard_indent();
    tres = dwarf_tag(die, &tag, err);
    if (tres != DW_DLV_OK) {
        print_error_and_continue(
            "ERROR: accessing tag of die!",
            tres, *err);
        return tres;
    }
    tagname = get_TAG_name(tag);
    if ( glflags.gf_print_usage_tag_attr) {
        record_tag_usage(tag);
    }
    tag_specific_globals_setup(dbg,tag,die_indent_level);
    ores = dwarf_dieoffset(die, &overall_offset, err);
    if (ores != DW_DLV_OK) {
        print_error_and_continue(
            "ERROR: failed dwarf_dieoffset call", ores, *err);
        return ores;
    }
    ores = dwarf_die_CU_offset(die, &offset, err);
    if (ores != DW_DLV_OK) {
        print_error_and_continue(
            "ERROR: dwarf_die_CU_offset failed", ores, *err);
        return ores;
    }

    if (glflags.nTrace[KIND_VISITED_INFO] &&
        glflags.gf_check_self_references) {
        printf("<%2d><0x%" DW_PR_XZEROS DW_PR_DUx
            " GOFF=0x%" DW_PR_XZEROS DW_PR_DUx "> ",
            die_indent_level, (Dwarf_Unsigned)offset,
            (Dwarf_Unsigned)overall_offset);
        print_indent_prefix(0,die_indent_level,2);
        printf("%s\n",tagname);
    }
    /* Print the die */
    if (PRINTING_DIES && print_else_name_match) {
        if (!ignore_die_stack) {
            die_stack[die_indent_level].already_printed_ = TRUE;
        }
        if (die_indent_level == 0) {
            print_cu_hdr_cudie(overall_offset, offset);
        } else if (local_symbols_already_begun == FALSE &&
            die_indent_level == 1 && !glflags.dense) {

            printf("\nLOCAL_SYMBOLS:\n");
            local_symbols_already_begun = TRUE;
        }

        /* Print just the Tags and Attributes */
        if (!glflags.gf_display_offsets) {
            /* Print using indentation */
            print_indent_prefix(0,die_indent_level,2);
            printf("%s\n",tagname);
        } else {
            if (glflags.dense) {
                if (glflags.gf_show_global_offsets) {
                    if (die_indent_level == 0) {
                        printf("<%d><0x%" DW_PR_DUx
                            "+0x%" DW_PR_DUx
                            " GOFF=0x%"
                            DW_PR_DUx ">", die_indent_level,
                            (Dwarf_Unsigned)
                            (overall_offset - offset),
                            (Dwarf_Unsigned)offset,
                            (Dwarf_Unsigned)overall_offset);
                    } else {
                        printf("<%d><0x%" DW_PR_DUx
                            " GOFF=0x%" DW_PR_DUx ">",
                            die_indent_level,
                            (Dwarf_Unsigned)offset,
                            (Dwarf_Unsigned)overall_offset);
                    }
                } else {
                    if (die_indent_level == 0) {
                        printf("<%d><0x%" DW_PR_DUx "+0x%"
                            DW_PR_DUx ">",
                            die_indent_level,
                            (Dwarf_Unsigned)
                            (overall_offset - offset),
                            (Dwarf_Unsigned)offset);
                    } else {
                        printf("<%d><0x%" DW_PR_DUx ">",
                            die_indent_level,
                            (Dwarf_Unsigned)offset);
                    }
                }
                printf("<%s>",tagname);
                if (glflags.verbose) {
                    Dwarf_Off agoff = 0;
                    Dwarf_Unsigned acount = 0;
                    printf(" <abbrev %" DW_PR_DUu ,abbrev_code);
                    if (glflags.gf_show_global_offsets) {
                        int agres = 0;

                        agres =
                            dwarf_die_abbrev_global_offset(
                            die, &agoff, &acount,err);
                        if (agres == DW_DLV_ERROR) {
                            print_error_and_continue(
                                "dwarf_die_abbrev_global_offset"
                                " call failed",
                                agres, *err);
                            return agres;
                        } else if (agres == DW_DLV_NO_ENTRY) {
                            print_error_and_continue(
                                "dwarf_die_abbrev_global_offset"
                                " no entry?",
                                agres, *err);
                            return agres;
                        } else {
                            printf(" ABGOFF = 0x%"
                                DW_PR_XZEROS DW_PR_DUx
                                " count = 0x%"
                                DW_PR_XZEROS DW_PR_DUx,
                                agoff, acount);
                        }
                    }
                    printf(">");
                }
            } else {
                if (glflags.gf_show_global_offsets) {
                    printf("<%2d><0x%" DW_PR_XZEROS DW_PR_DUx
                        " GOFF=0x%" DW_PR_XZEROS DW_PR_DUx ">",
                        die_indent_level, (Dwarf_Unsigned)offset,
                        (Dwarf_Unsigned)overall_offset);
                } else {
                    printf("<%2d><0x%" DW_PR_XZEROS DW_PR_DUx ">",
                        die_indent_level,
                        (Dwarf_Unsigned)offset);
                }

                /* Print using indentation */
                print_indent_prefix(0,die_indent_level, 2);
                printf("%s",tagname);
                if (glflags.verbose) {
                    Dwarf_Off agoff = 0;
                    Dwarf_Unsigned acount = 0;
                    printf(" <abbrev %" DW_PR_DUu,abbrev_code);
                    if (glflags.gf_show_global_offsets) {
                        int agres = 0;

                        agres = dwarf_die_abbrev_global_offset(
                            die, &agoff, &acount,err);
                        if (agres == DW_DLV_ERROR) {
                            print_error_and_continue(
                                "Call to "
                                "dwarf_die_abbrev_global_offset"
                                "failed ",
                                agres, *err);
                            return agres;
                        } else if (agres == DW_DLV_NO_ENTRY) {
                            print_error_and_continue(
                                "Call to "
                                "dwarf_die_abbrev_global_offset "
                                " returned NO_ENTRY!",
                                agres,*err);
                            return agres;
                        } else {
                            printf(" ABGOFF = 0x%"
                                DW_PR_XZEROS DW_PR_DUx
                                " count = 0x%"
                                DW_PR_XZEROS DW_PR_DUx,
                                agoff, acount);
                        }
                    }
                    printf(">");
                }
                fputs("\n",stdout);
            }
        }
    }
    if ((glflags.verbose > 2) && (die_indent_level == 0) &&
        srcfcnt  && PRINTING_DIES) {
        print_srcfiles(srcfiles,srcfcnt);
    }
    atres = dwarf_attrlist(die, &atlist, &atcnt, err);
    if (atres == DW_DLV_ERROR) {
        print_error_and_continue(
            "ERROR: A call to dwarf_attrlist failed. "
            " Impossible error.", atres,*err);
        return atres;
    }
    if (atres == DW_DLV_NO_ENTRY) {
        /* indicates there are no attrs.  It is not an error. */
        atcnt = 0;
    }

    /*  Get the offset for easy error reporting:
        This is not the CU die.  */
    atres = dwarf_die_offsets(die,&glflags.DIE_section_offset,
        &glflags.DIE_offset,err);
    if (atres == DW_DLV_ERROR) {
        print_error_and_continue(
            "ERROR: A call to dwarf_die_offsets failed in "
            "printing an attribute. "
            , atres,*err);
        dealloc_local_atlist(dbg,atlist,atcnt);
        return atres;
    }

    dd_check_die_functions(dbg,die);
    dd_check_attrlist_sensible(dbg,die,atlist,atcnt);
    for (i = 0; i < atcnt; i++) {
        int        ares = 0;
        Dwarf_Half attrnum = 0;

        ares = dwarf_whatattr(atlist[i], &attrnum, err);
        if (ares == DW_DLV_OK) {
            /*  Check duplicated attributes; use brute force
                as the number of attributes is quite small;
                the problem was detected with the
                LLVM toolchain, generating more than 12
                repeated attributes */
            ares = check_duplicated_attributes(dbg,i,
                attrnum,atlist,atcnt, err);
            if (ares != DW_DLV_OK) {
                return ares;
            }

            /* Print using indentation */
            if (!glflags.dense && PRINTING_DIES &&
                print_else_name_match) {
                print_indent_prefix(indentprespaces,
                    die_indent_level,2);
            }
            {
                Dwarf_Bool attr_match_localb = FALSE;
                int aresb = 0;

                aresb = print_attribute(dbg, die,
                    dieprint_cu_goffset,
                    attrnum,
                    atlist[i],
                    print_else_name_match, die_indent_level,
                    srcfiles, srcfcnt,
                    &lohipc,
                    &attr_match_localb,err);
                if (aresb == DW_DLV_ERROR) {
                    struct esb_s m;

                    esb_constructor(&m);
                    esb_append_printf_i(&m,
                        "ERROR: Failed printing attribute %d",
                        i);
                    esb_append_printf_i(&m,
                        " of %d attributes.",atcnt);
                    print_error_and_continue(
                        esb_get_string(&m),
                        aresb,*err);
                    esb_destructor(&m);
                    DROP_ERROR_INSTANCE(dbg,aresb,*err);
                }
                if (print_else_name_match == FALSE &&
                    attr_match_localb) {
                    attribute_matchedpod = TRUE;
                }
            }

            if (glflags.gf_record_dwarf_error &&
                glflags.gf_check_verbose_mode) {
                glflags.gf_record_dwarf_error = FALSE;
            }
        } else {
            struct esb_s m;

            esb_constructor(&m);
            esb_append_printf_i(&m,
                "ERROR: Failed getting attribute %d",
                i);
            esb_append_printf_i(&m,
                " of %d attributes.",
                atcnt);
            print_error_and_continue(esb_get_string(&m),ares,*err);
            esb_destructor(&m);
            dealloc_local_atlist(dbg,atlist,atcnt);
            return ares;
        }
    }
    /* atres might have been DW_DLV_NO_ENTRY, atlist NULL */
    if (atlist) {
        dealloc_local_atlist(dbg,atlist,atcnt);
    }
    if (PRINTING_DIES && glflags.dense && print_else_name_match) {
        printf("\n");
    }
    *an_attr_matched_io = attribute_matchedpod;
    return DW_DLV_OK;
}

/*  Encodings have undefined signedness. Accept either
    signedness.  The values are integer-like (they are defined
    in the DWARF specification), so the
    form the compiler uses (as long as it is
    a constant value) is a non-issue.

    The numbers need not be small (in spite of the
    function name), but the result should be an integer.

    If string_out is non-NULL, construct a string output, either
    an error message or the name of the encoding.
    The function pointer passed in is to code generated
    by a script at dwarfdump build time. The code for
    the val_as_string function is generated
    from dwarf.h.  See <build dir>/dwarf_names.c

    The known_signed bool is set TRUE(nonzero) or FALSE (zero)
    and *both* uval_out and sval_out are set to the value,
    though of course uval_out cannot represent a signed
    value properly and sval_out cannot represent all unsigned
    values properly.

    If string_out is non-NULL then attr_name and val_as_string
    must also be non-NULL.  */
int
dd_get_integer_and_name(Dwarf_Debug dbg,
    Dwarf_Attribute attrib,
    Dwarf_Unsigned * uval_out,
    const char *attr_name,
    struct esb_s* string_out,
    encoding_type_func val_as_string,
    Dwarf_Error * err,
    int show_form)
{
    Dwarf_Unsigned uval = 0;
    Dwarf_Bool is_info = TRUE;

    int vres = dwarf_formudata(attrib, &uval, err);
    /* if it is not formudata, lets check further */
    DROP_ERROR_INSTANCE(dbg,vres,*err);
    if (vres != DW_DLV_OK) {
        Dwarf_Signed sval = 0;
        int ires = 0;

        ires = dwarf_formsdata(attrib, &sval, err);
        /* It is not formudata, lets check further */
        DROP_ERROR_INSTANCE(dbg,ires,*err);
        if (ires != DW_DLV_OK) {
            int jres = 0;

            jres = dwarf_global_formref_b(attrib,&uval,
                &is_info,err);
            if (jres != DW_DLV_OK) {
                if (string_out) {
                    glflags.gf_count_major_errors++;
                    esb_append_printf_s(string_out,
                        "ERROR: %s has a bad form"
                        " for reading a value.",
                        attr_name);
                }
                return jres;
            }
            *uval_out = uval;
        } else {
            uval =  (Dwarf_Unsigned) sval;
            *uval_out = uval;
        }
    } else {
        *uval_out = uval;
    }
    if (string_out) {
        Dwarf_Half theform = 0;
        Dwarf_Half directform = 0;
        char fsbuf[ESB_FIXED_ALLOC_SIZE];
        struct esb_s fstring;
        int fres = 0;

        esb_constructor_fixed(&fstring,fsbuf,sizeof(fsbuf));
        fres  = get_form_values(attrib,&theform,
            &directform,err);
        if (fres == DW_DLV_ERROR) {
            return fres;
        }
        esb_append(&fstring, val_as_string((Dwarf_Half) uval));
        show_form_itself(show_form,glflags.verbose,theform,
            directform,&fstring);
        esb_append(string_out,esb_get_string(&fstring));
        esb_destructor(&fstring);
    }
    return DW_DLV_OK;
}

/*  Called for DW_AT_SUN_func_offsets
    We need a 32-bit signed number here.
    But we're getting rid of the __[u]int[n]_t
    dependence so lets use plain characters.

    This is rarely, if ever, used so lets
    report errors but not stop the processing.
    */

static void
get_FLAG_BLOCK_string(Dwarf_Debug dbg, Dwarf_Attribute attrib,
struct esb_s*esbp)
    {
    int fres = 0;
    Dwarf_Block *tempb = 0;
    Dwarf_Unsigned array_len = 0;
    Dwarf_Signed *array = 0;
    Dwarf_Unsigned next = 0;
    Dwarf_Error  fblkerr = 0;

    /* first get compressed block data */
    fres = dwarf_formblock (attrib,&tempb, &fblkerr);
    if (fres != DW_DLV_OK) {
        print_error_and_continue(
            "DW_FORM_blockn cannot get block\n",
            fres,fblkerr);
        DROP_ERROR_INSTANCE(dbg,fres,fblkerr);
        return;
    }

    fres = dwarf_uncompress_integer_block_a(dbg,
        tempb->bl_len,
        (void *)tempb->bl_data,
        &array_len,&array,&fblkerr);
    /*  uncompress block into 32bit signed int array.
        It's really a block of sleb numbers so the
        compression is minor unless the values
        are close to zero.  */
    if (fres != DW_DLV_OK) {
        dwarf_dealloc(dbg,tempb,DW_DLA_BLOCK);
        print_error_and_continue(
            "DW_AT_SUN_func_offsets cannot uncompress data\n",
            0,fblkerr);
        DROP_ERROR_INSTANCE(dbg,fres,fblkerr);
        dwarf_dealloc_uncompressed_block(dbg, array);
        return;
    }
    if (array_len == 0) {
        dwarf_dealloc_uncompressed_block(dbg, array);
        print_error_and_continue(
            "DW_AT_SUN_func_offsets has no data (array"
            " length is zero), something badly"
            " wrong",
            DW_DLV_OK,fblkerr);
        dwarf_dealloc(dbg,tempb,DW_DLA_BLOCK);
        return;
    }

    /* fill in string buffer */
    next = 0;
    while (next < array_len) {
        unsigned i = 0;
        /*  Print a full line */
        esb_append(esbp,"\n  ");
        for (i = 0 ; i < 2 && next < array_len; ++i,++next) {
            Dwarf_Signed vs = array[next];
            Dwarf_Unsigned vu = (Dwarf_Unsigned)vs;

            if (i== 1) {
                esb_append(esbp," ");
            }
            esb_append_printf_i(esbp,"%6" DW_PR_DSd " ",vs);
            esb_append_printf_u(esbp,
                "(0x%"  DW_PR_XZEROS DW_PR_DUx ")",vu);
        }
    }
    dwarf_dealloc(dbg,tempb,DW_DLA_BLOCK);
    /* free array buffer */
    dwarf_dealloc_uncompressed_block(dbg, array);
}

static const char *
get_rangelist_type_descr(Dwarf_Ranges *r)
{
    switch (r->dwr_type) {
    case DW_RANGES_ENTRY:             return "range entry";
    case DW_RANGES_ADDRESS_SELECTION: return "base address";
    case DW_RANGES_END:               return "range end";
    default: break;
    }
    /* Impossible. */
    return "ERROR: .debug_ranges range type Unknown";
}

/*  The string produced here will need to be passed
    through sanitized() before actually printing.
    Always returns DW_DLV_OK.  */
int
print_ranges_list_to_extra(Dwarf_Debug dbg,
    Dwarf_Die die /* may be NULL if unknown */,
    Dwarf_Unsigned originaloff,
    Dwarf_Unsigned finaloff,
    Dwarf_Ranges *rangeset,
    Dwarf_Signed rangecount,
    Dwarf_Unsigned bytecount,
    struct esb_s *stringbuf)
{
    const char    *sec_name = 0;
    int            res = 0;
    Dwarf_Signed   i = 0;
    struct esb_s   truename;
    char           buf[ESB_FIXED_ALLOC_SIZE];
    Dwarf_Unsigned base_address = 0;
    Dwarf_Bool     have_base_addr = FALSE;
    Dwarf_Error    grberr = 0;
    Dwarf_Bool     have_ranges_offset = FALSE;
    Dwarf_Unsigned ranges_offset = 0;

    esb_constructor_fixed(&truename,buf,sizeof(buf));
    /*  We don't want to set the compress data into
        the secname here. */
    get_true_section_name(dbg,".debug_ranges",
        &truename,FALSE);
    sec_name = esb_get_string(&truename);
    if (die) {
        res = dwarf_get_ranges_baseaddress(dbg,die,
            &have_base_addr, &base_address,
            &have_ranges_offset, &ranges_offset,
            &grberr);
        if (res == DW_DLV_ERROR) {
            dwarf_dealloc_error(dbg,grberr);
            grberr = 0;
        }
    }
    if (glflags.dense) {
        /* for dense, ignore cooked values.  */
        esb_append_printf_i(stringbuf,"< ranges: %"
            DW_PR_DSd,rangecount);
        esb_append_printf_s(stringbuf," ranges at %s" ,
            sanitized(sec_name));
        if (have_ranges_offset) {
            esb_append_printf_u(stringbuf," global offset %"
                DW_PR_DUu , finaloff);
            esb_append_printf_u(stringbuf," without base %"
                DW_PR_DUu , originaloff);
        } else {
            esb_append_printf_u(stringbuf,
                " offset base %" DW_PR_DUu ,
                finaloff);
        }
        esb_append_printf_u(stringbuf," (0x%"
            DW_PR_XZEROS DW_PR_DUx ") ",
            finaloff);
        if (have_base_addr) {
            esb_append_printf_u(stringbuf," (base addr 0x%"
                DW_PR_XZEROS DW_PR_DUx ") ",
                base_address);
        }
        esb_append_printf_u(stringbuf,"(%" DW_PR_DUu
            " bytes)>",
            bytecount);
    } else {
        esb_append_printf_i(stringbuf,"  ranges: %" DW_PR_DSd,
            rangecount);
        esb_append_printf_s(stringbuf," at %s" ,
            sanitized(sec_name));
        esb_append_printf_u(stringbuf," global offset %"
            DW_PR_DUu , finaloff);
        esb_append_printf_u(stringbuf," (0x%"
            DW_PR_XZEROS DW_PR_DUx ") ",
            finaloff);
        if (have_ranges_offset) {
            esb_append_printf_u(stringbuf," without base %"
                DW_PR_DUu , originaloff);
            esb_append_printf_u(stringbuf," (0x%"
                DW_PR_XZEROS DW_PR_DUx ") ",
                originaloff);
        }
        if (have_base_addr) {
            esb_append_printf_u(stringbuf," ( base addr 0x%"
                DW_PR_XZEROS DW_PR_DUx ") ",
                base_address);
        }
        esb_append_printf_u(stringbuf,"(%" DW_PR_DUu
            " bytes)\n",
            bytecount);
    }
    for (i = 0; i < rangecount; ++i) {
        Dwarf_Ranges * r = rangeset +i;
        const char *type = get_rangelist_type_descr(r);
        if (glflags.dense) {
            /* For dense, ignore cooked values */
            esb_append_printf_i(stringbuf,"<[%2" DW_PR_DSd,i);
            esb_append_printf_s(stringbuf,"] %s",type);
            esb_append_printf_u(stringbuf," 0x%"
                DW_PR_XZEROS  DW_PR_DUx,r->dwr_addr1);
            esb_append_printf_u(stringbuf," 0x%"
                DW_PR_XZEROS  DW_PR_DUx ">",r->dwr_addr2);
        } else {
            /*  See DWARF5 page 53, line 17
                the addresses match so nothing is there and
                the address itself is irrelevant. */
            esb_append_printf_i(stringbuf,"   [%2" DW_PR_DSd,i);
            esb_append_printf_s(stringbuf,"] %-12s",type);

            switch(r->dwr_type) {
            case DW_RANGES_ENTRY: {
                Dwarf_Bool emptyrange =
                    (r->dwr_type == DW_RANGES_ENTRY
                    && r->dwr_addr1 == r->dwr_addr2);

                esb_append_printf_u(stringbuf,
                    " raw: 0x%08x",r->dwr_addr1);
                esb_append_printf_u(stringbuf,
                    ",  0x%08x",r->dwr_addr2);
                if (emptyrange) {
                    esb_append(stringbuf," (empty range)");
                }
                if (have_base_addr) {
                    /* See DWARF4 2.17.3 page 38 and 39 */
                    esb_append_printf_u(stringbuf,
                        " cooked: 0x%08x",
                        r->dwr_addr1 +base_address);
                    esb_append_printf_u(stringbuf,
                        ",  0x%08x",
                        r->dwr_addr2 +base_address);
                }
                break;
            }
            case DW_RANGES_ADDRESS_SELECTION:
                esb_append_printf_u(stringbuf,
                    " 0x%08x",
                    r->dwr_addr2);
                have_base_addr = TRUE;
                base_address = r->dwr_addr2;
                break;
            case DW_RANGES_END:
                esb_append(stringbuf,
                    " 0,0");
                have_base_addr = FALSE;
                base_address = 0;
                break;
            default:
                esb_append_printf_u(stringbuf,
                    "ERROR: Impossible entry from .debug_ranges, "
                    "incorrect dwr_type is 0x%x\n",
                    r->dwr_type);
                break;
            }
            esb_append(stringbuf,"\n");
        }
    }
    esb_destructor(&truename);
    return DW_DLV_OK;
}

void
dd_do_dump_visited_info(int level, Dwarf_Off loff,Dwarf_Off goff,
    Dwarf_Off cu_die_goff,
    const char *atname, const char *valname)
{
    printf("<%2d><0x%" DW_PR_XZEROS DW_PR_DUx
        " GOFF=0x%" DW_PR_XZEROS DW_PR_DUx
        " CU-GOFF=0x%" DW_PR_XZEROS DW_PR_DUx
        "> ",
        level, loff, goff,cu_die_goff);
    print_indent_prefix(0,level,2);
    printf("%s -> %s\n",atname,valname);
}

/*  Always returns DW_DLV_OK.
    Expected attr is DW_AT_decl_file or DW_AT_call_file */
static int
turn_file_num_to_string(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Attribute attrib,
    Dwarf_Half theform,
    Dwarf_Half dwversion,
    Dwarf_Unsigned filenum,/* decl_file number */
    char **srcfiles,
    Dwarf_Signed srcfiles_cnt,
    struct esb_s * esbp,
    Dwarf_Error *err)
{
    Dwarf_Half offset_size=0;
    int vres = 0;
    char declmsgbuf[ESB_FIXED_ALLOC_SIZE];
    struct esb_s declmsg;
    char *fname = 0;
    Dwarf_Half attrnum = 0;

    vres = dwarf_whatattr(attrib,&attrnum,err);
    if (vres != DW_DLV_OK) {
        struct esb_s m;

        esb_constructor(&m);
        esb_append(&m,
            "ERROR: Cannot get DIE context "
            "attribute number for attr ");
        esb_append_printf_s(&m,
            "form %s ",
            get_FORM_name(theform));
        print_error_and_continue(
            esb_get_string(&m),vres,*err);
        esb_destructor(&m);
        if (vres == DW_DLV_ERROR) {
            DROP_ERROR_INSTANCE(dbg,vres,*err);
        }
        return DW_DLV_OK;
    }
    vres = dwarf_get_version_of_die(die,
        &dwversion,&offset_size);
    if (vres != DW_DLV_OK) {
        struct esb_s m;

        esb_constructor(&m);
        esb_append_printf_s(&m,
            "ERROR: Cannot get DIE context "
            "version number for attr %s ",
            get_AT_name(attrnum));
        esb_append_printf_s(&m,
            "form %s ",
            get_FORM_name(theform));
        print_error_and_continue(
            esb_get_string(&m),vres,*err);
        esb_destructor(&m);
        if (vres == DW_DLV_ERROR) {
            DROP_ERROR_INSTANCE(dbg,vres,*err);
        }
        return DW_DLV_OK;
    }
    esb_constructor_fixed(&declmsg,declmsgbuf,
        sizeof(declmsgbuf));
    if (!srcfiles) {
        if (glflags.verbose > 2) {
            esb_append_printf_s(&declmsg,
                " <%s file index ",
                get_AT_name(attrnum));
            esb_append_printf_u(&declmsg,
                "%" DW_PR_DUu,filenum);
            esb_append(&declmsg," No file list for CU>");
            esb_append(esbp, " ");
            esb_append(esbp,
                esb_get_string(&declmsg));
        }
        esb_destructor(&declmsg);
        return DW_DLV_OK;
    } else {
        Dwarf_Unsigned localud = filenum;
        Dwarf_Bool done = FALSE;

        if (dwversion == DWVERSION5) {
            if ( localud < (Dwarf_Unsigned)srcfiles_cnt) {
                fname = srcfiles[localud];
                esb_append(&declmsg,fname);
                done = TRUE;
            }
        } else {
            if (!localud) {
                /* Just print the number,
                there is no name. */
                done=TRUE;
            } else if (localud &&
                localud <= (Dwarf_Unsigned)srcfiles_cnt) {
                fname = srcfiles[localud - 1];
                esb_append(&declmsg,fname);
                done = TRUE;
            }
        }
        if (!done) {
            esb_append_printf_s(&declmsg,
                " <%s file index ",
                get_AT_name(attrnum));
            esb_append_printf_u(&declmsg,
                "%" DW_PR_DUu,filenum);
            esb_append(&declmsg," out of range>");
        }
    }
    if (fname) {
        esb_append(esbp, " ");
        esb_append(esbp,
            esb_get_string(&declmsg));
    }
    esb_destructor(&declmsg);
    return DW_DLV_OK;
}

static void
append_useful_die_name(Dwarf_Debug dbg,
    Dwarf_Die die,
    char **srcfiles,
    Dwarf_Signed srcfiles_cnt,
    struct esb_s *outstr,
    Dwarf_Error *err)
{
    int             res = 0;
    Dwarf_Attribute nattr = 0;
    Dwarf_Half      nattr_form = 0;
    Dwarf_Attribute lattr = 0;
    Dwarf_Unsigned  filenum = 0;
    Dwarf_Unsigned  linenum = 0;
    Dwarf_Half      version = 0;
    Dwarf_Half      offset_size = 0;

    res = dwarf_get_version_of_die(die,&version,&offset_size);
    if (res != DW_DLV_OK) {
        /* FAIL. */
        return;
    }
    res = dwarf_attr(die,DW_AT_decl_file, &nattr,err);
    if (res != DW_DLV_OK) {
        if (res == DW_DLV_ERROR) {
            DROP_ERROR_INSTANCE(dbg,res,*err);
        }
        return;
    }
    res  =dwarf_whatform(nattr,&nattr_form,err);
    if (res == DW_DLV_ERROR) {
        DROP_ERROR_INSTANCE(dbg,res,*err);
    }
    if (res != DW_DLV_OK) {
        if (res == DW_DLV_ERROR) {
            DROP_ERROR_INSTANCE(dbg,res,*err);
        }
        dwarf_dealloc_attribute(nattr);
        return;
    }
    res = dwarf_attr(die,DW_AT_decl_line,&lattr,err);
    if (res != DW_DLV_OK) {
        if (res == DW_DLV_ERROR) {
            DROP_ERROR_INSTANCE(dbg,res,*err);
        }
        linenum = 0;
    } else {
        res = dwarf_formudata(lattr,&linenum,err);
        if (res != DW_DLV_OK) {
            if (res == DW_DLV_ERROR) {
                DROP_ERROR_INSTANCE(dbg,res,*err);
            }
            linenum = 0;
        }
    }
    dwarf_dealloc_attribute(lattr);
    lattr = 0;
    turn_file_num_to_string(dbg,die,
        nattr,
        nattr_form,
        version,filenum, srcfiles,srcfiles_cnt,
        outstr,
        err);
    if (linenum) {
        esb_append_printf_u(outstr,
            " line: %u",linenum);
    }
    dwarf_dealloc_attribute(nattr);
    return;
}

/*  So far, designed for DW_AT_type with a Dwarf_Sig8
    reference.  Does not yet look at tied object
    even if one is present.
    Does not currently attempt to use any signature
    resolution fast-access DWARF sections.
*/
int
dd_print_sig8_target(Dwarf_Debug dbg,
    Dwarf_Attribute attrib,
    int die_indent_level,
    char **srcfiles,
    Dwarf_Signed srcfiles_cnt,
    struct esb_s * valname,
    Dwarf_Error *err)
{
    Dwarf_Sig8 signature;
    Dwarf_Die  targdie = 0;
    Dwarf_Bool targ_is_info = FALSE;
    Dwarf_Half targtag = 0;
    Dwarf_Unsigned targ_goff = 0;
    const char  * targtagname = 0;
    char  * targdiename = 0;
    int res = 0;

    res = dwarf_formsig8(attrib,&signature,err);
    if (res == DW_DLV_ERROR) {
        print_error_and_continue(
            "dwarf_formsig8 fails in "
            "attribute traversal",
            res, *err);
        return res;
    }
    if (res == DW_DLV_NO_ENTRY) {
        return res;
    }
    res = dwarf_find_die_given_sig8(dbg,
        &signature, &targdie,
        &targ_is_info,err);
    if (res == DW_DLV_ERROR) {
        print_error_and_continue(
            "dwarf_find_die_given_sig8 fails "
            "following a signature  in "
            "attribute traversal",
            res, *err);
        return res;
    }
    if (res == DW_DLV_NO_ENTRY) {
        /*  We did not find the target */
        return res;
    }
    res = dwarf_dieoffset(targdie, &targ_goff, err);
    if (res == DW_DLV_ERROR) {
        print_error_and_continue(
            "dwarf_dieoffset fails following a signature  in "
            "attribute traversal",
            res, *err);
        dwarf_dealloc_die(targdie);
        return res;
    }
    if (res == DW_DLV_OK) {
        if (!glflags.dense) {
            esb_append(valname,"\n");
            append_indent_prefix(valname,standard_indent(),
                die_indent_level,0);
        }
        esb_append_printf_u(valname,
            " <Target GOFF=0x%08x",targ_goff);
    }
    esb_append_printf_s(valname,
        " in section %s",
        targ_is_info?".debug_info":".debug_types");
    res = dwarf_tag(targdie,&targtag,err);
    if (res == DW_DLV_ERROR) {
        print_error_and_continue(
            "dwarf_tag fails following a signature in "
            "attribute traversal",
            res, *err);
        dwarf_dealloc_die(targdie);
        return res;
    }
    if (res == DW_DLV_OK) {
        esb_append_printf_u(valname,
            " TAG: 0x%02x ",targtag);
    }
    targtagname = get_TAG_name(targtag);
    esb_append_printf_s(valname,
        "(%s) ",targtagname);
    res = dwarf_diename(targdie,&targdiename,err);
    if (res == DW_DLV_ERROR) {
        print_error_and_continue(
            "dwarf_tag   fails following a signature in "
            "attribute traversal",
            res, *err);
        dwarf_dealloc_die(targdie);
        return res;
    }
    if (res == DW_DLV_NO_ENTRY) {
        append_useful_die_name(dbg,targdie,
            srcfiles,srcfiles_cnt,valname,err);
    }
    if (res == DW_DLV_OK) {
        esb_append_printf_s(valname,
            "name: %s",targdiename);
    }
    if (!glflags.dense) {
        esb_append(valname,">\n");
    }
    /*  If we get here we extablished the target. */
    dwarf_dealloc_die(targdie);
    return DW_DLV_OK;
}

static Dwarf_Bool
is_form_block(int form)
{
    if (form == DW_FORM_block1 ||
        form == DW_FORM_block2 ||
        form == DW_FORM_block4 ||
        form == DW_FORM_exprloc ||
        form == DW_FORM_block)  {
        return TRUE;
    }
    return FALSE;
}
/*  DW_FORM_data16 should not apply here. */
static Dwarf_Bool
is_simple_location_expr(int form)
{
    if (is_form_block(form)) {
        return TRUE;
    }
    if (form == DW_FORM_exprloc) {
        return TRUE;
    }
    return FALSE;
}
static Dwarf_Bool
is_location_form(int form)
{
    if (form == DW_FORM_data4 ||
        form == DW_FORM_data8 ||
        form == DW_FORM_sec_offset ||
        form == DW_FORM_loclistx ||
        form == DW_FORM_rnglistx ) {
        return TRUE;
    }
    return FALSE;
}

static void
show_attr_form_error(unsigned attr,
    unsigned form,
    struct esb_s *out)
{
    const char *n = 0;
    int res = 0;
    Dwarf_Error formerr = 0;

    esb_append(out,"ERROR: Attribute ");
    esb_append_printf_u(out,"%u",attr);

    esb_append(out," (");
    res = dwarf_get_AT_name(attr,&n);
    if (res != DW_DLV_OK) {
        n = "UknownAttribute";
    }
    esb_append(out,n);
    esb_append(out,") ");
    esb_append(out," has form ");
    esb_append_printf_u(out,"%u",form);
    esb_append(out," (");
    esb_append(out,get_FORM_name(form));
    esb_append(out,"), a form which is not appropriate");
    print_error_and_continue(
        esb_get_string(out), DW_DLV_OK,formerr);
}

/*  Traverse an attribute and following any reference
    in order to detect self references to DIES (loop).
    We do not use print_else_name_match here. Just looking
    for self references to report on.

    This does not work properly, given self-reference
    is normal in the complete graph that is DWARF DIEs.

*/
static int
traverse_attribute(Dwarf_Debug dbg, Dwarf_Die die,
    Dwarf_Off dieprint_cu_goffset,
    Dwarf_Bool is_info,
    Dwarf_Half attr,
    Dwarf_Attribute attr_in,
    char **srcfiles, Dwarf_Signed srcfcnt,
    int die_indent_level,
    Dwarf_Error *err)
{
    Dwarf_Attribute attrib = 0;
    const char * atname = 0;
    int tres = 0;
    Dwarf_Half tag = 0;
    struct esb_s valname;

    esb_constructor(&valname);
    is_info = dwarf_get_die_infotypes_flag(die);
    atname = get_AT_name(attr);

    /*  The following gets the real attribute,
        even in the face of an
        incorrect doubling, or worse, of attributes. */
    attrib = attr_in;
    /*  Do not get attr via dwarf_attr: if there are
        (erroneously)
        multiple of an attr in a DIE, dwarf_attr will
        not get the
        second, erroneous one and dwarfdump will
        print the first one
        multiple times. Oops. */

    tres = dwarf_tag(die, &tag, err);
    if (tres == DW_DLV_ERROR) {
        print_error_and_continue(
            "Cannot get DIE tag in traverse_attribute!",
            tres,*err);
        esb_destructor(&valname);
        return tres;
    } else if (tres == DW_DLV_NO_ENTRY) {
        tag = 0;
    } else {
        /* ok */
    }

    switch (attr) {
    case DW_AT_specification:
    case DW_AT_containing_type:
    case DW_AT_abstract_origin:
    case DW_AT_object_pointer:
    case DW_AT_small:
    case DW_AT_extension:
    case DW_AT_priority:
    case DW_AT_namelist_item:
    case DW_AT_friend: {
        /*  No need for DW_AT_types,
            DW_AT_sibling. The self-referential tests
            do work properly if included in the switch().  */
        int res = 0;
        Dwarf_Off die_goff = 0;
        Dwarf_Off ref_goff = 0;
        Dwarf_Die ref_die = 0;
        struct esb_s specificationstr;
        Dwarf_Half theform = 0;
        Dwarf_Half directform = 0;
        char buf[ESB_FIXED_ALLOC_SIZE];

        res = get_form_values(attrib,&theform,
            &directform,err);
        if (res != DW_DLV_OK) {
            esb_destructor(&valname);
            return res;
        }
        if (!dd_form_refers_local_info(theform)) {
            break;
        }
        esb_constructor_fixed(&specificationstr,buf,sizeof(buf));
        ++die_indent_level;
        if (die_indent_level >= DIE_STACK_SIZE ) {
            esb_destructor(&valname);
            report_die_stack_error(dbg,err);
            return DW_DLV_ERROR;
        }
        res = get_attr_value(dbg, tag, die,
            dieprint_cu_goffset,
            attrib, srcfiles, srcfcnt,
            &specificationstr,glflags.show_form_used,
            glflags.verbose, err);
        if (res != DW_DLV_OK) {
            esb_destructor(&valname);
            return res;
        }
        esb_append(&valname, esb_get_string(&specificationstr));
        esb_destructor(&specificationstr);

        /* Get the global offset for reference */
        if (theform == DW_FORM_ref_sig8) {
            res = dd_print_sig8_target(dbg,attrib,
                die_indent_level,
                srcfiles,srcfcnt,
                &valname,err);
            if (res == DW_DLV_ERROR) {
                esb_destructor(&valname);
                return res;
            }
            break;
        }
        res = dwarf_global_formref_b(attrib, &ref_goff,
            &is_info, err);
        if (res == DW_DLV_ERROR) {
            /* errno > 0 , < 1000 */
            int dwerrno = (int)dwarf_errno(*err);
            if (dwerrno == DW_DLE_REF_SIG8_NOT_HANDLED ) {
                /*  No need to stop, ref_sig8 refers out of
                    the current section. */
                DROP_ERROR_INSTANCE(dbg,res,*err);
                break;
            } else {
                print_error_and_continue(
                    "dwarf_global_formref_b fails in "
                    "attribute traversal",
                    res, *err);
                esb_destructor(&valname);
                return res;
            }
        }
        if (res == DW_DLV_NO_ENTRY) {
            return res;
        }
        /* Gives die offset in section. */
        res = dwarf_dieoffset(die, &die_goff, err);
        if (res == DW_DLV_ERROR) {
            /* errno > 0 , < 1000 */
            int dwerrno = (int)dwarf_errno(*err);
            if (dwerrno == DW_DLE_REF_SIG8_NOT_HANDLED ) {
                /*  No need to stop, ref_sig8 refers out of
                    the current section. */
                DROP_ERROR_INSTANCE(dbg,res,*err);
                break;
            } else {
                print_error_and_continue(
                    "dwarf_dieoffset fails in "
                    " attribute traversal", res, *err);
                esb_destructor(&valname);
                return res;
            }
        }

        /* Follow reference chain, looking for self references */
        res = dwarf_offdie_b(dbg,ref_goff,is_info,&ref_die,err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "dwarf_dieoff_b fails in "
                " attribute traversal", res, *err);
            esb_destructor(&valname);
            return res;
        }
        if (res == DW_DLV_OK) {
            Dwarf_Off target_die_cu_goff = 0;

            if (glflags.nTrace[KIND_VISITED_INFO]) {
                Dwarf_Off die_loff = 0;

                res = dwarf_die_CU_offset(die, &die_loff, err);
                if (res != DW_DLV_OK) {
                    esb_destructor(&valname);
                    return res;
                }
                dd_do_dump_visited_info(die_indent_level,
                    die_loff,die_goff,
                    dieprint_cu_goffset,
                    atname,esb_get_string(&valname));
            }
            ++die_indent_level;
            if (die_indent_level >= DIE_STACK_SIZE ) {
                report_die_stack_error(dbg,err);
                esb_destructor(&valname);
                return DW_DLV_ERROR;
            }
            res =dwarf_CU_dieoffset_given_die(ref_die,
                &target_die_cu_goff, err);
            if (res != DW_DLV_OK) {
                print_error_and_continue(
                    "dwarf_dieoffset() accessing cu_goff "
                    "die fails in traversal",
                    res,*err);
                esb_destructor(&valname);
                return res;
            }
            res = dd_traverse_one_die(dbg,attrib,ref_die,
                target_die_cu_goff,
                is_info,
                srcfiles,srcfcnt,die_indent_level,
                err);
            DeleteKeyInBucketGroup(glflags.pVisitedInfo,ref_goff);
            if (glflags.nTrace[KIND_VISITED_INFO]) {
                PrintBucketGroup("after dd_traverse_one_die PD B",
                    glflags.pVisitedInfo);
            }
            dwarf_dealloc_die(ref_die);
            if (res == DW_DLV_ERROR) {
                esb_destructor(&valname);
                return res;
            }
            --die_indent_level;
            ref_die = 0;
        }
        }
        break;
    default: break;
    } /* End switch. */
    esb_destructor(&valname);
    return DW_DLV_OK;
}

/*  Traverse one DIE in order to detect
    self references to DIES.

    This fails to deal with changing CUs via global
    references so srcfiles and cnt
    have possibly inappropriate values. FIXME.
*/
int
dd_traverse_one_die(Dwarf_Debug dbg,
    Dwarf_Attribute attrib,
    Dwarf_Die die,
    Dwarf_Off dieprint_cu_goffset,
    Dwarf_Bool is_info,
    char **srcfiles, Dwarf_Signed cnt,
    int die_indent_level,
    Dwarf_Error *err)
{
    Dwarf_Half tag = 0;
    Dwarf_Off overall_offset = 0;
    Dwarf_Signed atcnt = 0;
    int res = 0;

    res = dwarf_tag(die, &tag, err);
    if (res != DW_DLV_OK) {
        print_error_and_continue(
            "Problem accessing tag of die!"
            " from traverse_one_die", res, *err);
        return res;
    }
    res = dwarf_dieoffset(die, &overall_offset, err);
    if (res != DW_DLV_OK) {
        print_error_and_continue(
            "dwarf_dieoffset fails in traversing die",
            res, *err);
        return res;
    }

    if (glflags.nTrace[KIND_VISITED_INFO]) {
        Dwarf_Off offset = 0;
        const char * tagname = 0;
        res = dwarf_die_CU_offset(die, &offset, err);
        if (res != DW_DLV_OK) {
            print_error_and_continue(
                "dwarf_die_CU_offset fails in traversing die",
                res, *err);
            return res;
        }
        tagname = get_TAG_name(tag);
        dd_do_dump_visited_info(die_indent_level,offset,
            overall_offset, dieprint_cu_goffset,
            tagname,"");
    }

    DWARF_CHECK_COUNT(self_references_result,1);
    if (FindKeyInBucketGroup(glflags.pVisitedInfo,
        overall_offset)) {
        char * localvaln = NULL;
        Dwarf_Half attr = 0;
        struct esb_s bucketgroupstr;
        const char *atname = NULL;

        if (glflags.nTrace[KIND_VISITED_INFO]) {
            PrintBucketGroup(
                "after finding offset possible error PD C",
                glflags.pVisitedInfo);
        }
        esb_constructor(&bucketgroupstr);
        res = get_attr_value(dbg, tag, die,
            dieprint_cu_goffset,
            attrib, srcfiles,
            cnt, &bucketgroupstr,
            glflags.show_form_used, glflags.verbose,err);
        if (res != DW_DLV_OK) {
            return res;
        }
        localvaln = esb_get_string(&bucketgroupstr);
        res = dwarf_whatattr(attrib, &attr, err);
        if (res != DW_DLV_OK) {
            print_error_and_continue(
                "ERROR: dwarf_whatattr fails in traverse die",
                res,*err);
            return res;
        }
        atname = get_AT_name(attr);

        /* We have a self reference */
        DWARF_CHECK_ERROR3(self_references_result,
            "Invalid self reference to DIE: ",atname,localvaln);
        esb_destructor(&bucketgroupstr);
    } else {
        Dwarf_Signed i = 0;
        Dwarf_Attribute *atlist = 0;

        /* Add current DIE */
        AddEntryIntoBucketGroup(glflags.pVisitedInfo,
            overall_offset,
            0,0,0,NULL,FALSE);
        if (glflags.nTrace[KIND_VISITED_INFO]) {
            PrintBucketGroup(
                "after adding offset to table PD D",
                glflags.pVisitedInfo);
        }
        res = dwarf_attrlist(die, &atlist, &atcnt, err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "dwarf_attrlist fails in traverse die",
                res, *err);
            return res;
        } else if (res == DW_DLV_NO_ENTRY) {
            /* indicates there are no attrs.
            It is not an error. */
            atcnt = 0;
        }

        for (i = 0; i < atcnt; i++) {
            Dwarf_Half attr;
            int ares;

            ares = dwarf_whatattr(atlist[i], &attr, err);
            if (ares == DW_DLV_OK) {
                ares = traverse_attribute(dbg, die,
                    dieprint_cu_goffset,
                    is_info,
                    attr,
                    atlist[i],
                    srcfiles, cnt,
                    die_indent_level,err);
                if (ares == DW_DLV_ERROR) {
                    dealloc_local_atlist(dbg,atlist,atcnt);
                    return ares;
                }
            } else {
                dealloc_local_atlist(dbg,atlist,atcnt);
                print_error_and_continue(
                    "dwarf_whatattr entry missing in "
                    " traverse die",
                    ares, *err);
                return ares;
            }
        }

        dealloc_local_atlist(dbg,atlist,atcnt);
        /* Delete current DIE */
        DeleteKeyInBucketGroup(glflags.pVisitedInfo,
            overall_offset);
        if (glflags.nTrace[KIND_VISITED_INFO]) {
            PrintBucketGroup(
                "after deleting offset from table PD E",
                glflags.pVisitedInfo);
        }
    }
    return DW_DLV_OK;
}

/*  Extracted this from print_attribute()
    to get tolerable indents.
    In other words to make it readable.
    It uses global data fields excessively, but so does
    print_attribute().
    The majority of the code here is checking for
    compiler errors.
    Support for .debug_rnglists here is new May 2020.
    */
static int
print_range_attribute(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Half attr,
    Dwarf_Attribute attr_in,
    Dwarf_Half theform,
    Dwarf_Bool print_else_name_match,
    int *append_extra_string,
    struct esb_s *esb_extrap,
    Dwarf_Error *raerr)
{
    Dwarf_Unsigned original_off = 0;
    int fres = 0;
    Dwarf_Half cu_version = 2;
    Dwarf_Half cu_offset_size = 4;
    Dwarf_Bool is_info = TRUE;

    fres = dwarf_get_version_of_die(die,&cu_version,
        &cu_offset_size);
    if (fres != DW_DLV_OK) {
        simple_err_return_msg_either_action(fres,
            "\nERROR: Unable to get version of a DIE "
            "to print a range attribute so something "
            " is badly wrong. Assuming DWARF2, offset size 4"
            "  and continuing!");
    }
    if (theform == DW_FORM_rnglistx) {
        fres = dwarf_formudata(attr_in, &original_off, raerr);
        if (fres == DW_DLV_ERROR) {
            print_error_and_continue(
                "ERROR: In printing a range "
                "DW_FORM_rnglistx  attribute "
                "dwarf_formudata failed ",fres,*raerr);
            return fres;
        }
    } else {
        fres = dwarf_global_formref_b(attr_in, &original_off,
            &is_info,
            raerr);
        if (fres == DW_DLV_ERROR) {
            print_error_and_continue(
                "ERROR: In printing a range attribute "
                "dwarf_global_formref_b failed ",fres,*raerr);
            return fres;
        }
    }
    if (fres == DW_DLV_OK && cu_version < DWVERSION5) {
        Dwarf_Ranges *rangeset = 0;
        Dwarf_Signed rangecount = 0;
        Dwarf_Unsigned bytecount = 0;
        Dwarf_Unsigned realoffset = 0;
        /*  If this is a dwp the ranges will be
            missing or reported from a tied file.
            For now we add the ranges to dbg, not tiedbg
            as we do not mention tieddbg here.
            May need a new interface. FIXME?
            In the dwp case where the actual
            section is in tied (ie, a.out)
            the DW_AT_GNU_ranges_base
            is used, not the original_off.
            We really want to print the actual offset,
            in that case, not original_off.
            realoffset is that final actual section offset.
            */
        int rres = dwarf_get_ranges_b(dbg,original_off,
            die,
            &realoffset,
            &rangeset,
            &rangecount,&bytecount,raerr);
        if (rres == DW_DLV_OK) {

            /* Ignore ranges inside a stripped function  */
            if (!glflags.gf_suppress_checking_on_dwp &&
                glflags.gf_check_ranges &&
                glflags.in_valid_code &&
                checking_this_compiler()) {
                /*  Record the offset, as the ranges check
                    will be done at
                    the end of the compilation unit;
                    this approach solves
                    the issue of DWARF4 generating values
                    for the high pc
                    as offsets relative to the low pc
                    and the compilation
                    unit having DW_AT_ranges attribute. */
                int dores = 0;
                Dwarf_Off die_glb_offset = 0;
                Dwarf_Off die_off = 0;
                dores = dwarf_die_offsets(die,&die_glb_offset,
                    &die_off,raerr);
                if (dores == DW_DLV_ERROR) {
                    dwarf_dealloc_ranges(dbg,rangeset,rangecount);
                    return dores;
                }
                if (dores == DW_DLV_OK) {
                    record_range_array_info_entry(die_glb_offset,
                        realoffset);
                }
            }
            if (print_else_name_match) {
                *append_extra_string = 1;
                print_ranges_list_to_extra(dbg,die,
                    original_off, realoffset,
                    rangeset,rangecount,bytecount,
                    esb_extrap);
            }
            dwarf_dealloc_ranges(dbg,rangeset,rangecount);
        } else if (rres == DW_DLV_ERROR) {
            if ( glflags.gf_suppress_checking_on_dwp) {
                /* Ignore checks */
            } else if ( glflags.gf_do_print_dwarf) {
                printf("\ndwarf_get_ranges_b() "
                    "cannot find DW_AT_ranges at offset 0x%"
                    DW_PR_XZEROS DW_PR_DUx
                    " (0x%" DW_PR_XZEROS DW_PR_DUx ").",
                    original_off,
                    original_off);
            } else {
                DWARF_CHECK_COUNT(ranges_result,1);
                DWARF_CHECK_ERROR2(ranges_result,
                    get_AT_name(attr),
                    " cannot find DW_AT_ranges at offset");
            }
            return rres;
        } else {
            /* NO ENTRY */
            if ( glflags.gf_suppress_checking_on_dwp) {
                /* Ignore checks */
            } else if ( glflags.gf_do_print_dwarf) {
                printf("\ndwarf_get_ranges_b() "
                    "finds no DW_AT_ranges at offset 0x%"
                    DW_PR_XZEROS DW_PR_DUx
                    " (%" DW_PR_DUu ").",
                    original_off,
                    original_off);
            } else {
                DWARF_CHECK_COUNT(ranges_result,1);
                DWARF_CHECK_ERROR2(ranges_result,
                    get_AT_name(attr),
                    " fails to find DW_AT_ranges at offset");
            }
        }
        return DW_DLV_OK;
    } else if (fres == DW_DLV_OK && cu_version >= DWVERSION5) {
        /*  Here we have to access the .debug_rnglists section
            data with a new layout for DW5.  Here we
            do not need to actually use rleoffset since it
            is identical to original_off.  */
        int res = 0;
        Dwarf_Unsigned rleoffset = 0;

        res = handle_rnglists( attr_in,
            theform,
            original_off,
            &rleoffset,
            esb_extrap,
            glflags.verbose,
            raerr);
        if (print_else_name_match) {
            *append_extra_string = 1;
        }
        return res;
    }
    /*  DW_DLV_NO_ENTRY or DW_DLV_ERROR */
    if (glflags.gf_do_print_dwarf) {
        struct esb_s local;
        char tmp[ESB_FIXED_ALLOC_SIZE];

        esb_constructor_fixed(&local,tmp,sizeof(tmp));
        esb_append(&local,
            " fails to find DW_AT_ranges offset");
        esb_append_printf_u(&local," attr 0x%x",attr);
        esb_append_printf_u(&local," form 0x%x",theform);
        printf(" %s ",esb_get_string(&local));
        esb_destructor(&local);
    } else {
        DWARF_CHECK_COUNT(ranges_result,1);
        DWARF_CHECK_ERROR2(ranges_result,
            get_AT_name(attr),
            " fails to find DW_AT_ranges offset");
    }
    return fres;
}

/*  A DW_AT_name in a CU DIE will likely have dots
    and be entirely sensible. So lets
    not call things a possible error when they are not.
    Some assemblers allow '.' in an identifier too.

    This is a heuristic, not all that reliable.
    It is only used for a specific DWARF_CHECK_ERROR,
    and the 'altabi.' is from a specific
    (unnamed here) compiler.

    Return 0 (FALSE) if it is a vaguely standard identifier.
    Else return 1 (TRUE), meaning 'it might be a file name
    or have  '.' in it quite sensibly.'

    If we don't do the TAG check we might report "t.c"
    as a questionable DW_AT_name. Which would be silly.
*/
static Dwarf_Bool
dot_ok_in_identifier(int tag,
    const char *val)
{
    if (strncmp(val,"altabi.",7)) {
        /*  Ignore the names of the form 'altabi.name',
            which apply to one specific compiler.  */
        return TRUE;
    }
    if (tag == DW_TAG_compile_unit ||
        tag == DW_TAG_partial_unit ||
        tag == DW_TAG_imported_unit ||
        tag == DW_TAG_skeleton_unit ||
        tag == DW_TAG_type_unit) {
        return TRUE;
    }
    return FALSE;
}

static void
trim_quotes(const char *val,struct esb_s *es)
{
    if (val[0] == '"') {
        size_t l = strlen(val);
        if (l > 2 && val[l-1] == '"') {
            esb_appendn(es,val+1,l-2);
            return;
        }
    }
    esb_append(es,val);
}

static Dwarf_Bool
have_a_search_match(const char *valname,const char *atname)
{
    /*  valname may have had quotes inserted,
        but search_match_text
        will not. So we need to use a new copy,
        not valname here.  */
    char matchbuf[100]; /* not ESB_FIXED_ALLOC_SIZE ? */
    struct esb_s esb_match;
    char *s2;
    int res = 0;

    esb_constructor_fixed(&esb_match,matchbuf,sizeof(matchbuf));
    trim_quotes(valname,&esb_match);
    s2 = esb_get_string(&esb_match);
    if (glflags.search_match_text ) {
        if (!strcmp(s2,glflags.search_match_text) ||
            !strcmp(atname,glflags.search_match_text)) {

            esb_destructor(&esb_match);
            return TRUE;
        }
    }
    if (glflags.search_any_text) {
        if (is_strstrnocase(s2,glflags.search_any_text) ||
            is_strstrnocase(atname,glflags.search_any_text)) {

            esb_destructor(&esb_match);
            return TRUE;
        }
    }
    if (glflags.search_regex_text) {
        res = dd_re_exec((char *)s2);
        if (res == DW_DLV_OK) {
            esb_destructor(&esb_match);
            return TRUE;
        }
        res = dd_re_exec((char *)atname);
        if (res == DW_DLV_OK) {
            esb_destructor(&esb_match);
            return TRUE;
        }
    }
    esb_destructor(&esb_match);
    return FALSE;
}

/*  Use our local die_stack to try to determine
    signedness of the DW_AT_discr_list
    LEB numbers.   Returns -1 if we know
    it is signed.  Returns 1 if we know it is
    unsigned.  Returns 0 if we really do not know. */
static int
determine_discr_signedness(Dwarf_Debug dbg)
{
    Dwarf_Die parent = 0;
    Dwarf_Half tag = 0;
    int tres = 0;
    Dwarf_Error descrerr = 0;

    if (die_stack_indent_level < 1) {
        /*  We have no idea. */
        return 0;
    }
    parent = die_stack[die_stack_indent_level -1].die_;
    if (!parent) {
        /*  We have no idea. */
        return 0;
    }
    tres = dwarf_tag(parent, &tag, &descrerr);
    if (tres != DW_DLV_OK) {
        DROP_ERROR_INSTANCE(dbg,tres,descrerr);
        return 0;
    }
    if (tag != DW_TAG_variant_part) {
        return 0;
    }
    /*  Expect DW_AT_discr or DW_AT_type here, and if
        DW_AT_discr, that might have the DW_AT_type. */

    /*   FIXME: For now lets just punt, say unsigned. */
    return 1;
}

static void
checksignv(
    struct esb_s *strout,
    const char *title,
    Dwarf_Signed sv,
    Dwarf_Unsigned uv)
{
    /*  The test and output are not entirely meaningful, but
        it can be useful for readers of dwarfdump output. */
    if (uv == (Dwarf_Unsigned)sv) {
        /* Nothing to do here. */
        return;
    }
    esb_append(strout," <");
    esb_append(strout,title);
    esb_append(strout," ");
    esb_append_printf_i(strout,"%" DW_PR_DSd ":",sv);
    esb_append_printf_u(strout,"%" DW_PR_DUu ">",uv);
}

static int
append_discr_array_vals(Dwarf_Dsc_Head h,
    Dwarf_Unsigned arraycount,
    int isunsigned,
    struct esb_s *strout,
    Dwarf_Error*paerr)
{
    Dwarf_Unsigned u = 0;
    if (isunsigned == 0) {
        esb_append(strout,
            "<discriminant list signedness unknown>");
    }
    esb_append_printf_u(strout,
        "\n        discr list array len: "
        "%" DW_PR_DUu "\n",arraycount);
    for (u = 0; u < arraycount; u++) {
        int u2res = 0;
        Dwarf_Half dtype = 0;
        Dwarf_Signed slow = 0;
        Dwarf_Signed shigh = 0;
        Dwarf_Unsigned ulow = 0;
        Dwarf_Unsigned uhigh = 0;
        const char *dsc_name = "";

        u2res = dwarf_discr_entry_u(h,u,
            &dtype,&ulow,&uhigh,paerr);
        if (u2res == DW_DLV_ERROR) {
            print_error_and_continue(
                "DW_AT_discr_list entry access fail\n",
                u2res, *paerr);
            return u2res;
        }
        u2res = dwarf_discr_entry_s(h,u,
            &dtype,&slow,&shigh,paerr);
        if (u2res == DW_DLV_ERROR) {
            print_error_and_continue(
                "DW_AT_discr_list entry access fail\n",
                u2res, *paerr);
        }
        if (u2res == DW_DLV_NO_ENTRY) {
            glflags.gf_count_major_errors++;
            esb_append_printf_u(strout,
                "\n          "
                "ERROR: discr index missing! %"  DW_PR_DUu,u);
            break;
        }
        esb_append_printf_u(strout,
            "        "
            "%" DW_PR_DUu ": ",u);
        dsc_name = get_DSC_name(dtype);
        esb_append(strout,sanitized(dsc_name));
        esb_append(strout," ");
        if (!dtype) {
            if (isunsigned < 0) {
                esb_append_printf_i(strout,"%" DW_PR_DSd,slow);
                checksignv(strout,"as signed:unsigned",slow,ulow);
            } else {
                esb_append_printf_u(strout,"%" DW_PR_DUu,ulow);
                checksignv(strout,"as signed:unsigned",slow,ulow);
            }
        } else {
            if (isunsigned < 0) {
                esb_append_printf_i(strout,"%" DW_PR_DSd,slow);
                checksignv(strout,"as signed:unsigned",slow,ulow);
            } else {
                esb_append_printf_u(strout,"%" DW_PR_DUu,ulow);
                checksignv(strout,"as signed:unsigned",slow,ulow);
            }
            if (isunsigned < 0) {
                esb_append_printf_i(strout,", %" DW_PR_DSd,shigh);
                checksignv(strout,"as signed:unsigned",
                    shigh,uhigh);
            } else {
                esb_append_printf_u(strout,", %" DW_PR_DUu,uhigh);
                checksignv(strout,"as signed:unsigned",
                    shigh,uhigh);
            }
        }
        esb_append(strout,"\n");
    }
    return DW_DLV_OK;
}

static int
print_location_description(Dwarf_Debug dbg,
    Dwarf_Attribute attrib,
    Dwarf_Die die,
    int checking,
    Dwarf_Half attr,
    int die_indent_level,
    struct esb_s *base,
    struct esb_s *details,
    Dwarf_Error *err)
{
    /*  The attribute is a location description
        or location list. */
    int res = 0;
    Dwarf_Half theform = 0;
    Dwarf_Half directform = 0;
    Dwarf_Half version = 0;
    Dwarf_Half offset_size = 0;

    res = get_form_values(attrib,&theform,&directform,
        err);
    if (res == DW_DLV_ERROR) {
        return res;
    }
    res = dwarf_get_version_of_die(die,
        &version,&offset_size);
    if (is_simple_location_expr(theform)) {
        res  = print_location_list(dbg, die, attrib,
            checking,
            die_indent_level,
            TRUE,base,err);
        if (res == DW_DLV_ERROR) {
            return res;
        }
    } else if (is_location_form(theform)) {
        res  = print_location_list(dbg, die, attrib,
            checking,
            die_indent_level,
            FALSE,
            details,err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "ERROR: Cannot get location list"
                " data",
                res, *err);
            return res;
        }
    } else {
        show_attr_form_error(attr,theform,base);
    }
    return DW_DLV_OK;
}

/* This was inside print_attribute() */
static void
check_attr_tag_combination(Dwarf_Debug dbg,
Dwarf_Half tag,Dwarf_Half attr)
{
    const char *tagname = "<tag invalid>";
    int res = 0;

    DWARF_CHECK_COUNT(attr_tag_result,1);
    res = legal_tag_attr_combination(tag, attr);
    switch(res) {
    case TK_OK:
        break;
    case TK_SHOW_MESSAGE:
    /* Report errors only if tag-attr check is on */
        if (glflags.gf_check_tag_attr) {
            tagname = get_TAG_name(tag);
            tag_specific_globals_setup(dbg,tag,
                die_stack_indent_level);
            DWARF_CHECK_ERROR3(attr_tag_result,tagname,
                get_AT_name(attr),
                "check the tag-attr combination");
        }
        break;
    case TK_ERROR: /* fall thru */
    default:
        {
            static unsigned long errcount = 0;
            if (!errcount) {
                printf("ERROR: Tag 0x%x "
                    " Attribute 0x%x fails for unknown"
                    " reason, possibly out of memory"
                    " building large search trees."
                    " This message will not repeat\n",
                    tag,attr);
                glflags.gf_count_major_errors++;
            }
            ++errcount;
        }
        break;
    }
}

static void
remark_wrong_string_format(Dwarf_Half attr,
    Dwarf_Half theform)
{
    char buf[VSFBUFSZ+1];
    struct esb_s m;

    buf[0] = 0;
    esb_constructor_fixed(&m,buf,VSFBUFSZ);
    esb_append_printf_s(&m,
        "ERROR: Cannot print the value of "
        "attribute %s ",
        get_AT_name(attr));
    esb_append_printf_s(&m,
        "as it has form %s which seems wrong.",
        get_FORM_name(theform));
    esb_append(&m," Corrupted DWARF? Continuing.");
    simple_err_return_msg_either_action(DW_DLV_ERROR,
        esb_get_string(&m));
    esb_destructor(&m);
    return;
}

static int
print_attribute(Dwarf_Debug dbg, Dwarf_Die die,
    Dwarf_Off dieprint_cu_goffset,
    Dwarf_Half attr,
    Dwarf_Attribute attr_in,
    Dwarf_Bool print_else_name_match,
    int die_indent_level,
    char **srcfiles, Dwarf_Signed srcfiles_cnt,
    LoHiPc     * lohipc,
    Dwarf_Bool *attr_duplication,
    Dwarf_Error *err)
{
    Dwarf_Attribute attrib = 0;
    Dwarf_Unsigned  uval = 0;
    const char *    atname = 0;
    int             tres = 0;
    Dwarf_Half      tag = 0;
    int             append_extra_string = 0;
    Dwarf_Bool         found_search_attr = FALSE;
    Dwarf_Bool         bTextFound = FALSE;
    Dwarf_Addr      max_address = 0;
    struct esb_s    valname;
    struct esb_s    esb_extra;
    char            valbuf[ESB_FIXED_ALLOC_SIZE*3];
    char            xtrabuf[ESB_FIXED_ALLOC_SIZE*3];
    int             res = 0;
    Dwarf_Bool         checking = glflags.gf_do_check_dwarf;
    Dwarf_Half theform = 0;
    Dwarf_Half directform = 0;
    Dwarf_Half version = 0;
    Dwarf_Half offset_size = 0;
    enum Dwarf_Form_Class fc = DW_FORM_CLASS_UNKNOWN;
    Dwarf_Half address_size_base = 0;
    Dwarf_Half address_size_again = 0;
    Dwarf_Bool attribute_handled_by_switch = TRUE;
    Dwarf_Bool is_class_reference = FALSE;

    esb_constructor_fixed(&esb_extra,xtrabuf,sizeof(xtrabuf));
    esb_constructor_fixed(&valname,valbuf,sizeof(valbuf));
    atname = get_AT_name(attr);
    res = get_address_size_and_max(dbg,&address_size_base,
        &max_address,err);
    if (res != DW_DLV_OK) {
        print_error_and_continue(
            "Getting address maximum"
            " failed in printing attribute ",res,*err);
        return res;
    }
    /*  Not passing in error, this is just a check of the
        function we call here.  No Dwarf_Error
        will be allocated. */
    res = dwarf_get_die_address_size(die, &address_size_again,0);
    if (res == DW_DLV_OK) {
        static int showedsizeoddness;
        if ( !showedsizeoddness &&
            address_size_again != address_size_base) {
            printf("NOTE: DIE context address size of %u"
                " while object address size is %u."
                " This message will not be repeated.\n",
                address_size_again,address_size_base);
            showedsizeoddness++;
        }
    } else {
        printf("ERROR: DIE context address size "
            "could not be retrieved. Very odd\n");
        glflags.gf_count_major_errors++;
    }
    /*  The following gets the real attribute, even
        in the face of an
        incorrect doubling, or worse, of attributes. */
    attrib = attr_in;
    res = get_form_values(attrib,&theform,&directform, err);
    if (res == DW_DLV_ERROR) {
        print_error_and_continue(
            "ERROR: Cannot get form values",
            res, *err);
        esb_destructor(&valname);
        esb_destructor(&esb_extra);
        return res;
    }
    res = dwarf_get_version_of_die(die,&version,&offset_size);
    if (res != DW_DLV_OK) {
        print_error_and_continue(
            "ERROR: Cannot get DIE context version number"
            " for DW_AT_discr_list",
            DW_DLV_OK,0);
        esb_destructor(&valname);
        esb_destructor(&esb_extra);
        /* Returning DW_DLV_ERROR would be bogus */
        return DW_DLV_NO_ENTRY;
    }
    fc = dwarf_get_form_class(version,attr,offset_size,theform);
    /*  Do not get attr via dwarf_attr:
        if there are (erroneously)
        multiple of an attr in a DIE,
        dwarf_attr will not get the
        second, erroneous one and dwarfdump will
        print the first one
        multiple times. Oops. */
    tres = dwarf_tag(die, &tag, err);
    if (tres != DW_DLV_OK) {
        print_error_and_continue("Getting DIE tag "
            " failed in printing an attribute.",
            tres,*err);
        esb_destructor(&valname);
        esb_destructor(&esb_extra);
        return tres;
    }
    if ((glflags.gf_check_tag_attr ||
        glflags.gf_print_usage_tag_attr)&&
        checking_this_compiler()) {
        check_attr_tag_combination(dbg,tag,attr);
        record_attr_form_use(dbg,tag,attr,(Dwarf_Half)fc,
            theform,
            die_stack_indent_level);
    }

    switch (attr) {
    case DW_AT_language_version: {
        char         atnamebuf[ESB_FIXED_ALLOC_SIZE];
        struct esb_s langver;

        if (fc != DW_FORM_CLASS_CONSTANT) {
            remark_wrong_string_format(attr,theform);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return DW_DLV_NO_ENTRY;
        }
        esb_constructor_fixed(&langver,atnamebuf,
            sizeof(atnamebuf));
        tres = get_attr_value(dbg, tag, die,
            dieprint_cu_goffset,attrib, srcfiles, srcfiles_cnt,
            &langver, glflags.show_form_used,
            glflags.verbose,err);
        if (tres == DW_DLV_ERROR) {
            print_error_and_continue(
                "Cannot  get value "
                "for DW_AT_language_version",
                tres, *err);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return tres;
        }
        esb_empty_string(&valname);
        esb_append(&valname, esb_get_string(&langver));
        esb_destructor(&langver);
        }
        break;
    case DW_AT_language_name: {
        int          lv_lower_bound = 0;
        const char  *lv_version_details = 0;

        res = dd_get_integer_and_name(dbg, attrib,
            &uval,
            "DW_AT_language_name", &valname,
            get_LNAME_name, err,
            glflags.show_form_used);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "Cannot get DW_AT_language_name value. ",
                res,*err);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return res;
        }

        res = dwarf_language_version_data(uval,
            &lv_lower_bound, &lv_version_details);
        if (res == DW_DLV_OK) {
            esb_append_printf_i(&valname,
                " (low bound: %d",lv_lower_bound);
            if (lv_version_details) {
                esb_append_printf_s(&valname,
                    " format: %s)",
                    lv_version_details);
            } else {
                esb_append(&valname,
                    " format unspecified)");
            }
        } else {
            esb_append(&valname,
                "(unable to read language_version details)");
        }
        }
        break;
    case DW_AT_language:
        res = dd_get_integer_and_name(dbg, attrib,
            &uval,
            "DW_AT_language", &valname,
            get_LANG_name, err,
            glflags.show_form_used);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "Cannot get DW_AT_language value. ",
                res,*err);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return res;
        }
        break;
    case DW_AT_accessibility:
        res  = dd_get_integer_and_name(dbg, attrib,
            &uval,
            "DW_AT_accessibility",
            &valname, get_ACCESS_name,
            err,
            glflags.show_form_used);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "Cannot get DW_AT_accessibility value",
                res,*err);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return res;
        }
        break;
    case DW_AT_visibility:
        res = dd_get_integer_and_name(dbg, attrib,
            &uval,
            "DW_AT_visibility",
            &valname, get_VIS_name,
            err,
            glflags.show_form_used);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "Cannot get DW_AT_visibility value.",
                res,*err);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return res;
        }
        break;
    case DW_AT_virtuality:
        res = dd_get_integer_and_name(dbg, attrib,
            &uval,
            "DW_AT_virtuality",
            &valname,
            get_VIRTUALITY_name, err,
            glflags.show_form_used);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "Cannot get DW_AT_virtuality",
                res,*err);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return res;
        }
        break;
    case DW_AT_identifier_case:
        res = dd_get_integer_and_name(dbg, attrib,
            &uval,
            "DW_AT_identifier",
            &valname, get_ID_name,
            err,
            glflags.show_form_used);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "Cannot get DW_AT_identifier_case",
                res,*err);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return res;
        }
        break;
    case DW_AT_inline:
        res = dd_get_integer_and_name(dbg, attrib,
            &uval,
            "DW_AT_inline", &valname,
            get_INL_name, err,
            glflags.show_form_used);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "Cannot get DW_AT_inline", res,*err);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return res;
        }
        break;
    case DW_AT_encoding:
        res =dd_get_integer_and_name(dbg, attrib,
            &uval,
            "DW_AT_encoding", &valname,
            get_ATE_name, err,
            glflags.show_form_used);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "ERROR:Cannot get DW_AT_encoding",
                res,*err);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return res;
        }
        break;
    case DW_AT_ordering:
        res =dd_get_integer_and_name(dbg, attrib,
            &uval,
            "DW_AT_ordering", &valname,
            get_ORD_name, err,
            glflags.show_form_used);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "ERROR:Cannot get DW_AT_ordering",
                res,*err);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return res;
        }
        break;
    case DW_AT_calling_convention:
        res =dd_get_integer_and_name(dbg, attrib,
            &uval,
            "DW_AT_calling_convention",
            &valname, get_CC_name,
            err,
            glflags.show_form_used);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "ERROR:Cannot get DW_AT_calling_convention",
                res,*err);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return res;
        }
        break;
    case DW_AT_discr_list: {      /* DWARF2 */
        /*  This has one of the block forms.
            It should be in a DW_TAG_variant.
            Up to September 2016 it was treated as integer or name
            here, which was quite wrong. */

        if (fc == DW_FORM_CLASS_BLOCK) {
            int fres = 0;
            Dwarf_Block *tempb = 0;
            /*  the block is a series of entries each of one
                of these formats:
                DW_DSC_label  caselabel
                DW_DSC_range  lowvalue highvalue

                The values are all LEB. Signed or unsigned
                depending on the DW_TAG_variant_part owning
                the DW_TAG_variant.  The DW_TAG_variant_part
                will have a DW_AT_type or a DW_AT_discr
                and that attribute will reveal the
                signedness of all
                the leb values.
                As a practical matter DW_DSC_label/DW_DSC_range
                value (zero or one, so far)
                can safely be read as ULEB or SLEB
                and one gets a valid value whereas
                the caselabel, lowvalue,highvalue must be
                decoded with the proper sign. the high level
                (dwarfdump in this case) is the agent that
                should determine the proper signedness.  */

            fres = dwarf_formblock(attrib, &tempb,err);
            if (fres == DW_DLV_OK) {
                struct esb_s bformstr;
                int isunsigned = 0; /* Meaning unknown */
                Dwarf_Dsc_Head h = 0;
                Dwarf_Unsigned arraycount = 0;
                int sres = 0;
                char fbuf[ESB_FIXED_ALLOC_SIZE];

                esb_constructor_fixed(&bformstr,fbuf,
                    sizeof(fbuf));
                show_form_itself(glflags.show_form_used,
                    glflags.verbose,
                    theform, directform,&bformstr);
                isunsigned = determine_discr_signedness(dbg);
                esb_empty_string(&valname);

                sres = dwarf_discr_list(dbg,
                    (Dwarf_Small *)tempb->bl_data,
                    tempb->bl_len,
                    &h,&arraycount,err);
                if (sres == DW_DLV_NO_ENTRY) {
                    esb_append(&bformstr,
                        "<empty discriminant list>");
                    break;
                }
                if (sres == DW_DLV_ERROR) {
                    print_error_and_continue(
                        "ERROR: DW_AT_discr_list access fail",
                        sres, *err);
                    esb_destructor(&valname);
                    esb_destructor(&esb_extra);
                    return sres;
                }
                sres = append_discr_array_vals(h,arraycount,
                    isunsigned,&bformstr,err);
                if (sres == DW_DLV_ERROR) {
                    print_error_and_continue(
                        "ERROR: getting discriminant values "
                        "failed",
                        sres, *err);
                    esb_destructor(&valname);
                    esb_destructor(&esb_extra);
                    return sres;
                }

                if (glflags.verbose > 1) {
                    unsigned u = 0;
                    esb_append_printf_u(&bformstr,
                        "\n        block byte len:"
                        "0x%" DW_PR_XZEROS DW_PR_DUx
                        "\n        ",tempb->bl_len);
                    for (u = 0; u < tempb->bl_len; u++) {
                        esb_append_printf_u(&bformstr, "%02x ",
                            *(u +
                            (unsigned char *)tempb->bl_data));
                    }
                }
                esb_append(&valname, esb_get_string(&bformstr));
                dwarf_dealloc(dbg,h,DW_DLA_DSC_HEAD);
                dwarf_dealloc(dbg, tempb, DW_DLA_BLOCK);
                esb_destructor(&bformstr);
                tempb = 0;
            } else {
                print_error_and_continue(
                    "ERROR: DW_AT_discr_list: cannot get list"
                    "data", fres, *err);
                esb_destructor(&valname);
                esb_destructor(&esb_extra);
                return fres;
            }
        } else {
            print_error_and_continue(
                "DW_AT_discr_list is not form class BLOCK",
                fc, *err);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return fc;
        }
        }
        break;
    case DW_AT_allocated:
    case DW_AT_associated:
    case DW_AT_call_value:
    case DW_AT_call_data_value:
    case DW_AT_call_data_location:
    case DW_AT_call_origin:
    case DW_AT_call_target:
    case DW_AT_call_target_clobbered:
    case DW_AT_const_value:
    case DW_AT_count:
    case DW_AT_data_location:
    case DW_AT_data_member_location:
    case DW_AT_default_value:
    case DW_AT_frame_base:
    case DW_AT_GNU_call_site_value:
    case DW_AT_GNU_call_site_target:
    case DW_AT_byte_stride:
    case DW_AT_byte_size:
    case DW_AT_bit_size:
    case DW_AT_bit_stride:
    case DW_AT_location:
    case DW_AT_lower_bound:
    case DW_AT_rank:
    case DW_AT_return_addr:
    case DW_AT_segment:
    case DW_AT_static_link:
    case DW_AT_string_length:
    case DW_AT_upper_bound:
    case DW_AT_use_location:
    case DW_AT_vtable_elem_location:
        {
            /*  Value is a constant or a location
                description or location list.
                If a constant, it could be signed or
                unsigned.  Telling whether a constant
                or a reference is nontrivial
                since DW_FORM_data{4,8}
                could be either in DWARF{2,3}  */

            if (fc == DW_FORM_CLASS_CONSTANT) {
                struct esb_s classconstantstr;
                Dwarf_Bool chex = FALSE;
                int wres = 0;

                esb_constructor(&classconstantstr);
                /*  Makes no sense to look at type of our DIE
                    to determine how to print the constant. */
                wres = formxdata_print_value(dbg,NULL,attrib,
                    theform,
                    &classconstantstr,
                    err, chex);
                if (wres != DW_DLV_OK) {
                    esb_destructor(&valname);
                    esb_destructor(&esb_extra);
                    return wres;
                }
                show_form_itself(glflags.show_form_used,
                    glflags.verbose,
                    theform, directform, &classconstantstr);
                esb_empty_string(&valname);
                esb_append(&valname,
                    esb_get_string(&classconstantstr));
                esb_destructor(&classconstantstr);
                break;
            }
            /*  FALL THRU, this is a
                a location description, or a reference
                to one, or a mistake. */
        }
        {
        Dwarf_Bool showform = glflags.show_form_used;

        /*  If DW_FORM_block* && show_form_used
            get_attr_value() results
            in duplicating the form name (with -M). */
        /*  For block forms, this will show block len and bytes
            and if showing form, then form shown */
        res = get_attr_value(dbg, tag, die,
            dieprint_cu_goffset,
            attrib, srcfiles, srcfiles_cnt, &valname,
            showform,
            glflags.verbose,
            err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "Cannot get attr form value", res,*err);
            DROP_ERROR_INSTANCE(dbg,res,*err);
            break;
        }
        append_extra_string = TRUE;
        if (fc == DW_FORM_CLASS_EXPRLOC ||
            fc == DW_FORM_CLASS_LOCLIST ||
            fc == DW_FORM_CLASS_LOCLISTPTR) {
            /*  Form class exprloc is set even for DWARF2&3,
                see libdwarf: dwarf_get_form_class(). */
            res = print_location_description(dbg,attrib,die,
                checking,
                attr,die_indent_level,
                &valname,&esb_extra,err);
            if (res == DW_DLV_ERROR) {
                print_error_and_continue(
                    "Cannot get location data, attr "
                    "(with -M  also form) "
                    "follow", res,*err);
                DROP_ERROR_INSTANCE(dbg,res,*err);
                break;
            }
        }
        }
        break;
    case DW_AT_SUN_func_offsets:
        {
            /* value is a location description or location list */
            char buf[100];
            struct esb_s funcformstr;

            esb_constructor_fixed(&funcformstr,buf,sizeof(buf));
            get_FLAG_BLOCK_string(dbg, attrib,&funcformstr);
            show_form_itself(glflags.show_form_used,
                glflags.verbose, theform,
                directform,&funcformstr);
            esb_empty_string(&valname);
            esb_append(&valname, esb_get_string(&funcformstr));
            esb_destructor(&funcformstr);
        }
        break;
    case DW_AT_SUN_cf_kind:
    {
        Dwarf_Half     kind = 0;
        Dwarf_Unsigned tempud = 0;
        int wres = 0;
        struct esb_s cfkindstr;

        esb_constructor(&cfkindstr);
        wres = dwarf_formudata (attrib,&tempud, err);
        if (wres == DW_DLV_OK) {
            kind = (Dwarf_Half)tempud;
            esb_append(&cfkindstr,
                get_ATCF_name((unsigned int)kind));
            } else if (wres == DW_DLV_NO_ENTRY) {
                esb_append(&cfkindstr,  "?");
            } else {
                print_error_and_continue(
                    "Cannot get formudata length field for"
                    " DW_AT_SUN_cf_kind  ",
                    wres,*err);
                esb_destructor(&valname);
                esb_destructor(&esb_extra);
                return res;
            }
            show_form_itself(glflags.show_form_used,
                glflags.verbose,theform,
                directform,&cfkindstr);
            esb_empty_string(&valname);
            esb_append(&valname, esb_get_string(&cfkindstr));
            esb_destructor(&cfkindstr);
        }
        break;
    case DW_AT_low_pc:
    case DW_AT_high_pc:
        {
            int rv  = 0;
            rv = print_hipc_lopc_attribute(dbg, tag, die,
                dieprint_cu_goffset,
                srcfiles, srcfiles_cnt,
                attrib,
                attr,
                max_address,
                lohipc,
                &valname,
                err);
            if (rv != DW_DLV_OK) {
                esb_destructor(&valname);
                esb_destructor(&esb_extra);
                return rv;
            }
        }
        break;
    case DW_AT_ranges:
        {
            int rv;

            rv = get_attr_value(dbg, tag,die,
                dieprint_cu_goffset,attrib,
                srcfiles, srcfiles_cnt,
                &valname,
                glflags.show_form_used,glflags.verbose,err);
            if (rv == DW_DLV_ERROR) {
                print_error_and_continue(
                    "Cannot find Attr value"
                    "for DW_AT_ranges",
                    rv, *err);
                esb_destructor(&valname);
                esb_destructor(&esb_extra);
                return rv;
            }
            rv = print_range_attribute(dbg, die, attr,attr_in,
                theform,
                print_else_name_match,
                &append_extra_string,
                &esb_extra,err);
            if (rv == DW_DLV_ERROR) {
                print_error_and_continue(
                    "Cannot print range attribute "
                    "for DW_AT_ranges",
                    rv, *err);
                /*  We will not stop, this is an omission
                    in libdwarf on DWARF5 rnglists */
                DROP_ERROR_INSTANCE(dbg,rv,*err);
            }
        }
        break;
    case DW_AT_MIPS_linkage_name:
        {
        int ml = 0;
        char linknamebuf[ESB_FIXED_ALLOC_SIZE];
        struct esb_s linkagenamestr;

        esb_constructor_fixed(&linkagenamestr,linknamebuf,
            sizeof(linknamebuf));
        ml = get_attr_value(dbg, tag, die,
            dieprint_cu_goffset, attrib, srcfiles,
            srcfiles_cnt, &linkagenamestr, glflags.show_form_used,
            glflags.verbose,err);
        if (ml == DW_DLV_ERROR) {
            print_error_and_continue(
                "Cannot  get value "
                "for DW_AT_MIPS_linkage_name ",
                ml, *err);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            esb_destructor(&linkagenamestr);
            return ml;
        }
        if (fc != DW_FORM_CLASS_STRING) {
            remark_wrong_string_format(attr,theform);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            esb_destructor(&linkagenamestr);
            return DW_DLV_NO_ENTRY;
        }

        esb_empty_string(&valname);
        esb_append(&valname, esb_get_string(&linkagenamestr));
        esb_destructor(&linkagenamestr);

        if (glflags.gf_check_locations ||
            glflags.gf_check_ranges) {
            int local_show_form = 0;
            int local_verbose = 0;
            const char *name = 0;
            struct esb_s lesb;

            esb_constructor(&lesb);
            get_attr_value(dbg, tag, die,
                dieprint_cu_goffset,attrib,
                srcfiles, srcfiles_cnt,
                &lesb, local_show_form,local_verbose,err);

            /*  Look for specific name forms, attempting to
                notice and report 'odd' identifiers.
                Used in the SNC LinkOnce feature. */
            name = esb_get_string(&lesb);
            dd_safe_strcpy(glflags.PU_name,sizeof(glflags.PU_name),
                name,esb_string_len(&lesb));
            esb_destructor(&lesb);
        }
        }
        break;
    case DW_AT_name:
    case DW_AT_GNU_template_name:
        {
        char atnamebuf[ESB_FIXED_ALLOC_SIZE];
        struct esb_s templatenamestr;

        if (fc != DW_FORM_CLASS_STRING) {
            remark_wrong_string_format(attr,theform);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return DW_DLV_NO_ENTRY;
        }
        esb_constructor_fixed(&templatenamestr,atnamebuf,
            sizeof(atnamebuf));
        tres = get_attr_value(dbg, tag, die,
            dieprint_cu_goffset,attrib, srcfiles, srcfiles_cnt,
            &templatenamestr, glflags.show_form_used,
            glflags.verbose,err);
        if (tres == DW_DLV_ERROR) {
            print_error_and_continue(
                "Cannot  get value "
                "for DW_AT_name/DW_AT_GNU_template_name ",
                tres, *err);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return tres;
        }
        esb_empty_string(&valname);
        esb_append(&valname, esb_get_string(&templatenamestr));
        esb_destructor(&templatenamestr);

        if ( glflags.gf_check_names && checking_this_compiler()) {
            int local_show_form = FALSE;
            int local_verbose = 0;
            struct esb_s lesb;
            const char *name = 0;

            esb_constructor(&lesb);
            tres = get_attr_value(dbg, tag, die,
                dieprint_cu_goffset,attrib,
                srcfiles, srcfiles_cnt,
                &lesb, local_show_form,local_verbose,err);
            /*  Look for specific name forms, attempting to
                notice and report 'odd' identifiers. */
            if (tres == DW_DLV_ERROR) {
                print_error_and_continue(
                    "Cannot  get check_names value "
                    "for DW_AT_name/DW_AT_GNU_template_name ",
                    tres, *err);
                esb_destructor(&lesb);
                esb_destructor(&valname);
                esb_destructor(&esb_extra);
                return tres;
            }

            name = esb_get_string(&lesb);
            DWARF_CHECK_COUNT(names_result,1);
            if (!strcmp("\"(null)\"",name)) {
                DWARF_CHECK_ERROR(names_result,
                    "string attribute is \"(null)\".");
            } else {
                if (!dot_ok_in_identifier(tag,name) &&
                    !glflags.need_CU_name && strchr(name,'.')) {
                    /*  This is a suggestion there 'might' be
                        a surprising name, not a guarantee of an
                        error. */
                    DWARF_CHECK_ERROR(names_result,
                        "string attribute is invalid.");
                }
            }
            esb_destructor(&lesb);
        }
        }

        /*  If we are in checking mode and we do not
            have a PU name */
        if (( glflags.gf_check_locations ||
            glflags.gf_check_ranges) &&
            glflags.seen_PU && !glflags.PU_name[0]) {
            int local_show_form = FALSE;
            int local_verbose = 0;
            const char *name = 0;
            struct esb_s lesb;
            int vres = 0;

            esb_constructor(&lesb);
            vres = get_attr_value(dbg, tag, die,
                dieprint_cu_goffset,attrib,
                srcfiles, srcfiles_cnt,
                &lesb, local_show_form,local_verbose,err);
            if (vres == DW_DLV_ERROR) {
                print_error_and_continue(
                    "Cannot get check-locations value "
                    "for DW_AT_name/DW_AT_GNU_template_name ",
                    vres, *err);
                esb_destructor(&lesb);
                esb_destructor(&valname);
                esb_destructor(&esb_extra);
                return vres;
            }
            name = esb_get_string(&lesb);
            dd_safe_strcpy(glflags.PU_name,sizeof(glflags.PU_name),
                name, esb_string_len(&lesb));
            esb_destructor(&lesb);
        }

        /*  If we are processing the compile unit,
            record the name */
        if (glflags.seen_CU && glflags.need_CU_name) {
            /* Lets not get the form name included. */
            struct esb_s lesb;
            int local_show_form_used = FALSE;
            int local_verbose = 0;
            int sres = 0;

            esb_constructor(&lesb);
            sres = get_attr_value(dbg, tag, die,
                dieprint_cu_goffset,attrib,
                srcfiles, srcfiles_cnt,
                &lesb, local_show_form_used,local_verbose,
                err);
            if (sres == DW_DLV_ERROR) {
                print_error_and_continue(
                    "Cannot get CU name "
                    "for DW_AT_name/DW_AT_GNU_template_name ",
                    sres, *err);
                esb_destructor(&lesb);
                esb_destructor(&valname);
                esb_destructor(&esb_extra);
                return sres;
            }

            dd_safe_strcpy(glflags.CU_name,sizeof(glflags.CU_name),
                esb_get_string(&lesb),esb_string_len(&lesb));
            glflags.need_CU_name = FALSE;
            esb_destructor(&lesb);
        }
        break;

    case DW_AT_linkage_name:
    case DW_AT_comp_dir:
    case DW_AT_producer:
        {
        struct esb_s lesb;
        int pres = 0;

        if (fc != DW_FORM_CLASS_STRING) {
            remark_wrong_string_format(attr,theform);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return DW_DLV_NO_ENTRY;
        }
        esb_constructor(&lesb);
        pres = get_attr_value(dbg, tag, die,
            dieprint_cu_goffset,attrib, srcfiles, srcfiles_cnt,
            &lesb, glflags.show_form_used,glflags.verbose,
            err);
        if (pres == DW_DLV_ERROR) {
            print_error_and_continue(
                "Cannot  get value "
                "for DW_AT_producer",
                pres, *err);
            esb_destructor(&lesb);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return pres;
        }
        esb_empty_string(&valname);
        esb_append(&valname, esb_get_string(&lesb));
        esb_destructor(&lesb);
        /* If we are in checking mode, identify the compiler */
        if (attr == DW_AT_producer &&
            (glflags.gf_do_check_dwarf ||
            glflags.gf_search_is_on)) {
            /*  Do not use show-form here! We just want
                the producer name, not the form name. */
            int show_form_local = FALSE;
            int local_verbose = 0;
            struct esb_s local_e;

            esb_constructor(&local_e);
            pres = get_attr_value(dbg, tag, die,
                dieprint_cu_goffset,attrib,
                srcfiles, srcfiles_cnt,
                &local_e, show_form_local,local_verbose,err);
            if (pres == DW_DLV_ERROR) {
                print_error_and_continue(
                    "Cannot  get checking value "
                    "for DW_AT_producer",
                    pres, *err);
                esb_destructor(&local_e);
                esb_destructor(&valname);
                esb_destructor(&esb_extra);
                return pres;
            }
            /* Check if this compiler version is a target */
            update_compiler_target(esb_get_string(&local_e));
            esb_destructor(&local_e);
        }
        }
        break;
    default:
        attribute_handled_by_switch = FALSE;
        break;
    } /* end switch statement on attribute code */
    if (!attribute_handled_by_switch) {
        if (fc == DW_FORM_CLASS_REFERENCE) {
            is_class_reference = TRUE;
        }
    }

    /*  Note on checking for special symbols:
        When dealing with SNC linkonce symbols,
        the low_pc and high_pc
        are associated with a specific symbol;
        SNC always generate a name with
        DW_AT_MIPS_linkage_name; GCC does not;
        instead gcc generates
        DW_AT_abstract_origin or DW_AT_specification;
        in that case we have to
        traverse this attribute in order to get the
        name for the linkonce
        DW_AT_GNU_locviews are a reference too,
        and are actually an offset from a base address. */
    if (is_class_reference)  {
        tres = dd_trace_abstract_origin_etc(dbg,tag,die,
            dieprint_cu_goffset,
            theform,
            attr,
            attrib, srcfiles,
            srcfiles_cnt,&valname,&esb_extra,
            die_indent_level,err);
        if (tres != DW_DLV_OK) {
            return tres;
        }
    } else if (!attribute_handled_by_switch) {
        char ebuf[ESB_FIXED_ALLOC_SIZE];
        struct esb_s lesb;
        int dres = 0;

        esb_constructor_fixed(&lesb,ebuf,sizeof(ebuf));
        dres = get_attr_value(dbg, tag,die,
            dieprint_cu_goffset,attrib,
            srcfiles, srcfiles_cnt, &lesb,
            glflags.show_form_used,glflags.verbose,err);
        if (dres == DW_DLV_ERROR) {
            struct esb_s m;
            const char *n =
                get_AT_name(attr);
            esb_constructor(&m);
            esb_append(&m,
                "Cannot get get value for a ");
            esb_append(&m,n);
            print_error_and_continue(
                esb_get_string(&m),
                dres,*err);
            esb_destructor(&m);
            esb_destructor(&valname);
            esb_destructor(&esb_extra);
            return dres;
        }
        esb_empty_string(&valname);
        esb_append(&valname, esb_get_string(&lesb));
        esb_destructor(&lesb);
    }
    res = 0; /* value above no longer relevant */

    if (!print_else_name_match) {
        if (have_a_search_match(esb_get_string(&valname),atname)) {
            /* Count occurrence of text */
            ++glflags.search_occurrences;
            if ( glflags.gf_search_wide_format) {
                found_search_attr = TRUE;
            } else {
                PRINT_CU_INFO();
                bTextFound = TRUE;
            }
        }
    }
    /*  Above we created detailed messages in
        the valname and esb_extra strings.
        If we're just printing everything
        we will now print those.

        Otherwise If we're checking things we will print those
        that failed a check (a DWARF_CHECK message
        was just printed)

        Otherwise if searching for specific attributes
        (the else_name_match of the following if stmt)
        a match means we print the strings.

        Otherwise we will just discard the
        valname and esb_extra strings.

        That's why the big IF below
        has so much in it.
    */

    if ((PRINTING_UNIQUE && PRINTING_DIES && print_else_name_match)
        || bTextFound) {
        /*  Print just the Tags and Attributes */
        if (!glflags.gf_display_offsets) {
            printf("%-28s\n",atname);
        } else {
            if (glflags.dense) {
                char *v = 0;
                v = esb_get_string(&valname);
                printf(" %s<%s>", atname, sanitized(v));
                if (append_extra_string) {
                    v = esb_get_string(&esb_extra);
                    printf("%s", sanitized(v));
                }
            } else {
                char *v = 0;
                printf("%-28s", atname);
                if (strlen(atname) >= 28) {
                    printf(" ");
                }
                v = esb_get_string(&valname);
                printf("%s\n", sanitized(v));
                if (append_extra_string) {
                    v = esb_get_string(&esb_extra);
                    printf("%s", sanitized(v));
                }
            }
        }
    }
    esb_destructor(&valname);
    esb_destructor(&esb_extra);
    *attr_duplication = found_search_attr;
    return DW_DLV_OK;
}

static void
alloc_skip_branch_array(Dwarf_Unsigned no_of_ops,
    struct OpBranchHead_s *op_branch_checking)
{
    op_branch_checking->bh_opcount = 0;
    op_branch_checking->bh_ops_array = 0;
    if (!no_of_ops) {
        return;
    }
    if (no_of_ops >=  (Dwarf_Unsigned)0xffff) {
        /* Something wrong. Give up on the array of branch info */
        return;
    }
    op_branch_checking->bh_ops_array = (struct OpBranchEntry_s *)
        calloc(no_of_ops,sizeof(struct OpBranchEntry_s));
    if (!op_branch_checking->bh_ops_array) {
        return;
    }
    op_branch_checking->bh_opcount = (Dwarf_Half)no_of_ops;
}

static Dwarf_Bool
skip_branch_target_ok(struct OpBranchHead_s *op_branch_checking,
    Dwarf_Half index,
    struct OpBranchEntry_s*ein)
{
    Dwarf_Half i = 0;
    struct OpBranchEntry_s*ec = 0;

    if (ein->target_offset == ein->offset) {
        /* Infinite loop! */
        return FALSE;
    } else if (ein->target_offset < ein->offset) {
        ec = op_branch_checking->bh_ops_array;
        for ( ; i < index; ++i,++ec) {
            if ( ec->offset == ein->target_offset) {
                return TRUE;
            }
        }
        return FALSE;
    }
    i = index;
    ec = op_branch_checking->bh_ops_array+i;
    for ( ; i < op_branch_checking->bh_opcount; ++i,++ec) {
        if ( ec->offset == ein->target_offset) {
            return TRUE;
        }
    }
    return FALSE;
}

static void
check_skip_branch_offsets(struct OpBranchHead_s *op_branch_checking,
    struct esb_s *str)
{
    Dwarf_Half i = 0;
    Dwarf_Half high = op_branch_checking->bh_opcount;
    struct OpBranchEntry_s*e = op_branch_checking->bh_ops_array;

    for ( ; i < high ; ++i,++e) {
        char *opname = "DW_OP_skip";
        if (e->op != DW_OP_bra && e->op != DW_OP_skip) {
            continue;
        }
        if (e->op == DW_OP_bra) {
            opname = "DW_OP_bra";
        }
        if (skip_branch_target_ok(op_branch_checking,i,e)){
            continue;
        }
        /*  Oops. An error. The skip or branch target
            is wrong. Corrupt dwarf */
        esb_append_printf_s(str,
            "\nERROR: The operation %s",opname);
        esb_append_printf_u(str,
            " at expression block offset 0x%04x ",e->offset);
        esb_append_printf_u(str,
            " has target of 0x%04x which is not a valid "
            "expression offset in this expression block\n",
            e->target_offset);
        glflags.gf_count_major_errors++;
    }
}

static void
dealloc_skip_branch_array(struct OpBranchHead_s *op_branch_checking)
{
    if (op_branch_checking->bh_opcount) {
        free(op_branch_checking->bh_ops_array);
        op_branch_checking->bh_ops_array = 0;
        op_branch_checking->bh_opcount = 0;
    }
}

static Dwarf_Bool
op_is_skip_or_branch(Dwarf_Debug dbg,
    Dwarf_Locdesc_c exprc,
    Dwarf_Unsigned  index,
    Dwarf_Error    *err)
{
    Dwarf_Small op = 0;
    Dwarf_Unsigned opd1 = 0;
    Dwarf_Unsigned opd2 = 0;
    Dwarf_Unsigned opd3 = 0;
    Dwarf_Unsigned offsetforbranch = 0;
    int res = 0;

    res = dwarf_get_location_op_value_c(exprc,
        index,
        &op,&opd1,&opd2,&opd3,
        &offsetforbranch,
        err);
    if (res != DW_DLV_OK) {
        if (res == DW_DLV_ERROR) {
            DROP_ERROR_INSTANCE(dbg,res,*err);
        }
        return FALSE;
    }
    if (op == DW_OP_bra) {
        return TRUE;
    }
    if (op == DW_OP_skip) {
        return TRUE;
    }
    return FALSE;
}

int
dwarfdump_print_expression_operations(Dwarf_Debug dbg,
    Dwarf_Die       die,
    int             die_indent_level,
    Dwarf_Locdesc_c locdesc,  /* for 2015 interface. */
    Dwarf_Unsigned  entrycount,
    struct esb_s   *string_out,
    Dwarf_Error    *err)
{
    Dwarf_Unsigned no_of_ops = 0;
    Dwarf_Unsigned i = 0;
    Dwarf_Bool     has_skip_or_branch = FALSE;
    struct OpBranchHead_s op_branch_checking;
    int            stackdepth = 0;
    int            maxstackdepth = 0;
    Dwarf_Unsigned op_loop_count = 0;

    alloc_skip_branch_array(0,&op_branch_checking);
    /* ASSERT: locs != NULL */
    no_of_ops = entrycount;
    possibly_increase_esb_alloc(string_out,no_of_ops,100);
    if (no_of_ops > 1) {
        for (i = 0; i < no_of_ops; i++) {
            if (op_is_skip_or_branch(dbg,locdesc,i,err)) {
                has_skip_or_branch = TRUE;
                break;
            }
        }
    }
    if (has_skip_or_branch) {
        alloc_skip_branch_array(no_of_ops,&op_branch_checking);
    }
    for (i = 0; i < no_of_ops; i++) {
        int res = 0;
        Dwarf_Signed branchdistance = 0;
        int zerostackdepth = FALSE;
        int stackchange = 0;

        res = _dwarf_print_one_expr_op(dbg,die,
            die_indent_level,locdesc,i,
            has_skip_or_branch,
            &op_branch_checking,
            &stackchange,
            &branchdistance,
            &zerostackdepth,
            string_out,err);
        if (res == DW_DLV_ERROR) {
            dealloc_skip_branch_array(&op_branch_checking);
            return res;
        }
        if (branchdistance < 0) {
            op_loop_count++;
        }
        if (zerostackdepth) {
            /*  This is a DW_OP_piece kind of operation,
                so stack is conceptually separate per piece. */
            stackdepth = 0;
        } else {
            stackdepth = stackdepth + stackchange;
            if (stackdepth > maxstackdepth) {
                maxstackdepth = stackdepth;
            }
        }
    }
    if (glflags.verbose &&
        !glflags.dense &&
        ((maxstackdepth > 1) ||  op_loop_count)) {
        int indentprespaces = standard_indent();
        int indentpostspaces = 6;
        esb_append(string_out,"\n");
        append_indent_prefix(string_out,indentprespaces,
            die_indent_level,indentpostspaces);

        esb_append_printf_u(string_out,
            "DW_OPs= %u ",no_of_ops);
        esb_append_printf_i(string_out,
            "maxstackdepth= %d",maxstackdepth);
        esb_append_printf_i(string_out,
            " endingstackdepth= %d",stackdepth);
        if (op_loop_count) {
            /*  When DW_OP_bra/skip has us loop back
                to an earlier DW_OPs. */
            esb_append_printf_u(string_out,
                " loopcount= %u",op_loop_count);
        }
        esb_append(string_out,"\n");
        append_indent_prefix(string_out,indentprespaces,
            die_indent_level,indentpostspaces);
    }
    if (has_skip_or_branch) {
        check_skip_branch_offsets(&op_branch_checking,string_out);
    }
    dealloc_skip_branch_array(&op_branch_checking);
    return DW_DLV_OK;
}

static int
op_has_no_operands(Dwarf_Small op)
{
    return (_dwarf_opscounttab[op].oc_opcount == 0);
}

/*  Sometimes a DW_OP_implicit_const value
    ar a FORM_block*
    is actually a string from gcc.
    A crude heuristic.
    We are looking for ascii here hoping
    it is somewhat useful..
    It will be sanitized before being
    printed.
*/
static Dwarf_Bool
looks_like_string(unsigned long length,const unsigned char *bp)
{
    size_t str_len = 0;
    if (!bp || !length) {
        return FALSE;
    }
    if (bp[length-1]) {
        return FALSE;
    }
    if (length < 2) {
        /*  single zero byte: is silly to print empty string */
        return FALSE;
    }
    if (length > 5000) {
        /* arbitrary max as string */
        return FALSE;
    }
    /*  Ugly cast.... Sorry. */
    str_len = strlen((const char *)bp);
    if (str_len != (size_t)(length - 1)) {
        /*  Extra NUL bytes, not a string. */
        return FALSE;
    }
#ifdef HAVE_UTF8
    {
    int res = 0;

    res = dd_utf8_checkCodePoints((unsigned char *)bp);
    if (res == DW_DLV_ERROR) {
        return FALSE;
    }
    }
#else /* !HAVE_UTF8 */
    {
    const unsigned char *end = 0;

    end = bp+length-1;
    for ( ; bp < end; ++bp) {
        /*  A crude test */
        unsigned char c = *bp;
        if ( c < 0x08 || c > 0x7e) {
            return FALSE;
        }
    }
    }
#endif /* HAVE_UTF8 */
    return TRUE;
}

static void
show_contents(struct esb_s *string_out,
    unsigned long length,const unsigned char * bp)
{
    unsigned int i = 0;

    if (!length) {
        return;
    }
    esb_append(string_out," contents 0x");
    for (; i < length; ++i,++bp) {
        /*  Do not use DW_PR_DUx here,
            the value  *bp is a const unsigned char. */
        esb_append_printf_u(string_out,"%02x", *bp);
    }
}
static void
emit_op_indentation(struct esb_s *string_out,
    int die_indent_level,
    Dwarf_Unsigned index)
{
    if (!glflags.dense && !glflags.gf_expr_ops_joined) {
        int indentprespaces = 0;
        int indentpostspaces = 0;

        indentprespaces = standard_indent();
        indentpostspaces = 6;
        esb_append(string_out,"\n");
        append_indent_prefix(string_out,indentprespaces,
            die_indent_level,indentpostspaces);
    } else if (index > 0) {
        esb_append(string_out, " ");
    }
}

/*  When an expression op has another expression inside
    this does the printing.
    Not limited to a single level of nesting.
*/
static int
print_expression_inner_block(Dwarf_Debug dbg,
    Dwarf_Die       die,
    int             die_indent_level,
    Dwarf_Block    *block,
    struct esb_s   *string_out,
    Dwarf_Error    *error)
{
    int res = 0;
    Dwarf_Loc_Head_c header = 0;
    Dwarf_Unsigned   listlen = 0;
    Dwarf_Half       dw_version = 0;
    Dwarf_Bool       dw_is_info = 0;
    Dwarf_Bool       dw_is_dwo = 0;
    Dwarf_Half       dw_offset_size = 0;
    Dwarf_Half       dw_address_size = 0;
    Dwarf_Half       dw_extension_size = 0;
    Dwarf_Sig8      *dw_signature = 0;
    Dwarf_Off        dw_offset_of_length = 0;
    Dwarf_Unsigned   dw_total_byte_length = 0;
    Dwarf_Small     lle_value = 0;
    Dwarf_Unsigned  rawlowpc = 0;
    Dwarf_Unsigned  rawhipc = 0;
    Dwarf_Bool      debug_addr_unavailable = TRUE;
    Dwarf_Unsigned  lowpc_cooked = 0;
    Dwarf_Unsigned  hipc_cooked = 0;
    Dwarf_Unsigned  locexpr_op_count=0;
    Dwarf_Locdesc_c locentry = 0;
    Dwarf_Small     loclist_source = 0;
    Dwarf_Unsigned  expression_offset = 0;
    Dwarf_Unsigned  locdesc_offset =0;

    res = dwarf_cu_header_basics(die, &dw_version,&dw_is_info,
        &dw_is_dwo,&dw_offset_size,&dw_address_size,
        &dw_extension_size,&dw_signature,
        &dw_offset_of_length,&dw_total_byte_length,
        error);
    if (res != DW_DLV_OK) {
        return res;
    }
    res = dwarf_loclist_from_expr_c(dbg,
        block->bl_data,block->bl_len,
        dw_address_size,dw_offset_size,
        dw_version,
        &header,&listlen,error);
    if (res != DW_DLV_OK) {
        return res;
    }
    res = dwarf_get_locdesc_entry_d(header,
        0, /* Data from 0th LocDesc, the only entry */
        &lle_value,
        &rawlowpc, &rawhipc,
        &debug_addr_unavailable,
        &lowpc_cooked, &hipc_cooked,
        &locexpr_op_count,
        &locentry,
        &loclist_source,
        &expression_offset,
        &locdesc_offset,
        error);
    if (res == DW_DLV_ERROR) {
        dwarf_dealloc_loc_head_c(header);
        glflags.gf_count_major_errors++;
        printf("\nERROR: calling dwarf_get_locdesc_entry_d()"
            " on LocDesc 0");
        return res;
    } else if (res == DW_DLV_NO_ENTRY) {
        dwarf_dealloc_loc_head_c(header);
        return res;
    }
    /*  ASSERT: loclist_source == DW_LKIND_expression  */
    /*  ASSERT: lle_value == DW_LLE_start_end  */
    res = dwarfdump_print_expression_operations(dbg,
        die,
        die_indent_level,
        locentry,
        locexpr_op_count,
        string_out,error);
    dwarf_dealloc_loc_head_c(header);
    return res;
}

int
_dwarf_print_one_expr_op(Dwarf_Debug dbg,
    Dwarf_Die   die,
    int         die_indent_level,
    Dwarf_Locdesc_c exprc,
    Dwarf_Unsigned  index,
    Dwarf_Bool      has_skip_or_branch,
    struct OpBranchHead_s *oparray,
    int   *stackchange,
    Dwarf_Signed *branchdistance,
    int   * zerostackdepth,
    struct esb_s *string_out,
    Dwarf_Error *err)
{
    Dwarf_Small op = 0;
    Dwarf_Unsigned opd1 = 0;
    Dwarf_Unsigned opd2 = 0;
    Dwarf_Unsigned opd3 = 0;
    Dwarf_Unsigned offsetforbranch = 0;
    const char * op_name = 0;
    int indentprespaces = 0;
    int indentpostspaces = 0;
    Dwarf_Bool showblockoffsets = FALSE;
    struct OpBranchEntry_s *echecking = 0;

    emit_op_indentation(string_out,die_indent_level,index);
    if (!glflags.dense && !glflags.gf_expr_ops_joined) {
        indentprespaces = standard_indent();
        indentpostspaces = 6;
    }
    {
        /* DWARF 2,3,4 and DWARF5 style */
        int res = 0;
        res = dwarf_get_location_op_value_c(exprc,
            index,
            &op,&opd1,&opd2,&opd3,
            &offsetforbranch,
            err);
        if (res != DW_DLV_OK) {
            print_error_and_continue(
                "dwarf_get_location_op_value_c "
                "did not get a value!",
                res,*err);
            return res;
        }
    }
    op_name = get_OP_name(op);
    if (has_skip_or_branch &&
        glflags.verbose) {
        showblockoffsets = TRUE;
    }
    if (showblockoffsets ) {
        /*  New January 2021, showing offsets relevant to
            DW_OP_bra and DW_OP_skip .
            offsetforbranch comes from a 16 bit field, so
            is never large.
            */
        esb_append_printf_u(string_out,
            "<blkoff 0x%04" DW_PR_DUx "> ",
            offsetforbranch);
    }
    if (oparray && oparray->bh_opcount &&
        (Dwarf_Half)index <  oparray->bh_opcount) {
        echecking = oparray->bh_ops_array+ index;
        echecking->op = op;
        echecking->offset =  offsetforbranch;
    }
    esb_append(string_out, op_name);
    *stackchange =  _dwarf_opscounttab[op].oc_stackchange;
    if (op_has_no_operands(op)) {
        /* Nothing to add. */
    } else if (op >= DW_OP_breg0 && op <= DW_OP_breg31) {
        esb_append_printf_i(string_out,
            "%+" DW_PR_DSd , opd1);
    } else {
        switch (op) {
        case DW_OP_addr:
            bracket_hex(" ",opd1,"",string_out);
            break;
        case DW_OP_const1s:
        case DW_OP_const2s:
        case DW_OP_const4s:
        case DW_OP_const8s:
        case DW_OP_consts:
        case DW_OP_fbreg:
            esb_append(string_out," ");
            formx_signed(opd1,string_out);
            break;
        case DW_OP_skip:
        case DW_OP_bra: {
            Dwarf_Signed as_signed = (Dwarf_Signed)opd1;
            esb_append(string_out," ");
            formx_signed(as_signed,string_out);
            *branchdistance = as_signed;
            if (showblockoffsets) {
                /*  offsetforbranch is the op offset,
                    and we need the the value 1 past
                    the bytes in the DW_OP */
                Dwarf_Signed off  = offsetforbranch +2+1;

                off = off + as_signed;
                if (off < 0 ) {
                    esb_append_printf_i(string_out,
                        " <ERROR. branch/skip target erroneous %d>",
                        off);
                    glflags.gf_count_major_errors++;
                } else {
                    esb_append_printf_u(string_out,
                        " <target op: 0x%04" DW_PR_DUx ">",
                        (Dwarf_Unsigned)off);
                }
                if (echecking) {
                    echecking->target_offset = off;
                }
            }
            }
            break;
        case DW_OP_GNU_addr_index: /* unsigned val */
        case DW_OP_addrx:  /* DWARF5: unsigned val */
        case DW_OP_GNU_const_index:
        case DW_OP_constx: /* DWARF5: unsigned val */
        case DW_OP_const1u:
        case DW_OP_const2u:
        case DW_OP_const4u:
        case DW_OP_const8u:
        case DW_OP_constu:
        case DW_OP_pick:
        case DW_OP_plus_uconst:
        case DW_OP_regx:
        case DW_OP_piece:
        case DW_OP_deref_size:
        case DW_OP_xderef_size:
            if (op == DW_OP_piece) {
                *zerostackdepth = TRUE;
            }
            esb_append_printf_u(string_out,
                " %" DW_PR_DUu , opd1);
#if 0 /* add later? */
            Turn on later
            if (opd1 > 9) {
                esb_append_printf_u(string_out,
                    " (0x%" DW_PR_XZEROS DW_PR_DUx ")",
                    opd1);
            }
#endif
            break;
        case DW_OP_bregx:
            bracket_hex(" ",opd1,"",string_out);
            esb_append(string_out,"+");
            formx_signed(opd2,string_out);
            break;
        case DW_OP_call2: {
            bracket_hex(" ",opd1,"",string_out);
            check_die_expr_op_basic_data(dbg,die,op_name,
                indentprespaces,die_indent_level,indentpostspaces,
                NO_SPECIFIC_TAG,NON_ZERO_OFFSET_REQUIRED,
                WITHIN_CU,opd1,string_out);
            }
            break;
        case DW_OP_call4: {
            bracket_hex(" ",opd1,"",string_out);
            check_die_expr_op_basic_data(dbg,die,op_name,
                indentprespaces,die_indent_level,indentpostspaces,
                NO_SPECIFIC_TAG,NON_ZERO_OFFSET_REQUIRED,
                WITHIN_CU,opd1,string_out);
            }
            break;
        case DW_OP_call_ref: {
            bracket_hex(" ",opd1,"",string_out);
            check_die_expr_op_basic_data(dbg,die,op_name,
                indentprespaces,die_indent_level,indentpostspaces,
                NO_SPECIFIC_TAG,NON_ZERO_OFFSET_REQUIRED,
                WITHIN_CU,opd1,string_out);
            }
            break;
        case DW_OP_bit_piece:
            *zerostackdepth = TRUE;
            bracket_hex(" ",opd1,"",string_out);
            bracket_hex(" offset ",opd2,"",string_out);
            break;
        case DW_OP_implicit_value:
            {
                unsigned long print_len = (unsigned long)opd1;
                Dwarf_Block inr;
                int inres = 0;

                bracket_hex(" ",opd1,"",string_out);
                /*  The other operand is a block of opd1 bytes. */
                {
                    const unsigned char *bp = 0;
                    /*  This is a really ugly cast, a way
                        to implement DW_OP_implicit value in
                        this libdwarf context. */
                    bp = (const unsigned char *)(uintptr_t) opd2;
                    show_contents(string_out,print_len,bp);
                    if (looks_like_string(print_len,bp)) {
                        emit_op_indentation(string_out,
                            die_indent_level,index);
                        esb_append_printf_s(string_out,
                            "contents='%s'",(const char *)bp);
                    } else  {
                        inr.bl_len = opd1;
                        inr.bl_data = (Dwarf_Ptr)(uintptr_t)opd2;
                        inr.bl_from_loclist = DW_LKIND_expression;
                        inr.bl_section_offset = 0;
                        inres = print_expression_inner_block(dbg,die,
                            die_indent_level+1,&inr,
                            string_out,
                            err);
                        if (inres == DW_DLV_ERROR) {
                            DROP_ERROR_INSTANCE(dbg,inres,*err);
                        }
                    }
                }
            }
            break;

        /* We do not know what the operands, if any, are. */
        case DW_OP_HP_unknown:
        case DW_OP_HP_is_value:
        case DW_OP_HP_fltconst4:
        case DW_OP_HP_fltconst8:
        case DW_OP_HP_mod_range:
        case DW_OP_HP_unmod_range:
        case DW_OP_HP_tls:
        case DW_OP_INTEL_bit_piece:
            break;
        case DW_OP_stack_value:  /* DWARF4 */
            break;
        case DW_OP_GNU_uninit:  /* DW_OP_APPLE_uninit */
            /* No operands. */
            break;
        case DW_OP_GNU_encoded_addr:
            bracket_hex(" ",opd1,"",string_out);
            break;
        case DW_OP_GNU_variable_value: {
            bracket_hex(" ",opd1,"",string_out);
            check_die_expr_op_basic_data(dbg,die,op_name,
                indentprespaces,die_indent_level,indentpostspaces,
                NO_SPECIFIC_TAG,NON_ZERO_OFFSET_REQUIRED,
                WITHIN_CU,opd1,string_out);
            }
            break;
        case DW_OP_implicit_pointer:       /* DWARF5 */
        case DW_OP_GNU_implicit_pointer: {
            /*  opd1 is a section offset, not a CU offset.
                We don't know if DW_OP_GNU_implicit_pointer
                allows a zero offset meaning 'generic type'
                but GNU C++ 4.9.x-google 20150123 (prerelease)
                generates zero in DWARF4.
                DWARF5 does not allow zero.  */
            bracket_hex(" ",opd1,"",string_out);
            esb_append(string_out, " ");
            formx_signed(opd2,string_out);
            check_die_expr_op_basic_data(dbg,die,op_name,
                indentprespaces,die_indent_level,indentpostspaces,
                NO_SPECIFIC_TAG,
                op= DW_OP_implicit_pointer?
                    ZERO_OFFSET_GENERIC_TYPE:
                    NON_ZERO_OFFSET_REQUIRED,
                !WITHIN_CU,opd1,string_out);
            }
            break;
        case DW_OP_entry_value:       /* DWARF5 */
        case DW_OP_GNU_entry_value: {
            const unsigned char *bp = 0;
            unsigned long length = (unsigned long)opd1;
            Dwarf_Block inr;
            int inres = 0;

            bracket_hex(" ",opd1,"",string_out);
            bp = (Dwarf_Small *)(uintptr_t) opd2;
            if (!bp) {
                esb_append(string_out,
                    "ERROR: Null databyte pointer "
                    "DW_OP_entry_value ");
            } else {
                show_contents(string_out,length,bp);
                if (looks_like_string(length,bp)) {
                    emit_op_indentation(string_out,
                        die_indent_level,index);
                    esb_append_printf_s(string_out,
                        "contents='%s'",(const char *)bp);
                } else {
                    inr.bl_len = opd1;
                    inr.bl_data = (Dwarf_Ptr)(uintptr_t)opd2;
                    inr.bl_from_loclist = DW_LKIND_expression;
                    inr.bl_section_offset = 0;
                    inres = print_expression_inner_block(dbg,die,
                        die_indent_level+1,&inr,
                        string_out,
                        err);
                    if (inres == DW_DLV_ERROR) {
                        DROP_ERROR_INSTANCE(dbg,inres,*err);
                    }
                }
            }
            }
            break;
        case DW_OP_const_type:           /* DWARF5 */
        case DW_OP_GNU_const_type:
            {
            const unsigned char *bp = 0;
            Dwarf_Unsigned length = 0;
            /*  opd1 is cu-relative offset of type DIE.
                we have a die in the relevant CU in the arg
                list */

            esb_append_printf_u(string_out," 0x%"
                DW_PR_DUx,opd1);
            length = opd2;
            esb_append(string_out," const length: ");
            esb_append_printf_u(string_out,
                "%u" , length);
            /* Now point to the data bytes of the const. */
            bp = (Dwarf_Small *)(uintptr_t)opd3;
            if (!bp) {
                esb_append(string_out,
                    "ERROR: Null databyte pointer DW_OP_const_type ");
            } else {
                show_contents(string_out,(unsigned long)length,bp);
                if (looks_like_string((unsigned long)length,bp)) {
                    emit_op_indentation(string_out,
                        die_indent_level,index);
                    esb_append_printf_s(string_out,
                        "contents='%s'",(const char *)bp);
                }
            }
            esb_append(string_out,"\n");
#if 0 /* add later? */
            append_indent_prefix(string_out,indentprespaces,
                die_indent_level,indentpostspaces+2);
            esb_append(string_out," Target Die: ");
            show_target_die_offset_name(dbg,die,opd1,
                string_out, err);
#endif

            check_die_expr_op_basic_data(dbg,die,op_name,
                indentprespaces,die_indent_level,indentpostspaces,
                DW_TAG_base_type, NON_ZERO_OFFSET_REQUIRED,
                WITHIN_CU,opd1,string_out);
            }
            break;
        case DW_OP_regval_type:           /* DWARF5 */
        case DW_OP_GNU_regval_type: {
            esb_append_printf_u(string_out,
                " 0x%" DW_PR_DUx , opd1);
            bracket_hex(" ",opd2,"",string_out);
            check_die_expr_op_basic_data(dbg,die,op_name,
                indentprespaces,die_indent_level,indentpostspaces,
                DW_TAG_base_type,NON_ZERO_OFFSET_REQUIRED,
                WITHIN_CU,opd2,string_out);
            }
            break;
        case DW_OP_xderef_type: /* DWARF5 */
        case DW_OP_deref_type: /* DWARF5 */
        case DW_OP_GNU_deref_type: {
            esb_append_printf_u(string_out,
                " 0x%02" DW_PR_DUx , opd1);
            bracket_hex(" ",opd2,"",string_out);
            check_die_expr_op_basic_data(dbg,die,op_name,
                indentprespaces,die_indent_level,indentpostspaces,
                DW_TAG_base_type,NON_ZERO_OFFSET_REQUIRED,
                WITHIN_CU,opd2,string_out);
            }
            break;
        case DW_OP_convert: /* DWARF5 */
        case DW_OP_GNU_convert:
        case DW_OP_reinterpret: /* DWARF5 */
        case DW_OP_GNU_reinterpret:
        /*  For following case, unsure if non-zero opd2
            is required or not. Assume not */
        case DW_OP_GNU_parameter_ref:  {
            esb_append_printf_u(string_out,
                " 0x%02"  DW_PR_DUx , opd1);
            check_die_expr_op_basic_data(dbg,die,op_name,
                indentprespaces,die_indent_level,indentpostspaces,
                DW_TAG_base_type,ZERO_OFFSET_GENERIC_TYPE,
                WITHIN_CU,opd2,string_out);
            }
            break;
        default:
            {
                esb_append_printf_u(string_out,
                    " DWARF DW_OP_ unknown 0x%x", (unsigned)op);
            }
            break;
        }
    }
    return DW_DLV_OK;
}

void
loc_error_check(
    const char *     tagname,
    const char *     attrname,
    Dwarf_Addr lopcfinal,
    Dwarf_Addr hipcfinal,
    Dwarf_Unsigned offset,
    Dwarf_Addr base_address,
    Dwarf_Bool *bError)
{
    DWARF_CHECK_COUNT(locations_result,1);

    /*  Check the low_pc and high_pc are within
        a valid range in the .text section */
    if (glflags.nTrace[KIND_RANGES_INFO]) {
        PrintBucketGroup("Location ranges check PD lec",
            glflags.pRangesInfo);
    }
    if (IsValidInBucketGroup(glflags.pRangesInfo,lopcfinal) &&
        IsValidInBucketGroup(glflags.pRangesInfo,hipcfinal)) {
        /* Valid values; do nothing */
    } else {
        /*  At this point may be dealing with
            a linkonce symbol */
        if (IsValidInLinkonce(glflags.pLinkonceInfo,
            glflags.PU_name,
            lopcfinal,hipcfinal)) {
            /* Valid values; do nothing */
        } else {
            struct esb_s m;

            esb_constructor(&m);
            *bError = TRUE;
            esb_append_printf_s(&m,
                ".debug_loc[lists]: Address outside a "
                "valid .text range: TAG %s",tagname);
            esb_append_printf_s(&m," with attribute %s.",
                attrname);
            esb_append_printf_u(&m," final lowpc 0x%08x ",lopcfinal);
            esb_append_printf_u(&m," final hipc 0x%08x",hipcfinal);
            esb_append_printf_u(&m," locoffset 0x%08x",offset);
            esb_append_printf_u(&m," baseaddress 0x%08x",
                base_address);
            DWARF_CHECK_ERROR(locations_result,
                esb_get_string(&m));
            esb_destructor(&m);
        }
    }
}

/*  See  also print_loclists.c print_offset_entry_table() */
static void
print_ll_offsets_table(Dwarf_Debug dbg,
    Dwarf_Unsigned goff,
    Dwarf_Unsigned contextnum,
    Dwarf_Unsigned offset_table_offset,
    Dwarf_Unsigned offset_entry_count,
    Dwarf_Unsigned offset_size,
    struct esb_s  *esbp,
    Dwarf_Error *error)
{
    Dwarf_Unsigned e = 0;
    Dwarf_Unsigned colmax = 2;
    Dwarf_Unsigned col = 0;
    Dwarf_Unsigned loff = offset_table_offset;
    int            hasnewline = FALSE;
    int            res = 0;

    goff = goff + loff;
    for ( ; e < offset_entry_count; ++e) {
        Dwarf_Unsigned value = 0;

        if (e == 0) {
            esb_append_printf_s(esbp,"%s","\n");
            esb_append_printf_u(esbp,
                "   Location Offset Table at 0x%" DW_PR_XZEROS
                DW_PR_DUx  "\n",loff);
            esb_append_printf_u(esbp,
                "   (Added 0x%x to value for actual offsets)\n",
                loff);
            esb_append_printf_s(esbp,"%s","   [goff][loff][index]\n");
        }
        hasnewline = FALSE;
        res = dwarf_get_loclist_offset_index_value(dbg,
            contextnum,e,&value,0,error);
        if (res != DW_DLV_OK) {
            if (res == DW_DLV_ERROR) {
                char *msg = dwarf_errmsg(*error);
                esb_append_printf_s(esbp,
                    "ERROR  Offset table value not available %s\n",
                    msg);
                dwarf_dealloc_error(dbg,*error);
                *error = 0;
            } else {
                esb_append(esbp,
                    "Offset table value not available%s\n");
            }
        }
        if (col == 0) {
            esb_append_printf_u(esbp,
                "   [0x%" DW_PR_XZEROS DW_PR_DUx "]",
                goff);
            esb_append_printf_u(esbp,
                "[0x%" DW_PR_XZEROS DW_PR_DUx "]",
                loff);
            esb_append_printf_u(esbp,"[%2" DW_PR_DUu "]",e);

        }
        esb_append_printf_u(esbp,
            " 0x%" DW_PR_XZEROS DW_PR_DUx, value);
        esb_append_printf_u(esbp,
            "(0x%" DW_PR_XZEROS DW_PR_DUx ")",
            value+offset_table_offset);
        loff += offset_size;
        goff += offset_size;
        col++;
        if (col == colmax) {
            esb_append_printf_s(esbp,"%s","\n");
            hasnewline = TRUE;
            col = 0;
        }
    }
    if (!hasnewline) {
        esb_append_printf_s(esbp,"%s","\n");
    }
}

static void
print_loclists_context_head(Dwarf_Debug dbg,
    Dwarf_Small    lkind,
    Dwarf_Unsigned lle_count,
    Dwarf_Unsigned lle_version,
    Dwarf_Unsigned loclists_index,
    Dwarf_Unsigned bytes_total_in_lle,
    Dwarf_Half     offset_size,
    Dwarf_Half     address_size,
    Dwarf_Half     segment_selector_size,
    Dwarf_Unsigned overall_offset_of_this_context,
    Dwarf_Unsigned total_length_of_this_context,
    Dwarf_Unsigned offset_table_offset,
    Dwarf_Unsigned offset_table_entrycount,
    Dwarf_Bool     loclists_base_present,
    Dwarf_Addr     loclists_base,
    Dwarf_Bool     loclists_base_address_present,
    Dwarf_Addr     loclists_base_address,
    Dwarf_Bool     loclists_debug_addr_base_present,
    Dwarf_Addr     loclists_debug_addr_base,
    Dwarf_Unsigned loclists_offset_lle_set,
    struct esb_s  *esbp,
    Dwarf_Error *error)
{
    Dwarf_Unsigned goff = overall_offset_of_this_context;
    append_local_prefix(esbp);
    esb_append_printf_u(esbp,
        "bytes total this loclist: %3u",bytes_total_in_lle);
    append_local_prefix(esbp);
    esb_append_printf_u(esbp,
        "number of entries       : %3u", lle_count);
    append_local_prefix(esbp);
    esb_append_printf_u(esbp,
        "context number          : %3u",loclists_index);
    append_local_prefix(esbp);
    esb_append_printf_u(esbp,
        "version                 : %3u",lle_version);
    append_local_prefix(esbp);
    esb_append_printf_u(esbp,
        "address size            : %3u",address_size);
    append_local_prefix(esbp);
    esb_append_printf_u(esbp,
        "offset size             : %3u",offset_size);
    if (segment_selector_size) {
        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "segment selector size   : %3u",
            segment_selector_size);
    }
    if (lkind == DW_LKIND_loclists) {
        append_local_prefix(esbp);
            /*  FIXME  rename as
                context global offset
                once all else stabilized. */
            esb_append_printf_u(esbp,
            "offset of context       : 0x%"
            DW_PR_XZEROS DW_PR_DUx,
            overall_offset_of_this_context);
        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "offset table entrycount : %3u",
            offset_table_entrycount);
        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "offset table offset     : 0x%"
            DW_PR_XZEROS DW_PR_DUx,
            offset_table_offset);
        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "offset of this list set : 0x%"
            DW_PR_XZEROS DW_PR_DUx,
            loclists_offset_lle_set);
        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "length of context       : %3u",
            total_length_of_this_context);
        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "end of context offset   : 0x%"
            DW_PR_XZEROS DW_PR_DUx,
            overall_offset_of_this_context +
            total_length_of_this_context);
        if (offset_table_entrycount) {
            esb_append_printf_s(esbp,"%s\n","");
            append_local_prefix(esbp);
            print_ll_offsets_table(dbg,goff,
                loclists_index, offset_table_offset,
                offset_table_entrycount,offset_size,
                esbp, error);
        }
    }

    if (loclists_base_present) {
        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "loclists base offset    : 0x%"
            DW_PR_XZEROS DW_PR_DUx,
            loclists_base);
    }
    if (loclists_base_address_present) {
        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "DW_AT_low_pc(base addr) : 0x%"
            DW_PR_XZEROS DW_PR_DUx,
            loclists_base_address);
    }
    if (loclists_debug_addr_base_present) {
        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "DW_AT_addr_base         : 0x%"
            DW_PR_XZEROS DW_PR_DUx,
            loclists_debug_addr_base);
    }
    esb_append(esbp,"\n");
}

/*  Fill buffer with location lists data for printing.
    This does the details.
    It's up to the caller to determine which
    esb to put the result in. */
/*ARGSUSED*/
static int
print_location_list(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Attribute attr,
    Dwarf_Bool checking,
    int die_indent_level,
    int  no_end_newline,
    struct esb_s *details,
    Dwarf_Error* llerr)
{
    Dwarf_Unsigned no_of_elements = 0;
    Dwarf_Loc_Head_c loclist_head = 0; /* For DWARF2-DWARF5 */
    int            lres = 0;
    unsigned int   llent = 0;

    /*  Base address used to update entries in .debug_loc.
        CU_base_address is a global. Terrible way to
        pass in this value. FIXME. See also CU_low_address
        as base address is special for address ranges */
    Dwarf_Addr     base_address = glflags.CU_base_address;
    Dwarf_Addr     lopc = 0;
    Dwarf_Addr     hipc = 0;
    Dwarf_Bool     bError = FALSE;
    Dwarf_Small    lle_value = 0; /* DWARF5 */
    Dwarf_Unsigned lle_count = 0;
    Dwarf_Unsigned loclists_index = 0;

    /*  This is the section offset of the expression, not
        the location description prefix. */
    Dwarf_Unsigned expr_section_offset = 0;
    Dwarf_Half    address_size = 0;
    Dwarf_Half    segment_selector_size = 0;
    Dwarf_Addr     max_address = 0;
    Dwarf_Unsigned lle_version = 2;
    Dwarf_Half     version = 2;
    Dwarf_Half     offset_size = 4;
    Dwarf_Small    lkind = DW_LKIND_unknown;
    Dwarf_Unsigned bytes_total_in_lle = 0;
    Dwarf_Unsigned overall_offset_of_this_context = 0;
    Dwarf_Unsigned total_length_of_this_context = 0;
    Dwarf_Bool     loclists_base_present = FALSE;
    Dwarf_Unsigned loclists_base = 0;
    Dwarf_Bool     loclists_base_address_present = FALSE;
    Dwarf_Unsigned loclists_base_address = 0;
    Dwarf_Bool     loclists_debug_addr_base_present = FALSE;
    Dwarf_Unsigned loclists_debug_addr_base = 0;
    Dwarf_Unsigned loclists_offset_lle_set = 0;
    Dwarf_Unsigned offset_table_offset = 0;
    Dwarf_Unsigned offset_table_entrycount = 0;
    struct esb_s   section_truename;
    Dwarf_Unsigned goff = 0;
    Dwarf_Unsigned loff = 0;

    lres = dwarf_get_version_of_die(die,&version,
        &offset_size);
    if (lres != DW_DLV_OK) {
        /*  Is DW_DLV_ERROR (see libdwarf query.c),
            there is no error argument. */
        simple_err_only_return_action(lres,
            "\nERROR: die or context bad calling "
            "dwarf_get_version_of_die in print_location_list."
            " Something is very wrong.");
        return DW_DLV_NO_ENTRY;
    }

    lres = get_address_size_and_max(dbg,&address_size,
        &max_address,llerr);
    if (lres != DW_DLV_OK) {
        print_error_and_continue("Getting address size"
            " and maximum failed while getting location list",
            lres,*llerr);
        return lres;
    }
    {
        /* Preferred interface. Deals with DWARF2,3,4,5. */
        lres = dwarf_get_loclist_c(attr,&loclist_head,
            &no_of_elements,llerr);
        if (lres == DW_DLV_ERROR) {
            print_error_and_continue(
                "ERROR: dwarf_get_loclist_c fails",
                lres, *llerr);
            return lres;
        } else if (lres == DW_DLV_NO_ENTRY) {
            return lres;
        }
        lres = dwarf_get_loclist_head_basics(loclist_head,
            &lkind,
            &lle_count,&lle_version, &loclists_index,
            &bytes_total_in_lle,
            &offset_size, &address_size, &segment_selector_size,
            &overall_offset_of_this_context,
            &total_length_of_this_context,
            &offset_table_offset,
            &offset_table_entrycount,
            &loclists_base_present,&loclists_base,
            &loclists_base_address_present,&loclists_base_address,
            &loclists_debug_addr_base_present,
            &loclists_debug_addr_base,
            &loclists_offset_lle_set,llerr);
        if (lres != DW_DLV_OK) {
            dwarf_dealloc_loc_head_c(loclist_head);
            return lres;
        }
        goff = overall_offset_of_this_context;
        {   /* This is a small consistency/function check. */
            unsigned int check_lkind = DW_LKIND_unknown;
            int          lresx = 0;

            lresx = dwarf_get_loclist_head_kind(loclist_head,
                &check_lkind,llerr);
            if (lresx == DW_DLV_OK) {
                if (check_lkind != lkind) {
                    printf("ERROR: dwarf_get_loclist_head_kind "
                        " returned a bogus value! 0x%x vs 0x%x\n",
                        lkind,check_lkind);
                    glflags.gf_count_major_errors++;
                }
            } else {
                /*  The only possible return is DW_DLV_ERROR,
                    and means a NULL loclist_head!.
                    That will not happen here. */
                printf("ERROR: dwarf_get_loclist_head_kind "
                    " returned a  DW_DLV_ERROR!");
                glflags.gf_count_major_errors++;
                DROP_ERROR_INSTANCE(dbg,lresx,*llerr);
            }
        }
        version = (Dwarf_Half)lle_version;
        /*  append_local_prefix(esbp); No,
            here the newline grates, causes blank
            line in the output. So. Just add 6 spaces.
            the output already has a newline. */
        if (lkind != DW_LKIND_expression) {
            esb_append(details,"      ");
            esb_constructor(&section_truename);
            if (lkind == DW_LKIND_loclists) {
                get_true_section_name(dbg,".debug_loclists",
                &section_truename,FALSE);
            } else {
                get_true_section_name(dbg,".debug_loc",
                &section_truename,FALSE);
            }
            esb_append_printf_s(details,"%-15s",
                esb_get_string(&section_truename));
            esb_append_printf_u(details,
                " offset  :"
                " 0x%" DW_PR_XZEROS DW_PR_DUx,
                loclists_offset_lle_set);
            esb_destructor(&section_truename);
            if (glflags.verbose) {
                print_loclists_context_head(dbg,lkind,
                    lle_count, lle_version, loclists_index,
                    bytes_total_in_lle,
                    offset_size,address_size, segment_selector_size,
                    overall_offset_of_this_context,
                    total_length_of_this_context,
                    offset_table_offset, offset_table_entrycount,
                    loclists_base_present,loclists_base,
                    loclists_base_address_present,
                    loclists_base_address,
                    loclists_debug_addr_base_present,
                    loclists_debug_addr_base,
                    loclists_offset_lle_set,
                    details,llerr);
            } else {
                /* things look better with this...  no -v */
                esb_append(details,"\n   ");
            }
        }
    }
    possibly_increase_esb_alloc(details, no_of_elements,100);

    if (lkind == DW_LKIND_loclists) {
        loff = loclists_offset_lle_set;
        goff = overall_offset_of_this_context+loff;
    }
    for (llent = 0; llent < no_of_elements; ++llent) {
        Dwarf_Unsigned locdesc_offset = 0;
        Dwarf_Locdesc_c locentry = 0; /* 2015 */
        Dwarf_Unsigned rawlowpc = 0;
        Dwarf_Unsigned rawhipc = 0;
        Dwarf_Unsigned ulocentry_count = 0;
        Dwarf_Unsigned lle_byte_count = 0;
        Dwarf_Bool     debug_addr_unavailable = FALSE;
        /*  This has values DW_LKIND*, the same values
            that were in loclist source
            in 2019, but with
            the new value of DW_LKIND_loclists
            for DWARF5.  See libdwarf.h */
        Dwarf_Small loclist_source = 0;

        {
            lres = dwarf_get_locdesc_entry_e(loclist_head,
                llent,
                &lle_value,
                &rawlowpc,&rawhipc,
                &debug_addr_unavailable,
                &lopc, &hipc,
                &ulocentry_count,
                &lle_byte_count,
                &locentry,
                &loclist_source,
                &expr_section_offset,
                &locdesc_offset,
                llerr);
            if (lres == DW_DLV_ERROR) {
                print_error_and_continue(
                    "ERROR: dwarf_get_locdesc_entry_c fails",
                    lres, *llerr);
                dwarf_dealloc_loc_head_c(loclist_head);
                return lres;
            } else if (lres == DW_DLV_NO_ENTRY) {
                dwarf_dealloc_loc_head_c(loclist_head);
                return lres;
            }
        }
        if (loclist_source == DW_LKIND_expression &&
            lle_value != DW_LLE_start_end) {
            printf("ERROR: With DW_LKIND_expression"
                " lle_value should be DW_LLE_start_end (7) "
                "not %u\n",lle_value);
            glflags.gf_count_major_errors++;
        }
        /*  If we have a location list refering to the .debug_loc
            Check for specific compiler we are validating. */
        if ( glflags.gf_check_locations && glflags.in_valid_code &&
            loclist_source && checking_this_compiler()) {
            checking = TRUE;
        }
        if (!glflags.dense &&
            loclist_source != DW_LKIND_expression) {
            Dwarf_Bool need_newline = TRUE;

            if (llent == 0) {
                /* These messages go with the list of entries */
                switch(loclist_source){
                case DW_LKIND_loclist:
                    esb_append_printf_u(details,
                        "   <loclist at offset 0x%"
                        DW_PR_XZEROS DW_PR_DUx,
                        loclists_offset_lle_set);
                    esb_append_printf_i(details,
                        " with %ld entries follows>",
                        no_of_elements);
                    break;
                case DW_LKIND_GNU_exp_list:
                    esb_append_printf_u(details,
                        "   <dwo loclist at offset 0x%"
                        DW_PR_XZEROS DW_PR_DUx,
                        loclists_offset_lle_set);
                    esb_append_printf_i(details,
                        " with %ld entries follows>",
                        no_of_elements);
                    break;
                case DW_LKIND_loclists:
                    esb_append_printf_u(details,
                        "   <debug_loclists offset 0x%"
                        DW_PR_XZEROS DW_PR_DUx,
                        loclists_offset_lle_set);
                    esb_append_printf_i(details,
                        " with %ld entries follows>",
                        no_of_elements);
                    goff = loclists_offset_lle_set;
                    esb_append_printf_u(details, "\n   [0x%"
                        DW_PR_XZEROS
                        DW_PR_DUx "]",goff);
                    esb_append_printf_u(details, "[0x%"
                        DW_PR_XZEROS
                        DW_PR_DUx "]",loff);
                    need_newline = FALSE;
                    break;
                default: break;
                }
            }
            if (need_newline) {
                esb_append_printf_i(details, "\n   [%2d]",llent);
            } else {
                esb_append_printf_i(details, "[%2d]",llent);
            }
            loff += lle_byte_count;
            goff += lle_byte_count;
        }
        /*  We use DW_LLE names for DW_LKIND_loclist and
            DW_LKIND_loclists. We use LLEX names for
            DW_LKIND_GNU_exp_list */
        if (loclist_source) {
            Dwarf_Half tag = 0;
            Dwarf_Half attrnum = 0;
            const char *tagname = 0;
            const char *attrname = 0;
            int res = 0;

            res = dwarf_tag(die,&tag,llerr);
            if (res != DW_DLV_OK) {
                dwarf_dealloc_loc_head_c(loclist_head);
                return res;
            }
            res = dwarf_whatattr(attr,&attrnum,llerr);
            if (res != DW_DLV_OK) {
                dwarf_dealloc_loc_head_c(loclist_head);
                return res;
            }
            attrname = get_AT_name(attrnum);
            tagname = get_TAG_name(tag);
            if (loclist_source == DW_LKIND_GNU_exp_list) {
                print_llex_linecodes(checking,
                    tagname,
                    attrname,
                    llent,
                    lle_value,
                    base_address,
                    rawlowpc, rawhipc,
                    debug_addr_unavailable,
                    lopc, hipc,
                    locdesc_offset,
                    details,
                    &bError);
            } else if (loclist_source == DW_LKIND_loclist) {
                print_original_loclist_linecodes(checking,
                    tagname,
                    attrname,
                    llent,
                    lle_value,
                    base_address,
                    rawlowpc, rawhipc,
                    debug_addr_unavailable,
                    lopc, hipc,
                    locdesc_offset,
                    details,
                    &bError);
            } else {
                /* loclist_source == DW_LKIND_loclists */
                print_debug_loclists_linecodes(checking,
                    tagname,
                    attrname,
                    llent,
                    lle_value,
                    lle_byte_count,
                    base_address,
                    rawlowpc, rawhipc,
                    debug_addr_unavailable,
                    lopc, hipc,
                    locdesc_offset,
                    details,
                    &bError);
            }
        }
        lres = dwarfdump_print_expression_operations(dbg,
            die,
            die_indent_level,
            /*  Either llbuf or locentry non-zero.
                Not both. */
            locentry,
            ulocentry_count, /* How many ops in this loc desc */
            details,llerr);
        if (lres == DW_DLV_ERROR) {
            dwarf_dealloc_loc_head_c(loclist_head);
            return lres;
        }
    }
    if (!no_end_newline) {
        if (bError &&  glflags.gf_check_verbose_mode &&
            PRINTING_UNIQUE) {
            esb_append(details,"\n");
        } else if (glflags.gf_do_print_dwarf) {
            esb_append(details,"\n");
        }
    }
    dwarf_dealloc_loc_head_c(loclist_head);
    return DW_DLV_OK;
}

/*  New October 2017.
    The 'decimal' representation here is questionable.
    */
static void
formx_data16(Dwarf_Form_Data16 * u,
    struct esb_s *esbp, Dwarf_Bool hex_format)
{
    unsigned i = 0;

    for ( ; i < sizeof(Dwarf_Form_Data16); ++i){
        esb_append(esbp, "0x");
        if (hex_format) {
            esb_append_printf_u(esbp,
                "%02x ", u->fd_data[i]);
        } else {
            esb_append_printf_i(esbp, "%02d ", u->fd_data[i]);
        }
    }
}

static void
formx_unsigned(Dwarf_Unsigned u,
    struct esb_s *esbp,
    Dwarf_Bool hex_format)
{
    if (hex_format) {
        esb_append_printf_u(esbp,
            "0x%"  DW_PR_XZEROS DW_PR_DUx , u);
    } else {
        esb_append_printf_u(esbp,
            "%" DW_PR_DUu , u);
    }
}

static void
formx_signed(Dwarf_Signed s, struct esb_s *esbp)
{
    esb_append_printf_i(esbp, "%" DW_PR_DSd ,s);
}
static void
formx_unsigned_and_signed_if_neg(Dwarf_Unsigned tempud,
    Dwarf_Signed tempd,
    const char *leader,Dwarf_Bool hex_format,struct esb_s*esbp)
{
    formx_unsigned(tempud,esbp,hex_format);
    if (tempd < 0) {
        esb_append(esbp,leader);
        formx_signed(tempd,esbp);
        esb_append(esbp,")");
    }
}

/*  If the DIE DW_AT_type exists and is directly known signed/unsigned
    return -1 for signed 1 for unsigned.
    Otherwise return 0 meaning 'no information'.
    So we only need to a messy lookup once per type-die offset  */
static int
check_for_type_unsigned(Dwarf_Debug dbg,
    Dwarf_Die die)
{
    Dwarf_Bool is_info = 0;
    struct Helpertree_Base_s * helperbase = 0;
    struct Helpertree_Map_Entry_s *e = 0;
    int res = 0;
    Dwarf_Attribute attr = 0;
    Dwarf_Attribute encodingattr = 0;
    Dwarf_Error error = 0;
    Dwarf_Unsigned diegoffset = 0;
    Dwarf_Unsigned typedieoffset = 0;
    Dwarf_Die typedie = 0;
    Dwarf_Unsigned tempud = 0;
    int show_form_here = FALSE;
    int retval = 0;

    if (!die) {
        return 0;
    }
    is_info = dwarf_get_die_infotypes_flag(die);
    if (is_info) {
        helperbase = &helpertree_offsets_base_info;
    } else {
        helperbase = &helpertree_offsets_base_types;
    }
    res = dwarf_dieoffset(die,&diegoffset,&error);
    if (res == DW_DLV_ERROR) {
        dwarf_dealloc_error(dbg,error);
        return 0;
    } else if (res == DW_DLV_NO_ENTRY) {
        /* We don't know sign. */
        return 0;
    }
    /*  This might be wrong. See the typedieoffset check below,
        which is correct... */
    e = helpertree_find(diegoffset,helperbase);
    if (e) {
        return e->hm_val;
    }

    /*  We look up the DW_AT_type die, if any, and
        use that offset to check for signedness. */

    res = dwarf_attr(die, DW_AT_type, &attr,&error);
    if (res == DW_DLV_ERROR) {
        dwarf_dealloc_error(dbg,error);
        helpertree_add_entry(diegoffset, 0,helperbase);
        return 0;
    }
    if (res == DW_DLV_NO_ENTRY) {
        /* We don't know sign. */
        helpertree_add_entry(diegoffset, 0,helperbase);
        return 0;
    }
    res = dwarf_global_formref_b(attr, &typedieoffset,
        &is_info,&error);
    if (res == DW_DLV_ERROR) {
        dwarf_dealloc_attribute(attr);
        helpertree_add_entry(diegoffset, 0,helperbase);
        return 0;
    }
    if (res == DW_DLV_NO_ENTRY) {
        dwarf_dealloc_attribute(attr);
        helpertree_add_entry(diegoffset, 0,helperbase);
        return 0;
    }
    dwarf_dealloc_attribute(attr);
    attr = 0;
    e = helpertree_find(typedieoffset,helperbase);
    if (e) {
        return e->hm_val;
    }

    res = dwarf_offdie_b(dbg,typedieoffset,is_info, &typedie,&error);
    if (res == DW_DLV_ERROR) {
        dwarf_dealloc_error(dbg,error);
        helpertree_add_entry(diegoffset, 0,helperbase);
        helpertree_add_entry(typedieoffset, 0,helperbase);
        return 0;
    } else if (res == DW_DLV_NO_ENTRY) {
        helpertree_add_entry(diegoffset, 0,helperbase);
        helpertree_add_entry(typedieoffset, 0,helperbase);
        return 0;
    }
    res = dwarf_attr(typedie, DW_AT_encoding, &encodingattr,&error);
    if (res == DW_DLV_ERROR) {
        dwarf_dealloc_error(dbg,error);
        dwarf_dealloc_die(typedie);
        helpertree_add_entry(diegoffset, 0,helperbase);
        helpertree_add_entry(typedieoffset, 0,helperbase);
        return 0;
    }
    if (res == DW_DLV_NO_ENTRY) {
        dwarf_dealloc_die(typedie);
        helpertree_add_entry(diegoffset, 0,helperbase);
        helpertree_add_entry(typedieoffset, 0,helperbase);
        return 0;
    }
    res = dd_get_integer_and_name(dbg,
        encodingattr,
        &tempud,
        /* attrname */ (const char *) NULL,
        /* err_string */ ( struct esb_s *) NULL,
        (encoding_type_func) 0,
        &error,show_form_here);

    if (res != DW_DLV_OK) {
        DROP_ERROR_INSTANCE(dbg,res,error);
        /* dealloc attr *first* then die */
        dwarf_dealloc_attribute(encodingattr);
        dwarf_dealloc_die(typedie);
        helpertree_add_entry(diegoffset, 0,helperbase);
        helpertree_add_entry(typedieoffset, 0,helperbase);
        return 0;
    }
    if (tempud == DW_ATE_signed || tempud == DW_ATE_signed_char) {
        /*esb_append(esbp,"helper small encoding SIGNED ");*/
        retval = -1;
    } else {
        if (tempud == DW_ATE_unsigned ||
            tempud == DW_ATE_unsigned_char) {
                retval = 1;
        }
    }
    helpertree_add_entry(diegoffset,retval,helperbase);
    helpertree_add_entry(typedieoffset, retval,helperbase);
    /* dealloc attr *first* then die */
    dwarf_dealloc_attribute(encodingattr);
    dwarf_dealloc_die(typedie);
    return retval;
}

/*  We think this is an integer. Figure out how to print it.
    In case the signedness is ambiguous (such as on
    DW_FORM_data1 (ie, unknown signedness) print two ways.

    If we were to look at DW_AT_type in the base DIE
    we could follow it and determine if the type
    was unsigned or signed (usually easily) and
    use that information.
*/
static int
formxdata_print_value(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Attribute attrib,
    Dwarf_Half theform,
    struct esb_s *esbp,
    Dwarf_Error * pverr, Dwarf_Bool hex_format)
{
    Dwarf_Signed tempsd = 0;
    Dwarf_Unsigned tempud = 0;
    int sres = 0;
    int ures = 0;

    if (theform == DW_FORM_data16) {
        Dwarf_Form_Data16 v16;
        ures = dwarf_formdata16(attrib,
            &v16,pverr);
        if (ures == DW_DLV_OK) {
            formx_data16(&v16,
                esbp,hex_format);
            return DW_DLV_OK;
        } else if (ures == DW_DLV_NO_ENTRY) {
            /* impossible */
            return ures;
        } else {
            return ures;
        }
    }
    ures = dwarf_formudata(attrib, &tempud, pverr);
    if (ures == DW_DLV_OK) {
        sres = dwarf_formsdata(attrib, &tempsd, pverr);
        if (sres == DW_DLV_OK) {
            if (tempud == (Dwarf_Unsigned)tempsd && tempsd >= 0) {
                /*  Data is the same value and not negative,
                    so makes no difference which
                    we print. */
                formx_unsigned(tempud,esbp,hex_format);
                return DW_DLV_OK;
            } else {
                /*  Here we don't know if signed or not and
                    Assuming one or the other changes the
                    interpretation of the bits. */
                int helpertree_unsigned = 0;

                helpertree_unsigned =
                    check_for_type_unsigned(dbg,die);
                if (!die || !helpertree_unsigned) {
                    /* Signedness unclear. */
                    formx_unsigned_and_signed_if_neg(tempud,tempsd,
                        " (",hex_format,esbp);
                    return DW_DLV_OK;
                } else if (helpertree_unsigned > 0) {
                    formx_unsigned(tempud,esbp,hex_format);
                    return DW_DLV_OK;
                } else {
                    /* Value signed. */
                    formx_signed(tempsd,esbp);
                    return DW_DLV_OK;
                }
            }
        } else if (sres == DW_DLV_NO_ENTRY) {
            formx_unsigned(tempud,esbp,hex_format);
            return DW_DLV_OK;
        } else /* DW_DLV_ERROR */{
            formx_unsigned(tempud,esbp,hex_format);
            DROP_ERROR_INSTANCE(dbg,sres,*pverr);
            return DW_DLV_OK;
        }
    } else if (ures == DW_DLV_ERROR) {
        DROP_ERROR_INSTANCE(dbg,ures,*pverr);
        sres = dwarf_formsdata(attrib, &tempsd, pverr);
        if (sres == DW_DLV_OK) {
            formx_signed(tempsd,esbp);
            return sres;
        } else if (sres == DW_DLV_ERROR) {
            esb_append_printf_u(esbp,
                "<ERROR: form 0x%x ",theform);
            esb_append(esbp,get_FORM_name(theform));
            esb_append(esbp,
                " not readable signed or unsigned>");
            simple_err_only_return_action(sres,
                esb_get_string(esbp));
            /* Neither worked. */
            return DW_DLV_ERROR;
        }
    }
    /* NO_ENTRY is crazy, impossible. */
    return DW_DLV_NO_ENTRY;
}

static void
bracket_hex(const char *s1,
    Dwarf_Unsigned v,
    const char *s2,
    struct esb_s * esbp)
{
    Dwarf_Bool hex_format = TRUE;
    esb_append(esbp,s1);
    formx_unsigned(v,esbp,hex_format);
    esb_append(esbp,s2);
}

/*  Borrow the definition from pro_encode_nm.h */
/*  Bytes needed to encode a number.
    Not a tight bound, just a reasonable bound.
*/
#ifndef ENCODE_SPACE_NEEDED
#define ENCODE_SPACE_NEEDED   (2*sizeof(Dwarf_Unsigned))
#endif /* ENCODE_SPACE_NEEDED */

/*  Table indexed by the attribute value;
    only standard attributes
    are included, ie. in the range [1..DW_AT_lo_user];
    we waste a
    little bit of space, but accessing the table is fast. */

static void
check_decl_file_only(char **srcfiles,
    Dwarf_Unsigned fileindex,Dwarf_Signed srcfiles_cnt,
    Dwarf_Half dwversion,int attr)
{
    /*  Zero is always a legal index for
        DWARF2,3,4 and it means no source name provided.
        For DWARF 5
        we've determined that the standard is wrong
        and zero meaning none just does not work.
        File numbers must start from zero.
        See the dwarfstd.org wiki */

    DWARF_CHECK_COUNT(decl_file_result,1);
    if (dwversion >= DWVERSION5) {
        if (fileindex >= ((Dwarf_Unsigned)srcfiles_cnt)) {
            struct esb_s msgb;

            esb_constructor(&msgb);
            if (!srcfiles) {
                esb_append(&msgb,
                    "There is a file number=");
                esb_append_printf_u(&msgb,"%" DW_PR_DUu,fileindex);
                esb_append(&msgb," but no source files");
                /*  Extra space char here to avoid pointless
                    regression test issues with older versions. */
                esb_append(&msgb,"  are known.");
            } else {
                esb_append(&msgb,"Does not index to valid DWARF5"
                    " file name ");
                esb_append(&msgb,"filenum=");
                esb_append_printf_u(&msgb,"%" DW_PR_DUu,fileindex);
                esb_append(&msgb," arraysize=");
                esb_append_printf_i(&msgb,"%" DW_PR_DSd,srcfiles_cnt);
                esb_append(&msgb,".");
            }
            DWARF_CHECK_ERROR2(decl_file_result,
                get_AT_name(attr),
                esb_get_string(&msgb));
            esb_destructor(&msgb);
        }
        return;
    }

    /* zero means no file applicable */
    if (fileindex  &&
        fileindex > ((Dwarf_Unsigned)srcfiles_cnt)) {
        struct esb_s msgb;

        esb_constructor(&msgb);
        if (!srcfiles) {
            esb_append(&msgb,
                "There is a file number=");
            esb_append_printf_u(&msgb,"%" DW_PR_DUu,fileindex);
            esb_append(&msgb," but no source files");
            /*  Extra space char here to avoid pointless
                regression test issues with older versions. */
            esb_append(&msgb,"  are known.");
        } else {
            esb_append(&msgb,"Does not index to valid file name ");
            esb_append(&msgb,"filenum=");
            esb_append_printf_u(&msgb,"%" DW_PR_DUu,fileindex);
            esb_append(&msgb," arraysize=");
            esb_append_printf_i(&msgb,"%" DW_PR_DSd,srcfiles_cnt);
            esb_append(&msgb,".");
        }
        DWARF_CHECK_ERROR2(decl_file_result,
            get_AT_name(attr),
            esb_get_string(&msgb));
        esb_destructor(&msgb);
    }
}

static int
expand_rnglist_entries(
    Dwarf_Rnglists_Head rnglhead,
    Dwarf_Unsigned rnglentriescount,
    Dwarf_Unsigned rnglglobal_offset,
    struct esb_s *  esbp,
    int local_verbose,
    Dwarf_Error *err)
{
    Dwarf_Unsigned count = 0;
    Dwarf_Unsigned i = 0;
    int res = 0;
    Dwarf_Unsigned secoffset = rnglglobal_offset;
    Dwarf_Bool no_debug_addr_available = FALSE;

    count = rnglentriescount;
    if (local_verbose > 1) {
        esb_append(esbp,"\n      "
            "                                                 "
            "secoff");
    }
    for ( ; i < count; ++i) {
        unsigned entrylen = 0;
        unsigned rle_code = 0;
        Dwarf_Unsigned raw1 = 0;
        Dwarf_Unsigned raw2 = 0;
        Dwarf_Unsigned cooked1 = 0;
        Dwarf_Unsigned cooked2 = 0;

        res  = dwarf_get_rnglists_entry_fields_a(rnglhead,i,
            &entrylen,&rle_code,
            &raw1,&raw2,
            &no_debug_addr_available,
            &cooked1,&cooked2,
            err);
        if (res != DW_DLV_OK) {
            return res;
        }
        if (local_verbose || no_debug_addr_available ) {
            const char *codename = "<unknown code>";

            dwarf_get_RLE_name(rle_code,&codename);
            esb_append(esbp,"\n      ");
            esb_append_printf_u(esbp,"[%2u] ",i);
            esb_append_printf_s(esbp,"%-20s ",codename);
            if (rle_code != DW_RLE_end_of_list) {
                esb_append_printf_u(esbp," 0x%"
                    DW_PR_XZEROS DW_PR_DUx ,raw1);
                    esb_append_printf_u(esbp," 0x%"
                    DW_PR_XZEROS DW_PR_DUx ,raw2);
            } else {
                esb_append(esbp,
                    "                      ");
            }
            if (local_verbose > 1) {
                esb_append_printf_u(esbp," 0x%"
                    DW_PR_XZEROS DW_PR_DUx ,secoffset);
            }
            if (no_debug_addr_available) {
                esb_append(esbp," no .debug_addr available");
            }
        }
        if (!no_debug_addr_available) {
            const char *codename = "start,end";

            esb_append(esbp,"\n      ");
            esb_append_printf_u(esbp,"[%2u] ",i);
            if (rle_code == DW_RLE_base_addressx ||
                rle_code == DW_RLE_base_address) {
                codename = "base address";
            } else {
                if (rle_code == DW_RLE_end_of_list) {
                    codename = "end of list";
                }
            }
            esb_append_printf_s(esbp,"%-20s ",codename);
            if (rle_code != DW_RLE_end_of_list) {
                esb_append_printf_u(esbp," 0x%"
                    DW_PR_XZEROS DW_PR_DUx ,cooked1);
                esb_append_printf_u(esbp," 0x%"
                    DW_PR_XZEROS DW_PR_DUx ,cooked2);
            } else {
                esb_append(esbp,
                "                      ");
            }
            if (local_verbose > 1) {
                esb_append_printf_u(esbp," 0x%"
                    DW_PR_XZEROS DW_PR_DUx ,secoffset);
            }
        }
        secoffset += entrylen;
    }
    esb_append(esbp,"\n");
    return DW_DLV_OK;
}

/* DWARF5 .debug_rnglists[.dwo] only. */
static int
handle_rnglists( Dwarf_Attribute attrib,
    Dwarf_Half theform,
    Dwarf_Unsigned attrval,
    Dwarf_Unsigned *output_rle_set_offset,
    struct esb_s *  esbp,
    int local_verbose,
    Dwarf_Error *err)
{
    /* This is DWARF5, by definition. Not earlier. */
    Dwarf_Unsigned global_offset_of_rle_set = 0;
    Dwarf_Unsigned count_rnglists_entries = 0;
    Dwarf_Rnglists_Head rnglhead = 0;
    int res = 0;
    res = dwarf_rnglists_get_rle_head(attrib,
        theform,
        attrval, /* index val or offset depending on form */
        &rnglhead,
        &count_rnglists_entries,
        &global_offset_of_rle_set,
        err);
    if (res != DW_DLV_OK) {
        return res;
    }
    *output_rle_set_offset = global_offset_of_rle_set;
    /*  Here we put newline at start of each line
        of output, different from the usual practice */
    append_local_prefix(esbp);
    esb_append_printf_u(esbp,
        "Offset of rnglists entries: 0x%"
        DW_PR_XZEROS DW_PR_DUx,
        global_offset_of_rle_set);
    if (local_verbose > 1) {
        Dwarf_Unsigned version = 0;
        Dwarf_Unsigned context_index = 0;
        Dwarf_Unsigned rle_count = 0;
        Dwarf_Unsigned total_bytes_in_rle = 0;
        Dwarf_Half     loffset_size = 0;
        Dwarf_Half     laddress_size = 0;
        Dwarf_Half     lsegment_selector_size = 0;
        Dwarf_Unsigned section_offset_of_context = 0;
        Dwarf_Unsigned length_of_context = 0;
        Dwarf_Bool rnglists_base_present = 0;
        Dwarf_Unsigned rnglists_base = 0;
        Dwarf_Bool rnglists_base_address_present = 0;
        Dwarf_Unsigned rnglists_base_address = 0;
        Dwarf_Bool debug_addr_base_present = 0;
        Dwarf_Unsigned debug_addr_base;
        Dwarf_Unsigned offsets_table_offset = 0;
        Dwarf_Unsigned offsets_table_entrycount = 0;

        res = dwarf_get_rnglist_head_basics(rnglhead,
            &rle_count,
            &version,
            &context_index,
            &total_bytes_in_rle,
            &loffset_size,
            &laddress_size,
            &lsegment_selector_size,
            &section_offset_of_context,
            &length_of_context,
            &offsets_table_offset,
            &offsets_table_entrycount,
            &rnglists_base_present,&rnglists_base,
            &rnglists_base_address_present,&rnglists_base_address,
            &debug_addr_base_present,&debug_addr_base,
            err);
        if (res != DW_DLV_OK) {
            dwarf_dealloc_rnglists_head(rnglhead);
            return res;
        }
        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "Index of rnglist head     : %u",
            context_index);

        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "rnglist head version      : %u",
            version);
        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "Record count rnglist set  : %u",
            rle_count);
        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "Bytes this rnglist set    : %u",
            total_bytes_in_rle);
        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "offset size               : %u",
            loffset_size);
        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "address size              : %u",
            laddress_size);
        if (offsets_table_entrycount) {
            append_local_prefix(esbp);
            esb_append_printf_u(esbp,
                "offsets count             : %u",
                offsets_table_entrycount);
            append_local_prefix(esbp);
            esb_append_printf_u(esbp,
                "offsets table offset      : 0x%x",
                offsets_table_offset);
        }

        if (rnglists_base_present) {
            append_local_prefix(esbp);
            esb_append_printf_u(esbp,
                "rnglists base offset      : 0x%"

                DW_PR_XZEROS DW_PR_DUx,
                rnglists_base);
        }
        if (rnglists_base_address_present) {
            append_local_prefix(esbp);
            esb_append_printf_u(esbp,
                "CU DW_AT_low_pc (baseaddr): 0x%"
                DW_PR_XZEROS DW_PR_DUx,
                rnglists_base_address);
        }
        if (debug_addr_base_present) {
            append_local_prefix(esbp);
            esb_append_printf_u(esbp,
                "CU DW_AT_addr_base        : 0x%"
                DW_PR_XZEROS DW_PR_DUx,
                debug_addr_base);
        }
        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "section offset CU rnglists: 0x%"
                DW_PR_XZEROS DW_PR_DUx,
            section_offset_of_context);
        append_local_prefix(esbp);
        esb_append_printf_u(esbp,
            "section length CU rnglists: 0x%"
            DW_PR_XZEROS DW_PR_DUx,
            length_of_context);
        esb_append_printf_u(esbp, " (%u)",
            length_of_context);
    }
    res = expand_rnglist_entries(rnglhead,
        count_rnglists_entries,
        global_offset_of_rle_set,
        esbp,
        local_verbose,err);
    dwarf_dealloc_rnglists_head(rnglhead);
    return res;
}

/*  This detects a special case attr/form compiler
    mistake because we have one in the regression tests.
    A more general approach would be better. 5 February 2021 */
static void
check_sensible_addr_for_form(Dwarf_Debug dbg,
    Dwarf_Attribute attrib, Dwarf_Half theform)
{
    Dwarf_Error e = 0;
    Dwarf_Half attrnum = 0;
    int res = 0;

    res = dwarf_whatattr(attrib,&attrnum,&e);
    if (res == DW_DLV_OK) {
        switch(attrnum) {
        default: return;
        case DW_AT_stmt_list:
        case DW_AT_macros:
        case DW_AT_GNU_macros:
        {
            struct esb_s m;
            esb_constructor(&m);
            esb_append_printf_s(&m,
                "Attribute %s has form ",
                get_AT_name(attrnum));
            esb_append(&m,get_FORM_name(theform));
            esb_append(&m," which is improper DWARF. "
                "Attempting to continue.");
            glflags.gf_count_major_errors++;
            printf("ERROR / WARNING: %s\n",
                esb_get_string(&m));
            esb_destructor(&m);
        }
        }
        return;
    }
    /* Something crazy. Will be noted elsewhere. */
    if (res == DW_DLV_ERROR) {
        DROP_ERROR_INSTANCE(dbg,res,e);
    }
    return;
}

/*  Just report on error, do not pass any error on,
    it will be refound if appropriate. */
static void
check_indexing_function(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Unsigned index,
    Dwarf_Addr addr,
    struct esb_s *esbp)
{
    int cres = 0;
    Dwarf_Addr localaddr = 0;
    Dwarf_Error error = 0;

    if (!glflags.gf_check_attr_encoding) {
        return;
    }
    DWARF_CHECK_COUNT(check_functions_result,1);
    cres = dwarf_debug_addr_index_to_addr(die,
        index,&localaddr,&error);
    if (cres == DW_DLV_OK) {
        if (localaddr != addr) {
            struct esb_s m;

            esb_constructor(&m);
            esb_append_printf_u(&m,
                "(ERROR: dwarf_debug_addr_index_to_addr "
                "Returns bad value 0x%x"
                " vs expected ",
                localaddr);
            esb_append_printf_u(&m,
                "value 0x%x",addr);
            DWARF_CHECK_ERROR(check_functions_result,
                esb_get_string(&m));
            esb_append(esbp,esb_get_string(&m));
            esb_destructor(&m);
        }
        return;
    }
    if (cres == DW_DLV_ERROR) {
        DROP_ERROR_INSTANCE(dbg,cres,error);
        error = 0;
    }
    return;
}

static void
check_for_mips_fde(Dwarf_Debug dbg,
    Dwarf_Die die,
    struct esb_s*esbp)
{
    int lres = 0;
    Dwarf_Fde      fde = 0;
    Dwarf_Addr     low_pc = 0;
    Dwarf_Unsigned func_length = 0;
    Dwarf_Small   *fde_bytes = 0;
    Dwarf_Unsigned fde_byte_length = 0;
    Dwarf_Off      cie_offset = 0;
    Dwarf_Signed   cie_index = 0;
    Dwarf_Off      fde_offset = 0;

    /*  Not wanting a Dwarf_Error be allocated. */
    lres = dwarf_get_fde_for_die(dbg,die,&fde,0);
    if (lres != DW_DLV_OK) {
        esb_append(esbp,"<WARNING: Unable to find "
            "MIPS_fde for Die>");
        return;
    }
    lres = dwarf_get_fde_range(fde,&low_pc,&func_length,
        &fde_bytes,&fde_byte_length,
        &cie_offset,&cie_index,&fde_offset,0);
    dwarf_dealloc(dbg, fde, DW_DLA_FDE);
    if (lres != DW_DLV_OK) {
        fde = 0;
        esb_append(esbp,"<WARNING: Unable to "
            "extract fde address/range from its fde>");
        return;
    }
    esb_append_printf_u(esbp,"<FDE with lowpc 0x%"
        DW_PR_DUx, low_pc);
    if (glflags.verbose) {
        esb_append_printf_u(esbp," function length 0x%"
            DW_PR_DUu, func_length);
        esb_append_printf_u(esbp," fde offset 0x%"
            DW_PR_DUx, fde_offset);
        esb_append_printf_u(esbp," fde offset 0x%"
            DW_PR_DUx, fde_offset);
        esb_append_printf_u(esbp," cie index %"
            DW_PR_DUu, cie_index);
        esb_append_printf_u(esbp," cie offset %"
            DW_PR_DUx, cie_offset);
    }
    esb_append(esbp,">");
}

static int
suppress_block_as_string(Dwarf_Half attrnum)
{
    switch(attrnum) {
    case DW_AT_call_data_location:
    case DW_AT_call_data_value:
    case DW_AT_call_origin:
    case DW_AT_call_target:
    case DW_AT_call_target_clobbered:
    case DW_AT_call_value:
    case DW_AT_data_location:
    case DW_AT_data_member_location:
    case DW_AT_frame_base:
    case DW_AT_GNU_call_site_target:
    case DW_AT_GNU_call_site_value:
    case DW_AT_location:
    case DW_AT_return_addr:
    case DW_AT_segment:
    case DW_AT_string_length:
    case DW_AT_use_location:
    case DW_AT_vtable_elem_location:
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

/*  Fill buffer with attribute value.
    We pass in tag so we can try to do the right thing with
    broken compiler DW_TAG_enumerator

    'cnt' is signed for historical reasons (a mistake
    in an interface), but the value is never negative.

    We append to esbp's buffer.
*/
int
get_attr_value(Dwarf_Debug dbg, Dwarf_Half tag,
    Dwarf_Die die,
    Dwarf_Off dieprint_cu_goffset,
    Dwarf_Attribute attrib,
    char **srcfiles, Dwarf_Signed srcfiles_cnt,
    struct esb_s *esbp,
    int show_form,
    int local_verbose,
    Dwarf_Error *err)
{
    Dwarf_Half theform = 0;
    char * temps = 0;
    Dwarf_Block *tempb = 0;
    Dwarf_Signed tempsd = 0;
    Dwarf_Unsigned tempud = 0;
    Dwarf_Off off = 0;
    Dwarf_Die die_for_check = 0;
    Dwarf_Half tag_for_check = 0;
    Dwarf_Addr addr = 0;
    int fres = 0;
    int bres = 0;
    int wres = 0;
    int dres = 0;
    Dwarf_Half direct_form = 0;
    Dwarf_Bool is_info = 0;

    is_info = dwarf_get_die_infotypes_flag(die);
    /*  Dwarf_whatform gets the real form, DW_FORM_indir is
        never returned: instead the real form following
        DW_FORM_indir is returned. */
    fres = dwarf_whatform(attrib, &theform, err);
    /*  Depending on the form and the attribute, process the form. */
    if (fres == DW_DLV_ERROR) {
        print_error_and_continue(
            "dwarf_whatform cannot Find Attr Form",
            fres, *err);
        return fres;
    }
    if (fres == DW_DLV_NO_ENTRY) {
        return fres;
    }
    /*  dwarf_whatform_direct gets the 'direct' form, so if
        the form is DW_FORM_indir that is what is returned. */
    fres  = dwarf_whatform_direct(attrib, &direct_form, err);
    DROP_ERROR_INSTANCE(dbg,fres,*err);
    /*  Ignore errors in dwarf_whatform_direct() */

    switch (theform) {
    case DW_FORM_GNU_addr_index:
    case DW_FORM_addrx:
    case DW_FORM_addrx1    :  /* DWARF5 */
    case DW_FORM_addrx2    :  /* DWARF5 */
    case DW_FORM_addrx3    :  /* DWARF5 */
    case DW_FORM_addrx4    :  /* DWARF5 */
    case DW_FORM_addr:
        check_sensible_addr_for_form(dbg,attrib,theform);
        bres = dwarf_formaddr(attrib, &addr, err);
        if (bres == DW_DLV_OK) {
            if (dwarf_addr_form_is_indexed(theform)) {
                Dwarf_Unsigned index = 0;
                int res = dwarf_get_debug_addr_index
                    (attrib,&index,err);
                if (res != DW_DLV_OK) {
                    struct esb_s lstr;
                    esb_constructor(&lstr);
                    esb_append(&lstr,"ERROR: getting debug addr"
                        " index on form ");
                    esb_append(&lstr,get_FORM_name(theform));
                    esb_append(&lstr," missing index?!");
                    print_error_and_continue(
                        esb_get_string(&lstr),
                        res, *err);
                    esb_destructor(&lstr);
                    return res;
                }
                bracket_hex("(addr_index: ",index, ")",esbp);
                if (glflags.gf_do_check_dwarf) {
                    check_indexing_function(dbg,die,
                        index,addr,esbp);
                }
            }
            bracket_hex("",addr,"",esbp);
        } else if (bres == DW_DLV_ERROR) {
            if (DW_DLE_MISSING_NEEDED_DEBUG_ADDR_SECTION ==
                dwarf_errno(*err)) {
                Dwarf_Unsigned index = 0;
                int res = 0;

                DROP_ERROR_INSTANCE(dbg,bres,*err);
                glflags.gf_debug_addr_missing = 1;
                res = dwarf_get_debug_addr_index(attrib,&index,
                    err);
                if (res != DW_DLV_OK) {
                    struct esb_s lstr;
                    esb_constructor(&lstr);
                    esb_append(&lstr,get_FORM_name(theform));
                    esb_append(&lstr," missing index. ?!");
                    print_error_and_continue(
                        esb_get_string(&lstr),
                        res, *err);
                    esb_destructor(&lstr);
                    return res;
                }
                addr = 0;
                bracket_hex("(addr_index: ",index,
                    ")<no .debug_addr section>",esbp);
                /*  This is normal in a .dwo/DWP file. The .debug_addr
                    is in a .o and in the final executable. */
            } else {
                /* Some bad error. */
                struct esb_s lstr;

                esb_constructor(&lstr);
                esb_append(&lstr,get_FORM_name(theform));
                esb_append(&lstr," form with no addr ?!");
                print_error_and_continue(
                    esb_get_string(&lstr),
                    bres, *err);
                esb_destructor(&lstr);
                if (!glflags.gf_error_code_search_by_address) {
                    glflags.gf_error_code_search_by_address =
                        (int)dwarf_errno(*err);
                }
                return bres;
            }
        } else { /* DW_DLV_NO_ENTRY */
            struct esb_s lstr;

            esb_constructor(&lstr);
            esb_append(&lstr,get_FORM_name(theform));
            esb_append(&lstr," is a DW_DLV_NO_ENTRY? "
                "something is wrong.");
            print_error_and_continue(
                esb_get_string(&lstr),
                bres, *err);
            esb_destructor(&lstr);
            return bres;
        }
        break;
    case DW_FORM_ref_addr:
        {
        Dwarf_Half attr = 0;
        /*  DW_FORM_ref_addr is not accessed thru formref: ** it is an
            address (global section offset) in ** the .debug_info
            section.
            DWARF2 incorrectly specified the value here
            as being the size of an address, which never made any
            sense: it has always been an offset of a DIE somewhere in
            .debug_info .
            The definition was changed in DWARF3 to specify the value
            is an offset in .debug_info, not an address.  */
        bres = dwarf_global_formref_b(attrib, &off,&is_info,err);
        if (bres == DW_DLV_OK) {
            bracket_hex("<GOFF=",off, ">",esbp);
        } else {
            print_error_and_continue(
                "DW_FORM_ref_addr form with no reference?!"
                " Something is corrupted.",
                bres, *err);
            return bres;
        }
        wres = dwarf_whatattr(attrib, &attr, err);
        if (wres != DW_DLV_OK) {
            print_error_and_continue(
                "DW_FORM_ref_addr no attribute number?!"
                " Something is corrupted.",
                wres, *err);
            return wres;
        }
        if (attr == DW_AT_sibling) {
            /*  The value had better be inside the current CU
                else there is a nasty error here, as a sibling
                has to be in the same CU, it seems. */
            /*  The target offset (off) had better be
                following the die's global offset else
                we have a serious botch. this FORM
                defines the value as a .debug_info
                global offset. */
            Dwarf_Off cuoff = 0;
            Dwarf_Off culen = 0;
            Dwarf_Off die_overall_offset = 0;
            int res = 0;
            int ores = dwarf_dieoffset(die, &die_overall_offset,
                err);
            if (ores != DW_DLV_OK) {
                print_error_and_continue(
                    "dwarf_dieoffsetnot available for"
                    "DW_AT_sibling. Something is corrupted.",
                    ores, *err);
                return ores;
            }
            if (die_stack_indent_level >= DIE_STACK_SIZE ) {
                report_die_stack_error(dbg,err);
                return DW_DLV_ERROR;
            }
            SET_DIE_STACK_SIBLING(off);
            if (die_overall_offset >= off) {
                struct esb_s msg;

                esb_constructor(&msg);
                esb_append_printf_u(&msg,
                    "ERROR: Sibling DW_FORM_ref_offset 0x%"
                    DW_PR_XZEROS DW_PR_DUx ,
                    off);
                esb_append_printf_s(&msg,
                    " points %s die Global offset ",
                    (die_overall_offset == off)?"at":"before");
                esb_append_printf_u(&msg,
                    "0x%"  DW_PR_XZEROS  DW_PR_DUx,
                    die_overall_offset);
                simple_err_only_return_action(DW_DLV_ERROR,
                    esb_get_string(&msg));
                esb_destructor(&msg);
            }

            DWARF_CHECK_COUNT(tag_tree_result,1);
            res = dwarf_die_CU_offset_range(die,&cuoff,
                &culen,err);
            if (res != DW_DLV_OK) {
                DWARF_CHECK_ERROR(tag_tree_result,
                    "DW_AT_sibling DW_FORM_ref_addr "
                    "offset range of the CU "
                    "is not available");
                DROP_ERROR_INSTANCE(dbg,res,*err);
            } else {
                Dwarf_Off cuend = cuoff+culen;
                if (off <  cuoff || off >= cuend) {
                    DWARF_CHECK_ERROR(tag_tree_result,
                        "DW_AT_sibling DW_FORM_ref_addr "
                        "offset points "
                        "outside of current CU");
                }
            }
        } else if (attr == DW_AT_abstract_origin) {
            int lres = 0;
            Dwarf_Die targ_die_a = 0;
            int is_info_a = dwarf_get_die_infotypes_flag(die);

            DWARF_CHECK_COUNT(type_offset_result,1);
            lres = dwarf_offdie_b(dbg,off,is_info_a,
                &targ_die_a,err);
            if (lres == DW_DLV_OK) {
                dwarf_dealloc_die(targ_die_a);
            } else {
                DWARF_CHECK_ERROR(type_offset_result,
                    "DW_AT_abstract_origin DW_FORM_ref_addr "
                    "offset of the DIE "
                    "is not valid");
                if (lres == DW_DLV_ERROR) {
                    DROP_ERROR_INSTANCE(dbg,lres,*err);
                }
            }
        }
        }
        break;
    case DW_FORM_ref1:
    case DW_FORM_ref2:
    case DW_FORM_ref4:
    case DW_FORM_ref8:
    case DW_FORM_ref_udata:
        {
        int refres = 0;
        Dwarf_Half attr = 0;
        Dwarf_Off goff = 0; /* Global offset */

        refres = dwarf_whatattr(attrib, &attr, err);
        if (refres != DW_DLV_OK) {
            struct esb_s lstr;
            esb_constructor(&lstr);
            esb_append(&lstr,get_AT_name(attr));
            esb_append(&lstr,
                " is an attribute with no number? Impossible");
            print_error_and_continue(
                esb_get_string(&lstr),refres,*err);
            esb_destructor(&lstr);
            return refres;
        }
        /* CU-relative offset returned. */
        refres = dwarf_formref(attrib, &off,
            &is_info, err);
        if (refres != DW_DLV_OK) {
            struct esb_s msg;

            esb_constructor(&msg);
            /* Report incorrect offset */
            esb_append_printf_s(&msg,
                "reference form on attr %s has "
                "no valid cu_relative offset?! ",
                get_AT_name(attr));
            esb_append_printf_u(&msg,
                ", for offset=<0x%"  DW_PR_XZEROS  DW_PR_DUx ">",
                off);
            print_error_and_continue(
                esb_get_string(&msg),refres,*err);
            esb_destructor(&msg);
            return refres;
        }

        /*  Convert the local offset 'off' into a global section
            offset 'goff'. */
        refres = dwarf_convert_to_global_offset(attrib,
            off, &goff, err);
        if (refres != DW_DLV_OK) {
            /*  Report incorrect offset */
            struct esb_s msg;

            esb_constructor(&msg);
            esb_append_printf_u(&msg,
                "invalid offset"
                ",off=<0x%"  DW_PR_XZEROS  DW_PR_DUx "> ",
                off);
            esb_append(&msg,"attr: ");
            esb_append(&msg,get_AT_name(attr));
            esb_append(&msg,"local offset has no global offset! ");
            print_error_and_continue(
                esb_get_string(&msg), refres, *err);
            esb_destructor(&msg);
            return refres;
        }
        if (attr == DW_AT_sibling) {
            /*  The value had better be inside the current CU
                else there is a nasty error here, as a sibling
                has to be in the same CU, it seems.
                The target offset (off) had better be
                following the die's global offset else
                we have a serious botch. this FORM
                defines the value as a .debug_info
                global offset. */
            Dwarf_Off die_overall_offset = 0;
            int ores = dwarf_dieoffset(die, &die_overall_offset,err);
            if (ores != DW_DLV_OK) {
                struct esb_s lstr;
                esb_constructor(&lstr);
                esb_append(&lstr,"DW_AT_sibling attr and DIE has");
                esb_append(&lstr,"no die_offset?");
                print_error_and_continue(
                    esb_get_string(&lstr),
                    ores, *err);
                esb_destructor(&lstr);
                return ores;
            }
            SET_DIE_STACK_SIBLING(goff);
            if (die_overall_offset >= goff) {
                struct esb_s lstr;
                esb_constructor(&lstr);
                esb_append(&lstr,"ERROR in DW_AT_sibling: ");
                esb_append(&lstr,get_FORM_name(theform));
                esb_append_printf_u(&lstr,
                    " Sibling offset 0x%"  DW_PR_XZEROS  DW_PR_DUx
                    " points ",
                    goff);
                esb_append(&lstr,
                    (die_overall_offset == goff)?"at":"before");
                esb_append_printf_u(&lstr,
                    " its own die GOFF="
                    "0x%"  DW_PR_XZEROS  DW_PR_DUx,
                    die_overall_offset);
                simple_err_only_return_action(DW_DLV_ERROR,
                    esb_get_string(&lstr));
                esb_destructor(&lstr);
                /*  At present no way to create a Dwarf_Error
                    inside dwarfdump. */
            }
        }

        /*  Do references inside <> to distinguish them ** from
            constants. In dense form this results in <<>>. Ugly for
            dense form, but better than ambiguous. davea 9/94 */
        if (glflags.gf_show_global_offsets) {
            bracket_hex("<",off,"",esbp);
            bracket_hex(" GOFF=",goff,">",esbp);
        } else {
            bracket_hex("<",off,">",esbp);
        }

        if (glflags.gf_check_type_offset) {
            if (attr == DW_AT_type &&
                dd_form_refers_local_info(theform)) {
                dres = dwarf_offdie_b(dbg, goff,
                    is_info,
                    &die_for_check, err);
                if (dres != DW_DLV_OK) {
                    struct esb_s msgc;

                    esb_constructor(&msgc);
                    esb_append_printf_u(&msgc,
                        "DW_AT_type offset does not point to a DIE "
                        "for global offset 0x%"
                        DW_PR_XZEROS DW_PR_DUx, goff);
                    esb_append_printf_u(&msgc,
                        " cu off 0x%" DW_PR_XZEROS DW_PR_DUx,
                        dieprint_cu_goffset);
                    esb_append_printf_u(&msgc,
                        " local offset 0x%" DW_PR_XZEROS DW_PR_DUx,
                        off);
                    esb_append_printf_u(&msgc,
                        " tag 0x%x",tag);

                    DWARF_CHECK_ERROR(type_offset_result,
                        esb_get_string(&msgc));
                    DROP_ERROR_INSTANCE(dbg,dres,*err);
                    esb_destructor(&msgc);
                } else {
                    int tres2 =
                        dwarf_tag(die_for_check, &tag_for_check, err);
                    if (tres2 == DW_DLV_OK) {
                        switch (tag_for_check) {
                        case DW_TAG_array_type:
                        case DW_TAG_class_type:
                        case DW_TAG_enumeration_type:
                        case DW_TAG_pointer_type:
                        case DW_TAG_reference_type:
                        case DW_TAG_rvalue_reference_type:
                        case DW_TAG_restrict_type:
                        case DW_TAG_string_type:
                        case DW_TAG_structure_type:
                        case DW_TAG_subroutine_type:
                        case DW_TAG_typedef:
                        case DW_TAG_union_type:
                        case DW_TAG_ptr_to_member_type:
                        case DW_TAG_set_type:
                        case DW_TAG_subrange_type:
                        case DW_TAG_base_type:
                        case DW_TAG_const_type:
                        case DW_TAG_file_type:
                        case DW_TAG_packed_type:
                        case DW_TAG_thrown_type:
                        case DW_TAG_volatile_type:
                        case DW_TAG_template_type_parameter:
                        case DW_TAG_template_value_parameter:
                        case DW_TAG_unspecified_type:
                        /* Template alias */
                        case DW_TAG_template_alias:
                            /* OK */
                            break;
                        default:
                            {
                                struct esb_s msga;

                                esb_constructor(&msga);
                                esb_append_printf_u(&msga,
                                    "DW_AT_type offset "
                                    "0x%" DW_PR_XZEROS DW_PR_DUx,
                                    goff);
                                esb_append_printf_u(&msga,
                                    " does not point to Type"
                                    " info we got tag 0x%x ",
                                    tag_for_check);
                                esb_append(&msga,
                                    get_TAG_name(tag_for_check));
                                DWARF_CHECK_ERROR(type_offset_result,
                                    esb_get_string(&msga));
                                esb_destructor(&msga);
                            }
                            break;
                        }
                    } else {
                        DWARF_CHECK_ERROR(type_offset_result,
                            "DW_AT_type offset does not exist");
                        DROP_ERROR_INSTANCE(dbg,tres2,*err);
                    }
                }
            }
            dwarf_dealloc_die(die_for_check);
            die_for_check = 0;
        }
        }
        break;
    case DW_FORM_block:
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_exprloc:
        fres = dwarf_formblock(attrib, &tempb, err);
        if (fres == DW_DLV_OK) {
            unsigned u = 0;
            esb_append_printf_u(esbp, "len 0x%04x: ",
                tempb->bl_len);
            if (tempb->bl_len) {
                esb_append(esbp,"0x");
            }
            for (u = 0; u < tempb->bl_len; u++) {
                esb_append_printf_u(esbp,
                    "%02x",
                    *(u + (unsigned char *) tempb->bl_data));
            }
            if (tempb->bl_len) {
                Dwarf_Half attrblk = 0;
                int restf = FALSE;
                int atres = 0;

                esb_append(esbp,": ");
                atres = dwarf_whatattr(attrib, &attrblk, err);
                if (atres == DW_DLV_ERROR) {
                    /* Serious error. impossible? */
                    DROP_ERROR_INSTANCE(dbg,atres,*err);
                } else if (atres == DW_DLV_OK &&
                    !suppress_block_as_string(attrblk)) {
                    restf = looks_like_string(
                        (unsigned long)tempb->bl_len,
                        (unsigned char *)tempb->bl_data);
                    if (restf) {
                        esb_append(esbp,"Block As Quoted String: '");
                        esb_append(esbp,tempb->bl_data);
                        esb_append(esbp,"' ");
                    }
                }
            }
            dwarf_dealloc(dbg, tempb, DW_DLA_BLOCK);
            tempb = 0;
        } else {
            struct esb_s lstr;
            esb_constructor(&lstr);
            esb_append(&lstr,"Form form ");
            esb_append(&lstr,get_FORM_name(theform));
            esb_append(&lstr," cannot get block");
            print_error_and_continue(
                esb_get_string(&lstr),
                fres, *err);
            esb_destructor(&lstr);
            return fres;
        }
        break;
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_data8:
    case DW_FORM_data16:
        {
        Dwarf_Half attr = 0;
        fres = dwarf_whatattr(attrib, &attr, err);
        if (fres != DW_DLV_OK) {
            struct esb_s lstr;
            esb_constructor(&lstr);
            esb_append(&lstr,"Form ");
            esb_append(&lstr,get_FORM_name(theform));
            esb_append(&lstr," cannot get attribute");
            print_error_and_continue(
                esb_get_string(&lstr),
                fres, *err);
            esb_destructor(&lstr);
            return fres;
        } else {
            switch (attr) {
            case DW_AT_ordering:
            case DW_AT_byte_size:
            case DW_AT_bit_offset:
            case DW_AT_data_bit_offset:
            case DW_AT_bit_size:
            case DW_AT_inline:
            case DW_AT_language:
            case DW_AT_language_version:
            case DW_AT_visibility:
            case DW_AT_virtuality:
            case DW_AT_accessibility:
            case DW_AT_address_class:
            case DW_AT_calling_convention:
            case DW_AT_discr_list:      /* DWARF2 */
            case DW_AT_encoding:
            case DW_AT_identifier_case:
            case DW_AT_MIPS_loop_unroll_factor:
            case DW_AT_MIPS_software_pipeline_depth:
            case DW_AT_decl_column:
            case DW_AT_decl_file:
            case DW_AT_decl_line:
            case DW_AT_call_column:
            case DW_AT_call_file:
            case DW_AT_call_line:
            case DW_AT_start_scope:
            case DW_AT_byte_stride:
            case DW_AT_bit_stride:
            case DW_AT_count:
            case DW_AT_stmt_list:
            case DW_AT_MIPS_fde:
                {  int show_form_here = 0;
                wres = dd_get_integer_and_name(dbg,
                    attrib,
                    &tempud,
                    /* attrname */ (const char *) NULL,
                    /* err_string */ ( struct esb_s *) NULL,
                    (encoding_type_func) 0,
                    err,show_form_here);

                if (wres == DW_DLV_OK) {
                    Dwarf_Bool hex_format = TRUE;
                    Dwarf_Half dwversion = 0;
                    const char *version_name = 0;
                    const char *version_scheme = 0;

                    if (attr == DW_AT_language_version) {
                        hex_format = FALSE;
                        formx_unsigned(tempud,esbp,hex_format);
                        esb_append(esbp," (");
                        hex_format = TRUE;
                        formx_unsigned(tempud,esbp,hex_format);
                        esb_append(esbp,")");
                        wres = dwarf_lvn_name(die,&version_name,
                            &version_scheme);
                        if (wres == DW_DLV_OK) {
                            esb_append(esbp," (version: ");
                            if (version_name) {
                                esb_append(esbp,version_name);
                            }
                            esb_append(esbp,")");
                            esb_append(esbp," (version scheme: ");
                            if (version_scheme) {
                                esb_append(esbp,version_scheme);
                            }
                            esb_append(esbp,")");
                        }
                    } else {
                        formx_unsigned(tempud,esbp,hex_format);
                    }
                    /* Check attribute encoding */
                    if (glflags.gf_check_attr_encoding) {
                        check_attributes_encoding(attr,theform,
                            tempud);
                    }
                    if (attr == DW_AT_decl_file ||
                        attr == DW_AT_call_file) {
                        turn_file_num_to_string(dbg,die,
                            attrib,
                            theform,
                            dwversion,tempud,
                            srcfiles, srcfiles_cnt,esbp,err);
                        /*  Validate integrity of file indices
                            referenced in .debug_line */
                        if (glflags.gf_check_decl_file) {
                            check_decl_file_only(srcfiles,
                                tempud,srcfiles_cnt,
                                dwversion,attr);
                        }
                    } /* end decl_file and  call_file  processing */
                } else { /* not DW_DLV_OK on get small encoding */
                    struct esb_s lstr;
                    esb_constructor(&lstr);
                    esb_append(&lstr,"For form ");
                    esb_append(&lstr,get_FORM_name(theform));
                    esb_append(&lstr," and attribute ");
                    esb_append(&lstr,get_AT_name(attr));
                    esb_append(&lstr,
                        " Cannot get encoding attribute");
                    print_error_and_continue(
                        esb_get_string(&lstr),
                        wres, *err);
                    esb_destructor(&lstr);
                    return wres;
                }
                if (attr == DW_AT_MIPS_fde) {
                    check_for_mips_fde(dbg,die,esbp);
                }
                }
                break;
            case DW_AT_const_value: {
                /* Do not use hexadecimal format */
                Dwarf_Bool chex = FALSE;
                wres = formxdata_print_value(dbg,die,attrib,
                    theform,esbp, err, /* hex format?*/chex);
                if (wres == DW_DLV_OK){
                    /* String appended already. */
                } else if (wres == DW_DLV_NO_ENTRY) {
                    /* nothing? */
                } else {
                    struct esb_s lstr;
                    esb_constructor(&lstr);
                    esb_append(&lstr,"For form ");
                    esb_append(&lstr,get_FORM_name(theform));
                    esb_append(&lstr," and attribute ");
                    esb_append(&lstr,get_AT_name(attr));
                    esb_append(&lstr," Cannot get const value ");
                    print_error_and_continue(
                        esb_get_string(&lstr),
                        wres, *err);
                    esb_destructor(&lstr);
                    return wres;
                }
                }
                break;
            case DW_AT_GNU_dwo_id:
            case DW_AT_GNU_odr_signature:
            case DW_AT_dwo_id:
                {
                Dwarf_Sig8 v;

                v = zerosig;
                wres = dwarf_formsig8_const(attrib,&v,err);
                if (wres == DW_DLV_OK){
                    struct esb_s t;

                    esb_constructor(&t);
                    format_sig8_string(&v,&t);
                    esb_append(esbp,esb_get_string(&t));
                    esb_destructor(&t);
                } else if (wres == DW_DLV_NO_ENTRY) {
                    /* nothing? */
                    esb_append(esbp,"Impossible: no entry for ");
                    esb_append(esbp,get_FORM_name(theform));
                    esb_append(esbp," dwo_id");
                } else {
                    struct esb_s lstr;
                    esb_constructor(&lstr);
                    esb_append(&lstr,"For form ");
                    esb_append(&lstr,get_FORM_name(theform));
                    esb_append(&lstr," and attribute ");
                    esb_append(&lstr,get_AT_name(attr));
                    esb_append(&lstr,
                        " Cannot get  Dwarf_Sig8 value ");
                    print_error_and_continue(
                        esb_get_string(&lstr),
                        wres, *err);
                    esb_destructor(&lstr);
                    return wres;
                }
                }
                break;
            case DW_AT_upper_bound:
            case DW_AT_lower_bound:
            default:  {
                Dwarf_Bool chex = FALSE;
                Dwarf_Die  tdie = die;
                if (DW_AT_ranges        == attr ||
                    DW_AT_location      == attr ||
                    DW_AT_vtable_elem_location == attr ||
                    DW_AT_string_length == attr ||
                    DW_AT_return_addr   == attr ||
                    DW_AT_use_location  == attr ||
                    DW_AT_static_link   == attr ||
                    DW_AT_frame_base    == attr) {
                    /*  Do not look for data
                        type for unsigned/signed.
                        and do use HEX. */
                    chex = TRUE;
                    tdie = NULL;
                }
                /* Do not use hexadecimal format except for
                    DW_AT_ranges. */
                wres = formxdata_print_value(dbg,
                    tdie,attrib,
                    theform,esbp, err, chex);
                if (wres == DW_DLV_OK) {
                    /* String appended already. */
                } else if (wres == DW_DLV_NO_ENTRY) {
                    /* nothing? */
                } else {
                    struct esb_s lstr;
                    esb_constructor(&lstr);
                    esb_append(&lstr,"For form ");
                    esb_append(&lstr,get_FORM_name(theform));
                    esb_append(&lstr," and attribute ");
                    esb_append(&lstr,get_AT_name(attr));
                    esb_append(&lstr,
                        " Cannot get  Dwarf_Sig8 value ");
                    print_error_and_continue(
                        esb_get_string(&lstr),
                        wres, *err);
                    esb_destructor(&lstr);
                    return wres;
                }
                }
                break;
            }
        }
        if (glflags.gf_cu_name_flag) {
            if (attr == DW_AT_MIPS_fde) {
                if (glflags.fde_offset_for_cu_low ==
                    DW_DLV_BADOFFSET) {
                    glflags.fde_offset_for_cu_low
                        = glflags.fde_offset_for_cu_high = tempud;
                } else if (tempud < glflags.fde_offset_for_cu_low) {
                    glflags.fde_offset_for_cu_low = tempud;
                } else if (tempud > glflags.fde_offset_for_cu_high) {
                    glflags.fde_offset_for_cu_high = tempud;
                }
            }
        }
        }
        break;
    case DW_FORM_sdata: {
        Dwarf_Half attrs = 0;
        int ares = 0;
        Dwarf_Bool hxform=TRUE;
        Dwarf_Error attrerr = 0;

        ares = dwarf_whatattr(attrib, &attrs, &attrerr);
        {   /* Do regardless of ares value! */
            wres = dwarf_formsdata(attrib, &tempsd, err);
            if (wres == DW_DLV_OK) {
                tempud = tempsd;
                if (attrs == DW_AT_language_version) {
                    const char * version_name = 0;
                    const char * version_scheme = 0;

                    hxform = FALSE;
                    formx_signed(tempsd,esbp);
                    esb_append(esbp," (");
                    hxform = TRUE;
                    formx_unsigned_and_signed_if_neg(tempud,tempsd,
                        " (",hxform,esbp);
                    esb_append(esbp,")");
                    wres = dwarf_lvn_name(die,&version_name,
                        &version_scheme);
                    if (wres == DW_DLV_OK) {
                        esb_append(esbp," (version: ");
                        if (version_name) {
                            esb_append(esbp,version_name);
                        }
                        esb_append(esbp,")");
                        esb_append(esbp," (version scheme: ");
                        if (version_scheme) {
                            esb_append(esbp,version_scheme);
                        }
                        esb_append(esbp,")");
                    }
                } else {
                    formx_unsigned_and_signed_if_neg(tempud,tempsd,
                        " (",hxform,esbp);
                }
            } else if (wres == DW_DLV_NO_ENTRY) {
                /* nothing? */
            } else {
                print_error_and_continue(
                    "Cannot get DW_FORM_sdata value..",
                    wres, *err);
                return wres;
            }
        }
        if (ares == DW_DLV_ERROR) {
            /* leave no trace, caught elsewhere. */
            dwarf_dealloc_error(dbg,attrerr);
        }
        }
        break;
    case DW_FORM_udata: {
        Dwarf_Half attru = 0;
        int ares = 0;
        Dwarf_Error attrerr = 0;

        ares = dwarf_whatattr(attrib, &attru, err);
        {   /* Do regardless of ares value! */
            const char * version_name = 0;
            const char * version_scheme = 0;
            wres = dwarf_formudata(attrib, &tempud, err);
            if (wres == DW_DLV_OK) {
                Dwarf_Bool hex_format = TRUE;
                if (attru == DW_AT_language_version) {
                    hex_format = FALSE;
                    formx_unsigned(tempud,esbp,hex_format);
                    esb_append(esbp," (");
                    hex_format = TRUE;
                    formx_unsigned(tempud,esbp,hex_format);
                    esb_append(esbp,")");

                    wres = dwarf_lvn_name(die,&version_name,
                        &version_scheme);
                    if (wres == DW_DLV_OK) {
                        esb_append(esbp," (version: ");
                        if (version_name) {
                            esb_append(esbp,version_name);
                        }
                        esb_append(esbp,")");
                        esb_append(esbp," (version scheme: ");
                        if (version_scheme) {
                            esb_append(esbp,version_scheme);
                        }
                        esb_append(esbp,")");
                    }
                } else {
                    formx_unsigned(tempud,esbp,hex_format);
                }
            } else if (wres == DW_DLV_NO_ENTRY) {
                /* nothing? */
            } else {
                print_error_and_continue(
                    "Cannot get DW_FORM_udata value..",
                    wres, *err);
                return wres;
            }
        }
        if (ares == DW_DLV_ERROR) {
            /* leave no trace, caught elsewhere. */
            dwarf_dealloc_error(dbg,attrerr);
        }
        }
        break;
    /* various forms for strings. */
    case DW_FORM_string:
    case DW_FORM_strp:
    case DW_FORM_strx:   /* DWARF5 */
    case DW_FORM_strx1:  /* DWARF5 */
    case DW_FORM_strx2:  /* DWARF5 */
    case DW_FORM_strx3:  /* DWARF5 */
    case DW_FORM_strx4:  /* DWARF5 */
    case DW_FORM_strp_sup: /* DWARF5 String in altrnt: tied file */
    case DW_FORM_GNU_strp_alt: /* String in altrnt: tied file */
    case DW_FORM_line_strp: /* DWARF5, offset to .debug_line_str */
        /*  unsigned offset in size of an offset */
    case DW_FORM_GNU_str_index: {
        int sres = dwarf_formstring(attrib, &temps, err);
        if (sres == DW_DLV_OK) {
            if (theform == DW_FORM_strx ||
                theform == DW_FORM_strx1 ||
                theform == DW_FORM_strx2 ||
                theform == DW_FORM_strx3 ||
                theform == DW_FORM_strx4 ||
                theform == DW_FORM_GNU_str_index) {
                char saverbuf[ESB_FIXED_ALLOC_SIZE];
                struct esb_s saver;
                Dwarf_Unsigned index = 0;

                esb_constructor_fixed(&saver,saverbuf,
                    sizeof(saverbuf));
                sres = dwarf_get_debug_str_index(attrib,&index,err);
                esb_append(&saver,temps);
                if (sres == DW_DLV_OK) {
                    bracket_hex("(indexed string: ",index,")",esbp);
                } else {
                    DROP_ERROR_INSTANCE(dbg,sres,*err);
                    esb_append(esbp,
                        "(ERROR: indexed string:no "
                        "string provided?)");
                }
                esb_append(esbp, esb_get_string(&saver));
                esb_destructor(&saver);
            } else {
                esb_append(esbp,temps);
            }
        } else if (sres == DW_DLV_NO_ENTRY) {
            if (theform == DW_FORM_strx ||
                theform == DW_FORM_GNU_str_index ||
                theform == DW_FORM_strx1 ||
                theform == DW_FORM_strx2 ||
                theform == DW_FORM_strx3 ||
                theform == DW_FORM_strx4 ) {
                esb_append(esbp,
                    "(indexed string,no string provided?)");
            } else {
                esb_append(esbp, "<no string provided?>");
            }
        } else { /* DW_DLV_ERROR */
            if (theform == DW_FORM_strx ||
                theform == DW_FORM_GNU_str_index ||
                theform == DW_FORM_strx1 ||
                theform == DW_FORM_strx2 ||
                theform == DW_FORM_strx3 ||
                theform == DW_FORM_strx4 ) {
                struct esb_s lstr;

                esb_constructor(&lstr);
                esb_append(&lstr,"Cannot get an indexed string on ");
                esb_append(&lstr,get_FORM_name(theform));
                esb_append(&lstr,"....");
                print_error_and_continue(
                    esb_get_string(&lstr),
                    sres, *err);
                esb_destructor(&lstr);
                return sres;
            }
            {
                struct esb_s lstr;

                esb_constructor(&lstr);
                esb_append(&lstr,"Cannot get the form on ");
                esb_append(&lstr,get_FORM_name(theform));
                esb_append(&lstr,"....");
                print_error_and_continue(
                    esb_get_string(&lstr),
                    sres, *err);
                esb_destructor(&lstr);
                return sres;
            }
        }
        }
        break;
    case DW_FORM_flag: {
        Dwarf_Bool tempbool = 0;
        wres = dwarf_formflag(attrib, &tempbool, err);
        if (wres == DW_DLV_OK) {
            if (tempbool) {
                esb_append_printf_i(esbp,
                    "yes(%d)", tempbool);
            } else {
                esb_append(esbp,"no");
            }
        } else if (wres == DW_DLV_NO_ENTRY) {
            /* nothing? */
        } else {
            print_error_and_continue(
                "Cannot get formflag/p....", wres, *err);
            return wres;
        }
        }
        break;
    case DW_FORM_indirect:
        /*  We should not ever get here, since the true form was
            determined and direct_form has the DW_FORM_indirect
            if it is used here in this attr. */
        esb_append(esbp, get_FORM_name(theform));
        break;
    case DW_FORM_sec_offset: { /* DWARF4, DWARF5 */
        char* emptyattrname = 0;
        int show_form_here = 0;
        wres = dd_get_integer_and_name(dbg,
            attrib,
            &tempud,
            emptyattrname,
            /* err_string */ NULL,
            (encoding_type_func) 0,
            err,show_form_here);
        if (wres == DW_DLV_NO_ENTRY) {
            /* Show nothing? */
        } else if (wres == DW_DLV_ERROR) {
            print_error_and_continue(
                "ERROR: cannot et DW_FORM_sec_offset value content.",
                wres,*err);
            return wres;
        } else {
            bracket_hex("",tempud,"",esbp);
        }
        }

        break;
    case DW_FORM_flag_present: /* DWARF4 */
        esb_append(esbp,"yes(1)");
        break;
    case DW_FORM_ref_sig8: {  /* DWARF4 */
        Dwarf_Sig8 sig8data;

        sig8data = zerosig;
        wres = dwarf_formsig8(attrib,&sig8data,err);
        if (wres != DW_DLV_OK) {
            /* Show nothing? */
            print_error_and_continue(
                "ERROR: cannot et DW_FORM_ref_sig8 value content.",
                wres,*err);
            return wres;
        } else {
            struct esb_s sig8str;

            esb_constructor(&sig8str);
            format_sig8_string(&sig8data,&sig8str);
            esb_append(esbp,esb_get_string(&sig8str));
            esb_destructor(&sig8str);
            if (!show_form) {
                esb_append(esbp," <type signature>");
            }
        }
        }
        break;
    /* DWARF5, attr val is signed uleb */
    case DW_FORM_implicit_const: {
        wres = dwarf_formsdata(attrib, &tempsd, err);
        if (wres == DW_DLV_OK) {
            Dwarf_Bool hxform=TRUE;
            tempud = tempsd;
            formx_unsigned_and_signed_if_neg(tempud,tempsd,
                " (",hxform,esbp);
        } else if (wres == DW_DLV_NO_ENTRY) {
            /* nothing? */
        } else {
            print_error_and_continue(
                "ERROR: cannot get signed value of "
                "DW_FORM_implicit_const",
                wres,*err);
            return wres;
        }
        }
        break;
    case DW_FORM_loclistx:   /* DWARF5, index into .debug_loclists */
        wres = dwarf_formudata(attrib, &tempud, err);
        if (wres == DW_DLV_OK) {
            /*  Fall through to end to show the form details. */
            Dwarf_Bool hex_format = TRUE;
            esb_append(esbp,"<index to debug_loclists ");
            formx_unsigned(tempud,esbp,hex_format);
            esb_append(esbp,">");
            break;
        } else if (wres == DW_DLV_NO_ENTRY) {
            /* nothing? */
        } else {
            struct esb_s lstr;
            esb_constructor(&lstr);
            esb_append(&lstr,"Cannot get formudata on ");
            esb_append(&lstr,"DW_FORM_loclistx");
            esb_append(&lstr,"....");
            print_error_and_continue(
                esb_get_string(&lstr),
                wres, *err);
            esb_destructor(&lstr);
            return wres;
        }
        break;
    case DW_FORM_rnglistx: {
        /*  DWARF5, index into .debug_rnglists
            Can appear only on DW_AT_ranges attribute.*/
        wres = dwarf_formudata(attrib, &tempud, err);
        if (wres == DW_DLV_OK) {
            /*  Fall through to end to show the form details. */
            Dwarf_Bool hex_format = TRUE;
            esb_append(esbp,"<index to debug_rnglists ");
            formx_unsigned(tempud,esbp,hex_format);
            esb_append(esbp,">");
        } else if (wres == DW_DLV_NO_ENTRY) {
            /*  nothing? */
        } else {
            struct esb_s lstr;
            esb_constructor(&lstr);
            esb_append(&lstr,"Cannot get formudata on ");
            esb_append(&lstr,"DW_FORM_rnglistx");
            esb_append(&lstr,"....");
            print_error_and_continue(
                esb_get_string(&lstr),
                wres, *err);
            esb_destructor(&lstr);
            return wres;
        }
        }
        break;
    case DW_FORM_ref_sup4: /* DWARF5 */
    case DW_FORM_ref_sup8: /* DWARF5 */
    case DW_FORM_GNU_ref_alt: {
        /*  These refer to a supplemental or alternate object file,
            and we don't have those available to check. */
        bres = dwarf_global_formref_b(attrib, &off,&is_info, err);
        if (bres == DW_DLV_OK) {
            bracket_hex("",off,"",esbp);
        } else {
            struct esb_s lstr;
            esb_constructor(&lstr);
            esb_append(&lstr,get_FORM_name(theform));
            esb_append(&lstr," form with no reference?!");
            print_error_and_continue(
                esb_get_string(&lstr),
                bres,*err);
            esb_destructor(&lstr);
            return wres;
        }
        }
        break;
    default: {
        struct esb_s lstr;

        esb_constructor(&lstr);
        esb_append_printf_u(&lstr,
            "ERROR: dwarf_whatform unexpected value, "
            "form code 0x%04x",
            theform);
        simple_err_only_return_action(DW_DLV_ERROR,
            esb_get_string(&lstr));
        esb_destructor(&lstr);
        }
        break;
    } /* end switch on theform */
    show_form_itself(show_form,local_verbose,theform,
        direct_form,esbp);
    return DW_DLV_OK;
}

void
format_sig8_string(Dwarf_Sig8*data, struct esb_s *out)
{
    unsigned i = 0;
    esb_append(out,"0x");
    for (; i < sizeof(data->signature); ++i) {
        esb_append_printf_u(out,  "%02x",
            (unsigned char)(data->signature[i]));
    }
}

static int
get_form_values(Dwarf_Attribute attrib,
    Dwarf_Half * theform, Dwarf_Half * directform,
    Dwarf_Error *err)
{
    int res = 0;

    res = dwarf_whatform(attrib, theform, err);
    if (res != DW_DLV_OK) {
        return res;
    }
    res = dwarf_whatform_direct(attrib, directform, err);
    return res;
}

static void
show_form_itself(int local_show_form,
    int local_verbose,
    int theform,
    int directform, struct esb_s *esbp)
{
    if (local_show_form
        && directform && directform == DW_FORM_indirect) {
        char *form_indir = " (used DW_FORM_indirect";
        char *form_indir2 = ") ";
        esb_append(esbp, form_indir);
        if (local_verbose) {
            esb_append_printf_i(esbp," %d",DW_FORM_indirect);
        }
        esb_append(esbp, form_indir2);
    }
    if (local_show_form) {
        esb_append(esbp," <form ");
        esb_append(esbp,get_FORM_name(theform));
        if (local_verbose) {
            esb_append_printf_i(esbp," %d",theform);
        }
        esb_append(esbp,">");
    }
}
