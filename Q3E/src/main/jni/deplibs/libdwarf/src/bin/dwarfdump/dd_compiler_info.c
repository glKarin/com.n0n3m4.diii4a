/*
  Copyright 2010-2020 David Anderson. All rights reserved.

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
#include <stdio.h>  /* stdout fprintf() printf() */
#include <stdlib.h> /* exit() free() malloc() qsort() */
#include <string.h> /* memset() strcmp() stricmp()
    strlen() strncmp() */

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
#include "dd_makename.h"
#include "dd_sanitized.h"
#include "dd_safe_strcpy.h"
#include "dd_command_options.h"
#include "dd_compiler_info.h"
#include "libdwarf_private.h" /* For malloc/calloc debug */

/* Record compilers  whose CU names have been seen.
   Full CU names recorded here, though only a portion
   of the name may have been checked to cause the
   compiler data to be entered here.

   The detected/targeted arrays leave index 0 unused.
   There is no particularly good reason for this.
   current_compiler starts at -1, and is legitimately
   1 through COMPILER_TABLE_MAX-1.

   compilers_targeted_count and compilers_detected_count
   are the actual count, so loops use things like
   for (i = 1 ; i <= compilers_detected_count; ++1)
   for example.
*/
static Compiler compilers_detected[COMPILER_TABLE_MAX];
static int compilers_detected_count = 0;

/*  compilers_targeted is a list of indications of compilers
    on which we wish error checking (and the counts
    of checks made and errors found).   We do substring
    comparisons, so the compilers_targeted name might be simply a
    compiler version number or a short substring of a
    CU producer name.
    The +1 guarantees we do not overstep the array.
*/
static Compiler compilers_targeted[COMPILER_TABLE_MAX];
static int compilers_targeted_count = 0;
static int current_compiler = -1;

/* Indicates if the current CU is a target */
static Dwarf_Bool current_cu_is_checked_compiler = TRUE;

static int
has_cu_producer_prefix(const char *prefix)
{
    if (strncmp(glflags.CU_producer,prefix,strlen(prefix)) == 0) {
        return TRUE;
    }
    return FALSE;
}

static void
PRINT_CHECK_RESULT(char *str,
    Compiler *pCompiler, Dwarf_Check_Categories category)
{
    Dwarf_Check_Result result = pCompiler->results[category];
    printf("%-24s%10d  %10d\n", str, result.checks, result.errors);
}

