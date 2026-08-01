/*
Copyright (c) 2020, David Anderson
All rights reserved.

This software file is hereby placed in the public domain.
For use by anyone for any purpose.
*/

/* This uses this condensed table to make
   a simple fast-access C table.
   Reads dwarf.h to be sure the fast-access table
   has all the named DW_OP in dwarf.h present.
   Build and run with
   make rebuild
*/

#include <config.h>

#include <stdio.h>  /* FILE fclose() fopen() fprintf() printf() */
#include <stdlib.h> /* exit() getenv() */
#include <string.h> /* strcmp() strcpy() strncmp() strlen() */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_opscounttab.h"

#define MAXDEFINELINE 1000
static char *input_name = 0;
static char pathbuf[BUFSIZ];
static char buffer[BUFSIZ];

struct ops_table_s {
    unsigned char ot_first;
    unsigned char ot_last;
    signed   char ot_opcount;
    signed   char ot_stackchange;
};

/*  the ops are 8 bits max */
#define OPS_USED_SIZE    256
#define OPS_USED_DUPS    20
static int ops_used_table[OPS_USED_SIZE];
struct dups_tab_s {
    int dt_val;
    char dt_name1[100];
} dups[OPS_USED_DUPS];
int dups_used = 0;

static struct ops_table_s optabsource[]= {
{DW_OP_addr  ,         0               , 1, 1 },
{DW_OP_deref  ,        0               , 0, 0 },
{DW_OP_const1u,        DW_OP_consts    , 1, 1},
{DW_OP_dup,            DW_OP_over      , 0, 1},
{DW_OP_pick,           0               , 1, 1},
{DW_OP_swap,           0               , 0, 0},
{DW_OP_rot,            0               , 0, 0},
{DW_OP_xderef,         0               , 0, -1},
{DW_OP_abs,            0               , 0, 0},
{DW_OP_and,            DW_OP_mul       , 0, -1},
{DW_OP_neg,            DW_OP_not       , 0, 0},
{DW_OP_or,             DW_OP_plus      , 0, -1},
{DW_OP_plus_uconst,    0               , 1, 0},

{DW_OP_shl,            DW_OP_xor       , 0, -1},
{DW_OP_bra ,           0               , 1, -1},
{DW_OP_eq,             DW_OP_ne        , 0, -1},
{DW_OP_skip,           0               , 1, 0},
{DW_OP_lit0  ,         DW_OP_lit31     , 0, 1},
{DW_OP_reg0  ,         DW_OP_reg31     , 0, 0},
{DW_OP_breg0 ,         DW_OP_breg31    , 1, 1},
{DW_OP_regx  ,         DW_OP_fbreg     , 1, 0},
{DW_OP_bregx,          0               , 2, 1},
{DW_OP_piece,          0               , 1, 0},
{DW_OP_deref_size,     0               , 1, 0},
{DW_OP_xderef_size,    0               , 1, -1},
{DW_OP_nop,            0               , 0, 0},
{DW_OP_push_object_address ,0          , 0, 1},
/*  The called expr may change the stack,
    but not the call itself*/
{DW_OP_call2,          DW_OP_call_ref          , 1, 0},

{DW_OP_form_tls_address,0              , 0, 0},
{DW_OP_call_frame_cfa  ,0              , 0, 1},
{DW_OP_bit_piece,       0              , 2, 0},
{DW_OP_implicit_value,  0              , 2, 0},
{DW_OP_stack_value,     0              , 0, 0},
{DW_OP_implicit_pointer,0              , 2, 0},
{DW_OP_addrx,          DW_OP_constx    , 1, 1},
{DW_OP_entry_value,                   0, 2, 1},
{DW_OP_const_type,                    0, 3, 1},
{DW_OP_regval_type, DW_OP_deref_type   , 2, 1},
{DW_OP_xderef_type,                   0, 0, -1},
{DW_OP_convert /*0xa8*/,              0, 1, 0},
{DW_OP_reinterpret /* 0xa9*/,         0, 1, 0},
{DW_OP_GNU_push_tls_address /*0xe0*/, 0, 0, 1},
{DW_OP_HP_unknown /* 0xe0*/,          0, 0, 0},
{DW_OP_LLVM_form_aspace_address /* 0xe1*/,  0, 0, 1},
{DW_OP_HP_is_value /* 0xe1*/,         0, 1, 1},

{DW_OP_LLVM_push_lane /* 0xe2*/,      0, 0, 1},
{DW_OP_HP_fltconst4 /* 0xe2*/ ,       0, 1, 1},

{DW_OP_LLVM_offset /* 0xe3*/ ,        0, 0, 0},
{DW_OP_HP_fltconst8 /* 0xe3*/ ,       0, 1, 0},

{DW_OP_LLVM_offset_uconst /* 0xe4*/,  0, 1, 0},
{DW_OP_HP_mod_range /* 0xe4*/,        0, 2, 0},

{DW_OP_LLVM_bit_offset /* 0xe5*/,      0, 0, 0},
{DW_OP_HP_unmod_range /* 0xe5*/,      0, 2, 0},

{DW_OP_LLVM_call_frame_entry_reg /* 0xe6*/, 0, 1, 0},
{DW_OP_HP_tls /* 0xe6*/,              0, 0, 0},

{DW_OP_LLVM_undefined /* 0xe7*/,      0, 0, 0},
{DW_OP_LLVM_aspace_bregx /* 0xe8*/,   0, 2, 0},
{DW_OP_INTEL_bit_piece /* 0xe8*/,     0, 2, 0},
{DW_OP_LLVM_aspace_implicit_pointer /* 0xe9*/,   0, 2, 0},
{DW_OP_LLVM_piece_end /* 0xea*/,      0, 0, 0},
{DW_OP_LLVM_extend /* 0xeb*/,         0, 2, 0},

{DW_OP_LLVM_select_bit_piece /* 0xec*/, 0, 2, 0},
{DW_OP_WASM_location /* 0xed*/,       0, 1, 1},
{DW_OP_WASM_location_int /* 0xee*/,   0, 1, 1},

{DW_OP_GNU_uninit /* 0xf0*/,          0, 0, 0},/*unknown opcount*/
{DW_OP_APPLE_uninit /* 0xf0*/,        0, 1, 0},
{DW_OP_GNU_encoded_addr /*0xf1*/,     0, 1, 1},/*1 op is correct*/
{DW_OP_GNU_implicit_pointer /*0xf2*/, 0, 1, 1},/*1 op is correct*/
{DW_OP_GNU_entry_value /*0xf3*/,      0, 2, 1},/*2 op is correct*/
{DW_OP_GNU_const_type /*0xf4 */,      0, 3,1},/*3 is correct*/
{DW_OP_GNU_regval_type /* 0xf5*/,     0, 2,1},/*2 is correct*/
{DW_OP_GNU_deref_type /*0xf6*/,       0, 2,1},/*2 is correct*/
{DW_OP_GNU_convert /*0xf7*/,          0, 1,0},/*1 is correct*/
{DW_OP_PGI_omp_thread_num /*0xf8*/,   0, 0,1},/*just pushes*/
{DW_OP_GNU_reinterpret/* 0xf9*/,      0, 1,0},/*1 is correct*/
{DW_OP_GNU_parameter_ref/* 0xfa*/,    0, 1,1},/*1 is correct*/
{DW_OP_GNU_addr_index/* 0xfb*/,       0, 1,1},/*1 is correct.Fission*/
{DW_OP_GNU_const_index/* 0xfc*/,      0, 1,1},/*1 is correct.Fission*/
{DW_OP_GNU_variable_value/* 0xfd*/,   0, 1,1},/* GNU 2017*/
{0,0,0,0}
};

