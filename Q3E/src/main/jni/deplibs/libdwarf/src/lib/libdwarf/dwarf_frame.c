/*
  Copyright (C) 2000-2006 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2007-2025 David Anderson. All Rights Reserved.
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

#if 0 /* for debugging only */
static void dump_fde_table(
    Dwarf_Debug dbg,
    Dwarf_Fde fde,
    const char *msg,
    int call_line)
{
    Dwarf_Frame      fr = 0;
    Dwarf_Regtable3  *rt = 0;

    (void)dbg;
    printf("Fde %s call line %d \n",msg, call_line);
    printf("    ptr %p have frame tab %d pc_requested 0x%lx",
        (void *)fde,fde->fd_have_fde_frame_tab,
        (unsigned long)fde->fd_fde_pc_requested);

    fr = &fde->fd_fde_frame_table;
    rt = fr->fr_regtable;
    printf("    member: Frame addr %p regtable %p owns rt? %d\n",
        (void *)fr,(void *)rt,
        fr->fr_owns_regtable);
}
#endif /* debugging only */

#if 0 /* for debugging only */
static int maxtouch = 0;
#define MAXTOUCH 5000
static Dwarf_Unsigned mtarray[MAXTOUCH];
static void
add_alloc(void *a)
{
    Dwarf_Unsigned addr = (Dwarf_Unsigned)a;
    int i = 0;
    int firstzero = -1;
    int lefttofree = 0;
    int found = -1;

    for ( ; i < maxtouch; ++i) {
        if (!mtarray[i]) {
            if (firstzero == -1) {
                firstzero = i;
            }
        }
        if (mtarray[i] == addr) {
            printf("Duplicate alloc entry %d val 0x%08lx\n",
                i,(unsigned long)addr);
            found = i;
            break;
        }
    }
    if (found < 0) {
        if (firstzero != -1) {
            printf("Reuse alloc entry %d val 0x%08lx\n",
                firstzero,(unsigned long)addr);
            mtarray[firstzero] = addr;
        } else {
            printf("New alloc entry %d val 0x%08lx\n",
                maxtouch,(unsigned long)addr);
            mtarray[maxtouch] = addr;
            ++maxtouch;
        }
    }
    for (i = 0 ; i < maxtouch; ++i) {
        if (mtarray[i]) {
            lefttofree++;
        }
    }
    printf("Number of allocations that need be freed: %d\n",
        lefttofree);
}
static void
all_free(void *a)
{
    Dwarf_Unsigned addr = (Dwarf_Unsigned)a;
    int i = 0;
    int found = -1;
    int lefttofree = 0;
    if (a) {
        for ( ; i < maxtouch; ++i) {
            if (mtarray[i] == addr) {
                printf("Alloc freed entry %d val 0x%08lx\n",
                    i,(unsigned long)addr);
                found = i;
                mtarray[i] = 0;
                break;
            }
        }
        if (found < 0) {
            printf("Never found alloc to free! addr 0x%08lx\n",
                (unsigned long)addr);
        }
    }
    printf("To Free");
    for (i = 0 ; i < maxtouch; ++i) {
        if (mtarray[i]) {
            printf(" %p ",(void *)mtarray[i]);
            lefttofree++;
        }
    }
    printf("\n");
    printf("Number of allocs yet to be freed: %d\n",lefttofree);
}
#endif /* calloc/free counts for debugging libdwarf */
#if 0 /* dump_bytes FOR DEBUGGING */
static void
dump_bytes(const char *msg,Dwarf_Small * start, long len)
{
    Dwarf_Small *end = start + len;
    Dwarf_Small *cur = start;
    printf("%s (0x%lx) ",msg,(unsigned long)start);
    for (; cur < end; cur++) {
        printf("%02x", *cur);
    }
    printf("\n");
}
/* Only used for debugging libdwarf. */
static void dump_frame_rule(char *msg,
    Dwarf_Regtable3 *reg_rule);
#endif /*0*/

/*  The rules for register settings are described
    in libdwarf.pdf and the html version.
    (see Special Frame Registers).
*/
static int
regerror(Dwarf_Debug dbg,Dwarf_Error *error,
    int enumber,
    const char *msg)
{
    _dwarf_error_string(dbg,error,enumber,(char *)msg);
    return DW_DLV_ERROR;
}

int
_dwarf_validate_register_numbers(
    Dwarf_Debug dbg,
    Dwarf_Error *error)
{
    if (dbg->de_frame_numbers_validated) {
        return DW_DLV_OK;
    }
    if (dbg->de_frame_same_value_number ==
        dbg->de_frame_undefined_value_number) {
        return regerror(dbg,error,DW_DLE_DEBUGFRAME_ERROR,
            "DW_DLE_DEBUGFRAME_ERROR "
            "same_value == undefined_value");
    }
    if (dbg->de_frame_cfa_col_number ==
        dbg->de_frame_same_value_number) {
        return regerror(dbg,error,DW_DLE_DEBUGFRAME_ERROR,
            "DW_DLE_DEBUGFRAME_ERROR "
            "same_value == cfa_column_number ");
    }
    if (dbg->de_frame_cfa_col_number ==
        dbg->de_frame_undefined_value_number) {
        return regerror(dbg,error,DW_DLE_DEBUGFRAME_ERROR,
            "DW_DLE_DEBUGFRAME_ERROR "
            "undefined_value == cfa_column_number ");
    }
    if ((dbg->de_frame_rule_initial_value !=
        dbg->de_frame_same_value_number) &&
        (dbg->de_frame_rule_initial_value !=
        dbg->de_frame_undefined_value_number)) {
        return regerror(dbg,error,DW_DLE_DEBUGFRAME_ERROR,
            "DW_DLE_DEBUGFRAME_ERROR "
            "initial_value not set to "
            " same_value or undefined_value");
    }
    if (dbg->de_frame_undefined_value_number <=
        dbg->de_frame_reg_rules_entry_count) {
        return regerror(dbg,error,DW_DLE_DEBUGFRAME_ERROR,
            "DW_DLE_DEBUGFRAME_ERROR "
            "undefined_value less than number of registers");
    }
    if (dbg->de_frame_same_value_number <=
        dbg->de_frame_reg_rules_entry_count) {
        return regerror(dbg,error,DW_DLE_DEBUGFRAME_ERROR,
            "DW_DLE_DEBUGFRAME_ERROR "
            "same_value  <= number of registers");
    }
    if (dbg->de_frame_cfa_col_number <=
        dbg->de_frame_reg_rules_entry_count) {
        return regerror(dbg,error,DW_DLE_DEBUGFRAME_ERROR,
            "DW_DLE_DEBUGFRAME_ERROR "
            "cfa_column <= number of registers");
    }
    dbg->de_frame_numbers_validated = TRUE;
    return DW_DLV_OK;
}

int
dwarf_get_frame_section_name(Dwarf_Debug dbg,
    const char **sec_name,
    Dwarf_Error *error)
{
    struct Dwarf_Section_s *sec = 0;

    CHECK_DBG(dbg,error,"dwarf_get_frame_section_name()");
    if (error != NULL) {
        *error = NULL;
    }
    sec = &dbg->de_debug_frame;
    if (sec->dss_size == 0) {
        /* We don't have such a  section at all. */
        return DW_DLV_NO_ENTRY;
    }
    *sec_name = sec->dss_name;
    return DW_DLV_OK;
}

int
dwarf_get_frame_section_name_eh_gnu(Dwarf_Debug dbg,
    const char **sec_name,
    Dwarf_Error *error)
{
    struct Dwarf_Section_s *sec = 0;

    CHECK_DBG(dbg,error,"dwarf_get_frame_section_name_eh_gnu()");
    if (error != NULL) {
        *error = NULL;
    }
    sec = &dbg->de_debug_frame_eh_gnu;
    if (sec->dss_size == 0) {
        /* We don't have such a  section at all. */
        return DW_DLV_NO_ENTRY;
    }
    *sec_name = sec->dss_name;
    return DW_DLV_OK;
}

#if 0 /* printlist() for debugging */
static void
printlist(Dwarf_Frame_Instr x)
{
    int i = 0;
    Dwarf_Frame_Instr nxt = 0;

    printf("=========== print cur list of ptrs\n");
    for ( ; x ; x = nxt,++i) {
        printf("%d  inst 0x%lx nxt 0x%lx\n",
            i,(unsigned long)x,
            (unsigned long)x->fi_next);
        nxt = x->fi_next;
    }
    printf("=========== done cur list of ptrs\n");
}
#endif /*0*/

