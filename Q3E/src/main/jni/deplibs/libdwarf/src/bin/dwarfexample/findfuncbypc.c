/*
  Copyright (c) 2019 David Anderson.
  This source code is placed in the Public Domain.
  It may be copied or used in any way without restriction.
*/
/*  findfuncbypc.c
    This is an example of code reading dwarf .debug_info.
    It is kept simple to expose essential features.

    It does not do all possible error reporting or error handling.
    It does to a bit of error checking as a help in ensuring
    that some code works properly... for error checks.

    To use, try
        make
        ./findfuncbypc --pc=0x10000 ./findfuncbypc
*/

#include <config.h>

#include <stdio.h>  /* printf() */
#include <stdlib.h> /* exit() */
#include <string.h> /* memset() strcmp() strlen() strncmp() */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"

struct srcfilesdata {
    char ** srcfiles;
    Dwarf_Signed srcfilescount;
    int srcfilesres;
};
struct target_data_s {
    Dwarf_Debug td_dbg;
    Dwarf_Unsigned td_target_pc;     /* from argv */
    int            td_print_details; /* from argv */
    int            td_reportallfound; /* from argv */

    /*  cu die data. */
    Dwarf_Unsigned td_cu_lowpc;
    Dwarf_Unsigned td_cu_highpc;
    int            td_cu_haslowhighpc;
    Dwarf_Die      td_cu_die;
    char *         td_cu_name;
    char *         td_cu_comp_dir;
    Dwarf_Unsigned td_cu_number;
    struct srcfilesdata td_cu_srcfiles;
    /*  Help deal with DW_AT_ranges */
    /*  This is base offset of ranges, the value from
        DW_AT_rnglists_base. */
    Dwarf_Unsigned td_cu_ranges_base;

    /*  Following subprog related. Offset has
        tc_cu_ranges_base added in.   */
    Dwarf_Off      td_ranges_offset;
    /*  DIE data of appropriate DIE */
    /*  From DW_AT_name */
    char *         td_subprog_name;
    Dwarf_Unsigned td_subprog_fileindex;
    Dwarf_Die      td_subprog_die;
    Dwarf_Unsigned td_subprog_lowpc;
    Dwarf_Unsigned td_subprog_highpc;
    int            td_subprog_haslowhighpc;
    Dwarf_Unsigned td_subprog_lineaddr;
    Dwarf_Unsigned td_subprog_lineno;
    char *         td_subprog_srcfile; /* dealloc */
};
/* Adding return codes to DW_DLV, relevant to our purposes here. */
#define NOT_THIS_CU 10
#define IN_THIS_CU 11
#define FOUND_SUBPROG 12

static int look_for_our_target(Dwarf_Debug dbg,
    struct target_data_s *target_data,
    Dwarf_Error *errp);
static int examine_die_data(Dwarf_Debug dbg,
    int is_info,Dwarf_Die die,
    int level, struct target_data_s *td, Dwarf_Error *errp);
static int check_comp_dir(Dwarf_Debug dbg,Dwarf_Die die,
    struct target_data_s *td,Dwarf_Error *errp);
static int get_die_and_siblings(Dwarf_Debug dbg,
    Dwarf_Die in_die,
    int is_info, int in_level,int cu_number,
    struct target_data_s *td, Dwarf_Error *errp);

#if 0
DW_UT_compile                   0x01  /* DWARF5 */
DW_UT_type                      0x02  /* DWARF5 */
DW_UT_partial                   0x03  /* DWARF5 */
#endif

static int unittype      = DW_UT_compile;
static Dwarf_Bool g_is_info = TRUE;

int cu_version_stamp = 0;
int cu_offset_size = 0;

static void
printusage(void)
{
    printf("Usage example: "
        "./findfuncbypc --pc=0x10000 ./findfuncbypc\n");
    printf(" options list:\n");
    printf(" --help or -h prints this usage message and stops.\n");
    printf(" --pc=(hex or decimal pc address)\n");
    printf(" --printdetails  \n");
    printf("   prints some details of the discovery process\n");
    printf(" --allinstances\n");
    printf("   reports but does does not stop processing\n");
    printf("   on finding pc address\n");
    printf(" The argument following valid -- arguments must\n");
    printf("   be a valid object file path\n");
}

static void target_data_destructor( struct target_data_s *td);

static int
startswithextractnum(const char *arg,
    const char                  *lookfor,
    Dwarf_Unsigned              *numout)
{
    const char *s = 0;
    unsigned prefixlen = strlen(lookfor);
    Dwarf_Unsigned v = 0;
    char *endptr = 0;

    if (strncmp(arg,lookfor,prefixlen)) {
        return FALSE;
    }
    s = arg+prefixlen;
    /*  We are not making any attempt to deal with garbage.
        Pass in good data... */
    v = strtoul(s,&endptr,0);
    if (!*s || *endptr) {
        printf("Incoming argument in error: \"%s\"\n",arg);
        return FALSE;
    }
    *numout = v;
    return TRUE;
}

