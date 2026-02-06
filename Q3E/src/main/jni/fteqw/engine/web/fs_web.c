#include "quakedef.h"
#include "fs.h"

#if defined(FTE_TARGET_WEB)
#define FSWEB_OpenPath VFSOS_OpenPath
#define FSWEB_OpenTemp FS_OpenTemp

typedef struct {
	searchpathfuncs_t pub;
	int depth;
	void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle);
	char rootpath[1];
} webpath_t;
typedef struct {
	vfsfile_t funcs;
	qofs_t offset;
	int handle;
} vfswebfile_t;
static int QDECL VFSWEB_ReadBytes (struct vfsfile_s *file, void *buffer, int bytestoread)
{
	vfswebfile_t *intfile = (vfswebfile_t*)file;
	memset(buffer, 'e', bytestoread);
	int len = emscriptenfte_buf_read(intfile->handle, intfile->offset, buffer, bytestoread);
	intfile->offset += len;
	return len;
}
static int QDECL VFSWEB_WriteBytes (struct vfsfile_s *file, const void *buffer, int bytestoread)
{
	vfswebfile_t *intfile = (vfswebfile_t*)file;
	int len = emscriptenfte_buf_write(intfile->handle, intfile->offset, buffer, bytestoread);
	intfile->offset += len;
	return len;
}
static qboolean QDECL VFSWEB_Seek (struct vfsfile_s *file, qofs_t pos)
{
	vfswebfile_t *intfile = (vfswebfile_t*)file;
//	if (pos < 0)
//		return 0;
	intfile->offset = pos;
	return true;
}
static qofs_t QDECL VFSWEB_Tell (struct vfsfile_s *file)
{
	vfswebfile_t *intfile = (vfswebfile_t*)file;
	return intfile->offset;
}
static void QDECL VFSWEB_Flush(struct vfsfile_s *file)
{
//	vfswebfile_t *intfile = (vfswebfile_t*)file;
}
static qofs_t QDECL VFSWEB_GetSize (struct vfsfile_s *file)
{
	vfswebfile_t *intfile = (vfswebfile_t*)file;
	unsigned long l;
	l = emscriptenfte_buf_getsize(intfile->handle);
	return l;
}
static qboolean QDECL VFSWEB_Close(vfsfile_t *file)
{
	vfswebfile_t *intfile = (vfswebfile_t*)file;
	emscriptenfte_buf_release(intfile->handle);
	Z_Free(file);
	return true;
}
static qboolean QDECL VFSWEB_ClosePersist(vfsfile_t *file)
{
	vfswebfile_t *intfile = (vfswebfile_t*)file;
#ifdef _DEBUG
	Con_DPrintf("Persisting file %s\n", file->dbgname);
#endif
	emscriptenfte_buf_pushtolocalstore(intfile->handle);
	return VFSWEB_Close(file);
}

vfsfile_t *FSWEB_OpenTempHandle(int f)
{
	vfswebfile_t *file;

	if (f == -1)
	{
		Con_Printf("FSWEB_OpenTemp failed\n");
		return NULL;
	}

	file = Z_Malloc(sizeof(vfswebfile_t));
	file->funcs.Close = VFSWEB_Close;
#ifdef _DEBUG
	Q_strncpyz(file->funcs.dbgname, "FSWEB_OpenTemp", sizeof(file->funcs.dbgname));
#endif
	file->funcs.ReadBytes = VFSWEB_ReadBytes;
	file->funcs.WriteBytes = VFSWEB_WriteBytes;
	file->funcs.Seek = VFSWEB_Seek;
	file->funcs.Tell = VFSWEB_Tell;
	file->funcs.GetLen = VFSWEB_GetSize;
	file->funcs.Flush = VFSWEB_Flush;
	file->handle = f;

	return &file->funcs;
}

vfsfile_t *FSWEB_OpenTemp(void)
{
	return FSWEB_OpenTempHandle(emscriptenfte_buf_create());
}

vfsfile_t *VFSWEB_Open(const char *osname, const char *mode, qboolean *needsflush)
{
	int f;
	vfswebfile_t *file;
	//qboolean read = !!strchr(mode, 'r');
	qboolean write = !!strchr(mode, 'w');
	qboolean update = !!strchr(mode, '+');
	qboolean append = !!strchr(mode, 'a');
	qboolean persist = !!strchr(mode, 'p');

	if (needsflush)
		*needsflush = false;
	f = emscriptenfte_buf_open(osname, (write && !update)?2:(write||append));
	if (f == -1)
		return NULL;

	if (write || append)
	{
		if (needsflush)
			*needsflush = true;
	}

	file = Z_Malloc(sizeof(vfswebfile_t));
#ifdef _DEBUG
	Q_strncpyz(file->funcs.dbgname, osname, sizeof(file->funcs.dbgname));
#endif
	file->funcs.ReadBytes = VFSWEB_ReadBytes;
	file->funcs.WriteBytes = VFSWEB_WriteBytes;
	file->funcs.Seek = VFSWEB_Seek;
	file->funcs.Tell = VFSWEB_Tell;
	file->funcs.GetLen = VFSWEB_GetSize;
	if (persist && (write || append))
		file->funcs.Close = VFSWEB_ClosePersist;
	else
		file->funcs.Close = VFSWEB_Close;
	file->funcs.Flush = VFSWEB_Flush;
	file->handle = f;

	if (append)
		file->offset = VFSWEB_GetSize(&file->funcs);

	return &file->funcs;
}

