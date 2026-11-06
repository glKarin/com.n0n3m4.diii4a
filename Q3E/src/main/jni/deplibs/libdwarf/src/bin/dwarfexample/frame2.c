/*
Copyright (c) 2025 David Anderson.  All rights reserved.
This example code is hereby placed in the Public Domain
for anyone to use for any purpose.
*/

/*  frame2.c
    To use, try
        make
        ./frame2 frame2

    This uses public structs from libdwarf.h so it is
    stuck with limitations for compatibility.
    This demonstrates using those public structs
    to build your own map of frame rows while
    using less cpu.
    See Dwarf_Regtable3 in libdwarf.h

    See frame1.c as well.

 Options:
    --skip-all-printf
        Turn off all printf to allow focusing on
        just the time spent in libdwarf.
    --stop-at-fde-n=v
        where v is an integer, such as 8 .
        Useful for limiting regression test output.

    This is much more efficient reading frame data
    than frame1.c because we use an iterator
    and a callback, which avoids repeating
    register table setup.

    gcc/clang may produce .eh_frame without .debug_frame.
    To read .eh_frame call dwarf_get_fde_list_eh()
    as shown below instead of dwarf_get_fde_list() .

    All formatting is standard C (with
    long long types), references to
    DW_PR_* macros have been removed.
    As of December 29 2025, version 2.3.0
*/

#include <config.h>

#include <stddef.h> /* NULL */
#include <stdio.h>  /* printf() snprintf() */
#include <stdlib.h> /* exit() free() malloc() */
#include <string.h> /* strcat() strcmp() strcpy() strncmp() */

#ifdef _WIN32
#include <io.h> /* close() open() */
#elif defined HAVE_UNISTD_H
#include <unistd.h> /* close() */
#endif /* _WIN32 */

#ifdef HAVE_FCNTL_H
#include <fcntl.h> /* open() O_RDONLY */
#endif /* HAVE_FCNTL_H */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"

#ifdef _O_BINARY
/*  This is for a Windows environment */
#define O_BINARY _O_BINARY
#else
# ifndef O_BINARY
# define O_BINARY 0  /* So it does nothing in Linux/Unix */
# endif
#endif /* O_BINARY */
#if 0
static int print_frame_instrs(Dwarf_Debug dbg,
    Dwarf_Frame_Instr_Head frame_instr_head,
    Dwarf_Unsigned frame_instr_count,
    Dwarf_Error *error);
#endif

static void read_frame_data(Dwarf_Debug dbg,const char *sec);
static void print_regtable(Dwarf_Regtable3 *tab3);
static void print_reg(int r);
static int print_all_fde_rows(Dwarf_Debug dbg,
    Dwarf_Signed fdenum,
    Dwarf_Fde fde,
    Dwarf_Error *error);

static int keep_all_printf = 1;
static int just_print_selected_regs = 0;
static int print_selected_regs = 0;
static int stop_at_n_fdes = 0;

/*  Depending on the ABI we set INITIAL_VAL
    differently.  For ia64 initial value is
    UNDEF_VAL, for MIPS and others initial
    value is SAME_VAL.
    Here we'll set it UNDEF_VAL
    as that way we'll see when first set. */
#define UNDEF_VAL DW_FRAME_UNDEFINED_VAL
#define SAME_VAL DW_FRAME_SAME_VAL
#define CFA_VAL DW_FRAME_CFA_COL
/*#define INITIAL_VAL UNDEF_VAL */
#define INITIAL_VAL SAME_VAL

#if 0
/* Dumping a dwarf-expression as a byte stream. */
static void
dump_block(char *prefix, Dwarf_Small *data, Dwarf_Unsigned len)
{
    Dwarf_Small *end_data = data + len;
    Dwarf_Small *cur = data;
    int i = 0;

    printf("%s", prefix);
    for (; cur < end_data; ++cur, ++i) {
        if (i > 0 && i % 4 == 0)
            printf(" ");
        printf("%02x", 0xff & *cur);

    }
}
#endif

/*  Don't really want getopt here. yet.
    so hand do options. */

