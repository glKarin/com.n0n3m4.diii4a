#include "quakedef.h"
#include "fs.h"

#ifdef PACKAGE_Q1PAK

//
// in memory
//

typedef struct
{
	fsbucket_t bucket;

	char	name[MAX_QPATH];
	unsigned int		filepos, filelen;
} mpackfile_t;

typedef struct pack_s
{
	searchpathfuncs_t pub;
	char	descname[MAX_OSPATH];
	int		numfiles;
	mpackfile_t	*files;

	void		*mutex;
	vfsfile_t	*handle;
	qofs_t		filepos;	//the pos the subfiles left it at (to optimize calls to vfs_seek)
	qatomic32_t references;	//seeing as all vfiles from a pak file use the parent's vfsfile, we need to keep the parent open until all subfiles are closed.
} pack_t;

//
// on disk
//
typedef struct
{
	char	name[56];
	unsigned int		filepos, filelen;
} dpackfile_t;

typedef struct
{
	unsigned int		filepos, filelen;
	char	name[8];
} dwadfile_t;

typedef struct
{
	char	id[4];
	unsigned int		dirofs;
	unsigned int		dirlen;
} dpackheader_t;

typedef struct
{
	char	id[4];
	unsigned int		dirlen;
	unsigned int		dirofs;
} dwadheader_t;

#define	MAX_FILES_IN_PACK	2048

static void QDECL FSPAK_GetPathDetails(searchpathfuncs_t *handle, char *out, size_t outlen)
{
	pack_t *pak = (pack_t*)handle;

	*out = 0;
	if (pak->references != 1)
		Q_snprintfz(out, outlen, "(%i)", pak->references-1);
}
static void QDECL FSPAK_AddReference(searchpathfuncs_t *handle)
{
	pack_t *pak = (void*)handle;
	FTE_Atomic32_Inc(&pak->references);
}
static void QDECL FSPAK_ClosePath(searchpathfuncs_t *handle)
{
	qboolean stillopen;
	pack_t *pak = (void*)handle;

	stillopen = FTE_Atomic32_Dec(&pak->references) > 0;
	if (stillopen)
		return;	//not free yet


	VFS_CLOSE (pak->handle);

	Sys_DestroyMutex(pak->mutex);
	if (pak->files)
		Z_Free(pak->files);
	Z_Free(pak);
}
static void QDECL FSPAK_BuildHash(searchpathfuncs_t *handle, int depth, void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle))
{
	pack_t *pak = (void*)handle;
	int i;

	for (i = 0; i < pak->numfiles; i++)
	{
		AddFileHash(depth, pak->files[i].name, &pak->files[i].bucket, &pak->files[i]);
	}
}
static unsigned int QDECL FSPAK_FLocate(searchpathfuncs_t *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	mpackfile_t *pf = hashedresult;
	int i;
	pack_t		*pak = (void*)handle;

// look through all the pak file elements

	if (pf)
	{	//is this a pointer to a file in this pak?
		if (pf < pak->files || pf > pak->files + pak->numfiles)
			return FF_NOTFOUND;	//was found in a different path
	}
	else
	{
		for (i=0 ; i<pak->numfiles ; i++)	//look for the file
		{
			if (!Q_strcasecmp (pak->files[i].name, filename))
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
			Q_snprintfz(loc->rawname, sizeof(loc->rawname), "%s", pak->descname);
			loc->offset = pf->filepos;
			loc->len = pf->filelen;
		}
		return FF_FOUND;
	}
	return FF_NOTFOUND;
}
static int QDECL FSPAK_EnumerateFiles (searchpathfuncs_t *handle, const char *match, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *spath), void *parm)
{
	pack_t	*pak = (pack_t*)handle;
	int		num;

	for (num = 0; num<(int)pak->numfiles; num++)
	{
		if (wildcmp(match, pak->files[num].name))
		{
			//FIXME: time 0? maybe use the pak's mtime?
			if (!func(pak->files[num].name, pak->files[num].filelen, 0, parm, handle))
				return false;
		}
	}

	return true;
}