int
main(int argc, char **argv)
{
    Dwarf_Debug dbg = 0;
    const char *filepath = 0;
    int res = DW_DLV_ERROR;
    Dwarf_Error error;
    Dwarf_Handler errhand = 0;
    Dwarf_Ptr errarg = 0;
    int i = 0;
    Dwarf_Unsigned target_pc = 0;
    #define PATH_LEN 2000
    char real_path[PATH_LEN];
    struct target_data_s target_data;

    real_path[0] = 0;
    memset(&target_data,0, sizeof(target_data));
    for (i = 1; i < argc ; ++i) {
        if (startswithextractnum(argv[i],"--pc=",
            &target_pc)) {
            /* done */
            target_data.td_target_pc = target_pc;
        } else if (!strcmp(argv[i],"--printdetails")){
            target_data.td_print_details = TRUE;
        } else if (!strcmp(argv[i],"--allinstances")){
            target_data.td_reportallfound = TRUE;
        } else if (!strcmp(argv[i],"-h")){
            printusage();
            exit(0);
        } else if (!strcmp(argv[i],"--help")){
            printusage();
            exit(0);
        } else if (!strcmp(argv[i],"--suppress-de-alloc-tree")){
            /*  Do nothing. This function needs that
                left on (the default) */
        } else {
            /*  Assume next arg is a pathname.*/
            break;
        }
    }
    if (i > (argc-1)) {
        printusage();
        exit(EXIT_FAILURE);
    }
    filepath = argv[i];
    /*  Ignoring any later arguments on the command line */
    res = dwarf_init_path(filepath,
        real_path,
        PATH_LEN,
        DW_GROUPNUMBER_ANY,errhand,errarg,&dbg,&error);
    if (res == DW_DLV_ERROR) {
        printf("Giving up, cannot do DWARF processing of %s "
            "dwarf err %" DW_PR_DUu " %s\n",
            filepath,
            dwarf_errno(error),
            dwarf_errmsg(error));
        dwarf_dealloc_error(dbg,error);
        dwarf_finish(dbg);
        exit(EXIT_FAILURE);
    }
    if (res == DW_DLV_NO_ENTRY) {
        printf("Giving up, file %s not found\n",filepath);
        exit(EXIT_FAILURE);
    }
    res = look_for_our_target(dbg,&target_data,&error);
    if (res == FOUND_SUBPROG) {
        /* OK */
    } else if (res != DW_DLV_OK) {
        printf("Look for target PC failed!\n");
    }
    target_data_destructor(&target_data);
    res = dwarf_finish(dbg);
    if (res != DW_DLV_OK) {
        printf("dwarf_finish failed!\n");
    }
    return 0;
}

static void
resetsrcfiles(Dwarf_Debug dbg,struct srcfilesdata *sf)
{
    Dwarf_Signed sri = 0;
    for (sri = 0; sri < sf->srcfilescount; ++sri) {
        dwarf_dealloc(dbg, sf->srcfiles[sri], DW_DLA_STRING);
    }
    dwarf_dealloc(dbg, sf->srcfiles, DW_DLA_LIST);
    sf->srcfilesres = DW_DLV_ERROR;
    sf->srcfiles = 0;
    sf->srcfilescount = 0;
}
static void resetsubprog(Dwarf_Debug dbg,struct target_data_s *td)
{
    td->td_subprog_haslowhighpc = FALSE;
    if (td->td_subprog_die) {
        dwarf_dealloc(dbg,td->td_subprog_die,DW_DLA_DIE);
        td->td_subprog_die = 0;
    }
}
static void
reset_target_data(Dwarf_Debug dbg,struct target_data_s *td)
{
    if (td->td_cu_die) {
        dwarf_dealloc(dbg,td->td_cu_die,DW_DLA_DIE);
        td->td_cu_die = 0;
    }
    if (td->td_subprog_die) {
        dwarf_dealloc(dbg,td->td_subprog_die,DW_DLA_DIE);
        td->td_subprog_die = 0;
    }
    td->td_cu_haslowhighpc = FALSE;
    resetsrcfiles(dbg,&td->td_cu_srcfiles);
    resetsubprog(dbg,td);
}
static void
target_data_destructor( struct target_data_s *td)
{
    Dwarf_Debug dbg = 0;
    if (!td || !td->td_dbg) {
        return;
    }
    dbg = td->td_dbg;
    reset_target_data(dbg,td);
    if (td->td_subprog_srcfile) {
        dwarf_dealloc(dbg,td->td_subprog_srcfile,DW_DLA_STRING);
        td->td_subprog_srcfile = 0;
    }
}

static void
print_target_info( Dwarf_Debug dbg,
    struct target_data_s *td)
{
    (void)dbg;
    printf("FOUND function \"%s\", requested pc 0x%" DW_PR_DUx
        "\n",
        td->td_subprog_name?td->td_subprog_name:"<unknown>",
        td->td_target_pc);
    printf("      function lowpc 0x%" DW_PR_DUx
        " highpc 0x%" DW_PR_DUx "\n",
        td->td_subprog_lowpc,
        td->td_subprog_highpc);
    printf("      file name index 0x%" DW_PR_DUx "\n",
        td->td_subprog_fileindex);
    printf("      in:\n");
    printf("      CU %" DW_PR_DUu "  %s\n",
        td->td_cu_number,
        td->td_cu_name?td->td_cu_name:"<unknown>");
    printf("      comp-dir  %s\n",
        td->td_cu_comp_dir?td->td_cu_comp_dir:"<unknown>");

    printf("      Src line address 0x%" DW_PR_DUx
        " lineno %" DW_PR_DUu
        " %s\n",
        td->td_subprog_lineaddr,
        td->td_subprog_lineno,
        td->td_subprog_srcfile);
    printf("\n");
}

