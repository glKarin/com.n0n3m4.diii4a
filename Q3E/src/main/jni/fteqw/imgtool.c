#include "quakedef.h"
#include "shader.h"
#undef stderr
#define stderr stdout

#include <limits.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#endif
static qboolean verbose;
void VARGS Sys_Error (const char *fmt, ...)
{
	va_list		argptr;

	va_start (argptr,fmt);
	vfprintf (stderr,fmt,argptr);
	va_end (argptr);
	fflush(stderr);

	exit(1);
}
void VARGS Con_Printf (const char *fmt, ...)
{
	va_list		argptr;

	va_start (argptr,fmt);
	vfprintf (stderr,fmt,argptr);
	va_end (argptr);
	fflush(stderr);
}
void VARGS Con_TPrintf (const char *fmt, ...)
{
	va_list		argptr;

	va_start (argptr,fmt);
	vfprintf (stderr,fmt,argptr);
	va_end (argptr);
	fflush(stderr);
}
void VARGS Con_DPrintf (const char *fmt, ...)
{
	va_list		argptr;
	if (!verbose)
		return;

	va_start (argptr,fmt);
	vfprintf (stderr,fmt,argptr);
	va_end (argptr);
	fflush(stderr);
}
void VARGS Con_ThrottlePrintf (float *timer, int developerlevel, const char *fmt, ...)
{
	va_list		argptr;

	va_start (argptr,fmt);
	vfprintf (stderr,fmt,argptr);
	va_end (argptr);
	fflush(stderr);
}

void *ZF_Malloc(size_t size)
{
#if defined(__linux__)
	void *ret = NULL;
	if (!posix_memalign(&ret, max(sizeof(float)*4, sizeof(void*)), size))
		memset(ret, 0, size);
	return ret;
#else
	return calloc(size, 1);
#endif
}
void *Z_Malloc(size_t size)
{
	void *r = ZF_Malloc(size);
	if (!r)
		exit(1);
	return r;
}
void *BZ_Malloc(size_t size)
{
	return Z_Malloc(size);
}
void *BZF_Malloc(size_t size)
{
	return Z_Malloc(size);
}
void BZ_Free(void *p)
{
	free(p);
}
void Z_Free(void *p)
{
	free(p);
}
#ifdef _WIN32
// don't use these functions in MSVC8
#if (_MSC_VER < 1400)
int QDECL linuxlike_snprintf(char *buffer, int size, const char *format, ...)
{
#undef _vsnprintf
	int ret;
	va_list		argptr;

	if (size <= 0)
		return 0;
	size--;

	va_start (argptr, format);
	ret = _vsnprintf (buffer,size, format,argptr);
	va_end (argptr);

	buffer[size] = '\0';

	return ret;
}
int QDECL linuxlike_vsnprintf(char *buffer, int size, const char *format, va_list argptr)
{
#undef _vsnprintf
	int ret;

	if (size <= 0)
		return 0;
	size--;

	ret = _vsnprintf (buffer,size, format,argptr);

	buffer[size] = '\0';

	return ret;
}
#elif (_MSC_VER < 1900)
int VARGS linuxlike_snprintf_vc8(char *buffer, int size, const char *format, ...)
{
	int ret;
	va_list		argptr;

	va_start (argptr, format);
	ret = vsnprintf_s (buffer,size, _TRUNCATE, format,argptr);
	va_end (argptr);

	return ret;
}
#endif
#endif




#include <sys/stat.h>

void FS_CreatePath(const char *pname, enum fs_relative relativeto)
{
	char *t = strdup(pname), *sl = t;
	while ((sl=strchr(sl, '/')))
	{
		*sl=0;
#ifdef _WIN32
		CreateDirectoryA(t, NULL);
#else
		mkdir(t, 0777);
#endif
		*sl++='/';
	}
	free(t);
}
qboolean FS_Remove (const char *path, enum fs_relative relativeto)
{
	//remove is part of c89.
	if (remove(path) == -1)
		return false;
	return true;
}
qboolean FS_SystemPath(const char *fname, enum fs_relative relativeto, char *out, int outlen)
{
	Q_strncpyz(out, fname, outlen);
	return true;
}
char *COM_SkipPath (const char *pathname)
{
	const char	*last;

	last = pathname;
	while (*pathname)
	{
		if (*pathname=='/' || *pathname == '\\')
			last = pathname+1;
		pathname++;
	}
	return (char *)last;
}

#ifdef __unix__
#include <sys/stat.h>
#endif
void *FS_LoadMallocFile (const char *path, size_t *fsize)
{
	qbyte *data = NULL;
	FILE *f;
#ifdef __unix__
	struct stat sb;
	if (stat(path, &sb) < 0)
		return NULL;
	if ((sb.st_mode&S_IFMT) != S_IFREG)
		return NULL;
#endif

	f = fopen(path, "rb");
	if (f)
	{
		long int sz;
		if (fseek(f, 0, SEEK_END) >= 0)
		{
			sz = ftell(f);
			if (sz >= 0)
			{
				*fsize = sz;
				fseek(f, 0, SEEK_SET);
				data = ZF_Malloc(*fsize+1);
				if (data)
				{
					data[*fsize] = 0;
					fread(data, 1, *fsize, f);
				}
				else
					Con_Printf("Unable to allocate memory for %s\n", path);
			}
		}
		fclose(f);
	}
	return data;
}
#ifdef _WIN32
dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs)
{
	return NULL;
}
#else
#include <dlfcn.h>
void Sys_CloseLibrary(dllhandle_t *lib)
{
	dlclose((void*)lib);
}
dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs)
{
	int i;
	dllhandle_t *lib;

	lib = NULL;
	if (!lib)
		lib = dlopen (name, RTLD_LOCAL|RTLD_LAZY);
//	if (!lib && !strstr(name, ".so"))
//		lib = dlopen (va("%s.so", name), RTLD_LOCAL|RTLD_LAZY);
	if (!lib)
	{
//		Con_DPrintf("%s\n", dlerror());
		return NULL;
	}

	if (funcs)
	{
		for (i = 0; funcs[i].name; i++)
		{
			*funcs[i].funcptr = dlsym(lib, funcs[i].name);
			if (!*funcs[i].funcptr)
				break;
		}
		if (funcs[i].name)
		{
			Con_DPrintf("Unable to find symbol \"%s\" in \"%s\"\n", funcs[i].name, name);
			Sys_CloseLibrary((dllhandle_t*)lib);
			lib = NULL;
		}
	}

	return (dllhandle_t*)lib;
}
#endif

struct imgfile_s
{
	vfsfile_t pub;
	FILE *f;
};
static qboolean QDECL ImgFile_Close(struct vfsfile_s *file)
{
	struct imgfile_s *f = (struct imgfile_s*)file;
	fclose(f->f);
	free(f);
	return true;
}
static int QDECL ImgFile_WriteBytes(struct vfsfile_s *file, const void *buffer, int bytestowrite)
{
	struct imgfile_s *f = (struct imgfile_s*)file;
	return fwrite(buffer, 1, bytestowrite, f->f);
}
static qboolean QDECL ImgFile_Seek(struct vfsfile_s *file, qofs_t newofs)
{
	struct imgfile_s *f = (struct imgfile_s*)file;
	if (fseek(f->f, newofs, SEEK_SET)==0)
		return true;	//success
	return false;
}
static qofs_t QDECL ImgFile_Tell(struct vfsfile_s *file)
{
	struct imgfile_s *f = (struct imgfile_s*)file;
	return ftell(f->f);
}
vfsfile_t *QDECL FS_OpenVFS(const char *filename, const char *mode, enum fs_relative relativeto)
{
	if (!strcmp(mode, "wb"))
	{
		struct imgfile_s *r = malloc(sizeof(*r));
		r->f = fopen(filename, mode);
		r->pub.seekstyle = SS_UNSEEKABLE;
		r->pub.Close = ImgFile_Close;
		r->pub.WriteBytes = ImgFile_WriteBytes;
		r->pub.Seek = ImgFile_Seek;
		r->pub.Tell = ImgFile_Tell;
		if (r->f)
			return &r->pub;
		free(r);
	}
	return NULL;
}
qboolean COM_WriteFile (const char *filename, enum fs_relative fsroot, const void *data, int len)
{
	vfsfile_t *f = FS_OpenVFS(filename, "wb", fsroot);
	qboolean ret = false;
	if (f)
	{
		ret = len==VFS_WRITE(f, data, len);
		if (!VFS_CLOSE(f))
			ret = false;
	}
	return ret;
}
void QDECL Q_strncpyz(char *d, const char *s, int n)
{
	int i;
	n--;
	if (n < 0)
		return;	//this could be an error

	for (i=0; *s; i++)
	{
		if (i == n)
			break;
		*d++ = *s++;
	}
	*d='\0';
}

qboolean VARGS Q_vsnprintfz (char *dest, size_t size, const char *fmt, va_list argptr)
{
	size_t ret;
#ifdef _WIN32
	//doesn't null terminate.
	//returns -1 on truncation
	ret = _vsnprintf (dest, size, fmt, argptr);
	dest[size-1] = 0;	//shitty paranoia
#else
	//always null terminates.
	//returns length regardless of truncation.
	ret = vsnprintf (dest, size, fmt, argptr);
#endif
#ifdef _DEBUG
	if (ret>=size)
		Sys_Error("Q_vsnprintfz: Truncation\n");
#endif
	//if ret is -1 (windows oversize, or general error) then it'll be treated as unsigned so really long. this makes the following check quite simple.
	return ret>=size;
}

qboolean VARGS Q_snprintfz (char *dest, size_t size, const char *fmt, ...)
{
	va_list		argptr;
	qboolean ret;

	va_start (argptr, fmt);
	ret = Q_vsnprintfz(dest, size, fmt, argptr);
	va_end (argptr);
	return ret;
}

//palette data is used in lmps, as well as written into pcxes or wads, probably some other things.
qbyte		*host_basepal;
unsigned int	d_8to24rgbtable[256];
unsigned int	d_8to24bgrtable[256];
static qbyte default_quakepal[768] =
{	//the quake palette was released into the public domain (or at least gpl) to ease development of tools writing quake-format data.
0,0,0,15,15,15,31,31,31,47,47,47,63,63,63,75,75,75,91,91,91,107,107,107,123,123,123,139,139,139,155,155,155,171,171,171,187,187,187,203,203,203,219,219,219,235,235,235,15,11,7,23,15,11,31,23,11,39,27,15,47,35,19,55,43,23,63,47,23,75,55,27,83,59,27,91,67,31,99,75,31,107,83,31,115,87,31,123,95,35,131,103,35,143,111,35,11,11,15,19,19,27,27,27,39,39,39,51,47,47,63,55,55,75,63,63,87,71,71,103,79,79,115,91,91,127,99,99,
139,107,107,151,115,115,163,123,123,175,131,131,187,139,139,203,0,0,0,7,7,0,11,11,0,19,19,0,27,27,0,35,35,0,43,43,7,47,47,7,55,55,7,63,63,7,71,71,7,75,75,11,83,83,11,91,91,11,99,99,11,107,107,15,7,0,0,15,0,0,23,0,0,31,0,0,39,0,0,47,0,0,55,0,0,63,0,0,71,0,0,79,0,0,87,0,0,95,0,0,103,0,0,111,0,0,119,0,0,127,0,0,19,19,0,27,27,0,35,35,0,47,43,0,55,47,0,67,
55,0,75,59,7,87,67,7,95,71,7,107,75,11,119,83,15,131,87,19,139,91,19,151,95,27,163,99,31,175,103,35,35,19,7,47,23,11,59,31,15,75,35,19,87,43,23,99,47,31,115,55,35,127,59,43,143,67,51,159,79,51,175,99,47,191,119,47,207,143,43,223,171,39,239,203,31,255,243,27,11,7,0,27,19,0,43,35,15,55,43,19,71,51,27,83,55,35,99,63,43,111,71,51,127,83,63,139,95,71,155,107,83,167,123,95,183,135,107,195,147,123,211,163,139,227,179,151,
171,139,163,159,127,151,147,115,135,139,103,123,127,91,111,119,83,99,107,75,87,95,63,75,87,55,67,75,47,55,67,39,47,55,31,35,43,23,27,35,19,19,23,11,11,15,7,7,187,115,159,175,107,143,163,95,131,151,87,119,139,79,107,127,75,95,115,67,83,107,59,75,95,51,63,83,43,55,71,35,43,59,31,35,47,23,27,35,19,19,23,11,11,15,7,7,219,195,187,203,179,167,191,163,155,175,151,139,163,135,123,151,123,111,135,111,95,123,99,83,107,87,71,95,75,59,83,63,
51,67,51,39,55,43,31,39,31,23,27,19,15,15,11,7,111,131,123,103,123,111,95,115,103,87,107,95,79,99,87,71,91,79,63,83,71,55,75,63,47,67,55,43,59,47,35,51,39,31,43,31,23,35,23,15,27,19,11,19,11,7,11,7,255,243,27,239,223,23,219,203,19,203,183,15,187,167,15,171,151,11,155,131,7,139,115,7,123,99,7,107,83,0,91,71,0,75,55,0,59,43,0,43,31,0,27,15,0,11,7,0,0,0,255,11,11,239,19,19,223,27,27,207,35,35,191,43,
43,175,47,47,159,47,47,143,47,47,127,47,47,111,47,47,95,43,43,79,35,35,63,27,27,47,19,19,31,11,11,15,43,0,0,59,0,0,75,7,0,95,7,0,111,15,0,127,23,7,147,31,7,163,39,11,183,51,15,195,75,27,207,99,43,219,127,59,227,151,79,231,171,95,239,191,119,247,211,139,167,123,59,183,155,55,199,195,55,231,227,87,127,191,255,171,231,255,215,255,255,103,0,0,139,0,0,179,0,0,215,0,0,255,0,0,255,243,147,255,247,199,255,255,255,159,91,83
};
qbyte GetPaletteIndexNoFB(int red, int green, int blue)
{
	int i;
	int best=0;
	int bestdist=INT_MAX;
	int dist;
	for (i = 0; i < 256-32; i++)
	{
		const int diff[3] = {
			host_basepal[i*3+0]-red,
			host_basepal[i*3+1]-green,
			host_basepal[i*3+2]-blue};
		dist = DotProduct(diff,diff);
		if (dist < bestdist)
		{
			bestdist = dist;
			best = i;
			if (!dist)
				break;
		}
	}
	return best;
}
qbyte GetPaletteIndexRange(int first, int stop, int red, int green, int blue)
{
	int i;
	int best=0;
	int bestdist=INT_MAX;
	int dist;
	for (i = first; i < stop; i++)
	{
		const int diff[3] = {
			host_basepal[i*3+0]-red,
			host_basepal[i*3+1]-green,
			host_basepal[i*3+2]-blue};
		dist = DotProduct(diff,diff);
		if (dist < bestdist)
		{
			bestdist = dist;
			best = i;
			if (!dist)
				break;
		}
	}
	return best;
}

const char *palette = NULL;

sh_config_t sh_config;
viddef_t vid;

void ImgTool_SetupPalette(void)
{
	int i;
	FILE *fPAL;
	static qbyte cust_pal[768];

	host_basepal = default_quakepal;

	if (palette)
	{
		fPAL = fopen(palette, "rb");
	
		if (fPAL != NULL)
		{
			Con_Printf("using user-specified palette for files using palette.lmp\n");
			fread(cust_pal, 1, 768, fPAL);
			fclose(fPAL);
			host_basepal = cust_pal;
		}
		else
			Con_Printf("cannot find palette file %s\n", palette);
	}
	else
		Con_Printf("using built-in Quake palette for files using palette.lmp\n");

	for (i = 0; i < 256; i++)
	{
		d_8to24rgbtable[i] = (host_basepal[i*3+0]<<0)|(host_basepal[i*3+1]<<8)|(host_basepal[i*3+2]<<16);
		d_8to24bgrtable[i] = (host_basepal[i*3+0]<<16)|(host_basepal[i*3+1]<<8)|(host_basepal[i*3+2]<<0);
	}

	sh_config.texture2d_maxsize = 1u<<31;
	sh_config.texture3d_maxsize = 1u<<31;
	sh_config.texture2darray_maxlayers = 1u<<31;
	sh_config.texturecube_maxsize = 8192;
	sh_config.texture_non_power_of_two = true;
	sh_config.texture_non_power_of_two_pic = true;
	sh_config.texture_allow_block_padding = true;
	sh_config.npot_rounddown = true;	//shouldn't be relevant
	sh_config.havecubemaps = true;	//I don't think this matters.

	Image_Init();
}
#ifdef IMGTOOL
static void ImgTool_FreeMips(struct pendingtextureinfo *mips)
{
	size_t i;
	if (mips)
	{
		for (i = 0; i < mips->mipcount; i++)
			if (mips->mip[i].needfree)
				BZ_Free(mips->mip[i].data);
		if (mips->extrafree)
			BZ_Free(mips->extrafree);
		BZ_Free(mips);
	}
}

typedef struct
{
   unsigned int offset;				// Position of the entry in WAD
   unsigned int dsize;				// Size of the entry in WAD file
   unsigned int size;				// Size of the entry in memory
   char type;						// type of entry
   char cmprs;						// Compression. 0 if none.
   short dummy;						// Not used
   char name[16];					// we use only first 8
} wad2entry_t;
typedef struct
{
   char magic[4];					//should be WAD2
   unsigned int num;				//number of entries
   unsigned int offset;				//location of directory
} wad2_t;

static const char *imagetypename[] = {"2D", "3D", "Cube", "2DArray", "CubemapArray", "INVALID", "INVALID", "INVALID", "INVALID", "INVALID"};
struct opts_s
{
	int textype;
	const char *defaultext;		//.dds or whatever when the output's extension is not explicitly given.
	unsigned int flags;			//image flags to use (affects how textures get interpreted a little)
	unsigned int mipnum;		//when exporting to a mipless format, this is the mip level that is actually written. default 0.
	uploadfmt_t newpixelformat;	//try to convert to this pixel format on export.

	int width, height;
};

