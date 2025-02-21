#include "quakedef.h"
#include "fs.h"

//#define AVAIL_BZLIB
//#define DYNAMIC_BZLIB



//supported ZIP features:
//zip64 for huge zips
//utf-8 encoding support (non-utf-8 is always ibm437)
//symlink flag
//compression mode: store
//compression mode: deflate (via zlib)
//compression mode: deflate64 (via zlib's unofficial extras)
//compression mode: bzip2 (via libbz2)
//bigendian cpus. everything misaligned.
//weak/original file encryption (set fs_zip_password)
//split archives (see specific notes)

//NOT supported:
//other compression modes
//strong/aes encryption
//if central dir is crypted/compressed, the archive will fail to open
//crc verification (if present then the local header must match the central dir, but the file data itself will not be verified in part because that'd require decoding the entire file upfront first)
//infozip utf-8 name override.
//other 'extra' fields.

//split archives:
//the central directory must be stored exclusively inside the initial zip/pk3 file.
//additional volumes will use the first letter of the extension and then at least two base-10 digits denoting the volume index (eg pak0.pk3 + pak0.p00 + pak0.p01).
//individual files must not be split over files (no rollover on truncation)
//fteqcc has a feature to generate versioned centraldir-only zips using external volumes for the actual data. This can be used to avoid downloading redundant data when connecting to servers using older revisions, but such revisions must be centrally managed.



#ifdef AVAIL_BZLIB
#	include <bzlib.h>
#	ifdef DYNAMIC_BZLIB
#	ifndef WINAPI
#		define WINAPI
#	endif
#	define BZ2_bzDecompressInit pBZ2_bzDecompressInit
#	define BZ2_bzDecompress pBZ2_bzDecompress
#	define BZ2_bzDecompressEnd pBZ2_bzDecompressEnd
	static int (WINAPI *BZ2_bzDecompressInit)(bz_stream *strm, int verbosity, int small);
	static int (WINAPI *BZ2_bzDecompress)(bz_stream* strm);
	static int (WINAPI *BZ2_bzDecompressEnd)(bz_stream *strm);
	static qboolean BZLIB_LOADED(void)
	{
		static qboolean tried;
		static void *handle;
		if (!tried)
		{
			static dllfunction_t funcs[] =
			{
				{(void*)&BZ2_bzDecompressInit,	"BZ2_bzDecompressInit"},
				{(void*)&BZ2_bzDecompress,		"BZ2_bzDecompress"},
				{(void*)&BZ2_bzDecompressEnd,	"BZ2_bzDecompressEnd"},
				{NULL, NULL}
			};
			tried = true;
			handle = Sys_LoadLibrary("libbz2", funcs);
		}
		return handle != NULL;
	}
#	else
#		define BZLIB_LOADED() 1
#	endif
#endif

#ifdef AVAIL_ZLIB
# define ZIPCRYPT

# ifndef ZEXPORT
	#define ZEXPORT VARGS
# endif
# include <zlib.h>

# ifdef DYNAMIC_ZLIB
#  define ZLIB_LOADED() (zlib_handle!=NULL)
void *zlib_handle;
#  define ZSTATIC(n)
# else
#  define ZLIB_LOADED() 1
#  define ZSTATIC(n) = &n

# endif

#ifndef Z_U4
#define z_crc_t uLongf
#endif

//#pragma comment(lib, MSVCLIBSPATH "zlib.lib")

#ifdef DYNAMIC_ZLIB
	static int (ZEXPORT *qinflateEnd) (z_streamp strm) ZSTATIC(inflateEnd);
	static int (ZEXPORT *qinflate) (z_streamp strm, int flush) ZSTATIC(inflate);
	static int (ZEXPORT *qinflateInit2_) (z_streamp strm, int  windowBits,
										  const char *version, int stream_size) ZSTATIC(inflateInit2_);
	//static z_crc_t (ZEXPORT *qcrc32)   (uLong crc, const Bytef *buf, uInt len) ZSTATIC(crc32);
	#ifdef ZIPCRYPT
	static const z_crc_t *(ZEXPORT *qget_crc_table)   (void) ZSTATIC(get_crc_table);
	#endif
#else
	#define qinflateEnd		inflateEnd
	#define qinflate		inflate
	#define qinflateInit2_	inflateInit2_
	#define qdeflateEnd		deflateEnd
	#define qdeflate		deflate
	#define qdeflateInit2_	deflateInit2_
	#define qget_crc_table	get_crc_table
#endif

#define qinflateInit2(strm, windowBits) \
        qinflateInit2_((strm), (windowBits), ZLIB_VERSION, sizeof(z_stream))
#define qdeflateInit2(strm, level, method, windowBits, memLevel, strategy) \
        qdeflateInit2_((strm),(level),(method),(windowBits),(memLevel),\
                      (strategy),           ZLIB_VERSION, sizeof(z_stream))

qboolean LibZ_Init(void)
{
	#ifdef DYNAMIC_ZLIB
	static dllfunction_t funcs[] =
	{
		{(void*)&qinflateEnd,		"inflateEnd"},
		{(void*)&qinflate,			"inflate"},
		{(void*)&qinflateInit2_,	"inflateInit2_"},
		{(void*)&qdeflateEnd,		"deflateEnd"},
		{(void*)&qdeflate,			"deflate"},
		{(void*)&qdeflateInit2_,	"deflateInit2_"},
//		{(void*)&qcrc32,			"crc32"},
#ifdef ZIPCRYPT
		{(void*)&qget_crc_table,	"get_crc_table"},
#endif
		{NULL, NULL}
	};
	if (!ZLIB_LOADED())
		zlib_handle = Sys_LoadLibrary("zlib1", funcs);
	#endif
	return ZLIB_LOADED();
}

#ifdef AVAIL_GZDEC
typedef struct {
	unsigned char ident1;
	unsigned char ident2;
	unsigned char cm;
	unsigned char flags;
	unsigned int mtime;
	unsigned char xflags;
	unsigned char os;
	//unsigned short xlen;
	//unsigned char xdata[xlen];
	//unsigned char fname[];
	//unsigned char fcomment[];
	//unsigned short fhcrc;
	//unsigned char compresseddata[];
	//unsigned int crc32;
	//unsigned int isize;
} gzheader_t;
#define sizeofgzheader_t 10

#define	GZ_FTEXT	1
#define	GZ_FHCRC	2
#define GZ_FEXTRA	4
#define GZ_FNAME	8
#define GZ_FCOMMENT	16
#define GZ_RESERVED (32|64|128)
//outfile may be null
vfsfile_t *FS_DecompressGZip(vfsfile_t *infile, vfsfile_t *outfile)
{
	char inchar;
	unsigned short inshort;
	vfsfile_t *temp;
	gzheader_t header;

	if (VFS_READ(infile, &header, sizeofgzheader_t) == sizeofgzheader_t)
	{
		if (header.ident1 != 0x1f || header.ident2 != 0x8b || header.cm != 8 || header.flags & GZ_RESERVED)
		{
			VFS_SEEK(infile, 0);
			return infile;
		}
	}
	else
	{
		VFS_SEEK(infile, 0);
		return infile;
	}
	if (header.flags & GZ_FEXTRA)
	{
		VFS_READ(infile, &inshort, sizeof(inshort));
		inshort = LittleShort(inshort);
		VFS_SEEK(infile, VFS_TELL(infile) + inshort);
	}

	if (header.flags & GZ_FNAME)
	{
		Con_Printf("gzipped file name: ");
		do {
			if (VFS_READ(infile, &inchar, sizeof(inchar)) != 1)
				break;
			Con_Printf("%c", inchar);
		} while(inchar);
		Con_Printf("\n");
	}

	if (header.flags & GZ_FCOMMENT)
	{
		Con_Printf("gzipped file comment: ");
		do {
			if (VFS_READ(infile, &inchar, sizeof(inchar)) != 1)
				break;
			Con_Printf("%c", inchar);
		} while(inchar);
		Con_Printf("\n");
	}

	if (header.flags & GZ_FHCRC)
	{
		VFS_READ(infile, &inshort, sizeof(inshort));
	}



	if (outfile)
		temp = outfile;
	else
	{
		temp = FS_OpenTemp();
		if (!temp)
		{
			VFS_SEEK(infile, 0);	//doh
			return infile;
		}
	}


	{
		unsigned char inbuffer[16384];
		unsigned char outbuffer[16384];
		int ret;

		z_stream strm = {
			inbuffer,
			0,
			0,

			outbuffer,
			sizeof(outbuffer),
			0,

			NULL,
			NULL,

			NULL,
			NULL,
			NULL,

			Z_UNKNOWN,
			0,
			0
		};

		strm.avail_in = VFS_READ(infile, inbuffer, sizeof(inbuffer));
		strm.next_in = inbuffer;

		qinflateInit2(&strm, -MAX_WBITS);

		while ((ret=qinflate(&strm, Z_SYNC_FLUSH)) != Z_STREAM_END)
		{
			if (strm.avail_in == 0 || strm.avail_out == 0)
			{
				if (strm.avail_in == 0)
				{
					strm.avail_in = VFS_READ(infile, inbuffer, sizeof(inbuffer));
					strm.next_in = inbuffer;
					if (!strm.avail_in)
						break;
				}

				if (strm.avail_out == 0)
				{
					strm.next_out = outbuffer;
					VFS_WRITE(temp, outbuffer, strm.total_out);
					strm.total_out = 0;
					strm.avail_out = sizeof(outbuffer);
				}
				continue;
			}

			//doh, it terminated for no reason
			if (ret != Z_STREAM_END)
			{
				qinflateEnd(&strm);
				Con_Printf("Couldn't decompress gz file\n");
				VFS_CLOSE(temp);
				VFS_CLOSE(infile);
				return NULL;
			}
		}
		//we got to the end
		VFS_WRITE(temp, outbuffer, strm.total_out);

		qinflateEnd(&strm);

		if (temp->Seek)
			VFS_SEEK(temp, 0);
	}
	VFS_CLOSE(infile);

	return temp;
}


size_t ZLib_CompressBuffer(const qbyte *in, size_t insize, qbyte *out, size_t maxoutsize)
{	//compresses... returns 0 if the data would have grown.
	z_stream strm = {
		(qbyte*)in,
		insize,
		0,

		out,
		maxoutsize,
		0,

		NULL,
		NULL,

		NULL,
		NULL,
		NULL,

		Z_UNKNOWN,
		0,
		0
	};

	qdeflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	if (qdeflate(&strm, Z_FINISH) != Z_STREAM_END)
		strm.total_out = 0; //some sort of failure. probably needs more output buffer
	qdeflateEnd(&strm);

	return strm.total_out;
}
size_t ZLib_DecompressBuffer(const qbyte *in, size_t insize, qbyte *out, size_t maxoutsize)
{
	int ret;

	z_stream strm = {
		(qbyte*)in,
		insize,
		0,

		out,
		maxoutsize,
		0,

		NULL,
		NULL,

		NULL,
		NULL,
		NULL,

		Z_UNKNOWN,
		0,
		0
	};

	qinflateInit2(&strm, MAX_WBITS);

	while ((ret=qinflate(&strm, Z_SYNC_FLUSH)) != Z_STREAM_END)
	{
		if (strm.avail_in == 0 || strm.avail_out == 0)
		{
			if (strm.avail_in == 0)
				break;	//reached the end of the input
			//output not big enough, but max size is fixed.
			break;
		}

		//doh, it terminated for no reason
		if (ret != Z_STREAM_END)
			break;
	}
	qinflateEnd(&strm);

	return strm.total_out;
}








