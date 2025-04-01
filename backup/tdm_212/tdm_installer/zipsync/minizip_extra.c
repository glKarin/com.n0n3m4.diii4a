#include "minizip_extra.h"
#include <string.h>
#include "minizip_private.h"
#include "zlib.h"

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

extern int ZEXPORT unzIsZip64(unzFile file)
{
	unz64_s* s = (unz64_s*)file;
    if (s == NULL)
        return 0;
    return s->isZip64;
}

extern int ZEXPORT minizipCopyDataRaw(unzFile srcHandle, zipFile dstHandle, voidp buffer, unsigned bufSize)
{
	unz64_s* src = (unz64_s*)srcHandle;
    zip64_internal* dst = (zip64_internal*)dstHandle;

    if (dst == NULL)
        return ZIP_PARAMERROR;
    if (dst->in_opened_file_inzip == 0)
        return ZIP_PARAMERROR;
    if (src == NULL)
        return UNZ_PARAMERROR;
    if (src->pfile_in_zip_read == NULL)
        return UNZ_PARAMERROR;

    curfile64_info *dstFile = &dst->ci;
    file_in_zip64_read_info_s *srcFile = src->pfile_in_zip_read;

    if (srcFile->read_buffer == NULL)
        return UNZ_END_OF_LIST_OF_FILE;
    if (!srcFile->raw)
        return UNZ_PARAMERROR;
    if (!dstFile->raw)
        return UNZ_PARAMERROR;

    if (dstFile->pos_in_buffered_data != 0)
        return UNZ_PARAMERROR;  //buffer must be empty, since we don't know how to flush it

    if (ZSEEK64(srcFile->z_filefunc, srcFile->filestream, srcFile->pos_in_zipfile + srcFile->byte_before_the_zipfile, ZLIB_FILEFUNC_SEEK_SET) != 0)
        return UNZ_ERRNO;
    while (srcFile->rest_read_compressed > 0) {
        uInt numBytesToCopy = bufSize;
        if (numBytesToCopy > srcFile->rest_read_compressed)
            numBytesToCopy = srcFile->rest_read_compressed;
        if (ZREAD64(srcFile->z_filefunc, srcFile->filestream, buffer, numBytesToCopy) != numBytesToCopy)
            return UNZ_ERRNO;
        if (ZWRITE64(dst->z_filefunc, dst->filestream, buffer, numBytesToCopy) != numBytesToCopy)
            return ZIP_ERRNO;
        dstFile->totalCompressedData += numBytesToCopy;
        dstFile->totalUncompressedData += numBytesToCopy;
        srcFile->pos_in_zipfile += numBytesToCopy;
        srcFile->rest_read_compressed -= numBytesToCopy;
    }

    return UNZ_OK;
}

extern int ZEXPORT zipForceDataType(zipFile file, uLong internalAttrib)
{
    zip64_internal* zi = (zip64_internal*)file;

    if (zi == NULL)
        return ZIP_PARAMERROR;
    if (zi->in_opened_file_inzip == 0)
        return ZIP_PARAMERROR;

    curfile64_info *ci = &zi->ci;
    if (!ci)
        return ZIP_PARAMERROR;

    ci->stream.data_type = (internalAttrib & 1 ? Z_ASCII : Z_BINARY);

    return ZIP_OK;
}
