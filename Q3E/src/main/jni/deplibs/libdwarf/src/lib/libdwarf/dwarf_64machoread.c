/*
Copyright (c) 2019, David Anderson All rights reserved.
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

/*  This file dwarf_64machoread.c
    is separate so one can view/edit 32 and 64bit
    equivalent functions in separate windows
    to make human verification of 32/64 behavior
    matches appropriately. */

#include <config.h>
#include <stdlib.h> /* calloc() free() malloc() */
#include <string.h> /* memcpy() memset() strcmp() strdup() */
#include <stdio.h> /* debugging printf */

#include "dwarf.h"
#include "libdwarf.h"
#include "dwarf_local_malloc.h"
#include "libdwarf_private.h"
#include "dwarf_base_types.h"
#include "dwarf_safe_strcpy.h"
#include "dwarf_opaque.h"
#include "dwarf_error.h" /* for _dwarf_error() declaration */
#include "dwarf_reading.h"
#include "dwarf_memcpy_swap.h"
#include "dwarf_object_read_common.h"
#include "dwarf_universal.h"
#include "dwarf_macho_loader.h"
#include "dwarf_machoread.h"
#include "dwarf_object_detector.h"
#include "dwarf_safe_arithmetic.h"

/* load_macho_header64(dwarf_macho_object_access_internals_t *mfp) */
int
_dwarf_load_macho_header64(dwarf_macho_object_access_internals_t *mfp,
    int *errcode)
{
    struct mach_header_64 mh64;
    int res = 0;
    Dwarf_Unsigned inner = mfp->mo_inner_offset;

    if (sizeof(mh64) > mfp->mo_filesize) {
        *errcode = DW_DLE_FILE_TOO_SMALL;
        return DW_DLV_ERROR;
    }
    res = RRMOA(mfp->mo_fd, &mh64, inner, sizeof(mh64),
        (inner+mfp->mo_filesize), errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    /* Do not adjust endianness of magic, leave as-is. */
    ASNAR(memcpy,mfp->mo_header.magic,mh64.magic);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.cputype,mh64.cputype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.cpusubtype,
        mh64.cpusubtype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.filetype,mh64.filetype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.ncmds,mh64.ncmds);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.sizeofcmds,
        mh64.sizeofcmds);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.flags,mh64.flags);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.reserved,mh64.reserved);
    mfp->mo_command_count = (unsigned int)mfp->mo_header.ncmds;
    if (mfp->mo_header.sizeofcmds >= MAX_COMMANDS_SIZE ||
        mfp->mo_header.sizeofcmds >= mfp->mo_filesize ) {
        *errcode = DW_DLE_MACHO_CORRUPT_HEADER;
        return DW_DLV_ERROR;
    }
    mfp->mo_machine = mfp->mo_header.cputype;
    mfp->mo_flags = mfp->mo_header.flags;
    mfp->mo_command_start_offset = sizeof(mh64);
    return DW_DLV_OK;
}