//returns an unseekable write-only file that will decompress any data that is written to it.
//decompressed data will be written to the output
typedef struct
{
	vfsfile_t vf;
	vfsfile_t *outfile;
	qboolean autoclosefile;
	qboolean compress;

	//gzip header handling
	qboolean headerparsed;
	qbyte in[65536];
	int inlen;

	qbyte out[65536];
	z_stream strm;
} vf_gz_dec_t;

static qboolean QDECL FS_GZ_Dec_Close(vfsfile_t *f)
{
	vf_gz_dec_t *n = (vf_gz_dec_t*)f;

	if (n->compress)
		qdeflate(&n->strm, Z_FINISH);
	else
		qinflate(&n->strm, Z_FINISH);
	if (n->strm.next_out != n->out)
		VFS_WRITE(n->outfile, n->out, n->strm.next_out-n->out);

	if (n->compress)
		qdeflateEnd(&n->strm);
	else
		qinflateEnd(&n->strm);

	if (n->autoclosefile)
		VFS_CLOSE(n->outfile);
	Z_Free(n);
	return true;
}
static int QDECL FS_GZ_Dec_Write(vfsfile_t *f, const void *buffer, int len)
{
	int ret;
	vf_gz_dec_t *n = (vf_gz_dec_t*)f;

	if (n->headerparsed != 1)
	{
		qofs_t ofs = 0;
		gzheader_t *header;
		int chunk;
		if (len > sizeof(n->in) - n->inlen)
			chunk = sizeof(n->in) - n->inlen;
		else
			chunk = len;
		memmove(n->in, buffer, chunk);
		n->inlen += chunk;

		if (n->headerparsed == 2)
		{
			if (n->inlen >= 8)
			{
				//unsigned int crc = (n->in[0]<<0) | (n->in[1]<<8) | (n->in[2]<<16) | (n->in[3]<<24);
				unsigned int isize = (n->in[4]<<0) | (n->in[5]<<8) | (n->in[6]<<16) | (n->in[7]<<24);
				if (n->strm.total_out != isize)
					return -1;	//the file we just received decoded to a different length (yay, concat...).
				//FIXME: validate the decoded crc
				n->headerparsed = false;
				n->strm.total_in = 0;
				n->strm.total_out = 0;
				len = n->inlen - 8;
				n->inlen = 0;
				if (FS_GZ_Dec_Write(f, n->in + 8, len) != len)
					return -1;	//FIXME: make non-fatal somehow
				return chunk;
			}
			return chunk;
		}

		header = (gzheader_t*)n->in;
		ofs += sizeofgzheader_t;  if (ofs > n->inlen) goto noheader;

		if (header->ident1 != 0x1f || header->ident2 != 0x8b || header->cm != 8 || header->flags & GZ_RESERVED)
			return -1;	//ERROR

		if (header->flags & GZ_FEXTRA)
		{
			unsigned short ex;
			if (ofs+2 > n->inlen) goto noheader;
			ex = (n->in[ofs]<<0) | (n->in[ofs+1]<<8);	//read little endian
			ofs += 2+ex;  if (ofs > n->inlen) goto noheader;
		}

		if (header->flags & GZ_FNAME)
		{
			char ch;
//			Con_Printf("gzipped file name: ");
			do {
				if (ofs+1 > n->inlen) goto noheader;
				ch = n->in[ofs++];
//				Con_Printf("%c", ch);
			} while(ch);
//			Con_Printf("\n");
		}

		if (header->flags & GZ_FCOMMENT)
		{
			char ch;
//			Con_Printf("gzipped file comment: ");
			do {
				if (ofs+1 > n->inlen) goto noheader;
				ch = n->in[ofs++];
//				Con_Printf("%c", ch);
			} while(ch);
//			Con_Printf("\n");
		}

		if (header->flags & GZ_FHCRC)
		{
			ofs += 2;  if (ofs > n->inlen) goto noheader;
		}

		//if we got here then the header is now complete.
		//tail-recurse it.
		n->headerparsed = true;
		chunk = n->inlen - ofs;
		if (FS_GZ_Dec_Write(f, n->in + ofs, chunk) != chunk)
			return -1;	//FIXME: make non-fatal somehow
		return len;

noheader:
		if (n->inlen == sizeof(n->in))
			return -1;	//we filled our buffer. if we didn't get past the header yet then someone fucked up.
		return len;
	}


	n->strm.next_in = (char*)buffer;
	n->strm.avail_in = len;

	while(n->strm.avail_in)
	{
		if (n->compress)
			ret = qdeflate(&n->strm, Z_SYNC_FLUSH);
		else
			ret = qinflate(&n->strm, Z_SYNC_FLUSH);

		if (!n->strm.avail_out)
		{
			if (VFS_WRITE(n->outfile, n->out, n->strm.next_out-n->out) != n->strm.next_out-n->out)
				return -1;

			n->strm.next_out = n->out;
			n->strm.avail_out = sizeof(n->out);
		}

		if (ret == Z_OK)
			continue;

		//flush it
		if (VFS_WRITE(n->outfile, n->out, n->strm.next_out-n->out) != n->strm.next_out-n->out)
			return -1;
		n->strm.next_out = n->out;
		n->strm.avail_out = sizeof(n->out);

		if (ret == Z_STREAM_END)	//allow concat
		{
			int l = n->strm.avail_in;
			n->headerparsed = 2;
			n->inlen = 0;
			if (l != FS_GZ_Dec_Write(f, n->strm.next_in, n->strm.avail_in))
				return -1;
			return len;
		}

		switch (ret)
		{
		case Z_NEED_DICT:
			Con_Printf("Z_NEED_DICT\n");
			break;

		case Z_ERRNO:
			Con_Printf("Z_ERRNO\n");
			break;

		case Z_STREAM_ERROR:
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
		case Z_BUF_ERROR:
		case Z_VERSION_ERROR:
			Con_Printf("File is corrupt\n");
			break;
		default:
			Con_Printf("Bug!\n");
			break;
		}
		return -1;
	}

	return len;
}

vfsfile_t *FS_GZ_WriteFilter(vfsfile_t *outfile, qboolean autoclosefile, qboolean compress)
{
	vf_gz_dec_t *n = Z_Malloc(sizeof(*n));

	n->outfile = outfile;
	n->autoclosefile = autoclosefile;
	n->compress = compress;

	n->strm.next_in		= NULL;
	n->strm.avail_in 	= 0;
	n->strm.total_in	= 0;
	n->strm.next_out	= n->out;
	n->strm.avail_out 	= sizeof(n->out);
	n->strm.total_out	= 0;

	n->vf.Flush				= NULL;
	n->vf.GetLen			= NULL;
	n->vf.ReadBytes			= NULL;
	n->vf.Seek				= NULL;
	n->vf.Tell				= NULL;
	n->vf.Close				= FS_GZ_Dec_Close;
	n->vf.WriteBytes		= FS_GZ_Dec_Write;
	n->vf.seekstyle			= SS_UNSEEKABLE;

	if (n->compress)
	{
		n->headerparsed = true;	//deflate will write out a minimal header for us, so we can just splurge everything without rewinding.
		qdeflateInit2(&n->strm, Z_BEST_COMPRESSION, Z_DEFLATED, MAX_WBITS|16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	}
	else
		qinflateInit2(&n->strm, -MAX_WBITS);

	return &n->vf;
}

#endif
#endif





#ifdef PACKAGE_PK3

typedef struct
{
	fsbucket_t bucket;
	char	name[MAX_QPATH];
	unsigned int disknum;
	qofs_t	localpos;	//location of local header
	qofs_t	filelen;	//uncompressed size
	time_t	mtime;
	unsigned int		crc;
	unsigned int		flags;
} zpackfile_t;
#define ZFL_DEFLATED	(1u<<0)	//need to use zlib
#define ZFL_BZIP2		(1u<<1)	//bzip2 compression
#define ZFL_STORED		(1u<<2)	//direct access is okay
#define ZFL_SYMLINK		(1u<<3)	//file is a symlink
#define ZFL_CORRUPT		(1u<<4)	//file is corrupt or otherwise unreadable (usually just means we don't support reading it rather than actually corrupt, but hey).
#define ZFL_WEAKENCRYPT	(1u<<5)	//traditional zip encryption
#define ZFL_DEFLATE64D	(1u<<6)	//need to use zlib's 'inflateBack9' stuff.

#define ZFL_COMPRESSIONTYPE (ZFL_STORED|ZFL_CORRUPT|ZFL_DEFLATED|ZFL_DEFLATE64D|ZFL_BZIP2)


typedef struct zipfile_s
{
	searchpathfuncs_t pub;

	char			filename[MAX_OSPATH];
	unsigned int	numfiles;
	zpackfile_t		*files;

	//spanned zips are weird.
	//the starting zip is usually the last one, but should be null to simplify ref tracking.
	unsigned int	thisdisk;
	unsigned int	numspans;
	struct zipfile_s **spans;

	//info about the underlying file
	void			*mutex;
	qofs_t			curpos;	//cache position to avoid excess seeks
	qofs_t			rawsize;
	vfsfile_t		*raw;

	qatomic32_t		references;	//number of files open inside, so things don't crash if is closed in the wrong order.
} zipfile_t;


static void QDECL FSZIP_GetPathDetails(searchpathfuncs_t *handle, char *out, size_t outlen)
{
	zipfile_t *zip = (void*)handle;

	if (zip->references != 1)
		Q_snprintfz(out, outlen, "(%i)", zip->references-1);
	else
		*out = '\0';
}
static void QDECL FSZIP_UnclosePath(searchpathfuncs_t *handle)
{
	zipfile_t *zip = (void*)handle;
	FTE_Atomic32_Inc(&zip->references);
}
static void QDECL FSZIP_ClosePath(searchpathfuncs_t *handle)
{
	size_t s;
	qboolean stillopen;
	zipfile_t *zip = (void*)handle;

	stillopen = FTE_Atomic32_Dec(&zip->references) > 0;
	if (stillopen)
		return;	//not free yet

	VFS_CLOSE(zip->raw);
	for (s = 0; s < zip->numspans; s++)
	{
		if (zip->spans[s])
		{
			zip->spans[s]->pub.ClosePath(&zip->spans[s]->pub);
			zip->spans[s] = NULL;
		}
	}
	Z_Free(zip->spans);
	Sys_DestroyMutex(zip->mutex);
	if (zip->files)
		Z_Free(zip->files);
	Z_Free(zip);
}
static void QDECL FSZIP_BuildHash(searchpathfuncs_t *handle, int depth, void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle))
{
	zipfile_t *zip = (void*)handle;
	int i;

	for (i = 0; i < zip->numfiles; i++)
	{
		if (zip->files[i].flags & ZFL_CORRUPT)
			continue;
		AddFileHash(depth, zip->files[i].name, &zip->files[i].bucket, &zip->files[i]);
	}
}
static unsigned int QDECL FSZIP_FLocate(searchpathfuncs_t *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	zpackfile_t *pf = hashedresult;
	int i;
	zipfile_t	*zip = (void*)handle;
	int ret = FF_NOTFOUND;

// look through all the pak file elements

	if (pf)
	{	//is this a pointer to a file in this pak?
		if (pf < zip->files || pf >= zip->files + zip->numfiles)
			return FF_NOTFOUND;	//was found in a different path
	}
	else
	{
		for (i=0 ; i<zip->numfiles ; i++)	//look for the file
		{
			if (!Q_strcasecmp (zip->files[i].name, filename))
			{
				if (zip->files[i].flags & ZFL_CORRUPT)
					continue;
				pf = &zip->files[i];
				break;
			}
		}
	}

	if (pf)
	{
		ret = FF_FOUND;
		if (loc)
		{
			loc->fhandle = pf;
			*loc->rawname = 0;
			loc->offset = (qofs_t)-1;
			loc->len = pf->filelen;

			if (pf->flags & ZFL_SYMLINK)
				ret = FF_SYMLINK;

	//		if (unzLocateFileMy (zip->handle, loc->index, zip->files[loc->index].filepos) == 2)
	//			ret = FF_SYMLINK;
	//		loc->offset = unzGetCurrentFileUncompressedPos(zip->handle);
//			if (loc->offset<0)
//			{	//file not found, or is compressed.
//				*loc->rawname = '\0';
//				loc->offset=0;
//			}
		}
		else
			ret = FF_FOUND;
		return ret;
	}
	return FF_NOTFOUND;
}

