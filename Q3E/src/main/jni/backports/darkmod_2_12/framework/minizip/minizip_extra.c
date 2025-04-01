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
#include "minizip_extra.h"
#include <string.h>
#include "minizip_private.h"

extern unzFile unzReOpen (const char* path, unzFile file)
{
	unz64_s* s;
	unz64_s* zFile = (unz64_s*)file;

	if(zFile == NULL)
		return NULL;

	// create unz64_s* "s" as clone of "file"
	s=(unz64_s*)ALLOC(sizeof(unz64_s));
	if(s == NULL)
		return NULL;

	memcpy(s, zFile, sizeof(unz64_s));

	// create new filestream for path
	voidp fin = ZOPEN64(s->z_filefunc,
						path,
						ZLIB_FILEFUNC_MODE_READ | ZLIB_FILEFUNC_MODE_EXISTING);

	if( fin == NULL ) {
		TRYFREE(s);
		return NULL;
	}

	// set that filestream in s
	s->filestream = fin;

	unzOpenCurrentFile( s );

	return (unzFile)s;
}

extern int ZEXPORT unzseek(unzFile file, z_off_t offset, int origin)
{
	return unzseek64(file, (ZPOS64_T)offset, origin);
}

extern int ZEXPORT unzseek64(unzFile file, ZPOS64_T offset, int origin)
{
	unz64_s* s;
	file_in_zip64_read_info_s* pfile_in_zip_read_info;
	ZPOS64_T stream_pos_begin;
	ZPOS64_T stream_pos_end;
	int isWithinBuffer;
	ZPOS64_T position;

	if (file == NULL)
		return UNZ_PARAMERROR;

	s = (unz64_s*)file;
	pfile_in_zip_read_info = s->pfile_in_zip_read;

	if (pfile_in_zip_read_info == NULL)
		return UNZ_ERRNO;

	if (pfile_in_zip_read_info->compression_method != 0)
		return UNZ_ERRNO;

	if (origin == SEEK_SET)
		position = offset;
	else if (origin == SEEK_CUR)
		position = pfile_in_zip_read_info->total_out_64 + offset;
	else if (origin == SEEK_END)
		position = s->cur_file_info.compressed_size + offset;
	else
		return UNZ_PARAMERROR;

	if (position > s->cur_file_info.compressed_size)
		return UNZ_PARAMERROR;

	stream_pos_end = pfile_in_zip_read_info->pos_in_zipfile;
	stream_pos_begin = stream_pos_end;
	if (stream_pos_begin > UNZ_BUFSIZE)
		stream_pos_begin -= UNZ_BUFSIZE;
	else
		stream_pos_begin = 0;

	isWithinBuffer = pfile_in_zip_read_info->stream.avail_in != 0 &&
		(pfile_in_zip_read_info->rest_read_compressed != 0 || s->cur_file_info.compressed_size < UNZ_BUFSIZE) &&
		position >= stream_pos_begin && position < stream_pos_end;

	if (isWithinBuffer)
	{
		pfile_in_zip_read_info->stream.next_in += position - pfile_in_zip_read_info->total_out_64;
		pfile_in_zip_read_info->stream.avail_in = (uInt)(stream_pos_end - position);
	}
	else
	{
		pfile_in_zip_read_info->stream.avail_in = 0;
		pfile_in_zip_read_info->stream.next_in = 0;

		pfile_in_zip_read_info->pos_in_zipfile = pfile_in_zip_read_info->offset_local_extrafield + position;
		pfile_in_zip_read_info->rest_read_compressed = s->cur_file_info.compressed_size - position;
	}

	pfile_in_zip_read_info->rest_read_uncompressed -= (position - pfile_in_zip_read_info->total_out_64);
	pfile_in_zip_read_info->stream.total_out = (uLong)position;
	pfile_in_zip_read_info->total_out_64 = position;

	return UNZ_OK;
}
