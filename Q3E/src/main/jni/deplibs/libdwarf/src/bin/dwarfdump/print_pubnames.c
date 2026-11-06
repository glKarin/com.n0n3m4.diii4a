/*
Copyright (C) 2000-2006 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright 2007-2010 Sun Microsystems, Inc. All rights reserved.
Portions Copyright 2009-2011 SN Systems Ltd. All rights reserved.
Portions Copyright 2008-2020 David Anderson. All rights reserved.

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

#include <string.h> /* strlen() */
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
#include "print_sections.h"
#include "dd_sanitized.h"

static int
print_all_pubnames_style_records(Dwarf_Debug dbg,
    const char *linetitle,
    const char * section_true_name,
    Dwarf_Global *globbuf,
    Dwarf_Signed count,
    Dwarf_Error *err);

/*  This unifies the code for some error checks to
    avoid code duplication.
*/
static void
check_info_offset_sanity(
    const char *sec,
    const char *field,
    char *global,
    Dwarf_Unsigned offset, Dwarf_Unsigned maxoff)
{
    if (maxoff == 0) {
        /* Lets make a heuristic check. */
        if (offset > 0xffffffff) {
            printf("Warning: section %s %s %s offset 0x%"
                DW_PR_XZEROS DW_PR_DUx " "
                "exceptionally large \n",
                sec, field, global, offset);
        }
        return;
    }
    if (offset >= maxoff) {
        printf("Warning: section %s %s %s offset 0x%"
            DW_PR_XZEROS  DW_PR_DUx " "
                "larger than max of 0x%" DW_PR_DUx "\n",
                sec, field, global,  offset, maxoff);
    }
}

