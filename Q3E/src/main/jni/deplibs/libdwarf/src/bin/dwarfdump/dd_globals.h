/*
Copyright (C) 2000,2004,2005 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright (C) 2007-2020 David Anderson. All Rights Reserved.
Portions Copyright 2012-2018 SN Systems Ltd. All rights reserved.

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

#ifndef globals_INCLUDED
#define globals_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

#define DWARF_SECNAME_BUFFER_SIZE 50
#define ESB_FIXED_ALLOC_SIZE 300

/*  Used to avoid leakage when we hide errors.
    Use carefully: doing err when you mean
    *err will not be caught by the compiler
    and will not do what one wants.
    Using *err when err is correct will
    be caught by the compiler.  You are warned. */
#define DROP_ERROR_INSTANCE(d,r,e)        \
do {                                      \
    if ((r) == DW_DLV_ERROR) {            \
        if (e) {                          \
            dwarf_dealloc_error((d),(e)); \
            (e) = 0;                      \
        } else {                          \
            report_caller_error_drop_error((r), \
                __LINE__,__FILE__);       \
        }                                 \
    }                                     \
} while(0)

/*  Used in very few places here. */
#define DW_DLV_BADOFFSET   (~(Dwarf_Off)0)

/*  Version 5 returns DW_AT_decl_file differently
    than earlier versions */
#define DWVERSION4  4
#define DWVERSION5  5

/* FALSE for the flags means off */
#define LOHIPC_SAWADDR  1
#define LOHIPC_SAWOFFSET  2
struct LoHiPc_s {
    Dwarf_Unsigned lopc;

    /*  hival is either an address if sawhi ==LOHIPC_SAWADDR
        or an offset if sawhi_flag == LOWHIPC_SAWOFFSET.
        or zero (sawhi_flag == FALSE)*/
    Dwarf_Unsigned hival;

    /*  The result of adding lopc to hival. */
    Dwarf_Unsigned hifinal;

    /* non-zero if DW_AT_lowpc seen */
    Dwarf_Small sawlo_flag;

    /*  non-zero if DW_AT_high_pc seen
        defaults FALSE, otherwise is
        LOHIPC_SAWADDR  or LOHIPC_SAWOFFSET */
    Dwarf_Small sawhi_flag;

    /*  If non-zero, hifinal is set to the hipc address
        Defaults to FALSE*/
    Dwarf_Small havefinal_flag;
};

typedef struct LoHiPc_s LoHiPc;

/* Calculate wasted space */
extern void calculate_attributes_usage(Dwarf_Half attr,
    Dwarf_Half theform,
    Dwarf_Unsigned value);

extern Dwarf_Bool is_strstrnocase(const char *data,
    const char *pattern);

/*  Process TAGs for checking mode and reset pRangesInfo table
    if appropriate. */
extern void tag_specific_globals_setup(Dwarf_Debug dbg,
Dwarf_Half val,int die_indent_level);

extern int simple_err_return_msg_either_action(int res,
    const char *msg);
extern int simple_err_return_action(int res,const char *msg);
extern int simple_err_only_return_action(int res,const char *msg);
extern void print_error_and_continue (const char * msg,
    int res, Dwarf_Error err);
extern void print_error (Dwarf_Debug dbg, const char * msg,
    int res, Dwarf_Error err);

extern int print_line_numbers_this_cu (Dwarf_Debug dbg,
    Dwarf_Die in_die,
    char **srcfiles,
    Dwarf_Signed cnt,
    Dwarf_Error *err);

extern int print_ranges(Dwarf_Debug dbg);
extern int print_raw_all_rnglists(Dwarf_Debug dbg, Dwarf_Error *err);
extern int print_raw_all_loclists(Dwarf_Debug dbg, Dwarf_Error *err);
extern int print_pubnames_style(Dwarf_Debug dbg,
    int category,Dwarf_Error *);
extern int print_infos (Dwarf_Debug dbg,Dwarf_Bool is_info,
    Dwarf_Error *);
extern int print_abbrevs (Dwarf_Debug dbg,Dwarf_Error *);
extern int print_strings (Dwarf_Debug dbg,Dwarf_Error *);
extern int print_aranges (Dwarf_Debug dbg,Dwarf_Error *);
extern int print_static_funcs(Dwarf_Debug dbg,Dwarf_Error *);
extern int print_static_vars(Dwarf_Debug dbg,Dwarf_Error *);
enum type_type_e {SGI_TYPENAME, DWARF_PUBTYPES} ;
extern int print_types(Dwarf_Debug dbg,enum type_type_e type_type,
    Dwarf_Error *);
