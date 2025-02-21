#include "quakedef.h"
#include "fs.h"
#include "errno.h"
#if _POSIX_C_SOURCE >= 200112L
#include <sys/stat.h>
#endif

#if !defined(FTE_TARGET_WEB) && (!defined(_WIN32) || defined(FTE_SDL) || defined(WEBSVONLY))

#ifdef WEBSVONLY
	#define Z_Free free
	#define Z_Malloc malloc
	#define Con_Printf printf
	#define fs_readonly true
	#define FS_FlushFSHashFull()
	int Sys_EnumerateFiles (const char *gpath, const char *match, int (QDECL *func)(const char *fname, qofs_t fsize, time_t modtime, void *parm, searchpathfuncs_t *spath), void *parm, searchpathfuncs_t *spath) {return 0;}
#else
	#if !defined(_WIN32) || defined(FTE_SDL) || defined(WINRT) || defined(_XBOX)
		#define FSSTDIO_OpenPath VFSOS_OpenPath
	#endif
	#define FSSTDIO_OpenTemp FS_OpenTemp
#endif

typedef struct {
	searchpathfuncs_t pub;
	int depth;
	void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle);
	char rootpath[1];
} stdiopath_t;
typedef struct {
	vfsfile_t funcs;
	FILE *handle;
} vfsstdiofile_t;
static int QDECL VFSSTDIO_ReadBytes (struct vfsfile_s *file, void *buffer, int bytestoread)
{
	vfsstdiofile_t *intfile = (vfsstdiofile_t*)file;
	return fread(buffer, 1, bytestoread, intfile->handle);
}
static int QDECL VFSSTDIO_WriteBytes (struct vfsfile_s *file, const void *buffer, int bytestoread)
{
	vfsstdiofile_t *intfile = (vfsstdiofile_t*)file;
	return fwrite(buffer, 1, bytestoread, intfile->handle);
}
static qboolean QDECL VFSSTDIO_Seek (struct vfsfile_s *file, qofs_t pos)
{
	vfsstdiofile_t *intfile = (vfsstdiofile_t*)file;
#ifdef __USE_LARGEFILE64
	return fseeko64(intfile->handle, (off64_t)pos, SEEK_SET) == 0;
#elif _POSIX_C_SOURCE >= 200112L
	return fseeko(intfile->handle, (off_t)pos, SEEK_SET) == 0;
#else
	return fseek(intfile->handle, pos, SEEK_SET) == 0;
#endif
}
static qofs_t QDECL VFSSTDIO_Tell (struct vfsfile_s *file)
{
	vfsstdiofile_t *intfile = (vfsstdiofile_t*)file;
#ifdef __USE_LARGEFILE64
	return (qofs_t)ftello64(intfile->handle);
#elif _POSIX_C_SOURCE >= 200112L
	return (qofs_t)ftello(intfile->handle);
#else
	return ftell(intfile->handle);
#endif
}
static void QDECL VFSSTDIO_Flush(struct vfsfile_s *file)
{
	vfsstdiofile_t *intfile = (vfsstdiofile_t*)file;
	fflush(intfile->handle);
}
static qofs_t QDECL VFSSTDIO_GetSize (struct vfsfile_s *file)
{
	vfsstdiofile_t *intfile = (vfsstdiofile_t*)file;

#ifdef __USE_LARGEFILE64
	off64_t curpos;
	qofs_t maxlen;
	curpos = ftello64(intfile->handle);
	fseeko64(intfile->handle, 0, SEEK_END);
	maxlen = (qofs_t)ftello64(intfile->handle);
	fseeko64(intfile->handle, curpos, SEEK_SET);
#elif _POSIX_C_SOURCE >= 200112L
	off_t curpos;
	qofs_t maxlen;
	curpos = ftello(intfile->handle);
	fseeko(intfile->handle, 0, SEEK_END);
	maxlen = (qofs_t)ftello(intfile->handle);
	fseeko(intfile->handle, curpos, SEEK_SET);
#else
	unsigned int curpos;
	unsigned int maxlen;
	curpos = ftell(intfile->handle);
	fseek(intfile->handle, 0, SEEK_END);
	maxlen = ftell(intfile->handle);
	fseek(intfile->handle, curpos, SEEK_SET);
#endif

	return maxlen;
}
static qboolean QDECL VFSSTDIO_Close(vfsfile_t *file)
{
	qboolean success;
	vfsstdiofile_t *intfile = (vfsstdiofile_t*)file;
	success = !ferror(intfile->handle);
	fclose(intfile->handle);
	Z_Free(file);
	return success;
}

