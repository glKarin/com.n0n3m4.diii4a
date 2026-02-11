#include "../plugin.h"
#include "../../engine/common/fs.h"

static plugfsfuncs_t *filefuncs = NULL;
static plugthreadfuncs_t *threadfuncs = NULL;
#define Sys_CreateMutex() (threadfuncs?threadfuncs->CreateMutex():NULL)
#define Sys_LockMutex(m) (threadfuncs?threadfuncs->LockMutex(m):true)
#define Sys_UnlockMutex if(threadfuncs)threadfuncs->UnlockMutex
#define Sys_DestroyMutex if(threadfuncs)threadfuncs->DestroyMutex

typedef struct gmafile {
	fsbucket_t bucket;
	char name[MAX_QPATH];
	size_t ofs;
	size_t len;
} gmafile_t;

typedef struct gma {
	searchpathfuncs_t pub;
	char desc[MAX_OSPATH];
	vfsfile_t *handle;
	size_t num_files;
	gmafile_t *files;
	void *mutex;
	int references;
} gma_t;

static qboolean VFS_READZ(vfsfile_t *file, char *out, size_t outlen)
{
	size_t i;
	for (i = 0; i < outlen; i++)
	{
		if (VFS_READ(file, &out[i], sizeof(char)) != sizeof(char))
			break;
		if (out[i] == 0)
			return true;
	}

	// too long
	out[i] = 0;
	return false;
}

static void QDECL FSGMA_GetPathDetails(searchpathfuncs_t *handle, char *out, size_t outlen)
{
	gma_t *pak = (gma_t *)handle;
	*out = 0;
	if (pak->references != 1)
		Q_snprintfz(out, outlen, "(%i)", pak->references - 1);
}

static void QDECL FSGMA_ClosePath(searchpathfuncs_t *handle)
{
	qboolean stillopen;
	gma_t *pak = (gma_t *)handle;

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

static void QDECL FSGMA_BuildHash(searchpathfuncs_t *handle, int depth, void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle))
{
	gma_t *pak = (gma_t *)handle;
	int i;
	for (i = 0; i < pak->num_files; i++)
		AddFileHash(depth, pak->files[i].name, &pak->files[i].bucket, &pak->files[i]);
}

