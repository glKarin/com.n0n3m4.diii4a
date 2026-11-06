/*
Copyright (C) 2000-2006 Silicon Graphics, Inc.  All Rights Reserved.
Portions Copyright 2007-2010 Sun Microsystems, Inc. All rights reserved.
Portions Copyright 2009-2018 SN Systems Ltd. All rights reserved.
Portions Copyright 2008-2020 David Anderson. All rights reserved.
Portions Copyright 2015-2015 Google, Inc. All Rights Reserved

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

#include <string.h> /* strcmp() strlen() */
#include <time.h>   /* ctime() */
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
#include "dd_sanitized.h"
#include "dd_uri.h"

#include "print_sections.h"

/*
    Print line number information:
        [line] [address] <new statement>
        new basic-block
        filename
*/

#define DW_LINE_VERSION5 5

static void
print_source_intro(Dwarf_Debug dbg,Dwarf_Die cu_die)
{
    int         ores = 0;
    Dwarf_Off   off = 0;
    Dwarf_Error src_err = 0;

    ores = dwarf_dieoffset(cu_die, &off, &src_err);
    if (ores == DW_DLV_OK) {
        int lres = 0;
        const char *sec_name = 0;

        lres = dwarf_get_die_section_name_b(cu_die,
            &sec_name,&src_err);
        if (lres != DW_DLV_OK ||  !sec_name || !strlen(sec_name)) {
            sec_name = ".debug_info";
        }
        printf("Source lines (from CU-DIE at %s offset 0x%"
            DW_PR_XZEROS DW_PR_DUx "):\n",
            sec_name,
            (Dwarf_Unsigned) off);
        DROP_ERROR_INSTANCE(dbg,lres,src_err);
    } else {
        DROP_ERROR_INSTANCE(dbg,ores,src_err);
        printf("Source lines (for the CU-DIE at unknown"
            " location):\n");
    }
}

static void
print_common_line_error(const char *where, Dwarf_Error line_err)
{
    if (checking_this_compiler()) {
        struct esb_s  tmp_buff;
        char buftmp[ESB_FIXED_ALLOC_SIZE];

        esb_constructor_fixed(&tmp_buff,buftmp,sizeof(buftmp));
        esb_append_printf_s(&tmp_buff,
            "Error getting line details calling %s",
            where);
        esb_append_printf_s(&tmp_buff,
            " dwarf error is %s",
            dwarf_errmsg(line_err));
        DWARF_CHECK_ERROR(lines_result,esb_get_string(&tmp_buff));
        esb_destructor(&tmp_buff);
    }
}

static void
check_last_line_of_table(Dwarf_Addr   pc,
    Dwarf_Addr   elf_max_address,
    const char * sec_name)
{
    /*  Ignore those PU that have been stripped
        by the linker; their low_pc values are
        set to -1 (snc linker only) */
    /*  It is perfectly sensible for a compiler
        to leave a few bytes of NOP or other stuff
        after the last instruction in a subprogram,
        for cache-alignment or other purposes, so
        a mismatch here is not necessarily
        an error.  */

    if (glflags.gf_check_lines &&
        checking_this_compiler()) {
        DWARF_CHECK_COUNT(lines_result,1);
        if ((pc != glflags.PU_high_address) &&
            (glflags.PU_base_address !=
            elf_max_address)) {
            char addr_tmp[140];
            struct esb_s cm;

            esb_constructor_fixed(&cm,addr_tmp,
                sizeof(addr_tmp));
            esb_append_printf_s(&cm,
                "%s: Address",sanitized(sec_name));
            esb_append_printf_u(&cm,
                " 0x%" DW_PR_XZEROS DW_PR_DUx
                " DW_LNE_end_sequence address"
                " does not exactly match",pc);
            esb_append_printf_u(&cm,
                " high function addr: "
                " 0x%" DW_PR_XZEROS DW_PR_DUx,
                glflags.PU_high_address);
            DWARF_CHECK_ERROR(lines_result,
                esb_get_string(&cm));
            esb_destructor(&cm);
        }
    }
}

