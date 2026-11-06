/*  Copyright (c) 2019-2019, David Anderson
    All rights reserved.

    Redistribution and use in source and binary forms, with
    or without modification, are permitted provided that the
    following conditions are met:

    Redistributions of source code must retain the above
    copyright notice, this list of conditions and the following
    disclaimer.

    Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
    CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
    OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
    EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <config.h>

#include <stdio.h>  /* printf() */
#include <string.h> /* memset() */

#include "dwarf.h"
#include "libdwarf.h"
#include "dwarf_base_types.h"
#include "libdwarfp.h"
#include "dwarf_pro_incl.h"
#include "dwarf_pro_opaque.h"
#include "dwarf_string.h"

static int errcount;

/* int
_dwarf_log_extra_flagstrings(Dwarf_P_Debug dbg,
  const char *extra,
  int *err)
*/

static void
resetdbg(Dwarf_P_Debug dbg)
{
    memset(dbg,0,sizeof(struct Dwarf_P_Debug_s));
}

#if 0
"default_is_stmt"
"minimum_instruction_length"
"maximum_operations_per_instruction"
"opcode_base"
"line_base"
"line_range"
"linetable_version"
"segment_selector_size"
"segment_size"
dbg->de_line_inits.pi_default_is_stmt = (unsigned)v;
dbg->de_line_inits.pi_minimum_instruction_length = (unsigned)v;
dbg->de_line_inits.pi_maximum_operations_per_instruction =
    (unsigned)v;
dbg->de_line_inits.pi_opcode_base = (unsigned)v;
dbg->de_line_inits.pi_line_base = (int)v;
dbg->de_line_inits.pi_line_range = (int)v;
dbg->de_line_inits.pi_linetable_version = (unsigned)v;
dbg->de_line_inits.pi_segment_selector_size = (unsigned)v;
dbg->de_line_inits.pi_segment_size = (unsigned)v;
#endif /* 0 */

static const char *
returnvalstr(int res)
{
    if (res == DW_DLV_OK) return "DW_DLV_OK";
    if (res == DW_DLV_NO_ENTRY) return "DW_DLV_NO_ENTRY";
    if (res == DW_DLV_ERROR) return "DW_DLV_ERROR";
    return "Unknown";
}

static void
check_expected(int exp, int got, int err_exp, int err_got,
    Dwarf_Signed exp_val, Dwarf_Signed got_val, int line)
{
    if ( exp != got) {
        ++errcount;
        printf("Expected res %s, got %s line %d\n",
            returnvalstr(exp), returnvalstr(got),line);
    }
    if ( err_exp != err_got) {
        ++errcount;
        printf("Expected err code %d, got %d line %d\n",
            err_exp,err_got,line);
    }
    if ( exp_val != got_val) {
        ++errcount;
        printf("Expected value %" DW_PR_DSd ", got %"
            DW_PR_DSd " line %d\n",
            exp_val,got_val,line);
    }
}

