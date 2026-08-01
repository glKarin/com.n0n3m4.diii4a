/*
  Copyright 2010-2018 David Anderson. All rights reserved.

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

#include <stdlib.h> /* atoi() calloc() exit() free()
    malloc() strtol() */
#include <string.h> /* strcmp() strdup() strlen() */
#include <stdio.h> /* FILE decl for dd_esb.h */

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
#include "dd_dwconf.h"
#include "dd_getopt.h"
#include "dd_common.h"
#include "dd_makename.h"
#include "dd_uri.h"
#include "dd_esb.h"                /* For flexible string buffer. */
#include "dd_sanitized.h"
#include "dd_tag_common.h"
#include "dd_command_options.h"
#include "dd_compiler_info.h"
#include "dd_regex.h"
#include "dd_safe_strcpy.h"
#include "libdwarf_private.h" /* For malloc/calloc debug */

static const char *remove_quotes_pair(const char *text);
static void suppress_check_dwarf(void);

/*  These configure items are for the
    frame data.  We're flexible in
    the path to dwarfdump.conf .
    The HOME strings here are transformed in
    dwconf.c to reference the environment
    variable $HOME .

    As of August 2018 CONFPREFIX is always set as it
    comes from autoconf --prefix, aka  $prefix
    which defaults to /usr/local

    The install puts the .conf file in
    CONFPREFIX/dwarfdump/
*/
static char *config_file_defaults[] = {
    "dwarfdump.conf",
    "./dwarfdump.conf",
    "HOME/.dwarfdump.conf",
    "HOME/dwarfdump.conf",
/*  See Makefile.am dwarfdump_CFLAGS. This prefix
    is the --prefix option (defaults to /usr/local
    and Makefile.am adds /share/dwarfdump )
    We need 2 levels of macro to get the name turned into
    the string we want. */
#define STR2(s) # s
#define STR(s)  STR2(s)
    STR(CONFPREFIX) "/dwarfdump.conf",
    "/usr/share/dwarfdump/dwarfdump.conf",
    0
};
static const char *config_file_abi = 0;

/* Do printing of most sections.
   Do not do detailed checking.
*/
static void
do_all(void)
{
    glflags.gf_frame_flag = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag =  TRUE; /* .debug_types */
    glflags.gf_line_flag = TRUE;
    glflags.gf_no_follow_debuglink = FALSE;
    glflags.gf_no_follow_dsym = FALSE;
    glflags.gf_global_debuglink_paths = 0;
    glflags.gf_global_debuglink_count = 0;
    glflags.gf_pubnames_flag = TRUE;
    glflags.gf_macinfo_flag = TRUE;
    glflags.gf_macro_flag = TRUE;
    glflags.gf_print_all_srcfiles = FALSE;
    glflags.gf_aranges_flag = TRUE;
    /*  Do not do
        glflags.gf_loc_flag = TRUE
        glflags.gf_abbrev_flag = TRUE;
        glflags.gf_ranges_flag = TRUE;
        because nothing in
        the DWARF spec guarantees the sections are free of
        random bytes in areas not referenced by .debug_info.
        though for DWARF5 .debug_loclists is free
        of random bytes. See --print_raw_loclists */
    glflags.gf_string_flag = TRUE;
    /*  Do not do
        glflags.gf_reloc_flag = TRUE;
        as print_relocs makes no sense for non-elf dwarfdump users.*/
    glflags.gf_static_func_flag = TRUE; /* SGI only*/
    glflags.gf_static_var_flag = TRUE; /* SGI only*/
    glflags.gf_pubtypes_flag = TRUE;  /* both SGI typenames
        and dwarf_pubtypes. */
    glflags.gf_weakname_flag = TRUE; /* SGI only*/
    glflags.gf_gnu_debuglink_flag = FALSE;
    glflags.gf_debug_addr_flag = FALSE;
    glflags.gf_debug_names_flag = TRUE;
    glflags.gf_debug_sup_flag = TRUE;
}

static int
get_number_value(char *v_in,long int *v_out)
{
    long int v= 0;
    size_t lenszt = strlen(v_in);
    char *endptr = 0;

    if (lenszt < 1) {
        return DW_DLV_ERROR;
    }
    v = strtol(v_in,&endptr,10);
    if (endptr == v_in) {
        return DW_DLV_NO_ENTRY;
    }
    if (*endptr != '\0') {
        return DW_DLV_ERROR;
    }
    *v_out = v;
    return DW_DLV_OK;
}

static void suppress_print_dwarf(void)
{
    glflags.gf_do_print_dwarf = FALSE;
    glflags.gf_do_check_dwarf = TRUE;
}

/*  Remove matching leading/trailing quotes.
    Does not alter the passed in string.
    If quotes removed does a makename on a modified string. */
static const char *
remove_quotes_pair(const char *text)
{
    static char single_quote = '\'';
    static char double_quote = '\"';
    char quote = 0;
    const char *p = text;
    size_t textlenszt = strlen(text);

    if (textlenszt < 2) {
        return p;
    }

    /* Compare first character with ' or " */
    if (p[0] == single_quote) {
        quote = single_quote;
    } else {
        if (p[0] == double_quote) {
            quote = double_quote;
        }
        else {
            return p;
        }
    }
    {
        if (p[textlenszt - 1] == quote) {
            char *altered = calloc(1,textlenszt+1);
            const char *str2 = 0;

            /*  Here we delete the leading and trailing quote chars.
                Start at p-1. String is to be orig textlenszt - 2
                bytes long */
            dd_safe_strcpy(altered,textlenszt+1,p+1,textlenszt-2);
            str2 =  makename(altered);
            free(altered);
            return str2;
        }
    }
    return p;
}

static void suppress_check_dwarf(void)
{
    glflags.gf_do_print_dwarf = TRUE;
    if (glflags.gf_do_check_dwarf) {
        printf("Warning: check flag turned off, "
            "checking and printing are separate.\n");
    }
    glflags.gf_do_check_dwarf = FALSE;
    set_checks_off();
}

static struct esb_s uri_esb_data;
void
uri_data_constructor(void)
{
    esb_constructor(&uri_esb_data);
}
void
uri_data_destructor(void)
{
    esb_destructor(&uri_esb_data);
}
/*  The strings whose pointers are returned here
    from makename are destructed at dwarf_finish.
    makename() never returns NULL, so
    do_uri_translation() never does so either.

    If the user were to type dangerous (for the
    terminal session) characters this prints them
    out as such! So don't put such in the
    dwarfdump command line.
*/
const char *
do_uri_translation(const char *s,const char *context)
{
    struct esb_s str;
    char *finalstr = 0;
    if (!glflags.gf_uri_options_translation) {
        return makename(s);
    }
    esb_constructor(&str);
    translate_from_uri(s,&str);
    if (glflags.gf_do_print_uri_in_input) {
        if (strcmp(s,esb_get_string(&str))) {
            printf("Uri Translation on option %s\n",context);
            printf("    \'%s\'\n",s);
            printf("    \'%s\'\n",esb_get_string(&str));
        }
    }
    finalstr = makename(esb_get_string(&str));
    esb_destructor(&str);
    return finalstr;
}

/*  Support for short (-option) and long (--option) names options.
    These functions implement the individual options.
    They are called from
    short names and long names options.
    Implementation code is shared for
    both types of formats. */

/*  Handlers for the short/long names options. */
static void arg_check_abbrev(void);
static void arg_check_all(void);
static void arg_check_aranges(void);
static void arg_check_attr_dup(void);
static void arg_check_attr_encodings(void);
static void arg_check_attr_names(void);
static void arg_check_constants(void);
static void arg_check_files_lines(void);
static void arg_check_forward_refs(void);
static void arg_check_frame_basic(void);
static void arg_check_frame_extended(void);
static void arg_check_frame_info(void);
static void arg_check_gaps(void);
static void arg_check_loc(void);
static void arg_check_macros(void);
static void arg_check_pubnames(void);
static void arg_check_ranges(void);
static void arg_check_self_refs(void);
static void arg_check_show(void);
static void arg_check_silent(void);
static void arg_check_summary(void);
static void arg_suppress_debuglink_crc(void);
static void arg_check_tag_attr(void);
static void arg_check_tag_tag(void);
static void arg_check_type(void);
static void arg_check_unique(void);

static void arg_check_usage(void);
static void arg_check_usage_extended(void);
static void arg_check_functions(void);

static void arg_file_abi(void);
static void arg_file_line5(void);
static void arg_file_name(void);
static void arg_file_output(void);
static void arg_file_tied(void);
static void arg_file_use_no_libelf(void);

static void arg_format_attr_name(void);
static void arg_format_dense(void);
static void arg_format_ellipsis(void);
static void arg_format_expr_ops_joined(void);
static void arg_format_extensions(void);
static void arg_format_global_offsets(void);
static void arg_format_loc(void);
static void arg_format_registers(void);
static void arg_format_suppress_data(void);
static void arg_format_suppress_group(void);
static void arg_format_suppress_lookup(void);
static void arg_format_suppress_offsets(void);
static void arg_format_suppress_sanitize(void);
static void arg_format_suppress_uri(void);
static void arg_format_suppress_uri_msg(void);
static void arg_format_suppress_utf8(void);

static void arg_print_section_allocations(void);
static void arg_allocate_via_mmap(void);
static void arg_format_file(void);
static void arg_format_gcc(void);
static void arg_format_groupnumber(void);
static void arg_format_universalnumber(void);
static void arg_format_limit(void);
static void arg_format_producer(void);
static void arg_format_snc(void);

static void arg_print_all(void);
static void arg_print_all_srcfiles(void);
static void arg_print_abbrev(void);
static void arg_print_aranges(void);
static void arg_print_debug_frame(void);
static void arg_print_debug_addr(void);
static void arg_print_debug_names(void);
static void arg_print_gnu_debuglink(void);
static void arg_print_debug_gnu(void);
static void arg_print_debug_sup(void);
static void arg_print_fission(void);
static void arg_print_gnu_frame(void);
static void arg_print_info(void);
static void arg_print_language_version_table(void);
static void arg_print_lines(void);
static void arg_print_lines_short(void);
static void arg_print_loc(void);
static void arg_print_macinfo(void);
static void arg_print_machine_arch(void);
static void arg_print_pubnames(void);
static void arg_print_producers(void);
static void arg_print_ranges(void);
static void arg_print_raw_loclists(void);
static void arg_print_raw_rnglists(void);
static void arg_print_static(void);
static void arg_print_static_func(void);
static void arg_print_static_var(void);
static void arg_print_str_offsets(void);
static void arg_print_strings(void);
static void arg_print_types(void);
static void arg_print_weaknames(void);

static void arg_suppress_harmless(void);

static void arg_no_follow_debuglink(void);
static void arg_no_follow_dsym(void);
static void arg_add_debuglink_path(void);
static void arg_debuglink_path_invalid(void);

static void arg_search_any(void);
static void arg_search_any_count(void);
static void arg_search_match(void);
static void arg_search_match_count(void);
static void arg_search_regex(void);
static void arg_search_regex_count(void);
static void arg_search_count(void);
static void arg_search_invalid(void);

static void arg_search_print_children(void);
static void arg_search_print_parent(void);
static void arg_search_print_tree(void);

static void arg_help(void);
static void arg_trace(void);
static void arg_verbose(void);
static void arg_version(void);
static void arg_show_dwarfdump_conf(void);
static void arg_show_args(void);

static void arg_c_multiple_selection(void);
static void arg_h_multiple_selection(void);
static void arg_l_multiple_selection(void);
static void arg_k_multiple_selection(void);
static void arg_kx_multiple_selection(void);
static void arg_ku_multiple_selection(void);
static void arg_O_multiple_selection(void);
static void arg_S_multiple_selection(void);
static void arg_t_multiple_selection(void);
static void arg_W_multiple_selection(void);
static void arg_x_multiple_selection(void);

static void arg_not_supported(void);
static void arg_x_invalid(void);

/*  Extracted from 'process_args',
    as they are used by option handlers. */
static Dwarf_Bool arg_usage_error = FALSE;
static int arg_option = 0;

static const char *usage_debug_text[] = {
"Usage: DwarfDump <debug_options>",
"  These only useful with related checking options (-ka suffices)",
"  --trace=0 print this message and stop ",
"  --trace=1 Dump RangesInfo Table requires",
"    add -kl(--check-loc) and -km (--check-ranges)",
"  --trace=2 Dump Linkonce Table (linkonce section)",
"    add -kl(--check-lo) and -km (--check-ranges)",
"  --trace=3 Dump Visited Info, add -kS (--check-self-refs)",
""
};