static int
read_line_data(Dwarf_Debug dbg,
    struct target_data_s *td,
    Dwarf_Error *errp)
{
    int res = 0;
    Dwarf_Unsigned line_version = 0;
    Dwarf_Small table_type = 0;
    Dwarf_Line_Context line_context = 0;
    Dwarf_Signed i = 0;
    Dwarf_Signed baseindex  = 0;
    Dwarf_Signed endindex  = 0;
    Dwarf_Signed file_count  = 0;
    Dwarf_Unsigned dirindex  = 0;

    (void)dbg;
    res = dwarf_srclines_b(td->td_cu_die,&line_version,
        &table_type,&line_context,errp);
    if (res != DW_DLV_OK) {
        return res;
    }
    if (td->td_print_details) {
        printf(" Srclines: linetable version %" DW_PR_DUu
            " table count %d\n",line_version,table_type);
    }
    if (table_type == 0) {
        /* There are no lines here, just table header and names. */
        int sres = 0;

        sres = dwarf_srclines_files_indexes(line_context,
            &baseindex,&file_count,&endindex,errp);
        if (sres != DW_DLV_OK) {
            dwarf_srclines_dealloc_b(line_context);
            line_context = 0;
            return sres;
        }
        if (td->td_print_details) {
            printf("  Filenames base index %" DW_PR_DSd
                " file count %" DW_PR_DSd
                " endindex %" DW_PR_DSd "\n",
                baseindex,file_count,endindex);
        }
        for (i = baseindex; i < endindex; i++) {
            Dwarf_Unsigned modtime = 0;
            Dwarf_Unsigned flength = 0;
            Dwarf_Form_Data16 *md5data = 0;
            int vres = 0;
            const char *name = 0;

            vres = dwarf_srclines_files_data_b(line_context,i,
                &name,&dirindex, &modtime,&flength,
                &md5data,errp);
            if (vres != DW_DLV_OK) {
                dwarf_srclines_dealloc_b(line_context);
                line_context = 0;
                /* something very wrong. */
                return vres;
            }
            if (td->td_print_details) {
                printf("  [%" DW_PR_DSd "] "
                    " directory index %" DW_PR_DUu
                    " %s \n",i,dirindex,name);
            }
        }
        dwarf_srclines_dealloc_b(line_context);
        return DW_DLV_OK;
    } else if (table_type == 1) {
        const char * dir_name = 0;
        int sres = 0;
        Dwarf_Line *linebuf = 0;
        Dwarf_Signed linecount = 0;
        Dwarf_Signed dir_count = 0;
        Dwarf_Addr prev_lineaddr = 0;
        Dwarf_Unsigned prev_lineno = 0;
        char * prev_linesrcfile = 0;

        sres = dwarf_srclines_files_indexes(line_context,
            &baseindex,&file_count,&endindex,errp);
        if (sres != DW_DLV_OK) {
            dwarf_srclines_dealloc_b(line_context);
            line_context = 0;
            return sres;
        }
        if (td->td_print_details) {
            printf("  Filenames base index %" DW_PR_DSd
                " file count %" DW_PR_DSd
                " endindex %" DW_PR_DSd "\n",
                baseindex,file_count,endindex);
        }
        for (i = baseindex; i < endindex; i++) {
            Dwarf_Unsigned dirindexb = 0;
            Dwarf_Unsigned modtime = 0;
            Dwarf_Unsigned flength = 0;
            Dwarf_Form_Data16 *md5data = 0;
            int vres = 0;
            const char *name = 0;

            vres = dwarf_srclines_files_data_b(line_context,i,
                &name,&dirindexb, &modtime,&flength,
                &md5data,errp);
            if (vres != DW_DLV_OK) {
                dwarf_srclines_dealloc_b(line_context);
                line_context = 0;
                /* something very wrong. */
                return vres;
            }
            if (td->td_print_details) {
                printf("  [%" DW_PR_DSd "] "
                    " directory index %" DW_PR_DUu
                    " file: %s \n",i,dirindexb,name);
            }
        }
        sres = dwarf_srclines_include_dir_count(line_context,
            &dir_count,errp);
        if (sres != DW_DLV_OK) {
            dwarf_srclines_dealloc_b(line_context);
            line_context = 0;
            return sres;
        }
        if (td->td_print_details) {
            printf("  Directories count: %" DW_PR_DSd "\n",
                dir_count);
        }

        for (i =1; i <= dir_count; ++i) {
            dir_name = 0;
            sres = dwarf_srclines_include_dir_data(line_context,
                i,&dir_name,errp);
            if (sres == DW_DLV_ERROR) {
                dwarf_srclines_dealloc_b(line_context);
                line_context = 0;
                return sres;
            }
            if (sres == DW_DLV_NO_ENTRY) {
                printf("Something wrong in dir tables line %d %s\n",
                    __LINE__,__FILE__);
                break;
            }
            if (td->td_print_details) {
                printf("  [%" DW_PR_DSd "] directory: "
                    " %s \n",i,dir_name);
            }
        }

        /*  For this case where we have a line table we will likely
            wish to get the line details: */
        sres = dwarf_srclines_from_linecontext(line_context,
            &linebuf,&linecount,
            errp);
        if (sres != DW_DLV_OK) {
            dwarf_srclines_dealloc_b(line_context);
            line_context = 0;
            return sres;
        }
        /* The lines are normal line table lines. */
        for (i = 0; i < linecount; ++i) {
            Dwarf_Addr lineaddr = 0;
            Dwarf_Unsigned filenum = 0;
            Dwarf_Unsigned lineno = 0;
            char * linesrcfile = 0;

            sres = dwarf_lineno(linebuf[i],&lineno,errp);
            if (sres == DW_DLV_ERROR) {
                if (prev_linesrcfile) {
                    dwarf_dealloc(dbg,prev_linesrcfile,DW_DLA_STRING);
                }
                return sres;
            }
            sres = dwarf_line_srcfileno(linebuf[i],&filenum,errp);
            if (sres == DW_DLV_ERROR) {
                if (prev_linesrcfile) {
                    dwarf_dealloc(dbg,prev_linesrcfile,DW_DLA_STRING);
                }
                return sres;
            }
            if (filenum) {
                filenum -= 1;
            }
            sres = dwarf_lineaddr(linebuf[i],&lineaddr,errp);
            if (sres == DW_DLV_ERROR) {
                if (prev_linesrcfile) {
                    dwarf_dealloc(dbg,prev_linesrcfile,DW_DLA_STRING);
                }
                return sres;
            }
            sres = dwarf_linesrc(linebuf[i],&linesrcfile,errp);
            if (sres == DW_DLV_ERROR) {
                if (prev_linesrcfile) {
                    dwarf_dealloc(dbg,prev_linesrcfile,DW_DLA_STRING);
                }
                return sres;
            }
            if (td->td_print_details) {
                printf("  [%" DW_PR_DSd "] "
                    " address 0x%" DW_PR_DUx
                    " filenum %" DW_PR_DUu
                    " lineno %" DW_PR_DUu
                    " %s \n",i,lineaddr,filenum,lineno,linesrcfile);
            }
            if (lineaddr > td->td_target_pc) {
                /* Here we detect the right source and line */
                td->td_subprog_lineaddr = prev_lineaddr;
                td->td_subprog_lineno = prev_lineno;
                td->td_subprog_srcfile = prev_linesrcfile;
                dwarf_dealloc(dbg,linesrcfile,DW_DLA_STRING);
                return DW_DLV_OK;
            }
            prev_lineaddr = lineaddr;
            prev_lineno = lineno;
            if (prev_linesrcfile) {
                dwarf_dealloc(dbg,prev_linesrcfile,DW_DLA_STRING);
            }
            prev_linesrcfile = linesrcfile;
        }
        /*  Here we detect the right source and line (last such
            in this subprogram) */
        td->td_subprog_lineaddr = prev_lineaddr;
        td->td_subprog_lineno = prev_lineno;
        td->td_subprog_srcfile = prev_linesrcfile;
        dwarf_srclines_dealloc_b(line_context);
        return DW_DLV_OK;
    }
    return DW_DLV_ERROR;
#if 0
    /* ASSERT: table_type == 2,
    Experimental two-level line table. Version 0xf006
    We do not define the meaning of this non-standard
    set of tables here. */
    /*  For ’something C’ (two-level line tables)
        one codes something like this
        Note that we do not define the meaning
        or use of two-level li
        tables as these are experimental,
        not standard DWARF. */
    sres = dwarf_srclines_two_level_from_linecontext(line_context,
        &linebuf,&linecount,
        &linebuf_actuals,&linecount_actuals,
        &err);
    if (sres == DW_DLV_OK) {
        for (i = 0; i < linecount; ++i) {
            /*  use linebuf[i], these are
                the ’logicals’ entries. */
        }
        for (i = 0; i < linecount_actuals; ++i) {
            /*  use linebuf_actuals[i],
                these are the actuals entries */
        }
        dwarf_srclines_dealloc_b(line_context);
        line_context = 0;
        linebuf = 0;
        linecount = 0;
        linebuf_actuals = 0;
        linecount_actuals = 0;
    } else if (sres == DW_DLV_NO_ENTRY) {
        dwarf_srclines_dealloc_b(line_context);
        line_context = 0;
        linebuf = 0;
        linecount = 0;
        linebuf_actuals = 0;
        linecount_actuals = 0;
        return sres;
    } else { /*DW_DLV_ERROR */
        return sres;
    }
#endif /* 0 */
}

