/*
Copyright (C) 2000,2002,2004,2005 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright (C) 2007-2021 David Anderson. All Rights Reserved.
Portions Copyright 2007-2010 Sun Microsystems, Inc. All rights reserved.
Portions Copyright 2012 SN Systems Ltd. All rights reserved.

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

/* The address of the Free Software Foundation is
   Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.
   SGI has moved from the Crittenden Lane address.
*/

#include <config.h>

#include <stddef.h> /* NULL size_t */
#include <stdio.h>  /* FILE stdout stderr fprintf() printf() */
#include <stdlib.h> /* exit() free() malloc() qsort() realloc()
    getenv() */
#include <string.h> /* memset() strcmp() stricmp()
    strlen() strrchr() strstr() */

/* Windows specific header files */
#ifdef _WIN32
#ifdef HAVE_STDAFX_H
#include "stdafx.h"
#endif /* HAVE_STDAFX_H */
#include <io.h> /* close() dup2() */
#elif defined HAVE_UNISTD_H
#include <unistd.h> /* close() dup2() */
#endif /* _WIN32 */

#ifdef HAVE_FCNTL_H
#include <fcntl.h> /* O_RDONLY open() */
#endif /* HAVE_FCNTL_H */
#ifdef HAVE_UTF8
/*  locale.h is guaranteed in C90 and later,
    but langinfo.h might not be present. */
#include "locale.h"
#include "langinfo.h"
#endif /* HAVE_UTF8 */
#include <string.h>

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_makename.h"
#include "dd_macrocheck.h"
#include "dd_dwconf.h"
#include "dd_dwconf_using_functions.h"
#include "dd_common.h"
#include "dd_helpertree.h"
#include "dd_esb.h"                /* For flexible string buffer. */
#include "dd_esb_using_functions.h"
#include "dd_sanitized.h"
#include "dd_tag_common.h"
#include "dd_addrmap.h"
#include "dd_attr_form.h"
#include "print_debug_gnu.h"
#include "dd_naming.h" /* for get_FORM_name() */
#include "dd_command_options.h"
#include "dd_compiler_info.h"
#include "dd_safe_strcpy.h"
#include "dd_minimal.h"
#include "dd_mac_cputype.h"
#include "dd_elf_cputype.h"
#include "dd_pe_cputype.h"
#include "dd_all_srcfiles.h"

#ifndef O_RDONLY
/*  This is for a Windows environment */
# define O_RDONLY _O_RDONLY
#endif

#ifdef _O_BINARY
/*  This is for a Windows environment */
#define O_BINARY _O_BINARY
#else
# ifndef O_BINARY
# define O_BINARY 0  /* So it does nothing in Linux/Unix */
# endif
#endif /* O_BINARY */
/*  Basic ELF flags */
#ifndef ET_DYN
#define ET_DYN 3
#endif
#ifndef ET_EXEC
#define ET_EXEC 2
#endif
#ifndef ET_REL
#define ET_REL 1
#endif
#ifndef SHF_ALLOC
#define SHF_ALLOC 2
#endif

#define BYTES_PER_INSTRUCTION 4

#define LOCAL_PTR_ARY_COUNT 50

/* Build section information */
void build_linkonce_info(Dwarf_Debug dbg);

struct glflags_s glflags;

/* Functions used to manage the unique errors table */
static void allocate_unique_errors_table(void);
static void release_unique_errors_table(void);
#ifdef TESTING
static void dump_unique_errors_table(void);
#endif
static Dwarf_Bool add_to_unique_errors_table(char * error_text);

static struct esb_s esb_short_cu_name;
static struct esb_s esb_long_cu_name;
static struct esb_s dwarf_error_line;
static int global_basefd = -1;
static int global_tiedfd = -1;
static struct esb_s global_file_name;
static struct esb_s global_tied_file_name;

static void print_machine_arch(Dwarf_Debug dbg);

static void homeify(char *s, struct esb_s* out);

static int process_one_file(
    const char * file_name,
    const char * tied_file_name,
    char *       tempbuf,
    size_t       tempbuflen,
    struct dwconf_s *conf);

static int print_gnu_debuglink(Dwarf_Debug dbg,Dwarf_Error *err);

static int
open_a_file(const char * name)
{
    /* Set to a file number that cannot be legal. */
    int fd = -1;
    fd = open(name, O_RDONLY | O_BINARY);
    return fd;
}

static void
close_a_file(int f)
{
    if (f != -1) {
        close(f);
    }
}

void
global_destructors(void)
{
    makename_destructor();
    uri_data_destructor();
    esb_destructor(&esb_long_cu_name);
    esb_destructor(&esb_short_cu_name);
    esb_destructor(&dwarf_error_line);
    /*  Global flags initialization and esb-buffers destruction. */
    reset_global_flags();
    esb_destructor(&global_file_name);
    esb_destructor(&global_tied_file_name);
    free_all_dwconf(glflags.config_file_data);
    sanitized_string_destructor();
    ranges_esb_string_destructor();
    close_a_file(global_basefd);
    close_a_file(global_tiedfd);
#ifdef _WIN32
    /* Close the null device used during formatting printing */
    esb_close_null_device();
#endif /* _WIN32 */
    if (glflags.gf_global_debuglink_paths) {
        unsigned int i = 0;

        for ( ; i < glflags.gf_global_debuglink_count ; ++i ) {
            free(glflags.gf_global_debuglink_paths[i]);
            glflags.gf_global_debuglink_paths[i] =  0;
        }
        free(glflags.gf_global_debuglink_paths);
        glflags.gf_global_debuglink_paths = 0;
    }
    glflags.gf_global_debuglink_count = 0;
}

static void
check_for_notes(void)
{
    long int ect = glflags.gf_count_macronotes;
    const char *w = "was";
    const char *e = "MACRONOTE";
    if (!ect) {
        return;
    }
    if (ect > 1) {
        w = "were";
        e = "MACRONOTEs";
    }
    printf("There %s %ld DWARF %s reported: "
        "see MACRONOTE above.\n",
        w, ect,e);
}
static void
check_for_major_errors(void)
{
    long int ect = glflags.gf_count_major_errors;
    const char *w = "was";
    const char *e = "error";
    if (!ect) {
        return;
    }
    if (ect > 1) {
        w = "were";
        e = "errors";
    }
    printf("There %s %ld DWARF %s reported: "
        "see ERROR above.\n",
        w, ect,e);
}

static void
flag_data_pre_allocation(void)
{
    memset(glflags.section_high_offsets_global,0,
        sizeof(*glflags.section_high_offsets_global));
    /*  If we are checking .debug_line, .debug_ranges, .debug_aranges,
        or .debug_loc build the tables containing
        the pairs LowPC and HighPC. It is safer  (and not
        expensive) to build all
        of these at once so mistakes in options do not lead
        to coredumps (like -ka -p did once). */
    if (glflags.gf_check_decl_file || glflags.gf_check_ranges ||
        glflags.gf_check_locations ||
        glflags.gf_do_check_dwarf ||
        glflags.gf_check_self_references) {
        glflags.pRangesInfo = AllocateBucketGroup(KIND_RANGES_INFO);
        glflags.pLinkonceInfo =
            AllocateBucketGroup(KIND_LINKONCE_INFO);
        glflags.pVisitedInfo = AllocateBucketGroup(KIND_VISITED_INFO);
    }
    /* Create the unique error table */
    if (glflags.gf_print_unique_errors) {
        allocate_unique_errors_table();
    }
    /* Allocate range array to be used by all CUs */
    if (glflags.gf_check_ranges) {
        allocate_range_array_info();
    }
}

static void
flag_data_post_cleanup(void)
{
    if (glflags.pRangesInfo) {
        ReleaseBucketGroup(glflags.pRangesInfo);
        glflags.pRangesInfo = 0;
    }
    if (glflags.pLinkonceInfo) {
        ReleaseBucketGroup(glflags.pLinkonceInfo);
        glflags.pLinkonceInfo = 0;
    }
    if (glflags.pVisitedInfo) {
        ReleaseBucketGroup(glflags.pVisitedInfo);
        glflags.pVisitedInfo = 0;
    }
    /* Release range array to be used by all CUs */
    if (glflags.gf_check_ranges) {
        release_range_array_info();
    }
    /* Delete the unique error set */
    if (glflags.gf_print_unique_errors) {
        release_unique_errors_table();
    }
    clean_up_compilers_detected();
    destruct_abbrev_array();
}

void _dwarf_alloc_tree_counts(Dwarf_Unsigned *allocount,
    Dwarf_Unsigned *allosum,
    Dwarf_Unsigned *treecount,
    Dwarf_Unsigned *treesum,
    Dwarf_Unsigned *earlydealloccount,
    Dwarf_Unsigned *earlydeallocsize,
    Dwarf_Unsigned *unused1,
    Dwarf_Unsigned *unused2,
    Dwarf_Unsigned *unused3);

