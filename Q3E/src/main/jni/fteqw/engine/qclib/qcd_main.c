#include "progsint.h"
#include "qcc.h"

#if !defined(FTE_TARGET_WEB) && !defined(_XBOX)
#ifndef AVAIL_ZLIB
#define AVAIL_ZLIB
#endif
#endif

#ifdef NO_ZLIB
#undef AVAIL_ZLIB
#endif

#ifdef AVAIL_ZLIB
#include <zlib.h>
#endif

pbool QC_decodeMethodSupported(int method)
{
	if (method == 0)
		return true;
	if (method == 1)
		return true;
	if (method == 2)
	{
#ifdef AVAIL_ZLIB
		return false;
#endif
	}
	return false;
}


#ifdef ZLIB_DEFLATE64
#include "infback9.h"	//an obscure compile-your-own part of zlib.
struct def64ctx
{
	const char *in;
	char *out;
	size_t csize;
	size_t usize;

	char window[65536];
};
static unsigned int QC_Deflate64_Grab(void *vctx, unsigned char **bufptr)
{
	struct def64ctx *ctx = vctx;

	unsigned int avail = ctx->csize;
	*bufptr = (unsigned char *)ctx->in;
	ctx->csize = 0;
	ctx->in += avail;

	return avail;
}
static int QC_Deflate64_Spew(void *vctx, unsigned char *buf, unsigned int buflen)
{
	struct def64ctx *ctx = vctx;

	if (buflen > ctx->usize)
		return 1;	//over the size of our buffer...
	memcpy(ctx->out, buf, buflen);
	ctx->out += buflen;
	ctx->usize -= buflen;
	return 0;
}
#endif



char *QC_decode(progfuncs_t *progfuncs, int complen, int len, int method, const void *info, char *buffer)
{
	int i;
	if (method == 0)	//copy
	{
		if (complen != len) externs->Sys_Error("lengths do not match");
		memcpy(buffer, info, len);		
	}
	else if (method == 1)	//xor encryption
	{
		if (complen != len) externs->Sys_Error("lengths do not match");
		for (i = 0; i < len; i++)
			buffer[i] = ((const char*)info)[i] ^ 0xA5;
	}
#ifdef AVAIL_ZLIB
	else if (method == 2 || method == 8)	//compression (ZLIB)
	{
		z_stream strm = {
			(char*)info,
			complen,
			0,

			buffer,
			len,
			0,

			NULL,
			NULL,

			NULL,
			NULL,
			NULL,

			Z_BINARY,
			0,
			0
		};

		if (method == 8)
			inflateInit2(&strm, -MAX_WBITS);
		else
			inflateInit(&strm);
		if (Z_STREAM_END != inflate(&strm, Z_FINISH))	//decompress it in one go.
			externs->Sys_Error("Failed block decompression\n");
		inflateEnd(&strm);
	}
#endif
#ifdef ZLIB_DEFLATE64
	else if (method == 9)
	{
		z_stream strm = {NULL};
		struct def64ctx ctx;
		ctx.in = info;
		ctx.csize = complen;
		ctx.out = buffer;
		ctx.usize = len;

		strm.data_type = Z_UNKNOWN;
		inflateBack9Init(&strm, ctx.window);
		//getting inflateBack9 to
		if (Z_STREAM_END != inflateBack9(&strm, QC_Deflate64_Grab, &ctx, QC_Deflate64_Spew, &ctx))
		{	//some stream error?
			externs->Printf("Decompression error\n");
			buffer = NULL;
		}
		else if (ctx.csize != 0 || ctx.usize != 0)
		{	//corrupt file table?
			externs->Printf("Decompression size error\n");
			externs->Printf("read %i of %i bytes\n", (unsigned)ctx.csize, (unsigned)complen);
			externs->Printf("wrote %i of %i bytes\n", (unsigned)ctx.usize, (unsigned)len);
			buffer = NULL;
		}
		inflateBack9End(&strm);
		return buffer;
	}
#endif
	//add your decryption/decompression routine here.
	else
		externs->Sys_Error("Bad file encryption routine\n");


	return buffer;
}

