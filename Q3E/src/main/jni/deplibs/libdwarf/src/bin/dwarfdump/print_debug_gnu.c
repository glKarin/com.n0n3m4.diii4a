/*
Copyright (C) 2020 David Anderson. All Rights Reserved.

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

/*  To print .debug_gnu_pubnames, .debug_gnu_typenames */

#include <config.h>
#include <stdio.h> /* FILE decl for dd_esb.h */

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
#include "print_debug_gnu.h"
#include "print_sections.h"

char *ikind_types[8] = {
    "none",
    "type",
    "variable",
    "function",
    "other",
    "unknown5",
    "unknown6",
    "unknown7" };

static int printed_infosize_error_a = FALSE;
static int printed_infosize_error_b = FALSE;

static int
print_block_entries(
    Dwarf_Gnu_Index_Head head,
    Dwarf_Unsigned blocknum,
    Dwarf_Unsigned entrycount,
    Dwarf_Unsigned max_offset,
    Dwarf_Error *error)
{
    Dwarf_Unsigned i = 0;
    int res = 0;

    printf("    [   ] offset     Kind        Name\n");

    for ( ; i < entrycount; ++i) {
        Dwarf_Unsigned offset_in_debug_info = 0;
        const char *name = 0;
        unsigned char flag = 0;
        unsigned char staticorglobal = 0;
        unsigned char typeofentry = 0;
        /*  flag is all 8 bits and staticorglobal
            and typeofentry were extracted from the flag.
            Present here so we can check all 8 bits
            are correct (lowest 4 should be zero).  */

        res = dwarf_get_gnu_index_block_entry(head,
            blocknum,i,&offset_in_debug_info,
            &name,&flag,&staticorglobal,&typeofentry,
            error);
        if (res == DW_DLV_ERROR) {
            return res;
        }
        if (res == DW_DLV_NO_ENTRY) {
            printf("  ERROR: Block %" DW_PR_DUu
                " entry %" DW_PR_DUu
                " does not exist though entry count"
                " is %" DW_PR_DUu
                ", something is wrong\n",
                blocknum,
                i,entrycount);
            glflags.gf_count_major_errors++;
            return res;
        }
        printf("    [%3" DW_PR_DUu "] 0x%" DW_PR_XZEROS DW_PR_DUx,
            i,offset_in_debug_info);
        if (!printed_infosize_error_a &&
            offset_in_debug_info >= max_offset) {
            printed_infosize_error_a = TRUE;
            printf("  ERROR: Block %" DW_PR_DUu
                " entry %" DW_PR_DUu
                " debug_info offset 0x%" DW_PR_DUx
                " is greater than the debug_info section size"
                " of 0x%" DW_PR_DUx
                ", something is wrong. (message will not repeat)\n",
                blocknum,
                i,offset_in_debug_info,max_offset);
            glflags.gf_count_major_errors++;
        }
        printf(" %s,%-8s",
            staticorglobal?"s":"g",
            ikind_types[0x7 & typeofentry]);
        printf(" %s",sanitized(name));
        printf("\n");
        if (flag&0xf) {
            printf("  ERROR: Block %" DW_PR_DUu
                " entry %" DW_PR_DUu " flag 0x%x. "
                "The lower bits are non-zero "
                "so there may be a corruption problem.",
                blocknum,i, flag);
            glflags.gf_count_major_errors++;
            printf("\n");
        }
    }
    return DW_DLV_OK;
}

int attrlist[] = {
DW_AT_GNU_dwo_name,
DW_AT_dwo_name,
DW_AT_comp_dir,
DW_AT_GNU_dwo_id,
0 };