static int
process_line_table(Dwarf_Debug dbg,
    const char *sec_name,
    Dwarf_Line *linebuf,
    Dwarf_Signed linecount,
    Dwarf_Bool is_logicals_table,
    Dwarf_Bool is_actuals_table,
    Dwarf_Error *lt_err)
{
    char *padding = 0;
    Dwarf_Signed i = 0;
    Dwarf_Addr pc = 0;
    Dwarf_Unsigned lineno = 0;
    Dwarf_Unsigned logicalno = 0;
    Dwarf_Unsigned column = 0;
    Dwarf_Unsigned call_context = 0;
    char* subprog_name = 0;
    char* subprog_filename = 0;
    Dwarf_Unsigned subprog_line = 0;

    Dwarf_Bool newstatement = 0;
    Dwarf_Bool lineendsequence = 0;
    Dwarf_Bool new_basic_block = 0;
    int sres = 0;
    int ares = 0;
    int lires = 0;
    int cores = 0;
    char lastsrc_tmp[ESB_FIXED_ALLOC_SIZE];
    struct esb_s lastsrc;
    Dwarf_Addr elf_max_address = 0;
    Dwarf_Bool SkipRecord = FALSE;

    esb_constructor_fixed(&lastsrc,lastsrc_tmp,sizeof(lastsrc_tmp));
    glflags.current_section_id = DEBUG_LINE;
    /* line_flag is TRUE */
    get_address_size_and_max(dbg,0,&elf_max_address,lt_err);
    /* Padding for a nice layout */
    padding = glflags.gf_line_print_pc ? "            " : "";
    if (glflags.gf_do_print_dwarf) {
        /* Check if print of <pc> address is needed. */
        printf("\n");
        if (is_logicals_table) {
            printf("Logicals Table:\n");
            printf("%sNS new statement, PE prologue end, "
                "EB epilogue begin\n",padding);
            printf("%sDI=val discriminator value\n",
                padding);
            printf("%sCC=val context, SB=val subprogram\n",
                padding);
        } else if (is_actuals_table) {
            printf("Actuals Table:\n");
            printf("%sBB new basic block, ET end of text sequence\n"
                "%sIS=val ISA number\n",padding,padding);

        } else {
            /* Standard DWARF line table. */
            printf("%sNS new statement, BB new basic block, "
                "ET end of text sequence\n",padding);
            printf("%sPE prologue end, EB epilogue begin\n",padding);
            printf("%sIS=val ISA number, "
                "DI=val discriminator value\n",
                padding);
        }
        if (is_logicals_table || is_actuals_table) {
            printf("[ row]  ");
        }
        if (glflags.gf_line_print_pc) {
            printf("<pc>        ");
        }
        if (is_logicals_table) {
            printf("[lno,col] NS PE EB DI= CC= SB= uri:"
                " \"filepath\"\n");
        } else if (is_actuals_table) {
            printf("[logical] BB ET IS=\n");
        } else {
            printf("[lno,col] NS BB ET PE EB IS= DI= uri:"
                " \"filepath\"\n");
        }
    }
    for (i = 0; i < linecount; i++) {
        Dwarf_Line line = linebuf[i];
        char* lsrc_filename = 0;
        int nsres = 0;
        Dwarf_Bool found_line_error = FALSE;
        Dwarf_Bool has_is_addr_set = FALSE;
        char *where = NULL;

        if (glflags.gf_check_decl_file && checking_this_compiler()) {
            /* A line record with addr=0 was detected */
            if (SkipRecord) {
                /* Skip records that do not have is_addr_set */
                ares = dwarf_line_is_addr_set(line,
                    &has_is_addr_set, lt_err);
                if (ares == DW_DLV_OK && has_is_addr_set) {
                    SkipRecord = FALSE;
                }
                else {
                    /*  Keep ignoring records until we have
                        one with 'is_addr_set' */
                    continue;
                }
            }
        }

        if (glflags.gf_check_lines && checking_this_compiler()) {
            DWARF_CHECK_COUNT(lines_result,1);
        }

        /*  NO. lsrc_filename is a DW_DLA_STRING, do not assign
            a static string.
            lsrc_filename = "<unknown>";
        */
        if (!is_actuals_table) {
            Dwarf_Error aterr = 0;

            sres = dwarf_linesrc(line, &lsrc_filename, &aterr);
            if (sres == DW_DLV_ERROR) {
                /* Do not terminate processing  */
                if (glflags.gf_do_check_dwarf &&
                    checking_this_compiler()) {
                    where = "dwarf_linesrc()";
                    print_common_line_error(where,aterr);
                    found_line_error = TRUE;
                }
                DROP_ERROR_INSTANCE(dbg,sres,aterr);
            }
        }

        pc = 0;
        ares = dwarf_lineaddr(line, &pc, lt_err);

        if (ares == DW_DLV_ERROR) {
            /* Do not terminate processing */
            if (glflags.gf_do_check_dwarf &&
                checking_this_compiler()) {
                where = "dwarf_lineaddr()";
                print_common_line_error(where,*lt_err);
                found_line_error = TRUE;
            }
            pc = 0;
            DROP_ERROR_INSTANCE(dbg,ares,*lt_err);
        }
        if (ares == DW_DLV_NO_ENTRY) {
            pc = 0;
        }

        if (is_actuals_table) {
            lires = dwarf_linelogical(line, &logicalno, lt_err);
            if (lires == DW_DLV_ERROR) {
                /* Do not terminate processing */
                if (glflags.gf_do_check_dwarf &&
                    checking_this_compiler()) {
                    where = "dwarf_linelogical()";
                    print_common_line_error(where,*lt_err);
                    found_line_error = TRUE;
                }
                DROP_ERROR_INSTANCE(dbg,lires,*lt_err);
            }
            if (lires == DW_DLV_NO_ENTRY) {
                logicalno = 0;
            }
            column = 0;
        } else {
            lires = dwarf_lineno(line, &lineno, lt_err);
            if (lires == DW_DLV_ERROR) {
                /* Do not terminate processing */
                where = "dwarf_lineno()";
                print_common_line_error(where,*lt_err);
                found_line_error = TRUE;
                DROP_ERROR_INSTANCE(dbg,lires,*lt_err);
            }
            if (lires == DW_DLV_NO_ENTRY) {
                lineno = 0;
            }
            cores = dwarf_lineoff_b(line, &column, lt_err);
            if (cores == DW_DLV_ERROR) {
                /* Do not terminate processing */
                if (glflags.gf_do_check_dwarf &&
                    checking_this_compiler()) {
                    where = "dwarf_lineoff()";
                    print_common_line_error(where,*lt_err);
                    found_line_error = TRUE;
                }
                DROP_ERROR_INSTANCE(dbg,cores,*lt_err);
            }
            if (cores == DW_DLV_NO_ENTRY) {
                /*  Zero was always the correct default, meaning
                    the left edge. DWARF2/3/4 spec sec 6.2.2 */
                column = 0;
            }
        }

        /*  Process any possible error condition, though
            we won't be at the first such error. */
        if ((glflags.gf_check_decl_file ||
            glflags.gf_check_lines) &&
            checking_this_compiler()) {
            DWARF_CHECK_COUNT(decl_file_result,1);
            if (found_line_error) {
                /* DWARF_CHECK already issued. */
            } else if (glflags.gf_do_check_dwarf) {
                /*  Check the address lies with a
                    valid [lowPC:highPC]
                    in the .text section*/
                if (IsValidInBucketGroup(glflags.pRangesInfo,pc)) {
                    /* Valid values; do nothing */
                } else {
                    /*  At this point may be we are dealing with
                        a linkonce symbol. The problem we have here
                        is we have consumed the .debug_info section
                        and we are dealing just with the records
                        from the .debug_line, so no PU_name is
                        available and no high_pc.
                        Traverse the linkonce table if try to
                        match the pc value with one of those ranges.
                    */
                    if (glflags.gf_check_lines &&
                        checking_this_compiler()) {
                        DWARF_CHECK_COUNT(lines_result,1);
                    }
                    if (FindAddressInBucketGroup(
                        glflags.pLinkonceInfo,pc)){
                        /* Valid values; do nothing */
                    } else {
                        /*  The SN Systems Linker generates
                            line records
                            with addr=0, when dealing with linkonce
                            symbols and no stripping */
                        if (pc) {
                            if (glflags.gf_check_lines &&
                                checking_this_compiler()) {
                                char abuf[50];
                                struct esb_s atm;

                                esb_constructor_fixed(&atm,
                                    abuf,sizeof(abuf));
                                esb_append_printf_s(&atm,
                                    "%s: Address",
                                    sanitized(sec_name));
                                esb_append_printf_u(&atm,
                                    " 0x%" DW_PR_XZEROS DW_PR_DUx
                                    " outside a valid .text range",
                                    pc);
                                DWARF_CHECK_ERROR(lines_result,
                                    esb_get_string(&atm));
                                esb_destructor(&atm);
                            }
                        } else {
                            SkipRecord = TRUE;
                        }
                    }
                } /* end not in bucket group */
                /*  Check the last record for the .debug_line,
                    the one created by DW_LNE_end_sequence,
                    is the same as the high_pc
                    address for the last known user program
                    unit (PU).
                    There is no real reason */
                if ((i + 1 == linecount) &&
                    glflags.seen_PU_high_address &&
                    !is_logicals_table) {
                    check_last_line_of_table(pc,
                        elf_max_address,sec_name);
                }/* end seen_PU_high_address */
            }
        }

        /* Display the error information */
        if (found_line_error || glflags.gf_record_dwarf_error) {
            if (glflags.gf_check_verbose_mode && PRINTING_UNIQUE) {
                /*  Print the record number for better
                    error description */
                printf("Record = %"  DW_PR_DUu
                    " Addr = 0x%" DW_PR_XZEROS DW_PR_DUx
                    " [%4" DW_PR_DUu ",%2" DW_PR_DUu "] '%s'\n",
                    i, pc,lineno,column,
                    lsrc_filename?sanitized(lsrc_filename):"");
                /* The compilation unit was already printed */
                if (!glflags.gf_check_decl_file) {
                    PRINT_CU_INFO();
                }
            }
            glflags.gf_record_dwarf_error = FALSE;
            /* Due to a fatal error, skip current record */
            if (found_line_error) {
                dwarf_dealloc(dbg, lsrc_filename, DW_DLA_STRING);
                lsrc_filename = 0;
                continue;
            }
        }
        if (glflags.gf_do_print_dwarf) {
            if (is_logicals_table || is_actuals_table) {
                printf("[%4" DW_PR_DUu "]  ", i + 1);
            }
            /* Check if print of <pc> address is needed. */
            if (glflags.gf_line_print_pc) {
                printf("0x%" DW_PR_XZEROS DW_PR_DUx "  ", pc);
            }
            if (is_actuals_table) {
                printf("[%7" DW_PR_DUu "]", logicalno);
            } else {
                printf("[%4" DW_PR_DUu ",%2" DW_PR_DUu "]",
                    lineno, column);
            }
        }

        if (!is_actuals_table) {
            nsres = dwarf_linebeginstatement(line,
                &newstatement, lt_err);
            if (nsres == DW_DLV_OK) {
                if (newstatement && glflags.gf_do_print_dwarf) {
                    printf(" %s","NS");
                }
            } else if (nsres == DW_DLV_ERROR) {
                struct esb_s m;
                esb_constructor(&m);
                esb_append_printf_u(&m,
                    "\nERROR: dwarf_linebeginstatement failed"
                    " on linebuf index %u ",i);
                esb_append_printf_u(&m,
                    "of %u line records in the linebuf.",
                    linecount);
                simple_err_return_action(nsres,
                    esb_get_string(&m));
                esb_destructor(&m);
                dwarf_dealloc(dbg, lsrc_filename, DW_DLA_STRING);
                lsrc_filename = 0;
                return nsres;
            }
        }

        if (!is_logicals_table) {
            nsres = dwarf_lineblock(line,
                &new_basic_block, lt_err);
            if (nsres == DW_DLV_OK) {
                if (new_basic_block && glflags.gf_do_print_dwarf) {
                    printf(" %s","BB");
                }
            } else if (nsres == DW_DLV_ERROR) {
                struct esb_s m;
                esb_constructor(&m);
                esb_append_printf_u(&m,
                    "\nERROR: dwarf_lineblock failed"
                    " on linebuf index %u ",i);
                esb_append_printf_u(&m,
                    "of %u line records in the linebuf.",
                    linecount);
                simple_err_return_action(nsres,
                    esb_get_string(&m));
                esb_destructor(&m);
                dwarf_dealloc(dbg, lsrc_filename, DW_DLA_STRING);
                lsrc_filename = 0;
                return nsres;
            }
            nsres = dwarf_lineendsequence(line,
                &lineendsequence, lt_err);
            if (nsres == DW_DLV_OK) {
                if (lineendsequence &&
                    glflags.gf_do_print_dwarf) {
                    printf(" %s", "ET");
                }
            } else if (nsres == DW_DLV_ERROR) {
                struct esb_s m;
                esb_constructor(&m);
                esb_append_printf_u(&m,
                    "\nERROR: dwarf_lineendsequence failed"
                    " on linebuf index %u ",i);
                esb_append_printf_u(&m,
                    "of %u line records in the linebuf.",
                    linecount);
                simple_err_return_action(nsres,
                    esb_get_string(&m));
                esb_destructor(&m);
                dwarf_dealloc(dbg, lsrc_filename, DW_DLA_STRING);
                lsrc_filename = 0;
                return nsres;
            }
        }

        if (glflags.gf_do_print_dwarf) {
            Dwarf_Bool prologue_end = 0;
            Dwarf_Bool epilogue_begin = 0;
            Dwarf_Unsigned isa = 0;
            Dwarf_Unsigned discriminator = 0;
            int disres = dwarf_prologue_end_etc(line,
                &prologue_end,&epilogue_begin,
                &isa,&discriminator,lt_err);
            if (disres == DW_DLV_ERROR) {
                struct esb_s m;
                esb_constructor(&m);
                esb_append_printf_u(&m,
                    "\nERROR: dwarf_prologue_end_etc() failed"
                    " on linebuf index %u ",i);
                esb_append_printf_u(&m,
                    "of %u line records in the linebuf.",
                    linecount);
                simple_err_return_action(nsres,
                    esb_get_string(&m));
                esb_destructor(&m);
                dwarf_dealloc(dbg, lsrc_filename, DW_DLA_STRING);
                lsrc_filename = 0;
                return disres;
            }
            if (prologue_end && !is_actuals_table) {
                printf(" PE");
            }
            if (epilogue_begin && !is_actuals_table) {
                printf(" EB");
            }
            if (isa && !is_logicals_table) {
                printf(" IS=0x%" DW_PR_DUx, isa);
            }
            if (discriminator && !is_actuals_table) {
                printf(" DI=0x%" DW_PR_DUx, discriminator);
            }
            if (is_logicals_table) {
                call_context = 0;
                disres = dwarf_linecontext(line,
                    &call_context, lt_err);
                if (disres == DW_DLV_ERROR) {
                    struct esb_s m;
                    esb_constructor(&m);
                    esb_append_printf_u(&m,
                        "\nERROR: dwarf_linecontext() failed"
                        " on linebuf index %u ",i);
                    esb_append_printf_u(&m,
                        "of %u line records in the linebuf.",
                        linecount);
                        simple_err_return_action(nsres,
                        esb_get_string(&m));
                    esb_destructor(&m);
                    dwarf_dealloc(dbg, lsrc_filename, DW_DLA_STRING);
                    lsrc_filename = 0;
                    return disres;
                }
                if (call_context) {
                    printf(" CC=%" DW_PR_DUu, call_context);
                }
                {
                    Dwarf_Unsigned subprogno = 0;
                    disres = dwarf_line_subprogno(line,
                        &subprogno,0);
                    if (disres == DW_DLV_OK) {
                        /*  The validity of subprogno
                            is actually used&checked in
                            dwarf_line_subprog()
                            (called next)
                            so we need not check it here.  */
                    } else {
                        printf("ERROR: dwarf_line_subprogno()"
                            " impossibly! "
                            "fails. with result %d\n",disres);
                        glflags.gf_count_major_errors++;
                    }
                }
                subprog_name = 0;
                disres = dwarf_line_subprog(line,
                    &subprog_name,
                    &subprog_filename,
                    &subprog_line, lt_err);
                if (disres == DW_DLV_ERROR) {
                    struct esb_s m;
                    esb_constructor(&m);
                    esb_append_printf_u(&m,
                        "\nERROR: dwarf_line_subprog() failed"
                        " on linebuf index %u ",i);
                    esb_append_printf_u(&m,
                        "of %u line records in the linebuf.",
                        linecount);
                    simple_err_return_action(nsres,
                        esb_get_string(&m));
                    esb_destructor(&m);
                    dwarf_dealloc(dbg, lsrc_filename, DW_DLA_STRING);
                    lsrc_filename = 0;
                    return disres;
                }
                if (subprog_name && strlen(subprog_name)) {
                    /*  We do not print an empty name.
                        Clutters things up. */
                    printf(" SB=\"%s\"", sanitized(subprog_name));
                }
                dwarf_dealloc(dbg,subprog_filename, DW_DLA_STRING);
                subprog_filename = 0;
            }
        }

        if (!is_actuals_table) {
            if (i == 0  ||  glflags.verbose > 2 ||
                strcmp(lsrc_filename?lsrc_filename:"",
                    esb_get_string(&lastsrc))) {
                struct esb_s urs;
                char atmp2[ESB_FIXED_ALLOC_SIZE];

                esb_constructor_fixed(&urs,atmp2,sizeof(atmp2));
                esb_append(&urs, " uri: \"");
                translate_to_uri(lsrc_filename?
                    lsrc_filename:"",
                    &urs);
                esb_append(&urs,"\"");
                if (glflags.gf_do_print_dwarf) {
                    printf("%s",esb_get_string(&urs));
                }
                esb_destructor(&urs);
                esb_empty_string(&lastsrc);
                esb_append(&lastsrc,
                    lsrc_filename?lsrc_filename:"");
            } else {
                /*  Do not print name, it is the same
                    as the last name printed. */
            }
        }
        if (glflags.gf_do_print_dwarf) {
            printf("\n");
        }
        dwarf_dealloc(dbg,lsrc_filename, DW_DLA_STRING);
        lsrc_filename = 0;
    }
    esb_destructor(&lastsrc);
    return DW_DLV_OK;
}

