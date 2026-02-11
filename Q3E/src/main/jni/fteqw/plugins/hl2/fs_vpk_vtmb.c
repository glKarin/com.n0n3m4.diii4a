#include "../plugin.h"
#include "../../engine/common/fs.h"

static plugfsfuncs_t *filefuncs = NULL;
static plugthreadfuncs_t *threadfuncs = NULL;
#define Sys_CreateMutex() (threadfuncs?threadfuncs->CreateMutex():NULL)
#define Sys_LockMutex(m) (threadfuncs?threadfuncs->LockMutex(m):true)
#define Sys_UnlockMutex if(threadfuncs)threadfuncs->UnlockMutex
#define Sys_DestroyMutex if(threadfuncs)threadfuncs->DestroyMutex

typedef struct vvpkfile {
	fsbucket_t bucket;
	char name[MAX_QPATH];
	size_t ofs;
	size_t len;
} vvpkfile_t;

typedef struct vvpk {
	searchpathfuncs_t pub;
	char desc[MAX_OSPATH];
	vfsfile_t *handle;
	size_t num_files;
	vvpkfile_t *files;
	void *mutex;
	int references;
} vvpk_t;

static void QDECL FSVVPK_GetPathDetails(searchpathfuncs_t *handle, char *out, size_t outlen)
{
	vvpk_t *pak = (vvpk_t *)handle;
	*out = 0;
	if (pak->references != 1)
		Q_snprintfz(out, outlen, "(%i)", pak->references - 1);
}

static void QDECL FSVVPK_ClosePath(searchpathfuncs_t *handle)
{
	qboolean stillopen;
	vvpk_t *pak = (vvpk_t *)handle;

	if (!Sys_LockMutex(pak->mutex))
		return; //ohnoes
	stillopen = --pak->references > 0;
	Sys_UnlockMutex(pak->mutex);
	if (stillopen)
		return; //not free yet

	VFS_CLOSE(pak->handle);

	Sys_DestroyMutex(pak->mutex);

	plugfuncs->Free(pak->files);
	plugfuncs->Free(pak);
}

static void QDECL FSVVPK_BuildHash(searchpathfuncs_t *handle, int depth, void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle))
{
	vvpk_t *pak = (vvpk_t *)handle;
	int i;
	for (i = 0; i < pak->num_files; i++)
		AddFileHash(depth, pak->files[i].name, &pak->files[i].bucket, &pak->files[i]);
}

static unsigned int QDECL FSVVPK_FindFile(searchpathfuncs_t *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	vvpkfile_t *file = (vvpkfile_t *)hashedresult;
	vvpk_t *pak = (vvpk_t *)handle;
	int i;

	if (file)
	{
		//is this a pointer to a file in this pak?
		if (file < pak->files || file > pak->files + pak->num_files)
			return FF_NOTFOUND;
	}
	else
	{
		for (i = 0; i < pak->num_files; i++)
		{
			if (strcasecmp(filename, pak->files[i].name) == 0)
			{
				file = &pak->files[i];
				break;
			}
		}
	}

	if (file)
	{
		if (loc)
		{
			loc->fhandle = file;
			*loc->rawname = 0;
			loc->offset = (qofs_t)-1;
			loc->len = file->len;
		}

		return FF_FOUND;
	}

	return FF_NOTFOUND;
}

typedef struct vfsvvpk {
	vfsfile_t funcs;
	vvpk_t *parent;
	qofs_t startpos;
	qofs_t length;
	qofs_t currentpos;
} vfsvvpk_t;

static int QDECL VFSVVPK_ReadBytes(struct vfsfile_s *vfs, void *buffer, int bytestoread)
{
	vfsvvpk_t *vfsp = (vfsvvpk_t *)vfs;
	int read = 0;

	if (bytestoread + vfsp->currentpos > vfsp->length)
		bytestoread = vfsp->length - vfsp->currentpos;

	if (Sys_LockMutex(vfsp->parent->mutex))
	{
		VFS_SEEK(vfsp->parent->handle, vfsp->startpos + vfsp->currentpos);
		read = VFS_READ(vfsp->parent->handle, buffer, bytestoread);
		vfsp->currentpos += read;
		Sys_UnlockMutex(vfsp->parent->mutex);
	}

	return read;
}

static int QDECL VFSVVPK_WriteBytes(struct vfsfile_s *vfs, const void *buffer, int bytestoread)
{
	plugfuncs->Error("Cannot write to vpk files\n");
	return 0;
}

static qofs_t QDECL VFSVVPK_Tell(struct vfsfile_s *vfs)
{
	vfsvvpk_t *vfsp = (vfsvvpk_t *)vfs;
	return vfsp->currentpos;
}

static qofs_t QDECL VFSVVPK_GetLen(struct vfsfile_s *vfs)
{
	vfsvvpk_t *vfsp = (vfsvvpk_t *)vfs;
	return vfsp->length;
}

static qboolean QDECL VFSVVPK_Close(vfsfile_t *vfs)
{
	vfsvvpk_t *vfsp = (vfsvvpk_t *)vfs;
	FSVVPK_ClosePath(&vfsp->parent->pub); //tell the parent that we don't need it open any more (reference counts)
	plugfuncs->Free(vfsp); //free ourselves.
	return true;
}

static qboolean QDECL VFSVVPK_Seek(struct vfsfile_s *vfs, qofs_t pos)
{
	vfsvvpk_t *vfsp = (vfsvvpk_t *)vfs;
	if (pos > vfsp->length)
		return false;
	vfsp->currentpos = pos;
	return true;
}