static const char *usage_long_text[] = {
"Usage: DwarfDump <options> <object file>",
" ",
"-------------------------------------------------------------------",
"Print Debug Sections",
"-------------------------------------------------------------------",
"-b   --print-abbrev      Print abbrev section",
"-a   --print-all         Print all debug_* sections",
"-r   --print-aranges     Print aranges section",
"-F   --print-eh-frame    Print gnu .eh_frame section",
"-f   --print-frame       Print .debug_frame section",
"-I   --print-fission     Print fission sections:",
"                         .gdb_index, .debug_cu_index,",
"                         .debug_tu_index, .gnu.debuglink,",
"                         .note.gnu.build-id",
"     --print-gnu-debuglink Print .gnu_debuglink,",
"                         .note.gnu.build-id sections",
"     --print-debug-addr  Print .debug_addr section. (If DWARF4",
"                         add option -i to make this work)",
"     --print-debug-gnu   Print .debug_gnu_pubtypes and",
"                         .debug_gnu_pubnames sections",
"     --print-debug-names Print .debug_names section",
"     --print-debug-sup   Print .debug_sup section",
"-i   --print-info        Print info section",
"-l   --print-lines       Print line section",
"-ls  --print-lines-short Print line section, but do not",
"                         print <pc> address",
"-c   --print-loc         Does nothing, was never very useful",
"-m   --print-macinfo     Print DWARF5 style .debug_macro section",
"                         and DWARF2,3,4 .debug_macinfo section.",
"     --print-machine-arch Print selected items about objects",
"                         and sections",
"-P   --print-producers   Print list of compile units per producer",
"-p   --print-pubnames    Print pubnames section",
"-N   --print-ranges      Print ranges section",
"     --print-raw-rnglists Print entire .debug_rnglists section",
"     --print-raw-loclists Print entire .debug_loclists section",
"-ta  --print-static      Print both static sections",
"-tf  --print-static-func Print static func section",
"-tv  --print-static-var  Print static var section",
"-s   --print-strings     Print raw .debug_str section",
"     --print-str-offsets Print raw .debug_str_offsets section",
"-y   --print-type        Print pubtypes section",
"-w   --print-weakname    Print weakname section",
" ",
"-------------------------------------------------------------------",
"Print Elf Relocation Data",
"-------------------------------------------------------------------",
"  libelf not present, use GNU readelf or readelfobj",
"  to see relocations",
" ",
"-------------------------------------------------------------------",
"Print Elf Section Headers",
"-------------------------------------------------------------------",
"  libelf not present, use GNU readelf or readelfobj",
"  to see elf file details",
" ",
"-------------------------------------------------------------------",
"Check DWARF Integrity",
"-------------------------------------------------------------------",
"-kb  --check-abbrev         Check abbreviations",
"-ka  --check-all            Do all checks",
"-kM  --check-aranges        Check ranges list (.debug_aranges)",
"-kD  --check-attr-dup       Check duplicated attributes",
"                            in dwarfdump itself",
"-kE  --check-attr-encodings Notice attribute/class/form encodings",
"                            and print a report on them",
"-kn  --check-attr-names     Examine names in attributes",
"                            and report when names contain dot (.)",
"                            to find possible naming oddities",
"-kc  --check-constants      Examine DWARF constants",
"-kF  --check-files-lines    Examine integrity of files-lines",
"                            attributes",
"-kR  --check-forward-refs   Check forward references to DIEs",
"                            (declarations)",
"-kx  --check-frame-basic    Basic frames check (.eh_frame,",
"                            .debug_frame)",
"-kxe --check-frame-extended Extensive frames check (.eh_frame,",
"                            .debug_frame)",
"-kf  --check-frame-info     Examine frame information (use with",
"                            -f or -F)",
"-kg  --check-gaps           Check debug info gaps",
"-kl  --check-loc            Check location list (.debug_loc)",
"-kw  --check-macros         Check macros",
"-ke  --check-pubnames       Examine attributes of pubnames",
"-km  --check-ranges         Check ranges list (.debug_ranges)",
"-kS  --check-self-refs      Check self references to DIEs",
"-kd  --check-show           Show check results",
"-ks  --check-silent         Perform checks in silent mode",
"-ki  --check-summary        Display summary for all compilers",
"-kr  --check-tag-attr       Examine tag-attr relation, see -kt",
"                            Does not print a summary, see -ku",
"-kt  --check-tag-tag        Examine tag-tag relations",
"                            Unless -C (--format-extensions)",
"                            option given certain common",
"                            tag-attr and tag-tag extensions are",
"                            assumed to be ok (not reported).",
"                            Does not print a summary, see -ku",
"-ky  --check-type           Examine type info",
"-kG  --check-unique         Print only unique errors",
"-ku  --check-usage          Print tag-tag, tag-attr, and",
"                            attr-form usage summaries",
"-kuf --check-usage-extended The same as -ku.",
"     --check-functions      Adds calls of libdwarf functions not ",
"                            otherwise called (for code coverage).",
" ",
"-------------------------------------------------------------------",
"Print Output Qualifiers",
"-------------------------------------------------------------------",
"-M   --format-attr-name        Print the form name for each",
"                               attribute",
"-d   --format-dense            One line per entry (info section)",
"-e   --format-ellipsis         Short names for tags, attrs etc.",
"-G   --format-global-offsets   Show global die offsets",
"-g   --format-loc              (Was loclist support. Do not use.)",
"     --format-expr-ops-joined  Print each group of DWARF DW_OPs",
"                               on one line rather than one",
"                               per line.",
"-R   --format-registers        Print frame register names as",
"                               r33 etc and allow up to 1200",
"                               registers using a generic",
"                               register set.",
"-Q   --format-suppress-data    Suppress printing section data",
"-x noprintsectiongroups",
"     --format-suppress-group   Do not print section groups",
"-n   --format-suppress-lookup  Suppress frame information function",
"                               name lookup(when printing frame",
"                               information from multi-gigabyte",
"                               object files this",
"                               option may save significant time).",
"-D   --format-suppress-offsets Do not show offsets",
"-x nosanitizestrings",
"     --format-suppress-sanitize Arbitrary string characters",
"                               come thru printf. Can be dangerous",
"                               to your terminal window.",
"-U   --format-suppress-uri     Suppress uri-translate ",
"                               meaning do not translate %xx",
"                               into the byte it represents in uri",
"-q   --format-suppress-uri-msg Suppress uri-did-translate",
"                               notification: by default prints a",
"                               note about each detected uri",
"                               translation.",
"     --format-suppress-utf8    Suppress printing of utf8 as utf8 by",
"                               always using uri-style printing",
"-C   --format-extensions       Print (with -ki) warnings",
"                               for some common DWARF extensions",
"                               (by default extensions accepted",
"                               as standard).",
" ",
"-------------------------------------------------------------------",
"Print Output Limiters",
"-------------------------------------------------------------------",
"-u<file> --format-file=<file>  Print only specified file (CU name)",
"-x groupnumber=<n>    ",
"         --format-group-number=<n> Groupnumber to print",
"-x universalnumber=<n>    ",
"         --format-universalnumber=<n> Binary to print.",
"                               Only applies to",
"                               Mach-O universal binaries.",
"-H<num>  --format-limit=<num>  Limit output to the first <num>",
"                               major units.",
"                               Stop after <num> compilation units",
"-c<str>  --format-producer=<str> Check only specific compiler",
"                               objects  <str> is described by",
"                               'DW_AT_producer'  -c'350.1' ",
"                               check only compiler objects",
"                               with 350.1 in the CU name",
"-cs      --format-snc          Check only SNC compiler objects",
"-cg      --format-gcc          Check only gcc compiler objects",
" ",
"-------------------------------------------------------------------",
"File Specifications",
"-------------------------------------------------------------------",
"-x abi=<abi>     --file-abi=<abi>      Name abi in dwarfdump.conf",
"-x name=<path>   --file-name=<path>    Name dwarfdump.conf",
"-x line5=<val>   --file-line5=<val>    Table DWARF5 new interfaces",
"                                       where <val> is: std or s2l",
"-O file=<path>   --file-output=<path>  Name the output file",
"-x tied=<path>   --file-tied=<path>    Name the Split Dwarf",
"                                       skeleton object file",
"                 --file-use-no-libelf  Use non-libelf to",
"                                       read objects",
"                                         (as much as possible)",
" ",
"-------------------------------------------------------------------",
"GNU debuglink/Apple dSYM options",
"-------------------------------------------------------------------",
" --no-follow-debuglink       Do not follow GNU debuglink, ",
"                             just use the file directly so,",
"                             debuglink  global paths ",
"                             and debugid links are ignored.",
" --add-debuglink_path=<text> Add the path to the list of",
"                             global paths debuglink searches",
" --suppress-debuglink-crc    Tell libdwarf to avoid calculating",
"                             crc values, saving some runtime at",
"                             startup and removing a ",
"                             safety check but allowing debuglink",
"                             and debugid paths to be used.",
" --no-follow-dsym            Do not follow an Apple dSYM directory",
"                             path to find DWARF data.",
"                             .eh_frame format data may be in",
"                             the executable object.",
"-------------------------------------------------------------------",
"Search text in attributes",
"-------------------------------------------------------------------",
"-S any=<text>    --search-any=<text>       Search any <text>",
"-Svany=<text>    --search-any-count=<text> print number of",
"                                           occurrences",
"-S match=<text>  --search-match=<text>     Search matching <text>",
"-Svmatch=<text>  --search-match-count<text> print number of",
"                                           occurrences",
"-S regex=<text>  --search-regex=<text>     Use Posix Basic regular",
"                               expression matching",
"-Svregex=<text>  --search-regex-count<text> print number of",
"                                            occurrences",
"                             only one -S option allowed, any= and",
"                             regex= only usable if the functions",
"                             required are found at configure time",
" ",
"-Wc  --search-print-children Print children tree",
"                             (wide format) with -S",
"-Wp  --search-print-parent   Print parent tree ",
"                             (wide format) with -S",
"-W   --search-print-tree     Print parent/children tree ",
"                             (wide format) with -S",
" ",
"-------------------------------------------------------------------",
"Help, Version, and Miscellaneous",
"-------------------------------------------------------------------",
"-h   --help          Print this dwarfdump help message.",
"-v   --verbose       Show more information.",
"-vv  --verbose-more  Show even more information.",
"-V   --version       Print version information.",
"     --print-section-allocations Print summary of section ",
"                         allocations via mmap and those via malloc.",
"     --print-all-srcfiles print a sorted list of unique ",
"                    file names defined in the DWARF.",
"     --print-language-version-table print the DWARF6 ",
"                         language version table.",
"     --show-dwarfdump-conf Show what dwarfdump.conf is being used.",
"     --show-args    Show the  current date, time, library version,",
"                    dwarfdump version, and command arguments.",
"     --suppress-de-alloc-tree Turns off the libdwarf-cleanup of",
"                    libdwarf-allocated memory on calling",
"                    dwarf_finish(). Used to test that",
"                    dwarfdump does dealloc everywhere",
"                    it should for minimum memory use.",
"     --suppress-harmless-errors Turns off the libdwarf checks",
"                    for harmless errors. Improves libdwarf",
"                    performance.",
"     --no-dup-attr-check Turns off libdwarf checking for",
"                    duplicated compiler-emitted attributes",
"                    in reading abbreviation data so duplicates",
"                    will not cause a DW_DLV_ERROR return from",
"                    libdwarf.  dwarfdump --check-attr-dup will ",
"                    check for them in dwarfdump.",
"     --trace=0      Shows --trace values (1,2,3) that",
"                    turn on detailed tracking of dwarfdump",
"                    internal tables (for debugging dwarfdump).",
"     --allocate-via-mmap  When possible allocations for",
"                    loading sections will be with mmap.",
"                    See also environment variable",
"                    DWARF_WHICH_ALLOC.",
"",
};