int
main(int argc, char **argv)
{

    Dwarf_Debug dbg = 0;
    int fd = -1;
    const char *filepath = "<stdin>";
    int res = DW_DLV_ERROR;
    Dwarf_Error error;
    Dwarf_Handler errhand = 0;
    Dwarf_Ptr errarg = 0;
    int regtabrulecount = 0;
    int curopt = 0;
    const char *countstr = "--stop-at-fde-n=";
    int countstr_len = strlen(countstr);

    for (curopt = 1;curopt < argc; ++curopt) {
        if (strncmp(argv[curopt],"--",2)) {
            break;
        }
        if (!strcmp(argv[curopt],"--skip-all-printf")) {
            keep_all_printf = 0;
            continue;
        }
        if (!strcmp(argv[curopt],"--just-print-selected-regs")) {
            just_print_selected_regs++;
            continue;
        }
        if (!strncmp(argv[curopt],countstr,countstr_len)) {
            char *carg = argv[curopt];
            if (!carg[countstr_len]) {
                printf("Improper %s arg, needs a number. Ignored\n",
                    carg);
            } else {
                stop_at_n_fdes = atoi((const char *)
                    (carg+countstr_len));
            }
        }
        if (!strcmp(argv[curopt],"--print-selected-regs")) {
            print_selected_regs++;
            continue;
        }
        if (!strcmp(argv[curopt],"--suppress-de-alloc-tree")) {
            /* Do nothing, ignore the argument */
            continue;
        }
    }

    if (curopt == argc ) {
        fd = 0; /* stdin */
    } else if (curopt == (argc-1)) {
        filepath = argv[curopt];
        fd = open(filepath,O_RDONLY|O_BINARY);
        if (fd < 0) {
            printf("Unable to open %s, giving up.\n",filepath);
            exit(EXIT_FAILURE);
        }
    } else {
        printf("Too many args, giving up. \n");
        exit(EXIT_FAILURE);
    }
    res = dwarf_init_b(fd,DW_GROUPNUMBER_ANY,
        errhand,errarg, &dbg,&error);
    if (res != DW_DLV_OK) {
        printf("Giving up, dwarf_init failed, "
            "cannot do DWARF processing\n");
        if (res == DW_DLV_ERROR) {
            printf("Error code %s\n",dwarf_errmsg(error));
        }
        dwarf_dealloc_error(dbg,error);
        exit(EXIT_FAILURE);
    }
    /*  Do this setting after init before any real operations.
        These return the old values, but here we do not
        need to know the old values.  The sizes and
        values here are higher than most ABIs and entirely
        arbitrary.

        The setting of initial_value to
        the same as undefined-value (the other possible choice being
        same-value) is arbitrary, different ABIs do differ, and
        you have to know which is right.

        In dwarfdump we get the SAME_VAL, UNDEF_VAL,
        INITIAL_VAL CFA_VAL from dwconf_s struct. 

        Do not make regtabrulecount much higher than
        necessary for your system, a value higher 
        than necessary wastes cpu time and memory use.
        The default is DW_FRAME_HIGHEST_NORMAL_REGISTER (188) */
    regtabrulecount=1999;/* for performance measurement. */
    dwarf_set_frame_undefined_value(dbg, UNDEF_VAL);
    dwarf_set_frame_rule_initial_value(dbg, INITIAL_VAL);
    dwarf_set_frame_same_value(dbg,SAME_VAL);
    dwarf_set_frame_cfa_value(dbg,CFA_VAL);
    dwarf_set_frame_rule_table_size(dbg,regtabrulecount);

    read_frame_data(dbg,".debug_frame");
    read_frame_data(dbg,".eh_frame");
    res = dwarf_finish(dbg);
    if (res != DW_DLV_OK) {
        printf("dwarf_finish failed!\n");
    }
    close(fd);
    return 0;
}