/*
   Iterate through dwarf and print all info.
*/
int
main(int argc, char *argv[])
{
    const char     *file_name = 0;
    unsigned        ftype = 0;
    unsigned        endian = 0;
    unsigned        offsetsize = 0;
    Dwarf_Unsigned  filesize = 0;
    int             errcode = 0;
    char           *temp_path_buf = 0;
    size_t          temp_path_buf_len = 0;
    int             res = 0;
    /* path_source will be DW_PATHSOURCE_basic  */
    unsigned char   path_source = DW_PATHSOURCE_unspecified;

#ifdef _WIN32
    /*  Open the null device used during formatting printing */
    if (!esb_open_null_device()) {
        printf("ERROR dwarfdump: Unable to open null device.\n");
        exit(EXIT_FAILURE);
    }
#endif /* _WIN32 */

    /*  Global flags initialization and esb-buffers construction. */
    init_global_flags();

    set_checks_off();
    uri_data_constructor();
    esb_constructor(&esb_short_cu_name);
    esb_constructor(&esb_long_cu_name);
    esb_constructor(&dwarf_error_line);
#ifdef _WIN32
    /*  Often we redirect the output to a file, but we have found
        issues due to the buffering associated with stdout.
        Some issues were fixed just by the use of 'fflush',
        but the main issued remained.
        The stdout stream is buffered, so will only display
        what's in the buffer after it reaches a newline
        (or when it's told to).
        We have a few options to print immediately:
        - Print to stderr instead using fprintf.
        - Print to stdout and flush stdout whenever
            we need it to using fflush.
        - We can also disable buffering on stdout by using setbuf:
            setbuf(stdout,NULL);
            Make stdout unbuffered; this seems to work for all cases.
        The problem is no longer present. Now, for practical
        purposes, there is no stderr output, all is stdout.
        September 2018.  */

    /*  Calling setbuf() with NULL argument, it turns off
        all buffering for the specified stream.
        Then writing to and/or reading from the stream
        will be exactly as directed by the program.
        But if dwarfdump is used over a network drive,
        it shows a dramatic
        slowdown when sending the output to a file.
        An operation that takes
        couple of seconds, it was taking few hours. */
    /*  setbuf(stdout,NULL); */
    /*  Redirect stderr to stdout. */
    /*  No more redirect needed. We only use stdout */
#endif /* _WIN32 */

#ifdef HAVE_UTF8
    {
        char *langinf = 0;

        setlocale(LC_CTYPE, "");
        setlocale(LC_NUMERIC, "");
        langinf = nl_langinfo(CODESET);
        if (strcmp(langinf,"UTF-8") && strcmp(langinf,"UTF8")) {
            glflags.gf_print_utf8_flag = FALSE;
        }else {
            glflags.gf_print_utf8_flag = TRUE;
        }
    }
#endif /* HAVE_UTF8 */
    file_name = process_args(argc, argv);
    /*  print_version_details already done in
        dd_command_options.c */
    print_args(argc,argv);

    /*  Redirect stdout  to an specific file */
    if (glflags.output_file) {
        if (NULL == freopen(glflags.output_file,"w",stdout)) {
            printf("ERROR dwarfdump: Unable to redirect "
                "output to '%s'\n",
                glflags.output_file);
            global_destructors();
            exit(EXIT_FAILURE);
        }
        /* Record version and arguments in the output file */
        print_version_details(argv[0]);
        print_args(argc,argv);
    }

    /*  Allow the user to hide some warnings by using
        command line options */
    {
        Dwarf_Cmdline_Options wcmd;
        /*  The struct has just one field!.
            If glflags.gf_check_verbose_mode is non-zero
            this tells libdwarf to emit a detailed
            message (which flows to the caller via
            _dwarf_printf()) about the header problem.
            Defaults to zero for print options,
            Set to 1 for check options like -ka
            dwarfdump "-ks  --check-silent" sets it zero. */

        wcmd.check_verbose_mode = glflags.gf_check_verbose_mode;
        dwarf_record_cmdline_options(wcmd);
    }
    if (glflags.gf_check_functions) {
        static const Dwarf_Signed stab[] =
            {0,1,-1,100,-100,-10000000,10000000};
        int i = 0;
        int len = sizeof(stab)/sizeof(stab[0]);
        char vbuf[100];

        vbuf[0] = 0;
        DWARF_CHECK_COUNT(check_functions_result,1);
        for (i = 0; i < len; ++i) {
            Dwarf_Signed basevalue = 0;
            Dwarf_Signed decodedvalue = 0;
            Dwarf_Unsigned silen = 0;
            int leblen = 0;

            basevalue = stab[i];
            memset(vbuf,0,sizeof(vbuf));
            res = dwarf_encode_signed_leb128(basevalue,
                &leblen,
                vbuf,(int)sizeof(vbuf));
            if (res == DW_DLV_ERROR) {
                DWARF_CHECK_ERROR(check_functions_result,
                    "Got error encoding Encoding Dwarf_Signed");
                break;
            }
            res = dwarf_decode_signed_leb128(
                vbuf,&silen, &decodedvalue,
                vbuf + sizeof(vbuf));
            if (res == DW_DLV_ERROR) {
                DWARF_CHECK_ERROR(check_functions_result,
                    "Got error encoding Decoding Dwarf_Signed");
                break;
            }
            if ( decodedvalue != basevalue) {
                DWARF_CHECK_ERROR(check_functions_result,
                    "Decode Dwarf_signed does not match"
                    "starting value");
                break;
            }
        }
    }
    /* ======= BEGIN FINDING NAMES AND OPENING FDs ===== */
    /*  The 200+2 etc is more than suffices for the expansion that a
        MacOS dsym or a GNU debuglink might need, we hope. */
    temp_path_buf_len = strlen(file_name)*3 + 200 + 2;
    temp_path_buf = malloc(temp_path_buf_len);
    if (!temp_path_buf) {
        printf("%s ERROR:  Unable to malloc %lu bytes "
            "for possible path string %s.\n",
            glflags.program_name,(unsigned long)temp_path_buf_len,
            file_name);
        global_destructors();
        exit(EXIT_FAILURE);
    }
    temp_path_buf[0] = 0;
    /*  This data scan is to find Elf objects and
        unknown objects early.  If the user
        asks for certain options
        that will rule out handling GNU_debuglink
        on that object.  This does not concern itself
        with dSYM or debuglink at all. */
    res = dwarf_object_detector_path_b(file_name,
        0,0,
        0,0,
        &ftype,&endian,&offsetsize,&filesize,
        &path_source,&errcode);
    if (res != DW_DLV_OK) {
        if (res == DW_DLV_ERROR) {
            printf("%s ERROR:  Can't open/use %s. Error %s\n",
                glflags.program_name, sanitized(file_name),
                dwarf_errmsg_by_number(errcode));
        } else {
            printf("%s ERROR:  Can't open %s. "
                "There is no valid object file with that name)\n",
                glflags.program_name, sanitized(file_name));
        }
        global_destructors();
        free(temp_path_buf);
        exit(EXIT_FAILURE);
    }
    esb_append(&global_file_name,file_name);
    temp_path_buf[0] = 0;
    global_basefd = open_a_file(esb_get_string(
        &global_file_name));
    if (global_basefd == -1) {
        printf("%s ERROR:  can't open.. %s\n",
            glflags.program_name,
            esb_get_string(&global_file_name));
        global_destructors();
        free(temp_path_buf);
        exit(EXIT_FAILURE);
    }

    if (esb_string_len(glflags.config_file_tiedpath) > 0) {
        unsigned         tftype = 0;
        unsigned         tendian = 0;
        unsigned         toffsetsize = 0;
        Dwarf_Unsigned   tfilesize = 0;
        const char * tied_file_name = 0;
        /* path_source will be DW_PATHSOURCE_basic  */
        unsigned char    tpath_source = 0;

        temp_path_buf[0] = 0;
        tied_file_name = esb_get_string(glflags.config_file_tiedpath);
        /*  A genuine tiedpath cannot be dsym or debuglink. */
        res = dwarf_object_detector_path_b (tied_file_name,
            0,0,
            0,0,
            &tftype,&tendian,&toffsetsize,&tfilesize,
            &tpath_source,&errcode);
        if (res != DW_DLV_OK) {
            if (res == DW_DLV_ERROR) {
                char *errmsg = 0;
                struct esb_s m;

                esb_constructor(&m);
                errmsg = dwarf_errmsg_by_number(errcode);
                homeify((char *)tied_file_name,&m);
                printf("%s ERROR:  can't open tied file"
                    ".. %s: %s\n",
                    glflags.program_name,
                    sanitized(esb_get_string(&m)),
                    errmsg);
                esb_destructor(&m);
            } else {
                struct esb_s m;
                esb_constructor(&m);
                homeify((char *)tied_file_name,&m);
                printf(
                    "%s ERROR: tied file not an object file '%s'.\n",
                    glflags.program_name,
                    sanitized(esb_get_string(&m)));
                esb_destructor(&m);
            }
            glflags.gf_count_major_errors++;
            global_destructors();
            free(temp_path_buf);
            exit(EXIT_FAILURE);
        }
        if (ftype != tftype || endian != tendian ||
            offsetsize != toffsetsize) {
            struct esb_s m;
            struct esb_s mgf;

            esb_constructor(&m);
            esb_constructor(&mgf);
            homeify((char *)sanitized(tied_file_name),&m);
            homeify((char *)esb_get_string(&global_file_name),&mgf);
            printf("%s ERROR:  tied file \'%s\' and "
                "main file \'%s\' not "
                "the same kind of object!\n",
                glflags.program_name,
                sanitized(esb_get_string(&m)),
                sanitized(esb_get_string(&mgf)));
            esb_destructor(&m);
            esb_destructor(&mgf);
            free(temp_path_buf);
            global_destructors();
            glflags.gf_count_major_errors++;
            exit(EXIT_FAILURE);
        }
        esb_append(&global_tied_file_name,tied_file_name);
        global_tiedfd = open_a_file(esb_get_string(
            &global_tied_file_name));
        if (global_tiedfd == -1) {
            struct esb_s m;

            esb_constructor(&m);
            homeify((char *)sanitized(esb_get_string(
                &global_tied_file_name)), &m);
            printf("%s ERROR:  can't open tied file"
                "... %s\n",
                glflags.program_name,
                esb_get_string(&m));
            esb_destructor(&m);
            global_destructors();
            glflags.gf_count_major_errors++;
            free(temp_path_buf);
            exit(EXIT_FAILURE);
        }
    }
    /* ======= end FINDING NAMES AND OPENING FDs ===== */
    temp_path_buf[0] = 0;
    /* ======= BEGIN PROCESSING OBJECT FILES BY TYPE ===== */
    if ((ftype == DW_FTYPE_ELF && (glflags.gf_reloc_flag ||
        glflags.gf_header_flag)) ||
        ftype == DW_FTYPE_ARCHIVE) {
        printf("ERROR Can't process %s: archives and "
            "printing elf headers not supported in this dwarfdump "
            "--disable-libelf build.\n",
            file_name);
        glflags.gf_count_major_errors++;
    } else if (ftype == DW_FTYPE_ELF ||
        ftype ==  DW_FTYPE_MACH_O  ||
        ftype ==  DW_FTYPE_APPLEUNIVERSAL  ||
        ftype == DW_FTYPE_PE  ) {
        flag_data_pre_allocation();
        close_a_file(global_basefd);
        global_basefd = -1;
        close_a_file(global_tiedfd);
        global_tiedfd = -1;
        process_one_file(
            esb_get_string(&global_file_name),
            esb_get_string(&global_tied_file_name),
            temp_path_buf, (unsigned int)temp_path_buf_len,
            glflags.config_file_data);
        flag_data_post_cleanup();
    } else {
        printf("ERROR Can't process %s: unhandled format\n",
            file_name);
        glflags.gf_count_major_errors++;
    }
    free(temp_path_buf);
    temp_path_buf = 0;
    temp_path_buf_len = 0;
    /* ======= END PROCESSING OBJECT FILES BY TYPE ===== */

    /*  These cleanups only necessary once all
        objects processed. */
    /*  In case of a serious DWARF error
        we  try to get here, we try not
        to  exit(1) by using print_error() */
    check_for_major_errors();
    check_for_notes();
    flag_data_post_cleanup();
    global_destructors();
    free(temp_path_buf);
    /*  As the tool have reached this point, it means there are
        no internal errors and we should return an OKAY condition,
        regardless if the file being processed has
        minor errors. */
    exit(0);
}

void
print_any_harmless_errors(Dwarf_Debug dbg)
{
    /*  We do not need to initialize the local array,
        libdwarf does it. */
    const char *buf[LOCAL_PTR_ARY_COUNT];
    unsigned totalcount = 0;
    unsigned i = 0;
    unsigned printcount = 0;
    int res = dwarf_get_harmless_error_list(dbg,
        LOCAL_PTR_ARY_COUNT,buf,
        &totalcount);
    if (res == DW_DLV_NO_ENTRY) {
        return;
    }
    if (totalcount > 0) {
        printf("\n*** HARMLESS ERROR COUNT: %u ***\n",totalcount);
    }
    for (i = 0 ; buf[i]; ++i) {
        ++printcount;
        DWARF_CHECK_COUNT(harmless_result,1);
        DWARF_CHECK_ERROR(harmless_result,buf[i]);
    }
    if (totalcount > printcount) {
        /*harmless_result.checks += (totalcount - printcount); */
        DWARF_CHECK_COUNT(harmless_result,(totalcount - printcount));
        /*harmless_result.errors += (totalcount - printcount); */
        DWARF_ERROR_COUNT(harmless_result,(totalcount - printcount));
    }
}

/* Print a summary of search results */
static void
print_search_results(void)
{
    const char *search_type = 0;
    const char *search_text = 0;
    if (glflags.search_any_text) {
        search_type = "any";
        search_text = glflags.search_any_text;
    } else {
        if (glflags.search_match_text) {
            search_type = "match";
            search_text = glflags.search_match_text;
        } else {
            search_type = "regex";
            search_text = glflags.search_regex_text;
        }
    }
    fflush(stdout);
    printf("\nSearch type      : '%s'\n",search_type);
    printf("Pattern searched : '%s'\n",search_text);
    printf("Occurrences Found: %d\n",glflags.search_occurrences);
    fflush(stdout);
}

/*  This is for dwarf_print_lines(), we are not using
    the userdata because dwarfdump does not need it. */
static void
printf_callback_for_libdwarf(void *userdata,
    const char *data)
{
    (void)userdata;
    printf("%s",sanitized(data));
}

int
get_address_size_and_max(Dwarf_Debug dbg,
    Dwarf_Half * size,
    Dwarf_Addr * max,
    Dwarf_Error *aerr)
{
    int dres = 0;
    Dwarf_Half lsize = 4;
    /* Get address size and largest representable address */
    dres = dwarf_get_address_size(dbg,&lsize,aerr);
    if (dres != DW_DLV_OK) {
        return dres;
    }
    if (max) {
        *max = (lsize == 8 ) ? 0xffffffffffffffffULL : 0xffffffff;
    }
    if (size) {
        *size = lsize;
    }
    return DW_DLV_OK;
}

/* dbg is often null when dbgtied was passed in. */
static void
dbgsetup(Dwarf_Debug dbg,struct dwconf_s *setup_config_file_data)
{
    if (!dbg) {
        return;
    }
    dwarf_set_frame_rule_initial_value(dbg,
        (Dwarf_Half)setup_config_file_data->cf_initial_rule_value);
    dwarf_set_frame_rule_table_size(dbg,
        (Dwarf_Half)setup_config_file_data->cf_table_entry_count);
    dwarf_set_frame_cfa_value(dbg,
        (Dwarf_Half)setup_config_file_data->cf_cfa_reg);
    dwarf_set_frame_same_value(dbg,
        (Dwarf_Half)setup_config_file_data->cf_same_val);
    dwarf_set_frame_undefined_value(dbg,
        (Dwarf_Half)setup_config_file_data->cf_undefined_val);
    if (setup_config_file_data->cf_address_size) {
        /*  In libdwarf we mostly use Dwarf_Half,
            but Dwarf_Small works ok here. */
        dwarf_set_default_address_size(dbg,
            (Dwarf_Small)setup_config_file_data->cf_address_size);
    }
    dwarf_set_harmless_error_list_size(dbg,50);
}

/*  Callable at any time, Sets section sizes with the sizes
    known as of the call.
    Repeat whenever about to  reference a size that might not
    have been set as of the last call. */