enum longopts_vals {
OPT_BEGIN = 999,

/* Check DWARF Integrity         */
OPT_CHECK_ABBREV,             /* -kb  --check-abbrev   */
OPT_CHECK_ALL,                /* -ka  --check-all      */
OPT_CHECK_ARANGES,            /* -kM  --check-aranges  */
OPT_CHECK_ATTR_DUP,           /* -kD  --check-attr-dup */
OPT_CHECK_ATTR_ENCODINGS,     /* -kE  --check-attr-encodings*/
OPT_CHECK_ATTR_NAMES,         /* -kn  --check-attr-names    */
OPT_CHECK_CONSTANTS,          /* -kc  --check-constants     */
OPT_CHECK_FILES_LINES,        /* -kF  --check-files-lines   */
OPT_CHECK_FORWARD_REFS,       /* -kR  --check-forward-refs  */
OPT_CHECK_FRAME_BASIC,        /* -kx  --check-frame-basic   */
OPT_CHECK_FRAME_EXTENDED,     /* -kxe --check-frame-extended*/
OPT_CHECK_FRAME_INFO,         /* -kf  --check-frame-info    */
OPT_CHECK_GAPS,               /* -kg  --check-gaps          */
OPT_CHECK_LOC,                /* -kl  --check-loc           */
OPT_CHECK_MACROS,             /* -kw  --check-macros        */
OPT_CHECK_PUBNAMES,           /* -ke  --check-pubnames      */
OPT_CHECK_RANGES,             /* -km  --check-ranges        */
OPT_CHECK_SELF_REFS,          /* -kS  --check-self-refs     */
OPT_CHECK_SHOW,               /* -kd  --check-show          */
OPT_CHECK_SILENT,             /* -ks  --check-silent        */
OPT_CHECK_SUMMARY,            /* -ki  --check-summary       */
OPT_CHECK_TAG_ATTR,           /* -kr  --check-tag-attr      */
OPT_CHECK_TAG_TAG,            /* -kt  --check-tag-tag       */
OPT_CHECK_TYPE,               /* -ky  --check-type          */
OPT_CHECK_UNIQUE,             /* -kG  --check-unique        */
OPT_CHECK_USAGE,              /* -ku  --check-usage         */
OPT_CHECK_USAGE_EXTENDED,     /* -kuf --check-usage-extended*/
OPT_CHECK_FUNCTIONS,          /*  --check-functions*/

/* Speeds up libdwarf to do this */
OPT_SUPPRESS_HARMLESS,        /* --suppress-harmless-errors */

/* File Specifications    */
OPT_FILE_ABI,          /* -x abi=<abi>    --file-abi=<abi>     */
OPT_FILE_LINE5,        /* -x line5=<val>  --file-line5=<val>   */
OPT_FILE_NAME,         /* -x name=<path>  --file-name=<path>   */
OPT_FILE_OUTPUT,       /* -O file=<path>  --file-output=<path> */
OPT_FILE_TIED,         /* -x tied=<path>  --file-tied=<path>   */
OPT_FILE_USE_NO_LIBELF,/* --file-use-no-libelf=<path>        */

/* Print Output Qualifiers  */
OPT_FORMAT_ATTR_NAME,         /* -M   --format-attr-name       */
OPT_FORMAT_DENSE,             /* -d   --format-dense           */
OPT_FORMAT_ELLIPSIS,          /* -e   --format-ellipsis        */
OPT_FORMAT_EXPR_OPS_JOINED,   /*      --format-expr-ops-joined */
OPT_FORMAT_EXTENSIONS,        /* -C   --format-extensions      */
OPT_FORMAT_GLOBAL_OFFSETS,    /* -G   --format-global-offsets  */
OPT_FORMAT_LOC,               /* -g   --format-loc             */
OPT_FORMAT_REGISTERS,         /* -R   --format-registers       */
OPT_FORMAT_SUPPRESS_DATA,     /* -Q   --format-suppress-data   */
OPT_FORMAT_SUPPRESS_GROUP  ,  /* -x   --format-suppress-group  */
OPT_FORMAT_SUPPRESS_LOOKUP,   /* -n   --format-suppress-lookup */
OPT_FORMAT_SUPPRESS_OFFSETS,  /* -D   --format-suppress-offsets  */
OPT_FORMAT_SUPPRESS_SANITIZE,
    /* -x no-sanitize-strings --format-suppress-sanitize  */
OPT_FORMAT_SUPPRESS_URI,      /* -U   --format-suppress-uri    */
OPT_FORMAT_SUPPRESS_URI_MSG,  /* -q   --format-suppress-uri-msg  */

OPT_FORMAT_SUPPRESS_UTF8,     /*  --format-suppress-utf8 */
OPT_FORMAT_UNIVERSALNUMBER,   /*  --format-universalnumber */

/* Print Output Limiters    */
OPT_FORMAT_FILE,              /* -u<file> --format-file=<file> */
OPT_FORMAT_GCC,               /* -cg      --format-gcc         */
OPT_FORMAT_GROUP_NUMBER,      /* -x<n>  --format-group-number=<n>*/

/* -x<n>  --format-universalnumber=<n>*/
OPT_FORMAT_GROUP_UNIVERSALNUMBER,

OPT_FORMAT_LIMIT,             /* -H<num>  --format-limit=<num>   */
OPT_FORMAT_PRODUCER,          /* -c<str>  --format-producer=<str> */
OPT_FORMAT_SNC,               /* -cs      --format-snc           */

/* Print Debug Sections                                   */
OPT_PRINT_ABBREV,             /* -b   --print-abbrev      */
OPT_PRINT_ALL,                /* -a   --print-all         */
OPT_PRINT_ALL_SRCFILES,       /*      --print-all-srcfiles         */
OPT_PRINT_ARANGES,            /* -r   --print-aranges     */
OPT_PRINT_DEBUG_NAMES,        /*      --print-debug-names */
OPT_PRINT_DEBUG_ADDR,         /*      --print-debug-addr */
OPT_PRINT_GNU_DEBUGLINK,      /*      --print-gnu-debuglink  */
OPT_PRINT_DEBUG_GNU,          /*      --print-debug-gnu   */
OPT_PRINT_DEBUG_SUP,          /*      --print-debug-sup   */
OPT_PRINT_EH_FRAME,           /* -F   --print-eh-frame    */
OPT_PRINT_FISSION,            /* -I   --print-fission     */
OPT_PRINT_FRAME,              /* -f   --print-frame       */
OPT_PRINT_INFO,               /* -i   --print-info        */
OPT_PRINT_LINES,              /* -l   --print-lines       */
OPT_PRINT_LINES_SHORT,        /* -ls  --print-lines-short */
OPT_PRINT_LOC,                /* -c   --print-loc         */
OPT_PRINT_MACINFO,            /* -m   --print-macinfo     */
OPT_PRINT_MACHINE_ARCH,       /*      --print-machine-arch */
OPT_PRINT_PRODUCERS,          /* -P   --print-producers   */
OPT_PRINT_PUBNAMES,           /* -p   --print-pubnames    */
OPT_PRINT_RANGES,             /* -N   --print-ranges      */
OPT_PRINT_RAW_LOCLISTS,       /*      --print-raw-loclists */
OPT_PRINT_RAW_RNGLISTS,       /*      --print-raw-rnglists */
OPT_PRINT_STATIC,             /* -ta  --print-static      */
OPT_PRINT_STATIC_FUNC,        /* -tf  --print-static-func */
OPT_PRINT_STATIC_VAR,         /* -tv  --print-static-var  */
OPT_PRINT_STRINGS,            /* -s   --print-strings     */
OPT_PRINT_STR_OFFSETS,        /*      --print-str-offsets */
OPT_PRINT_TYPE,               /* -y   --print-type        */
OPT_PRINT_WEAKNAME,           /* -w   --print-weakname    */
OPT_PRINT_ALLOCATIONS,        /* --print-section-allocations */
OPT_PRINT_LANGUAGE_VERSION_TABLE,
    /* --print-language-version_table */
/* debuglink options */
OPT_NO_FOLLOW_DEBUGLINK,     /* --no-follow-debuglink */
OPT_ADD_DEBUGLINK_PATH,      /* --add-debuglink-path=<text> */
OPT_SUPPRESS_DEBUGLINK_CRC,  /* --suppress-debuglink-crc */
OPT_NO_FOLLOW_DSYM,          /* --no-follow-dsym */

/* Search text in attributes                        */
OPT_SEARCH_ANY,       /* -S any=<text>   --search-any=<text>  */
OPT_SEARCH_ANY_COUNT, /* -Svany=<text>   --search-any-count=<text> */
OPT_SEARCH_MATCH,     /* -S match=<text> --search-match=<text>     */
OPT_SEARCH_MATCH_COUNT,
    /* -Svmatch=<text> --search-match-count<text>*/
OPT_SEARCH_PRINT_CHILDREN, /* -Wc --search-print-children */
OPT_SEARCH_PRINT_PARENT, /* -Wp --search-print-parent    */
OPT_SEARCH_PRINT_TREE,        /* -W  --search-print-tree  */
OPT_SEARCH_REGEX,       /* -S regex=<text> --search-regex=<text> */
OPT_SEARCH_REGEX_COUNT,
    /* -Svregex=<text> --search-regex-count<text>*/

/* Help & Version                                            */
OPT_HELP,                     /* -h  --help                  */
OPT_VERBOSE,                  /* -v  --verbose               */
OPT_VERBOSE_MORE,             /* -vv --verbose-more          */
OPT_VERSION,                  /* -V  --version               */
OPT_SHOW_DWARFDUMP_CONF,      /*   --show-dwarfdump-conf     */
OPT_SHOW_ARGS,                /*   --show-args               */

/* Trace                                                     */
OPT_TRACE,                    /* -# --trace=<num>            */

OPT_NO_DUP_ATTR_CHECK,        /*  --no-dup-attr-check  */
OPT_ALLOC_TREE_OFF,           /* --suppress-de-alloc-tree */
OPT_ALLOCATE_VIA_MMAP,        /* --allocate-via-mmap */
OPT_END
};

