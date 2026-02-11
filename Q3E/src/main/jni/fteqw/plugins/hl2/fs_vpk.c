#include "../plugin.h"
#include "../../engine/common/fs.h"

plugfsfuncs_t *filefuncs;

static plugthreadfuncs_t *threading;
#define Sys_CreateMutex() (threading?threading->CreateMutex():NULL)
#define Sys_LockMutex(m) (threading?threading->LockMutex(m):true)
#define Sys_UnlockMutex if(threading)threading->UnlockMutex
#define Sys_DestroyMutex if(threading)threading->DestroyMutex



//
// in memory
//

typedef struct
{
	fsbucket_t bucket;

	char	name[MAX_QPATH];
	const struct dvpkfile_s *file;
} mvpkfile_t;

typedef struct vpk_s
{
	searchpathfuncs_t pub;
	char	descname[MAX_OSPATH];

	qbyte *treedata;	//raw file list
	size_t treesize;

	struct vpk_s	**fragments;
	size_t numfragments;

	void		*mutex;
	vfsfile_t	*handle;
	unsigned int filepos;	//the pos the subfiles left it at (to optimize calls to vfs_seek)
	int references;	//seeing as all vfiles from a pak file use the parent's vfsfile, we need to keep the parent open until all subfiles are closed.

	size_t		numfiles;
	mvpkfile_t	files[1];	//processed file list
} vpk_t;

//
// on disk
//
typedef struct dvpkfile_s
{	//chars because these are misaligned.
	unsigned char crc[4];
	unsigned char preloadsize[2];
	unsigned char archiveindex[2];
	unsigned char archiveoffset[4];
	unsigned char archivesize[4];
	unsigned char sentinel[2];//=0xffff;
} dvpkfile_t;

typedef struct
{
	//v1
	unsigned int magic;
	unsigned int version;
	unsigned int tablesize;

	//v2
	unsigned int filedatasize;
	unsigned int archivemd5size;
	unsigned int globalmd5size;
	unsigned int signaturesize;
} dvpkheader_t;