static qboolean ImgTool_MipExport(struct opts_s *args, vfsfile_t *outfile, struct pendingtextureinfo *in, const char *mipname, int wadtype);
static struct pendingtextureinfo *ImgTool_DecodeMiptex(struct opts_s *args, miptex_t *mip, size_t fsize, qbyte *basepal);
void Image_GenerateMips(struct pendingtextureinfo *mips, unsigned int flags);
int Image_WritePNG (const char *filename, enum fs_relative fsroot, int compression, void **buffers, int numbuffers, qintptr_t bufferstride, int width, int height, enum uploadfmt fmt, qboolean writemetadata);
qboolean WriteTGA(const char *filename, enum fs_relative fsroot, const qbyte *fte_restrict rgb_buffer, qintptr_t bytestride, int width, int height, enum uploadfmt fmt);

static enum uploadfmt ImgTool_ASTCToLDR(uploadfmt_t fmt)
{
	if (fmt >= PTI_ASTC_FIRST && fmt <= PTI_ASTC_LAST)
	{
		if (fmt >= PTI_ASTC_4X4_HDR)
			return (fmt-PTI_ASTC_4X4_HDR)+PTI_ASTC_4X4_LDR;
		if (fmt >= PTI_ASTC_4X4_SRGB)
			return (fmt-PTI_ASTC_4X4_SRGB)+PTI_ASTC_4X4_LDR;
	}
	if (fmt == PTI_BC1_RGB)
		return PTI_BC1_RGBA;
	return fmt;
}
#ifdef _WIN32
static void FS_MakeTempName(char *out, size_t outsize, char *prefix, char *suffix)
{
	static char temp_path[MAX_PATH];
	char temp_file_name[MAX_PATH];
	if (!*temp_path && !GetTempPathA(sizeof(temp_path), temp_path))
		Sys_Error("FS_MakeTempName failed to get temp path\n");
	if (!GetTempFileNameA(temp_path, prefix, 0, temp_file_name))
		Sys_Error("FS_MakeTempName failed\n");

	Q_snprintfz(out, outsize, "%s%s", temp_file_name, suffix);
}
#else
#include <unistd.h>
static void FS_MakeTempName(char *out, size_t outsize, char *prefix, char *suffix)
{
	snprintf(out, outsize, "/tmp/%sXXXXXX%s", prefix, suffix);
	close(mkstemps(out, strlen(suffix)));	//bsd4.3/posix1-2001
}
#endif

static qboolean ImgTool_HasAlpha(struct pendingtextureinfo *mips)
{
	if (mips->encoding == PTI_RGBA8 || mips->encoding == PTI_BGRA8 || mips->encoding == PTI_LLLA8 || mips->encoding == PTI_RGBA8_SRGB || mips->encoding == PTI_BGRA8_SRGB)
	{
		size_t l = 0, pixels, p;
		qbyte *d;
		for (l = 0; l < mips->mipcount; l++)
		{
			pixels = mips->mip[l].width * mips->mip[l].height * mips->mip[l].depth * 4;
			d = mips->mip[l].data;
			d+=3;
			for (p = 0; p < pixels; p+=4)
				if (d[p] != 255)
					return true;	//a transparent pixel!
		}
		return false;
	}
	else if (mips->encoding == PTI_L8A8 || mips->encoding == PTI_L8A8_SRGB)
	{
		size_t l = 0, pixels, p;
		qbyte *d;
		for (l = 0; l < mips->mipcount; l++)
		{
			pixels = mips->mip[l].width * mips->mip[l].height * mips->mip[l].depth * 2;
			d = mips->mip[l].data;
			d+=1;
			for (p = 0; p < pixels; p+=2)
				if (d[p] != 255)
					return true;	//a transparent pixel!
		}
		return false;
	}
	else if (mips->encoding == PTI_RGBA16)
	{
		size_t l = 0, pixels, p;
		unsigned short *d;
		for (l = 0; l < mips->mipcount; l++)
		{
			pixels = mips->mip[l].width * mips->mip[l].height * mips->mip[l].depth * 4;
			d = mips->mip[l].data;
			d+=3;
			for (p = 0; p < pixels; p+=4)
				if (d[p] != 0xffff)
					return true;	//a transparent pixel!
		}
		return false;
	}
	else
		return Image_FormatHasAlpha(mips->encoding);
}

//copys all the data out and everything!
static struct pendingtextureinfo *ImgTool_DupeMipchain(struct pendingtextureinfo *src)
{
	struct pendingtextureinfo *dest;
	qbyte *data;
	size_t size = 0;
	size_t m;
	for(m = 0; m < src->mipcount; m++)
		size += src->mip[m].datasize;
	dest = Z_Malloc(sizeof(*dest)+size);
	*dest = *src;
	data = (qbyte*)(dest+1);
	for(m = 0; m < src->mipcount; m++)
	{
		dest->mip[m].data = data;
		dest->mip[m].needfree = false;
		memcpy(data, src->mip[m].data, src->mip[m].datasize);
		data += src->mip[m].datasize;
	}
	dest->extrafree = NULL;	//part of the texinfo itself.
	return dest;
}

static qboolean ImgTool_ConvertPixelFormat(struct opts_s *args, const char *inname, struct pendingtextureinfo *mips)
{
	struct pendingtextureinfo tmp, *ret;
	size_t m;
	char raw[MAX_OSPATH];
	char comp[MAX_OSPATH];
	char command[MAX_OSPATH*3];

	qbyte *fdata;
	size_t fsize;
	int bb,bw,bh,bd;
	qboolean canktx = false;
	uploadfmt_t targfmt = args->newpixelformat;
	int d,l, layers, r;

	//force it to bc1 if bc2 or bc3 with no alpha channel.
	if ((targfmt == PTI_BC2_RGBA || targfmt == PTI_BC3_RGBA) && !ImgTool_HasAlpha(mips))
		targfmt = PTI_BC1_RGB;

	if (targfmt >= PTI_ASTC_FIRST && targfmt <= PTI_ASTC_LAST)
	{
		Q_snprintfz(command, sizeof(command), "astcenc -c");
		canktx = true;
	}
	else if (targfmt == PTI_BC1_RGB)
		Q_snprintfz(command, sizeof(command), "nvcompress -bc1%s", (args->flags&IF_TRYBUMP)?"n":"");
	else if (targfmt == PTI_BC1_RGB_SRGB)
		Q_snprintfz(command, sizeof(command), "nvcompress -bc1%s -srgb -dds10", (args->flags&IF_TRYBUMP)?"n":"");
	else if (targfmt == PTI_BC1_RGBA)
		Q_snprintfz(command, sizeof(command), "nvcompress -bc1a");
	else if (targfmt == PTI_BC1_RGBA_SRGB)
		Q_snprintfz(command, sizeof(command), "nvcompress -bc1a -srgb -dds10");
	else if (targfmt == PTI_BC2_RGBA)
		Q_snprintfz(command, sizeof(command), "nvcompress -bc2");
	else if (targfmt == PTI_BC2_RGBA_SRGB)
		Q_snprintfz(command, sizeof(command), "nvcompress -bc2 -srgb -dds10");
	else if (targfmt == PTI_BC3_RGBA)
		Q_snprintfz(command, sizeof(command), "nvcompress -bc3%s", (args->flags&IF_TRYBUMP)?"n":"");
	else if (targfmt == PTI_BC3_RGBA_SRGB)
		Q_snprintfz(command, sizeof(command), "nvcompress -bc3%s -srgb -dds10", (args->flags&IF_TRYBUMP)?"n":"");
	else if (targfmt == PTI_BC4_R)
		Q_snprintfz(command, sizeof(command), "nvcompress -bc4");
	else if (targfmt == PTI_BC5_RG)
		Q_snprintfz(command, sizeof(command), "nvcompress -bc5");
	else if (targfmt == PTI_BC6_RGB_SFLOAT || targfmt == PTI_BC6_RGB_UFLOAT)
		Q_snprintfz(command, sizeof(command), "nvcompress -bc6");
	else if (targfmt == PTI_BC7_RGBA)
		Q_snprintfz(command, sizeof(command), "nvcompress -bc7");
	else if (targfmt == PTI_BC7_RGBA_SRGB)
		Q_snprintfz(command, sizeof(command), "nvcompress -bc7 -srgb");
	else
	{
		if (mips->encoding != targfmt)
		{
			qboolean forceformats[PTI_MAX];
			for (m = 0; m < PTI_MAX; m++)
				forceformats[m] = (m == targfmt);
			Image_ChangeFormat(mips, forceformats, PTI_INVALID, inname);
			if (mips->encoding == targfmt)
				return true;

			//switch to common formats...
			for (m = 0; m < PTI_MAX; m++)
				forceformats[m] = (m == targfmt) || (m==PTI_RGBA8) || (m==PTI_RGBA32F);
			Image_ChangeFormat(mips, forceformats, PTI_INVALID, inname);
			//and try again...
			for (m = 0; m < PTI_MAX; m++)
				forceformats[m] = (m == targfmt);
			Image_ChangeFormat(mips, forceformats, PTI_INVALID, inname);

			return (mips->encoding == targfmt);
		}
		return true;
	}
	if (canktx)
		FS_MakeTempName(raw, sizeof(raw), "itr", ".ktx");
	else
		FS_MakeTempName(raw, sizeof(raw), "itr", ".png");
	FS_MakeTempName(comp, sizeof(comp), "itc", ".ktx");

	tmp.type = mips->type;
	tmp.encoding = mips->encoding;
	tmp.extrafree = NULL;
	tmp.mipcount = 1;

	Image_BlockSizeForEncoding(targfmt, &bb, &bw, &bh, &bd);
	Q_snprintfz(command+strlen(command), sizeof(command)-strlen(command), " \"%s\" \"%s\"", raw, comp);
	if (targfmt >= PTI_ASTC_FIRST && targfmt <= PTI_ASTC_LAST)
	{
		if (bd!=1)
			Q_snprintfz(command+strlen(command), sizeof(command)-strlen(command), " %ix%ix%i -exhaustive", bw, bh, bd);
		else
			Q_snprintfz(command+strlen(command), sizeof(command)-strlen(command), " %ix%i -exhaustive", bw, bh);
	}
	if (targfmt >= PTI_ASTC_4X4_SRGB && targfmt <= PTI_ASTC_12X12_SRGB)
		Q_strncatz(command, " -srgb", sizeof(command));
	if (targfmt >= PTI_ASTC_4X4_HDR && targfmt <= PTI_ASTC_12X12_HDR)
		Q_strncatz(command, " -hdr", sizeof(command));
	if (targfmt >= PTI_BC1_RGB && targfmt <= PTI_BC7_RGBA_SRGB && (strstr(inname, "_n.")||strstr(inname, "_norm.")))
		Q_strncatz(command, " -normal", sizeof(command));	//looks like a normalmap... tweak metrics to favour normalised results.

#ifdef _WIN32
   Q_strncatz(command, "> NUL 2>&1", sizeof(command));
#else
   Q_strncatz(command, ">> /dev/null", sizeof(command));
#endif

	if (!canktx)
	{
		qboolean allowformats[PTI_MAX];
		//make sure the source pixel format is acceptable if we're forced to write a png
		for (m = 0; m < PTI_MAX; m++)
			allowformats[m] =
					(m == PTI_RGBA8) || (m == PTI_RGBX8) ||
					(m == PTI_BGRA8) || (m == PTI_BGRX8) ||
					(m == PTI_LLLA8) || (m == PTI_LLLX8) ||
					(m == PTI_RGBA16) ||
					(m == PTI_L8) || (m == PTI_L8A8) ||
					/*(m == PTI_L16) ||*/
					(m == PTI_BGR8) || (m == PTI_BGR8) ||
					0;
		Image_ChangeFormat(mips, allowformats, PTI_INVALID, inname);
	}

//	Con_Printf("%s: Compressing %u mips\n", inname, mips->mipcount);

	Image_BlockSizeForEncoding(mips->encoding, &bb, &bw, &bh, &bd);
	for (m = 0; m < mips->mipcount; m++)
	{
		qbyte *srcdata = mips->mip[m].data;
		size_t srcsize = mips->mip[m].datasize;
		if (mips->type == PTI_3D)
		{
			layers = 1;
			d = mips->mip[m].depth;
			tmp.type = PTI_2D;
		}
		else
		{
			layers = mips->mip[m].depth;
			d = 1;
			tmp.type = PTI_2D;
		}
		for (l = 0; l < layers; l++)
		{
//			Con_DPrintf("Compressing %s mip %u, layer %u\n", inname, (unsigned)m, l);
			tmp.mip[0] = mips->mip[m];
			tmp.mip[0].needfree = false;
			tmp.mip[0].depth = d;
			tmp.mip[0].datasize = srcsize/layers;
			tmp.mip[0].data = srcdata + l * tmp.mip[0].datasize;
			(void)tmp;

			if (canktx)
			{
	#ifdef IMAGEFMT_KTX
				if (!Image_WriteKTXFile(raw, FS_SYSTEM, &tmp))
	#endif
					break;
			}
			else
			{
	#ifdef AVAIL_PNGLIB
				if (!Image_WritePNG(raw, FS_SYSTEM, 0, &tmp.mip[0].data, 1, tmp.mip[0].width*bb, tmp.mip[0].width, tmp.mip[0].height, tmp.encoding, false))
	#endif
					break;
			}

			r = system(command);
			if (r != EXIT_SUCCESS)
			{
				Con_Printf("The following system command failed with code %i: %s\n", r, command);
				break;
			}

			fdata = FS_LoadMallocFile(comp, &fsize);
			ret = Image_LoadMipsFromMemory(IF_NOMIPMAP, comp, comp, fdata, fsize);
			if (ret &&	ret->mip[0].width == mips->mip[m].width &&
						ret->mip[0].height == mips->mip[m].height &&
						ret->mip[0].depth == d &&
						ImgTool_ASTCToLDR(ret->encoding) == ImgTool_ASTCToLDR(targfmt))
			{
				if (layers == 1)	//just copy it over. FIXME: memory leak
					mips->mip[m] = ret->mip[0];
				else
				{
					if (!l)
					{
						mips->mip[m].datasize = ret->mip[0].datasize * layers;
						mips->mip[m].data = BZ_Malloc(mips->mip[m].datasize);
						mips->mip[m].needfree = true;
					}
					else if (ret->mip[0].datasize != mips->mip[m].datasize/layers)
						break;	//erk..?
					memcpy((qbyte*)mips->mip[m].data + l * ret->mip[0].datasize, ret->mip[0].data, ret->mip[0].datasize);
				}
				continue;
			}
			else if (!ret)
				Con_Printf("Failed to read intermediate file %s\n", comp);
			else
				Con_Printf("intermediate file %s has unexpected size/depth/format %s:%i*%i*%i vs %s:%i*%i*%i\n", comp,
						Image_FormatName(ret->encoding), ret->mip[0].width, ret->mip[0].height, ret->mip[0].depth,
						Image_FormatName(targfmt), mips->mip[m].width, mips->mip[m].height, d);
			break;
		}
		if (l != layers)
			break;
	}

	mips->encoding = targfmt;
	mips->mipcount = m;

	if (mips->mipcount && targfmt >= PTI_BC1_RGB && targfmt <= PTI_BC7_RGBA_SRGB)
	{	//d3d has some annoying limitations.
		//do not warn for astc files, their block sizes are too weird.
		Image_BlockSizeForEncoding(targfmt, &bb, &bw, &bh, &bd);
		if (mips->mip[0].width%bw || mips->mip[0].height%bh || mips->mip[0].depth%bd)
			Con_Printf("%s: mip0 of %i*%i is not a multiple of %i*%i (d3d warning)\n", inname, mips->mip[0].width, mips->mip[0].height, bw, bh);
	}

	FS_Remove(raw, FS_SYSTEM);
	FS_Remove(comp, FS_SYSTEM);
	return true;
}

const char *COM_GetFileExtension (const char *in, const char *term)
{
	const char *dot;

	if (!term)
		term = in + strlen(in);

	for (dot = term-1; dot >= in && *dot != '/' && *dot != '\\'; dot--)
	{
		if (*dot == '.')
			return dot;
	}
	return term+strlen(term);
}
static struct pendingtextureinfo *ImgTool_Read(struct opts_s *args, const char *inname)
{
	qbyte *indata;
	size_t fsize;
	struct pendingtextureinfo *in;
	indata = FS_LoadMallocFile(inname, &fsize);
	if (!indata)
		Con_Printf("%s: unable to read\n", inname);
	else
	{
		const char *ex = COM_GetFileExtension(inname, NULL);
		if (!strcasecmp(ex, ".mip"))
			in = ImgTool_DecodeMiptex(args, (miptex_t*)indata, fsize, NULL);
		else
			in = Image_LoadMipsFromMemory(args->flags|IF_NOMIPMAP, inname, inname, indata, fsize);
		if (!in)
		{
			Con_Printf("%s: unsupported format\n", inname);
			BZ_Free(indata);
		}
		else
		{
			Con_DPrintf("%s: %s %s, %i*%i, %i mips\n", inname, imagetypename[in->type], Image_FormatName(in->encoding), in->mip[0].width, in->mip[0].height, in->mipcount);
			return in;
		}
	}
	return NULL;
}
static struct pendingtextureinfo *ImgTool_Combine(struct opts_s *args, const char **namelist, unsigned int filecount)
{
	struct pendingtextureinfo *r, *t;
	unsigned int i;
	unsigned int layers = 0;
	unsigned int j = 0;

	struct
	{
		const char *fname;
		struct pendingtextureinfo *in;
	} *srcs, *tmpsrcs;

	if (args->textype == PTI_3D)
		args->flags |= IF_NOMIPMAP;	//generate any mipmaps after...