extern int print_weaknames(Dwarf_Debug dbg, Dwarf_Error *);
extern int print_debug_names(Dwarf_Debug dbg,Dwarf_Error *);
int print_debug_sup(Dwarf_Debug dbg, Dwarf_Error *error);
extern int print_debug_addr(Dwarf_Debug dbg, Dwarf_Error *error);
int print_all_abbrevs_for_cu(Dwarf_Debug dbg,
    Dwarf_Unsigned  offset,
    Dwarf_Unsigned abbrev_num_in,
    Dwarf_Unsigned *length_out,
    Dwarf_Unsigned *abbrev_num_out,
    Dwarf_Error    *error);

/*  Space used to record range information */
extern void allocate_range_array_info(void);
extern void release_range_array_info(void);
extern void record_range_array_info_entry(Dwarf_Off die_off,
    Dwarf_Off range_off);
extern int check_range_array_info(Dwarf_Debug dbg,
    Dwarf_Die die, Dwarf_Error *);

int should_skip_this_cu(Dwarf_Debug dbg,Dwarf_Bool *,
    Dwarf_Die cu_die);

int get_address_size_and_max(Dwarf_Debug dbg,
    Dwarf_Half * size,
    Dwarf_Addr * max,
    Dwarf_Error *err);

/* Returns the producer of the CU */
int get_cu_name(Dwarf_Debug dbg,Dwarf_Die cu_die,
    Dwarf_Off  dieprint_cu_offset,
    char **short_name,char **long_name,
    Dwarf_Error *err);

/* Get number of abbreviations for a CU */
extern void get_abbrev_array_info(Dwarf_Debug dbg,
    Dwarf_Unsigned offset);
/* Validate an abbreviation */
extern void validate_abbrev_code(Dwarf_Unsigned abbrev_code);

extern int print_one_die(
    Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Off dieprint_cu_offset,
    Dwarf_Bool print_information,
    int die_indent_level,
    char **srcfiles,
    Dwarf_Signed cnt,
    Dwarf_Bool *an_attr_duplicated,
    Dwarf_Bool ignore_die_stack,
    Dwarf_Error *err);

/* Check for specific compiler */
extern Dwarf_Bool checking_this_compiler(void);
extern void update_compiler_target(const char *producer_name);
extern void add_cu_name_compiler_target(char *name);

/*  General error reporting routines. These were
    macros for a short time and when changed into functions
    they kept (for now) their capitalization.
    The capitalization will likely change. */
extern void PRINT_CU_INFO(void);
extern void DWARF_CHECK_COUNT(Dwarf_Check_Categories category,
    int inc);
extern void DWARF_ERROR_COUNT(Dwarf_Check_Categories category,
    int inc);
extern void DWARF_CHECK_ERROR_PRINT_CU(void);
#define DWARF_CHECK_ERROR(c,d)    DWARF_CHECK_ERROR3(c,d,0,0)
#define DWARF_CHECK_ERROR2(c,d,e) DWARF_CHECK_ERROR3(c,d,e,0)
extern void DWARF_CHECK_ERROR3(Dwarf_Check_Categories category,
    const char *str1, const char *str2, const char *strexpl);

extern int print_macinfo_by_offset(Dwarf_Debug dbg,
    Dwarf_Die cudie,Dwarf_Unsigned offset,Dwarf_Error *);

void ranges_esb_string_destructor(void);
void global_destructors(void);
void destruct_abbrev_array(void);

int get_proc_name_by_die(Dwarf_Debug dbg,
    Dwarf_Die die, Dwarf_Addr low_pc,
    struct esb_s *proc_name,
    Dwarf_Die *cu_die_for_print_frames,
    void **pcMap,
    Dwarf_Error *err);

extern void dump_block(char *prefix, char *data, Dwarf_Signed len);

extern int print_gdb_index(Dwarf_Debug dbg,Dwarf_Error *err);
extern int print_debugfission_index(Dwarf_Debug dbg,
    const char *type, Dwarf_Error *);

void clean_up_die_esb(void);

int print_macros_5style_this_cu(Dwarf_Debug dbg, Dwarf_Die cu_die,
    char **srcfiles,
    Dwarf_Signed cnt,
    int do_print_dwarf /* not relying on gf_do_print_dwarf here */,
    int descend_into_imports /* TRUE means follow imports */,
    Dwarf_Bool in_import_list /* helps make print readable */,
    Dwarf_Unsigned offset,
    Dwarf_Error *);

/* Detailed attributes encoding space */
int print_attributes_encoding(Dwarf_Debug dbg,Dwarf_Error *);

/* Detailed tag and attributes usage */
int print_tag_attributes_usage(void);
void record_tag_usage(Dwarf_Half tag);
void reset_usage_rate_tag_trees(void);

int  print_section_groups_data(Dwarf_Debug dbg,Dwarf_Error *);
void update_section_flags_per_groups(void);
void groups_restore_subsidiary_flags(void);

int legal_tag_attr_combination(Dwarf_Half tag,
    Dwarf_Half attr);
int legal_tag_tree_combination(Dwarf_Half parent_tag,
    Dwarf_Half child_tag);

int print_str_offsets_section(Dwarf_Debug dbg,Dwarf_Error *);