static void
set_global_section_sizes(Dwarf_Debug dbg)
{
    dwarf_get_section_max_offsets_d(dbg,
        &glflags.section_high_offsets_global->debug_info_size,
        &glflags.section_high_offsets_global->debug_abbrev_size,
        &glflags.section_high_offsets_global->debug_line_size,
        &glflags.section_high_offsets_global->debug_loc_size,
        &glflags.section_high_offsets_global->debug_aranges_size,
        &glflags.section_high_offsets_global->debug_macinfo_size,
        &glflags.section_high_offsets_global->debug_pubnames_size,
        &glflags.section_high_offsets_global->debug_str_size,
        &glflags.section_high_offsets_global->debug_frame_size,
        &glflags.section_high_offsets_global->debug_ranges_size,
        &glflags.section_high_offsets_global->debug_pubtypes_size,
        &glflags.section_high_offsets_global->debug_types_size,
        &glflags.section_high_offsets_global->debug_macro_size,
        &glflags.section_high_offsets_global->debug_str_offsets_size,
        &glflags.section_high_offsets_global->debug_sup_size,
        &glflags.section_high_offsets_global->debug_cu_index_size,
        &glflags.section_high_offsets_global->debug_tu_index_size,
        &glflags.section_high_offsets_global->debug_names_size,
        &glflags.section_high_offsets_global->debug_loclists_size,
        &glflags.section_high_offsets_global->debug_rnglists_size);

}

/*  Set limits for Ranges Information.
    The linker may
    put parts of the text(code) in additional sections
    such as .init .fini __libc_freeres_fn
    .rodata __libc_subfreeres __libc_atexit too. */
#define LIKELYNAMESMAX 3

/*  This is useless if the compiler uses unusual section
    names like "text" */
static const char *likely_ns[LIKELYNAMESMAX] = {
/*  .text is first as it is often the only thing.See below. */
".init",
".text",
".fini"
};
#define ORIGLKLYTEXTINDEX  1
struct likely_names_s {
    const char *   name;
    int            origindex;
    Dwarf_Unsigned low;
    Dwarf_Unsigned size;
    Dwarf_Unsigned end;
};
static struct likely_names_s likely_names[LIKELYNAMESMAX];
#if 0 /* FOR DEBUG ONLY */
static void
printlnrec(const char *msg,struct likely_names_s * ln,
    int line,char * fn)
{
    printf("%s: name %s origindx %d "
        "low 0x%lx "
        "size 0x%lx "
        "end 0x%lx "
        " line  %d %s\n",msg,
        ln->name,ln->origindex,
        (unsigned long)ln->low,
        (unsigned long)ln->size,
        (unsigned long)ln->end,line,fn);
}
#endif /* 0 */

static int
likelycmp(const void *l_in, const void *r_in)
{
    struct likely_names_s *l = (struct likely_names_s *)l_in;
    struct likely_names_s *r = (struct likely_names_s *)r_in;

    if (l->low < r->low) {
        return -1;
    }
    if (l->low > r->low ) {
        return 1;
    }
    if (l->end < r->end) {
        return -1;
    }
    if (l->end > r->end) {
        return 1;
    }
    return 0;
}

static int
limit_of_code_when_elf(Dwarf_Debug dbg,
    Dwarf_Unsigned         objtype /* Elf ET_EXEC etc */,
    struct likely_names_s *ln,
    Dwarf_Unsigned         count)
{
    Dwarf_Unsigned ct = 0;
    unsigned int  etype = objtype&0xff;

    if (etype != ET_DYN && etype != ET_EXEC &&
        etype != ET_REL) {
        /* We do not know what this is. */
        return DW_DLV_OK;
    }
    for (ct = 0 ; ct < count; ct++) {
        Dwarf_Unsigned caddr = 0;
        Dwarf_Unsigned csize = 0;
        Dwarf_Unsigned cflags = 0;
        Dwarf_Unsigned coffset = 0;
        const char    *cname = 0;
        int res = 0;
        Dwarf_Error err = 0;
        struct likely_names_s *lx = 0;

        lx = ln + ct;
        /*  The truncation of ct here is awful. Sorry. */
        res = dwarf_get_section_info_by_index_a(dbg,(int)ct,&cname,
            &caddr,&csize,&cflags,&coffset,&err);
        if (res == DW_DLV_ERROR) {
            dwarf_dealloc_error(dbg,err);
            return res;
        }
        if (res == DW_DLV_NO_ENTRY) {
            continue;
        }
        if (!(cflags & SHF_ALLOC)) {
            continue;
        }
        lx->name = cname;
        lx->low = caddr;
        lx->size = csize;
        lx->end = csize +caddr;
        /* Horrible cast. Sorry. */
        lx->origindex = (int)ct;
    }
    return DW_DLV_OK;
}

/*  There is no error arg.  We return DW_DLV_ERROR or
    DW_DLV_NO_ENTRY or DW_DLV_OK  */
static int
limit_of_code_non_elf(Dwarf_Debug dbg,
    struct likely_names_s *ln,
    Dwarf_Unsigned lncount,
    Dwarf_Unsigned *basesize_out,
    Dwarf_Unsigned *baselow_out)
{
    Dwarf_Unsigned basesize = 0;
    Dwarf_Unsigned baselow = 0;
    Dwarf_Unsigned ct = 0;
    Dwarf_Unsigned lnindex = 0;

    for (ct = 0 ; ct < lncount; ct++) {
        Dwarf_Unsigned clow = 0;
        Dwarf_Unsigned csize = 0;
        int            res = 0;
        Dwarf_Error    err = 0;
        /* Just looks for .text and .init and  .fini for ranges. */
        const char    *name = likely_ns[ct];
        struct likely_names_s *lx = 0;

        lx = ln + ct;
        res = dwarf_get_section_info_by_name_a(dbg,name,
            &clow,&csize,0,0,&err);
        if (res == DW_DLV_ERROR) {
            dwarf_dealloc_error(dbg,err);
            return res;
        }
        if (res == DW_DLV_NO_ENTRY) {
            continue;
        }
        lx->name = name;
        lx->low = clow;
        lx->size = csize;
        lx->end = csize +clow;
        /* Horrible cast. Sorry. */
        lx->origindex = (int)ct;
        if (ct == ORIGLKLYTEXTINDEX) {
            basesize = csize;
            baselow  = clow;
        }
        ++lnindex;
    }
    if (!lnindex) {
        return DW_DLV_NO_ENTRY;
    }
    if (lnindex == 1) {
        *baselow_out = baselow;
        *basesize_out  = basesize;
        return DW_DLV_OK;
    }
    return DW_DLV_OK;
}
/*  This is a bit slow, but happens only once for a dbg.
    It is not as much help as I expected in avoiding
    line table content CHECK warnings because, so far,
    those come from .init csu code and the DWARF has
    no subprogram information nor any high/low pc
    information at all.

    Builds a list of addr, endaddr entries,
    sorts by addr, merges into an overall low, high pair.

*/
static int
calculate_likely_limits_of_code(Dwarf_Debug dbg,
    Dwarf_Unsigned *lower,
    Dwarf_Unsigned *size)
{
    struct likely_names_s *ln = 0;
    Dwarf_Bool             ln_is_malloc = FALSE;
    Dwarf_Unsigned         baselow = 0;
    Dwarf_Unsigned         basesize = 0;
    Dwarf_Unsigned         baseend = 0;
    int                   lnindex = 0;
    int                   lncount = 0;
    int                   res = 0;
    Dwarf_Small           dw_ftype = 0;
    Dwarf_Small           dw_obj_pointersize = 0;
    Dwarf_Bool            dw_obj_is_big_endian = 0;
    Dwarf_Unsigned        dw_obj_machine = 0;
    Dwarf_Unsigned        dw_obj_type = 0; /* ELF ET_EXEC etc*/
    Dwarf_Unsigned        dw_obj_flags = 0;
    Dwarf_Small           dw_path_source = 0;
    Dwarf_Unsigned        dw_ub_offset = 0;
    Dwarf_Unsigned        dw_ub_count = 0;
    Dwarf_Unsigned        dw_ub_index = 0;
    Dwarf_Unsigned        dw_comdat_groupnumber = 0;

    res = dwarf_machine_architecture_a(dbg,
        &dw_ftype,
        &dw_obj_pointersize,
        &dw_obj_is_big_endian,
        &dw_obj_machine,
        &dw_obj_type,
        &dw_obj_flags,
        &dw_path_source,
        &dw_ub_offset,
        &dw_ub_count,
        &dw_ub_index,
        &dw_comdat_groupnumber);
    if (res != DW_DLV_OK) {
        return DW_DLV_NO_ENTRY;
    }

    if (dw_ftype == DW_FTYPE_ELF ) {
        lncount = (int)dwarf_get_section_count(dbg);
        if (!lncount) {
            return DW_DLV_NO_ENTRY;
        }
        if (lncount > 50) {
            /*  Very odd. Let's truncate as
                it seens sensible to give up finding
                valid addresses */
            lncount = 50;
        }
        ln = calloc(lncount,sizeof(struct likely_names_s));
        if (!ln) {
            return DW_DLV_ERROR;
        }
        ln_is_malloc = TRUE;
        res = limit_of_code_when_elf(dbg,
            dw_obj_type, ln, lncount);
        if (res != DW_DLV_OK) {
            free(ln);
            ln = 0;
            return res;
        }
    } else {
        lncount = LIKELYNAMESMAX;
        memset(likely_names,0,sizeof(likely_names));
        res = limit_of_code_non_elf(dbg,
            likely_names,
            lncount,
            &basesize,&baselow);
        ln = likely_names;
    }

    qsort(ln,lncount,sizeof(struct likely_names_s),
        likelycmp);
    baselow = ln->low;
    basesize =ln->size;
    baseend = ln->end;
    for (lnindex = 1; lnindex<lncount; ++lnindex) {
        struct likely_names_s*lx = ln+lnindex;
        if (lx->end > baseend) {
            baseend = lx->end;
            basesize = (baseend - baselow);
        }
    }
    if (ln_is_malloc) {
        free(ln);
    }
    if (!baselow) {
        /*  Initial 'page' is certainly not a valid
            address from dwarf. But ET_REL maybe. */
        baselow = 512;
    }
    *lower = baselow;
    *size  = basesize;
    return DW_DLV_OK;
}

/*  So that regression testing can work better
    we substitute '$HOME" where the string s
    began with the value of that environment
    variable.  Otherwise, we just fill in
    the esb with the name as it came in.

    ASSERT: s non-null and points to a valid C string.
*/
static void
homeify(char *s, struct esb_s* out)
{
    char *home = getenv("HOME");
    size_t homelen = 0;

    if (!home) {
        /* giving up */
        esb_append(out,s);
        return;
    }
    homelen = strlen(home);
    if (strlen(s) <= homelen) {
        /*  Giving up, s is shorter than $HOME alone. */
        esb_append(out,s);
        return;
    }
    /*  Checking one-past the hoped-for home prefix */
    if (homelen && s[homelen] != '/') {
        /* giving up */
        esb_append(out,s);
        return;
    }
    if (strncmp(s,(const char *)home,homelen)) {
        /*  Giving up, the initial characters do not
            match $HOME */
        esb_append(out,s);
        return;
    }
    esb_append(out,"$HOME");
    /*  Append, starting at the / in s */
    esb_append(out,s+homelen);
    return;
}

/*  Given a file which is an object type
    we think we can read, process the dwarf data.  */
static int
process_one_file(
    const char * file_name,
    const char * tied_file_name,
    char *       temp_path_buf,
    size_t       temp_path_buf_len,
    struct dwconf_s *l_config_file_data)
{
    Dwarf_Debug   dbg = 0;
    Dwarf_Debug   dbgtied = 0;
    int           dres = 0;
    struct Dwarf_Printf_Callback_Info_s printfcallbackdata;
    Dwarf_Half    elf_address_size = 0;      /* Target pointer size */
    Dwarf_Error   onef_err = 0;
    const char   *title = 0;
    unsigned char path_source = 0;
    int           localerrno = 0;