static void
error_report(int errcode,
    const char *text,
    Dwarf_Error *error) {
    if (errcode == DW_DLV_ERROR) {
        printf("  ERROR: %s"
            ", ignoring other attributes here: %s\n",
            text,
            dwarf_errmsg(*error));
        glflags.gf_count_major_errors++;
        return;
    } else {
        printf("  ERROR impossible DW_DLV_NO_ENTRY: %s"
            ", ignoringother attributes here. \n",
            text);
    }
    return;
}
static void
print_selected_attributes(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Half version,
    Dwarf_Half offset_size,
    Dwarf_Error *error)
{
    int res = 0;
    int i = 0;

    for ( ; attrlist[i]; ++i) {
        Dwarf_Attribute attr = 0;
        int attrid = 0;
        const char * atname = 0;
        Dwarf_Half form = 0;
        enum Dwarf_Form_Class fclass = 0;
        char *formstring = 0;
        int print_str = FALSE;
        struct esb_s m;
        Dwarf_Sig8 sig;

        attrid = attrlist[i];
        res = dwarf_attr(die,attrid,&attr,error);
        if (res == DW_DLV_ERROR) {
            error_report(res,"dwarf_attr() returned error",error);
            dwarf_dealloc_error(dbg,*error);
            *error = 0;
            return;
        }
        if (res == DW_DLV_NO_ENTRY) {
            continue;
        }
        /* ok, this attribute is present */
        atname = get_AT_name(attrid);
        res = dwarf_whatform(attr,&form,error);
        if (res == DW_DLV_ERROR) {
            error_report(res,"dwarf_whatform() problem: ",error);
            dwarf_dealloc_error(dbg,*error);
            dwarf_dealloc_attribute(attr);
            *error = 0;
            return;
        }
        if (res == DW_DLV_NO_ENTRY) {
            /* impossible, cannot get here */
            dwarf_dealloc_attribute(attr);
            continue;
        }
        esb_constructor(&m);
        fclass = dwarf_get_form_class(version,attrid,
            offset_size,form);
        if (fclass == DW_FORM_CLASS_STRING) {
            res = dwarf_formstring(attr,&formstring,error);
            if (res == DW_DLV_OK) {
                print_str = TRUE;
                esb_append(&m,formstring);
            } else {
                error_report(res,"dwarf_formstring() returned error",
                    error);
                if (res == DW_DLV_ERROR) {
                    dwarf_dealloc_error(dbg,*error);
                    *error = 0;
                }
                dwarf_dealloc_attribute(attr);
                esb_destructor(&m);
                break;
            }
        } else if (fclass == DW_FORM_CLASS_CONSTANT)  {
            if (form == DW_FORM_data8) {
                res = dwarf_formsig8_const(attr,&sig,error);
                if (res == DW_DLV_OK) {
                    print_str = TRUE;
                    format_sig8_string(&sig,&m);
                } else {
                    error_report(res,"dwarf_formsig8_const() "
                        "returned error",error);
                    if (res == DW_DLV_ERROR) {
                        dwarf_dealloc_error(dbg,*error);
                        *error = 0;
                    }
                    esb_destructor(&m);
                    dwarf_dealloc_attribute(attr);
                    break;
                }
            }
        } else if (fclass == DW_FORM_CLASS_REFERENCE) {
            /* includes DW_FORM_ref_sig8, DW_FORM_ref* */
            if (form == DW_FORM_ref_sig8) {
                res = dwarf_formsig8(attr,&sig,error);
                if (res == DW_DLV_OK) {
                    print_str = TRUE;
                    esb_constructor(&m);
                    format_sig8_string(&sig,&m);
                } else {
                    error_report(res,"dwarf_formsig8() "
                        "problem error",error);
                    if (res == DW_DLV_ERROR) {
                        dwarf_dealloc_error(dbg,*error);
                        *error = 0;
                    }
                    esb_destructor(&m);
                    dwarf_dealloc_attribute(attr);
                    break;
                }
            }
        }
        if (print_str) {
            printf("  %-18s                  : %s\n",
                atname,esb_get_string(&m));
        }
        dwarf_dealloc_attribute(attr);
        esb_destructor(&m);
    }
}

static int
print_die_basics(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Error *error)
{
    int res = 0;
    Dwarf_Half tag = 0;
    Dwarf_Half version = 0;
    Dwarf_Bool is_info = 0;
    Dwarf_Bool is_dwo = 0;
    Dwarf_Half offset_size = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Half extension_size = 0;
    Dwarf_Sig8 *signature = 0;
    Dwarf_Off  offset_of_length = 0;
    Dwarf_Unsigned  total_byte_length = 0;

    res = dwarf_cu_header_basics(die, &version,
        &is_info,&is_dwo,
        &offset_size,&address_size,&extension_size,
        &signature,&offset_of_length,&total_byte_length,error);
    if (res != DW_DLV_OK) {
        if (res == DW_DLV_ERROR) {
            printf("ERROR: Cannot access compilation unit data: %s",
                dwarf_errmsg(*error));
            dwarf_dealloc_error(dbg,*error);
            *error = 0;
        } else {
            printf("ERROR:  Cannot access compilation unit data"
                "No such found");
        }
        glflags.gf_count_major_errors++;
        return DW_DLV_OK;
    }
    printf("  Compilation unit data follows\n");
    printf("  CU version                          : %d\n",version);
    if (!is_info) {
        printf("  CU  section is .debug_types");
    }
    printf("  CU section is dwo?                  : %s\n",
        is_dwo?"yes":"no");
    printf("  CU offset size                      : %u\n",
        offset_size);
    printf("  CU extension size                   : %u\n",
        extension_size);
    printf("  CU address size                     : %u\n",
        address_size);
    printf("  CU beginning offset                 : 0x%"
        DW_PR_XZEROS DW_PR_DUx "\n",
        offset_of_length);
    printf("  CU total length                     : 0x%"
        DW_PR_XZEROS DW_PR_DUx "\n",
        total_byte_length);

    if (signature) {
        struct esb_s m;
        char buf[24];

        esb_constructor_fixed(&m,buf,sizeof(buf));
        printf("  CU signature                        : ");
        format_sig8_string(signature,&m);
        printf("%s\n", esb_get_string(&m));
        esb_destructor(&m);
    }
    res = dwarf_tag(die,&tag,error);
    if (res != DW_DLV_OK) {
        if (res == DW_DLV_ERROR) {
            printf("ERROR: Cannot access DIE tag  ERROR: %s\n",
                dwarf_errmsg(*error));
            dwarf_dealloc_error(dbg,*error);
            *error = 0;
        } else {
            printf("ERROR:  Cannot access DIE tag "
                "No such found\n");
        }
        printf("\n");
        glflags.gf_count_major_errors++;
    } else {
        const char *actual_tag_name = 0;

        actual_tag_name = get_TAG_name(tag);
        printf("  CU die TAG                          : "
            "%s\n", actual_tag_name);
    }
    print_selected_attributes(dbg,die,version,offset_size,error);
    return DW_DLV_OK;
}

