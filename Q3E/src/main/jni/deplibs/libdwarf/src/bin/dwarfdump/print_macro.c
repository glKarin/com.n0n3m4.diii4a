/*
Copyright 2015-2020 David Anderson. All rights reserved.

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

#include <stdlib.h> /* calloc() free() */
#include <string.h> /* memcpy() strcmp() strdup() strlen() */
#include <stddef.h> /* size_t */
#include <stdio.h> /* FILE decl for dd_esb.h, printf etc */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_naming.h"
#include "dd_esb.h"
#include "dd_esb_using_functions.h"
#include "dd_uri.h"
#include "dd_makename.h"
#include "dd_tsearchbal.h"
#include "print_sections.h"
#include "dd_macrocheck.h"
#include "dd_sanitized.h"
#include "dd_safe_strcpy.h"

/*  See the comments at the beginning of macrocheck.c */

static int
strcmp_for_macdef_tdel( const void *l,const void *r)
{
    const char *ls = (const char *)l;
    const char *rs = (const char *)r;
    return strcmp(ls,rs);
}

/*  Extensible array, of pointers to file records .  */
#define MACFILE_ARRAY_START_LEN 100
unsigned macfile_array_len;
unsigned macfile_array_next_to_use;
/*  Array of pointers to extended macfile_entry records...
    records with the file name string appended, so
    not all the same length.   */
macfile_entry ** macfile_array;

static int macdef_tree_compare_func(const void *l, const void *r);
static void macdef_tree_insert(char *key,
    unsigned opnum,

    unsigned operator,
    Dwarf_Unsigned line,
    Dwarf_Unsigned offset,
    const char * string,
    Dwarf_Unsigned macro_unit_offset,
    void **map);
static macdef_entry * macdef_tree_find(char *key,
    void **map);
static macdef_entry * macdef_tree_create_entry(char *key,
    unsigned opnum,
    unsigned operator,
    Dwarf_Unsigned line,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned macro_unit_offset,
    const char * string);
static void macfile_array_destroy(void);
void macdef_tree_run_checks(void);

static void macdef_free_func(void *mx)
{
    struct madef_entry *m = mx;
    /* frees both strings and the struct */
    free(m);
}

macfile_entry *
macfile_from_array_index(
    unsigned index)
{
    macfile_entry *m = 0;
    m =  macfile_array[index];
    return m;
}

static void
macdef_tree_destroy_inner( void *tree)
{
    dwarf_tdestroy(tree,macdef_free_func);
}
static void
destroy_macdef_tree(void)
{
    macdef_tree_destroy_inner(macdefundeftree);
    macdefundeftree = 0;
}
static void
destroy_macro_globals(void)
{
    macfile_array_destroy();
    destroy_macdef_tree();
    /*  The stack is static, not malloc,
        and only has unsigned vals, nothing
        to free. */
    macfile_stack_next_to_use = 0;
}

static int
print_macros_5style_this_cu_inner(Dwarf_Debug dbg, Dwarf_Die cu_die,
    char **dwarf_srcfiles,
    Dwarf_Signed srcfiles_count,
    int do_print_dwarf /* not relying on gf_do_print_dwarf here */,
    int descend_into_inport/* TRUE means follow imports */,
    int by_offset /* if TRUE is an imported macro unit
        so the offset is relevant.
        If false is the set for the CU itself.  */,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned lineno,
    unsigned fileno,
    int level,
    Dwarf_Error *err);

static const char *nonameavail = "<no-name-available>";
static const char *nofileseenyet =
    "<Before-First-DW_MACRO_start_file>";

static void
print_stack_crash(void)
{
    unsigned i = 0;
    printf("MACRONOTE: The start file operation just above"
        " exceeds the max allowed of %d. "
        "Possibly corrupt dwarf\n",MACFILE_STACK_DEPTH_MAX);
    glflags.gf_count_macronotes++;
    printf("    []  op#    line   filenum   filename\n");
    for (i = 0; i < macfile_stack_next_to_use; ++i) {
        macfile_entry * m = macfile_array[macfile_stack[i]];
        printf("    [%u] %3u %4" DW_PR_DUu
            " %2" DW_PR_DUu
            " MOFF=0x%" DW_PR_DUx
            " %s\n",
            i, m->ms_operatornum,m->ms_line,m->ms_filenum,
            m->ms_macro_unit_offset,
            sanitized(m->ms_filename));
    }
}

/*  Return 0 if format is bogus */
static unsigned
find_set_keyend(char *str)
{
    char c = 0;
    char *cp = str;
    unsigned int len = 0;

    for ( ; *cp ; ++cp) {
        c=*cp;
        /*  The following two returns are
            the norm for #define */
        if (c == ' ') {
            *cp = 0;
            return len;
        } else if (c == '(') {
            *cp = 0;
            return len;
        }
        ++len;
        continue;
    }
    /*  This is the normal return for #undef.
        If an incoming #define string is empty or has
        no spaces we get here too.*/
    return len;
}

static int
is_define_op(unsigned int operator)
{
    switch(operator) {
    case DW_MACRO_define:
    case DW_MACRO_define_strx:
    case DW_MACRO_define_sup:
    case DW_MACRO_define_strp:
        return TRUE;
    }
    return FALSE;
}

