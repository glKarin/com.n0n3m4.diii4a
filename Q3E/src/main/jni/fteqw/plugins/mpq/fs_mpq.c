#include "quakedef.h"
#include "../plugin.h"
//#include "../engine.h"
#include "fs.h"
#include <assert.h>

#ifdef AVAIL_ZLIB
	#include <zlib.h>
#endif
#include "blast.h"

//http://bazaar.launchpad.net/~jeanfrancois.roy/mpqkit/trunk/files
//http://www.zezula.net/en/mpq/main.html
//http://www.wc3c.net/tools/specs/QuantamMPQFormat.txt

static size_t activempqcount; //number of active archives. we can't unload the dll while we still have files open.
#ifdef MULTITHREAD
static plugthreadfuncs_t *threading;
#define Sys_CreateMutex() (threading?threading->CreateMutex():NULL)
#define Sys_LockMutex if(threading)threading->LockMutex
#define Sys_UnlockMutex if(threading)threading->UnlockMutex
#define Sys_DestroyMutex if(threading)threading->DestroyMutex
#endif

typedef unsigned long long ofs_t;

typedef struct
{
	char mpq_magic[4];
	unsigned int header_size;
	unsigned int archive_size;
	unsigned short version;
	unsigned short sector_size_shift;
	unsigned int hash_table_offset;
	unsigned int block_table_offset;
	unsigned int hash_table_length;
	unsigned int block_table_length;
} mpqheader_t;

enum
{
	MPQFileValid				= 0x80000000,
	MPQFileHasSectorAdlers		= 0x04000000,
	MPQFileStopSearchMarker		= 0x02000000,
	MPQFileOneSector			= 0x01000000,
	MPQFilePatch				= 0x00100000,
	MPQFileOffsetAdjustedKey	= 0x00020000,
	MPQFileEncrypted			= 0x00010000,
	MPQFileCompressed			= 0x00000200,
	MPQFileDiabloCompressed		= 0x00000100,
	MPQFileFlagsMask			= 0x87030300
};
typedef struct
{
	unsigned int offset;
	unsigned int archived_size;
	unsigned int size;
	unsigned int flags;
} mpqblock_t;

typedef struct
{
	unsigned int hash_a;
	unsigned int hash_b;
	unsigned short locale;
	unsigned short platform;
	unsigned int block_table_index;
} mpqhash_t;

typedef struct 
{
	searchpathfuncs_t pub;

	char desc[MAX_OSPATH];
	void *mutex;
	vfsfile_t *file;
	ofs_t filestart;
	ofs_t fileend;

	unsigned int references;

	unsigned int sectorsize;

	mpqhash_t *hashdata;
	unsigned int hashentries;

	mpqblock_t *blockdata;
	unsigned int blockentries;

	char *listfile;

	mpqheader_t header_0;
	struct
	{
		unsigned long long extended_block_offset_table_offset;
		unsigned short hash_table_offset_high;
		unsigned short block_table_offset_high;
	} header_1;
} mpqarchive_t;

typedef struct
{
	vfsfile_t funcs;
	unsigned int flags;
	unsigned int encryptkey;
	mpqarchive_t *archive;
	qofs_t foffset;
	qofs_t flength;	//decompressed size
	qofs_t alength;	//size on disk

	ofs_t archiveoffset;

	unsigned int buffersect;
	unsigned int bufferlength;
	char *buffer;
	unsigned int *sectortab;
} mpqfile_t;


static qboolean crypt_table_initialized = false;
static unsigned int crypt_table[0x500];

static void mpq_init_cryptography(void)
{
	// prepare crypt_table
	unsigned int seed   = 0x00100001;
	unsigned int index1 = 0;
	unsigned int index2 = 0;
	unsigned int i;

	if (!crypt_table_initialized)
	{
		crypt_table_initialized = true;

		for (index1 = 0; index1 < 0x100; index1++)
		{
			for (index2 = index1, i = 0; i < 5; i++, index2 += 0x100)
			{
				unsigned int temp1, temp2;

				seed  = (seed * 125 + 3) % 0x2AAAAB;
				temp1 = (seed & 0xFFFF) << 0x10;

				seed  = (seed * 125 + 3) % 0x2AAAAB;
				temp2 = (seed & 0xFFFF);

				crypt_table[index2] = (temp1 | temp2);
			}
		}
	}
}