static void QDECL FSVPK_GetPathDetails(searchpathfuncs_t *handle, char *out, size_t outlen)
{
	vpk_t *pak = (void*)handle;

	*out = 0;
	if (pak->references != 1)
		Q_snprintfz(out, outlen, "(%i)", pak->references-1);
}
static void QDECL FSVPK_ClosePath(searchpathfuncs_t *handle)
{
	qboolean stillopen;
	size_t i;
	vpk_t *pak = (void*)handle;

	if (!Sys_LockMutex(pak->mutex))
		return;	//ohnoes
	stillopen = --pak->references > 0;
	Sys_UnlockMutex(pak->mutex);
	if (stillopen)
		return;	//not free yet


	VFS_CLOSE (pak->handle);

	Sys_DestroyMutex(pak->mutex);
	for (i = 0; i < pak->numfragments; i++)
	{
		if (pak->fragments[i])
			pak->fragments[i]->pub.ClosePath(&pak->fragments[i]->pub);
		pak->fragments[i] = NULL;
	}
	plugfuncs->Free(pak->fragments);
	plugfuncs->Free(pak->treedata);
	plugfuncs->Free(pak);
}
static void QDECL FSVPK_BuildHash(searchpathfuncs_t *handle, int depth, void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle))
{
	vpk_t *pak = (void*)handle;
	int i;

	for (i = 0; i < pak->numfiles; i++)
	{
		AddFileHash(depth, pak->files[i].name, &pak->files[i].bucket, &pak->files[i]);
	}
}
static unsigned int QDECL FSVPK_FLocate(searchpathfuncs_t *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	mvpkfile_t *pf = hashedresult;
	int i;
	vpk_t		*pak = (void*)handle;

// look through all the pak file elements

	if (pf)
	{	//is this a pointer to a file in this pak?
		if (pf < pak->files || pf > pak->files + pak->numfiles)
			return FF_NOTFOUND;	//was found in a different path
	}
	else
	{
		char fname[MAX_QPATH];
		for (i = 0; i < MAX_QPATH-1 && filename[i]; i++)
		{
			if (filename[i] >= 'A' && filename[i] <= 'Z')
				fname[i] = filename[i] - 'A' + 'a';
			else
				fname[i] = filename[i];
		}
		fname[i] = 0;
		for (i=0 ; i<pak->numfiles ; i++)	//look for the file
		{
			if (!strcmp (pak->files[i].name, fname))
			{
				pf = &pak->files[i];
				break;
			}
		}
	}

	if (pf)
	{
		if (loc)
		{
			loc->fhandle = pf;
			*loc->rawname = 0;
			loc->offset = (qofs_t)-1;
			loc->len =	 ((pf->file->preloadsize[0]<<0)|(pf->file->preloadsize[1]<<8))
						+((pf->file->archivesize[0]<<0)|(pf->file->archivesize[1]<<8)|(pf->file->archivesize[2]<<16)|(pf->file->archivesize[3]<<24));
		}
		return FF_FOUND;
	}
	return FF_NOTFOUND;
}
static int QDECL FSVPK_EnumerateFiles (searchpathfuncs_t *handle, const char *match, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *spath), void *parm)
{
	vpk_t	*pak = (void*)handle;
	int		num;
	mvpkfile_t *file;
	qofs_t	size;

	for (num = 0; num<(int)pak->numfiles; num++)
	{
		if (filefuncs->WildCmp(match, pak->files[num].name))
		{
			file = &pak->files[num];
			//FIXME: time 0? maybe use the pak's mtime?
			size = ((file->file->preloadsize[0]<<0)|(file->file->preloadsize[1]<<8))
				  +((file->file->archivesize[0]<<0)|(file->file->archivesize[1]<<8)|(file->file->archivesize[2]<<16)|(file->file->archivesize[3]<<24));
			if (!func(file->name, size, 0, parm, handle))
				return false;
		}
	}

	return true;
}

static int QDECL FSVPK_GeneratePureCRC(searchpathfuncs_t *handle, const int *seed)
{
	vpk_t *pak = (void*)handle;

	return filefuncs->BlockChecksum(pak->treedata, pak->treesize);
}

