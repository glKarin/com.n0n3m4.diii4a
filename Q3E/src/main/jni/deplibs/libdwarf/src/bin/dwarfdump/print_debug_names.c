/*
Copyright 2017-2018 David Anderson. All rights reserved.

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
#include <stdio.h> /* FILE decl for dd_esb.h, printf etc */
#include <string.h>
#include <stdlib.h>

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_naming.h"
#include "dd_sanitized.h"
#include "dd_esb.h"
#include "dd_esb_using_functions.h"

#define ATTR_ARRAY_SIZE 10

struct Dnames_Abb_Check_s {
    Dwarf_Unsigned ac_code;
    Dwarf_Unsigned ac_codeoffset;
    Dwarf_Unsigned ac_codelength;
    Dwarf_Unsigned ac_pairscount;
    Dwarf_Unsigned ac_nametablerefs;
    Dwarf_Half ac_tag;
};
static        Dwarf_Unsigned      abblist_nexttouse = 0;
static        Dwarf_Unsigned      abblist_count     = 0;
static        Dwarf_Unsigned      abblist_ab_table_len = 0;
static struct Dnames_Abb_Check_s *abblist = 0;

static void
reset_abblist(void)
{
    if (abblist) {
        free(abblist);
        abblist = 0;
    }
    abblist_count = 0;
    abblist_ab_table_len = 0;
    abblist_nexttouse = 0;
}

#if 0 /*debugging only */
static void
dump_table(const char *msg,unsigned long count,
    Dwarf_Half *idx, Dwarf_Half *form)
{
    unsigned long i = 0;
    for ( ; i < count;    ++i) {
        printf("[%lu] %x %x\n",i,idx[i],form[i]);
    }
}
#endif

static void
printindent(unsigned int l)
{
    unsigned int i = 0;
    for ( ; i < l; ++i) {
        printf(" ");
    }
}

static const Dwarf_Sig8 zerosig;
/*  The table entries here are indexed starting at 0
    Whereas names table area indexed starting at 1.  */
static int
print_cu_table(unsigned int indent,Dwarf_Dnames_Head dn,
    const char     *type,
    Dwarf_Unsigned  offset_count,
    Dwarf_Unsigned  sig_count,
    Dwarf_Bool     *has_single_cu_offset,
    Dwarf_Unsigned *single_cu_offset,
    Dwarf_Error    *error)
{
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned totalcount = offset_count+sig_count;
    int res = 0;
    Dwarf_Bool formtu = FALSE;

    if (type[0] == 't' && type[1] == 'u' &&
        type[2] == 0) {
        formtu = TRUE;
    } else if (type[0] == 'c' && type[1] == 'u' &&
        type[2] == 0) {
        formtu = FALSE;
    } else {
        printf("\nERROR: Calling print_cu_table with type"
            "%s is invalid. Must be tu or cu ."
            "Not printing this cu table set\n",
            type);
        glflags.gf_count_major_errors++;
        return DW_DLV_NO_ENTRY;
    }
    if (formtu) {
        printf("\n");
        printindent(indent);
        printf("%s List. Entry count: %" DW_PR_DUu
        " (local tu count %" DW_PR_DUu
        ",foreign tu count %" DW_PR_DUu
        ")\n",type,totalcount,offset_count,sig_count);
    } else {
        printindent(indent);
        printf("%s List. Entry count: %" DW_PR_DUu "\n",
            type,totalcount);
    }
    for (i = 0 ; i < totalcount; ++i) {
        Dwarf_Unsigned offset = 0;
        Dwarf_Sig8     signature;

        signature = zerosig;
        res = dwarf_dnames_cu_table(dn,type,i,
            &offset, &signature,error);
        if (res == DW_DLV_NO_ENTRY) {
            break;
        }
        if (res == DW_DLV_ERROR) {
            return res;
        }
        if (i == 0 && !formtu) {
            *single_cu_offset = offset;
            *has_single_cu_offset = TRUE;
        }
        /*  Could equally test for non-zero offset here. */
        if (i < offset_count) {

            printindent(indent);
            printf("[%4" DW_PR_DUu "] ",i);
            printf("CU-offset:  0x%"
                DW_PR_XZEROS DW_PR_DUx "\n",
                offset);
            continue;
        }

        {
            static char sigarea[32];
            struct esb_s sig8str;

            esb_constructor_fixed(&sig8str,sigarea,sizeof(sigarea));
            format_sig8_string(&signature,&sig8str);
            printindent(indent);
            printf("[%4" DW_PR_DUu "] " ,i);
            printf("Signature:  %s\n",esb_get_string(&sig8str));
            esb_destructor(&sig8str);
        }
    }
    return DW_DLV_OK;
}

