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

#ifndef GLFLAGS_H
#define GLFLAGS_H

/*  All the dwarfdump flags are gathered into a single
    global struct as it has been hard to know how many there
    were or what they were all for. */

struct esb_s;
struct dwconf_s;

enum line_flag_type_e {
    singledw5,/* Meaning choose single table DWARF5 new interfaces. */
    s2l,      /* Meaning choose two-level DWARF5 new interfaces. */
    orig,     /* Meaning choose DWARF2,3,4 single level interface. */
    orig2l    /* Meaning choose DWARF 2,3,4 two-level interface. */
};

/* Check categories corresponding to the -k option */
typedef enum /* Dwarf_Check_Categories */ {
    abbrev_code_result,
    pubname_attr_result,
    reloc_offset_result,
    attr_tag_result,
    tag_tree_result,
    type_offset_result,
    decl_file_result,
    ranges_result,
    lines_result,
    aranges_result,
    /*  Harmless errors are errors detected inside libdwarf but
        not reported via DW_DLE_ERROR returns because the errors
        won't really affect client code.  The 'harmless' errors
        are reported and otherwise ignored.  It is difficult to report
        the error when the error is noticed by libdwarf, the error
        is reported at a later time. See
        dwarf_set_harmless_errors() to turn off such error checks.
        The other errors dwarfdump reports are also generally harmless
        but are detected by dwarfdump so it's possible to report the
        error as soon as the error is discovered. */
    harmless_result,
    fde_duplication,
    frames_result,
    locations_result,
    names_result,
    abbreviations_result,
    dwarf_constants_result,
    di_gaps_result,
    forward_decl_result,
    self_references_result,
    attr_encoding_result,
    duplicated_attributes_result,
    total_check_result,
    attr_formclass_result,
    check_functions_result,
    LAST_CATEGORY  /* Must be last */
} Dwarf_Check_Categories;

struct section_high_offsets_s {
    Dwarf_Unsigned debug_info_size;
    Dwarf_Unsigned debug_abbrev_size;
    Dwarf_Unsigned debug_line_size;
    Dwarf_Unsigned debug_loc_size;
    Dwarf_Unsigned debug_aranges_size;
    Dwarf_Unsigned debug_macinfo_size;
    Dwarf_Unsigned debug_pubnames_size;
    Dwarf_Unsigned debug_str_size;
    Dwarf_Unsigned debug_frame_size;
    Dwarf_Unsigned debug_ranges_size;
    Dwarf_Unsigned debug_pubtypes_size;
    Dwarf_Unsigned debug_types_size;
    Dwarf_Unsigned debug_macro_size;
    Dwarf_Unsigned debug_str_offsets_size;
    Dwarf_Unsigned debug_sup_size;
    Dwarf_Unsigned debug_cu_index_size;
    Dwarf_Unsigned debug_tu_index_size;
    Dwarf_Unsigned debug_names_size;
    Dwarf_Unsigned debug_loclists_size;
    Dwarf_Unsigned debug_rnglists_size;
};

/* Options to enable debug tracing. */
#define MAX_TRACE_LEVEL 3

struct glflags_s {

    /*  This so both
        dwarf_loclist_n()  and dwarf_get_loclist_c()
        and the dwarf_loclist_from_expr
        variations can be
        tested. Defaults to new
        dwarf_get_loclist_c(). See -g option.
        original IRIX dwarf_loclist() no longer tested
        as of October 2015. */
    Dwarf_Bool gf_use_old_dwarf_loclist;
    enum line_flag_type_e gf_line_flag_selection;

    Dwarf_Bool gf_abbrev_flag;
    Dwarf_Bool gf_aranges_flag; /* .debug_aranges section. */
    Dwarf_Bool gf_debug_names_flag; /* .debug_names section */
    Dwarf_Bool gf_eh_frame_flag;   /* GNU .eh_frame section. */
    Dwarf_Bool gf_frame_flag;      /* .debug_frame section. */
    Dwarf_Bool gf_gdbindex_flag;   /* .gdbindex section. */
    Dwarf_Bool gf_gnu_debuglink_flag;   /* .gnu_debuglink section. */
    Dwarf_Bool gf_debug_gnu_flag;  /* .debug_gnu_pubtypes, pubnames*/
    Dwarf_Bool gf_debug_sup_flag;  /* .debug_sup */
    Dwarf_Bool gf_info_flag;  /* .debug_info */
    Dwarf_Bool gf_line_flag;
    Dwarf_Bool gf_no_follow_debuglink;
    Dwarf_Bool gf_no_follow_dsym;
    Dwarf_Bool gf_line_print_pc;
    Dwarf_Bool gf_line_skeleton_flag;
    Dwarf_Bool gf_loc_flag;
    Dwarf_Bool gf_macinfo_flag; /* DWARF2,3,4. Old macro section*/
    Dwarf_Bool gf_macro_flag; /* DWARF5 */
    Dwarf_Bool gf_pubnames_flag;
    Dwarf_Bool gf_debug_addr_flag;
    Dwarf_Bool gf_print_language_version_table;
    Dwarf_Bool gf_ranges_flag; /* .debug_ranges section. */
    Dwarf_Bool gf_reloc_flag;  /* Elf relocations, not DWARF. */
    Dwarf_Bool gf_static_func_flag;/* SGI only */
    Dwarf_Bool gf_static_var_flag; /* SGI only */
    Dwarf_Bool gf_string_flag;
    Dwarf_Bool gf_pubtypes_flag;   /* SGI only */
    Dwarf_Bool gf_types_flag; /* .debug_types, not all CU types */
    Dwarf_Bool gf_weakname_flag;   /* SGI only */