/*  Depending on version, either read the return address register
    as a ubyte or as an leb number.
    The form of this value changed for DWARF3.
*/
int
_dwarf_get_return_address_reg(Dwarf_Small *frame_ptr,
    int version,
    Dwarf_Debug dbg,
    Dwarf_Byte_Ptr section_end,
    unsigned long *size,
    Dwarf_Unsigned *return_address_register,
    Dwarf_Error *error)
{
    Dwarf_Unsigned uvalue = 0;
    Dwarf_Unsigned leb128_length = 0;

    if (version == 1) {
        if (frame_ptr >= section_end) {
            _dwarf_error(NULL, error, DW_DLE_DF_FRAME_DECODING_ERROR);
            return DW_DLV_ERROR;
        }
        *size = 1;
        uvalue = *(unsigned char *) frame_ptr;
        *return_address_register = uvalue;
        return DW_DLV_OK;
    }
    DECODE_LEB128_UWORD_LEN_CK(frame_ptr,uvalue,leb128_length,
        dbg,error,section_end);
    *size = (unsigned long)leb128_length;
    *return_address_register = uvalue;
    return DW_DLV_OK;
}

/* Trivial consumer function.
*/
int
dwarf_get_cie_of_fde(Dwarf_Fde fde,
    Dwarf_Cie * cie_returned, Dwarf_Error * error)
{
    if (!fde) {
        _dwarf_error(NULL, error, DW_DLE_FDE_NULL);
        return DW_DLV_ERROR;
    }

    *cie_returned = fde->fd_cie;
    return DW_DLV_OK;

}

int
dwarf_get_cie_index(
    Dwarf_Cie cie,
    Dwarf_Signed* indx,
    Dwarf_Error* error )
{
    if (cie == NULL)
    {
        _dwarf_error(NULL, error, DW_DLE_CIE_NULL);
        return DW_DLV_ERROR;
    }

    *indx = cie->ci_index;
    return DW_DLV_OK;
}

/*  For g++ .eh_frame fde and cie.
    the cie id is different as the
    definition of the cie_id in an fde
        is the distance back from the address of the
        value to the cie.
    Or 0 if this is a TRUE cie.
    Non standard dwarf, designed this way to be
    convenient at run time for an allocated
    (mapped into memory as part of the running image) section.
*/
int
dwarf_get_fde_list_eh(Dwarf_Debug dbg,
    Dwarf_Cie ** cie_data,
    Dwarf_Signed * cie_element_count,
    Dwarf_Fde ** fde_data,
    Dwarf_Signed * fde_element_count,
    Dwarf_Error * error)
{
    int res = 0;

    CHECK_DBG(dbg,error,"dwarf_get_fde_list_eh()");
    res = _dwarf_load_section(dbg,
        &dbg->de_debug_frame_eh_gnu,error);
    if (res != DW_DLV_OK) {
        return res;
    }
    res = _dwarf_get_fde_list_internal(dbg,
        cie_data,
        cie_element_count,
        fde_data,
        fde_element_count,
        dbg->de_debug_frame_eh_gnu.dss_data,
        dbg->de_debug_frame_eh_gnu.dss_index,
        dbg->de_debug_frame_eh_gnu.dss_size,
        /* cie_id_value */ 0,
        /* use_gnu_cie_calc= */ 1,
        error);
    return res;
}

/*  For standard dwarf .debug_frame
    cie_id is -1  in a cie, and
    is the section offset in the .debug_frame section
    of the cie otherwise.  Standard dwarf
*/
int
dwarf_get_fde_list(Dwarf_Debug dbg,
    Dwarf_Cie ** cie_data,
    Dwarf_Signed * cie_element_count,
    Dwarf_Fde ** fde_data,
    Dwarf_Signed * fde_element_count,
    Dwarf_Error * error)
{
    int res = 0;

    CHECK_DBG(dbg,error,"dwarf_get_fde_list()");
    res = _dwarf_load_section(dbg, &dbg->de_debug_frame,error);
    if (res != DW_DLV_OK) {
        return res;
    }
    res = _dwarf_get_fde_list_internal(dbg, cie_data,
        cie_element_count,
        fde_data,
        fde_element_count,
        dbg->de_debug_frame.dss_data,
        dbg->de_debug_frame.dss_index,
        dbg->de_debug_frame.dss_size,
        (Dwarf_Unsigned)DW_CIE_ID,
        /* use_gnu_cie_calc= */ 0,
        error);

    return res;
}

/*  Only works on dwarf sections, not eh_frame
    because based on DW_AT_MIPS_fde.
    Given a Dwarf_Die, see if it has a
    DW_AT_MIPS_fde attribute and if so use that
    to get an fde offset.
    Then create a Dwarf_Fde to return thru the ret_fde pointer.
    Also creates a cie (pointed at from the Dwarf_Fde).  */
int
dwarf_get_fde_for_die(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Fde * ret_fde, Dwarf_Error * error)
{
    Dwarf_Attribute attr;
    Dwarf_Unsigned fde_offset = 0;
    Dwarf_Signed signdval = 0;
    Dwarf_Fde new_fde = 0;
    unsigned char *fde_ptr = 0;
    unsigned char *fde_start_ptr = 0;
    unsigned char *fde_end_ptr = 0;
    unsigned char *cie_ptr = 0;
    Dwarf_Unsigned cie_id = 0;
    Dwarf_Half     address_size = 0;

    /* Fields for the current Cie being read. */
    int res = 0;
    int resattr = 0;
    int sdatares = 0;

    struct cie_fde_prefix_s prefix;
    struct cie_fde_prefix_s prefix_c;

    CHECK_DBG(dbg,error,"dwarf_get_fde_for_die()");
    if (!die ) {
        _dwarf_error_string(NULL, error, DW_DLE_DIE_NULL,
            "DW_DLE_DIE_NUL: in dwarf_get_fde_for_die(): "
            "Called with Dwarf_Die argument null");
        return DW_DLV_ERROR;
    }
    resattr = dwarf_attr(die, DW_AT_MIPS_fde, &attr, error);
    if (resattr != DW_DLV_OK) {
        return resattr;
    }
    /* why is this formsdata? FIX */
    sdatares = dwarf_formsdata(attr, &signdval, error);
    if (sdatares != DW_DLV_OK) {
        dwarf_dealloc_attribute(attr);
        return sdatares;
    }
    res = dwarf_get_die_address_size(die,&address_size,error);
    if (res != DW_DLV_OK) {
        dwarf_dealloc_attribute(attr);
        return res;
    }
    dwarf_dealloc_attribute(attr);
    res = _dwarf_load_section(dbg, &dbg->de_debug_frame,error);
    if (res != DW_DLV_OK) {
        return res;
    }
    fde_offset = signdval;
    fde_start_ptr = dbg->de_debug_frame.dss_data;
    fde_ptr = fde_start_ptr + fde_offset;
    fde_end_ptr = fde_start_ptr + dbg->de_debug_frame.dss_size;
    res = _dwarf_validate_register_numbers(dbg,error);
    if (res == DW_DLV_ERROR) {
        return res;
    }

    /*  First read in the 'common prefix' to figure out
        what we are to do with this entry. */
    memset(&prefix_c, 0, sizeof(prefix_c));
    memset(&prefix, 0, sizeof(prefix));
    res = _dwarf_read_cie_fde_prefix(dbg, fde_ptr,
        dbg->de_debug_frame.dss_data,
        dbg->de_debug_frame.dss_index,
        dbg->de_debug_frame.dss_size,
        &prefix,
        error);
    if (res == DW_DLV_ERROR) {
        return res;
    }
    if (res == DW_DLV_NO_ENTRY) {
        return res;
    }
    fde_ptr = prefix.cf_addr_after_prefix;
    cie_id = prefix.cf_cie_id;
    if (cie_id  >=  dbg->de_debug_frame.dss_size ) {
        _dwarf_error_string(dbg, error, DW_DLE_NO_CIE_FOR_FDE,
            "DW_DLE_NO_CIE_FOR_FDE: "
            "dwarf_get_fde_for_die fails as the CIE id "
            "offset is impossibly large");
        return DW_DLV_ERROR;
    }
    /*  Pass NULL, not section pointer, for 3rd argument.
        de_debug_frame.dss_data has no eh_frame relevance. */
    res = _dwarf_create_fde_from_after_start(dbg, &prefix,
        fde_start_ptr,
        dbg->de_debug_frame.dss_size,
        fde_ptr,
        fde_end_ptr,
        /* use_gnu_cie_calc= */ 0,
        /* Dwarf_Cie = */ 0,
        address_size,
        &new_fde, error);
    if (res == DW_DLV_ERROR) {
        return res;
    }
    if (res == DW_DLV_NO_ENTRY) {
        return res;
    }
    /* DW_DLV_OK */

    /*  This is the only situation this is set.
        and is really dangerous. as fde and cie
        are set for dealloc by dwarf_finish(). */
    /*  Now read the cie corresponding to the fde,
        _dwarf_read_cie_fde_prefix checks
        cie_ptr for being within the section. */
    if (cie_id  >=  dbg->de_debug_frame.dss_size ) {
        _dwarf_error_string(dbg, error, DW_DLE_NO_CIE_FOR_FDE,
            "DW_DLE_NO_CIE_FOR_FDE: "
            "dwarf_get_fde_for_die fails as the CIE id "
            "offset is impossibly large");
        return DW_DLV_ERROR;
    }
    cie_ptr = new_fde->fd_section_ptr + cie_id;
    if ((Dwarf_Unsigned)(uintptr_t)cie_ptr  <
        (Dwarf_Unsigned)(uintptr_t)new_fde->fd_section_ptr ||
        (Dwarf_Unsigned)(uintptr_t)cie_ptr <  cie_id) {
        dwarf_dealloc(dbg,new_fde,DW_DLA_FDE);
        new_fde = 0;
        _dwarf_error_string(dbg, error, DW_DLE_NO_CIE_FOR_FDE,
            "DW_DLE_NO_CIE_FOR_FDE: "
            "dwarf_get_fde_for_die fails as the CIE id "
            "offset is impossibly large");
        return DW_DLV_ERROR;
    }
    res = _dwarf_read_cie_fde_prefix(dbg, cie_ptr,
        dbg->de_debug_frame.dss_data,
        dbg->de_debug_frame.dss_index,
        dbg->de_debug_frame.dss_size,
        &prefix_c, error);
    if (res == DW_DLV_ERROR) {
        dwarf_dealloc(dbg,new_fde,DW_DLA_FDE);
        new_fde = 0;
        return res;
    }
    if (res == DW_DLV_NO_ENTRY) {
        dwarf_dealloc(dbg,new_fde,DW_DLA_FDE);
        new_fde = 0;
        return res;
    }

    cie_ptr = prefix_c.cf_addr_after_prefix;
    cie_id = prefix_c.cf_cie_id;

    if (cie_id == (Dwarf_Unsigned)DW_CIE_ID) {
        int res2 = 0;
        Dwarf_Cie new_cie = 0;

        /*  Pass NULL, not section pointer, for 3rd argument.
            de_debug_frame.dss_data has no eh_frame relevance. */
        res2 = _dwarf_create_cie_from_after_start(dbg,
            &prefix_c,
            fde_start_ptr,
            cie_ptr,
            fde_end_ptr,
            /* cie_count= */ 0,
            /* use_gnu_cie_calc= */
            0, &new_cie, error);
        if (res2 != DW_DLV_OK) {
            dwarf_dealloc(dbg, new_fde, DW_DLA_FDE);
            return res;
        }
        new_fde->fd_cie = new_cie;
        new_fde->fd_fde_owns_cie = TRUE;
    } else {
        dwarf_dealloc(dbg,new_fde,DW_DLA_FDE);
        new_fde = 0;
        _dwarf_error_string(dbg, error, DW_DLE_NO_CIE_FOR_FDE,
            "DW_DLE_NO_CIE_FOR_FDE: "
            "The CIE id is not a true cid id. Corrupt DWARF.");
        return DW_DLV_ERROR;
    }
    *ret_fde = new_fde;
    return DW_DLV_OK;
}