static vfsfile_t *QDECL FSVVPK_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode)
{
	vvpk_t *pak = (vvpk_t *)handle;
	vfsvvpk_t *vfs;
	vvpkfile_t *f = (vvpkfile_t *)loc->fhandle;

	if (strcmp(mode, "rb") != 0)
		return NULL;

	vfs = plugfuncs->Malloc(sizeof(vfsvvpk_t));

	vfs->parent = pak;
	if (!Sys_LockMutex(vfs->parent->mutex))
	{
		plugfuncs->Free(vfs);
		return NULL;
	}

	vfs->parent->references++;
	Sys_UnlockMutex(vfs->parent->mutex);

	vfs->startpos = f->ofs;
	vfs->length = loc->len;
	vfs->currentpos = 0;

#ifdef _DEBUG
	Q_strlcpy(vfs->funcs.dbgname, f->name, sizeof(vfs->funcs.dbgname));
#endif
	vfs->funcs.Close = VFSVVPK_Close;
	vfs->funcs.GetLen = VFSVVPK_GetLen;
	vfs->funcs.ReadBytes = VFSVVPK_ReadBytes;
	vfs->funcs.Seek = VFSVVPK_Seek;
	vfs->funcs.Tell = VFSVVPK_Tell;
	vfs->funcs.WriteBytes = VFSVVPK_WriteBytes; //not supported

	return (vfsfile_t *)vfs;
}

static void QDECL FSVVPK_ReadFile(searchpathfuncs_t *handle, flocation_t *loc, char *buffer)
{
	vfsfile_t *f;
	f = FSVVPK_OpenVFS(handle, loc, "rb");
	if (!f)
		return;
	VFS_READ(f, buffer, loc->len);
	VFS_CLOSE(f);
}

static int QDECL FSVVPK_EnumerateFiles(searchpathfuncs_t *handle, const char *match, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *spath), void *parm)
{
	vvpk_t *pak = (vvpk_t *)handle;
	int i;

	for (i = 0; i < pak->num_files; i++)
	{
		if (filefuncs->WildCmp(match, pak->files[i].name))
		{
			if (!func(pak->files[i].name, pak->files[i].len, 0, parm, handle))
				return false;
		}
	}

	return true;
}

static int QDECL FSVVPK_GeneratePureCRC(searchpathfuncs_t *handle, const int *seed)
{
	vvpk_t *pak = (vvpk_t *)handle;
	return filefuncs->BlockChecksum(pak->files, sizeof(*pak->files) * pak->num_files);
}

searchpathfuncs_t *QDECL FSVVPK_LoadArchive(vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix)
{
	vvpk_t *pak;
	int i;
	int fsize;
	uint32_t num_files, ofs_files;
	uint8_t sentinel;

	// sanity checks
	if (!file) return NULL;
	if (prefix && *prefix) return NULL;

	// last ditch check to make sure this is a vtmb vpk
	if (strncasecmp(filename, "pack", 4) != 0)
		return NULL;

	// validate footer
	fsize = VFS_GETLEN(file);
	if (fsize < 9)
		return NULL;
	VFS_SEEK(file, fsize - ((sizeof(uint32_t) * 2) + sizeof(uint8_t)));
	VFS_READ(file, &num_files, sizeof(num_files));
	VFS_READ(file, &ofs_files, sizeof(ofs_files));
	VFS_READ(file, &sentinel, sizeof(sentinel));
	if (sentinel != 0)
		return NULL;

	num_files = LittleLong(num_files);
	ofs_files = LittleLong(ofs_files);

	// alloc
	pak = plugfuncs->Malloc(sizeof(*pak));
	pak->files = plugfuncs->Malloc(sizeof(*pak->files) * num_files);

	// read tree
	VFS_SEEK(file, ofs_files);
	for (i = 0; i < num_files; i++)
	{
		uint32_t len_name;
		uint32_t ofs_data;
		uint32_t len_data;
		VFS_READ(file, &len_name, sizeof(len_name));
		len_name = LittleLong(len_name);
		if (len_name > MAX_QPATH - 1)
		{
			Con_Printf("%s: filename in vpk is too long\n", filename);
			return NULL;
		}

		VFS_READ(file, pak->files[i].name, len_name);
		VFS_READ(file, &ofs_data, sizeof(ofs_data));
		VFS_READ(file, &len_data, sizeof(len_data));

		pak->files[i].ofs = LittleLong(ofs_data);
		pak->files[i].len = LittleLong(len_data);
	}

	// setup info
	pak->references++;
	pak->num_files = num_files;
	pak->handle = file;
	strcpy(pak->desc, desc);
	VFS_SEEK(pak->handle, 0);
	pak->mutex = Sys_CreateMutex();

	// setup funcs
	pak->pub.fsver = FSVER;
	pak->pub.GetPathDetails = FSVVPK_GetPathDetails;
	pak->pub.ClosePath = FSVVPK_ClosePath;
	pak->pub.BuildHash = FSVVPK_BuildHash;
	pak->pub.FindFile = FSVVPK_FindFile;
	pak->pub.ReadFile = FSVVPK_ReadFile;
	pak->pub.EnumerateFiles = FSVVPK_EnumerateFiles;
	pak->pub.GeneratePureCRC = FSVVPK_GeneratePureCRC;
	pak->pub.OpenVFS = FSVVPK_OpenVFS;

	return (searchpathfuncs_t *)pak;
}

qboolean VVPK_Init(void)
{
	// get funcs
	threadfuncs = plugfuncs->GetEngineInterface(plugthreadfuncs_name, sizeof(*threadfuncs));
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	if (!filefuncs) // threadfuncs optional
		return false;

	return true;
}
