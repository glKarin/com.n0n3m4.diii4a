/*
Copyright (c) 2009-2025 David Anderson.  All rights reserved.
This example code is hereby placed in the Public Domain
for anyone to use for any purpose.
*/

/*  frame1.c
    It specifically calls dwarf_expand_frame_instructions()
    to verify that works without crashing!

    To use, try
        make
        ./frame1 frame1

    This uses public structs from libdwarf.h so it is
    stuck with limitations for compatibility.
    See Dwarf_Regtable3 in libdwarf.h .

    See frame2.c for a faster way to
    build a complete map of all frame rows.

    Options:
    --skip-all-printf
        Turn off all printf to allow focusing on
        just the time spent in libdwarf.
    --just-print-selected-regs
        Avoid extra printing for purposes not needed
        for the main purpose here.
    --stop-at-fde-n=v
        where v is an integer, such as 8 .
        Useful for limiting regression test output.

    There needs to be a more flexible alternative, one
    can be updated when DWARF changes.

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

#define FALSE 0
#define TRUE 1
#ifdef _O_BINARY
/*  This is for a Windows environment */
#define O_BINARY _O_BINARY
#else
# ifndef O_BINARY
# define O_BINARY 0  /* So it does nothing in Linux/Unix */
# endif
#endif /* O_BINARY */
static int print_frame_instrs(Dwarf_Debug dbg,
    Dwarf_Frame_Instr_Head frame_instr_head,
    Dwarf_Unsigned frame_instr_count,
    Dwarf_Error *error);

static void read_frame_data(Dwarf_Debug dbg,const char *sec);
static void print_fde_instrs(Dwarf_Debug dbg, Dwarf_Fde fde,
    Dwarf_Error *error);
static void print_regtable(Dwarf_Regtable3 *tab3);
static void print_cie_instrs(Dwarf_Debug dbg,
    Dwarf_Cie cie,Dwarf_Error *error);
static void print_fde_selected_regs( Dwarf_Fde fde);
static void print_reg(int r);

static int just_print_selected_regs = 0;
static int keep_all_printf = 1;
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
    regtabrulecount=1999; /* For performance measurement */
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
            printf("\nStopping at %d FDEs by request\n",
                stop_at_n_fdes);
            break;
        }
        res = dwarf_get_cie_of_fde(fde_data[fdenum],&cie,&error);
        if (res != DW_DLV_OK) {
            printf("Error accessing cie of fdenum %lld" 
                " to get its cie\n",fdenum);
            exit(EXIT_FAILURE);
        }
        if (keep_all_printf) {
        printf("Print cie of fde %lld\n",fdenum);
        print_cie_instrs(dbg,cie,&error);
        printf("\nPrint fde %lld\n",fdenum);
        }
        if (just_print_selected_regs) {
            print_fde_selected_regs(fde_data[fdenum]);
        } else {
            print_fde_instrs(dbg,fde_data[fdenum],&error);
            if (print_selected_regs) {
                print_fde_selected_regs(fde_data[fdenum]);
            }
        }
    }

    /* Done with the data. */
    dwarf_dealloc_fde_cie_list(dbg,cie_data,cie_element_count,
        fde_data, fde_element_count);
    return;
}