static void
add_def_undef(unsigned opnum,
    Dwarf_Unsigned offset,
    unsigned int operator,
    Dwarf_Unsigned line_number,
    const char * macro_string,
    Dwarf_Off  macro_unit_offset,
    struct esb_s *mtext,
    Dwarf_Bool didprintdwarf)
{
    unsigned int key_length = 0;
    macfile_entry *m = 0;
    macdef_entry *meb = 0;
    char * keystr = 0;
    int isdef = FALSE;
    int allocerror = FALSE;

    if (!strcmp(esb_get_string(mtext),nonameavail)) {
        /*  we have no string, just the fake we provide.
            hard to check much in this case. */
        return;
    }
    /*  Have to strdup here as find_set_keyend
        modifies what it is passed. */
    keystr = strdup((const char *)macro_string);
    if (!keystr) {
        if (!allocerror) {
            glflags.gf_count_major_errors++;
            printf("ERROR: Macro define/undef "
                "macro op string strdup() fails, "
                "Out of memory. Some dwarfdump"
                "reporting will be incorrect.\n");
            allocerror = TRUE;
        }
        return;
    }
    key_length = find_set_keyend(keystr);
    if (!key_length) {
        if (!didprintdwarf) {
            printf("%s",sanitized(esb_get_string(mtext)));
        }
        glflags.gf_count_major_errors++;
        printf("ERROR: the above define/undef "
            "macro op is missing its "
            "macro name. Corrupt DWARF.\n");
        free(keystr);
        return;
    }
    isdef = is_define_op(operator);
    meb = macdef_tree_find(keystr,&macdefundeftree);
    if (!meb) {
        /* Unknown key.  */
        macdef_tree_insert(keystr,opnum,operator,line_number,
            offset,macro_string,
            macro_unit_offset,
            &macdefundeftree);
        meb = macdef_tree_find(keystr,&macdefundeftree);
        if (!meb) {
            printf("ERROR: Unable to find key \"%s\" "
                "in macdef tree though just created.\n",
                sanitized(keystr));
            free(keystr);
            return;
        }
        if (isdef) {
            meb->md_defined = isdef;
            meb->md_defcount = 1;
        } else {
            meb->md_undefined = !isdef;
            meb->md_undefcount = 1;
        }
        free(keystr);
        return;
    }
    /* A key we have seen */
    if (isdef) {
        if (meb->md_defined) {
            if (!strcmp(macro_string,meb->md_string)) {
                /* duplicate def. Legal C */
            } else {
                /* Not duplicate def. Bogus. */
                if (!didprintdwarf) {
                    printf("%s",sanitized(esb_get_string(mtext)));
                }
                glflags.gf_count_macronotes++;
                printf("MACRONOTE: Duplicating the macro "
                    "name \"%s\" but with a different spelling "
                    "seems to be an error\n",
                    sanitized(keystr));
                printf(" Earlier spelling in operator %u is\n",
                    meb->md_operatornum);
                m = macfile_from_array_index(
                    meb->md_file_array_entry);
                printf("  MOFF=0x%" DW_PR_XZEROS DW_PR_DUx
                    " from line %" DW_PR_DUu " file %s",
                    meb->md_macro_unit_offset,
                    meb->md_line,
                    sanitized(m->ms_filename));
                printf(" %s\n",sanitized(meb->md_string));
                printf(" new spelling with operator %u is\n",
                    opnum);
                m = macfile_from_array_index(
                    macfile_array_next_to_use -1);
                printf("  MOFF=0x%" DW_PR_XZEROS DW_PR_DUx
                    " from line %" DW_PR_DUu " file %s",
                    macro_unit_offset,
                    line_number,
                    sanitized(m->ms_filename));
                printf(" %s\n",sanitized(macro_string));
                meb->md_defcount++;
            }
        } else {
            /*  Now defining something new with this key. */
            /* Should we print something? Warn or not? */
            unsigned defcount = meb->md_defcount+1;
            unsigned undefcount = meb->md_undefcount;
            macdef_entry *mee = 0;
            dwarf_tdelete(keystr,&macdefundeftree,
                strcmp_for_macdef_tdel);
            macdef_tree_insert(keystr,opnum,
                operator,line_number,offset,
                macro_string,
                macro_unit_offset,
                &macdefundeftree);
            mee = macdef_tree_find(keystr,&macdefundeftree);
            if (!mee) {
                printf("ERROR: Unable to find key \"%s\" "
                    "in macdef tree though just created..\n",
                    sanitized(keystr));
                return;
            }
            mee->md_defined = TRUE;
            mee->md_defcount = defcount;
            mee->md_undefcount = undefcount+1;
        }
        free(keystr);
        /* free(me); */
        return;
    }
    /*  We have seen it and we have an undef for it now */
    if (meb->md_defined) {
        /*  Was def of something with this key,
            now undefining with this key. */
        unsigned defcount = meb->md_defcount;
        unsigned undefcount = meb->md_undefcount+1;
        macdef_entry *mec = 0;
        dwarf_tdelete(keystr,&macdefundeftree,
            strcmp_for_macdef_tdel);
        macdef_tree_insert(keystr,opnum,
            operator,line_number,offset,macro_string,
            macro_unit_offset,
            &macdefundeftree);
        mec = macdef_tree_find(keystr,&macdefundeftree);
        if (!mec) {
            printf("ERROR: Unable to find key \"%s\" "
                "in macdef tree though just created..\n",
                sanitized(keystr));
            return;
        }
        mec->md_defined = FALSE;
        mec->md_undefined = TRUE;
        mec->md_defcount = defcount;
        mec->md_undefcount = undefcount+1;
        /* free(me); */
        free(keystr);
        return;
    }
    if (!meb->md_undefined) {
        /* Simply a normal first undef of a defined thing. */
        unsigned defcount = meb->md_defcount;
        unsigned undefcount = meb->md_undefcount+1;
        macdef_entry *med = 0;
        dwarf_tdelete(keystr,&macdefundeftree,
            strcmp_for_macdef_tdel);
        macdef_tree_insert(keystr,opnum,
            operator,line_number,offset,macro_string,
            macro_unit_offset,
            &macdefundeftree);
        med = macdef_tree_find(keystr,&macdefundeftree);
        if (!med) {
            printf("ERROR: Unable to find key \"%s\" "
                "in macdef tree though just created.3.\n",
                sanitized(keystr));
            return;
        }
        med->md_defined = FALSE;
        med->md_undefined = TRUE;
        med->md_defcount = defcount;
        med->md_undefcount = undefcount+1;
        /* free(me); */
        free(keystr);
        return;
    }
    /* ASSERT: meb->md_undefined TRUE */
    /* In tree is marked undef. So undef again */
    if (!didprintdwarf) {
        printf("%s",sanitized(esb_get_string(mtext)));
    }
    if (!glflags.gf_do_check_dwarf) {
        free(keystr);
        return;
    }
    glflags.gf_count_macronotes++;
    printf("MACRONOTE: Duplicating the undefine of macro "
        "name \"%s\" "
        "could possibly be an error.\n",
        sanitized(keystr));
    printf(" Earlier in operator %u is\n", meb->md_operatornum);
    m = macfile_from_array_index(meb->md_file_array_entry);
    printf("  MOFF=0x%" DW_PR_XZEROS DW_PR_DUx
        " from line %" DW_PR_DUu " file %s",
        meb->md_macro_unit_offset,
        meb->md_line,
        sanitized(m->ms_filename));
    printf(" %s\n",sanitized(meb->md_key));
    printf(" new  in operator %u is\n", opnum);
    m = macfile_from_array_index(macfile_array_next_to_use -1);
    printf("  MOFF=0x%" DW_PR_XZEROS DW_PR_DUx
        " from line %" DW_PR_DUu " file %s",
        macro_unit_offset,
        line_number,
        sanitized(m->ms_filename));
    printf(" %s\n",sanitized(keystr));
    free(keystr);
    /*     free(me); */
    return;
}
static void
expand_array_file_if_required(void)
{
    if (macfile_array_next_to_use >= macfile_array_len) {
        /*  ASSERT: useme == macfile_array_len */
        unsigned oldlen = macfile_array_len;
        unsigned newlen = 2*oldlen;
        macfile_entry ** newar = 0;

        if (!newlen) {
            /*  We have seen nothing, make a fresh start. */
            newlen = MACFILE_ARRAY_START_LEN;
            free(macfile_array);
            macfile_array = 0;
            macfile_array_next_to_use = 0;
            macfile_array_len = 0;
        }
        newar = (macfile_entry **)calloc(newlen,
            sizeof(macfile_entry *));
        if (!newar) {
            /*  Out of memory. */
            printf("\nERROR: out of memory attempting "
                "allocation of %u "
                "entries on macfile_array. Skipping entry.\n",newlen);
            glflags.gf_count_major_errors++;
            return;
        }
        if (oldlen && macfile_array) {
            memcpy(newar,macfile_array,
                oldlen*sizeof(macfile_entry *));
            free(macfile_array);
            macfile_array = 0;
        }
        macfile_array_next_to_use = oldlen;
        macfile_array_len = newlen;
        macfile_array = newar;
    }
}

