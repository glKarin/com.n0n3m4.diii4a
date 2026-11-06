/*
  Copyright (C) 2000,2002,2004 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright 2002-2010 Sun Microsystems, Inc. All rights reserved.
  Portions Copyright 2008-2021 David Anderson. All rights reserved.
  Portions Copyright 2010-2012 SN Systems Ltd. All rights reserved.

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

/* The target address is given: the place in the source integer
   is to be determined.
*/
#ifdef WORDS_BIGENDIAN
#define WRITE_UNALIGNED(dbg,dest,source, srclength,len_out) \
    { \
        dbg->de_copy_word(dest,                     \
            ((const char *)source) +(srclength)-(len_out),\
            (len_out)) ;                            \
    }
#else /* LITTLE ENDIAN */
#define WRITE_UNALIGNED(dbg,dest,source, srclength,len_out) \
    { \
        dbg->de_copy_word( (dest) , \
            ((const char *)source) ,      \
            (len_out)) ;            \
    }
#endif /* BIG- LITTLE-ENDIAN */

#if defined(sparc) && defined(sun)
#define REL32 Elf32_Rela
#define REL64 Elf64_Rela
#define REL_SEC_PREFIX ".rela"
#else
#define REL32 Elf32_Rel
#define REL64 Elf64_Rel
#define REL_SEC_PREFIX ".rel"
#endif

#ifndef SHT_REL
#define SHT_REL 9
#endif /*SHT_REL */
#ifndef SHN_UNDEF
#define SHN_UNDEF 0
#endif /* SHN_UNDEF */

#ifndef SHF_MIPS_NOSTRIP
/* if this is not defined, we probably don't need it: just use 0 */
#define SHF_MIPS_NOSTRIP 0
#endif
#ifndef R_MIPS_NONE
#define R_MIPS_NONE 0
#endif

#ifdef WORDS_BIGENDIAN
#define ASNOUT(t,s,l)                       \
    do {                                    \
        unsigned sbyte = 0;                 \
        const char *p = 0;                  \
        if (l > sizeof(s)) {                \
            _dwarf_p_error(dbg, error,      \
                DW_DLE_DEBUG_FRAME_LENGTH_BAD);\
            return DW_DLV_ERROR;            \
        }                                   \
        sbyte = sizeof(s) - l;              \
        p = (const char *)(&s);             \
        dbg->de_copy_word(t,(const void *)(p+sbyte),l);\
    } while (0)
#else /* LITTLEENDIAN */
#define ASNOUT(t,s,l)                       \
    do {                                    \
        const char *p = 0;                  \
        if (l > sizeof(s)) {                \
            _dwarf_p_error(dbg, error,      \
                DW_DLE_DEBUG_FRAME_LENGTH_BAD);\
            return DW_DLV_ERROR;            \
        }                                   \
        p = (const char *)(&s);             \
        dbg->de_copy_word(t,(const void *)p,l); \
    } while (0)
#endif /* ENDIANNESS */

/* Indicates no relocation needed. */
#define NO_ELF_SYM_INDEX        0
