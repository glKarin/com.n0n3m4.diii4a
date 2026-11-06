/* Copyright 2018-2026 David Anderson. All rights reserved.

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

/*
This reads elf headers and creates generic-elf
structures containing the Elf headers.

These two enums used for type safety in passing
values.  See dwarf_elfread.h

enum RelocRela
enum RelocOffsetSize

dwarf_elfread.c
calls
    _dwarf_load_elf_relx(intfc,i,...,enum RelocRela,errcode)
        calls _dwarf_elf_load_a_relx_batch(ep,...enum RelocRela,
            enum RelocOffsetSize,errcode)
            which calls generic_rel_from_rela32(ep,gsh,relp,grel
            or    calls generic_rel_from_rela64(ep,gsh,relp,grel
            or    calls generic_rel_from_rel32(ep,gsh,relp,grel...
            or    calls generic_rel_from_rel64(ep,gsh,relp,grel...
*/

#include <config.h>

#include <stddef.h> /* size_t */
#include <stdlib.h> /* calloc() free() malloc() */
#include <stdio.h> /* printf debugging */
#include <string.h> /* memcpy() strcmp() strdup()
    strlen() strncmp() */

#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for exit(), C89 malloc */
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_MALLOC_H
/* Useful include for some Windows compilers. */
#include <malloc.h>
#endif /* HAVE_MALLOC_H */
#include <sys/types.h>   /* for open() */
#include <sys/stat.h>   /* for open() */
#include <fcntl.h>   /* for open() */
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* lseek read close */
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_ZLIB_H
#include "zlib.h"
#endif /* ZLIB */
#ifdef HAVE_ZSTD_H
#include "zstd.h"
#endif /* ZSTD */

#include "dwarf.h"
#include "libdwarf.h"
#include "dwarf_local_malloc.h"
#include "libdwarf_private.h"
#include "dwarf_base_types.h"
#include "dwarf_opaque.h"
#include "dwarf_memcpy_swap.h"
#include "dwarf_reading.h"
#include "dwarf_elf_defines.h"
#include "dwarf_elfstructs.h"
#include "dwarf_elfread.h"
#include "dwarf_object_detector.h"
#include "dwarf_object_read_common.h"
#include "dwarf_util.h"
#include "dwarf_secname_ck.h"

#if 0 /* debugging only dumpsizes() */
/*  One example of calling this.
    place just before DW_DLE_SECTION_SIZE_OR_OFFSET_LARGE
    dumpsizes(__LINE__,strsectlength,strpsh->gh_offset,
    ep->f_filesize);
*/
static void
dumpsizes(int line,Dwarf_Unsigned s,
    Dwarf_Unsigned o,
    Dwarf_Unsigned fsz)
{
    Dwarf_Unsigned tlen = s + o;
    printf("Size Error DEBUGONLY sz 0x%llx off 0x%llx fsx 0x%llx "
        "sum 0x%llx line %d \n",
        s,o,fsz,tlen,line);
}
#endif /*0*/

/*  The following actually assumes (as used here)
    that t is 8 bytes (integer) while s is
    l bytes.
    Used only in dwarf_elf_load_headers.c for
    compressed sections.
    Just slightly different from the ASNAR generally
    used in libdwarf.  */
#ifdef WORDS_BIGENDIAN
#define ASNARLRAW(dwcopy,ec,t,s,l)        \
    do {                                  \
        unsigned tbyte = sizeof(t) - (l); \
        *ec = 0;                           \
        if (sizeof(t) < (l)) {            \
            *ec = DW_DLE_ZLIB_UNCOMPRESS_ERROR; \
        }                                 \
        (t) = 0;                          \
        dwcopy(((char *)&(t))+tbyte ,&(s)[0],(l));\
    } while (0)
#else /* LITTLE ENDIAN */
#define ASNARLRAW(dwcopy,ec,t,s,l)    \
    do {                              \
        *ec = 0;                       \
        if (sizeof(t) < (l)) {        \
            *ec = DW_DLE_ZLIB_UNCOMPRESS_ERROR; \
        }                             \
        dwcopy(&(t),&(s)[0],(l));     \
    } while (0)
#endif /* end LITTLE- BIG-ENDIAN */

int nibblecounts[16] = {
0,1,1,2,
1,2,2,3,
2,2,2,3,
2,3,3,4
};

static int
getbitsoncount(Dwarf_Unsigned v_in)
{
    int           bitscount = 0;
    Dwarf_Unsigned v = v_in;

    while (v) {
        unsigned int nibble = v & 0xf;
        bitscount += nibblecounts[nibble];
        v >>= 4;
    }
    return bitscount;
}

#if defined(HAVE_ZLIB) && defined(HAVE_ZSTD)
/*  This is exclusively for reading .symtab and .symstr
    sections. See dwarf_elf_init() for decompressing all
    other sections. We need decompress to do relocations (if any
    relocations and if either of these sections compressed).  */
int
_dwarf_do_decompress_elf(dwarf_elf_object_access_internals_t *ep,
    struct generic_shdr *psh,
    int* error)
{
    Dwarf_Small   *basesrc = 0;
    Dwarf_Small   *dest = 0;
    Dwarf_Unsigned destlen = 0;
    Dwarf_Unsigned srclen = 0;
    Dwarf_Unsigned flags = 0;
    Dwarf_Small   *endsection = 0;
    int            zstdcompress = FALSE;
    Dwarf_Unsigned uncompressed_len = 0;

    basesrc = (Dwarf_Small*)psh->gh_content;
    srclen = psh->gh_size;
    flags = psh->gh_flags;

    endsection = basesrc + srclen;
    if ((basesrc + 12) > endsection) {
        *error = DW_DLE_ZLIB_SECTION_SHORT;
        /*_dwarf_error_string(dbg, error,DW_DLE_ZLIB_SECTION_SHORT,
            "DW_DLE_ZLIB_SECTION_SHORT"
            "Section too short to be either zlib or zstd related"); */
        return DW_DLV_ERROR;
    }
    uncompressed_len = 0;
    /*  We are looking at the first bytes of the section content,
        not a section name string. */
    if (!strncmp("ZLIB",(const char *)basesrc,4)) {
        /*  This should be impossible */
        unsigned i = 0;
        unsigned l = 8;
        unsigned char *c = basesrc+4;
        for ( ; i < l; ++i,c++) {
            uncompressed_len <<= 8;
            uncompressed_len += *c;
        }
        basesrc = basesrc + 12;
        srclen -= 12;
        *error = DW_DLE_ZLIB_SECTION_SHORT;
        return DW_DLV_OK;
    } else  if (flags & SHF_COMPRESSED) {
        /*  The prefix is a struct:
            unsigned int type; followed by pad if following are 64bit!
            size-of-target-address size
            size-of-target-address
        */
        Dwarf_Small *ptr    = (Dwarf_Small *)basesrc;
        Dwarf_Unsigned type = 0;
        Dwarf_Unsigned size = 0;
        /* Dwarf_Unsigned addralign = 0; */
        unsigned fldsize    = ep->f_pointersize/8;
        unsigned structsize = 3* fldsize;
        ASNARLRAW(ep->f_copy_word,error,type,ptr,DWARF_32BIT_SIZE);
        if (*error) {
            return DW_DLV_ERROR;
        }
        ptr += fldsize;
        ASNARLRAW(ep->f_copy_word,error,size,ptr,fldsize);
        if (*error) {
            return DW_DLV_ERROR;
        }
        type = psh->gh_type;
        switch(type) {
        case ELFCOMPRESS_ZLIB:
            break;
        case ELFCOMPRESS_ZSTD:
            zstdcompress = TRUE;
            break;
        default: {
            /* Likely a corrupt object file. */
            *error = DW_DLE_COMPRESSED_FORMAT_ODD;
            return DW_DLV_ERROR;
        }
        }
        uncompressed_len = size;
        basesrc    += structsize;
        srclen -= structsize;
    } else {
        /* Likely a corrupt object file. */
        *error = DW_DLE_COMPRESSED_FORMAT_UNKNOWN;
        return DW_DLV_ERROR;
    }
    /*  Dropped heuristic of excess compress inflation.
        Not reliable. */
    if ((basesrc +srclen) > endsection) {
        *error = DW_DLE_ZLIB_SECTION_SHORT;
        return DW_DLV_ERROR;
    }
    destlen = uncompressed_len;
    dest = malloc(destlen);
    if (!dest) {
        *error = DW_DLE_ALLOC_DECOMPRESS_FAIL;
        return DW_DLV_ERROR;
    }
    /*  uncompress is a zlib function. */
    if (!zstdcompress) {
        int res = 0;
        uLongf dlen = destlen;

        res = uncompress(dest,&dlen,basesrc,srclen);
        if (res == Z_BUF_ERROR) {
            free(dest);
            *error = DW_DLE_ZLIB_BUF_ERROR;
            return DW_DLV_ERROR;
        } else if (res == Z_MEM_ERROR) {
            free(dest);
            *error = DW_DLE_ZLIB_BUF_ERROR;
            return DW_DLV_ERROR;
        } else if (res != Z_OK) {
            free(dest);
            *error = DW_DLE_ZLIB_DATA_ERROR;
            return DW_DLV_ERROR;
        }
    }
    /*  ZSTD_decompress is a zstd function. */
    if (zstdcompress) {
        size_t zsize =
            ZSTD_decompress(dest,destlen,basesrc,srclen);
        if (zsize != destlen) {
            free(dest);
            *error = DW_DLE_ZSTD_DATA_ERROR;
            return DW_DLV_ERROR;
        }
    }
    /* Z_OK */
    free(psh->gh_content);
    psh->gh_content = (char *)dest;
    psh->gh_load_type = Dwarf_Alloc_Malloc;
    psh->gh_was_alloc = TRUE;
    psh->gh_size = destlen;
    psh->gh_compressed_len = srclen;
    return DW_DLV_OK;
}
#endif /*defined(HAVE_ZLIB) && defined(HAVE_ZSTD)*/

static int
_dwarf_load_elf_section_is_dwarf(const char *sname,
    Dwarf_Unsigned sectype,
    int *is_rela,int *is_rel)
{
    *is_rel = FALSE;
    *is_rela = FALSE;
    if (_dwarf_ignorethissection(sname)) {
        return FALSE;
    }
    if (sectype == SHT_REL) {
        return TRUE;
    }
    if (sectype == SHT_RELA) {
        *is_rela = TRUE;
        return TRUE;
    }
    if (!strncmp(sname,".rel",4)) {
        if (!strncmp(sname,".rela.",6)) {
            *is_rela = TRUE;
            return TRUE;
        }
        if (!strncmp(sname,".rel.",5)) {
            *is_rela = TRUE;
            return TRUE;
        }
        /*  Else something is goofy/Impossible */
        return FALSE;
    }
    if (!strncmp(sname,".debug_",7)) {
        return TRUE;
    }
    if (!strncmp(sname,".zdebug_",8)) {
        return TRUE;
    }
    if (!strcmp(sname,".eh_frame")) {
        return TRUE;
    }
    if (!strncmp(sname,".gdb_index",10)) {
        return TRUE;
    }
    return FALSE;
}

