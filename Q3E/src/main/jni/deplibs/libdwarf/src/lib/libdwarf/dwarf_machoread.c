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

/*  See also dwarf_64machoread.c for the 64bit
    reading code. Makes it easier (using separate files
    view/edit simultaneously) to ensore the 32
    and 64 bit behave equivalently.
    A useful resource is
    https://en.wikipedia.org/wiki/Mach-O */

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

#if 0 /* dump_bytes */
static void
dump_bytes(const char *msg,Dwarf_Small * start, long len)
{
    Dwarf_Small *end = start + len;
    Dwarf_Small *cur = start;
    int ct = 0;
    printf("%s (0x%lx) ",msg,(unsigned long)start);
    for (; cur < end; cur++) {
        printf("%02x", *cur);
        ct++;
        if (ct %4 == 0) {
            printf(" ");
        }
    }
    printf("\n");
}
#endif /*0*/
#if 0 /* print_arch_item debugging */
static void
print_arch_item(unsigned int i,
    struct  Dwarf_Universal_Arch_s* arch)
{
    printf(" Universal Binary Index %u\n",i);
    printf("   cpu     0x%x\n",(unsigned)arch->au_cputype);
    printf("   cpusubt 0x%x\n",(unsigned)arch->au_cpusubtype);
    printf("   offset  0x%x\n",(unsigned)arch->au_offset);
    printf("   size    0x%x\n",(unsigned)arch->au_size);
    printf("   align   0x%x\n",(unsigned)arch->au_align);
}
#endif

#if 0 /* This check is inappropriate.  */
/*  The actual list gets added-to by Apple */
/*  One wonders if a duplicated segname name is an error.
    I suppose so, but we do not yet check for that. */
static const char *
knownsegnames[] = {
SEG_DWARF,
SEG_TEXT,
SEG_DATA,
SEG_DATA_CONST,
SEG_ICON,
SEG_IMPORT,
SEG_LINKEDIT,
SEG_OBJC,
SEG_PAGEZERO,
SEG_UNIXSTACK,
};

int
_dwarf_is_known_segname(char *sname)
{
    char *s_in = sname;
    int i = 0;
    int end = sizeof(knownsegnames)/sizeof(char *);

    for ( ; i < end; ++i) {
        if (strcmp(s_in,knownsegnames[i])) {
            continue;
        }
        return TRUE;
    }
    return FALSE;
}
#endif
/*  We do not expect non-ascii characters in section
    names, they are defined by the compiler-writers
    and ABI rules. We allow an empty name... */
int
_dwarf_not_ascii(const char *s)
{
    unsigned char *cp = (unsigned char *)s;
    for (  ; *cp ; ++cp) {
        if (*cp < 0x20 || *cp > 0x7e) {
            return TRUE;
        }
    }
    return FALSE;
}

/* MACH-O and dwarf section names */
static struct macho_sect_names_s {
    char const *ms_moname; /* Macho sect name */
    char const *ms_dwname; /* Elf/dwarf name */
} const SectionNames [] = {
    { "", "" },  /* ELF index-0 entry */
    { "__debug_abbrev",         ".debug_abbrev"   },
    { "__debug_aranges",        ".debug_aranges"  },
    { "__debug_frame",          ".debug_frame" /* a guess */},
    { "__eh_frame",             ".eh_frame"       },
    { "__debug_info",           ".debug_info"     },
    { "__debug_addr",           ".debug_addr"     },
    { "__debug_line",           ".debug_line"     },
    { "__debug_rnglists",       ".debug_rnglists" },
    { "__debug_loclists",       ".debug_loclists" },
    { "__debug_macinfo",        ".debug_macinfo"  },
    { "__debug_loc",            ".debug_loc"      },
    { "__debug_pubnames",       ".debug_pubnames" },
    { "__debug_pubtypes",       ".debug_pubtypes" },
    { "__debug_str",            ".debug_str"      },
    { "__debug_str_offs",       ".debug_str_offsets" },
    { "__debug_line_str",       ".debug_line_str" },
    { "__debug_ranges",         ".debug_ranges"   },
    { "__debug_macro",          ".debug_macro"    },
    { "__debug_names",          ".debug_names"    },
    { "__debug_gdb_scri",       ".debug_gdb_scripts" },
    { "__text",                 ".text"           },
};

static int
_dwarf_object_detector_universal_head_fd(
    int fd,
    Dwarf_Unsigned      dw_filesize,
    unsigned int       *dw_contentcount,
    Dwarf_Universal_Head * dw_head,
    int            *errcode);

static int _dwarf_macho_object_access_init(
    int  fd,
    unsigned uninumber,
    unsigned ftype,
    unsigned endian,
    unsigned offsetsize,
    unsigned * universalbinary_count,
    Dwarf_Unsigned filesize,
    Dwarf_Obj_Access_Interface_a **binary_interface,
    int *localerrnum);

static Dwarf_Small macho_get_byte_order (void *obj)
{
    dwarf_macho_object_access_internals_t *macho =
        (dwarf_macho_object_access_internals_t*)(obj);
    return macho->mo_endian;
}