#define HASH_POSITION 0
#define HASH_NAME_A 1
#define HASH_NAME_B 2
#define HASH_KEY 3
static unsigned int mpq_hash_cstring(const char *string, unsigned int type)
{
	unsigned int seed1 = 0x7FED7FED;
	unsigned int seed2 = 0xEEEEEEEE;
	unsigned int shifted_type = (type << 8);
	unsigned int ch;

	assert(crypt_table_initialized);
	assert(string);

	while (*string != 0)
	{
		ch = *string++;
		if (ch == '/')
			ch = '\\';
		if (ch > 0x60 && ch < 0x7b) ch -= 0x20;

		seed1 = crypt_table[shifted_type + ch] ^ (seed1 + seed2);
		seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
	}

	return seed1;
}

#define MPQSwapInt32LittleToHost(a) (a)
#define MPQSwapInt32HostToLittle(a) (a)

static void mpq_decrypt(void* data, size_t length, unsigned int key, qboolean disable_output_swapping)
{
	unsigned int* buffer32 = (unsigned int*)data;
	unsigned int seed = 0xEEEEEEEE;
	unsigned int ch;

	assert(crypt_table_initialized);
	assert(data);

	// round to 4 bytes
	length = length / 4;

	if (disable_output_swapping)
	{
		while (length-- > 0)
		{
			ch = MPQSwapInt32LittleToHost(*buffer32);

			seed += crypt_table[0x400 + (key & 0xFF)];
			ch = ch ^ (key + seed);

			key = ((~key << 0x15) + 0x11111111) | (key >> 0x0B);
			seed = ch + seed + (seed << 5) + 3;

			*buffer32++ = ch;
		}
	}
	else
	{
		while (length-- > 0)
		{
			ch = MPQSwapInt32LittleToHost(*buffer32);

			seed += crypt_table[0x400 + (key & 0xFF)];
			ch = ch ^ (key + seed);

			key = ((~key << 0x15) + 0x11111111) | (key >> 0x0B);
			seed = ch + seed + (seed << 5) + 3;

			*buffer32++ = MPQSwapInt32HostToLittle(ch);
		}
	}
}

#define HASH_TABLE_EMPTY 0xffffffff
#define HASH_TABLE_DELETED 0xfffffffe
static unsigned int mpq_lookuphash(mpqarchive_t *mpq, const char *filename, int locale)
{
	unsigned int initial_position = mpq_hash_cstring(filename, HASH_POSITION) % mpq->hashentries;
	unsigned int current_position = initial_position;
	unsigned int hash_a = mpq_hash_cstring(filename, HASH_NAME_A);
	unsigned int hash_b = mpq_hash_cstring(filename, HASH_NAME_B);

	// Search through the hash table until we either find the file we're looking for, or we find an unused hash table entry, 
	// indicating the end of the cluster of used hash table entries
	while (mpq->hashdata[current_position].block_table_index != HASH_TABLE_EMPTY)
	{
		if (mpq->hashdata[current_position].block_table_index != HASH_TABLE_DELETED)
		{
			if (mpq->hashdata[current_position].hash_a == hash_a && 
				mpq->hashdata[current_position].hash_b == hash_b && 
				mpq->hashdata[current_position].locale == locale)
			{
				return current_position;
			}
		}

		current_position++;
		current_position %= mpq->hashentries;

		//avoid infinity
		if (current_position == initial_position)
			break;
	}

	return HASH_TABLE_EMPTY;
}