void print_any_harmless_errors(Dwarf_Debug dbg);

void print_secname(Dwarf_Debug dbg,const char *secname);

void  report_caller_error_drop_error(int dwdlv,
    int line, char *filename);

/*  encoding_type_func used in print_die.c and
    print_lopc_hipc_attr.c  */
typedef const char *(*encoding_type_func)(unsigned);
int dd_get_integer_and_name(Dwarf_Debug dbg,
    Dwarf_Attribute attrib,
    Dwarf_Unsigned * uval_out,
    const char *attr_name,
    struct esb_s* string_out,
    encoding_type_func val_as_string,
    Dwarf_Error * err,
    int show_form);

int print_original_loclist_linecodes(Dwarf_Bool    checking,
    const char *  tagname,
    const char *  attrname,
    unsigned int  llent,
    Dwarf_Small   lle_value,
    Dwarf_Addr    base_address,
    Dwarf_Addr    rawlopc,
    Dwarf_Addr    rawhipc,
    Dwarf_Bool    debug_addr_unavailable,
    Dwarf_Addr    lopc,
    Dwarf_Addr    hipc,
    Dwarf_Unsigned locdesc_offset,
    struct esb_s * ebsp,
    Dwarf_Bool   * bError);
int print_llex_linecodes(Dwarf_Bool    checking,
    const char *  tagname,
    const char *  attrname,
    unsigned int  llent,
    Dwarf_Small   lle_value,
    Dwarf_Addr    base_address,
    Dwarf_Addr    rawlopc,
    Dwarf_Addr    rawhipc,
    Dwarf_Bool    debug_addr_unavailable,
    Dwarf_Addr    lopc,
    Dwarf_Addr    hipc,
    Dwarf_Unsigned locdesc_offset,
    struct esb_s * ebsp,
    Dwarf_Bool   * bError);
int print_debug_loclists_linecodes(Dwarf_Bool    checking,
    const char *  tagname,
    const char *  attrname,
    unsigned int  llent,
    Dwarf_Small   lle_value,
    Dwarf_Unsigned lle_byte_count,
    Dwarf_Addr    base_address,
    Dwarf_Addr    rawlopc,
    Dwarf_Addr    rawhipc,
    Dwarf_Bool    debug_addr_unavailable,
    Dwarf_Addr    lopc,
    Dwarf_Addr    hipc,
    Dwarf_Unsigned locdesc_offset,
    struct esb_s * ebsp,
    Dwarf_Bool   * bError);
void loc_error_check(
    const char *tagname,
    const char *attrname,
    Dwarf_Addr lopcfinal,
    Dwarf_Addr hipcfinal,
    Dwarf_Unsigned offset,
    Dwarf_Addr base_address,
    Dwarf_Bool *bError);
int print_hipc_lopc_attribute(Dwarf_Debug dbg,
    Dwarf_Half tag,
    Dwarf_Die die,
    Dwarf_Unsigned dieprint_cu_goffset,
    char ** srcfiles,
    Dwarf_Signed cnt,
    Dwarf_Attribute attrib,
    Dwarf_Half attr,
    Dwarf_Unsigned max_address,
    LoHiPc  *lohipc,
    struct esb_s *valname,
    Dwarf_Error *err);
int dd_trace_abstract_origin_etc(
    Dwarf_Debug dbg,
    Dwarf_Half  tag,
    Dwarf_Die   die,
    Dwarf_Off   dieprint_cu_offset,
    Dwarf_Half  theform,
    Dwarf_Half  attrnum,
    Dwarf_Attribute attrib,
    char ** srcfiles,
    Dwarf_Signed srcfiles_cnt,
    struct esb_s *valname,
    struct esb_s *esb_extra,
    int die_indent_level,
    Dwarf_Error *err);
int
dd_traverse_one_die(Dwarf_Debug dbg,
    Dwarf_Attribute attrib,
    Dwarf_Die die,
    Dwarf_Off dieprint_cu_goffset,
    Dwarf_Bool is_info,
    char **srcfiles, Dwarf_Signed cnt,
    int die_indent_level,
    Dwarf_Error *err);
void
dd_do_dump_visited_info(int level, Dwarf_Off loff,Dwarf_Off goff,
    Dwarf_Off cu_die_goff,
    const char *atname, const char *valname);
int
dd_print_sig8_target(Dwarf_Debug dbg,
    Dwarf_Attribute attrib,
    int die_indent_level,
    char **srcfiles,
    Dwarf_Signed srcfiles_cnt,
    struct esb_s * valname,
    Dwarf_Error *err);

Dwarf_Bool dd_form_refers_local_info(Dwarf_Half form);
void print_language_version_table(void);

void check_attributes_encoding(Dwarf_Half attr,Dwarf_Half theform,
    Dwarf_Unsigned value);

#ifdef __cplusplus
}
#endif

#endif /* globals_INCLUDED */