/* Simply shows the instructions at hand for this fde. */
static void
print_cie_instrs(Dwarf_Debug dbg,Dwarf_Cie cie,Dwarf_Error *error)
{
    int res = DW_DLV_ERROR;
    Dwarf_Unsigned bytes_in_cie = 0;
    Dwarf_Small version = 0;
    char *augmentation = 0;
    Dwarf_Unsigned code_alignment_factor = 0;
    Dwarf_Signed data_alignment_factor = 0;
    Dwarf_Half   return_address_register_rule = 0;
    Dwarf_Small   *instrp = 0;
    Dwarf_Unsigned instr_len = 0;
    Dwarf_Half offset_size = 0;
    Dwarf_Signed cie_index = 0;

    res = dwarf_get_cie_info_b(cie,&bytes_in_cie,
        &version, &augmentation, &code_alignment_factor,
        &data_alignment_factor, &return_address_register_rule,
        &instrp,&instr_len,&offset_size,error);
    if (res != DW_DLV_OK) {
        printf("Unable to get cie info!\n");
        exit(EXIT_FAILURE);
    }
    res = dwarf_get_cie_index(cie,&cie_index,error);
    if (res != DW_DLV_OK) {
        printf("Unable to get cie index!\n");
        exit(EXIT_FAILURE);
    }
    printf("CIE info\n");
    printf("  cie index              : %lld\n",
        cie_index);
    printf("  cie length             : 0x%llx (%llu)\n",
        bytes_in_cie,bytes_in_cie);
    printf("  cie version            : %d\n",version);
    printf("  cie augmenter          : %s\n",
        augmentation?augmentation:"<none>");
    printf("  code alignment factor  : %llu\n",
        code_alignment_factor);
    printf("  data alignment factor  : %lld\n",
        data_alignment_factor);
    printf("  return address register: %u\n",
        return_address_register_rule);
    printf("  initial instructions length: %llu\n",
        instr_len);
    printf("  offset size            : %u\n",offset_size);
    {
        Dwarf_Frame_Instr_Head frame_instr_head = 0;
        Dwarf_Unsigned frame_instr_count = 0;
        res = dwarf_expand_frame_instructions(cie,
            instrp,instr_len,
            &frame_instr_head,
            &frame_instr_count,
            error);
        if (res != DW_DLV_OK) {
            printf("dwarf_expand_frame_instructions failed!\n");
            exit(EXIT_FAILURE);
        }
        printf("CIE op count: %llu\n",frame_instr_count);
        print_frame_instrs(dbg,frame_instr_head,
            frame_instr_count, error);
        dwarf_dealloc_frame_instr_head(frame_instr_head);
    }
}

static void
print_fde_col(Dwarf_Signed k,
    Dwarf_Addr   jsave,
    Dwarf_Small  value_type,
    Dwarf_Signed offset_relevant,
    Dwarf_Signed reg_used,
    Dwarf_Signed offset,
    Dwarf_Block *block,
    Dwarf_Addr   row_pc,
    Dwarf_Bool   has_more_rows,
    Dwarf_Addr   subsequent_pc)
{
    char *type_title = "";
    Dwarf_Unsigned rule_id = k;

    (void)has_more_rows;
    (void)subsequent_pc;
    if (row_pc != jsave) {
        if (keep_all_printf) {
        printf(" row_pc=0x%llx" ,row_pc);
        }
    }
    if (keep_all_printf) {
        if (k == CFA_VAL) {
            printf(" col=%lld [CFA] ",k);
        } else {
            printf(" col=%lld ",k);
        }
    }
    switch(value_type) {
    case DW_EXPR_OFFSET:
        type_title = "DW_EXPR_OFFSET";
        goto preg2;
    case DW_EXPR_VAL_OFFSET:
        type_title = "DW_EXPR_VAL_OFFSET";

        preg2:
        if (keep_all_printf) {
        printf("<%s ", type_title);
        if (reg_used == SAME_VAL) {
            printf(" SAME_VAL");
            /* break; */
        } else if (reg_used == UNDEF_VAL) {
            printf(" UNDEF_VAL");
        }
        printf("[");
        print_reg(rule_id);

        printf("=");
        if (offset_relevant == 0) {
            print_reg(reg_used);
            printf(" ");
        } else {
            printf("%02lld", offset);
            printf("(");
            print_reg(reg_used);
            printf(") ");
        }
        printf("]");
        }
        break;
    case DW_EXPR_EXPRESSION:
        type_title = "DW_EXPR_EXPRESSION";
        goto pexp2;
    case DW_EXPR_VAL_EXPRESSION:
        type_title = "DW_EXPR_VAL_EXPRESSION";

        pexp2:
        if (keep_all_printf) {
        printf("<%s ", type_title);
        print_reg(rule_id);
        printf("=");
        printf("expr-block-len=%llu", block->bl_len);
        {
            char pref[40];

            strcpy(pref, "<");
            strcat(pref, type_title);
            strcat(pref, "bytes:");
            /*  The data being dumped comes direct from
                libdwarf so libdwarf validated it. */
            dump_block(pref, block->bl_data, block->bl_len);
            printf("%s", "> \n");
#if 0
            if (glflags.verbose) {
                struct esb_s exprstring;
                esb_constructor(&exprstring);
                get_string_from_locs(dbg,
                    block_ptr,offset,addr_size,
                    offset_size,version,&exprstring);
                printf("<expr:%s>",esb_get_string(&exprstring));
                esb_destructor(&exprstring);
            }
#endif
        }
        }
        break;
    default:
        printf("Internal error in libdwarf, value type %d\n",
            value_type);
        exit(EXIT_FAILURE);
    }
#if 0
    if (has_more_rows) {
        printf(" has_more_rows? %s next pc: 0x%llx>",
            has_more_rows?"yes.":"no.",
            subsequent_pc);
    } else {
        printf("%s", ">");
    }
#endif
    if (keep_all_printf) {
    printf("%s", ">");
    printf("\n");
    }
}