static int
print_buckets(unsigned int indent,Dwarf_Dnames_Head dn,
    Dwarf_Unsigned bucket_count,
    Dwarf_Error *error)
{
    int res = 0;
    Dwarf_Unsigned bn = 0;

    if (!bucket_count) {
        return DW_DLV_NO_ENTRY;
    }

    printf("\n");
    printindent(indent);
    printf("Bucket (hash) table entry count: "
        "%" DW_PR_DUu "\n",bucket_count);
    printindent(indent);
    printf("[ ]    nameindex collisioncount\n");
    for ( ; bn < bucket_count; ++bn) {
        Dwarf_Unsigned name_index = 0;
        Dwarf_Unsigned collision_count = 0;

        res = dwarf_dnames_bucket(dn,bn,&name_index,
            &collision_count,error);
        if (res == DW_DLV_ERROR) {
            return res;
        }
        if (res == DW_DLV_NO_ENTRY) {
            break;
        }
        printindent(indent);
        printf("[ %3" DW_PR_DUu "] ",bn);
        printf("%6" DW_PR_DUu " %6" DW_PR_DUu ,
            name_index,
            collision_count);
        printf("\n");
    }
    printf("\n");
    return DW_DLV_OK;
}

static void
P_Entry(const char * leader,Dwarf_Unsigned value)
{
    printf("   %13s  0x%" DW_PR_XZEROS DW_PR_DUx
        " (%8" DW_PR_DUu
        ")\n",
        leader,value,value);
}
static int
print_dnames_offsets(unsigned int indent,Dwarf_Dnames_Head dn,
    Dwarf_Error *error)
{
    Dwarf_Unsigned header_offset = 0;
    Dwarf_Unsigned cu_table_offset = 0;
    Dwarf_Unsigned tu_local_offset = 0;
    Dwarf_Unsigned foreign_tu_offset = 0;
    Dwarf_Unsigned buckets_offset = 0;
    Dwarf_Unsigned hashes_offset = 0;
    Dwarf_Unsigned stringoffsets_offset = 0;
    Dwarf_Unsigned entryoffsets_offset = 0;
    Dwarf_Unsigned abbrev_table_offset = 0;
    Dwarf_Unsigned entry_pool_offset = 0;
    int res = 0;

    printf("\n");
    printindent(indent);
    printf("Table Offsets \n");
    res = dwarf_dnames_offsets(dn,&header_offset,
        &cu_table_offset, &tu_local_offset, &foreign_tu_offset,
        &buckets_offset, &hashes_offset,
        &stringoffsets_offset,&entryoffsets_offset,
        &abbrev_table_offset,&entry_pool_offset,error);
    if (res == DW_DLV_ERROR) {
        return res;
    }
    P_Entry("Header        :",header_offset);
    P_Entry("CU table      :",cu_table_offset);
    P_Entry("TU local      :",tu_local_offset);
    P_Entry("Foreign TU    :",foreign_tu_offset);
    P_Entry("Buckets       :",buckets_offset);
    P_Entry("Hashes        :",hashes_offset);
    P_Entry("String Offsets:",stringoffsets_offset);
    P_Entry("Entry Offsets :",entryoffsets_offset);
    P_Entry("Abbrev Table  :",abbrev_table_offset);
    P_Entry("Entry Pool:   :",entry_pool_offset);
    return DW_DLV_OK;
}

static void
update_abblist_count(Dwarf_Unsigned ab_code,Dwarf_Half tag)
{
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned curcount = abblist_nexttouse;
    struct Dnames_Abb_Check_s *cur = abblist;

    if (!abblist) {
        /* Not counting. */
        return;
    }
    for ( ; i < curcount; ++i,++cur) {
        if (cur->ac_code == ab_code) {
            if (cur->ac_tag != tag) {
                printf("ERROR: Abbrev code %" DW_PR_DUu
                    "has tag 0x%x but internal array shows"
                    "tag 0x%x\n",ab_code,
                    tag,cur->ac_tag);
                glflags.gf_count_major_errors++;
            }
            break;
        }
    }
    if (i >= abblist_nexttouse) {
        printf("ERROR: Abbrev code %" DW_PR_DUu
            " does not appear in the abbrev table. Corrupt data\n",
            ab_code);
        glflags.gf_count_major_errors++;
        return;
    }
    cur->ac_nametablerefs += 1 ;
}

static void
insert_ab_in_abblist(
    Dwarf_Unsigned abbrev_code,
    Dwarf_Half     abbrev_tag,
    Dwarf_Unsigned abbrev_offset,
    Dwarf_Unsigned actual_attr_count)
{
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned curcount = abblist_nexttouse;
    struct Dnames_Abb_Check_s *cur = abblist;

    for ( ; i < curcount; ++i,++cur) {
        if (cur->ac_code == abbrev_code) {
            break;
        }
    }
    if (i >= curcount) {
        if (i >= abblist_count) {
            static int printed = FALSE;

            if (!printed) {
                printf("ERROR: Impossible, out of room for "
                    "abbrev list entry checking, count"
                    " is %" DW_PR_DUu "\n",abblist_count);
                glflags.gf_count_major_errors++;
                printed = TRUE;
            }
            return;
        }
        cur->ac_code = abbrev_code;
        cur->ac_tag = abbrev_tag;
        cur->ac_codeoffset = abbrev_offset;
        cur->ac_codelength = 0; /* unknown yet */
        cur->ac_pairscount = actual_attr_count;
        cur->ac_nametablerefs = 0;
        abblist_nexttouse++;
    } else {
        printf("ERROR: Impossible duplicate abbrev code "
            "at abbrev entry %" DW_PR_DUu "\n",i);
        glflags.gf_count_major_errors++;
    }
}