static int QDECL FSPAK_GeneratePureCRC(searchpathfuncs_t *handle, const int *seed)
{	//this is really weak. :(
	pack_t *pak = (void*)handle;

	int result;
	int *filecrcs;
	int numcrcs=0;
	int i;

	filecrcs = BZ_Malloc((pak->numfiles+1)*sizeof(int));
	if (seed)	//so exploiters can't cache it in purity replies.
		filecrcs[numcrcs++] = *seed;

	for (i = 0; i < pak->numfiles; i++)
	{
		if (pak->files[i].filelen > 0)
		{
			filecrcs[numcrcs++] = pak->files[i].filepos ^ pak->files[i].filelen ^ CalcHashInt(&hash_crc16, pak->files[i].name, sizeof(56));
		}
	}

	result = CalcHashInt(&hash_md4, filecrcs, numcrcs*sizeof(int));

	BZ_Free(filecrcs);
	return result;
}

typedef struct {
	vfsfile_t funcs;
	pack_t *parentpak;
	qofs_t startpos;
	qofs_t length;
	qofs_t currentpos;
} vfspack_t;
static int QDECL VFSPAK_ReadBytes (struct vfsfile_s *vfs, void *buffer, int bytestoread)
{
	vfspack_t *vfsp = (vfspack_t*)vfs;
	int read;

	if (bytestoread == 0)
		return 0;

	if (vfsp->currentpos - vfsp->startpos + bytestoread > vfsp->length)
		bytestoread = vfsp->length - (vfsp->currentpos - vfsp->startpos);
	if (bytestoread <= 0)
	{
		return -1;
	}

	if (Sys_LockMutex(vfsp->parentpak->mutex))
	{
		if (vfsp->parentpak->filepos != vfsp->currentpos)
			VFS_SEEK(vfsp->parentpak->handle, vfsp->currentpos);
		read = VFS_READ(vfsp->parentpak->handle, buffer, bytestoread);
		vfsp->currentpos += read;
		vfsp->parentpak->filepos = vfsp->currentpos;
		Sys_UnlockMutex(vfsp->parentpak->mutex);
	}
	else
		read = 0;

	return read;
}
static int QDECL VFSPAK_WriteBytes (struct vfsfile_s *vfs, const void *buffer, int bytestoread)
{	//not supported.
	Sys_Error("Cannot write to pak files\n");
	return 0;
}
static qboolean QDECL VFSPAK_Seek (struct vfsfile_s *vfs, qofs_t pos)
{
	vfspack_t *vfsp = (vfspack_t*)vfs;
	if (pos > vfsp->length)
		return false;
	vfsp->currentpos = pos + vfsp->startpos;

	return true;
}
static qofs_t QDECL VFSPAK_Tell (struct vfsfile_s *vfs)
{
	vfspack_t *vfsp = (vfspack_t*)vfs;
	return vfsp->currentpos - vfsp->startpos;
}
static qofs_t QDECL VFSPAK_GetLen (struct vfsfile_s *vfs)
{
	vfspack_t *vfsp = (vfspack_t*)vfs;
	return vfsp->length;
}
static qboolean QDECL VFSPAK_Close(vfsfile_t *vfs)
{
	vfspack_t *vfsp = (vfspack_t*)vfs;
	FSPAK_ClosePath(&vfsp->parentpak->pub);	//tell the parent that we don't need it open any more (reference counts)
	Z_Free(vfsp);	//free ourselves.
	return true;
}
static vfsfile_t *QDECL FSPAK_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode)
{
	pack_t *pack = (pack_t*)handle;
	vfspack_t *vfs;

	if (strcmp(mode, "rb"))
		return NULL; //urm, unable to write/append

	vfs = Z_Malloc(sizeof(vfspack_t));

	vfs->parentpak = pack;
	FTE_Atomic32_Inc(&vfs->parentpak->references);

	vfs->startpos = loc->offset;
	vfs->length = loc->len;
	vfs->currentpos = vfs->startpos;

#ifdef _DEBUG
	{
		mpackfile_t *pf = loc->fhandle;
		Q_strncpyz(vfs->funcs.dbgname, pf->name, sizeof(vfs->funcs.dbgname));
	}
#endif
	vfs->funcs.Close = VFSPAK_Close;
	vfs->funcs.GetLen = VFSPAK_GetLen;
	vfs->funcs.ReadBytes = VFSPAK_ReadBytes;
	vfs->funcs.Seek = VFSPAK_Seek;
	vfs->funcs.Tell = VFSPAK_Tell;
	vfs->funcs.WriteBytes = VFSPAK_WriteBytes;	//not supported

	return (vfsfile_t *)vfs;
}

