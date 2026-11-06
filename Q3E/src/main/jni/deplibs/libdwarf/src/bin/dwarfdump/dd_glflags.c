/*
Copyright (C) 2017-2020 David Anderson. All Rights Reserved.

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

/*  All the dwarfdump flags are gathered into a single
    global struct as it has been hard to know how many there
    were or what they were all for. */

#include <config.h>

#include <limits.h> /* INT_MAX */
#include <string.h> /* strcmp() strlen() */
#include <stdio.h> /* FILE decl for dd_esb.h */

#include "dwarf.h"
#include "libdwarf.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_esb.h"                /* For flexible string buffer. */
#include "dd_dwconf.h"
#include "dd_safe_strcpy.h"

#ifdef TRIVIAL_NAMING  /* for make rebuild */
struct glflags_s glflags;

#endif /*TRIVIAL_NAMING*/

static struct section_high_offsets_s _section_high_offsets_global;
static struct dwconf_s _config_file_data;

/* used in special_program_name() only */
static struct esb_s _newprogname;

static struct esb_s _cu_name;
static struct esb_s _config_file_path;
static struct esb_s _config_file_tiedpath;

void
init_global_flags(void)
{
    glflags.gf_debug_names_flag = FALSE;
    glflags.gf_info_flag = FALSE;
    glflags.gf_use_old_dwarf_loclist = FALSE;  /* This so both
        dwarf_loclist_n()  and dwarf_get_loclist_c()
        and the dwarf_loclist_from_expr
        variations can be
        tested. Defaults to new
        dwarf_get_loclist_c(). See -g option.
        The original IRIX dwarf_loclist() no longer tested
        as of October 2015. */

    glflags.gf_line_flag_selection = s2l;
    glflags.gf_line_flag = FALSE;

    /*  Setting this FALSE tells dwarfdump to use the old
        line-table interfaces.  using:
            -x line5=no
        The new interfaces allow for both two-level line tables
        and access to line table headers in case
        we have a DWARF5 skeleton line table (a line table
        header with no lines).
        */
    glflags.gf_line_skeleton_flag = TRUE;
    glflags.gf_line_print_pc = TRUE;    /* Print <pc> addresses. */
    glflags.gf_abbrev_flag = FALSE;
    glflags.gf_frame_flag = FALSE;      /* .debug_frame section. */
    glflags.gf_eh_frame_flag = FALSE;   /* GNU .eh_frame section. */
    glflags.gf_pubnames_flag = FALSE;
    glflags.gf_debug_addr_flag = FALSE;

    /* DWARF2,3,4. Old macro section*/
    glflags.gf_macinfo_flag = FALSE;

    /* DWARF5(and DWARF4 extension) new macro section */
    glflags.gf_macro_flag = FALSE;

    glflags.gf_loc_flag = FALSE;
    glflags.gf_aranges_flag = FALSE; /* .debug_aranges section. */
    glflags.gf_ranges_flag = FALSE; /* .debug_ranges section. */
    glflags.gf_string_flag = FALSE;
    glflags.gf_reloc_flag = FALSE;
    glflags.gf_static_func_flag = FALSE;
    glflags.gf_static_var_flag = FALSE;
    glflags.gf_types_flag = FALSE;
    glflags.gf_weakname_flag = FALSE;
    glflags.gf_show_args_flag = FALSE;

    /* Control printing of Elf header. */
    glflags.gf_header_flag = FALSE;

    glflags.gf_gdbindex_flag = FALSE;
    glflags.gf_print_utf8_flag = FALSE;

    /* List of CUs per compiler */
    glflags.gf_producer_children_flag = FALSE;

    glflags.gf_check_abbrev_code = FALSE;
    glflags.gf_check_pubname_attr = FALSE;
    glflags.gf_check_reloc_offset = FALSE;
    glflags.gf_check_tag_attr = FALSE;
    glflags.gf_check_tag_tree = FALSE;
    glflags.gf_check_type_offset = FALSE;
    glflags.gf_check_decl_file = FALSE;
    glflags.gf_check_macros = FALSE;
    glflags.gf_check_lines = FALSE;
    glflags.gf_check_fdes = FALSE;
    glflags.gf_check_ranges = FALSE;
    glflags.gf_check_aranges = FALSE;
    glflags.gf_check_harmless = FALSE;
    glflags.gf_check_abbreviations = FALSE;
    glflags.gf_check_dwarf_constants = FALSE;
    glflags.gf_check_di_gaps = FALSE;
    glflags.gf_check_forward_decl = FALSE;
    glflags.gf_check_self_references = FALSE;
    glflags.gf_check_functions = FALSE;

    /*  Enable certain checks inside libdwarf.
        This is the dwarfdump default */
    glflags.gf_suppress_harmless = FALSE;

    /* Attributes encoding */
    glflags.gf_check_attr_encoding = FALSE;

    glflags.gf_generic_1200_regs = FALSE;
    glflags.gf_suppress_check_extensions_tables = FALSE;

    /* Tells dwarfdump not to look for duplicated attrs. */
    glflags.gf_check_duplicated_attributes = FALSE;

    /* Tells libdwarf to look for duplicated attrs. */
    glflags.gf_no_check_duplicated_attributes = FALSE;
    glflags.gf_no_sanitize_strings = FALSE;
    glflags.gf_print_section_allocations = FALSE;
    glflags.gf_allocation_via_mmap = FALSE;

    /* lots of checks make no sense on a dwp debugfission object. */
    glflags.gf_suppress_checking_on_dwp = FALSE;
    glflags.gf_file_use_no_libelf = FALSE;

    /*  suppress_nested_name_search is a band-aid.
        A workaround. A real fix for N**2 behavior is needed.  */
    glflags.gf_suppress_nested_name_search = FALSE;
    glflags.gf_uri_options_translation = TRUE;
    glflags.gf_do_print_uri_in_input = TRUE;

    glflags.gf_machine_arch_flag = FALSE;

    glflags.gf_print_unique_errors = FALSE;
    glflags.gf_found_error_message = FALSE;

    /* if TRUE, use old behavior of all expr ops joined on one line*/
    glflags.gf_expr_ops_joined     = FALSE;

    glflags.gf_print_raw_rnglists  = FALSE;
    glflags.gf_check_names = FALSE;

    /* During '-k' mode, display errors */
    glflags.gf_check_verbose_mode = TRUE;

    glflags.gf_check_frames = FALSE;

    /* Extensive frames check */
    glflags.gf_check_frames_extended = FALSE;
    glflags.gf_print_all_srcfiles = FALSE;

    glflags.gf_check_locations      = FALSE;
    glflags.gf_print_usage_tag_attr = FALSE;
    glflags.gf_check_all_compilers  = TRUE;
    glflags.gf_check_snc_compiler   = FALSE;
    glflags.gf_check_gcc_compiler   = FALSE;
    glflags.gf_print_summary_all    = FALSE;

    /*  The check and print flags here make it easy to
        allow check-only or print-only.  We no longer support
        check-and-print in a single run.  */
    glflags.gf_do_check_dwarf       = FALSE;
    glflags.gf_do_print_dwarf       = FALSE;
    glflags.gf_check_show_results   = FALSE;
    glflags.gf_record_dwarf_error   = FALSE;  /* A test has
        failed, this
        is normally set FALSE shortly after being set TRUE, it is
        a short-range hint we should print something we might not
        otherwise print (under the circumstances). */

    glflags.gf_check_debug_names    = FALSE;

    /* Display parent/children when in wide format? */
    glflags.gf_display_parent_tree  = FALSE;
    glflags.gf_display_children_tree = FALSE;
    glflags.gf_stop_indent_level    = 0;

    /*  10 is sort of arbitrary, it tries to keep
        the indentation sane ( a 500 level DIE tree
        is just too much to represent with prefix
        spaces).*/
    glflags.gf_max_space_indent = 10;
    /* Print search results in wide format? */
    glflags.gf_search_wide_format   = FALSE;
    /* -S option: strings for 'any' and 'match' */
    glflags.gf_search_is_on         = FALSE;
    glflags.gf_search_print_results = FALSE;
    glflags.gf_cu_name_flag         = FALSE;
    glflags.gf_show_global_offsets  = FALSE;
    glflags.gf_display_offsets      = TRUE;

    /*  Means we need to search by addr. Split Dwarf. */
    glflags.gf_debug_addr_missing = 0;
    glflags.gf_error_code_search_by_address = 0;
    glflags.gf_all_cus_seen_search_by_address = 0;

    /*  Base address has a special meaning in
        DWARF4 relative to address ranges. */
    glflags.seen_PU = FALSE;              /* Detected a PU */
    glflags.seen_CU = FALSE;              /* Detected a CU */
    glflags.need_CU_name = TRUE;          /* Need CU name */
    glflags.need_CU_base_address = TRUE;  /* Need CU Base address */
    glflags.need_CU_high_address = TRUE;  /* Need CU High address */
    glflags.need_PU_valid_code = TRUE;    /* Need PU valid code */

    /*  Involved with need_PU_valid_code (set
        in dwarfdump.c), in_valid_code set when subprogram DIE
        or CU DIE  has lowpc and highpc.   */
    glflags.in_valid_code = FALSE;

    /* Detected a Base address for PU */
    glflags.seen_PU_base_address = FALSE;

    /* Detected a High address for PU */
    glflags.seen_PU_high_address = FALSE;

    glflags.PU_base_address = 0;       /* PU Base address */
    glflags.PU_high_address = 0;       /* PU High address */

    glflags.DIE_offset = 0;         /* DIE offset in compile unit */
    glflags.DIE_section_offset = 0; /* DIE offset in .debug_info */

    /*  These globals  enable better error reporting. */

    /* CU DIE offset in compile unit */
    glflags.DIE_CU_offset = 0;

    /* CU DIE offset in .debug_info */
    glflags.DIE_CU_overall_offset = 0;
    glflags.current_section_id = 0; /* Section being process */

    /*  Base Address is needed for range lists and
        must come from a CU.
        Low address is for information and can come from a function
        or something in the CU. */
    glflags.CU_base_address = 0;       /* CU Base address */
    glflags.CU_low_address = 0;        /* CU low address */
    glflags.CU_high_address = 0;       /* CU High address */

    glflags.fde_offset_for_cu_low = DW_DLV_BADOFFSET;
    glflags.fde_offset_for_cu_high = DW_DLV_BADOFFSET;

    glflags.program_name = NULL;
    glflags.program_fullname = NULL;

    /* Able to generate report on search */
    glflags.search_any_text = 0;
    glflags.search_match_text = 0;
    glflags.search_regex_text = 0;
    glflags.search_occurrences = 0;

    /*  Start verbose at zero. verbose can
        be incremented with -v but not decremented. */
    glflags.verbose = 0;
    glflags.gf_show_dwarfdump_conf = 0;

    glflags.dense = FALSE;
    glflags.ellipsis = FALSE;
    glflags.show_form_used = FALSE;

    /*  break_after_n_units is mainly for testing.
        It enables easy limiting of output size/running time
        when one wants the output limited.
        For example,
            -H 2
        limits the -i output to 2 compilation units and
        the -f or -F output to 2 FDEs and 2 CIEs.
    */
    glflags.break_after_n_units = INT_MAX;

    glflags.section_high_offsets_global =
        &_section_high_offsets_global;

    glflags.pRangesInfo = NULL;
    glflags.pLinkonceInfo = NULL;
    glflags.pVisitedInfo = NULL;

    /*  These names make diagnostic messages more complete, the
        fixed length is safe, though ultra long names will get
        truncated. */
    glflags.PU_name[0] = 0;
    glflags.CU_name[0] = 0;
    glflags.CU_producer[0] = 0;

    /*  Options to enable debug tracing. */
    {
        int i = 0;
        for ( ; i <= MAX_TRACE_LEVEL; ++i) {
            glflags.nTrace[i] = 0;
        }
    }

    /*  Output filename */
    glflags.output_file = 0;
    glflags.group_number = 0;
    glflags.gf_universalnumber = 0;/* for Mach-O universal binaries */

    /*  Global esb-buffers. */
    glflags.newprogname = &_newprogname;
    esb_constructor(glflags.newprogname);

    glflags.cu_name = &_cu_name;
    esb_constructor(glflags.cu_name);

    glflags.config_file_path = &_config_file_path;
    esb_constructor(glflags.config_file_path);

    glflags.config_file_tiedpath = &_config_file_tiedpath;
    esb_constructor(glflags.config_file_tiedpath);

    glflags.config_file_data = &_config_file_data;

    /*  Check errors. */
    glflags.check_error = 0;
}