static void
print_abblist_use(unsigned int indent)
{
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned table_count = abblist_count;
    Dwarf_Unsigned table_byte_size  = abblist_ab_table_len;
    struct Dnames_Abb_Check_s *cur = 0;
    Dwarf_Off last_offset = 0;

    if (!abblist_count || !abblist) {
        return;
    }

    cur = abblist;
    for (i=0; i < table_count ; ++i,++cur) {
        Dwarf_Unsigned prevlen = 0;
        if (!i) {
            if (cur->ac_codeoffset) {
                printf("ERROR: Seemingly initial abbrev"
                    "is at non-zero offset 0x%" DW_PR_DUx "\n",
                    cur->ac_codeoffset);
                glflags.gf_count_major_errors++;
            }
            last_offset = cur->ac_codeoffset;
            continue;
        }
        if ( cur->ac_codeoffset  <= last_offset) {
            printf("ERROR: abbrev code offsets out of order"
                " 0x%" DW_PR_DUx
                " followed by 0x%" DW_PR_DUx "\n",
                last_offset,
                cur->ac_codeoffset);
            glflags.gf_count_major_errors++;
        }
        prevlen = cur->ac_codeoffset - last_offset;
        (cur-1)->ac_codelength = prevlen;
        last_offset = cur->ac_codeoffset;
    }
    /*  Pull back to last */
    --cur;
    /*  Adjust codelength for table_byte_size, by 1 */
    cur->ac_codelength =
        table_byte_size - cur->ac_codeoffset -1;
    cur = abblist;
    printindent(indent);
    printf("Abbreviation List: %" DW_PR_DUu " entries.\n",
        table_count);
    printf("       U is Number of name entries using this abbrev\n");
    printf("       P is Number of IDX/FORM pairs on this abbrev\n");
    printf("            not counting end of list 0\n");
    printindent(indent);

    printf("[   ]  code          tag                   "
        "  aboffs bytes U     P\n");
    for (i=0; i < table_count ; ++i,++cur) {
        const char *tagname= "<none>";
        printindent(indent);
        printf("[%3" DW_PR_DUu "] ",i);
        printf(" %4" DW_PR_DUu ,cur->ac_code);
        printf(" (0x%04" DW_PR_DUx ")" ,cur->ac_code);
        dwarf_get_TAG_name((unsigned int)cur->ac_tag,&tagname);
        printf(" %-25s",tagname);

        printf(" 0x%04" DW_PR_DUx ,cur->ac_codeoffset);
        printf(" %2" DW_PR_DUu ,cur->ac_codelength);
        printf("   %2" DW_PR_DUu ,cur->ac_nametablerefs);
        printf("    %2" DW_PR_DUu "\n",cur->ac_pairscount-1);
    }
}

/*  The abbrev_table_length here is in bytes, not entries.
    The name_table_count is in entries, indexed as
    1 through name_table_count. */