    if (glflags.gf_no_check_duplicated_attributes) {
        /*  This means libdwarf won't check for duplicated
            attributes. Generally unwise as this allows
            a kind of Denial Of Service with a tailored
            compilation unit: makes some calls
            with a tailored CU very very very slow. */
        dwarf_library_allow_dup_attr(TRUE);
    }
    /*  If using a tied file group number should be
        2 DW_GROUPNUMBER_DWO
        but in a dwp or separate-split-dwarf object then
        0 DW_GROUPNUMBER_ANY will find the .dwo data
        automatically. */
    {
        /*  This will go for the real main file, whether
            an underlying dSYM or via debuglink or
            if those find nothing then the original.
            Unless glflags.gf_no_follow_debuglink
            or glflags.gf_no_follow_dsym (on Apple) is set! */
        char  *tb = temp_path_buf;
        size_t tblen = temp_path_buf_len;
        title = "dwarf_init_path_dl fails.";
        if (glflags.gf_no_follow_debuglink ||
            glflags.gf_no_follow_dsym) {
            tb = 0;
            tblen = 0;
        }
        dres = dwarf_init_path_dl_a(file_name,
            tb,(unsigned int)tblen,
            (unsigned int)glflags.group_number,
            (unsigned int)glflags.gf_universalnumber,
            NULL, NULL, &dbg,
            glflags.gf_global_debuglink_paths,
            (unsigned int)glflags.gf_global_debuglink_count,
            &path_source,
            &onef_err);
    }
    if (dres == DW_DLV_NO_ENTRY) {
        if (glflags.group_number > 0) {
            struct esb_s m;

            esb_constructor(&m);
            homeify((char *)file_name,&m);
            printf("No DWARF information present in %s "
                "for section group %d \n",
                sanitized(esb_get_string(&m)),glflags.group_number);
            esb_destructor(&m);
        } else {
            struct esb_s m;

            esb_constructor(&m);
            homeify((char *)file_name,&m);
            printf("No DWARF information present in %s\n",
                sanitized(esb_get_string(&m)));
            esb_destructor(&m);
        }
        return dres;
    }
    if (dres == DW_DLV_ERROR) {
        /* Prints error, cleans up Dwarf_Error data. */
        print_error_and_continue(
            title,dres,onef_err);
        DROP_ERROR_INSTANCE(dbg,dres,onef_err);
        return DW_DLV_NO_ENTRY;
    }
    if (path_source == DW_PATHSOURCE_dsym) {
        struct esb_s homifiedname;

        esb_constructor(&homifiedname);
        homeify(temp_path_buf,&homifiedname);
        printf("Filename by dSYM is %s\n",
            sanitized(esb_get_string(&homifiedname)));
        esb_destructor(&homifiedname);
    } else if (path_source == DW_PATHSOURCE_debuglink) {
        struct esb_s homifiedname;

        esb_constructor(&homifiedname);
        homeify(temp_path_buf,&homifiedname);
        printf("Filename by debuglink is %s\n",
            sanitized(esb_get_string(&homifiedname)));
        esb_destructor(&homifiedname);
        glflags.gf_gnu_debuglink_flag = TRUE;
    } else { /* Nothing to print yet. */ }
    {
        Dwarf_Unsigned index = 0;
        Dwarf_Unsigned count = 0;
        dres = dwarf_get_universalbinary_count(dbg,&index,&count);
        if (dres == DW_DLV_OK) {
            const char * name = "object";
            if (count != 1) {
                name = "objects";
            }
            printf("This is a Mach-O Universal Binary with %"
                DW_PR_DUu " %s.  This is object %"
                DW_PR_DUu "\n",
                count,name,index);
        }
        if (glflags.gf_machine_arch_flag) {
            print_machine_arch(dbg);
        }
    }
    if (tied_file_name && strlen(tied_file_name)) {
        {
            /*  The tied file we define as group 1, BASE.
                Cannot follow debuglink or dSYM,
                is a tied file  */
            dres = dwarf_init_path_a(tied_file_name,
                0,0,  /* ignore dSYM & debuglink */
                DW_GROUPNUMBER_BASE,
                glflags.gf_universalnumber,
                0,0,
                &dbgtied,
                &onef_err);
            /* path_source = DW_PATHSOURCE_basic; */
        }
        if (dres == DW_DLV_NO_ENTRY) {
            struct esb_s m;

            esb_constructor(&m);
            homeify((char *)tied_file_name,&m);
            printf("No DWARF information present in tied file: %s\n",
                sanitized(esb_get_string(&m)));
            esb_destructor(&m);
            return dres;
        }
        if (dres == DW_DLV_ERROR) {
            /*  Prints error, cleans up Dwarf_Error data.
                Never returns*/
            print_error(dbg,
                "dwarf_init_path on tied_file",
                dres, onef_err);
        }
        {
            Dwarf_Unsigned index = 0;
            Dwarf_Unsigned count = 0;
            dres = dwarf_get_universalbinary_count(dbg,&index,&count);
            if (dres == DW_DLV_OK) {
                const char * name = "object";
                if (count != 1) {
                    name = "objects";
                }
                printf("The tied-file is a Mach-O Universal Binary "
                    "with %"
                    DW_PR_DUu " %s.  This is object %"
                    DW_PR_DUu "\n",
                    count,name,index);
            }
        }
    }

    memset(&printfcallbackdata,0,sizeof(printfcallbackdata));
    printfcallbackdata.dp_fptr = printf_callback_for_libdwarf;
    dwarf_register_printf_callback(dbg,&printfcallbackdata);
    if (dbgtied) {
        dwarf_register_printf_callback(dbgtied,&printfcallbackdata);
    }
    memset(&printfcallbackdata,0,sizeof(printfcallbackdata));

    dbgsetup(dbg,l_config_file_data);
    dbgsetup(dbgtied,l_config_file_data);
    /*  Speed things up by not checking for
        harmless errors (in libdwarf).
        As of 25 November 2025 disabled is
        the default in libdwarf v2.2.1 :
        dwarf_set_harmless_errors_enabled(dbgtied,0);
        dwarf_set_harmless_errors_enabled(dbg,0); */
    if (!glflags.gf_suppress_harmless) {
        /*  This is the default in dwarfdump: check
            for harmless errors. So we tell libdwarf
            to check.  */
        if (dbgtied) {
            dwarf_set_harmless_errors_enabled(dbgtied,1);
        }
        dwarf_set_harmless_errors_enabled(dbg,1);
    }
    dres = get_address_size_and_max(dbg,&elf_address_size,0,
        &onef_err);
    if (dres != DW_DLV_OK) {
        print_error(dbg,"Unable to read address"
            " size so unable to continue",
            dres,onef_err);
    }
    if (glflags.gf_check_tag_attr ||
        glflags.gf_print_usage_tag_attr ||
        glflags.gf_check_tag_tree) {
        dres = dd_build_tag_attr_form_base_trees(&localerrno);
        if (dres != DW_DLV_OK) {
            simple_err_return_msg_either_action(dres,
                "ERROR: Failed to initialize tag/attribute/form"
                " tables properly");
        }
    }

    /*  Ok for dbgtied to be NULL. */
    dres = dwarf_set_tied_dbg(dbg,dbgtied,&onef_err);
    if (dres != DW_DLV_OK) {
        print_error(dbg, "dwarf_set_tied_dbg() failed",
            dres, onef_err);
    }