static void
add_array_file_entry(unsigned k,
    Dwarf_Unsigned offset,
    unsigned int   operator,
    Dwarf_Unsigned line_number,
    Dwarf_Unsigned index,
    Dwarf_Off      macro_unit_offset,
    const char   * macro_string)
{
    size_t namelen = strlen(macro_string) +1;
    unsigned stroff = sizeof(macfile_entry);
    macfile_entry *m = 0;
    /*  Room for the string, its NUL, and the entry struct
        fields. */
    unsigned long  alloclen = (unsigned long)
        (sizeof(macfile_entry) + namelen);

    expand_array_file_if_required();
    m = (macfile_entry*) calloc(1,alloclen);
    if (!m) {
        return;
    }
    m->ms_operatornum = k;
    m->ms_operator = operator;
    m->ms_line = line_number;
    m->ms_filenum = index;
    m->ms_offset = offset;
    m->ms_macro_unit_offset = macro_unit_offset;
    m->ms_array_number = macfile_array_next_to_use;
    m->ms_filename = (char *)m + stroff;
    dd_safe_strcpy(m->ms_filename,namelen,macro_string,namelen-1);
    macfile_array[macfile_array_next_to_use] = m;
    macfile_stack[macfile_stack_next_to_use] =
        macfile_array_next_to_use;
    macfile_array_next_to_use++;
    macfile_stack_next_to_use++;
    if (macfile_stack_next_to_use > macfile_stack_max_seen) {
        macfile_stack_max_seen = macfile_stack_next_to_use;
    }
}

static void
add_to_file_stack(unsigned k,
    Dwarf_Unsigned offset,
    unsigned int   operator,
    Dwarf_Unsigned line_number,
    Dwarf_Unsigned index,
    Dwarf_Off      macro_unit_offset,
    const char   * macro_string,
    struct esb_s * mtext,
    Dwarf_Bool didprintdwarf)
{
    if (operator == DW_MACRO_end_file) {
        unsigned stack_useme = 0;

        /* DW_MACRO_end_file */
        if (!didprintdwarf && !glflags.gf_do_check_dwarf) {
            printf("%s",sanitized(esb_get_string(mtext)));
        }
        if (!glflags.gf_do_check_dwarf) {
            macfile_entry *m = 0;
            m = macfile_from_array_index(
                macfile_array_next_to_use -1);
            if (macfile_stack_next_to_use < 1) {
                printf("MACRONOTE: End file operation just above"
                    "  MOFF=0x%" DW_PR_XZEROS DW_PR_DUx
                    " file %s"
                    " has no applicable start file!"
                    " Possibly corrupt dwarf.\n",
                    macro_unit_offset,
                    sanitized(m->ms_filename));
                glflags.gf_count_macronotes++;
                return;
            }
        }
        /* Leave the file array untouched. */
        stack_useme = macfile_stack_next_to_use -1;
        macfile_stack[stack_useme] = 0;
        macfile_stack_next_to_use = stack_useme;
        return;
    }
    if (macfile_stack_next_to_use >= MACFILE_STACK_DEPTH_MAX) {
        if (!didprintdwarf) {
            printf("%s",sanitized(esb_get_string(mtext)));
        }
        print_stack_crash();
        return;
    }
    add_array_file_entry(k,offset,operator,
        line_number,index,
        macro_unit_offset,macro_string);
    return;
}

static void
print_source_intro(Dwarf_Debug dbg,Dwarf_Die cu_die)
{
    Dwarf_Off off = 0;
    int ores = 0;
    Dwarf_Error err = 0;

    ores = dwarf_dieoffset(cu_die, &off, &err);
    if (ores == DW_DLV_OK) {
        int lres = 0;
        const char *sec_name = 0;

        lres = dwarf_get_die_section_name_b(cu_die,
            &sec_name,&err);
        if (lres != DW_DLV_OK ||  !sec_name || !strlen(sec_name)) {
            sec_name = ".debug_info";
        }
        if (lres == DW_DLV_ERROR) {
            dwarf_dealloc_error(dbg,err);
            err = 0;
        }
        printf("Macro data from CU-DIE at %s offset 0x%"
            DW_PR_XZEROS DW_PR_DUx ":\n",
            sanitized(sec_name),
            (Dwarf_Unsigned) off);
    } else {
        printf("Macro data (for the CU-DIE at unknown location):\n");
    }
    if (ores == DW_DLV_ERROR) {
        dwarf_dealloc_error(dbg,err);
    }
}

static void
derive_error_message(unsigned k,
    Dwarf_Half macro_operator,
    Dwarf_Unsigned number_of_ops,
    int  res,Dwarf_Error *err,
    const char *operator_string)
{
    const char *name = "";
    struct esb_s m;

    dwarf_get_MACRO_name(macro_operator,&name);
    esb_constructor(&m);
    if (res == DW_DLV_ERROR) {
        esb_append(&m,
            "ERROR from ");
    } else {
        esb_append(&m,
            "ERROR. NO_ENTRY from ");
    }
    esb_append(&m,operator_string);
    esb_append_printf_s(&m,
        " for operand %s ",sanitized(name));
    esb_append_printf_u(&m,
        " operand %u ",k);
    esb_append_printf_u(&m,
        " of %u operands",number_of_ops);
    print_error_and_continue(esb_get_string(&m),
        res,*err);
    esb_destructor(&m);
}

/*   For all macro/macinfo versions. */
void
print_split_macro_value(const char *m_in)
{
    char typbuf[400];
    char    *local = (char *)m_in;
    char    *valstart = 0;
    char    *cur = local;
    char     lastchar = 0;
    struct esb_s m;

    if (!glflags.verbose) {
        return;
    }
    typbuf[0] = 0;
    valstart = dwarf_find_macro_value_start(local);
    if (!*valstart) {
        /*  No value, just a name */
        printf("         Name Only: %s\n",local);
        return;
    }
    esb_constructor_fixed(&m,typbuf,sizeof(typbuf));
    /*  Has name<space>value
        or name(operands)<space>value */
    lastchar = 0;
    for ( ;*cur && cur != valstart; ++cur) {
        esb_appendn(&m,cur,1);
        lastchar = *cur;
    }
    if (lastchar != ' ') {
        printf("ERROR: Macro missing space to separate"
            "name from value!");
        glflags.gf_count_major_errors++;
        return;
    }
    printf("         Name : %s\n",sanitized(esb_get_string(&m)));
    printf("         Value: %s\n",sanitized(valstart));
    esb_destructor(&m);
}