/* Print checks and errors for a specific compiler */
static void
print_specific_checks_results(Compiler *pCompiler)
{
    printf("\nDWARF CHECK RESULT\n");
    printf("<item>                    <checks>    <errors>\n");
    if (glflags.gf_check_pubname_attr) {
        PRINT_CHECK_RESULT("pubname_attr", pCompiler,
            pubname_attr_result);
    }
    if (glflags.gf_check_tag_attr) {
        PRINT_CHECK_RESULT("attr_tag", pCompiler, attr_tag_result);
        PRINT_CHECK_RESULT("attr_formclass", pCompiler,
            attr_formclass_result);
    }
    if (glflags.gf_check_tag_tree) {
        PRINT_CHECK_RESULT("tag_tree", pCompiler, tag_tree_result);
    }
    if (glflags.gf_check_type_offset) {
        PRINT_CHECK_RESULT("type_offset", pCompiler,
            type_offset_result);
    }
    if (glflags.gf_check_decl_file) {
        PRINT_CHECK_RESULT("decl_file", pCompiler, decl_file_result);
    }
    if (glflags.gf_check_ranges) {
        PRINT_CHECK_RESULT("ranges", pCompiler, ranges_result);
    }
    if (glflags.gf_check_lines) {
        PRINT_CHECK_RESULT("line_table", pCompiler, lines_result);
    }
    if (glflags.gf_check_fdes) {
        PRINT_CHECK_RESULT("fde_table", pCompiler, fde_duplication);
    }
    if (glflags.gf_check_aranges) {
        PRINT_CHECK_RESULT("aranges", pCompiler, aranges_result);
    }

    if (glflags.gf_check_names) {
        PRINT_CHECK_RESULT("names",pCompiler, names_result);
    }
    if (glflags.gf_check_frames) {
        PRINT_CHECK_RESULT("frames",pCompiler, frames_result);
    }
    if (glflags.gf_check_locations) {
        PRINT_CHECK_RESULT("locations",pCompiler,
            locations_result);
    }

    if (glflags.gf_check_harmless) {
        PRINT_CHECK_RESULT("harmless_errors", pCompiler,
            harmless_result);
    }

    if (glflags.gf_check_abbreviations) {
        PRINT_CHECK_RESULT("abbreviations", pCompiler,
            abbreviations_result);
    }

    if (glflags.gf_check_dwarf_constants) {
        PRINT_CHECK_RESULT("dwarf_constants",
            pCompiler, dwarf_constants_result);
    }

    if (glflags.gf_check_di_gaps) {
        PRINT_CHECK_RESULT("debug_info_gaps", pCompiler,
            di_gaps_result);
    }

    if (glflags.gf_check_forward_decl) {
        PRINT_CHECK_RESULT("forward_declarations",
            pCompiler, forward_decl_result);
    }

    if (glflags.gf_check_self_references) {
        PRINT_CHECK_RESULT("self_references",
            pCompiler, self_references_result);
    }

    /* Display attributes encoding results */
    if (glflags.gf_check_attr_encoding) {
        PRINT_CHECK_RESULT("attr_encoding", pCompiler,
            attr_encoding_result);
    }

    /*  Duplicated attributes  This is set means libdwarf did no
        check of duplicated attributes so dwarfdump must do it.
        Instead of a reversal it is showing which does the checks.
        See dwarfdump.c dwarf_set_no_attr_dup_check() call. */
    if (glflags.gf_no_check_duplicated_attributes) {
        PRINT_CHECK_RESULT("duplicated_attributes",
            pCompiler, duplicated_attributes_result);
    }
    if (glflags.gf_check_functions) {
        PRINT_CHECK_RESULT("function_coverage",
            pCompiler, check_functions_result);
    }

    PRINT_CHECK_RESULT("** Summarize **",pCompiler,
        total_check_result);
    fflush(stdout);
}

/*  Add a CU name to the current compiler entry, specified by the
    'current_compiler'; the name is added to the 'compilers_detected'
    table and is printed if the '-P' or '--print-producers'
    option is specified in the
    command line. */
void
add_cu_name_compiler_target(char *name)
{
    a_name_chain *cu_last = 0;
    a_name_chain *nc = 0;
    Compiler *pCompiler = 0;

    if (current_compiler < 1) {
        printf("ERROR Current  compiler set to %d, cannot add "
            "Compilation unit name.  Giving up.",current_compiler);
        exit(EXIT_FAILURE);
    }
    pCompiler = &compilers_detected[current_compiler];
    cu_last = pCompiler->cu_last;
    /* Record current cu name */
    nc = (a_name_chain *)malloc(sizeof(a_name_chain));
    if (!nc) {
        printf("ERROR Out of memory "
            "allocating compiler target %s "
            "(not saved)\n",name);
        glflags.gf_count_major_errors++;
        return;
    }
    nc->item = makename(name);
    nc->next = NULL;
    if (cu_last) {
        cu_last->next = nc;
    } else {
        pCompiler->cu_list = nc;
    }
    pCompiler->cu_last = nc;
}

/* Reset a compiler entry, so all fields are properly set */
void
reset_compiler_entry(Compiler *compiler)
{
    memset(compiler,0,sizeof(Compiler));
}

