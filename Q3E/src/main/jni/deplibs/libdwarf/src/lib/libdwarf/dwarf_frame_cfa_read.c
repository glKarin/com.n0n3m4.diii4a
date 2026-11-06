/*
  Copyright (C) 2000-2006 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2007-2026 David Anderson. All Rights Reserved.
  Portions Copyright 2012 SN Systems Ltd. All rights reserved.

  This program is free software; you can redistribute it
  and/or modify it under the terms of version 2.1 of the
  GNU Lesser General Public License as published by the Free
  Software Foundation.

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

  You should have received a copy of the GNU Lesser General
  Public License along with this program; if not, write the
  Free Software Foundation, Inc., 51 Franklin Street - Fifth
  Floor, Boston MA 02110-1301, USA.

*/

/*  This reads DW_CFA_* instructions in a frame table,
    .debug_frame or GNU .eh_frame.

    These instructions are often spoken of as CFI in ordinary
    conversation so I suppose the file name should
    have cfi not cfa...
*/

#include <config.h>

#include <stdlib.h> /* calloc() free() */
#include <string.h> /* memset() */
#include <stdio.h> /* memset() */
#include <limits.h> /* MAX/MIN() */

#if defined(_WIN32) && defined(HAVE_STDAFX_H)
#include "stdafx.h"
#endif /* HAVE_STDAFX_H */

#ifdef HAVE_STDINT_H
#include <stdint.h> /* uintptr_t */
#endif /* HAVE_STDINT_H */

#include "dwarf.h"
#include "libdwarf.h"
#include "dwarf_local_malloc.h"
#include "libdwarf_private.h"
#include "dwarf_base_types.h"
#include "dwarf_opaque.h"
#include "dwarf_alloc.h"
#include "dwarf_error.h"
#include "dwarf_util.h"
#include "dwarf_frame.h"
#include "dwarf_arange.h" /* Using Arange as a way to build a list */
#include "dwarf_string.h"
#include "dwarf_safe_arithmetic.h"

/*  Dwarf_Unsigned is always 64 bits */
#define INVALIDUNSIGNED(x)  ((x) & (((Dwarf_Unsigned)1) << 63))

#define MIN(a,b)  (((a) < (b))? (a):(b))

#if 0  /* dump_frame_rule() FOR DEBUGGING */
/* Used solely for debugging libdwarf. */
static void
dump_frame_rule(char *msg, Dwarf_Regtable_Entry3 *reg_rule)
{
    printf
        ("%s type %s (0x%" DW_PR_XZEROS DW_PR_DUx
        "), is_off %" DW_PR_DUu
        " reg %" DW_PR_DUu " offset %" DW_PR_DSd
        " blocklen %" DW_PR_DUu "\n",
        msg,
        (reg_rule->dw_value_type == DW_EXPR_OFFSET) ?
            "DW_EXPR_OFFSET" :
        (reg_rule->dw_value_type == DW_EXPR_VAL_OFFSET) ?
            "DW_EXPR_VAL_OFFSET" :
        (reg_rule->dw_value_type == DW_EXPR_VAL_EXPRESSION) ?
            "DW_EXPR_VAL_EXPRESSION" :
        (reg_rule->dw_value_type == DW_EXPR_EXPRESSION) ?
            "DW_EXPR_EXPRESSION" : "Unknown",
        (Dwarf_Unsigned) reg_rule->dw_value_type,
        (Dwarf_Unsigned) reg_rule->dw_offset_relevant,
        (Dwarf_Unsigned) reg_rule->dw_regnum,
        (Dwarf_Unsigned)reg_rule->dw_offset,
        (Dwarf_Unsigned)reg_rule->dw_block.bl_len);
    return;
}
#endif /*0*/

/*  Cleans up the in-process linked list of these
    in case of early exit in
    _dwarf_exec_frame_instr.  */
static void
_dwarf_free_dfi_list(Dwarf_Frame_Instr fr)
{
    Dwarf_Frame_Instr cur = fr;
    Dwarf_Frame_Instr next = 0;
    for ( ; cur ; cur = next) {
        next = cur->fi_next;
        free(cur);
    }
}

/*
    _dwarf_exec_frame_instr() is the heart of the
    debug_frame stuff.  Don't even think of reading
    this without reading both the Libdwarf and consumer
    API carefully first.  This function executes frame
    instructions contained in a Cie or an Fde, but does in
    a number of different ways depending on the information
    sought.  start_instr_ptr points to the first byte of
    the frame instruction stream, and final_instr_ptr one
    past the last byte.

    In addition have the structs Dwarf_Regtable_Entry3_s
    and struct Dwarf_Regtable3_s from libdwarf.h on hand
    to refer to.  From dwarf_frame.h: Dwarf_Frame_s,
    Dwarf_Cie_s, and Dwarf_Fde_s.

    The function begins reading instructions
    from the CIE pertaining pertaining to the
    FDE being read, then continues reading
    instructions at start_instr_ptr and ends
    at final_instr_ptr (possibly earlier if
    the call has search_pc TRUE and the pc
    is in a valid row of the virtual frame table).
    initial_loc is the low pc of the FDE, but
    starting with the CIE initial instructions
    to prime the set of instructions.

    The offsets returned in the frame instructions are
    factored.  That is they need to be multiplied by either
    the code_alignment_factor or the data_alignment_factor,
    as appropriate to obtain the actual offset.  This makes
    it possible to expand an instruction stream without the
    corresponding Cie.  However, when an Fde frame instr
    sequence is being expanded there must be a valid Cie
    with a pointer to an initial table row.

    If successful, returns DW_DLV_OK
        And sets returned_count thru the pointer
        if make_instr is TRUE.
        If make_instr is FALSE returned_count
        should NOT be used by the caller (returned_count
        is set to 0 thru the pointer by this routine...)
    If unsuccessful, returns DW_DLV_ERROR
        and sets returned_error to the error code

    It does not do a whole lot of input validation being a private
    function.  Please make sure inputs are valid.

    Arguments:
    make_instr - make list of frame instr? 0/1
    ret_frame_instr -  Ptr to list of ptrs to frame instrs
    search_pc  - Search for a pc value?  0/1
    search_pc_val -  Search for this pc value
    initial_loc - Initial code location value, low pc
        of the fde.
    start_instr_ptr -   Ptr to start of frame instrs.
    final_instr_ptr -   Ptr just past frame instrs.
    table           -   Ptr to struct with last row.
    cie             -   Ptr to Cie used by the Fde.
    iter_data       - data used when iterating using a
        callback function.

    (Option 1) If make_instr is TRUE, it makes a list
    of pointers to Dwarf_Frame_Op structures containing
    an easy to parse intermediate language representing
    the frame instructions involved.  A pointer to this
    list is returned in ret_frame_instr.  Make_instr is
    TRUE only when a list of frame instructions is to be
    returned.  In this case since we are not interested in
    the contents of the table, the input Cie can be NULL.
    This is the only case where the input Cie can be NULL.
    The table argument is NULL.  Only called this way
    from dwarf_expand_frame_instructions()
    Returns the list by setting *ret_frame_instr_head
    and *returned_frame_instr_count.

    (Option 2) if  iter_data is non-null  we are not
    returning data of interest here, but we are calling
    a user-written function to return row user data.
    We will iterate through all the rows in the fde  and
    call the callback function with each row.  Only called
    this way from dwarf_iterate_fde_all_regs3().

    If neither of the above applies:

    (Option 3) If search_pc is non-zero, frame instructions
    are executed till either a location is reached that
    is greater than the search_pc_val provided, or all
    instructions are executed.  At this point the last
    row of the table generated is returned in a structure.
    A pointer to this structure is supplied in table.

        if (has_more_rows and subsequent_pc are non-null) then:
        *has_more_rows is set TRUE if there are instruction
        bytes following the detection of search_over.
        If all the instruction bytes have been seen
        then *has_more_rows is set FALSE.

        If *has_more_rows is TRUE then *subsequent_pc
        is set to the pc value that is the following
        row in the table (so callers can easily iterate
        through all rows in the Frame).

    (Option 4) This function is also used to create the
    initial table row defined by a Cie and save it in
    the CIE ci_initial_table field.  In this case, the
    search_pc and search_pc_val are 0.  Typically an
    immediately following libdwarf call to the function
    will have search_pc as TRUE.

    Different cies may have distinct address-sizes, so the cie
    is used to determine address-size.
*/

