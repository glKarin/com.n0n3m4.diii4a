/*
  Copyright 2018-2018 David Anderson.  All Rights Reserved.

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

#include <stddef.h> /* NULL */
#include <stdio.h>  /* printf() */
#include <string.h> /* memcpy() */

#ifdef HAVE_STDINT_H
#include <stdint.h> /* uintptr_t */
#endif

#include "dwarf.h"
#include "libdwarf.h"
#include "dwarf_base_types.h"
#include "libdwarfp.h"
#include "dwarf_pro_incl.h"
#include "dwarf_pro_opaque.h"
#include "dwarf_pro_error.h"
#include "dwarf_pro_alloc.h"
#include "dwarf_pro_arange.h"
#include "dwarf_pro_section.h"
#include "dwarf_pro_reloc.h"
#include "dwarf_pro_dnames.h"

static struct Dwarf_P_Dnames_Head_s staticdnames =
{
/* fake unit length*/58,
/* version */5,
/*offset size*/4,
/*offset*/0,
/* cu count */1,
/* tu ct */0,
/* foreigntu ct */0,
/* bucket ct */0,
/* name ct */1,
/* abbrev_table_size*/7,
/* augstringsize*/0,
/* augstring*/0
};

/* total length encoded: 6 bytes. */
unsigned char abbrv[7] =
{/* abbrev code*/ 2,
/* DW_TAG_subprogram */ 0x2e,
/* DW_IDX_compile_unit*/1,
/* DW_FORM_udata */7,
/* end abbrev */ 0,0,
/* end abbrev block */0};

unsigned char entry[3] =
{ /* abbrev code */2,
/* Offset of subprogram */0x32,
/* terminate this abbrev set */0 };

Dwarf_Unsigned stroffsets[1] = {0x42}; /* main */

/* Set to match subprog */
Dwarf_Unsigned dieoffset[1] = {12};