/* Unified pubnames style output.
   The error checking here against maxoff may be useless
   (in that libdwarf may return an error if the offset is bad
   and we will not get called here).
   But we leave it in nonetheless as it looks sensible.
   In at least one gigantic executable such offsets turned out wrong.
*/
static int
print_pubname_style_entry(Dwarf_Debug dbg,
    Dwarf_Signed globi,
    Dwarf_Signed globcount,
    const char *line_title,
    char *name,
    Dwarf_Half dietag,
    Dwarf_Unsigned global_die_off,
    Dwarf_Unsigned cu_die_off,
    Dwarf_Unsigned global_cuh_offset,
    Dwarf_Unsigned maxoff,Dwarf_Error *err)
{
    Dwarf_Die die = NULL;
    Dwarf_Off die_CU_off = 0;
    int dres = 0;
    int ddres = 0;
    int cudres = 0;
    Dwarf_Bool is_info = TRUE;
    Dwarf_Unsigned cu_local_die_offset = 0;

    if (global_die_off >= global_cuh_offset) {
        /* ASSERT: Always true. */
        cu_local_die_offset = global_die_off - global_cuh_offset;
    }
    /*  get die at global_die_off, a check
        on offset correctness.  For this old section
        in DWARF only .debug_info could be appropriate,
        never .debug_types. Hence is_info = TRUE */
    dres = dwarf_offdie_b(dbg, global_die_off, is_info, &die, err);
        /*  Some llvm version puts the global die offset into
            pubnames with the result that
            we will get an error here but we just
            let that create an error,
            papering it over here by subtracting out
            the applicable debug_info CU header offset
            is problematic. */
    if (dres != DW_DLV_OK) {
        struct esb_s details;
        esb_constructor(&details);
        esb_append(&details,line_title);
        esb_append(&details," dwarf_offdie_b : "
            "die offset does not reference valid DIE ");
        esb_append_printf_u(&details,"at offset 0x%"
            DW_PR_DUx, global_die_off);
        esb_append(&details,".");
        print_error_and_continue(
            esb_get_string(&details), dres, *err);
        esb_destructor(&details);
        return dres;
    }

    /* get offset of die from its cu-header */
    ddres = dwarf_die_CU_offset(die, &die_CU_off, err);
    if (ddres != DW_DLV_OK) {
        struct esb_s details;
        esb_constructor(&details);
        esb_append(&details,line_title);
        esb_append(&details," cannot get CU die offset");
        print_error_and_continue(
            esb_get_string(&details), dres, *err);
        esb_destructor(&details);
        die_CU_off = 0;
        dwarf_dealloc(dbg, die, DW_DLA_DIE);
        return ddres;
    }

    /* Get die at offset cu_die_off to check its existence. */
    {
        Dwarf_Die cu_die = NULL;
        Dwarf_Error localerr = 0;
        cudres = dwarf_offdie_b(dbg, cu_die_off,is_info,
            &cu_die, &localerr);
        if (cudres != DW_DLV_OK) {
            printf("ERROR: dwarf_offdie_b called "
                " CU die offset 0x%" DW_PR_DUx
                " does not reference a valid CU DIE\n",
                cu_die_off);
            if (cudres == DW_DLV_ERROR) {
                dwarf_dealloc_error(dbg,localerr);
                localerr = 0;
            }
            glflags.gf_count_major_errors++;
        } else {
            /* It exists, all is well. */
            dwarf_dealloc_die(cu_die);
        }
    }
    /* Display offsets */
    if (glflags.gf_display_offsets) {
        /* Print 'name' at the end for better layout */
        if (!globi) {
            printf(" %s data.  %" DW_PR_DSd " %s\n",
                line_title,globcount,
                globcount==1?"entry":"entries");
            printf("  CUhdr      DIE        CU DIE     DIE\n");
            printf("  in sect    in CU      in sect    in sect\n");
        }
        printf(" 0x%" DW_PR_XZEROS DW_PR_DUx
            " 0x%" DW_PR_XZEROS DW_PR_DUx
            " 0x%" DW_PR_XZEROS DW_PR_DUx
            " 0x%" DW_PR_XZEROS DW_PR_DUx ,
            global_cuh_offset,
            cu_local_die_offset,
            cu_die_off,global_die_off);
        if (dietag) {
            const char * tagname = "";
            dwarf_get_TAG_name(dietag,&tagname);
            printf(" %-18s",tagname);
        }
#if 0 /* debugging only */
        printf("%s die-in-sect 0x%" DW_PR_XZEROS DW_PR_DUx
            ", cu-in-sect 0x%" DW_PR_XZEROS DW_PR_DUx ","
            " die-in-cu 0x%" DW_PR_XZEROS DW_PR_DUx
            ", cu-header-in-sect 0x%" DW_PR_XZEROS DW_PR_DUx ,
            line_title,
            global_die_off, cu_die_off,
            (Dwarf_Unsigned) die_CU_off,
            /*  Following is absolute offset of the
                beginning of the cu */
            global_cu_offset);
#endif
    }
    /* Print 'name' at the end for better layout */
    printf(" '%s'\n",name);
    dwarf_dealloc(dbg, die, DW_DLA_DIE);
    check_info_offset_sanity(line_title,
        "die offset", name, global_die_off, maxoff);
    check_info_offset_sanity(line_title,
        "die cu offset", name, die_CU_off, maxoff);
    check_info_offset_sanity(line_title,
        "cu offset", name,
        cu_local_die_offset, maxoff);
    return DW_DLV_OK;
}

static void
print_globals_header(
    Dwarf_Off      pub_section_hdr_offset,
    Dwarf_Unsigned length_size, /* from pubnames header */
    Dwarf_Unsigned length, /* from pubnames header */
    Dwarf_Unsigned version,
    Dwarf_Off info_header_offset,
    Dwarf_Unsigned info_length)
{
    printf("Pub section offset   0x%" DW_PR_XZEROS DW_PR_DUx
        " (%" DW_PR_DUu ")\n",
        pub_section_hdr_offset,pub_section_hdr_offset);
    printf("  offset size        0x%" DW_PR_XZEROS DW_PR_DUx
        " (%" DW_PR_DUu ")\n",
        length_size,
        length_size);
    printf("  length             0x%" DW_PR_XZEROS DW_PR_DUx
        " (%" DW_PR_DUu ")\n",
        length,
        length);
    printf("  version            0x%" DW_PR_XZEROS DW_PR_DUx
        " (%" DW_PR_DUu ")\n",
        version,
        version);
    printf("  info hdr offset    0x%" DW_PR_XZEROS DW_PR_DUx
        " (%" DW_PR_DUu ")\n",
        info_header_offset,
        info_header_offset);
    printf("  info hdr length    0x%" DW_PR_XZEROS DW_PR_DUx
        " (%" DW_PR_DUu ")\n",
        info_length,
        info_length);
}