/*  In dwarfdump we use
    dwarf_get_fde_info_for_cfa_reg3_b() to get subsequent pc
    and avoid incrementing pc for the next cfa.

    Here, to verify function added in May 2018,
    we instead use dwarf_get_fde_info_for_reg3_c()
    which has the has_more_rows and subsequent_pc functions
    for the case where one is tracking a particular register
    and not closely watching the CFA value itself. */

static const Dwarf_Block dwblockzero;
static void
print_fde_selected_regs( Dwarf_Fde fde)
{
    Dwarf_Error oneferr = 0;
    /*  Arbitrary column numbers for testing. 
        Before 2.3.0 it was impossible get the CFA 'column'.
    */
    static int selected_cols[] = {CFA_VAL,1,2,3,4,5,6,7,8,
        9,10,11,12,13,14,15,16};
    static int selected_cols_count =
        sizeof(selected_cols)/sizeof(selected_cols[0]);
    Dwarf_Signed k = 0;
    int fres = 0;

    Dwarf_Addr low_pc = 0;
    Dwarf_Unsigned func_length = 0;
    Dwarf_Small *fde_bytes = NULL;
    Dwarf_Unsigned fde_bytes_length = 0;
    Dwarf_Off cie_offset = 0;
    Dwarf_Signed cie_index = 0;
    Dwarf_Off fde_offset = 0;
    Dwarf_Fde curfde = fde;
    Dwarf_Cie cie = 0;
    Dwarf_Addr jsave = 0;
    Dwarf_Addr high_addr = 0;
    Dwarf_Addr next_jsave = 0;
    Dwarf_Bool has_more_rows = FALSE;
    Dwarf_Addr subsequent_pc = 0;
    Dwarf_Error error = 0;
    int res = 0;

    fres = dwarf_get_fde_range(curfde,
        &low_pc, &func_length,
        &fde_bytes,
        &fde_bytes_length,
        &cie_offset, &cie_index,
        &fde_offset, &oneferr);

    if (fres == DW_DLV_ERROR) {
        printf("FAIL: dwarf_get_fde_range err %llu"
            " line %d\n",
            dwarf_errno(oneferr),__LINE__);
        exit(EXIT_FAILURE);
    }
    if (fres == DW_DLV_NO_ENTRY) {
        printf("No fde range data available\n");
        return;
    }
    res = dwarf_get_cie_of_fde(fde,&cie,&error);
    if (res != DW_DLV_OK) {
        printf("Error getting cie from fde\n");
        exit(EXIT_FAILURE);
    }

    high_addr = low_pc + func_length;
    /*  Could check has_more_rows here instead of high_addr,
        If we initialized has_more_rows to 1 above. */
    for (jsave = low_pc ; next_jsave < high_addr;
        jsave = next_jsave) {
        next_jsave = jsave+1;
        if (keep_all_printf) {
        printf("\n");
        printf(" FDE columns (registers) for pc 0x%llx"
            "\n",jsave);
        }
        /* First, access CFA 'column',  */
        for (k = 0; k < selected_cols_count ; ++k ) {
            Dwarf_Unsigned reg = 0;
            Dwarf_Unsigned offset_relevant = 0;
            int            fires = 0;
            Dwarf_Small    value_type = 0;
            Dwarf_Block    block; /* not initialized */
            Dwarf_Signed   offset = 0;
            Dwarf_Addr     row_pc = 0;
            Dwarf_Unsigned col = selected_cols[k];

            block = dwblockzero;
            fires = dwarf_get_fde_info_for_reg3_c(curfde,
                col,
                jsave,
                &value_type,
                &offset_relevant,
                &reg,
                &offset,
                &block,
                &row_pc,
                &has_more_rows,
                &subsequent_pc,
                &oneferr);
            if (fires == DW_DLV_ERROR) {
                printf("FAIL: reading reg err %llu line %d",
                    dwarf_errno(oneferr),__LINE__);
                exit(EXIT_FAILURE);
            }
            if (fires == DW_DLV_NO_ENTRY) {
                continue;
            }
            print_fde_col(
                selected_cols[k],jsave,
                value_type,offset_relevant,
                reg,offset,&block,row_pc,
                has_more_rows, subsequent_pc);
            if (has_more_rows) {
                next_jsave = subsequent_pc;
            } else {
                next_jsave = high_addr;
            }
        }
    }
}