static Dwarf_Small macho_get_length_size (void *obj)
{
    dwarf_macho_object_access_internals_t *macho =
        (dwarf_macho_object_access_internals_t*)(obj);
    return macho->mo_offsetsize/8;
}

static Dwarf_Small macho_get_pointer_size (void *obj)
{
    dwarf_macho_object_access_internals_t *macho =
        (dwarf_macho_object_access_internals_t*)(obj);
    return macho->mo_pointersize/8;
}
static Dwarf_Unsigned macho_get_file_size (void *obj)
{
    dwarf_macho_object_access_internals_t *macho =
        (dwarf_macho_object_access_internals_t*)(obj);
    return macho->mo_filesize;
}

static Dwarf_Unsigned macho_get_section_count (void *obj)
{
    dwarf_macho_object_access_internals_t *macho =
        (dwarf_macho_object_access_internals_t*)(obj);
    return macho->mo_dwarf_sectioncount;
}

static int macho_get_section_info (void *obj,
    Dwarf_Unsigned section_index,
    Dwarf_Obj_Access_Section_a *return_section,
    int *error)
{
    dwarf_macho_object_access_internals_t *macho =
        (dwarf_macho_object_access_internals_t*)(obj);

    (void)error;
    if (section_index < macho->mo_dwarf_sectioncount) {
        struct generic_macho_section *sp = 0;

        sp = macho->mo_dwarf_sections + section_index;
        return_section->as_name   = sp->dwarfsectname;
        return_section->as_type   = 0;
        return_section->as_flags  = sp->flags;
        return_section->as_addr   = sp->addr;
        return_section->as_offset = sp->offset;
        return_section->as_size   = sp->size;
        return_section->as_link   = 0;
        return_section->as_info   = 0;
        return_section->as_addralign = 0;
        return_section->as_entrysize = 0;
        return DW_DLV_OK;
    }
    return DW_DLV_NO_ENTRY;
}

static int
macho_load_section (void *obj, Dwarf_Unsigned section_index,
    Dwarf_Small **return_data, int *error)
{
    dwarf_macho_object_access_internals_t *macho =
        (dwarf_macho_object_access_internals_t*)(obj);

    if (0 < section_index &&
        section_index < macho->mo_dwarf_sectioncount) {
        int res = 0;
        /*   inner is zero except if unified binary.
             If unified, mo_filesize does not include
             inner (inner is the distance from zero
             to the present macho header in the overall
             universal binary). */
        Dwarf_Unsigned inner = macho->mo_inner_offset;
        Dwarf_Unsigned full_offset = 0;

        struct generic_macho_section *sp =
            macho->mo_dwarf_sections + section_index;
        if (sp->loaded_data) {
            *return_data = sp->loaded_data;
            return DW_DLV_OK;
        }
        if (!sp->size) {
            return DW_DLV_NO_ENTRY;
        }
        full_offset = sp->size + sp->offset + inner;
        if (full_offset < sp->size ||
            full_offset < sp->offset ||
            full_offset < inner) {
            *error = DW_DLE_ARITHMETIC_OVERFLOW;
            return DW_DLV_ERROR;
        }
        if ((sp->size + sp->offset) > macho->mo_filesize) {
            *error = DW_DLE_FILE_TOO_SMALL;
            return DW_DLV_ERROR;
        }
        sp->loaded_data = malloc((size_t)sp->size);
        if (!sp->loaded_data) {
            *error = DW_DLE_ALLOC_FAIL;
            return DW_DLV_ERROR;
        }
        res = RRMOA(macho->mo_fd,
            sp->loaded_data, (inner+sp->offset),
            sp->size,
            (inner+macho->mo_filesize), error);
        if (res != DW_DLV_OK) {
            free(sp->loaded_data);
            sp->loaded_data = 0;
            return res;
        }
        *return_data = sp->loaded_data;
        return DW_DLV_OK;
    }
    return DW_DLV_NO_ENTRY;
}

static void
_dwarf_destruct_macho_internals(
    dwarf_macho_object_access_internals_t *mp)
{
    Dwarf_Unsigned i = 0;

    if (mp->mo_destruct_close_fd) {
        _dwarf_closer(mp->mo_fd);
        mp->mo_fd = -1;
    }
    if (mp->mo_commands){
        free(mp->mo_commands);
        mp->mo_commands = 0;
    }
    if (mp->mo_segment_commands){
        free(mp->mo_segment_commands);
        mp->mo_segment_commands = 0;
    }
    free((char *)mp->mo_path);
    if (mp->mo_dwarf_sections) {
        struct generic_macho_section *sp = 0;

        sp = mp->mo_dwarf_sections;
        for ( i=0; i < mp->mo_dwarf_sectioncount; ++i,++sp) {
            if (sp->loaded_data) {
                free(sp->loaded_data);
                sp->loaded_data = 0;
            }
        }
        free(mp->mo_dwarf_sections);
        mp->mo_dwarf_sections = 0;
    }
    free(mp);
    return;
}