static const int secid[] = {
DEBUG_PUBNAMES,
DEBUG_PUBTYPES,
DEBUG_FUNCNAMES,
DEBUG_TYPENAMES,
DEBUG_VARNAMES,
DEBUG_WEAKNAMES
};
static const char *typename[] = {
"global",
"pubtype",
"static-func",
"type",
"static-var",
"weakname",
};
static const char *stdsecname[] = {
".debug_pubnames",
".debug_pubtypes",
".debug_funcnames",
".debug_typenames",
".debug_varnames",
".debug_weaknames"
};
/* Get all the data in any .debug_pubnames style section.
*/
int
print_pubnames_style(Dwarf_Debug dbg,
    int category,
    Dwarf_Error *err)
{
    Dwarf_Global *globbuf = NULL;
    Dwarf_Signed count = 0;
    /* Offset to previous CU */
    char         tempsanbuf[ESB_FIXED_ALLOC_SIZE];
    struct esb_s unsanitname;
    char         tempsanbuf2[ESB_FIXED_ALLOC_SIZE];
    struct esb_s unsanitname2;
    char         sanbuf[ESB_FIXED_ALLOC_SIZE];
    struct esb_s sanitname;
    int          res = 0;
    int          trueres = 0;
    int          trueres2 = 0;
    const char  *tname = 0;
    const char  *stdsection = 0;

    if (category > DW_GL_WEAKS) {
        printf("ERROR: passing category to print_pubnames "
            "that is unusable and ignored: %d\n",category);
        return DW_DLV_OK;
    }
    glflags.current_section_id = secid[category];
    tname = typename[category];
    stdsection = stdsecname[category];
    if (glflags.verbose) {
        /* For best testing! */
        res = dwarf_return_empty_pubnames(dbg,1);
        if (res != DW_DLV_OK) {
            simple_err_return_msg_either_action(res,
                "ERROR: Erroneous "
                "libdwarf call "
                "of dwarf_return_empty_pubnames: "
                "dwarfdump internal error");
            return res;
        }
    }
    /*  Globals picks up global entries from the
        selected section as of 0.6.0 December 2022 */
    esb_constructor_fixed(&unsanitname,tempsanbuf,
        sizeof(tempsanbuf));
    esb_constructor_fixed(&unsanitname2,tempsanbuf2,
        sizeof(tempsanbuf2));
    esb_constructor_fixed(&sanitname,sanbuf,sizeof(sanbuf));
    trueres = get_true_section_name(dbg,stdsection,
        &unsanitname,TRUE);
    trueres2 =  DW_DLV_NO_ENTRY;
    if (category == DW_GL_GLOBALS) {
        trueres2= get_true_section_name(dbg,".debug_names",
            &unsanitname2,TRUE);
    }
    if (trueres == DW_DLV_NO_ENTRY) {
        if (trueres2 == DW_DLV_NO_ENTRY) {
            /*  unsanitname ok as is, nothing to print
                but the name of the empty section.  */
        } else {
            esb_empty_string(&unsanitname);
            esb_append(&unsanitname,esb_get_string(&unsanitname2));
        }
    } else {
        if (trueres2 == DW_DLV_NO_ENTRY) {
            /*  unsanitname ok as is, nothing to print
                but the name of the real section.  */
        } else {
            esb_append(&unsanitname," and ");
            esb_append(&unsanitname,esb_get_string(&unsanitname2));
        }
    }
    esb_destructor(&unsanitname2);

    /*  Sanitized cannot be safely reused,there is a static buffer,
        so we make a safe copy. */
    esb_append(&sanitname,sanitized(esb_get_string(&unsanitname)));
    esb_destructor(&unsanitname);
    if (glflags.gf_check_functions) {
        DWARF_CHECK_COUNT(check_functions_result,1);
        /*  For coverage of these two API functions,
            call directly. */
        switch(category) {
        case DW_GL_GLOBALS:
            res = dwarf_get_globals(dbg,
                &globbuf, &count, err);
            break;
        case DW_GL_PUBTYPES:
            res = dwarf_get_pubtypes(dbg,
                &globbuf, &count, err);
            break;
        default:
            res = dwarf_globals_by_type(dbg,category,
                &globbuf, &count, err);
            break;
        }
    } else {
        res = dwarf_globals_by_type(dbg,category,
            &globbuf, &count, err);
    }
    if (res == DW_DLV_NO_ENTRY) {
        return res;
    }
    if (res == DW_DLV_OK && !count) {
        dwarf_globals_dealloc(dbg,globbuf,count);
        return res;
    }
    if (glflags.gf_do_print_dwarf) {
        printf("\n%s\n",esb_get_string(&sanitname));
    }
    if (res == DW_DLV_ERROR) {
        esb_destructor(&sanitname);
        return res;
    }
    res = print_all_pubnames_style_records(dbg,
        tname,
        esb_get_string(&sanitname),
        globbuf,count,err);
    if (res == DW_DLV_ERROR) {
        struct esb_s m;

        dwarf_globals_dealloc(dbg,globbuf,count);
        esb_constructor(&m);
        esb_append(&m,"ERROR: "
            "failed reading pubnames style section ");
        esb_append_printf_s(&m,"%s ",stdsection);
        esb_append_printf_s(&m,"type %s ",tname);
        simple_err_return_msg_either_action(res,
            esb_get_string(&m));
        esb_destructor(&sanitname);
        esb_destructor(&m);
        return res;
    }
    if (res == DW_DLV_NO_ENTRY) {
        dwarf_globals_dealloc(dbg,globbuf,count);
        esb_destructor(&sanitname);
        return res;
    }
    esb_destructor(&sanitname);
    dwarf_globals_dealloc(dbg,globbuf,count);
    dwarf_return_empty_pubnames(dbg,0);
    return res;
}