	srcs = alloca(sizeof(*srcs)*filecount);
	for (i = 0, j = 0; i < filecount; i++)
	{
		srcs[j].in = t = ImgTool_Read(args, namelist[i]);
		if (srcs[j].in)
		{
			if (!j)
			{
				//get the image loader to massage pixel formats...
				memset(sh_config.texfmt, 0, sizeof(sh_config.texfmt));
				sh_config.texfmt[srcs[j].in->encoding] = true;
			}
			else if (!t->mipcount || !t->mip[0].data)
			{
				Con_Printf("%s: no valid image data\n", namelist[i]);
				ImgTool_FreeMips(srcs[j].in);
				continue;
			}
			else if (t->encoding != srcs[0].in->encoding)
			{
				Con_Printf("%s: mismatched pixel format, (%s not %s) cannot combine\n", namelist[i], Image_FormatName(t->encoding), Image_FormatName(srcs[0].in->encoding));
				ImgTool_FreeMips(srcs[j].in);
				continue;
			}
			else if (t->type == PTI_CUBE && t->mip[0].depth != 6)
			{
				Con_Printf("%s: incorrect cubemap data\n", namelist[i]);
				ImgTool_FreeMips(srcs[j].in);
				continue;
			}
			else if (srcs[0].in->mip[0].width != t->mip[0].width || srcs[0].in->mip[0].height != t->mip[0].height)
			{
				Con_Printf("%s: incorrect image size\n", namelist[i]);
				ImgTool_FreeMips(srcs[j].in);
				continue;
			}
			else if (t->mip[0].depth == 0)
			{
				Con_Printf("%s: no layers\n", namelist[i]);
				ImgTool_FreeMips(srcs[j].in);
				continue;
			}
			layers += t->mip[0].depth;
			srcs[j++].fname = namelist[i];
		}
	}
	filecount = j;

	//FIXME: reorder input images to handle ft/bk/lt/rt/up/dn, and flip+rotate+etc to match quake

	if (args->textype == PTI_CUBE)
	{
		int facetype[6];
		static const struct {qboolean flipx, flipy, flipd;} skyboxflips[] ={
			{true,  false, true},
			{false, true,  true},
			{true,  true,  false},
			{false, false, false},
			{true,  false, true},
			{true,  false, true}
		};
		for (i = 0; i < 6; i++)
			facetype[i] = 0;
		for (i = 0; i < filecount; i++)
		{
			const char *ex = COM_GetFileExtension(srcs[i].fname, NULL);
			if (ex && ex-srcs[i].fname > 2 && srcs[i].in->mip[0].depth == 1)
			{
				if (!strncasecmp(ex-2, "rt", 2))
					facetype[0] = -i-1;
				else if (!strncasecmp(ex-2, "lf", 2))
					facetype[1] = -i-1;
				else if (!strncasecmp(ex-2, "ft", 2))
					facetype[2] = -i-1;
				else if (!strncasecmp(ex-2, "bk", 2))
					facetype[3] = -i-1;
				else if (!strncasecmp(ex-2, "up", 2))
					facetype[4] = -i-1;
				else if (!strncasecmp(ex-2, "dn", 2))
					facetype[5] = -i-1;

				else if (!strncasecmp(ex-2, "px", 2))
					facetype[0] = i+1;
				else if (!strncasecmp(ex-2, "nx", 2))
					facetype[1] = i+1;
				else if (!strncasecmp(ex-2, "py", 2))
					facetype[2] = i+1;
				else if (!strncasecmp(ex-2, "ny", 2))
					facetype[3] = i+1;
				else if (!strncasecmp(ex-2, "pz", 2))
					facetype[4] = i+1;
				else if (!strncasecmp(ex-2, "nz", 2))
					facetype[5] = i+1;
			}
		}
		if (facetype[0] && facetype[1] && facetype[2] && facetype[3] && facetype[4] && facetype[5])
		{
			Con_Printf("Reordering images to match cubemap\n");
			tmpsrcs = alloca(sizeof(*tmpsrcs)*filecount);
			memcpy(tmpsrcs, srcs, sizeof(*tmpsrcs)*filecount);
			for (i = 0; i < 6; i++)
			{
				if (facetype[i] < 0)
				{	//flip to match legacy skyboxes
					unsigned bb,bw,bh,bd;
					srcs[i] = tmpsrcs[-facetype[i]-1];
					t = srcs[i].in;
					Image_BlockSizeForEncoding(t->encoding, &bb,&bw,&bh,&bd);
					if (bw == 1 && bh == 1 && bd == 1)
					{
						for (j = 0; j < t->mipcount; j++)
						{
							void *data = Image_FlipImage(t->mip[j].data, BZ_Malloc(t->mip[j].datasize), &t->mip[j].width, &t->mip[j].height, bb, skyboxflips[i].flipx, skyboxflips[i].flipy, skyboxflips[i].flipd);
							if (t->mip[j].needfree)
								Z_Free(t->mip[j].data);
							t->mip[j].data = data;
							t->mip[j].needfree = true;
						}
					}
				}
				else
					srcs[i] = tmpsrcs[facetype[i]-1];
			}
		}
		else
			Con_Printf("WARNING: Cubemap ordering unknown!\n");
	}

	if (!filecount)
		Con_Printf("no valid input files\n");
	else if (!layers)
		Con_Printf("Images must have at least one layer\n");
	else if (args->textype == PTI_2D && layers != 1)
		Con_Printf("2D images must have one layer exactly, sorry\n");
	else if (args->textype == PTI_CUBE && layers != 6)
		Con_Printf("Cubemaps must have 6 layers exactly\n");
	else if (args->textype == PTI_CUBE_ARRAY && layers % 6)
		Con_Printf("Cubemap arrays must have a multiple of 6 layers exactly\n");
	else
	{
		t = srcs[0].in;
		r = Z_Malloc(sizeof(*t));
		r->type = args->textype;
		r->extrafree = NULL;
		r->encoding = t->encoding;

		if (args->textype == PTI_3D)
			r->mipcount = 1;
		else
			r->mipcount = t->mipcount;
		for (j = 0; j < t->mipcount; j++)
		{
			r->mip[j].datasize = t->mip[j].datasize*layers;
			r->mip[j].width = t->mip[j].width;
			r->mip[j].height = t->mip[j].height;
			r->mip[j].depth = 0;
			r->mip[j].needfree = true;
			r->mip[j].data = BZ_Malloc(r->mip[j].datasize);
		}

		for (i = 0, j = 0; i < filecount; i++)
		{
			t = srcs[i].in;
			if (!t)
			{
				ImgTool_FreeMips(r);
				return NULL;
			}

			for (j = 0; j < r->mipcount; j++)
			{
				if (r->mip[j].width != t->mip[j].width || r->mip[j].height != t->mip[j].height || t->mip[j].depth != 1)
				{
					Con_Printf("%s: mismatched mipmap sizes\n", namelist[i]);
					continue;
				}
				memcpy((qbyte*)r->mip[j].data + t->mip[j].datasize*r->mip[j].depth, t->mip[j].data, t->mip[j].datasize);
				r->mip[j].depth++;
			}
		}

		for (i = 0; i < filecount; i++)
			ImgTool_FreeMips(srcs[i].in);

		printf("%s: %s %s, %i*%i, %i mips\n", "combined", imagetypename[r->type], Image_FormatName(r->encoding), r->mip[0].width, r->mip[0].height, r->mipcount);
		return r;
	}

	for (i = 0; i < filecount; i++)
		ImgTool_FreeMips(srcs[i].in);
	return NULL;
}
static void ImgTool_Convert(struct opts_s *args, struct pendingtextureinfo *in, const char *inname, const char *outname)
{
	size_t k;
	const char *outext;
	qboolean allowcompressed = false;
	char newout[MAX_OSPATH];

	if (!outname)
	{
		outext = COM_GetFileExtension(inname, NULL);
		k = min(MAX_OSPATH-2-strlen(args->defaultext), outext-inname);
		memcpy(newout, inname, k);
		newout[k++] = '.';
		strcpy(newout+k, args->defaultext);
		outname = newout;
	}



	outext = COM_GetFileExtension(outname, NULL);
	if (!strcmp(outext, ".dds") || !strcmp(outext, ".ktx"))
		allowcompressed = true;

	if (in)
	{
		if (!strcmp(outext, ".ktx") || !strcmp(outext, ".dds") || args->mipnum >= in->mipcount)
		{
			if (!(args->flags & IF_NOMIPMAP) && in->mipcount == 1)
				Image_GenerateMips(in, args->flags);
		}

		if (args->mipnum >= in->mipcount)
		{
			ImgTool_FreeMips(in);
			Con_Printf("%s: Requested output mip number was out of bounds %i >= %i\n", outname, args->mipnum, in->mipcount);
			return;
		}
		for (k = 0; k < args->mipnum; k++)
		{
			if (in->mip[k].needfree)
				BZ_Free(in->mip[k].data);
		}
		in->mipcount -= k;
		memmove(in->mip, &in->mip[k], sizeof(in->mip[0])*in->mipcount);

		Con_Printf("%s(%s)->", inname, Image_FormatName(in->encoding));

		if (args->newpixelformat != PTI_INVALID && (args->newpixelformat < PTI_BC1_RGB || allowcompressed) && ImgTool_ConvertPixelFormat(args, inname, in))
			Con_Printf("(%s)->", Image_FormatName(in->encoding));

		if (!in->mipcount)
			Con_Printf("%s: no image data\n", inname);
		else if (!strcasecmp(outext, ".mip"))
		{
			vfsfile_t *fs = FS_OpenVFS(outname, "wb", FS_SYSTEM);
			if (!fs)
				Con_Printf("%s(%s): Write failed\n", outname, Image_FormatName(in->encoding));
			else
			{
				if (!ImgTool_MipExport(args, fs, in, outname, 2))
					Con_Printf("%s: export failed\n", outname);
				VFS_CLOSE(fs);
			}
		}
#ifdef IMAGEFMT_KTX
		else if (!strcasecmp(outext, ".ktx"))
		{
			if (!Image_WriteKTXFile(outname, FS_SYSTEM, in))
				Con_Printf("%s(%s): Write failed\n", outname, Image_FormatName(in->encoding));
		}
#endif
#ifdef IMAGEFMT_DDS
		else if (!strcasecmp(outext, ".dds"))
		{
			if (!Image_WriteDDSFile(outname, FS_SYSTEM, in))
				Con_Printf("%s(%s): Write failed\n", outname, Image_FormatName(in->encoding));
		}
#endif
		else
		{
			int bb,bw,bh,bd;

			if (in->type != PTI_2D)
				Con_Printf("%s: Unable to write %s file to 2d image format\n", outname, imagetypename[in->type]);
#ifdef IMAGEFMT_PNG
			else if (!strcasecmp(outext, ".png"))
			{
#ifdef AVAIL_PNGLIB
				qboolean outformats[PTI_MAX];
				//force the format, because we can.
				for (k = 0; k < PTI_MAX; k++)
					outformats[k] =
							(k == PTI_RGBA8) || (k == PTI_RGBX8) ||
							(k == PTI_BGRA8) || (k == PTI_BGRX8) ||
							(k == PTI_LLLA8) || (k == PTI_LLLX8) ||
							(k == PTI_RGBA16) || (k == PTI_P8) ||
							(k == PTI_L8) || (k == PTI_L8A8) ||
							/*(k == PTI_L16) ||*/
							(k == PTI_BGR8) || (k == PTI_BGR8) ||
							0;
				if (!outformats[in->encoding])
					Image_ChangeFormat(in, outformats, PTI_INVALID, outname);
				Image_BlockSizeForEncoding(in->encoding, &bb, &bw,&bh,&bd);
				if (!Image_WritePNG(outname, FS_SYSTEM, 0, &in->mip[0].data, 1, in->mip[0].width*bb, in->mip[0].width, in->mip[0].height, in->encoding, false))
#endif
					Con_Printf("%s(%s): Write failed\n", outname, Image_FormatName(in->encoding));
			}
#endif
#ifdef IMAGEFMT_TGA
			else if (!strcasecmp(outext, ".tga"))
			{
				qboolean outformats[PTI_MAX];
				for (k = 0; k < PTI_MAX; k++)
					outformats[k] =
							(k == PTI_RGBA8) || (k == PTI_RGBX8) ||
							(k == PTI_BGRA8) || (k == PTI_BGRX8) ||
							(k == PTI_LLLA8) || (k == PTI_LLLX8) ||
							(k == PTI_RGBA16F) || (k == PTI_R16F) ||	//half-float tgas is a format extension, but allow it.
							(k == PTI_L8) || (k == PTI_L8A8) ||
							/*(k == PTI_L16) ||*/
							(k == PTI_BGR8) || (k == PTI_BGR8) ||
							(k == PTI_ARGB1555) ||
							0;
				if (!outformats[in->encoding])
					Image_ChangeFormat(in, outformats, PTI_INVALID, outname);
				Image_BlockSizeForEncoding(in->encoding, &bb, &bw,&bh,&bd);
				if (!WriteTGA(outname, FS_SYSTEM, in->mip[0].data, in->mip[0].width*bb, in->mip[0].width, in->mip[0].height, in->encoding))
					Con_Printf("%s(%s): Write failed\n", outname, Image_FormatName(in->encoding));
			}
#endif
#ifdef IMAGEFMT_PCX
			else if (!strcasecmp(outext, ".pcx"))
			{
				qboolean outformats[PTI_MAX];
				for (k = 0; k < PTI_MAX; k++)
					outformats[k] =
							(k == PTI_P8) ||
							(k == TF_SOLID8) ||
							(k == TF_TRANS8) ||
							(k == TF_H2_TRANS8_0) ||
							0;
				if (!outformats[in->encoding])
					Image_ChangeFormat(in, outformats, PTI_INVALID, outname);
				Image_BlockSizeForEncoding(in->encoding, &bb, &bw,&bh,&bd);
				if (!WritePCXfile(outname, FS_SYSTEM, in->mip[0].data, in->mip[0].width, in->mip[0].height, in->mip[0].width*bb, host_basepal, false))
					Con_Printf("%s(%s): Write failed\n", outname, Image_FormatName(in->encoding));
			}
#endif
			else if (!strcasecmp(outext, ".lmp"))
			{
				int i;
				vfsfile_t *f = NULL;
				qboolean outformats[PTI_MAX];
				for (k = 0; k < PTI_MAX; k++)
					outformats[k] =
							(k == PTI_P8) ||
							(k == TF_SOLID8) ||
							(k == TF_TRANS8) ||
							(k == TF_H2_TRANS8_0) ||
							0;
				if (!outformats[in->encoding])
					Image_ChangeFormat(in, outformats, PTI_INVALID, outname);
				Image_BlockSizeForEncoding(in->encoding, &bb, &bw,&bh,&bd);

				if (!outformats[in->encoding])
					Con_Printf("%s(%s): couldn't palettize\n", outname, Image_FormatName(in->encoding));
				else
				{
					f = FS_OpenVFS(outname, "wb", FS_SYSTEM);
					if (f)
					{
						i = LittleLong(in->mip[0].width);
						VFS_WRITE(f, &i, sizeof(i));
						i = LittleLong(in->mip[0].height);
						VFS_WRITE(f, &i, sizeof(i));
						VFS_WRITE(f, in->mip[0].data, in->mip[0].width*bb*in->mip[0].height);
						VFS_CLOSE(f);
					}
					else
						Con_Printf("%s(%s): Couldn't open for writing\n", outname, Image_FormatName(in->encoding));
				}
			}
			else
				Con_Printf("%s: Unknown output file format\n", outname);
		}

		if (in->mipcount > 1)
		{
			Con_Printf("%s(%s): %s %i*%i*%i, %i mips\n", outname, Image_FormatName(in->encoding), imagetypename[in->type], in->mip[0].width, in->mip[0].height, in->mip[0].depth, in->mipcount);
			for (k = 0; k < in->mipcount; k++)
				if (in->mip[k].depth == 1 && in->type == PTI_2D)
					Con_DPrintf("\t%u: %i*%i, %u\n", (unsigned)k, in->mip[k].width, in->mip[k].height, (unsigned)in->mip[k].datasize);
				else
					Con_DPrintf("\t%u: %i*%i*%i, %u\n", (unsigned)k, in->mip[k].width, in->mip[k].height, in->mip[k].depth, (unsigned)in->mip[k].datasize);
		}
		else
			Con_Printf("%s(%s): %s %i*%i*%i, %u bytes\n", outname, Image_FormatName(in->encoding), imagetypename[in->type], in->mip[0].width, in->mip[0].height, in->mip[0].depth, (unsigned)in->mip[0].datasize);

		ImgTool_FreeMips(in);
	}
	fflush(stdout);
}

static struct pendingtextureinfo *ImgTool_DecodeMiptex(struct opts_s *args, miptex_t *mip, size_t size, qbyte *pal)
{
	qbyte *data = (qbyte*)mip + (mip->offsets[3]?mip->offsets[3] + (mip->width>>3)*(mip->height>>3):sizeof(miptex_t));
	qbyte *dataend = (qbyte*)mip + size;
	struct pendingtextureinfo *out = Z_Malloc(sizeof(*out));
	qbyte *newdata = NULL;
	int neww=0, newh=0, sz;
	unsigned int bw,bh,bb,bd, i;
	out->type = PTI_2D;

	out->encoding = PTI_INVALID;

	Con_DPrintf("%s: width %u, height %u, size %u\n", mip->name, mip->width, mip->height, (unsigned int)size);