static int
is_empty_section(Dwarf_Unsigned type)
{
    if (type == SHT_NOBITS) {
        return TRUE;
    }
    if (type == SHT_NULL) {
        return TRUE;
    }
    return FALSE;
}

static int
generic_ehdr_from_32(dwarf_elf_object_access_internals_t *ep,
    struct generic_ehdr *ehdr, dw_elf32_ehdr *e,
    int *errcode)
{
    int i = 0;

    for (i = 0; i < EI_NIDENT; ++i) {
        ehdr->ge_ident[i] = e->e_ident[i];
    }
    ASNAR(ep->f_copy_word,ehdr->ge_type,e->e_type);
    ASNAR(ep->f_copy_word,ehdr->ge_machine,e->e_machine);
    ASNAR(ep->f_copy_word,ehdr->ge_version,e->e_version);
    ASNAR(ep->f_copy_word,ehdr->ge_entry,e->e_entry);
    ASNAR(ep->f_copy_word,ehdr->ge_phoff,e->e_phoff);
    ASNAR(ep->f_copy_word,ehdr->ge_shoff,e->e_shoff);
    ASNAR(ep->f_copy_word,ehdr->ge_flags,e->e_flags);
    ASNAR(ep->f_copy_word,ehdr->ge_ehsize,e->e_ehsize);
    ASNAR(ep->f_copy_word,ehdr->ge_phentsize,e->e_phentsize);
    ASNAR(ep->f_copy_word,ehdr->ge_phnum,e->e_phnum);
    ASNAR(ep->f_copy_word,ehdr->ge_shentsize,e->e_shentsize);
    ASNAR(ep->f_copy_word,ehdr->ge_shnum,e->e_shnum);
    ASNAR(ep->f_copy_word,ehdr->ge_shstrndx,e->e_shstrndx);
    if (!ehdr->ge_shoff) {
        return DW_DLV_NO_ENTRY;
    }
    if (ehdr->ge_shoff < sizeof(dw_elf32_ehdr)) {
        /* offset is inside the header! */
        *errcode = DW_DLE_TOO_FEW_SECTIONS;
        return DW_DLV_ERROR;
    }
    if (ehdr->ge_shstrndx == SHN_XINDEX) {
        ehdr->ge_strndx_extended = TRUE;
    } else {
        ehdr->ge_strndx_in_strndx = TRUE;
        if (ehdr->ge_shstrndx < 1) {
            *errcode = DW_DLE_NO_SECT_STRINGS;
            return DW_DLV_ERROR;
        }
    }
    /*  If !ge_strndx_extended && !ehdr->ge_shnum
        this is a very unusual case.  */
    if (!ehdr->ge_shnum) {
        ehdr->ge_shnum_extended = TRUE;
    } else {
        ehdr->ge_shnum_in_shnum = TRUE;
        if (!ehdr->ge_shnum) {
            return DW_DLV_NO_ENTRY;
        }
        if (ehdr->ge_shnum < 3) {
            *errcode = DW_DLE_TOO_FEW_SECTIONS;
            return DW_DLV_ERROR;
        }
    }
    if (ehdr->ge_shnum_in_shnum &&
        ehdr->ge_strndx_in_strndx &&
        (ehdr->ge_shstrndx >= ehdr->ge_shnum)) {
            *errcode = DW_DLE_NO_SECT_STRINGS;
            return DW_DLV_ERROR;
    }

    ep->f_machine = (unsigned int)ehdr->ge_machine;
    ep->f_ftype = (unsigned int)ehdr->ge_type;
    ep->f_ehdr = ehdr;
    ep->f_loc_ehdr.g_name = "Elf File Header";
    ep->f_loc_ehdr.g_offset = 0;
    ep->f_loc_ehdr.g_count = 1;
    ep->f_loc_ehdr.g_entrysize = sizeof(dw_elf32_ehdr);
    ep->f_loc_ehdr.g_totalsize = sizeof(dw_elf32_ehdr);
    return DW_DLV_OK;
}

static int
generic_ehdr_from_64(dwarf_elf_object_access_internals_t* ep,
    struct generic_ehdr *ehdr, dw_elf64_ehdr *e,
    int *errcode)
{
    int i = 0;

    for (i = 0; i < EI_NIDENT; ++i) {
        ehdr->ge_ident[i] = e->e_ident[i];
    }
    ASNAR(ep->f_copy_word,ehdr->ge_type,e->e_type);
    ASNAR(ep->f_copy_word,ehdr->ge_machine,e->e_machine);
    ASNAR(ep->f_copy_word,ehdr->ge_version,e->e_version);
    ASNAR(ep->f_copy_word,ehdr->ge_entry,e->e_entry);
    ASNAR(ep->f_copy_word,ehdr->ge_phoff,e->e_phoff);
    ASNAR(ep->f_copy_word,ehdr->ge_shoff,e->e_shoff);
    ASNAR(ep->f_copy_word,ehdr->ge_flags,e->e_flags);
    ASNAR(ep->f_copy_word,ehdr->ge_ehsize,e->e_ehsize);
    ASNAR(ep->f_copy_word,ehdr->ge_phentsize,e->e_phentsize);
    ASNAR(ep->f_copy_word,ehdr->ge_phnum,e->e_phnum);
    ASNAR(ep->f_copy_word,ehdr->ge_shentsize,e->e_shentsize);
    ASNAR(ep->f_copy_word,ehdr->ge_shnum,e->e_shnum);
    ASNAR(ep->f_copy_word,ehdr->ge_shstrndx,e->e_shstrndx);
    if (!ehdr->ge_shoff) {
        return DW_DLV_NO_ENTRY;
    }
    if (ehdr->ge_shoff < sizeof(dw_elf64_ehdr)) {
        /* zero or offset is inside the header! */
        *errcode = DW_DLE_TOO_FEW_SECTIONS;
        return DW_DLV_ERROR;
    }
    if (ehdr->ge_shstrndx == SHN_XINDEX) {
        ehdr->ge_strndx_extended = TRUE;
    } else {
        ehdr->ge_strndx_in_strndx = TRUE;
        if (ehdr->ge_shstrndx < 1) {
            *errcode = DW_DLE_NO_SECT_STRINGS;
            return DW_DLV_ERROR;
        }
    }
    if (!ehdr->ge_shnum) {
        ehdr->ge_shnum_extended = TRUE;
    } else {
        ehdr->ge_shnum_in_shnum = TRUE;
        if (!ehdr->ge_shnum) {
            return DW_DLV_NO_ENTRY;
        }
        if (ehdr->ge_shnum < 3) {
            *errcode = DW_DLE_TOO_FEW_SECTIONS;
            return DW_DLV_ERROR;
        }
    }
    if (ehdr->ge_shnum_in_shnum &&
        ehdr->ge_strndx_in_strndx &&
        (ehdr->ge_shstrndx >= ehdr->ge_shnum)) {
            *errcode = DW_DLE_NO_SECT_STRINGS;
            return DW_DLV_ERROR;
    }
    ep->f_machine = (unsigned int)ehdr->ge_machine;
    ep->f_ftype = (unsigned int)ehdr->ge_type;
    ep->f_ehdr = ehdr;
    ep->f_loc_ehdr.g_name = "Elf File Header";
    ep->f_loc_ehdr.g_offset = 0;
    ep->f_loc_ehdr.g_count = 1;
    ep->f_loc_ehdr.g_entrysize = sizeof(dw_elf64_ehdr);
    ep->f_loc_ehdr.g_totalsize = sizeof(dw_elf64_ehdr);
    return DW_DLV_OK;
}

static void
copysection32(
    dwarf_elf_object_access_internals_t *ep,
    struct generic_shdr *gshdr,
    dw_elf32_shdr *psh)
{
    ASNAR(ep->f_copy_word,gshdr->gh_name,psh->sh_name);
    ASNAR(ep->f_copy_word,gshdr->gh_type,psh->sh_type);
    ASNAR(ep->f_copy_word,gshdr->gh_flags,psh->sh_flags);
    ASNAR(ep->f_copy_word,gshdr->gh_addr,psh->sh_addr);
    ASNAR(ep->f_copy_word,gshdr->gh_offset,psh->sh_offset);
    ASNAR(ep->f_copy_word,gshdr->gh_size,psh->sh_size);
    ASNAR(ep->f_copy_word,gshdr->gh_link,psh->sh_link);
    ASNAR(ep->f_copy_word,gshdr->gh_info,psh->sh_info);
    ASNAR(ep->f_copy_word,gshdr->gh_addralign,psh->sh_addralign);
    ASNAR(ep->f_copy_word,gshdr->gh_entsize,psh->sh_entsize);
}