static void
safe_strcpy(char *targ,char *src,unsigned targlen, unsigned srclen)
{
    if (srclen > targlen) {
        printf("Target name does not fit in buffer.\n"
            "In buildopstabcount.c increase buffer size "
            " from %u \n",(unsigned int)sizeof(buffer));
        exit(EXIT_FAILURE);
    }
    strcpy(targ,src);
}

/*  Issue error message and exit there is a mismatch,
    this should never fail.  */
static void
validate_name(char *name,unsigned long v,unsigned int linenum)
{
    int res = 0;
    const char *s = 0;
    int count = 0;
    if (v >= OPS_USED_SIZE) {
        printf("FAIL! Opcode 0x%lx too large. Impossible\n", v);
        exit(EXIT_FAILURE);

    }
    ops_used_table[v]++;
    count = ops_used_table[v];
    res = dwarf_get_OP_name(v,&s);
    if (res != DW_DLV_OK) {
        printf("Fail dwarf.h line %u value %lu as no OP name! %s\n",
            linenum,v,name);
        exit(EXIT_FAILURE);
    }
    if ( count > 1) {
        fprintf(stderr,"Op 0x%lx used %d times. %s and now %s\n",
            v,count,s, name);
        if (dups_used >= OPS_USED_DUPS) {
            printf("Too many dups, increase table size\n");
            exit(EXIT_FAILURE);
        }
        dups[dups_used].dt_val = v;
        safe_strcpy(dups[dups_used].dt_name1,name,
            sizeof(dups[dups_used].dt_name1),strlen(name));
        dups_used++;
        return;
    }
    if (strcmp(name,s)) {
        printf("Fail dwarf.h line %u value %lu as  OP "
            "name mismatch! %s\n",
            linenum,v,name);
        exit(EXIT_FAILURE);
    }
}