static int
look_for_our_target(Dwarf_Debug dbg,
    struct target_data_s *td,
    Dwarf_Error *errp)
{
    Dwarf_Unsigned cu_header_length = 0;
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half     address_size = 0;
    Dwarf_Half     version_stamp = 0;
    Dwarf_Half     offset_size = 0;
    Dwarf_Half     extension_size = 0;
    Dwarf_Unsigned typeoffset = 0;
    Dwarf_Half     header_cu_type = unittype;
    Dwarf_Bool     is_info = g_is_info;
    int cu_number = 0;

    for (;;++cu_number) {
        Dwarf_Die no_die = 0;
        Dwarf_Die cu_die = 0;
        int res = DW_DLV_ERROR;
        Dwarf_Sig8     signature;

        memset(&signature,0, sizeof(signature));
        reset_target_data(dbg,td);
        res = dwarf_next_cu_header_d(dbg,is_info,&cu_header_length,
            &version_stamp, &abbrev_offset,
            &address_size, &offset_size,
            &extension_size,&signature,
            &typeoffset, 0,
            &header_cu_type,errp);
        if (res == DW_DLV_ERROR) {
            char *em = dwarf_errmsg(*errp);
            printf("Error in dwarf_next_cu_header: %s\n",em);
            target_data_destructor(td);
            dwarf_finish(dbg);
            exit(EXIT_FAILURE);
        }
        if (res == DW_DLV_NO_ENTRY) {
            /* Done. */
            return DW_DLV_NO_ENTRY;
        }
        cu_version_stamp = version_stamp;
        cu_offset_size   = offset_size;
        /* The CU will have a single sibling, a cu_die. */
        res = dwarf_siblingof_b(dbg,no_die,is_info,
            &cu_die,errp);
        if (res == DW_DLV_ERROR) {
            char *em = dwarf_errmsg(*errp);
            printf("Error in dwarf_siblingof_b on CU die: %s\n",em);
            target_data_destructor(td);
            dwarf_finish(dbg);
            exit(EXIT_FAILURE);
        }
        if (res == DW_DLV_NO_ENTRY) {
            /* Impossible case. */
            printf("no entry! in dwarf_siblingof on CU die \n");
            target_data_destructor(td);
            dwarf_finish(dbg);
            exit(EXIT_FAILURE);
        }

        td->td_cu_die = cu_die;
        res = get_die_and_siblings(dbg,cu_die,is_info,0,cu_number,
            td,errp);
        if (res == FOUND_SUBPROG) {
            read_line_data(dbg,td,errp);
            print_target_info(dbg,td);
            if (td->td_reportallfound) {
                return res;
            }
            reset_target_data(dbg,td);
            return res;
        }
        else if (res == IN_THIS_CU) {
            char *em = dwarf_errmsg(*errp);
            printf("Impossible return code in reading DIEs: %s\n",em);
            target_data_destructor(td);
            dwarf_finish(dbg);
            exit(EXIT_FAILURE);
        }
        else if (res == NOT_THIS_CU) {
            /* This is normal. Keep looking */
        }
        else if (res == DW_DLV_ERROR) {
            char *em = dwarf_errmsg(*errp);
            printf("Error in reading DIEs: %s\n",em);
            target_data_destructor(td);
            dwarf_finish(dbg);
            exit(EXIT_FAILURE);
        }
        else if (res == DW_DLV_NO_ENTRY) {
            /* This is odd. Assume normal. */
        } else {
            /* DW_DLV_OK. normal. */
        }
        reset_target_data(dbg,td);
    }
    return DW_DLV_NO_ENTRY;
}