static int
print_dnames_abbrevtable(unsigned int indent,Dwarf_Dnames_Head dn,
    Dwarf_Unsigned abbrev_table_length /* bytes */)
{
    int res = DW_DLV_OK;
    Dwarf_Unsigned abbrev_offset     = 0;
    Dwarf_Unsigned abbrev_code       = 0;
    Dwarf_Unsigned abbrev_tag        = 0;
    Dwarf_Unsigned array_size        = ATTR_ARRAY_SIZE;
    static Dwarf_Half idxattr_array[ATTR_ARRAY_SIZE];
    static Dwarf_Half form_array[ATTR_ARRAY_SIZE];
    Dwarf_Unsigned actual_attr_count = 0;
    Dwarf_Unsigned i                 = 0;

    for (  ;res == DW_DLV_OK; ++i) {
        res = dwarf_dnames_abbrevtable(dn,i,
            &abbrev_offset,
            &abbrev_code, &abbrev_tag,
            array_size,
            idxattr_array,form_array,
            &actual_attr_count);
        if (res != DW_DLV_OK) {
            break;
        }
    }
    res = DW_DLV_OK;
    abblist_count = i;
    if (abblist_count) {
        abblist = calloc(abblist_count,
            sizeof(struct Dnames_Abb_Check_s));
        if (!abblist) {
            printf("ERROR: Unable to allocate %" DW_PR_DUu
                "entries of a struct to check "
                "for wasted abbrev space\n",
                abblist_count);
            glflags.gf_count_major_errors++;
        }
    }
    printf("\n");
    printindent(indent);
    printf("Debug Names abbreviation table entries per Name: length %"
        DW_PR_DUu " (0x%04" DW_PR_DUx ") bytes.\n",
        abbrev_table_length,
        abbrev_table_length);
    printindent(indent);
    printf("[NameIndex] abbrev_offset abbrev_code"
        "   count idxattr\n");
    res = DW_DLV_OK;
    for (i = 0  ; res == DW_DLV_OK; ++i) {
        Dwarf_Unsigned limit = 0;
        Dwarf_Unsigned k     = 0;
        const char *tagname = "<TAGunknown>";

        /*  Never returns DW_DLV_ERROR */
        res = dwarf_dnames_abbrevtable(dn,i,
            &abbrev_offset,
            &abbrev_code, &abbrev_tag,
            array_size,
            idxattr_array,form_array,
            &actual_attr_count);
        if (res == DW_DLV_NO_ENTRY) {
            break;
        }
        if (abblist) {
            insert_ab_in_abblist(abbrev_code,(Dwarf_Half)abbrev_tag,
                abbrev_offset,actual_attr_count);
        }
        dwarf_get_TAG_name((unsigned int)abbrev_tag,&tagname);
        printindent(indent+2);
        printf("[%4" DW_PR_DUu "] ",i);
        printf("     0x%" DW_PR_XZEROS DW_PR_DUx " ",abbrev_offset);
        printf("     0x%05" DW_PR_DUx ,abbrev_code);
        printf("     %3" DW_PR_DUu " ",actual_attr_count);
        printf("     0x%04" DW_PR_DUx " %s",abbrev_tag,tagname);
        printf("\n");
        limit = actual_attr_count;
        if (limit > ATTR_ARRAY_SIZE) {
            printf("   \nERROR: allowed %" DW_PR_DUu " pairs,"
                " But we have %" DW_PR_DUu "pairs!\n",
                array_size, actual_attr_count);
            glflags.gf_count_major_errors++;
        }
        printindent(indent+4);
        printf("[abbrindex] idxattr  form \n");
        for (k = 0; k < limit; ++k) {
            const char *idname = "<unknownidx>";
            const char *formname = "<unknownform>";
            Dwarf_Half a = idxattr_array[k];
            Dwarf_Half f = form_array[k];

            printindent(indent+4);
            printf("[%3" DW_PR_DUu "] ",k);
            printf("0x%04x ",a);
            printf("0x%04x ",f);
            if (a || f) {
                dwarf_get_IDX_name(a,&idname);
                printf("%-19s",idname);
                dwarf_get_FORM_name(f,&formname);
                printf("%15s",formname);
                if (!(a && f)){
                    printf("\nERROR: improper idx/form pair!\n");
                    glflags.gf_count_major_errors++;
                }
            } else {
                printf(" (end of list)");
            }
            printf("\n");
        }
    }
    return DW_DLV_OK;
}

static int
print_attr_array(unsigned int indent,
    Dwarf_Unsigned attr_count,
    Dwarf_Unsigned array_size,
    Dwarf_Half *   idxattr_array,
    Dwarf_Half *   form_array)
{
    Dwarf_Unsigned k = 0;
    Dwarf_Unsigned count = attr_count;
    if (!attr_count) {
        printindent(indent);
        printf("No idxattr/form content available\n");
        return DW_DLV_NO_ENTRY;
    }
    if (array_size < attr_count) {
        printindent(indent);
        printf("Array size %" DW_PR_DUu
            " but count is %" DW_PR_DUu
            " so some entries not available\n",
            array_size,attr_count);
        count = array_size;
    }
    printindent(indent);
    printf("[]    idxnum formnum    idxname            formname\n");
    for ( ; k < count; ++k) {
        const char *idname = 0;
        const char *formname = 0;
        Dwarf_Half a = idxattr_array[k];
        Dwarf_Half f = form_array[k];

        printindent(indent);
        printf("[%3" DW_PR_DUu "]", k);
        printf(" 0x%04u 0x%04u", a,f);
        if (k == (count-1)) {
            if (a || f) {
                printf("\nERROR: last entry should be 0,0"
                    "not 0x%x a 0x%xf \n",a,f);
                glflags.gf_count_major_errors++;
                break;
            } else {
                printf(" (end of list)\n");
                continue;
            }
        }
        dwarf_get_IDX_name(a,&idname);
        printf(" %-19s",idname);
        dwarf_get_FORM_name(f,&formname);
        printf("%15s",formname);
        printf("\n");
    }
    return DW_DLV_OK;
}

/*  Check die offset for correctness.
    Only dealing with local cu/tu (not foreighn tu)*/