static int
print_macro_ops(Dwarf_Debug dbg,
    Dwarf_Die cu_die,
    char ** dwarf_srcfiles,
    Dwarf_Signed    srcfiles_count,
    Dwarf_Macro_Context mcontext,
    Dwarf_Unsigned number_of_ops,
    int do_print_dwarf /* not relying on gf_do_print_dwarf here */,
    int descend_into_import /* TRUE means follow imports */,
    int by_offset /* if TRUE is an imported macro unit */,
    Dwarf_Unsigned macro_unit_offset /* of this set*/,
    Dwarf_Unsigned *macro_unit_length /* return val */,
    int level,
    Dwarf_Error *err)
{
    unsigned k = 0;

    for (k = 0; k < number_of_ops; ++k) {
        Dwarf_Unsigned  section_offset = 0;
        Dwarf_Half      macro_operator = 0;
        Dwarf_Half      forms_count = 0;
        const Dwarf_Small *formcode_array = 0;
        Dwarf_Unsigned  line_number = 0;
        Dwarf_Unsigned  index = 0;
        Dwarf_Unsigned  offset =0;
        const char    * macro_string =0;
        int lres = 0;
        static char mbuf[100];
        struct esb_s mtext;

        esb_constructor_fixed(&mtext,mbuf,sizeof(mbuf));
        lres = dwarf_get_macro_op(mcontext,
            k, &section_offset,&macro_operator,
            &forms_count, &formcode_array,err);
        if (lres != DW_DLV_OK) {
            struct esb_s m;

            esb_constructor(&m);
            if (lres == DW_DLV_ERROR) {
                esb_append(&m,
                    "ERROR from  dwarf_get_macro_op()");
            } else {
                esb_append(&m,
                    "ERROR. NO_ENTRY from  dwarf_get_macro_op()");
            }
            esb_append_printf_u(&m,
                " for operand %u ",k);
            esb_append_printf_u(&m,
                " of %u operands",number_of_ops);
            print_error_and_continue(esb_get_string(&m),
                lres,*err);
            esb_destructor(&m);
            esb_destructor(&mtext);
            return lres;
        }
        esb_append_printf_i(&mtext,"   [%3d] ",k);
        if (by_offset && descend_into_import) {
            esb_append_printf_u(&mtext," <MOFF=0x%"
                DW_PR_XZEROS DW_PR_DUx ">",
                macro_unit_offset);
        }
        esb_append_printf_u(&mtext,"0x%02x",macro_operator);
        esb_append_printf_s(&mtext," %-20s",
            (macro_operator?
                get_MACRO_name(macro_operator):
                "end-of-macros"));
        if (glflags.gf_show_global_offsets) {
            esb_append_printf_u(&mtext," <GOFF=0x%"
                DW_PR_XZEROS DW_PR_DUx ">",
                section_offset);
        }
        if (glflags.show_form_used && forms_count > 0) {
            unsigned l = 0;

            esb_append_printf_u(&mtext,"\n     Forms count %2u:",
                forms_count);
            for (; l < forms_count;++l) {
                Dwarf_Small form = formcode_array[l];
                esb_append_printf_u(&mtext," 0x%02x",
                    form);
                esb_append_printf_s(&mtext," %-18s ",
                    get_FORM_name(form));

            }
            esb_append(&mtext,"\n   ");
        }
        switch(macro_operator) {
        case 0: {
            /*  End of these DWARF_MACRO ops */
            Dwarf_Unsigned macro_unit_len = section_offset +1 -
                macro_unit_offset;
            esb_append_printf_u(&mtext,
                " op offset 0x%" DW_PR_XZEROS DW_PR_DUx,
                section_offset);
            esb_append_printf_u(&mtext,
                " macro unit length %" DW_PR_DUu,
                macro_unit_len);
            esb_append_printf_u(&mtext,
                " next byte offset 0x%" DW_PR_XZEROS DW_PR_DUx,
                section_offset+1);
            *macro_unit_length = macro_unit_len;
            esb_append(&mtext,"\n");
            if (do_print_dwarf) {
                printf("%s",sanitized(esb_get_string(&mtext)));
            }
            }
            break;
        case DW_MACRO_end_file:
            if (do_print_dwarf) {
                esb_append(&mtext,"\n");
            }
            if (do_print_dwarf) {
                printf("%s",sanitized(esb_get_string(&mtext)));
            }
            add_to_file_stack(k,offset,macro_operator,
                line_number,offset,
                macro_unit_offset,"",
                &mtext,do_print_dwarf);
            break;
        case DW_MACRO_define:
        case DW_MACRO_undef: {
            lres = dwarf_get_macro_defundef(mcontext,
                k,
                &line_number,
                &index,
                &offset,
                &forms_count,
                &macro_string,
                err);
            if (lres != DW_DLV_OK) {
                derive_error_message(k,macro_operator,
                    number_of_ops,
                    lres,err,"dwarf_get_macro_defundef");
                esb_destructor(&mtext);
                return lres;
            }
            esb_append_printf_u(&mtext,"  line %u",line_number);
            esb_append_printf_s(&mtext," %s\n",
                macro_string?
                sanitized(macro_string):nonameavail);
            if (do_print_dwarf) {
                printf("%s",sanitized(esb_get_string(&mtext)));
                if (macro_string) {
                    print_split_macro_value(macro_string);
                }
            }
            add_def_undef(k,offset,macro_operator,
                line_number,macro_string,
                macro_unit_offset,
                &mtext,do_print_dwarf);
            break;
            }

        case DW_MACRO_define_strp:
        case DW_MACRO_undef_strp: {
            lres = dwarf_get_macro_defundef(mcontext,
                k,
                &line_number,
                &index,
                &offset,
                &forms_count,
                &macro_string,
                err);
            if (lres != DW_DLV_OK) {
                derive_error_message(k,macro_operator,
                    number_of_ops,
                    lres,err,"dwarf_get_macro_defundef");
                esb_destructor(&mtext);
                return lres;
            }
            esb_append_printf_u(&mtext,
                "  line %" DW_PR_DUu,line_number);
            esb_append_printf_u(&mtext,
                " str offset 0x%" DW_PR_XZEROS DW_PR_DUx,
                offset);
            esb_append_printf_s(&mtext,
                " %s\n",macro_string?
                sanitized(macro_string):nonameavail);
            if (do_print_dwarf) {
                printf("%s",esb_get_string(&mtext));
                if (macro_string) {
                    print_split_macro_value(macro_string);
                }
            }
            add_def_undef(k,offset,macro_operator,
                line_number,macro_string,
                macro_unit_offset,
                &mtext,do_print_dwarf);
            }
            break;
        case DW_MACRO_define_strx:
        case DW_MACRO_undef_strx: {
            lres = dwarf_get_macro_defundef(mcontext,
                k,
                &line_number,
                &index,
                &offset,
                &forms_count,
                &macro_string,
                err);
            if (lres != DW_DLV_OK) {
                derive_error_message(k,macro_operator,
                    number_of_ops,
                    lres,err,"dwarf_get_macro_defundef");
                esb_destructor(&mtext);
                return lres;
            }
            esb_append_printf_u(&mtext,
                "  line %" DW_PR_DUu,line_number);
            esb_append_printf_u(&mtext,
                " str offset 0x%" DW_PR_XZEROS DW_PR_DUx,
                offset);
            esb_append_printf_s(&mtext,
                " %s\n",macro_string?
                sanitized(macro_string):nonameavail);
            if (do_print_dwarf) {
                printf("%s",sanitized(esb_get_string(&mtext)));
                if (macro_string) {
                    print_split_macro_value(macro_string);
                }
            }
            add_def_undef(k,offset,macro_operator,
                line_number,macro_string,
                macro_unit_offset,
                &mtext,do_print_dwarf);
            break;
            }
        case DW_MACRO_define_sup:
        case DW_MACRO_undef_sup: {
            /*  The strings here are from a supplementary
                object file, not this object file.
                Until we have a way to find
                the supplementary object file
                those will show name
                "<no-name-available>"
                */
            /*  We do not add these to the MacroCheck
                treer */
            lres = dwarf_get_macro_defundef(mcontext,
                k,
                &line_number,
                &index,
                &offset,
                &forms_count,
                &macro_string,
                err);
            if (lres != DW_DLV_OK) {
                derive_error_message(k,macro_operator,
                    number_of_ops,
                    lres,err,"dwarf_get_macro_defundef");
                esb_destructor(&mtext);
                return lres;
            }
            esb_append_printf_u(&mtext,
                "  line %" DW_PR_DUu,line_number);
            esb_append_printf_u(&mtext,
                " str offset 0x%" DW_PR_XZEROS DW_PR_DUx,
                offset);
            esb_append_printf_s(&mtext,
                " %s\n",macro_string?
                sanitized(macro_string):nonameavail);
            if (do_print_dwarf) {
                printf("%s",sanitized(esb_get_string(&mtext)));
                if (macro_string) {
                    print_split_macro_value(macro_string);
                }
            }
            break;
            }
        case DW_MACRO_start_file: {
            lres = dwarf_get_macro_startend_file(mcontext,
                k,&line_number,
                &index,
                &macro_string,err);
            /*  The above call knows how to reference
                its one srcfiles data and has the
                .debug_macro version. So we do not
                need to worry about getting the file name
                here. */
            if (lres != DW_DLV_OK) {
                derive_error_message(k,macro_operator,
                    number_of_ops,
                    lres,err,"dwarf_get_macro_startend_file");
                esb_destructor(&mtext);
                return lres;
            }
            esb_append_printf_u(&mtext,"  line %" DW_PR_DUu,
                line_number);
            esb_append_printf_u(&mtext," file number %"
                DW_PR_DUu " ",
                index);
            esb_append(&mtext,macro_string?
                macro_string: "<no-name-available>");
            esb_append(&mtext,"\n");
            if (do_print_dwarf) {
                printf("%s",sanitized(esb_get_string(&mtext)));
            }
            add_to_file_stack(k,offset,macro_operator,
                line_number,index,
                macro_unit_offset,macro_string,
                &mtext,do_print_dwarf);
            break;
            }
        case DW_MACRO_import: {
            int mres = 0;
            lres = dwarf_get_macro_import(mcontext,
                k,&offset,err);
            if (lres != DW_DLV_OK) {
                derive_error_message(k,macro_operator,
                    number_of_ops,
                    lres,err,"dwarf_get_macro_import");
                esb_destructor(&mtext);
                return lres;
            }
            if (do_print_dwarf) {
                esb_append_printf_u(&mtext,
                    "  offset 0x%" DW_PR_XZEROS DW_PR_DUx,
                    offset);
            }
            esb_append(&mtext,"\n");
            if (do_print_dwarf) {
                printf("%s",sanitized(esb_get_string(&mtext)));
            }
            if (descend_into_import) {
                /*  not do_print_dwarf */
                macfile_entry *mac_e = 0;
                mac_e = macfile_from_array_index(
                    macfile_array_next_to_use-1);
                mres = macro_import_stack_present(offset);
                if (mres == DW_DLV_OK) {
                    printf("ERROR: While Printing DWARF5 macros "
                        "we find a recursive nest of imports "
                        " noted with offset 0x%"
                        DW_PR_XZEROS DW_PR_DUx " so we stop now. \n",
                        offset);
                    print_macro_import_stack();
                    glflags.gf_count_major_errors++;
                    return DW_DLV_NO_ENTRY;
                }
                mres = print_macros_5style_this_cu_inner(dbg,
                    cu_die,
                    dwarf_srcfiles,srcfiles_count,
                    FALSE /* turns off do_print_dwarf */,
                    descend_into_import,
                    TRUE /* by offset */,
                    offset,
                    mac_e->ms_line,
                    macfile_array_next_to_use-1,
                    level+1,
                    err);
                if (mres == DW_DLV_ERROR) {
                    struct esb_s m;

                    esb_constructor(&m);
                    esb_append_printf_u(&m,
                        "ERROR: Printing DWARF5 macros "
                        " at offset 0x%x "
                        "for the import CU failed. ",
                        offset);
                    print_error_and_continue(esb_get_string(&m),
                        mres,*err);
                    DROP_ERROR_INSTANCE(dbg,mres,*err);
                    esb_destructor(&m);
                }
            }
            break;
            }
        case DW_MACRO_import_sup: {
            lres = dwarf_get_macro_import(mcontext,
                k,&offset,err);
            if (lres != DW_DLV_OK) {
                derive_error_message(k,macro_operator,
                    number_of_ops,
                    lres,err,"dwarf_get_macro_import");
                esb_destructor(&mtext);
                return lres;
            }
#if 0 /* Possibly future use .debug_sup */
            add_macro_import_sup();
                /* The supplementary object file is not available,
                So we cannot check the import references
                or know the size. As of December 2020 */
#endif
            if (do_print_dwarf) {
                printf("  sup_offset 0x%" DW_PR_XZEROS DW_PR_DUx "\n"
                    ,offset);
            }
            break;
            }
        } /*  End switch(macro_operator) */
        esb_destructor(&mtext);
    }
    return DW_DLV_OK;
}