	//header [legacymip0 legacymip1 legacymip2] [extsize extcode extdata]*n [legacymip3]
	//extcode NAME: extdata-8 bytes of replacement name
	//extdata pixelformats, extdata is: Width Height newmip0...N where N is 1*1 mip, using round-down logic.
	//compressed and legacy data ommitted means all 4 offsets are 0.
	//compressed-only data has offset[3] state the termination position, but offset[0,1,2] MUST be 0 still (extension data starts right after the header, offset3 points to the end of the miptex and has no data there.
	//legacy-only data is densely packed or whatever.
	//half-life palette data might be glued onto the end. we don't care to handle that.
	Con_DPrintf("%i bytes of extended data\n", (unsigned)(dataend-data));
	Con_DPrintf("offset[0]: %u\n", mip->offsets[0]);
	Con_DPrintf("offset[1]: %u\n", mip->offsets[1]);
	Con_DPrintf("offset[2]: %u\n", mip->offsets[2]);
	Con_DPrintf("offset[3]: %u\n", mip->offsets[3]);
	if (data+4 < dataend && data[0]==0x00 && data[1]==0xfb&&data[2]==0x2b&&data[3]==0xaf)	//magic id to say that there's actually extensions here...
	{
		data+=4;
		for (; data+4 < dataend; data += sz)
		{	//we could recognise more,
			uploadfmt_t fmt = PTI_INVALID;
			size_t csz, w, h;
			sz = (data[0]<<0) | (data[1]<<8) | (data[2]<<16) | (data[3]<<24);

			if (sz < 4 || sz > dataend-data)		{	Con_Printf("%s: Invalid miptex extension\n", mip->name);	break;}
			else if (sz > 8 && !strncmp(data+4, "NAME", 4))		continue;	//FIXME
			else if (sz > 16 && !strncmp(data+4, "RGBA", 4))	fmt = PTI_RGBA8;
			else if (sz > 16 && !strncmp(data+4, "RGB", 4))		fmt = PTI_RGB8;
			else if (sz > 16 && !strncmp(data+4, "565", 4))		fmt = PTI_RGB565;
			else if (sz > 16 && !strncmp(data+4, "5551", 4))	fmt = PTI_RGBA5551;
			else if (sz > 16 && !strncmp(data+4, "4444", 4))	fmt = PTI_RGBA4444;
			else if (sz > 16 && !strncmp(data+4, "LUM8", 4))	fmt = PTI_L8;	//greyscale. because why not.
			else if (sz > 16 && !strncmp(data+4, "BC1", 4))		fmt = PTI_BC1_RGBA;
			else if (sz > 16 && !strncmp(data+4, "BC2", 4))		fmt = PTI_BC2_RGBA;
			else if (sz > 16 && !strncmp(data+4, "BC3", 4))		fmt = PTI_BC3_RGBA;
			else if (sz > 16 && !strncmp(data+4, "BC4", 4))		fmt = PTI_BC4_R;
			else if (sz > 16 && !strncmp(data+4, "BC5", 4))		fmt = PTI_BC5_RG;
			else if (sz > 16 && !strncmp(data+4, "BC6", 4))		fmt = PTI_BC6_RGB_UFLOAT;
			else if (sz > 16 && !strncmp(data+4, "BC7", 4))		fmt = PTI_BC7_RGBA;
			else if (sz > 16 && !strncmp(data+4, "ETC1", 4))	fmt = PTI_ETC1_RGB8;
			else if (sz > 16 && !strncmp(data+4, "ETC2", 4))	fmt = PTI_ETC2_RGB8;
			else if (sz > 16 && !strncmp(data+4, "ETCP", 4))	fmt = PTI_ETC2_RGB8A1;
			else if (sz > 16 && !strncmp(data+4, "ETCA", 4))	fmt = PTI_ETC2_RGB8A8;
			else if (sz > 16 && !strncmp(data+4, "AST4", 4))	fmt = PTI_ASTC_4X4_LDR;
			else if (sz > 16 && !strncmp(data+4, "AS54", 4))	fmt = PTI_ASTC_5X4_LDR;
			else if (sz > 16 && !strncmp(data+4, "AST5", 4))	fmt = PTI_ASTC_5X5_LDR;
			else if (sz > 16 && !strncmp(data+4, "AS65", 4))	fmt = PTI_ASTC_6X5_LDR;
			else if (sz > 16 && !strncmp(data+4, "AS85", 4))	fmt = PTI_ASTC_8X5_LDR;
			else if (sz > 16 && !strncmp(data+4, "AS05", 4))	fmt = PTI_ASTC_10X5_LDR;
			else if (sz > 16 && !strncmp(data+4, "AST6", 4))	fmt = PTI_ASTC_6X6_LDR;
			else if (sz > 16 && !strncmp(data+4, "AS86", 4))	fmt = PTI_ASTC_8X6_LDR;
			else if (sz > 16 && !strncmp(data+4, "AS06", 4))	fmt = PTI_ASTC_10X6_LDR;
			else if (sz > 16 && !strncmp(data+4, "AST8", 4))	fmt = PTI_ASTC_8X8_LDR;			
			else if (sz > 16 && !strncmp(data+4, "AS08", 4))	fmt = PTI_ASTC_10X8_LDR;
			else if (sz > 16 && !strncmp(data+4, "AST0", 4))	fmt = PTI_ASTC_10X10_LDR;
			else if (sz > 16 && !strncmp(data+4, "AS20", 4))	fmt = PTI_ASTC_12X10_LDR;
			else if (sz > 16 && !strncmp(data+4, "AST2", 4))	fmt = PTI_ASTC_12X12_LDR;
			else if (sz > 16 && !strncmp(data+4, "EXP5", 4))	fmt = PTI_E5BGR9;
			else {Con_Printf("%s: Unknown miptex extension %4s\n", mip->name, data+4);continue;}

			if (out->encoding != PTI_INVALID)	//use the first format we support, allowing prioritisation.
				continue;

			Image_BlockSizeForEncoding(fmt, &bb, &bw, &bh, &bd);
			w = data[ 8] | (data[ 9]<<8) | (data[10]<<16) | (data[11]<<24);
			h = data[12] | (data[13]<<8) | (data[14]<<16) | (data[15]<<24);
			for (csz = 16; w || h; w>>=1, h>>=1)
			{
				w = max(1, w);
				h = max(1, h);
				csz += bb*((w+bw-1)/bw)*((h+bh-1)/bh);
			}
			if (sz == csz)
			{	//mip chain is complete etc
				out->encoding = fmt;
				newdata = data+16;
				neww = data[ 8] | (data[ 9]<<8) | (data[10]<<16) | (data[11]<<24);
				newh = data[12] | (data[13]<<8) | (data[14]<<16) | (data[15]<<24);
			}
			else
				Con_Printf("%s: Chain size of %u doesn't match expected %u\n", mip->name, (unsigned)csz-16, sz-16);
		}
	}

	//only use our if there were no corrupt sections.
	if (data == dataend && newdata && neww && newh)
	{
		Image_BlockSizeForEncoding(out->encoding, &bb, &bw, &bh, &bd);
		for (out->mipcount = 0; out->mipcount < countof(out->mip) && neww && newh; out->mipcount++, neww>>=1, newh>>=1)
		{
			neww = max(1, neww);
			newh = max(1, newh);
			out->mip[out->mipcount].width = neww;
			out->mip[out->mipcount].height = newh;
			out->mip[out->mipcount].depth = 1;
			out->mip[out->mipcount].datasize = bb;
			out->mip[out->mipcount].datasize *= (out->mip[out->mipcount].width + bw-1)/bw;
			out->mip[out->mipcount].datasize *= (out->mip[out->mipcount].height + bh-1)/bh;
			out->mip[out->mipcount].datasize *= (out->mip[out->mipcount].depth + bd-1)/bd;
			out->mip[out->mipcount].data = newdata;
			newdata += out->mip[out->mipcount].datasize;
		}
	}
	else
	{
		if ((((dataend-data)+3)&~3) == (((256*3+2)+3)&~3) && data[0]==0&&data[1]==1)
			pal = data+2;	//halflife format...

		if (*mip->name == '{')
			out->encoding = TF_TRANS8;
		else if (!strncasecmp(mip->name, "sky", 3))
			out->encoding = TF_H2_TRANS8_0;
		else
			out->encoding = PTI_P8;

		if (pal)
		{	//bake the palette directly. rgb(a) data out.
			qbyte *idx, *rgb, *pi, *tr;
			size_t s;
			if (out->encoding == TF_TRANS8)
				out->encoding = PTI_RGBA8, tr = pal+255;
			else if (out->encoding == TF_H2_TRANS8_0)
				out->encoding = PTI_RGBA8, tr = pal+0;
			else //if (out->encoding == PTI_P8)
				out->encoding = PTI_RGBX8, tr = NULL;
			for (out->mipcount = 0; out->mipcount < 4 && mip->offsets[out->mipcount]; out->mipcount++)
			{
				out->mip[out->mipcount].width = mip->width>>out->mipcount;
				out->mip[out->mipcount].height = mip->height>>out->mipcount;
				out->mip[out->mipcount].depth = 1;
				s = out->mip[out->mipcount].width*(size_t)out->mip[out->mipcount].height*out->mip[out->mipcount].depth;
				out->mip[out->mipcount].datasize = s*4;
				rgb = out->mip[out->mipcount].data = BZ_Malloc(out->mip[out->mipcount].datasize);
				idx = (char*)mip + mip->offsets[out->mipcount];
				if (mip->offsets[out->mipcount]+s > size)
				{
					out->mip[out->mipcount].data = mip;
					out->mip[out->mipcount].datasize = 0;
					Con_Printf("%s: Mip%i offsets/size exceed size of texture\n", mip->name, out->mipcount);
				}
				else while (s-->0)
				{
					pi = pal+3**idx++;
					*rgb++ = pi[0];
					*rgb++ = pi[1];
					*rgb++ = pi[2];
					*rgb++ = (pi==tr)?0:255;
				}
			}
		}
		else
		{
			for (out->mipcount = 0; out->mipcount < 4 && mip->offsets[out->mipcount]; out->mipcount++)
			{
				out->mip[out->mipcount].width = mip->width>>out->mipcount;
				out->mip[out->mipcount].height = mip->height>>out->mipcount;
				out->mip[out->mipcount].depth = 1;
				out->mip[out->mipcount].datasize = out->mip[out->mipcount].width*(size_t)out->mip[out->mipcount].height*out->mip[out->mipcount].depth;
				out->mip[out->mipcount].data = (char*)mip + mip->offsets[out->mipcount];

				if (mip->offsets[out->mipcount]+out->mip[out->mipcount].datasize > size)
				{
					out->mip[out->mipcount].data = mip;
					out->mip[out->mipcount].datasize = 0;
					Con_Printf("%s: Mip%i offsets/size exceed size of texture\n", mip->name, out->mipcount);
				}
			}
		}
	}
	if (*mip->name == '*')
		*mip->name = '#';	//convert from * to #, so its a valid file name.

	if (args)
	{
		if (args->defaultext && !strcasecmp(args->defaultext, "mip"))
		{
			char newout[MAX_OSPATH];
			size_t k = strlen(mip->name);
			vfsfile_t *fs;
			memcpy(newout, mip->name, k);
			newout[k++] = '.';
			strcpy(newout+k, args->defaultext);

			fs = FS_OpenVFS(newout, "wb", FS_SYSTEM);
			if (!fs)
				Con_Printf("%s(%s): Write failed\n", newout, Image_FormatName(out->encoding));
			else
			{
				VFS_WRITE(fs, mip, size);
				VFS_CLOSE(fs);
			}

			fflush(stdout);
		}
		else
			ImgTool_Convert(args, out, mip->name, NULL);
		return NULL;
	}
	return out;
}
static void ImgTool_PrintInfo(const char *inname, struct pendingtextureinfo *in)
{
	size_t m;
	if (in->mipcount == 1 && in->type == PTI_2D && in->mip[0].depth == 1)
		printf("%-20s(%s): %4i*%-4i\n", inname, Image_FormatName(in->encoding), in->mip[0].width, in->mip[0].height);
	else if (in->mipcount == 1)
		printf("%-20s(%s): %s, %i*%i*%i, %u bytes\n", inname, Image_FormatName(in->encoding), imagetypename[in->type], in->mip[0].width, in->mip[0].height, in->mip[0].depth, (unsigned)in->mip[0].datasize);
	else
	{
		/*if (mip)
			printf("%-20s(%s): \"%s\"%s %i*%i, %i mips\n", inname, Image_FormatName(in->encoding), mip->name, mip->offsets[0]?"":" (stripped)", mip->width, mip->height, in->mipcount);
		else*/
			printf("%-20s(%s): %s, %i*%i*%i, %i mips\n", inname, Image_FormatName(in->encoding), imagetypename[in->type], in->mip[0].width, in->mip[0].height, in->mip[0].depth, in->mipcount);
		if (verbose)
			for (m = 0; m < in->mipcount; m++)
				printf("\t%u: %i*%i*%i, %u\n", (unsigned)m, in->mip[m].width, in->mip[m].height, in->mip[m].depth, (unsigned)in->mip[m].datasize);
	}
}
static void ImgTool_Enumerate(struct opts_s *args, const char *inname, void(*callback)(const char *name, struct pendingtextureinfo *mips))
{
	qbyte *indata;
	size_t fsize;
	size_t m;
	struct pendingtextureinfo *in;
	qbyte *basepal = NULL;
	indata = FS_LoadMallocFile(inname, &fsize);
	if (!indata)
		printf("%s: unable to read\n", inname);
	else if (fsize >= sizeof(wad2_t) && indata[0] == 'W' && indata[1] == 'A' && indata[2] == 'D')
	{
		const wad2_t *w = (const wad2_t *)indata;
		const wad2entry_t *e = (const wad2entry_t *)(indata+w->offset);
		printf("%s: wad%c file with %i entries\n", inname, w->magic[3], w->num);
		for (m = 0; m < w->num; m++, e++)
		{
			switch(e->type)
			{
			case TYP_QPIC:
				{
					size_t sz=min(e->size, e->dsize);
					unsigned int w=0;
					unsigned int h=0;
					in = NULL;
					if (!strcasecmp(e->name, "CONCHARS") && (e->size==128*128 || e->size==128*128+8))
					{	//special hack for buggy conchars, which is listed as a miptex for some reason, with no qpic header (it not being a qpic lump)
						printf("\t%16.16s: corrupt CONCHARS lump - wrongly marked as qpic. Treating as a legacy engine would...\n", e->name);
						in = Z_Malloc(sizeof(*in));
						in->encoding = TF_H2_TRANS8_0;
						in->mip[0].data = indata+e->offset;
						in->mip[0].datasize = 128*128;
						w = 128;
						h = 128;
					}
					else
					{
						if (sz >= 8)
						{
							w = (indata[e->offset+0]<<0)|(indata[e->offset+1]<<8)|(indata[e->offset+2]<<16)|(indata[e->offset+3]<<24);
							h = (indata[e->offset+4]<<0)|(indata[e->offset+5]<<8)|(indata[e->offset+6]<<16)|(indata[e->offset+7]<<24);
						}
						if (sz == w*h+8)
						{	//quake
							in = Z_Malloc(sizeof(*in));
							in->encoding = TF_TRANS8;
							in->mip[0].data = indata+e->offset+8;
							in->mip[0].datasize = w*h;
						}
						else if (((sz+3)&~3) == (((w*h+10+768+3)&~3)))
						{	//halflife
							const unsigned char *src = indata+e->offset+8;
							const unsigned char *pal = indata+e->offset+8+w*h+2, *p;
							unsigned char *dst;
							sz = w*h;
							in = Z_Malloc(sizeof(*in)+w*h*3);
							in->encoding = PTI_RGB8;
							in->mip[0].data = dst = (unsigned char*)(in+1);
							in->mip[0].datasize = sz*3;

							while (sz --> 0)
							{
								p = pal+*src++*3;
								*dst++ = *p++;
								*dst++ = *p++;
								*dst++ = *p++;
							}
						}
						else
							printf("\t%16.16s: missized qpic (%u %u, %u bytes)\n", e->name, w, h, (unsigned int)sz);
					}
					if (in)
					{
//						printf("\n");
						in->type = PTI_2D;
						in->mipcount = 1;
						in->mip[0].width = w;
						in->mip[0].height = h;
						in->mip[0].depth = 1;
						in->mip[0].needfree = false;

						callback(e->name, in);
						ImgTool_FreeMips(in);
					}
				}
				break;
			case 67:	//hl...
			case TYP_MIPTEX:
				{
					miptex_t *mip = (miptex_t *)(indata+e->offset);

					if (!strcasecmp(e->name, "CONCHARS") && e->size==128*128)
					{	//special hack for conchars, which is listed as a miptex for some reason, with no qpic header (it not being a qpic lump)
						in = Z_Malloc(sizeof(*in));
						in->encoding = TF_H2_TRANS8_0;
						in->mip[0].data = indata+e->offset+8;
						in->mip[0].datasize = 128*128;
						in->type = PTI_2D;
						in->mipcount = 1;
						in->mip[0].width = 128;
						in->mip[0].height = 128;
						in->mip[0].depth = 1;
						in->mip[0].needfree = false;
					}
					else
						in = ImgTool_DecodeMiptex(NULL, mip, min(e->size, e->dsize), basepal);

					if (in)
						callback(e->name, in);
/*
					//mip name SHOULD match entry name... but gah!
					if (strcasecmp(e->name, mip->name))
						printf("\t%16.16s (%s): ", e->name, mip->name);
					else
						printf("\t%16.16s: ", mip->name);
					printf("%u*%u%s", mip->width, mip->height, mip->offsets[0]?"":" (omitted)");

					if (in->encoding != PTI_P8)
						printf(" (%s %u*%u)", Image_FormatName(in->encoding), in->mip[0].width, in->mip[0].height);*/
//					printf("\n");
					ImgTool_FreeMips(in);
				}
				break;
			case TYP_PALETTE:
				if (e->size == 768)
				{
					basepal = indata+e->offset;
					if (!memcmp(basepal, host_basepal, 768))
						basepal = NULL;
				}
				else
					printf("\t%16.16s: palette - %u bytes\n", e->name, e->size);
				break;
			case 70:
				printf("\t%16.16s: Halflife Font (%u bytes)\n", e->name, e->size);
				break;
			default:
				printf("\t%16.16s: ENTRY TYPE %u (%u bytes)\n", e->name, e->type, e->size);
				break;
			}
		}
	}
	else if (fsize >= sizeof(dheader_t) && (
		!memcmp(indata, BSPVERSION)		||
		!memcmp(indata, BSPVERSIONHL)	||
		!memcmp(indata, BSPVERSIONPREREL)||
		!memcmp(indata, BSPVERSION_LONG1)||
		!memcmp(indata, BSPVERSION_LONG2)))
	{	//q1bsp
		dheader_t *bsp = (dheader_t*)indata;
		dmiptexlump_t *texlump = (dmiptexlump_t*)(indata + bsp->lumps[LUMP_TEXTURES].fileofs);
		miptex_t *miptex;
		size_t i;
		size_t sz = bsp->lumps[LUMP_TEXTURES].filelen;
		printf("%-20s: bsp file (%u textures)\n", inname, texlump->nummiptex);
		for (i = texlump->nummiptex; i --> 0; )
		{
			if (texlump->dataofs[i] < 0 || texlump->dataofs[i] >= bsp->lumps[LUMP_TEXTURES].filelen)
			{
				char syn[MAX_QPATH];
				sprintf(syn, "unnamed%u", (unsigned)i);
				printf("%-20s---<NO DATA>--- %d\n", syn, texlump->dataofs[i]);
				continue;
			}

			miptex = (miptex_t*)((qbyte*)texlump + texlump->dataofs[i]);

			in = ImgTool_DecodeMiptex(NULL, miptex, sz - texlump->dataofs[i], NULL);
			sz = texlump->dataofs[i];
			callback(miptex->name, in);
/*			if (in->encoding != PTI_P8)
				printf("\t%16.16s: %u*%u%s (%s: %i*%i)\n", miptex->name, miptex->width, miptex->height, miptex->offsets[0]?"":" (external data)", Image_FormatName(in->encoding), in->mip[0].width, in->mip[0].height);
			else
				printf("\t%16.16s: %u*%u%s\n", miptex->name, miptex->width, miptex->height, miptex->offsets[0]?"":" (external data)");
*/			ImgTool_FreeMips(in);
		}
	}
	else if (fsize >= sizeof(dmdl_t) && (
		!memcmp(indata,IDPOLYHEADER) && (((dmdl_t*)indata)->version == ALIAS_VERSION || ((dmdl_t*)indata)->version == 50)))
	{	//quake's mdl format. also hexen2's missionpack format.
		int i, j, numframes;
		char skinname[256];
		dmdl_t *mdl = (dmdl_t *)indata;
		daliasskintype_t *pskintype = (daliasskintype_t*)(indata + sizeof(*mdl)-((((dmdl_t*)indata)->version == 50)?0:sizeof(int)));
		daliasskingroup_t *pskingroup;
		daliasskininterval_t *intervals;
		uploadfmt_t encoding;

		struct pendingtextureinfo *out = Z_Malloc(sizeof(*out));

		if( mdl->flags & MFH2_TRANSPARENT )
			encoding = TF_H2_T7G1;	//hexen2
		else if( mdl->flags & MFH2_HOLEY )
			encoding = TF_H2_TRANS8_0;	//hexen2
		else if( mdl->flags & MFH2_SPECIAL_TRANS )
			encoding = TF_H2_T4A4;	//hexen2
		else
			encoding = TF_SOLID8;

		printf("%-20s: mdl file (%u skingroups)\n", inname, mdl->numskins);
		for (i = 0; i < mdl->numskins; i++)
		{
			switch(LittleLong(pskintype->type))
			{
			case ALIAS_SKIN_SINGLE:
				out = Z_Malloc(sizeof(*out));
				out->type = PTI_2D;
				out->encoding = encoding;
				out->mipcount = 1;
				out->mip[0].datasize = mdl->skinwidth*mdl->skinheight;
				out->mip[0].width = mdl->skinwidth;
				out->mip[0].height = mdl->skinheight;
				out->mip[0].depth = 1;
				out->mip[0].needfree = false;
				out->mip[0].data = (qbyte*)(pskintype+1);

				pskintype = (daliasskintype_t *)((char *)out->mip[0].data+out->mip[0].datasize);
				printf("\t%s_%u: %i*%i\n", inname, i, mdl->skinwidth, mdl->skinheight);
				snprintf(skinname, sizeof(skinname), "%s_%u", inname, i);
				callback(skinname, out);
				ImgTool_FreeMips(out);
				break;

			default:
				pskingroup = (daliasskingroup_t*)(pskintype+1);
				intervals = (daliasskininterval_t *)(pskingroup+1);
				numframes = LittleLong(pskingroup->numskins);
				for (j = 0; j < numframes; j++)
				{
					out = Z_Malloc(sizeof(*out));
					out->type = PTI_2D;
					out->encoding = encoding;
					out->mipcount = 1;
					out->mip[0].datasize = mdl->skinwidth*mdl->skinheight;
					out->mip[0].width = mdl->skinwidth;
					out->mip[0].height = mdl->skinheight;
					out->mip[0].depth = 1;
					out->mip[0].needfree = false;
					out->mip[0].data = (qbyte *)(intervals + numframes) + mdl->skinwidth*mdl->skinheight*j;

					printf("\t\t%s_%u_%u: %i*%i @ %g\n", inname, i, j, mdl->skinwidth, mdl->skinheight, intervals[j].interval);
					snprintf(skinname, sizeof(skinname), "%s_%u_%u", inname, i, j);
					callback(skinname, out);
					ImgTool_FreeMips(out);
				}
				pskintype = (daliasskintype_t *)((qbyte *)(intervals+numframes) + mdl->skinwidth*mdl->skinheight*numframes);
				break;
			}
		}
	}
	//else spr
	else
	{
		const miptex_t *mip = NULL;
		const char *ex = COM_GetFileExtension(inname, NULL);
		if (!strcasecmp(ex, ".mip"))
		{
			in = ImgTool_DecodeMiptex(NULL, (miptex_t*)indata, fsize, NULL);
			if (fsize >= sizeof(miptex_t))
				mip = (const miptex_t*)indata;
		}
		else
			in = Image_LoadMipsFromMemory(args->flags|IF_NOMIPMAP, inname, inname, indata, fsize);
		if (!in)
			printf("%-20s: unsupported format\n", inname);
		else
			callback(inname, in);
		(void)mip;
		ImgTool_FreeMips(in);
	}
	fflush(stdout);
}