static void
test1(Dwarf_P_Debug dbg)
{
    int res = 0;
    int err = 0;

    res = _dwarf_log_extra_flagstrings(dbg,"default_is_stmt=1",&err);
    check_expected(DW_DLV_OK,res,0,err,1,
        dbg->de_line_inits.pi_default_is_stmt,
        __LINE__);

    err = 0;
    res = _dwarf_log_extra_flagstrings(dbg,0,&err);
    check_expected(DW_DLV_NO_ENTRY,res,0,err,0,0,
        __LINE__);
    err = 0;
    res = _dwarf_log_extra_flagstrings(dbg,"",&err);
    check_expected(DW_DLV_NO_ENTRY,res,0,err,0,0,
        __LINE__);

    err = 0;
    resetdbg(dbg);
    res = _dwarf_log_extra_flagstrings(dbg,",line_base=-1,",&err);
    check_expected(DW_DLV_OK,res,0,err,-1,
        dbg->de_line_inits.pi_line_base,
        __LINE__);

    err = 0;
    resetdbg(dbg);
    res = _dwarf_log_extra_flagstrings(dbg,",,line_base=-1,,",&err);
    check_expected(DW_DLV_OK,res,0,err,-1,
        dbg->de_line_inits.pi_line_base,
        __LINE__);

    resetdbg(dbg);
    err = 0;
    res = _dwarf_log_extra_flagstrings(dbg,",,line_base =-1,,",&err);
    check_expected(DW_DLV_ERROR,res,err,
        DW_DLE_PRO_INIT_EXTRAS_ERR,
        dbg->de_line_inits.pi_line_base,0,
        __LINE__);

}
static void
test2(Dwarf_P_Debug dbg)
{
    int res = 0;
    int err = 0;

    resetdbg(dbg);
    err = 0;

    res = _dwarf_log_extra_flagstrings(dbg,
        "default_is_stmt=1,line_base=2",&err);
    check_expected(DW_DLV_OK,res,err,0,
        dbg->de_line_inits.pi_default_is_stmt,1,
        __LINE__);
    check_expected(DW_DLV_OK,res,err,0,
        dbg->de_line_inits.pi_line_base,2,
        __LINE__);

    resetdbg(dbg);
    err = 0;
    /* intentional typo here */
    res = _dwarf_log_extra_flagstrings(dbg,
        "minimum_instruction_lengty=3,"
        "maximum_operations_per_instruction=4",&err);
    check_expected(DW_DLV_ERROR,res,
        err,DW_DLE_PRO_INIT_EXTRAS_UNKNOWN,
        dbg->de_line_inits.pi_minimum_instruction_length,
        0,
        __LINE__);
    check_expected(DW_DLV_ERROR,res,
        err,DW_DLE_PRO_INIT_EXTRAS_UNKNOWN,
        dbg->de_line_inits.pi_maximum_operations_per_instruction,
        0,
        __LINE__);

    resetdbg(dbg);
    err = 0;

    res = _dwarf_log_extra_flagstrings(dbg,
        "default_is_stmt=1,"
        "minimum_instruction_length=2,"
        "maximum_operations_per_instruction=3,"
        "opcode_base=4,"
        "line_base=5,"
        "line_range=6,"
        "linetable_version=7,"
        "segment_selector_size=8,"
        "segment_size=9",
        &err);

    check_expected(DW_DLV_OK,res,err,0,
        dbg->de_line_inits.pi_default_is_stmt,1,
        __LINE__);
    check_expected(DW_DLV_OK,res,err,0,
        dbg->de_line_inits.pi_minimum_instruction_length,2,
        __LINE__);
    check_expected(DW_DLV_OK,res,err,0,
        dbg->de_line_inits.pi_maximum_operations_per_instruction,3,
        __LINE__);
    check_expected(DW_DLV_OK,res,err,0,
        dbg->de_line_inits.pi_opcode_base,4,
        __LINE__);
    check_expected(DW_DLV_OK,res,err,0,
        dbg->de_line_inits.pi_line_base,5,
        __LINE__);
    check_expected(DW_DLV_OK,res,err,0,
        dbg->de_line_inits.pi_line_range,6,
        __LINE__);
    check_expected(DW_DLV_OK,res,err,0,
        dbg->de_line_inits.pi_linetable_version,7,
        __LINE__);

    check_expected(DW_DLV_OK,res,err,0,
        dbg->de_line_inits.pi_segment_selector_size,8,
        __LINE__);

    check_expected(DW_DLV_OK,res,err,0,
        dbg->de_line_inits.pi_segment_size,9,
        __LINE__);
}

static void
test3(Dwarf_P_Debug dbg)
{
    int res = 0;
    int err = 0;

    /*  The following identical to a current dwarfgen.cc use. */
    res = _dwarf_log_extra_flagstrings(dbg,
        "opcode_base=13,"
        "minimum_instruction_length=1,"
        "line_base=-1,"
        "line_range=4",&err);
    check_expected(DW_DLV_OK,res,err,0,
        dbg->de_line_inits.pi_opcode_base,13,
        __LINE__);
    check_expected(DW_DLV_OK,res,err,0,
        dbg->de_line_inits.pi_minimum_instruction_length,1,
        __LINE__);
    check_expected(DW_DLV_OK,res,err,0,
        dbg->de_line_inits.pi_line_base,-1,
        __LINE__);
    check_expected(DW_DLV_OK,res,err,0,
        dbg->de_line_inits.pi_line_range,4,
        __LINE__);

}

int main(void)
{

    struct Dwarf_P_Debug_s sdbg;
    Dwarf_P_Debug dbg =  &sdbg;

    resetdbg(dbg);

    test1(dbg);

    resetdbg(dbg);

    test2(dbg);
    resetdbg(dbg);
    test3(dbg);

    if (errcount) {
        return 1;
    }
    return 0;
}