int
dwarf_get_fde_range(Dwarf_Fde fde,
    Dwarf_Addr * low_pc,
    Dwarf_Unsigned * func_length,
    Dwarf_Byte_Ptr * fde_bytes,
    Dwarf_Unsigned * fde_byte_length,
    Dwarf_Off * cie_offset,
    Dwarf_Signed * cie_index,
    Dwarf_Off * fde_offset, Dwarf_Error * error)
{
    Dwarf_Debug dbg;

    if (fde == NULL) {
        _dwarf_error(NULL, error, DW_DLE_FDE_NULL);
        return DW_DLV_ERROR;
    }

    dbg = fde->fd_dbg;
    if (IS_INVALID_DBG(dbg)) {
        _dwarf_error_string(NULL, error, DW_DLE_FDE_DBG_NULL,
            "DW_DLE_FDE_DBG_NULL: Either null or it contains"
            "a stale Dwarf_Debug pointer");
        return DW_DLV_ERROR;
    }
    /*  We have always already done the section load here,
        so no need to load the section. We did the section
        load in order to create the
        Dwarf_Fde pointer passed in here. */
    if (low_pc != NULL)
        *low_pc = fde->fd_initial_location;
    if (func_length != NULL)
        *func_length = fde->fd_address_range;
    if (fde_bytes != NULL)
        *fde_bytes = fde->fd_fde_start;
    if (fde_byte_length != NULL)
        *fde_byte_length = fde->fd_length;
    if (cie_offset != NULL)
        *cie_offset = fde->fd_cie_offset;
    if (cie_index != NULL)
        *cie_index = fde->fd_cie_index;
    if (fde_offset != NULL)
        *fde_offset = fde->fd_fde_start - fde->fd_section_ptr;

    return DW_DLV_OK;
}

/*  IRIX specific function.   The exception tables
    have C++ destructor information and are
    at present undocumented.  */
int
dwarf_get_fde_exception_info(Dwarf_Fde fde,
    Dwarf_Signed *
    offset_into_exception_tables,
    Dwarf_Error * error)
{
    Dwarf_Debug dbg;

    dbg = fde->fd_dbg;
    if (IS_INVALID_DBG(dbg)) {
        _dwarf_error_string(NULL, error, DW_DLE_FDE_DBG_NULL,
            "DW_DLE_FDE_DBG_NULL: Either null or it contains"
            "a stale Dwarf_Debug pointer");
        return DW_DLV_ERROR;
    }

    *offset_into_exception_tables =
        fde->fd_offset_into_exception_tables;
    return DW_DLV_OK;
}

/*  A consumer code function.
    Given a CIE pointer, return the normal CIE data thru
    pointers.
    Special augmentation data is not returned here.
*/
int
dwarf_get_cie_info_b(Dwarf_Cie cie,
    Dwarf_Unsigned *bytes_in_cie,
    Dwarf_Small    *ptr_to_version,
    char          **augmenter,
    Dwarf_Unsigned *code_alignment_factor,
    Dwarf_Signed   *data_alignment_factor,
    Dwarf_Half     *return_address_register,
    Dwarf_Byte_Ptr      *initial_instructions,
    Dwarf_Unsigned *initial_instructions_length,
    Dwarf_Half     *offset_size,
    Dwarf_Error    *error)
{
    Dwarf_Debug dbg = 0;

    if (!cie) {
        _dwarf_error(NULL, error, DW_DLE_CIE_NULL);
        return DW_DLV_ERROR;
    }
    dbg = cie->ci_dbg;
    if (IS_INVALID_DBG(dbg)) {
        _dwarf_error_string(NULL, error, DW_DLE_CIE_DBG_NULL,
            "DW_DLE_CIE_DBG_NULL: Either null or it contains"
            "a stale Dwarf_Debug pointer");
        return DW_DLV_ERROR;
    }
    if (ptr_to_version != NULL)
        *ptr_to_version =
            (Dwarf_Small)cie->ci_cie_version_number;
    if (augmenter != NULL)
        *augmenter = cie->ci_augmentation;
    if (code_alignment_factor != NULL)
        *code_alignment_factor = cie->ci_code_alignment_factor;
    if (data_alignment_factor != NULL)
        *data_alignment_factor = cie->ci_data_alignment_factor;
    if (return_address_register != NULL)
        *return_address_register =
            (Dwarf_Half)cie->ci_return_address_register;
    if (initial_instructions != NULL)
        *initial_instructions = cie->ci_cie_instr_start;
    if (initial_instructions_length != NULL) {
        *initial_instructions_length = cie->ci_length +
            cie->ci_length_size +
            cie->ci_extension_size -
            (cie->ci_cie_instr_start - cie->ci_cie_start);
    }
    if (offset_size) {
        *offset_size = cie->ci_length_size;
    }
    *bytes_in_cie = (cie->ci_length);
    return DW_DLV_OK;
}