/*  We follow imports if building_primary_tree
    and in that case following imports we
    turn do_print_dwarf FALSE.
*/
static int
print_macros_5style_this_cu_inner(Dwarf_Debug dbg, Dwarf_Die cu_die,
    char **dwarf_srcfiles,
    Dwarf_Signed srcfiles_count,
    int do_print_dwarf /* not relying on gf_do_print_dwarf here */,
    int descend_into_import /* TRUE means follow imports */,
    int by_offset /* if TRUE is an imported macro unit
        so the offset is relevant.
        If false is the set for the CU itself.  */,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned lineno,
    unsigned filenum,
    int level,
    Dwarf_Error *err)
{
    int lres = 0;
    Dwarf_Unsigned version = 0;
    Dwarf_Macro_Context macro_context = 0;
    Dwarf_Unsigned macro_unit_offset = 0;
    Dwarf_Unsigned number_of_ops = 0;
    Dwarf_Unsigned ops_total_byte_len = 0;
    Dwarf_Unsigned context_total_byte_len = 0;
    Dwarf_Off dieprint_cu_goffset = 0;
    Dwarf_Off cudie_local_offset = 0;
    int atres = 0;

    glflags.current_section_id = DEBUG_MACRO;
    if (!by_offset) {
        lres = dwarf_get_macro_context(cu_die,
            &version,&macro_context,
            &macro_unit_offset,
            &number_of_ops,
            &ops_total_byte_len,
            err);
        offset = macro_unit_offset;
    } else {
        lres = dwarf_get_macro_context_by_offset(cu_die,
            offset,
            &version,&macro_context,
            &number_of_ops,
            &ops_total_byte_len,
            err);
        macro_unit_offset = offset;
    }
    if (lres == DW_DLV_NO_ENTRY) {
        return lres;
    }
    if (lres == DW_DLV_ERROR) {
        print_error_and_continue("Unable to dwarf_get_macro_context()"
            " for the DWARF 5 style macro",
            lres,*err);
        return lres;
    }
    /*  If we fail to get the offsets we won't worry about it. */
    atres = dwarf_die_offsets(cu_die,&dieprint_cu_goffset,
        &cudie_local_offset,err);
    DROP_ERROR_INSTANCE(dbg,atres,*err);
    lres = dwarf_macro_context_total_length(macro_context,
        &context_total_byte_len,err);
    if (lres != DW_DLV_OK) {
        dwarf_dealloc_macro_context(macro_context);
        return lres;
    }
    add_macro_import(&macro_check_tree,
        (!level)? 1:0,
        offset,lineno,filenum);
    add_macro_area_len(&macro_check_tree,offset,
        context_total_byte_len);
    lres = macro_import_stack_push(offset);
    if (lres == DW_DLV_ERROR) {
        dwarf_dealloc_macro_context(macro_context);
        /* message printed. Give up. */
        return DW_DLV_NO_ENTRY;
    }

    if (do_print_dwarf) {
        struct esb_s truename;
        char buf[DWARF_SECNAME_BUFFER_SIZE];

        esb_constructor_fixed(&truename,buf,sizeof(buf));
        get_true_section_name(dbg,".debug_macro",
            &truename,TRUE);
        /* This does not return */

        if (!by_offset) {
            printf("\n%s: Macro info for a single cu at macro Offset"
                " 0x%" DW_PR_XZEROS DW_PR_DUx "\n",
                sanitized(esb_get_string(&truename)),
                macro_unit_offset);
            print_source_intro(dbg,cu_die);
        } else {
            printf("\n%s: Macro info for imported macro unit "
                "at macro Offset "
                "0x%" DW_PR_XZEROS DW_PR_DUx
                "\n",
                sanitized(esb_get_string(&truename)),
                offset);
        }
        esb_destructor(&truename);
    } else {
        /* We are checking, not printing. */
        Dwarf_Half tag = 0;
        int tres = dwarf_tag(cu_die, &tag, err);
        if (tres != DW_DLV_OK) {
            /*  Something broken here. */
            dwarf_dealloc_macro_context(macro_context);
            print_error_and_continue("Unable to get CU DIE tag "
                "though we could see it earlier. "
                "Something broken.",
                tres,*err);
            return tres;
        } else if (tag == DW_TAG_type_unit) {
            dwarf_dealloc_macro_context(macro_context);
            /*  Not checking since type units missing
                address or range in CU header. */
            return DW_DLV_OK;
        }
    }
    if (do_print_dwarf && glflags.verbose > 1) {
        Dwarf_Bool attr_dup = FALSE;
        int pdres = 0;
        pdres = print_one_die(dbg, cu_die,
            dieprint_cu_goffset,
            /* print_information= */ 1,
            /* indent level */0,
            dwarf_srcfiles,srcfiles_count,
            &attr_dup,
            /* ignore_die_stack= */TRUE,err);
        if (pdres == DW_DLV_ERROR) {
            dwarf_dealloc_macro_context(macro_context);
            return pdres;
        }
    }
    {
        Dwarf_Half lversion =0;
        Dwarf_Unsigned mac_offset =0;
        Dwarf_Unsigned mac_len =0;
        Dwarf_Unsigned mac_header_len =0;
        Dwarf_Unsigned line_offset =0;
        unsigned mflags = 0;
        Dwarf_Bool has_line_offset = FALSE;
        Dwarf_Bool has_offset_size_64 = FALSE;
        Dwarf_Bool has_operands_table = FALSE;
        Dwarf_Half opcode_count = 0;
        Dwarf_Half offset_size = 4;
        Dwarf_Unsigned macro_unit_length = 0;
        const char *prefix = "";

        if (by_offset) {
            prefix = "        ";
        }
        lres = dwarf_macro_context_head(macro_context,
            &lversion, &mac_offset,&mac_len,
            &mac_header_len,&mflags,&has_line_offset,
            &line_offset,
            &has_offset_size_64,&has_operands_table,
            &opcode_count,err);
        if (lres == DW_DLV_NO_ENTRY) {
            dwarf_dealloc_macro_context(macro_context);
            /* Impossible */
            return lres;
        }
        if (lres == DW_DLV_ERROR) {
            dwarf_dealloc_macro_context(macro_context);
            print_error_and_continue(
                "ERROR: dwarf_macro_context_head failed",
                lres,*err);
            return lres;
        }
        if (has_offset_size_64) {
            offset_size = 8;
        }
        /*  If pure checking we won't print this header info */
        if (!glflags.gf_do_check_dwarf) {
            /*  To understand imports we really need the basic
                data shown on all targeted macro offsets.
                This is a start, allowing us to track
                the imported tables. Add verbose to see
                the rest printed just below */
            printf("%s  Nested import level: %d\n",prefix,level);
            printf("%s  Macro version      : %d\n",prefix,lversion);
            printf("%s  macro section offset 0x%"
                DW_PR_XZEROS DW_PR_DUx "\n",prefix,
                mac_offset);
        }
        if (glflags.verbose && !glflags.gf_do_check_dwarf) {
            printf("%s  flags: 0x%x, "
                "offsetsize64? %s, "
                "lineoffset? %s, "
                "operands_table? %s\n",
                prefix,
                mflags,
                has_offset_size_64?"yes":" no",
                has_line_offset   ?"yes":" no",
                has_operands_table?"yes":" no");
            printf("%s  offset size 0x%x\n",prefix,offset_size);
            printf("%s  header length: 0x%" DW_PR_XZEROS DW_PR_DUx
                "  total length: 0x%" DW_PR_XZEROS DW_PR_DUx "\n",
                prefix,
                mac_header_len,mac_len);
            if (has_line_offset) {
                printf("  debug_line_offset: 0x%"
                    DW_PR_XZEROS DW_PR_DUx "\n",
                    line_offset);
            }
            if (has_operands_table) {
                Dwarf_Half i = 0;

                for (i = 0; i < opcode_count; ++i) {
                    Dwarf_Half opcode_num = 0;
                    Dwarf_Half operand_count = 0;
                    const Dwarf_Small *operand_array = 0;
                    Dwarf_Half j = 0;

                    lres = dwarf_macro_operands_table(macro_context,
                        i, &opcode_num, &operand_count,
                        &operand_array,err);
                    if (lres == DW_DLV_NO_ENTRY) {
                        struct esb_s m;

                        dwarf_dealloc_macro_context(macro_context);
                        esb_constructor(&m);

                        esb_append_printf_u(&m,
                            "ERROR: dwarf_macro_operands_table()"
                            " returns NO_ENTRY for index %u ",
                            i);
                        esb_append_printf_u(&m,
                            "  of %u indexes. ",
                            opcode_count);
                        print_error_and_continue(esb_get_string(&m),
                            lres,*err);
                        esb_destructor(&m);
                        return lres;
                    }
                    if (lres == DW_DLV_ERROR) {
                        struct esb_s m;

                        dwarf_dealloc_macro_context(macro_context);
                        esb_constructor(&m);
                        esb_append_printf_u(&m,
                            "ERROR: dwarf_macro_operands_table()"
                            " returns ERROR for index %u ",
                            i);
                        esb_append_printf_u(&m,
                            "  of %u indexes. ",
                            opcode_count);
                        print_error_and_continue(esb_get_string(&m),
                            lres,*err);
                        esb_destructor(&m);
                        return lres;
                    }
                    if (opcode_num == 0) {
                        printf("%s  [%3u]  end of macro operands.",
                            prefix,i);
                        /*  Continue just in case something is wrong
                            and there are more operands! */
                        continue;
                    }
                    printf("%s  [%3u]  op: 0x%04x  %20s  "
                        "operandcount: %u\n",
                        prefix,
                        i,opcode_num,
                        get_MACRO_name(opcode_num),
                        operand_count);
                    for (j = 0; j < operand_count; ++j) {
                        Dwarf_Small opnd = operand_array[j];
                        printf("%s    [%3u] 0x%04x %20s\n",
                            prefix,j,opnd,
                            get_FORM_name(opnd));
                    }
                }
            }
        }
        if (do_print_dwarf) {
            printf("  MacroInformationEntries count: %" DW_PR_DUu
                ", bytes length: %" DW_PR_DUu "\n",
                number_of_ops,ops_total_byte_len);
        }
        lres = print_macro_ops(dbg,
            cu_die,
            dwarf_srcfiles,
            srcfiles_count,
            macro_context,number_of_ops,
            do_print_dwarf /*not gf_do_print_dwarf here*/,
            descend_into_import /* TRUE means follow imports */,
            by_offset /* if TRUE is an imported macro set */,
            macro_unit_offset,
            &macro_unit_length,
            level,
            err);
        if (lres != DW_DLV_OK) {
            struct esb_s m;

            dwarf_dealloc_macro_context(macro_context);
            esb_constructor(&m);
            if (lres == DW_DLV_ERROR){
                esb_append(&m,
                    "ERROR: print_macro_ops() failed"
                    " returns ERROR  ");
            } else {
                esb_append(&m,
                    "ERROR: print_macro_ops() failed"
                    " returns NO_ENTRY  ");
            }
            print_error_and_continue(esb_get_string(&m),
                lres,*err);
            esb_destructor(&m);
            return lres;
        }
        /*  macro_unit_offset for macro_unit_length bytes
            is a real macro unit. */
    }
    if (do_print_dwarf) {
        mark_macro_offset_printed(&macro_check_tree,offset);
    }
    lres = macro_import_stack_pop();
    dwarf_dealloc_macro_context(macro_context);
    macro_context = 0;
    if (lres != DW_DLV_OK) {
        return DW_DLV_NO_ENTRY;
    }
    return DW_DLV_OK;
}