/*  Recursion, following DIE tree.
    On initial call in_die is a cu_die (in_level 0 )
*/
static int
get_die_and_siblings(Dwarf_Debug dbg, Dwarf_Die in_die,
    int is_info,int in_level,int cu_number,
    struct target_data_s *td,
    Dwarf_Error *errp)
{
    int res = DW_DLV_ERROR;
    Dwarf_Die cur_die=in_die;
    Dwarf_Die child = 0;

    td->td_cu_number = cu_number;
    res = examine_die_data(dbg,is_info,in_die,in_level,td,errp);
    if (res == DW_DLV_ERROR) {
        printf("Error in die access , level %d \n",in_level);
        dwarf_finish(dbg);
        exit(EXIT_FAILURE);
    }
    if (res == DW_DLV_NO_ENTRY) {
        return res;
    }
    if (res == NOT_THIS_CU) {
        return res;
    }
    if (res == IN_THIS_CU) {
        /*  Fall through to examine details. */
    } else if (res == FOUND_SUBPROG) {
        return res;
    } else {
        /*  ASSERT: res == DW_DLV_OK */
        /*  Fall through to examine details. */
    }

    /*  Now look at the children of the incoming DIE */
    for (;;) {
        Dwarf_Die sib_die = 0;
        res = dwarf_child(cur_die,&child,errp);
        if (res == DW_DLV_ERROR) {
            printf("Error in dwarf_child , level %d \n",in_level);
            dwarf_finish(dbg);
            exit(EXIT_FAILURE);
        }
        if (res == DW_DLV_OK) {
            int res2 = 0;

            res2 = get_die_and_siblings(dbg,child,is_info,
                in_level+1,cu_number,td,errp);
            if (child != td->td_cu_die &&
                child != td->td_subprog_die) {
                /* No longer need 'child' die. */
                dwarf_dealloc(dbg,child,DW_DLA_DIE);
            }
            if (res2 == FOUND_SUBPROG) {
                return res2;
            }
            else if (res2 == IN_THIS_CU) {
                /* fall thru */
            }
            else if (res2 == NOT_THIS_CU) {
                return res2;
            }
            else if (res2 == DW_DLV_ERROR) {
                return res2;
            }
            else if (res2 == DW_DLV_NO_ENTRY) {
                /* fall thru */
            }
            else { /* DW_DLV_OK */
                /* fall thru */
            }
            child = 0;
        }
        res = dwarf_siblingof_b(dbg,cur_die,is_info,
            &sib_die,errp);
        if (res == DW_DLV_ERROR) {
            char *em = dwarf_errmsg(*errp);
            printf("Error in dwarf_siblingof_b , level %d :%s \n",
                in_level,em);
            dwarf_finish(dbg);
            exit(EXIT_FAILURE);
        }
        if (res == DW_DLV_NO_ENTRY) {
            /* Done at this level. */
            break;
        }
        /* res == DW_DLV_OK */
        if (cur_die != in_die) {
            if (child != td->td_cu_die &&
                child != td->td_subprog_die) {
                dwarf_dealloc(dbg,cur_die,DW_DLA_DIE);
            }
        }
        cur_die = sib_die;
        res = examine_die_data(dbg,is_info,cur_die,in_level,td,errp);
        if (res == DW_DLV_ERROR) {
            return res;
        }
        else if (res == DW_DLV_OK) {
            /* fall through */
        }
        else if (res == FOUND_SUBPROG) {
            return res;
        }
        else if (res == NOT_THIS_CU) {
            /* impossible */
        }
        else if (res == IN_THIS_CU) {
            /* impossible */
        } else  {/* res == DW_DLV_NO_ENTRY */
            /* impossible */
            /* fall through */
        }
    }
    return DW_DLV_OK;
}
#if 0
static void
get_addr(Dwarf_Attribute attr,Dwarf_Addr *val)
{
    Dwarf_Error error = 0;
    int res;
    Dwarf_Addr uval = 0;
    Dwarf_Error *errp  = 0;

    errp = &error;
    res = dwarf_formaddr(attr,&uval,errp);
    if (res == DW_DLV_OK) {
        *val = uval;
        return;
    }
    return;
}
static void
get_number(Dwarf_Attribute attr,Dwarf_Unsigned *val)
{
    Dwarf_Error error = 0;
    int res;
    Dwarf_Signed sval = 0;
    Dwarf_Unsigned uval = 0;
    Dwarf_Error *errp  = 0;

    errp = &error;
    res = dwarf_formudata(attr,&uval,errp);
    if (res == DW_DLV_OK) {
        *val = uval;
        return;
    }
    res = dwarf_formsdata(attr,&sval,errp);
    if (res == DW_DLV_OK) {
        *val = sval;
        return;
    }
    return;
}
#endif