/* Return the register rules for all registers at
   a given pc. If iterator_data is non-null
   we will be calling this just once using
   the iterator_data and its callback pointer.
*/
int
_dwarf_get_fde_info_for_a_pc_row(Dwarf_Fde fde,
    Dwarf_Addr pc_requested,
    Dwarf_Frame table,
    Dwarf_Unsigned cfa_reg_col_num,
    Dwarf_Bool * has_more_rows,
    Dwarf_Addr * subsequent_pc,
    struct Dwarf_Allreg_Args_s *iterator_data,
    Dwarf_Error * error)
{
    Dwarf_Debug dbg = 0;
    Dwarf_Cie cie = 0;
    int res = 0;

    if (fde == NULL) {
        _dwarf_error(NULL, error, DW_DLE_FDE_NULL);
        return DW_DLV_ERROR;
    }
    dbg = fde->fd_dbg;
    if (IS_INVALID_DBG(dbg)) {
        _dwarf_error(NULL, error, DW_DLE_FDE_DBG_NULL);
        return DW_DLV_ERROR;
    }

    if (pc_requested < fde->fd_initial_location ||
        pc_requested >=
        fde->fd_initial_location + fde->fd_address_range) {
        _dwarf_error(dbg, error, DW_DLE_PC_NOT_IN_FDE_RANGE);
        return DW_DLV_ERROR;
    }

    cie = fde->fd_cie;
    if (!cie->ci_initial_table) {
        /*  We do not have an initial table yet.
            It will be remembered in future calls. */

        Dwarf_Small *instrstart = cie->ci_cie_instr_start;
        Dwarf_Small *instrend = instrstart +cie->ci_length +
            cie->ci_length_size +
            cie->ci_extension_size -
            (cie->ci_cie_instr_start -
            cie->ci_cie_start);
        Dwarf_Regtable_Entry3 *cfa= 0;
        Dwarf_Regtable_Entry3 *rules= 0;
        Dwarf_Frame cieframe = 0;
        Dwarf_Unsigned rules_count =
            dbg->de_frame_reg_rules_entry_count;

        if (instrend > cie->ci_cie_end) {
            _dwarf_error(dbg, error,DW_DLE_CIE_INSTR_PTR_ERROR);
            return DW_DLV_ERROR;
        }
        cieframe = (Dwarf_Frame)_dwarf_get_alloc(dbg,
            DW_DLA_FRAME, 1);
        if (!cieframe) {
            _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
            return DW_DLV_ERROR;
        }
        cie->ci_initial_table = cieframe;

        /*  The initial table is a separate
            allocation, not part of
            argument 'table' */
        res = _dwarf_initialize_frame_table(dbg,cieframe,
            rules_count, NULL,error);
        if (res == DW_DLV_ERROR) {
            dwarf_dealloc(dbg,cieframe,DW_DLA_FRAME);
            cieframe = 0;
            cie->ci_initial_table = 0;
            return res;
        }
        rules = cieframe->fr_regtable->rt3_rules;
        _dwarf_init_reg_rules_dw3(rules,
            0,rules_count ,dbg->de_frame_rule_initial_value);
        cfa = &cieframe->fr_regtable->rt3_cfa_rule;
        _dwarf_init_reg_rules_dw3(cfa,
            0,1,dbg->de_frame_rule_initial_value);
        res = _dwarf_exec_frame_instr( /* make_instr= */ FALSE,
            /* search_pc */ FALSE,
            /* search_pc_val */ 0,
            /* location */ 0,
            instrstart,
            instrend,
            cieframe,
            cie, dbg,
            cfa_reg_col_num,
            has_more_rows,
            subsequent_pc,
            NULL,NULL,
            NULL /* no iterator data, no callback here*/,
            error);
        if (res != DW_DLV_OK) {
            dwarf_dealloc(dbg,cieframe,DW_DLA_FRAME);
            cieframe = 0;
            cie->ci_initial_table = 0;
            return res;
        }
    }
    /*  So we have CIE initial instructions in
        cieframe Dwarf_Regtable3
        So do the next step, exec some frame instrs
        till it is time to stop.  */

    {
        Dwarf_Small *instr_end = fde->fd_length +
            fde->fd_length_size +
            fde->fd_extension_size + fde->fd_fde_start;
        if (instr_end > fde->fd_fde_end) {
            _dwarf_error(dbg, error,DW_DLE_FDE_INSTR_PTR_ERROR);
            return DW_DLV_ERROR;
        }
        /*  This call will pick up cie->initial_table
            instructions from cie, if any exist.  It always
            starts from byte zero of the fde instructions. */
        res = _dwarf_exec_frame_instr( /* make_instr= */ FALSE,
            /* search_pc */ TRUE,
            pc_requested,
            fde->fd_initial_location,
            fde->fd_fde_instr_start,
            instr_end,
            table,
            cie,dbg,
            cfa_reg_col_num,
            has_more_rows,
            subsequent_pc,
            NULL,NULL,
            iterator_data,
            error);
    }
    return res;
}

/*  Called from user code. See libdwarf.h for required
    condition on reg_table passed in.
*/
int
dwarf_get_fde_info_for_all_regs3_b(Dwarf_Fde fde,
    Dwarf_Addr       pc_requested,
    Dwarf_Regtable3 *reg_table,
    Dwarf_Addr      *row_pc,
    Dwarf_Bool      *has_more_rows,
    Dwarf_Addr      *subsequent_pc,
    Dwarf_Error     *error)
{
    Dwarf_Debug              dbg = 0;
    struct Dwarf_Frame_s    *fde_frame_table;
    int                      res = 0;
    /*  Constraints in Dwarf_Regtable3 mean
        a table size (meaning number of registers in a frame)
        must fit in a Dwarf_Half (16 bits unsigned). */
    Dwarf_Unsigned           output_table_real_data_size = 0;

    (void) row_pc;
    FDE_NULL_CHECKS_AND_SET_DBG(fde, dbg);
    fde_frame_table   = &(fde->fd_fde_frame_table);
    output_table_real_data_size = reg_table->rt3_reg_table_size;
    output_table_real_data_size =
        MIN(output_table_real_data_size,
            dbg->de_frame_reg_rules_entry_count);
    res = _dwarf_validate_register_numbers(dbg,error);
    if (res == DW_DLV_ERROR) {
        return res;
    }
    res = _dwarf_initialize_frame_table(dbg, fde_frame_table,
        output_table_real_data_size, reg_table, error);
    if (res != DW_DLV_OK) {
        _dwarf_empty_frame_table(fde_frame_table);
        fde->fd_have_fde_frame_tab = FALSE;
        return res;
    }
    /* Allocate array of internal structs to match,
        in count, what was passed in. */
    /*  _dwarf_get_fde_info_for_a_pc_row will perform
        more sanity checks and create a frame row in
        fde_frame_table. */
    res = _dwarf_get_fde_info_for_a_pc_row(fde, pc_requested,
        fde_frame_table,
        dbg->de_frame_cfa_col_number,
        has_more_rows,subsequent_pc,
        NULL /* No iterator data */,
        error);
    *row_pc = fde_frame_table->fr_loc;
    if (res != DW_DLV_OK) {
        _dwarf_empty_frame_table(fde_frame_table);
        fde->fd_have_fde_frame_tab = FALSE;
        return res;
    }
    _dwarf_empty_frame_table(fde_frame_table);
    return DW_DLV_OK;
}

int
dwarf_get_fde_info_for_all_regs3(Dwarf_Fde fde,
    Dwarf_Addr pc_requested,
    Dwarf_Regtable3 * reg_table,
    Dwarf_Addr * row_pc,
    Dwarf_Error * error)
{
    int res = dwarf_get_fde_info_for_all_regs3_b(fde,pc_requested,
        reg_table,row_pc,NULL,NULL,error);

    return res;
}

/*  Table_column DW_FRAME_CFA_COL is not meaningful.
    Use  dwarf_get_fde_info_for_cfa_reg3_b() to get the CFA.
    Call dwarf_set_frame_cfa_value() to set the correct column
    after calling dwarf_init()
    (DW_FRAME_CFA_COL3 is a sensible column to use).  */