/*  Record which compiler was used (or notice we saw
    it before) and set a couple variables as
    a side effect (which are used all over
    in this one source file):

        current_cu_is_checked_compiler
        (used in checking_this_compiler() )

        current_compiler

    The compiler/producer name is from DW_AT_producer.
*/
void
update_compiler_target(const char *producer_name)
{
    Dwarf_Bool cFound = FALSE;
    int index = 0;

    dd_safe_strcpy(glflags.CU_producer,sizeof(glflags.CU_producer),
        producer_name,
        strlen(producer_name));
    current_cu_is_checked_compiler = FALSE;

    /* This list of compilers is just a start:
        GCC id : "GNU"
        SNC id : "SN Systems" */

    /* Find a compiler version to check */
    if (compilers_targeted_count) {
        for (index = 1; index <= compilers_targeted_count; ++index) {
            if (is_strstrnocase(glflags.CU_producer,
                compilers_targeted[index].name)) {
                compilers_targeted[index].verified = TRUE;
                current_cu_is_checked_compiler = TRUE;
                break;
            }
        }
    } else {
        /* Internally the strings do not include quotes */
        Dwarf_Bool snc_compiler = has_cu_producer_prefix("SN");
        Dwarf_Bool gcc_compiler = has_cu_producer_prefix("GNU");
        current_cu_is_checked_compiler =
            glflags.gf_check_all_compilers ||
            (snc_compiler && glflags.gf_check_snc_compiler) ||
            (gcc_compiler && glflags.gf_check_gcc_compiler) ;
    }

    /* Check for already detected compiler */
    for (index = 1; index <= compilers_detected_count; ++index) {
        const char *name = compilers_detected[index].name;
        if (
#if _WIN32
            !stricmp(name,glflags.CU_producer)
#else
            !strcmp(name, glflags.CU_producer)
#endif /* _WIN32 */
            ) {
            /* Set current compiler index */
            current_compiler = index;
            cFound = TRUE;
            break;
        } else {
        }
    }
    if (!cFound) {
        /* Record a new detected compiler name. */
        if ((compilers_detected_count + 1) < COMPILER_TABLE_MAX) {
            Compiler *pCompiler = 0;
            char *cmp = makename(glflags.CU_producer);
            /*  Set current compiler index,
                first compiler at position [1] */
            current_compiler = ++compilers_detected_count;
            pCompiler = &compilers_detected[current_compiler];
            reset_compiler_entry(pCompiler);
            pCompiler->name = cmp;
        }
    }
}

void
clean_up_compilers_detected(void)
{
    memset(&compilers_detected[0],0,
        COMPILER_TABLE_MAX*sizeof(Compiler));
    compilers_detected_count = 0;
}

/*  Are we checking for errors from the
    compiler of the current compilation unit?
*/
Dwarf_Bool
checking_this_compiler(void)
{
    /*  This flag has been update by 'update_compiler_target()'
        and indicates if the current CU is in a targeted compiler
        specified by the user. Default value is TRUE, which
        means test all compilers until a CU is detected. */
    return current_cu_is_checked_compiler;
}

static int
qsort_compare_compiler(const void *elem1,const void *elem2)
{
    Compiler cmp1 = *(Compiler *)elem1;
    Compiler cmp2 = *(Compiler *)elem2;
    int cnt1 = cmp1.results[total_check_result].errors;
    int cnt2 = cmp2.results[total_check_result].errors;

    if (cnt1 < cnt2) {
        return 1;
    }
    if (cnt1 > cnt2) {
        return -1;
    }
    /* When error counts match, sort on name. */
    {
        int v = strcmp(cmp2.name, cmp1.name);
        return v;
    }
}