static void
_dwarf_destruct_macho_access(void *obj)
{
    struct Dwarf_Obj_Access_Interface_a_s * aip =
        (struct Dwarf_Obj_Access_Interface_a_s *)obj;

    dwarf_macho_object_access_internals_t *mp = 0;

    if (!aip) {
        return;
    }
    mp = (dwarf_macho_object_access_internals_t *)aip->ai_object;
    _dwarf_destruct_macho_internals(mp);
    aip->ai_object = 0;
    free(aip);
    return;
}

/* load_macho_header32(dwarf_macho_object_access_internals_t *mfp)*/
static int
load_macho_header32(dwarf_macho_object_access_internals_t *mfp,
    int *errcode)
{
    struct mach_header mh32;
    int res = 0;
    Dwarf_Unsigned inner = mfp->mo_inner_offset;

    if (sizeof(mh32) > mfp->mo_filesize) {
        *errcode = DW_DLE_FILE_TOO_SMALL;
        return DW_DLV_ERROR;
    }
    res = RRMOA(mfp->mo_fd, &mh32, inner, sizeof(mh32),
        (inner+mfp->mo_filesize), errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    /* Do not adjust endianness of magic, leave as-is. */
    ASNAR(memcpy,mfp->mo_header.magic,mh32.magic);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.cputype,mh32.cputype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.cpusubtype,
        mh32.cpusubtype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.filetype,mh32.filetype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.ncmds,mh32.ncmds);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.sizeofcmds,
        mh32.sizeofcmds);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.flags,mh32.flags);
    mfp->mo_header.reserved = 0;
    mfp->mo_command_count = (unsigned int)mfp->mo_header.ncmds;
    if (mfp->mo_header.sizeofcmds >= mfp->mo_filesize ||
        mfp->mo_header.sizeofcmds >= MAX_COMMANDS_SIZE ) {
        *errcode = DW_DLE_MACHO_CORRUPT_HEADER;
        return DW_DLV_ERROR;
    }
    mfp->mo_machine = mfp->mo_header.cputype;
    mfp->mo_flags = mfp->mo_header.flags;
    mfp->mo_command_start_offset = sizeof(mh32);
    return DW_DLV_OK;
}

int
_dwarf_load_macho_header(dwarf_macho_object_access_internals_t *mfp,
    int *errcode)
{
    int res = 0;

    if (mfp->mo_offsetsize == 32) {
        res = load_macho_header32(mfp,errcode);
    } else if (mfp->mo_offsetsize == 64) {
        res = _dwarf_load_macho_header64(mfp,errcode);
    } else {
        *errcode = DW_DLE_OFFSET_SIZE;
        return DW_DLV_ERROR;
    }
    return res;
}