void
reset_global_flags(void)
{
    esb_destructor(glflags.newprogname);
    esb_destructor(glflags.cu_name);
    esb_destructor(glflags.config_file_path);
    esb_destructor(glflags.config_file_tiedpath);
}

/*  When we add a 'print' option after an option
    requests one or more checks
    we turn off all checking, putting it back to default
    checking state. */
void
set_checks_off(void)
{
    glflags.gf_check_abbrev_code = FALSE;
    glflags.gf_check_pubname_attr = FALSE;
    glflags.gf_check_reloc_offset = FALSE;
    glflags.gf_check_tag_attr = FALSE;
    glflags.gf_check_tag_tree = FALSE;
    glflags.gf_check_type_offset = FALSE;
    glflags.gf_check_decl_file = FALSE;
    glflags.gf_check_lines = FALSE;
    glflags.gf_check_fdes = FALSE;
    glflags.gf_check_ranges = FALSE;
    glflags.gf_check_aranges = FALSE;
    glflags.gf_check_harmless = FALSE;
    glflags.gf_check_abbreviations = FALSE;
    glflags.gf_check_dwarf_constants = FALSE;
    glflags.gf_check_di_gaps = FALSE;
    glflags.gf_check_forward_decl = FALSE;
    glflags.gf_check_self_references = FALSE;
    glflags.gf_check_attr_encoding = FALSE;
    /* applies to libdwarf */
    glflags.gf_no_check_duplicated_attributes = FALSE;
    /* applies to dwarfdump */
    glflags.gf_check_duplicated_attributes = FALSE;
    glflags.gf_check_debug_names = FALSE;
}