    /*  Get .text and .debug_ranges info if in check mode.
        Depending on the section count and layout
        it is possible this
        will not get all the sections it wants to:
        See calculate_likely_limits_of_code().  */
    if (glflags.gf_do_check_dwarf) {
        Dwarf_Addr lower = 0;
        Dwarf_Addr upper = 0;
        Dwarf_Unsigned size = 0;
        Dwarf_Debug dbg_with_code = dbg;
        int res = 0;

        if (dbgtied) {
            /*  Assuming tied is executable main is dwo/dwp */
            dbg_with_code = dbgtied;
        }
        res = calculate_likely_limits_of_code(dbg_with_code,
            &lower,&size);
        upper = lower + size;
        /*  Set limits for Ranges Information.
            Some objects have CUs for startup code
            and the expanded range here turns out
            not to actually help.   */
        if (res == DW_DLV_OK && glflags.pRangesInfo) {
            /*  Recording high/low for .test .init .fini */
            SetLimitsBucketGroup(glflags.pRangesInfo,lower,upper);
            AddEntryIntoBucketGroup(glflags.pRangesInfo,
                1,
                lower,lower,
                upper,
                ".text",
                TRUE);
        }
        /*  Build section information for linkonce.
            linkonce is related to, for example,
            gcc .gnu.linkonce.?? sections we have no reason to think
            this really works, we have no test cases as of 2022. */
        build_linkonce_info(dbg);
    }
    if (glflags.gf_section_groups_flag) {
        int res = 0;
        Dwarf_Error err = 0;

        res = print_section_groups_data(dbg,&err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing section groups had a problem.",
                res,err);
            DROP_ERROR_INSTANCE(dbg,res,err);
        }
        /*  If groupnum > 2 this turns off some
            of the gf_flags here so we don't print
            section names of things we do not
            want to print. */
        update_section_flags_per_groups();
    }
    reset_overall_CU_error_data();
    if (glflags.gf_print_all_srcfiles ||
        glflags.gf_info_flag || glflags.gf_line_flag ||
        glflags.gf_types_flag ||
        glflags.gf_check_macros || glflags.gf_macinfo_flag ||
        glflags.gf_macro_flag ||
        glflags.gf_cu_name_flag || glflags.gf_search_is_on ||
        glflags.gf_producer_children_flag) {
        Dwarf_Error err = 0;
        int res = 0;

        reset_overall_CU_error_data();
        res = print_infos(dbg,TRUE,&err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing .debug_info had a problem.",
                res,err);
            DROP_ERROR_INSTANCE(dbg,res,err);
        }
        reset_overall_CU_error_data();
        res = print_infos(dbg,FALSE,&err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing .debug_types had a problem.",
                res,err);
            DROP_ERROR_INSTANCE(dbg,res,err);
        }
        {
            set_global_section_sizes(dbg);
            /*  The statistics are for ALL of the
                DWARF5 (and DWARF4 with .debug_macro)
                across all CUs.  */
            if (macro_check_tree) {
                /* debug_macro_size is to check the section end */
                print_macrocheck_statistics("DWARF5 .debug_macro",
                    &macro_check_tree,
                    /* DWARF5 */ TRUE,
                    glflags.section_high_offsets_global->
                        debug_macro_size);
            }
        }
        if (glflags.gf_check_macros) {
            if (macinfo_check_tree) {
                /* debug_macinfo_size is to check the section end */
                print_macrocheck_statistics("DWARF2 .debug_macinfo",
                    &macinfo_check_tree,
                    /* DWARF5 */ FALSE,
                    glflags.section_high_offsets_global->
                        debug_macinfo_size);
            }
        }
        clear_macrocheck_statistics(&macro_check_tree);
        clear_macrocheck_statistics(&macinfo_check_tree);
    }
    if (glflags.gf_gdbindex_flag) {
        int res = 0;
        Dwarf_Error err = 0;

        reset_overall_CU_error_data();
        /*  By definition if gdb_index is present
            then "cu" and "tu" will not be. And vice versa.  */
        res = print_gdb_index(dbg,&err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing the gdb index section had a problem "
                ,res,err);
            DROP_ERROR_INSTANCE(dbg,res,err);
        }
        res = print_debugfission_index(dbg,"cu",&err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing the debugfission cu section "
                "had a problem "
                ,res,err);
            DROP_ERROR_INSTANCE(dbg,res,err);
        }
        res = print_debugfission_index(dbg,"tu",&err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing the debugfission tu section "
                "had a problem "
                ,res,err);
            DROP_ERROR_INSTANCE(dbg,res,err);
        }
    }
    if (glflags.gf_pubnames_flag) {
        int res = 0;
        Dwarf_Error err = 0;

        reset_overall_CU_error_data();
        res = print_pubnames_style(dbg,DW_GL_GLOBALS,&err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing pubnames data had a problem ",res,err);
            DROP_ERROR_INSTANCE(dbg,res,err);
        }
    }
    if (glflags.gf_debug_addr_flag) {
        Dwarf_Error err = 0;
        int res = 0;

        reset_overall_CU_error_data();
        res = print_debug_addr(dbg,&err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing the .debug_addr section"
                " had a problem.",res,err);
            DROP_ERROR_INSTANCE(dbg,res,err);
        }
    }
    if (glflags.gf_abbrev_flag) {
        Dwarf_Error err = 0;
        int res = 0;

        reset_overall_CU_error_data();
        res = print_abbrevs(dbg,&err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing the .debug_abbrev section"
                " had a problem.",res,err);
            DROP_ERROR_INSTANCE(dbg,res,err);
        }
    }
    if (glflags.gf_string_flag) {
        Dwarf_Error err = 0;
        int res = 0;

        reset_overall_CU_error_data();
        res = print_strings(dbg,&err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing the .debug_str section"
                " had a problem.",res,err);
            DROP_ERROR_INSTANCE(dbg,res,err);
        }
    }
    if (glflags.gf_aranges_flag) {
        Dwarf_Error err = 0;
        int res = 0;

        reset_overall_CU_error_data();
        res = print_aranges(dbg,&err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing the aranges section"
                " had a problem.",res,err);
            DROP_ERROR_INSTANCE(dbg,res,err);
        }
    }
    if (glflags.gf_ranges_flag) {
        reset_overall_CU_error_data();
        print_ranges(dbg);
        /* Never returns DW_DLV_ERROR */
    }
    if (glflags.gf_print_raw_loclists) {
        int res = 0;
        Dwarf_Error err = 0;
        /*  This for DWARF5 loclists.
            No raw printing of .debug_loc available. */

        reset_overall_CU_error_data();
        res = print_raw_all_loclists(dbg,&err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing the raw .debug_loclists section"
                " had a problem.",res,err);
            DROP_ERROR_INSTANCE(dbg,res,err);
        }
    }
    if (glflags.gf_print_raw_rnglists &&
        glflags.gf_do_print_dwarf) {

        int res = 0;
        Dwarf_Error err = 0;

        reset_overall_CU_error_data();
        res = print_raw_all_rnglists(dbg,&err);
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing the raw .debug_rnglists section"
                " had a problem.",res,err);
            DROP_ERROR_INSTANCE(dbg,res,err);
        }
    }
    if (glflags.gf_frame_flag || glflags.gf_eh_frame_flag) {
        int sres = 0;
        Dwarf_Error err = 0;
        int want_eh = 0;
        /*  These three shared .eh_frame and .debug_frame
            as they are about the DIEs, not about frames. */
        Dwarf_Die cu_die_for_print_frames = 0;
        void *map_lowpc_to_name = 0;
        void *lowpcSet = 0;

        reset_overall_CU_error_data();
        if (glflags.gf_frame_flag) {
            want_eh = 0;
            sres = print_frames(dbg,want_eh,
                l_config_file_data,
                &cu_die_for_print_frames,
                &map_lowpc_to_name,
                &lowpcSet,
                &err);
            if (sres == DW_DLV_ERROR) {
                print_error_and_continue(
                    "printing standard frame data had a problem.",
                    sres,err);
                DROP_ERROR_INSTANCE(dbg,sres,err);
            }
        }
        if (glflags.gf_eh_frame_flag) {
            want_eh = 1;
            sres = print_frames(dbg, want_eh,
                l_config_file_data,
                &cu_die_for_print_frames,
                &map_lowpc_to_name,
                &lowpcSet,
                &err);
            if (sres == DW_DLV_ERROR) {
                print_error_and_continue(
                    "printing eh frame data had a problem.",sres,
                    err);
                DROP_ERROR_INSTANCE(dbg,sres,err);
            }
        }
        addr_map_destroy(lowpcSet);
        addr_map_destroy(map_lowpc_to_name);
        if (cu_die_for_print_frames) {
            dwarf_dealloc_die(cu_die_for_print_frames);
        }
    }
    if (glflags.gf_static_func_flag) {
        int sres = 0;
        Dwarf_Error err = 0;

        reset_overall_CU_error_data();
        sres = print_pubnames_style(dbg,DW_GL_FUNCS,&err);
        if (sres == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing SGI static funcs had a problem.",sres,err);
            DROP_ERROR_INSTANCE(dbg,sres,err);
        }
    }
    if (glflags.gf_static_var_flag) {
        int sres = 0;
        Dwarf_Error err = 0;

        reset_overall_CU_error_data();
        sres = print_pubnames_style(dbg,DW_GL_VARS,&err);
        if (sres == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing SGI static vars had a problem.",sres,err);
            DROP_ERROR_INSTANCE(dbg,sres,err);
        }
    }
    /*  DWARF_PUBTYPES is the standard typenames dwarf section.
        SGI_TYPENAME is the same concept but is SGI specific ( it was
        defined 10 years before dwarf pubtypes). */

    if (glflags.gf_pubtypes_flag) {
        Dwarf_Error err = 0;
        int tres = 0;

        reset_overall_CU_error_data();
        tres = print_pubnames_style(dbg,DW_GL_PUBTYPES,&err);
        if (tres == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing pubtypes had a problem.",tres,err);
            DROP_ERROR_INSTANCE(dbg,tres,err);
        }
        reset_overall_CU_error_data();
        tres = print_pubnames_style(dbg,DW_GL_TYPES,&err);
        if (tres == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing SGI typenames had a problem.",tres,err);
            DROP_ERROR_INSTANCE(dbg,tres,err);
        }
    }
    if (glflags.gf_weakname_flag) {
        Dwarf_Error err = 0;
        int res3 = 0;

        reset_overall_CU_error_data();
        res3 = print_pubnames_style(dbg,DW_GL_WEAKS,&err);
        if (res3 == DW_DLV_ERROR) {
            print_error_and_continue(
                "printing weaknames had a problem.",res3,err);
            DROP_ERROR_INSTANCE(dbg,res3,err);
        }
    }
    if (glflags.gf_reloc_flag) {
        /* Harmless, though likely not needed here. */
        reset_overall_CU_error_data();
    }
    if (glflags.gf_debug_names_flag) {
        int nres = 0;
        Dwarf_Error err = 0;
        reset_overall_CU_error_data();
        nres = print_debug_names(dbg,&err);
        if (nres == DW_DLV_ERROR) {
            print_error_and_continue(
                "print .debug_names section failed", nres, err);
            DROP_ERROR_INSTANCE(dbg,nres,err);
        }
    }

    /*  Print search results */
    if (glflags.gf_search_print_results && glflags.gf_search_is_on) {
        /* No dwarf errors possible in this function. */
        print_search_results();
    }

    /*  The right time to do this is unclear. But we need to do it. */
    if (glflags.gf_check_harmless) {
        /* No dwarf errors possible in this function. */
        print_any_harmless_errors(dbg);
    }

    /*  Print error report only if errors have been detected
        Print error report if the -kd option.
        No errors possible in this function. */
    print_checks_results();

    /*  Print the detailed attribute usage space
        and free the attributes_encoding data allocated.
        Option -kE
        Also prints the attr/formclass/form reports
        from attr_form.c  See build_attr_form_base()
        call above and record_attr_form_use() in print_die.c */
    if (glflags.gf_check_attr_encoding ) {
        int ares = 0;
        Dwarf_Error aerr = 0;

        ares = print_attributes_encoding(dbg,&aerr);
        if (ares == DW_DLV_ERROR) {
            print_error_and_continue(
                "print attributes encoding failed", ares, aerr);
            DROP_ERROR_INSTANCE(dbg,ares,aerr);
        }
    }

    /*  Print the tags and attribute usage  -ku or -kuf */
    if (glflags.gf_print_usage_tag_attr) {
        int tres = 0;
        Dwarf_Error err = 0;

        tres = print_tag_attributes_usage();
        if (tres == DW_DLV_ERROR) {
            print_error_and_continue(
                "print tag attributes usage failed", tres, err);
            DROP_ERROR_INSTANCE(dbg,tres,err);
        }
    }

    if (glflags.gf_print_str_offsets) {
        /*  print the .debug_str_offsets section, if any. */
        int lres = 0;
        Dwarf_Error err = 0;

        lres = print_str_offsets_section(dbg,&err);
        if (lres == DW_DLV_ERROR) {
            print_error_and_continue(
                "print .debug_str_offsets failed", lres, err);
            DROP_ERROR_INSTANCE(dbg,lres,err);
        }
    }

    /*  prints nothing unless section .gnu_debuglink is present.
        Lets print for a few critical sections.  */
    if (glflags.gf_gnu_debuglink_flag) {
        int lresdl = 0;
        Dwarf_Error err = 0;

        lresdl = print_gnu_debuglink(dbg,&err);
        if (lresdl == DW_DLV_ERROR) {
            print_error_and_continue(
                "print gnu_debuglink data failed", lresdl, err);
            DROP_ERROR_INSTANCE(dbg,lresdl,err);
            err = 0;
        }
    }
    if (glflags.gf_debug_gnu_flag) {
        int lres = 0;
        Dwarf_Error err = 0;

        lres = print_debug_gnu(dbg,&err);
        if (lres == DW_DLV_ERROR) {
            print_error_and_continue(
                "print .debug_gnu* section failed", lres, err);
            DROP_ERROR_INSTANCE(dbg,lres,err);
            err = 0;
        }
    }
    if (glflags.gf_debug_sup_flag) {
        int lres = 0;
        Dwarf_Error err = 0;

        lres = print_debug_sup(dbg,&err);
        if (lres == DW_DLV_ERROR) {
            print_error_and_continue(
                "print .debug_sup* section failed", lres, err);
            DROP_ERROR_INSTANCE(dbg,lres,err);
        }
    }
    if (glflags.gf_print_section_allocations) {
        Dwarf_Unsigned mmap_count = 0;
        Dwarf_Unsigned mmap_size = 0;
        Dwarf_Unsigned malloc_count = 0;
        Dwarf_Unsigned malloc_size = 0;
        Dwarf_Unsigned total_alloc = 0;
        enum Dwarf_Sec_Alloc_Pref pref = 0;
        dwarf_get_mmap_count(dbg,&mmap_count,
            &mmap_size,
            &malloc_count, &malloc_size);
        printf("\n");
        printf("Section allocation summary:\n");
        printf("  Count sections mmap-ed          : %8"
            DW_PR_DUu "\n",
            mmap_count);
        printf("  Size sections mmap-ed           : %8"
            DW_PR_DUu  " (0x%" DW_PR_XZEROS DW_PR_DUx   ")\n",
            mmap_size,mmap_size);

        printf("  Count sections malloc-ed        : %8"
            DW_PR_DUu "\n",
            malloc_count);
        printf("  Size  sections malloc-ed        : %8"
            DW_PR_DUu  " (0x%" DW_PR_XZEROS DW_PR_DUx   ")\n",
            malloc_size,malloc_size);
        total_alloc = malloc_size + mmap_size;
        printf("  Total section allocation (bytes): %8"
            DW_PR_DUu " (0x%"  DW_PR_XZEROS DW_PR_DUx ")\n",
            total_alloc,total_alloc);

        pref = dwarf_set_load_preference(0);
        printf("  Global preference for sections  : %s\n",
            pref == Dwarf_Alloc_Malloc?"Dwarf_Alloc_Malloc":
            pref == Dwarf_Alloc_Mmap?  "Dwarf_Alloc_Mmap":
            "<Unknown. an ERROR");
    }
    if (glflags.gf_print_all_srcfiles) {
        dd_print_all_srcfiles();
        dd_destroy_all_srcfiles();
    }

    if (glflags.gf_print_language_version_table) {
        print_language_version_table();
    }
    if (glflags.gf_debug_addr_missing) {
        printf("\nERROR: At some point "
            "the .debug_addr section was needed but missing, "
            "meaning some frame information was missing "
            "relevant function names. See the dwarfdump "
            " option --file-tied=</path/to/executable>.");
        glflags.gf_count_major_errors++;
    }
    if (glflags.gf_error_code_search_by_address) {
        printf("\nERROR: At some point "
            "There was some data corruption in frame data "
            "so at least the following error occurred: "
            "%s .\n",
            dwarf_errmsg_by_number(
            glflags.gf_error_code_search_by_address));
        glflags.gf_count_major_errors++;
    }

    /*  Could finish dbg first. Either order ok. */
    if (dbgtied) {
        dres = dwarf_finish(dbgtied);
        if (dres != DW_DLV_OK) {
            print_error_and_continue(
                "dwarf_finish failed on tied dbg", dres, onef_err);
            DROP_ERROR_INSTANCE(dbg,dres,onef_err);
        }
        dbgtied = 0;
    }
    groups_restore_subsidiary_flags();
    dres = dwarf_finish(dbg);
    if (dres != DW_DLV_OK) {
        print_error_and_continue(
            "dwarf_finish failed", dres, onef_err);
        DROP_ERROR_INSTANCE(dbg,dres,onef_err);
        dbg = 0;
    }
    printf("\n");
    dd_destroy_tag_attr_form_trees();
    destruct_abbrev_array();
    esb_close_null_device();
    release_range_array_info();
    helpertree_clear_statistics(&helpertree_offsets_base_info);
    helpertree_clear_statistics(&helpertree_offsets_base_types);
    return 0;
}

/*  ==============START of dwarfdump error print functions. */
int
simple_err_return_msg_either_action(int res,const char *msg)
{
    const char *etype = "No-entry";
    if (res == DW_DLV_ERROR) {
        etype="Major error";
    }
    glflags.gf_count_major_errors++;
    printf("%s fails. %s\n",msg,etype);
    return res;
}
int
simple_err_return_action(int res,const char *msg)
{
    if (res == DW_DLV_ERROR) {
        const char *etype = "Major error";
        glflags.gf_count_major_errors++;
        printf("%s %s\n",msg, etype);
    }
    return res;
}

int
simple_err_only_return_action(int res,const char *msg)
{
    const char *etype="Major error";
    /*const char *msg = "\nERROR: dwarf_get_address_size() fails."; */

    glflags.gf_count_major_errors++;
    printf("%s %s\n",msg,etype);
    return res;
}