#if !defined(MINIMAL) && !defined(OMIT_QCC)
int QC_encodecrc(int len, char *in)
{
#ifdef AVAIL_ZLIB
	return crc32(0, in, len);
#else
	return 0;
#endif
}
void SafeWrite(int hand, const void *buf, long count);
int SafeSeek(int hand, int ofs, int mode);
//we are allowed to trash our input here.
int QC_encode(progfuncs_t *progfuncs, int len, int method, const char *in, int handle)
{
	if (method == 0) //copy, allows a lame pass-through.
	{		
		SafeWrite(handle, in, len);
		return len;
	}
	/*else if (method == 1)	//xor encryption, not secure. maybe useful for the string table.
	{
		for (i = 0; i < len; i++)
			in[i] = in[i] ^ 0xA5;
		SafeWrite(handle, in, len);
		return len;
	}*/
	else if (method == 2 || method == 8)	//compression (ZLIB)
	{
#ifdef AVAIL_ZLIB
		char out[8192];
		int i=0;

		z_stream strm = {
			(char *)in,
			len,
			0,

			out,
			sizeof(out),
			0,

			NULL,
			NULL,

			NULL,
			NULL,
			NULL,

			Z_BINARY,
			0,
			0
		};

		if (method == 8)
			deflateInit2(&strm, 9, Z_DEFLATED, -MAX_WBITS, 9, Z_DEFAULT_STRATEGY);		//zip deflate compression
		else
			deflateInit(&strm, Z_BEST_COMPRESSION);	//zlib compression
		while(deflate(&strm, Z_FINISH) == Z_OK)
		{
			SafeWrite(handle, out, sizeof(out) - strm.avail_out);	//compress in chunks of 8192. Saves having to allocate a huge-mega-big buffer
			i+=sizeof(out) - strm.avail_out;
			strm.next_out = out;
			strm.avail_out = sizeof(out);
		}
		SafeWrite(handle, out, sizeof(out) - strm.avail_out);
		i+=sizeof(out) - strm.avail_out;
		deflateEnd(&strm);
		return i;
#endif
		externs->Sys_Error("ZLIB compression not supported in this build");
		return 0;
	}
	//add your compression/decryption routine here.
	else
	{
		externs->Sys_Error("Wierd method");
		return 0;
	}
}
#endif