typedef struct {
	vfsfile_t funcs;
	vpk_t *parentpak;
	const qbyte *preloaddata;
	size_t preloadsize;
	qofs_t startpos;
	qofs_t length;
	qofs_t currentpos;
} vfsvpk_t;
static int QDECL VFSVPK_ReadBytes (struct vfsfile_s *vfs, void *buffer, int bytestoread)
{
	vfsvpk_t *vfsp = (void*)vfs;
	int read, preread;

	if (bytestoread <= 0)
		return 0;

	if (vfsp->currentpos < vfsp->preloadsize)
	{
		preread = bytestoread;
		if (preread > vfsp->preloadsize-vfsp->currentpos)
			preread = vfsp->preloadsize-vfsp->currentpos;
		if (preread < 0)
			return -1;	//erk...
		memcpy(buffer, vfsp->preloaddata+vfsp->currentpos, preread);
		vfsp->currentpos += preread;
		if (preread == bytestoread)
			return preread;	//we're done, no need to seek etc
		bytestoread -= preread;
		buffer = (char*)buffer+preread;
	}
	else
		preread = 0;

	if (vfsp->currentpos + bytestoread > vfsp->length)
		bytestoread = vfsp->length - vfsp->currentpos;
	if (bytestoread <= 0)
	{
		if (preread)
			return preread;
		return -1;
	}

	if (Sys_LockMutex(vfsp->parentpak->mutex))
	{
		if (vfsp->parentpak->filepos != vfsp->startpos+vfsp->currentpos-vfsp->preloadsize)
			VFS_SEEK(vfsp->parentpak->handle, vfsp->startpos+vfsp->currentpos-vfsp->preloadsize);
		read = VFS_READ(vfsp->parentpak->handle, buffer, bytestoread);
		if (read > 0)
		{
			vfsp->currentpos += read;
			read += preread;
		}
		else if (preread)
			read = preread;
		vfsp->parentpak->filepos = vfsp->startpos+vfsp->currentpos-vfsp->preloadsize;
		Sys_UnlockMutex(vfsp->parentpak->mutex);
	}
	else
		read = preread;

	return read;
}
static int QDECL VFSVPK_WriteBytes (struct vfsfile_s *vfs, const void *buffer, int bytestoread)
{	//not supported.
	plugfuncs->Error("Cannot write to vpk files\n");
	return 0;
}
static qboolean QDECL VFSVPK_Seek (struct vfsfile_s *vfs, qofs_t pos)
{
	vfsvpk_t *vfsp = (void*)vfs;
	if (pos > vfsp->length)
		return false;
	vfsp->currentpos = pos;// + vfsp->startpos;

	return true;
}
static qofs_t QDECL VFSVPK_Tell (struct vfsfile_s *vfs)
{
	vfsvpk_t *vfsp = (void*)vfs;
	return vfsp->currentpos;// - vfsp->startpos;
}
static qofs_t QDECL VFSVPK_GetLen (struct vfsfile_s *vfs)
{
	vfsvpk_t *vfsp = (void*)vfs;
	return vfsp->length;
}
static qboolean QDECL VFSVPK_Close(vfsfile_t *vfs)
{
	vfsvpk_t *vfsp = (void*)vfs;
	FSVPK_ClosePath(&vfsp->parentpak->pub);	//tell the parent that we don't need it open any more (reference counts)
	plugfuncs->Free(vfsp);	//free ourselves.
	return true;
}
static vfsfile_t *QDECL FSVPK_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode)
{
	vpk_t *pack = (void*)handle;
	vfsvpk_t *vfs;
	mvpkfile_t *f = loc->fhandle;
	unsigned int frag;

	if (strcmp(mode, "rb"))
		return NULL; //urm, unable to write/append

	frag = (f->file->archiveindex[0]<<0)|(f->file->archiveindex[1]<<8);
	if (frag >= pack->numfragments || !pack->fragments[frag])
		return NULL;
	pack = pack->fragments[frag];

	vfs = plugfuncs->Malloc(sizeof(vfsvpk_t));

	vfs->parentpak = pack;
	if (!Sys_LockMutex(pack->mutex))
	{
		plugfuncs->Free(vfs);
		return NULL;
	}
	vfs->parentpak->references++;
	Sys_UnlockMutex(pack->mutex);

	vfs->preloaddata = (const qbyte*)f->file + sizeof(*f->file);
	vfs->preloadsize = (f->file->preloadsize[0]<<0) | (f->file->preloadsize[1]<<8);

	vfs->startpos = (f->file->archiveoffset[0]<<0)|(f->file->archiveoffset[1]<<8)|(f->file->archiveoffset[2]<<16)|(f->file->archiveoffset[3]<<24);
	vfs->length = loc->len;
	vfs->currentpos = 0;

#ifdef _DEBUG
	{
		mvpkfile_t *pf = loc->fhandle;
		Q_strlcpy(vfs->funcs.dbgname, pf->name, sizeof(vfs->funcs.dbgname));
	}
#endif
	vfs->funcs.Close = VFSVPK_Close;
	vfs->funcs.GetLen = VFSVPK_GetLen;
	vfs->funcs.ReadBytes = VFSVPK_ReadBytes;
	vfs->funcs.Seek = VFSVPK_Seek;
	vfs->funcs.Tell = VFSVPK_Tell;
	vfs->funcs.WriteBytes = VFSVPK_WriteBytes;	//not supported

	return (vfsfile_t *)vfs;
}