/* Print a summary of checks and errors */
void
print_checks_results(void)
{
    int index = 0;
    Compiler *pCompilers;
    Compiler *pCompiler;

    /*  Sort based on errors detected; the first entry is reserved
        and unused count started at 1 */
    pCompilers = &compilers_detected[1];
    if (compilers_detected_count > 1) {
        qsort((void *)pCompilers,compilers_detected_count,
            sizeof(Compiler),qsort_compare_compiler);
    }

    /* Print list of CUs for each compiler detected */
    if (glflags.gf_producer_children_flag) {

        a_name_chain *nc = 0;
        a_name_chain *nc_next = 0;
        int count = 0;
        int total = 0;

        printf("\n*** CU NAMES PER COMPILER ***\n");
        for (index = 1; index <= compilers_detected_count; ++index) {
            pCompiler = &compilers_detected[index];
            printf("\n%02d: %s",index,sanitized(pCompiler->name));
            count = 0;
            for (nc = pCompiler->cu_list; nc; nc = nc_next) {
                printf("\n    %02d: '%s'",++count,
                sanitized(nc->item));
                nc_next = nc->next;
                free(nc);
            }
            total += count;
            printf("\n");
        }
        printf("\nDetected %d CU names\n",total);
    }

    /* Print error report only if errors have been detected */
    /* Print error report if the -kd option */
    if ((glflags.gf_do_check_dwarf && glflags.check_error) ||
        glflags.gf_check_show_results) {
        int count = 0;
        int compilers_not_detected = 0;
        int compilers_verified = 0;

        /* Find out how many compilers have been verified. */
        for (index = 1; index <= compilers_detected_count; ++index) {
            if (compilers_detected[index].verified) {
                ++compilers_verified;
            }
        }
        /* Find out how many compilers have been not detected. */
        for (index = 1; index <= compilers_targeted_count; ++index) {
            if (!compilers_targeted[index].verified) {
                ++compilers_not_detected;
            }
        }

        /* Print compilers detected list */
        printf(
            "\n%d Compilers detected:\n",compilers_detected_count);
        for (index = 1; index <= compilers_detected_count; ++index) {
            pCompiler = &compilers_detected[index];
            printf("%02d: %s\n",index,sanitized(pCompiler->name));
        }

        /*  Print compiler list specified by the user with the
            '-c<str>', that were not detected. */
        if (compilers_not_detected) {
            count = 0;
            printf(
                "\n%d Compilers not detected:\n",
                compilers_not_detected);
            for (index = 1; index <= compilers_targeted_count;
                ++index) {
                if (!compilers_targeted[index].verified) {
                    printf(
                        "%02d: '%s'\n",
                        ++count,
                        sanitized(compilers_targeted[index].name));
                }
            }
        }

        count = 0;
        printf("\n%d Compilers verified:\n",compilers_verified);
        for (index = 1; index <= compilers_detected_count; ++index) {
            pCompiler = &compilers_detected[index];
            if (pCompiler->verified) {
                printf("%02d: errors = %5d, %s\n",
                    ++count,
                    pCompiler->results[total_check_result].errors,
                    sanitized(pCompiler->name));
            }
        }

        /*  Print summary if we have verified compilers or
            if the -kd option used. */
        if (compilers_verified || glflags.gf_check_show_results) {
            /* Print compilers detected summary*/
            if (glflags.gf_print_summary_all) {
                count = 0;
                printf("\n*** ERRORS PER COMPILER ***\n");
                for (index = 1; index <= compilers_detected_count;
                    ++index) {
                    pCompiler = &compilers_detected[index];
                    if (pCompiler->verified) {
                        printf("\n%02d: %s",
                            ++count,sanitized(pCompiler->name));
                        print_specific_checks_results(pCompiler);
                    }
                }
            }

            /* Print general summary (all compilers checked) */
            printf("\n*** TOTAL ERRORS FOR ALL COMPILERS ***\n");
            print_specific_checks_results(&compilers_detected[0]);
        }
    }
    fflush(stdout);
}

void DWARF_CHECK_COUNT(Dwarf_Check_Categories category, int inc)
{
    Compiler * c = 0;

    c = &compilers_detected[0];
    c->results[category].checks += inc;
    c->results[total_check_result].checks += inc;
    if (current_compiler > 0 &&
        current_compiler <  COMPILER_TABLE_MAX) {
        c = &compilers_detected[current_compiler];
        c->results[category].checks += inc;
        c->results[total_check_result].checks += inc;
        c->verified = TRUE;
    }
}

void DWARF_ERROR_COUNT(Dwarf_Check_Categories category, int inc)
{
    Compiler * c = 0;

    c = &compilers_detected[0];
    c->results[category].errors += inc;
    c->results[total_check_result].errors += inc;
    if (current_compiler > 0 &&
        current_compiler <  COMPILER_TABLE_MAX) {
        c = &compilers_detected[current_compiler];
        c->results[category].errors += inc;
        c->results[total_check_result].errors += inc;
    }
}

Dwarf_Bool
record_producer(char *name)
{
    Dwarf_Bool recorded = FALSE;
    if ((compilers_targeted_count+1) < COMPILER_TABLE_MAX) {
        Compiler *pCompiler = 0;
        const char *cmp = 0;
        cmp = do_uri_translation(name,"-c<compiler name>");
        /*  First compiler at position [1] */
        compilers_targeted_count++;
        pCompiler = &compilers_targeted[compilers_targeted_count];
        reset_compiler_entry(pCompiler);
        pCompiler->name = cmp;
        glflags.gf_check_all_compilers = FALSE;
        recorded = TRUE;
    }
    return recorded;
}