/*  Used when have only processed *some* list items
    and dealloc'd.  This finishes up.
    Not really essential as dwarf_finish() would
    do it anyway, but Coverity Scan does not
    understand that dwarf_finish() cleans up.
    So this usefully avoids a coverity scan
    defect report.  */
static void
dealloc_rest_of_list(Dwarf_Debug dbg,
    Dwarf_Attribute *attrbuf,
    Dwarf_Signed attrcount,
    Dwarf_Signed i)
{
    for ( ; i < attrcount; ++i) {
        dwarf_dealloc_attribute(attrbuf[i]);
    }
    dwarf_dealloc(dbg,attrbuf,DW_DLA_LIST);
}

static int
getlowhighpc(
    Dwarf_Die die,
    int  *have_pc_range,
    Dwarf_Addr *lowpc_out,
    Dwarf_Addr *highpc_out,
    Dwarf_Error*error)
{
    Dwarf_Addr hipc = 0;
    int res = 0;
    Dwarf_Half form = 0;
    enum Dwarf_Form_Class formclass = 0;

    *have_pc_range = FALSE;
    res = dwarf_lowpc(die,lowpc_out,error);
    if (res == DW_DLV_OK) {
        res = dwarf_highpc_b(die,&hipc,&form,&formclass,error);
        if (res == DW_DLV_OK) {
            if (formclass == DW_FORM_CLASS_CONSTANT) {
                hipc += *lowpc_out;
            }
            *highpc_out = hipc;
            *have_pc_range = TRUE;
            return DW_DLV_OK;
        }
    }
    /*  Cannot check ranges yet, we don't know the ranges base
        offset yet. */
    return DW_DLV_NO_ENTRY;
}

static int
check_subprog_ranges_for_match(Dwarf_Debug dbg,
    Dwarf_Die die,
    struct target_data_s *td,
    int  *have_pc_range,
    Dwarf_Addr *lowpc_out,
    Dwarf_Addr *highpc_out,
    Dwarf_Error *errp)
{
    int res = 0;
    Dwarf_Ranges *ranges;
    Dwarf_Signed  ranges_count;
    Dwarf_Unsigned  byte_count;
    Dwarf_Signed i = 0;
    Dwarf_Addr baseaddr = 0;
    Dwarf_Off actualoffset = 0;
    int done = FALSE;

    /*  Check libdwarf to ensure ranges_base not added
        again. */
    res = dwarf_get_ranges_b(dbg,td->td_ranges_offset,
        die,
        &actualoffset,
        &ranges,
        &ranges_count,
        &byte_count,
        errp);
    if (res != DW_DLV_OK) {
        return res;
    }
    for (i=0; i < ranges_count && !done; ++i) {
        Dwarf_Ranges *cur = ranges+i;
        Dwarf_Addr lowpc = 0;
        Dwarf_Addr highpc = 0;
        switch(cur->dwr_type) {
        case DW_RANGES_ENTRY:
            lowpc = cur->dwr_addr1 +baseaddr;
            highpc = cur->dwr_addr2 +baseaddr;
            if (td->td_target_pc < lowpc ||
                td->td_target_pc >= highpc) {
                /* We have no interest in this CU */
                break;
            }
            *lowpc_out = lowpc;
            *highpc_out = highpc;
            *have_pc_range = TRUE;
            done = TRUE;
            res = FOUND_SUBPROG;
            break;
        case DW_RANGES_ADDRESS_SELECTION:
            baseaddr = cur->dwr_addr2;
            break;
        case DW_RANGES_END:
            break;
        default:
            printf("Impossible debug_ranges content!"
                " enum val %d \n",(int)cur->dwr_type);
            dwarf_finish(dbg);
            exit(EXIT_FAILURE);
        }
    }
    dwarf_dealloc_ranges(dbg,ranges,ranges_count);
    return res;
}

/*  The return value is not interesting. Getting
    the name is interesting. */
static int
get_name_from_abstract_origin(Dwarf_Debug dbg,
    int is_info,
    Dwarf_Die die,
    char ** name,
    Dwarf_Error *errp)
{
    int res = 0;
    Dwarf_Die abrootdie = 0;
    Dwarf_Attribute ab_attr = 0;
    Dwarf_Off ab_offset = 0;

    res = dwarf_attr(die,DW_AT_abstract_origin,&ab_attr,errp);
    if (res != DW_DLV_OK) {
        return res;
    }

    res = dwarf_global_formref(ab_attr,&ab_offset,errp);
    if (res != DW_DLV_OK) {
        dwarf_dealloc(dbg,ab_attr,DW_DLA_ATTR);
        return res;
    }

    dwarf_dealloc(dbg,ab_attr,DW_DLA_ATTR);
    res = dwarf_offdie_b(dbg,ab_offset,is_info,&abrootdie,errp) ;
    if (res != DW_DLV_OK) {
        return res;
    }
    res = dwarf_diename(abrootdie,name,errp);
    dwarf_dealloc_die(abrootdie);
    return res;
}