static void QDECL FSVPK_ReadFile(searchpathfuncs_t *handle, flocation_t *loc, char *buffer)
{
	vfsfile_t *f;
	f = FSVPK_OpenVFS(handle, loc, "rb");
	if (!f)	//err...
		return;
	VFS_READ(f, buffer, loc->len);
	VFS_CLOSE(f);
}

static unsigned int FSVPK_WalkTree(vpk_t *vpk, const char *start, const char *end)
{	//the weird arrangement of these files is presumably an indicator of how source handles its file types.
	const char *ext, *path, *name;
	const dvpkfile_t *file;
	size_t preloadsize;
	unsigned int files = 0;
	while(start < end)
	{	//extensions
		ext = start;
		if (!*ext)
		{
			start++;
			break;
		}
		if (ext[0] == ' ' && !ext[1])
			ext = "";
		start += strlen(start)+1;
		while(start < end)
		{	//paths
			path = start;
			if (!*path)
			{
				start++;
				break;
			}
			if (path[0] == ' ' && !path[1])
				path = "";
			start += strlen(start)+1;
			while(start < end)
			{	//names
				name = start;
				if (!*name)
				{
					start++;
					break;
				}
				if (name[0] == ' ' && !name[1])
					name = "";
				start += strlen(start)+1;

				file = (const dvpkfile_t*)start;
				preloadsize = (file->preloadsize[0]<<0) | (file->preloadsize[1]<<8);
				start += sizeof(*file)+preloadsize;
				if (start > end)
					return 0;	//truncated...
				if (file->sentinel[0] != 0xff || file->sentinel[1] != 0xff)
					return 0;	//sentinel failure
//				Con_Printf("Found file %s%s%s%s%s\n", path, *path?"/":"", name, *ext?".":"", ext);
				if (!vpk)
					files++;
				else if (files < vpk->numfiles)
				{
					unsigned int frag = (file->archiveindex[0]<<0)|(file->archiveindex[1]<<8);
					Q_snprintfz(vpk->files[files].name, sizeof(vpk->files[files].name), "%s%s%s%s%s", path, *path?"/":"", name, *ext?".":"", ext);
					filefuncs->CleanUpPath(vpk->files[files].name);	//just in case...
					vpk->files[files].file = file;

					if (vpk->numfragments < frag+1)
						vpk->numfragments = frag+1;
					files++;
				}
			}
		}
	}
	return files;
}

searchpathfuncs_t *QDECL FSVVPK_LoadArchive(vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix);