static void
check_die_pub_offset( Dwarf_Debug dbg,
    Dwarf_Unsigned    cu_hd_offset,
    Dwarf_Unsigned    local_die_offset)
{
    Dwarf_Error error = 0;
    int sres = 0;
    Dwarf_Bool is_info = TRUE;
    Dwarf_Die itemdie = 0;
    Dwarf_Unsigned global_offset = local_die_offset+
        cu_hd_offset;
    Dwarf_Unsigned cuhdr_offset = 0;

    sres = dwarf_offdie_b(dbg,global_offset,
        is_info,&itemdie,&error);
    if (sres != DW_DLV_OK) {
        printf("ERROR No global DIE at cuhdroff="
            "0x%" DW_PR_DUx
            " + culocaldieoffset="
            "0x%" DW_PR_DUx
            " = "
            "0x%" DW_PR_DUx "\n\n",
            cuhdr_offset,local_die_offset,
            global_offset);
        if (sres == DW_DLV_ERROR) {
            dwarf_dealloc_error(dbg,error);
        }
        glflags.gf_count_major_errors++;
        error = 0;
    }  else {
        dwarf_dealloc_die(itemdie);
    }
}

/*  Uses local error, issues error when appropriate.
    Has no other side effect.
    Checking DIE offset . Messier than it should be. */
static void
_dwarf_check_dn_die_offset(Dwarf_Debug dbg,Dwarf_Dnames_Head dn,
    Dwarf_Bool     single_cu_case,
    Dwarf_Unsigned local_type_unit_count,
    Dwarf_Unsigned single_cu_offset,
    Dwarf_Unsigned local_die_offset,
    Dwarf_Bool has_cu_table_index,
    Dwarf_Bool has_tu_table_index,
    Dwarf_Unsigned cu_table_index,
    Dwarf_Unsigned tu_table_index)
{
    int            cres = 0;
    Dwarf_Error    chkerror = 0;
    Dwarf_Unsigned cu_hd_offset = 0;
    Dwarf_Sig8     sig8;

    if (!single_cu_case) {
        /* tu or multiple CUs */
        if (has_cu_table_index) {
            cres = dwarf_dnames_cu_table(dn,"cu",
                cu_table_index,&cu_hd_offset,&sig8,&chkerror);
            if (cres != DW_DLV_OK) {
                printf("\nERROR: Cannot get "
                    "dwarf_dnames_cu_table on cu \n");
                glflags.gf_count_major_errors++;
                if (cres == DW_DLV_ERROR) {
                    dwarf_dealloc_error(dbg,chkerror);
                }
                return;
            }
        } else {
            if (has_tu_table_index) {
                if (tu_table_index < local_type_unit_count) {
                    cres = dwarf_dnames_cu_table(dn,"tu",
                        tu_table_index,&cu_hd_offset,
                        &sig8,&chkerror);
                    if (cres != DW_DLV_OK) {
                        printf("\nERROR: Cannot get "
                            "dwarf_dnames_cu_table on tu \n");
                        glflags.gf_count_major_errors++;
                        if (cres == DW_DLV_ERROR) {
                            dwarf_dealloc_error(dbg,chkerror);
                        }
                        return;
                    }
                } else {
                    printf("\nERROR: debug_names tu_table_index >= "
                        "local_type_unit_count \n");
                        glflags.gf_count_major_errors++;
                        return;
                }
            }
        }
    } else {
        /*  Single unit case means CU */
        cu_hd_offset = single_cu_offset;
    }
    check_die_pub_offset(dbg,
        cu_hd_offset, local_die_offset);
    return;
}

#define MAXPAIRS 8 /* The standard defines 5.*/
static Dwarf_Half     idx_array[MAXPAIRS];
static Dwarf_Half     form_array[MAXPAIRS];
static Dwarf_Unsigned offsets_array[MAXPAIRS];
static Dwarf_Sig8     signatures_array[MAXPAIRS];