static vfsfile_t *MPQ_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode);
static void	MPQ_ClosePath(searchpathfuncs_t *handle)
{
	mpqarchive_t *mpq = (void*)handle;
	Sys_LockMutex(mpq->mutex);
	if (--mpq->references)
	{
		Sys_UnlockMutex(mpq->mutex);
		return;
	}
	Sys_UnlockMutex(mpq->mutex);
	Sys_DestroyMutex(mpq->mutex);
	VFS_CLOSE(mpq->file);
	free(mpq->blockdata);
	free(mpq->hashdata);
	free(mpq->listfile);
	free(mpq);
	activempqcount--;
}
static unsigned int MPQ_FindFile(searchpathfuncs_t *handle, flocation_t *loc, const char *name, void *hashedresult)
{
	mpqarchive_t *mpq = (void*)handle;
	unsigned int blockentry;

	if (hashedresult)
	{
		mpqblock_t *block = hashedresult;
		if (block >= mpq->blockdata && block <= mpq->blockdata + mpq->blockentries)
			blockentry = (mpqblock_t*)block - mpq->blockdata;
		else
			return FF_NOTFOUND;
	}
	else
	{
		unsigned int hashentry = mpq_lookuphash(mpq, name, 0);
		if (hashentry == HASH_TABLE_EMPTY)
			return FF_NOTFOUND;
		blockentry = mpq->hashdata[hashentry].block_table_index;
		if (blockentry > mpq->blockentries)
			return FF_NOTFOUND;
	}
	if (loc)
	{
		loc->fhandle = &mpq->blockdata[blockentry];
		loc->offset = 0;
		*loc->rawname = 0;
		loc->len = mpq->blockdata[blockentry].size;
//		loc->foo = foo;
	}

	if (mpq->blockdata[blockentry].flags & MPQFilePatch)
	{
		Con_DPrintf("Cannot cope with patch files\n");
		return FF_NOTFOUND;
	}
	return FF_FOUND;
}
static void	MPQ_ReadFile(searchpathfuncs_t *handle, flocation_t *loc, char *buffer)
{
	vfsfile_t *f;
	f = MPQ_OpenVFS(handle, loc, "rb");
	if (!f)	//err...
		return;
	VFS_READ(f, buffer, loc->len);
	VFS_CLOSE(f);
}

static int mpqwildcmp(const char *wild, const char *string, char **end)
{
	int s, w;
	while (*string)
	{
		s = *string;
		if (s == '\r' || s == '\n' || s == ';')
			break;
		w = *wild;

		if (s >= 'A' && s <= 'Z')
			s = s-'A'+'a';
		if (w >= 'A' && w <= 'Z')
			w = w-'A'+'a';

		if (w == '*')
		{
			w = wild[1];
			if (w >= 'A' && w <= 'Z')
				w = w-'A'+'a';
			if (w == s || s == '/' || s == '\\')
			{
				//* terminates if we get a match on the char following it, or if its a \ or / char
				wild++;
				continue;
			}
			string++;
		}
		else if ((w == s) || (w == '?'))
		{
			//this char matches
			wild++;
			string++;
		}
		else
		{
			//failure
			while (*string && *string != '\r' && *string != '\n' && *string != ';')
				string++;
			*end = (char*)string;
			return false;
		}
	}
	*end = (char*)string;

	while (*wild == '*')
	{
		wild++;
	}
	return !*wild;
}
static int MPQ_EnumerateFiles(searchpathfuncs_t *handle, const char *match, int (QDECL *func)(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath), void *parm)
{
	int ok = 1;
	char *s, *n;
	char name[MAX_QPATH];
	flocation_t loc;
	mpqarchive_t *mpq = (mpqarchive_t*)handle;
	if (mpq->listfile)
	{
		s = mpq->listfile;
		for (s = mpq->listfile; *s && ok; s = n)
		{
			if (mpqwildcmp(match, s, &n) && n - s < MAX_QPATH-1)
			{
				memcpy(name, s, n - s);
				name[n-s] = 0;

				if (!MPQ_FindFile(handle, &loc, name, NULL))
					loc.len = 0;
				ok = func(name, loc.len, 0, parm, handle);
			}

			while (*n == '\n' || *n == '\r' || *n == ';')
				n++;
		}
	}
	return ok;
}
static void	MPQ_BuildHash(searchpathfuncs_t *handle, int depth, void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle))
{
	char *s, *n;
	char name[MAX_QPATH];
	mpqarchive_t *mpq = (void*)handle;
	flocation_t loc;
	if (mpq->listfile)
	{
		s = mpq->listfile;
		for (s = mpq->listfile; ; s = n)
		{
			while (*s == '\n' || *s == '\r' || *s == ';')
				s++;
			if (!*s)
				break;
			n = s;
			while (*n && *n != '\r' && *n != '\n' && *n != ';')
				n++;

			if (n-s >= sizeof(name))
				continue;

			memcpy(name, s, n - s);
			name[n-s] = 0;
			//precompute the name->block lookup. fte normally does the hashing outside the archive code.
			//however, its possible multiple hash tables point to a single block, so we need to pass null for the third arg (or allocate fsbucket_ts one per hash instead of buckets).
			if (MPQ_FindFile(&mpq->pub, &loc, name, NULL))
				AddFileHash(depth, name, NULL, loc.fhandle);
		}
	}
}