static int QC_ReadRawInt(const unsigned char *blob)
{
	return (blob[0]<<0) | (blob[1]<<8) | (blob[2]<<16) | (blob[3]<<24);
}
static int QC_ReadRawShort(const unsigned char *blob)
{
	return (blob[0]<<0) | (blob[1]<<8);
}
int QC_EnumerateFilesFromBlob(const void *blob, size_t blobsize, void (*cb)(const char *name, const void *compdata, size_t compsize, int method, size_t plainsize))
{
	unsigned int cdentries;
	unsigned int cdlen;
	unsigned int cdstart;
	unsigned int zipoffset = 0;
	const unsigned char *eocd;
	const unsigned char *cd;
	unsigned int ofs_le;
	unsigned int cd_nl,cd_el,cd_cl;
	int ret = 0;

	const unsigned char *le;
	unsigned int csize, usize;
	int method; //negatives for errors.
	unsigned int le_nl,le_el;
	char name[256];

	const void *data;

	if (blobsize < 22)
		return ret;
	if (!strncmp(blob, "PACK", 4))
	{
		const packheader_t *head = blob;
		const packfile_t *f = (packfile_t*)((char*)blob + head->dirofs);
		for (ret = 0; ret < head->dirlen/sizeof(*f); ret++, f++)
		{
			cb(f->name, (const char*)blob+f->filepos, f->filelen, 0, f->filelen);
		}
		return ret;
	}

	//treat it as a zip (with no comment, too lazy to scan)
	eocd = blob;
	eocd += blobsize-22;
	for (cdlen = 0; ; eocd--, cdlen++)
	{
		if (cdlen > 65535 || eocd < (const unsigned char*)blob)
		{
			printf("No zip EOCD\n");
			return ret;
		}
		if (QC_ReadRawInt(eocd+0) == 0x06054b50)
			break;
	}

	if (QC_ReadRawShort(eocd+4) || QC_ReadRawShort(eocd+6) || QC_ReadRawShort(eocd+20)!=cdlen || QC_ReadRawShort(eocd+8) != QC_ReadRawShort(eocd+10))
	{		//this-disk					start-disk					comment-length			numfiles_thisdisk			numfiles_all
		printf("Unsupported zip\n");
		return ret;
	}
	cdstart = QC_ReadRawInt(eocd+16);
	cdlen = QC_ReadRawInt(eocd+12);
	cdentries = QC_ReadRawShort(eocd+10);
	cd = blob;
	cd += cdstart + zipoffset;
	if (cdlen < 46 || cd+cdlen>=(const unsigned char*)blob+blobsize || cd[0]!='P'||cd[1]!='K'||cd[2]!=1||cd[3]!=2)
	{	//cd looks corrupt? assume eocd starts right after the cd and use that as an offset at the start of the zip (concatenated onto a binary or w/e)
		zipoffset += (eocd-(const unsigned char*)blob) - (cdstart+cdlen);

		cd = blob;
		cd += cdstart + zipoffset;

		if (cdlen < 46 || zipoffset > blobsize || cd+cdlen>=(const unsigned char*)blob+blobsize || cd[0]!='P'||cd[1]!='K'||cd[2]!=1||cd[3]!=2)
		{
			printf("Zip CentralDir not found: %s\n", cd);
			return ret;
		}
		//okay, we're at an offset.
		printf("Zip offset is %u\n", zipoffset);
	}


	for(; cdentries --> 0 && (QC_ReadRawInt(cd+0) == 0x02014b50); cd += 46 + cd_nl+cd_el+cd_cl)
	{
		data = NULL, csize=usize=0, method=-1;

		cd_nl = QC_ReadRawShort(cd+28);	//name length
		cd_el = QC_ReadRawShort(cd+30);	//extras length
		cd_cl = QC_ReadRawShort(cd+32);	//comment length

		ofs_le = QC_ReadRawInt(cd+42);

		if (cd_nl < sizeof(name))	//make can't be too long...
			QC_strlcpy(name, cd+46, (cd_nl+1<sizeof(name))?cd_nl+1:sizeof(name));
		else
			QC_strlcpy(name, "?", sizeof(name));

		//1=encrypted
		//2,4=encoder flags
		//8=crc etc info is dodgy
		//10=enhanced deflate
		//20=patchdata
		//40=strong encryption
		//80,100,200,400=unused
		//800=utf-8
		//1000=enh comp
		//2000=masked localheader
		//4000,8000=reserved
		if (!(QC_ReadRawShort(cd+8) & ~0x80e))	//only accept known cd general purpose flags
		{
			if (ofs_le+46 < blobsize)
			{
				le = (const unsigned char*)blob + zipoffset + QC_ReadRawInt(cd+42);

				if (QC_ReadRawInt(le+0) == 0x04034b50)	//needs proper local entry tag
				if (!(QC_ReadRawShort(le+6) & ~0x80e))	//ignore unsupported general purpose flags
				{
					le_nl = QC_ReadRawShort(le+26);
					le_el = QC_ReadRawShort(le+28);
					if (cd_nl == le_nl)	//name (length) must match...
//					if (cd_el != le_el) //extras does NOT match
					{
						unsigned short gpflags = QC_ReadRawShort(le+6);
						if (gpflags & (1u<<3))
						{	//stream-compressed (csize+usize+crc are AFTER the data... just fall back to the central directory instead)
							csize = QC_ReadRawInt(cd+20);
							usize = QC_ReadRawInt(cd+24);
						}
						else
						{
							csize = QC_ReadRawInt(le+18);
							usize = QC_ReadRawInt(le+22);
						}

						//parse extra
						if (le_el)
						{
							const pbyte *extra = le + 30 + le_nl, *extraend = extra + le_el;
							unsigned short extrachunk_tag;
							unsigned short extrachunk_len;

							while(extra+4 < extraend)
							{
								extrachunk_tag = QC_ReadRawShort(extra+0);
								extrachunk_len = QC_ReadRawShort(extra+2);
								if (extra + extrachunk_len > extraend)
									break;	//error
								extra += 4;

								switch(extrachunk_tag)
								{
								case 1:	//zip64 extended information extra field. the attributes are only present if the reegular file info is nulled out with a -1
									if (usize == 0xffffffffu)
									{
										usize = QC_ReadRawInt/*64*/(extra);
										if (QC_ReadRawInt(extra+4))
											method=-1-method;
										extra += 8;
									}
									if (csize == 0xffffffffu)
									{
										csize = QC_ReadRawInt/*64*/(extra);
										if (QC_ReadRawInt(extra+4))
											method=-1-method;
										extra += 8;
									}
									break;
								default:
/*									printf("Unknown chunk %x\n", extrachunk_tag);
								case 0x000a:	//NTFS (timestamps)
								case 0x5455:	//extended timestamp
								case 0x7875:	//unix uid/gid
*/									extra += extrachunk_len;
									break;
								}
							}
						}

						data = le+30+le_nl+le_el;

						method = QC_ReadRawShort(le+8);
						if (method >= 0 && (method != 0
#ifdef AVAIL_ZLIB
							&& method != 8
#endif
#ifdef ZLIB_DEFLATE64
							&& method != 9
#endif
						 ))
							method=-1-method;
					}
				}
			}
		}
		cb(name, data, csize, method, usize);
		ret++;
	}
	return ret;
}