/*  This is N*M overall but the numbers are small, so it's
    unimportant. */
static void
validate_op_listed(char *curdefname,unsigned long v,
    unsigned int linenum)
{
    struct ops_table_s * ops = optabsource ;

    for ( ; ; ++ops) {
        unsigned int j = 0;
        if (!ops->ot_first &&
            !ops->ot_last &&
            !ops->ot_opcount) {
            break;
        }
        if (v == ops->ot_first) {
            validate_name(curdefname,v,linenum);
            return;
        }
        if (ops->ot_last && v == ops->ot_last) {
            validate_name(curdefname,v,linenum);
            return;
        }
        for ( j = ops->ot_first; j < ops->ot_last; ++j) {
            if (v == j) {
                validate_name(curdefname,v,linenum);
                return;
            }
        }
    }
    printf("Failed to find %s val 0x%lu at dwarf.h "
        "line %u in optabsource\n",
        curdefname,v, linenum);
    exit(EXIT_FAILURE);
}

static void
check_if_optabsource_complete(char *path)
{
    FILE *fin = 0;
    unsigned int linenum = 0;
    int loop_done = FALSE;
    const char *oldop = "#define DW_OP_";
    int   oldoplen = strlen(oldop);

    fin = fopen(path,"r");
    if (!fin) {
        printf("Unable to open dwarf.h to read %s\n",path);
        exit(EXIT_FAILURE);
    }
    for ( ;!loop_done;++linenum) {
        char *line = 0;
        char * pastname  = 0;
        unsigned int linelen = 0;
        char *curdefname = 0;
        unsigned int curdefnamelen = 0;
        char *endptr     = 0;
        char *numstart   = 0;
        unsigned long v  = 0;

        line = fgets(buffer,MAXDEFINELINE,fin);
        if (!line) {
            break;
        }
        linelen = strlen(line);
        line[linelen-1] = 0;
        --linelen;
        if (linelen >= (unsigned)(MAXDEFINELINE-1)) {
            printf("define line %u is too long!\n",linenum);
            exit(EXIT_FAILURE);
        }
        if (strncmp(line,oldop,oldoplen)) {
            /* Not ours. */
            continue;
        }
        /* ASSERT: line ends with NUL byte. */
        curdefname = line+8;
        for ( ; ; curdefnamelen++) {
            pastname = curdefname +curdefnamelen;
            if (!*pastname) {
                /* At end of line. Missing value. */
                printf("define line %u of %s: has no number value!\n",
                    linenum,path);
                exit(EXIT_FAILURE);
            }
            if (*pastname == ' ') {
                /* Ok. Now look for value. */
                numstart = pastname + 1;
                break;
            }
        }
        *pastname = 0;
        if (!strcmp(curdefname,"DW_OP_lo_user")) {
            /* This is special, we ignore it. */
            continue;
        }
        if (!strcmp(curdefname,"DW_OP_hi_user")) {
            /* This is special, we ignore it. */
            continue;
        }
        /*  Skip spaces. */
        for ( ; *numstart == ' '; ++numstart) { }
        endptr = 0;
        v = strtoul(numstart,&endptr,0);
        if (v > 0xff) {
            printf("define line %u: DW_OP number value "
                "unreasonable. %lu\n",
                linenum,v);
            exit(EXIT_FAILURE);
        }
        if (v == 0 && endptr == numstart) {
            printf("define line %u of %s: number value missing.\n",
                linenum,path);
            printf("Leaving a space as in #define A B 3"
                " in dwarf.h.in will cause this.\n");
            exit(EXIT_FAILURE);
        }
        if (*endptr != ' ' && *endptr != 0) {
            unsigned char e = *endptr;
            printf("define line %u: number value terminates oddly "
                "char: %u 0x%x line %s\n",
                linenum,e,e,line);
            exit(EXIT_FAILURE);
        }
        if (!v) {
            printf("define line %u: DW_OP number value "
                "zero unreasonable.\n",
                linenum);
            exit(EXIT_FAILURE);
        }
        validate_op_listed(curdefname,v,linenum);
    }
    fclose(fin);
}

static int
havedup(int j, char **out)
{
    int i = 0;
    for (; i < OPS_USED_DUPS; ++i) {
        if (dups[i].dt_val == j) {
            *out = dups[i].dt_name1;
            return 1;
        }
    }
    return 0;
}