static int		MPQ_GeneratePureCRC (searchpathfuncs_t *handle, const int *seed)
{
	return 0;
}

static qboolean	MPQ_PollChanges(searchpathfuncs_t *handle)
{
	return false;
}

static searchpathfuncs_t	*MPQ_OpenArchive(vfsfile_t *file, const char *desc, const char *prefix)
{
	flocation_t lloc;
	mpqarchive_t *mpq;
	mpqheader_t header;
	ofs_t block_ofs;
	ofs_t hash_ofs;

	VFS_SEEK(file, 0);

	if (prefix && *prefix)
		return NULL;	//not supported at this time

	if (VFS_READ(file, &header, sizeof(header)) != sizeof(header))
		return NULL;

	if (memcmp(header.mpq_magic, "MPQ\x1a", 4))
		return NULL;

	mpq = malloc(sizeof(*mpq));
	memset(mpq, 0, sizeof(*mpq));
	Q_strlcpy(mpq->desc, desc, sizeof(mpq->desc));
	mpq->header_0 = header;
	mpq->file = file;
	mpq->filestart = 0;
	mpq->sectorsize = 512 << mpq->header_0.sector_size_shift;

	block_ofs = header.block_table_offset;
	hash_ofs = header.hash_table_offset;
	if (header.version >= 1)
	{
		VFS_READ(file, &mpq->header_1, sizeof(mpq->header_1));
	}
	block_ofs |= ((ofs_t)mpq->header_1.block_table_offset_high)<<32u;
	hash_ofs |= ((ofs_t)mpq->header_1.hash_table_offset_high)<<32u;

	mpq->fileend = VFS_GETLEN(file);

	if (block_ofs + sizeof(*mpq->blockdata) * mpq->blockentries > mpq->fileend ||
		hash_ofs + sizeof(*mpq->hashdata) * mpq->hashentries > mpq->fileend)
	{
		Con_Printf("\"%s\" appears truncated\n", desc);
		free(mpq);
		return NULL;
	}


	mpq->hashentries = mpq->header_0.hash_table_length;
	mpq->hashdata = malloc(sizeof(*mpq->hashdata) * mpq->hashentries);
	VFS_SEEK(file, hash_ofs);
	VFS_READ(file, mpq->hashdata, sizeof(*mpq->hashdata) * mpq->hashentries);
	mpq_decrypt(mpq->hashdata, sizeof(*mpq->hashdata) * mpq->hashentries, mpq_hash_cstring("(hash table)", HASH_KEY), false);

	mpq->blockentries = mpq->header_0.block_table_length;
	mpq->blockdata = malloc(sizeof(*mpq->blockdata) * mpq->blockentries);
	VFS_SEEK(file, block_ofs);
	VFS_READ(file, mpq->blockdata, sizeof(*mpq->blockdata) * mpq->blockentries);
	mpq_decrypt(mpq->blockdata, sizeof(*mpq->blockdata) * mpq->blockentries, mpq_hash_cstring("(block table)", HASH_KEY), true);

	/*for (i = 0; i < mpq->header_0.block_table_length; i++)
	{
		Con_Printf("offset = %08x, csize = %i, usize=%i, flags=%s%s%s%s%s%s%s%s%s\n", mpq->blockdata[i].offset, mpq->blockdata[i].archived_size, mpq->blockdata[i].size, 
				(mpq->blockdata[i].flags & MPQFileValid)?"valid ":"",
				(mpq->blockdata[i].flags & MPQFileHasSectorAdlers)?"sectoradlers ":"",
				(mpq->blockdata[i].flags & MPQFileStopSearchMarker)?"stopsearch ":"",
				(mpq->blockdata[i].flags & MPQFileOneSector)?"singlesector ":"",
				(mpq->blockdata[i].flags & MPQFileOffsetAdjustedKey)?"offsetadjust ":"",
				(mpq->blockdata[i].flags & MPQFileEncrypted)?"encrypted ":"",
				(mpq->blockdata[i].flags & MPQFileCompressed)?"compressed ":"",
				(mpq->blockdata[i].flags & MPQFileDiabloCompressed)?"dcompressed ":"",
				(mpq->blockdata[i].flags & ~MPQFileFlagsMask)?"OTHERS ":""
			);
	}*/

	activempqcount++;
	mpq->references = 1;
	mpq->mutex = Sys_CreateMutex();

	if (MPQ_FindFile(&mpq->pub, &lloc, "(listfile)", NULL))
	{
		char *bs;
		mpq->listfile = malloc(lloc.len+2);
		mpq->listfile[0] = 0;
		mpq->listfile[lloc.len] = 0;
		mpq->listfile[lloc.len+1] = 0;
		MPQ_ReadFile(&mpq->pub, &lloc, mpq->listfile);
		bs = mpq->listfile;
		while(1)
		{
			bs = strchr(bs, '\\');
			if (bs)
				*bs++ = '/';
			else
				break;
		}
	}

	mpq->pub.fsver				= FSVER;
	mpq->pub.ClosePath			= MPQ_ClosePath;
	mpq->pub.BuildHash			= MPQ_BuildHash;
	mpq->pub.FindFile			= MPQ_FindFile;
	mpq->pub.ReadFile			= MPQ_ReadFile;
	mpq->pub.EnumerateFiles		= MPQ_EnumerateFiles;
	mpq->pub.GeneratePureCRC	= MPQ_GeneratePureCRC;
	mpq->pub.OpenVFS			= MPQ_OpenVFS;
	mpq->pub.PollChanges		= MPQ_PollChanges;

	return &mpq->pub;
}