static struct dwoption longopts[] =  {

/* Check DWARF Integrity. */
{"check-abbrev",         dwno_argument, 0, OPT_CHECK_ABBREV        },
{"check-all",            dwno_argument, 0, OPT_CHECK_ALL           },
{"check-aranges",        dwno_argument, 0, OPT_CHECK_ARANGES       },
{"check-attr-dup",       dwno_argument, 0, OPT_CHECK_ATTR_DUP      },
{"check-attr-encodings", dwno_argument, 0, OPT_CHECK_ATTR_ENCODINGS},
{"check-attr-names",     dwno_argument, 0, OPT_CHECK_ATTR_NAMES    },
{"check-constants",      dwno_argument, 0, OPT_CHECK_CONSTANTS     },
{"check-files-lines",    dwno_argument, 0, OPT_CHECK_FILES_LINES   },
{"check-forward-refs",   dwno_argument, 0, OPT_CHECK_FORWARD_REFS  },
{"check-frame-basic",    dwno_argument, 0, OPT_CHECK_FRAME_BASIC   },
{"check-frame-extended", dwno_argument, 0, OPT_CHECK_FRAME_EXTENDED},
{"check-frame-info",     dwno_argument, 0, OPT_CHECK_FRAME_INFO    },
{"check-gaps",           dwno_argument, 0, OPT_CHECK_GAPS          },
{"check-loc",            dwno_argument, 0, OPT_CHECK_LOC           },
{"check-macros",         dwno_argument, 0, OPT_CHECK_MACROS        },
{"check-pubnames",       dwno_argument, 0, OPT_CHECK_PUBNAMES      },
{"check-ranges",         dwno_argument, 0, OPT_CHECK_RANGES        },
{"check-self-refs",      dwno_argument, 0, OPT_CHECK_SELF_REFS     },
{"check-show",           dwno_argument, 0, OPT_CHECK_SHOW          },
{"check-silent",         dwno_argument, 0, OPT_CHECK_SILENT        },
{"check-summary",        dwno_argument, 0, OPT_CHECK_SUMMARY       },
{"check-tag-attr",       dwno_argument, 0, OPT_CHECK_TAG_ATTR      },
{"check-tag-tag",        dwno_argument, 0, OPT_CHECK_TAG_TAG       },
{"check-type",           dwno_argument, 0, OPT_CHECK_TYPE          },
{"check-unique",         dwno_argument, 0, OPT_CHECK_UNIQUE        },
{"check-usage",          dwno_argument, 0, OPT_CHECK_USAGE         },
{"check-usage-extended", dwno_argument, 0, OPT_CHECK_USAGE_EXTENDED},
{"check-functions",      dwno_argument, 0, OPT_CHECK_FUNCTIONS},

/* File Specifications. */
{"file-abi",    dwrequired_argument, 0, OPT_FILE_ABI   },
{"file-line5",  dwrequired_argument, 0, OPT_FILE_LINE5 },
{"file-name",   dwrequired_argument, 0, OPT_FILE_NAME  },
{"file-output", dwrequired_argument, 0, OPT_FILE_OUTPUT},
{"file-tied",   dwrequired_argument, 0, OPT_FILE_TIED  },
{"file-use-no-libelf",   dwno_argument, 0, OPT_FILE_USE_NO_LIBELF  },

/* Print Output Qualifiers. */
{"format-attr-name",         dwno_argument, 0,
    OPT_FORMAT_ATTR_NAME        },
{"format-dense",             dwno_argument, 0,
    OPT_FORMAT_DENSE            },
{"format-ellipsis",          dwno_argument, 0,
    OPT_FORMAT_ELLIPSIS         },
{"format-expr-ops-joined",   dwno_argument, 0,
    OPT_FORMAT_EXPR_OPS_JOINED },
{"format-extensions",        dwno_argument, 0,
    OPT_FORMAT_EXTENSIONS       },
{"format-global-offsets",    dwno_argument, 0,
    OPT_FORMAT_GLOBAL_OFFSETS   },
{"format-loc",               dwno_argument, 0,
    OPT_FORMAT_LOC              },
{"format-registers",         dwno_argument, 0,
    OPT_FORMAT_REGISTERS        },
{"format-suppress-data",     dwno_argument, 0,
    OPT_FORMAT_SUPPRESS_DATA    },
{"format-suppress-group",    dwno_argument, 0,
    OPT_FORMAT_SUPPRESS_GROUP   },
{"format-suppress-lookup",   dwno_argument, 0,
    OPT_FORMAT_SUPPRESS_LOOKUP  },
{"format-suppress-offsets",  dwno_argument, 0,
OPT_FORMAT_SUPPRESS_OFFSETS },
{"format-suppress-sanitize", dwno_argument, 0,
    OPT_FORMAT_SUPPRESS_SANITIZE},
{"format-suppress-uri",      dwno_argument, 0,
    OPT_FORMAT_SUPPRESS_URI     },
{"format-suppress-uri-msg",  dwno_argument, 0,
    OPT_FORMAT_SUPPRESS_URI_MSG },
{"format-suppress-utf8",  dwno_argument, 0,
    OPT_FORMAT_SUPPRESS_UTF8 },

/* Print Output Limiters. */
{"format-file",         dwrequired_argument, 0, OPT_FORMAT_FILE },
{"format-gcc",          dwno_argument,       0, OPT_FORMAT_GCC },
{"format-group-number", dwrequired_argument, 0,
    OPT_FORMAT_GROUP_NUMBER},
{"format-universalnumber", dwrequired_argument, 0,
    OPT_FORMAT_UNIVERSALNUMBER},
{"format-limit",        dwrequired_argument, 0, OPT_FORMAT_LIMIT },
{"format-producer",     dwrequired_argument, 0, OPT_FORMAT_PRODUCER},
{"format-snc",          dwno_argument,       0, OPT_FORMAT_SNC },

/* Print Debug Sections. */
{"print-abbrev",      dwno_argument, 0, OPT_PRINT_ABBREV     },
{"print-all",         dwno_argument, 0, OPT_PRINT_ALL        },
{"print-all-srcfiles",dwno_argument, 0, OPT_PRINT_ALL_SRCFILES },
{"print-aranges",     dwno_argument, 0, OPT_PRINT_ARANGES    },
{"print-debug-addr",  dwno_argument, 0, OPT_PRINT_DEBUG_ADDR},
{"print-debug-names", dwno_argument, 0, OPT_PRINT_DEBUG_NAMES},
{"print-gnu-debuglink", dwno_argument,0,OPT_PRINT_GNU_DEBUGLINK},
{"print-debug-gnu",   dwno_argument, 0, OPT_PRINT_DEBUG_GNU  },
{"print-debug-sup",   dwno_argument, 0, OPT_PRINT_DEBUG_SUP  },
{"print-eh-frame",    dwno_argument, 0, OPT_PRINT_EH_FRAME   },
{"print-fission",     dwno_argument, 0, OPT_PRINT_FISSION    },
{"print-frame",       dwno_argument, 0, OPT_PRINT_FRAME      },
{"print-info",        dwno_argument, 0, OPT_PRINT_INFO       },
{"print-language-version-table",       dwno_argument, 0,
    OPT_PRINT_LANGUAGE_VERSION_TABLE },
{"print-lines",       dwno_argument, 0, OPT_PRINT_LINES      },
{"print-lines-short", dwno_argument, 0, OPT_PRINT_LINES_SHORT},
{"print-loc",         dwno_argument, 0, OPT_PRINT_LOC        },
{"print-macinfo",     dwno_argument, 0, OPT_PRINT_MACINFO    },
{"print-machine-arch", dwno_argument, 0, OPT_PRINT_MACHINE_ARCH},
{"print-producers",   dwno_argument, 0, OPT_PRINT_PRODUCERS  },
{"print-pubnames",    dwno_argument, 0, OPT_PRINT_PUBNAMES   },
{"print-ranges",      dwno_argument, 0, OPT_PRINT_RANGES     },
{"print-raw-loclists",dwno_argument, 0, OPT_PRINT_RAW_LOCLISTS},
{"print-raw-rnglists",dwno_argument, 0, OPT_PRINT_RAW_RNGLISTS},
{"print-static",      dwno_argument, 0, OPT_PRINT_STATIC     },
{"print-static-func", dwno_argument, 0, OPT_PRINT_STATIC_FUNC},
{"print-static-var",  dwno_argument, 0, OPT_PRINT_STATIC_VAR },
{"print-strings",     dwno_argument, 0, OPT_PRINT_STRINGS    },
{"print-str-offsets", dwno_argument, 0, OPT_PRINT_STR_OFFSETS},
{"print-type",        dwno_argument, 0, OPT_PRINT_TYPE       },
{"print-weakname",    dwno_argument, 0, OPT_PRINT_WEAKNAME   },
{"print-section-allocations", dwno_argument, 0,
    OPT_PRINT_ALLOCATIONS },

/*  GNU debuglink options */
{"no-follow-debuglink", dwno_argument, 0,OPT_NO_FOLLOW_DEBUGLINK},
{"add-debuglink-path", dwrequired_argument, 0,OPT_ADD_DEBUGLINK_PATH},
{"suppress-debuglink-crc", dwno_argument, 0,
    OPT_SUPPRESS_DEBUGLINK_CRC},
{"no-follow-dsym", dwno_argument, 0,OPT_NO_FOLLOW_DSYM},

/* Search text in attributes. */
{"search-any",            dwrequired_argument, 0,OPT_SEARCH_ANY  },
{"search-any-count",      dwrequired_argument, 0,
    OPT_SEARCH_ANY_COUNT     },
{"search-match",          dwrequired_argument, 0,
    OPT_SEARCH_MATCH },
{"search-match-count",    dwrequired_argument, 0,
    OPT_SEARCH_MATCH_COUNT   },
{"search-print-children", dwno_argument,  0,
    OPT_SEARCH_PRINT_CHILDREN},
{"search-print-parent",   dwno_argument,  0,
    OPT_SEARCH_PRINT_PARENT  },
{"search-print-tree",     dwno_argument,  0,
    OPT_SEARCH_PRINT_TREE    },
{"search-regex",          dwrequired_argument, 0, OPT_SEARCH_REGEX },
{"search-regex-count",    dwrequired_argument, 0,
    OPT_SEARCH_REGEX_COUNT   },

/* Help & Version & miscellaneous. */
{"help",          dwno_argument, 0, OPT_HELP         },
{"verbose",       dwno_argument, 0, OPT_VERBOSE      },
{"verbose-more",  dwno_argument, 0, OPT_VERBOSE_MORE },
{"version",       dwno_argument, 0, OPT_VERSION      },
{"show-dwarfdump-conf",dwno_argument, 0, OPT_SHOW_DWARFDUMP_CONF },
{"show-args",     dwno_argument, 0, OPT_SHOW_ARGS },
{"no-dup-attr-check", dwno_argument, 0, OPT_NO_DUP_ATTR_CHECK },
{"allocate-via-mmap", dwno_argument, 0, OPT_ALLOCATE_VIA_MMAP },

/* Trace. */
{"trace", dwrequired_argument, 0, OPT_TRACE},

{"suppress-de-alloc-tree",dwno_argument,0,OPT_ALLOC_TREE_OFF},
{"suppress-harmless-errors",dwno_argument,0,OPT_SUPPRESS_HARMLESS},
{0,0,0,0}
};

/*  Handlers for the command line options. */
/*  Option '--print-debug-addr' */
void arg_print_debug_addr(void)
{
    glflags.gf_debug_addr_flag = TRUE;
}

/*  Option '--print-debug-names' */
void arg_print_debug_names(void)
{
    glflags.gf_debug_names_flag = TRUE;
}
/*  Option '--print-gnu-debuglink' */
void arg_print_gnu_debuglink(void)
{
    glflags.gf_gnu_debuglink_flag = TRUE;
}
/*  Option '--print-debug-gnu' */
void arg_print_debug_gnu(void)
{
    glflags.gf_debug_gnu_flag = TRUE;
}
/*  Option '--print-debug-sup' */
void arg_print_debug_sup(void)
{
    glflags.gf_debug_sup_flag = TRUE;
}

/*  Option '--print-str-offsets' */
void arg_print_str_offsets(void)
{
    glflags.gf_print_str_offsets = TRUE;
}

void arg_trace(void)
{
    int nTraceLevel = 0;

    if (!dwoptarg || !dwoptarg[0]) {
        /*  --trace=0 to dwarfdump gets us here. */
        print_usage_message(usage_debug_text);
        makename_destructor();
        global_destructors();
        exit(OKAY);
    }

    nTraceLevel = atoi(dwoptarg);
    if (nTraceLevel >= 0 && nTraceLevel <= MAX_TRACE_LEVEL) {
        glflags.nTrace[nTraceLevel] = 1;
    }
    /* Display dwarfdump debug options. */
    if ( glflags.nTrace[KIND_OPTIONS]) {
        /*  --trace=0 to dwarfdump gets us here. */
        print_usage_message(usage_debug_text);
        makename_destructor();
        global_destructors();
        exit(OKAY);
    }
}

/*  Option '-a' */
void arg_print_all(void)
{
    suppress_check_dwarf();
    do_all();
}
/*  Option '--print-all-srcfiles' */
void arg_print_all_srcfiles(void)
{
    glflags.gf_print_all_srcfiles = TRUE;
}