/*  descend_into_import is TRUE if we are supposed to
    run through the import trees when imported versus
    running through imported later by stacking import offsets.
    It is passed as the value of do_check_dwarf(), so
    we could have left off the argument and used
    do_check_dwarf() here.
*/
int
print_macros_5style_this_cu(Dwarf_Debug dbg, Dwarf_Die cu_die,
    char **dwarf_srcfiles,
    Dwarf_Signed srcfiles_count,
    int do_print_dwarf /* not relying on gf_do_print_dwarf here */,
    int descend_into_import /* TRUE means follow imports */,

        /*  if TRUE is an imported macro unit
            so the offset is relevant.
            If false is for the CU itself, offset not relevant. */
    int by_offset ,

    Dwarf_Unsigned offset,
    Dwarf_Error *err)
{
    int res = 0;

    if (macfile_array_next_to_use || macfile_stack_next_to_use ||
        macdefundeftree || macfile_array) {
        printf("ERROR: dwarfdump internal files not properly "
            "initialized, internal dwarfdump bug. "
            "No macro access done. "
            "Pretending no macro section present\n");
        glflags.gf_count_major_errors++;
        return DW_DLV_NO_ENTRY;
    }
    add_array_file_entry(0,0,DW_MACRO_start_file,
        0,0,0,nofileseenyet);
    res =print_macros_5style_this_cu_inner(dbg,cu_die,
        dwarf_srcfiles,
        srcfiles_count,
        do_print_dwarf,
        descend_into_import,
        by_offset,
        offset,
        0,0,
        0,
        err);
    macdef_tree_run_checks();
    destroy_macro_globals();

    /*  Do NOT clear macrocheck statistics  here,
        wait till all CUs processed before clearing. */
    return res;
}