struct filelist_s
{
	const char **exts;
	size_t numfiles;
	struct {
		const char *rootpath; //the basepath that was passed to the filelist scan.
		char *name;
		size_t baselen; //length up to but not including the filename extension.
	} *file;
	size_t maxfiles; //to avoid reallocs
};
static void FileList_Release(struct filelist_s *list)
{
	size_t i;
	for (i = 0; i < list->numfiles; i++)
		free(list->file[i].name);
	free(list->file);
	list->numfiles = 0;
	list->maxfiles = 0;
}
static void FileList_Add(struct filelist_s *list, const char *rootpath, const char *fname)
{
	size_t i;
	size_t baselen;
	const char *ext = COM_GetFileExtension(fname, NULL);
	for (i = 0; ; i++)
	{
		if (!list->exts[i])
			return;	//extension wasn't in the list.
		if (!strcmp(list->exts[i], ext))
			break;	//one of the accepted extensions
	}
	baselen = ext?ext-fname:strlen(fname);
	for (i = 0; i < list->numfiles; i++)
	{
		if (list->file[i].baselen == baselen && !strncasecmp(list->file[i].name, fname, baselen))
		{
			if (strcasecmp(list->file[i].name+baselen, fname+baselen) > 0)
			{
				Con_Printf("Ignoring dupe file %s (using %s)\n", list->file[i].name, fname);
				//use the 'lower' one instead, for consistency between systems.
				list->file[i].rootpath = rootpath;
				list->file[i].name = strdup(fname);
				list->file[i].baselen = baselen;
			}
			else
				Con_Printf("Ignoring dupe file %s (using %s)\n", fname, list->file[i].name);
			return; //file already listed, but maybe with a different extension
		}
	}
	if (i == list->maxfiles)
	{
		list->maxfiles += 64;
		list->file = realloc(list->file, sizeof(*list->file)*list->maxfiles);
	}
	list->file[i].rootpath = rootpath;
	list->file[i].name = strdup(fname);
	list->file[i].baselen = baselen;
	list->numfiles++;
}
#ifdef _WIN32
static void ImgTool_TreeScan(struct filelist_s *list, const char *rootpath, const char *subpath)
{	//FIXME: convert to utf-8.
	HANDLE h;
	WIN32_FIND_DATAA fd;
	char file[MAX_OSPATH];

	if (subpath && *subpath)
		Q_snprintfz(file, sizeof(file), "%s/%s", rootpath, subpath);
	else
		Q_snprintfz(file, sizeof(file), "%s", rootpath);
	if (GetFileAttributesA(file) & FILE_ATTRIBUTE_DIRECTORY)	//if its a directory then scan it.
		Q_snprintfz(file+strlen(file), sizeof(file)-strlen(file), "/*");

	h = FindFirstFileA(file, &fd);
	if (h != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (*fd.cFileName == '.')
				continue;	//skip .. (and unix hidden files, because urgh)

			if (subpath && *subpath)
				Q_snprintfz(file, sizeof(file), "%s/%s", subpath, fd.cFileName);
			else
				Q_snprintfz(file, sizeof(file), "%s", fd.cFileName);

			if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
				; //don't report hidden entries.
			else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				ImgTool_TreeScan(list, rootpath, file);
			else
				FileList_Add(list, rootpath, file);
		} while(FindNextFileA(h, &fd));
		FindClose(h);
	}
}
#else
#include <dirent.h>
#include <fnmatch.h>
static void ImgTool_TreeScan(struct filelist_s *list, const char *basepath, const char *subpath)
{
	DIR *dir;
	char file[MAX_OSPATH];
	struct dirent *ent;
	struct stat sb;

	if (subpath && *subpath)
		Q_snprintfz(file, sizeof(file), "%s/%s", basepath, subpath);
	else
		Q_snprintfz(file, sizeof(file), "%s", basepath);
	stat(file, &sb);
	if ((sb.st_mode & S_IFMT) == S_IFDIR)
	{
		dir = opendir(file);
		if (!dir)
		{
			Con_Printf("Failed to open dir %s\n", file);
			return;
		}
		for (;;)
		{
			ent = readdir(dir);
			if (!ent)
				break;
			if (*ent->d_name == '.')
				continue;
			else if (ent->d_type == DT_DIR)
			{
				if (!subpath)
					continue;
				if (*subpath)
					Q_snprintfz(file, sizeof(file), "%s/%s", subpath, ent->d_name);
				else
					Q_snprintfz(file, sizeof(file), "%s", ent->d_name);
				ImgTool_TreeScan(list, basepath, file);
			}
			else if (ent->d_type == DT_REG)
			{
				if (subpath && *subpath)
					Q_snprintfz(file, sizeof(file), "%s/%s", subpath, ent->d_name);
				else
					Q_snprintfz(file, sizeof(file), "%s", ent->d_name);
				FileList_Add(list, basepath, file);
			}
		}
		closedir(dir);
	}
	else
	{
		if (*file == '/')
			FileList_Add(list, "", file);
		else
			FileList_Add(list, ".", file);
	}
}
#endif
static void ImgTool_TreeConvert(struct opts_s *args, const char *destpath, const char *srcpath)
{
	size_t newfiles=0, skippedfiles=0, processedfiles=0;
	char file[MAX_OSPATH];
	char dest[MAX_OSPATH];
	const char *exts[] = {".png", ".bmp", ".tga", ".jpg", ".exr", ".hdr", ".pcx", NULL};
	struct filelist_s list = {exts};
	size_t i, destlen = strlen(destpath)+1;
	ImgTool_TreeScan(&list, srcpath, "");

	if (!list.numfiles)
		Con_Printf("No suitable files found in directory: %s\n", srcpath);

	for (i = 0; i < list.numfiles; i++)
	{
		struct stat statsrc, statdst;
		Q_snprintfz(file, sizeof(file), "%s/%s", srcpath, list.file[i].name);
		Q_snprintfz(dest, sizeof(dest), "%s/%s", destpath, list.file[i].name);
		Q_snprintfz(dest+destlen+list.file[i].baselen, sizeof(dest)-destlen-list.file[i].baselen, ".dds");

		if (stat(file, &statsrc) < 0)
		{
			Con_Printf("stat(\"%s\") failed...\n", file);
			continue;
		}
		if (stat(dest, &statdst) < 0)
		{
			statdst.st_mtime = INT_MIN; //make it look old
			newfiles++;
		}
		if (statdst.st_mtime <= statsrc.st_mtime)
		{
			processedfiles++;
//			Con_Printf("Image file %s -> %s\n", file, dest);
			FS_CreatePath(dest, FS_SYSTEM);
			ImgTool_Convert(args, ImgTool_Read(args, file), file, dest);
		}
		else
		{
			skippedfiles++;
//			Con_Printf("Unmodified image file %s -> %s\n", file, dest);
		}
	}
	Con_Printf("found: %u, processed: %u, skipped: %u, new: %u\n", (unsigned int)list.numfiles, (unsigned int)processedfiles, (unsigned int)skippedfiles, (unsigned int)newfiles);
	FileList_Release(&list);
	return;
}