static int
load_segment_command_content32(
    dwarf_macho_object_access_internals_t *mfp,
    struct generic_macho_command *mmp,
    struct generic_macho_segment_command *msp,
    Dwarf_Unsigned mmpindex,
    int *errcode)
{
    struct segment_command sc;
    int res = 0;
    Dwarf_Unsigned filesize = mfp->mo_filesize;
    Dwarf_Unsigned segoffset = mmp->offset_this_command;
    Dwarf_Unsigned afterseghdr = segoffset + sizeof(sc);
    Dwarf_Unsigned inner = mfp->mo_inner_offset;
    Dwarf_Unsigned fulloffset = mfp->mo_inner_offset;

    if (segoffset > filesize ||
        mmp->cmdsize > filesize ||
        (mmp->cmdsize + segoffset) > filesize ) {
        *errcode = DW_DLE_MACH_O_SEGOFFSET_BAD;
        return DW_DLV_ERROR;
    }
    fulloffset = segoffset+inner;
    if (fulloffset <segoffset || fulloffset < inner) { 
        /* overflow */
        *errcode = DW_DLE_ARITHMETIC_OVERFLOW;
        return DW_DLV_ERROR;
    }
    if (afterseghdr > filesize ) {
        *errcode = DW_DLE_MACH_O_SEGOFFSET_BAD;
        return DW_DLV_ERROR;
    }
    res = RRMOA(mfp->mo_fd, &sc, (inner+segoffset),
        sizeof(sc), (inner+filesize), errcode);
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
    if (msp->fileoff > mfp->mo_filesize ||
        msp->filesize > mfp->mo_filesize) {
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

static int
_dwarf_macho_load_segment_commands(
    dwarf_macho_object_access_internals_t *mfp,int *errcode)
{
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned segtotsize = 0;
    int            res = 0;
    struct generic_macho_command *mmp = 0;
    struct generic_macho_segment_command *msp = 0;

    if (mfp->mo_segment_count < 1) {
        return DW_DLV_OK;
    }
    res = _dwarf_uint64_mult(mfp->mo_segment_count,
        sizeof(struct generic_macho_segment_command),
        &segtotsize);
    if (res == DW_DLV_ERROR) {
        *errcode = DW_DLE_MACHO_CORRUPT_COMMAND;
        return DW_DLV_ERROR;
    }

    mfp->mo_segment_commands =
        (struct generic_macho_segment_command *)
        calloc(1, segtotsize);
    if (!mfp->mo_segment_commands) {
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }

    mmp = mfp->mo_commands;
    msp = mfp->mo_segment_commands;

    /*  This is a heuristic sanity check for a badly
        damaged object.
        See dwarfbug DW202412-009. */
    if ( mfp->mo_header.sizeofcmds > MAX_COMMANDS_SIZE) {
        *errcode = DW_DLE_MACHO_SEGMENT_COUNT_HEURISTIC_FAIL;
        return DW_DLV_ERROR;
    }
    for (i = 0 ; i < mfp->mo_command_count; ++i,++mmp) {
        unsigned cmd = (unsigned)mmp->cmd;

        if (cmd == LC_SEGMENT) {
            res = load_segment_command_content32(mfp,mmp,msp,
                i,errcode);
            ++msp;
        } else if (cmd == LC_SEGMENT_64) {
            res = _dwarf_load_segment_command_content64(mfp,mmp,msp,
                i,errcode);
            ++msp;
        } else { /* fall through, not a command of interest */ }
        if (res != DW_DLV_OK) {
            free(mfp->mo_segment_commands);
            mfp->mo_segment_commands = 0;
            return res;
        }
    }
    return DW_DLV_OK;
}

static int
_dwarf_macho_load_dwarf_section_details32(
    dwarf_macho_object_access_internals_t *mfp,
    struct generic_macho_segment_command *segp,
    Dwarf_Unsigned segi, int *errcode)
{
    int res = 0;
    Dwarf_Unsigned seci = 0;
    Dwarf_Unsigned seccount = segp->nsects;
    Dwarf_Unsigned secalloc = seccount+1;

    /* offset of sections being added */
    Dwarf_Unsigned curoff = segp->sectionsoffset;
    Dwarf_Unsigned shdrlen = sizeof(struct section);
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
                is therefore approximate sanity test. */
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
        struct section mosec;
        Dwarf_Unsigned endoffset = 0;
        Dwarf_Unsigned inner = mfp->mo_inner_offset;
        Dwarf_Unsigned offplussize = 0;
        Dwarf_Unsigned innercur = 0;

        endoffset = curoff + sizeof(mosec);
        if (curoff >=  mfp->mo_filesize ||
            endoffset > mfp->mo_filesize) {
            *errcode  = DW_DLE_MACHO_CORRUPT_SECTIONDETAILS;
            return DW_DLV_ERROR;
        }
        innercur = inner+curoff;
        if (innercur < inner || innercur <curoff) {
            /* overflow */
            *errcode = DW_DLE_MACHO_CORRUPT_SECTIONDETAILS;
            return DW_DLV_ERROR;
        }
        offplussize = inner+mfp->mo_filesize;
        if (offplussize < inner || offplussize <mfp->mo_filesize) {
            /* overflow */
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
        _dwarf_safe_strcpy(secs->segname, sizeof(secs->segname),
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
            not dSYM, so do not check here:
            No check for __text.
            So all sections in __DWARF checked  */
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
        secs->generic_segment_num  = segi;
        secs->offset_of_sec_rec = curoff;
    }
    return DW_DLV_OK;
}

static int
_dwarf_macho_load_dwarf_section_details(
    dwarf_macho_object_access_internals_t *mfp,
    struct generic_macho_segment_command *segp,
    Dwarf_Unsigned segi,int *errcode)
{
    int res = 0;

    if (mfp->mo_offsetsize == 32) {
        res = _dwarf_macho_load_dwarf_section_details32(mfp,
            segp,segi,errcode);
    } else if (mfp->mo_offsetsize == 64) {
        res = _dwarf_macho_load_dwarf_section_details64(mfp,
            segp,segi,errcode);
    } else {
        *errcode = DW_DLE_OFFSET_SIZE;
        return DW_DLV_ERROR;
    }
    return res;
}

static int
_dwarf_macho_load_dwarf_sections(
    dwarf_macho_object_access_internals_t *mfp,int *errcode)
{
    Dwarf_Unsigned segi = 0;
    Dwarf_Unsigned ftype = mfp->mo_header.filetype;

    struct generic_macho_segment_command *segp =
        mfp->mo_segment_commands;
    if (ftype != MH_DSYM &&
        ftype != MH_EXECUTE &&
        ftype != MH_OBJECT) {
        /* We do not think it can have DWARF */
        return DW_DLV_OK;
    }
    if (mfp->mo_segment_count > MAX_COMMANDS_SIZE) {
        /*  It's really bad, the size is supposed to be size on disk,
            so this is a likely silly check. */
        *errcode = DW_DLE_MACHO_CORRUPT_SECTIONDETAILS;
        return DW_DLV_ERROR;
    }
    for ( ; segi < mfp->mo_segment_count; ++segi,++segp) {
        int res = 0;
        if (0 == strcmp(segp->segname,"__PAGEZERO")) {
            continue;
        }
        if (0 ==strcmp(segp->segname,"__LINKEDIT")) {
            continue;
        }
        if (0 ==strcmp(segp->segname,"__DATA")) {
            continue;
        }
        res = _dwarf_macho_load_dwarf_section_details(mfp,
            segp,segi,errcode);
        if (res != DW_DLV_OK) {
            return res;
        }
    }
    return DW_DLV_OK;
}

/* Works the same, 32 or 64 bit */
int
_dwarf_load_macho_commands(
    dwarf_macho_object_access_internals_t *mfp,int *errcode)
{
    Dwarf_Unsigned cmdi = 0;
    Dwarf_Unsigned curoff = mfp->mo_command_start_offset;
    struct load_command mc;
    struct generic_macho_command *mcp = 0;
    unsigned segment_command_count = 0;
    int res = 0;
    Dwarf_Unsigned inner = mfp->mo_inner_offset;
    Dwarf_Unsigned commandsizetotal = 0;

    if (mfp->mo_command_count >= mfp->mo_filesize) {
        /* corrupt object. */
        *errcode = DW_DLE_MACH_O_SEGOFFSET_BAD;
        return DW_DLV_ERROR;
    }
    if ((curoff + mfp->mo_command_count * sizeof(mc)) >=
        mfp->mo_filesize) {
        /* corrupt object. */
        *errcode = DW_DLE_MACH_O_SEGOFFSET_BAD;
        return DW_DLV_ERROR;
    }

    mfp->mo_commands = (struct generic_macho_command *) calloc(
        mfp->mo_command_count,sizeof(struct generic_macho_command));
    if (!mfp->mo_commands) {
        /* out of memory */
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    mcp = mfp->mo_commands;
    for ( ; cmdi < mfp->mo_header.ncmds; ++cmdi,++mcp ) {
        res = RRMOA(mfp->mo_fd, &mc,
            inner+curoff, sizeof(mc),
            inner+mfp->mo_filesize, errcode);
        if (res != DW_DLV_OK) {
            free(mfp->mo_commands);
            mfp->mo_commands = 0;
            return res;
        }
        ASNAR(mfp->mo_copy_word,mcp->cmd,mc.cmd);
        ASNAR(mfp->mo_copy_word,mcp->cmdsize,mc.cmdsize);
        commandsizetotal += mcp->cmdsize;
        mcp->offset_this_command = curoff;
        curoff += mcp->cmdsize;
        if (mcp->cmdsize > mfp->mo_filesize ||
            curoff > mfp->mo_filesize) {
            /* corrupt object */
            free(mfp->mo_commands);
            mfp->mo_commands = 0;
            *errcode = DW_DLE_FILE_OFFSET_BAD;
            return DW_DLV_ERROR;
        }
        if (mcp->cmd == LC_SEGMENT || mcp->cmd == LC_SEGMENT_64) {
            segment_command_count++;
        }
    }
    mfp->mo_segment_count = segment_command_count;
    mfp->mo_segment_size_total = commandsizetotal;
    res = _dwarf_macho_load_segment_commands(mfp,errcode);
    if (res != DW_DLV_OK) {
        free(mfp->mo_commands);
        mfp->mo_commands = 0;
        return res;
    }
    res = _dwarf_macho_load_dwarf_sections(mfp,errcode);
    if (res != DW_DLV_OK) {
        free(mfp->mo_commands);
        mfp->mo_commands = 0;
    }
    return res;
}
int
_dwarf_macho_setup(int fd,
    char *true_path,
    unsigned universalnumber,
    unsigned ftype,
    unsigned endian,
    unsigned offsetsize,
    Dwarf_Unsigned filesize,
    unsigned groupnumber,
    Dwarf_Handler errhand,
    Dwarf_Ptr errarg,
    Dwarf_Debug *dbg,Dwarf_Error *error)
{
    Dwarf_Obj_Access_Interface_a *binary_interface = 0;
    dwarf_macho_object_access_internals_t *intfc = 0;
    int res = DW_DLV_OK;
    int localerrnum = 0;
    unsigned universalbinary_count = 0;

    res = _dwarf_macho_object_access_init(
        fd,
        universalnumber,
        ftype,endian,offsetsize,
        &universalbinary_count,
        filesize,
        &binary_interface,
        &localerrnum);
    if (res != DW_DLV_OK) {
        if (res == DW_DLV_NO_ENTRY) {
            return res;
        }
        _dwarf_error(NULL, error, localerrnum);
        return DW_DLV_ERROR;
    }
    /*  allocates and initializes Dwarf_Debug,
        generic code */
    res = dwarf_object_init_b(binary_interface, errhand, errarg,
        groupnumber, dbg, error);
    if (res != DW_DLV_OK){
        _dwarf_destruct_macho_access(binary_interface);
        return res;
    }
    intfc = binary_interface->ai_object;
    intfc->mo_path = strdup(true_path);
    (*dbg)->de_obj_flags = intfc->mo_flags;
    (*dbg)->de_obj_machine = intfc->mo_machine;
    (*dbg)->de_universalbinary_index = universalnumber;
    (*dbg)->de_universalbinary_count = universalbinary_count;
    return res;
}

/*  Initialize the methods needed to read Macho object files. */
static Dwarf_Obj_Access_Methods_a const macho_methods = {
    macho_get_section_info,
    macho_get_byte_order,
    macho_get_length_size,
    macho_get_pointer_size,
    macho_get_file_size,
    macho_get_section_count,
    macho_load_section,
    /*  We do not do macho relocations.
        dsym files do not require it. */
    0,
    /*Not handling mmap yet. */
    0,
    _dwarf_destruct_macho_access

};

/* Reads universal binary headers, gets to
   the chosen inner binary, and returns the
   values from the inner binary.
   The filesize being that of the inner binary,
   and the fileoffset being the offset of the inner
   binary (so by definition > 0);
*/

static int
_dwarf_macho_inner_object_fd(int fd,
    unsigned int uninumber,
    Dwarf_Unsigned outer_filesize,
    unsigned int *ftype,
    unsigned int *unibinarycount,
    unsigned int *endian,
    unsigned int *offsetsize,
    Dwarf_Unsigned *fileoffset,
    Dwarf_Unsigned *filesize,
    int *errcode)
{
    int res = 0;
    Dwarf_Universal_Head  head = 0;
    Dwarf_Unsigned innerbase = 0;
    Dwarf_Unsigned innersize = 0;

    res =  _dwarf_object_detector_universal_head_fd(
        fd, outer_filesize, unibinarycount,
        &head, errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    if (uninumber >= *unibinarycount) {
        *errcode = DW_DLE_UNIVERSAL_BINARY_ERROR;
        _dwarf_dealloc_universal_head(head);
        return DW_DLV_ERROR;
    }
    /*  Now find the precise details of uninumber
        instance we want */

    innerbase = head->au_arches[uninumber].au_offset;
    innersize = head->au_arches[uninumber].au_size;
    if (innersize >= outer_filesize ||
        innerbase >= outer_filesize) {
        *errcode = DW_DLE_UNIVERSAL_BINARY_ERROR;
        _dwarf_dealloc_universal_head(head);
        return DW_DLV_ERROR;
    }
    /* Now access inner to return its specs */
    {
        /*  But ignore the size this returns!
            we determined that above. the following call
            does not get the inner size, we got that
            just above here! */
        Dwarf_Unsigned fake_size = 0;

        res = _dwarf_object_detector_fd_a(fd,
            ftype,endian,offsetsize,innerbase,&fake_size,
            errcode);
        if (res != DW_DLV_OK) {
            _dwarf_dealloc_universal_head(head);
            return res;
        }
    }
    *fileoffset = innerbase;
    *filesize = innersize;
    _dwarf_dealloc_universal_head(head);
    return DW_DLV_OK;
}

/*  On any error this frees internals argument. */
static int
_dwarf_macho_object_access_internals_init(
    dwarf_macho_object_access_internals_t * internals,
    int  fd,
    unsigned uninumber,
    unsigned ftype,
    unsigned endian,
    unsigned offsetsize,
    unsigned *unibinarycount,
    Dwarf_Unsigned filesize,
    int *errcode)
{
    Dwarf_Unsigned i  = 0;
    struct generic_macho_section *sp = 0;
    int res = 0;
    unsigned int   ftypei = ftype;
    unsigned int   endiani = endian;
    unsigned int   offsetsizei = offsetsize;
    Dwarf_Unsigned filesizei = filesize;
    Dwarf_Unsigned fileoffseti = 0;
    unsigned int unibinarycounti = 0;

    if (ftype == DW_FTYPE_APPLEUNIVERSAL) {
        Dwarf_Unsigned endoffset = 0;
        res = _dwarf_macho_inner_object_fd(fd,
            uninumber,
            filesize,
            &ftypei,&unibinarycounti,&endiani,
            &offsetsizei,&fileoffseti,&filesizei,errcode);
        if (res != DW_DLV_OK) {
            return res;
        }
        /*  At this point filesize is of the entire universal binary
            file, filesizei is size of the uninumber-th macho binary 
            in the overall file. 
            fileoffseti is the offset of the uninumber-th
            macho binary in the overall file */
        endoffset = fileoffseti+filesizei;
        if (endoffset < fileoffseti ||
            endoffset < filesizei) {
            /* overflow */
            *errcode = DW_DLE_UNIVERSAL_BINARY_ERROR;
            return DW_DLV_ERROR;
        }
        *unibinarycount = unibinarycounti;
        endian = endiani;
    }

    internals->mo_ident[0]    = 'M';
    internals->mo_ident[1]    = '1';
    internals->mo_fd          = fd;
    internals->mo_offsetsize  = offsetsizei;
    internals->mo_pointersize = offsetsizei;
    internals->mo_inner_offset  = fileoffseti;
    internals->mo_filesize    = filesizei;
    internals->mo_ftype       = ftypei;
    internals->mo_uninumber   = uninumber;
    internals->mo_universal_count = unibinarycounti;

#ifdef WORDS_BIGENDIAN
    if (endian == DW_END_little ) {
        internals->mo_copy_word = _dwarf_memcpy_swap_bytes;
        internals->mo_endian = DW_END_little;
    } else {
        internals->mo_copy_word = _dwarf_memcpy_noswap_bytes;
        internals->mo_endian = DW_END_big;
    }
#else  /* LITTLE ENDIAN */
    if (endian == DW_END_little ) {
        internals->mo_copy_word = _dwarf_memcpy_noswap_bytes;
        internals->mo_endian = DW_END_little;
    } else {
        internals->mo_copy_word = _dwarf_memcpy_swap_bytes;
        internals->mo_endian = DW_END_big;
    }
#endif /* LITTLE- BIG-ENDIAN */
    res = _dwarf_load_macho_header(internals,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    /* Load sections */
    res = _dwarf_load_macho_commands(internals,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    if (internals->mo_dwarf_sections) {
        sp = internals->mo_dwarf_sections+1;
    } else {
        /*  There are no dwarf sections, but could be .text */
    }
    for (i = 1; i < internals->mo_dwarf_sectioncount ; ++i,++sp) {
        int j = 1;
        int lim = sizeof(SectionNames)/sizeof(SectionNames[0]);
        sp->dwarfsectname = "";
        for ( ; j < lim; ++j) {
            if (!strcmp(sp->sectname,SectionNames[j].ms_moname)) {
                sp->dwarfsectname = SectionNames[j].ms_dwname;
                break;
            }
        }
        if (sp->dwarfsectname[0] == 0) {
            /* if not matched, keep the apple section name */
            sp->dwarfsectname = sp->sectname;
        }
    }
    return DW_DLV_OK;
}

static int
_dwarf_macho_object_access_init(
    int  fd,
    unsigned uninumber,
    unsigned ftype,
    unsigned endian,
    unsigned offsetsize,
    unsigned * universalbinary_count,
    Dwarf_Unsigned filesize,
    Dwarf_Obj_Access_Interface_a **binary_interface,
    int *localerrnum)
{
    int res = 0;
    dwarf_macho_object_access_internals_t *internals = 0;
    Dwarf_Obj_Access_Interface_a *intfc = 0;

    internals = malloc(
        sizeof(dwarf_macho_object_access_internals_t));
    if (!internals) {
        *localerrnum = DW_DLE_ALLOC_FAIL;
        /* Impossible case, we hope. Give up. */
        return DW_DLV_ERROR;
    }
    memset(internals,0,sizeof(*internals));
    res = _dwarf_macho_object_access_internals_init(internals,
        fd,
        uninumber,
        ftype, endian, offsetsize,
        universalbinary_count,
        filesize,
        localerrnum);
    if (res != DW_DLV_OK){
        _dwarf_destruct_macho_internals(internals);
        return res;
    }
    intfc = malloc(sizeof(Dwarf_Obj_Access_Interface_a));
    if (!intfc) {
        /* Impossible case, we hope. Give up. */
        _dwarf_destruct_macho_internals(internals);
        *localerrnum = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    /* Initialize the interface struct */
    intfc->ai_object = internals;
    intfc->ai_methods = &macho_methods;
    *binary_interface = intfc;
    return DW_DLV_OK;
}

static unsigned long
magic_copy(unsigned char *d, unsigned len)
{
    unsigned i = 0;
    unsigned long v = 0;

    v = d[0];
    for (i = 1 ; i < len; ++i) {
        v <<= 8;
        v |=  d[i];
    }
    return v;
}

/*  fa points to fat arch in object-file format
    duhd points to space to fill in (in au_rches) data
    for each macho object in the fat object. */
static int
fill_in_uni_arch_32(
    struct fat_arch * fa,
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
        dua->au_reserved = 0;
    }
    return DW_DLV_OK;
}

static const struct Dwarf_Universal_Head_s duhzero;
static const struct fat_header fhzero;
static int
_dwarf_object_detector_universal_head_fd(
    int fd,
    Dwarf_Unsigned        dw_filesize,
    unsigned int         *dw_contentcount,
    Dwarf_Universal_Head * dw_head,
    int                *errcode)
{
    struct Dwarf_Universal_Head_s  duhd;
    struct Dwarf_Universal_Head_s *duhdp = 0;
    struct  fat_header fh;
    int     res = 0;
    void (*word_swap) (void *, const void *, unsigned long);
    int     locendian = 0;
    int     locoffsetsize = 0;

    duhd = duhzero;
    fh = fhzero;
    /*  A universal head is always at offset zero. */
    duhd.au_filesize = dw_filesize;
    if (sizeof(fh) >= dw_filesize) {
        *errcode = DW_DLE_UNIVERSAL_BINARY_ERROR;
        return DW_DLV_ERROR;
    }
    res = RRMOA(fd,&fh,0,sizeof(fh), dw_filesize,errcode);
    if (res != DW_DLV_OK) {
        return res;

    }
    duhd.au_magic = magic_copy((unsigned char *)&fh.magic[0],4);
    if (duhd.au_magic == FAT_MAGIC) {
        locendian = DW_END_big;
        locoffsetsize = 32;
    } else if (duhd.au_magic == FAT_CIGAM) {
        locendian = DW_END_little;
        locoffsetsize = 32;
    }else if (duhd.au_magic == FAT_MAGIC_64) {
        locendian = DW_END_big;
        locoffsetsize = 64;
    } else if (duhd.au_magic == FAT_CIGAM_64) {
        locendian = DW_END_little;
        locoffsetsize = 64;
    } else {
        *errcode = DW_DLE_FILE_WRONG_TYPE;
        return DW_DLV_ERROR;
    }
#ifdef WORDS_BIGENDIAN
    if (locendian == DW_END_little) {
        word_swap = _dwarf_memcpy_swap_bytes;
    } else {
        word_swap = _dwarf_memcpy_noswap_bytes;
    }
#else  /* LITTLE ENDIAN */
    if (locendian == DW_END_little) {
        word_swap = _dwarf_memcpy_noswap_bytes;
    } else {
        word_swap = _dwarf_memcpy_swap_bytes;
    }
#endif /* LITTLE- BIG-ENDIAN */
    ASNAR(word_swap,duhd.au_count,fh.nfat_arch);
    /*  The limit is a first-cut safe heuristic. */
    if (duhd.au_count >= (dw_filesize/2) ) {
        *errcode = DW_DLE_UNIVERSAL_BINARY_ERROR ;
        return DW_DLV_ERROR;
    }
    duhd.au_arches = (struct  Dwarf_Universal_Arch_s*)
        calloc(duhd.au_count, sizeof(struct Dwarf_Universal_Arch_s));
    if (!duhd.au_arches) {
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    if (locoffsetsize == 32) {
        struct fat_arch * fa = 0;

        fa = (struct fat_arch *)calloc(duhd.au_count,
            sizeof(struct fat_arch));
        if (!fa) {
            free(duhd.au_arches);
            duhd.au_arches = 0;
            free(fa);
            *errcode = DW_DLE_ALLOC_FAIL;
            return DW_DLV_ERROR;
        }
        if (sizeof(fh)+duhd.au_count*sizeof(*fa) >= dw_filesize) {
            free(duhd.au_arches);
            duhd.au_arches = 0;
            free(fa);
            *errcode = DW_DLE_FILE_OFFSET_BAD;
            return DW_DLV_ERROR;
        }
        res = RRMOA(fd,fa,/*offset=*/sizeof(fh),
            duhd.au_count*sizeof(*fa),
            dw_filesize,errcode);
        if (res != DW_DLV_OK) {
            free(duhd.au_arches);
            duhd.au_arches = 0;
            free(fa);
            return res;
        }
        res = fill_in_uni_arch_32(fa,&duhd,word_swap,
            errcode);
        free(fa);
        fa = 0;
        if (res != DW_DLV_OK) {
            free(duhd.au_arches);
            duhd.au_arches = 0;
            return res;
        }
    } else { /* 64 */
        struct fat_arch_64 * fa = 0;
        fa = (struct fat_arch_64 *)calloc(duhd.au_count,
            sizeof(struct fat_arch_64));
        if (!fa) {
            free(duhd.au_arches);
            duhd.au_arches = 0;
            *errcode = DW_DLE_ALLOC_FAIL;
            return DW_DLV_ERROR;
        }
        if (sizeof(fh)+duhd.au_count*sizeof(*fa) >= dw_filesize) {
            free(duhd.au_arches);
            duhd.au_arches = 0;
            free(fa);
            *errcode = DW_DLE_FILE_OFFSET_BAD ;
            return DW_DLV_ERROR;
        }
        res = RRMOA(fd,fa,/*offset*/sizeof(fh),
            duhd.au_count*sizeof(fa),
            dw_filesize,errcode);
        if (res == DW_DLV_ERROR) {
            /* *errcode set by RRMOA */
            free(duhd.au_arches);
            duhd.au_arches = 0;
            free(fa);
            return res;
        }
        res = _dwarf_fill_in_uni_arch_64(fa,&duhd,word_swap,
            errcode);
        free(fa);
        fa = 0;
        if (res != DW_DLV_OK) {
            free(duhd.au_arches);
            duhd.au_arches = 0;
            return res;
        }
    }

    duhdp = malloc(sizeof(*duhdp));
    if (!duhdp) {
        free(duhd.au_arches);
        duhd.au_arches = 0;
        *errcode = DW_DLE_ALLOC_FAIL;
        return res;
    }
    memcpy(duhdp,&duhd,sizeof(duhd));
    *dw_contentcount = (unsigned int)duhd.au_count;
    duhdp->au_arches = duhd.au_arches;
    *dw_head = duhdp;
    return DW_DLV_OK;
}

void
_dwarf_dealloc_universal_head(Dwarf_Universal_Head dw_head)
{
    if (!dw_head) {
        return;
    }
    free(dw_head->au_arches);
    dw_head->au_arches = 0;
    free(dw_head);
}