    Dwarf_Bool gf_print_utf8_flag;
    Dwarf_Bool gf_print_section_allocations;
    Dwarf_Bool gf_allocation_via_mmap;
    Dwarf_Bool gf_print_all_srcfiles;

    Dwarf_Bool gf_header_flag; /* Control printing of Elf header. */
    Dwarf_Bool gf_section_groups_flag;

    /* Print list of CUs per compiler */
    Dwarf_Bool gf_producer_children_flag;

    Dwarf_Bool gf_check_abbrev_code;
    Dwarf_Bool gf_check_pubname_attr;
    Dwarf_Bool gf_check_reloc_offset;
    Dwarf_Bool gf_check_tag_attr;
    Dwarf_Bool gf_check_tag_tree;
    Dwarf_Bool gf_check_type_offset;
    Dwarf_Bool gf_check_decl_file;
    Dwarf_Bool gf_check_macros;
    Dwarf_Bool gf_check_lines;
    Dwarf_Bool gf_check_fdes;
    Dwarf_Bool gf_check_ranges;
    Dwarf_Bool gf_check_aranges;
    Dwarf_Bool gf_check_harmless;
    Dwarf_Bool gf_check_abbreviations;
    Dwarf_Bool gf_check_dwarf_constants;
    Dwarf_Bool gf_check_di_gaps;
    Dwarf_Bool gf_check_forward_decl;
    Dwarf_Bool gf_check_self_references;
    Dwarf_Bool gf_check_attr_encoding;   /* Attributes encoding */
    Dwarf_Bool gf_generic_1200_regs;
    Dwarf_Bool gf_suppress_check_extensions_tables;

    /* a call to libdwarf will speed up the library
        See dwarf_set_harmless_errors_enabled() */
    int        gf_suppress_harmless;

    /* The following tells libdwarf not to check for duplicated
        attributes if TRUE. */
    Dwarf_Bool gf_no_check_duplicated_attributes;

    /* The following tells dwarfdump to check for duplicated
        attributes if TRUE. */
    Dwarf_Bool gf_check_duplicated_attributes;
    Dwarf_Bool gf_check_functions;

    /* lots of checks make no sense on a dwp debugfission object. */
    Dwarf_Bool gf_suppress_checking_on_dwp;

    /*  suppress_nested_name_search is a band-aid.
        A workaround. A real fix for N**2 behavior is needed.  */
    Dwarf_Bool gf_suppress_nested_name_search;
    Dwarf_Bool gf_uri_options_translation;
    Dwarf_Bool gf_do_print_uri_in_input;

    /* Print global (unique) error messages */
    Dwarf_Bool gf_print_unique_errors;
    Dwarf_Bool gf_found_error_message;

    Dwarf_Bool gf_check_names;

    /*  During '-k' mode, display errors if non-zero.
        If > 1 then display DWARF CHECK errors
        in full detail from  dd_check_attr_encoding.c */
    int        gf_check_verbose_mode;

    Dwarf_Bool gf_check_frames;
    Dwarf_Bool gf_check_frames_extended; /* Extensive frames check */
    Dwarf_Bool gf_check_locations;       /* Location list check */

    Dwarf_Bool gf_print_usage_tag_attr;      /* Print usage */

    Dwarf_Bool gf_check_all_compilers;
    Dwarf_Bool gf_check_snc_compiler; /* Check SNC compiler */
    Dwarf_Bool gf_check_gcc_compiler;
    Dwarf_Bool gf_print_summary_all;
    Dwarf_Bool gf_file_use_no_libelf;