#ifdef _WIN32
static qboolean QDECL VFSSTDIO_CloseTemp(vfsfile_t *file)
{
	qboolean success;
	vfsstdiofile_t *intfile = (vfsstdiofile_t*)file;
	char *fname = (char*)(intfile+1); 
	success = !ferror(intfile->handle);
	fclose(intfile->handle);
	_unlink(fname);
	Z_Free(file);
	return success;
}
#endif

vfsfile_t *FSSTDIO_OpenTemp(void)
{
	FILE *f;
	vfsstdiofile_t *file;

#ifdef _WIN32
	/*microsoft's tmpfile will nearly always fail, as it insists on writing to the root directory and that requires running everything with full admin rights.
	warning: there's a race condition between tempnam and fopen. if the file is not opened exclusively then we can end up with issues
	on windows, fopen is typically exclusive anyway, but not on unix. but on unix, tmpfile is actually usable, so special-case the windows code
	we also have a special close function to ensure the file is deleted too
	*/
	char *fname = _tempnam(NULL, "ftemp");
	f = fopen(fname, "w+b");
	if (!f)
		return NULL;

	file = Z_Malloc(sizeof(vfsstdiofile_t) + strlen(fname)+1);
	file->funcs.Close = VFSSTDIO_CloseTemp;
	strcpy((char*)(file+1), fname);
	free(fname);
#else
#ifdef __USE_LARGEFILE64
	f = tmpfile64();
#else
	f = tmpfile();
#endif
	if (!f)
		return NULL;

	file = Z_Malloc(sizeof(vfsstdiofile_t));
	file->funcs.Close = VFSSTDIO_Close;
#endif
#ifdef _DEBUG
	Q_strncpyz(file->funcs.dbgname, "FSSTDIO_OpenTemp", sizeof(file->funcs.dbgname));
#endif
	file->funcs.ReadBytes = VFSSTDIO_ReadBytes;
	file->funcs.WriteBytes = VFSSTDIO_WriteBytes;
	file->funcs.Seek = VFSSTDIO_Seek;
	file->funcs.Tell = VFSSTDIO_Tell;
	file->funcs.GetLen = VFSSTDIO_GetSize;
	file->funcs.Flush = VFSSTDIO_Flush;
	file->handle = f;

	return (vfsfile_t*)file;
}

#if 0//def ANDROID
vfsfile_t *Sys_OpenAsset(const char *fname);
#endif

vfsfile_t *VFSSTDIO_Open(const char *osname, const char *mode, qboolean *needsflush)
{
	FILE *f;
	vfsstdiofile_t *file;
	qboolean read = !!strchr(mode, 'r');
	qboolean write = !!strchr(mode, 'w');
	qboolean append = !!strchr(mode, 'a');
	qboolean text = !!strchr(mode, 't');
//	qboolean dolock = !!strchr(mode, 'l');
	char newmode[5];
	int modec = 0;

	if (needsflush)
		*needsflush = false;

#if 0//def ANDROID
//	if (!strncmp("asset/", osname, 6))
	{
		if (append || write)
			return NULL;
		return Sys_OpenAsset(osname);
	}
#endif

	if (read)
		newmode[modec++] = 'r';
	if (write)
		newmode[modec++] = 'w';
	if (append)
		newmode[modec++] = 'a';
//	if (append)
//		newmode[modec++] = '+';
	if (text)
		newmode[modec++] = 't';
	else
		newmode[modec++] = 'b';
#ifdef __linux__
	newmode[modec++] = 'e';	//otherwise forks get messy.
#endif
	newmode[modec++] = '\0';

#ifdef __USE_LARGEFILE64
	f = fopen64(osname, newmode);
#else
	f = fopen(osname, newmode);
#endif
	if (!f)
		return NULL;

	if (write || append)
	{
		if (needsflush)
			*needsflush = true;
	}

	file = Z_Malloc(sizeof(vfsstdiofile_t));
#ifdef _DEBUG
	Q_strncpyz(file->funcs.dbgname, osname, sizeof(file->funcs.dbgname));
#endif
	file->funcs.ReadBytes = VFSSTDIO_ReadBytes;
	file->funcs.WriteBytes = VFSSTDIO_WriteBytes;
	file->funcs.Seek = VFSSTDIO_Seek;
	file->funcs.Tell = VFSSTDIO_Tell;
	file->funcs.GetLen = VFSSTDIO_GetSize;
	file->funcs.Close = VFSSTDIO_Close;
	file->funcs.Flush = VFSSTDIO_Flush;
	file->handle = f;

	return (vfsfile_t*)file;
}