static void QDECL FSPAK_ReadFile(searchpathfuncs_t *handle, flocation_t *loc, char *buffer)
{
	vfsfile_t *f;
	f = FSPAK_OpenVFS(handle, loc, "rb");
	if (!f)	//err...
		return;
	VFS_READ(f, buffer, loc->len);
	VFS_CLOSE(f);
}


/*
=================
COM_LoadPackFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
searchpathfuncs_t *QDECL FSPAK_LoadArchive (vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix)
{
	dpackheader_t	header = {};
	int				i;
	mpackfile_t		*newfiles;
	int				numpackfiles;
	pack_t			*pack;
	vfsfile_t		*packhandle;
	dpackfile_t		info;
	int read;

	packhandle = file;
	if (packhandle == NULL)
		return NULL;

	if (prefix && *prefix)
		return NULL;	//not supported at this time

	read = VFS_READ(packhandle, &header, sizeof(header));
	if (read < sizeof(header) || header.id[0] != 'P' || header.id[1] != 'A'
	|| header.id[2] != 'C' || header.id[3] != 'K')
	{
		header.dirofs = 0;
		Con_Printf("%s is not a pak - \"%s\"\n", desc, header.id);
		return NULL;
	}
	header.dirofs = LittleLong (header.dirofs);
	header.dirlen = LittleLong (header.dirlen);

	numpackfiles = header.dirlen / sizeof(dpackfile_t);

	newfiles = (mpackfile_t*)Z_Malloc (numpackfiles * sizeof(mpackfile_t));

	VFS_SEEK(packhandle, header.dirofs);

	pack = (pack_t*)Z_Malloc (sizeof (pack_t));
// parse the directory
	for (i=0 ; i<numpackfiles ; i++)
	{
		*info.name = '\0';
		read = VFS_READ(packhandle, &info, sizeof(info));
		if (read != sizeof(info))
		{
			Con_Printf("PAK file table truncated, only found %i files out of %i\n", i, numpackfiles);
			numpackfiles = i;
			break;
		}

		memcpy(newfiles[i].name, info.name, sizeof(info.name));
		newfiles[i].name[min(sizeof(info.name), MAX_QPATH-1)] = 0; //paranoid
		COM_CleanUpPath(newfiles[i].name);	//blooming tanks.
		newfiles[i].filepos = LittleLong(info.filepos);
		newfiles[i].filelen = LittleLong(info.filelen);
	}
/*
	if (crc != PAK0_CRC || numpackfiles != PAK0_COUNT)
		com_modified = true;
*/
	strcpy (pack->descname, desc);
	pack->handle = packhandle;
	pack->numfiles = numpackfiles;
	pack->files = newfiles;
	pack->filepos = 0;
	VFS_SEEK(packhandle, pack->filepos);

	pack->references++;

	pack->mutex = Sys_CreateMutex();