static unsigned int QDECL FSGMA_FindFile(searchpathfuncs_t *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	gmafile_t *file = (gmafile_t *)hashedresult;
	gma_t *pak = (gma_t *)handle;
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

typedef struct vfsgma {
	vfsfile_t funcs;
	gma_t *parent;
	qofs_t startpos;
	qofs_t length;
	qofs_t currentpos;
} vfsgma_t;

static int QDECL VFSGMA_ReadBytes(struct vfsfile_s *vfs, void *buffer, int bytestoread)
{
	vfsgma_t *vfsp = (vfsgma_t *)vfs;
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

static int QDECL VFSGMA_WriteBytes(struct vfsfile_s *vfs, const void *buffer, int bytestoread)
{
	plugfuncs->Error("Cannot write to gma files\n");
	return 0;
}

static qofs_t QDECL VFSGMA_Tell(struct vfsfile_s *vfs)
{
	vfsgma_t *vfsp = (vfsgma_t *)vfs;
	return vfsp->currentpos;
}

static qofs_t QDECL VFSGMA_GetLen(struct vfsfile_s *vfs)
{
	vfsgma_t *vfsp = (vfsgma_t *)vfs;
	return vfsp->length;
}

static qboolean QDECL VFSGMA_Close(vfsfile_t *vfs)
{
	vfsgma_t *vfsp = (vfsgma_t *)vfs;
	FSGMA_ClosePath(&vfsp->parent->pub); //tell the parent that we don't need it open any more (reference counts)
	plugfuncs->Free(vfsp); //free ourselves.
	return true;
}

static qboolean QDECL VFSGMA_Seek(struct vfsfile_s *vfs, qofs_t pos)
{
	vfsgma_t *vfsp = (vfsgma_t *)vfs;
	if (pos > vfsp->length)
		return false;
	vfsp->currentpos = pos;
	return true;
}

static vfsfile_t *QDECL FSGMA_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode)
{
	gma_t *pak = (gma_t *)handle;
	vfsgma_t *vfs;
	gmafile_t *f = (gmafile_t *)loc->fhandle;

	if (strcmp(mode, "rb") != 0)
		return NULL;

	vfs = plugfuncs->Malloc(sizeof(vfsgma_t));

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
	vfs->funcs.Close = VFSGMA_Close;
	vfs->funcs.GetLen = VFSGMA_GetLen;
	vfs->funcs.ReadBytes = VFSGMA_ReadBytes;
	vfs->funcs.Seek = VFSGMA_Seek;
	vfs->funcs.Tell = VFSGMA_Tell;
	vfs->funcs.WriteBytes = VFSGMA_WriteBytes; //not supported

	return (vfsfile_t *)vfs;
}

static void QDECL FSGMA_ReadFile(searchpathfuncs_t *handle, flocation_t *loc, char *buffer)
{
	vfsfile_t *f;
	f = FSGMA_OpenVFS(handle, loc, "rb");
	if (!f)
		return;
	VFS_READ(f, buffer, loc->len);
	VFS_CLOSE(f);
}

static int QDECL FSGMA_EnumerateFiles(searchpathfuncs_t *handle, const char *match, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *spath), void *parm)
{
	gma_t *pak = (gma_t *)handle;
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

static int QDECL FSGMA_GeneratePureCRC(searchpathfuncs_t *handle, const int *seed)
{
	gma_t *pak = (gma_t *)handle;
	return filefuncs->BlockChecksum(pak->files, sizeof(*pak->files) * pak->num_files);
}

static searchpathfuncs_t *QDECL FSGMA_LoadArchive(vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix)
{
	char magic[4];
	uint8_t version;
	uint64_t steamid;
	uint64_t timestamp;
	uint8_t padding;
	char addon_name[MAX_QPATH];
	char addon_description[MAX_QPATH];
	char addon_author[MAX_QPATH];
	uint32_t addon_version;
	gma_t *pak;
	int i;
	uint64_t ofs;

	// sanity checks
	if (!file) return NULL;
	if (prefix && *prefix) return NULL;

	// validate magic
	if (VFS_READ(file, magic, sizeof(magic)) < sizeof(magic))
		return NULL;
	if (memcmp(magic, "GMAD", 4) != 0)
		return NULL;

	// validate version
	if (VFS_READ(file, &version, sizeof(version)) < sizeof(version))
		return NULL;
	if (version != 3) // i don't know what's different in the other versions
		return NULL;

	// read other stuff
	if (VFS_READ(file, &steamid, sizeof(steamid)) < sizeof(steamid))
		return NULL;
	if (VFS_READ(file, &timestamp, sizeof(timestamp)) < sizeof(timestamp))
		return NULL;
	if (VFS_READ(file, &padding, sizeof(padding)) < sizeof(padding))
		return NULL;
	if (!VFS_READZ(file, addon_name, sizeof(addon_name)))
		return NULL;
	if (!VFS_READZ(file, addon_description, sizeof(addon_description)))
		return NULL;
	if (!VFS_READZ(file, addon_author, sizeof(addon_author)))
		return NULL;
	if (VFS_READ(file, &addon_version, sizeof(addon_version)) < sizeof(addon_version))
		return NULL;

	// alloc
	pak = plugfuncs->Malloc(sizeof(*pak));

	// read file table
	pak->num_files = 0;
	ofs = 0;
	while (1)
	{
		uint32_t id;
		uint64_t len;
		uint32_t crc;

		// read id (0 means end of file list)
		VFS_READ(file, &id, sizeof(id));
		if (id == 0)
			break;

		pak->files = plugfuncs->Realloc(pak->files, sizeof(*pak->files) * (pak->num_files + 1));

		// read path, len, crc
		VFS_READZ(file, pak->files[pak->num_files].name, sizeof(pak->files[pak->num_files].name));
		VFS_READ(file, &len, sizeof(len));
		VFS_READ(file, &crc, sizeof(crc));

		pak->files[pak->num_files].len = LittleI64(len);
		pak->files[pak->num_files].ofs = ofs;
		ofs += len;

		pak->num_files++;
	}

	// fix up file data offsets
	ofs = VFS_TELL(file);
	for (i = 0; i < pak->num_files; i++)
		pak->files[i].ofs += ofs;

	// setup info
	pak->references++;
	pak->handle = file;
	strcpy(pak->desc, desc);
	VFS_SEEK(pak->handle, 0);
	pak->mutex = Sys_CreateMutex();

	// setup funcs
	pak->pub.fsver = FSVER;
	pak->pub.GetPathDetails = FSGMA_GetPathDetails;
	pak->pub.ClosePath = FSGMA_ClosePath;
	pak->pub.BuildHash = FSGMA_BuildHash;
	pak->pub.FindFile = FSGMA_FindFile;
	pak->pub.ReadFile = FSGMA_ReadFile;
	pak->pub.EnumerateFiles = FSGMA_EnumerateFiles;
	pak->pub.GeneratePureCRC = FSGMA_GeneratePureCRC;
	pak->pub.OpenVFS = FSGMA_OpenVFS;

	return (searchpathfuncs_t *)pak;
}

qboolean GMA_Init(void)
{
	// get funcs
	threadfuncs = plugfuncs->GetEngineInterface(plugthreadfuncs_name, sizeof(*threadfuncs));
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	if (!filefuncs) // threadfuncs optional
		return false;

	// export our fs function
	if (!plugfuncs->ExportFunction("FS_RegisterArchiveType_gma", FSGMA_LoadArchive))
		return false;

	return true;
}