static int
generic_shdr_from_shdr32(dwarf_elf_object_access_internals_t *ep,
    Dwarf_Unsigned * count_out,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned entsize,
    Dwarf_Unsigned count,
    int *errcode)
{
    dw_elf32_shdr          *psh =0;
    dw_elf32_shdr          *orig_psh =0;
    struct generic_ehdr *ehdr = ep->f_ehdr;
    struct generic_shdr *gshdr =0;
    struct generic_shdr *orig_gshdr =0;
    Dwarf_Unsigned i = 0;
    int res = 0;

    *count_out = 0;
    psh = (dw_elf32_shdr *)calloc(count , entsize);
    if (!psh) {
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    gshdr = (struct generic_shdr *)calloc(count,sizeof(*gshdr));
    if (!gshdr) {
        free(psh);
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }

    orig_psh = psh;
    orig_gshdr = gshdr;
    res = RRMOA(ep->f_fd,psh,offset,count*entsize,
        ep->f_filesize,errcode);
    if (res != DW_DLV_OK) {
        free(orig_psh);
        free(orig_gshdr);
        return res;
    }
    for (i = 0; i < count;
        ++i,  psh++,gshdr++) {
        int isempty     = FALSE;
        int bitsoncount = 0;

        gshdr->gh_secnum = i;
        copysection32(ep,gshdr,psh);
        if (gshdr->gh_size >= ep->f_filesize &&
            gshdr->gh_type != SHT_NOBITS) {
            free(orig_psh);
            free(orig_gshdr);
            *errcode = DW_DLE_SECTION_SIZE_ERROR;
            return DW_DLV_ERROR;
        }
        isempty = is_empty_section(gshdr->gh_type);
        if (i == 0) {
            Dwarf_Unsigned shnum = 0;
            Dwarf_Unsigned shstrx = 0;

            /*  Catch errors asap */
            if (!ehdr->ge_shnum_extended) {
                shnum = gshdr->gh_size;
            }
            if (!ehdr->ge_strndx_extended) {
                shstrx = gshdr->gh_link;
            }
            /*  We require that section zero be 'empty'
                per the Elf ABI.
                gh_link and gh_size are sometimes used
                with the elf header, so we do not check
                them here. */
            if (!isempty || gshdr->gh_name || gshdr->gh_flags ||
                shnum || shstrx ||
                gshdr->gh_addr ||
                gshdr->gh_info) {
                free(orig_psh);
                free(orig_gshdr);
                *errcode = DW_DLE_IMPROPER_SECTION_ZERO;
                return DW_DLV_ERROR;
            }
        }
        bitsoncount = getbitsoncount(gshdr->gh_flags);
        if (bitsoncount > 8) {
            free(orig_psh);
            free(orig_gshdr);
            *errcode = DW_DLE_BAD_SECTION_FLAGS;
            return DW_DLV_ERROR;
        }

        if (gshdr->gh_type == SHT_REL || gshdr->gh_type == SHT_RELA){
            gshdr->gh_reloc_target_secnum = gshdr->gh_info;
        }
    }
    free(orig_psh);
    *count_out = count;
    ep->f_shdr = orig_gshdr;
    ep->f_loc_shdr.g_name = "Section Header";
    ep->f_loc_shdr.g_count = count;
    ep->f_loc_shdr.g_offset = offset;
    ep->f_loc_shdr.g_entrysize = sizeof(dw_elf32_shdr);
    ep->f_loc_shdr.g_totalsize = sizeof(dw_elf32_shdr)*count;
    return DW_DLV_OK;
}

static void
copysection64(
    dwarf_elf_object_access_internals_t *ep,
    struct generic_shdr *gshdr,
    dw_elf64_shdr *psh)
{
    ASNAR(ep->f_copy_word,gshdr->gh_name,psh->sh_name);
    ASNAR(ep->f_copy_word,gshdr->gh_type,psh->sh_type);
    ASNAR(ep->f_copy_word,gshdr->gh_flags,psh->sh_flags);
    ASNAR(ep->f_copy_word,gshdr->gh_addr,psh->sh_addr);
    ASNAR(ep->f_copy_word,gshdr->gh_offset,psh->sh_offset);
    ASNAR(ep->f_copy_word,gshdr->gh_size,psh->sh_size);
    ASNAR(ep->f_copy_word,gshdr->gh_link,psh->sh_link);
    ASNAR(ep->f_copy_word,gshdr->gh_info,psh->sh_info);
    ASNAR(ep->f_copy_word,gshdr->gh_addralign,psh->sh_addralign);
    ASNAR(ep->f_copy_word,gshdr->gh_entsize,psh->sh_entsize);
}

static int
generic_shdr_from_shdr64(dwarf_elf_object_access_internals_t *ep,
    Dwarf_Unsigned * count_out,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned entsize,
    Dwarf_Unsigned count,
    int *errcode)
{
    dw_elf64_shdr          *psh =0;
    dw_elf64_shdr          *orig_psh =0;
    struct generic_shdr *gshdr =0;
    struct generic_shdr *orig_gshdr =0;
    struct generic_ehdr *ehdr = ep->f_ehdr;
    Dwarf_Unsigned i = 0;
    int res = 0;

    *count_out = 0;
    psh = (dw_elf64_shdr *)calloc(count , entsize);
    if (!psh) {
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    gshdr = (struct generic_shdr *)calloc(count,sizeof(*gshdr));
    if (gshdr == 0) {
        free(psh);
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    orig_psh = psh;
    orig_gshdr = gshdr;
    res = RRMOA(ep->f_fd,psh,offset,count*entsize,
        ep->f_filesize,errcode);
    if (res != DW_DLV_OK) {
        free(orig_psh);
        free(orig_gshdr);
        return res;
    }
    for ( i = 0; i < count;
        ++i,  psh++,gshdr++) {
        int bitsoncount = 0;
        int isempty = FALSE;

        gshdr->gh_secnum = i;
        copysection64(ep,gshdr,psh);
        if (gshdr->gh_size >= ep->f_filesize &&
            gshdr->gh_type != SHT_NOBITS) {
            free(orig_psh);
            free(orig_gshdr);
            *errcode = DW_DLE_SECTION_SIZE_ERROR;
            return DW_DLV_ERROR;
        }
        isempty = is_empty_section(gshdr->gh_type);
        if (i == 0) {
            Dwarf_Unsigned shnum = 0;
            Dwarf_Unsigned shstrx = 0;

            /*  Catch errors asap */
            if (!ehdr->ge_shnum_extended) {
                shnum = gshdr->gh_size;
            }
            if (!ehdr->ge_strndx_extended) {
                shstrx = gshdr->gh_link;
            }
            /*  We require that section zero be 'empty'
                per the Elf ABI.
                But gh_link  and gh_size might be used for
                ge_shstrndx and ge_shnum, respectively*/
            if (!isempty || gshdr->gh_name || gshdr->gh_flags ||
                shnum || shstrx ||
                gshdr->gh_addr ||
                gshdr->gh_info) {
                free(orig_psh);
                free(orig_gshdr);
                *errcode = DW_DLE_IMPROPER_SECTION_ZERO;
                return DW_DLV_ERROR;
            }
        }
        bitsoncount = getbitsoncount(gshdr->gh_flags);
        if (bitsoncount > 8) {
            free(orig_psh);
            free(orig_gshdr);
            *errcode = DW_DLE_BAD_SECTION_FLAGS;
            return DW_DLV_ERROR;
        }

        if (gshdr->gh_type == SHT_REL ||
            gshdr->gh_type == SHT_RELA){
            gshdr->gh_reloc_target_secnum = gshdr->gh_info;
        }
    }
    free(orig_psh);
    *count_out = count;
    ep->f_shdr = orig_gshdr;
    ep->f_loc_shdr.g_name = "Section Header";
    ep->f_loc_shdr.g_count = count;
    ep->f_loc_shdr.g_offset = offset;
    ep->f_loc_shdr.g_entrysize = sizeof(dw_elf64_shdr);
    ep->f_loc_shdr.g_totalsize = sizeof(dw_elf64_shdr)*count;
    return DW_DLV_OK;
}

static int
_dwarf_generic_elf_load_symbols32(
    dwarf_elf_object_access_internals_t *ep,
    struct generic_symentry **gsym_out,
    struct generic_shdr *psh,
    Dwarf_Unsigned *count_out,int *errcode)
{
    Dwarf_Unsigned  ecount = 0;
    Dwarf_Unsigned  size2 = 0;
    Dwarf_Unsigned  offset =0;
    Dwarf_Unsigned  size =0;
    Dwarf_Unsigned  i = 0;
    dw_elf32_sym   *psym = 0;
    struct generic_symentry * gsym = 0;
    struct generic_symentry * orig_gsym = 0;
    Dwarf_Unsigned flags = 0;
    Dwarf_Small   *content = 0;
    int            res = 0;

    flags = psh->gh_flags;
    size = psh->gh_size;
    offset = psh->gh_offset;
    if (psh->gh_content) {
        *errcode = DW_DLE_ELF_SECTION_ERROR;
        return DW_DLV_ERROR;
    }
    if (psh->gh_sht_group_array ||
        psh->gh_sht_group_array_count ||
        psh->gh_content) {
        *errcode = DW_DLE_ELF_GRPSTRING_SECTION_ERROR;
        return DW_DLV_ERROR;
    }
    content = calloc(1,size);
    if (!content) {
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    res = RRMOA(ep->f_fd,content,offset,size,
        ep->f_filesize,errcode);
    if (res != DW_DLV_OK) {
        free(content);
        return res;
    }
    psh->gh_content = (char *)content;
    psh->gh_load_type = Dwarf_Alloc_Malloc;
    if (flags& SHF_COMPRESSED) {
#if defined(HAVE_ZLIB) && defined(HAVE_ZSTD)
        *errcode = 0;
        _dwarf_do_decompress_elf(ep,psh,errcode);
        /* decompress and set new section size */
        if (*errcode) {
            free(psh->gh_content);
            psh->gh_content = 0;
            return DW_DLV_ERROR;
        }
#else /* COMPRESSED TEST */
        free(psh->gh_content);
        psh->gh_content = 0;
        *errcode = DW_DLE_ZLIB_ZSTD_MISSING;
        return DW_DLV_ERROR;
#endif /* COMPRESSED TEST */
    }
    size = psh->gh_size;
    ecount = (long)(size/sizeof(dw_elf32_sym));
    size2 = ecount * sizeof(dw_elf32_sym);
    if (size != size2) {
        free(psh->gh_content);
        psh->gh_content = 0;
        *errcode = DW_DLE_SYMBOL_SECTION_SIZE_ERROR;
        return DW_DLV_ERROR;
    }
    if (size >= ep->f_filesize ) {
        free(psh->gh_content);
        psh->gh_content = 0;
        *errcode = DW_DLE_SYMBOL_SECTION_SIZE_ERROR;
        return DW_DLV_ERROR;
    }
    psym = (dw_elf32_sym *)psh->gh_content;
    gsym = calloc(ecount,sizeof(struct generic_symentry));
    if (!gsym) {
        free(psh->gh_content);
        psh->gh_content = 0;
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    orig_gsym = gsym;
    for ( i = 0; i < ecount; ++i,++psym,++gsym) {
        Dwarf_Unsigned bind = 0;
        Dwarf_Unsigned type = 0;

        ASNAR(ep->f_copy_word,gsym->gs_name,psym->st_name);
        ASNAR(ep->f_copy_word,gsym->gs_value,psym->st_value);
        ASNAR(ep->f_copy_word,gsym->gs_size,psym->st_size);
        ASNAR(ep->f_copy_word,gsym->gs_info,psym->st_info);
        ASNAR(ep->f_copy_word,gsym->gs_other,psym->st_other);
        ASNAR(ep->f_copy_word,gsym->gs_shndx,psym->st_shndx);
        bind = gsym->gs_info >> 4;
        type = gsym->gs_info & 0xf;
        gsym->gs_bind = bind;
        gsym->gs_type = type;
    }
    psh->gh_was_alloc = TRUE;
    psh->gh_load_type = Dwarf_Alloc_Malloc;
    *count_out = ecount;
    *gsym_out = orig_gsym;
    return DW_DLV_OK;
}

static int
_dwarf_generic_elf_load_symbols64(
    dwarf_elf_object_access_internals_t *ep,
    struct generic_symentry **gsym_out,
    struct generic_shdr *psh,
    Dwarf_Unsigned *count_out,int *errcode)
{
    Dwarf_Unsigned ecount = 0;
    Dwarf_Unsigned size = 0;
    Dwarf_Unsigned offset = 0;
    Dwarf_Unsigned size2 = 0;
    Dwarf_Unsigned i = 0;
    dw_elf64_sym  *psym = 0;
    struct generic_symentry * gsym = 0;
    struct generic_symentry * orig_gsym = 0;
    int            res = 0;
    Dwarf_Unsigned flags = 0;
    char *         content = 0;

    flags = psh->gh_flags;
    size = psh->gh_size;
    offset = psh->gh_offset;
    if (psh->gh_sht_group_array ||
        psh->gh_sht_group_array_count ||
        psh->gh_content) {
        *errcode = DW_DLE_ELF_GRPSTRING_SECTION_ERROR;
        return DW_DLV_ERROR;
    }
    content = calloc(1,size);
    if (!content) {
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    res = RRMOA(ep->f_fd,content,offset,size,
        ep->f_filesize,errcode);
    if (res != DW_DLV_OK) {
        free(content);
        return res;
    }
    psh->gh_content = content;
    psh->gh_load_type = Dwarf_Alloc_Malloc;
    if (flags& SHF_COMPRESSED) {
#if defined(HAVE_ZLIB) && defined(HAVE_ZSTD)
        *errcode = 0;
        _dwarf_do_decompress_elf(ep,psh,errcode);
        /* decompress and set new section size */

        if (*errcode) {
            free(psh->gh_content);
            psh->gh_content = 0;
            return DW_DLV_ERROR;
        }
#else /* COMPRESSED TEST */
        free(psh->gh_content);
        psh->gh_content = 0;
        *errcode = DW_DLE_ZLIB_ZSTD_MISSING;
        return DW_DLV_ERROR;
#endif /* COMPRESSED TEST */
    }
    size = psh->gh_size;
    ecount = (long)(size/sizeof(dw_elf64_sym));
    size2 = ecount * sizeof(dw_elf64_sym);
    if (size != size2) {
        free(psh->gh_content);
        psh->gh_content = 0;
        *errcode = DW_DLE_SYMBOL_SECTION_SIZE_ERROR;
        return DW_DLV_ERROR;
    }
    if (size >= ep->f_filesize ) {
        free(psh->gh_content);
        psh->gh_content = 0;
        *errcode = DW_DLE_SYMBOL_SECTION_SIZE_ERROR;
        return DW_DLV_ERROR;
    }
    psym = (dw_elf64_sym *)psh->gh_content;
    gsym = calloc(ecount,sizeof(struct generic_symentry));
    if (!gsym) {
        free(psh->gh_content);
        psh->gh_content = 0;
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    res = RRMOA(ep->f_fd,psym,offset,size,
        ep->f_filesize,errcode);
    if (res!= DW_DLV_OK) {
        free(gsym);
        free(psh->gh_content);
        psh->gh_content = 0;
        *errcode = DW_DLE_ALLOC_FAIL;
        return res;
    }
    orig_gsym = gsym;
    for ( i = 0; i < ecount; ++i,++psym,++gsym) {
        Dwarf_Unsigned bind = 0;
        Dwarf_Unsigned type = 0;

        ASNAR(ep->f_copy_word,gsym->gs_name,psym->st_name);
        ASNAR(ep->f_copy_word,gsym->gs_value,psym->st_value);
        ASNAR(ep->f_copy_word,gsym->gs_size,psym->st_size);
        ASNAR(ep->f_copy_word,gsym->gs_info,psym->st_info);
        ASNAR(ep->f_copy_word,gsym->gs_other,psym->st_other);
        ASNAR(ep->f_copy_word,gsym->gs_shndx,psym->st_shndx);
        bind = gsym->gs_info >> 4;
        type = gsym->gs_info & 0xf;
        gsym->gs_bind = bind;
        gsym->gs_type = type;
    }
    psh->gh_was_alloc = TRUE;
    *count_out = ecount;
    *gsym_out = orig_gsym;
    return DW_DLV_OK;
}

static int
_dwarf_generic_elf_load_symbols(
    dwarf_elf_object_access_internals_t *ep,
    Dwarf_Unsigned secnum,
    struct generic_shdr *psh,
    struct generic_symentry **gsym_out,
    Dwarf_Unsigned *count_out,int *errcode)
{
    int res = 0;
    struct generic_symentry *gsym = 0;
    Dwarf_Unsigned count = 0;

    if (!secnum) {
        return DW_DLV_NO_ENTRY;
    }
    if (psh->gh_size > ep->f_filesize) {
        *errcode = DW_DLE_SECTION_SIZE_ERROR;
        return DW_DLV_ERROR;
    }
    if (ep->f_offsetsize == 32) {
        res = _dwarf_generic_elf_load_symbols32(ep,
            &gsym,psh,
            &count,errcode);
    } else if (ep->f_offsetsize == 64) {
        res = _dwarf_generic_elf_load_symbols64(ep,
            &gsym,psh,
            &count,errcode);
    } else {
        *errcode = DW_DLE_OFFSET_SIZE;
        return DW_DLV_ERROR;
    }
    if (res == DW_DLV_OK) {
        *gsym_out = gsym;
        *count_out = count;
    } else {
        free(psh->gh_content);
        psh->gh_content = 0;
        psh->gh_was_alloc = FALSE;
    }
    return res;
}

int
_dwarf_load_elf_symtab_symbols(
    dwarf_elf_object_access_internals_t *ep, int*errcode)
{
    int res = 0;
    struct generic_symentry *gsym = 0;
    Dwarf_Unsigned count = 0;
    Dwarf_Unsigned secnum = ep->f_symtab_sect_index;
    struct generic_shdr * psh = 0;

    if (!secnum) {
        return DW_DLV_NO_ENTRY;
    }
    psh = ep->f_shdr + secnum;
    if (psh->gh_size > ep->f_filesize) {
        *errcode = DW_DLE_SECTION_SIZE_ERROR;
        return DW_DLV_ERROR;
    }
    res = _dwarf_generic_elf_load_symbols(ep,
        secnum,
        psh,
        &gsym,
        &count,errcode);
    if (res == DW_DLV_OK) {
        ep->f_symtab = gsym;
        ep->f_loc_symtab.g_count = count;
    }
    return res;
}

static int
generic_rel_from_rela32(
    dwarf_elf_object_access_internals_t *ep,
    struct generic_shdr * gsh,
    dw_elf32_rela *relp,
    struct generic_rela *grel,
    int *errcode)
{
    Dwarf_Unsigned ecount = 0;
    Dwarf_Unsigned size = gsh->gh_size;
    Dwarf_Unsigned size2 = 0;
    Dwarf_Unsigned i = 0;

    ecount = size/sizeof(dw_elf32_rela);
    size2 = ecount * sizeof(dw_elf32_rela);
    if (size >= ep->f_filesize) {
        *errcode = DW_DLE_RELOCATION_SECTION_SIZE_ERROR;
        return  DW_DLV_ERROR;
    }
    if (size != size2) {
        *errcode = DW_DLE_RELOCATION_SECTION_SIZE_ERROR;
        return  DW_DLV_ERROR;
    }
    for ( i = 0; i < ecount; ++i,++relp,++grel) {
        ASNAR(ep->f_copy_word,grel->gr_offset,relp->r_offset);
        ASNAR(ep->f_copy_word,grel->gr_info,relp->r_info);
        /* addend signed */
        ASNAR(ep->f_copy_word,grel->gr_addend,relp->r_addend);
        SIGN_EXTEND(grel->gr_addend,sizeof(relp->r_addend));
        grel->gr_sym  = grel->gr_info>>8; /* ELF32_R_SYM */
        grel->gr_type = grel->gr_info & 0xff;
        grel->gr_is_rela = TRUE;
    }
    return DW_DLV_OK;
}

static int
generic_rel_from_rela64(
    dwarf_elf_object_access_internals_t *ep,
    struct generic_shdr * gsh,
    dw_elf64_rela *relp,
    struct generic_rela *grel, int *errcode)
{
    Dwarf_Unsigned ecount = 0;
    Dwarf_Unsigned size = gsh->gh_size;
    Dwarf_Unsigned size2 = 0;
    Dwarf_Unsigned i = 0;
    int objlittleendian = (ep->f_endian == DW_END_little);
    int ismips64 = (ep->f_machine == EM_MIPS);
    int issparcv9 = (ep->f_machine == EM_SPARCV9);

    ecount = size/sizeof(dw_elf64_rela);
    size2 = ecount * sizeof(dw_elf64_rela);
    if (size >= ep->f_filesize) {
        *errcode = DW_DLE_RELOCATION_SECTION_SIZE_ERROR;
        return  DW_DLV_ERROR;
    }
    if (size != size2) {
        *errcode = DW_DLE_RELOCATION_SECTION_SIZE_ERROR;
        return  DW_DLV_ERROR;
    }
    for ( i = 0; i < ecount; ++i,++relp,++grel) {
        ASNAR(ep->f_copy_word,grel->gr_offset,relp->r_offset);
        ASNAR(ep->f_copy_word,grel->gr_info,relp->r_info);
        ASNAR(ep->f_copy_word,grel->gr_addend,relp->r_addend);
        SIGN_EXTEND(grel->gr_addend,sizeof(relp->r_addend));
        if (ismips64 && objlittleendian ) {
            char realsym[4];

            memcpy(realsym,&relp->r_info,sizeof(realsym));
            ASNAR(ep->f_copy_word,grel->gr_sym,realsym);
            grel->gr_type  = relp->r_info[7];
            grel->gr_type2 = relp->r_info[6];
            grel->gr_type3 = relp->r_info[5];
        } else if (issparcv9) {
            /*  Always Big Endian?  */
            char realsym[4];

            memcpy(realsym,&relp->r_info,sizeof(realsym));
            ASNAR(ep->f_copy_word,grel->gr_sym,realsym);
            grel->gr_type  = relp->r_info[7];
        } else {
            grel->gr_sym  = grel->gr_info >> 32;
            grel->gr_type = grel->gr_info & 0xffffffff;
        }
        grel->gr_is_rela = TRUE;
    }
    return DW_DLV_OK;
}

static int
generic_rel_from_rel32(
    dwarf_elf_object_access_internals_t *ep,
    struct generic_shdr * gsh,
    dw_elf32_rel *relp,
    struct generic_rela *grel,int *errcode)
{
    Dwarf_Unsigned ecount = 0;
    Dwarf_Unsigned size = gsh->gh_size;
    Dwarf_Unsigned size2 = 0;
    Dwarf_Unsigned i = 0;

    ecount = size/sizeof(dw_elf32_rel);
    size2 = ecount * sizeof(dw_elf32_rel);
    if (size >= ep->f_filesize) {
        *errcode = DW_DLE_RELOCATION_SECTION_SIZE_ERROR;
        return  DW_DLV_ERROR;
    }
    if (size != size2) {
        *errcode = DW_DLE_RELOCATION_SECTION_SIZE_ERROR;
        return  DW_DLV_ERROR;
    }
    for ( i = 0; i < ecount; ++i,++relp,++grel) {
        ASNAR(ep->f_copy_word,grel->gr_offset,relp->r_offset);
        ASNAR(ep->f_copy_word,grel->gr_info,relp->r_info);
        grel->gr_addend  = 0; /* Unused for plain .rel */
        grel->gr_sym  = grel->gr_info >>8; /* ELF32_R_SYM */
        grel->gr_is_rela = FALSE;
        grel->gr_type = grel->gr_info & 0xff;
    }
    return DW_DLV_OK;
}

static int
generic_rel_from_rel64(
    dwarf_elf_object_access_internals_t *ep,
    struct generic_shdr * gsh,
    dw_elf64_rel *relp,
    struct generic_rela *grel,int *errcode)
{
    Dwarf_Unsigned ecount = 0;
    Dwarf_Unsigned size = gsh->gh_size;
    Dwarf_Unsigned size2 = 0;
    Dwarf_Unsigned i = 0;
    int objlittleendian = (ep->f_endian == DW_END_little);
    int ismips64 = (ep->f_machine == EM_MIPS);
    int issparcv9 = (ep->f_machine == EM_SPARCV9);

    ecount = size/sizeof(dw_elf64_rel);
    size2 = ecount * sizeof(dw_elf64_rel);
    if (size >= ep->f_filesize) {
        *errcode = DW_DLE_RELOCATION_SECTION_SIZE_ERROR;
        return DW_DLV_ERROR;
    }
    if (size != size2) {
        *errcode = DW_DLE_RELOCATION_SECTION_SIZE_ERROR;
        return DW_DLV_ERROR;
    }
    for ( i = 0; i < ecount; ++i,++relp,++grel) {
        ASNAR(ep->f_copy_word,grel->gr_offset,relp->r_offset);
        ASNAR(ep->f_copy_word,grel->gr_info,relp->r_info);
        grel->gr_addend  = 0; /* Unused for plain .rel */
        if (ismips64 && objlittleendian ) {
            char realsym[4];

            memcpy(realsym,&relp->r_info,sizeof(realsym));
            ASNAR(ep->f_copy_word,grel->gr_sym,realsym);
            grel->gr_type  = relp->r_info[7];
            grel->gr_type2 = relp->r_info[6];
            grel->gr_type3 = relp->r_info[5];
        } else if (issparcv9) {
            /*  Always Big Endian?  */
            char realsym[4];

            memcpy(realsym,&relp->r_info,sizeof(realsym));
            ASNAR(ep->f_copy_word,grel->gr_sym,realsym);
            grel->gr_type  = relp->r_info[7];
        } else {
            grel->gr_sym  = grel->gr_info >>32;
            grel->gr_type = grel->gr_info & 0xffffffff;
        }
        grel->gr_is_rela = FALSE;
    }
    return DW_DLV_OK;
}

int
_dwarf_load_elf_symstr(
    dwarf_elf_object_access_internals_t *ep, int *errcode)
{
    struct generic_shdr *strpsh = 0;
    int res = 0;
    Dwarf_Unsigned strsectindex  =0;
    Dwarf_Unsigned strsectlength = 0;
    Dwarf_Unsigned flags = 0;

    if (!ep->f_symtab_sect_strings_sect_index) {
        return DW_DLV_NO_ENTRY;
    }
    strsectindex = ep->f_symtab_sect_strings_sect_index;
    strsectlength = ep->f_symtab_sect_strings_max;
    strpsh = ep->f_shdr + strsectindex;
    if (strsectindex == ep->f_elf_shstrings_index) {
        /*  content loaded already by
            _dwarf_elf_load_sectstrings() as
            they are the same section number. */
        ep->f_symtab_sect_strings_sect_index = strpsh->gh_secnum;
        ep->f_symtab_sect_strings_max = ep->f_elf_shstrings_max;
        ep->f_symtab_sect_strings = strpsh->gh_content;
        return DW_DLV_OK;
    }
    /*  if strpsh->gh_sht_group_array is non-zero
        then the section is set up as a GROUP section
        and reading as a string section is absurd.
        And gh_content set in a way we do not want.*/
    if (strpsh->gh_sht_group_array ||
        strpsh->gh_sht_group_array_count ||
        strpsh->gh_content) {
        *errcode = DW_DLE_ELF_GRPSTRING_SECTION_ERROR;
        return DW_DLV_ERROR;
    }
    /*  Alloc an extra byte as a guaranteed NUL byte
        at the end of the strings in case the section
        is corrupted and lacks a NUL at end. */
    if (strsectlength > ep->f_filesize ||
        strpsh->gh_offset >ep->f_filesize ||
        (strsectlength + strpsh->gh_offset) >
            ep->f_filesize) {
        *errcode = DW_DLE_SECTION_SIZE_OR_OFFSET_LARGE;
        return DW_DLV_ERROR;
    }
    if (ep->f_symtab_sect_strings_sect_index ==
        ep->f_elf_shstrings_index ) {
        ep->f_symtab_sect_strings = ep->f_elf_shstrings_data;
        ep->f_symtab_sect_strings_max = ep->f_elf_shstrings_max;
        return DW_DLV_OK;
    }
    ep->f_symtab_sect_strings = calloc(1,strsectlength+1);
    flags = strpsh->gh_flags;
    if (!ep->f_symtab_sect_strings) {
        ep->f_symtab_sect_strings = 0;
        ep->f_symtab_sect_strings_max = 0;
        ep->f_symtab_sect_strings_sect_index = 0;
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    strpsh->gh_load_type = Dwarf_Alloc_Malloc;
    res = RRMOA(ep->f_fd,ep->f_symtab_sect_strings,
        strpsh->gh_offset,
        strsectlength,
        ep->f_filesize,errcode);
    if (res != DW_DLV_OK) {
        free(ep->f_symtab_sect_strings);
        ep->f_symtab_sect_strings = 0;
        return res;
    }
    strpsh->gh_content = ep->f_symtab_sect_strings;
    strpsh->gh_was_alloc = TRUE;
    if (flags& SHF_COMPRESSED) {
#if defined(HAVE_ZLIB) && defined(HAVE_ZSTD)
        /* decompress and set new section size */
        *errcode = 0;
        _dwarf_do_decompress_elf(ep,strpsh,errcode);
        if (*errcode) {
            /*  gh_content will cause the free */
            ep->f_symtab_sect_strings = 0;
            return DW_DLV_ERROR;
        }
        ep->f_symtab_sect_strings = strpsh->gh_content;
#else /* COMPRESSED TEST */
        /*  gh_content will cause the free */
        ep->f_symtab_sect_strings = 0;
        *errcode = DW_DLE_ZLIB_ZSTD_MISSING;
        return DW_DLV_ERROR;
#endif /* COMPRESSED TEST */
    }
    return DW_DLV_OK;
}

/*  The string section should not be filled in
    or its content read yet. This is called
    right after initial headers array created.
    No gh_content is set yet.  */
static int
_dwarf_elf_load_sectstrings(
    dwarf_elf_object_access_internals_t *ep,
    Dwarf_Unsigned stringsection,
    int *errcode)
{
    int res = 0;
    struct generic_shdr *psh = 0;
    Dwarf_Unsigned secoffset = 0;
    Dwarf_Unsigned flags =  0;

    if (stringsection >= ep->f_loc_shdr.g_count) {
        *errcode = DW_DLE_SECTION_INDEX_BAD;
        return DW_DLV_ERROR;
    }
    psh = ep->f_shdr+stringsection;
    flags = psh->gh_flags;

    secoffset = psh->gh_offset;
    if (is_empty_section(psh->gh_type)) {
        *errcode = DW_DLE_ELF_STRING_SECTION_MISSING;
        return DW_DLV_ERROR;
    }
    if (secoffset >= ep->f_filesize ||
        psh->gh_size > ep->f_filesize ||
        (secoffset + psh->gh_size) > ep->f_filesize) {
        *errcode = DW_DLE_SECTION_SIZE_OR_OFFSET_LARGE;
        return DW_DLV_ERROR;
    }
    if (psh->gh_content) {
        *errcode = DW_DLE_ELF_STRING_SECTION_ERROR;
        return DW_DLV_ERROR;
    }
    /* An extra zero byte so always null-terminated */
    psh->gh_content = (char *)calloc(1,psh->gh_size+1);
    if (!psh->gh_content) {
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    res = RRMOA(ep->f_fd,psh->gh_content,secoffset,
        psh->gh_size,
        ep->f_filesize,errcode);
    if (res != DW_DLV_OK) {
        free(ep->f_elf_shstrings_data);
        ep->f_elf_shstrings_data = 0;
        free(psh->gh_content);
        psh->gh_content = 0;
        return res;
    }
    psh->gh_load_type = Dwarf_Alloc_Malloc;
    psh->gh_was_alloc = TRUE;
    ep->f_elf_shstrings_index = stringsection;
    ep->f_elf_shstrings_max = psh->gh_size;
    ep->f_elf_shstrings_length = psh->gh_size;
    ep->f_elf_shstrings_data = psh->gh_content;
    if (flags& SHF_COMPRESSED) {
#if defined(HAVE_ZLIB) && defined(HAVE_ZSTD)
        *errcode = 0;
        _dwarf_do_decompress_elf(ep,psh,errcode);
        /* decompress and set new section size */
        if (*errcode) {
            return DW_DLV_ERROR;
        }
        ep->f_elf_shstrings_max = psh->gh_size;
        ep->f_elf_shstrings_length = psh->gh_size;
        ep->f_elf_shstrings_data = psh->gh_content;
#else /* COMPRESSED TEST */
        /* We cannot decompress, so we really have nothing. */
        free(psh->gh_content);
        psh->gh_content = 0;
        ep->f_elf_shstrings_max = 0;
        ep->f_elf_shstrings_length = 0;
        ep->f_elf_shstrings_data = 0;
        psh->gh_was_alloc = FALSE;
        *errcode = DW_DLE_ZLIB_ZSTD_MISSING;
        return DW_DLV_ERROR;
#endif /* COMPRESSED TEST */
    }
    return DW_DLV_OK;
}

static const dw_elf32_shdr shd32zero;
static const struct generic_shdr  shdgzero;

/*  Has a side effect of setting count, number
    in the ehdr  ep points to. */
static int
get_counts_from_sec32_zero(
    dwarf_elf_object_access_internals_t * ep,
    Dwarf_Unsigned offset,
    Dwarf_Bool     *have_shdr_count,
    Dwarf_Unsigned *shdr_count,
    Dwarf_Bool     *have_shstrndx_number,
    Dwarf_Unsigned *shstrndx_number,
    int            *errcode)
{
    dw_elf32_shdr       shd32;
    struct generic_shdr shdg;
    int res = 0;
    Dwarf_Unsigned size = sizeof(shd32);
    struct generic_ehdr * geh  = ep->f_ehdr;

    shd32 =  shd32zero;
    shdg  = shdgzero;
    res = RRMOA(ep->f_fd,&shd32,offset,size,
        ep->f_filesize,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    copysection32(ep,&shdg,&shd32);
    if (geh->ge_shnum_extended) {
        geh->ge_shnum = shdg.gh_size;
        geh->ge_shnum_in_shnum = TRUE;
        if (geh->ge_shnum  < 3) {
            *errcode = DW_DLE_TOO_FEW_SECTIONS;
            return DW_DLV_ERROR;
        }
    }
    *have_shdr_count = TRUE;
    *shdr_count = geh->ge_shnum;
    if (geh->ge_strndx_extended) {
        geh->ge_shstrndx = shdg.gh_link;
        geh->ge_strndx_in_strndx = TRUE;
    }
    if (geh->ge_shnum_in_shnum &&
        geh->ge_strndx_in_strndx&&
        (geh->ge_shstrndx >= geh->ge_shnum)) {
            *errcode = DW_DLE_NO_SECT_STRINGS;
            return DW_DLV_ERROR;
    }
    *have_shstrndx_number = TRUE;
    *shstrndx_number = geh->ge_shstrndx;
    return DW_DLV_OK;
}

static int
elf_load_sectheaders32(
    dwarf_elf_object_access_internals_t *ep,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned entsize,
    Dwarf_Unsigned count,
    int *errcode)
{
    Dwarf_Unsigned generic_count = 0;
    Dwarf_Unsigned shdr_count = 0;
    Dwarf_Bool have_shdr_count = FALSE;
    Dwarf_Unsigned shstrndx_number = 0;
    Dwarf_Bool have_shstrndx_number = FALSE;
    struct generic_ehdr *ehp = 0;
    int res = 0;

    if (entsize < sizeof(dw_elf32_shdr)) {
        *errcode = DW_DLE_SECTION_SIZE_ERROR;
        return DW_DLV_ERROR;
    }
    ehp = ep->f_ehdr;
    if (!ehp->ge_shnum_in_shnum || !ehp->ge_strndx_in_strndx) {
        res = get_counts_from_sec32_zero(ep,offset,
            &have_shdr_count,&shdr_count,
            &have_shstrndx_number,&shstrndx_number,
            errcode);
        if (res != DW_DLV_OK) {
            return res;
        }
        if (have_shdr_count) {
            count = shdr_count;
        }
    }
    if (count == 0) {
        return DW_DLV_NO_ENTRY;
    }
    if ((offset > ep->f_filesize)||
        (entsize > 200)||
        (count > ep->f_filesize) ||
        ((count *entsize +offset) > ep->f_filesize)) {
        *errcode = DW_DLE_SECTION_SIZE_OR_OFFSET_LARGE;
        return DW_DLV_ERROR;
    }
    res = generic_shdr_from_shdr32(ep,&generic_count,
        offset,entsize,count,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    if (generic_count != count) {
        *errcode = DW_DLE_ELF_SECTION_COUNT_MISMATCH;
        return DW_DLV_ERROR;
    }
    return DW_DLV_OK;
}
static const dw_elf64_shdr shd64zero;
/*  Has a side effect of setting count, number
    in the ehdr  ep points to. */
static int
get_counts_from_sec64_zero(
    dwarf_elf_object_access_internals_t * ep,
    Dwarf_Unsigned offset,
    Dwarf_Bool     *have_shdr_count,
    Dwarf_Unsigned *shdr_count,
    Dwarf_Bool     *have_shstrndx_number,
    Dwarf_Unsigned *shstrndx_number,
    int            *errcode)
{
    dw_elf64_shdr       shd64;
    struct generic_shdr shdg;
    int res = 0;
    Dwarf_Unsigned size = sizeof(shd64);
    struct generic_ehdr * geh  = ep->f_ehdr;

    shd64 =  shd64zero;
    shdg  = shdgzero;
    res = RRMOA(ep->f_fd,&shd64,offset,size,
        ep->f_filesize,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    copysection64(ep,&shdg,&shd64);
    if (geh->ge_shnum_extended) {
        geh->ge_shnum = shdg.gh_size;
        geh->ge_shnum_in_shnum = TRUE;
        if (geh->ge_shnum  < 3) {
            *errcode = DW_DLE_TOO_FEW_SECTIONS;
            return DW_DLV_ERROR;
        }
    }
    *have_shdr_count = TRUE;
    *shdr_count = geh->ge_shnum;
    if (geh->ge_strndx_extended) {
        geh->ge_shstrndx = shdg.gh_link;
        geh->ge_strndx_in_strndx = TRUE;
    }
    if (geh->ge_shnum_in_shnum    &&
        geh->ge_strndx_in_strndx &&
        (geh->ge_shstrndx >= geh->ge_shnum)) {
            *errcode = DW_DLE_NO_SECT_STRINGS;
            return DW_DLV_ERROR;
    }

    *have_shstrndx_number = TRUE;
    *shstrndx_number = geh->ge_shstrndx;
    return DW_DLV_OK;
}

static int
elf_load_sectheaders64(
    dwarf_elf_object_access_internals_t *ep,
    Dwarf_Unsigned offset,Dwarf_Unsigned entsize,
    Dwarf_Unsigned count,int*errcode)
{
    Dwarf_Unsigned generic_count = 0;
    Dwarf_Unsigned shdr_count = 0;
    Dwarf_Bool have_shdr_count = FALSE;
    Dwarf_Unsigned shstrndx_number = 0;
    Dwarf_Bool have_shstrndx_number = FALSE;
    struct generic_ehdr *ehp = 0;
    int res = 0;

    ehp = ep->f_ehdr;
    if (!ehp->ge_shnum_in_shnum || !ehp->ge_strndx_in_strndx ) {
        res = get_counts_from_sec64_zero(ep,offset,
            &have_shdr_count,&shdr_count,
            &have_shstrndx_number,&shstrndx_number,
            errcode);
        if (res != DW_DLV_OK) {
            return res;
        }
        if (have_shdr_count) {
            count = shdr_count;
        }
    }
    if (count == 0) {
        return DW_DLV_NO_ENTRY;
    }
    if (entsize < sizeof(dw_elf64_shdr)) {
        *errcode = DW_DLE_SECTION_SIZE_ERROR;
        return DW_DLV_ERROR;
    }
    if ((offset > ep->f_filesize)||
        (entsize > 200)||
        (count > ep->f_filesize) ||
        ((count *entsize +offset) > ep->f_filesize)) {
        *errcode = DW_DLE_SECTION_SIZE_OR_OFFSET_LARGE;
        return DW_DLV_ERROR;
    }

    res = generic_shdr_from_shdr64(ep,&generic_count,
        offset,entsize,count,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    if (generic_count != count) {
        *errcode = DW_DLE_ELF_SECTION_COUNT_MISMATCH;
        return DW_DLV_ERROR;
    }
    return DW_DLV_OK;
}

static int
_dwarf_elf_load_a_relx_batch(
    dwarf_elf_object_access_internals_t *ep,
    struct generic_shdr * gsh,
    struct generic_rela ** grel_out,
    Dwarf_Unsigned *count_out,
    enum RelocRela localrela,
    enum RelocOffsetSize localoffsize,
    int *errcode)
{
    Dwarf_Unsigned count = 0;
    Dwarf_Unsigned size = 0;
    Dwarf_Unsigned flags = 0;
    Dwarf_Unsigned size2 = 0;
    Dwarf_Unsigned sizeg = 0;
    Dwarf_Unsigned offset = 0;
    int            res = 0;
    Dwarf_Unsigned object_reclen = 0;
    struct generic_rela *grel = 0;
    char *         relp = 0;
    int            local_alloc = FALSE;

    /*  ASSERT: Caller guarantees localoffsetsize
        is a valid 4 or 8. */
    /*  ASSERT: Caller guarantees localrela is one
        of the 2 valid values 1 or 2 */

    flags = gsh->gh_flags;
    offset = gsh->gh_offset;
    size = gsh->gh_size;
    if (size == 0) {
        return DW_DLV_NO_ENTRY;
    }
    relp = (char *)gsh->gh_content;
    if (!relp) {
        relp = (char *)malloc(size);
        local_alloc = TRUE;
    }
    if (!relp) {
        *errcode = DW_DLE_REL_ALLOC;
        return DW_DLV_NO_ENTRY;
    }
    if ((offset > ep->f_filesize)||
        (size > ep->f_filesize) ||
        ((size +offset) > ep->f_filesize)) {
        *errcode = DW_DLE_SECTION_SIZE_OR_OFFSET_LARGE;
        if (local_alloc) {
            free(relp);
        }
        return DW_DLV_ERROR;
    }
    res = RRMOA(ep->f_fd,relp,offset,size,
        ep->f_filesize,errcode);
    if (res != DW_DLV_OK) {
        free(relp);
        free(grel);
        return res;
    }
    if (local_alloc) {
        gsh->gh_content = relp;
        gsh->gh_was_alloc = TRUE;
        gsh->gh_load_type = Dwarf_Alloc_Malloc;
    }
    if (flags& SHF_COMPRESSED) {
#if defined(HAVE_ZLIB) && defined(HAVE_ZSTD)
        *errcode = 0;
        _dwarf_do_decompress_elf(ep,gsh,errcode);
        /* decompress and set new section size */
        if (*errcode) {
            return DW_DLV_ERROR;
        }
#else /* COMPRESSED TEST */
        *errcode = DW_DLE_ZLIB_ZSTD_MISSING;
        return DW_DLV_ERROR;
#endif /* COMPRESSED TEST */
    }
    size = gsh->gh_size;
    if (localoffsize == RelocOffset32) {
        if (localrela ==  RelocIsRela) {
            object_reclen = sizeof(dw_elf32_rela);
            count = (long)(size/object_reclen);
            size2 = count * object_reclen;
            if (size != size2) {
                *errcode = DW_DLE_SECTION_SIZE_ERROR;
                return DW_DLV_ERROR;
            }
        } else {
            object_reclen = sizeof(dw_elf32_rel);
            count = (long)(size/object_reclen);
            size2 = count * object_reclen;
            if (size != size2) {
                *errcode = DW_DLE_SECTION_SIZE_ERROR;
                return DW_DLV_ERROR;
            }
        }
    } else {
        if (localrela ==  RelocIsRela) {
            object_reclen = sizeof(dw_elf64_rela);
            count = (long)(size/object_reclen);
            size2 = count * object_reclen;
            if (size != size2) {
                *errcode = DW_DLE_SECTION_SIZE_ERROR;
                return DW_DLV_ERROR;
            }
        } else {
            object_reclen = sizeof(dw_elf64_rel);
            count = (long)(size/object_reclen);
            size2 = count * object_reclen;
            if (size != size2) {
                *errcode = DW_DLE_SECTION_SIZE_ERROR;
                return DW_DLV_ERROR;
            }
        }
    }

    sizeg = count*sizeof(struct generic_rela);
    grel = (struct generic_rela *)malloc(sizeg);
    if (!grel) {
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    if (localoffsize == RelocOffset32) {
        if (localrela ==  RelocIsRela) {
            res = generic_rel_from_rela32(ep,gsh,
                (dw_elf32_rela *)gsh->gh_content,grel,errcode);
        } else {
            res = generic_rel_from_rel32(ep,gsh,
                (dw_elf32_rel *)gsh->gh_content,grel,errcode);
        }
    } else {
        if (localrela ==  RelocIsRela) {
            res = generic_rel_from_rela64(ep,gsh,
                (dw_elf64_rela *)gsh->gh_content,grel,errcode);
        } else {
            res = generic_rel_from_rel64(ep,gsh,
                (dw_elf64_rel *)gsh->gh_content,grel,errcode);
        }
    }
    if (res == DW_DLV_OK) {
        gsh->gh_relcount = count;
        gsh->gh_rels = grel;
        *count_out = count;
        *grel_out = grel;
        return res;
    }
    /* Some sort of issue */
    count_out = 0;
    free(grel);
    return res;
}

/*  Is this rel/rela section related to dwarf at all?
    set oksecnum zero if not. Else set targ secnum.
    Never returns DW_DLV_NO_ENTRY. */
static int
this_rel_is_a_section_dwarf_related(
    dwarf_elf_object_access_internals_t *ep,
    struct generic_shdr *gshdr,
    unsigned *oksecnum_out,
    int *errcode)
{
    Dwarf_Unsigned oksecnum = 0;
    struct generic_shdr *gstarg = 0;

    if (gshdr->gh_type != SHT_RELA &&
        gshdr->gh_type != SHT_REL)  {
        *oksecnum_out = 0;
        return DW_DLV_OK;
    }
    oksecnum = gshdr->gh_reloc_target_secnum;
    if (oksecnum >= ep->f_loc_shdr.g_count) {
        *oksecnum_out = 0;
        *errcode = DW_DLE_ELF_SECTION_ERROR;
        return DW_DLV_ERROR;
    }
    gstarg = ep->f_shdr+oksecnum;
    if (!gstarg->gh_is_dwarf) {
        *oksecnum_out = 0; /* no reloc needed. */
        return DW_DLV_OK;
    }
    *oksecnum_out = (unsigned)oksecnum;
    return DW_DLV_OK;
}
/*  Secnum here is the secnum of rela. Not
    the target of the relocations.
    This also loads .rel. */
int
_dwarf_load_elf_relx(
    dwarf_elf_object_access_internals_t *ep,
    Dwarf_Unsigned secnum,
    enum RelocRela localr,
    int *errcode)
{
    struct generic_shdr *gshdr = 0;
    Dwarf_Unsigned seccount = 0;
    unsigned offsetsize = 0;
    struct generic_rela *grp = 0;
    Dwarf_Unsigned count_read = 0;
    int res = 0;
    unsigned oksec = 0;
    enum RelocOffsetSize localoffsize = RelocOffset32;

    /*  ASSERT: Caller guarantees localr is
        a valid RelocRela  */
    if (!ep) {
        *errcode = DW_DLE_INTERNAL_NULL_POINTER;
        return DW_DLV_ERROR;
    }
    offsetsize = ep->f_offsetsize;
    seccount = ep->f_loc_shdr.g_count;
    if (secnum >= seccount) {
        *errcode = DW_DLE_ELF_SECTION_ERROR;
        return DW_DLV_ERROR;
    }
    gshdr = ep->f_shdr +secnum;
    if (is_empty_section(gshdr->gh_type)) {

        return DW_DLV_NO_ENTRY;
    }

    res = this_rel_is_a_section_dwarf_related(ep,gshdr,
        &oksec,errcode);
    if (res == DW_DLV_ERROR) {
        return res;
    }
    if (!oksec) {
        return DW_DLV_OK;
    }
    /*  We will actually read these relocations. */
    if (offsetsize == 64) {
        localoffsize = RelocOffset64;
    } else if (offsetsize == 32) {
        localoffsize = RelocOffset32;
    } else {
        *errcode = DW_DLE_OFFSET_SIZE;
        return DW_DLV_ERROR;
    }
    /*  ASSERT: localoffsize is now a valid enum value,
        one of the two defined. */
    res = _dwarf_elf_load_a_relx_batch(ep,
        gshdr,&grp,&count_read,localr,localoffsize,errcode);
    if (res == DW_DLV_ERROR) {
        return res;
    }
    if (res == DW_DLV_NO_ENTRY) {
        return res;
    }
    gshdr->gh_rels = grp;
    gshdr->gh_relcount = count_read;
    return DW_DLV_OK;
}

static int
validate_section_name_string(Dwarf_Unsigned section_length,
    Dwarf_Unsigned string_loc_index,
    const char * strings_start,
    int  * errcode)
{
    const char *endpoint = strings_start + section_length;
    const char *cur = 0;

    if (section_length <= string_loc_index) {
        *errcode = DW_DLE_SECTION_STRING_OFFSET_BAD;
        return DW_DLV_ERROR;
    }
    cur = string_loc_index+strings_start;
    for ( ; cur < endpoint;++cur) {
        if (!*cur) {
            return DW_DLV_OK;
        }
    }
    *errcode = DW_DLE_SECTION_STRING_OFFSET_BAD;
    return DW_DLV_ERROR;
}

/*  Without proper section names in place nothing
    is going to work in reading DWARF sections.
    This assumes we are set up with the section
    strings read in and pointed to by ep->f_elf_shstrings_data. */
static int
_dwarf_elf_load_sect_namestrings(
    dwarf_elf_object_access_internals_t *ep,
    int *errcode)
{
    struct generic_shdr *gshdr = 0;
    Dwarf_Unsigned generic_count = 0;
    Dwarf_Unsigned i = 1;
    const char *stringsecbase = 0;

    stringsecbase = ep->f_elf_shstrings_data;
    gshdr = ep->f_shdr;
    generic_count = ep->f_loc_shdr.g_count;
    /*  Here we ensure gh_namestring set to something with
        null termination */
    for (i = 0; i < generic_count; i++, ++gshdr) {
        const char *namestr =
            "<No valid Elf section strings exist>";
        int res = 0;

        if (!ep->f_ehdr->ge_shstrndx || !stringsecbase) {
            gshdr->gh_namestring = namestr;
            continue;
        }
        namestr = "<Invalid sh_name value. Corrupt Elf.>";
        res = validate_section_name_string(ep->f_elf_shstrings_length,
            gshdr->gh_name, stringsecbase,
            errcode);
        if (res != DW_DLV_OK) {
            gshdr->gh_namestring = namestr;
            if (res == DW_DLV_ERROR) {
                return res;
            }
            /* no entry, missing strings. */
            *errcode = DW_DLE_NO_SECT_STRINGS;
            return DW_DLV_ERROR;
        } else {
            gshdr->gh_namestring = stringsecbase + gshdr->gh_name;
        }
    }
    return DW_DLV_OK;
}

/*  The C standard ensures these are all appropriate
    zero bits. */
static const dw_elf32_ehdr eh32_zero;
static const dw_elf64_ehdr eh64_zero;

static int
elf_load_elf_header32(
    dwarf_elf_object_access_internals_t *ep,int *errcode)
{
    int res = 0;
    dw_elf32_ehdr ehdr32;
    struct generic_ehdr *ehdr = 0;

    ehdr32 = eh32_zero;
    res = RRMOA(ep->f_fd,&ehdr32,0,sizeof(ehdr32),
        ep->f_filesize,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    ehdr = (struct generic_ehdr *)calloc(1,
        sizeof(struct generic_ehdr));
    if (!ehdr) {
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    res  = generic_ehdr_from_32(ep,ehdr,&ehdr32,errcode);
    if (res != DW_DLV_OK) {
        free(ehdr);
        return res;
    }
    ep->f_machine = (unsigned)ehdr->ge_machine;
    ep->f_flags = ehdr->ge_flags;
    return res;
}
static int
elf_load_elf_header64(
    dwarf_elf_object_access_internals_t *ep,int *errcode)
{
    int res = 0;
    dw_elf64_ehdr ehdr64;
    struct generic_ehdr *ehdr = 0;

    ehdr64 = eh64_zero;
    res = RRMOA(ep->f_fd,&ehdr64,0,sizeof(ehdr64),
        ep->f_filesize,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    ehdr = (struct generic_ehdr *)calloc(1,
        sizeof(struct generic_ehdr));
    if (!ehdr) {
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    res  = generic_ehdr_from_64(ep,ehdr,&ehdr64,errcode);
    if (res != DW_DLV_OK) {
        free(ehdr);
        return res;
    }
    ep->f_machine = (unsigned)ehdr->ge_machine;
    ep->f_flags = ehdr->ge_flags;
    return res;
}

int
_dwarf_load_elf_header(
    dwarf_elf_object_access_internals_t *ep,int*errcode)
{
    unsigned offsetsize = ep->f_offsetsize;
    int res = 0;

    if (offsetsize == 32) {
        res = elf_load_elf_header32(ep,errcode);
    } else if (offsetsize == 64) {
        if (sizeof(Dwarf_Unsigned) < 8) {
            *errcode =  DW_DLE_INTEGER_TOO_SMALL;
            return DW_DLV_ERROR;
        }
        res = elf_load_elf_header64(ep,errcode);
    } else {
        *errcode = DW_DLE_OFFSET_SIZE;
        return DW_DLV_ERROR;
    }
    return res;
}

static int
validate_links(
    dwarf_elf_object_access_internals_t *ep,
    Dwarf_Unsigned knownsect,
    Dwarf_Unsigned string_sect,
    int *errcode)
{
    struct generic_shdr* pshk = 0;

    if (!knownsect) {
        return DW_DLV_OK;
    }
    if (!string_sect) {
        *errcode = DW_DLE_ELF_STRING_SECTION_ERROR;
        return DW_DLV_ERROR;
    }
    pshk = ep->f_shdr + knownsect;
    if (string_sect != pshk->gh_link) {
        *errcode = DW_DLE_ELF_SECTION_LINK_ERROR;
        return DW_DLV_ERROR;
    }
    return DW_DLV_OK;
}

static int
string_endswith(const char *n,const char *q)
{
    size_t len = strlen(n);
    size_t qlen = strlen(q);
    const char *startpt = 0;

    if ( len < qlen) {
        return FALSE;
    }
    startpt = n + (len-qlen);
    if (strcmp(startpt,q)) {
        return FALSE;
    }
    return TRUE;
}

/*  We are allowing either SHT_GROUP or .group to indicate
    a group section, but really one should have both
    or neither! */
static int
elf_sht_groupsec(Dwarf_Unsigned type, const char *sname)
{
    /*  ARM compilers name SHT group "__ARM_grp<long name here>"
        not .group */
    if ((type == SHT_GROUP) || (!strcmp(sname,".group"))){
        return TRUE;
    }
    return FALSE;
}

/*  For SHT_GROUP sections.
    A group section starts with a 32bit flag
    word with value 1.
    32bit section numbers of the sections
    in the group follow the flag field. */
static int
read_gs_section_group(
    dwarf_elf_object_access_internals_t *ep,
    struct generic_shdr* psh,
    int *errcode)
{
    Dwarf_Unsigned i = 0;
    int res = 0;

    if (!psh->gh_sht_group_array) {
        Dwarf_Unsigned seclen = psh->gh_size;
        char *data = 0;
        char *dp = 0;
        Dwarf_Unsigned flags = psh->gh_flags;
        Dwarf_Unsigned* grouparray = 0;
        char dblock[4];
        Dwarf_Unsigned va = 0;
        Dwarf_Unsigned count = 0;
        Dwarf_Unsigned groupmallocsize = 0;
        int foundone = 0;

        if (seclen >= ep->f_filesize) {
            *errcode = DW_DLE_ELF_SECTION_GROUP_ERROR;
            return DW_DLV_ERROR;
        }
        if (seclen < DWARF_32BIT_SIZE) {
            *errcode = DW_DLE_ELF_SECTION_GROUP_ERROR;
            return DW_DLV_ERROR;
        }
        if (psh->gh_content) {
            /* Should NOT be set earlier! */
            *errcode = DW_DLE_ELF_SECTION_GROUP_ERROR;
            return DW_DLV_ERROR;
        }
        data = malloc(seclen);
        if (!data) {
            *errcode = DW_DLE_ALLOC_FAIL;
            return DW_DLV_ERROR;
        }
        if (psh->gh_entsize != DWARF_32BIT_SIZE) {
            *errcode = DW_DLE_ELF_SECTION_GROUP_ERROR;
            free(data);
            return DW_DLV_ERROR;
        }
        if (!psh->gh_entsize) {
            free(data);
            *errcode = DW_DLE_ELF_SECTION_GROUP_ERROR;
            return DW_DLV_ERROR;
        }
        count = seclen/psh->gh_entsize;
        if (count >= ep->f_loc_shdr.g_count) {
            /* Impossible */
            free(data);
            *errcode = DW_DLE_ELF_SECTION_GROUP_ERROR;
            return DW_DLV_ERROR;
        }
        res = RRMOA(ep->f_fd,data,psh->gh_offset,seclen,
            ep->f_filesize,errcode);
        if (res != DW_DLV_OK) {
            free(data);
            return res;
        }
        psh->gh_content = data;
        data = 0;
        psh->gh_was_alloc = TRUE;
        psh->gh_load_type = Dwarf_Alloc_Malloc;
        if (flags & SHF_COMPRESSED) {
#if defined(HAVE_ZLIB) && defined(HAVE_ZSTD)
            *errcode = 0;
            /*  decompress call will change gh_content
                pointer value.  */
            _dwarf_do_decompress_elf(ep,psh,errcode);
            /* decompress and set new section size */
            if (*errcode) {
                return DW_DLV_ERROR;
            }
#else /* COMPRESSED TEST */
            *errcode = DW_DLE_ZLIB_ZSTD_MISSING;
            return DW_DLV_ERROR;
#endif /* COMPRESSED TEST */
        }
        /*  Adding 1 is silly but possibly avoids a warning
            from a particular compiler. */
        groupmallocsize =  (1+count) * sizeof(Dwarf_Unsigned);
        if (groupmallocsize >= ep->f_filesize) {
            *errcode = DW_DLE_ELF_SECTION_GROUP_ERROR;
            return DW_DLV_ERROR;
        }
        grouparray = malloc(groupmallocsize);
        if (!grouparray) {
            *errcode = DW_DLE_ALLOC_FAIL;
            return DW_DLV_ERROR;
        }
        memcpy(dblock,psh->gh_content,DWARF_32BIT_SIZE);
        ASNAR(memcpy,va,dblock);
        /* There is ambiguity on the endianness of this stuff. */
        if (va != 1 && va != 0x1000000) {
            /*  Could be corrupted elf object. */
            *errcode = DW_DLE_ELF_SECTION_GROUP_ERROR;
            free(grouparray);
            return DW_DLV_ERROR;
        }
        grouparray[0] = 1;
        /*  A .group section will have 0 to G sections
            listed. Ignore the initial 'version' value
            of 1 in [0] */
        dp = psh->gh_content;
        /* Skip the initial group version */
        dp = dp + DWARF_32BIT_SIZE;
        /* Remember all the group members */
        for ( i = 1; i < count; ++i,dp += DWARF_32BIT_SIZE) {
            Dwarf_Unsigned gseca = 0;
            Dwarf_Unsigned gsecb = 0;
            struct generic_shdr* targpsh = 0;

            memcpy(dblock,dp,DWARF_32BIT_SIZE);
            ASNAR(memcpy,gseca,dblock);
            /*  Loading gseca and gsecb with different endianness.
                Only one of them can be of any use. */
            ASNAR(_dwarf_memcpy_swap_bytes,gsecb,dblock);
            if (!gseca) {
                /*  zero! Oops. No point in looking at gsecb */
                free(grouparray);
                *errcode = DW_DLE_ELF_SECTION_GROUP_ERROR;
                return DW_DLV_ERROR;
            }
            if (gseca >= ep->f_loc_shdr.g_count) {
                /*  Might be confused endianness by
                    the compiler generating the SHT_GROUP.
                    This is pretty horrible. */
                if (gsecb >= ep->f_loc_shdr.g_count) {
                    *errcode = DW_DLE_ELF_SECTION_GROUP_ERROR;
                    free(grouparray);
                    return DW_DLV_ERROR;
                }
                /*  Looks as though gsecb is the correct
                    interpretation.  Yes, ugly. */
                gseca = gsecb;
            }
            grouparray[i] = gseca;
            targpsh = ep->f_shdr + gseca;
            if (_dwarf_ignorethissection(targpsh->gh_namestring)){
                /* dp ok as is. Iterate again. */
                continue;
            }
            if (targpsh->gh_section_group_number) {
                /* multi-assignment to groups. Oops. */
                free(grouparray);
                *errcode = DW_DLE_ELF_SECTION_GROUP_ERROR;
                return DW_DLV_ERROR;
            }
            targpsh->gh_section_group_number =
                ep->f_sg_next_group_number;
            foundone = 1;
        }
        dp = 0;
        if (foundone) {
            ++ep->f_sg_next_group_number;
            ++ep->f_sht_group_type_section_count;
        }
        psh->gh_sht_group_array = grouparray;
        psh->gh_sht_group_array_count = count;
    }
    return DW_DLV_OK;
}
/*  Does related things.
    A)  Counts the number of SHT_GROUP
        and for each builds an array of the sections in the group
        (which we expect are all DWARF-related)
        and sets the group number in each mentioned section.
    B)  Counts the number of SHF_GROUP flags.
    C)  If gnu groups:
        ensure all the DWARF sections marked with right group
        based on A(we will mark unmarked as group 1,
        DW_GROUPNUMBER_BASE).
    D)  If arm groups (SHT_GROUP zero, SHF_GROUP non-zero):
        Check the relocations of all SHF_GROUP section
        FIXME: algorithm needed.

    If SHT_GROUP and SHF_GROUP this is GNU groups.
    If no SHT_GROUP and have SHF_GROUP this is
    arm cc groups and we must use relocation information
    to identify the group members.

    It seems(?) impossible for an object to have both
    dwo sections and (SHF_GROUP or SHT_GROUP), but
    we do not rule that out here.  */
static int
_dwarf_elf_setup_all_section_groups(
    dwarf_elf_object_access_internals_t *ep,
    int *errcode)
{
    struct generic_shdr* psh = 0;
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned count = 0;
    int res = 0;

    count = ep->f_loc_shdr.g_count;
    psh = ep->f_shdr;

    /* Does step A and step B */
    for (i = 0; i < count; ++psh,++i) {
        const char *name = psh->gh_namestring;

        if (is_empty_section(psh->gh_type)) {
            /*  No data here. */
            continue;
        }
        if (elf_sht_groupsec(psh->gh_type,name)) {
            ep->f_shf_group_flag_section_count++;
        } else {
            continue;
        }
        /* Looks like a section group. Do Step A. */
        res  =read_gs_section_group(ep,psh,errcode);
        if (res != DW_DLV_OK) {
            return res;
        }
    }
    /*  Any sections not marked above or here are in
        grep DW_GROUPNUMBER_BASE (1).
        Section C. */
    psh = ep->f_shdr;
    for (i = 0; i < count; ++psh,++i) {
        const char *name = psh->gh_namestring;
        int is_rel = FALSE;
        int is_rela = FALSE;

        if (is_empty_section(psh->gh_type)) {
            /*  No data here. */
            continue;
        }
        if (elf_sht_groupsec(psh->gh_type,name)) {
            continue;
        }
        /* Not a section group */
        if (string_endswith(name,".dwo")) {
            if (psh->gh_section_group_number) {
                /* multi-assignment to groups. Oops. */
                *errcode = DW_DLE_ELF_SECTION_GROUP_ERROR;
                return DW_DLV_ERROR;
            }
            psh->gh_is_dwarf = TRUE;
            psh->gh_section_group_number = DW_GROUPNUMBER_DWO;
            ep->f_dwo_group_section_count++;
        } else if (_dwarf_load_elf_section_is_dwarf(name,
            psh->gh_type,
            &is_rela,&is_rel)) {
            if (!psh->gh_section_group_number) {
                psh->gh_section_group_number = DW_GROUPNUMBER_BASE;
            }
            psh->gh_is_dwarf = TRUE;
        } else {
            /* Do nothing. */
        }
    }
    if (ep->f_sht_group_type_section_count) {
        /*  Not ARM. Done. */
    }
    if (!ep->f_shf_group_flag_section_count) {
        /*  Nothing more to do. */
        return DW_DLV_OK;
    }
    return DW_DLV_OK;
}

static int
_dwarf_elf_find_sym_sections(
    dwarf_elf_object_access_internals_t *ep,
    int *errcode)
{
    struct generic_shdr* psh = 0;
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned count = 0;
    int res = 0;

    count = ep->f_loc_shdr.g_count;
    psh = ep->f_shdr;
    for (i = 0; i < count; ++psh,++i) {
        const char *name = psh->gh_namestring;
        if (is_empty_section(psh->gh_type)) {
            /*  No data here. */
            continue;
        }
        if (!strcmp(name,".dynsym")) {
            ep->f_dynsym_sect_index = i;
            ep->f_loc_dynsym.g_offset = psh->gh_offset;
        } else if (!strcmp(name,".dynstr")) {
            ep->f_dynsym_sect_strings_sect_index = i;
            ep->f_dynsym_sect_strings_max = psh->gh_size;
        } else if (!strcmp(name,".symtab")) {
            ep->f_symtab_sect_index = i;
            ep->f_loc_symtab.g_offset = psh->gh_offset;
        } else if (!strcmp(name,".strtab")) {
            ep->f_symtab_sect_strings_sect_index = i;
            ep->f_symtab_sect_strings_max = psh->gh_size;
        } else if (!strcmp(name,".dynamic")) {
            ep->f_dynamic_sect_index = i;
            ep->f_loc_dynamic.g_offset = psh->gh_offset;
        }
    }
    res = validate_links(ep,ep->f_symtab_sect_index,
        ep->f_symtab_sect_strings_sect_index,errcode);
    if (res!= DW_DLV_OK) {
        return res;
    }
    return DW_DLV_OK;
}

int
_dwarf_load_elf_sectheaders(
    dwarf_elf_object_access_internals_t *ep,int*errcode)
{
    int res = 0;

    if (ep->f_offsetsize == 32) {
        res  = elf_load_sectheaders32(ep,ep->f_ehdr->ge_shoff,
            ep->f_ehdr->ge_shentsize,
            ep->f_ehdr->ge_shnum,errcode);
    } else  if (ep->f_offsetsize == 64) {
        res  = elf_load_sectheaders64(ep,ep->f_ehdr->ge_shoff,
            ep->f_ehdr->ge_shentsize,
            ep->f_ehdr->ge_shnum,errcode);
    } else {
        *errcode = DW_DLE_OFFSET_SIZE;
        return DW_DLV_ERROR;
    }
    if (res != DW_DLV_OK) {
        return res;
    }
    res  = _dwarf_elf_load_sectstrings(ep,
        ep->f_ehdr->ge_shstrndx,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    res  = _dwarf_elf_load_sect_namestrings(ep,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    res  = _dwarf_elf_find_sym_sections(ep,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    res = _dwarf_elf_setup_all_section_groups(ep,errcode);
    return res;
}