struct blastdata_s
{
	void *outdata;
	unsigned int outlen;
	void *indata;
	unsigned int inlen;
};
static unsigned mpqf_blastin(void *how, unsigned char **buf)
{
	struct blastdata_s *args = how;
	*buf = args->indata;
	return args->inlen;
}
static int mpqf_blastout(void *how, unsigned char *buf, unsigned len)
{
	struct blastdata_s *args = how;
	if (len > args->outlen)
		return 1;
	memcpy(args->outdata, buf, len);
	args->outdata = (char*)args->outdata + len;
	args->outlen -= len;
	return 0;
}
static void MPQF_decompress(qboolean legacymethod, void *outdata, unsigned int outlen, void *indata, unsigned int inlen)
{
#ifdef AVAIL_ZLIB
	int ret;
#endif

	int methods;
	if (legacymethod)
		methods = 8;
	else
	{
		methods = *(unsigned char*)indata;
		indata = (char*)indata + 1;
		inlen--;
	}

	if (methods == 8)
	{
		struct blastdata_s args = {outdata, outlen, indata, inlen};
		blast(mpqf_blastin, &args, mpqf_blastout, &args);
	}
#ifdef AVAIL_ZLIB
	else if (methods == 2)
	{
		z_stream strm =
		{
			indata,
			inlen,
			0,

			outdata,
			outlen,
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

		inflateInit2(&strm, MAX_WBITS);

		while ((ret=inflate(&strm, Z_SYNC_FLUSH)) != Z_STREAM_END)
		{
			if (strm.avail_in == 0 || strm.avail_out == 0)
			{
				if (strm.avail_in == 0)
				{
					break;
				}

				if (strm.avail_out == 0)
				{
					break;
				}
				continue;
			}

			//doh, it terminated for no reason
			if (ret != Z_STREAM_END)
			{
				inflateEnd(&strm);
				Con_Printf("Couldn't decompress gz file\n");
				return;
			}
		}

		inflateEnd(&strm);
	}
#endif
	else
	{
		Con_Printf("mpq: unsupported decompression method - %x\n", methods);
		memset(outdata, 0, outlen);
	}
}

static int MPQF_readbytes (struct vfsfile_s *file, void *buffer, int bytestoread)
{
	int bytesread = 0;
	mpqfile_t *f = (mpqfile_t *)file;

	if (bytestoread + f->foffset > f->flength)
		bytestoread = f->flength - f->foffset;
	if (bytestoread < 0)
		return 0;

	if (!(f->flags & (MPQFileCompressed|MPQFileDiabloCompressed)))
	{
		//no compression, just a raw file.
		Sys_LockMutex(f->archive->mutex);
		VFS_SEEK(f->archive->file, f->archiveoffset + f->foffset);
		bytesread = VFS_READ(f->archive->file, buffer, bytestoread);
		Sys_UnlockMutex(f->archive->mutex);
		f->foffset += bytesread;
	}
	else if (f->flags & MPQFileOneSector)
	{
		//fairly simple packed data, no sector nonsense. decode in one go
		if (!f->buffer)
		{
			char *cdata = malloc(f->alength);
			f->buffer = malloc(f->flength);
			Sys_LockMutex(f->archive->mutex);
			VFS_SEEK(f->archive->file, f->archiveoffset);
			VFS_READ(f->archive->file, cdata, f->alength);
			Sys_UnlockMutex(f->archive->mutex);

			if (f->flags & MPQFileEncrypted)
			{
				mpq_decrypt(cdata, f->alength, f->encryptkey, false);
			}
			if (f->flags & (MPQFileCompressed|MPQFileDiabloCompressed))
			{
				//decompress
				MPQF_decompress(!!(f->flags&MPQFileDiabloCompressed), f->buffer, f->flength, cdata, f->alength);
			}
			else
			{
				//lazy...
				memcpy(f->buffer, cdata, f->flength);
			}
			free(cdata);
		}
		memcpy((char*)buffer+bytesread, f->buffer + f->foffset, bytestoread);
		f->foffset += bytestoread;
		bytesread += bytestoread;
	}
	else
	{
		//sectors are weird.
		//sectors are allocated for decompressed size, not compressed. I have no idea how this works.
		//time to find out.
		for (;;)
		{
			int numsects = (f->flength + (f->archive->sectorsize) - 1) / f->archive->sectorsize;
			int sectidx = f->foffset / f->archive->sectorsize;
			qboolean lastsect = false;
			int chunkofs, chunklen;
			if (sectidx >= numsects-1)
			{
				lastsect = true;
				sectidx = numsects-1;
			}

			if (sectidx != f->buffersect || !f->buffer)
			{
				int rawsize;
				char *cdata;
				f->buffersect = sectidx;
				if (!f->sectortab)
				{
					f->sectortab = malloc((numsects+1) * sizeof(*f->sectortab));
					if (!f->sectortab)
						plugfuncs->Error("out of memory");
					Sys_LockMutex(f->archive->mutex);
					VFS_SEEK(f->archive->file, f->archiveoffset);
					VFS_READ(f->archive->file, f->sectortab, (numsects+1) * sizeof(*f->sectortab));
					Sys_UnlockMutex(f->archive->mutex);

					if (f->flags & MPQFileEncrypted)
						mpq_decrypt(f->sectortab, (numsects+1) * sizeof(*f->sectortab), f->encryptkey-1, true);
				}
				//data is packed, sector table gives offsets. there's an extra index on the end which is the size of the last sector.
				rawsize = f->sectortab[sectidx+1]-f->sectortab[sectidx];

				cdata = malloc(rawsize);
				if (!cdata)
					plugfuncs->Error("out of memory");
				if (!f->buffer)
					f->buffer = malloc(f->archive->sectorsize);
				if (!f->buffer)
					plugfuncs->Error("out of memory");
				Sys_LockMutex(f->archive->mutex);
				VFS_SEEK(f->archive->file, f->archiveoffset + f->sectortab[sectidx]);
				VFS_READ(f->archive->file, cdata, rawsize);
				Sys_UnlockMutex(f->archive->mutex);

				if (lastsect)
					f->bufferlength = f->flength - ((numsects-1)*f->archive->sectorsize);
				else
					f->bufferlength = f->archive->sectorsize;

				if (f->flags & MPQFileEncrypted)
					mpq_decrypt(cdata, rawsize, f->encryptkey+sectidx, false);
				if (f->flags & (MPQFileCompressed|MPQFileDiabloCompressed))
				{
					//decompress
					MPQF_decompress(!!(f->flags&MPQFileDiabloCompressed), f->buffer, f->bufferlength, cdata, rawsize);
				}
				else
				{
					//lazy...
					memcpy(f->buffer, cdata, f->bufferlength);
				}
				free(cdata);
			}

			chunkofs = (f->foffset%f->archive->sectorsize);
			chunklen = f->archive->sectorsize - chunkofs;
			if (chunklen > bytestoread)
				chunklen = bytestoread;
			bytestoread -= chunklen;
			memcpy((char*)buffer+bytesread, f->buffer + chunkofs, chunklen);
			f->foffset += chunklen;
			bytesread += chunklen;
			if (!chunklen || !bytestoread)
				break;
		}
	}

	return bytesread;
}
static int MPQF_writebytes (struct vfsfile_s *file, const void *buffer, int bytestoread)
{
//	mpqfile_t *f = (mpqfile_t *)file;
	return 0;
}
static qboolean MPQF_seek (struct vfsfile_s *file, qofs_t pos)
{
	mpqfile_t *f = (mpqfile_t *)file;
	if (pos > f->flength)
		return false;
	f->foffset = pos;
	return true;
}
static qofs_t MPQF_tell (struct vfsfile_s *file)
{
	mpqfile_t *f = (mpqfile_t *)file;
	return f->foffset;
}
static qofs_t MPQF_getlen (struct vfsfile_s *file)
{
	mpqfile_t *f = (mpqfile_t *)file;
	return f->flength;
}
static qboolean MPQF_close (struct vfsfile_s *file)
{
	mpqfile_t *f = (mpqfile_t *)file;
	if (f->buffer)
		free(f->buffer);
	if (f->sectortab)
		free(f->sectortab);

	MPQ_ClosePath(&f->archive->pub);

	free(f);

	return true;
}
static void MPQF_flush (struct vfsfile_s *file)
{
}

static qboolean MPQF_GetKey(unsigned int flags, unsigned int blockoffset, unsigned int blocksize, unsigned int *key)
{
	if (flags & MPQFileEncrypted)
	{
		*key = mpq_hash_cstring("(listfile)", HASH_KEY);

		if (flags & MPQFileOffsetAdjustedKey)
			*key = (*key + (unsigned int)(blockoffset)) ^ blocksize;
	}
	else
		*key = 0;

	return true;
}

static vfsfile_t *MPQ_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode)
{
	mpqarchive_t *mpq = (void*)handle;
	mpqblock_t *block = loc->fhandle;
	mpqfile_t *f;

	if (block->flags & MPQFilePatch)
	{
		Con_Printf("Cannot cope with patch files\n");
		return NULL;
	}

	f = malloc(sizeof(*f));
	f->buffer = NULL;
	f->buffersect = -1;
	f->sectortab = NULL;
	f->foffset = 0;
	f->archiveoffset = block->offset;
	MPQF_GetKey(block->flags, f->archiveoffset, block->size, &f->encryptkey);
	f->flags = block->flags;
	f->archive = mpq;
	f->flength = block->size;
	f->alength = block->archived_size;
	f->funcs.ReadBytes = MPQF_readbytes;
	f->funcs.WriteBytes = MPQF_writebytes;
	f->funcs.Seek = MPQF_seek;
	f->funcs.Tell = MPQF_tell;
	f->funcs.GetLen = MPQF_getlen;
	f->funcs.Close = MPQF_close;
	f->funcs.Flush = MPQF_flush;

	Sys_LockMutex(mpq->mutex);
	mpq->references++;
	Sys_UnlockMutex(mpq->mutex);

	return &f->funcs;
}

qboolean MPQ_MayShutdown(void)
{
	return activempqcount==0;
}

qboolean Plug_Init(void)
{
	mpq_init_cryptography();

#ifdef MULTITHREAD
	threading = plugfuncs->GetEngineInterface("Threading", sizeof(*threading));
#endif

	//we can't cope with being closed randomly. files cannot be orphaned safely.
	//so ask the engine to ensure we don't get closed before everything else is.
	plugfuncs->ExportFunction("MayShutdown", NULL);

	if (!plugfuncs->ExportFunction("FS_RegisterArchiveType_mpq", MPQ_OpenArchive))
	{
		Con_Printf("mpq: Engine doesn't support filesystem plugins\n");
		return false;
	}
	if (!plugfuncs->ExportFunction("FS_RegisterArchiveType_MPQ", MPQ_OpenArchive))
	{
		Con_Printf("mpq: Engine doesn't support filesystem plugins\n");
		return false;
	}

	return true;
}