static int
print_frame_instrs(Dwarf_Debug dbg,
    Dwarf_Frame_Instr_Head frame_instr_head,
    Dwarf_Unsigned frame_instr_count,
    Dwarf_Error *error)
{
    Dwarf_Unsigned i = 0;

    if (keep_all_printf) {
    printf("\nPrint %llu frame instructions\n",
        frame_instr_count);
    }
    for ( ; i < frame_instr_count; ++i) {
        int res = 0;
        Dwarf_Unsigned  instr_offset_in_instrs = 0;
        Dwarf_Small     cfa_operation = 0;
        const char     *fields= 0;
        Dwarf_Unsigned  u0 = 0;
        Dwarf_Unsigned  u1 = 0;
        Dwarf_Signed    s0 = 0;
        Dwarf_Signed    s1 = 0;
        Dwarf_Block     expression_block;
        Dwarf_Unsigned  code_alignment_factor = 0;
        Dwarf_Signed    data_alignment_factor = 0;
        const char     *op_name = 0;

        expression_block = dwblockzero;
        res = dwarf_get_frame_instruction(frame_instr_head,
            i,&instr_offset_in_instrs, &cfa_operation,
            &fields, &u0,&u1,&s0,&s1,
            &code_alignment_factor,
            &data_alignment_factor,
            &expression_block,error);
        if (res != DW_DLV_OK) {
            if (res == DW_DLV_ERROR) {
                printf("ERROR reading frame instruction "
                    "%llu\n",
                    frame_instr_count);
                dwarf_dealloc_error(dbg,*error);
                *error = 0;
            } else {
                printf("NO ENTRY reading frame instruction "
                    " %llu\n",frame_instr_count);
            }
            break;
        }
        if (keep_all_printf) {
        dwarf_get_CFA_name(cfa_operation,&op_name);
        printf("[%2llu]  %llu %s ",i,
            instr_offset_in_instrs,op_name);
        switch(fields[0]) {
        case 'u': {
            if (!fields[1]) {
                printf("%llu (0x%llx\n",
                    u0,u0);
            }
            if (fields[1] == 'c') {
                Dwarf_Unsigned final =
                    u0*code_alignment_factor;
                printf("%llu",final);
#if 0
                if (glflags.verbose) {
                    printf("  (%llu * %llu)",
                        u0,code_alignment_factor);

                }
#endif
                printf("\n");
            }
        }
        break;
        case 'r': {
            if (!fields[1]) {
                printf("r%llu\n",u0);
                break;
            }
            if (fields[1] == 'u') {
                if (!fields[2]) {
                    printf("%llu",u1);
                    printf("\n");
                    break;
                }
                if (fields[2] == 'd') {
                    Dwarf_Signed final =
                        (Dwarf_Signed)u0 *
                        data_alignment_factor;
                    printf("%lld",final);
                    printf("\n");
                }
            }
            if (fields[1] == 'r') {
                printf("r%llu\n",u0);
                printf(" ");
                printf("r%llu\n",u1);
                printf("\n");
            }
            if (fields[1] == 's') {
                if (fields[2] == 'd') {
                    Dwarf_Signed final = s1 * data_alignment_factor;
                    printf("r%llu\n",u0);
                    printf("%llu", final);
#if 0
                    if (glflags.verbose) {
                        printf("  (%lld * %lld",
                            s1,data_alignment_factor);
                    }
#endif
                    printf("\n");
                }
            }
            if (fields[1] == 'b') {
                /* rb */
                printf("r%llu\n",u0);
                printf("%llu",u0);
                printf(" expr block len %llu\n",
                    expression_block.bl_len);
                dump_block("    ", expression_block.bl_data,
                    (Dwarf_Signed) expression_block.bl_len);
                printf("\n");
#if 0
                if (glflags.verbose) {
                    print_expression(dbg,die,&expression_block,
                        addr_size,offset_size,
                        version);
                }
#endif
            }
        }
        break;
        case 's': {
            if (fields[1] == 'd') {
                Dwarf_Signed final = s0*data_alignment_factor;

                printf(" %lld",final);
#if 0
                if (glflags.verbose) {
                    printf("  (%lld * %lld",
                        s0,data_alignment_factor);
                }
#endif
                printf("\n");
            }
        }
        break;
        case 'b': {
            if (!fields[1]) {
                printf(" expr block len %llu\n",
                    expression_block.bl_len);
                dump_block("    ", expression_block.bl_data,
                    (Dwarf_Signed) expression_block.bl_len);
                printf("\n");
#if 0
                if (glflags.verbose) {
                    print_expression(dbg,die,&expression_block,
                        addr_size,offset_size,
                        version);
                }
#endif
            }
        }
        break;
        case 0:
            printf("\n");
        break;
        default:
            printf("UNKNOWN FIELD 0x%x\n",fields[0]);
        }
        }
    }
    return DW_DLV_OK;
}