static void
read_frame_data(Dwarf_Debug dbg,const char *sect)
{
    Dwarf_Error error;
    Dwarf_Signed cie_element_count = 0;
    Dwarf_Signed fde_element_count = 0;
    Dwarf_Cie *cie_data = 0;
    Dwarf_Fde *fde_data = 0;
    int res = DW_DLV_ERROR;
    Dwarf_Signed fdenum = 0;

    /*  If you wish to read .eh_frame data, use
        dwarf_get_fde_list_eh() instead.
        Get debug_frame with dwarf_get_fde_list. */
    if (keep_all_printf) {
    printf(" Print %s\n",sect);
    }
    if (!strcmp(sect,".eh_frame"))
    {
        res = dwarf_get_fde_list_eh(dbg,&cie_data,&cie_element_count,
            &fde_data,&fde_element_count,&error);
    } else {
        res = dwarf_get_fde_list(dbg,&cie_data,&cie_element_count,
            &fde_data,&fde_element_count,&error);
    }
    if (res == DW_DLV_NO_ENTRY) {
        printf("No %s data present\n",sect);
        return;
    }
    if ( res == DW_DLV_ERROR) {
        printf("Error reading frame data ");
        exit(EXIT_FAILURE);
    }
    if (keep_all_printf) {
    printf( "%lld cies present. "
        "%lld fdes present. \n",
        cie_element_count,fde_element_count);
    }
    /*if (fdenum >= fde_element_count) {
        printf("Want fde %d but only %lld present\n",fdenum,
            fde_element_count);
        exit(EXIT_FAILURE);
    }*/

    for (fdenum = 0; fdenum < fde_element_count; ++fdenum) {
        Dwarf_Cie cie = 0;

        if (stop_at_n_fdes && fdenum >= stop_at_n_fdes) {
            if (keep_all_printf) {
            printf("\nStopping at %d FDEs by request\n",
                stop_at_n_fdes);
            }
            break;
        }
        res = dwarf_get_cie_of_fde(fde_data[fdenum],&cie,&error);
        if (res != DW_DLV_OK) {
            printf("Error accessing cie of fdenum %lld"
                " to get its cie\n",fdenum);
            exit(EXIT_FAILURE);
        }
        if (keep_all_printf) {
        printf("\nPrint fde %lld\n",fdenum);
        }
        /*  This call gets all the reg data via
            the callback function. */
        res = print_all_fde_rows(dbg,fdenum,fde_data[fdenum],&error);
        if (res != DW_DLV_OK) {
            printf("Failure: return %d, on fdenum %ld\n",
                res,
                (signed long)fdenum);
            dwarf_finish(dbg);
            exit(1);
        }
    }

    /* Done with the data. */
    dwarf_dealloc_fde_cie_list(dbg,cie_data,cie_element_count,
        fde_data, fde_element_count);
    return;
}

/*  Local data within user code. dw_callback_user_data.
    Defined and used for the frame row callback.
    Gives the callback whatever data the
    user program provides . */
struct user_data_s {
    Dwarf_Debug    ld_dbg;
    int            ld_fdenum;
    Dwarf_Unsigned ld_lowpc;
};

/*  We must print or copy the data for the
    table row as it will be updated (changed)
    to have the next frame table row data as
    soon as this returns.  */
static int
frame2_callback_all_regs3(Dwarf_Regtable3 *table,
    Dwarf_Addr row_pc,
    Dwarf_Bool has_more_rows,
    Dwarf_Addr subsequent_pc,
    void      *user_data)
{
#if 0
    struct user_data_s * userd =  0;
#endif

    if (!table || !user_data) {
        return DW_DLV_ERROR;
    }
#if 0
    /* Do something with user_data when required */
    userd = (struct user_data_s *)user_data;
#endif
    if (keep_all_printf) {
    if (has_more_rows) {
        printf("row_pc 0x%lx "
            "subsequentpc 0x%lx\n",
            (unsigned long)row_pc,
            (unsigned long) subsequent_pc);
    } else {
        printf("row_pc 0x%lx\n",
            (unsigned long)row_pc);
    }
    print_regtable(table);
    }
    return DW_DLV_OK;
}

static const Dwarf_Regtable3 zero_regtab3;