static vfsfile_t *QDECL FSZIP_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode);
static void QDECL FSZIP_ReadFile(searchpathfuncs_t *handle, flocation_t *loc, char *buffer)
{
	vfsfile_t *f;
	f = FSZIP_OpenVFS(handle, loc, "rb");
	if (!f)	//err...
		return;
	VFS_READ(f, buffer, loc->len);
	VFS_CLOSE(f);
}

static int QDECL FSZIP_EnumerateFiles (searchpathfuncs_t *handle, const char *match, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *spath), void *parm)
{
	zipfile_t *zip = (void*)handle;
	int		num;

	for (num = 0; num<(int)zip->numfiles; num++)
	{
		if (wildcmp(match, zip->files[num].name))
		{
			if (!func(zip->files[num].name, zip->files[num].filelen, zip->files[num].mtime, parm, &zip->pub))
				return false;
		}
	}

	return true;
}

static int QDECL FSZIP_GeneratePureCRC(searchpathfuncs_t *handle, const int *seed)
{
	zipfile_t *zip = (void*)handle;

	int result;
	int *filecrcs;
	int numcrcs=0;
	int i;

	filecrcs = BZ_Malloc((zip->numfiles+1)*sizeof(int));
	if (seed)
		filecrcs[numcrcs++] = *seed;

	for (i = 0; i < zip->numfiles; i++)
	{
		if (zip->files[i].filelen>0)
			filecrcs[numcrcs++] = zip->files[i].crc;
	}

	result = CalcHashInt(&hash_md4, filecrcs, numcrcs*sizeof(int));

	BZ_Free(filecrcs);
	return result;
}

struct decompressstate
{
	struct decompressstate *(*Reinit)(zipfile_t *source, qofs_t start, qofs_t csize, qofs_t usize, char *filename, char *password, unsigned int crc);
	qofs_t (*Read)(struct decompressstate *st, qbyte *buffer, qofs_t bytes);
	qboolean (*Seek)(struct decompressstate *st, qofs_t *fileofs, qofs_t newofs);	//may fail, file will be fully decompressed.
	void (*Destroy)(struct decompressstate *st);

	zipfile_t *source;
	qofs_t cstart;	//position of start of data
	qofs_t cofs;	//current read offset
	qofs_t cend;	//compressed data ends at this point.
	qofs_t usize;	//data remaining. refuse to read more bytes than this.
	unsigned char inbuffer[16384];
	unsigned char outbuffer[16384];
	unsigned int readoffset;

#ifdef ZIPCRYPT
	qboolean encrypted;
	unsigned int cryptkey[3];
	unsigned int initialkey[3];
	const z_crc_t * crctable;
#endif

#ifdef AVAIL_ZLIB
	z_stream strm;
#endif
#ifdef AVAIL_BZLIB
	bz_stream bstrm;
#endif
};

#if defined(AVAIL_ZLIB) || defined(AVAIL_BZLIB)
#define DO_ZIP_DECOMPRESS

#ifdef ZIPCRYPT
#define CRC32(c, b) ((*(st->crctable+(((int)(c) ^ (b)) & 0xff))) ^ ((c) >> 8))

static void FSZIP_UpdateKeys(struct decompressstate *st, unsigned char ch)
{
	st->cryptkey[0] = CRC32(st->cryptkey[0], ch);
	st->cryptkey[1] += (st->cryptkey[0] & 0xffu);
	st->cryptkey[1] = st->cryptkey[1] * 0x8088405u + 1;
	ch = st->cryptkey[1] >> 24;
	st->cryptkey[2] = CRC32(st->cryptkey[2], ch);
}
static unsigned char FSZIP_DecryptByte(struct decompressstate *st)
{
	unsigned int temp;
	temp = (st->cryptkey[2]&0xffff) | 2;
	return ((temp * (temp ^ 1)) >> 8) & 0xff;
}

static qboolean FSZIP_SetupCrytoKeys(struct decompressstate *st, const char *password, char *cryptheader, unsigned int crc)
{
	unsigned int u;
	st->crctable = qget_crc_table();
	st->encrypted = true;
	st->cryptkey[0] = 0x12345678;
	st->cryptkey[1] = 0x23456789;
	st->cryptkey[2] = 0x34567890;
	while (*password)
		FSZIP_UpdateKeys(st, *password++);

	for (u = 0; u < 12; u++)
	{
		unsigned char ch = cryptheader[u] ^ FSZIP_DecryptByte(st);
		FSZIP_UpdateKeys(st, ch);
		cryptheader[u] = ch;
	}

	memcpy(st->initialkey, st->cryptkey, sizeof(st->initialkey));

	//cryptheader[11] should be the high byte of the file's crc
	//[10] might be the second byte, but also might not be.
	if (cryptheader[11] != (unsigned char)(crc>>24))
		return false;
//	if (cryptheader[10] != (unsigned char)(crc>>16))
//		return false;
	return true;
}
static void FSZIP_DecryptBlock(struct decompressstate *st, char *block, size_t blocksize)
{
    while (blocksize--)
	{
        unsigned char temp = *block ^ FSZIP_DecryptByte(st);
        FSZIP_UpdateKeys(st, temp);
        *block++ = temp;
	}
}
#endif

#ifdef AVAIL_ZLIB
//if the offset is still within our decompressed block then we can just rewind a smidge
static qboolean FSZIP_Deflate_Seek(struct decompressstate *st, qofs_t *offset, qofs_t newoffset)
{
	qofs_t dist;
	if (newoffset <= *offset)
	{	//rewinding
		dist = *offset-newoffset;
		if (st->readoffset >= dist)
		{
			st->readoffset -= dist;
			*offset -= dist;
			return true;
		}
		//went too far back, we lost that data.
	}
	else
	{	//seek forward... mneh
		dist = newoffset - *offset;
		if (st->readoffset+dist <= st->strm.total_out)
		{
			st->readoffset += dist;
			*offset += dist;
			return true;
		}
		//fail. we could just decompress more, but we're probably better off copying to a temp file instead.
	}
	return false;
}
static qofs_t FSZIP_Deflate_Read(struct decompressstate *st, qbyte *buffer, qofs_t bytes)
{
	qboolean eof = false;
	int err;
	qofs_t read = 0;
	while(bytes)
	{
		if (st->readoffset < st->strm.total_out)
		{
			unsigned int consume = st->strm.total_out-st->readoffset;
			if (consume > bytes)
				consume = bytes;
			memcpy(buffer, st->outbuffer+st->readoffset, consume);
			buffer += consume;
			bytes -= consume;
			read += consume;
			st->readoffset += consume;
			continue;
		}
		else if (eof)
			break;	//no input available, and nothing in the buffers.

		st->strm.total_out = 0;
		st->strm.avail_out = sizeof(st->outbuffer);
		st->strm.next_out = st->outbuffer;
		st->readoffset = 0;
		if (!st->strm.avail_in)
		{
			qofs_t sz;
			sz = st->cend - st->cofs;
			if (sz > sizeof(st->inbuffer))
				sz = sizeof(st->inbuffer);
			if (sz)
			{
				//feed it.
				if (Sys_LockMutex(st->source->mutex))
				{
					VFS_SEEK(st->source->raw, st->cofs);
					st->strm.avail_in = VFS_READ(st->source->raw, st->inbuffer, sz);
					Sys_UnlockMutex(st->source->mutex);
				}
				else
					st->strm.avail_in = 0;
				st->strm.next_in = st->inbuffer;
				st->cofs += st->strm.avail_in;
#ifdef ZIPCRYPT
				if (st->encrypted)
					FSZIP_DecryptBlock(st, st->inbuffer, st->strm.avail_in);
#endif
			}
			if (!st->strm.avail_in)
				eof = true;
		}
		err = qinflate(&st->strm,Z_SYNC_FLUSH);
		if (err == Z_STREAM_END)
			eof = true;
		else if (err != Z_OK)
			break;
	}
	return read;
}

static void FSZIP_Deflate_Destroy(struct decompressstate *st)
{
	qinflateEnd(&st->strm);
	Z_Free(st);
}

static struct decompressstate *FSZIP_Deflate_Init(zipfile_t *source, qofs_t start, qofs_t csize, qofs_t usize, char *filename, char *password, unsigned int crc)
{
	struct decompressstate *st;
	if (!ZLIB_LOADED())
	{
		Con_Printf("zlib not available\n");
		return NULL;
	}
	st = Z_Malloc(sizeof(*st));
	st->Reinit	= FSZIP_Deflate_Init;
	st->Read	= FSZIP_Deflate_Read;
	st->Seek	= FSZIP_Deflate_Seek;
	st->Destroy	= FSZIP_Deflate_Destroy;
	st->source = source;

#ifdef ZIPCRYPT
	if (password && csize >= 12)
	{
		char entropy[12];
		if (Sys_LockMutex(source->mutex))
		{
			VFS_SEEK(source->raw, start);
			VFS_READ(source->raw, entropy, sizeof(entropy));
			Sys_UnlockMutex(source->mutex);
		}
		if (!FSZIP_SetupCrytoKeys(st, password, entropy, crc))
		{
			Con_Printf("Invalid password, cannot decrypt %s\n", filename);
			Z_Free(st);
			return NULL;
		}
		start += sizeof(entropy);
		csize -= sizeof(entropy);
	}
#endif