static int
macdef_tree_compare_func(const void *l, const void *r)
{
    const macdef_entry *ml = l;
    const macdef_entry *mr = r;
    int res = 0;

    res = strcmp(ml->md_key,mr->md_key);
    return res;
}

static void
macdef_tree_insert(char *key,
    unsigned opnum,
    unsigned operator,
    Dwarf_Unsigned line,
    Dwarf_Unsigned offset,
    const char * string,
    Dwarf_Unsigned macro_unit_offset,
    void **map)
{
    void *retval     = 0;
    macdef_entry *re = 0;
    macdef_entry  *e = 0;

    e  = macdef_tree_create_entry(key,
        opnum,operator,line,offset,macro_unit_offset,string);
    e->md_defcount = 0;
    e->md_undefcount = 0;
    e->md_undefined = FALSE;
    e->md_defined = FALSE;
    /*  tsearch records e's contents unless e
        is already present . We must not free it till
        destroy time if it got added to tree1.  */
    retval = dwarf_tsearch(e,map, macdef_tree_compare_func);
    if (retval) {
        re = *(macdef_entry **)retval;
        if (re != e) {
            /*  We returned an existing record, e not needed.
                Increment refcounts. */
            macdef_free_func(e);
        } else {
            /* Record e got added to tree1, do not free record e. */
        }
    }
}
static macdef_entry *
macdef_tree_create_entry(char *key,
    unsigned opnum,
    unsigned operator,
    Dwarf_Unsigned line,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned macro_unit_offset,
    const char * string)
{
    char *keyspace = 0;
    size_t klen = strlen(key) +1;
    size_t slen = strlen(string) +1;
    size_t finallen = sizeof(macdef_entry) + klen + slen;
    macdef_entry *me =
        (macdef_entry*)calloc(1,finallen);
    if (!me) {
        return 0;
    }
    keyspace = sizeof(macdef_entry) + (char *)me;
    me->md_key = keyspace;
    dd_safe_strcpy(me->md_key,klen,key,klen-1);
    me->md_operatornum = opnum;
    /*  We will set md_define, md_undefined,
        and the md_defcount and md_undefcount
        elsewhere. */
    me->md_defined    = FALSE;
    me->md_undefined  = FALSE;
    me->md_operator = operator;
    me->md_line = line;
    me->md_offset = offset;
    me->md_macro_unit_offset = macro_unit_offset;
    me->md_string = keyspace + klen;
    me->md_file_array_entry = macfile_array_next_to_use-1;
    dd_safe_strcpy(me->md_string,slen,string,slen-1);
    return me;
}