int main(int argc, char**argv)
{
    struct ops_table_s *op = 0;
    const char *tailpath = "/src/lib/libdwarf/dwarf.h";
    char  *path  = 0;
    int inindex  = 0;
    int outindex = 0;
    int f        = 0;
    int l        = 0;
    int c        = 0;
    int res      = 0;
    int lastop   = 0;
    unsigned len = 0;

    if (argc > 1) {
        if (argc != 3) {
            printf("Expected -f <filename> of base code path\n");
            exit(EXIT_FAILURE);
        }
        if (strcmp(argv[1],"-f")) {
            printf("Expected -f\n");
            exit(EXIT_FAILURE);
        }
        path=argv[2];
    } else {
        /* env var should be set with base path of code */
        path = getenv("DWTOPSRCDIR");
        if (!path) {
            printf("Expected environment variable "
                "DWTOPSRCDIR with path of "
                "base directory (usually called 'code')\n");
            exit(EXIT_FAILURE);
        }
    }
    len = strlen(path);
    if (len >= sizeof(pathbuf)) {
        printf(" buildopstab Input path greater length "
            "than makes any sense:"
            " Giving up\n");
        exit(EXIT_FAILURE);

    }
    safe_strcpy(pathbuf,path,sizeof(pathbuf),len);
    {
        size_t remaining =  sizeof(pathbuf) -len -1;
        size_t tailpathlen = strlen(tailpath);
        if (tailpathlen >= remaining) {
            printf(" buildopstab Input tailpath greater "
                "length fits in buf: "
                "Giving up\n");
            exit(EXIT_FAILURE);
        }
        /* Notice tailpath has a leading /  */
        safe_strcpy(pathbuf+len,(char *)tailpath,
            remaining,tailpathlen);
    }
    input_name = pathbuf;

    check_if_optabsource_complete(input_name);

    printf("/*  Generated expression ops table, "
        "do not edit. */\n");
    printf("#include \"dd_opscounttab.h\"\n");
    printf("\n");
    printf("struct dwarf_opscounttab_s _dwarf_opscounttab[] = {\n");
    for ( ;  ; ++inindex) {
        const char *opn = 0;
        int sc = 0;

        op = &optabsource[inindex];
        f = op->ot_first;
        if (!f) {
            break;
        }
        if (lastop && f < lastop) {
            printf("FAILED buildopscounttab on OP,out of sequence"
                " f=0x%x lastop=0x%x\n",
                (unsigned)f,(unsigned)lastop);
            exit(EXIT_FAILURE);
        }
        if (f == lastop) {
            /*  A duplicate, ignore here. */
            continue;
        }
        l = op->ot_last;
        c = op->ot_opcount;
        sc = op->ot_stackchange;

        while (f > outindex) {
            printf("{/* %-26s 0x%02x*/ %d, 0},\n","unused",
                outindex,-1);
            ++outindex;
        }
        if (!l) {
            res = dwarf_get_OP_name(f,&opn);
            if (res != DW_DLV_OK) {
                printf("FAILED buildopscounttab on OP 0x%x\n",
                    f);
                exit(EXIT_FAILURE);
            }
            lastop = f;

            printf("{/* %-26s 0x%02x*/ %d, %d},\n",opn,f,c,sc);
            {
                char *dup = 0;
                if (havedup(f,&dup)) {
                    printf("    /* above has alt spelling %s */",
                        dup);
                    printf("\n");
                }
            }
            ++outindex;
        } else {
            int j = f;
            for ( ; j <= l; ++j) {
                res = dwarf_get_OP_name(j,&opn);
                if (res != DW_DLV_OK) {
                    printf("FAILED buildopscounttab on OP 0x%x\n",
                        f);
                    exit(EXIT_FAILURE);
                }
                printf("{/* %-26s 0x%2x*/ %d, %d}",opn,j,c,sc);
                printf(",\n");
                {
                    /*  Should NOT happen, dups should
                        be singleton entries */
                    char *dup = 0;
                    if (havedup(j,&dup)) {
                        printf("    /* above has alt spelling %s */",
                            dup);
                        printf("\n");
                        fprintf(stderr," FAIL an entry "
                            " in a list has dup. Fix table");
                        exit(EXIT_FAILURE);
                    }
                }
                ++outindex;
                lastop = j;
            }
        }
    }
    while (outindex < DWOPS_ARRAY_SIZE) {
        printf("{/* %-26s 0x%02x*/ %d, 0},\n","unused",outindex,-1);
        ++outindex;
    }
    printf("};\n");
    return 0;
}