static int
check_subprog_details(Dwarf_Debug dbg,
    int is_info,
    Dwarf_Die die,
    struct target_data_s *td,
    int  *have_pc_range_out,
    Dwarf_Addr *lowpc_out,
    Dwarf_Addr *highpc_out,
    Dwarf_Error *errp)
{
    int res = 0;
    Dwarf_Addr lowpc = 0;
    Dwarf_Addr highpc = 0;
    int finalres = 0;
    int have_pc_range = FALSE;

    res = getlowhighpc(die, &have_pc_range,
        &lowpc,&highpc,errp);
    if (res == DW_DLV_OK) {
        if (have_pc_range) {
            int res2 = DW_DLV_OK;
            char *name = 0;

            if (td->td_target_pc < lowpc ||
                td->td_target_pc >= highpc) {
                /* We have no interest in this subprogram */
                finalres =  DW_DLV_OK;
            } else {
                td->td_subprog_die = die;
                td->td_subprog_lowpc = lowpc;
                *lowpc_out = lowpc;
                *highpc_out = highpc;
                *have_pc_range_out = have_pc_range;
                td->td_subprog_highpc = highpc;
                td->td_subprog_haslowhighpc = have_pc_range;
                res2 = dwarf_diename(die,&name,errp);
                if (res2 == DW_DLV_OK) {
                    td->td_subprog_name = name;
                } else {
                    get_name_from_abstract_origin(dbg,is_info,
                        die, &name, errp);
                }
                td->td_subprog_name = name;
                name = 0;
                /* Means this is an address match */
                finalres = FOUND_SUBPROG;
            }
        }
    }
    {
        Dwarf_Signed i = 0;
        Dwarf_Signed atcount = 0;
        Dwarf_Attribute *atlist = 0;

        res = dwarf_attrlist(die,&atlist,&atcount,errp);
        if (res != DW_DLV_OK) {
            return res;
        }
        for (i = 0; i < atcount ; ++i) {
            Dwarf_Half atr = 0;
            Dwarf_Attribute attrib =  atlist[i];

            res = dwarf_whatattr(attrib,&atr,errp);
            if (res != DW_DLV_OK) {
                /* Something is very wrong here.*/
                if (td->td_print_details) {
                    printf("dwarf_whatattr returns bad errcode %d\n",
                        res);
                }
                dealloc_rest_of_list(dbg,atlist,atcount,i);
                return res;
            }
            if (atr == DW_AT_ranges) {
                int res2 = 0;
                int res4 = 0;
                Dwarf_Off ret_offset = 0;
                int has_low_hi = FALSE;
                Dwarf_Addr low = 0;
                Dwarf_Addr high = 0;

                res2 = dwarf_global_formref(attrib,
                    &ret_offset,errp);
                if (res2 != DW_DLV_OK) {
                    dealloc_rest_of_list(dbg,atlist,atcount,i);
                    return res2;
                }
                td->td_ranges_offset = ret_offset +
                    td->td_cu_ranges_base;
                res4 = check_subprog_ranges_for_match(dbg,
                    die,
                    td,
                    &has_low_hi,
                    &low,
                    &high,
                    errp);
                if (res4 == DW_DLV_OK) {
                    continue;
                }
                if (res4 == DW_DLV_NO_ENTRY) {
                    continue;
                }
                if (res4 == FOUND_SUBPROG) {
                    td->td_subprog_lowpc = lowpc;
                    td->td_subprog_highpc = highpc;
                    td->td_subprog_haslowhighpc = has_low_hi;
                    finalres = FOUND_SUBPROG;
                    continue;
                }
                dealloc_rest_of_list(dbg,atlist,atcount,i);
                /* ASSERT: res4 == DW_DLV_ERROR; */
                return res4;
            } else if (atr == DW_AT_decl_file ) {
                int res5 = 0;
                Dwarf_Unsigned file_index = 0;

                res5 = dwarf_formudata(attrib,&file_index,errp);
                if (res5 != DW_DLV_OK) {
                    dealloc_rest_of_list(dbg,atlist,atcount,i);
                    return res5;
                }
                td->td_subprog_fileindex = file_index;
            }
            dwarf_dealloc(dbg,attrib,DW_DLA_ATTR);
        }
        dwarf_dealloc(dbg,atlist,DW_DLA_LIST);
    }
    return finalres;
}

static int
check_comp_dir(Dwarf_Debug dbg,Dwarf_Die die,
    struct target_data_s *td,Dwarf_Error *errp)
{
    int res = 0;
    int finalres = DW_DLV_NO_ENTRY;
    int have_pc_range = FALSE;
    Dwarf_Addr lowpc = 0;
    Dwarf_Addr highpc = 0;
    Dwarf_Off real_ranges_offset = 0;
    int rdone = FALSE;