/*
=================
COM_LoadPackFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
static searchpathfuncs_t *QDECL FSVPK_LoadArchive (vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix)
{
	dvpkheader_t	header;
	int				i;
	int				numpackfiles;
	vpk_t			*vpk, *f;
	vfsfile_t		*packhandle;
	int				read;
	qbyte			*tree;
	unsigned int	frag;
	unsigned int tablesize;

	packhandle = file;
	if (packhandle == NULL)
		return NULL;

	if (prefix && *prefix)
		return NULL;	//not supported at this time

	read = VFS_READ(packhandle, &header, sizeof(header));
	header.magic = LittleLong(header.magic);
	header.version = LittleLong(header.version);
	header.tablesize = LittleLong(header.tablesize);

	header.filedatasize = LittleLong(header.filedatasize);
	header.archivemd5size = LittleLong(header.archivemd5size);
	header.globalmd5size = LittleLong(header.globalmd5size);
	header.signaturesize = LittleLong(header.signaturesize);

	if (read < 12 || header.magic != 0x55aa1234 || header.tablesize <= 0)
	{	//this will include the non-dir files too.
//		Con_Printf("%s is not a vpk\n", desc);

		/* HACK: pre 2009 Left4Dead */
		if (header.magic == 7630198) {
			VFS_SEEK(packhandle, 0);
			tablesize = 1313113;
		} else {
			// try to load as vtmb vpk
			return FSVVPK_LoadArchive(file, parent, filename, desc, prefix);
		}
	} else {
		i = LittleLong(header.version);
		if (i == 2)
			;//VFS_SEEK(packhandle, 7*sizeof(int));
		else if (i == 1)
			VFS_SEEK(packhandle, 3*sizeof(int));
		else
		{
			Con_Printf("vpk %s is version %x (unspported)\n", desc, i);
			return NULL;
		}
		tablesize = header.tablesize;
	}
	
	tree = plugfuncs->Malloc(tablesize);
	read = VFS_READ(packhandle, tree, tablesize);

	numpackfiles = FSVPK_WalkTree(NULL, tree, tree+read);

	vpk = (vpk_t*)plugfuncs->Malloc (sizeof (*vpk) + sizeof(*vpk->files)*(numpackfiles-1));
	vpk->treedata = tree;
	vpk->treesize = read;
	vpk->numfiles = numpackfiles;
	vpk->numfragments = 0;
	vpk->numfiles = FSVPK_WalkTree(vpk, tree, tree+read);

	strcpy (vpk->descname, desc);
	vpk->handle = packhandle;
	vpk->filepos = 0;
	VFS_SEEK(packhandle, vpk->filepos);

	vpk->references++;

	vpk->mutex = Sys_CreateMutex();

	Con_Printf ("Added vpkfile %s (%i files)\n", desc, numpackfiles);

	vpk->pub.fsver			= FSVER;
	vpk->pub.GetPathDetails = FSVPK_GetPathDetails;
	vpk->pub.ClosePath = FSVPK_ClosePath;
	vpk->pub.BuildHash = FSVPK_BuildHash;
	vpk->pub.FindFile = FSVPK_FLocate;
	vpk->pub.ReadFile = FSVPK_ReadFile;
	vpk->pub.EnumerateFiles = FSVPK_EnumerateFiles;
	vpk->pub.GeneratePureCRC = FSVPK_GeneratePureCRC;
	vpk->pub.OpenVFS = FSVPK_OpenVFS;

	vpk->fragments = plugfuncs->Malloc(vpk->numfragments*sizeof(*vpk->fragments));
	for(frag = 0; frag < vpk->numfragments; frag++)
	{
		flocation_t loc;
		char fragname[MAX_OSPATH], *ext;
		Q_strlcpy(fragname, filename, sizeof(fragname));
		ext = strrchr(fragname, '.');
		if (!ext)
			ext = fragname + strlen(fragname);
		if (ext-fragname>4 && !strncmp(ext-4, "_dir", 4))
			ext-=4;
		Q_snprintfz(ext, sizeof(fragname)-(ext-fragname), "_%03u.vpk", frag);
		if (parent->FindFile(parent, &loc, fragname, NULL) != FF_FOUND)
			continue;
		packhandle = parent->OpenVFS(parent, &loc, "rb");
		if (!packhandle)
			continue;

		vpk->fragments[frag] = f = (vpk_t*)plugfuncs->Malloc(sizeof(*f));
//		Q_strncpyz(f->descname, splitname, sizeof(f->descname));
		f->handle = packhandle;
//		f->rawsize = VFS_GETLEN(f->raw);
		f->references = 1;
		f->mutex = Sys_CreateMutex();
		f->pub.ClosePath			= FSVPK_ClosePath;
	}
	return &vpk->pub;
}

qboolean VVPK_Init(void);

qboolean VPK_Init(void)
{
	threading = plugfuncs->GetEngineInterface(plugthreadfuncs_name, sizeof(*threading));
	if (!threading)
		return false;
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	if (!filefuncs)
		return false;

	// vtmb
	if (!VVPK_Init())
		return false;

	//we can't cope with being closed randomly. files cannot be orphaned safely.
	//so ask the engine to ensure we don't get closed before everything else is.
	plugfuncs->ExportFunction("MayShutdown", NULL);

	if (!plugfuncs->ExportFunction("FS_RegisterArchiveType_vpk", FSVPK_LoadArchive))
	{
		Con_Printf("hl2: Engine doesn't support filesystem plugins\n");
		return false;
	}

	return true;
}