/*  New May 2018.
    If one is tracking the value of a single table
    column through a function, this lets us
    skip to the next pc value easily.

    if pc_requested is a change from the last
    pc_requested on this pc, this function
    returns *has_more_rows and *subsequent_pc
    (null pointers passed are acceptable, the
    assignment through the pointer is skipped
    if the pointer is null).
    Otherwise *has_more_rows and *subsequent_pc
    are not set.

    The offset returned is Unsigned, which was
    always wrong. Cast to Dwarf_Signed to use it.
*/
int
dwarf_get_fde_info_for_reg3_b(Dwarf_Fde fde,
    Dwarf_Half      table_column,
    Dwarf_Addr      requested,
    Dwarf_Small    *value_type,
    Dwarf_Unsigned *offset_relevant,
    Dwarf_Unsigned *register_num,
    Dwarf_Unsigned *offset,
    Dwarf_Block    *block,
    Dwarf_Addr     *row_pc_out,
    Dwarf_Bool     *has_more_rows,
    Dwarf_Addr     *subsequent_pc,
    Dwarf_Error    *error)
{
    Dwarf_Signed soff = 0;
    int res = 0;

    res = dwarf_get_fde_info_for_reg3_c(
        fde,table_column,requested,
        value_type,offset_relevant,
        register_num,&soff,
        block,row_pc_out,has_more_rows,
        subsequent_pc,error);
    if (offset) {
        *offset = (Dwarf_Unsigned)soff;
    }
    return res;
}
/*  New September 2023.
    The same as dwarf_get_fde_info_for_reg3_b() but here
    offset is signed as it should have always been.
*/
int
dwarf_get_fde_info_for_reg3_c(Dwarf_Fde fde,
    Dwarf_Half      table_column,
    Dwarf_Addr      pc_requested,
    Dwarf_Small    *value_type,
    Dwarf_Unsigned *offset_relevant,
    Dwarf_Unsigned *register_num,
    Dwarf_Signed   *offset,
    Dwarf_Block    *block,
    Dwarf_Addr     *row_pc_out,
    Dwarf_Bool     *has_more_rows,
    Dwarf_Addr     *subsequent_pc,
    Dwarf_Error    *error)
{
    struct Dwarf_Frame_s *fde_frame_table =
        &(fde->fd_fde_frame_table);
    int            res = DW_DLV_ERROR;
    Dwarf_Debug    dbg = 0;
    Dwarf_Unsigned table_real_data_size = 0;
    Dwarf_Bool     col_is_cfa = FALSE;

    FDE_NULL_CHECKS_AND_SET_DBG(fde, dbg);
    res = _dwarf_validate_register_numbers(dbg,error);
    if (res == DW_DLV_ERROR) {
        return res;
    }
    if (table_column == dbg->de_frame_cfa_col_number) {
        col_is_cfa = TRUE;
    }
    table_real_data_size = dbg->de_frame_reg_rules_entry_count;
    /*  The second part of the 'if' here is just in case it's not
        inside the table.  For non-MIPS it must be
        outside the table.  It was a mistake
        to put it in the table (as register 0) in 1993.  */
    if (!fde->fd_have_fde_frame_tab  ||
        fde->fd_fde_pc_requested != pc_requested) {
        if (fde->fd_have_fde_frame_tab) {
            /*  The pc_requested changed, so we
                to restart the calculation from cie
                and the first byte of fde instructions */
            _dwarf_empty_frame_table(fde_frame_table);
            fde->fd_have_fde_frame_tab = FALSE;
        }
        /* We do not yet have a frame table */
        res = _dwarf_initialize_frame_table(dbg, fde_frame_table,
            table_real_data_size,NULL, error);
        if (res != DW_DLV_OK) {
            return res;
        }
        if (!col_is_cfa && table_column >= table_real_data_size) {
            _dwarf_empty_frame_table(fde_frame_table);
            fde->fd_have_fde_frame_tab = FALSE;
            _dwarf_error(dbg, error, DW_DLE_FRAME_TABLE_COL_BAD);
            return DW_DLV_ERROR;
        }
    }
    /*  _dwarf_get_fde_info_for_a_pc_row will perform
        more sanity checks and run the instructions
        until stopped.  */
    res = _dwarf_get_fde_info_for_a_pc_row(fde,
        pc_requested, fde_frame_table,
        dbg->de_frame_cfa_col_number,
        has_more_rows,subsequent_pc,
        NULL /* no iterator data */,
        error);
    if (res != DW_DLV_OK) {
        _dwarf_empty_frame_table(fde_frame_table);
        fde->fd_have_fde_frame_tab = FALSE;
        return res;
    }
    if (row_pc_out) {
        *row_pc_out = fde_frame_table->fr_loc;
    }
    /*  Without value_type the data cannot be understood,
        so we insist on it being present, we don't test it. */
    if (col_is_cfa) {
        Dwarf_Regtable3 * regt = fde_frame_table->fr_regtable;
        if (register_num) {
            *register_num = regt->rt3_cfa_rule.dw_regnum;
        }
        if (offset) {
            *offset = (Dwarf_Signed)regt->rt3_cfa_rule.dw_offset;
        }
        if (block) {
            *block = regt->rt3_cfa_rule.dw_block;
        }
        *value_type =
            regt->rt3_cfa_rule.dw_value_type;
        *offset_relevant =
            regt->rt3_cfa_rule.dw_offset_relevant;
    } else {
        Dwarf_Regtable3 * regt = fde_frame_table->fr_regtable;
        Dwarf_Regtable_Entry3 *rules = regt->rt3_rules;
        if (register_num) {
            *register_num = rules[table_column].dw_regnum;
        }
        if (offset) {
            *offset = (Dwarf_Signed)rules[table_column].dw_offset;
        }
        if (block) {
            *block = rules[table_column].dw_block;
        }
        *value_type = rules[table_column].dw_value_type;
        *offset_relevant = rules[table_column].dw_offset_relevant;
    }
    fde->fd_have_fde_frame_tab = TRUE;
    /*  Memoized to avoid recalculation! */
    fde->fd_fde_pc_requested = pc_requested;
    return DW_DLV_OK;

}

/*
    This deals with the  CFA by not
    making the CFA a column number, which means
    DW_FRAME_CFA_COL3 is, like DW_CFA_SAME_VALUE,
    a special value, not something one uses as an index.

    Call dwarf_set_frame_cfa_value() to set the correct column
    after calling dwarf_init().
    DW_FRAME_CFA_COL3 is a sensible column to use.
*/
int
dwarf_get_fde_info_for_cfa_reg3_b(Dwarf_Fde fde,
    Dwarf_Addr      pc_requested,
    Dwarf_Small    *value_type,
    Dwarf_Unsigned *offset_relevant,
    Dwarf_Unsigned *register_num,
    Dwarf_Unsigned *offset,
    Dwarf_Block    *block,
    Dwarf_Addr     *row_pc_out,
    Dwarf_Bool     *has_more_rows,
    Dwarf_Addr     *subsequent_pc,
    Dwarf_Error    *error)
{
    Dwarf_Signed soff = 0;
    int res = 0;

    res = dwarf_get_fde_info_for_cfa_reg3_c(fde,
        pc_requested, value_type,offset_relevant,
        register_num,&soff,block, row_pc_out,
        has_more_rows,subsequent_pc,error);
    if (offset) {
        *offset = (Dwarf_Unsigned)soff;
    }
    return res;
}
/*
    New September 2023. With the offset argument
    a signed value.  This is more correct, so
    convert from dwarf_get_fde_info_for_cfa_reg3_b
    when convenient.
*/
int
dwarf_get_fde_info_for_cfa_reg3_c(Dwarf_Fde fde,
    Dwarf_Addr      pc_requested,
    Dwarf_Small    *value_type,
    Dwarf_Unsigned *offset_relevant,
    Dwarf_Unsigned *register_num,
    Dwarf_Signed   *offset,
    Dwarf_Block    *block,
    Dwarf_Addr     *row_pc_out,
    Dwarf_Bool     *has_more_rows,
    Dwarf_Addr     *subsequent_pc,
    Dwarf_Error    *error)
{
    int res = DW_DLV_OK;
    Dwarf_Debug dbg = 0;

    FDE_NULL_CHECKS_AND_SET_DBG(fde, dbg);

    res = dwarf_get_fde_info_for_reg3_c(fde,
        (Dwarf_Half)dbg->de_frame_cfa_col_number,
        pc_requested,
        value_type,
        offset_relevant,
        register_num,
        offset,
        block,
        row_pc_out,
        has_more_rows,
        subsequent_pc,
        error);
    return res;
}

/*  Return pointer to the instructions in the dwarf fde.  */
int
dwarf_get_fde_instr_bytes(Dwarf_Fde inFde,
    Dwarf_Small   ** outinstrs,
    Dwarf_Unsigned * outinstrslen,
    Dwarf_Error    * error)
{
    Dwarf_Unsigned len = 0;
    Dwarf_Small *instrs = 0;
    Dwarf_Debug dbg = 0;

    if (!inFde) {
        _dwarf_error(dbg, error, DW_DLE_FDE_NULL);
        return DW_DLV_ERROR;
    }
    dbg = inFde->fd_dbg;
    if (IS_INVALID_DBG(dbg)) {
        _dwarf_error_string(NULL, error, DW_DLE_FDE_DBG_NULL,
            "DW_DLE_FDE_DBG_NULL: Either null or it contains"
            "a stale Dwarf_Debug pointer");
        return DW_DLV_ERROR;
    }
    instrs = inFde->fd_fde_instr_start;
    len = inFde->fd_fde_end - inFde->fd_fde_instr_start;
    *outinstrs = instrs;
    *outinstrslen = len;
    return DW_DLV_OK;
}