char *PDECL filefromprogs(pubprogfuncs_t *ppf, progsnum_t prnum, const char *fname, size_t *size, char *buffer)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	int num;
	includeddatafile_t *s;
	if (size)
		*size = 0;
	if (!pr_progstate[prnum].progs)
		return NULL;
	if (pr_progstate[prnum].progs->version != PROG_EXTENDEDVERSION)
		return NULL;
	if (pr_progstate[prnum].progs->secondaryversion != PROG_SECONDARYVERSION16 &&
		pr_progstate[prnum].progs->secondaryversion != PROG_SECONDARYVERSION32)
		return NULL;

	num = *(int*)((char *)pr_progstate[prnum].progs + pr_progstate[prnum].progs->ofsfiles);
	s = (includeddatafile_t *)((char *)pr_progstate[prnum].progs + pr_progstate[prnum].progs->ofsfiles+4);	
	while(num>0)
	{
		if (!strcmp(s->filename, fname))
		{
			if (size)
				*size = s->size;
			if (!buffer)
				return NULL;
			return QC_decode(progfuncs, s->compsize, s->size, s->compmethod, (char *)pr_progstate[prnum].progs+s->ofs, buffer);
		}

		s++;
		num--;
	}	

	if (size)
		*size = 0;
	return NULL;
}

/*
char *filefromnewprogs(progfuncs_t *progfuncs, const char *prname, const char *fname, int *size, char *buffer)
{
	int num;
	includeddatafile_t *s;	
	progstate_t progs;
	if (!PR_ReallyLoadProgs(progfuncs, prname, -1, &progs, false))
	{
		if (size)
			*size = 0;
		return NULL;
	}

	if (progs.progs->version < PROG_EXTENDEDVERSION)
		return NULL;
	if (!progs.progs->ofsfiles)
		return NULL;

	num = *(int*)((char *)progs.progs + progs.progs->ofsfiles);
	s = (includeddatafile_t *)((char *)progs.progs + progs.progs->ofsfiles+4);	
	while(num>0)
	{
		if (!strcmp(s->filename, fname))
		{
			if (size)
				*size = s->size;
			if (!buffer)
				return (char *)0xffffffff;
			return QC_decode(progfuncs, s->compsize, s->size, s->compmethod, (char *)progs.progs+s->ofs, buffer);
		}

		s++;
		num--;
	}	

	if (size)
		*size = 0;
	return NULL;
}
*/