/*  Here we need to populate Dwarf_Regtable3,
    The public regtable, with the cfa row and with
    the one row of registers. */
static int
_dwarf_emit_row(
    struct Dwarf_Allreg_Args_s *iter_data,
    Dwarf_Addr row_pc,
    Dwarf_Bool has_more_rows,
    Dwarf_Addr subsequent_pc,
    int        *errcode)
{
    int res = 0;
    Dwarf_Frame fr = 0;
    Dwarf_Regtable3 *rt = 0;

    fr = iter_data->aa_frameregtable;
    if (row_pc != fr->fr_loc) {
        *errcode = DW_DLE_FRAME_ITERATOR_ERR;
        return DW_DLV_ERROR;
    }
    rt = fr->fr_regtable;
    res = iter_data->aa_callback(
        rt,row_pc,
        has_more_rows,subsequent_pc,
        iter_data->aa_user_data);
    if (res == DW_DLV_ERROR) {
        *errcode = DW_DLE_FRAME_ITERATOR_ERR;
        return DW_DLV_ERROR;
    }
    return res;
}

/*
Option 1:
If table is NULL and make_instr is TRUE and we are
called from dwarf_expand_frame_instructions which hands
us instr_start and instr_end. we allocate our own
table here. We discard the table we created before
returning.

If table is NULL and  iterator_data is true then
table is non-null soiterator call requesting
callbacks. We allocate our own table here,
and on return we discard the table we created.

Otherwise search_pc is TRUE

*/