/*  Option '-b' */
void arg_print_abbrev(void)
{
    glflags.gf_abbrev_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-c[...]' */
void arg_c_multiple_selection(void)
{
    /* Specify compiler name. */
    if (dwoptarg) {
        switch (dwoptarg[0]) {
        case 's': arg_format_snc();      break;
        case 'g': arg_format_gcc();      break;
        default:  arg_format_producer(); break;
        }
    } else {
        arg_print_loc();
    }
}

/*  Option '-c' with no other letters. See just above. */
void arg_print_loc(void)
{
    printf("The -c option to print .debug_loc (unsafely) "
        "is ignored\n");
    /*  glflags.gf_loc_flag = TRUE;
        suppress_check_dwarf();  */
}

/*  Option '-cs' */
void arg_format_snc(void)
{
    /* -cs : Check SNC compiler */
    glflags.gf_check_snc_compiler = TRUE;
    glflags.gf_check_all_compilers = FALSE;
}

/*  Option '-cg' --format-gcc */
void arg_format_gcc(void)
{
    /* -cg : Check GCC compiler */
    glflags.gf_check_gcc_compiler = TRUE;
    glflags.gf_check_all_compilers = FALSE;
}

/*  Option '-c<producer>' */
void arg_format_producer(void)
{
    /*  Assume a compiler version to check,
        most likely a substring of a compiler name.  */
    if (!dwoptarg || !dwoptarg[0]) {
        printf("ERROR: -c<producer> requires "
            " a producer string\n");
        glflags.gf_count_major_errors++;
    }
    if (!record_producer(dwoptarg)) {
        printf("ERROR: Compiler table max %d exceeded, "
            "limiting the tracked compilers to %d\n",
            COMPILER_TABLE_MAX,COMPILER_TABLE_MAX);
        glflags.gf_count_major_errors++;
    }
}

/*  Option '--format-expr-ops-joined'
    restoring pre- December 2020 expression block printing */
void arg_format_expr_ops_joined(void)
{
    glflags.gf_expr_ops_joined = TRUE;
}

/* Option --suppress-harmless-errors */
void arg_suppress_harmless(void)
{
    glflags.gf_suppress_harmless = TRUE;
}
/*  Option '-C' -format-extensions */
void arg_format_extensions(void)
{
    glflags.gf_suppress_check_extensions_tables = TRUE;
}

/*  Option '-d' */
void arg_format_dense(void)
{
    glflags.gf_do_print_dwarf = TRUE;
    /*  This is sort of useless unless printing,
        but harmless, so we do not insist we
        are printing with suppress_check_dwarf(). */
    glflags.dense = TRUE;
}

/*  Option '-D' */
void arg_format_suppress_offsets(void)
{
    /* Do not emit offset in output */
    glflags.gf_display_offsets = FALSE;
}

/*  Option '-e' */
void arg_format_ellipsis(void)
{
    suppress_check_dwarf();
    glflags.ellipsis = TRUE;
}

/*  Option '-f' */
void arg_print_debug_frame(void)
{
    glflags.gf_frame_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-F' */
void arg_print_gnu_frame(void)
{
    glflags.gf_eh_frame_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-g' */
void arg_format_loc(void)
{
    /*  info_flag = TRUE;  removed  from -g. Nov 2015
        Entirely removed the effect of the -g 2021 */
    glflags.gf_use_old_dwarf_loclist = FALSE;
    suppress_check_dwarf();
}

/*  Option '-G' */
void arg_format_global_offsets(void)
{
    glflags.gf_show_global_offsets = TRUE;
}

/*  Option '-h' */
void arg_h_multiple_selection(void)
{
    if (dwoptarg) {
        arg_usage_error = TRUE;
    } else {
        arg_help();
    }
}

/*  Option '-h' */
void arg_help(void)
{
    print_usage_message(usage_long_text);
    makename_destructor();
    global_destructors();
    exit(OKAY);
}

/*  Option '-H' */
void arg_format_limit(void)
{
    int break_val = 0;

    if (!dwoptarg || !dwoptarg[0]) {
        printf("\nERROR The -H option requires a limit value\n");
        glflags.gf_count_major_errors++;
        return;
    }
    break_val = atoi(dwoptarg);
    if (break_val > 0) {
        glflags.break_after_n_units = break_val;
    }
}

/*  Option '-i' */
void arg_print_info(void)
{
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-I' */
void arg_print_fission(void)
{
    glflags.gf_gdbindex_flag = TRUE;
    glflags.gf_gnu_debuglink_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-k[...]' */
void arg_k_multiple_selection(void)
{
    if (!dwoptarg || !dwoptarg[0]) {
        printf("ERROR: The -k option requires at least"
            " one selection\n");
        glflags.gf_count_major_errors++;
        return;
    }
    switch (dwoptarg[0]) {
    case 'a': arg_check_all();             break;
    case 'b': arg_check_abbrev();          break;
    case 'c': arg_check_constants();       break;
    case 'd': arg_check_show();            break;
    case 'D': arg_check_attr_dup();        break;
    case 'e': arg_check_pubnames();        break;
    case 'E': arg_check_attr_encodings();  break;
    case 'f': arg_check_frame_info();      break;
    case 'F': arg_check_files_lines();     break;
    case 'g': arg_check_gaps();            break;
    case 'G': arg_check_unique();          break;
    case 'i': arg_check_summary();         break;
    case 'l': arg_check_loc();             break;
    case 'm': arg_check_ranges();          break;
    case 'M': arg_check_aranges();         break;
    case 'n': arg_check_attr_names();      break;
    case 'r': arg_check_tag_attr();        break;
    case 'R': arg_check_forward_refs();    break;
    case 's': arg_check_silent();          break;
    case 'S': arg_check_self_refs();       break;
    case 't': arg_check_tag_tag();         break;
    case 'u': arg_ku_multiple_selection(); break;
    case 'w': arg_check_macros();          break;
    case 'x': arg_kx_multiple_selection(); break;
    case 'y': arg_check_type();            break;
    default: arg_usage_error = TRUE;       break;
    }
}

/*  Option '-ka' */
void arg_check_all(void)
{
    suppress_print_dwarf();
    glflags.gf_check_pubname_attr = TRUE;
    glflags.gf_check_tag_attr = TRUE;
    glflags.gf_check_tag_tree = TRUE;
    glflags.gf_check_type_offset = TRUE;
    glflags.gf_gnu_debuglink_flag = FALSE;
    glflags.gf_check_names = TRUE;
    glflags.gf_pubnames_flag = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    glflags.gf_gdbindex_flag = TRUE;
    glflags.gf_check_decl_file = TRUE;
    glflags.gf_check_macros = TRUE;
    glflags.gf_check_frames = TRUE;
    glflags.gf_check_frames_extended = FALSE;
    glflags.gf_check_locations = TRUE;
    glflags.gf_frame_flag = TRUE;
    glflags.gf_eh_frame_flag = TRUE;
    glflags.gf_check_ranges = TRUE;
    glflags.gf_check_lines = TRUE;
    glflags.gf_check_fdes = TRUE;
    glflags.gf_check_harmless = TRUE;
    glflags.gf_check_aranges = TRUE;
    glflags.gf_aranges_flag = TRUE;  /* Aranges section */
    glflags.gf_check_abbreviations = TRUE;
    glflags.gf_check_dwarf_constants = TRUE;
    glflags.gf_check_di_gaps = TRUE;
    glflags.gf_check_forward_decl = TRUE;
    glflags.gf_check_self_references = TRUE;
    glflags.gf_check_attr_encoding = TRUE;
    glflags.gf_print_usage_tag_attr = TRUE;

    /*  Not letting libdwarf check */
    glflags.gf_no_check_duplicated_attributes = TRUE;
    /*  Let dwarfdump check */
    glflags.gf_check_duplicated_attributes = TRUE;

    glflags.gf_check_functions = TRUE;
}

/*  Option '-kb' --check-abbrev */
void arg_check_abbrev(void)
{
    /* Abbreviations */
    suppress_print_dwarf();
    glflags.gf_check_abbreviations = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    /*  For some checks is worth trying the plain
        .debug_abbrev section on its own. */
    glflags.gf_abbrev_flag = TRUE;
}

/*  Option '-kc' --check-constants */
void arg_check_constants(void)
{
    /* DWARF constants */
    suppress_print_dwarf();
    glflags.gf_check_dwarf_constants = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kd' --check-show */
void arg_check_show(void)
{
    /* Display check results */
    suppress_print_dwarf();
    glflags.gf_check_show_results = TRUE;
}

/*  Option '-kD' --check-attr-dup */
void arg_check_attr_dup(void)
{
    /* Check duplicated attributes in dwarfdump */
    suppress_print_dwarf();
    /* avoid checking attr dups in libdwarf itself */
    glflags.gf_no_check_duplicated_attributes = TRUE;
    /* Let dwarfdump find the dups */
    glflags.gf_check_duplicated_attributes = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    /*  For some checks is worth trying the plain
        .debug_abbrev section on its own. */
    glflags.gf_abbrev_flag = TRUE;
}

/*  Option '-ke' --check-pubnames */
void arg_check_pubnames(void)
{
    suppress_print_dwarf();
    glflags.gf_check_pubname_attr = TRUE;
    glflags.gf_pubnames_flag = TRUE;
    glflags.gf_check_harmless = TRUE;
    glflags.gf_check_fdes = TRUE;
}

/*  Option '-kE' --check-attr-encodings */
void arg_check_attr_encodings(void)
{
    /* Attributes encoding usage */
    suppress_print_dwarf();
    glflags.gf_check_attr_encoding = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kf' --check-frame-info */
void arg_check_frame_info(void)
{
    suppress_print_dwarf();
    glflags.gf_check_harmless = TRUE;
    glflags.gf_check_fdes = TRUE;
}

/*  Option '-kF' --check-files-lines */
void arg_check_files_lines(void)
{
    /* files-lines */
    suppress_print_dwarf();
    glflags.gf_check_decl_file = TRUE;
    glflags.gf_check_lines = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kg' */
void arg_check_gaps(void)
{
    /* Check debug info gaps */
    suppress_print_dwarf();
    glflags.gf_check_di_gaps = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kG' */
void arg_check_unique(void)
{
    /* Print just global (unique) errors */
    suppress_print_dwarf();
    glflags.gf_print_unique_errors = TRUE;
}

/*  Option '-ki' */
void arg_check_summary(void)
{
    /* Summary for each compiler */
    suppress_print_dwarf();
    glflags.gf_print_summary_all = TRUE;
}

/*  Option '-kl' --check-loc */
void arg_check_loc(void)
{
    /* Locations list */
    suppress_print_dwarf();
    glflags.gf_check_locations = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    glflags.gf_loc_flag = TRUE;
}

/*  Option '-km' */
void arg_check_ranges(void)
{
    /* Ranges */
    suppress_print_dwarf();
    glflags.gf_check_ranges = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kM' */
void arg_check_aranges(void)
{
    /* Aranges */
    suppress_print_dwarf();
    glflags.gf_check_aranges = TRUE;
    glflags.gf_aranges_flag = TRUE;
}

/*  Option '-kn'
    Force examination of .debug_info and .debug_types
    and record attribute name (DW_AT_name). Will report any names
    with dot (.) except in compile-unit DIEs
    as those have source names like t.c so dot is
    to be expected.  */
void arg_check_attr_names(void)
{
    /* invalid names */
    suppress_print_dwarf();
    glflags.gf_check_names = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kr --check-tag-attr'
    Force examination of .debug_info and .debug_types
    and record tag/attr uses. */
void arg_check_tag_attr(void)
{
    suppress_print_dwarf();
    glflags.gf_check_tag_attr = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    glflags.gf_check_harmless = TRUE;
}

/*  Option '-kR'
    Force examination of .debug_info and .debug_types
    and record tag/attr uses. */
void arg_check_forward_refs(void)
{
    /* forward declarations in DW_AT_specification */
    suppress_print_dwarf();
    glflags.gf_check_forward_decl = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-ks' */
void arg_check_silent(void)
{
    /* Check verbose mode */
    suppress_print_dwarf();
    glflags.gf_check_verbose_mode = FALSE;
}

/*  Option '-kS' --check-self-refs */
void arg_check_self_refs(void)
{
    /*  self references in:
        DW_AT_specification, DW_AT_type, DW_AT_abstract_origin */
    suppress_print_dwarf();
    glflags.gf_check_self_references = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kt'--check-tag-tag
    Force examination of .debug_info and .debug_types
    and record tag and attr uses. */
void arg_check_tag_tag(void)
{
    suppress_print_dwarf();
    glflags.gf_check_tag_tree = TRUE;
    glflags.gf_check_harmless = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-ku[...]' */
void arg_ku_multiple_selection(void)
{
    /* Tag-Tree and Tag-Attr usage */
    if (!dwoptarg || !dwoptarg[0]) {
        printf("ERROR: The -ku option requires at least"
            " one selection\n");
        glflags.gf_count_major_errors++;
        return;
    }
    if (dwoptarg[1]) {
        switch (dwoptarg[1]) {
        case 'f': arg_check_usage_extended(); break;
        default: arg_usage_error = TRUE;      break;
        }
    } else {
        arg_check_usage();
    }
}

/*  Option '-ku' --check-usage  */
void arg_check_usage(void)
{
    suppress_print_dwarf();
    glflags.gf_print_usage_tag_attr = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kuf' --check-usage-extended,
    Now obsolete as all such reports
    are full reports, but accepted and means
    the same as -ku.  */
void arg_check_usage_extended(void)
{
    arg_check_usage();
}

/*  Option '-kw' --check-macros */
void arg_check_macros(void)
{
    suppress_print_dwarf();
    glflags.gf_check_macros = TRUE;
    glflags.gf_macro_flag = TRUE;
    glflags.gf_macinfo_flag = TRUE;
}

/*  Option '-kx[...]' */
void arg_kx_multiple_selection(void)
{
    /* Frames check */
    if (dwoptarg[1]) {
        switch (dwoptarg[1]) {
        case 'e': arg_check_frame_extended(); break;
        default: arg_usage_error = TRUE;      break;
        }
    } else {
        arg_check_frame_basic();
    }
}

/*  Option '-kx' --check-frame-basic */
void arg_check_frame_basic(void)
{
    suppress_print_dwarf();
    glflags.gf_check_frames = TRUE;
    glflags.gf_frame_flag = TRUE;
    glflags.gf_eh_frame_flag = TRUE;
}

/*  Option '-kxe' --check-frame-extended */
void arg_check_frame_extended(void)
{
    arg_check_frame_basic();

    /* -xe : Extended frames check */
    glflags.gf_check_frames = FALSE;
    glflags.gf_check_frames_extended = TRUE;
}

/*  Option '-ky' */
void arg_check_type(void)
{
    suppress_print_dwarf();
    glflags.gf_check_type_offset = TRUE;
    glflags.gf_check_harmless = TRUE;
    glflags.gf_check_decl_file = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_pubtypes_flag = TRUE;
    glflags.gf_check_ranges = TRUE;
    glflags.gf_check_aranges = TRUE;
}

/*  Option '--check-functions' */
void arg_check_functions(void)
{
    suppress_print_dwarf();
    glflags.gf_check_functions = TRUE;
}

/*  Option '-l[...]' */
void arg_l_multiple_selection(void)
{
    if (dwoptarg) {
        switch (dwoptarg[0]) {
        case 's': arg_print_lines_short(); break;
        default: arg_usage_error = TRUE;   break;
        }
    } else {
        arg_print_lines();
    }
}

/*  Option '--print-language-version-table' */
void arg_print_language_version_table(void)
{
    /* shows the DWARF6 language version table */
    glflags.gf_print_language_version_table = TRUE;
    suppress_check_dwarf();
}

/*  Option '-l' */
void arg_print_lines(void)
{
    /* Enable to suppress offsets printing */
    glflags.gf_line_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-ls' */
void arg_print_lines_short(void)
{
    /* -ls : suppress <pc> addresses */
    glflags.gf_line_print_pc = FALSE;
    arg_print_lines();
}

/*  Option '-m' */
void arg_print_macinfo(void)
{
    glflags.gf_macinfo_flag = TRUE; /* DWARF2,3,4 */
    glflags.gf_macro_flag   = TRUE; /* DWARF5 */
    suppress_check_dwarf();
}
/*  Option '--print-machine-arch' */
void arg_print_machine_arch(void)
{
    glflags.gf_machine_arch_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-M' */
void arg_format_attr_name(void)
{
    glflags.show_form_used =  TRUE;
}

/*  Option '-n' */
void arg_format_suppress_lookup(void)
{
    glflags.gf_suppress_nested_name_search = TRUE;
}

/*  Option '-N' */
void arg_print_ranges(void)
{
    glflags.gf_ranges_flag = TRUE;
    suppress_check_dwarf();
}
/*  Option '--print-raw-rnglists' */
void arg_print_raw_rnglists(void)
{
    glflags.gf_print_raw_rnglists = TRUE;
    suppress_check_dwarf();
}
/*  Option '--print-raw-loclists' */
void arg_print_raw_loclists(void)
{
    glflags.gf_print_raw_loclists = TRUE;
    suppress_check_dwarf();
}

/*  Option '-O' */
void arg_O_multiple_selection(void)
{
    /* Output filename */
    /*  -O file=<filename> */
    if (!dwoptarg || !dwoptarg[0]) {
        arg_usage_error = TRUE;
        return;
    }
    if (strncmp(dwoptarg,"file=",5) == 0) {
        dwoptarg = &dwoptarg[5];
        arg_file_output();
    } else {
        arg_usage_error = TRUE;
    }
}

/*  Option '-O file=' */
void arg_file_output(void)
{
    const char *ctx = arg_option > OPT_BEGIN ?
        "--file-output=" : "-O file=";
    const char *path = 0;

    if (!dwoptarg || !dwoptarg[0]) {
        arg_usage_error = TRUE;
        return;
    }
    path = do_uri_translation(dwoptarg,ctx);
    if (strlen(path) > 0) {
        glflags.output_file = path;
    } else {
        arg_usage_error = TRUE;
    }
}

/*  Option '-p' */
void arg_print_pubnames(void)
{
    glflags.gf_pubnames_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-P' */
void arg_print_producers(void)
{
    /* List of CUs per compiler */
    glflags.gf_producer_children_flag = TRUE;
}

/*  Option '-q' */
void arg_format_suppress_uri_msg(void)
{
    /* Suppress uri-did-translate notification */
    glflags.gf_do_print_uri_in_input = FALSE;
}
/*  Option '--format-suppress-utf8' */
void arg_format_suppress_utf8(void)
{
    /* Suppress uri-did-translate notification */
    glflags.gf_print_utf8_flag = FALSE;
}

/*  Option '-Q' */
void arg_format_suppress_data(void)
{
    /* Q suppresses section data printing. */
    glflags.gf_do_print_dwarf = FALSE;
}

/*  Option '-r' */
void arg_print_aranges(void)
{
    glflags.gf_aranges_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-R' */
void arg_format_registers(void)
{
    glflags.gf_generic_1200_regs = TRUE;
}

/*  Option '-s' */
void arg_print_strings(void)
{
    glflags.gf_string_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-S' */
void arg_S_multiple_selection(void)
{
    /* 'v' option, to print number of occurrences */
    /* -S[v]match|any|regex=text*/
    if (!dwoptarg || !dwoptarg[0]) {
        arg_usage_error = TRUE;
        return;
    }
    if (dwoptarg[0] == 'v') {
        ++dwoptarg;
        arg_search_count();
    }

    if (strncmp(dwoptarg,"match=",6) == 0) {
        dwoptarg = &dwoptarg[6];
        arg_search_match();
    } else if (strncmp(dwoptarg,"any=",4) == 0) {
        dwoptarg = &dwoptarg[4];
        arg_search_any();
    }
    else if (strncmp(dwoptarg,"regex=",6) == 0) {
        dwoptarg = &dwoptarg[6];
        arg_search_regex();
    }
    else {
        arg_search_invalid();
    }
}

/*  Option --no-follow-debuglink */
void arg_no_follow_debuglink(void)
{
    glflags.gf_no_follow_debuglink = TRUE;
}
/*  Option --no-follow-dsym */
void arg_no_follow_dsym(void)
{
    glflags.gf_no_follow_dsym = TRUE;
}

/*  Option --suppress-debuglink-crc */
void arg_suppress_debuglink_crc(void)
{
    dwarf_suppress_debuglink_crc(1);
}

static int
insert_debuglink_path(char *p)
{
    char ** newarray = 0;
    unsigned int curcount = glflags.gf_global_debuglink_count;
    unsigned newcount = curcount+1;
    unsigned u = 0;
    char * pstr = 0;

    newarray = (char **)malloc(newcount * sizeof(char *));
    if (!newarray) {
        printf("ERROR Unable to malloc space for"
            " debuglink paths. "
            " malloc %u pointers failed.\n",newcount);
        printf("Global debuglink path ignored: %s\n",
            sanitized(p));
        glflags.gf_count_major_errors++;
        return DW_DLV_ERROR;
    }
    pstr = strdup(p);
    if (!pstr) {
        printf("ERROR Unable to malloc space"
            " for debuglink path: "
            "count stays at %u\n",curcount);
        printf("Global debuglink path ignored: %s\n",
            sanitized(p));
        glflags.gf_count_major_errors++;
        free(newarray);
        return DW_DLV_ERROR;
    }
    for ( u = 0; u < curcount; ++u) {
        newarray[u] = glflags.gf_global_debuglink_paths[u];
    }
    newarray[curcount] = pstr;
    free(glflags.gf_global_debuglink_paths);
    glflags.gf_global_debuglink_paths = newarray;
    glflags.gf_global_debuglink_count = newcount;
    return DW_DLV_OK;
}

/*  Option --add-debuglink-path=<text> */
void arg_add_debuglink_path(void)
{
    if (!dwoptarg || !dwoptarg[0]) {
        arg_usage_error = TRUE;
        return;
    }
    if (strncmp(dwoptarg,"add-debuglink-path=",21) == 0) {
        dwoptarg = &dwoptarg[21];
        if (strlen(dwoptarg)) {
            int res = 0;

            res = insert_debuglink_path(dwoptarg);
            if (res == DW_DLV_OK) {
                return;
            }
        }
    }
    arg_debuglink_path_invalid();
}

/*  Option '-S any=' */
void arg_search_any(void)
{
    const char *tempstr = 0;
    const char *ctx = arg_option > OPT_BEGIN ?
        "--search-any=" : "-S any=";

    /* -S any=<text> */
    if (!dwoptarg || !dwoptarg[0]) {
        arg_usage_error = TRUE;
        return;
    }
    glflags.gf_search_is_on = TRUE;
    glflags.search_any_text = makename(dwoptarg);
    tempstr = remove_quotes_pair(glflags.search_any_text);
    if (!tempstr){
        printf("ERROR regcomp: unable to compile "
            " search expression %s, out of memory\n",
            glflags.search_any_text);
        glflags.gf_count_major_errors++;
        return;
    }
    glflags.search_any_text = do_uri_translation(tempstr,ctx);
    if (strlen(glflags.search_any_text) <= 0) {
        arg_search_invalid();
    }
}

/*  Option '-Sv any=' */
void arg_search_any_count(void)
{
    arg_search_count();
    arg_search_any();
}

/*  Option '-S match=' */
void arg_search_match(void)
{
    const char *tempstr = 0;
    const char *ctx = arg_option > OPT_BEGIN ?
        "--search-match=" : "-S match=";

    if (!dwoptarg || !dwoptarg[0]) {
        arg_usage_error = TRUE;
        return;
    }
    /* -S match=<text> */
    glflags.gf_search_is_on = TRUE;
    glflags.search_match_text = makename(dwoptarg);
    tempstr = remove_quotes_pair(glflags.search_match_text);
    if (!tempstr){
        printf("ERROR: regcomp unable to compile "
            " search expression match=%s, out of memory\n",
            glflags.search_match_text);
        glflags.gf_count_major_errors++;
        return;
    }
    glflags.search_match_text = do_uri_translation(tempstr,ctx);
    if (strlen(glflags.search_match_text) <= 0) {
        arg_search_invalid();
    }
}

/*  Option '-Sv match=' */
void arg_search_match_count(void)
{
    arg_search_count();
    arg_search_match();
}

/*  Option '-S regex=' */
void arg_search_regex(void)
{
    const char *tempstr = 0;
    const char *ctx = arg_option > OPT_BEGIN ?
        "--search-regex=" : "-S regex=";
    int res = 0;

    if (!dwoptarg || !dwoptarg[0]) {
        arg_usage_error = TRUE;
        return;
    }
    /* The compiled regex occupies a static area */
    /* -S regex=<regular expression> */
    glflags.gf_search_is_on = TRUE;
    glflags.search_regex_text = makename(dwoptarg);
    tempstr = remove_quotes_pair(glflags.search_regex_text);
    if (!tempstr){
        printf("regcomp: unable to compile "
            " search regular expression %s, out of memory\n",
            glflags.search_regex_text);
        glflags.gf_count_major_errors++;
        return;
    }
    glflags.search_regex_text = do_uri_translation(tempstr,ctx);
    if (glflags.search_regex_text &&
        strlen(glflags.search_regex_text) > 0) {
        res = dd_re_comp(glflags.search_regex_text);

        if (res != DW_DLV_OK) {
            printf("ERROR: regcomp: unable to compile "
                " search regular expression %s "
                "so regex= ignored.\n",
                glflags.search_regex_text);
            glflags.search_regex_text = 0;
            glflags.gf_search_is_on = FALSE;
            glflags.gf_count_major_errors++;
        }
    } else {
        arg_search_invalid();
    }
}

/*  Option '-Sv regex=' */
void arg_search_regex_count(void)
{
    arg_search_count();
    arg_search_regex();
}

/*  Option '-Sv' */
void arg_search_count(void)
{
    glflags.gf_search_print_results = TRUE;
}

/*  Option '-t' */
void arg_t_multiple_selection(void)
{
    if (!dwoptarg || !dwoptarg[0]) {
        arg_usage_error = TRUE;
        return;
    }
    switch (dwoptarg[0]) {
    case 'a': arg_print_static();      break;
    case 'f': arg_print_static_func(); break;
    case 'v': arg_print_static_var();  break;
    default: arg_usage_error = TRUE;   break;
    }
}

/*  Option '-ta' */
void arg_print_static(void)
{
    /* all */
    glflags.gf_static_func_flag =  TRUE;
    glflags.gf_static_var_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-tf' */
void arg_print_static_func(void)
{
    /* .debug_static_func */
    glflags.gf_static_func_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-tv' */
void arg_print_static_var(void)
{
    /* .debug_static_var */
    glflags.gf_static_var_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-u' */
void arg_format_file(void)
{
    const char *ctx = arg_option > OPT_BEGIN ?
        "--format-file=" : "-u<cu name>";
    const char *tstr = 0;

    if (!dwoptarg || !dwoptarg[0]) {
        arg_usage_error = TRUE;
        return;
    }
    /* compile unit */
    glflags.gf_cu_name_flag = TRUE;
    tstr = do_uri_translation(dwoptarg,ctx);
    if (!tstr) {
    }
    esb_append(glflags.cu_name,tstr);
}

/*  Option '-U' */
void arg_format_suppress_uri(void)
{
    glflags.gf_uri_options_translation = FALSE;
}

/*  Option '-v' --verbose */
void arg_verbose(void)
{
    glflags.verbose++;
}
/*  Option '--show-dwarfdump-conf' */
void arg_show_dwarfdump_conf(void)
{
    glflags.gf_show_dwarfdump_conf++;
}
/*  Option '--show-args'
    which causes dwarfdump to print the current
    version of the program, date, and time of the run,
    and to show the command line arguments. */
void arg_show_args(void)
{
    glflags.gf_show_args_flag = TRUE;
    print_version_details(glflags.program_fullname);
}

/*  Option '-V' */
void arg_version(void)
{
    /* Display dwarfdump compilation date and time */
    print_version_details(glflags.program_fullname);
    arg_show_args();
    makename_destructor();
    global_destructors();
    exit(OKAY);
}

/*  Option '-w' */
void arg_print_weaknames(void)
{
    /* .debug_weaknames */
    glflags.gf_weakname_flag = TRUE;
    suppress_check_dwarf();
}
/* Option --print-section-allocations */
void arg_print_section_allocations(void)
{
    glflags. gf_print_section_allocations = TRUE;
}

/*  Option '-W[...]' */
void arg_W_multiple_selection(void)
{
    if (dwoptarg) {
        if (!dwoptarg[0]) {
            arg_usage_error = TRUE;
            return;
        }
        switch (dwoptarg[0]) {
        case 'c': arg_search_print_children(); break;
        case 'p': arg_search_print_parent();   break;
        default: arg_usage_error = TRUE;       break;
        }
    } else {
        arg_search_print_tree();
    }
}

/*  Option '-W' */
void arg_search_print_tree(void)
{
    /* Search results in wide format */
    glflags.gf_search_wide_format = TRUE;

    /* -W : Display parent and children tree */
    glflags.gf_display_children_tree = TRUE;
    glflags.gf_display_parent_tree = TRUE;
}

/*  Option '-Wc' */
void arg_search_print_children(void)
{
    /* -Wc : Display children tree */
    arg_search_print_tree();
    glflags.gf_display_children_tree = TRUE;
    glflags.gf_display_parent_tree = FALSE;
}

/*  Option '-Wp' */
void arg_search_print_parent(void)
{
    /* -Wp : Display parent tree */
    arg_search_print_tree();
    glflags.gf_display_children_tree = FALSE;
    glflags.gf_display_parent_tree = TRUE;
}

/*  Option '-x[...]' */
void arg_x_multiple_selection(void)
{
    if (!dwoptarg || !dwoptarg[0]) {
        arg_usage_error = TRUE;
        return;
    }
    if (strncmp(dwoptarg,"name=",5) == 0) {
        dwoptarg = &dwoptarg[5];
        arg_file_name();
    } else if (strncmp(dwoptarg,"abi=",4) == 0) {
        dwoptarg = &dwoptarg[4];
        arg_file_abi();
    } else if (strncmp(dwoptarg,"groupnumber=",12) == 0) {
        dwoptarg = &dwoptarg[12];
        arg_format_groupnumber();
    } else if (strncmp(dwoptarg,"universalnumber=",16) == 0) {
        dwoptarg = &dwoptarg[12];
        arg_format_universalnumber();
    } else if (strncmp(dwoptarg,"tied=",5) == 0) {
        dwoptarg = &dwoptarg[5];
        arg_file_tied();
    } else if (strncmp(dwoptarg,"line5=",6) == 0) {
        dwoptarg = &dwoptarg[6];
        arg_file_line5();
    } else if (strcmp(dwoptarg,"nosanitizestrings") == 0) {
        arg_format_suppress_sanitize();
    } else if (strcmp(dwoptarg,"noprintsectiongroups") == 0) {
        arg_format_suppress_group();
    } else {
        arg_x_invalid();
    }
}

/*  Option '-x abi=' */
static void
arg_file_abi(void)
{
    const char *ctx = arg_option > OPT_BEGIN ?
        "--file-abi=" : "-x abi=";
    const char *abi = 0;

    if (!dwoptarg || !dwoptarg[0]) {
        printf("ERROR *abi= does not allow an empty text\n");
        glflags.gf_count_major_errors++;
        arg_usage_error = TRUE;
        return;
    }
    /*  -x abi=<abi> meaning select abi from dwarfdump.conf
        file. Must always select abi to use dwarfdump.conf */
    abi = do_uri_translation(dwoptarg,ctx);
    if (strlen(abi) > 0) {
        config_file_abi = abi;
    } else {
        arg_x_invalid();
    }
}

/*  Option '-x universalnumber='
    greater than zero only applies to Mach-O universal
    object files*/
static void
arg_format_universalnumber(void)
{
    /*  By default prints the lowest universal number in the object.
        Default is  -x universalnumber=0 */

    long int gnum = 0;
    int res = 0;

    if (!dwoptarg || !dwoptarg[0]) {
        printf("ERROR *groupnumber= does not allow an empty text\n");
        glflags.gf_count_major_errors++;
        arg_usage_error = TRUE;
        return;
    }
    res = get_number_value(dwoptarg,&gnum);
    if (res == DW_DLV_OK && gnum >= 0) {
        glflags.gf_universalnumber = gnum;
    } else {
        arg_x_invalid();
    }
}

/*  Option '-x groupnumber=' */
static void
arg_format_groupnumber(void)
{
    /*  By default prints the lowest groupnumber in the object.
        Default is  -x groupnumber=0
        For group 1 (standard base dwarfdata)
            -x groupnumber=1
        For group 1 (DWARF5 .dwo sections and dwp data)
            -x groupnumber=2 */
    long int gnum = 0;
    int res = 0;

    if (!dwoptarg || !dwoptarg[0]) {
        printf("ERROR *groupnumber= does not allow an empty text\n");
        glflags.gf_count_major_errors++;
        arg_usage_error = TRUE;
        return;
    }
    res = get_number_value(dwoptarg,&gnum);
    if (res == DW_DLV_OK) {
        glflags.group_number = gnum;
    } else {
        arg_x_invalid();
    }
}

/*  Option '-x line5=' --file-line5=<v> */
static void
arg_file_line5(void)
{
    if (!dwoptarg || !dwoptarg[0]) {
        printf("ERROR *line5= does not allow an empty text\n");
        glflags.gf_count_major_errors++;
        arg_usage_error = TRUE;
        return;
    }
    if (!strcmp(dwoptarg,"std")) {
        glflags.gf_line_flag_selection = singledw5;
    } else if (!strcmp(dwoptarg,"s2l")) {
        glflags.gf_line_flag_selection= s2l;
    } else if (!strcmp(dwoptarg,"orig")) {
        /* drop orig */
        glflags.gf_line_flag_selection= singledw5;
    } else if (!strcmp(dwoptarg,"orig2l")) {
        /* drop orig2l */
        glflags.gf_line_flag_selection= s2l;
    } else {
        arg_x_invalid();
    }
}

/*  Option '-x name=' */
static void arg_file_name(void)
{
    const char *ctx = arg_option > OPT_BEGIN ?
        "--file-name=" : "-x name=";
    const char *path = 0;

    if (!dwoptarg || !dwoptarg[0]) {
        printf("ERROR *name= does not allow an empty text\n");
        glflags.gf_count_major_errors++;
        arg_usage_error = TRUE;
        return;
    }
    /*  -x name=<path> meaning name dwarfdump.conf file. */
    path = do_uri_translation(dwoptarg,ctx);
    if (strlen(path) > 0) {
        esb_empty_string(glflags.config_file_path);
        esb_append(glflags.config_file_path,path);
    } else {
        arg_x_invalid();
    }
}

/*  Option '-x noprintsectiongroups' */
static void arg_format_suppress_group(void)
{
    glflags.gf_section_groups_flag = FALSE;
}

/*  Option '-x nosanitizestrings' '--format-suppress-sanitize' */
static void arg_format_suppress_sanitize(void)
{
    glflags.gf_no_sanitize_strings = TRUE;
}

/*  Option '--no-dup-attr-check'
    Results in telling libdwarf not to check for
    duplicate attributes in abbreviations.*/
static void arg_no_dup_attr_check(void)
{
    glflags.gf_no_check_duplicated_attributes = TRUE;
}

/*  Option '-x tied=' */
static void arg_file_tied(void)
{
    const char *ctx = arg_option > OPT_BEGIN ?
        "--file-tied=" : "-x tied=";
    const char *tiedpath = 0;

    if (!dwoptarg || !dwoptarg[0]) {
        printf("ERROR *tied= does not allow an empty text\n");
        glflags.gf_count_major_errors++;
        arg_usage_error = TRUE;
        return;
    }
    tiedpath = do_uri_translation(dwoptarg,ctx);
    if (strlen(tiedpath) > 0) {
        esb_empty_string(glflags.config_file_tiedpath);
        esb_append(glflags.config_file_tiedpath,tiedpath);
    } else {
        arg_x_invalid();
    }
}

/*  Option '--file-use-no-libelf' */
static void arg_file_use_no_libelf(void)
{
    glflags.gf_file_use_no_libelf = TRUE;
}

/* -y */
static void arg_print_types(void)
{
    /* .debug_pubtypes */
    /* Also for SGI-only, and obsolete, .debug_typenames */
    suppress_check_dwarf();
    glflags.gf_pubtypes_flag = TRUE;
}

/*  Option not supported */
static void arg_not_supported(void)
{
    printf("ERROR Option -%c is no longer supported:ignored\n",
        arg_option);
    glflags.gf_count_major_errors++;
}

/* Error message for --add-debuglink-path=path  */
static void arg_debuglink_path_invalid(void)
{
    printf("--add-debuglink-path=<text>\n");
    /*  Add quotes around string so any invisible chars
        kind of show up */
    if (!dwoptarg || !dwoptarg[0]) {
        printf("is allowed, not an empty <text>\n");
    } else {
        printf("is allowed, not  \"%s\"\n",dwoptarg);
    }
    glflags.gf_count_major_errors++;
    arg_usage_error = TRUE;

}
/*  Error message for invalid '-S' option. */
static void arg_search_invalid(void)
{
    printf("-S any=<text> or -S match=<text> or"
        " -S regex=<text>\n");
    if (!dwoptarg || !dwoptarg[0]) {
        printf("is allowed, not an empty <text>\n");
    } else {
        printf("is allowed, not -S %s\n",dwoptarg);
    }
    arg_usage_error = TRUE;
    glflags.gf_count_major_errors++;
}

/*  Error message for invalid '-x' option. */
static void arg_x_invalid(void)
{
    printf("-x name=<path-to-conf> \n");
    printf(" and  \n");
    printf("-x abi=<abi-in-conf> \n");
    printf(" and  \n");
    printf("-x tied=<tied-file-path> \n");
    printf(" and  \n");
    printf("-x line5={std,s2l,orig,orig2l} \n");
    printf(" and  \n");
    printf("-x nosanitizestrings \n");
    if (!dwoptarg || !dwoptarg[0]) {
        printf("is allowed, not an empty text after the =\n");
    } else {
        printf("are legal, not -x %s\n", dwoptarg);
    }
    arg_usage_error = TRUE;
    glflags.gf_count_major_errors++;
}
/*  --allocate-via-mmap */
static void arg_allocate_via_mmap(void)
{
    glflags.gf_allocation_via_mmap = TRUE;
    dwarf_set_load_preference(Dwarf_Alloc_Mmap);
}

/*  Process the command line arguments and set the
    appropriate options. All
    the options are within the global flags structure. */
static void
set_command_options(int argc, char *argv[])
{
    int longindex = 0;

    /* j unused */
    while ((arg_option = dwgetopt_long(argc, argv,
        "#:abc::CdDeE::fFgGhH:iIk:l::mMnNo::O:pPqQrRsS:t:"
        "u:UvVwW::x:yz",
        longopts,&longindex)) != EOF) {

        switch (arg_option) {
        case '#': arg_trace();                   break;
        case 'a': arg_print_all();               break;
        case 'b': arg_print_abbrev();            break;
        case 'c': arg_c_multiple_selection();    break;
        case 'C': arg_format_extensions();       break;
        case 'd': arg_format_dense();            break;
        case 'D': arg_format_suppress_offsets(); break;
        case 'e': arg_format_ellipsis();         break;
        case 'f': arg_print_debug_frame();       break;
        case 'F': arg_print_gnu_frame();         break;
        case 'g': arg_format_loc();              break;
        case 'G': arg_format_global_offsets();   break;
        case 'h': arg_h_multiple_selection();    break;
        case 'H': arg_format_limit();            break;
        case 'i': arg_print_info();              break;
        case 'I': arg_print_fission();           break;
        case 'k': arg_k_multiple_selection();    break;
        case 'l': arg_l_multiple_selection();    break;
        case 'm': arg_print_macinfo();           break;
        case 'M': arg_format_attr_name();        break;
        case 'n': arg_format_suppress_lookup();  break;
        case 'N': arg_print_ranges();            break;
        case 'O': arg_O_multiple_selection();    break;
        case 'p': arg_print_pubnames();          break;
        case 'P': arg_print_producers();         break;
        case 'q': arg_format_suppress_uri_msg(); break;
        case 'Q': arg_format_suppress_data();    break;
        case 'r': arg_print_aranges();           break;
        case 'R': arg_format_registers();        break;
        case 's': arg_print_strings();           break;
        case 'S': arg_S_multiple_selection();    break;
        case 't': arg_t_multiple_selection();    break;
        case 'u': arg_format_file();             break;
        case 'U': arg_format_suppress_uri();     break;
        case 'v': arg_verbose();                 break;
        case 'V': arg_version();                 break;
        case 'w': arg_print_weaknames();         break;
        case 'W': arg_W_multiple_selection();    break;
        case 'x': arg_x_multiple_selection();    break;
        case 'y': arg_print_types();             break;
        case 'z': arg_not_supported();           break;

        /* Check DWARF Integrity. */
        case OPT_CHECK_ABBREV:         arg_check_abbrev();    break;
        case OPT_CHECK_ALL:            arg_check_all();       break;
        case OPT_CHECK_ARANGES:        arg_check_aranges();   break;
        case OPT_CHECK_ATTR_DUP:       arg_check_attr_dup();  break;
        case OPT_CHECK_ATTR_ENCODINGS: arg_check_attr_encodings();
            break;
        case OPT_CHECK_ATTR_NAMES:     arg_check_attr_names();break;
        case OPT_CHECK_CONSTANTS:      arg_check_constants(); break;
        case OPT_CHECK_FILES_LINES:    arg_check_files_lines();
            break;
        case OPT_CHECK_FORWARD_REFS:   arg_check_forward_refs();
            break;
        case OPT_CHECK_FRAME_BASIC:    arg_check_frame_basic();
            break;
        case OPT_CHECK_FRAME_EXTENDED: arg_check_frame_extended();
            break;
        case OPT_CHECK_FRAME_INFO:     arg_check_frame_info();break;
        case OPT_CHECK_GAPS:           arg_check_gaps();     break;
        case OPT_CHECK_LOC:            arg_check_loc();      break;
        case OPT_CHECK_MACROS:         arg_check_macros();   break;
        case OPT_CHECK_PUBNAMES:       arg_check_pubnames(); break;
        case OPT_CHECK_RANGES:         arg_check_ranges();   break;
        case OPT_CHECK_SELF_REFS:      arg_check_self_refs();break;
        case OPT_CHECK_SHOW:           arg_check_show();     break;
        case OPT_CHECK_SILENT:         arg_check_silent();   break;
        case OPT_CHECK_SUMMARY:        arg_check_summary();  break;
        case OPT_CHECK_TAG_ATTR:       arg_check_tag_attr(); break;
        case OPT_CHECK_TAG_TAG:        arg_check_tag_tag();  break;
        case OPT_CHECK_TYPE:           arg_check_type();     break;
        case OPT_CHECK_UNIQUE:         arg_check_unique();   break;
        case OPT_CHECK_USAGE:          arg_check_usage();break;
        case OPT_CHECK_USAGE_EXTENDED: arg_check_usage_extended();
            break;
        case OPT_CHECK_FUNCTIONS:      arg_check_functions(); break;

        /* File Specifications. */
        case OPT_FILE_ABI:    arg_file_abi();    break;
        case OPT_FILE_LINE5:  arg_file_line5();  break;
        case OPT_FILE_NAME:   arg_file_name();   break;
        case OPT_FILE_OUTPUT: arg_file_output(); break;
        case OPT_FILE_TIED:   arg_file_tied();   break;
        case OPT_FILE_USE_NO_LIBELF: arg_file_use_no_libelf(); break;

        /* Print Output Qualifiers. */
        case OPT_FORMAT_ATTR_NAME:
            arg_format_attr_name();        break;
        case OPT_FORMAT_DENSE:
            arg_format_dense();            break;
        case OPT_FORMAT_ELLIPSIS:
            arg_format_ellipsis();         break;
        case OPT_FORMAT_EXPR_OPS_JOINED:
            arg_format_expr_ops_joined(); break;
        case OPT_FORMAT_EXTENSIONS:
            arg_format_extensions();       break;
        case OPT_FORMAT_GLOBAL_OFFSETS:
            arg_format_global_offsets();   break;
        case OPT_FORMAT_LOC:
            arg_format_loc();              break;
        case OPT_FORMAT_REGISTERS:
            arg_format_registers();        break;
        case OPT_FORMAT_SUPPRESS_DATA:
            arg_format_suppress_data();    break;
        case OPT_FORMAT_SUPPRESS_GROUP:
            arg_format_suppress_group();   break;
        case OPT_FORMAT_SUPPRESS_OFFSETS:
            arg_format_suppress_offsets(); break;
        case OPT_FORMAT_SUPPRESS_LOOKUP:
            arg_format_suppress_lookup();  break;
        case OPT_FORMAT_SUPPRESS_SANITIZE:
            arg_format_suppress_sanitize();break;
        case OPT_FORMAT_SUPPRESS_URI:
            arg_format_suppress_uri();     break;
        case OPT_FORMAT_SUPPRESS_URI_MSG:
            arg_format_suppress_uri_msg(); break;
        case OPT_FORMAT_SUPPRESS_UTF8:
            arg_format_suppress_utf8(); break;

        /* Print Output Limiters. */
        case OPT_FORMAT_FILE:         arg_format_file();       break;
        case OPT_FORMAT_GCC:          arg_format_gcc();        break;
        case OPT_FORMAT_GROUP_NUMBER: arg_format_groupnumber(); break;
        case OPT_FORMAT_UNIVERSALNUMBER: arg_format_universalnumber();
            break;
        case OPT_FORMAT_LIMIT:        arg_format_limit();      break;
        case OPT_FORMAT_PRODUCER:     arg_format_producer();   break;
        case OPT_FORMAT_SNC:          arg_format_snc();        break;

        /* Print Debug Sections. */
        case OPT_PRINT_ABBREV:      arg_print_abbrev();      break;
        case OPT_PRINT_ALL:         arg_print_all();         break;
        case OPT_PRINT_ALL_SRCFILES: arg_print_all_srcfiles(); break;
        case OPT_PRINT_ARANGES:     arg_print_aranges();     break;
        case OPT_PRINT_DEBUG_NAMES: arg_print_debug_names(); break;
        case OPT_PRINT_DEBUG_ADDR:  arg_print_debug_addr();  break;
        case OPT_PRINT_GNU_DEBUGLINK: arg_print_gnu_debuglink();
            break;
        case OPT_PRINT_DEBUG_GNU:   arg_print_debug_gnu(); break;
        case OPT_PRINT_DEBUG_SUP:   arg_print_debug_sup(); break;
        case OPT_PRINT_EH_FRAME:    arg_print_gnu_frame();   break;
        case OPT_PRINT_FISSION:     arg_print_fission();     break;
        case OPT_PRINT_FRAME:       arg_print_debug_frame(); break;
        case OPT_PRINT_INFO:        arg_print_info();        break;
        case OPT_PRINT_LANGUAGE_VERSION_TABLE:
            arg_print_language_version_table();       break;
        case OPT_PRINT_LINES:       arg_print_lines();       break;
        case OPT_PRINT_LINES_SHORT: arg_print_lines_short(); break;
        case OPT_PRINT_LOC:         arg_print_loc();         break;
        case OPT_PRINT_MACINFO:     arg_print_macinfo();     break;
        case OPT_PRINT_MACHINE_ARCH: arg_print_machine_arch(); break;
        case OPT_PRINT_PRODUCERS:   arg_print_producers();   break;
        case OPT_PRINT_PUBNAMES:    arg_print_pubnames();    break;
        case OPT_PRINT_RANGES:      arg_print_ranges();      break;
        case OPT_PRINT_RAW_LOCLISTS:arg_print_raw_loclists();break;
        case OPT_PRINT_RAW_RNGLISTS:arg_print_raw_rnglists();break;
        case OPT_PRINT_STATIC:      arg_print_static();      break;
        case OPT_PRINT_STATIC_FUNC: arg_print_static_func(); break;
        case OPT_PRINT_STATIC_VAR:  arg_print_static_var();  break;
        case OPT_PRINT_STRINGS:     arg_print_strings();     break;
        case OPT_PRINT_STR_OFFSETS: arg_print_str_offsets(); break;
        case OPT_PRINT_TYPE:        arg_print_types();       break;
        case OPT_PRINT_WEAKNAME:    arg_print_weaknames();   break;
        case OPT_PRINT_ALLOCATIONS: arg_print_section_allocations();
            break;

        /* debuglink attributes */
        case OPT_NO_FOLLOW_DEBUGLINK: arg_no_follow_debuglink();break;
        case OPT_ADD_DEBUGLINK_PATH: arg_add_debuglink_path();  break;
        case OPT_SUPPRESS_DEBUGLINK_CRC:
            arg_suppress_debuglink_crc(); break;
        case OPT_NO_FOLLOW_DSYM: arg_no_follow_dsym();break;

        /* Search text in attributes. */
        case OPT_SEARCH_ANY:            arg_search_any();
            break;
        case OPT_SEARCH_ANY_COUNT:      arg_search_any_count();
            break;
        case OPT_SEARCH_MATCH:          arg_search_match();
            break;
        case OPT_SEARCH_MATCH_COUNT:    arg_search_match_count();
            break;
        case OPT_SEARCH_PRINT_CHILDREN: arg_search_print_children();
            break;
        case OPT_SEARCH_PRINT_PARENT:   arg_search_print_parent();
            break;
        case OPT_SEARCH_PRINT_TREE:     arg_search_print_tree();break;
        case OPT_SEARCH_REGEX:          arg_search_regex();break;
        case OPT_SEARCH_REGEX_COUNT:    arg_search_regex_count();
            break;

        /* Help & Version. */
        case OPT_HELP:          arg_help();          break;
        case OPT_VERBOSE:       arg_verbose();       break;
        case OPT_VERBOSE_MORE:  arg_verbose();       break;
        case OPT_VERSION:       arg_version();       break;
        case OPT_SHOW_DWARFDUMP_CONF:
            arg_show_dwarfdump_conf();break;
        case OPT_SHOW_ARGS:
            arg_show_args();break;
        /* Trace. */
        case OPT_TRACE: arg_trace(); break;
        case OPT_NO_DUP_ATTR_CHECK:
            /*  Suppress checking for duplicate attributes
                in abbreviations entries,
                It is rare compilers emit duplicates, but
                if one did, use this option to let dwarfdump show
                things as they are rather than generating an error.*/
            arg_no_dup_attr_check(); break;

        case OPT_SUPPRESS_HARMLESS: arg_suppress_harmless();break;
        case OPT_ALLOC_TREE_OFF:
            /*  Suppress nearly all libdwarf de_alloc_tree
                record keeping. */
            dwarf_set_de_alloc_flag(FALSE);
            break;
        case OPT_ALLOCATE_VIA_MMAP:
            arg_allocate_via_mmap();
            break;

        default: arg_usage_error = TRUE; break;
        }
    }
}

/*  This is a hack allowing us to pretend that
    dwarfdump  --suppress-de-alloc-tree foo.o
    has no arguments.  Because the args here really
    are special for use by dwarfdump developers  or
    special situations and
    even with these special args we want do_all() to be
    called by process_args() below if there are no
    'normal' - or -- args.
    And of course -v --verbose are special too.
    So regression testing can behave identically
    with or without the specials.
    Function new March 6, 2020  */
static const char *simplestdargs[] ={
"-v",
"-vv",
"-vvv",
"-vvvv",
"-vvvvv",
"-vvvvvv",
"--verbose",
"--show-dwarfdump-conf",
"--show-args",
"--no-dup-form-check",
"--verbose-more",
"--suppress-de-alloc-tree",
"--suppress-debuglink-crc",
"--no-follow-debuglink",
"--no-dup-attr-check",
"--allocate-via-mmap",
0
};

static int
lacking_normal_args (int argct,char **args)
{
    char * curarg = 0;
    int i = 0;

    for ( i = 0; i < argct ; ++i) {
        int k = 0;
        int simple = FALSE;
        curarg = args[i];
        if (curarg[0] != '-') {
            /*  Standard case. */
            return TRUE;
        }
        for (k = 0; simplestdargs[k]; ++k) {
            if (!strcmp(curarg,simplestdargs[k])) {
                simple = TRUE;
                break;
            }
        }
        if (simple) {
            continue;
        }
        /*  Not one of the specials, a normal argument,
            so we have some 'real' args. */
        return FALSE;
    }
    /*  Never found any non-simple argument, let regular
        arg processing deal with it. */
    return TRUE;
}

/* process arguments and return object filename */
const char *
process_args(int argc, char *argv[])
{
    /*  If building for a regression test run
        on msys2 (and everywhere) , use fixed
        name, fullname instead of argv[0], so tests pass
        identically in all supported environments */
#ifdef DWREGRESSIONTEMP
        /* for the benefit of testing on msys2 so names
            match. We do it for all platforms for
            full consistency. */
    glflags.program_name     = "./dwarfdump";
    glflags.program_fullname = "./dwarfdump";
#else /* ! DWREGRESSIONTEMP */
    glflags.program_name     = argv[0];
    glflags.program_fullname = argv[0];
#endif /* DWREGRESSIONTEMP */

    suppress_check_dwarf();
    if (argv[1] && lacking_normal_args(argc-1,argv+1)) {
        /*  The default setting of what to print or do */
        do_all();
    }
    glflags.gf_section_groups_flag = TRUE;

    /*  Process the arguments and set the appropriate options. */
    set_command_options(argc, argv);
    if (config_file_abi && glflags.gf_generic_1200_regs) {
        printf("Specifying both -R and -x abi= is not "
            "allowed. Use one "
            "or the other.  -x abi= ignored.\n");
        config_file_abi = 0;
    }
    {
        /*  even with no abi we look as there may be
            a dwarfdump option there we know about. */
        int res = 0;
        res = find_conf_file_and_read_config(
            esb_get_string(glflags.config_file_path),
            config_file_abi,
            config_file_defaults,
            glflags.config_file_data);
        if (res == FOUND_ERROR) {
            if (!glflags.gf_do_print_dwarf &&
                !glflags.gf_do_check_dwarf) {
                printf("Frame not configured due to "
                    "configure error(s).\n");
                printf("(Since no print or check options provided "
                    "dwarfdump may now silently exit)\n");
            } else {
                printf("Frame not configured due to "
                    "configure error(s), "
                    "using generic 100 registers.\n"
                    "Frame section access suppressed.\n");
                glflags.gf_eh_frame_flag = FALSE;
                glflags.gf_frame_flag = FALSE;
            }
        } else if (res == FOUND_DONE || res == FOUND_OPTION) {
            if (glflags.gf_generic_1200_regs) {
                init_generic_config_1200_regs(
                    glflags.config_file_data);
            }
        } else {
            /* FOUND_ABI_START nothing to do. */
        }
    }
    if (arg_usage_error ) {
        printf("%s option error.\n",glflags.program_name);
        printf("To see the options list: %s -h\n",
            glflags.program_name);
        makename_destructor();
        global_destructors();
        exit(EXIT_FAILURE);
    }
    if (dwoptind < (argc - 1)) {
        printf("Multiple apparent object file names "
            "provided to %s\n",glflags.program_name);
        printf("Only a single object name is allowed\n");
        printf("To see the options list: %s -h\n",
            glflags.program_name);
        makename_destructor();
        global_destructors();
        exit(EXIT_FAILURE);
    }
    if (dwoptind > (argc - 1)) {
        printf("No object file name provided to %s\n",
            glflags.program_name);
        printf("To see the options list: %s -h\n",
            glflags.program_name);
        makename_destructor();
        global_destructors();
        exit(EXIT_FAILURE);
    }
    /*  FIXME: it seems silly to be printing section names
        where the section does not exist in the object file.
        However we continue the long-standard practice
        of printing such by default in most cases. For now. */
    if (glflags.group_number == DW_GROUPNUMBER_DWO) {
        /*  For split-dwarf/DWO some sections make no sense.
            This prevents printing of meaningless headers where no
            data can exist. */
        glflags.gf_pubnames_flag = FALSE;
        glflags.gf_eh_frame_flag = FALSE;
        glflags.gf_frame_flag    = FALSE;
        glflags.gf_macinfo_flag  = FALSE;
        glflags.gf_aranges_flag  = FALSE;
        glflags.gf_ranges_flag   = FALSE;
        glflags.gf_static_func_flag = FALSE;
        glflags.gf_static_var_flag = FALSE;
        glflags.gf_weakname_flag = FALSE;
    }
    if (glflags.group_number > DW_GROUPNUMBER_BASE) {
        /* These no longer apply, no one uses. */
        glflags.gf_static_func_flag = FALSE;
        glflags.gf_static_var_flag = FALSE;
        glflags.gf_weakname_flag = FALSE;
        glflags.gf_pubnames_flag = FALSE;
    }

    if (glflags.gf_do_check_dwarf) {
        if (glflags.verbose) {
            /*  Specifically now set > 1 for
                dd_check_attr_encoding.c
                to know extra detail is wanted there.
                Somewhat messy conflation of flags. */
            glflags.gf_check_verbose_mode++;
        }
        /*  Set specific verbosity when checking
            (checking means checking-only).
            And ensure non-zero for error reporting CU/DIE
            details during checking.
            This affects how dd_check_attr_encoding.c
            works. */
        glflags.verbose = 1;
    }
    return do_uri_translation(argv[dwoptind],"file-to-process");
}