/*  Making this a named string makes it simpler to change
    what the reset,or 'I do not know'  value is for
    CU name or producer name for PRINT_CU_INFO.
    different string string is seen in dwarfdump.c
    when getting producer name for the list of CUs
    shown by -P  --print-producers and
    in 'Compilers detected' output.  */
static const char * default_cu_producer = "<unknown>";
void
reset_overall_CU_error_data(void)
{
    dd_safe_strcpy(glflags.CU_name,sizeof(glflags.CU_name),
        default_cu_producer,strlen(default_cu_producer));
    dd_safe_strcpy(glflags.CU_producer,sizeof(glflags.CU_producer),
        default_cu_producer,strlen(default_cu_producer));
    glflags.DIE_offset = 0;
    glflags.DIE_section_offset = 0;
    glflags.DIE_CU_offset = 0;
    glflags.DIE_CU_overall_offset = 0;
    glflags.CU_base_address = 0;
    glflags.CU_high_address = 0;
    glflags.CU_low_address = 0;
}

Dwarf_Bool
cu_data_is_set(void)
{
    if (strcmp(glflags.CU_name,default_cu_producer) ||
        strcmp(glflags.CU_producer,default_cu_producer)) {
        return 1;
    }
    if (glflags.DIE_offset || glflags.DIE_section_offset) {
        return 1;
    }
    if (glflags.CU_base_address || glflags.CU_low_address ||
        glflags.CU_high_address) {
        return 1;
    }
    return 0;
}