vfsfile_t *VFSOS_Open(const char *osname, const char *mode)
{
	vfsfile_t *f;
	qboolean needsflush;
	f = VFSWEB_Open(osname, mode, &needsflush);
	if (needsflush)
		FS_FlushFSHashFull();
	return f;
}

static vfsfile_t *QDECL FSWEB_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode)
{
	vfsfile_t *f;
	webpath_t *sp = (void*)handle;
	qboolean needsflush;

	f = VFSWEB_Open(loc->rawname, mode, &needsflush);
	if (needsflush && sp->AddFileHash)
		sp->AddFileHash(sp->depth, loc->rawname, NULL, sp);
	return f;
}

static void QDECL FSWEB_ClosePath(searchpathfuncs_t *handle)
{
	Z_Free(handle);
}
static qboolean QDECL FSWEB_PollChanges(searchpathfuncs_t *handle)
{
//	webpath_t *np = handle;
	return true;	//can't verify that or not, so we have to assume the worst
}
static int QDECL FSWEB_RebuildFSHash(const char *filename, qofs_t filesize, time_t mtime, void *data, searchpathfuncs_t *spath)
{
	webpath_t *sp = (void*)spath;
	void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle) = data;
	if (filename[strlen(filename)-1] == '/')
	{	//this is actually a directory

		char childpath[256];
		Q_snprintfz(childpath, sizeof(childpath), "%s*", filename);
		Sys_EnumerateFiles(sp->rootpath, childpath, FSWEB_RebuildFSHash, data, spath);
		return true;
	}
	AddFileHash(sp->depth, filename, NULL, sp);
	return true;
}
static void QDECL FSWEB_BuildHash(searchpathfuncs_t *handle, int depth, void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle))
{
	webpath_t *sp = (void*)handle;
	sp->depth = depth;
	sp->AddFileHash = AddFileHash;
	Sys_EnumerateFiles(sp->rootpath, "*", FSWEB_RebuildFSHash, AddFileHash, handle);
}
static qboolean QDECL FSWEB_FLocate(searchpathfuncs_t *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	webpath_t *sp = (void*)handle;
	int len;
	char netpath[MAX_OSPATH];

	if (hashedresult && (void *)hashedresult != handle)
		return false;

/*
	if (!static_registered)
	{	// if not a registered version, don't ever go beyond base
		if ( strchr (filename, '/') || strchr (filename,'\\'))
			continue;
	}
*/

// check a file in the directory tree
	snprintf (netpath, sizeof(netpath)-1, "%s/%s", sp->rootpath, filename);

	{
		vfsfile_t *f = VFSWEB_Open(netpath, "rb", NULL);
		if (!f)
			return false;
		len = VFS_GETLEN(f);
		VFS_CLOSE(f);
	}
	if (loc)
	{
		loc->len = len;
		loc->offset = 0;
		loc->fhandle = NULL;
		Q_strncpyz(loc->rawname, netpath, sizeof(loc->rawname));
	}
	return true;
}
static void QDECL FSWEB_ReadFile(searchpathfuncs_t *handle, flocation_t *loc, char *buffer)
{
	vfsfile_t *f;
	size_t result;

	f = VFSWEB_Open(loc->rawname, "rb", NULL);
	if (!f)	//err...
		return;
	VFS_SEEK(f, loc->offset);
	result = VFS_READ(f, buffer, loc->len); // do soemthing with result

	if (result != loc->len)
		Con_Printf("FSWEB_ReadFile() fread: Filename: %s, expected %u, result was %u \n",loc->rawname,(unsigned int)loc->len,(unsigned int)result);

	VFS_CLOSE(f);
}
static int QDECL FSWEB_EnumerateFiles (searchpathfuncs_t *handle, const char *match, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *spath), void *parm)
{
	webpath_t *sp = (webpath_t*)handle;
	return Sys_EnumerateFiles(sp->rootpath, match, func, parm, handle);
}


searchpathfuncs_t *QDECL FSWEB_OpenPath(vfsfile_t *mustbenull,  searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix)
{
	webpath_t *np;
	int dlen = strlen(desc);
	if (mustbenull)
		return NULL;
	np = Z_Malloc(sizeof(*np) + dlen);
	if (np)
	{
		np->depth = 0;
		memcpy(np->rootpath, desc, dlen+1);
	}

	np->pub.fsver			= FSVER;
	np->pub.ClosePath		= FSWEB_ClosePath;
	np->pub.BuildHash		= FSWEB_BuildHash;
	np->pub.FindFile		= FSWEB_FLocate;
	np->pub.ReadFile		= FSWEB_ReadFile;
	np->pub.EnumerateFiles	= FSWEB_EnumerateFiles;
	np->pub.OpenVFS			= FSWEB_OpenVFS;
	np->pub.PollChanges		= FSWEB_PollChanges;
	return &np->pub;
}

#endif