	st->cstart = st->cofs = start;
	st->cend = start + csize;
	st->usize = usize;

	st->strm.data_type = Z_UNKNOWN;
	qinflateInit2(&st->strm, -MAX_WBITS);
	return st;
}
#endif

#if defined(ZLIB_DEFLATE64) && !defined(AVAIL_ZLIB)
	#undef ZLIB_DEFLATE64	//don't be silly.
#endif
#if defined(ZLIB_DEFLATE64)
#include "infback9.h"	//an obscure compile-your-own part of zlib.
struct def64ctx
{
	vfsfile_t *src;
	vfsfile_t *dst;
	qofs_t csize;
	qofs_t usize;
	unsigned int crc;

	qbyte inbuf[0x8000];
};
static unsigned int FSZIP_Deflate64_Grab(void *vctx, unsigned char **bufptr)
{
	struct def64ctx *ctx = vctx;
	int avail;
	avail = sizeof(ctx->inbuf);
	if (avail > ctx->csize)
		avail = ctx->csize;	//don't over-read.
	if (avail <= 0)
	{
		*bufptr = NULL;
		return 0;
	}

	avail = VFS_READ(ctx->src, ctx->inbuf, avail);
	*bufptr = ctx->inbuf;

	if (avail < 0)
		avail = 0;	//treated as eof...
	ctx->csize -= avail;
	return avail;
}
static int FSZIP_Deflate64_Spew(void *vctx, unsigned char *buf, unsigned int buflen)
{
	struct def64ctx *ctx = vctx;

	//update the crc
	ctx->crc = crc32(ctx->crc, buf, buflen);
	ctx->usize += buflen;

	if (VFS_WRITE(ctx->dst, buf, buflen) != buflen)
		return 1;	//failure returns non-zero.
	return 0;
}
//inflateBack stuff is apparently not restartable, and must read the entire file
static vfsfile_t *FSZIP_Deflate64(vfsfile_t *src, qofs_t csize, qofs_t usize, unsigned int crc32)
{
	z_stream strm = {NULL};
	qbyte window[65536];
	struct def64ctx ctx;
	ctx.src = src;
	ctx.dst = VFSPIPE_Open(1, true);
	ctx.csize = csize;
	ctx.usize = 0;
	ctx.crc = 0;

	strm.data_type = Z_UNKNOWN;
	inflateBack9Init(&strm, window);
	//getting inflateBack9 to
	if (Z_STREAM_END != inflateBack9(&strm, FSZIP_Deflate64_Grab, &ctx, FSZIP_Deflate64_Spew, &ctx))
	{	//some stream error?
		Con_Printf("Decompression error\n");
		VFS_CLOSE(ctx.dst);
		ctx.dst = NULL;
	}
	else if (ctx.csize != 0 || ctx.usize != usize)
	{	//corrupt file table?
		Con_Printf("Decompression size error\n");
		Con_Printf("read %i of %i bytes\n", (unsigned)ctx.csize, (unsigned)csize);
		Con_Printf("wrote %i of %i bytes\n", (unsigned)ctx.usize, (unsigned)usize);
		VFS_CLOSE(ctx.dst);
		ctx.dst = NULL;
	}
	else if (ctx.crc != crc32)
	{	//corrupt file table?
		Con_Printf("CRC32 error\n");
		VFS_CLOSE(ctx.dst);
		ctx.dst = NULL;
	}
	inflateBack9End(&strm);

	return ctx.dst;
}
#endif

#ifdef AVAIL_BZLIB
//if the offset is still within our decompressed block then we can just rewind a smidge
static qboolean FSZIP_BZip2_Seek(struct decompressstate *st, qofs_t *offset, qofs_t newoffset)
{
	qofs_t dist;
	if (newoffset <= *offset)
	{	//rewinding
		dist = *offset-newoffset;
		if (st->readoffset >= dist)
		{
			st->readoffset -= dist;
			*offset -= dist;
			return true;
		}
		//went too far back, we lost that data.
	}
	else
	{	//seek forward... mneh
		dist = newoffset - *offset;
		if (st->readoffset+dist <= st->bstrm.total_out_lo32)
		{
			st->readoffset += dist;
			*offset += dist;
			return true;
		}
		//fail. we could just decompress more, but we're probably better off copying to a temp file instead.
	}
	return false;
}
//decompress in chunks.
static qofs_t FSZIP_BZip2_Read(struct decompressstate *st, qbyte *buffer, qofs_t bytes)
{
	qboolean eof = false;
	int err;
	qofs_t read = 0;
	while(bytes)
	{
		if (st->readoffset < st->bstrm.total_out_lo32)
		{
			unsigned int consume = st->bstrm.total_out_lo32-st->readoffset;
			if (consume > bytes)
				consume = bytes;
			memcpy(buffer, st->outbuffer+st->readoffset, consume);
			buffer += consume;
			bytes -= consume;
			read += consume;
			st->readoffset += consume;
			continue;
		}
		else if (eof)
			break;	//no input available, and nothing in the buffers.

		st->bstrm.total_out_lo32 = 0;
		st->bstrm.total_out_hi32 = 0;
		st->bstrm.avail_out = sizeof(st->outbuffer);
		st->bstrm.next_out = st->outbuffer;
		st->readoffset = 0;
		if (!st->bstrm.avail_in)
		{
			qofs_t sz;
			sz = st->cend - st->cofs;
			if (sz > sizeof(st->inbuffer))
				sz = sizeof(st->inbuffer);
			if (sz)
			{
				//feed it.
				if (Sys_LockMutex(st->source->mutex))
				{
					VFS_SEEK(st->source->raw, st->cofs);
					st->bstrm.avail_in = VFS_READ(st->source->raw, st->inbuffer, sz);
					Sys_UnlockMutex(st->source->mutex);
				}
				else
					st->bstrm.avail_in = 0;
				st->bstrm.next_in = st->inbuffer;
				st->cofs += st->bstrm.avail_in;
#ifdef ZIPCRYPT
				if (st->encrypted)
					FSZIP_DecryptBlock(st, st->inbuffer, st->bstrm.avail_in);
#endif
			}
			if (!st->bstrm.avail_in)
				eof = true;
		}
		err = BZ2_bzDecompress(&st->bstrm);
		if (err == BZ_FINISH_OK)
			eof = true;
		else if (err < 0)
			break;
	}
	return read;
}

static void FSZIP_BZip2_Destroy(struct decompressstate *st)
{
	BZ2_bzDecompressEnd(&st->bstrm);
	Z_Free(st);
}

static struct decompressstate *FSZIP_BZip2_Init(zipfile_t *source, qofs_t start, qofs_t csize, qofs_t usize, char *filename, char *password, unsigned int crc)
{
	struct decompressstate *st;
	if (!BZLIB_LOADED())
	{
		Con_Printf("bzlib not available\n");
		return NULL;
	}
	st = Z_Malloc(sizeof(*st));
	st->Reinit	= FSZIP_BZip2_Init;
	st->Read	= FSZIP_BZip2_Read;
	st->Seek	= FSZIP_BZip2_Seek;
	st->Destroy	= FSZIP_BZip2_Destroy;
	st->source	= source;

#ifdef ZIPCRYPT
	if (password && csize >= 12)
	{
		char entropy[12];
		if (Sys_LockMutex(source->mutex))
		{
			VFS_SEEK(source->raw, start);
			VFS_READ(source->raw, entropy, sizeof(entropy));
			Sys_UnlockMutex(source->mutex);
		}
		if (!FSZIP_SetupCrytoKeys(st, password, entropy, crc))
		{
			Con_Printf("Invalid password, cannot decrypt %s\n", filename);
			Z_Free(st);
			return NULL;
		}
		start += sizeof(entropy);
		csize -= sizeof(entropy);
	}
#endif

	st->cstart = st->cofs = start;
	st->cend = start + csize;
	st->usize = usize;

	BZ2_bzDecompressInit(&st->bstrm, 0, false);
	return st;
}
#endif

struct decompressstate *FSZIP_Decompress_Rewind(struct decompressstate *decompress)
{
	qofs_t cstart = decompress->cstart, csize = decompress->cend - cstart;
	qofs_t usize = decompress->usize;
	struct decompressstate *(*Reinit)(zipfile_t *source, qofs_t start, qofs_t csize, qofs_t usize, char *filename, char *password, unsigned int crc) = decompress->Reinit;
	zipfile_t *source = decompress->source;
#ifdef ZIPCRYPT	//we need to preserve any crypto stuff if we're restarting the stream
	qboolean encrypted = decompress->encrypted;
	unsigned int cryptkeys[3];
	const z_crc_t *crctab = decompress->crctable;
	memcpy(cryptkeys, decompress->initialkey, sizeof(cryptkeys));
#endif

	decompress->Destroy(decompress);

	decompress = Reinit(source, cstart, csize, usize, NULL, NULL, 0);
#ifdef ZIPCRYPT
	decompress->encrypted = encrypted;
	decompress->crctable = crctab;
	memcpy(decompress->initialkey, cryptkeys, sizeof(decompress->initialkey));
	memcpy(decompress->cryptkey, cryptkeys, sizeof(decompress->cryptkey));
#endif
	return decompress;
}

static vfsfile_t *FSZIP_Decompress_ToTempFile(struct decompressstate *decompress)
{	//if they're going to seek on a file in a zip, let's just copy it out
	qofs_t upos = 0, usize = decompress->usize;
	qofs_t chunk;
	qbyte buffer[16384];
	vfsfile_t *defer;

	defer = FS_OpenTemp();
	if (defer)
	{
		decompress = FSZIP_Decompress_Rewind(decompress);

		while (upos < usize)
		{
			chunk = usize - upos;
			if (chunk > sizeof(buffer))
				chunk = sizeof(buffer);
			if (!decompress->Read(decompress, buffer, chunk))
				break;
			if (VFS_WRITE(defer, buffer, chunk) != chunk)
				break;
			upos += chunk;
		}
		decompress->Destroy(decompress);

		return defer;
	}
	return NULL;
}
#endif

//an open vfsfile_t
typedef struct {
	vfsfile_t funcs;

	vfsfile_t *defer;	//if set, reads+seeks are defered to this file instead.

	//in case we're forced away.
	zipfile_t *parent;
#ifdef DO_ZIP_DECOMPRESS
	struct decompressstate *decompress;
#endif
	qofs_t pos;
	qofs_t length;	//try and optimise some things
//	int index;
	qofs_t startpos;	//file data offset
} vfszip_t;