/*  Allows getting an fde from its table via an index.
    With more error checking than simply indexing oneself.  */
int
dwarf_get_fde_n(Dwarf_Fde * fde_data,
    Dwarf_Unsigned fde_index,
    Dwarf_Fde * returned_fde, Dwarf_Error * error)
{
    Dwarf_Debug dbg = 0;
    Dwarf_Unsigned fdecount = 0;

    if (fde_data == NULL) {
        _dwarf_error(dbg, error, DW_DLE_FDE_PTR_NULL);
        return DW_DLV_ERROR;
    }

    FDE_NULL_CHECKS_AND_SET_DBG(*fde_data, dbg);
    /* Assumes fde_data table has at least one entry. */
    fdecount = fde_data[0]->fd_is_eh?
        dbg->de_fde_count_eh:dbg->de_fde_count;
    if (fde_index >= fdecount) {
        return DW_DLV_NO_ENTRY;
    }
    *returned_fde = (*(fde_data + fde_index));
    return DW_DLV_OK;
}

/*  Lopc and hipc are extensions to the interface to
    return the range of addresses that are described
    by the returned fde.  */
int
dwarf_get_fde_at_pc(Dwarf_Fde * fde_data,
    Dwarf_Addr pc_of_interest,
    Dwarf_Fde * returned_fde,
    Dwarf_Addr * lopc,
    Dwarf_Addr * hipc, Dwarf_Error * error)
{
    Dwarf_Debug dbg = NULL;
    Dwarf_Fde fde = NULL;
    Dwarf_Fde entryfde = NULL;
    Dwarf_Signed fdecount = 0;

    if (fde_data == NULL) {
        _dwarf_error(NULL, error, DW_DLE_FDE_PTR_NULL);
        return DW_DLV_ERROR;
    }

    /*  Assumes fde_data table has at least one entry. */
    entryfde = *fde_data;
    FDE_NULL_CHECKS_AND_SET_DBG(entryfde, dbg);
    fdecount = entryfde->fd_is_eh?
        dbg->de_fde_count_eh:dbg->de_fde_count;
    {
        /*  The fdes are sorted by their addresses. Binary search to
            find correct fde. */
        Dwarf_Signed low = 0;
        Dwarf_Signed high = fdecount - 1L;
        Dwarf_Signed middle = 0;
        Dwarf_Fde cur_fde;

        while (low <= high) {
            middle = (low + high) / 2;
            cur_fde = fde_data[middle];
            if (pc_of_interest < cur_fde->fd_initial_location) {
                high = middle - 1;
            } else if (pc_of_interest >=
                (cur_fde->fd_initial_location +
                cur_fde->fd_address_range)) {
                low = middle + 1;
            } else {
                fde = fde_data[middle];
                break;
            }
        }
    }

    if (fde) {
        if (lopc != NULL)
            *lopc = fde->fd_initial_location;
        if (hipc != NULL)
            *hipc =
                fde->fd_initial_location + fde->fd_address_range - 1;
        *returned_fde = fde;
        return DW_DLV_OK;
    }

    return DW_DLV_NO_ENTRY;
}

/*  Expands a single frame instruction block
    from a specific cie or fde into a
    Dwarf_Frame_Instr_Head.

    Call dwarf_set_frame_cfa_value() to set the correct column
    after calling dwarf_init().
    DW_FRAME_CFA_COL3 is a sensible column to use.
*/
int
dwarf_expand_frame_instructions(Dwarf_Cie cie,
    Dwarf_Small   *instruction,
    Dwarf_Unsigned i_length,
    Dwarf_Frame_Instr_Head * returned_instr_head,
    Dwarf_Unsigned * returned_instr_count,
    Dwarf_Error * error)
{
    int res = DW_DLV_ERROR;
    Dwarf_Debug dbg = 0;
    Dwarf_Small * instr_start = instruction;
    Dwarf_Small * instr_end = (Dwarf_Small *)instruction + i_length;
    Dwarf_Unsigned rules_count = 0;
    Dwarf_Frame   cieframe = 0;

    if (cie == 0) {
        _dwarf_error(NULL, error, DW_DLE_DBG_NULL);
        return DW_DLV_ERROR;
    }
    dbg = cie->ci_dbg;
    if (!returned_instr_head  || !returned_instr_count) {
        _dwarf_error_string(dbg, error, DW_DLE_RET_OP_LIST_NULL,
            "DW_DLE_RET_OP_LIST_NULL: "
            "Calling dwarf_expand_frame_instructions without "
            "a non-NULL Dwarf_Frame_Instr_Head pointer and "
            "count pointer seems wrong.");
        return DW_DLV_ERROR;
    }
    if ( instr_end < instr_start) {
        /*  Impossible unless there was wraparond somewhere and
            we missed it. */
        _dwarf_error(dbg, error,DW_DLE_FDE_INSTR_PTR_ERROR);
        return DW_DLV_ERROR;
    }
    rules_count = dbg->de_frame_reg_rules_entry_count;
    cieframe = (Dwarf_Frame)_dwarf_get_alloc(dbg,
        DW_DLA_FRAME, 1);
    if (!cieframe) {
        _dwarf_error_string(dbg,error, DW_DLE_DF_ALLOC_FAIL,
            "DW_DLE_DF_ALLOC_FAIL: Failing setup of frame"
            " in dwarf_expand_frame_instructions");
        return DW_DLV_ERROR;
    }
    res = _dwarf_initialize_frame_table(dbg,
        cieframe, rules_count, NULL,error);
    if (res != DW_DLV_OK ) {
        dwarf_dealloc(dbg,cieframe,DW_DLA_FRAME);
        return res;
    }
    /*  We will not touch the rules here, but they
        now exist. */
    res = _dwarf_exec_frame_instr( /* make_instr= */ TRUE,
        /* search_pc */ FALSE,
        /* search_pc_val */ 0,
        /* location */ 0,
        instr_start,
        instr_end,
        cieframe,
        cie,
        dbg,
        dbg->de_frame_cfa_col_number,
        /* has more rows */0,
        /* subsequent_pc */0,
        returned_instr_head,
        returned_instr_count,
        NULL /* no iterator data, no callback here */,
        error);
    dwarf_dealloc(dbg,cieframe,DW_DLA_FRAME);
    return res;
}

/*  Call to access  a single CFA frame instruction.
    The 2021 DW_CFA_LLVM addition for hetrogenous
    debugging has a third field,  an address space
    value.  */
int
dwarf_get_frame_instruction(Dwarf_Frame_Instr_Head head,
    Dwarf_Unsigned    instr_index,
    Dwarf_Unsigned  * instr_offset_in_instrs,
    Dwarf_Small     * cfa_operation,
    const char     ** fields_description,
    Dwarf_Unsigned  * u0,
    Dwarf_Unsigned  * u1,
    Dwarf_Signed    * s0,
    Dwarf_Signed    * s1,
    Dwarf_Unsigned  * code_alignment_factor,
    Dwarf_Signed    * data_alignment_factor,
    Dwarf_Block     * expression_block,
    Dwarf_Error     * error)
{
    Dwarf_Unsigned aspace = 0;
    return dwarf_get_frame_instruction_a(head,
        instr_index,
        instr_offset_in_instrs,
        cfa_operation,
        fields_description,
        u0,
        u1,
        & aspace,
        s0,
        s1,
        code_alignment_factor,
        data_alignment_factor,
        expression_block,
        error);
}
int
dwarf_get_frame_instruction_a(Dwarf_Frame_Instr_Head head,
    Dwarf_Unsigned    instr_index,
    Dwarf_Unsigned  * instr_offset_in_instrs,
    Dwarf_Small     * cfa_operation,
    const char     ** fields_description,
    Dwarf_Unsigned  * u0,
    Dwarf_Unsigned  * u1,
    Dwarf_Unsigned  * u2,
    Dwarf_Signed    * s0,
    Dwarf_Signed    * s1,
    Dwarf_Unsigned  * code_alignment_factor,
    Dwarf_Signed    * data_alignment_factor,
    Dwarf_Block     * expression_block,
    Dwarf_Error     * error)
{
    Dwarf_Frame_Instr ip = 0;
    Dwarf_Debug dbg = 0;
    if (!head) {
        _dwarf_error_string(dbg, error,DW_DLE_CFA_INSTRUCTION_ERROR,
            "DW_DLE_CFA_INSTRUCTION_ERROR: Head argument NULL "
            " calling dwarf_get_frame_instruction");
        return DW_DLV_ERROR;
    }
    if (!head->fh_dbg) {
        _dwarf_error_string(dbg, error,DW_DLE_CFA_INSTRUCTION_ERROR,
            "DW_DLE_CFA_INSTRUCTION_ERROR: Head missing "
            "Dwarf_Debug field "
            " calling dwarf_get_frame_instruction");
        return DW_DLV_ERROR;
    }
    dbg = head->fh_dbg;
    if (instr_index >= head->fh_array_count) {
        return DW_DLV_NO_ENTRY;
    }
    ip = head->fh_array[instr_index];
    if (!ip) {
        _dwarf_error_string(dbg, error,DW_DLE_CFA_INSTRUCTION_ERROR,
            "DW_DLE_CFA_INSTRUCTION_ERROR: instr array missing "
            "calling dwarf_get_frame_instruction");
        return DW_DLV_ERROR;
    }
    *instr_offset_in_instrs = ip->fi_instr_offset;
    *cfa_operation = ip->fi_op;
    *fields_description = ip->fi_fields;
    *u0 = ip->fi_u0;
    *u1 = ip->fi_u1;
    *u2 = ip->fi_u2;
    *s0 = ip->fi_s0;
    *s1 = ip->fi_s1;
    /*  These next two might be known to caller already,
        so let caller not pass useless pointers. */
    if (code_alignment_factor) {
        *code_alignment_factor = ip->fi_code_align_factor;
    }
    if (data_alignment_factor) {
        *data_alignment_factor = ip->fi_data_align_factor;
    }
    *expression_block = ip->fi_expr;
    return DW_DLV_OK;
}