/*  Gets every row in the frame data */
static int
print_all_fde_rows(Dwarf_Debug dbg,
    Dwarf_Signed fdenum,
    Dwarf_Fde fde,
    Dwarf_Error *error)
{
    int             res;
    Dwarf_Addr      lowpc = 0;
    Dwarf_Unsigned   func_length = 0;
    Dwarf_Small    *fde_bytes;
    Dwarf_Unsigned  fde_byte_length = 0;
    Dwarf_Off       cie_offset = 0;
    Dwarf_Signed    cie_index = 0;
    Dwarf_Off       fde_offset = 0;
    struct user_data_s user_data;
    Dwarf_Unsigned  oldrulecount = 0;
    Dwarf_Regtable3 reg_table;

    reg_table = zero_regtab3;
    res = dwarf_get_fde_range(fde,&lowpc,&func_length,&fde_bytes,
        &fde_byte_length,&cie_offset,&cie_index,&fde_offset,error);
    if (res != DW_DLV_OK) {
        printf("Problem getting fde range \n");
        return res;
    }
    user_data.ld_dbg = dbg;
    user_data.ld_fdenum = fdenum;
    user_data.ld_lowpc = lowpc;
    oldrulecount = dwarf_set_frame_rule_table_size(dbg,1);
    dwarf_set_frame_rule_table_size(dbg,oldrulecount);
    reg_table = zero_regtab3;
    reg_table.rt3_reg_table_size = oldrulecount;
    reg_table.rt3_rules = (struct Dwarf_Regtable_Entry3_s *)calloc(
        oldrulecount,
        sizeof(struct Dwarf_Regtable_Entry3_s));
    if (!reg_table.rt3_rules) {
        printf("Unable to calloc for %lu rules\n",
            (unsigned long)oldrulecount);
        exit(EXIT_FAILURE);
    }
    res = dwarf_iterate_fde_all_regs3(fde,
        &reg_table,
        frame2_callback_all_regs3,
        (void*)(&user_data),error);
    /* DW_DLV_OK or DW_DLV_NO_ENTRY or DW_DLV_ERROR */
    switch(res) {
    case DW_DLV_ERROR:
        printf("DW_DLV_ERROR from "
            "dwarf_iterate_fde_all_regs3 "
            " fde number: %ld %s\n",(signed long)fdenum,
            dwarf_errmsg(*error));
        break;
    case DW_DLV_NO_ENTRY:
        printf("DW_DLV_NO_ENTRY from "
            "dwarf_iterate_fde_all_regs3\n");
        break;
    case DW_DLV_OK:
        break;
    default:
        printf("ERROR IMPOSSIBLE return from "
            "dwarf_iterate_fde_all_regs3\n");
        break;
    }
    free(reg_table.rt3_rules);
    reg_table.rt3_rules = 0;
    return res;
}

static void
print_reg(int r)
{
    if (keep_all_printf) {
    switch(r) {
    case SAME_VAL:
        printf(" [DW_FRAME_SAME_VAL] ");
        break;
    case UNDEF_VAL:
        printf(" [DW_FRAME_UNDEF_VAL] ");
        break;
    case CFA_VAL:
        printf("[(CFA)] ");
        break;
    default:
        printf("[r%d] ",r);
        break;
    }
    }
}

#if 0
static char *
value_type_name(int valuetype,char *buf,unsigned buflen)
{
    buf[0] = 0;
    switch(valuetype) {
    case DW_EXPR_OFFSET:
        return "DW_EXPR_OFFSET";
    case DW_EXPR_VAL_OFFSET:
        return "DW_EXPR_VAL_OFFSET";
    case DW_EXPR_EXPRESSION:
        return "DW_EXPR_EXPRESSION";
    case DW_EXPR_VAL_EXPRESSION:
        return "DW_EXPR_VAL_EXPRESSION";
    default:
        break;
    }
    snprintf(buf,buflen, "Unknown(%d)",valuetype);
    return buf;
}
#endif

/*  The ugly type casts (signed long)(Dwarf_Signed) on dw_offset
    are because the public struct struct Dwarf_Regtable_Entry3_s
    has dw_offset as unsigned though it is really a signed
    value. We have avoided breaking source compatibility so far. */