#if !defined(_WIN32) || defined(FTE_SDL) || defined(WINRT) || defined(_XBOX)
vfsfile_t *VFSOS_Open(const char *osname, const char *mode)
{
	vfsfile_t *f;
	qboolean needsflush;
	f = VFSSTDIO_Open(osname, mode, &needsflush);
	if (needsflush)
		FS_FlushFSHashFull();
	return f;
}
#endif

static vfsfile_t *QDECL FSSTDIO_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode)
{
	vfsfile_t *f;
	stdiopath_t *sp = (void*)handle;
	qboolean needsflush;

	f = VFSSTDIO_Open(loc->rawname, mode, &needsflush);
	if (needsflush && sp->AddFileHash)
		sp->AddFileHash(sp->depth, loc->rawname, NULL, sp);
	return f;
}

static void QDECL FSSTDIO_ClosePath(searchpathfuncs_t *handle)
{
	Z_Free(handle);
}
static qboolean QDECL FSSTDIO_PollChanges(searchpathfuncs_t *handle)
{
//	stdiopath_t *np = handle;
	return true;	//can't verify that or not, so we have to assume the worst
}
static int QDECL FSSTDIO_RebuildFSHash(const char *filename, qofs_t filesize, time_t mtime, void *data, searchpathfuncs_t *spath)
{
	stdiopath_t *sp = (void*)spath;
	void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle) = data;
	if (filename[strlen(filename)-1] == '/')
	{	//this is actually a directory

		char childpath[256];
		if (!Q_snprintfz(childpath, sizeof(childpath), "%s*", filename))
			Sys_EnumerateFiles(sp->rootpath, childpath, FSSTDIO_RebuildFSHash, data, spath);
		return true;
	}
	AddFileHash(sp->depth, filename, NULL, sp);
	return true;
}
static void QDECL FSSTDIO_BuildHash(searchpathfuncs_t *handle, int depth, void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle))
{
	stdiopath_t *sp = (void*)handle;
	sp->depth = depth;
	sp->AddFileHash = AddFileHash;
	Sys_EnumerateFiles(sp->rootpath, "*", FSSTDIO_RebuildFSHash, AddFileHash, handle);
}

static unsigned int QDECL FSSTDIO_CreateLoc(searchpathfuncs_t *handle, flocation_t *loc, const char *filename)
{
	stdiopath_t *sp = (void*)handle;
	char	*ofs;

	if (fs_readonly)
		return FF_NOTFOUND;

	loc->len = 0;
	loc->offset = 0;
	loc->fhandle = handle;
	if (Q_snprintfz(loc->rawname, sizeof(loc->rawname), "%s/%s", sp->rootpath, filename))
		return FF_NOTFOUND;	//too long...

	for (ofs = loc->rawname+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/')
		{	// create the directory
			*ofs = 0;
			Sys_mkdir (loc->rawname);
			*ofs = '/';
		}
	}

	return FF_FOUND;
}
static unsigned int QDECL FSSTDIO_FLocate(searchpathfuncs_t *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	stdiopath_t *sp = (void*)handle;
	qofs_t len;
	char netpath[MAX_OSPATH];

	if (hashedresult && (void *)hashedresult != handle)
		return FF_NOTFOUND;

/*
	if (!static_registered)
	{	// if not a registered version, don't ever go beyond base
		if ( strchr (filename, '/') || strchr (filename,'\\'))
			continue;
	}
*/

// check a file in the directory tree
	if (Q_snprintfz (netpath, sizeof(netpath), "%s/%s", sp->rootpath, filename))
		return FF_NOTFOUND;

#if _POSIX_C_SOURCE >= 200112L
	{
		struct stat sb;
		if (stat(netpath, &sb) == 0)
		{
			len = sb.st_size;
			if ((sb.st_mode & S_IFMT) != S_IFREG)
				return FF_NOTFOUND; //no directories nor sockets! boo! (any simlink will already have been resolved)
		}
		else
			return FF_NOTFOUND; //some kind of screwup.
	}
#elif 0//defined(ANDROID)
	{
		vfsfile_t *f = VFSSTDIO_Open(netpath, "rb", NULL);
		if (!f)
			return false;
		len = VFS_GETLEN(f);
		VFS_CLOSE(f);
	}
#else
	{
#ifdef __USE_LARGEFILE64
		FILE *f = fopen64(netpath, "rb");
#else
		FILE *f = fopen(netpath, "rb");
#endif
		if (!f)
			return FF_NOTFOUND;

#ifdef __USE_LARGEFILE64
		fseeko64(f, 0, SEEK_END);
		len = ftello64(f);
#elif _POSIX_C_SOURCE >= 200112L
		fseeko(f, 0, SEEK_END);
		len = ftello(f);
#else
		fseek(f, 0, SEEK_END);
		len = ftell(f);
#endif
		fclose(f);
	}
#endif
	if (loc)
	{
		loc->len = len;
		loc->offset = 0;
		loc->fhandle = handle;
		Q_strncpyz(loc->rawname, netpath, sizeof(loc->rawname));
	}
	return FF_FOUND;
}
static void QDECL FSSTDIO_ReadFile(searchpathfuncs_t *handle, flocation_t *loc, char *buffer)
{
	FILE *f;
	size_t result;

#ifdef __USE_LARGEFILE64
	f = fopen64(loc->rawname, "rb");
#else
	f = fopen(loc->rawname, "rb");
#endif
	if (!f)	//err...
		return;
#ifdef __USE_LARGEFILE64
	fseeko64(f, loc->offset, SEEK_SET);
#elif _POSIX_C_SOURCE >= 200112L
	fseeko(f, loc->offset, SEEK_SET);
#else
	fseek(f, loc->offset, SEEK_SET);
#endif
	result = fread(buffer, 1, loc->len, f); // do soemthing with result

	if (result != loc->len)
		Con_Printf("FSSTDIO_ReadFile() fread: Filename: %s, expected %u, result was %u (%s)\n",loc->rawname,(unsigned int)loc->len,(unsigned int)result,strerror(errno));

	fclose(f);
}
static int QDECL FSSTDIO_EnumerateFiles (searchpathfuncs_t *handle, const char *match, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *spath), void *parm)
{
	stdiopath_t *sp = (stdiopath_t*)handle;
	return Sys_EnumerateFiles(sp->rootpath, match, func, parm, handle);
}