static void
dd_check_globals_data(Dwarf_Debug dbg,
    Dwarf_Global glob,
    int       nres,
    char     *name,
    Dwarf_Off global_die_off)
{
    char       *check_name = 0;
    Dwarf_Off   cfglobal_die_offset = 0;
    Dwarf_Error cferr = 0;
    int         rescf = 0;

    if (!glflags.gf_check_functions) {
        return;
    }
    DWARF_CHECK_COUNT(check_functions_result,1);
    rescf = dwarf_globname(glob, &check_name,&cferr);
    if (rescf == nres) {
        if (nres == DW_DLV_OK) {
            if (strcmp(name,check_name)) {
                /* FAIL */
                DWARF_CHECK_ERROR(check_functions_result,
                    "Name mismatch with dwarf_globname");
            }
        }
    } else {
        DWARF_CHECK_ERROR(check_functions_result,
            "DW_DLV mismatch with dwarf_globname");
    }
    if (cferr) {
        DROP_ERROR_INSTANCE(dbg,rescf,cferr);
    }
    rescf = dwarf_global_die_offset(glob,
        &cfglobal_die_offset,&cferr);
    if (rescf == nres) {
        if (nres == DW_DLV_OK) {
            if (global_die_off != cfglobal_die_offset) {
                /* FAIL */
                DWARF_CHECK_ERROR(check_functions_result,
                    "Die offset mismatch with "
                    "dwarf_global_die_offset");
            }
        }
    } else {
        DWARF_CHECK_ERROR(check_functions_result,
            "DW_DLV mismatch with "
                "dwarf_global_die_offset");
    }
    if (cferr) {
        DROP_ERROR_INSTANCE(dbg,rescf,cferr);
    }
}