static int QDECL VFSZIP_ReadBytes (struct vfsfile_s *file, void *buffer, int bytestoread)
{
	int read;
	vfszip_t *vfsz = (vfszip_t*)file;

	if (vfsz->defer)
		return VFS_READ(vfsz->defer, buffer, bytestoread);

#ifdef DO_ZIP_DECOMPRESS
	if (vfsz->decompress)
	{
		read = vfsz->decompress->Read(vfsz->decompress, buffer, bytestoread);
	}
	else
#endif
	if (Sys_LockMutex(vfsz->parent->mutex))
	{
		VFS_SEEK(vfsz->parent->raw, vfsz->pos+vfsz->startpos);
		if (vfsz->pos + bytestoread > vfsz->length)
			bytestoread = max(0, vfsz->length - vfsz->pos);
		read = VFS_READ(vfsz->parent->raw, buffer, bytestoread);
		Sys_UnlockMutex(vfsz->parent->mutex);
	}
	else
		read = 0;

	if (read < bytestoread)
		((char*)buffer)[read] = 0;

	vfsz->pos += read;
	return read;
}
static qboolean QDECL VFSZIP_Seek (struct vfsfile_s *file, qofs_t pos)
{
	vfszip_t *vfsz = (vfszip_t*)file;

	if (vfsz->defer)
		return VFS_SEEK(vfsz->defer, pos);

#ifdef DO_ZIP_DECOMPRESS
	//This is *really* inefficient
	if (vfsz->decompress)
	{	//if they're going to seek on a file in a zip, let's just copy it out
		if (vfsz->decompress->Seek(vfsz->decompress, &vfsz->pos, pos))
			return true;
		else if (pos == 0)
			vfsz->decompress = FSZIP_Decompress_Rewind(vfsz->decompress);
		else
		{
			vfsz->defer = FSZIP_Decompress_ToTempFile(vfsz->decompress);
			vfsz->decompress = NULL;
			if (vfsz->defer)
				return VFS_SEEK(vfsz->defer, pos);
			return false;
		}
	}
#endif

	if (pos > vfsz->length)
		return false;
	vfsz->pos = pos;

	return true;
}
static qofs_t QDECL VFSZIP_Tell (struct vfsfile_s *file)
{
	vfszip_t *vfsz = (vfszip_t*)file;

	if (vfsz->defer)
		return VFS_TELL(vfsz->defer);

	return vfsz->pos;
}
static qofs_t QDECL VFSZIP_GetLen (struct vfsfile_s *file)
{
	vfszip_t *vfsz = (vfszip_t*)file;
	return vfsz->length;
}
static qboolean QDECL VFSZIP_Close (struct vfsfile_s *file)
{
	vfszip_t *vfsz = (vfszip_t*)file;

	if (vfsz->defer)
		VFS_CLOSE(vfsz->defer);

#ifdef DO_ZIP_DECOMPRESS
	if (vfsz->decompress)
		vfsz->decompress->Destroy(vfsz->decompress);
#endif

	FSZIP_ClosePath(&vfsz->parent->pub);
	Z_Free(vfsz);

	return true;	//FIXME: return an error if the crc was wrong.
}

static qboolean FSZIP_ValidateLocalHeader(zipfile_t *zip, zpackfile_t *zfile, qofs_t *datastart, qofs_t *datasize);

static qboolean QDECL FSZIP_FileStat (searchpathfuncs_t *handle, flocation_t *loc, time_t *mtime)
{
	zpackfile_t *pf = loc->fhandle;
	*mtime = pf->mtime;
	return true;
}

static vfsfile_t *QDECL FSZIP_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode)
{
	zipfile_t *zip = (void*)handle;
	vfszip_t *vfsz;
	unsigned int flags;
	qofs_t datasize = 0;
	zpackfile_t *pf = loc->fhandle;

	if (strcmp(mode, "rb"))
		return NULL; //urm, unable to write/append

	if (qofs_Error(loc->len))
		return NULL;

	flags = pf->flags;
	if (flags & ZFL_CORRUPT)
		return NULL;

	if (zip->thisdisk != pf->disknum)
	{
		if (pf->disknum >= zip->numspans)
		{
			Con_Printf("file %s has oob disk number\n", pf->name);
			return NULL;
		}
		zip = zip->spans[pf->disknum];
		if (!zip)
		{
			zip = (void*)handle;
			Con_Printf("spanned zip %s lacks segment %u for file %s\n", zip->filename, pf->disknum+1, pf->name);
			return NULL;
		}
	}

	vfsz = Z_Malloc(sizeof(vfszip_t));

	vfsz->parent = zip;
	vfsz->length = loc->len;

#ifdef _DEBUG
	Q_strncpyz(vfsz->funcs.dbgname, pf->name, sizeof(vfsz->funcs.dbgname));
#endif
	vfsz->funcs.Close = VFSZIP_Close;
	vfsz->funcs.GetLen = VFSZIP_GetLen;
	vfsz->funcs.ReadBytes = VFSZIP_ReadBytes;
	//vfsz->funcs.WriteBytes = VFSZIP_WriteBytes;
	vfsz->funcs.Seek = VFSZIP_Seek;
	vfsz->funcs.Tell = VFSZIP_Tell;
	vfsz->funcs.WriteBytes = NULL;
	vfsz->funcs.seekstyle = SS_SLOW;

	//NOTE: pf->name is the quakeified name, and may have an extra prefix/stripped prefix for certain zips - different from what you'd see if you opened the zip yourself. this is only relevant for debugging, whuch might be misleading but is not fatal.
	if (!FSZIP_ValidateLocalHeader(zip, pf, &vfsz->startpos, &datasize))
	{
		Con_Printf(CON_WARNING"file %s:%s is incompatible or inconsistent with zip central directory\n", zip->filename, pf->name);
		Z_Free(vfsz);
		return NULL;
	}

	if (flags & ZFL_DEFLATE64D)
	{	//crap
#if defined(ZLIB_DEFLATE64)
		vfsfile_t *tmp = NULL;
		qofs_t startpos = vfsz->startpos;
		qofs_t usize = vfsz->length;
		qofs_t csize = datasize;
		Z_Free(vfsz);

		Con_Printf(CON_WARNING"file %s:%s was compressed with deflate64\n", zip->filename, pf->name);

		if (Sys_LockMutex(zip->mutex))
		{
			VFS_SEEK(vfsz->parent->raw, startpos);
			tmp = FSZIP_Deflate64(zip->raw, csize, usize, pf->crc);
			Sys_UnlockMutex(zip->mutex);
		}

		return tmp;
#else
		Con_Printf(CON_WARNING"%s:%s: deflate64 not supported\n", COM_SkipPath(zip->filename), pf->name);
		Z_Free(vfsz);
		return NULL;
#endif
	}

	if (flags & ZFL_DEFLATED)
	{
#ifdef AVAIL_ZLIB
#ifdef ZIPCRYPT
		//FIXME: Cvar_Get is not threadsafe, and nor is accessing the cvar...
		char *password = (flags & ZFL_WEAKENCRYPT)?Cvar_Get("fs_zip_password", "thisispublic", 0, "Filesystem")->string:NULL;
#else
		char *password = NULL;
#endif
		vfsz->decompress = FSZIP_Deflate_Init(zip, vfsz->startpos, datasize, vfsz->length, pf->name, password, pf->crc);
		if (!vfsz->decompress)
#else
		Con_Printf("%s:%s: zlib not supported\n", COM_SkipPath(zip->filename), pf->name);
#endif
		{
			/*
			windows explorer tends to use deflate64 on large files, which zlib and thus we, do not support, thus this is a 'common' failure path
			this might also trigger from other errors, of course.
			*/
			Z_Free(vfsz);
			return NULL;
		}
	}
	if (flags & ZFL_BZIP2)
	{
#ifdef AVAIL_BZLIB
#ifdef ZIPCRYPT
		//FIXME: Cvar_Get is not threadsafe, and nor is accessing the cvar...
		char *password = (flags & ZFL_WEAKENCRYPT)?Cvar_Get("fs_zip_password", "thisispublic", 0, "Filesystem")->string:NULL;
#else
		char *password = NULL;
#endif
		vfsz->decompress = FSZIP_BZip2_Init(zip, vfsz->startpos, datasize, vfsz->length, pf->name, password, pf->crc);
		if (!vfsz->decompress)
#else
		Con_Printf("%s:%s: libbz2 not supported\n", COM_SkipPath(zip->filename), pf->name);
#endif
		{
			/*
			windows explorer tends to use deflate64 on large files, which zlib and thus we, do not support, thus this is a 'common' failure path
			this might also trigger from other errors, of course.
			*/
			Z_Free(vfsz);
			return NULL;
		}
	}

	FTE_Atomic32_Inc(&zip->references);
	return (vfsfile_t*)vfsz;
}

struct zipinfo
{
	unsigned int	thisdisk;					//this disk number
	unsigned int	centraldir_startdisk;		//central directory starts on this disk
	qofs_t			centraldir_numfiles_disk;	//number of files in the centraldir on this disk
	qofs_t			centraldir_numfiles_all;	//number of files in the centraldir on all disks
	qofs_t			centraldir_size;			//total size of the centraldir
	qofs_t			centraldir_offset;			//start offset of the centraldir with respect to the first disk that contains it
	unsigned short	commentlength;				//length of the comment at the end of the archive.

	unsigned int	zip64_centraldirend_disk;	//zip64 weirdness
	qofs_t			zip64_centraldirend_offset;
	unsigned int	diskcount;
	qofs_t			zip64_eocdsize;
	unsigned short	zip64_version_madeby;
	unsigned short	zip64_version_needed;

	unsigned short	centraldir_compressionmethod;
	unsigned short	centraldir_algid;

	qofs_t			centraldir_end;
	qofs_t			zipoffset;
};
struct zipcentralentry
{
	unsigned char	*fname;
	qofs_t			cesize;
	unsigned int	flags;
	time_t			mtime;

	//PK12
	unsigned short	version_madeby;
	unsigned short	version_needed;
	unsigned short	gflags;
	unsigned short	cmethod;
	unsigned short	lastmodfiletime;
	unsigned short	lastmodfiledate;
	unsigned int	crc32;
	qofs_t			csize;
	qofs_t			usize;
	unsigned short	fname_len;
	unsigned short	extra_len;
	unsigned short	comment_len;
	unsigned int	disknum;
	unsigned short	iattributes;
	unsigned int	eattributes;
	qofs_t	localheaderoffset;	//from start of disk
};

struct ziplocalentry
{
      //PK34
	unsigned short	version_needed;
	unsigned short	gpflags;
	unsigned short	cmethod;
	unsigned short	lastmodfiletime;
	unsigned short	lastmodfiledate;
	unsigned int	crc32;
	qofs_t			csize;
	qofs_t			usize;
	unsigned short	fname_len;
	unsigned short	extra_len;
};

#define LittleU2FromPtr(p) (unsigned int)((p)[0] | ((p)[1]<<8u))
#define LittleU4FromPtr(p) (unsigned int)((p)[0] | ((p)[1]<<8u) | ((p)[2]<<16u) | ((p)[3]<<24u))
#define LittleU8FromPtr(p) qofs_Make(LittleU4FromPtr(p), LittleU4FromPtr((p)+4))

#define SIZE_LOCALENTRY 30
#define SIZE_ENDOFCENTRALDIRECTORY 22
#define SIZE_ZIP64ENDOFCENTRALDIRECTORY_V1 56
#define SIZE_ZIP64ENDOFCENTRALDIRECTORY_V2 84
#define SIZE_ZIP64ENDOFCENTRALDIRECTORY SIZE_ZIP64ENDOFCENTRALDIRECTORY_V2
#define SIZE_ZIP64ENDOFCENTRALDIRECTORYLOCATOR 20