/*  max_offset can be zero if no .debug_info
    section present. */
static void
note_info_errors(Dwarf_Unsigned i,
    Dwarf_Unsigned max_offset,
    Dwarf_Unsigned offset_into_debug_info,
    Dwarf_Unsigned size_of_debug_info_area)
{
    if (printed_infosize_error_b) {
        return;
    }
    if (size_of_debug_info_area > max_offset) {
        printf("  ERROR: Block %" DW_PR_DUu
            " required size of .debug_info"
            " is %" DW_PR_DUu
            " (0x%" DW_PR_DUx ")"
            " which is greater than the size of "
            ".debug_info of %" DW_PR_DUu
            " bytes (message will not repeat)\n",
            i,
            size_of_debug_info_area,size_of_debug_info_area,
            max_offset);
        printed_infosize_error_b = TRUE;
        glflags.gf_count_major_errors++;
        return;
    }
    if (offset_into_debug_info >= max_offset) {
        printf("  ERROR: Block %" DW_PR_DUu
            " required offset into .debug_info"
            " is %" DW_PR_DUu
            " (0x%" DW_PR_DUx ")"
            " which is greater than the size of "
            ".debug_info of %" DW_PR_DUu
            " bytes (message will not repeat)\n",
            i,offset_into_debug_info,
            offset_into_debug_info,
            max_offset);
        printed_infosize_error_b = TRUE;
        glflags.gf_count_major_errors++;
        return;
    }
    if ((size_of_debug_info_area+offset_into_debug_info)
        > max_offset) {
        printf("  ERROR: Block %" DW_PR_DUu
            " required offset+size into .debug_info"
            " is %" DW_PR_DUu
            " which is greater than the size of "
            ".debug_info of %" DW_PR_DUu
            " bytes (message will not repeat)\n",
            i,offset_into_debug_info+size_of_debug_info_area,
            max_offset);
        printed_infosize_error_b = TRUE;
        glflags.gf_count_major_errors++;
        return;
    }
}