static void
dd_check_pubname_attr(Dwarf_Debug dbg,
    const char *section_true_name,
    Dwarf_Signed i,
    Dwarf_Bool is_info,
    char      *name,
    Dwarf_Off  global_die_off,
    Dwarf_Off  cu_die_off,
    Dwarf_Off *prev_cu_die_off)
{
    Dwarf_Bool  has_attr = 0;
    int         dres = 0;
    int         ares = 0;
    Dwarf_Die   die = 0;
    Dwarf_Error err = 0;

    if (!glflags.gf_check_pubname_attr) {
        return;
    }
    /*  We are processing a new set of pubnames
        for a different CU; get the producer ID,
        at 'cu_off' to see if we need to skip
        these pubnames */
    if (cu_die_off != *prev_cu_die_off) {
        char         proname[100];
        struct esb_s producername;
        Dwarf_Die    lcudie = 0;

        /* Record offset for previous CU */
        *prev_cu_die_off = cu_die_off;
        dres = dwarf_offdie_b(dbg, cu_die_off,
            is_info, &lcudie, &err);
        if (dres != DW_DLV_OK) {
            struct esb_s msge;

            esb_constructor(&msge);
            esb_append(&msge,
                "ERROR Accessing dwarf_offdie ");
            esb_append_printf_i(&msge,
                " f1df0or index %d in ",i);
            esb_append(&msge,section_true_name);
            esb_append(&msge,".");
            simple_err_return_msg_either_action(dres,
                esb_get_string(&msge));
            esb_destructor(&msge);
            DROP_ERROR_INSTANCE(dbg,dres,err);
            return;
        }
        /*  Get producer name for this CU
            and update compiler list */
        esb_constructor_fixed(&producername,proname,
            sizeof(proname));
        dres = get_producer_name(dbg,lcudie,cu_die_off,
            &producername,&err);
        dwarf_dealloc(dbg,lcudie,DW_DLA_DIE);
        if (dres == DW_DLV_ERROR) {
            DROP_ERROR_INSTANCE(dbg,dres,err);
            return;
        }
        update_compiler_target(
            esb_get_string(&producername));
        glflags.DIE_CU_overall_offset = cu_die_off;
        esb_destructor(&producername);
    }

    dres = dwarf_offdie_b(dbg, global_die_off,
        is_info, &die, &err);
    if (dres != DW_DLV_OK) {
        struct esb_s msge;

        esb_constructor(&msge);
        esb_append(&msge,"ERROR Accessing dwarf_offdie ");
        esb_append_printf_i(&msge," for index %d in ",i);
        esb_append(&msge,section_true_name);
        esb_append(&msge,". ");
        if (dres == DW_DLV_ERROR) {
            esb_append(&msge,dwarf_errmsg(err));
        } else {
            esb_append(&msge,"offdie returned DW_DLV_NO_ENTRY");
        }
        simple_err_return_msg_either_action(dres,
            esb_get_string(&msge));
        esb_destructor(&msge);
        DROP_ERROR_INSTANCE(dbg,dres,err);
        return;
    }
    ares =
        dwarf_hasattr(die, DW_AT_external,
        &has_attr, &err);
    if (ares == DW_DLV_ERROR) {
        struct esb_s msgd;

        esb_constructor(&msgd);
        esb_append(&msgd,"ERROR: hasattr on DW_AT_external"
            " from ");
        esb_append(&msgd,section_true_name);
        esb_append(&msgd," fails.");
        dwarf_dealloc(dbg, die, DW_DLA_DIE);
        /* print_error does not return */
        simple_err_return_msg_either_action(ares,
            esb_get_string(&msgd));
        esb_destructor(&msgd);
        DROP_ERROR_INSTANCE(dbg,dres,err);
        return;
    }
    /*  DW_DLV_NO_ENTRY is odd. Check that if checking. */
    /*  Check for specific compiler */
    if (checking_this_compiler()) {
        DWARF_CHECK_COUNT(pubname_attr_result,1);
        if (ares == DW_DLV_OK && has_attr) {
            /* Should the value of flag be examined? */
        } else {
            DWARF_CHECK_ERROR2(pubname_attr_result,name,
                "DIE pubname refers to does not have "
                "DW_AT_external");
        }
    }
    dwarf_dealloc(dbg, die, DW_DLA_DIE);
}