static void ImgTool_WadExtract(struct opts_s *args, const char *wadname)
{
	qbyte *indata;
	size_t fsize;
	size_t m;
	qbyte *basepal = NULL;
	indata = FS_LoadMallocFile(wadname, &fsize);
	if (!indata)
		printf("%s: unable to read\n", wadname);
	else if (fsize >= sizeof(wad2_t) && indata[0] == 'W' && indata[1] == 'A' && indata[2] == 'D')
	{
		const wad2_t *w = (const wad2_t *)indata;
		const wad2entry_t *e = (const wad2entry_t *)(indata+w->offset);
		int i;
		char clean[sizeof(e->name)+1];

		for (m = 0; m < w->num; m++, e++)
		{
			switch(e->type)
			{
			case 67:	//hl...
			case TYP_MIPTEX:
				{
					miptex_t *mip = (miptex_t *)(indata+e->offset);

					if (!strcasecmp(e->name, "CONCHARS") && e->size==128*128)
					{	//special hack for conchars, which is listed as a miptex for some reason, with no qpic header (it not being a qpic lump)
						struct pendingtextureinfo *out = Z_Malloc(sizeof(*out));
						out->encoding = TF_H2_TRANS8_0;
						out->type = PTI_2D;
						out->mip[0].width = 128;
						out->mip[0].height = 128;
						out->mip[0].depth = 1;
						out->mip[0].datasize = out->mip[0].width*out->mip[0].height*out->mip[0].depth;
						out->mip[0].data = (char*)mip;
						out->mipcount = 1;
						ImgTool_Convert(args, out, "conchars", NULL);
						break;
					}

					ImgTool_DecodeMiptex(args, mip, e->dsize, basepal);
				}
				break;
			case TYP_QPIC:
				{
					int *qpic = (int *)(indata+e->offset);
					struct pendingtextureinfo *out = Z_Malloc(sizeof(*out));
					size_t sz;
					qbyte *p;

					if (e->size < 8 || 8+qpic[0]*qpic[1] != e->size)
					{
						printf("invalid size/header for qpic lump: %s\n", e->name);
						break;
					}

					out->type = PTI_2D;
					out->mip[0].width = LittleLong(qpic[0]);
					out->mip[0].height = LittleLong(qpic[1]);
					out->mip[0].depth = 1;
					out->mip[0].datasize = out->mip[0].width*out->mip[0].height*out->mip[0].depth;
					out->mip[0].data = (char*)(qpic+2);
					out->mipcount = 1;

					for (sz = 0, p = out->mip[0].data; sz < out->mip[0].datasize; sz++)
						if (p[sz] == 255)
							break;
					out->encoding = sz<out->mip[0].datasize?TF_TRANS8:PTI_P8;

					for (i = 0; i < sizeof(e->name); i++)
					{	//lowercase it.
						if (e->name[i] >= 'A' && e->name[i] <= 'Z')
							clean[i] = (e->name[i]-'A')+'a';
						else
							clean[i] = e->name[i];
					}
					clean[sizeof(e->name)] = 0;

					ImgTool_Convert(args, out, clean, NULL);
				}
				break;
			case TYP_PALETTE:
				if (e->size == 768)
				{
					basepal = indata+e->offset;
					if (!memcmp(basepal, host_basepal, 768))
						basepal = NULL;
				}
				FALLTHROUGH
			default:
				printf("skipping %s\n", e->name);
				break;
			}
		}
	}
	else if (fsize >= sizeof(dheader_t) && (
		!memcmp(indata, BSPVERSION)		||
//		!memcmp(indata, BSPVERSIONHL)	||
		!memcmp(indata, BSPVERSIONPREREL)||
		!memcmp(indata, BSPVERSION_LONG1)||
		!memcmp(indata, BSPVERSION_LONG2)))
	{	//q1bsp
		dheader_t *bsp = (dheader_t*)indata;
		dmiptexlump_t *texlump = (dmiptexlump_t*)(indata + bsp->lumps[LUMP_TEXTURES].fileofs);
		miptex_t *miptex;
		size_t i;
		size_t sz = bsp->lumps[LUMP_TEXTURES].filelen;
		for (i = texlump->nummiptex; i --> 0; )
		{
			if (texlump->dataofs[i] < 0 || texlump->dataofs[i] >= bsp->lumps[LUMP_TEXTURES].filelen)
				continue;

			miptex = (miptex_t*)((qbyte*)texlump + texlump->dataofs[i]);
			if (*miptex->name && miptex->width && miptex->height && miptex->offsets[0]>0)
			{
				ImgTool_DecodeMiptex(args, miptex, sz - texlump->dataofs[i], basepal);
				sz = texlump->dataofs[i];
			}
		}
	}
	else if (fsize >= sizeof(dmdl_t) && (
		!memcmp(indata,IDPOLYHEADER) && ((dmdl_t*)indata)->version == ALIAS_VERSION))
	{
		char imgname[1024];
		int i, j, numframes;
		dmdl_t *mdl = (dmdl_t *)indata;
		daliasskintype_t *pskintype = (daliasskintype_t*)(indata + sizeof(*mdl));
		daliasskingroup_t *pskingroup;
		daliasskininterval_t *intervals;

		struct pendingtextureinfo *out = Z_Malloc(sizeof(*out));
		out->type = PTI_2D;

	#ifdef HEXEN2
		if( mdl->flags & MFH2_TRANSPARENT )
			out->encoding = TF_H2_T7G1;	//hexen2
		else
	#endif
		 if( mdl->flags & MFH2_HOLEY )
			out->encoding = TF_H2_TRANS8_0;	//hexen2
	#ifdef HEXEN2
		else if( mdl->flags & MFH2_SPECIAL_TRANS )
			out->encoding = TF_H2_T4A4;	//hexen2
	#endif
		else
			out->encoding = TF_SOLID8;

		out->mipcount = 1;
		out->mip[0].datasize = mdl->skinwidth*mdl->skinheight;
		out->mip[0].width = mdl->skinwidth;
		out->mip[0].height = mdl->skinheight;
		out->mip[0].depth = 1;

		for (i = 0; i < mdl->numskins; i++)
		{
			switch(LittleLong(pskintype->type))
			{
			case ALIAS_SKIN_SINGLE:
				out->mip[0].data = (qbyte*)(pskintype+1);
				Q_snprintfz(imgname, sizeof(imgname), "%s_%i.", wadname, i);
				ImgTool_Convert(args, out, imgname, NULL);
				pskintype = (daliasskintype_t *)((char *)out->mip[0].data+out->mip[0].datasize);
				break;

			default:
				pskingroup = (daliasskingroup_t*)(pskintype+1);
				intervals = (daliasskininterval_t *)(pskingroup+1);
				numframes = LittleLong(pskingroup->numskins);
				out->mip[0].data = (qbyte *)(intervals + numframes);

				for (j = 0; j < numframes; j++,out->mip[0].data=(char*)out->mip[0].data+out->mip[0].datasize)
				{
					Q_snprintfz(imgname, sizeof(imgname), "%s_%i_%i.", wadname, i, j);
					ImgTool_Convert(args, out, imgname, NULL);
				}
				pskintype = (daliasskintype_t *)out->mip[0].data;
				break;
			}
		}
	}
	else
		printf("%s: does not appear to be a wad file\n", wadname);
}
int Image_SortPalette(const void *av, const void *bv)
{
	const struct
	{
		qbyte r,g,b;
		int count;
	} *a=av, *b=bv;
	return b->count - a->count;
}
static qbyte *Image_GenPalette(struct pendingtextureinfo *in, const char *name)
{
	unsigned int t;
	qbyte *pal = Z_Malloc(768);
	qbyte *d;
	qboolean hasalpha = false;
	struct
	{
		qbyte r,g,b;
		int count;
	} *p = NULL;
	size_t nump=0, maxp=0, i;
	static qboolean mippixelformats[PTI_MAX] = {[PTI_RGBA8]=true};
	Image_ChangeFormat(in, mippixelformats, PTI_INVALID, name); //make sure its rgbx
	t = in->mip[0].width*in->mip[0].height*in->mip[0].depth;
	d = in->mip[0].data;
	for(; t --> 0; d += 4)
	{
		for(i = 0; i < nump; i++)
		{
			if (p[i].r == d[0] &&
				p[i].g == d[1] &&
				p[i].b == d[2])
			{
				p[i].count++;
				break;
			}
		}
		if (i == nump)
		{	//new rgb value.
			if (nump == maxp)
			{	//urgh. need more.
				maxp += 64;
				p = realloc(p, sizeof(*p)*maxp);
			}
			p[i].r = d[0];
			p[i].g = d[1];
			p[i].b = d[2];
			p[i].count = 1;
			nump++;
		}
	}
	qsort(p, nump, sizeof(*p), Image_SortPalette);
	if (nump > 255)
		Con_Printf("%s: %u unique colours\n", name, (unsigned)nump);
	else
		Con_DPrintf("%s: %u unique colours\n", name, (unsigned)nump);
	if (nump >= 256)
		nump = 256;
	else
		memset(pal, 0, 768); //make sure its set to something.
	for (i = 0; i < nump; i++)
	{	//fill in the 256 most common colours. should probably compress them a bit if there's more.
		pal[i*3+0] = p[i].r;
		pal[i*3+1] = p[i].g;
		pal[i*3+2] = p[i].b;
	}

	if (hasalpha == true) {
		pal[255*3+0] = 0;
		pal[255*3+1] = 0;
		pal[255*3+2] = 255;
	}

	free(p);
	return pal;
}
//spits out our extended .mip format
static qboolean ImgTool_MipExport(struct opts_s *args, vfsfile_t *outfile, struct pendingtextureinfo *in, const char *mipname, int wadtype)
{
	struct pendingtextureinfo *highcolour = NULL;
	char *highcode = NULL, *ext;
	size_t u;
	unsigned int m, tsz;
	miptex_t mip;
	qbyte *pal = host_basepal;

	static qboolean mippixelformats[PTI_MAX] = {[PTI_P8]=true};

	if (!in || !in->mipcount)
	{
		Con_Printf("%s: unable to load any mips\n", mipname);
		return false;
	}

	in = ImgTool_DupeMipchain(in);
	if (in->mipcount < 4)
		Image_GenerateMips(in, args->flags);
	if (in->encoding == PTI_P8 || args->newpixelformat == PTI_INVALID)
	{
		highcode = NULL; //no, don't store it weirdly...
	}
	else
	{
		highcolour = ImgTool_DupeMipchain(in);
		Image_GenerateMips(highcolour, args->flags);
		for (u = 1; u < countof(sh_config.texfmt); u++)
			sh_config.texfmt[u] = true;
		if (!ImgTool_ConvertPixelFormat(args, mipname, highcolour))
		{
			Con_Printf("%s: Unable to convert to requested pixel format\n", mipname);
			ImgTool_FreeMips(highcolour);
			highcolour = NULL;
		}
		else if (highcolour->mip[highcolour->mipcount-1].width != 1 || highcolour->mip[highcolour->mipcount-1].height != 1)
		{
			Con_Printf("%s: Mipchain truncated\n", mipname);
			ImgTool_FreeMips(highcolour);
			highcolour = NULL;
		}
		else for (u = 1; u < highcolour->mipcount; u++)
		{	//mip chain must round down consistently.
			if (highcolour->mip[u].width != max(1,highcolour->mip[u-1].width>>1) ||
				highcolour->mip[u].height!= max(1,highcolour->mip[u-1].height>>1))
			{
				Con_Printf("%s: Mipchain sized wrongly\n", mipname);
				ImgTool_FreeMips(highcolour);
				highcolour = NULL;
				break;
			}
		}

		if (highcolour)	switch(highcolour->encoding)
		{
		case PTI_BC1_RGB:
		case PTI_BC1_RGBA:		highcode = "BC1";	break;	//not in any core gl, but uniquitous on desktop, but not mobile.
		case PTI_BC2_RGBA:		highcode = "BC2";	break;
		case PTI_BC3_RGBA:		highcode = "BC3";	break;
		case PTI_BC4_R:			highcode = "BC4";	break;
		case PTI_BC5_RG:		highcode = "BC5";	break;
		case PTI_BC6_RGB_UFLOAT:highcode = "BC6";	break;	//aka bptc, core in gl4.2 (not gles)
		case PTI_BC7_RGBA:		highcode = "BC7";	break;	//aka bptc, core in gl4.2 (not gles)
		case PTI_ETC1_RGB8:		highcode = "ETC1";	break;	//available on most gles2 devices.
		case PTI_ETC2_RGB8:		highcode = "ETC2";	break;	//core in gles3 (or gl4.3)
		case PTI_ETC2_RGB8A1:	highcode = "ETCP";	break;	//core in gles3 (or gl4.3)
		case PTI_ETC2_RGB8A8:	highcode = "ETCA";	break;	//core in gles3 (or gl4.3)
		case PTI_ASTC_4X4_LDR:	highcode = "AST4";	break;	//core in gles3.2
		case PTI_ASTC_5X4_LDR:	highcode = "AS54";	break;	//core in gles3.2
		case PTI_ASTC_5X5_LDR:	highcode = "AST5";	break;	//core in gles3.2
		case PTI_ASTC_6X5_LDR:	highcode = "AS65";	break;	//core in gles3.2
		case PTI_ASTC_6X6_LDR:	highcode = "AST6";	break;	//core in gles3.2
		case PTI_ASTC_8X5_LDR:	highcode = "AS85";	break;	//core in gles3.2
		case PTI_ASTC_8X6_LDR:	highcode = "AS86";	break;	//core in gles3.2
		case PTI_ASTC_10X5_LDR:	highcode = "AS05";	break;	//core in gles3.2
		case PTI_ASTC_10X6_LDR:	highcode = "AS06";	break;	//core in gles3.2
		case PTI_ASTC_8X8_LDR:	highcode = "AST8";	break;	//core in gles3.2
		case PTI_ASTC_10X8_LDR:	highcode = "AS08";	break;	//core in gles3.2
		case PTI_ASTC_10X10_LDR:highcode = "AST0";	break;	//core in gles3.2
		case PTI_ASTC_12X10_LDR:highcode = "AS20";	break;	//core in gles3.2
		case PTI_ASTC_12X12_LDR:highcode = "AST2";	break;	//core in gles3.2
		case PTI_RGB565:		highcode = "565";	break;
		case PTI_RGBA5551:		highcode = "5551";	break;
		case PTI_RGBA4444:		highcode = "4444";	break;
		case PTI_RGB8:			highcode = "RGB";	break;	//generally needs reformatting to rgbx.
		case PTI_RGBA8:			highcode = "RGBA";	break;	//bloaty
		case PTI_L8:			highcode = "LUM8";	break;
		case PTI_E5BGR9:		highcode = "EXP5";	break;	//gl3+
		default:
			Con_Printf("%s: unsupported pixel format(%s) for miptex\n", mipname, Image_FormatName(highcolour->encoding));
			ImgTool_FreeMips(highcolour);
			highcolour = NULL;
			break;
		}
	}

	if (args->width && args->height && in->mipcount >= 1)
	{
		qbyte *newimg;
		unsigned int bb, bw, bh, bd;
		Image_BlockSizeForEncoding(in->encoding, &bb, &bw, &bh, &bd);
		newimg = Image_ResampleTexture(in->encoding, in->mip[0].data, in->mip[0].width, in->mip[0].height, NULL, args->width, args->height);
		if (newimg)
		{
			in->mipcount = 1;	//urgh
			if (in->mip[0].needfree)
				BZ_Free(in->mip[0].data);
			in->mip[0].data = newimg;
			in->mip[0].needfree = true;
			in->mip[0].width = args->width;
			in->mip[0].height = args->height;
			in->mip[0].depth = 1;
			in->mip[0].datasize = bb*((in->mip[0].width+bw-1)/bw)*((in->mip[0].height+bh-1)/bh)*((in->mip[0].depth+bd-1)/bd);

			Image_GenerateMips(in, args->flags);
		}
		else
			Con_Printf("%s: unable to resize %s\n", mipname, Image_FormatName(in->encoding));
	}

	if (args->mipnum >= in->mipcount)
	{
		Con_Printf("%s: not enough mips\n", mipname);
		ImgTool_FreeMips(in);
		ImgTool_FreeMips(highcolour);
		return false;
	}

	//strip out all but the 4 mip levels we care about.
	for (u = 0; u < in->mipcount; u++)
	{
		if (u >= args->mipnum && u < args->mipnum+4)
		{
			if (wadtype<2)
			{	//if we're stripping out the wad data (so that the engine ends up requiring external textures) then do it now before palettizing, for efficiency.
				if (in->mip[u].needfree)
					BZ_Free(in->mip[u].data);
				in->mip[u].data = NULL;
				in->mip[u].datasize = 0;
			}
		}
		else
		{
			if (in->mip[u].needfree)
				BZ_Free(in->mip[u].data);
			memset(&in->mip[u], 0, sizeof(in->mip[u]));
		}
	}
	in->mipcount -= args->mipnum;
	if (in->mipcount > 4)
		in->mipcount = 4;
	memmove(&in->mip[0], &in->mip[args->mipnum], sizeof(in->mip[0])*in->mipcount);
	memset(&in->mip[in->mipcount], 0, sizeof(in->mip[0])*((args->mipnum+4)-in->mipcount)); //null it out, just in case.

	if (in->mip[0].data)
	{
		if (in->encoding != PTI_P8)
		{
			qbyte *basepal = host_basepal;
			if (wadtype == 3) {
				pal = host_basepal = Image_GenPalette(in, mipname);
			}

			if (*mipname=='{') {
				memset(mippixelformats, 0, sizeof(mippixelformats));
				mippixelformats[TF_TRANS8] = true;
			}

			Image_ChangeFormat(in, mippixelformats, (*mipname=='{')?TF_TRANS8:PTI_INVALID, mipname);
			host_basepal = basepal;
		}
		if (!(in->encoding == PTI_P8 || in->encoding == TF_TRANS8))
		{	//erk! we failed to palettize...
			ImgTool_FreeMips(in);
			ImgTool_FreeMips(highcolour);
			Con_Printf("%s: paletizing error (source format %s)\n", mipname, Image_FormatName(in->encoding));
			return false;
		}
	}

	if (!in->mip[0].width || (in->mip[0].width & 15))
		Con_Printf("%s(%i): WARNING: miptex width is not a multiple of 16 - %i*%i\n", mipname, args->mipnum, in->mip[0].width, in->mip[0].height);
	if (!in->mip[0].height || (in->mip[0].height & 15))
		Con_Printf("%s(%i): WARNING: miptex height is not a multiple of 16 - %i*%i\n", mipname, args->mipnum, in->mip[0].width, in->mip[0].height);

	memset(mip.name, 0, sizeof(mip.name));
	Q_strncpyz(mip.name, mipname, sizeof(mip.name));
	ext = (char*)COM_GetFileExtension (mip.name, NULL);
	while (*ext) *ext++=0;
	if (*mip.name == '#')
		*mip.name = '*';	//make it a proper turb
	mip.width = in->mip[0].width;
	mip.height = in->mip[0].height;
	mip.offsets[0] = in->mip[0].datasize?sizeof(mip):0;
	mip.offsets[1] = in->mip[1].datasize?mip.offsets[0]+in->mip[0].datasize:0;
	mip.offsets[2] = in->mip[2].datasize?mip.offsets[1]+in->mip[1].datasize:0;
	mip.offsets[3] = in->mip[3].datasize?mip.offsets[2]+in->mip[2].datasize:0;

	tsz = sizeof(mip)+in->mip[0].datasize+in->mip[1].datasize+in->mip[2].datasize+in->mip[3].datasize;
	VFS_WRITE(outfile, &mip, sizeof(mip));
	VFS_WRITE(outfile, in->mip[0].data, in->mip[0].datasize);
	VFS_WRITE(outfile, in->mip[1].data, in->mip[1].datasize);
	VFS_WRITE(outfile, in->mip[2].data, in->mip[2].datasize);
	VFS_WRITE(outfile, in->mip[3].data, in->mip[3].datasize);
	if (wadtype == 3)
	{
		tsz += 2 + 256*3;
		VFS_WRITE(outfile, "\x00\x01", 2);
		VFS_WRITE(outfile, pal, 256*3);
	}

	if (highcolour)
	{
		unsigned int highsize;
		VFS_WRITE(outfile, "\x00\xfb\x2b\xaf", 4);	//magic id to say that there's actually extensions here...
		tsz += 4;
		//spit out our high-colour lump here
		for (highsize = 16, m = 0; m < highcolour->mipcount; m++)
			highsize += highcolour->mip[m].datasize;
		VFS_WRITE(outfile, &highsize, 4);
		VFS_WRITE(outfile, highcode, 4);
		VFS_WRITE(outfile, &highcolour->mip[0].width, 4);
		VFS_WRITE(outfile, &highcolour->mip[0].height, 4);
		for (m = 0; m < highcolour->mipcount; m++)
			VFS_WRITE(outfile, highcolour->mip[m].data, highcolour->mip[m].datasize);
		tsz += highsize;

		ImgTool_FreeMips(highcolour);

		Con_Printf("%s: %ix%i (%s: %ix%i %i)\n", mip.name, mip.width, mip.height, highcode, highcolour->mip[0].width, highcolour->mip[0].height, highcolour->mipcount);
	}
	else
		Con_Printf("%s: %ix%i%s\n", mip.name, mip.width, mip.height, wadtype==3?"+pal":"");

	//and pad it, just in case.
	if (tsz & 3)
		VFS_WRITE(outfile, "\0\0\0\0", 4-(tsz&3));

	ImgTool_FreeMips(in);
	return true;
}
static void ImgTool_WadConvert(struct opts_s *args, const char *destpath, const char **srcpaths, size_t numpaths, int wadtype/*x,2,3*/)
{
	char file[MAX_OSPATH];
	const char *exts[] = {".mip", ".png", ".bmp", ".tga", ".exr", ".hdr", ".dds", ".ktx", ".xcf", ".pcx", ".jpg", NULL};
	struct filelist_s list = {exts};
	size_t i, u;
	vfsfile_t *f;
	char *inname;
	qbyte *indata;
	size_t fsize;
	wad2_t wad2;
	wad2entry_t *wadentries = NULL, *entry;
	size_t maxentries = 0;
	qboolean qpics;
	struct pendingtextureinfo *in;
	if (!numpaths)
		ImgTool_TreeScan(&list, ".", NULL);
	else while(numpaths --> 0)
		ImgTool_TreeScan(&list, *srcpaths++, NULL);

	if (!list.numfiles)
	{
		printf("%s: No files specified\n", destpath);
		return;
	}

	if (!strcasecmp(COM_GetFileExtension(destpath, NULL), ".bsp"))
	{
	}
	else
	{
		qpics = !strcasecmp("gfx.wad", COM_SkipPath(destpath));

		f = FS_OpenVFS(destpath, "wb", FS_SYSTEM);
		wad2.magic[0] = 'W';
		wad2.magic[1] = 'A';
		wad2.magic[2] = 'D';
		wad2.magic[3] = (wadtype==3)?'3':'2';	//wad3 instead of 2, so we can include a palette for tools to validate against
		wad2.num = 0;
		wad2.offset = 0;
		VFS_WRITE(f, &wad2, 12);

		if (wadtype == 2 && !qpics)
		{	//WAD2 texture files generally have a palette lump.
			if (wad2.num == maxentries)
			{
				maxentries += 64;
				wadentries = realloc(wadentries, sizeof(*wadentries)*maxentries);
			}
			entry = &wadentries[wad2.num++];
			memset(entry, 0, sizeof(*entry));
			Q_strncpyz(entry->name, "PALETTE", 16);
			entry->type = TYP_PALETTE;
			entry->offset = VFS_TELL(f);

			//and the lump data.
			VFS_WRITE(f, host_basepal, 256*3);

			entry->size = entry->dsize = VFS_TELL(f)-entry->offset;
		}

		for (i = 0; i < list.numfiles; i++)
		{
			Q_snprintfz(file, sizeof(file), "%s/%s", list.file[i].rootpath, list.file[i].name);
			inname = list.file[i].name;
			if (list.file[i].baselen > 15)
			{
				Con_Printf("Path too long for wad - %s\n", inname);
				continue;
			}
			indata = FS_LoadMallocFile(file, &fsize);
			if (!indata)
			{
				Con_Printf("Unable to open %s\n", inname);
				continue;
			}

			if (wad2.num == maxentries)
			{
				maxentries += 64;
				wadentries = realloc(wadentries, sizeof(*wadentries)*maxentries);
			}
			entry = &wadentries[wad2.num];
			memset(entry, 0, sizeof(*entry));
			Q_strncpyz(entry->name, inname, 16);
			if (list.file[i].baselen < sizeof(entry->name))
				entry->name[list.file[i].baselen] = 0; //kill any .tga
			if (*entry->name == '#')
				*entry->name = '*';	//* is not valid in a filename, yet needed for turbs, so by convention # is used instead. this is only relevant for the first char.
			entry->type = TYP_MIPTEX;
			entry->offset = VFS_TELL(f);

			if (!strcasecmp(COM_GetFileExtension(file, NULL), ".mip"))
			{	//.mip files can just be loaded directly
				//I just hope they are actually q1 format and not hl, for instance.
				if (wadtype == 3)
				{
					Con_Printf("refusing to inject q1 miptex into halflife wad-3 file\n");
					Z_Free(indata);
					continue;
				}
				VFS_WRITE(f, indata, fsize);
				wad2.num++;
				entry->size = entry->dsize = VFS_TELL(f)-entry->offset;
				Z_Free(indata);
			}
			else
			{
				if (wadtype == 3)
				{
					for (u = 0; u < sizeof(entry->name); u++)
						entry->name[u] = toupper(entry->name[u]);
					entry->type = 67;	//halflife's mips actually use a different type from q1 ones.
				}

				//try to decompress everything to a nice friendly palletizable range.
				for (u = 1; u < countof(sh_config.texfmt); u++)
					sh_config.texfmt[u] = (u==PTI_RGBA8)||(u==PTI_RGBX8)||(u==PTI_P8)||(u==args->newpixelformat);
				in = Image_LoadMipsFromMemory(args->flags, inname, file, indata, fsize);

				if (qpics)
				{
					qboolean mippixelformats[PTI_MAX];
					memset(mippixelformats, 0, sizeof(mippixelformats));

					if (!strcasecmp(entry->name, "CONCHARS") && in->mip[0].width==128&&in->mip[0].height==128)
					{
						mippixelformats[TF_H2_TRANS8_0] = true;

						entry->type = TYP_MIPTEX;	//yes, weird. match vanilla quake. explicitly avoid qpic to avoid corruption in the first 8 bytes (due to the engine's early endian swapping)
												//FIXME: encoding should be pti_trans8_0...
					}
					else
					{
						mippixelformats[TF_TRANS8] = true;
						entry->type = TYP_QPIC;
						//qpics need a header
						VFS_WRITE(f, &in->mip[0].width, sizeof(int));
						VFS_WRITE(f, &in->mip[0].height, sizeof(int));
					}
					if (!mippixelformats[in->encoding])
						Image_ChangeFormat(in, mippixelformats, PTI_INVALID, entry->name);
					if (!mippixelformats[in->encoding])
						continue;
					//and now the 8bit pixel data itself
					VFS_WRITE(f, in->mip[0].data, in->mip[0].datasize);
				}
				else if (!ImgTool_MipExport(args, f, in, entry->name, wadtype))
					continue;

				wad2.num++;
				entry->size = entry->dsize = VFS_TELL(f)-entry->offset;
			}
		}
		wad2.offset = VFS_TELL(f);
		VFS_WRITE(f, wadentries, sizeof(*wadentries)*wad2.num);
		VFS_SEEK(f, 0);
		VFS_WRITE(f, &wad2, sizeof(wad2));
		VFS_CLOSE(f);
	}

	free(wadentries);
	FileList_Release(&list);
}