/*  Used by dwarfdump -v to print offsets, for debugging
    dwarf info.
    The dwarf_ version is preferred over the obsolete _dwarf version.
    _dwarf version kept for compatibility.
*/
int
_dwarf_fde_section_offset(Dwarf_Debug dbg, Dwarf_Fde in_fde,
    Dwarf_Off * fde_off, Dwarf_Off * cie_off,
    Dwarf_Error * error)
{
    return dwarf_fde_section_offset(dbg,in_fde,fde_off,
        cie_off,error);
}
int
dwarf_fde_section_offset(Dwarf_Debug dbg, Dwarf_Fde in_fde,
    Dwarf_Off * fde_off, Dwarf_Off * cie_off,
    Dwarf_Error * error)
{
    char *start = 0;
    char *loc = 0;

    CHECK_DBG(dbg,error,"dwarf_fde_section_offset()");
    if (!in_fde) {
        _dwarf_error(dbg, error, DW_DLE_FDE_NULL);
        return DW_DLV_ERROR;
    }
    start = (char *) in_fde->fd_section_ptr;
    loc = (char *) in_fde->fd_fde_start;

    *fde_off = (loc - start);
    *cie_off = in_fde->fd_cie_offset;
    return DW_DLV_OK;
}

/* Used by dwarfdump -v to print offsets, for debugging
   dwarf info.
   The dwarf_ version is preferred over the obsolete _dwarf version.
   _dwarf version kept for compatibility.
*/
int
_dwarf_cie_section_offset(Dwarf_Debug dbg, Dwarf_Cie in_cie,
    Dwarf_Off * cie_off, Dwarf_Error * error)
{
    return dwarf_cie_section_offset(dbg,in_cie,cie_off,error);
}

int
dwarf_cie_section_offset(Dwarf_Debug dbg, Dwarf_Cie in_cie,
    Dwarf_Off * cie_off, Dwarf_Error * error)
{
    char *start = 0;
    char *loc = 0;

    CHECK_DBG(dbg,error,"dwarf_cie_section_offset()");
    if (!in_cie) {
        _dwarf_error(dbg, error, DW_DLE_CIE_NULL);
        return DW_DLV_ERROR;
    }
    start = (char *) in_cie->ci_section_ptr;
    loc = (char *) in_cie->ci_cie_start;

    *cie_off = (loc - start);
    return DW_DLV_OK;
}

/*  Returns  a pointer to target-specific augmentation data
    thru augdata
    and returns the length of the data thru augdata_len.

    It's up to the consumer code to know how to interpret the bytes
    of target-specific data (endian issues apply too, these
    are just raw bytes pointed to).
    See  Linux Standard Base Core Specification version 3.0 for
    the details on .eh_frame info.

    Returns DW_DLV_ERROR if fde is NULL or some other serious
    error.
    Returns DW_DLV_NO_ENTRY if there is no target-specific
    augmentation data.

    The bytes pointed to are in the Dwarf_Cie, and as long as that
    is valid the bytes are there. No 'dealloc' call is needed
    for the bytes.  */
int
dwarf_get_cie_augmentation_data(Dwarf_Cie cie,
    Dwarf_Small ** augdata,
    Dwarf_Unsigned * augdata_len,
    Dwarf_Error * error)
{
    if (cie == NULL) {
        _dwarf_error(NULL, error, DW_DLE_CIE_NULL);
        return DW_DLV_ERROR;
    }
    if (cie->ci_gnu_eh_augmentation_len == 0) {
        return DW_DLV_NO_ENTRY;
    }
    *augdata = (Dwarf_Small *) (cie->ci_gnu_eh_augmentation_bytes);
    *augdata_len = cie->ci_gnu_eh_augmentation_len;
    return DW_DLV_OK;
}

/*  Returns  a pointer to target-specific augmentation data
    thru augdata
    and returns the length of the data thru augdata_len.

    It's up to the consumer code to know how to interpret the bytes
    of target-specific data (endian issues apply too, these
    are just raw bytes pointed to).
    See  Linux Standard Base Core Specification version 3.0 for
    the details on .eh_frame info.

    Returns DW_DLV_ERROR if fde is NULL or some other serious
    error.
    Returns DW_DLV_NO_ENTRY if there is no target-specific
    augmentation data.

    The bytes pointed to are in the Dwarf_Fde, and as long as that
    is valid the bytes are there. No 'dealloc' call is needed
    for the bytes.  */
int
dwarf_get_fde_augmentation_data(Dwarf_Fde fde,
    Dwarf_Small * *augdata,
    Dwarf_Unsigned * augdata_len,
    Dwarf_Error * error)
{
    Dwarf_Cie cie = 0;

    if (fde == NULL) {
        _dwarf_error(NULL, error, DW_DLE_FDE_NULL);
        return DW_DLV_ERROR;
    }
    if (!fde->fd_gnu_eh_aug_present) {
        return DW_DLV_NO_ENTRY;
    }
    cie = fde->fd_cie;
    if (cie == NULL) {
        _dwarf_error(NULL, error, DW_DLE_CIE_NULL);
        return DW_DLV_ERROR;
    }
    *augdata = (Dwarf_Small *) fde->fd_gnu_eh_augmentation_bytes;
    *augdata_len = fde->fd_gnu_eh_augmentation_len;
    return DW_DLV_OK;
}

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

/*  This allows consumers to set the 'initial value' so that
    an ISA/ABI specific default can be used, dynamically,
    at run time.  Useful for dwarfdump and non-MIPS architectures..
    The value  defaults to one of
        DW_FRAME_SAME_VALUE or DW_FRAME_UNKNOWN_VALUE
    but dwarfdump can dump multiple ISA/ABI objects so
    we may want to get this set to what the ABI says is correct.

    Returns the value that was present before we changed it here.  */
Dwarf_Half
dwarf_set_frame_rule_initial_value(Dwarf_Debug dbg,
    Dwarf_Half value)
{
    Dwarf_Half orig = 0;
    int invalid = FALSE;

    invalid = IS_INVALID_DBG(dbg);
    if (invalid) {
        return 0;
    }
    orig = (Dwarf_Half)dbg->de_frame_rule_initial_value;
    dbg->de_frame_rule_initial_value = value;
    dbg->de_frame_numbers_validated = FALSE;
    return orig;
}

/*  This allows consumers to set the array size of the  reg rules
    table so that
    an ISA/ABI specific value can be used, dynamically,
    at run time.  Useful for non-MIPS architectures.
    The value  defaults  to DW_FRAME_LAST_REG_NUM.
    but dwarfdump can dump multiple ISA/ABI objects so
    consumers want to get this set to what the ABI says is correct.

    Returns the value that was present before we changed it here.
*/

