/*
  Copyright (C) 2000-2010 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright 2007-2010 Sun Microsystems, Inc. All rights reserved.
  Portions Copyright 2008-2018 David Anderson. All rights reserved.
  Portions Copyright 2008-2010 Arxan Technologies, Inc. All rights reserved.
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

#ifndef _LIBDWARFP_H
#define _LIBDWARFP_H

#ifdef DWP_API
#undef DWP_API
#endif /* DWP_API */

#ifndef LIBDWARF_STATIC
# if defined(_WIN32) || defined(__CYGWIN__)
#  ifdef LIBDWARF_BUILD
#   define DWP_API __declspec(dllexport)
#  else /* !LIBDWARF_BUILD */
#   define DWP_API __declspec(dllimport)
#  endif /* LIBDWARF_BUILD */
# elif (defined(__SUNPRO_C)  || defined(__SUNPRO_CC))
#  if defined(PIC) || defined(__PIC__)
#   define DWP_API __global
#  endif /* __PIC__ */
# elif (defined(__GNUC__) && __GNUC__ >= 4) || \
    defined(__INTEL_COMPILER)
#  if defined(PIC) || defined(__PIC__)
#   define DWP_API __attribute__ ((visibility("default")))
#  endif /* PIC */
# endif /* WIN32 SUNPRO GNUC  */
#endif /* !LIBDWARF_STATIC */

#ifndef DWP_API
#define DWP_API
#endif