static macdef_entry *
macdef_tree_find(char *key, void**tree)
{
    void *retval = 0;
    macdef_entry *re = 0;
    macdef_entry *e = 0;

    e = macdef_tree_create_entry(key, 0,0,0,0,0,"<fake>");
    retval = dwarf_tfind(e,tree, macdef_tree_compare_func);
    if (retval) {
        re = *(macdef_entry **)retval;
    }
    /*  The one we created here must be deleted, it is dead.
        We look at the returned one instead. */
    macdef_free_func(e);
    return re;

}

static void
macfile_array_destroy(void)
{
    unsigned i = 0;
    for ( ; i < macfile_array_next_to_use; ++i ) {
        macfile_entry *m = macfile_array[i];

        /*  Frees the macfile_entry and the filename string
            attached to the end of the struct. */
        free(m);
        macfile_array[i] = 0;
    }
    free(macfile_array);
    macfile_array_len = 0;
    macfile_array_next_to_use = 0;
    macfile_array = 0;
}

static Dwarf_Unsigned walk_reccount = 0;
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
    walk_reccount += 1;
}
static Dwarf_Unsigned
macro_count_recs(void **base)
{
    walk_reccount = 0;
    dwarf_twalk(*base,macro_walk_count_recs);
    return walk_reccount;
}

/*  These are file-static, not local, as we need to
    access in a tree-walk. */
static macdef_entry **mac_as_array = 0;
static unsigned mac_as_array_next = 0;
static void
macro_walk_to_array(const void *nodep,const DW_VISIT  which,
    const int depth)
{
    macdef_entry * re = *(macdef_entry**)nodep;

    (void)depth;
    if (which == dwarf_postorder || which == dwarf_endorder) {
        return;
    }
    mac_as_array[mac_as_array_next] = re;
    mac_as_array_next++;
}

static int
macdef_qsort_compare(const void *lin, const void *rin)
{
    const macdef_entry *l =
        *(const macdef_entry **)lin;
    const macdef_entry *r =
        *(const macdef_entry **)rin;
    int res = 0;

    res = strcmp(l->md_key,r->md_key);
    if (res) {
        return res;
    }
    if (l->md_operatornum < r->md_operatornum) {
        return -1;
    }
    if (l->md_operatornum > r->md_operatornum) {
        return 1;
    }
    /*  No two can have the same md_operatornum,
        so this is impossible. */
    return 0;
}

static void
print_macdef_warn(unsigned i, macdef_entry *m,unsigned warncount)
{
    if (!warncount) {
        printf("     macro        "
            "            defs  undefs at-end\n");
    }
    printf("[%2d] %-24s",i,m->md_key);
    printf(" %2u",m->md_defcount);
    printf("     %2u",m->md_undefcount);
    printf("  %s",m->md_defined?"defined":"undefined");
    printf("\n");
}

/*  Check the macdefundef tree for the unusual
    Check the macfile_stack for leftovers.
    The tree starts with 0 and that entry
    is a fake for macro ops before a DW_MACRO_start_file
    encountered. */
void
macdef_tree_run_checks(void)
{
    unsigned i = 0;
    unsigned warncount = 0;
    Dwarf_Unsigned me_array_count = 0;

    free(mac_as_array);
    mac_as_array = 0;
    mac_as_array_next = 0;
    if (macfile_stack_next_to_use > 1) {
        printf("MACRONOTE: The DWARF5 macro start-file stack has"
            " %u entries left on the stack. Missing "
            " some end-file entries?\n",
            macfile_stack_next_to_use);
        glflags.gf_count_macronotes++;
        printf("    []  op#    line   filenum   filename\n");
        for (i = 0; i < macfile_stack_next_to_use; ++i) {
            macfile_entry * m = macfile_array[macfile_stack[i]];
            printf("    [%u] %3u %4" DW_PR_DUu
                " %2" DW_PR_DUu " %s\n",
                i, m->ms_operatornum,m->ms_line,m->ms_filenum,
                sanitized(m->ms_filename));
        }
    }
    /* Now check the def/undef tree left */
    if (!glflags.gf_do_check_dwarf) {
        return;
    }
    if (!macdefundeftree) {
        return;
    }
    me_array_count = macro_count_recs(&macdefundeftree);
    if (me_array_count) {
        mac_as_array = (macdef_entry**) calloc(me_array_count,
            sizeof(macdef_entry*));
    }
    if (!mac_as_array) {
        /* done */
        return;
    }
    warncount = 0;
    mac_as_array_next = 0;
    dwarf_twalk(macdefundeftree, macro_walk_to_array);
    qsort(mac_as_array,
        me_array_count,sizeof(macdef_entry*),
        macdef_qsort_compare);

    for (i = 0; i < me_array_count; ++i) {
        macdef_entry *m = mac_as_array[i];

        if (!m->md_defined &&
            m->md_defcount == m->md_undefcount) {
            /* totally normal.*/
            continue;
        }
        if (m->md_defined &&
            m->md_defcount ==1 && m->md_undefcount == 0) {
            /* totally normal.*/
            continue;
        }
        print_macdef_warn(i,m,warncount);
        ++warncount;
    }
    free(mac_as_array);
    mac_as_array = 0;
    mac_as_array_next = 0;
}