/* Here we test the interfaces into Dwarf_Line_Context. */
static int
print_line_context_record(Dwarf_Line_Context line_context,
    Dwarf_Error *err)
{
    int vres = 0;
    Dwarf_Unsigned lsecoff = 0;
    Dwarf_Unsigned version = 0;
    Dwarf_Signed dir_count = 0;
    Dwarf_Signed baseindex = 0;
    Dwarf_Signed file_count = 0;
    Dwarf_Signed endindex = 0;
    Dwarf_Signed i = 0;
    Dwarf_Signed subprog_count = 0;
    const char *name = 0;
    Dwarf_Small table_count = 0;
    struct esb_s bufr;
    Dwarf_Signed include_dir_base = 1; /* DWARF2.3,4 */
    Dwarf_Signed include_dir_limit = 0; /* set below */
    char bufr_tmp[ESB_FIXED_ALLOC_SIZE];

    esb_constructor_fixed(&bufr,bufr_tmp,sizeof(bufr_tmp));
    printf("Line Context data\n");
    vres = dwarf_srclines_table_offset(line_context,&lsecoff,
        err);
    if (vres != DW_DLV_OK) {
        simple_err_return_action(vres,
            "\nERROR: dwarf_srclines_table_offset() failed"
            ". Something broken");
        return vres;
    }
    printf(" Line Section Offset 0x%"
        DW_PR_XZEROS DW_PR_DUx "\n", lsecoff);
    vres = dwarf_srclines_version(line_context,&version,
        &table_count, err);
    if (vres != DW_DLV_OK) {
        simple_err_return_action(vres,
            "\nERROR: dwarf_srclines_version() failed"
            ". Something broken");
        return vres;
    }
    printf(" version number      0x%" DW_PR_DUx
        " %" DW_PR_DUu "\n",
        version,version);
    printf(" number of line tables  %d.\n", table_count);
    vres = dwarf_srclines_comp_dir(line_context,&name,err);
    if (vres != DW_DLV_OK) {
        simple_err_return_action(vres,
            "\nERROR: dwarf_srclines_comp_dir() failed"
            ". Something broken");
        return vres;
    }
    if (name) {
        printf(" Compilation directory: %s\n",name);
    } else {
        printf(" Compilation directory: <unknown"
            " no DW_AT_comp_dir>\n");
    }

    vres = dwarf_srclines_include_dir_count(line_context,
        &dir_count,err);
    if (vres != DW_DLV_OK) {
        simple_err_return_action(vres,
            "\nERROR: dwarf_srclines_comp_dir() failed"
            ". Something broken");
        return vres;
    }
    printf(" include directory count 0x%"
        DW_PR_DUx " %" DW_PR_DSd "\n",
        (Dwarf_Unsigned)dir_count,dir_count);
    if (version == DW_LINE_VERSION5) {
        include_dir_base = 0;
        include_dir_limit = dir_count;
    } else {
        include_dir_base = 1;
        include_dir_limit = dir_count+1;
    }
    for (i = include_dir_base; i < include_dir_limit; ++i) {
        vres = dwarf_srclines_include_dir_data(line_context,i,
            &name,err);
        if (vres != DW_DLV_OK) {
            struct esb_s m;

            esb_constructor(&m);
            esb_append_printf_i(&m,
                "\nERROR: Error accessing include directory "
                "  %d ",i);
            esb_append_printf_i(&m,
                "(max allowed index is %d).",dir_count);
            simple_err_return_action(vres,
                esb_get_string(&m));
            esb_destructor(&m);
            return vres;
        }
        printf("  [%2" DW_PR_DSd "]  \"%s\"\n",i,name);
    }

    vres = dwarf_srclines_files_indexes(line_context,
        &baseindex,&file_count,&endindex,err);
    if (vres != DW_DLV_OK) {
        simple_err_return_action(vres,
            "\nERROR: Error accessing files indexes");
        return vres;
    }
    printf( " files count 0x%"
        DW_PR_DUx " %" DW_PR_DUu "\n",
        file_count,file_count);
    /*  Set up so just one loop control needed
        for all versions of line tables. */
    for (i = baseindex; i < endindex; ++i) {
        Dwarf_Unsigned dirindex = 0;
        Dwarf_Unsigned modtime = 0;
        Dwarf_Unsigned flength = 0;
        Dwarf_Form_Data16 *md5data = 0;

        vres = dwarf_srclines_files_data_b(line_context,i,
            &name,&dirindex, &modtime,&flength,
            &md5data,err);
        if (vres != DW_DLV_OK) {
            struct esb_s m;

            esb_constructor(&m);
            esb_append_printf_i(&m,
                "\nERROR: Error accessing line_context "
                " calling  dwarf_srclines_files_data_b() "
                "with index %d ",i);
            esb_append_printf_i(&m,
                "(end index is %d).",endindex);
            simple_err_return_action(vres,
                esb_get_string(&m));
            esb_destructor(&m);
            return vres;
        }
        esb_empty_string(&bufr);
        if (name) {
            esb_empty_string(&bufr);
            esb_append(&bufr,"\"");
            esb_append(&bufr,name);
            esb_append(&bufr,"\"");
        } else {
            esb_append(&bufr,"<ERROR:NULL name in files list>");
        }
        printf("  [%2" DW_PR_DSd "]  %-24s ,",
            i,esb_get_string(&bufr));
        printf(" directory index  %2" DW_PR_DUu ,dirindex);
        printf(",  file length %2" DW_PR_DUu ,flength);
        if (md5data) {
            char *c = (char *)md5data;
            char *end = c+sizeof(*md5data);
            printf(", file md5 value 0x");
            while(c < end) {
                printf("%02x",0xff&*c);
                ++c;
            }
            printf(" ");
        }
        if (modtime) {
            time_t tt3 = (time_t)modtime;

            /* ctime supplies newline */
            printf( "file mod time 0x%lx %s", (unsigned long)tt3,
                ctime(&tt3));
        } else {
            printf("  file mod time 0\n");
        }
    }
    esb_destructor(&bufr);

    vres = dwarf_srclines_subprog_count(line_context,&subprog_count,
        err);
    if (vres != DW_DLV_OK) {
        simple_err_return_msg_either_action(vres,
            "ERROR: dwarf_srclines_subprog_count() on a "
            "line_context fails");
        return vres;
    }
    if (subprog_count == 0) {
        return DW_DLV_OK;
    }
    /*  The following is for the experimental table
        which is only DWARF4 so far, so no need for
        a dwarf_srclines_subprog_indexes() function. Yet. */
    printf(" subprograms count (experimental) 0x%"
        DW_PR_DUx " %" DW_PR_DUu "\n",
        subprog_count,subprog_count);
    for (i = 1; i <= subprog_count; ++i) {
        Dwarf_Unsigned decl_file = 0;
        Dwarf_Unsigned decl_line = 0;
        vres = dwarf_srclines_subprog_data(line_context,i,
            &name,&decl_file, &decl_line,err);
        if (vres != DW_DLV_OK) {
            struct esb_s m;

            esb_constructor(&m);
            esb_append_printf_i(&m,
                "\nERROR: Error accessing line_context "
                " calling  dwarf_srclines_subprog_data() "
                "with index %d ",i);
            esb_append_printf_i(&m,
                "(end index is %d).",subprog_count);
            simple_err_return_action(vres,
                esb_get_string(&m));
            esb_destructor(&m);
            return vres;
        }
        printf("  [%2" DW_PR_DSd "]  \"%s\""
            ", fileindex %2" DW_PR_DUu
            ", lineindex  %2" DW_PR_DUu
            "\n",
            i,name,decl_file,decl_line);
    }
    return DW_DLV_OK;
}