    /*  The check and print flags here make it easy to
        allow check-only or print-only.  We no longer support
        check-and-print in a single run.  */
    Dwarf_Bool gf_do_check_dwarf;
    Dwarf_Bool gf_do_print_dwarf;
    Dwarf_Bool gf_check_show_results;  /* Display checks results. */
    Dwarf_Bool gf_record_dwarf_error;  /* A test has failed, this
        is normally set FALSE shortly after being set TRUE, it is
        a short-range hint we should print something we might not
        otherwise print (under the circumstances). */

    Dwarf_Bool gf_check_debug_names;
    Dwarf_Bool gf_no_sanitize_strings;
    Dwarf_Bool gf_suppress_dup_attr_form;

    /* Display parent/children when in wide format? */
    Dwarf_Bool gf_display_parent_tree;
    Dwarf_Bool gf_display_children_tree;
    int     gf_stop_indent_level;

    /*  ====Searching for function name (printing frame data).  */

    /*  split dwarf lookup by address or die.
        if non-zero then .debug_addr is needed but missing.
        Tells dwarfdump to only search by address
        locally. */
    char     gf_debug_addr_missing;

    /* Other error in lookup by address or by_die */
    int     gf_error_code_search_by_address;

    /* Avoid some unnecessary work lookup by address. */
    char    gf_all_cus_seen_search_by_address;

    /*  Die indents >= this prefix an indent count instead
        of actual spaces. */
    int     gf_max_space_indent;

    /*  ====End searching for function name. */

    /* Print search results in wide format? */
    Dwarf_Bool gf_search_wide_format;

    /* -S option: strings for 'any' and 'match' */
    Dwarf_Bool gf_search_is_on;

    Dwarf_Bool gf_search_print_results;
    Dwarf_Bool gf_cu_name_flag;
    Dwarf_Bool gf_show_global_offsets;
    Dwarf_Bool gf_display_offsets;
    Dwarf_Bool gf_print_str_offsets;
    Dwarf_Bool gf_machine_arch_flag;
    Dwarf_Bool gf_expr_ops_joined;
    Dwarf_Bool gf_print_raw_rnglists;
    Dwarf_Bool gf_print_raw_loclists;

    unsigned long gf_count_major_errors;
    unsigned long gf_count_macronotes;

    char **  gf_global_debuglink_paths;
    unsigned gf_global_debuglink_count;
    Dwarf_Bool gf_show_args_flag;

    /*  Base address has a special meaning in DWARF4,5
        relative to address ranges. */
    Dwarf_Bool seen_PU;               /* Detected a PU */
    Dwarf_Bool seen_CU;               /* Detected a CU */
    Dwarf_Bool need_CU_name;          /* Need CU name */
    Dwarf_Bool need_CU_base_address;  /* Need CU Base address */
    Dwarf_Bool need_CU_high_address;  /* Need CU High address */
    Dwarf_Bool need_PU_valid_code;    /* Need PU valid code */
    Dwarf_Bool in_valid_code;         /* set/reset in  subprogram
        and compile-unit DIES.*/

    Dwarf_Bool seen_PU_base_address;  /* Detected a Base address
        for PU */
    Dwarf_Bool seen_PU_high_address;  /* Detected a High address
        for PU */
    Dwarf_Addr PU_base_address;    /* PU Base address */
    Dwarf_Addr PU_high_address;    /* PU High address */

    Dwarf_Off  DIE_offset;         /* DIE offset in compile unit */
    Dwarf_Off  DIE_section_offset; /* DIE offset in .debug_info */

    /*  These globals enable better error reporting. */
    Dwarf_Off  DIE_CU_offset;      /* CU DIE offset in compile unit */

    /* CU DIE offset in .debug_info */
    Dwarf_Off  DIE_CU_overall_offset;

    int current_section_id;        /* Section being process */

    /*  Base Address is needed for range lists and
        must come from a CU.
        Low address is for information and can come from a function
        or something in the CU. */
    Dwarf_Addr CU_base_address;       /* CU Base address */
    Dwarf_Addr CU_low_address;        /* CU low address */
    Dwarf_Addr CU_high_address;       /* CU High address */

    Dwarf_Off fde_offset_for_cu_low;
    Dwarf_Off fde_offset_for_cu_high;

    const char *program_name;
    const char *program_fullname;

    const char *search_any_text;
    const char *search_match_text;
    const char *search_regex_text;
    int search_occurrences;

    /*  Start verbose at zero. verbose can
        be incremented with -v but not decremented. */
    int verbose;
    int gf_show_dwarfdump_conf;/* Incremented with
        --show-dwarfdump-conf, never decremented */
    Dwarf_Bool dense;
    Dwarf_Bool ellipsis;
    Dwarf_Bool show_form_used;

    /*  break_after_n_units is mainly for testing.
        It enables easy limiting of output size/running time
        when one wants the output limited.
        For example,
            -H 2
        limits the -i output to 2 compilation units and
        the -f or -F output to 2 FDEs and 2 CIEs.
    */
    int break_after_n_units;