int
dwarf_force_dnames(Dwarf_P_Debug dbg,
    int elfsectno,
    Dwarf_Error * error)
{
    Dwarf_P_Dnames dn;
    Dwarf_Unsigned datalen = 0;
    Dwarf_Unsigned withunitlength = 0;
    unsigned char *data = 0;
    unsigned char *startdata = 0;
    unsigned int zero = 0;
    struct Dwarf_P_Dnames_Head_s *dh = &staticdnames;
    uintptr_t bytes = 0;

    if (dbg == NULL) {
        _dwarf_p_error(NULL, error, DW_DLE_DBG_NULL);
        return DW_DLV_ERROR;
    }
    if (!elfsectno) {
        dbg->de_force_dnames = TRUE;
        dn = (Dwarf_P_Dnames)
            _dwarf_p_get_alloc(dbg, sizeof(struct Dwarf_P_Dnames_s));
        if (dn == NULL) {
            _dwarf_p_error(dbg, error, DW_DLE_ALLOC_FAIL);
            return DW_DLV_ERROR;
        }
        dbg->de_dnames = dn;
        return DW_DLV_OK;
    }
    dn = dbg->de_dnames;
    if (!dbg->de_dnames) {
        return DW_DLV_NO_ENTRY;
    }
    dn->dn_dbg = dbg;
    dn->dn_create_section = TRUE;
    datalen = 8*4 +
        /* offset CU */
        dh->dh_offset_size  +
        /* str offsets, entry offsets */
        dh->dh_offset_size *2 +
        sizeof(abbrv) +
        sizeof(entry);
    withunitlength = datalen + 4;/* 4 byte length, 32 bit offset */

    GET_CHUNK(dbg, dbg->de_elf_sects[DEBUG_NAMES],
        data, (unsigned long)withunitlength, error);
    startdata = data;

    /*WRITE_UNALIGNED(dbg,dest,source, srclength,len_out)*/
    /* 1 */
    WRITE_UNALIGNED(dbg, (void *)data,
        (const void *)&datalen,
        sizeof(datalen) , SIZEOFT32);
    data += SIZEOFT32;
    /* 2 */
    WRITE_UNALIGNED(dbg, (void *)data,
        (const void *)&dh->dh_version,
        sizeof(dh->dh_version),
        SIZEOFT16);
    data += SIZEOFT16;

    /* 3 */
    WRITE_UNALIGNED(dbg, (void *)data,
        (const void *)&zero,sizeof(zero),
        SIZEOFT16);
    data += SIZEOFT16;

    /* 4 */
    WRITE_UNALIGNED(dbg, (void *)data,
        (const void *)&dh->dh_comp_unit_count,
        sizeof(dh->dh_comp_unit_count),
        SIZEOFT32);
    data += SIZEOFT32;
    /* 5 */
    WRITE_UNALIGNED(dbg, (void *)data,
        (const void *)&dh->dh_local_type_unit_count,
        sizeof(dh->dh_local_type_unit_count),
        SIZEOFT32);
    data += SIZEOFT32;
    /* 6 */

    WRITE_UNALIGNED(dbg, (void *)data,
        (const void *)&dh->dh_foreign_type_unit_count,
        sizeof(dh->dh_foreign_type_unit_count),
        SIZEOFT32);
    data += SIZEOFT32;
    /* 7 */
    WRITE_UNALIGNED(dbg, (void *)data,
        (const void *)&dh->dh_bucket_count,
        sizeof(dh->dh_bucket_count),
        SIZEOFT32);
    data += SIZEOFT32;
    /* 8 */
    WRITE_UNALIGNED(dbg, (void *)data,
        (const void *)&dh->dh_name_count,
        sizeof(dh->dh_name_count),
        SIZEOFT32);
    data += SIZEOFT32;

    /* 9 */
    WRITE_UNALIGNED(dbg, (void *)data,
        (const void *)&dh->dh_abbrev_table_size,
        sizeof(dh->dh_abbrev_table_size),
        SIZEOFT32);
    data += SIZEOFT32;

    /* 10 */
    WRITE_UNALIGNED(dbg, (void *)data,
        (const void *)&dh->dh_augmentation_string_size,
        sizeof(dh->dh_augmentation_string_size),
        SIZEOFT32);
    data += SIZEOFT32;
    if (dh->dh_augmentation_string_size) {
        /* 11 */
        memcpy((void *)data,dh->dh_augmentation_string,
            dh->dh_augmentation_string_size);
        data += dh->dh_augmentation_string_size;
        bytes = data - startdata;
    }
    bytes = data - startdata;

    /*  The CU offset table. A single entry */
    WRITE_UNALIGNED(dbg, (void *)data,
        (const void *)&zero,
        sizeof(stroffsets[0]),
        dh->dh_offset_size);
    data +=  dh->dh_offset_size;
    bytes = data - startdata;

    /* Now the string offsets table */
    WRITE_UNALIGNED(dbg, (void *)data,
        (const void *)&stroffsets[0],
        sizeof(stroffsets[0]),
        dh->dh_offset_size);
    data +=  dh->dh_offset_size;
    bytes = data - startdata;
    /* Now the Entry Offsets (DIE offsets)  array */
    WRITE_UNALIGNED(dbg, (void *)data,
        (const void *)&dieoffset[0],
        sizeof(dieoffset[0]),
        dh->dh_offset_size);
    data +=  dh->dh_offset_size;
    bytes = data - startdata;

    memcpy((void *)data,abbrv,sizeof(abbrv));
    data += sizeof(abbrv);
    bytes = data - startdata;

    memcpy((void *)data,entry,sizeof(entry));
    data += sizeof(entry);
    bytes = data - startdata;
    if (bytes != withunitlength) {
        printf("FAIL writing debug_names "
            "bytes written: %lu "
            "bytes allocated: %lu\n",
            (unsigned long)bytes,
            (unsigned long)withunitlength);
    }
#if 0
    dbg->de_dnames_blob = startdata;
    dbg->de_dnames_bloblength = withunitlength;
#endif
    return DW_DLV_OK;
}