//	Con_TPrintf ("Added packfile %s (%i files)\n", desc, numpackfiles);

	pack->pub.fsver			= FSVER;
	pack->pub.GetPathDetails = FSPAK_GetPathDetails;
	pack->pub.AddReference = FSPAK_AddReference;
	pack->pub.ClosePath = FSPAK_ClosePath;
	pack->pub.BuildHash = FSPAK_BuildHash;
	pack->pub.FindFile = FSPAK_FLocate;
	pack->pub.ReadFile = FSPAK_ReadFile;
	pack->pub.EnumerateFiles = FSPAK_EnumerateFiles;
	pack->pub.GeneratePureCRC = FSPAK_GeneratePureCRC;
	pack->pub.OpenVFS = FSPAK_OpenVFS;
	return &pack->pub;
}

#ifdef PACKAGE_DOOMWAD
searchpathfuncs_t *QDECL FSDWD_LoadArchive (vfsfile_t *packhandle, searchpathfuncs_t *parent, const char *wadname, const char *desc, const char *prefix)
{
	dwadheader_t	header;
	int				i;
	mpackfile_t		*newfiles;
	int				numpackfiles;
	pack_t			*pack;
	dwadfile_t		info;

	int section=0;
	char sectionname[MAX_QPATH];
	char filename[52];
	char neatwadname[52];

	if (packhandle == NULL)
		return NULL;

	if (prefix && *prefix)
		return NULL;	//not supported at this time

	VFS_READ(packhandle, &header, sizeof(header));
	if (header.id[1] != 'W'	|| header.id[2] != 'A' || header.id[3] != 'D')
		return NULL;	//not a doom wad

	//doom wads come in two sorts. iwads and pwads.
	//iwads are the master wads, pwads are meant to replace parts of the master wad.
	//this is awkward, of course.
	//we ignore the i/p bit for the most part, but with maps, pwads are given a prefixed name.
	if (header.id[0] == 'I')
		*neatwadname = '\0';
	else if (header.id[0] == 'P')
	{
		COM_FileBase(desc, neatwadname, sizeof(neatwadname));
		strcat(neatwadname, "#");
	}
	else
		return NULL;

	header.dirofs = LittleLong (header.dirofs);
	header.dirlen = LittleLong (header.dirlen);

	numpackfiles = header.dirlen;
	newfiles = (mpackfile_t*)Z_Malloc (numpackfiles * sizeof(mpackfile_t));
	VFS_SEEK(packhandle, header.dirofs);

	//doom wads are awkward.
	//they have no directory structure, except for start/end 'files'.
	//they follow along the lines of lumps after the parent name.
	//a map is the name of that map, and then a squence of the lumps that form that map (found by next-with-that-name).
	//this is a problem for a real virtual filesystem, so we add a hack to recognise special names and expand them specially.
	for (i=0 ; i<numpackfiles ; i++)
	{
		VFS_READ (packhandle, &info, sizeof(info));

		strcpy (filename, info.name);
		filename[8] = '\0';
		Q_strlwr(filename);

		newfiles[i].filepos = LittleLong(info.filepos);
		newfiles[i].filelen = LittleLong(info.filelen);

		switch(section)	//be prepared to remap filenames.
		{
newsection:
		case 0:
			if (info.filelen == 0)
			{	//marker for something...

				if (!strcmp(filename, "s_start"))
				{
					section = 2;
					sprintf (newfiles[i].name, "sprites/%s", filename);	//the model loader has a hack to recognise .dsp
					break;
				}
				if (!strcmp(filename, "p_start"))
				{
					section = 3;
					sprintf (newfiles[i].name, "patches/%s.pat", filename); //the map loader will find these.
					break;
				}
				if (!strcmp(filename, "f_start"))
				{
					section = 4;
					sprintf (newfiles[i].name, "flats/%s", filename);	//the map loader will find these
					break;
				}
				if ((filename[0] == 'e' && filename[2] == 'm') || !strncmp(filename, "map", 3))
				{	//this is the start of a beutiful new map
					section = 1;
					strcpy(sectionname, filename);
					sprintf (newfiles[i].name, "maps/%s%s.bsp", neatwadname, filename);	//generate fake bsps to allow the server to find them
					newfiles[i].filepos = 0;
					newfiles[i].filelen = 4;
					break;
				}
				if (!Q_strncmp(filename, "gl_", 3) && ((filename[3] == 'e' && filename[5] == 'm') || !strncmp(filename+3, "map", 3)))
				{	//this is the start of a beutiful new map
					section = 5;
					strcpy(sectionname, filename+3);
					break;
				}
			}

			sprintf (newfiles[i].name, "wad/%s", filename);	//but there are many files that we don't recognise/know about. archive them off to keep the vfs moderatly clean.
			break;
		case 1:	//map section
			if (strcmp(filename, "things") &&
				strcmp(filename, "linedefs") &&
				strcmp(filename, "sidedefs") &&
				strcmp(filename, "vertexes") &&
				strcmp(filename, "segs") &&
				strcmp(filename, "ssectors") &&
				strcmp(filename, "nodes") &&
				strcmp(filename, "sectors") &&
				strcmp(filename, "reject") &&
				strcmp(filename, "blockmap"))
			{
				section = 0;
				goto newsection;
			}
			sprintf (newfiles[i].name, "maps/%s%s.%s", neatwadname, sectionname, filename);
			break;
		case 5:	//glbsp output section
			if (strcmp(filename, "gl_vert") &&
				strcmp(filename, "gl_segs") &&
				strcmp(filename, "gl_ssect") &&
				strcmp(filename, "gl_pvs") &&
				strcmp(filename, "gl_nodes"))
			{
				section = 0;
				goto newsection;
			}
			sprintf (newfiles[i].name, "maps/%s%s.%s", neatwadname, sectionname, filename);
			break;
		case 2:	//sprite section. sprites use the first 4 letters to identify the name of the sprite, and the last 4 to define its frame+dir [+ xflipped frame+dir]
			//FIXME: inject a '.dsp' file for filesystem accountability when we see an a0 (or a1) postfix.
			if (!strcmp(filename, "s_end"))
			{
				section = 0;
				goto newsection;
			}
			sprintf (newfiles[i].name, "sprites/%s", filename);
			break;
		case 3:	//patches section. they need the textures1/2 lump in order to position correctly as actual textures.
			if (!strcmp(filename, "p_end"))
			{
				section = 0;
				goto newsection;
			}
			sprintf (newfiles[i].name, "patches/%s.pat", filename);
			break;
		case 4:	//flats section. note that these are raw p8 64*64 images
			if (!strcmp(filename, "f_end"))
			{
				section = 0;
				goto newsection;
			}
			sprintf (newfiles[i].name, "flats/%s.raw", filename);
			break;
		}
	}

	pack = (pack_t*)Z_Malloc (sizeof (pack_t));
	strcpy (pack->descname, desc);
	pack->handle = packhandle;
	pack->numfiles = numpackfiles;
	pack->files = newfiles;
	pack->filepos = 0;
	VFS_SEEK(packhandle, pack->filepos);

	pack->references++;

	Con_TPrintf ("Added packfile %s (%i files)\n", desc, numpackfiles);

	pack->mutex = Sys_CreateMutex();

	pack->pub.fsver			= FSVER;
	pack->pub.GetPathDetails = FSPAK_GetPathDetails;
	pack->pub.ClosePath = FSPAK_ClosePath;
	pack->pub.BuildHash = FSPAK_BuildHash;
	pack->pub.FindFile = FSPAK_FLocate;
	pack->pub.ReadFile = FSPAK_ReadFile;
	pack->pub.EnumerateFiles = FSPAK_EnumerateFiles;
	pack->pub.GeneratePureCRC = FSPAK_GeneratePureCRC;
	pack->pub.OpenVFS = FSPAK_OpenVFS;
	return &pack->pub;
}
#endif
#endif