int
_dwarf_exec_frame_instr(Dwarf_Bool make_instr,
    Dwarf_Bool     search_pc,
    Dwarf_Addr     search_pc_val,
    Dwarf_Addr     initial_loc,
    Dwarf_Small   *start_instr_ptr,
    Dwarf_Small   *final_instr_ptr,
    Dwarf_Frame    table, /* ASSERT non-null */
    Dwarf_Cie      cie,
    Dwarf_Debug    dbg,
    Dwarf_Unsigned reg_num_of_cfa,
    Dwarf_Bool    *has_more_rows,
    Dwarf_Addr    *subsequent_pc,
    Dwarf_Frame_Instr_Head *ret_frame_instr_head /*for make_instr*/,
    Dwarf_Unsigned *returned_frame_instr_count,
    struct Dwarf_Allreg_Args_s *iter_data,
    Dwarf_Error *error)
{
/*  The following macro depends on macreg and
    machigh_reg both being unsigned to avoid
    unintended behavior and to avoid compiler warnings when
    high warning levels are turned on.  To avoid
    truncation turning a bogus large value into a smaller
    sensible-seeming value we use Dwarf_Unsigned for register
    numbers. */
#define ERROR_IF_REG_NUM_TOO_HIGH(macreg,machigh_reg)        \
    do {                                                     \
        if ((macreg) >= (machigh_reg)) {                     \
            SER(DW_DLE_DF_REG_NUM_TOO_HIGH); \
        }                                                    \
    } /*CONSTCOND */ while (0)
#define FREELOCALMALLOC                  \
        _dwarf_free_dfi_list(ilisthead); \
        ilisthead =0;                    \
        free(dfi); dfi = 0
/* SER === SIMPLE_ERROR_RETURN */
#define SER(code)                     \
        FREELOCALMALLOC;              \
        _dwarf_error(dbg,error,(code)); \
        return DW_DLV_ERROR
#define SERSTRING(code,m)             \
        FREELOCALMALLOC;              \
        _dwarf_error_string(dbg,error,(code),m); \
        return DW_DLV_ERROR
/*  m must be a quoted string */
#define SERINST(m)                    \
        FREELOCALMALLOC;              \
        _dwarf_error_string(dbg,error,DW_DLE_ALLOC_FAIL, \
            "DW_DLE_ALLOC_FAIL: " m); \
        return DW_DLV_ERROR

#define UPDATE_STD_RULE(reg_no,offrel,valtype,          \
        regnum,result)                                  \
        localregtab[reg_no].dw_offset_relevant = offrel;\
        localregtab[reg_no].dw_value_type = valtype;    \
        localregtab[reg_no].dw_regnum = (Dwarf_Half)regnum; \
        localregtab[reg_no].dw_offset = result

    /*  Sweeps the frame instructions. */
    Dwarf_Small *instr_ptr = 0;
    Dwarf_Frame_Instr dfi = 0;

    /*  Register numbers not limited to just 255,
        thus not using Dwarf_Small.  */
    typedef Dwarf_Unsigned reg_num_type;

    Dwarf_Unsigned factored_N_value = 0;
    Dwarf_Signed signed_factored_N_value = 0;
    Dwarf_Addr current_loc = initial_loc;       /* code location/
        pc-value corresponding to the frame instructions.
        Starts at zero when the caller has no value to pass in. */

    /*  Must be min de_pointer_size bytes and must be at least 4 */
    Dwarf_Unsigned adv_loc = 0;

    /*  localregtab contains the result of all operations on
        registers up to the point where we stop the search
        or run out of instructions. */
    Dwarf_Unsigned         reg_count =
        dbg->de_frame_reg_rules_entry_count;
    Dwarf_Regtable3       *regtab3 =  0;
    Dwarf_Regtable_Entry3 *cfa_reg =  0;
    Dwarf_Regtable_Entry3 *localregtab = 0;

    /*  This is used to end executing frame instructions.  */
    /*  Becomes TRUE when search_pc is TRUE and current_loc */
    /*  is greater than search_pc_val.  */
    Dwarf_Bool search_over = FALSE;

    Dwarf_Addr possible_subsequent_pc = 0;

    Dwarf_Half address_size = (cie)? cie->ci_address_size:
        dbg->de_pointer_size;

    /*  Stack_table points to the row (Dwarf_Frame ie) being
        pushed or popped by a remember or restore instruction.
        Top_stack points to
        the top of the stack of rows. */
    Dwarf_Frame stack_table = NULL;
    Dwarf_Frame top_stack = NULL;

    /*  These are used only when make_instr is TRUE. Curr_instr is a
        poineer to the current frame instruction executed.
        Curr_instr_ptr, head_instr_list, and curr_instr_list are
        used to form a chain of Dwarf_Frame_Op structs.
        Dealloc_instr_ptr is
        used to deallocate the structs used to form the chain.
        Head_instr_block points to a contiguous list of
        pointers to the
        Dwarf_Frame_Op structs executed. */
    /*  Build single linked list of instrs, and
        at end turn into array. */
    Dwarf_Frame_Instr ilisthead = 0;
    Dwarf_Frame_Instr *ilistlastptr = &ilisthead;
    /*  Counts the number of frame instructions
        in the returned instrs if instruction
        details are asked for. Else 0. */
    Dwarf_Unsigned instr_count = 0;

    /*  These are the alignment_factors taken from the Cie provided.
        When no input Cie is provided they are set to 1, because only
        factored offsets are required. */
    Dwarf_Signed code_alignment_factor = 1;
    Dwarf_Signed data_alignment_factor = 1;

    /*  This flag indicates when an actual alignment factor
        is needed.
        So if a frame instruction that computes an offset
        using an alignment factor is encountered when this
        flag is set, an error is returned because the Cie
        did not have a valid augmentation. */
    Dwarf_Bool need_augmentation = FALSE;
    Dwarf_Unsigned instr_area_length = 0;
    Dwarf_Unsigned i = 0;
    Dwarf_Bool local_has_more_rows = FALSE;

    if (has_more_rows) {
        local_has_more_rows = *has_more_rows;
    }
    if (!table) {
        SERSTRING(DW_DLE_DEBUG_FRAME_NULL,
            "DW_DLE_DEBUG_FRAME_NULL:"
            " meaning table argument null, library error.");
    }
    regtab3 =  table->fr_regtable;
    cfa_reg = &regtab3->rt3_cfa_rule;
    localregtab = regtab3->rt3_rules;
    if (iter_data) {
        iter_data->aa_frameregtable  = table;
    }
    {
        /*  We preload the CFA and rules with the cie
            initial CFA and rules */
        struct Dwarf_Regtable_Entry3_s *t1reg = localregtab;
        if (cie && cie->ci_initial_table != NULL) {
            unsigned curreg =0;
            unsigned minregcount =
                (unsigned)MIN(reg_count,
                cie->ci_initial_table->fr_reg_count);
            struct Dwarf_Regtable_Entry3_s *t2reg =
                cie->ci_initial_table->fr_regtable->rt3_rules;

            for ( ; curreg < minregcount ;
                curreg++, t1reg++, t2reg++) {
                *t1reg = *t2reg;
            }
            *cfa_reg =
                cie->ci_initial_table->fr_regtable->rt3_cfa_rule;
        } else {
            /* No cie table, so just set sensible zeros */
            _dwarf_init_reg_rules_dw3(localregtab,0,reg_count,
                dbg->de_frame_rule_initial_value);
            _dwarf_init_reg_rules_dw3(cfa_reg,0, 1,
                dbg->de_frame_rule_initial_value);
        }
    }
    /*  The idea here is that the code_alignment_factor and
        data_alignment_factor which are needed for certain
        instructions are valid only when the Cie has a proper
        augmentation string. So if the augmentation is not
        right, only Frame instruction can be read. */
    if (cie != NULL && cie->ci_augmentation != NULL) {
        code_alignment_factor = cie->ci_code_alignment_factor;
        data_alignment_factor = cie->ci_data_alignment_factor;
    } else {
        need_augmentation = !make_instr;
    }
    instr_ptr = start_instr_ptr;
    instr_area_length = (uintptr_t)
        (final_instr_ptr - start_instr_ptr);
    /* LOOP TOP */
    while ((instr_ptr < final_instr_ptr) && (!search_over)) {
        Dwarf_Small   instr = 0;
        Dwarf_Small   opcode = 0;
        reg_num_type  reg_no = 0;
        Dwarf_Unsigned adv_pc = 0;
        Dwarf_Off fp_instr_offset = 0;
        Dwarf_Small * base_instr_ptr = 0;

        if (instr_ptr < start_instr_ptr) {
            SERINST("DW_DLE_DF_NEW_LOC_LESS_OLD_LOC: "
                "Following instruction bytes we find impossible "
                "decrease in a pointer");
        }
        fp_instr_offset = instr_ptr - start_instr_ptr;
        if (instr_ptr >= final_instr_ptr) {
            SER(DW_DLE_DF_FRAME_DECODING_ERROR);
            return DW_DLV_ERROR;
        }
        instr = *(Dwarf_Small *) instr_ptr;
        instr_ptr += sizeof(Dwarf_Small);
        base_instr_ptr = instr_ptr;
        if ((instr & 0xc0) == 0x00) {
            opcode = instr;     /* is really extended op */
        } else {
            opcode = instr & 0xc0;      /* is base op */
        }
        if (make_instr) {
            dfi = calloc(1,sizeof(*dfi));
            if (!dfi) {
                SERINST("DW_CFA_advance_loc out of memory");
            }
            dfi->fi_op = opcode;
            dfi->fi_instr_offset = fp_instr_offset;
            dfi->fi_fields = "";
        }
        switch (opcode) {
        case DW_CFA_lo_user: {
            if (make_instr) {
                dfi->fi_fields = "";
            }
        }
        break;
        case DW_CFA_advance_loc: {
            Dwarf_Unsigned adv_pc_val = 0;
            int alres = 0;

            /* base op */
            adv_pc_val = instr &DW_FRAME_INSTR_OFFSET_MASK;
            if (need_augmentation) {
                SER(DW_DLE_DF_NO_CIE_AUGMENTATION);
            }

            /* CHECK OVERFLOW */
            alres = _dwarf_uint64_mult(adv_pc_val,
                code_alignment_factor,&adv_pc);
            if (alres == DW_DLV_ERROR) {
                _dwarf_error_string(dbg,error,
                    DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "unsigned 64bit multiply overflow");
                FREELOCALMALLOC;
                return DW_DLV_ERROR;
            }
            if (INVALIDUNSIGNED(adv_pc)) {
                SERSTRING(DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "negative new location");
            }

            alres = _dwarf_uint64_add(current_loc,
                (Dwarf_Unsigned)adv_pc,
                &possible_subsequent_pc);
            if (alres == DW_DLV_ERROR) {
                SERSTRING(DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "add overflowed");
            }
            table->fr_loc = current_loc;
            if (iter_data && instr_ptr > start_instr_ptr) {
                int errcode = 0;
                local_has_more_rows =
                    (instr_ptr < final_instr_ptr)?1:0;
                alres = _dwarf_emit_row(iter_data,
                    current_loc,
                    local_has_more_rows,
                    possible_subsequent_pc,
                    &errcode);
                if (alres == DW_DLV_ERROR) {
                    SER(errcode);
                }
                current_loc = possible_subsequent_pc;
                continue;
            }
            search_over = search_pc &&
                (possible_subsequent_pc > search_pc_val);
            /* If gone past pc needed, retain old pc.  */
            if (!search_over) {
                current_loc = possible_subsequent_pc;
            }
            if (make_instr) {
                dfi->fi_fields = "uc";
                dfi->fi_u0 = adv_pc_val;
                dfi->fi_code_align_factor = code_alignment_factor;
            }
            }
            break;
        case DW_CFA_offset: {  /* base op */
            /*  The offset is unsigned per the Standard */
            int adres = 0;
            Dwarf_Signed result = 0;
            reg_no = (reg_num_type) (instr &
                DW_FRAME_INSTR_OFFSET_MASK);
            ERROR_IF_REG_NUM_TOO_HIGH(reg_no, reg_count);
            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &factored_N_value,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            if (need_augmentation) {
                SER( DW_DLE_DF_NO_CIE_AUGMENTATION);
            }
            if (INVALIDUNSIGNED(factored_N_value)) {
                SERSTRING(DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "negative factored_N_value location");
            }
            /*  CHECK OVERFLOW */
            adres = _dwarf_int64_mult(
                (Dwarf_Signed)factored_N_value,
                data_alignment_factor, &result,dbg,error);
            if (adres == DW_DLV_ERROR) {
                FREELOCALMALLOC;
                return DW_DLV_ERROR;
            }
            /*  dw_offset should be signed for most
                purposes, but was not
                when initially defined in 1993.
                Not sure what range of values should
                be allowed.
                For DW_CFA_offset the offset value is
                defined as unsigned per DWARF standard. */
            localregtab[reg_no].dw_offset = (Dwarf_Unsigned)result;
            localregtab[reg_no].dw_offset_relevant = 1;
            localregtab[reg_no].dw_regnum =
                (Dwarf_Half)reg_num_of_cfa;
            localregtab[reg_no].dw_value_type = DW_EXPR_OFFSET;
            if (make_instr) {
                dfi->fi_fields = "rud";
                dfi->fi_u0 = reg_no;
                dfi->fi_u1 = factored_N_value;
                dfi->fi_data_align_factor =
                    data_alignment_factor;
            }
            }
            break;
        case DW_CFA_restore: { /* base op */
            /* Restoring a single register */
            reg_no = (instr & DW_FRAME_INSTR_OFFSET_MASK);
            ERROR_IF_REG_NUM_TOO_HIGH(reg_no, reg_count);

            if (cie != NULL && cie->ci_initial_table != NULL) {
                Dwarf_Regtable_Entry3 * rt = 0;

                rt = cie->ci_initial_table->
                    fr_regtable->rt3_rules;
                localregtab[reg_no] = rt[reg_no];
            } else if (!make_instr) {
                SER(DW_DLE_DF_MAKE_INSTR_NO_INIT);
            }
            if (make_instr) {
                dfi->fi_fields = "r";
                dfi->fi_u0 = reg_no;
            }
            }
            break;
        case DW_CFA_set_loc: {
            Dwarf_Addr new_loc = 0;
            int adres = 0;
            adres=_dwarf_read_unaligned_ck_wrapper(dbg,
                &new_loc,
                instr_ptr, address_size,
                final_instr_ptr,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            instr_ptr += address_size;
            if (new_loc != 0 && current_loc != 0) {
                /*  Pre-relocation or before current_loc
                    is set the test comparing new_loc
                    and current_loc makes no
                    sense. Testing for non-zero (above) is a way
                    (fallible) to check that current_loc, new_loc
                    are already relocated.  */
                if (new_loc <= current_loc) {
                    /*  Within a frame, address must increase.
                    Seemingly it has not.
                    Seems to be an error. */
                    SER(DW_DLE_DF_NEW_LOC_LESS_OLD_LOC);
                }
            }
            table->fr_loc = current_loc;
            if (iter_data && instr_ptr > start_instr_ptr) {
                int errcode = 0;
                local_has_more_rows =
                    (instr_ptr < final_instr_ptr)?1:0;
                adres = _dwarf_emit_row(iter_data,
                    current_loc,
                    local_has_more_rows,possible_subsequent_pc,
                    &errcode);
                if (adres == DW_DLV_ERROR) {
                    SER(errcode);
                }
                current_loc = possible_subsequent_pc;
                continue;
            }
            search_over = search_pc && (new_loc > search_pc_val);
            /* If gone past pc needed, retain old pc.  */
            possible_subsequent_pc =  new_loc;
            if (!search_over) {
                current_loc = possible_subsequent_pc;
            }
            if (make_instr) {
                dfi->fi_fields = "u";
                dfi->fi_u0 = new_loc;
            }
            }
            break;
        case DW_CFA_advance_loc1:
        {
            int adres = 0;
            Dwarf_Unsigned advloc_val = 0;
            adres=_dwarf_read_unaligned_ck_wrapper(dbg,
                &advloc_val,
                instr_ptr, sizeof(Dwarf_Small),
                final_instr_ptr,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            instr_ptr += sizeof(Dwarf_Small);
            if (need_augmentation) {
                SER(DW_DLE_DF_NO_CIE_AUGMENTATION);
            }
            /* CHECK OVERFLOW */
            adres = _dwarf_uint64_mult(
                advloc_val,
                code_alignment_factor,
                &adv_loc);
            if (adres == DW_DLV_ERROR) {
                _dwarf_error_string(dbg,error,
                    DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "unsigned 64bit multiply overflow");
                FREELOCALMALLOC;
                return adres;
            }

            /* CHECK OVERFLOW add */
            adres = _dwarf_uint64_add(current_loc,adv_loc,
                &possible_subsequent_pc);
            if (adres == DW_DLV_ERROR) {
                SERSTRING(DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "add overflowed calcating subsequent pc");
            }
            table->fr_loc = current_loc;
            if (iter_data && instr_ptr > start_instr_ptr) {
                int errcode = 0;
                local_has_more_rows =
                    (instr_ptr < final_instr_ptr)?1:0;
                adres = _dwarf_emit_row(iter_data,
                    current_loc,
                    local_has_more_rows,possible_subsequent_pc,
                    &errcode);
                if (adres == DW_DLV_ERROR) {
                    SER(errcode);
                }
                current_loc = possible_subsequent_pc;
                continue;
            }
            search_over = search_pc &&
                (possible_subsequent_pc > search_pc_val);
            /* If gone past pc needed, retain old pc.  */
            if (!search_over) {
                current_loc = possible_subsequent_pc;
            }
            if (make_instr) {
                dfi->fi_fields = "uc";
                dfi->fi_u0 = advloc_val;
                dfi->fi_code_align_factor =
                    code_alignment_factor;
            }
            break;
        }

        case DW_CFA_advance_loc2:
        {
            int adres = 0;
            Dwarf_Unsigned advloc_val = 0;
            adres=_dwarf_read_unaligned_ck_wrapper(dbg, &advloc_val,
                instr_ptr, DWARF_HALF_SIZE,
                final_instr_ptr,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            instr_ptr += DWARF_HALF_SIZE;
            if (need_augmentation) {
                SER(DW_DLE_DF_NO_CIE_AUGMENTATION);
            }
            /* CHECK OVERFLOW */
            adres = _dwarf_uint64_mult(
                advloc_val,
                code_alignment_factor,
                &adv_loc);
            if (adres == DW_DLV_ERROR) {
                _dwarf_error_string(dbg,error,
                    DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "unsigned 64bit multiply overflow");
                FREELOCALMALLOC;
                return adres;
            }
            /* CHECK OVERFLOW add */
            if (INVALIDUNSIGNED(adv_loc)) {
                SERSTRING( DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "negative new location");
            }

            /* CHECK OVERFLOW add */
            adres = _dwarf_uint64_add(current_loc,adv_loc,
                &possible_subsequent_pc);
            if (adres == DW_DLV_ERROR) {
                SERSTRING(DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "add overflowed");
            }
            table->fr_loc = current_loc;
            if (iter_data && instr_ptr > start_instr_ptr) {
                int errcode = 0;
                local_has_more_rows =
                    (instr_ptr < final_instr_ptr)?1:0;
                adres =_dwarf_emit_row(iter_data,
                    current_loc,
                    local_has_more_rows,possible_subsequent_pc,
                    &errcode);
                if (adres == DW_DLV_ERROR) {
                    SER(errcode);
                }
                current_loc = possible_subsequent_pc;
                continue;

            }
            search_over = search_pc &&
                (possible_subsequent_pc > search_pc_val);
            /* If gone past pc needed, retain old pc.  */
            if (!search_over) {
                current_loc = possible_subsequent_pc;
            }
            if (make_instr) {
                dfi->fi_fields = "uc";
                dfi->fi_u0 = advloc_val;
                dfi->fi_code_align_factor =
                    code_alignment_factor;
            }
            break;
        }

        case DW_CFA_advance_loc4:
        {
            int adres = 0;
            Dwarf_Unsigned advloc_val = 0;

            adres=_dwarf_read_unaligned_ck_wrapper(dbg, &advloc_val,
                instr_ptr,  DWARF_32BIT_SIZE,
                final_instr_ptr,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            instr_ptr += DWARF_32BIT_SIZE;
            if (need_augmentation) {
                SER(DW_DLE_DF_NO_CIE_AUGMENTATION);
            }
            /* CHECK OVERFLOW */
            adres = _dwarf_uint64_mult(
                advloc_val,
                code_alignment_factor,&adv_loc);
            if (adres == DW_DLV_ERROR) {
                _dwarf_error_string(dbg,error,
                    DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "unsigned 64bit multiply overflow");
                FREELOCALMALLOC;
                return adres;
            }
            /* CHECK OVERFLOW add */
            adres =_dwarf_uint64_add(current_loc,
                adv_loc,&possible_subsequent_pc);
            if (adres == DW_DLV_ERROR) {
                SERSTRING(DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "unsigned add overflowed");
            }

            table->fr_loc = current_loc;
            if (iter_data && instr_ptr > start_instr_ptr) {
                int errcode = 0;
                local_has_more_rows =
                    (instr_ptr < final_instr_ptr)?1:0;
                adres = _dwarf_emit_row(iter_data,
                    current_loc,
                    local_has_more_rows,possible_subsequent_pc,
                    &errcode);
                if (adres == DW_DLV_ERROR) {
                    SER(errcode);
                }
                current_loc = possible_subsequent_pc;
                continue;
            }
            search_over = search_pc &&
                (possible_subsequent_pc > search_pc_val);
            /* If gone past pc needed, retain old pc.  */
            if (!search_over) {
                current_loc = possible_subsequent_pc;
            }
            if (make_instr) {
                dfi->fi_fields = "uc";
                dfi->fi_u0 = advloc_val;
                dfi->fi_code_align_factor =
                    code_alignment_factor;
            }
            break;
        }
        case DW_CFA_MIPS_advance_loc8:
        {
            int adres = 0;
            Dwarf_Unsigned advloc_val = 0;
            adres=_dwarf_read_unaligned_ck_wrapper(dbg, &advloc_val,
                instr_ptr,  DWARF_64BIT_SIZE,
                final_instr_ptr,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            instr_ptr += DWARF_64BIT_SIZE;
            if (need_augmentation) {
                SER(DW_DLE_DF_NO_CIE_AUGMENTATION);
            }
            /* CHECK OVERFLOW */
            adres = _dwarf_uint64_mult(advloc_val,
                code_alignment_factor,&adv_loc);
            if (adres == DW_DLV_ERROR) {
                _dwarf_error_string(dbg,error,
                    DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "unsigned 64bit multiply overflow");
                FREELOCALMALLOC;
                return adres;
            }
            /* CHECK OVERFLOW add */
            adres = _dwarf_uint64_add(current_loc,
                adv_loc,&possible_subsequent_pc);
            if (adres == DW_DLV_ERROR) {
                SERSTRING(DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "unsigned add overflowed");
            }
            table->fr_loc = current_loc;
            if (iter_data && instr_ptr > start_instr_ptr) {
                int errcode = 0;
                local_has_more_rows =
                    (instr_ptr < final_instr_ptr)?1:0;
                adres = _dwarf_emit_row(iter_data,
                    current_loc,
                    local_has_more_rows,possible_subsequent_pc,
                    &errcode);
                if (adres == DW_DLV_ERROR) {
                    SER(errcode);
                }
                current_loc = possible_subsequent_pc;
                continue;
            }
            search_over = search_pc &&
                (possible_subsequent_pc > search_pc_val);
            /* If gone past pc needed, retain old pc.  */
            if (!search_over) {
                current_loc = possible_subsequent_pc;
            }
            if (make_instr) {
                dfi->fi_fields = "u";
                dfi->fi_u0 = advloc_val;
                dfi->fi_code_align_factor =
                    code_alignment_factor;
            }
            break;
        }

        case DW_CFA_offset_extended:
        {
            Dwarf_Unsigned lreg = 0;
            Dwarf_Signed  result = 0;
            int adres = 0;
            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &lreg,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            reg_no = (reg_num_type) lreg;
            ERROR_IF_REG_NUM_TOO_HIGH(reg_no, reg_count);
            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &factored_N_value,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            if (need_augmentation) {
                SER(DW_DLE_DF_NO_CIE_AUGMENTATION);
            }
            if (INVALIDUNSIGNED(factored_N_value)) {
                SERSTRING(DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "negative new location");
            }
            /*  CHECK OVERFLOW */
            adres = _dwarf_int64_mult((Dwarf_Signed)factored_N_value,
                data_alignment_factor, &result,
                dbg,error);
            if (adres == DW_DLV_ERROR) {
                FREELOCALMALLOC;
                return adres;
            }
            UPDATE_STD_RULE(reg_no,1,DW_EXPR_OFFSET,
                reg_num_of_cfa,result);
            if (make_instr) {
                dfi->fi_fields = "rud";
                dfi->fi_u0 = lreg;
                dfi->fi_u1 = factored_N_value;
                dfi->fi_data_align_factor =
                    data_alignment_factor;
            }
            break;
        }

        case DW_CFA_restore_extended:
        {
            /* Restoring a single register (possibly cfa!) */
            Dwarf_Unsigned lreg = 0;
            int adres = 0;

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &lreg,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            reg_no = (reg_num_type) lreg;
            ERROR_IF_REG_NUM_TOO_HIGH(reg_no, reg_count);
            if (cie != NULL && cie->ci_initial_table != NULL) {
                Dwarf_Regtable_Entry3 * rt = 0;

                rt = cie->ci_initial_table->
                    fr_regtable->rt3_rules;
                localregtab[reg_no] = rt[reg_no];
            } else {
                if (!make_instr) {
                    SER(DW_DLE_DF_MAKE_INSTR_NO_INIT);
                }
            }
            if (make_instr) {
                dfi->fi_fields = "r";
                dfi->fi_u0 = lreg;
            }
            break;
        }

        case DW_CFA_undefined:
        {
            Dwarf_Unsigned lreg = 0;
            int adres = 0;

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &lreg,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            reg_no = (reg_num_type) lreg;
            ERROR_IF_REG_NUM_TOO_HIGH(reg_no, reg_count);
            UPDATE_STD_RULE(reg_no,0,DW_EXPR_OFFSET,
                dbg->de_frame_undefined_value_number, 0);
            if (make_instr) {
                dfi->fi_fields = "r";
                dfi->fi_u0 = lreg;
            }
            break;
        }

        case DW_CFA_same_value:
        {
            Dwarf_Unsigned lreg = 0;
            int adres = 0;

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &lreg,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            reg_no = (reg_num_type) lreg;
            ERROR_IF_REG_NUM_TOO_HIGH(reg_no, reg_count);
            UPDATE_STD_RULE(reg_no,0,DW_EXPR_OFFSET,
                dbg->de_frame_same_value_number, 0);
            if (make_instr) {
                dfi->fi_fields = "r";
                dfi->fi_u0 = lreg;
            }
            break;
        }

        case DW_CFA_register:
        {
            Dwarf_Unsigned lreg;
            reg_num_type reg_noA = 0;
            reg_num_type reg_noB = 0;
            int adres = 0;

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
            &lreg,error);
                if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            reg_noA = (reg_num_type) lreg;
            ERROR_IF_REG_NUM_TOO_HIGH(reg_noA, reg_count);
            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &lreg,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            reg_noB = (reg_num_type) lreg;
            if (reg_noB > reg_count) {
                SER(DW_DLE_DF_REG_NUM_TOO_HIGH);
            }
            UPDATE_STD_RULE(reg_noA,0,DW_EXPR_OFFSET,
                reg_noB, 0);
            if (make_instr) {
                dfi->fi_fields = "rr";
                dfi->fi_u0 = reg_noA;
                dfi->fi_u1 = reg_noB;
            }
            break;
        }

        case DW_CFA_remember_state:
        {
            int rsres = 0;
            Dwarf_Regtable3 *regtab = 0;
            stack_table = (Dwarf_Frame)
                _dwarf_get_alloc(dbg, DW_DLA_FRAME, 1);
            if (stack_table == NULL) {
                SER(DW_DLE_DF_ALLOC_FAIL);
            }
            rsres = _dwarf_initialize_frame_table(dbg,
                stack_table,reg_count,
                NULL,error);
            if (rsres == DW_DLV_ERROR) {
                FREELOCALMALLOC;
                return rsres;
            }
            regtab = stack_table->fr_regtable;
            for (i = 0; i < reg_count; i++) {
                regtab->rt3_rules[i] = localregtab[i];
            }
            regtab->rt3_cfa_rule = *cfa_reg;
            if (top_stack != NULL) {
                stack_table->fr_next = top_stack;
            }
            top_stack = stack_table;
            if (make_instr) {
                dfi->fi_fields = "";
            }
            }
            break;
        case DW_CFA_restore_state:
        {
            Dwarf_Regtable3 *regtab = 0;
            if (top_stack == NULL) {
                SER(DW_DLE_DF_POP_EMPTY_STACK);
            }
            stack_table = top_stack;
            top_stack = stack_table->fr_next;
            regtab = stack_table->fr_regtable;

            for (i = 0; i < reg_count; i++) {
                localregtab[i] = regtab->rt3_rules[i];
            }
            *cfa_reg = regtab->rt3_cfa_rule;
            dwarf_dealloc(dbg, stack_table, DW_DLA_FRAME);
            if (make_instr) {
                dfi->fi_fields = "";
            }
            break;
        }

        case DW_CFA_def_cfa:
        {
            Dwarf_Unsigned lreg = 0;
            int adres = 0;
            Dwarf_Off nonfactoredoffset = 0;

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &lreg,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            reg_no = lreg;
            ERROR_IF_REG_NUM_TOO_HIGH(reg_no, reg_count);
            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &nonfactoredoffset,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            if (need_augmentation) {
                SER(DW_DLE_DF_NO_CIE_AUGMENTATION);
            }
            if (INVALIDUNSIGNED(nonfactoredoffset)) {
                SERSTRING(DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "DW_CFA_def_cfa offset unrepresantable "
                    "as signed");
            }
            cfa_reg->dw_offset_relevant = 1;
            cfa_reg->dw_value_type = DW_EXPR_OFFSET;
            cfa_reg->dw_regnum  = (Dwarf_Half)reg_no;
            cfa_reg->dw_offset = (Dwarf_Signed)nonfactoredoffset;
            if (make_instr) {
                dfi->fi_fields = "ru";
                dfi->fi_u0 = lreg;
                dfi->fi_u1 = nonfactoredoffset;
            }
            break;
        }

        case DW_CFA_def_cfa_register:
        {
            Dwarf_Unsigned lreg = 0;
            int adres = 0;

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &lreg,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            reg_no = (reg_num_type) lreg;
            ERROR_IF_REG_NUM_TOO_HIGH(reg_no, reg_count);
            cfa_reg->dw_regnum = (Dwarf_Half)reg_no;
            /*  Do NOT set dw_offset_relevant or
                dw_block(len) or
                dw_offset here.
                See dwarf2/3 spec.  */
            if (make_instr) {
                dfi->fi_fields = "r";
                dfi->fi_u0 = lreg;
            }
            break;
        }

        case DW_CFA_def_cfa_offset:
        {
            int adres = 0;
            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &factored_N_value,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            if (need_augmentation) {
                SER(DW_DLE_DF_NO_CIE_AUGMENTATION);
            }
            /*  Do set dw_offset_relevant here,
                as here factored_N_value counts.  */
            if (INVALIDUNSIGNED(factored_N_value)) {
                SERSTRING(DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "DW_CFA_def_cfa_offset unrepresantable "
                    "as signed");
            }
            cfa_reg->dw_offset_relevant = 1;
            cfa_reg->dw_value_type = DW_EXPR_OFFSET;
            cfa_reg->dw_offset = (Dwarf_Signed)factored_N_value;
            if (make_instr) {
                dfi->fi_fields = "u";
                dfi->fi_u0 = factored_N_value;
            }
            break;
        }
        /*  This is for Metaware with augmentation string HC
            We do not really know what to do with it. */
        case DW_CFA_METAWARE_info:
        {
            int adres = 0;
            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &factored_N_value,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            if (INVALIDUNSIGNED(factored_N_value)) {
                SERSTRING(DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "DW_CFA_METAWARE_info unrepresantable as signed");
            }
            /* Not really known what the value means or is. */
            cfa_reg->dw_offset_relevant = 1;
            cfa_reg->dw_value_type = DW_EXPR_OFFSET;
            cfa_reg->dw_offset = (Dwarf_Signed)factored_N_value;
            if (make_instr) {
                dfi->fi_fields = "u";
                dfi->fi_u0 = factored_N_value;
            }
            break;
        }
        case DW_CFA_nop:
        {
            if (make_instr) {
                dfi->fi_fields = "";
            }
            break;
        }
        /* DWARF3 ops begin here. */
        case DW_CFA_def_cfa_expression: {
            /*  A single DW_FORM_block representing a dwarf
                expression. The form block establishes the way to
                compute the CFA. */
            Dwarf_Unsigned block_len = 0;
            int adres = 0;

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &block_len,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            cfa_reg->dw_offset_relevant = 0;  /* arbitrary */
            cfa_reg->dw_value_type = DW_EXPR_EXPRESSION;
            cfa_reg->dw_block.bl_len = block_len;
            cfa_reg->dw_block.bl_data = instr_ptr;
            if (make_instr) {
                dfi->fi_fields = "b";
                dfi->fi_expr.bl_len = block_len;
                dfi->fi_expr.bl_data = instr_ptr;
            }
            if (block_len >= instr_area_length) {
                SERSTRING(DW_DLE_DF_FRAME_DECODING_ERROR,
                    "DW_DLE_DF_FRAME_DECODING_ERROR: "
                    "DW_CFA_def_cfa_expression "
                    "block len overflows instructions "
                    "available range.");
            }
            instr_ptr += block_len;
            if (instr_area_length < block_len ||
                instr_ptr < base_instr_ptr) {
                SERSTRING(DW_DLE_DF_FRAME_DECODING_ERROR,
                    "DW_DLE_DF_FRAME_DECODING_ERROR: "
                    "DW_CFA_def_cfa_expression "
                    "block len overflows instructions "
                    "available range.");
            }
        }
        break;
        case DW_CFA_expression: {
            /*  An unsigned leb128 value is the first operand (a
            register number). The second operand is single
            DW_FORM_block representing a dwarf expression. The
            evaluator pushes the CFA on the evaluation stack
            then evaluates the expression to compute the value
            of the register contents. */
            Dwarf_Unsigned lreg = 0;
            Dwarf_Unsigned block_len = 0;
            int adres = 0;

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &lreg,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            reg_no = (reg_num_type) lreg;
            ERROR_IF_REG_NUM_TOO_HIGH(reg_no, reg_count);

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &block_len,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            localregtab[lreg].dw_offset_relevant = 0; /* arbitrary */
            localregtab[lreg].dw_value_type = DW_EXPR_EXPRESSION;
            localregtab[lreg].dw_regnum = (Dwarf_Half)reg_no;
            localregtab[lreg].dw_block.bl_data = instr_ptr;
            localregtab[lreg].dw_block.bl_len = block_len;
            if (make_instr) {
                dfi->fi_fields = "rb";
                dfi->fi_u0 = lreg;
                dfi->fi_expr.bl_len = block_len;
                dfi->fi_expr.bl_data = instr_ptr;
            }
            instr_ptr += block_len;
            if (instr_area_length < block_len ||
                instr_ptr < base_instr_ptr) {
                SERSTRING(DW_DLE_DF_FRAME_DECODING_ERROR,
                    "DW_DLE_DF_FRAME_DECODING_ERROR: "
                    "DW_CFA_expression "
                    "block len overflows instructions "
                    "available range.");
            }
            }
            break;
        case DW_CFA_offset_extended_sf: {
            /*  The first operand is an unsigned leb128 register
                number. The second is a signed factored offset.
                Identical to DW_CFA_offset_extended except the
                second operand is signed */
            Dwarf_Unsigned lreg = 0;
            int adres = 0;
            Dwarf_Signed result = 0;

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &lreg,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            reg_no = (reg_num_type) lreg;
            ERROR_IF_REG_NUM_TOO_HIGH(reg_no, reg_count);
            adres = _dwarf_leb128_sword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &signed_factored_N_value,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            if (need_augmentation) {
                SER(DW_DLE_DF_NO_CIE_AUGMENTATION);
            }
            /* CHECK OVERFLOW */
            adres = _dwarf_int64_mult(signed_factored_N_value,
                data_alignment_factor,
                &result,dbg,error);
            if (adres == DW_DLV_ERROR) {
                FREELOCALMALLOC;
                return adres;
            }
            UPDATE_STD_RULE(reg_no,1,DW_EXPR_OFFSET,
                reg_num_of_cfa,result);
            if (make_instr) {
                dfi->fi_fields = "rsd";
                dfi->fi_u0 = lreg;
                dfi->fi_s1 = signed_factored_N_value;
                dfi->fi_data_align_factor =
                    data_alignment_factor;
            }
            }
            break;
        case DW_CFA_def_cfa_sf: {
            /*  The first operand is an unsigned leb128 register
                number. The second is a signed leb128 factored
                offset. Identical to DW_CFA_def_cfa except
                that the second operand is signed
                and factored. */
            Dwarf_Unsigned lreg = 0;
            int adres = 0;
            Dwarf_Signed result =0;

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &lreg,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            reg_no = lreg;
            ERROR_IF_REG_NUM_TOO_HIGH(reg_no, reg_count);
            adres = _dwarf_leb128_sword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &signed_factored_N_value,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            if (need_augmentation) {
                SER(DW_DLE_DF_NO_CIE_AUGMENTATION);
            }
            /*  CHECK OVERFLOW */
            adres = _dwarf_int64_mult(signed_factored_N_value,
                data_alignment_factor,
                &result,dbg,error);
            if (adres == DW_DLV_ERROR) {
                FREELOCALMALLOC;
                return adres;
            }
            cfa_reg->dw_offset_relevant = 1;
            cfa_reg->dw_value_type = DW_EXPR_OFFSET;
            cfa_reg->dw_regnum = (Dwarf_Half)reg_no;
            cfa_reg->dw_offset = result;
            if (make_instr) {
                dfi->fi_fields = "rsd";
                dfi->fi_u0 = lreg;
                dfi->fi_s1 = signed_factored_N_value;
                dfi->fi_data_align_factor =
                    data_alignment_factor;
            }
            }
            break;
        case DW_CFA_def_cfa_offset_sf: {
            /*  The operand is a signed leb128 operand
                representing a factored offset.  Identical to
                DW_CFA_def_cfa_offset except the operand is
                signed and factored. */
            int adres = 0;
            Dwarf_Signed result = 0;

            adres = _dwarf_leb128_sword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &signed_factored_N_value,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            if (need_augmentation) {
                SER(DW_DLE_DF_NO_CIE_AUGMENTATION);
            }
            /*  CHECK OVERFLOW */
            adres = _dwarf_int64_mult(signed_factored_N_value,
                data_alignment_factor,
                &result,dbg,error);
            if (adres == DW_DLV_ERROR) {
                FREELOCALMALLOC;
                return adres;
            }
            /*  Do set dw_offset_relevant here, as
                here factored_N_value counts.  */
            cfa_reg->dw_offset_relevant = 1;
            cfa_reg->dw_value_type = DW_EXPR_OFFSET;
            cfa_reg->dw_offset = result;
            if (make_instr) {
                dfi->fi_fields = "sd";
                dfi->fi_s0 = signed_factored_N_value;
                dfi->fi_data_align_factor =
                    data_alignment_factor;
            }
            }
            break;
        case DW_CFA_val_offset: {
            /*  The first operand is an unsigned leb128 register
                number. The second is a factored unsigned offset.
                Makes the register be a val_offset(N)
                rule with N =
                factored_offset*data_alignment_factor. */
            Dwarf_Unsigned lreg = 0;
            int adres = 0;
            Dwarf_Signed result = 0;

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &lreg,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            reg_no = (reg_num_type) lreg;
            ERROR_IF_REG_NUM_TOO_HIGH(reg_no, reg_count);
            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &factored_N_value,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            if (INVALIDUNSIGNED(factored_N_value) ) {
                SERSTRING(DW_DLE_ARITHMETIC_OVERFLOW,
                    "DW_DLE_ARITHMETIC_OVERFLOW "
                    "in DW_CFA_val_offset factored value");
            }
            if (need_augmentation) {
                SER(DW_DLE_DF_NO_CIE_AUGMENTATION);
            }
            /*  CHECK OVERFLOW */
            adres = _dwarf_int64_mult(
                (Dwarf_Signed)factored_N_value,
                data_alignment_factor,
                &result,dbg,error);
            if (adres == DW_DLV_ERROR) {
                FREELOCALMALLOC;
                return adres;
            }
            UPDATE_STD_RULE(reg_no,1,DW_EXPR_VAL_OFFSET,
                reg_num_of_cfa,result);
            if (make_instr) {
                dfi->fi_fields = "rud";
                dfi->fi_u0 = lreg;
                dfi->fi_u1 = factored_N_value;
                dfi->fi_data_align_factor =
                    data_alignment_factor;
            }
            break;
        }
        case DW_CFA_val_offset_sf: {
            /*  The first operand is an unsigned leb128 register
                number. The second is a factored signed offset.
                Makes the register be a val_offset(N) rule
                with
                N = factored_offset*data_alignment_factor. */
            Dwarf_Unsigned lreg = 0;
            Dwarf_Signed result = 0;
            int adres = 0;

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &lreg,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            ERROR_IF_REG_NUM_TOO_HIGH(reg_no, reg_count);
            adres = _dwarf_leb128_sword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &signed_factored_N_value,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            if (need_augmentation) {
                SER(DW_DLE_DF_NO_CIE_AUGMENTATION);
            }
            adres = _dwarf_int64_mult(signed_factored_N_value,
                data_alignment_factor,&result,
                dbg,error);
            if (adres == DW_DLV_ERROR) {
                FREELOCALMALLOC;
                return adres;
            }
            /*  Do set dw_offset_relevant here, as
                here factored_N_value counts.  */
            localregtab[reg_no].dw_offset_relevant = 1;
            localregtab[reg_no].dw_value_type =
                DW_EXPR_VAL_OFFSET;
            localregtab[reg_no].dw_offset = result;
            if (make_instr) {
                dfi->fi_fields = "rsd";
                dfi->fi_u0 = lreg;
                dfi->fi_s1 = signed_factored_N_value;
                dfi->fi_data_align_factor =
                    data_alignment_factor;
            }
            }
            break;
        case DW_CFA_val_expression: {
            /*  The first operand is an unsigned leb128 register
                number. The second is a DW_FORM_block
                representing a
                DWARF expression. The rule for the register
                number becomes a val_expression(E) rule. */
            Dwarf_Unsigned lreg = 0;
            Dwarf_Unsigned block_len = 0;
            int adres = 0;

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &lreg,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            reg_no = (reg_num_type) lreg;
            ERROR_IF_REG_NUM_TOO_HIGH(reg_no, reg_count);
            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &block_len,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            localregtab[lreg].dw_offset_relevant = 0; /* arbitrary */
            localregtab[lreg].dw_value_type =
                DW_EXPR_VAL_EXPRESSION;
            localregtab[lreg].dw_offset = 0;
            localregtab[lreg].dw_block.bl_data = instr_ptr;
            localregtab[lreg].dw_block.bl_len = block_len;
            if (make_instr) {
                dfi->fi_fields = "rb";
                dfi->fi_u0 = lreg;
                dfi->fi_expr.bl_len = block_len;
                dfi->fi_expr.bl_data = instr_ptr;
            }
            instr_ptr += block_len;
            if (instr_area_length < block_len ||
                instr_ptr < base_instr_ptr) {
                SERSTRING(DW_DLE_DF_FRAME_DECODING_ERROR,
                    "DW_DLE_DF_FRAME_DECODING_ERROR: "
                    "DW_CFA_val_expression "
                    "block len overflows instructions "
                    "available range.");
            }
        }
        break;
        /* END DWARF3 new ops. */

#ifdef DW_CFA_GNU_window_save
        case DW_CFA_GNU_window_save: {
            /*  No information: this just tells
                unwinder to restore
                the window registers from the previous frame's
                window save area */
            if (make_instr) {
                dfi->fi_fields = "";
            }
        }
        break;
#endif
#ifdef  DW_CFA_GNU_args_size
            /*  Single uleb128 is the current arg area
                size in bytes. No
                register exists yet to save this in.
                the value of must be added to
                an x86 register to get the correct
                stack pointer.
                https://lists.nongnu.org/archive/html/
                libunwind-devel/2016-12/msg00004.html
                https://refspecs.linuxfoundation.org/
                LSB_3.0.0/LSB-PDA/LSB-PDA.junk/dwarfext.html
            */
        case DW_CFA_GNU_args_size: {
            Dwarf_Unsigned asize = 0;
            int adres = 0;

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &asize,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            /*  Currently not put into ru_* reg rules, not
                sure what to do with it. */
            /*  This is the total size of arguments
                pushed on the stack.  */
            if (make_instr) {
                dfi->fi_fields = "u";
                dfi->fi_u0 = asize;
            }
        }
        break;
#endif
        case DW_CFA_LLVM_def_aspace_cfa: {
            Dwarf_Unsigned lreg = 0;
            Dwarf_Unsigned offset = 0;
            Dwarf_Unsigned addrspace = 0;
            int adres = 0;

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &lreg,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            ERROR_IF_REG_NUM_TOO_HIGH(lreg, reg_count);
            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &offset,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &addrspace,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            if (make_instr) {
                dfi->fi_fields = "rua";
                dfi->fi_u0 = lreg;
                dfi->fi_u1 = offset;
                dfi->fi_u2 = addrspace;
            }
        }
        break;
        case DW_CFA_LLVM_def_aspace_cfa_sf: {
            Dwarf_Unsigned lreg = 0;
            Dwarf_Signed offset = 0;
            Dwarf_Signed result = 0;
            Dwarf_Unsigned addrspace = 0;
            int adres = 0;

            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &lreg,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            ERROR_IF_REG_NUM_TOO_HIGH(lreg, reg_count);
            adres = _dwarf_leb128_sword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &offset,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            adres = _dwarf_leb128_uword_wrapper(dbg,
                &instr_ptr,final_instr_ptr,
                &addrspace,error);
            if (adres != DW_DLV_OK) {
                FREELOCALMALLOC;
                return adres;
            }
            /*  CHECK OVERFLOW */
            adres = _dwarf_int64_mult(
                (Dwarf_Signed)offset,
                data_alignment_factor,
                &result,dbg,error);
            if (adres == DW_DLV_ERROR) {
                FREELOCALMALLOC;
                return DW_DLV_ERROR;
            }
            UPDATE_STD_RULE(reg_no,1,DW_EXPR_OFFSET,
                reg_num_of_cfa,result);
            if (make_instr) {
                dfi->fi_fields = "rsda";
                dfi->fi_u0 = lreg;
                dfi->fi_s1 = offset;
                dfi->fi_u2 = addrspace;
                dfi->fi_data_align_factor =
                    data_alignment_factor;
            }
        }
        break;
        default: {
            /*  ERROR, we have an opcode we know nothing
                about.  */
            dwarfstring ms;

            dwarfstring_constructor(&ms);
            dwarfstring_append_printf_u(&ms,
                "DW_DLE_DF_FRAME_DECODING_ERROR:  "
                "instr opcode 0x%x unknown",opcode);
            _dwarf_error_string(dbg,error,
                DW_DLE_DF_FRAME_DECODING_ERROR,
                dwarfstring_string(&ms));
            dwarfstring_destructor(&ms);
            FREELOCALMALLOC;
            return DW_DLV_ERROR;
        }
        } /* end switch on opcode */
        if (make_instr) {
            /* add dfi to end of singly-linked list */
            instr_count++;
            (*ilistlastptr) = dfi;
            ilistlastptr = &dfi->fi_next;
            /* dfi itself is stale, the pointer is on the list */
            dfi = 0;
        }
    } /*  end for-loop on ops */ /* LOOP END */
    /*  If frame instruction decoding was right we would
        stop exactly at
        final_instr_ptr. */
    if (instr_ptr > final_instr_ptr) {
        SER(DW_DLE_DF_FRAME_DECODING_ERROR);
    }
    table->fr_loc = current_loc;
    if (iter_data) {
        /* Final row */
        int fres = 0;
        int errcode = 0;

        fres = _dwarf_emit_row(iter_data,
            current_loc,
            FALSE,possible_subsequent_pc,&errcode);
        if (fres == DW_DLV_ERROR) {
            SER(errcode);
        }
        FREELOCALMALLOC;
        return DW_DLV_OK;
    }
    /*  If search_over is set the last instr was an advance_loc
        so we are not done with rows. */
    if ((instr_ptr == final_instr_ptr) && !search_over) {
        if (has_more_rows) {
            *has_more_rows = FALSE;
        }
        if (subsequent_pc) {
            *subsequent_pc = 0;
        }
    } else {
        if (has_more_rows) {
            *has_more_rows = TRUE;
        }
        if (subsequent_pc) {
            *subsequent_pc = possible_subsequent_pc;
        }
    }
    /*  Fill in the actual output table, the space the
        caller passed in. */
    /* Dealloc anything remaining on stack. */
    for (; top_stack != NULL;) {
        stack_table = top_stack;
        top_stack = top_stack->fr_next;
        dwarf_dealloc(dbg, stack_table, DW_DLA_FRAME);
        stack_table = 0;
    }
    if (make_instr) {
        Dwarf_Frame_Instr_Head head = 0;
        Dwarf_Frame_Instr *instrptrs   = 0;
        Dwarf_Frame_Instr *curinstrptr = 0;
        Dwarf_Frame_Instr cur         = 0;
        Dwarf_Frame_Instr next        = 0;
        Dwarf_Unsigned    ic          = 0;

        head= (Dwarf_Frame_Instr_Head)
            _dwarf_get_alloc(dbg, DW_DLA_FRAME_INSTR_HEAD,1);
        if (!head) {
            SER(DW_DLE_DF_ALLOC_FAIL);
        }
        instrptrs= (Dwarf_Frame_Instr *)
            _dwarf_get_alloc(dbg, DW_DLA_LIST,instr_count);
        if (!instrptrs) {
            dwarf_dealloc(dbg,head,DW_DLA_FRAME_INSTR_HEAD);
            SER(DW_DLE_DF_ALLOC_FAIL);
        }
        head->fh_array = instrptrs;
        head->fh_array_count = instr_count;
        head->fh_dbg = dbg;
        head->fh_cie = cie;
        cur = ilisthead;
        curinstrptr = instrptrs;
        for ( ; cur ; ic++,cur = next,++curinstrptr) {
            *curinstrptr = cur;
            next = cur->fi_next;
            cur->fi_next = 0;
        }
        ilisthead = 0;
        if (ic != instr_count) {
            dwarfstring m;

            FREELOCALMALLOC;
            dwarf_dealloc(dbg,head,DW_DLA_FRAME_INSTR_HEAD);
            dwarfstring_constructor(&m);
            dwarfstring_append_printf_u(&m,
                "DW_DLE_DF_FRAME_DECODING_ERROR: "
                "Instruction array build, instr count %u",
                instr_count);
            dwarfstring_append_printf_u(&m,
                " index i %u. Impossible error.",ic);
            _dwarf_error_string(dbg,error,
                DW_DLE_DF_FRAME_DECODING_ERROR,
                dwarfstring_string(&m));
            dwarfstring_destructor(&m);
            return DW_DLV_ERROR;
        }
        *ret_frame_instr_head = head;
        *returned_frame_instr_count =  instr_count;
    } else {
        if (ret_frame_instr_head) {
            *ret_frame_instr_head = 0;
        }
        if (returned_frame_instr_count) {
            *returned_frame_instr_count = 0;
        }
    }
    FREELOCALMALLOC;
    return DW_DLV_OK;
#undef ERROR_IF_REG_NUM_TOO_HIGH
#undef FREELOCALMALLOC
#undef SER
}