static const Dwarf_Regtable3 zerotab3;
/* Just prints the instructions in the fde. */
static void
print_fde_instrs(Dwarf_Debug dbg,
    Dwarf_Fde fde, Dwarf_Error *error)
{
    int res;
    Dwarf_Addr lowpc = 0;
    Dwarf_Unsigned func_length = 0;
    Dwarf_Small * fde_bytes;
    Dwarf_Unsigned fde_byte_length = 0;
    Dwarf_Off cie_offset = 0;
    Dwarf_Signed cie_index = 0;
    Dwarf_Off fde_offset = 0;
    Dwarf_Addr arbitrary_addr = 0;
    Dwarf_Addr subsequent_pc = 0;
    Dwarf_Bool has_more_rows =  TRUE;
    Dwarf_Addr actual_pc = 0;
    Dwarf_Regtable3 tab3;
    Dwarf_Unsigned oldrulecount = 0;
    Dwarf_Small  *outinstrs = 0;
    Dwarf_Unsigned instrslen = 0;
    Dwarf_Cie cie = 0;

    res = dwarf_get_fde_range(fde,&lowpc,&func_length,&fde_bytes,
        &fde_byte_length,&cie_offset,&cie_index,&fde_offset,error);
    if (res != DW_DLV_OK) {
        printf("Problem getting fde range \n");
        exit(EXIT_FAILURE);
    }
    /*  As a test case, we could chose an address, but
        with care as lowpc and func length */
    arbitrary_addr = lowpc + (func_length/2);
    if (keep_all_printf) {
    printf("function low pc 0x%llx" 
        "  and length 0x%llx"
        "  and addr we choose 0x%llx"
        "\n",
        lowpc,func_length,arbitrary_addr);
    }

    /*  1 is arbitrary. We are winding up getting the
        rule count here while leaving things unchanged. */
    tab3 = zerotab3;
    oldrulecount = dwarf_set_frame_rule_table_size(dbg,1);
    dwarf_set_frame_rule_table_size(dbg,oldrulecount);

    tab3.rt3_reg_table_size = oldrulecount;
    tab3.rt3_rules = (struct Dwarf_Regtable_Entry3_s *)calloc(
        oldrulecount,
        sizeof(struct Dwarf_Regtable_Entry3_s));
    if (!tab3.rt3_rules) {
        printf("Unable to calloc for %lu rules\n",
           (unsigned long)oldrulecount);
        exit(EXIT_FAILURE);
    }

    printf("Now read frame data at chosen address\n");
    res = dwarf_get_fde_info_for_all_regs3(fde,arbitrary_addr ,
        &tab3,&actual_pc,error);
#if 0
    printf("function  Requested_pc 0x%llx"
        " Actual addr of row 0x%llx\n",
        arbitrary_addr,actual_pc);
#endif
    if (res != DW_DLV_OK) {
        free(tab3.rt3_rules);
        tab3.rt3_rules = 0;
        printf("dwarf_get_fde_info_for_all_regs3 failed!\n");
        exit(EXIT_FAILURE);
    }
    /*  Now for an example of iterating through a range of addrs
        efficiently, lets redo the above and iterate pc values.
        the function called is new as of 0.9.0 October 2023. */
    if (keep_all_printf) {
    printf("Now read frame data again, starting at lowpc\n");
    }
    for (arbitrary_addr=lowpc; has_more_rows  ;
        arbitrary_addr = subsequent_pc) {
        res = dwarf_get_fde_info_for_all_regs3_b(fde,arbitrary_addr,
            &tab3,&actual_pc,&has_more_rows, &subsequent_pc,error);
        if (res != DW_DLV_OK) {
            free(tab3.rt3_rules);
            tab3.rt3_rules = 0;
            printf("dwarf_get_fde_info_for_all_regs3_b failed!\n");
            exit(EXIT_FAILURE);
        }
        if (keep_all_printf) {
        printf("row_pc 0x%lx hasmore %s subsequent_pc 0x%llx\n",
            (unsigned long)actual_pc,has_more_rows?"yes":"no",
            subsequent_pc);
        printf("\nRegtable at pc 0x%llx\n",actual_pc);
        }
        print_regtable(&tab3);
        if (has_more_rows && keep_all_printf) {
            printf("  Next row to print is pc 0x%llx\n",
                subsequent_pc);
        }
    }
    /*print_regtable(&tab3); */

    res = dwarf_get_fde_instr_bytes(fde,&outinstrs,&instrslen,error);
    if (res != DW_DLV_OK) {
        free(tab3.rt3_rules);
        tab3.rt3_rules = 0;
        printf("dwarf_get_fde_instr_bytes failed!\n");
        exit(EXIT_FAILURE);
    }
    res = dwarf_get_cie_of_fde(fde,&cie,error);
    if (res != DW_DLV_OK) {
        free(tab3.rt3_rules);
        tab3.rt3_rules = 0;
        printf("Error getting cie from fde\n");
        exit(EXIT_FAILURE);
    }

    {
        Dwarf_Frame_Instr_Head frame_instr_head = 0;
        Dwarf_Unsigned frame_instr_count = 0;
        res = dwarf_expand_frame_instructions(cie,
            outinstrs,instrslen,
            &frame_instr_head,
            &frame_instr_count,
            error);
        if (res != DW_DLV_OK) {
            free(tab3.rt3_rules);
            tab3.rt3_rules = 0;
            printf("dwarf_expand_frame_instructions failed!\n");
            exit(EXIT_FAILURE);
        }
        if (keep_all_printf) {
        printf("Frame op count: %llu\n",frame_instr_count);
        }
        print_frame_instrs(dbg,frame_instr_head,
            frame_instr_count, error);

        dwarf_dealloc_frame_instr_head(frame_instr_head);
    }
    free(tab3.rt3_rules);
    tab3.rt3_rules = 0;
}