//check the local header and determine the place the data actually starts.
//we don't do much checking, just validating a few fields that are copies of fields we tracked elsewhere anyway.
//don't bother with the crc either
static qboolean FSZIP_ValidateLocalHeader(zipfile_t *zip, zpackfile_t *zfile, qofs_t *datastart, qofs_t *datasize)
{
	struct ziplocalentry local;
	qbyte localdata[SIZE_LOCALENTRY];
	qofs_t localstart = zfile->localpos;

	
	if (!Sys_LockMutex(zip->mutex))
		return false;	//ohnoes
	VFS_SEEK(zip->raw, localstart);
	VFS_READ(zip->raw, localdata, sizeof(localdata));
	Sys_UnlockMutex(zip->mutex);

	//make sure we found the right sort of table.
	if (localdata[0] != 'P' ||
		localdata[1] != 'K' ||
		localdata[2] != 3 ||
		localdata[3] != 4)
		return false;

	local.version_needed = LittleU2FromPtr(localdata+4);
	local.gpflags = LittleU2FromPtr(localdata+6);
	local.cmethod = LittleU2FromPtr(localdata+8);
	local.lastmodfiletime = LittleU2FromPtr(localdata+10);
	local.lastmodfiledate = LittleU2FromPtr(localdata+12);
	local.crc32 = LittleU4FromPtr(localdata+14);
	local.csize = LittleU4FromPtr(localdata+18);
	local.usize = LittleU4FromPtr(localdata+22);
	local.fname_len = LittleU2FromPtr(localdata+26);
	local.extra_len = LittleU2FromPtr(localdata+28);

	localstart += SIZE_LOCALENTRY;
	localstart += local.fname_len;

	//parse extra
	if (local.usize == 0xffffffffu || local.csize == 0xffffffffu)	//don't bother otherwise.
	if (local.extra_len)
	{
		qbyte extradata[65536];
		qbyte *extra = extradata;
		qbyte *extraend = extradata + local.extra_len;
		unsigned short extrachunk_tag;
		unsigned short extrachunk_len;

		if (!Sys_LockMutex(zip->mutex))
			return false;	//ohnoes
		VFS_SEEK(zip->raw, localstart);
		VFS_READ(zip->raw, extradata, sizeof(extradata));
		Sys_UnlockMutex(zip->mutex);


		while(extra+4 < extraend)
		{
			extrachunk_tag = LittleU2FromPtr(extra+0);
			extrachunk_len = LittleU2FromPtr(extra+2);
			if (extra + extrachunk_len > extraend)
				break;	//error
			extra += 4;

			switch(extrachunk_tag)
			{
			case 1:	//zip64 extended information extra field. the attributes are only present if the reegular file info is nulled out with a -1
				if (local.usize == 0xffffffffu)
				{
					local.usize = LittleU8FromPtr(extra);
					extra += 8;
				}
				if (local.csize == 0xffffffffu)
				{
					local.csize = LittleU8FromPtr(extra);
					extra += 8;
				}
				break;
			default:
/*				Con_Printf("Unknown chunk %x\n", extrachunk_tag);
			case 0x000a:	//NTFS (timestamps)
			case 0x5455:	//extended timestamp
			case 0x7875:	//unix uid/gid
*/				extra += extrachunk_len;
				break;
			}
		}
	}
	localstart += local.extra_len;
	*datastart = localstart;	//this is the end of the local block, and the start of the data block (well, actually, should be encryption, but we don't support that).
	*datasize = local.csize;	//FIXME: this is often masked. use the central directory's value instead.

	if (local.gpflags & (1u<<3))
	{
		//crc, usize, and csize were not known at the time the file was compressed.
		//there is a 'data descriptor' after the file data, but to parse that we would need to decompress the file.
		//instead, just depend upon upon the central directory and don't bother checking.
	}
	else
	{
		if (local.crc32 != zfile->crc)
			return false;
		if (local.usize != zfile->filelen)
			return false;
	}

	//FIXME: with pure paths, we still don't bother checking the crc (again, would require decompressing the entire file in advance).

	if (localstart+local.csize > zip->rawsize)
		return false;	//FIXME: proper spanned zips fragment compressed data over multiple spans, but we don't support that

	if (local.cmethod == 0)
		return (zfile->flags & ZFL_COMPRESSIONTYPE) == ZFL_STORED;
#if defined(AVAIL_ZLIB)
	if (local.cmethod == 8)
		return (zfile->flags & ZFL_COMPRESSIONTYPE) == ZFL_DEFLATED;
#endif
#if defined(ZLIB_DEFLATE64)
	if (local.cmethod == 9)
		return (zfile->flags & ZFL_COMPRESSIONTYPE) == ZFL_DEFLATE64D;
#endif
#ifdef AVAIL_BZLIB
	if (local.cmethod == 12)
		return (zfile->flags & ZFL_COMPRESSIONTYPE) == ZFL_BZIP2;
#endif
	return false;	//some other method that we don't know.
}

static qboolean FSZIP_ReadCentralEntry(zipfile_t *zip, qbyte *data, struct zipcentralentry *entry)
{
	struct tm t;
	entry->flags = 0;
	entry->fname = "";
	entry->fname_len = 0;

	if (data[0] != 'P' ||
		data[1] != 'K' ||
		data[2] != 1 ||
		data[3] != 2)
		return false;	//verify the signature

	//if we read too much, we'll catch it after.
	entry->version_madeby = LittleU2FromPtr(data+4);
	entry->version_needed = LittleU2FromPtr(data+6);
	entry->gflags = LittleU2FromPtr(data+8);
	entry->cmethod = LittleU2FromPtr(data+10);
	entry->lastmodfiletime = LittleU2FromPtr(data+12);
	entry->lastmodfiledate = LittleU2FromPtr(data+14);
	entry->crc32 = LittleU4FromPtr(data+16);
	entry->csize = LittleU4FromPtr(data+20);
	entry->usize = LittleU4FromPtr(data+24);
	entry->fname_len = LittleU2FromPtr(data+28);
	entry->extra_len = LittleU2FromPtr(data+30);
	entry->comment_len = LittleU2FromPtr(data+32);
	entry->disknum = LittleU2FromPtr(data+34);
	entry->iattributes = LittleU2FromPtr(data+36);
	entry->eattributes = LittleU4FromPtr(data+38);
	entry->localheaderoffset = LittleU4FromPtr(data+42);
	entry->cesize = 46;

	//mark the filename position
	entry->fname = data+entry->cesize;
	entry->cesize += entry->fname_len;

	memset(&t, 0, sizeof(t));
	t.tm_mday =  (entry->lastmodfiledate&0x001f)>>0;
	t.tm_mon  =  (entry->lastmodfiledate&0x01e0)>>5;
	t.tm_year = ((entry->lastmodfiledate&0xfe00)>>9) + (1980 - 1900);
	t.tm_sec  = ((entry->lastmodfiletime&0x001f)>>0)*2;
	t.tm_min  =  (entry->lastmodfiletime&0x07e0)>>5;
	t.tm_hour =  (entry->lastmodfiletime&0xf800)>>11;
	entry->mtime = mktime(&t);

	//parse extra
	if (entry->extra_len)
	{
		qbyte *extra = data + entry->cesize;
		qbyte *extraend = extra + entry->extra_len;
		unsigned short extrachunk_tag;
		unsigned short extrachunk_len;
		while(extra+4 < extraend)
		{
			extrachunk_tag = LittleU2FromPtr(extra+0);
			extrachunk_len = LittleU2FromPtr(extra+2);
			if (extra + extrachunk_len > extraend)
				break;	//error
			extra += 4;

			switch(extrachunk_tag)
			{
			case 1:	//zip64 extended information extra field. the attributes are only present if the reegular file info is nulled out with a -1
				if (entry->usize == 0xffffffffu)
				{
					entry->usize = LittleU8FromPtr(extra);
					extra += 8;
				}
				if (entry->csize == 0xffffffffu)
				{
					entry->csize = LittleU8FromPtr(extra);
					extra += 8;
				}
				if (entry->localheaderoffset == 0xffffffffu)
				{
					entry->localheaderoffset = LittleU8FromPtr(extra);
					extra += 8;
				}
				if (entry->disknum == 0xffffu)
				{
					entry->disknum = LittleU4FromPtr(extra);
					extra += 4;
				}
				break;
#if !defined(_MSC_VER) || _MSC_VER > 1200
			case 0x000a:	//NTFS extra field
				//0+4: reserved
				//4+2: subtag(must be 1, for times)
				//6+2: subtagsize(times: must be == 8*3+4)
				//8+8: mtime
				//16+8: atime
				//24+8: ctime
				if (extrachunk_len >= 32 && LittleU2FromPtr(extra+4) == 1 && LittleU2FromPtr(extra+6) == 8*3)	
					entry->mtime = LittleU8FromPtr(extra+8) / 10000000ULL - 11644473600ULL;
				else
					Con_Printf("zip: unsupported ntfs subchunk %x\n", extrachunk_tag);
				extra += extrachunk_len;
				break;
#endif
			case 0x5455:
				if (extra[0] & 1)
					entry->mtime = LittleU4FromPtr(extra+1);
				//access and creation do NOT exist in the central header.
				extra += extrachunk_len;
				break;
			case 0x7075:	//unicode (utf-8) filename replacements.
				if (extra[0] == 1)	//version
				{
					//if (LittleU4FromPtr(extra+1) == qcrc32(?,entry->fname, entry->fnane_len))
					{
						entry->fname = extra+5;
						entry->fname_len = extrachunk_len-5;
						extra += extrachunk_len;

						entry->gflags |= (1u<<11);	//just set that flag. we don't support comments anyway.
					}
				}
				break;
			default:
/*				Con_Printf("Unknown chunk %x\n", extrachunk_tag);
			case 0x5455:	//extended timestamp
			case 0x7875:	//unix uid/gid
			case 0x9901:	//aes crypto
*/				extra += extrachunk_len;
				break;
			}
		}
		entry->cesize += entry->extra_len;
	}

	//parse comment
	entry->cesize += entry->comment_len;

	//check symlink flags
	{
		qbyte madeby = entry->version_madeby>>8;	//system
		//vms, unix, or beos file attributes includes a symlink attribute.
		//symlinks mean the file contents is just the name of another file.
		if (madeby == 2 || madeby == 3 || madeby == 16)
		{
			unsigned short unixattr = entry->eattributes>>16;
			if ((unixattr & 0xF000) == 0xA000)//fa&S_IFMT==S_IFLNK 
				entry->flags |= ZFL_SYMLINK;
			else if ((unixattr & 0xA000) == 0xA000)//fa&S_IFMT==S_IFLNK 
				entry->flags |= ZFL_SYMLINK;
		}
	}

	if (entry->gflags & (1u<<0))	//encrypted
	{
#ifdef ZIPCRYPT
		entry->flags |= ZFL_WEAKENCRYPT;
#else
		entry->flags |= ZFL_CORRUPT;
#endif
	}


	if (entry->gflags & (1u<<5))	//is patch data
		entry->flags |= ZFL_CORRUPT;
	else if (entry->gflags & (1u<<6))	//strong encryption
		entry->flags |= ZFL_CORRUPT;
	else if (entry->gflags & (1u<<13))	//strong encryption
		entry->flags |= ZFL_CORRUPT;
	else if (entry->cmethod == 0)
		entry->flags |= ZFL_STORED;
	//1: shrink
	//2-5: reduce
	//6: implode
	//7: tokenize
	else if (entry->cmethod == 8)	//8: deflate
		entry->flags |= ZFL_DEFLATED;
	else if (entry->cmethod == 9)	//9: deflate64 - patented. sometimes written by microsoft's crap, so this might be problematic. only minor improvements.
		entry->flags |= ZFL_DEFLATE64D;
	//10: implode
	else if (entry->cmethod == 12)	//12: bzip2
		entry->flags |= ZFL_BZIP2;
//	else if (entry->cmethod == 14)
//		entry->flags |= ZFL_LZMA;
	//19: lz77
	//97: wavpack
	//98: ppmd
	else 
		entry->flags |= ZFL_CORRUPT;	//unsupported compression method.

	if ((entry->flags & ZFL_WEAKENCRYPT) && !(entry->flags & ZFL_DEFLATED))
		entry->flags |= ZFL_CORRUPT;	//only support decryption with deflate.
	return true;
}