    struct section_high_offsets_s *section_high_offsets_global;

    /*  pRangesInfo records the DW_AT_high_pc and DW_AT_low_pc
        and is used to check that line range info falls inside
        the known valid ranges.   The data is per CU, and is
        reset per CU in tag_specific_checks_setup(). */
    Bucket_Group *pRangesInfo;

    /*  pLinkonceInfo records data about the link once sections.
        If a line range is not valid in the current CU it might
        be valid in a linkonce section, this data records the
        linkonce sections.  So it is filled in when an
        object file is read and remains unchanged for an entire
        object file.  */
    Bucket_Group *pLinkonceInfo;

    /*  pVisitedInfo records a recursive traversal of DIE attributes
        DW_AT_specification DW_AT_abstract_origin DW_AT_type
        that let DWARF refer (as in a general graph) to
        arbitrary other DIEs.
        These traversals use pVisitedInfo to
        detect any compiler errors that introduce circular references.
        Printing of the traversals is also done on request.
        Entries are added and deleted as they are visited in
        a depth-first traversal.  */
    Bucket_Group *pVisitedInfo;

    /*  Compilation Unit information for improved error messages.
        If the strings are too short we just truncate so fixed length
        here is fine.  */
    #define COMPILE_UNIT_NAME_LEN 512
    char PU_name[COMPILE_UNIT_NAME_LEN]; /* PU Name */
    char CU_name[COMPILE_UNIT_NAME_LEN]; /* CU Name */
    char CU_producer[COMPILE_UNIT_NAME_LEN];  /* CU Producer Name */

    /*  Options to enable debug tracing. */
    int nTrace[MAX_TRACE_LEVEL + 1];

    /* Output filename */
    const char *output_file;
    int         group_number;
    unsigned gf_universalnumber; /* for Mach-O universal binaries*/

    /*  Global esb-buffers. */
    struct esb_s *newprogname;
    struct esb_s *cu_name;
    struct esb_s *config_file_path;
    struct esb_s *config_file_tiedpath;
    struct dwconf_s *config_file_data;

    /*  Check errors. */
    int check_error;
};

extern struct glflags_s glflags;

void init_global_flags(void);
void reset_global_flags(void);
void set_checks_off(void);
void reset_overall_CU_error_data(void);
Dwarf_Bool cu_data_is_set(void);

/*  Shortcuts for additional trace options.
    Indexes into glflags.nTrace[] */
#define KIND_OPTIONS        0   /* Dump options and stop. */
#define KIND_RANGES_INFO    1   /* Dump RangesInfo Table. */
#define KIND_LINKONCE_INFO  2   /* Dump Linkonce Table. */
#define KIND_VISITED_INFO   3   /* Dump Visited Info. */

/*  Section IDs. See also libdwarfp/pro_opaque.h DEBUG_INFO etc
    as we arbitrarily use the same numbering here.
    In pro_opaque the numbering matters.
    Here it helps us record where we are at any instant.
    It's not necessary they match but seems good to do.
    Here we have some extra sections not related to
    relocations so we just add those beginning with 22 */
#define         DEBUG_INFO      0
#define         DEBUG_LINE      1
#define         DEBUG_ABBREV    2
#define         DEBUG_FRAME     3
#define         DEBUG_ARANGES   4
#define         DEBUG_PUBNAMES  5
#define         DEBUG_FUNCNAMES 6
#define         DEBUG_TYPENAMES 7
#define         DEBUG_VARNAMES  8
#define         DEBUG_WEAKNAMES 9
#define         DEBUG_MACINFO   10 /* DWARF 2,3,4 only */
#define         DEBUG_LOC       11
#define         DEBUG_RANGES    12
#define         DEBUG_TYPES     13
#define         DEBUG_PUBTYPES  14
#define         DEBUG_NAMES     15 /* DWARF5. aka dnames */
#define         DEBUG_STR       16
#define         DEBUG_LINE_STR  17
#define         DEBUG_MACRO     18 /* DWARF 5. */
#define         DEBUG_LOCLISTS  19 /* DWARF 5. */
#define         DEBUG_RNGLISTS  20 /* DWARF 5. */
#define         DEBUG_SUP       21 /* DWARF 5. */

#define         DEBUG_GDB_INDEX    22
#define         DEBUG_FRAME_EH_GNU 23
#define         DEBUG_GNU_PUBNAMES 24
#define         DEBUG_GNU_PUBTYPES 25

/*  Print the information only if unique errors
    is set and it is first time */
#define PRINTING_UNIQUE (!glflags.gf_found_error_message)
#endif /* GLFLAGS_H */
