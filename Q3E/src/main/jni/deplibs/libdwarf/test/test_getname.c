/*  Copyright (c) 2021 David Anderson
    This test code is hereby placed in the public domain
    for anyone to use in any way.  */

#include <config.h>

#include <stdio.h>  /* printf() */
#include <string.h> /* strcmp() */
#include <stdlib.h> /* exit() */

#include "dwarf.h"
#include "libdwarf.h"

static const char *
resname(int res)
{
    if (res == DW_DLV_OK) {
        return "DW_DLV_OK";
    }
    if (res == DW_DLV_NO_ENTRY) {
        return "DW_DLV_NO_ENTRY";
    }
    if (res == DW_DLV_ERROR) {
        return "DW_DLV_ERROR";
    }
    return "BOGUS ERROR CODE";
}

static int
check_result(int actres, int expres,
    const char * actname,
    const char * expname, int line)
{
    int errcount = 0;
    if (expres != actres) {
        printf("getnametest Expected return %s but got %s line %d\n",
            resname(expres),resname(actres),line);
        errcount++;
    }
    if (expres == DW_DLV_OK) {
        int err = 0;
        if (actname && expname) {
            if (!strcmp(actname,expname)) {
                return 0;
            }
            err = 1;
        } else {
            err = 1;
        }
        printf("getnametest Expected name %s but got %s line %d\n",
            expname?expname:"Bogus expected name",
            actname?actname:"Bogus actual name",line);
        errcount += err;
    }
    return errcount;
}

int main(void)
{
    int res = 0;
    int errcount = 0;
    const char *name = 0;

    res = dwarf_get_DEFAULTED_name(DW_DEFAULTED_no,&name);
    errcount += check_result(res,DW_DLV_OK,name,"DW_DEFAULTED_no",
        __LINE__);

    res = dwarf_get_DEFAULTED_name(DW_DEFAULTED_in_class,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_DEFAULTED_in_class",
        __LINE__);
    res = dwarf_get_DEFAULTED_name(DW_DEFAULTED_out_of_class,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_DEFAULTED_out_of_class",
        __LINE__);

    res = dwarf_get_DEFAULTED_name(3 ,&name);
    errcount += check_result(res,DW_DLV_NO_ENTRY,name,
        "unexpected",
        __LINE__);

    res = dwarf_get_GNUIKIND_name(DW_GNUIKIND_none,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_GNUIKIND_none",
        __LINE__);

    res = dwarf_get_GNUIKIND_name(DW_GNUIKIND_variable,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_GNUIKIND_variable",
        __LINE__);

    res = dwarf_get_GNUIKIND_name(DW_GNUIKIND_other,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_GNUIKIND_other",
        __LINE__);

    res = dwarf_get_GNUIKIND_name(5,&name);
    errcount += check_result(res,DW_DLV_NO_ENTRY,name,
        "unexpected",
        __LINE__);

    res = dwarf_get_GNUIVIS_name(DW_GNUIVIS_global,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_GNUIVIS_global",
        __LINE__);
    res = dwarf_get_GNUIVIS_name(DW_GNUIVIS_static,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_GNUIVIS_static",
        __LINE__);
    res = dwarf_get_GNUIVIS_name(2,&name);
    errcount += check_result(res,DW_DLV_NO_ENTRY,name,
        "unexpected",
        __LINE__);

    res = dwarf_get_IDX_name(DW_IDX_compile_unit,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_IDX_compile_unit",
        __LINE__);
    res = dwarf_get_IDX_name(DW_IDX_die_offset,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_IDX_die_offset",
        __LINE__);
    res = dwarf_get_IDX_name(DW_IDX_type_hash,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_IDX_type_hash",
        __LINE__);
    res = dwarf_get_IDX_name(6,&name);
    errcount += check_result(res,DW_DLV_NO_ENTRY,name,
        "unexpected",
        __LINE__);

    res = dwarf_get_ISA_name(DW_ISA_UNKNOWN,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_ISA_UNKNOWN",
        __LINE__);
    res = dwarf_get_ISA_name(DW_ISA_ARM_thumb,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_ISA_ARM_thumb",
        __LINE__);
    res = dwarf_get_ISA_name(DW_ISA_ARM_arm,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_ISA_ARM_arm",
        __LINE__);
    res = dwarf_get_ISA_name(3,&name);
    errcount += check_result(res,DW_DLV_NO_ENTRY,name,
        "unexpected",
        __LINE__);

    res = dwarf_get_LLEX_name(DW_LLEX_end_of_list_entry,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_LLEX_end_of_list_entry",
        __LINE__);
    res = dwarf_get_LLEX_name(DW_LLEX_start_end_entry,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_LLEX_start_end_entry",
        __LINE__);
    res = dwarf_get_LLEX_name(DW_LLEX_start_end_entry,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_LLEX_start_end_entry",
        __LINE__);
    res = dwarf_get_LLEX_name(5,&name);
    errcount += check_result(res,DW_DLV_NO_ENTRY,name,
        "unexpected",
        __LINE__);

    res = dwarf_get_LNCT_name(DW_LNCT_path,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_LNCT_path",
        __LINE__);
    res = dwarf_get_LNCT_name(DW_LNCT_size,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_LNCT_size",
        __LINE__);
    res = dwarf_get_LNCT_name(DW_LNCT_MD5,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_LNCT_MD5",
        __LINE__);
    res = dwarf_get_LNCT_name(DW_LNCT_GNU_subprogram_name,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_LNCT_GNU_subprogram_name",
        __LINE__);
    res = dwarf_get_LNCT_name(DW_LNCT_LLVM_source,&name);
    errcount += check_result(res,DW_DLV_OK,name,
        "DW_LNCT_LLVM_source",
        __LINE__);
    res = dwarf_get_LNCT_name(9,&name);
    errcount += check_result(res,DW_DLV_NO_ENTRY,name,
        "unexpected",
        __LINE__);
    if (errcount) {
        printf("FAIL getnametest.c\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}