/*   Does not increment glflags.gf_count_major_errors
    unless a return code is not one of the three
    standard values.
*/
static void
print_error_maybe_continue( const char * msg,
    int dwarf_ret_val,
    Dwarf_Error lerr,
    Dwarf_Bool do_continue)
{
    unsigned long realmajorerr = glflags.gf_count_major_errors;
    printf("\n");
    if (dwarf_ret_val == DW_DLV_ERROR) {
        /* We do not dwarf_dealloc the error here. */
        char * errmsg = dwarf_errmsg(lerr);

        /*  We now (April 2016) guarantee the
            error number is in
            the error string so we do not need to print
            the dwarf_errno() value to show the number. */
        if (do_continue) {
            printf(
                "%s ERROR:  %s:  %s. "
                "Attempting to continue.\n",
                glflags.program_name, msg, errmsg);
        } else {
            printf( "%s ERROR:  %s:  %s\n",
                glflags.program_name, msg, errmsg);
        }
    } else if (dwarf_ret_val == DW_DLV_NO_ENTRY) {
        printf("%s NO ENTRY:  %s: \n",
            glflags.program_name, msg);
    } else if (dwarf_ret_val == DW_DLV_OK) {
        printf("%s:  %s \n", glflags.program_name, msg);
    } else {
        printf("%s ERROR InternalError:  %s:  code %d\n",
            glflags.program_name, msg, dwarf_ret_val);
        ++realmajorerr;
    }
    /* Display compile unit name */
    PRINT_CU_INFO();
    glflags.gf_count_major_errors = realmajorerr;
}

void
print_error(Dwarf_Debug dbg,
    const char * msg,
    int dwarf_ret_val,
    Dwarf_Error lerr)
{
    print_error_maybe_continue(msg,dwarf_ret_val,lerr,FALSE);
    glflags.gf_count_major_errors++;
    if (dwarf_ret_val == DW_DLV_ERROR) {
        /*  If dbg was never initialized
            this still cleans up the Error data. */
        DROP_ERROR_INSTANCE(dbg,dwarf_ret_val,lerr);
        dwarf_finish(dbg);
        check_for_major_errors();
        check_for_notes();
    }
    global_destructors();
    flag_data_post_cleanup();
    dd_destroy_tag_attr_form_trees();
    exit(EXIT_FAILURE);
}
/* ARGSUSED */
void
print_error_and_continue(const char * msg,
    int dwarf_ret_val,
    Dwarf_Error lerr)
{
    glflags.gf_count_major_errors++;
    print_error_maybe_continue(msg,dwarf_ret_val,lerr,TRUE);
}
/*  ==============END of dwarfdump error print functions. */

static Dwarf_Bool
is_a_string_form(int sf)
{
    switch(sf){
    case DW_FORM_string:
    case DW_FORM_GNU_strp_alt:
    case DW_FORM_strp_sup:
    case DW_FORM_GNU_str_index:
    case DW_FORM_strx:
    case DW_FORM_strx1:
    case DW_FORM_strx2:
    case DW_FORM_strx3:
    case DW_FORM_strx4:
    case DW_FORM_strp:
    case DW_FORM_line_strp:
        /*  There is some hope we can actually get
            the string itself, depending on
            other factors */
        return TRUE;
    default: break;
    }
    /* Nope. No string is possible */
    return FALSE;
}
/*  Always sets the return argument *should_skip,
    whether it returns DW_DLV_NO_ENTRY or
    DW_DLV_ERROR or DW_DLV_OK.
    determines if the CU should be
    skipped as the DW_AT_name of the CU
    does not match the command-line-supplied
    cu name.  The two callers ignore the
    return value.
    This suppresses any errors it finds, no
    Dwarf_Error is lost and none is returned. */
int
should_skip_this_cu(Dwarf_Debug dbg, Dwarf_Bool*should_skip,
    Dwarf_Die cu_die)
{
    Dwarf_Half tag = 0;
    Dwarf_Attribute attrib = 0;
    Dwarf_Half theform = 0;
    Dwarf_Error skperr;
    int dares = 0;
    int tres = 0;
    int fres = 0;

    tres = dwarf_tag(cu_die, &tag, &skperr);
    if (tres != DW_DLV_OK) {
        print_error_and_continue("ERROR: "
        "Cannot get the TAG of the cu_die to check "
        " if we should skip this CU or not.",
            tres, skperr);
        *should_skip = FALSE;
        DROP_ERROR_INSTANCE(dbg,tres,skperr);
        return tres;
    }
    dares = dwarf_attr(cu_die, DW_AT_name, &attrib, &skperr);
    if (dares != DW_DLV_OK) {
        print_error_and_continue("should skip this cu? "
            " cu die has no DW_AT_name attribute!",
            dares, skperr);
        *should_skip = FALSE;
        DROP_ERROR_INSTANCE(dbg,dares,skperr);
        return dares;
    }
    fres = dwarf_whatform(attrib, &theform, &skperr);
    if (fres == DW_DLV_OK) {
        if (is_a_string_form(theform)) {
            char * temps = 0;
            int sres = dwarf_formstring(attrib, &temps,
                &skperr);
            if (sres == DW_DLV_OK) {
                char *lcun = esb_get_string(glflags.cu_name);
                char *p = temps;
                if (lcun[0] != '/') {
                    p = strrchr(temps, '/');
                    if (p == NULL) {
                        p = temps;
                    } else {
                        p++;
                    }
                }
                /* Ignore case if Windows */
#if _WIN32
                if (stricmp(lcun, p)) {
                    /* skip this cu. */
                    dwarf_dealloc_attribute(attrib);
                    attrib = 0;
                    *should_skip = TRUE;
                    return DW_DLV_OK;
                }
#else
                if (strcmp(lcun, p)) {
                    /* skip this cu. */
                    dwarf_dealloc_attribute(attrib);
                    attrib = 0;
                    *should_skip = TRUE;
                    return DW_DLV_OK;
                }
#endif /* _WIN32 */

            } else if (sres == DW_DLV_ERROR) {
                struct esb_s m;

                dwarf_dealloc_attribute(attrib);
                attrib = 0;
                esb_constructor(&m);
                esb_append(&m,"In determining if we should "
                    "skip this CU dwarf_formstring "
                    "gets an error on form ");
                esb_append(&m,get_FORM_name(theform));
                esb_append(&m,".");

                print_error_and_continue(
                    esb_get_string(&m),
                    sres, skperr);
                *should_skip = FALSE;
                esb_destructor(&m);
                return sres;
            } else {
                /* DW_DLV_NO_ENTRY on the string itself */
                dwarf_dealloc_attribute(attrib);
                attrib = 0;
                *should_skip = FALSE;
                return sres;
            }
        }
    } else if (fres == DW_DLV_ERROR) {
        /*  DW_DLV_ERROR */
        print_error_and_continue(
            "dwarf_whatform failed on a CU_die when"
            " attempting to determine if this CU should"
            " be skipped.",
            fres, skperr);
        DROP_ERROR_INSTANCE(dbg,fres,skperr);
        fres = DW_DLV_NO_ENTRY;
    } else  {
        /* DW_DLV_NO_ENTRY, nothing to print */
    }
    dwarf_dealloc_attribute(attrib);
    attrib = 0;
    *should_skip = FALSE;
    return fres;
}

/*  Returns the cu of the CUn the name fields when it can,
    else a no-entry
    else DW_DLV_ERROR.  */
int
get_cu_name(Dwarf_Debug dbg, Dwarf_Die cu_die,
    Dwarf_Off dieprint_cu_offset,
    char * *short_name, char * *long_name,
    Dwarf_Error *lerr)
{
    Dwarf_Attribute name_attr = 0;
    int ares = 0;

    ares = dwarf_attr(cu_die, DW_AT_name, &name_attr, lerr);
    if (ares == DW_DLV_ERROR) {
        print_error_and_continue(
            "dwarf_attr fails on DW_AT_name on the CU die",
            ares, *lerr);
        return ares;
    } else if (ares == DW_DLV_NO_ENTRY) {
        *short_name = "<unknown name>";
        *long_name = "<unknown name>";
    } else {
        /* DW_DLV_OK */
        /*  The string return is valid until the next call to this
            function; so if the caller needs to keep the returned
            string, the string must be copied (makename()). */
        char *filename = 0;

        esb_empty_string(&esb_long_cu_name);
        ares = get_attr_value(dbg, DW_TAG_compile_unit,
            cu_die, dieprint_cu_offset,
            name_attr, NULL, 0, &esb_long_cu_name,
            0 /*show_form_used*/,0 /* verbose */,lerr);
        if (ares != DW_DLV_OK)  {
            *short_name = "<unknown name>";
            *long_name = "<unknown name>";
            dwarf_dealloc_attribute(name_attr);
            return ares;
        }
        *long_name = esb_get_string(&esb_long_cu_name);
        /* Generate the short name (filename) */
        filename = strrchr(*long_name,'/');
        if (!filename) {
            filename = strrchr(*long_name,'\\');
        }
        if (filename) {
            ++filename;
        } else {
            filename = *long_name;
        }
        esb_empty_string(&esb_short_cu_name);
        esb_append(&esb_short_cu_name,filename);
        *short_name = esb_get_string(&esb_short_cu_name);
        dwarf_dealloc_attribute(name_attr);
    }
    return ares;
}

/*  Returns the producer of the CU
    Caller must ensure producernameout is
    a valid, constructed, empty esb_s instance before calling.
    */
int
get_producer_name(Dwarf_Debug dbg, Dwarf_Die cu_die,
    Dwarf_Off dieprint_cu_offset,
    struct esb_s *producernameout,
    Dwarf_Error *err)
{
    Dwarf_Attribute producer_attr = 0;
    int ares = 0;
    /*  See also glflags.c for "<unknown>" as default producer
        string */

    if (!cu_die) {
        glflags.gf_count_major_errors++;
        esb_append(producernameout,
            "\"<ERROR: CU-missing-DW_AT_producer (null cu_die)>\"");
        return DW_DLV_NO_ENTRY;
    }
    ares = dwarf_attr(cu_die, DW_AT_producer,
        &producer_attr, err);
    if (ares == DW_DLV_ERROR) {
        glflags.gf_count_major_errors++;
        esb_append(producernameout,
            "\"<ERROR: CU-DW_AT_producer-error>\"");
        return ares;
    }
    if (ares == DW_DLV_NO_ENTRY) {
        /*  We add extra quotes so it looks more like
            the names for real producers that get_attr_value
            produces. */
        /* Same string is in glflags.c */
        esb_append(producernameout,
            "\"<ERROR: CU-missing-DW_AT_producer>\"");
        dwarf_dealloc_attribute(producer_attr);
        return ares;
    }
    /*  DW_DLV_OK */
    ares = get_attr_value(dbg, DW_TAG_compile_unit,
        cu_die, dieprint_cu_offset,
        producer_attr, NULL, 0, producernameout,
        0 /*show_form_used*/,0 /* verbose */,err);
    dwarf_dealloc_attribute(producer_attr);
    return ares;
}

void
print_secname(Dwarf_Debug dbg,const char *secname)
{
    if (glflags.gf_do_print_dwarf) {
        struct esb_s truename;
        char buf[DWARF_SECNAME_BUFFER_SIZE];

        esb_constructor_fixed(&truename,buf,sizeof(buf));
        get_true_section_name(dbg,secname,
            &truename,TRUE);
        printf("\n%s\n",sanitized(esb_get_string(&truename)));
        esb_destructor(&truename);
    }
}

/*  We'll check for errors when checking.
    print only if printing (as opposed to checking). */