#ifdef FTE_SDL
static void SDLL_Loop(void);
static void ImgTool_View(const char *inname, struct pendingtextureinfo *in);
#endif
int main(int argc, const char **argv)
{
	static const struct
	{
		const char *alias;
		uploadfmt_t fmt;
	} fmtaliases[] = {
		{"BC1", PTI_BC1_RGBA},
		{"BC2", PTI_BC2_RGBA},
		{"BC3", PTI_BC3_RGBA},
		{"BC4", PTI_BC4_R},
		{"BC5", PTI_BC5_RG},
		{"BC6", PTI_BC6_RGB_UFLOAT},
		{"BC7", PTI_BC7_RGBA},
		{"ETC1", PTI_ETC1_RGB8},
		{"ETC2", PTI_ETC2_RGB8},
		{"ETCP", PTI_ETC2_RGB8A1},
		{"ETCA", PTI_ETC2_RGB8A8},
		{"ASTC4x4",	PTI_ASTC_4X4_LDR},
		{"ASTC5x4", PTI_ASTC_5X4_LDR},
		{"ASTC5x5",	PTI_ASTC_5X5_LDR},
		{"ASTC6x5", PTI_ASTC_6X5_LDR},
		{"ASTC6x6",	PTI_ASTC_6X6_LDR},
		{"ASTC8x5", PTI_ASTC_8X5_LDR},
		{"ASTC8x6", PTI_ASTC_8X6_LDR},
		{"ASTC10x5",PTI_ASTC_10X5_LDR},
		{"ASTC10x6",PTI_ASTC_10X6_LDR},
		{"ASTC8x8",	PTI_ASTC_8X8_LDR},
		{"ASTC10x8",PTI_ASTC_10X8_LDR},
		{"ASTC10x10",PTI_ASTC_10X10_LDR},
		{"ASTC12x10",PTI_ASTC_12X10_LDR},
		{"ASTC12x12",PTI_ASTC_12X12_LDR},
		{"LUM8", PTI_L8},
		{"RGBA", PTI_RGBA8},
		{"RGB", PTI_RGB8},
	};
	enum
	{
		mode_unspecified,
		mode_info,
#ifdef FTE_SDL
		mode_view,
#endif
		mode_convert,
		mode_autotree,
		mode_genwadx,
		mode_genwad2,
		mode_genwad3,
		mode_extractwad,
	} mode = mode_unspecified;
	size_t u, f;
	qboolean nomoreopts = false;
	struct opts_s args;
	size_t files = 0;
	const char *outname = NULL;

	for (u = 1; u < countof(sh_config.texfmt); u++)
		sh_config.texfmt[u] = true;

	args.flags = 0;
	args.newpixelformat = PTI_INVALID;
	args.mipnum = 0;
	args.textype = PTI_ANY;
	args.defaultext = NULL;
	args.width = args.height = 0;

	if (argc==1)
		goto showhelp;

	for (u = 1; u < argc; u++)
	{
		if (*argv[u] == '-' && !nomoreopts)
		{
			if (!strcmp(argv[u], "--"))
				nomoreopts = true;
			else if (!strcmp(argv[u], "-?") || !strcmp(argv[u], "--help"))
			{
showhelp:
				Con_Printf(DISTRIBUTION " Image Tool\n");
				Con_Printf("show info  : %s [-i] in.ktx [in2.ext ...]\n", argv[0]);
				Con_Printf("compress   : %s [-c] --ext ktx --astc_6x6_ldr [--nomips] in.png [in2.png ...]\n", argv[0]);
				Con_Printf("compress   : %s [-c] --ext dds --bc3 [--premul] [--nomips] in.png\n\tConvert pixel format (to bc3 aka dxt5) before writing to output file.\n", argv[0]);
				Con_Printf("convert    : %s [-c] --ext png in.exr [in2.pcx ...]\n\tConvert input file(s) to different file format, while trying to preserve pixel formats.\n", argv[0]);
				Con_Printf("merge      : %s -o output [--cube|--3d|--2darray|--cubearray] [--bc1] foo_*.png\n\tConvert to different file format, while trying to preserve pixel formats.\n", argv[0]);
				Con_Printf("recursive  : %s -r [--ext dds] --astc_6x6_ldr destdir srcdir\n\tCompresses the files to dds (writing to an optionally different directory)\n", argv[0]);
				Con_Printf("decompress : %s --decompress [--exportmip 0] [--nomips] in.ktx out.png\n\tDecompresses any block-compressed pixel data.\n", argv[0]);
				Con_Printf("create mips: %s [-c] --ext mip [--bc1] [--resize width height] [--exportmip 2] *.dds\n", argv[0]);
				Con_Printf("create xwad: %s --genwadx [--exportmip 2] [--bc1] out.wad srcdir\n", argv[0]);
				Con_Printf("create wad : %s -w [--exportmip 2] out.wad *.mipsrcdir\n", argv[0]);
				Con_Printf("extract wad: %s -x [--ext png] src.wad\n", argv[0]);
				Con_Printf("extract bsp: %s -x [--ext png] src.bsp\n", argv[0]);

				Image_PrintInputFormatVersions();
				Con_Printf("Supported compressed/interesting pixelformats are:\n");
				for (f = 0; f < PTI_MAX; f++)
				{
					int bb,bw,bh,bd;
					Image_BlockSizeForEncoding(f, &bb,&bw,&bh,&bd);
					if (f >= PTI_ASTC_FIRST && f <= PTI_ASTC_LAST)
					{
						if (f >= PTI_ASTC_4X4_SRGB)
							continue;
						Con_Printf(" --%-16s %5.3g-bpp (requires astcenc)\n", Image_FormatName(f), 8*(float)bb/(bw*bh*bd));
					}
					else if (f==PTI_BC1_RGB||f==PTI_BC1_RGBA||f==PTI_BC2_RGBA||f==PTI_BC3_RGBA||f==PTI_BC4_R||f==PTI_BC5_RG)
						Con_Printf(" --%-16s %5.3g-bpp (requires nvcompress)\n", Image_FormatName(f), 8*(float)bb/(bw*bh*bd));
					else if (f==PTI_BC6_RGB_UFLOAT || f==PTI_BC6_RGB_SFLOAT || f==PTI_BC7_RGBA)
						Con_Printf(" --%-16s %5.3g-bpp (requires nvcompress 2.1+)\n", Image_FormatName(f), 8*(float)bb/(bw*bh*bd));
					else if (	f==PTI_RGBA16F ||
								f==PTI_RGBA32F ||
								f==PTI_E5BGR9 ||
								f==PTI_B10G11R11F ||
								f==PTI_RGB565 ||
								f==PTI_RGBA4444 ||
								f==PTI_ARGB4444 ||
								f==PTI_RGBA5551 ||
								f==PTI_ARGB1555 ||
								f==PTI_A2BGR10 ||
//								f==PTI_R8 ||
//								f==PTI_R16 ||
//								f==PTI_R16F ||
//								f==PTI_R32F ||
								f==PTI_RG8 ||
								f==PTI_L8 ||
								f==PTI_L8A8 ||
							0)
						Con_Printf(" --%-16s %5.3g-bpp\n", Image_FormatName(f), 8*(float)bb/(bw*bh*bd));
//					else
//						Con_DPrintf(" --%-16s %5.3g-bpp (unsupported)\n", Image_FormatName(f), 8*(float)bb/(bw*bh*bd));
				}
				return EXIT_SUCCESS;
			}
			else if (!files && (!strcmp(argv[u], "-c") || !strcmp(argv[u], "--convert")))
				mode = mode_convert;
			else if (!files && (!strcmp(argv[u], "-d") || !strcmp(argv[u], "--decompress")))
			{	//remove any (weird) gpu formats
				for (f = PTI_BC1_RGB; f < PTI_ASTC_LAST; f++)
					sh_config.texfmt[f] = false;
				mode = mode_convert;
			}
			else if (!files && (!strcmp(argv[u], "-r") || !strcmp(argv[u], "--auto")))
				mode = mode_autotree;
			else if (!files && (!strcmp(argv[u], "-i") || !strcmp(argv[u], "--info")))
				mode = mode_info;
#ifdef FTE_SDL
			else if (!files && (!strcmp(argv[u], "-v") || !strcmp(argv[u], "--view")))
				mode = mode_view;
#endif
			else if (!files && (!strcmp(argv[u], "-w") || !strcmp(argv[u], "--genwad2")))
				mode = mode_genwad2;
			else if (!files && (!strcmp(argv[u], "-w") || !strcmp(argv[u], "--genwad3")))
				mode = mode_genwad3;
			else if (!files && (!strcmp(argv[u], "-w") || !strcmp(argv[u], "--genwadx")))
				mode = mode_genwadx;
			else if (!files && (!strcmp(argv[u], "-x") || !strcmp(argv[u], "--extractwad")))
				mode = mode_extractwad;
			else if (!files && (!strcmp(argv[u], "-v") || !strcmp(argv[u], "--verbose")))
				verbose = true;
			else if (!files && (!strcmp(argv[u], "-o") || !strcmp(argv[u], "--outfile")))
			{
				if (u+1 < argc)
					outname = argv[++u];
				else
				{
					Con_Printf("--outfile requires output filename\n");
					return 1;
				}
			}
			else if (!files && (!strcmp(argv[u], "-p") || !strcmp(argv[u], "--palette")))
			{
				if (u+1 < argc)
					palette = argv[++u];
				else
				{
					Con_Printf("--palette requires palette filename\n");
					return 1;
				}
			}
			else if (!strcmp(argv[u], "--resize"))
			{
				if (u+2 < argc)
				{
					args.width = atoi(argv[++u]);
					args.height = atoi(argv[++u]);
				}
				else
				{
					Con_Printf("--resize requires width+height values\n");
					return 1;
				}
			}
			else if (!strcmp(argv[u], "--2d"))
				args.textype = PTI_2D;
			else if (!strcmp(argv[u], "--3d"))
				args.textype = PTI_3D;
			else if (!strcmp(argv[u], "--cube"))
				args.textype = PTI_CUBE;
			else if (!strcmp(argv[u], "--2darray"))
				args.textype = PTI_2D_ARRAY;
			else if (!strcmp(argv[u], "--cubearray"))
				args.textype = PTI_CUBE_ARRAY;
			else if (!strcmp(argv[u], "--nomips")	)
				args.flags |= IF_NOMIPMAP;
			else if (!strcmp(argv[u], "--mips"))
				args.flags &= ~IF_NOMIPMAP;
			else if (!strcmp(argv[u], "--premul")	)
				args.flags |= IF_PREMULTIPLYALPHA;
			else if (!strcmp(argv[u], "--nopremul"))
				args.flags &= ~IF_PREMULTIPLYALPHA;
			else if (!strcmp(argv[u], "--ext"))
			{
				if (mode == mode_unspecified)
					mode = mode_convert;
				if (u+1 < argc)
					args.defaultext = argv[++u];
				else
				{
					Con_Printf("--ext requires output extension\n");
					return 1;
				}
			}
			else if (!strcmp(argv[u], "--exportmip"))
			{
				char *e = "erk";
				if (u+1 < argc)
					args.mipnum = strtoul(argv[++u], &e, 10);
				if (*e)
				{
					Con_Printf("--exportmip requires trailing numeric argument\n");
					return 1;
				}
			}
			else
			{
				if (argv[u][1] == '-')
				{
					//try aliases first.
					for (f = 0; f < countof(fmtaliases); f++)
					{
						if (!strcasecmp(argv[u]+2, fmtaliases[f].alias))
						{
							args.newpixelformat = fmtaliases[f].fmt;
							if (mode == mode_unspecified)
								mode = mode_convert;
							break;
						}
					}
					if (f < countof(fmtaliases))
						continue;

					//now try our formal format names
					for (f = 0; f < PTI_MAX; f++)
					{
						if (!strcasecmp(argv[u]+2, Image_FormatName(f)))
						{
							args.newpixelformat = f;
							if (mode == mode_unspecified)
								mode = mode_convert;
							break;
						}
					}
					if (f < PTI_MAX)
						continue;

					//nope, not a format name
				}
				Con_Printf("Unknown arg %s\n", argv[u]);
				goto showhelp;
			}
			argv[u] = NULL;
		}
		else
			argv[files++] = argv[u];
	}

	ImgTool_SetupPalette();

	if (mode == mode_unspecified && args.textype!=PTI_ANY)
		mode = mode_convert;

	if (!args.defaultext)
	{
		if (mode == mode_unspecified)
		{
#ifdef FTE_SDL
			mode = mode_view;
#else
			mode = mode_info;
#endif
		}

		if (mode == mode_extractwad)
			args.defaultext = "png";	//something the user expects to be able to view easily (and lossless)
		else if (args.newpixelformat >= PTI_BC1_RGB && args.newpixelformat < PTI_BC4_R)
			args.defaultext = "dds";
		else
			args.defaultext = "ktx";
	}
	else
	{
		if (mode == mode_unspecified)
			mode = mode_convert;
	}

	if (mode == mode_info)
	{	//just print info about each listed file.
		for (u = 0; u < files; u++)
			ImgTool_Enumerate(&args, argv[u], ImgTool_PrintInfo);
	}
	else if (mode == mode_convert && args.textype!=PTI_ANY && outname)	//overwrite input
	{
		ImgTool_Convert(&args, ImgTool_Combine(&args, argv, files), "combined", outname);
	}
	else if (mode == mode_convert && args.textype==PTI_ANY && (!outname||files==1))	//list of files (output filenames will be generated according to -ext arg)
	{
		//-c src1 src2 src3
		for (u = 0; u < files; u++)
			ImgTool_Convert(&args, ImgTool_Read(&args, argv[u]), argv[u], NULL);
	}
	else if (mode == mode_autotree && files == 2)
		ImgTool_TreeConvert(&args, argv[0], argv[1]);
	else if ((mode == mode_genwad2 || mode == mode_genwad3 || mode == mode_genwadx))
		ImgTool_WadConvert(&args, argv[0], argv+1, files-1, mode-(mode_genwadx-1));
	else if ((mode == mode_extractwad) && files == 1)
		ImgTool_WadExtract(&args, argv[0]);
#ifdef FTE_SDL
	else if (mode == mode_view)
	{
		for (u = 0; u < files; u++)
			ImgTool_Enumerate(&args, argv[u], ImgTool_View);
		SDLL_Loop();
	}
#endif
	else
	{
		printf("%u files\n", (int)files);
		printf("unsupported arg count for mode\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

#ifdef FTE_SDL
#include <SDL.h>

struct sdlwindow_s
{
	SDL_Window *w;
	SDL_Renderer *r;
	int width;
	int height;
	float scale;

	size_t texshown;
	size_t texcount;
	struct
	{
		char *name;
		size_t w, h;
		uploadfmt_t fmt;
		SDL_Texture *t;
	} *tex;
};
static struct
{
	qboolean inited;
	qboolean tried;
	size_t windowcount;

	int				(SDLCALL *Init)				(Uint32 flags);
	SDL_Window *	(SDLCALL *CreateWindow)		(const char *title, int x, int y, int w, int h, Uint32 flags);
	void *			(SDLCALL *SetWindowData)	(SDL_Window * window, const char *name, void *userdata);
	void *			(SDLCALL *GetWindowData)	(SDL_Window * window, const char *name);
	void			(SDLCALL *SetWindowTitle)	(SDL_Window * window, const char *title);
	void			(SDLCALL *SetWindowSize)	(SDL_Window * window, int w, int h);
	Uint32			(SDLCALL *GetWindowFlags)	(SDL_Window * window);
	SDL_Renderer *	(SDLCALL *CreateRenderer)	(SDL_Window * window, int index, Uint32 flags);
	int				(SDLCALL *GetRendererInfo)	(SDL_Renderer * renderer, SDL_RendererInfo * info);
	SDL_Texture *	(SDLCALL *CreateTexture)	(SDL_Renderer * renderer, Uint32 format, int access, int w, int h);
	int				(SDLCALL *UpdateTexture)	(SDL_Texture * texture, const SDL_Rect * rect, const void *pixels, int pitch);
	int				(SDLCALL *RenderCopy)		(SDL_Renderer * renderer, SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect);
	void			(SDLCALL *RenderPresent)	(SDL_Renderer * renderer);
	int				(SDLCALL *WaitEvent)		(SDL_Event * event);
	int				(SDLCALL *PollEvent)		(SDL_Event * event);
	SDL_Window *	(SDLCALL *GetWindowFromID)	(Uint32 id);
	void			(SDLCALL *DestroyTexture)	(SDL_Texture * texture);
	void			(SDLCALL *DestroyRenderer)	(SDL_Renderer * renderer);
	void			(SDLCALL *DestroyWindow)	(SDL_Window * window);
	void			(SDLCALL *Quit)				(void);

	struct sdlwindow_s *texview;
} sdl;
static qboolean SDLL_Setup(void)
{
	static dllfunction_t funcs[] =
	{
		{(void**)&sdl.Init,				"SDL_Init"},
		{(void**)&sdl.CreateWindow,		"SDL_CreateWindow"},
		{(void**)&sdl.SetWindowData,	"SDL_SetWindowData"},
		{(void**)&sdl.GetWindowData,	"SDL_GetWindowData"},
		{(void**)&sdl.SetWindowTitle,	"SDL_SetWindowTitle"},
		{(void**)&sdl.SetWindowSize,	"SDL_SetWindowSize"},
		{(void**)&sdl.GetWindowFlags,	"SDL_GetWindowFlags"},
		{(void**)&sdl.CreateRenderer,	"SDL_CreateRenderer"},
		{(void**)&sdl.GetRendererInfo,	"SDL_GetRendererInfo"},
		{(void**)&sdl.CreateTexture,	"SDL_CreateTexture"},
		{(void**)&sdl.UpdateTexture,	"SDL_UpdateTexture"},
		{(void**)&sdl.RenderCopy,		"SDL_RenderCopy"},
		{(void**)&sdl.RenderPresent,	"SDL_RenderPresent"},
		{(void**)&sdl.WaitEvent,		"SDL_WaitEvent"},
		{(void**)&sdl.PollEvent,		"SDL_PollEvent"},
		{(void**)&sdl.GetWindowFromID,	"SDL_GetWindowFromID"},
		{(void**)&sdl.DestroyTexture,	"SDL_DestroyTexture"},
		{(void**)&sdl.DestroyRenderer,	"SDL_DestroyRenderer"},
		{(void**)&sdl.DestroyWindow,	"SDL_DestroyWindow"},
		{(void**)&sdl.Quit,				"SDL_Quit"},
		{NULL,NULL}
	};

	if (!sdl.tried)
	{
		sdl.tried = true;

#ifdef _WIN32
		if (!Sys_LoadLibrary("SDL2.dll", funcs))
#else
		if (!Sys_LoadLibrary("libSDL2-2.0.so.0", funcs))
		if (!Sys_LoadLibrary("libSDL2-2.0.so", funcs))
		if (!Sys_LoadLibrary("libSDL2-2.so", funcs))
		if (!Sys_LoadLibrary("libSDL2.so", funcs))
		if (!Sys_LoadLibrary("SDL2.so", funcs))
#endif
		{
			printf("Unable to load SDL2 library\n");
			return sdl.inited;
		}

		sdl.Init(SDL_INIT_VIDEO);
		sdl.inited = true;
	}
	return sdl.inited;
}
static void SDLL_KillWindow(struct sdlwindow_s *wc)
{
	while (wc->texcount --> 0)
	{
		if (wc->tex[wc->texcount].t)
			sdl.DestroyTexture(wc->tex[wc->texcount].t);
		BZ_Free(wc->tex[wc->texcount].name);
	}
	BZ_Free(wc->tex);
	if (wc->r)
		sdl.DestroyRenderer(wc->r);
	if (wc->w)
		sdl.DestroyWindow(wc->w);
	Z_Free(wc);
	sdl.windowcount--;
	if (sdl.texview == wc)
		sdl.texview = NULL;
}
static void SDLL_RepaintWindow(struct sdlwindow_s *wc)
{
	if (wc->texshown < wc->texcount)
	{
		SDL_Rect dest;
		dest.x=dest.y=0;
		dest.w=dest.h=1;
		sdl.RenderCopy(wc->r, wc->tex[wc->texshown].t, &dest, NULL);

		dest.w = wc->tex[wc->texshown].w*wc->scale;
		dest.h = wc->tex[wc->texshown].h*wc->scale;
		dest.x = (wc->width - dest.w)/2;
		dest.y = (wc->height - dest.h)/2;
		sdl.RenderCopy(wc->r, wc->tex[wc->texshown].t, NULL, &dest);
		sdl.RenderPresent(wc->r);
	}
}
static void SDLL_Change(struct sdlwindow_s *wc, size_t newshown)
{
	if (newshown < wc->texcount)
	{
		int w, h;
		char title[512];
		wc->texshown = newshown;
		if (wc->texcount==1)
			snprintf(title, sizeof(title), "%s %s", wc->tex[wc->texshown].name, Image_FormatName(wc->tex[wc->texshown].fmt));
		else
			snprintf(title, sizeof(title), "[%u/%u] %s %s", 1+(unsigned int)newshown, (unsigned int)wc->texcount, wc->tex[wc->texshown].name, Image_FormatName(wc->tex[wc->texshown].fmt));
		sdl.SetWindowTitle(wc->w, title);

		w = wc->tex[wc->texshown].w * wc->scale;
		h = wc->tex[wc->texshown].h * wc->scale;
		if (w > 1024)
			w = 1024;
		if (h > 768)
			h = 768;
		if (wc->width < w || wc->height < h)
			if (!(sdl.GetWindowFlags(wc->w) & (SDL_WINDOW_MAXIMIZED|SDL_WINDOW_FULLSCREEN)))	//SetWindowSize seems to bug out on linux when its maximized.
			{
				wc->width = w;
				wc->height = h;
				sdl.SetWindowSize(wc->w, w, h);
			}
		SDLL_RepaintWindow(wc);
	}
}
static void SDLL_Event(SDL_Event *ev)
{
	struct sdlwindow_s *wc;
	switch (ev->type)
	{
	case SDL_KEYDOWN:
		wc = sdl.GetWindowData(sdl.GetWindowFromID(ev->key.windowID), "uptr");
		if (!wc) break;
		switch(ev->key.keysym.sym)
		{
		case SDLK_ESCAPE:
		case SDLK_q:
			SDLL_KillWindow(wc);
			break;
		case SDLK_LEFT:
			wc->scale = 1;
			SDLL_Change(wc, wc->texshown-1);
			break;
		case SDLK_RIGHT:
			wc->scale = 1;
			SDLL_Change(wc, wc->texshown+1);
			break;
		case SDLK_UP:
			wc->scale /= 0.9;
			if (wc->scale > 4)
				wc->scale = 4;
			SDLL_Change(wc, wc->texshown);
			break;
		case SDLK_DOWN:
			wc->scale *= 0.9;
			if (wc->scale < 0.25)
				wc->scale = 0.25;
			SDLL_Change(wc, wc->texshown);
			break;
		}
		break;
	case SDL_WINDOWEVENT:
		wc = sdl.GetWindowData(sdl.GetWindowFromID(ev->window.windowID), "uptr");
		if (!wc) break;
		switch (ev->window.event)
		{
		case SDL_WINDOWEVENT_CLOSE:
			SDLL_KillWindow(wc);
			break;
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			wc->width = ev->window.data1;
			wc->height = ev->window.data2;
			break;
		case SDL_WINDOWEVENT_EXPOSED:
			SDLL_RepaintWindow(wc);
			break;
		}
		break;
	//don't bother with SDL_QUIT, it doesn't get sent if we kill the last window via a keypress. just count live windows instead.
	default:
//		printf("event type %x\n", ev->type);
		break;
	}
}
static void SDLL_Loop(void)
{
	SDL_Event ev;
	if (!sdl.inited)
		return;

	if (sdl.texview)
		SDLL_Change(sdl.texview, sdl.texview->texshown);

	while (sdl.windowcount && sdl.WaitEvent(&ev))
		SDLL_Event(&ev);

	sdl.Quit();
}
static void ImgTool_View(const char *inname, struct pendingtextureinfo *in)
{
	unsigned int sdlfmt;
	SDL_RendererInfo rinfo;
	int s;
	qboolean outformats[PTI_MAX] = {false};
	struct sdlwindow_s *wc;
	SDL_Event ev;
	uploadfmt_t origencoding;

	if (!in || in->mipcount < 1 || in->mip[0].width <= 0 || in->mip[0].height <= 0)
		return;
	origencoding = in->encoding;

	if (!SDLL_Setup())
	{
		ImgTool_PrintInfo(inname, in);
		return;
	}

	while (sdl.windowcount>=64 && sdl.WaitEvent(&ev))
		SDLL_Event(&ev);

	if (sdl.texview)
		wc = sdl.texview;
	else
	{
		sdl.texview = wc = Z_Malloc(sizeof(*wc));
		wc->scale = 2;

		s = 1;
		while (	 (in->mip[0].width*s < 256 && in->mip[0].height*s < 512)||
				 (in->mip[0].height*s < 256 && in->mip[0].width*s < 512))
			s<<=1;
		wc->w = sdl.CreateWindow("textureview", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 64, 64, SDL_WINDOW_RESIZABLE);
		sdl.windowcount++;
		if (wc->w)
		{
			sdl.SetWindowData(wc->w, "uptr", wc);

			//needs a rendering context too
			wc->r = sdl.CreateRenderer(wc->w, -1, SDL_RENDERER_SOFTWARE);
			if (!wc->r)
			{
				printf("Unable to create rendering context\n");
				SDLL_KillWindow(wc);
				return;
			}
		}
	}
	if (wc->r)
	{
		//figure out which formats we can pass to sdl
		sdl.GetRendererInfo(wc->r, &rinfo);
		while (rinfo.num_texture_formats --> 0)
		{
			switch(rinfo.texture_formats[rinfo.num_texture_formats])
			{
			//packed formats use hex ordering in both apis.
			case SDL_PIXELFORMAT_RGB565:		outformats[PTI_RGB565] = true;	break;
//			case SDL_PIXELFORMAT_BGR565:		outformats[PTI_BGR565] = true;	break;
			case SDL_PIXELFORMAT_RGBA4444:		outformats[PTI_RGBA4444] = true;	break;
			case SDL_PIXELFORMAT_ABGR4444:		outformats[PTI_ARGB4444] = true;	break;
			case SDL_PIXELFORMAT_RGBA5551:		outformats[PTI_RGBA5551] = true;	break;
			case SDL_PIXELFORMAT_ARGB1555:		outformats[PTI_ARGB1555] = true;	break;
//			case SDL_PIXELFORMAT_ARGB2101010:	outformats[PTI_A2RGB10] = true;	break;
//			case SDL_PIXELFORMAT_ABGR2101010:	outformats[PTI_A2BGR10] = true;	break;

			//these sdl aliases are for explicit byte orders, rather than packed.
			case SDL_PIXELFORMAT_RGBA32:		outformats[PTI_RGBA8] = true;	break;
			case SDL_PIXELFORMAT_BGRA32:		outformats[PTI_BGRA8] = true;	break;
			case SDL_PIXELFORMAT_BGR24:			outformats[PTI_RGB8] = true;	break;
			case SDL_PIXELFORMAT_RGB24:			outformats[PTI_BGR8] = true;	break;
//			case SDL_PIXELFORMAT_ARGB32:		outformats[PTI_ARGB8] = true;	break;
//			case SDL_PIXELFORMAT_ABGR32:		outformats[PTI_ABGR8] = true;	break;
/*#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			case SDL_PIXELFORMAT_RGBX8888:		outformats[PTI_RGBX8] = true;	break;
			case SDL_PIXELFORMAT_BGRX8888:		outformats[PTI_BGRX8] = true;	break;
#else
			case SDL_PIXELFORMAT_XBGR8888:		outformats[PTI_RGBX8] = true;	break;
			case SDL_PIXELFORMAT_XRGB8888:		outformats[PTI_BGRX8] = true;	break;
#endif*/
			}
		}

		//convert our image, if needed.
		if (!outformats[in->encoding])
			Image_ChangeFormat(in, outformats, PTI_INVALID, inname);
		if (!outformats[in->encoding])
			sdlfmt = SDL_PIXELFORMAT_UNKNOWN,printf("Unable to convert to usable pixel format\n");
		else switch(in->encoding)
		{
		//packed formats
		case PTI_RGB565:	sdlfmt = SDL_PIXELFORMAT_RGB565;		break;
//		case PTI_BGR565:	sdlfmt = SDL_PIXELFORMAT_BGR565;		break;
		case PTI_RGBA4444:	sdlfmt = SDL_PIXELFORMAT_RGBA4444;		break;
		case PTI_ARGB4444:	sdlfmt = SDL_PIXELFORMAT_ARGB4444;		break;
		case PTI_RGBA5551:	sdlfmt = SDL_PIXELFORMAT_RGBA5551;		break;
		case PTI_ARGB1555:	sdlfmt = SDL_PIXELFORMAT_ARGB1555;		break;
//		case PTI_A2RGB10:	sdlfmt = SDL_PIXELFORMAT_ARGB2101010;	break;
//		case PTI_A2BGR10:	sdlfmt = SDL_PIXELFORMAT_ABGR2101010;	break;

		//byte-ordered formats.
		case PTI_RGBA8:		sdlfmt = SDL_PIXELFORMAT_RGBA32;		break;
		case PTI_BGRA8:		sdlfmt = SDL_PIXELFORMAT_BGRA32;		break;
		case PTI_RGB8:		sdlfmt = SDL_PIXELFORMAT_RGB24;			break;
		case PTI_BGR8:		sdlfmt = SDL_PIXELFORMAT_BGR24;			break;
/*#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		case PTI_RGBX8:		sdlfmt = SDL_PIXELFORMAT_RGBX8888;		break;
		case PTI_BGRX8:		sdlfmt = SDL_PIXELFORMAT_BGRX8888;		break;
#else
		case PTI_RGBX8:		sdlfmt = SDL_PIXELFORMAT_XBGR8888;		break;
		case PTI_BGRX8:		sdlfmt = SDL_PIXELFORMAT_XRGB8888;		break;
#endif*/

		default:			sdlfmt = SDL_PIXELFORMAT_UNKNOWN;		 break;	//shouldn't happen.
		}

		wc->tex = realloc(wc->tex, sizeof(*wc->tex)*(wc->texcount+1));
		wc->tex[wc->texcount].name = Z_StrDup(inname);
		wc->tex[wc->texcount].w = in->mip[0].width;
		wc->tex[wc->texcount].h = in->mip[0].height;
		wc->tex[wc->texcount].fmt = origencoding;
		wc->tex[wc->texcount].t = sdl.CreateTexture(wc->r, sdlfmt, SDL_TEXTUREACCESS_STATIC, in->mip[0].width, in->mip[0].height);	//which needs a texture...
		if (wc->tex[wc->texcount].t)
		{
			sdl.UpdateTexture(wc->tex[wc->texcount].t, NULL, in->mip[0].data, in->mip[0].datasize/in->mip[0].height);				//with our image data
			if (!wc->texcount++)
				SDLL_Change(wc, 0);

			while (sdl.PollEvent(&ev))
				SDLL_Event(&ev);
		}
		else
		{
			printf("Unable to create texture %s %u*%u\n", Image_FormatName(in->encoding), in->mip[0].width, in->mip[0].height);
			BZ_Free(wc->tex[wc->texcount].name);
		}
	}
	else
	{
		printf("Unable to create window\n");
		SDLL_KillWindow(wc);
	}
}
#endif
#endif