static int
print_all_pubnames_style_records(Dwarf_Debug dbg,
    const char *linetitle,
    const char * section_true_name,
    Dwarf_Global *globbuf,
    Dwarf_Signed count,
    Dwarf_Error *err)
{
    Dwarf_Unsigned maxoff = get_info_max_offset(dbg);
    Dwarf_Unsigned lastcudieoff = 0;
    Dwarf_Addr elf_max_address = 0;
    Dwarf_Signed i = 0;
    int          ares = 0;
    Dwarf_Bool   is_info = TRUE;
    Dwarf_Off    prev_cu_die_off = elf_max_address;

    ares = get_address_size_and_max(dbg,0,&elf_max_address,err);
    if (ares != DW_DLV_OK) {
        simple_err_return_msg_either_action(ares,
            "ERROR: print_pubnames style call to get "
            "address size and max address"
            " fails.");
        return ares;
    }

    for (i = 0; i < count; i++) {
        int nres = 0;
        int cures3 = 0;
        Dwarf_Off global_die_off = 0;
        Dwarf_Off cu_die_off = 0;
        Dwarf_Off global_cuh_off = 0;
        char *name = 0;
        Dwarf_Global  thisglobal = 0;

        /*  Turns the cu-local die_off in globbuf
            entry into a global die_off.  The cu_off
            returned is the offset of the CU DIE,
            not the CU header. */
        thisglobal = globbuf[i];
        nres = dwarf_global_name_offsets(thisglobal,
            &name, &global_die_off, &cu_die_off,
            err);
        dd_check_globals_data(dbg,thisglobal,nres,
            name,global_die_off);
        if (nres != DW_DLV_OK) {
            struct esb_s m;

            esb_constructor(&m);
            esb_append_printf_i(&m,
                "ERROR: dwarf_global_name_offsets for globals index "
                " %d ", i);
            esb_append_printf_i(&m," with globals count %d.",
                count);
            simple_err_return_msg_either_action(nres,
                esb_get_string(&m));
            esb_destructor(&m);
            return nres;
        }
        if (glflags.verbose) {
            /*  We know no global_die_off can be zero
                (except for the fake global created when
                the debug_pubnames for a CU has no actual entries)
                we do not need to check for i==0 to detect
                this is the initial global record and
                we want to print this pubnames section CU header. */
            if (lastcudieoff != cu_die_off) {
                Dwarf_Off pub_section_hdr_offset = 0;
                Dwarf_Unsigned pub_offset_size = 0;
                Dwarf_Unsigned pub_length = 0;
                Dwarf_Unsigned pub_version = 0;
                Dwarf_Off info_header_offset = 0;
                Dwarf_Unsigned info_length = 0;
                int            header_category = 0;

                nres = dwarf_get_globals_header(thisglobal,
                    &header_category,
                    &pub_section_hdr_offset,
                    &pub_offset_size,
                    &pub_length,&pub_version,
                    &info_header_offset,&info_length,err);
                if (nres != DW_DLV_OK) {
                    struct esb_s msge;

                    esb_constructor(&msge);
                    esb_append(&msge,
                        "ERROR Access dwarf_get_globals_header ");
                    esb_append_printf_i(&msge,
                        " for index %d in ",i);
                    esb_append(&msge,section_true_name);
                    esb_append(&msge,".");
                    simple_err_return_msg_either_action(nres,
                        esb_get_string(&msge));
                    esb_destructor(&msge);
                    return nres;
                }
                if (glflags.gf_do_print_dwarf) {
                    print_globals_header(pub_section_hdr_offset,
                        pub_offset_size,
                        pub_length,
                        pub_version,
                        info_header_offset,
                        info_length);
                }
            }
        }
        lastcudieoff = cu_die_off;
        if (glflags.verbose) {
            /* If verbose we can see a zero die_off. */
            if (!global_die_off  && !strlen(name)) {
                /*  A different and impossible cu die offset in case
                    of an empty pubnames CU. */
                continue;
            }
        }
        /*  This gets the CU header offset, which
            is the offset that global_die_off needs to be added to
            to calculate the DIE offset. Note that
            dwarf_global_name_offsets already did that
            addition properly so this call is just so
            we can print the CU header offset. */
        cures3 = dwarf_global_cu_offset(thisglobal,
            &global_cuh_off, err);
        if (cures3 != DW_DLV_OK) {
            struct esb_s msge;

            esb_constructor(&msge);
            esb_append(&msge,"ERROR Access dwarf_global_cu_offset ");
            esb_append_printf_i(&msge," for index %d in ",i);
            esb_append(&msge,section_true_name);
            esb_append(&msge,".");
            simple_err_return_msg_either_action(cures3,
                esb_get_string(&msge));
            esb_destructor(&msge);
            return cures3;
        }

        dd_check_pubname_attr(dbg,
            section_true_name,i,
            is_info,name,global_die_off,
            cu_die_off, &prev_cu_die_off);
        /* Now print name, after the test */
        if (glflags.gf_do_print_dwarf ||
            (glflags.gf_record_dwarf_error &&
            glflags.gf_check_verbose_mode)) {
            int res = 0;
            Dwarf_Half dietag =
                dwarf_global_tag_number(thisglobal);

            res  = print_pubname_style_entry(dbg,
                i,count,
                linetitle,
                name,
                dietag,
                global_die_off, cu_die_off,
                global_cuh_off,
                maxoff,err);
            if (res != DW_DLV_OK) {
                return res;
            }
            glflags.gf_record_dwarf_error = FALSE;
        }
    }
    return DW_DLV_OK;
}