static int
print_name_values(unsigned int indent,
    Dwarf_Debug dbg,
    Dwarf_Dnames_Head dn,
    Dwarf_Unsigned name_index,
    Dwarf_Unsigned offset_in_entrypool,
    Dwarf_Unsigned local_type_unit_count,
    Dwarf_Bool     single_cu_case,
    Dwarf_Unsigned single_cu_offset,
    Dwarf_Error * error)
{
    int res = 0;
    Dwarf_Unsigned abbrev_code = 0;
    Dwarf_Half     tag         = 0;
    Dwarf_Unsigned value_count = 0;
    Dwarf_Unsigned index_of_abbrev = 0;
    Dwarf_Unsigned offset_of_initial_value = 0;
    Dwarf_Unsigned offset_next_entry_pool = 0;
    const char    *idname = "<DW_IDX-unknown>";
    Dwarf_Unsigned i = 0;
    const char    *tagname = "<TAGunknown";
    Dwarf_Unsigned cu_table_index = 0;
    Dwarf_Unsigned tu_table_index = 0;
    Dwarf_Unsigned local_die_offset = 0;
    Dwarf_Bool has_cu_table_index = FALSE;
    Dwarf_Bool has_tu_table_index = FALSE;

    res = dwarf_dnames_entrypool(dn,
        offset_in_entrypool,
        &abbrev_code,&tag,&value_count,&index_of_abbrev,
        &offset_of_initial_value,
        error);
    if (res != DW_DLV_OK) {
        return res;
    }
    printindent(indent);
    dwarf_get_TAG_name((unsigned int)tag,&tagname);
    printf(
        "Nameindex=%" DW_PR_DUu
        " abbrevcode=%" DW_PR_DUu
        " (0x%04" DW_PR_DUx ")"
        " abbrevindex=%" DW_PR_DUu
        "\n",
        name_index,
        abbrev_code,
        abbrev_code,
        index_of_abbrev);
    printindent(indent);
    printf(
        "Tag 0x%04x     %-16s\n",
        tag,tagname);
    printindent(indent);
    printf(
        "Valuecount %5" DW_PR_DUu
        "  valuesoffset 0x%04" DW_PR_DUx
        "\n",
        value_count, offset_of_initial_value);
    if (value_count > MAXPAIRS) {
        printf("\nERROR: The number of values in an entrypool"
            " entry is %" DW_PR_DUu
            " but  the max allowed is %d\n",value_count,MAXPAIRS);
        glflags.gf_count_major_errors++;
        return DW_DLV_OK;
    }
    update_abblist_count(abbrev_code,tag);
    res = dwarf_dnames_entrypool_values(dn,
        index_of_abbrev,
        offset_of_initial_value,
        value_count,
        idx_array,
        form_array,
        offsets_array,
        signatures_array,
        &single_cu_case,&single_cu_offset,
        &offset_next_entry_pool,
        error);
    if (res != DW_DLV_OK) {
        return res;
    }

    indent += 2;
    printindent(indent);
    printf("Entrypool Values. Entry count:%" DW_PR_DUu ".\n",
        value_count);
    if (single_cu_case) {
        printindent(indent);
        printf("Single CU case. CUoffset defaults to: 0x%"
            DW_PR_XZEROS DW_PR_DUx "\n",
            single_cu_offset);
    }
    printindent(indent);
    printf("[ ]  idxattr    idxname           value\n");
    for (i = 0; i < value_count; ++i) {
        Dwarf_Half idx = idx_array[i];

        printindent(indent);
        printf("[%2" DW_PR_DUu "] ",i);

        if (!idx) {
            if (i == (value_count-1)) {
                printf(" 0 (end of list)\n");
                continue;
            }
        }
        dwarf_get_IDX_name(idx,&idname);
        printf("     %2u %-19s ",idx,idname);
        switch(idx) {
        case DW_IDX_compile_unit:
            /*  compile units special-case a single CU, and
                this instance eliminates that special case here. */
            printf(" CUindex= %" DW_PR_DUu ,offsets_array[i]);
            cu_table_index = offsets_array[i];
            has_cu_table_index = TRUE;
            single_cu_case = FALSE;
            break;
        case DW_IDX_type_unit: {
            /*  type units do not special-case a single CU */
            printf(" typeunitindex= %" DW_PR_DUu
                " (0x%" DW_PR_XZEROS DW_PR_DUx ")",
                offsets_array[i],
                offsets_array[i]);
            has_tu_table_index = TRUE;
            tu_table_index = offsets_array[i];
            }
            break;
        case DW_IDX_die_offset: {
            printf(" DIEcuLocalOff=%" DW_PR_DUu
                " (0x%" DW_PR_XZEROS DW_PR_DUx ")",
                offsets_array[i],
                offsets_array[i]);
            local_die_offset = offsets_array[i];
            }
            break;
        case DW_IDX_parent: {
            Dwarf_Half f = form_array[i];
            if (f == DW_FORM_flag || f == DW_FORM_flag_present) {
                if (offsets_array[i] ) {
                    printf(" FORM-flag-present "
                        "(Parent generated but not indexed)");
                } else {
                    printf(" FORM_flag (false)");
                }
            } else {
                printf(" pooloffsetofparent=%" DW_PR_DUu
                    " (0x%" DW_PR_XZEROS DW_PR_DUx ")",
                    offsets_array[i],
                    offsets_array[i]);
            }
            }
            break;
        case DW_IDX_type_hash: {
            struct esb_s m;

            esb_constructor(&m);
            format_sig8_string((Dwarf_Sig8*)&signatures_array[i],
                &m);
            printf(" typehash= %s",esb_get_string(&m));
            esb_destructor(&m);
            break;
        }
        default: {
            printf("\nERROR: idxattr %u is unknown!\n",idx);
            glflags.gf_count_major_errors++;
        }
        }
        printf("\n");
    }
    _dwarf_check_dn_die_offset(dbg,dn,
        single_cu_case,
        local_type_unit_count,
        single_cu_offset,
        local_die_offset,
        has_cu_table_index,
        has_tu_table_index,
        cu_table_index,
        tu_table_index);
    return DW_DLV_OK;
}