static int
print_gnu_debuglink(Dwarf_Debug dbg, Dwarf_Error *error)
{
    int         res = 0;
    char *      name = 0;
    unsigned char *crcbytes = 0;
    char *      link_path = 0;
    unsigned    link_path_len = 0;
    unsigned    buildidtype = 0;
    char       *buildidowner = 0;
    unsigned char *buildidbyteptr = 0;
    unsigned    buildidlength = 0;
    char      **paths_array = 0;
    unsigned    paths_array_length = 0;

    res = dwarf_gnu_debuglink(dbg,
        &name,
        &crcbytes,
        &link_path,     /* free this */
        &link_path_len,
        &buildidtype,
        &buildidowner,
        &buildidbyteptr, &buildidlength,
        &paths_array,  /* free this */
        &paths_array_length,
        error);
    if (res == DW_DLV_NO_ENTRY) {
        return res;
    }
    if (res == DW_DLV_ERROR) {
        print_secname(dbg,".gnu_debuglink");
        return res;
    }
    if (crcbytes) {
        print_secname(dbg,".gnu_debuglink");
        /* Done with error checking, so print if we are printing. */
        if (glflags.gf_do_print_dwarf)  {
            printf(" Debuglink name  : %s",sanitized(name));
            {
                unsigned char *crc = 0;
                unsigned char *end = 0;

                crc = crcbytes;
                end = crcbytes +4;
                printf("   crc 0X: ");
                for (; crc < end; crc++) {
                    printf("%02x ", *crc);
                }
            }
            printf("\n");
            if (link_path_len) {
                printf(" Debuglink target: %s\n",
                    sanitized(link_path));
            }
        }
    }
    if (buildidlength) {
        print_secname(dbg,".note.gnu.build-id");
        if (glflags.gf_do_print_dwarf)  {
            printf(" Build-id  type     : %u\n", buildidtype);
            printf(" Build-id  ownername: %s\n",
                sanitized(buildidowner));
            printf(" Build-id  length   : %u\n",buildidlength);
            printf(" Build-id           : ");
            {
                const unsigned char *cur = 0;
                const unsigned char *end = 0;

                cur = buildidbyteptr;
                end = cur + buildidlength;
                for (; cur < end; cur++) {
                    printf("%02x", (unsigned char)*cur);
                }
            }
            printf("\n");
        }
    }
    if (paths_array_length) {
        unsigned i = 0;

        printf(" Possible "
            ".gnu_debuglink/.note.gnu.build-id pathnames for\n");
        printf(" an alternate object file with more detailed "
            "DWARF\n");
        for ( ; i < paths_array_length; ++i) {
            char *path = paths_array[i];
            char           outpath[2000];
            unsigned long  outpathlen = sizeof(outpath);
            unsigned int   ftype = 0;
            unsigned int   endian = 0;
            unsigned int   offsetsize = 0;
            Dwarf_Unsigned filesize = 0;
            /*  See DW_PATHSOURCE_dsym for what
                pathsource might be set to in the
                call below. */
            unsigned char  pathsource = 0;
            int            errcode  = 0;

            printf("  [%u] %s\n",i,sanitized(path));
            res = dwarf_object_detector_path_b(path,
                outpath,outpathlen,
                0,0,
                &ftype,&endian,&offsetsize,
                &filesize, &pathsource, &errcode);
            if (res == DW_DLV_NO_ENTRY) {
                if (glflags.verbose) {
                    printf(" file above does not exist\n");
                }
                continue;
            }
            if (res == DW_DLV_ERROR) {
                printf("       access attempt of the above leads"
                    " to error %s\n",
                    dwarf_errmsg_by_number(errcode));
                continue;
            }
            switch(ftype) {
            case DW_FTYPE_ELF:
                printf("       file above is an Elf object\n");
                break;
            case DW_FTYPE_MACH_O:
                printf("       file above is a Mach-O object\n");
                break;
            case DW_FTYPE_PE:
                printf("       file above is a PE object");
                break;
            case DW_FTYPE_ARCHIVE:
                if (glflags.verbose) {
                    printf("       file above is an archive "
                        "so ignore it.\n");
                }
                continue;
            default:
                if (glflags.verbose) {
                    printf("       file above is not"
                        " any object type we recognize\n");
                }
                continue;
            }
        }
        printf("\n");
    }
    free(link_path);
    free(paths_array);
    return DW_DLV_OK;
}

/* GCC linkonce names */
char *lo_text           = ".text."; /*".gnu.linkonce.t.";*/
char *lo_debug_abbr     = ".gnu.linkonce.wa.";
char *lo_debug_aranges  = ".gnu.linkonce.wr.";
char *lo_debug_frame_1  = ".gnu.linkonce.wf.";
char *lo_debug_frame_2  = ".gnu.linkonce.wF.";
char *lo_debug_info     = ".gnu.linkonce.wi.";
char *lo_debug_line     = ".gnu.linkonce.wl.";
char *lo_debug_macinfo  = ".gnu.linkonce.wm.";
char *lo_debug_loc      = ".gnu.linkonce.wo.";
char *lo_debug_pubnames = ".gnu.linkonce.wp.";
char *lo_debug_ranges   = ".gnu.linkonce.wR.";
char *lo_debug_str      = ".gnu.linkonce.ws.";

/* SNC compiler/linker linkonce names */
char *nlo_text           = ".text.";
char *nlo_debug_abbr     = ".debug.wa.";
char *nlo_debug_aranges  = ".debug.wr.";
char *nlo_debug_frame_1  = ".debug.wf.";
char *nlo_debug_frame_2  = ".debug.wF.";
char *nlo_debug_info     = ".debug.wi.";
char *nlo_debug_line     = ".debug.wl.";
char *nlo_debug_macinfo  = ".debug.wm.";
char *nlo_debug_loc      = ".debug.wo.";
char *nlo_debug_pubnames = ".debug.wp.";
char *nlo_debug_ranges   = ".debug.wR.";
char *nlo_debug_str      = ".debug.ws.";

/* Build linkonce section information */
void
build_linkonce_info(Dwarf_Debug dbg)
{
    Dwarf_Unsigned nCount = 0;
    Dwarf_Unsigned section_index = 0;
    int res = 0;

    static char **linkonce_names[] = {
        &lo_text,            /* .text */
        &nlo_text,           /* .text */
        &lo_debug_abbr,      /* .debug_abbr */
        &nlo_debug_abbr,     /* .debug_abbr */
        &lo_debug_aranges,   /* .debug_aranges */
        &nlo_debug_aranges,  /* .debug_aranges */
        &lo_debug_frame_1,   /* .debug_frame */
        &nlo_debug_frame_1,  /* .debug_frame */
        &lo_debug_frame_2,   /* .debug_frame */
        &nlo_debug_frame_2,  /* .debug_frame */
        &lo_debug_info,      /* .debug_info */
        &nlo_debug_info,     /* .debug_info */
        &lo_debug_line,      /* .debug_line */
        &nlo_debug_line,     /* .debug_line */
        &lo_debug_macinfo,   /* .debug_macinfo */
        &nlo_debug_macinfo,  /* .debug_macinfo */
        &lo_debug_loc,       /* .debug_loc */
        &nlo_debug_loc,      /* .debug_loc */
        &lo_debug_pubnames,  /* .debug_pubnames */
        &nlo_debug_pubnames, /* .debug_pubnames */
        &lo_debug_ranges,    /* .debug_ranges */
        &nlo_debug_ranges,   /* .debug_ranges */
        &lo_debug_str,       /* .debug_str */
        &nlo_debug_str,      /* .debug_str */
        NULL
    };

    const char *section_name = NULL;
    Dwarf_Addr section_addr = 0;
    Dwarf_Unsigned section_size = 0;
    Dwarf_Error error = 0;
    int nIndex = 0;

    nCount = dwarf_get_section_count(dbg);

    /*  FIXME: dwarf_get_section_info_by_index_a() only
        works for section indices
        as int. It works acceptably, but will fail with
        more than 32000 sections
        (a very large number) with 32bit Windows. */
    /* Ignore section with index=0 */
    for (section_index = 1;
        section_index < nCount;
        ++section_index) {
        res = dwarf_get_section_info_by_index_a(dbg,
            (int)(unsigned int)section_index,
            &section_name,
            &section_addr,
            &section_size,0,0,
            &error);
        if (res == DW_DLV_OK) {
            for (nIndex = 0; linkonce_names[nIndex]; ++nIndex) {
                if (section_name == strstr(section_name,
                    *linkonce_names[nIndex])) {

                    /* Insert only linkonce sections */
                    AddEntryIntoBucketGroup(glflags.pLinkonceInfo,
                        section_index,
                        section_addr,section_addr,
                        section_addr + section_size,
                        section_name,
                        TRUE);
                    break;
                }
            }
        } else if (res == DW_DLV_ERROR) {
            dwarf_dealloc_error(dbg,error);
            error = 0;
        }
    }
    if ( glflags.nTrace[KIND_LINKONCE_INFO]) {
        /* see --trace=2 option */
        /*  Unlikely this is ever useful...at present. */
        PrintBucketGroup("SN linkonce setup done dd A",
            glflags.pLinkonceInfo);
    }
}

/* Check for specific TAGs and initialize some
    information used by '-k' options */
void
tag_specific_globals_setup(Dwarf_Debug dbg,
    Dwarf_Half val,int die_indent_level)
{
    switch (val) {
    /*  DW_TAG_type unit will not have addresses */
    /*  DW_TAG_skeleton unit will have addresses, but
        likely no children. But they are useful
        as marking glflags.seen_CU TRUE is useful */
    case DW_TAG_partial_unit:
    case DW_TAG_compile_unit:
    case DW_TAG_type_unit:
    case DW_TAG_skeleton_unit:
        /* To help getting the compile unit name */
        glflags.seen_CU = TRUE;
        /*  If we are checking line information, build
            the table containing the pairs LowPC and HighPC */
        if (glflags.gf_check_decl_file ||
            glflags.gf_check_ranges ||
            glflags.gf_check_locations) {
            Dwarf_Debug td = 0;

            if (!dbg) {
                ResetBucketGroup(glflags.pRangesInfo);
            } else {
                /*  Only returns DW_DLV_OK */
                dwarf_get_tied_dbg(dbg,&td,0);
                /*  With a tied-dbg we do not have
                    detailed address ranges, so
                    do not reset the single .text-size
                    bucket group */
                if (!td) {
                    ResetBucketGroup(glflags.pRangesInfo);
                }
            }
        }
        /*  The following flag indicate that only
            low_pc and high_pc
            values found in DW_TAG_subprograms
            are going to be considered when
            building the address table used to check
            ranges, lines, etc */
        glflags.need_PU_valid_code = TRUE;
        break;

    case DW_TAG_subprogram:
        /* Keep track of a PU */
        if (die_indent_level == 1) {
            /*  A DW_TAG_subprogram can be nested,
                when is used to
                declare a member function for a
                local class; process the DIE
                only if we are at level zero in the DIEs tree */
            glflags.seen_PU = TRUE;
            glflags.seen_PU_base_address = FALSE;
            glflags.seen_PU_high_address = FALSE;
            glflags.PU_name[0] = 0;
            glflags.need_PU_valid_code = TRUE;
        }
        break;
    default: break;
    }
}

/*  Print CU basic information but
    use the local DIE for the offsets. */
void
PRINT_CU_INFO(void)
{
    Dwarf_Unsigned loff = glflags.DIE_offset;
    Dwarf_Unsigned goff = glflags.DIE_section_offset;
    char           hbufarea[50];
    char           lbufarea[50];
    struct esb_s   hbuf;
    struct esb_s   lbuf;

    if (glflags.current_section_id == DEBUG_LINE ||
        glflags.current_section_id == DEBUG_FRAME ||
        glflags.current_section_id == DEBUG_FRAME_EH_GNU ||
        glflags.current_section_id == DEBUG_ARANGES ||
        glflags.current_section_id == DEBUG_MACRO ||
        glflags.current_section_id == DEBUG_PUBNAMES ||
        glflags.current_section_id == DEBUG_MACINFO ) {
        /*  These sections involve the CU die, so
            use the CU offsets.
            The DEBUG_MAC* cases are logical but
            not yet useful (Dec 2015).
            In other cases the local DIE offset makes
            more sense. */
        loff = glflags.DIE_CU_offset;
        goff = glflags.DIE_CU_overall_offset;
    }
    if (!cu_data_is_set()) {

        return;
    }
    esb_constructor_fixed(&hbuf,hbufarea,sizeof(hbufarea));
    esb_constructor_fixed(&lbuf,lbufarea,sizeof(lbufarea));
    printf("\n");
    printf("CU Name = %s\n",sanitized(glflags.CU_name));
    printf("CU Producer = %s\n",sanitized(glflags.CU_producer));
    printf("DIE OFF = 0x%" DW_PR_XZEROS DW_PR_DUx
        " GOFF = 0x%" DW_PR_XZEROS DW_PR_DUx,
        loff,goff);
    /* We used to print leftover and incorrect values at times. */
    if (glflags.need_CU_high_address) {
        esb_append(&hbuf,"unknown   ");
    } else {
        /* safe, hbuf is large enough. */
        esb_append_printf_u(&hbuf,"0x%"  DW_PR_XZEROS DW_PR_DUx,
            glflags.CU_high_address);
    }
    if (glflags.need_CU_base_address) {
        esb_append(&lbuf,"unknown   ");
    } else {
        /* safe, lbuf is large enough. */
        esb_append_printf_u(&lbuf,"0x%"  DW_PR_XZEROS DW_PR_DUx,
            glflags.CU_low_address);
    }
    printf(", Low PC = %s, High PC = %s", esb_get_string(&lbuf),
        esb_get_string(&hbuf));
    printf("\n");
    esb_destructor(&hbuf);
    esb_destructor(&lbuf);
}