#include <sys/stat.h>
static qboolean QDECL FSSTDIO_FileStat (searchpathfuncs_t *handle, flocation_t *loc, time_t *mtime)
{
	struct stat s;
	if (stat(loc->rawname, &s) != -1)
	{
		*mtime = s.st_mtime;
		return true;
	}
	return false;
}

static qboolean	 QDECL FSSTDIO_RenameFile(searchpathfuncs_t *handle, const char *oldname, const char *newname)
{
	stdiopath_t *sp = (void*)handle;
	char oldsyspath[MAX_OSPATH];
	char newsyspath[MAX_OSPATH];
	if (fs_readonly)
		return false;
	if (Q_snprintfz (oldsyspath, sizeof(oldsyspath), "%s/%s", sp->rootpath, oldname))
		return false;	//too long
	if (Q_snprintfz (newsyspath, sizeof(newsyspath), "%s/%s", sp->rootpath, newname))
		return false;	//too long
	return Sys_Rename(oldsyspath, newsyspath);
}
static qboolean QDECL FSSTDIO_RemoveFile(searchpathfuncs_t *handle, const char *filename)
{
	stdiopath_t *sp = (void*)handle;
	char syspath[MAX_OSPATH];
	if (fs_readonly)
		return false;
	if (Q_snprintfz (syspath, sizeof(syspath), "%s/%s", sp->rootpath, filename))
		return false;	//too long
	if (*filename && filename[strlen(filename)-1] == '/')
		return Sys_rmdir(syspath);
	return Sys_remove(syspath);
}

searchpathfuncs_t *QDECL FSSTDIO_OpenPath(vfsfile_t *mustbenull, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix)
{
	stdiopath_t *np;
	int dlen = strlen(desc);
	if (mustbenull)
		return NULL;
	if (prefix && *prefix)
		return NULL;	//don't try to support this. too risky with absolute paths etc.
	np = Z_Malloc(sizeof(*np) + dlen);
	if (np)
	{
		np->depth = 0;
		memcpy(np->rootpath, desc, dlen+1);
	}

	np->pub.fsver			= FSVER;
	np->pub.ClosePath		= FSSTDIO_ClosePath;
	np->pub.BuildHash		= FSSTDIO_BuildHash;
	np->pub.FindFile		= FSSTDIO_FLocate;
	np->pub.ReadFile		= FSSTDIO_ReadFile;
	np->pub.EnumerateFiles	= FSSTDIO_EnumerateFiles;
	np->pub.OpenVFS			= FSSTDIO_OpenVFS;
	np->pub.PollChanges		= FSSTDIO_PollChanges;
	np->pub.FileStat		= FSSTDIO_FileStat;
	np->pub.CreateFile		= FSSTDIO_CreateLoc;
	np->pub.RenameFile		= FSSTDIO_RenameFile;
	np->pub.RemoveFile		= FSSTDIO_RemoveFile;
	return &np->pub;
}

#endif