    res = getlowhighpc(die,
        &have_pc_range,
        &lowpc,&highpc,errp);
    if (res == DW_DLV_OK) {
        if (have_pc_range) {
            if (td->td_target_pc < lowpc ||
                td->td_target_pc >= highpc) {
                /* We have no interest in this CU */
                res =  NOT_THIS_CU;
            } else {
                td->td_cu_lowpc = lowpc;
                td->td_cu_highpc = highpc;
                /* td->td_cu_haslowhighpc = have_pc_range; */
                res = IN_THIS_CU;
            }
        }
    }
    finalres = res;
    {
        Dwarf_Signed atcount = 0;
        Dwarf_Attribute *atlist = 0;
        Dwarf_Signed j = 0;
        int alres = 0;

        alres = dwarf_attrlist(die,&atlist,&atcount,errp);
        if (alres != DW_DLV_OK) {
            return alres;
        }
        for (j = 0; j < atcount ; ++j) {
            Dwarf_Half atr = 0;
            Dwarf_Attribute attrib =  atlist[j];
            int resb = 0;

            resb = dwarf_whatattr(attrib,&atr,errp);
            if (resb != DW_DLV_OK) {
                dwarf_dealloc(dbg,atlist,DW_DLA_LIST);
                /* Something is very wrong here.*/
                printf("dwarf_whatattr returns bad errcode %d, "
                    "serious error somewhere.\n",
                    res);
                return resb;
            }
            if (atr == DW_AT_name) {
                char *name = 0;
                resb = dwarf_formstring(attrib,&name,errp);
                if (resb == DW_DLV_OK) {
                    td->td_cu_name = name;
                }
            } else if (atr == DW_AT_comp_dir) {
                char *name = 0;
                resb = dwarf_formstring(attrib,&name,errp);
                if (resb == DW_DLV_OK) {
                    td->td_cu_comp_dir = name;
                }
            } else if (atr == DW_AT_rnglists_base ||
                atr == DW_AT_GNU_ranges_base) {
                Dwarf_Off rbase = 0;

                resb = dwarf_global_formref(attrib,&rbase,errp);
                if (resb != DW_DLV_OK) {
                    dwarf_dealloc(dbg,atlist,DW_DLA_LIST);
                    return resb;
                }
                td->td_cu_ranges_base = rbase;
            } else if (atr == DW_AT_ranges) {
                /* we have actual ranges. */
                Dwarf_Off rbase = 0;

                resb = dwarf_global_formref(attrib,&rbase,errp);
                if (resb != DW_DLV_OK) {
                    dwarf_dealloc(dbg,atlist,DW_DLA_LIST);
                    return resb;
                }
                real_ranges_offset = rbase;
                rdone = TRUE;
            }
            dwarf_dealloc(dbg,attrib,DW_DLA_ATTR);
        }
        dwarf_dealloc(dbg,atlist,DW_DLA_LIST);
    }
    if (rdone) {
        /*  We can do get ranges now as we already saw
            ranges base above (if any). */
        int resr = 0;
        Dwarf_Ranges *ranges = 0;
        Dwarf_Signed  ranges_count =0;
        Dwarf_Unsigned  byte_count =0;
        Dwarf_Off     actualoffset = 0;
        Dwarf_Signed k = 0;
        int done = FALSE;

        resr = dwarf_get_ranges_b(dbg,real_ranges_offset,
            die,
            &actualoffset,
            &ranges,
            &ranges_count,
            &byte_count,
            errp);
        if (resr != DW_DLV_OK) {
            /* Something badly wrong here. */
            return res;
        }
        for (k=0; k < ranges_count && !done; ++k) {
            Dwarf_Ranges *cur = ranges+k;
            Dwarf_Addr lowpcr = 0;
            Dwarf_Addr highpcr = 0;
            Dwarf_Addr baseaddr = td->td_cu_ranges_base;

            switch(cur->dwr_type) {
            case DW_RANGES_ENTRY:
                lowpc = cur->dwr_addr1 +baseaddr;
                highpc = cur->dwr_addr2 +baseaddr;
                if (td->td_target_pc < lowpc ||
                    td->td_target_pc >= highpc) {
                    /* We have no interest in this CU */
                    break;
                }
                td->td_cu_lowpc = lowpcr;
                td->td_cu_highpc = highpcr;
                td->td_cu_haslowhighpc = TRUE;
                done = TRUE;
                finalres = IN_THIS_CU;
                break;
            case DW_RANGES_ADDRESS_SELECTION:
                baseaddr = cur->dwr_addr2;
                break;
            case DW_RANGES_END:
                break;
            default:
                dwarf_finish(dbg);
                printf("Impossible debug_ranges content!"
                    " enum val %d \n",(int)cur->dwr_type);
                exit(EXIT_FAILURE);
            }
        }
        dwarf_dealloc_ranges(dbg,ranges,ranges_count);
    }
    return finalres;
}

static int
examine_die_data(Dwarf_Debug dbg,
    int is_info,
    Dwarf_Die die,
    int level,
    struct target_data_s *td,
    Dwarf_Error *errp)
{
    Dwarf_Half tag = 0;
    int res = 0;

    res = dwarf_tag(die,&tag,errp);
    if (res != DW_DLV_OK) {
        printf("Error in dwarf_tag , level %d \n",level);
        dwarf_finish(dbg);
        exit(EXIT_FAILURE);
    }
    if ( tag == DW_TAG_subprogram ||
        tag == DW_TAG_inlined_subroutine) {
        int have_pc_range = 0;
        Dwarf_Addr lowpc = 0;
        Dwarf_Addr highpc = 0;

        res = check_subprog_details(dbg,is_info,die,td,
            &have_pc_range,
            &lowpc,
            &highpc,
            errp);
        if (res == FOUND_SUBPROG) {
            td->td_subprog_die = die;
            return res;
        }
        else if (res == DW_DLV_ERROR)  {
            return res;
        }
        else if (res ==  DW_DLV_NO_ENTRY) {
            /* impossible? */
            return res;
        }
        else if (res ==  NOT_THIS_CU) {
            /* impossible */
            return res;
        }
        else if (res ==  IN_THIS_CU) {
            /* impossible */
            return res;
        } else {
            /* DW_DLV_OK */
        }
        return DW_DLV_OK;
    } else if (tag == DW_TAG_compile_unit ||
        tag == DW_TAG_partial_unit ||
        tag == DW_TAG_type_unit) {

        if (level) {
            /*  Something badly wrong, CU DIEs are only
                at level 0. */
            return NOT_THIS_CU;
        }
        res = check_comp_dir(dbg,die,td,errp);
        return res;
    }
    /*  Keep looking */
    return DW_DLV_OK;
}