static void
print_reg(int r)
{
    switch(r) {
    case SAME_VAL:
        printf(" [DW_FRAME_SAME_VAL] ");
        break;
    case UNDEF_VAL:
        printf(" [DW_FRAME_UNDEF_VAL] ");
        break;
    case CFA_VAL:
        printf(" [(CFA)] ");
        break;
    default:
        printf(" [r%d] ",r);
        break;
    }
}

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

static void
print_one_regentry(const char *prefix_i,
    struct Dwarf_Regtable_Entry3_s *entry)
{
    char buf[100];
    const char *prefix = prefix_i;
    int is_cfa = !strcmp("cfa",prefix);
    if (is_cfa) {
        prefix="cfa  ";
    } else if (entry->dw_regnum == DW_FRAME_SAME_VAL) {
        return;
    }

    buf[0] = 0;
    printf("%s ",prefix);
    printf("type: [%s] ",
        value_type_name(entry->dw_value_type,
        buf,(unsigned)sizeof(buf)));
    switch(entry->dw_value_type) {
    case DW_EXPR_OFFSET:
        print_reg(entry->dw_regnum);
        printf("   [offset_rel? %s ",
            entry->dw_offset_relevant?"yes.":"no.");
        if (entry->dw_offset_relevant) {
            printf(" Offset  %lld " ,
                (Dwarf_Signed)entry->dw_offset);
            if (is_cfa) {
                printf("Defines cfa value");
            } else {
                printf("Address of value is CFA plus signed offset");
            }
            if (!is_cfa  && entry->dw_regnum != CFA_VAL) {
                printf(" compiler botch, regnum != CFA_VAL");
            }
        } else {
            printf("Value in register");
        }
        printf("]");
        break;
    case DW_EXPR_VAL_OFFSET:
        print_reg(entry->dw_regnum);
        printf("[");
        printf(" offset  %lld " ,
            (Dwarf_Signed)entry->dw_offset);
        if (is_cfa) {
            printf("does this make sense? No?");
        } else {
            printf("value at CFA plus signed offset");
        }
        if (!is_cfa  && entry->dw_regnum != CFA_VAL) {
            printf(" compiler botch, regnum != CFA_VAL");
        }
        printf("]");
        break;
    case DW_EXPR_EXPRESSION:
        print_reg(entry->dw_regnum);
        printf("[");
        if (entry->dw_offset_relevant) {
            printf(" FAIL. ERROR: a DW_EXPR_EXPRESSION "
                "must not have the dw_offset marked as "
                "offset_relevant \n");
            printf(" offset_rel  ERROR: %d ",
                entry->dw_offset_relevant);
            printf(" offset  %lld " ,
                (Dwarf_Signed)entry->dw_offset);
        }
        printf("Block ptr set? %s ",
            entry->dw_block.bl_data?"yes":"no");
        printf(" Value is at address given by expr val ");
        /* printf(" block-ptr  0x%llx ",
            (Dwarf_Unsigned)entry->dw_block_ptr); */
        printf("]");
        break;
    case DW_EXPR_VAL_EXPRESSION:
        printf("[");
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
            printf(" offset  %lld " ,
                (Dwarf_Signed)entry->dw_offset);
        }
        printf(" Value is expr val ");
        if (!entry->dw_block.bl_data) {
            printf("Compiler or libdwarf botch, "
                "NULL block data pointer. ");
        }
        /* printf(" block-ptr  0x%llx ",
            (Dwarf_Unsigned)entry->dw_block.bl_data); */
        printf("]");
        break;
    default: break;
    }
    printf("\n");
}

static void
print_regtable(Dwarf_Regtable3 *tab3)
{
    int r;
    /* We won't print too much. A bit arbitrary. */
    int max = 20;
    if (max > tab3->rt3_reg_table_size) {
        max = tab3->rt3_reg_table_size;
    }
    print_one_regentry("cfa",&tab3->rt3_cfa_rule);
    for (r = 0; r < max; r++) {
        char rn[30];
        snprintf(rn,sizeof(rn),"reg %d",r);
        print_one_regentry(rn,tab3->rt3_rules+r);
    }
}