static int
print_names_table(unsigned int indent,
    Dwarf_Debug dbg,
    Dwarf_Dnames_Head dn,
    Dwarf_Unsigned name_count,
    Dwarf_Unsigned bucket_count,
    Dwarf_Unsigned local_type_unit_count,
    Dwarf_Bool     has_single_cu_offset,
    Dwarf_Unsigned single_cu_offset,
    Dwarf_Error * error)
{
    Dwarf_Unsigned i = 1;
    int res                  = 0;
    Dwarf_Unsigned bucketnum = 0;
    Dwarf_Unsigned hashval   = 0;
    Dwarf_Unsigned offset_to_debug_str = 0;
    char * namestr          = 0;
    Dwarf_Unsigned offset_in_entrypool = 0;
    Dwarf_Unsigned abbrev_code = 0;
    Dwarf_Half abbrev_tag    = 0;
    Dwarf_Unsigned array_size = ATTR_ARRAY_SIZE;
    static Dwarf_Half nt_idxattr_array[ATTR_ARRAY_SIZE];
    static Dwarf_Half nt_form_array[ATTR_ARRAY_SIZE];
    Dwarf_Unsigned attr_count = 0;

    memset(nt_idxattr_array,0,sizeof(Dwarf_Half) * ATTR_ARRAY_SIZE);
    memset(nt_form_array,0,sizeof(Dwarf_Half) * ATTR_ARRAY_SIZE);
    printf("\n");
    printindent(indent);
    printf("Names Table, entry count %" DW_PR_DUu "\n",name_count);
    printindent(indent);
    printf("(Any DW_IDX_die_offset not in .debug_info"
        " will be reported as an error.)\n");
    for ( ; i <= name_count;++i) {
        const char *tagname = "<TAGunknown>";
        printindent(indent);
        res = dwarf_dnames_name(dn,i,
            &bucketnum, &hashval,
            &offset_to_debug_str,&namestr,
            &offset_in_entrypool, &abbrev_code,
            &abbrev_tag,
            array_size, nt_idxattr_array,
            nt_form_array,
            &attr_count,error);
        if (res == DW_DLV_ERROR) {
            return res;
        }
        if (res == DW_DLV_NO_ENTRY) {
            printf("[%4" DW_PR_DUu "] ",i);
            printf("\nERROR: NO ENTRY on name index "
                "%" DW_PR_DUu " is impossible \n",i);
            glflags.gf_count_major_errors++;
            printf("\n");
            continue;
        }
        printf("[Entry %4" DW_PR_DUu "] ",i);
        if (bucket_count) {
            printf("Bucket# %" DW_PR_DUu " ",bucketnum);
            printf("hash 0x%" DW_PR_XZEROS DW_PR_DUx " ",hashval);
        }
        printf("nameoffset=0x%" DW_PR_DUx , offset_to_debug_str);
        printf(" \"%s\"",namestr?sanitized(namestr):"<null>");
        printf("\n");
        printindent(indent+4);
        dwarf_get_TAG_name((unsigned int)abbrev_tag,&tagname);
        printf("EntrypoolOffset=0x%06" DW_PR_DUx ,
            offset_in_entrypool);
        printf(" abbrevcode=%4" DW_PR_DUu,
            abbrev_code);
        printf(" (0x%04" DW_PR_DUx ")",
            abbrev_code);
        printf(" attrcount=%" DW_PR_DUu,
            attr_count);
        printf(" arraysz=%" DW_PR_DUu "\n",
            array_size);
        printindent(indent+4);
        printf("Tag=0x%04x      %-16s",
            abbrev_tag,tagname);
        printf("\n");
        if (glflags.verbose) {
            print_attr_array(indent+2,
                attr_count,array_size,
                nt_idxattr_array, nt_form_array);
        }
        res = print_name_values(indent+6,
            dbg,
            dn,i,offset_in_entrypool,
            local_type_unit_count,
            has_single_cu_offset,
            single_cu_offset,
            error);
        if (res != DW_DLV_OK) {
            return res;
        }
    }
    return DW_DLV_OK;
}