void
DWARF_CHECK_ERROR_PRINT_CU(void)
{
    if (glflags.gf_check_verbose_mode) {
        if (glflags.gf_print_unique_errors) {
            if (!glflags.gf_found_error_message) {
                PRINT_CU_INFO();
            }
        } else {
            PRINT_CU_INFO();
        }
    }
    glflags.check_error++;
    glflags.gf_record_dwarf_error = TRUE;
}

/*  Sometimes is useful, just to know the kind of errors
    in an object file; not much interest in the number
    of errors; the specific case is just to have a general
    idea about the DWARF quality in the file */

char ** set_unique_errors = NULL;
unsigned int set_unique_errors_entries = 0;
unsigned int set_unique_errors_size = 0;
#define SET_UNIQUE_ERRORS_DELTA 64

/*  Create the space to store the unique error messages */
void
allocate_unique_errors_table(void)
{
    if (!set_unique_errors) {
        set_unique_errors = (char **)
            malloc(SET_UNIQUE_ERRORS_DELTA * sizeof(char*));
        set_unique_errors_size = SET_UNIQUE_ERRORS_DELTA;
        set_unique_errors_entries = 0;
    }
}

#ifdef TESTING
/* Just for debugging purposes, dump the unique errors table */
void
dump_unique_errors_table(void)
{
    unsigned int index;
    printf("*** Unique Errors Table ***\n");
    printf("Delta  : %d\n",SET_UNIQUE_ERRORS_DELTA);
    printf("Size   : %d\n",set_unique_errors_size);
    printf("Entries: %d\n",set_unique_errors_entries);
    for (index = 0; index < set_unique_errors_entries; ++index) {
        printf("%3d: '%s'\n",index,set_unique_errors[index]);
    }
}
#endif

/*  Release the space used to store the unique error messages */
void
release_unique_errors_table(void)
{
    free(set_unique_errors);
    set_unique_errors = 0;
    set_unique_errors_entries = 0;
    set_unique_errors_size = 0;
}

/*  Returns TRUE if the text is already in the set; otherwise FALSE */
Dwarf_Bool
add_to_unique_errors_table(char * error_text)
{
    unsigned int index;
    size_t len;
    char * stored_text;
    char * filtered_text;
    char * start = NULL;
    char * end = NULL;
    char * pattern = "0x";
    char * white = " ";
    char * question = "?";

    /* Create a copy of the incoming text */
    filtered_text = makename(error_text);
    if (!filtered_text) {
        /* Lets not do anything. */
        return TRUE;
    }
    len = strlen(filtered_text);

    /*  Remove from the error_text, any hexadecimal
        numbers (start with 0x),
        because for some errors, an additional
        information is given in the
        form of addresses; we are interested just in
        the general error. */
    start = strstr(filtered_text,pattern);
    while (start) {
        /* We have found the start of the pattern; look for a space */
        end = strstr(start,white);
        if (!end) {
            /* Preserve any line terminator */
            end = filtered_text + len -1;
        }
        memset(start,*question,end - start);
        start = strstr(filtered_text,pattern);
    }

    /* Check if the error text is already in the table */
    for (index = 0; index < set_unique_errors_entries; ++index) {
        stored_text = set_unique_errors[index];
        if (!strcmp(stored_text,filtered_text)) {
            return TRUE;
        }
    }

    /*  Store the new text; check if we have space
        to store the error text */
    if (set_unique_errors_entries + 1 == set_unique_errors_size) {
        char ** newsun = 0;
        unsigned int newsun_size = set_unique_errors_size +
            SET_UNIQUE_ERRORS_DELTA;

        newsun = (char **)realloc(set_unique_errors,
            newsun_size* sizeof(char*));
        if (!newsun) {
            static int ercount = 0;
            if (!ercount) {
                printf("\nERROR: Out of space recording unique"
                    " errors. Attempting to continue. "
                    " Message will not repeat.\n");
                glflags.gf_count_major_errors++;
            }
            ++ercount;
            return FALSE;
        }
        set_unique_errors = newsun;
        set_unique_errors_size = newsun_size;
    }
    set_unique_errors[set_unique_errors_entries] = filtered_text;
    ++set_unique_errors_entries;
    return FALSE;
}

/*
    Print a DWARF error message and if in "reduced" output
    only print one error of each kind; this feature is useful,
    when we are interested only in the kind of errors and
    not on the number of errors.

    PRECONDITION: if s3 non-null so are s1,s2.
        If  s2 is non-null so is s1.
        s1 is always non-null. */

static void
print_dwarf_check_error(const char *s1,
    const char *s2,
    const char *s3)
{
    static Dwarf_Bool do_init = TRUE;
    Dwarf_Bool found = FALSE;
    char * error_text = NULL;
    static char *leader =  "\n*** DWARF CHECK: ";
    static char *trailer = " ***\n";

    if (do_init) {
        esb_constructor(&dwarf_error_line);
        do_init = FALSE;
    }
    esb_empty_string(&dwarf_error_line);
    esb_append(&dwarf_error_line,leader);
    if (s3) {
        esb_append(&dwarf_error_line,s1);
        esb_append(&dwarf_error_line," -> ");
        esb_append(&dwarf_error_line,s2);
        esb_append(&dwarf_error_line,": ");
        esb_append(&dwarf_error_line,s3);
    } else if (s2) {
        esb_append(&dwarf_error_line,s1);
        esb_append(&dwarf_error_line,": ");
        esb_append(&dwarf_error_line,s2);
    } else {
        esb_append(&dwarf_error_line,s1);
    }
    esb_append(&dwarf_error_line,trailer);

    error_text = esb_get_string(&dwarf_error_line);
    if (glflags.gf_print_unique_errors) {
        found = add_to_unique_errors_table(error_text);
        if (!found) {
            printf("%s",error_text);
        }
    } else {
        printf("%s",error_text);
    }

    /*  To indicate if the current error message has
        been found or not */
    glflags.gf_found_error_message = found;
}

void
DWARF_CHECK_ERROR3(Dwarf_Check_Categories category,
    const char *str1, const char *str2, const char *strexpl)
{
    if (checking_this_compiler()) {
        DWARF_ERROR_COUNT(category,1);
        if (glflags.gf_check_verbose_mode) {
            print_dwarf_check_error(str1, str2,strexpl);
        }
        /* does glflags.check_error++; */
        /* sets glflags.gf_record_dwarf_error = TRUE; */
        DWARF_CHECK_ERROR_PRINT_CU();
    }
}

/*  This is too much to put in the DROP_ERROR_INSTANCE macro,
    so we put it here rather arbitrarily.  */
void
report_caller_error_drop_error(int dwdlv,
    int line, char *fname)
{
    printf("\nERROR in dwarfdump:"
        " The value passed to the macro DROP_ERROR_INSTANCE "
        "is not one of the three allowed values, but is "
        "%d. dwarfdump has a bug. "
        " See line %d file %s\n",dwdlv,line,fname);
    glflags.gf_count_major_errors++;

}
void
dd_minimal_count_global_error(void)
{
    glflags.gf_count_major_errors++;
}

static const char *ft[] = {
"DW_FTYPE_UNKNOWN",
"DW_FTYPE_ELF",
"DW_FTYPE_MACH_O",
"DW_FTYPE_PE",
"DW_FTYPE_ARCHIVE",
"DW_FTYPE_APPLEUNIVERSAL"
};

static
const char * get_ftype_name(Dwarf_Small ftype)
{
    if (ftype >DW_FTYPE_APPLEUNIVERSAL) {
        dd_minimal_count_global_error();
        return "ERROR: Impossible ftype value";
    }
    return ft[ftype];
}

static const char *psn[] = {
"DW_PATHSOURCE_unspecified",
"DW_PATHSOURCE_basic, standard",
"DW_PATHSOURCE_dsym, MacOS",
"DW_PATHSOURCE_debuglink, GNU debuglink"
};

static
const char * get_pathsource_name(Dwarf_Small ps)
{
    if (ps >DW_PATHSOURCE_debuglink) {
        dd_minimal_count_global_error();
        return "ERROR: Impossible pathsource value";
    }
    return psn[ps];
}

static const char *
get_machine_name(Dwarf_Unsigned machine,
    Dwarf_Small ftype)
{
    switch(ftype) {
    case DW_FTYPE_ELF:
        return dd_elf_arch_name(machine);
    case DW_FTYPE_PE:
        return dd_pe_arch_name(machine);
    case DW_FTYPE_APPLEUNIVERSAL:
    case DW_FTYPE_MACH_O:
        return dd_mach_arch_name(machine);
    default:
        return "Unexpected DW_FTYPE!";
    }
}

/*  'machine' number meaning the cpu architecture */
static void
print_machine_arch(Dwarf_Debug dbg)
{
    int res = 0;
    Dwarf_Small    dw_ftype = 0;
    Dwarf_Small    dw_obj_pointersize = 0;
    Dwarf_Bool     dw_obj_is_big_endian = 0;
    Dwarf_Unsigned dw_obj_machine = 0;
    Dwarf_Unsigned dw_obj_flags = 0;
    Dwarf_Small    dw_path_source = 0;
    Dwarf_Unsigned dw_ub_offset = 0;
    Dwarf_Unsigned dw_ub_count = 0;
    Dwarf_Unsigned dw_ub_index = 0;
    Dwarf_Unsigned dw_comdat_groupnumber = 0;
    res = dwarf_machine_architecture(dbg,
        &dw_ftype,
        &dw_obj_pointersize,
        &dw_obj_is_big_endian,
        &dw_obj_machine,
        &dw_obj_flags,
        &dw_path_source,
        &dw_ub_offset,
        &dw_ub_count,
        &dw_ub_index,
        &dw_comdat_groupnumber);
    if (res != DW_DLV_OK) {
        printf("ERROR: Impossible error calling "
            "dwarf_machine_architecture!\n");
        dd_minimal_count_global_error();
        return;
    }
    printf("Machine Architecture, Object Information fields\n");
    printf("  Filetype              : %d  (%s)\n",dw_ftype,
        get_ftype_name(dw_ftype));
    printf("  Pointersize           : %u\n",dw_obj_pointersize);
    printf("  endian                : %s\n", dw_obj_is_big_endian?
        "big endian":"little endian");
    printf("  machine/architecture  : %" DW_PR_DUu " (0x%" DW_PR_DUx
        ") <%s>\n",dw_obj_machine,dw_obj_machine,
            get_machine_name(dw_obj_machine, dw_ftype));
    printf("  machine flags         : 0x%" DW_PR_DUx "\n",
        dw_obj_flags);
    printf("  path source           : %u  (%s)\n",dw_path_source,
        get_pathsource_name(dw_path_source));
    if (glflags.verbose || dw_ftype == DW_FTYPE_APPLEUNIVERSAL) {
    printf("  MacOS Universal Binary \n");
    printf("    Object Offset in UB : Ox%" DW_PR_DUx "\n",
        dw_ub_offset);
    printf("    UB object count     : %" DW_PR_DUu "\n",
        dw_ub_count);
    printf("    UB index            : %" DW_PR_DUu "\n",
        dw_ub_index);
    }
    if (glflags.verbose ||dw_ftype == DW_FTYPE_ELF) {
        printf("  Comdat group number   : %" DW_PR_DUu
            "  (%s)\n",
            dw_comdat_groupnumber,
            dw_comdat_groupnumber==1?"DW_GROUPNUMBER_BASE, standard":
            dw_comdat_groupnumber==2?
            "DW_GROUPNUMBER_DWO, .debug.dwo etc":
            "(a requested non-default group number)");
    }
    if (glflags.verbose) {
        Dwarf_Error error = 0;
        int j = 0;
        int fres = DW_DLV_OK;

        printf("  []    offset     size       flags      addr\n");
        for ( ; fres == DW_DLV_OK; ++j) {
            const char    *name = 0;
            Dwarf_Addr     addr = 0;
            Dwarf_Unsigned size = 0;
            Dwarf_Unsigned flags = 0;
            Dwarf_Unsigned offset = 0;

            fres = dwarf_get_section_info_by_index_a(dbg,j,
                &name,&addr,&size,&flags,&offset,&error);
            if (fres != DW_DLV_OK) {
                break;
            }
            printf("  [%3d]"
                " 0x%" DW_PR_XZEROS DW_PR_DUx
                " 0x%" DW_PR_XZEROS DW_PR_DUx
                " 0x%" DW_PR_XZEROS DW_PR_DUx
                " 0x%" DW_PR_XZEROS DW_PR_DUx
                " %s\n",j,offset,size,flags,addr,name);
        }
        if (fres == DW_DLV_ERROR) {
            char *errm = dwarf_errmsg(error);
            printf("ERROR: dwarf_get_secion_info_by_index_a "
                "failed %s\n",
                errm);
            dwarf_dealloc_error(dbg,error);
            glflags.gf_count_major_errors++;
            error = 0;
        }
    }
}