static void
print_one_regentry(const char *prefix_i,
    struct Dwarf_Regtable_Entry3_s *entry)
{
    const char *prefix = prefix_i;
    int is_cfa = !strcmp("cfa",prefix);
    if (is_cfa) {
        prefix="cfa=";
    }
    if (entry->dw_regnum == DW_FRAME_SAME_VAL) {
        return;
    }

    if (keep_all_printf) {
    printf("  <%s",prefix);
    switch(entry->dw_value_type) {
    case DW_EXPR_OFFSET:
        print_reg(entry->dw_regnum);
        if (keep_all_printf) {
        if (entry->dw_offset_relevant) {
            printf(" %+ld",
                (signed long)(Dwarf_Signed)entry->dw_offset);
            if (!is_cfa  && entry->dw_regnum != CFA_VAL) {
                printf(" compiler botch, regnum %lu != CFA_VAL",
                    (unsigned long)entry->dw_regnum);
            }
        }
        printf(">\n");
        }
        break;
    case DW_EXPR_VAL_OFFSET:
        print_reg(entry->dw_regnum);
        if (keep_all_printf) {
        printf(" %+ld",(signed long)(Dwarf_Signed)entry->dw_offset);
        }
        if (!is_cfa  && entry->dw_regnum != CFA_VAL) {
            printf(" compiler botch, regnum %lu != CFA_VAL",
                (unsigned long)entry->dw_regnum);
        }
        if (keep_all_printf) {
        printf(">\n");
        }
        break;
    case DW_EXPR_EXPRESSION:
        print_reg(entry->dw_regnum);
        if (keep_all_printf) {
        if (entry->dw_offset_relevant) {
            printf(" FAIL. ERROR: a DW_EXPR_EXPRESSION "
                "must not have the dw_offset marked as "
                "offset_relevant \n");
            printf(" offset_rel  ERROR: %d ",
                entry->dw_offset_relevant);
            printf(" %+ld", 
                (signed long)(Dwarf_Signed)entry->dw_offset);
        }
        printf("Block ptr set? %s ",
            entry->dw_block.bl_data?"yes":"no");
        printf(" Value is at address given by expr val ");
        /* printf(" block-ptr  0x%llx ",
            (Dwarf_Unsigned)entry->dw_block_ptr); */
        printf(">\n");
        }
        break;
    case DW_EXPR_VAL_EXPRESSION:
        if (keep_all_printf) {
        printf(" expression byte len  %llu " ,
            entry->dw_block.bl_len);
        printf("Block ptr set? %s ",
            entry->dw_block.bl_data?"yes":"no");
        if (entry->dw_offset_relevant) {
            printf(" FAIL. ERROR: a DW_EXPR_VAL_EXPRESSION "
                "must not have the dw_offset marked as "
                "offset_relevant \n");
            printf(" offset_rel  ERROR: %d ",
                entry->dw_offset_relevant);
            printf("  %+ld",
                (signed long)(Dwarf_Signed)entry->dw_offset);
        }
        printf(" Value is expr val ");
        if (!entry->dw_block.bl_data) {
            printf("Compiler or libdwarf botch, "
                "NULL block data pointer. ");
        }
        /* printf(" block-ptr  0x%llx ",
            (Dwarf_Unsigned)entry->dw_block.bl_data); */
        printf(">\n");
        }
        break;
    default: break;
    }
    }
}

static void
print_regtable(Dwarf_Regtable3 *tab3)
{
    int r = 0;
    /* We won't print too much. A bit arbitrary. */
    int max = 20;

    if (keep_all_printf) {
    if (max > tab3->rt3_reg_table_size) {
        max = tab3->rt3_reg_table_size;
    }
    print_one_regentry("cfa",&tab3->rt3_cfa_rule);
    for (r = 0; r < max; r++) {
        char rn[30];
        snprintf(rn,sizeof(rn),"reg %2d=",r);
        print_one_regentry(rn,tab3->rt3_rules+r);
    }
    printf("\n");
    }
}