static int
print_dname_record(Dwarf_Debug dbg,Dwarf_Dnames_Head dn,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned new_offset,
    Dwarf_Error *error)
{
    int res = 0;
    Dwarf_Unsigned comp_unit_count = 0;
    Dwarf_Unsigned local_type_unit_count = 0;
    Dwarf_Unsigned foreign_type_unit_count = 0;
    Dwarf_Unsigned bucket_count = 0;
    Dwarf_Unsigned name_count = 0;
    Dwarf_Unsigned abbrev_table_size = 0;
    Dwarf_Unsigned entry_pool_size = 0;
    Dwarf_Unsigned augmentation_string_size = 0;
    Dwarf_Unsigned section_size = 0;
    Dwarf_Half table_version = 0;
    Dwarf_Half offset_size = 0;
    char * augstring = 0;
    unsigned int indent = 0;
    Dwarf_Bool     has_single_cu_offset;
    Dwarf_Bool     has_single_tu_offset;
    Dwarf_Unsigned single_cu_offset = 0;
    Dwarf_Unsigned single_tu_offset = 0;

    res = dwarf_dnames_sizes(dn,&comp_unit_count,
        &local_type_unit_count,
        &foreign_type_unit_count,
        &bucket_count,
        &name_count,&abbrev_table_size,
        &entry_pool_size,&augmentation_string_size,
        &augstring,
        &section_size,&table_version,
        &offset_size,
        error);
    if (res != DW_DLV_OK) {
        return res;
    }
    abblist_ab_table_len = abbrev_table_size;
    printf("\n");
    printf("Name table offset       : 0x%"
        DW_PR_XZEROS DW_PR_DUx " (offset of header length field)\n",
        offset);
    printf("Next name table offset  : 0x%"
        DW_PR_XZEROS DW_PR_DUx "\n",
        new_offset);
    printf("Section size            : 0x%"
        DW_PR_XZEROS DW_PR_DUx "\n",
        section_size);
    printf("Table version           : %4u\n",
        table_version);
    printf("Comp unit count         : %4" DW_PR_DUu "\n",
        comp_unit_count);
    printf("Type unit count         : %4" DW_PR_DUu "\n",
        local_type_unit_count);
    printf("Foreign Type unit count : %4" DW_PR_DUu "\n",
        foreign_type_unit_count);
    printf("Bucket count            : %4" DW_PR_DUu "\n",
        bucket_count);
    printf("Name count              : %4" DW_PR_DUu "\n",
        name_count);
    printf("Abbrev table length     : %4" DW_PR_DUu
        " (0x%04" DW_PR_DUx ")\n",
        abbrev_table_size,
        abbrev_table_size);
    printf("Entry pool size         : %4" DW_PR_DUu
        " (0x%04" DW_PR_DUx ")\n",
        entry_pool_size,
        entry_pool_size);
    printf("Augmentation string size: %4" DW_PR_DUu
        " (0x%04" DW_PR_DUx ")\n",
        augmentation_string_size,
        augmentation_string_size);
    if (augmentation_string_size > 0) {
        printf("Augmentation string     : %s\n",
            sanitized(augstring));
    }
    if (glflags.verbose) {
        res = print_dnames_offsets(indent+2,dn,error);
        if (res == DW_DLV_ERROR) {
            return res;
        }
    }
    if (glflags.verbose) {
        print_dnames_abbrevtable(indent+2,dn,
            abbrev_table_size);
    }
    res = print_cu_table(indent+2,dn,"cu",comp_unit_count,
        0,&has_single_cu_offset,&single_cu_offset,error);
    if (res == DW_DLV_ERROR) {
        return res;
    }
    /*  There is no single offset default for TU indexes,
        only CU tables that have such a default. */
    res = print_cu_table(indent+2,dn,"tu",local_type_unit_count,
        foreign_type_unit_count,
        &has_single_tu_offset,&single_tu_offset,error);
    if (res == DW_DLV_ERROR) {
        return res;
    }
    res = print_buckets(indent+2,dn,bucket_count,error);
    if (res == DW_DLV_ERROR) {
        return res;
    }
    res = print_names_table(indent+2,dbg,dn,name_count,
        bucket_count,
        local_type_unit_count,
        has_single_cu_offset,
        single_cu_offset,error);
    if (res == DW_DLV_ERROR) {
        return res;
    }
    print_abblist_use(indent+2);
    return DW_DLV_OK;
}

int
print_debug_names(Dwarf_Debug dbg,Dwarf_Error *error)
{
    Dwarf_Dnames_Head dnhead = 0;
    Dwarf_Unsigned offset = 0;
    Dwarf_Unsigned new_offset = 0;
    int res = 0;
    const char * section_name = ".debug_names";
    struct esb_s truename;
    char buf[DWARF_SECNAME_BUFFER_SIZE];

    if (!dbg) {
        printf("\nERROR: Cannot print .debug_names, "
            "no Dwarf_Debug passed in\n");
        return DW_DLV_NO_ENTRY;
    }
    glflags.current_section_id = DEBUG_NAMES;
    /* Do nothing if not printing. */
    if (!glflags.gf_do_print_dwarf) {
        return DW_DLV_OK;
    }

    /*  Only print anything if we know it has debug names
        present. */
    reset_abblist();
    res = dwarf_dnames_header(dbg,offset,&dnhead,&new_offset,error);
    if (res == DW_DLV_NO_ENTRY) {
        reset_abblist();
        return res;
    }
    esb_constructor_fixed(&truename,buf,sizeof(buf));
    get_true_section_name(dbg,section_name,
        &truename,TRUE);
    printf("\n%s\n",sanitized(esb_get_string(&truename)));
    esb_destructor(&truename);

    while (res == DW_DLV_OK) {
        res = print_dname_record(dbg,dnhead,
            offset,new_offset,error);
        if (res != DW_DLV_OK) {
            dwarf_dealloc_dnames(dnhead);
            dnhead = 0;
            reset_abblist();
            return res;
        }
        offset = new_offset;
        dwarf_dealloc_dnames(dnhead);
        dnhead = 0;
        reset_abblist();
        res = dwarf_dnames_header(dbg,offset,&dnhead,
            &new_offset,error);
    }
    reset_abblist();
    return res;
}