//for compatibility with windows, this is an IBM437 -> unicode translation (ucs-2 is sufficient here)
//the zip codepage is defined as either IBM437(aka: dos) or UTF-8. Systems that use a different codepage are buggy.
//this table is for filenames, so lets just fix up windows problems here.
unsigned short ibmtounicode[256] =
{
	0x0000,0x263A,0x263B,0x2665,0x2666,0x2663,0x2660,0x2022,0x25D8,0x25CB,0x25D9,0x2642,0x2640,0x266A,0x266B,0x263C,	//0x00(non-ascii, display-only)
	0x25BA,0x25C4,0x2195,0x203C,0x00B6,0x00A7,0x25AC,0x21A8,0x2191,0x2193,0x2192,0x2190,0x221F,0x2194,0x25B2,0x25BC,	//0x10(non-ascii, display-only)
	0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,	//0x20(ascii)
	0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,	//0x30(ascii)
	0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,	//0x40(ascii)
	0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005A,0x005B,   '/',0x005D,0x005E,0x005F,	//0x50(ascii)
	0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,	//0x60(ascii)
	0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x2302,	//0x70(mostly ascii, one display-only)
	0x00C7,0x00FC,0x00E9,0x00E2,0x00E4,0x00E0,0x00E5,0x00E7,0x00EA,0x00EB,0x00E8,0x00EF,0x00EE,0x00EC,0x00C4,0x00C5,	//0x80(non-ascii, printable)
	0x00C9,0x00E6,0x00C6,0x00F4,0x00F6,0x00F2,0x00FB,0x00F9,0x00FF,0x00D6,0x00DC,0x00A2,0x00A3,0x00A5,0x20A7,0x0192,	//0x90(non-ascii, printable)
	0x00E1,0x00ED,0x00F3,0x00FA,0x00F1,0x00D1,0x00AA,0x00BA,0x00BF,0x2310,0x00AC,0x00BD,0x00BC,0x00A1,0x00AB,0x00BB,	//0xa0(non-ascii, printable)
	0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,0x2555,0x2563,0x2551,0x2557,0x255D,0x255C,0x255B,0x2510,	//0xb0(box-drawing, printable)
	0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x255E,0x255F,0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0x2567,	//0xc0(box-drawing, printable)
	0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256B,0x256A,0x2518,0x250C,0x2588,0x2584,0x258C,0x2590,0x2580,	//0xd0(box-drawing, printable)
	0x03B1,0x00DF,0x0393,0x03C0,0x03A3,0x03C3,0x00B5,0x03C4,0x03A6,0x0398,0x03A9,0x03B4,0x221E,0x03C6,0x03B5,0x2229,	//0xe0(maths(greek), printable)
	0x2261,0x00B1,0x2265,0x2264,0x2320,0x2321,0x00F7,0x2248,0x00B0,0x2219,0x00B7,0x221A,0x207F,0x00B2,0x25A0,0x00A0,	//0xf0(maths, printable)
};

static qboolean FSZIP_EnumerateCentralDirectory(zipfile_t *zip, struct zipinfo *info, const char *prefix)
{
	qboolean success = false;
	zpackfile_t		*f;
	struct zipcentralentry entry;
	qofs_t ofs = 0;
	unsigned int i;
	//lazily read the entire thing.
	qbyte *centraldir = BZ_Malloc(info->centraldir_size);
	if (centraldir)
	{
		VFS_SEEK(zip->raw, info->centraldir_offset+info->zipoffset);
		if (VFS_READ(zip->raw, centraldir, info->centraldir_size) == info->centraldir_size)
		{
			zip->numfiles = info->centraldir_numfiles_disk;
			zip->files = f = Z_Malloc (zip->numfiles * sizeof(zpackfile_t));

			for (i = 0; i < zip->numfiles; i++)
			{
				if (!FSZIP_ReadCentralEntry(zip, centraldir+ofs, &entry) || ofs + entry.cesize > info->centraldir_size)
					break;

				f->disknum = entry.disknum;
				f->crc = entry.crc32;

				//copy out the filename and lowercase it
				if (entry.gflags & (1u<<11))
				{	//already utf-8 encoding, still need to work around windows though.
					if (entry.fname_len > sizeof(f->name)-1)
						entry.fname_len = sizeof(f->name)-1;
					for (i = 0; i < entry.fname_len; i++)
						f->name[i] = (entry.fname[i]=='\\')?'/':entry.fname[i];
					memcpy(f->name, entry.fname, entry.fname_len);
					f->name[entry.fname_len] = 0;
				}
				else
				{	//legacy charset
					int i;
					int nlen = 0;
					int cl;
					for (i = 0; i < entry.fname_len; i++)
					{
						cl = utf8_encode(f->name+nlen, ibmtounicode[entry.fname[i]], sizeof(f->name)-1 - nlen);
						if (!cl)	//overflowed, truncate cleanly.
							break;
						nlen += cl;
					}
					f->name[nlen] = 0;
				}
				ofs += entry.cesize;

				if (prefix && *prefix)
				{
					if (!strcmp(prefix, ".."))
					{	//strip leading directories blindly
						char *c; 
						for (c = f->name; *c; )
						{
							if (*c++ == '/')
								break;
						}
						memmove(f->name, c, strlen(c)+1);
					}
					else if (*prefix == '/')
					{	//move any files with the specified prefix to the root
						size_t prelen = strlen(prefix+1);
						if (!strncmp(prefix+1, f->name, prelen))
						{
							if (f->name[prelen] == '/')
								prelen++;
							memmove(f->name, f->name+prelen, strlen(f->name+prelen)+1);
						}
						else
							continue;
					}
					else
					{	//add the specified text to the start of each file name (handy for 'maps')
						size_t prelen = strlen(prefix);
						size_t oldlen = strlen(f->name);
						if (prelen+1+oldlen+1 > sizeof(f->name))
							*f->name = 0;
						else
						{
							memmove(f->name+prelen+1, f->name, oldlen);
							f->name[prelen] = '/';
							memmove(f->name, prefix, prelen);
						}
					}
				}
				Q_strlwr(f->name);

				f->filelen = *f->name?entry.usize:0;
				f->localpos = entry.localheaderoffset+info->zipoffset;
				f->flags = entry.flags;
				f->mtime = entry.mtime;

				f++;
			}

			success = i == zip->numfiles;
			if (!success)
			{
				Z_Free(zip->files);
				zip->files = NULL;
				zip->numfiles = 0;
			}
			zip->numfiles = f - zip->files;
		}
	}

	BZ_Free(centraldir);
	return success;
}

static qboolean FSZIP_FindEndCentralDirectory(zipfile_t *zip, struct zipinfo *info)
{
	qboolean result = false;
	//zip comment is capped to 65k or so, so we can use a single buffer for this
	qbyte traildata[0x10000 + SIZE_ENDOFCENTRALDIRECTORY+SIZE_ZIP64ENDOFCENTRALDIRECTORYLOCATOR];
	qbyte *magic;
	unsigned int trailsize = 0x10000 + SIZE_ENDOFCENTRALDIRECTORY+SIZE_ZIP64ENDOFCENTRALDIRECTORYLOCATOR;
	if (trailsize > zip->rawsize)
		trailsize = zip->rawsize;
	//FIXME: do in a loop to avoid a huge block of stack use
	VFS_SEEK(zip->raw, zip->rawsize - trailsize);
	VFS_READ(zip->raw, traildata, trailsize);

	memset(info, 0, sizeof(*info));

	for (magic = traildata+trailsize-SIZE_ENDOFCENTRALDIRECTORY; magic >= traildata; magic--)
	{
		if (magic[0] == 'P' && 
			magic[1] == 'K' && 
			magic[2] == 5 && 
			magic[3] == 6)
		{
			info->centraldir_end = (zip->rawsize-trailsize)+(magic-traildata);

			info->thisdisk					= LittleU2FromPtr(magic+4);
			info->centraldir_startdisk		= LittleU2FromPtr(magic+6);
			info->centraldir_numfiles_disk	= LittleU2FromPtr(magic+8);
			info->centraldir_numfiles_all	= LittleU2FromPtr(magic+10);
			info->centraldir_size			= LittleU4FromPtr(magic+12);
			info->centraldir_offset			= LittleU4FromPtr(magic+16);
			info->commentlength				= LittleU2FromPtr(magic+20);

			info->diskcount = info->thisdisk+1;	//zips are normally opened via the last file.

			result = true;
			break;
		}
	}

	if (!result)
		Con_Printf("%s: unable to find end-of-central-directory (not a zip?)\n", zip->filename);	//usually just not a zip.
	else

	//now look for a zip64 header.
	//this gives more larger archives, more files, larger files, more spanned disks.
	//note that the central directory itself is the same, it just has a couple of extra attributes on files that need them.
	for (magic -= SIZE_ZIP64ENDOFCENTRALDIRECTORYLOCATOR; magic >= traildata; magic--)
	{
		if (magic[0] == 'P' && 
			magic[1] == 'K' && 
			magic[2] == 6 && 
			magic[3] == 7)
		{
			qbyte z64eocd[SIZE_ZIP64ENDOFCENTRALDIRECTORY];

			info->zip64_centraldirend_disk		= LittleU4FromPtr(magic+4);
			info->zip64_centraldirend_offset	= LittleU8FromPtr(magic+8);
			info->diskcount						= LittleU4FromPtr(magic+16);

			VFS_SEEK(zip->raw, info->zip64_centraldirend_offset);
			VFS_READ(zip->raw, z64eocd, sizeof(z64eocd));

			if (z64eocd[0] == 'P' &&
				z64eocd[1] == 'K' &&
				z64eocd[2] == 6 &&
				z64eocd[3] == 6)
			{
				info->zip64_eocdsize						= LittleU8FromPtr(z64eocd+4) + 12;
				info->zip64_version_madeby					= LittleU2FromPtr(z64eocd+12);
				info->zip64_version_needed       			= LittleU2FromPtr(z64eocd+14);
				info->thisdisk             					= LittleU4FromPtr(z64eocd+16);
				info->centraldir_startdisk					= LittleU4FromPtr(z64eocd+20);
				info->centraldir_numfiles_disk  			= LittleU8FromPtr(z64eocd+24);
				info->centraldir_numfiles_all				= LittleU8FromPtr(z64eocd+32);
				info->centraldir_size   					= LittleU8FromPtr(z64eocd+40);
				info->centraldir_offset						= LittleU8FromPtr(z64eocd+48);

				if (info->zip64_eocdsize >= 84)
				{
					info->centraldir_compressionmethod		= LittleU2FromPtr(z64eocd+56);
//					info->zip64_2_centraldir_csize			= LittleU8FromPtr(z64eocd+58);
//					info->zip64_2_centraldir_usize			= LittleU8FromPtr(z64eocd+66);
					info->centraldir_algid					= LittleU2FromPtr(z64eocd+74);
//					info.zip64_2_bitlen						= LittleU2FromPtr(z64eocd+76);
//					info->zip64_2_flags						= LittleU2FromPtr(z64eocd+78);
//					info->zip64_2_hashid					= LittleU2FromPtr(z64eocd+80);
//					info->zip64_2_hashlength				= LittleU2FromPtr(z64eocd+82);
					//info->zip64_2_hashdata				= LittleUXFromPtr(z64eocd+84, info->zip64_2_hashlength);
				}
			}
			else
			{
				Con_Printf("%s: zip64 end-of-central directory at unknown offset.\n", zip->filename);
				result = false;
			}

			if (info->diskcount < 1 || info->zip64_centraldirend_disk != info->thisdisk)
			{	//must read the segment with the central directory.
				Con_Printf("%s: archive is spanned\n", zip->filename);
				return false;
			}

			break;
		}
	}

	if (info->thisdisk != info->centraldir_startdisk || info->centraldir_numfiles_disk != info->centraldir_numfiles_all)
	{	//must read the segment with the central directory.
		Con_Printf("%s: archive is spanned\n", zip->filename);
		result = false;
	}
	if (info->centraldir_compressionmethod || info->centraldir_algid)
	{
		Con_Printf("%s: encrypted centraldir\n", zip->filename);
		result = false;
	}

	return result;
}