#ifdef __cplusplus
extern "C" {
#endif
/*
    libdwarfp.h

    For libdwarf producers
    Does not stand on its own, so includes libdwarf.h.
*/

/* Opaque types for Producer Library. */
typedef struct Dwarf_P_Debug_s*       Dwarf_P_Debug;
typedef struct Dwarf_P_Die_s*         Dwarf_P_Die;
typedef struct Dwarf_P_Attribute_s*   Dwarf_P_Attribute;
typedef struct Dwarf_P_Fde_s*         Dwarf_P_Fde;
typedef struct Dwarf_P_Expr_s*        Dwarf_P_Expr;
typedef Dwarf_Unsigned                Dwarf_Tag;

/*  ===== the following DW_DLC options are for producing DWARF,
    not used for reading/consuming DWARF. */
/* dwarf_producer_init* access flag modifiers
   No longer depends on compile-time settings for
   how to produce 64bit offset. See DW_DLC_IRIX_OFFSET64.
   Historic  versions. One of
   If DW_DLC_POINTER64 is not set DW_DLC_POINTER32 is assumed.
   If DW_DLC_OFFSET64 or DW_DLC_IRIX_OFFSET64 is not
   set 32bit offset DWARF is assumed.
   Non-MIPS Non IA64 should use DW_DLC_SYMBOLIC_RELOCATIONS
   and handle the relocation creation for the target
   itself using the symbolic relocations to do so.
   See the Dwarf_Rel_Type enum relocation indicators.
*/

/*  64-bit address-size target */
#define DW_DLC_SIZE_64            0x40000000 /* old spelling */
#define DW_DLC_POINTER64          0x40000000 /* correct spelling */

/*  32-bit address-size target */
#define DW_DLC_SIZE_32            0x20000000 /* old spelling */
#define DW_DLC_POINTER32          0x20000000 /* correct spelling */
/*  32-bit offset-size ELF object (ELFCLASS32) */
#define DW_DLC_ELF_OFFSET_SIZE_32 0x00400000 /* DO NOT USE */
/*  DW_DLC_OFFSET32 is the default, the bit is not checked. */
#define DW_DLC_OFFSET32           0x00010000 /* default offset size */

/*  64-bit offset-size DWARF offsets  */
#define DW_DLC_OFFSET_SIZE_64     0x10000000 /* old spelling */
#define DW_DLC_OFFSET64           0x10000000 /* correct spelling */

/*  64-bit offset-size ELF object (ELFCLASS64)  */
#define DW_DLC_ELF_OFFSET_SIZE_64 0x00020000 /* DO NOT USE. */

/*  Special for IRIX only. For producing DWARF with
    Elf 64bit offset headers and non-standard IRIX
    64bit offset DWARF .debug_info etc compilation unit headers. */
#define DW_DLC_IRIX_OFFSET64      0x00200000

/*  Old style Elf binary relocation (.rel) records. The default
    For producing DWARF */
#define DW_DLC_STREAM_RELOCATIONS 0x02000000
/*  Usable with assembly output because it is up to the producer to
    deal with locations in whatever manner the calling producer
    code wishes.  For example, when the libdwarf caller wishes
    to produce relocations differently than the binary
    relocation bits that libdwarf Stream Relocations generate.
    */
#define DW_DLC_SYMBOLIC_RELOCATIONS 0x04000000

#define DW_DLC_TARGET_BIGENDIAN     0x08000000
#define DW_DLC_TARGET_LITTLEENDIAN  0x00100000
/* ===== END DW_DLC options LIBDWARFP */

enum Dwarf_Rel_Type {
    dwarf_drt_none,        /* Should not get to caller */
    dwarf_drt_data_reloc,  /* Simple normal relocation. */
    dwarf_drt_segment_rel, /* Special reloc, exceptions. */
    /* dwarf_drt_first_of_length_pair  and drt_second
        are for for the  .word end - begin case. */
    dwarf_drt_first_of_length_pair,
    dwarf_drt_second_of_length_pair
};
typedef struct Dwarf_P_Marker_s * Dwarf_P_Marker;
struct Dwarf_P_Marker_s {
    Dwarf_Unsigned ma_marker;
    Dwarf_Unsigned ma_offset;
};
typedef struct Dwarf_Relocation_Data_s  * Dwarf_Relocation_Data;
struct Dwarf_Relocation_Data_s {
    unsigned char drd_type;   /* Cast to/from Dwarf_Rel_Type
        to keep size small in struct. */
    unsigned char drd_length; /* Length in bytes of data being
        relocated. 4 for 32bit data,
        8 for 64bit data. */
    Dwarf_Unsigned       drd_offset; /* Where the data to reloc is. */
    Dwarf_Unsigned       drd_symbol_index;
};

typedef struct Dwarf_P_String_Attr_s  * Dwarf_P_String_Attr;
struct Dwarf_P_String_Attr_s {
    /* Offset of string attribute data */
    Dwarf_Unsigned        sa_offset;
    Dwarf_Unsigned        sa_nbytes;
};

/* DWARF Producer Interface */
/*  New form June, 2011. Adds user_data argument. */
typedef int (*Dwarf_Callback_Func)(
    const char*     /*name*/,
    int             /*size*/,
    Dwarf_Unsigned  /*type*/,
    Dwarf_Unsigned  /*flags*/,
    Dwarf_Unsigned  /*link*/,
    Dwarf_Unsigned  /*info*/,
    Dwarf_Unsigned* /*sect_name_index*/,
    void *          /*user_data*/,
    int*            /*error*/);

/*  Returns DW_DLV_OK or DW_DLV_ERROR and
    if DW_DLV_OK returns the Dwarf_P_Debug
    pointer through the dbg_returned argument. */
DWP_API int dwarf_producer_init(
    Dwarf_Unsigned        /*flags*/,
    Dwarf_Callback_Func   /*func*/,
    Dwarf_Handler         /*errhand*/,
    Dwarf_Ptr             /*errarg*/,
    void *                /*user_data*/,
    const char *isa_name, /* See isa/abi names in pro_init.c */
    const char *dwarf_version, /* V2 V3 V4 or V5. */
    const char *extra,    /* Extra input strings, comma separated. */
    Dwarf_P_Debug *,      /* dbg_returned */
    Dwarf_Error *         /*error*/);

/*  Returns DW_DLV_OK or DW_DLV_ERROR.
    The desired form must be DW_FORM_string (the default)
    or DW_FORM_strp.  */
DWP_API int dwarf_pro_set_default_string_form(Dwarf_P_Debug /*dbg*/,
    int /*desired_form*/,
    Dwarf_Error*     /*error*/);

/*  New September 2016. The preferred interface. */
DWP_API int dwarf_transform_to_disk_form_a(Dwarf_P_Debug /*dbg*/,
    Dwarf_Unsigned *   /*nbufs_out*/,
    Dwarf_Error*     /*error*/);

/* New September 2016. Preferred. */
DWP_API int dwarf_get_section_bytes_a(Dwarf_P_Debug /*dbg*/,
    Dwarf_Unsigned   /*dwarf_section*/,
    Dwarf_Unsigned*  /*elf_section_index*/,
    Dwarf_Unsigned*  /*length*/,
    Dwarf_Ptr     *  /*section_bytes*/,
    Dwarf_Error*     /*error*/);

DWP_API int  dwarf_get_relocation_info_count(
    Dwarf_P_Debug    /*dbg*/,
    Dwarf_Unsigned * /*count_of_relocation_sections*/,
    int *                /*drd_buffer_version*/,
    Dwarf_Error*     /*error*/);

DWP_API int dwarf_get_relocation_info(
    Dwarf_P_Debug         /*dbg*/,
    Dwarf_Unsigned        * /*elf_section_index*/,
    Dwarf_Unsigned        * /*elf_section_index_link*/,
    Dwarf_Unsigned        * /*relocation_buffer_count*/,
    Dwarf_Relocation_Data * /*reldata_buffer*/,
    Dwarf_Error*            /*error*/);

/* v1:  no drd_length field, enum explicit */
/* v2:  has the drd_length field, enum value in uchar member */
#define DWARF_DRD_BUFFER_VERSION 2

/* Markers are not written  to DWARF2/3/4, they are user
   defined and may be used for any purpose.
*/
DWP_API int dwarf_add_die_marker_a(Dwarf_P_Debug dbg,
    Dwarf_P_Die die,
    Dwarf_Unsigned marker,
    Dwarf_Error * error);

DWP_API int dwarf_get_die_markers_a(
    Dwarf_P_Debug     /*dbg*/,
    Dwarf_P_Marker *  /*marker_list*/,
    Dwarf_Unsigned *  /*marker_count*/,
    Dwarf_Error *     /*error*/);

DWP_API int dwarf_get_string_attributes_count(Dwarf_P_Debug,
    Dwarf_Unsigned *,
    int *,
    Dwarf_Error *);

DWP_API int dwarf_get_string_attributes_info(Dwarf_P_Debug,
    Dwarf_Signed *,
    Dwarf_Unsigned *,
    Dwarf_P_String_Attr *,
    Dwarf_Error *);

DWP_API void dwarf_reset_section_bytes(Dwarf_P_Debug /*dbg*/);

/*  Returns DW_DLV_OK or DW_DLV_ERROR */
DWP_API int dwarf_producer_finish_a(Dwarf_P_Debug /*dbg*/,
    Dwarf_Error* /*error*/);

/* New July 2020 for testing DWARF5 */
DWP_API int dwarf_add_debug_sup(Dwarf_P_Debug /*dbg*/,
    Dwarf_Half      /*version*/,
    Dwarf_Small     /*is_supplementary*/,
    char          * /*filename*/,
    Dwarf_Unsigned  /*checksum_len*/,
    Dwarf_Small   * /*checksum*/,
    Dwarf_Error * /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_AT_targ_address_c(Dwarf_P_Debug /*dbg*/,
    Dwarf_P_Die         /*ownerdie*/,
    Dwarf_Half          /*attr*/,
    Dwarf_Unsigned      /*pc_value*/,
    Dwarf_Unsigned      /*sym_index*/,
    Dwarf_P_Attribute * /*attr_out*/,
    Dwarf_Error*        /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_AT_block_a(Dwarf_P_Debug /*dbg*/,
    Dwarf_P_Die     /*ownerdie*/,
    Dwarf_Half      /*attr*/,
    Dwarf_Small*    /*block_data*/,
    Dwarf_Unsigned  /*block_len*/,
    Dwarf_P_Attribute * /*attr_out*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_AT_ref_address_a(Dwarf_P_Debug /*dbg*/,
    Dwarf_P_Die     /*ownerdie*/,
    Dwarf_Half      /*attr*/,
    Dwarf_Unsigned  /*pc_value*/,
    Dwarf_Unsigned  /*sym_index*/,
    Dwarf_P_Attribute * /*attr_out*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_AT_unsigned_const_a(Dwarf_P_Debug /*dbg*/,
    Dwarf_P_Die     /*ownerdie*/,
    Dwarf_Half      /*attr*/,
    Dwarf_Unsigned  /*value*/,
    Dwarf_P_Attribute * /*attr_out*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_AT_signed_const_a(Dwarf_P_Debug /*dbg*/,
    Dwarf_P_Die     /*ownerdie*/,
    Dwarf_Half      /*attr*/,
    Dwarf_Signed    /*value*/,
    Dwarf_P_Attribute * /*attr_out*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_AT_reference_c(Dwarf_P_Debug /*dbg*/,
    Dwarf_P_Die     /*ownerdie*/,
    Dwarf_Half      /*attr*/,
    Dwarf_P_Die     /*otherdie*/,
    Dwarf_P_Attribute * /*attr_out*/,
    Dwarf_Error*    /*error*/);

/* The following is for out-of-order cu-local
   references.  Allowing nominating the target Dwarf_P_Die
   after calling dwarf_add_AT_reference with a NULL otherdie
   after a single pass thru the DIE generation. Needed
   for forward-references.
   New 22 October, 2013.
*/
DWP_API int dwarf_fixup_AT_reference_die(Dwarf_P_Debug /*dbg*/,
    Dwarf_Half    /* attrnum */,
    Dwarf_P_Die   /* sourcedie*/,
    Dwarf_P_Die   /* targetdie*/,
    Dwarf_Error * /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_AT_dataref_a(
    Dwarf_P_Debug   /*dbg*/,
    Dwarf_P_Die     /*ownerdie*/,
    Dwarf_Half      /*attr*/,
    Dwarf_Unsigned  /*pcvalue*/,
    Dwarf_Unsigned  /*sym_index*/,
    Dwarf_P_Attribute * /*attr_out*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_AT_const_value_string_a(
    Dwarf_P_Die /*ownerdie*/,
    char*           /*string_value*/,
    Dwarf_P_Attribute * /*attr_out*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_AT_location_expr_a(Dwarf_P_Debug /*dbg*/,
    Dwarf_P_Die     /*ownerdie*/,
    Dwarf_Half      /*attr*/,
    Dwarf_P_Expr    /*loc_expr*/,
    Dwarf_P_Attribute * /*attr_out*/,
    Dwarf_Error*    /*error*/);

/*  Preferred as of December 2018. */
DWP_API int dwarf_add_AT_string_a(Dwarf_P_Debug /*dbg*/,
    Dwarf_P_Die     /*ownerdie*/,
    Dwarf_Half      /*attr*/,
    char*           /*string*/,
    Dwarf_P_Attribute * /*attr_out*/,
    Dwarf_Error*     /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_AT_flag_a(Dwarf_P_Debug /*dbg*/,
    Dwarf_P_Die     /*ownerdie*/,
    Dwarf_Half      /*attr*/,
    Dwarf_Small     /*flag*/,
    Dwarf_P_Attribute * /*attr_out*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_AT_producer_a(Dwarf_P_Die /*ownerdie*/,
    char*           /*producer_string*/,
    Dwarf_P_Attribute * /*attr_out*/,
    Dwarf_Error*    /*error*/);

/*  October 2017 for DW_FORM_data16. Usable with any attribute,
    though it should only be in limited use. DWARF5 only.
    Returns DW_DLV_OK on success, DW_DLV_ERROR on failure.
    Returns the new attribute pointer through *return_attr. */
DWP_API int dwarf_add_AT_data16(Dwarf_P_Die /*ownerdie*/,
    Dwarf_Half    /*attrnum*/,
    Dwarf_Form_Data16 * /* pointstovalue */,
    Dwarf_P_Attribute * /* return_attr */,
    Dwarf_Error * /*error*/);

/*  November 2018. DW_AT_implicit const generation. */
DWP_API int dwarf_add_AT_implicit_const(Dwarf_P_Die /*ownerdie*/,
    Dwarf_Half    /*attrnum*/,
    Dwarf_Signed  /*signed_value*/,
    Dwarf_P_Attribute * /*outattr*/,
    Dwarf_Error * /*error*/);

/*  New December 2018. */
DWP_API int dwarf_add_AT_any_value_sleb_a(Dwarf_P_Die /*ownerdie*/,
    Dwarf_Half    /*attrnum*/,
    Dwarf_Signed  /*signed_value*/,
    Dwarf_P_Attribute * /*outattr*/,
    Dwarf_Error * /*error*/);

/*  Preferred as of December 2018. */
DWP_API int dwarf_add_AT_const_value_signedint_a(
    Dwarf_P_Die     /*ownerdie*/,
    Dwarf_Signed    /*signed_value*/,
    Dwarf_P_Attribute * /*outattr*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_AT_any_value_uleb_a(Dwarf_P_Die /*ownerdie*/,
    Dwarf_Half      /*attrnum*/,
    Dwarf_Unsigned  /*signed_value*/,
    Dwarf_P_Attribute * /*outattr*/,
    Dwarf_Error *   /*error*/);

/*  New December 2018.  .Only for DW_AT_const_value. */
DWP_API int dwarf_add_AT_const_value_unsignedint_a(
    Dwarf_P_Die     /*ownerdie*/,
    Dwarf_Unsigned  /*unsigned_value*/,
    Dwarf_P_Attribute * /*outattr*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_AT_comp_dir_a(Dwarf_P_Die /*ownerdie*/,
    char*           /*current_working_directory*/,
    Dwarf_P_Attribute * /*outattr*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_AT_name_a(Dwarf_P_Die    /*die*/,
    char*           /*name*/,
    Dwarf_P_Attribute * /*outattr*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_AT_with_ref_sig8_a(
    Dwarf_P_Die   /*ownerdie */,
    Dwarf_Half    /*attrnum */,
    const Dwarf_Sig8 *  /*sig8_in*/,
    Dwarf_P_Attribute * /*outattr*/,
    Dwarf_Error * /*error*/);

/* Producer line creation functions (.debug_line) */
DWP_API int dwarf_add_directory_decl_a(Dwarf_P_Debug /*dbg*/,
    char*           /*name*/,
    Dwarf_Unsigned * /*index_in_directories*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_file_decl_a(Dwarf_P_Debug /*dbg*/,
    char*           /*name*/,
    Dwarf_Unsigned  /*dir_index*/,
    Dwarf_Unsigned  /*time_last_modified*/,
    Dwarf_Unsigned  /*length*/,
    Dwarf_Unsigned * /*file_entry_count_out*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_line_entry_c(Dwarf_P_Debug /*dbg*/,
    Dwarf_Unsigned  /*file_index*/,
    Dwarf_Addr      /*code_address*/,
    Dwarf_Unsigned  /*lineno*/,
    Dwarf_Signed    /*column_number*/,
    Dwarf_Bool      /*is_source_stmt_begin*/,
    Dwarf_Bool      /*is_basic_block_begin*/,
    Dwarf_Bool      /*is_epilogue_begin*/,
    Dwarf_Bool      /*is_prologue_end*/,
    Dwarf_Unsigned  /*isa*/,
    Dwarf_Unsigned  /*discriminator*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_lne_set_address_a(Dwarf_P_Debug /*dbg*/,
    Dwarf_Unsigned  /*offset*/,
    Dwarf_Unsigned  /*symbol_index*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_lne_end_sequence_a(Dwarf_P_Debug /*dbg*/,
    Dwarf_Addr      /*end_address*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_frame_cie_a(Dwarf_P_Debug /*dbg*/,
    char*           /*augmenter*/,
    Dwarf_Small     /*code_alignment_factor*/,
    Dwarf_Small     /*data_alignment_factor*/,
    Dwarf_Small     /*return_address_reg*/,
    Dwarf_Ptr       /*initialization_bytes*/,
    Dwarf_Unsigned  /*init_byte_len*/,
    Dwarf_Unsigned * /*cie_index_out*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_frame_fde_c(
    Dwarf_P_Debug  /*dbg*/,
    Dwarf_P_Fde    /*fde*/,
    Dwarf_P_Die    /*die*/,
    Dwarf_Unsigned /*cie*/,
    Dwarf_Addr     /*virt_addr*/,
    Dwarf_Unsigned /*code_len*/,
    Dwarf_Unsigned /*sym_idx*/,
    Dwarf_Unsigned /*sym_idx_of_end*/,
    Dwarf_Addr     /*offset_from_end_sym*/,
    Dwarf_Unsigned * /*index_to_fde*/,
    Dwarf_Error*   /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_frame_info_c(
    Dwarf_P_Debug   /*dbg*/,
    Dwarf_P_Fde     /*fde*/,
    Dwarf_P_Die     /*die*/,
    Dwarf_Unsigned  /*cie*/,
    Dwarf_Addr      /*virt_addr*/,
    Dwarf_Unsigned  /*code_len*/,
    Dwarf_Unsigned  /*symidx*/,
    Dwarf_Unsigned  /*end_symbol */,
    Dwarf_Addr      /*offset_from_end_symbol */,
    Dwarf_Signed    /*offset_into_exception_tables*/,
    Dwarf_Unsigned  /*exception_table_symbol*/,
    Dwarf_Unsigned * /*fde_index_out*/,
    Dwarf_Error*    /*error*/);

/*  New December 2018. Preferred version. */
DWP_API int dwarf_add_fde_inst_a(
    Dwarf_P_Fde     /*fde*/,
    Dwarf_Small     /*op*/,
    Dwarf_Unsigned  /*val1*/,
    Dwarf_Unsigned  /*val2*/,
    Dwarf_Error*    /*error*/);

/* New September 17, 2009 */
DWP_API int dwarf_insert_fde_inst_bytes(
    Dwarf_P_Debug  /*dbg*/,
    Dwarf_P_Fde    /*fde*/,
    Dwarf_Unsigned /*len*/,
    Dwarf_Ptr      /*ibytes*/,
    Dwarf_Error*   /*error*/);

DWP_API int dwarf_new_fde_a(Dwarf_P_Debug    /*dbg*/,
    Dwarf_P_Fde * /*fde_out*/,
    Dwarf_Error* /*error*/);

DWP_API int dwarf_fde_cfa_offset_a(
    Dwarf_P_Fde     /*fde*/,
    Dwarf_Unsigned  /*register_number*/,
    Dwarf_Signed    /*offset*/,
    Dwarf_Error*    /*error*/);

/*  die creation & addition routines
    dwarf_new_die_a() new September 2016.
    Preferred over dwarf_new_die(). */
DWP_API int dwarf_new_die_a(
    Dwarf_P_Debug   /*dbg*/,
    Dwarf_Tag       /*tag*/,
    Dwarf_P_Die     /*parent*/,
    Dwarf_P_Die     /*child*/,
    Dwarf_P_Die     /*left */,
    Dwarf_P_Die     /*right*/,
    Dwarf_P_Die   * /*die_out*/,
    Dwarf_Error*    /*error*/);

/* New September 2016. */
DWP_API int dwarf_add_die_to_debug_a(
    Dwarf_P_Debug   /*dbg*/,
    Dwarf_P_Die     /*die*/,
    Dwarf_Error*    /*error*/);

/* Markers are not written  to DWARF2/3/4, they are user
   defined and may be used for any purpose.
*/
DWP_API int dwarf_get_die_marker_a(
    Dwarf_P_Debug   /*dbg*/,
    Dwarf_P_Die     /*die*/,
    Dwarf_Unsigned *  /*marker*/,
    Dwarf_Error *   /*error*/);

/*  New September 2016. Preferred version */
DWP_API int dwarf_die_link_a(
    Dwarf_P_Die     /*die*/,
    Dwarf_P_Die     /*parent*/,
    Dwarf_P_Die     /*child*/,
    Dwarf_P_Die     /*left*/,
    Dwarf_P_Die     /*right*/,
    Dwarf_Error*    /*error*/);

/*  dwarf_compress_integer_block_a( new 11 February 2019.
    Like the earlier version this turns an array of signed
    integers into a block of sleb values (and if the
    values are small enough it might be a compression!
    Or it could be an expansion...). Return DW_DLV_OK
    on success. Supercedes dwarf_compress_integer_block():
    as no ugly cast needed to know if
    dwarf_compress_integer_block_a() succeeds or not. */
DWP_API int dwarf_compress_integer_block_a(
    Dwarf_P_Debug    /*dbg*/,
    Dwarf_Unsigned   /*input_array_length*/,
    Dwarf_Signed   * /*input_array*/,
    Dwarf_Unsigned * /*output_block_len*/,
    void          ** /*output_block_returned*/,
    Dwarf_Error    * /*error */);
/* Operations to create location expressions. */
DWP_API int dwarf_new_expr_a(Dwarf_P_Debug /*dbg*/,
    Dwarf_P_Expr * /*expr_out*/,
    Dwarf_Error* /*error*/);

DWP_API void dwarf_expr_reset(
    Dwarf_P_Expr      /*expr*/,
    Dwarf_Error*      /*error*/);

DWP_API int dwarf_add_expr_gen_a(
    Dwarf_P_Expr      /*expr*/,
    Dwarf_Small       /*opcode*/,
    Dwarf_Unsigned    /*val1*/,
    Dwarf_Unsigned    /*val2*/,
    Dwarf_Unsigned  * /*next_byte_offset*/,
    Dwarf_Error*      /*error*/);

DWP_API int dwarf_add_expr_addr_c(
    Dwarf_P_Expr      /*expr*/,
    Dwarf_Unsigned    /*addr*/,
    Dwarf_Unsigned    /*sym_index*/,
    Dwarf_Unsigned * /*next_byte_offset_out*/,
    Dwarf_Error*      /*error*/);

DWP_API int dwarf_expr_current_offset_a(
    Dwarf_P_Expr      /*expr*/,
    Dwarf_Unsigned * /*next_byte_offset_out*/,
    Dwarf_Error*      /*error*/);

DWP_API int dwarf_expr_into_block_a(
    Dwarf_P_Expr      /*expr*/,
    Dwarf_Unsigned*   /*length*/,
    Dwarf_Small    ** /*start_address*/,
    Dwarf_Error*      /*error*/);

DWP_API int dwarf_add_arange_c(
    Dwarf_P_Debug  /*dbg*/,
    Dwarf_Addr     /*begin_address*/,
    Dwarf_Unsigned /*length*/,
    Dwarf_Unsigned /*symbol_index*/,
    Dwarf_Unsigned /*end_symbol_index*/,
    Dwarf_Addr     /*offset_from_end_symbol*/,
    Dwarf_Error *  /*error*/);

DWP_API int dwarf_add_pubname_a(
    Dwarf_P_Debug      /*dbg*/,
    Dwarf_P_Die        /*die*/,
    char*              /*pubname_name*/,
    Dwarf_Error*       /*error*/);

DWP_API int dwarf_add_pubtype_a(
    Dwarf_P_Debug      /*dbg*/,
    Dwarf_P_Die        /*die*/,
    char*              /*pubtype_name*/,
    Dwarf_Error*       /*error*/);

DWP_API int dwarf_add_funcname_a(
    Dwarf_P_Debug      /*dbg*/,
    Dwarf_P_Die        /*die*/,
    char*              /*func_name*/,
    Dwarf_Error*       /*error*/);

DWP_API int dwarf_add_typename_a(
    Dwarf_P_Debug     /*dbg*/,
    Dwarf_P_Die       /*die*/,
    char*             /*type_name*/,
    Dwarf_Error*      /*error*/);

DWP_API int dwarf_add_varname_a(
    Dwarf_P_Debug     /*dbg*/,
    Dwarf_P_Die       /*die*/,
    char*             /*var_name*/,
    Dwarf_Error*      /*error*/);

DWP_API int dwarf_add_weakname_a(
    Dwarf_P_Debug    /*dbg*/,
    Dwarf_P_Die      /*die*/,
    char*            /*weak_name*/,
    Dwarf_Error*     /*error*/);

/*  .debug_names producer functions */

/*  dwarf_force_dnames forces creation
    of .debug_names (if DWARF5 being produced)
    even if empty. Only for testing libdwarf. */
DWP_API int dwarf_force_dnames(Dwarf_P_Debug /* dbg */,
    int  /*elfsectno*/,
    Dwarf_Error*     /*error*/);

/*  Other debug_names functions are needed... FIXME */

/*  end .debug_names producer functions */

/*  .debug_macinfo producer functions
    Functions must be called in right order: the section is output
    In the order these are presented.
*/
DWP_API int dwarf_def_macro(Dwarf_P_Debug /*dbg*/,
    Dwarf_Unsigned   /*line*/,
    char *           /*macname, with (arglist), no space before (*/,
    char *           /*macvalue*/,
    Dwarf_Error*     /*error*/);

DWP_API int dwarf_undef_macro(Dwarf_P_Debug /*dbg*/,
    Dwarf_Unsigned   /*line*/,
    char *           /*macname, no arglist, of course*/,
    Dwarf_Error*     /*error*/);

DWP_API int dwarf_start_macro_file(Dwarf_P_Debug /*dbg*/,
    Dwarf_Unsigned   /*fileindex*/,
    Dwarf_Unsigned   /*linenumber*/,
    Dwarf_Error*     /*error*/);

DWP_API int dwarf_end_macro_file(Dwarf_P_Debug /*dbg*/,
    Dwarf_Error*     /*error*/);

DWP_API int dwarf_vendor_ext(Dwarf_P_Debug /*dbg*/,
    Dwarf_Unsigned   /*constant*/,
    char *           /*string*/,
    Dwarf_Error*     /*error*/);

/* end macinfo producer functions */

DWP_API int dwarf_pro_get_string_stats(Dwarf_P_Debug /*dbg*/,
    Dwarf_Unsigned * /*str_count*/,
    Dwarf_Unsigned * /*str_total_length*/,
    Dwarf_Unsigned * /*count_debug_str*/,
    Dwarf_Unsigned * /*len_debug_str*/,
    Dwarf_Unsigned * /*reused_count*/,
    Dwarf_Unsigned * /*reused_len*/,
    Dwarf_Error    * /*error*/);

#ifdef __cplusplus
}
#endif
#endif /* _LIBDWARFP_H */