int
_dwarf_load_segment_command_content64(
    dwarf_macho_object_access_internals_t *mfp,
    struct generic_macho_command *mmp,
    struct generic_macho_segment_command *msp,
    Dwarf_Unsigned mmpindex,int *errcode)
{
    struct segment_command_64 sc;
    int res = 0;
    Dwarf_Unsigned filesize = mfp->mo_filesize;
    Dwarf_Unsigned segoffset = mmp->offset_this_command;
    Dwarf_Unsigned afterseghdr = segoffset + sizeof(sc);
    Dwarf_Unsigned inner = mfp->mo_inner_offset;

    if (segoffset > filesize ||
        mmp->cmdsize > filesize ||
        (mmp->cmdsize + segoffset) > filesize ) {
        *errcode = DW_DLE_MACH_O_SEGOFFSET_BAD;
        return DW_DLV_ERROR;
    }
    res = RRMOA(mfp->mo_fd,&sc,inner+segoffset,
        sizeof(sc), inner+filesize, errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    ASNAR(mfp->mo_copy_word,msp->cmd,sc.cmd);
    ASNAR(mfp->mo_copy_word,msp->cmdsize,sc.cmdsize);
    _dwarf_safe_strcpy(msp->segname,sizeof(msp->segname),
        sc.segname,sizeof(sc.segname));
    ASNAR(mfp->mo_copy_word,msp->vmaddr,sc.vmaddr);
    ASNAR(mfp->mo_copy_word,msp->vmsize,sc.vmsize);
    ASNAR(mfp->mo_copy_word,msp->fileoff,sc.fileoff);
    ASNAR(mfp->mo_copy_word,msp->filesize,sc.filesize);
    if (msp->fileoff > filesize ||
        msp->filesize > filesize) {
        /* corrupt */
        *errcode = DW_DLE_MACHO_CORRUPT_COMMAND;
        return DW_DLV_ERROR;
    }
    if ((msp->fileoff+msp->filesize ) > filesize) {
        /* corrupt */
        *errcode = DW_DLE_MACHO_CORRUPT_COMMAND;
        return DW_DLV_ERROR;
    }
    ASNAR(mfp->mo_copy_word,msp->maxprot,sc.maxprot);
    ASNAR(mfp->mo_copy_word,msp->initprot,sc.initprot);
    ASNAR(mfp->mo_copy_word,msp->nsects,sc.nsects);
    if (msp->nsects >= mfp->mo_filesize) {
        *errcode = DW_DLE_MACHO_CORRUPT_COMMAND;
        return DW_DLV_ERROR;
    }
    ASNAR(mfp->mo_copy_word,msp->flags,sc.flags);
    msp->macho_command_index = mmpindex;
    msp->sectionsoffset = afterseghdr;
    return DW_DLV_OK;
}

int
_dwarf_macho_load_dwarf_section_details64(
    dwarf_macho_object_access_internals_t *mfp,
    struct generic_macho_segment_command *segp,
    Dwarf_Unsigned segi,
    int *errcode)
{
    int            res = 0;
    Dwarf_Unsigned seci = 0;
    Dwarf_Unsigned seccount = segp->nsects;
    Dwarf_Unsigned secalloc = seccount+1;

    /* offset of sections being added */
    Dwarf_Unsigned curoff = segp->sectionsoffset;
    Dwarf_Unsigned shdrlen = sizeof(struct section_64);
    Dwarf_Unsigned newcount = 0;
    struct generic_macho_section *secs = 0;

    if (mfp->mo_dwarf_sections) {
        Dwarf_Unsigned secssizetot = 0;
        struct generic_macho_section * originalsections =
            mfp->mo_dwarf_sections;

        if (!seccount) {
            /* No sections. Odd. Unexpected. */
            return DW_DLV_OK;
        }
        newcount = mfp->mo_dwarf_sectioncount + seccount;
        res = _dwarf_uint64_mult(newcount,
            sizeof(struct generic_macho_section),
            &secssizetot);
        if (res != DW_DLV_OK) {
            /* overflow */
            *errcode = DW_DLE_MACHO_CORRUPT_SECTIONDETAILS;
            return DW_DLV_ERROR;
        }
        if (secssizetot > mfp->mo_filesize ) {

            /*  Really supposed to refer to size on disk, this
                is therefore approximate test. */
            *errcode = DW_DLE_MACHO_CORRUPT_SECTIONDETAILS;
            return DW_DLV_ERROR;
        }
        secs = (struct generic_macho_section *)calloc(
            1,secssizetot);
        if (!secs) {
            *errcode = DW_DLE_ALLOC_FAIL;
            return DW_DLV_OK;
        }
        memcpy(secs,mfp->mo_dwarf_sections,
            mfp->mo_dwarf_sectioncount*
            sizeof(struct generic_macho_section));
        mfp->mo_dwarf_sections = secs;
        seci =  mfp->mo_dwarf_sectioncount ;
        mfp->mo_dwarf_sectioncount = newcount;
        free(originalsections);
        secs += seci;
        secs->offset_of_sec_rec = curoff;
        secalloc = newcount;
    } else {
        Dwarf_Unsigned secssizetot = 0;

        newcount = secalloc;
        res = _dwarf_uint64_mult(newcount,
            sizeof(struct generic_macho_section),
            &secssizetot);
        if (res != DW_DLV_OK) {
            /* overflow */
            *errcode = DW_DLE_MACHO_CORRUPT_SECTIONDETAILS;
            return DW_DLV_ERROR;
        }
        if (secssizetot > mfp->mo_filesize ) {
            /*  Really supposed to refer to size on disk, this
                is therefore approximate test. */
            *errcode = DW_DLE_MACHO_CORRUPT_SECTIONDETAILS;
            return DW_DLV_ERROR;
        }
        secs = (struct generic_macho_section *)calloc(
            1,secssizetot);
        if (!secs) {
            *errcode = DW_DLE_ALLOC_FAIL;
            return DW_DLV_OK;
        }
        mfp->mo_dwarf_sections = secs;
        mfp->mo_dwarf_sectioncount = secalloc;
        secs->offset_of_sec_rec = curoff;
        /*  Leave 0 section all zeros except our offset,
            elf-like in a sense */
        secs->dwarfsectname = "";
        seci = 1;
        ++secs;
    }
    for (; seci < secalloc; ++seci,++secs,curoff += shdrlen ) {
        struct section_64 mosec;
        Dwarf_Unsigned endoffset = 0;
        Dwarf_Unsigned inner = mfp->mo_inner_offset;
        Dwarf_Unsigned offplussize = 0;
        Dwarf_Unsigned innercur = 0;

        endoffset = curoff + sizeof(mosec);
        if (curoff >=  mfp->mo_filesize ||
            endoffset > mfp->mo_filesize) {
            *errcode = DW_DLE_MACHO_CORRUPT_SECTIONDETAILS;
            return DW_DLV_ERROR;
        }
        innercur = inner+curoff;
        if (innercur < inner || innercur <curoff) {
            *errcode = DW_DLE_MACHO_CORRUPT_SECTIONDETAILS;
            return DW_DLV_ERROR;
        }
        /* inner refers to universal binaries */
        offplussize = inner+mfp->mo_filesize;
        if (offplussize < inner || offplussize <mfp->mo_filesize) {
            *errcode = DW_DLE_MACHO_CORRUPT_SECTIONDETAILS;
            return DW_DLV_ERROR;
        }
        res = RRMOA(mfp->mo_fd, &mosec,
            innercur, sizeof(mosec),
            offplussize, errcode);
        if (res != DW_DLV_OK) {
            return res;
        }
        _dwarf_safe_strcpy(secs->sectname,
            sizeof(secs->sectname),
            mosec.sectname,sizeof(mosec.sectname));
        if (_dwarf_not_ascii(secs->sectname) ) {
            *errcode  = DW_DLE_MACHO_CORRUPT_SECTIONDETAILS;
            return DW_DLV_ERROR;
        }
        _dwarf_safe_strcpy(secs->segname,
            sizeof(secs->segname),
            mosec.segname,sizeof(mosec.segname));
        ASNAR(mfp->mo_copy_word,secs->addr,mosec.addr);
        ASNAR(mfp->mo_copy_word,secs->size,mosec.size);
        ASNAR(mfp->mo_copy_word,secs->offset,mosec.offset);
        ASNAR(mfp->mo_copy_word,secs->align,mosec.align);
        ASNAR(mfp->mo_copy_word,secs->reloff,mosec.reloff);
        ASNAR(mfp->mo_copy_word,secs->nreloc,mosec.nreloc);
        ASNAR(mfp->mo_copy_word,secs->flags,mosec.flags);
        /*offplussize = secs->offset+secs->size; */
        res = _dwarf_uint64_add(secs->offset,secs->size,
            &offplussize);
        if (res == DW_DLV_ERROR){
            /* overflow in add */
            *errcode  = DW_DLE_MACHO_CORRUPT_SECTIONDETAILS;
            return DW_DLV_ERROR;
        }
        /*  __text section size apparently refers to executable,
            not dSYM, so do not check here
            No check for __text.
            All sections in __DWARF checked  */
        if (0 == strcmp(secs->segname,"__DWARF")) {
            if (secs->offset > mfp->mo_filesize ||
                secs->size > mfp->mo_filesize ||
                offplussize > mfp->mo_filesize) {
                *errcode  = DW_DLE_MACHO_CORRUPT_SECTIONDETAILS;
                return DW_DLV_ERROR;
            }
        }
        secs->reserved1 = 0;
        secs->reserved2 = 0;
        secs->reserved3 = 0;
        secs->offset_of_sec_rec = curoff;
        secs->generic_segment_num  = segi;
    }
    return DW_DLV_OK;
}

int
_dwarf_fill_in_uni_arch_64(
    struct fat_arch_64 * fa,
    struct Dwarf_Universal_Head_s *duhd,
    void (*word_swap) (void *, const void *, unsigned long),
    int *errcode)
{
    Dwarf_Unsigned i = 0;
    struct Dwarf_Universal_Arch_s * dua = 0;

    dua = duhd->au_arches;
    for ( ; i < duhd->au_count; ++i,++fa,++dua) {
        ASNAR(word_swap,dua->au_cputype,fa->cputype);
        ASNAR(word_swap,dua->au_cpusubtype,fa->cpusubtype);
        ASNAR(word_swap,dua->au_offset,fa->offset);
        if (dua->au_offset >= duhd->au_filesize) {
            *errcode = DW_DLE_UNIV_BIN_OFFSET_SIZE_ERROR;
            return DW_DLV_ERROR;
        }
        ASNAR(word_swap,dua->au_size,fa->size);
        if (dua->au_size >= duhd->au_filesize) {
            *errcode = DW_DLE_UNIV_BIN_OFFSET_SIZE_ERROR;
            return DW_DLV_ERROR;
        }
        if ((dua->au_size+dua->au_offset) > duhd->au_filesize) {
            *errcode = DW_DLE_UNIV_BIN_OFFSET_SIZE_ERROR;
            return DW_DLV_ERROR;
        }
        ASNAR(word_swap,dua->au_align,fa->align);
        if (dua->au_align >= 32) {
            *errcode = DW_DLE_UNIV_BIN_OFFSET_SIZE_ERROR;
            return DW_DLV_ERROR;
        }
        ASNAR(word_swap,dua->au_reserved,fa->reserved);
    }
    return DW_DLV_OK;
}
