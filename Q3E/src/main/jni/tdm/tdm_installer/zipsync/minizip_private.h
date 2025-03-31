#pragma once

#include <minizip/unzip.h>
#include <minizip/zip.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

//stgatilov: Note that these data structures must be kept in sync with stuff defined in unzip.c and zip.c.
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



#ifndef Z_BUFSIZE
#define Z_BUFSIZE (64*1024) //(16384)
#endif

#define SIZEDATA_INDATABLOCK (4096-(4*4))

typedef struct linkedlist_datablock_internal_s
{
  struct linkedlist_datablock_internal_s* next_datablock;
  uLong  avail_in_this_block;
  uLong  filled_in_this_block;
  uLong  unused; /* for future use and alignment */
  unsigned char data[SIZEDATA_INDATABLOCK];
} linkedlist_datablock_internal;

typedef struct linkedlist_data_s
{
    linkedlist_datablock_internal* first_block;
    linkedlist_datablock_internal* last_block;
} linkedlist_data;

typedef struct
{
    z_stream stream;            /* zLib stream structure for inflate */
#ifdef HAVE_BZIP2
    bz_stream bstream;          /* bzLib stream structure for bziped */
#endif

    int  stream_initialised;    /* 1 is stream is initialised */
    uInt pos_in_buffered_data;  /* last written byte in buffered_data */

    ZPOS64_T pos_local_header;     /* offset of the local header of the file
                                     currently writing */
    char* central_header;       /* central header data for the current file */
    uLong size_centralExtra;
    uLong size_centralheader;   /* size of the central header for cur file */
    uLong size_centralExtraFree; /* Extra bytes allocated to the centralheader but that are not used */
    uLong flag;                 /* flag of the file currently writing */

    int  method;                /* compression method of file currently wr.*/
    int  raw;                   /* 1 for directly writing raw data */
    Byte buffered_data[Z_BUFSIZE];/* buffer contain compressed data to be writ*/
    uLong dosDate;
    uLong crc32;
    int  encrypt;
    int  zip64;               /* Add ZIP64 extended information in the extra field */
    ZPOS64_T pos_zip64extrainfo;
    ZPOS64_T totalCompressedData;
    ZPOS64_T totalUncompressedData;
#ifndef NOCRYPT
    unsigned long keys[3];     /* keys defining the pseudo-random sequence */
    const z_crc_t* pcrc_32_tab;
    unsigned crypt_header_size;
#endif
} curfile64_info;

typedef struct
{
    zlib_filefunc64_32_def z_filefunc;
    voidpf filestream;        /* io structure of the zipfile */
    linkedlist_data central_dir;/* datablock with central dir in construction*/
    int  in_opened_file_inzip;  /* 1 if a file in the zip is currently writ.*/
    curfile64_info ci;            /* info on the file currently writing */

    ZPOS64_T begin_pos;            /* position of the beginning of the zipfile */
    ZPOS64_T add_position_when_writing_offset;
    ZPOS64_T number_entry;

#ifndef NO_ADDFILEINEXISTINGZIP
    char *globalcomment;
#endif

} zip64_internal;


#ifdef __cplusplus
}
#endif