static int
print_all_blocks(Dwarf_Debug dbg,
    Dwarf_Gnu_Index_Head head,
    Dwarf_Unsigned       block_count,
    Dwarf_Error         *error)
{
    Dwarf_Unsigned i = 0;
    int res = 0;

    Dwarf_Unsigned max_offset = get_info_max_offset(dbg);

    for ( ; i < block_count; ++i) {
        Dwarf_Unsigned block_length            = 0;
        Dwarf_Half version                     = 0;
        Dwarf_Unsigned offset_into_debug_info  = 0;
        Dwarf_Unsigned size_of_debug_info_area = 0;
        Dwarf_Unsigned entrycount              = 0;

        res = dwarf_get_gnu_index_block(head,i,
            &block_length,&version,
            &offset_into_debug_info,
            &size_of_debug_info_area,
            &entrycount,error);
        if (res == DW_DLV_NO_ENTRY) {
            printf("  ERROR: Block %" DW_PR_DUu
                " does not exist though block count"
                " is %" DW_PR_DUu
                ", something is wrong\n",
                i,block_count);
            glflags.gf_count_major_errors++;
            return res;
        }
        if (res == DW_DLV_ERROR) {
            return res;
        }
        printf("  Blocknumber                         : "
            "%" DW_PR_DUu "\n",i);
        printf("  Block length                        : "
            "%" DW_PR_DUu "\n",block_length);
        printf("  Version                             : "
            "%u\n",version);
        printf("  Offset into .debug_info section     : "
            "0x%" DW_PR_XZEROS DW_PR_DUx "\n",offset_into_debug_info);
        printf("  Size of area in .debug_info section : "
            "%" DW_PR_DUu "\n",size_of_debug_info_area);
        note_info_errors(i,max_offset,offset_into_debug_info,
            size_of_debug_info_area);
        printf("  Number of entries in block          : "
            "%" DW_PR_DUu "\n",entrycount);
        /*  The CU offsets appear to be those in
            the executable here. Not in
            any dwo object. The offsets within
            the entries in a block are a different
            story and some of that seems odd,
            the content names many things  in libraries,
            not just the executable or its dwo. ?
            */
        res = dwarf_get_cu_die_offset_given_cu_header_offset_b(
            dbg,offset_into_debug_info,/*is_info = */ TRUE,
            &offset_into_debug_info,error);
        if (res != DW_DLV_OK) {
            printf("  ERROR: Block %" DW_PR_DUu
                " has an invalid .debug_info offset of "
                "0x%" DW_PR_DUx
                ", something is wrong\n",
                i,offset_into_debug_info);
            if (res == DW_DLV_ERROR) {
                dwarf_dealloc_error(dbg,*error);
                *error = 0;
            }
            glflags.gf_count_major_errors++;
        } else {
            Dwarf_Die die = 0;
            Dwarf_Bool is_info = TRUE;
            res = dwarf_offdie_b(dbg,offset_into_debug_info,
                is_info, &die,error);
            if (res != DW_DLV_OK) {
                printf("  ERROR: Block %" DW_PR_DUu
                    " cu DIE offset 0x%" DW_PR_DUx
                    " is not a valid DIE offset in .debug_info\n",
                    i, offset_into_debug_info);
                if (res == DW_DLV_ERROR) {
                    dwarf_dealloc_error(dbg,*error);
                    *error = 0;
                }
                glflags.gf_count_major_errors++;
            } else {
                /* Always returns DW_DLV_OK */
                print_die_basics(dbg, die, error);
                printf("\n");
                dwarf_dealloc_die(die);
            }
        }
        res = print_block_entries(head,i,entrycount,
            max_offset,error);
        if (res == DW_DLV_ERROR) {
            return res;
        }
    }
    return DW_DLV_OK;
}

/*  November 25,2020: gdb 10.2 binutils source
    can print these sections
    but gdb does not, AFAICT, use this at all.
    (binutils can print it, as can we).
    The Block offset is part of the skeleton, and refers
    to the skeleton CU DIEs (when
    that is involved) but the individual item offsets
    are referring to I-do-not-know-what.
    Block zero refers to the single CU_DIE in
    the .dwo file. The others....?
    Nothing suggests how things actually connect up. */
int
print_debug_gnu(Dwarf_Debug dbg,
    Dwarf_Error *error)
{
    int res = 0;
    Dwarf_Gnu_Index_Head head = 0;
    Dwarf_Bool for_pubnames = TRUE;
    Dwarf_Unsigned block_count = 0;
    const char *stdname = 0;
    char buf[DWARF_SECNAME_BUFFER_SIZE];
    struct esb_s truename;
    unsigned int i = 0;

    printed_infosize_error_a = FALSE;
    printed_infosize_error_b = FALSE;
    for (i = 0; i < 2; i++) {
        esb_constructor_fixed(&truename,buf,
            DWARF_SECNAME_BUFFER_SIZE);
        if (!i) {
            glflags.current_section_id = DEBUG_GNU_PUBNAMES;
            for_pubnames = TRUE;
            stdname =  ".debug_gnu_pubnames";
        } else {
            for_pubnames = FALSE;
            glflags.current_section_id = DEBUG_GNU_PUBTYPES;
            stdname = ".debug_gnu_pubtypes";
        }
        get_true_section_name(dbg,stdname, &truename,TRUE);
        res = dwarf_get_gnu_index_head(dbg,for_pubnames,
            &head, &block_count,error);
        if (res == DW_DLV_ERROR) {
            glflags.gf_count_major_errors++;
            printf("ERROR: problem reading %s. %s\n",
                sanitized(esb_get_string(&truename)),
                dwarf_errmsg(*error));
            dwarf_dealloc_error(dbg,*error);
            *error = 0;
            continue;
        } else if (res == DW_DLV_NO_ENTRY) {
            continue;
        }
        printf("\n%s with %" DW_PR_DUu
            " blocks of names\n",
            sanitized(esb_get_string(&truename)),
            block_count);
        res = print_all_blocks(dbg,head,block_count,error);
        if (res == DW_DLV_ERROR) {
            glflags.gf_count_major_errors++;
            printf("ERROR: problem reading %s. %s\n",
                sanitized(esb_get_string(&truename)),
                dwarf_errmsg(*error));
            dwarf_dealloc_error(dbg,*error);
            *error = 0;
        } else if (res == DW_DLV_NO_ENTRY) {
            /* impossible */
        } else {
            /* normal */
        }
        dwarf_gnu_index_dealloc(head);
        esb_destructor(&truename);
    }
    return DW_DLV_OK;
}