Dwarf_Half
dwarf_set_frame_rule_table_size(Dwarf_Debug dbg, Dwarf_Half value)
{
    Dwarf_Half orig = 0;
    int invalid = FALSE;

    invalid = IS_INVALID_DBG(dbg);
    if (invalid) {
        return 0;
    }
    orig = (Dwarf_Half)dbg->de_frame_reg_rules_entry_count;
    /*  Take the caller-specified value, but do not
        let the value be too small.  */
    if (value < 50) {
        value = 50;    /* arbitrarily */
    }
    dbg->de_frame_reg_rules_entry_count = value;
    dbg->de_frame_numbers_validated = FALSE;
    return orig;
}
/*  This allows consumers to set the CFA register value
    so that an ISA/ABI specific value can be used, dynamically,
    at run time.  Useful for non-MIPS architectures.
    The value  defaults  to DW_FRAME_CFA_COL3 and should be
    higher than any real register in the ABI.
    Dwarfdump can dump multiple ISA/ABI objects so
    consumers want to get this set to what the ABI says is correct.

    Returns the value that was present before we changed it here.  */

Dwarf_Half
dwarf_set_frame_cfa_value(Dwarf_Debug dbg, Dwarf_Half value)
{
    Dwarf_Half orig = 0;
    int invalid = FALSE;

    invalid = IS_INVALID_DBG(dbg);
    if (invalid) {
        return 0;
    }
    orig = (Dwarf_Half)dbg->de_frame_cfa_col_number;
    dbg->de_frame_cfa_col_number = value;
    dbg->de_frame_numbers_validated = FALSE;
    return orig;
}
/* Similar to above, but for the other crucial fields for frames. */
Dwarf_Half
dwarf_set_frame_same_value(Dwarf_Debug dbg, Dwarf_Half value)
{
    Dwarf_Half orig = 0;
    int invalid = FALSE;

    invalid = IS_INVALID_DBG(dbg);
    if (invalid) {
        return 0;
    }
    orig = (Dwarf_Half)dbg->de_frame_same_value_number;
    dbg->de_frame_same_value_number = value;
    dbg->de_frame_numbers_validated = FALSE;
    return orig;
}
Dwarf_Half
dwarf_set_frame_undefined_value(Dwarf_Debug dbg, Dwarf_Half value)
{
    Dwarf_Half orig = 0;
    int invalid = FALSE;

    invalid = IS_INVALID_DBG(dbg);
    if (invalid) {
        return 0;
    }
    orig = (Dwarf_Half)dbg->de_frame_same_value_number;
    dbg->de_frame_undefined_value_number = value;
    dbg->de_frame_numbers_validated = FALSE;
    return orig;
}

/*  Does something only if value passed in is greater than 0 and
    a size than we can handle (in number of bytes).  */
Dwarf_Small
dwarf_set_default_address_size(Dwarf_Debug dbg,
    Dwarf_Small value  )
{
    Dwarf_Small orig = 0;
    int invalid = FALSE;

    invalid = IS_INVALID_DBG(dbg);
    if (invalid) {
        return 0;
    }
    orig = dbg->de_pointer_size;
    if (value > 0 && value <= sizeof(Dwarf_Addr)) {
        dbg->de_pointer_size = value;
    }
    return orig;
}

static const struct Dwarf_Frame_s zero_frame;
/*  Fills in frame, creates zeroed Regtable or
    records user-created zeroed Regtable */

int
_dwarf_initialize_frame_table(Dwarf_Debug dbg,
    Dwarf_Frame      frame_table,
    Dwarf_Unsigned   table_real_data_size,
    Dwarf_Regtable3 *regtable,
    Dwarf_Error     *error)
{
    Dwarf_Frame fr = frame_table;
    Dwarf_Regtable3 *rt = 0;
    Dwarf_Regtable_Entry3 *rules = 0;

    _dwarf_empty_frame_table(frame_table);
    *frame_table = zero_frame;
    fr->fr_loc = 0;
    fr->fr_next = 0;
    fr->fr_owns_regtable = TRUE;
    fr->fr_reg_count = table_real_data_size;
    if (regtable) {
        fr->fr_regtable = regtable;
        fr->fr_owns_regtable = FALSE;
        return DW_DLV_OK;
    }
    fr->fr_owns_regtable = TRUE;
    rt = (Dwarf_Regtable3 *)calloc(1, sizeof(Dwarf_Regtable3));
    if (!rt) {
        _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
        return DW_DLV_ERROR;
    }
    fr->fr_regtable = rt;
    rules  = (Dwarf_Regtable_Entry3*)
        calloc((size_t)table_real_data_size,
            sizeof(Dwarf_Regtable_Entry3));
    rt->rt3_reg_table_size = (Dwarf_Half)table_real_data_size;
    if (!rules) {
        free(rt);
        fr->fr_regtable = 0;
        _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
        return DW_DLV_ERROR;
    }
    rt->rt3_rules = rules;
    return DW_DLV_OK;
}

/*  Any frame table, not just fde frame table */
void
_dwarf_empty_frame_table(struct Dwarf_Frame_s *frame)
{
    Dwarf_Regtable3 * rt = 0;

    if (!frame) {
        return; /* impossible */
    }
    if (frame->fr_owns_regtable) {
        rt = frame->fr_regtable;
        if (rt) {
            free(rt->rt3_rules);
            rt->rt3_rules = 0;
            rt->rt3_reg_table_size = 0;
            free(rt);
        }
        frame->fr_regtable = 0;
        frame->fr_owns_regtable = FALSE;
    }
}

/*  Return DW_DLV_OK if we succeed. else return DW_DLV_ERROR.  */
int
_dwarf_frame_constructor(Dwarf_Debug dbg, void *frame)
{
    (void)frame;
    if (IS_INVALID_DBG(dbg)) {
        return DW_DLV_ERROR;
    }
    return DW_DLV_OK;
}

void
_dwarf_frame_destructor(void *frame)
{
    struct Dwarf_Frame_s *fp = frame;
    _dwarf_empty_frame_table(fp);
}
void
_dwarf_cie_destructor(void *c)
{
    Dwarf_Cie cie = (Dwarf_Cie)c;

    if (cie->ci_initial_table) {
        Dwarf_Frame f = cie->ci_initial_table;
        dwarf_dealloc(cie->ci_dbg,f,DW_DLA_FRAME);
        cie->ci_initial_table = 0;
    }
}

void
_dwarf_fde_destructor(void *f)
{
    struct Dwarf_Fde_s *fde = f;

    if (fde->fd_fde_owns_cie) {
        Dwarf_Debug dbg = fde->fd_dbg;

        if (!dbg->de_in_tdestroy) {
            /*  This is just for dwarf_get_fde_for_die() and
                must not be applied in alloc tree destruction. */
            dwarf_dealloc(fde->fd_dbg,fde->fd_cie,DW_DLA_CIE);
            fde->fd_cie = 0;
        }
    }
    if (fde->fd_have_fde_frame_tab) {
        _dwarf_empty_frame_table(&fde->fd_fde_frame_table);
        fde->fd_have_fde_frame_tab = FALSE;
        fde->fd_fde_frame_table = zero_frame;
    }
}
void
_dwarf_frame_instr_destructor(void *f)
{
    Dwarf_Frame_Instr_Head head = f;
    Dwarf_Debug dbg = head->fh_dbg;
    Dwarf_Unsigned count = head->fh_array_count;
    Dwarf_Unsigned i = 0;

    for ( ; i < count ; ++i) {
        free(head->fh_array[i]);
        head->fh_array[i] = 0;
    }
    dwarf_dealloc(dbg,head->fh_array,DW_DLA_LIST);
    head->fh_array = 0;
    head->fh_array_count = 0;
}
void
dwarf_dealloc_frame_instr_head(Dwarf_Frame_Instr_Head h)
{
    if (!h) {
        return;
    }
    dwarf_dealloc(h->fh_dbg,h,DW_DLA_FRAME_INSTR_HEAD);
}

/*  For any remaining columns after what fde has.
    Set Dwarf_Regtable_Entry3 to
    standard initial values. */
void
_dwarf_init_reg_rules_dw3(
    Dwarf_Regtable_Entry3 *base,
    Dwarf_Unsigned first, Dwarf_Unsigned last,
    Dwarf_Unsigned initial_value)
{
    Dwarf_Regtable_Entry3 *r = base+first;
    Dwarf_Unsigned i = first;
    for (; i < last; ++i,++r) {
        r->dw_offset_relevant = 0;
        r->dw_value_type = DW_EXPR_OFFSET;
        r->dw_regnum = (Dwarf_Half)initial_value;
        r->dw_offset = 0;
        r->dw_args_size = 0;
        r->dw_block.bl_data = 0;
        r->dw_block.bl_len = 0;
    }
}