int
print_line_numbers_this_cu(Dwarf_Debug dbg, Dwarf_Die cu_die,
    char **srcfiles,
    Dwarf_Signed srcf_count,
    Dwarf_Error *err)
{
    Dwarf_Unsigned lineversion = 0;
    Dwarf_Signed linecount = 0;
    Dwarf_Line *linebuf = NULL;
    Dwarf_Signed linecount_actuals = 0;
    Dwarf_Line *linebuf_actuals = NULL;
    Dwarf_Small  table_count = 0;
    int lres = 0;
    int line_errs = 0;
    Dwarf_Line_Context line_context = 0;
    const char *sec_name = 0;
    Dwarf_Off cudie_local_offset = 0;
    Dwarf_Off dieprint_cu_goffset = 0;
    int atres = 0;

    glflags.current_section_id = DEBUG_LINE;

    /* line_flag is TRUE */

    lres = dwarf_get_line_section_name_from_die(cu_die,
        &sec_name,err);
    if (lres != DW_DLV_OK || !sec_name || !strlen(sec_name)) {
        sec_name = ".debug_line";
    }
    DROP_ERROR_INSTANCE(dbg,lres,*err);

    /* The offsets will be zero if it fails. Let it pass. */
    atres = dwarf_die_offsets(cu_die,&dieprint_cu_goffset,
        &cudie_local_offset,err);
    DROP_ERROR_INSTANCE(dbg,atres,*err);

    if (glflags.gf_do_print_dwarf) {
        struct esb_s truename;
        char buf[ESB_FIXED_ALLOC_SIZE];

        esb_constructor_fixed(&truename,buf,sizeof(buf));
        get_true_section_name(dbg,".debug_line",
            &truename,FALSE); /* Ignore the COMPRESSED flags */
        printf("\n%s: line number info for a single cu\n",
            sanitized(esb_get_string(&truename)));
        esb_destructor(&truename);
    } else {
        /* We are checking, not printing. */
        Dwarf_Half tag = 0;
        int tres = dwarf_tag(cu_die, &tag, err);
        if (tres != DW_DLV_OK) {
            /*  Something broken here. */
            struct esb_s m;
            esb_constructor(&m);
            esb_append(&m,
                "\nERROR: Unable to get CU DIE dwarf tag "
                "attempting to print line numbers for a CU ");
            if (tres == DW_DLV_ERROR) {
                esb_append(&m,dwarf_errmsg(*err));
            }
            simple_err_return_msg_either_action(tres,
                esb_get_string(&m));
            esb_destructor(&m);
            return tres;
        } else if (tag == DW_TAG_type_unit) {
            /*  Not checking since type units missing
                address or range in CU header. */
            return DW_DLV_NO_ENTRY;
        }
    }

    if (glflags.verbose > 1) {
        int errcount = 0;
        Dwarf_Bool attr_dup = FALSE;
        int lresv = 0;
        print_source_intro(dbg,cu_die);
        lresv = print_one_die(dbg, cu_die,
            dieprint_cu_goffset,
            /* print_information= */ 1,
            /* indent level */0,
            srcfiles,srcf_count,
            &attr_dup,
            /* ignore_die_stack= */TRUE,
            err);
        if (lresv == DW_DLV_ERROR) {
            return lresv;
        }
        DWARF_CHECK_COUNT(lines_result,1);
        lresv = dwarf_print_lines(cu_die, err,&errcount);
        if (errcount > 0) {
            DWARF_ERROR_COUNT(lines_result,errcount);
            DWARF_CHECK_COUNT(lines_result,(errcount-1));
        }
        if (lresv == DW_DLV_ERROR) {
            print_error_and_continue("Failed to print CU lines",
                lresv, *err);
        }
        return lresv;
    }

    if (glflags.gf_check_lines && checking_this_compiler()) {
        int lres2 = 0;

        DWARF_CHECK_COUNT(lines_result,1);
        lres2 = dwarf_check_lineheader_b(cu_die,&line_errs,
            err);
        if (lres2 == DW_DLV_ERROR) {
            print_error_and_continue(
                "dwarf_check_lineheader_b found a serious error",
                lres2, *err);
            dwarf_dealloc(dbg,*err,DW_DLA_ERROR);
            *err = 0;
        }
        if (line_errs > 0) {
            /* does glflags.check_error++; */
            /* sets glflags.gf_record_dwarf_error = TRUE; */
            DWARF_CHECK_ERROR_PRINT_CU();
            DWARF_ERROR_COUNT(lines_result,line_errs);
            DWARF_CHECK_COUNT(lines_result,(line_errs-1));
        }
    }
    /*  The following is complicated by a desire to test
        various line table interface functions.  Hence
        we test line_flag_selection.

        Normal code should pick an interface
        (for most  the best choice is what we here call
        glflags.gf_line_flag_selection ==  singledw5)
        and use just that interface set.

        Sorry about the length of the code that
        results from having so many interfaces.  */
    if (glflags.gf_line_flag_selection ==  singledw5) {
        lres = dwarf_srclines_b(cu_die,&lineversion,
            &table_count,&line_context,
            err);
        if (lres == DW_DLV_OK) {
            lres = dwarf_srclines_from_linecontext(line_context,
                &linebuf, &linecount,err);
        }
    } else {
        lres = dwarf_srclines_b(cu_die,&lineversion,
            &table_count,&line_context,
            err);
        if (lres == DW_DLV_OK) {
            lres = dwarf_srclines_two_level_from_linecontext(
                line_context,
                &linebuf, &linecount,
                &linebuf_actuals, &linecount_actuals,
                err);
        }
    }
    if (lres == DW_DLV_ERROR) {
        /* Do not terminate processing */
        if (glflags.gf_check_decl_file ||
            glflags.gf_check_lines) {
            DWARF_CHECK_COUNT(decl_file_result,1);
            DWARF_CHECK_ERROR2(decl_file_result,"dwarf_srclines",
                dwarf_errmsg(*err));
            /* Clear error condition */
            glflags.gf_record_dwarf_error = FALSE;
        } else {
            print_error_and_continue("dwarf_srclines",
                lres, *err);
        }
        DROP_ERROR_INSTANCE(dbg,lres,*err);
        return DW_DLV_OK;
    } else if (lres == DW_DLV_NO_ENTRY) {
        /* no line information is included */
    } else if (table_count > 0) {
        /* lres DW_DLV_OK */
        if (glflags.gf_do_print_dwarf) {
            if (line_context && glflags.verbose) {
                lres = print_line_context_record(line_context,err);
                if (lres != DW_DLV_OK){
                    /*  Should we issue message
                        about this call? */
                    dwarf_srclines_dealloc_b(line_context);
                    return lres;
                }
            }
            print_source_intro(dbg,cu_die);
            if (glflags.verbose) {
                int dres = 0;
                Dwarf_Bool attr_dup = FALSE;
                /* FIXME */
                dres = print_one_die(dbg, cu_die,
                    dieprint_cu_goffset,
                    /* print_information= */ TRUE,
                    /* indent_level= */ 0,
                    /* srcfiles= */ 0, /* cnt= */ 0,

                    &attr_dup,
                    /* ignore_die_stack= */TRUE,err);
                if (dres == DW_DLV_ERROR) {
                    dwarf_srclines_dealloc_b(line_context);
                    return dres;
                }
            }
        }
        if (glflags.gf_line_flag_selection ==  singledw5 ||
            glflags.gf_line_flag_selection == s2l) {
            /*  This now *all* cases possible when
                the table_count > 0. */
            int ltres = 0;

            if (table_count == 0 || table_count == 1) {
                /* ASSERT: is_single_table == true */
                Dwarf_Bool is_logicals = FALSE;
                Dwarf_Bool is_actuals = FALSE;

                ltres = process_line_table(dbg,sec_name,
                    linebuf, linecount,
                    is_logicals,is_actuals,err);
                if (ltres == DW_DLV_ERROR) {
                    /* what if NO_ENTRY? */
                    dwarf_srclines_dealloc_b(line_context);
                    return ltres;
                }
            } else {
                Dwarf_Bool is_logicals = TRUE;
                Dwarf_Bool is_actuals = FALSE;

                ltres = process_line_table(dbg,sec_name,
                    linebuf, linecount,
                    is_logicals, is_actuals,err);
                if (ltres != DW_DLV_OK) {
                    dwarf_srclines_dealloc_b(line_context);
                    return ltres;
                }
                ltres = process_line_table(dbg,sec_name,
                    linebuf_actuals,
                    linecount_actuals,
                    !is_logicals, !is_actuals,err);
                if (ltres != DW_DLV_OK) {
                    dwarf_srclines_dealloc_b(line_context);
                    return ltres;
                }
            }
            dwarf_srclines_dealloc_b(line_context);
            line_context = 0;
            linebuf = 0;
        }
        /* end, table_count > 0 */
    } else { /* table_count == 0 */
        /* lres DW_DLV_OK */
        /*  table_count == 0. no lines in table.
            Just a line table header. */
        if (glflags.gf_do_print_dwarf) {
            int ores = 0;
            Dwarf_Unsigned off = 0;

            print_source_intro(dbg,cu_die);
            if (glflags.verbose) {
                int dpres = 0;
                Dwarf_Bool attr_dup = FALSE;

                /* FIXME */
                dpres = print_one_die(dbg, cu_die,
                    dieprint_cu_goffset,
                    /* print_information= */ TRUE,
                    /* indent_level= */ 0,
                    /* srcfiles= */ 0, /* cnt= */ 0,
                    &attr_dup,
                    /* ignore_die_stack= */TRUE,
                    err);
                if (dpres == DW_DLV_ERROR) {
                    dwarf_srclines_dealloc_b(line_context);
                    return dpres;
                }
            }
            if (line_context) {
                if (glflags.verbose > 2) {
                    ores = print_line_context_record(
                        line_context,err);
                    if (ores != DW_DLV_OK) {
                        simple_err_return_msg_either_action(
                            ores,
                            "ERROR: line context record "
                            " where table count is 0 has a"
                            " problem");
                        dwarf_srclines_dealloc_b(line_context);
                        return ores;
                    }
                }
                ores = dwarf_srclines_table_offset(
                    line_context,
                    &off,err);
                if (ores != DW_DLV_OK) {
                    simple_err_return_msg_either_action(
                        ores,
                        "ERROR: line context table_offset "
                        " where table count is 0 has a"
                        " problem");
                    dwarf_srclines_dealloc_b(line_context);
                    return ores;
                } else {
                    printf(" Line table is present (offset 0x%"
                        DW_PR_XZEROS DW_PR_DUx
                        ") but no lines present\n", off);
                }
            } else {
                printf(" Line table is present but"
                    " no lines present\n");
            }
        }
        if (glflags.gf_line_flag_selection ==  singledw5 ||
            glflags.gf_line_flag_selection == s2l) {
            /*  Now this is all cases...
                also deletes the linebuf... */
            dwarf_srclines_dealloc_b(line_context);
            line_context = 0;
            linebuf = 0;
        }
        /* end, linecounttotal == 0 */
    }
    if (line_context) {
        /* also deletes the linebuf... */
        dwarf_srclines_dealloc_b(line_context);
        line_context = 0;
        linebuf = 0;
    }
    return DW_DLV_OK;
}