/*
=================
COM_LoadZipFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
searchpathfuncs_t *QDECL FSZIP_LoadArchive (vfsfile_t *packhandle, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix)
{
	zipfile_t *zip;
	struct zipinfo info;

	if (!packhandle)
		return NULL;

	zip = Z_Malloc(sizeof(zipfile_t));
	Q_strncpyz(zip->filename, desc, sizeof(zip->filename));
	zip->raw = packhandle;
	zip->rawsize = VFS_GETLEN(zip->raw);

	//find the footer
	if (!FSZIP_FindEndCentralDirectory(zip, &info))
	{
		Z_Free(zip);
		Con_TPrintf ("Failed opening zipfile \"%s\" corrupt?\n", desc);
		return NULL;
	}

	//find the central directory.
	if (!FSZIP_FindEndCentralDirectory(zip, &info))
	{
		Z_Free(zip);
		Con_TPrintf ("Failed opening zipfile \"%s\" corrupt?\n", desc);
		return NULL;
	}

	//now read it.
	if (!FSZIP_EnumerateCentralDirectory(zip, &info, prefix))
	{
		//uh oh... the central directory wasn't where it was meant to be!
		//assuming that the endofcentraldir is packed at the true end of the centraldir (and that we're not zip64 and thus don't have an extra block), then we can guess based upon the offset difference
		info.zipoffset = info.centraldir_end - (info.centraldir_offset+info.centraldir_size);
		if (!FSZIP_EnumerateCentralDirectory(zip, &info, prefix))
		{
			Z_Free(zip);
			Con_TPrintf ("zipfile \"%s\" appears to be missing its central directory\n", desc);
			return NULL;
		}
	}

	zip->thisdisk = info.thisdisk;
	if (info.diskcount > 1)
	{
		if (parent && filename)
		{
			zipfile_t *szip;
			unsigned int s;
			flocation_t loc;
			char splitname[MAX_OSPATH];
			char *ext;
			zip->numspans = info.diskcount;
			zip->spans = Z_Malloc(info.diskcount*sizeof(*zip->spans));
			for(s = 0; s < zip->numspans; s++)
			{
				if (info.thisdisk == s)
					continue;	//would be weird.

				Q_strncpyz(splitname, filename, sizeof(splitname));
				ext = strrchr(splitname, '.');
				if (ext)
				{
					ext++;
					if (*ext)
						ext++;	//skip the first letter of the extension
				}
				else
					ext = splitname + strlen(splitname);
				Q_snprintfz(ext, sizeof(splitname)-(ext-splitname), *ext?"%02u":"%03u", s+1);
				if (parent->FindFile(parent, &loc, splitname, NULL) != FF_FOUND)
					continue;
				packhandle = parent->OpenVFS(parent, &loc, "rb");
				if (!packhandle)
					continue;

				zip->spans[s] = szip = Z_Malloc(sizeof(zipfile_t));
				szip->thisdisk = s;
				Q_strncpyz(szip->filename, splitname, sizeof(szip->filename));
				szip->raw = packhandle;
				szip->rawsize = VFS_GETLEN(szip->raw);
				szip->references = 1;
				szip->mutex = Sys_CreateMutex();
				szip->pub.ClosePath			= FSZIP_ClosePath;
			}
		}
		else
			Con_TPrintf ("spanned zip \"%s\" with no path info\n", desc);
	}

	zip->references = 1;
	zip->mutex = Sys_CreateMutex();

//	Con_TPrintf ("Added zipfile %s (%i files)\n", desc, zip->numfiles);

	zip->pub.fsver				= FSVER;
	zip->pub.GetPathDetails		= FSZIP_GetPathDetails;
	zip->pub.ClosePath			= FSZIP_ClosePath;
	zip->pub.AddReference		= FSZIP_UnclosePath;
	zip->pub.BuildHash			= FSZIP_BuildHash;
	zip->pub.FindFile			= FSZIP_FLocate;
	zip->pub.ReadFile			= FSZIP_ReadFile;
	zip->pub.EnumerateFiles		= FSZIP_EnumerateFiles;
	zip->pub.FileStat			= FSZIP_FileStat;
	zip->pub.GeneratePureCRC	= FSZIP_GeneratePureCRC;
	zip->pub.OpenVFS			= FSZIP_OpenVFS;
	return &zip->pub;
}





#if 0
//our download protocol permits requesting various different parts of the file.
//this means that its theoretically possible to download sections pk3s such that we can skip blocks that contain files that we already have.

typedef struct
{
	struct archivedeltafuncs_s
	{
		//called when the downloader needs to know a new target region. each call should return a new region, eventually returning the entire file as data is injected from elsewhere
		//return false on error.
		qboolean (*GetNextRange) (struct archivedeltafuncs_s *archive, qofs_t *start, qofs_t *end);

		//called when the download completes/aborts. frees memory.
		void (*Finish) (struct archivedeltafuncs_s);
	} pub;

	qdownload_t *dl;

	enum
	{
		step_eocd,
		step_cd,
		step_file
	} step;

	struct
	{
		qofs_t start;
		qofs_t end;
	} eocd;

	zipfile_t *old;

	zipfile_t zip;
	struct zipinfo info;
} ziparchivedelta_t;

void FSZIPDL_Finish(struct archivedeltafuncs_s *archive)
{
	ziparchivedelta_t *za = (ziparchivedelta_t*)archive;

	za->old->pub.ClosePath(&za->old->pub);

	if (za->zip.files)
		Z_Free(za->zip.files);

	Z_Free(za);
}
qboolean FSZIPDL_GetNextRange(struct archivedeltafuncs_s *archive, qofs_t *start, qofs_t *end)
{
	flocation_t loc;
	int i;
	ziparchivedelta_t *za = (ziparchivedelta_t*)archive;
	switch(za->step)
	{
	case step_eocd:
		//find the header (actually, the trailer)
		*start = za->eocd.start;
		*end = za->eocd.end;
		za->step++;
		break;
	case step_cd:
		if (!FSZIP_FindEndCentralDirectory(&za->zip, &za->info))
			return false;	//corrupt/unusable.

		//central directory should be located at this position
		*start = za->info.centraldir_offset+za->info.zipoffset;
		*end = za->info.centraldir_end+za->info.zipoffset;
		za->step++;
		break;
	case step_file:
		if (FSZIP_EnumerateCentralDirectory(&za->zip, &za->info))
		{
			return false;	//couldn't find the central directory properly (fixme: this can happen with zip32 self-extractors)
		}

		//walk through the central directory looking for matching files in the existing versions of the zip
		for (i = 0; i < za->zip.numfiles; i++)
			if (FSZIP_FLocate(&za->old->pub, &loc, za->zip.files[i].name, NULL) != FF_NOTFOUND)
			{
				if (za->old->files[loc.index].crc == za->zip.files[i].crc)
				{
					qofs_t datastart, datasize;
					if (FSZIP_ValidateLocalHeader(za->old, &za->old->files[loc.index], &datastart, &datasize))
					{
						size_t chunk;
						datastart = za->old->files[loc.index].localpos;

						//tell the download context that we already know this data region
						DL_Completed(za->dl, datastart, datastart + datasize);
						//and inject the data into the downloaded file.
						VFS_SEEK(za->old->raw, datastart);
						VFS_SEEK(za->dl->file, za->zip.files[i].localpos);
						for(;datasize;)
						{
							char copybuffer[0x10000];
							chunk = datasize;
							if (chunk > sizeof(copybuffer))
								chunk = sizeof(copybuffer);

							VFS_READ(za->old->raw, copybuffer, chunk);
							datasize -= chunk;
						}
						//FIXME: graphics/progress updates.
					}
				}
			}

		//anything we didn't receive yet needs to be completed. the download code is meant to track the holes.
		*start = 0;
		*end = za->eocd.end;
		za->step++;
		break;
	default:
		return false;
	}
	return true;
}
struct archivedeltafuncs_s *FSZIP_OpenDeltaZip(qdownload_t *dl)
{
	ziparchivedelta_t *za;
	unsigned int trailsize = 0x10000 + SIZE_ENDOFCENTRALDIRECTORY+SIZE_ZIP64ENDOFCENTRALDIRECTORYLOCATOR;
	za->eocd.end = dl->size;
	if (dl->size < trailsize)
		za->eocd.start = 0;
	else
		za->eocd.start = dl->size - trailsize;

	za->dl = dl;
	za->zip.rawsize = dl->size;
	za->zip.raw = dl->file;
	za->pub.GetNextRange = FSZIPDL_GetNextRange;
	return &za->pub;
}
#endif
#endif

