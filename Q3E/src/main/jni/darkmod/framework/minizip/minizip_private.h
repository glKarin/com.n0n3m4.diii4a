/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#pragma once

#include <minizip/unzip.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

//stgatilov: Note that these data structures must be kept in sync with stuff defined in unzip.c.
//Given that minizip's development is almost frozen, I guess they will rarely change.

#ifndef ALLOC
# define ALLOC(size) (malloc(size))
#endif

#ifndef UNZ_BUFSIZE
#define UNZ_BUFSIZE (16384)
#endif

/* unz_file_info64_internal contain internal info about a file in zipfile*/
typedef struct unz_file_info64_internal_s
{
    ZPOS64_T offset_curfile;/* relative offset of local header 8 bytes */
} unz_file_info64_internal;

/* file_in_zip_read_info_s contain internal information about a file in zipfile,
    when reading and decompress it */
typedef struct
{
    char  *read_buffer;         /* internal buffer for compressed data */
    z_stream stream;            /* zLib stream structure for inflate */

#ifdef HAVE_BZIP2
    bz_stream bstream;          /* bzLib stream structure for bziped */
#endif

    ZPOS64_T pos_in_zipfile;       /* position in byte on the zipfile, for fseek*/
    uLong stream_initialised;   /* flag set if stream structure is initialised*/

    ZPOS64_T offset_local_extrafield;/* offset of the local extra field */
    uInt  size_local_extrafield;/* size of the local extra field */
    ZPOS64_T pos_local_extrafield;   /* position in the local extra field in read*/
    ZPOS64_T total_out_64;

    uLong crc32;                /* crc32 of all data uncompressed */
    uLong crc32_wait;           /* crc32 we must obtain after decompress all */
    ZPOS64_T rest_read_compressed; /* number of byte to be decompressed */
    ZPOS64_T rest_read_uncompressed;/*number of byte to be obtained after decomp*/
    zlib_filefunc64_32_def z_filefunc;
    voidpf filestream;        /* io structure of the zipfile */
    uLong compression_method;   /* compression method (0==store) */
    ZPOS64_T byte_before_the_zipfile;/* byte before the zipfile, (>0 for sfx)*/
    int   raw;
} file_in_zip64_read_info_s;

/* unz64_s contain internal information about the zipfile
*/
typedef struct
{
    zlib_filefunc64_32_def z_filefunc;
    int is64bitOpenFunction;
    voidpf filestream;        /* io structure of the zipfile */
    unz_global_info64 gi;       /* public global information */
    ZPOS64_T byte_before_the_zipfile;/* byte before the zipfile, (>0 for sfx)*/
    ZPOS64_T num_file;             /* number of the current file in the zipfile*/
    ZPOS64_T pos_in_central_dir;   /* pos of the current file in the central dir*/
    ZPOS64_T current_file_ok;      /* flag about the usability of the current file*/
    ZPOS64_T central_pos;          /* position of the beginning of the central dir*/

    ZPOS64_T size_central_dir;     /* size of the central directory  */
    ZPOS64_T offset_central_dir;   /* offset of start of central directory with
                                   respect to the starting disk number */

    unz_file_info64 cur_file_info; /* public info about the current file in zip*/
    unz_file_info64_internal cur_file_info_internal; /* private info about it*/
    file_in_zip64_read_info_s* pfile_in_zip_read; /* structure about the current
                                        file if we are decompressing it */
    int encrypted;

    int isZip64;

#    ifndef NOUNCRYPT
    unsigned long keys[3];     /* keys defining the pseudo-random sequence */
    const z_crc_t* pcrc_32_tab;
#    endif
} unz64_s;

#ifdef __cplusplus
}
#endif
