#include "hash.h"

/*
Threading:
When the main thread will harm the filesystem tree/hash, it will first lock fs_thread_mutex (FIXME: make a proper rwlock).
Worker threads must thus lock that mutex for any opens (to avoid it changing underneath it), but can unlock it as soon as the open call returns.
Files may be shared between threads, but not simultaneously.
The filesystem driver is responsible for closing the pak/pk3 once all files are closed, and must ensure that opens+reads+closes as well as archive closure are thread safe.
*/

#define FSVER 3


#define FF_NOTFOUND		(0u)	//file wasn't found
#define FF_FOUND		(1u<<0u)	//file was found
#define FF_SYMLINK		(1u<<1u)	//file contents are the name of a different file (symlink). do a recursive lookup on the name
#define FF_DIRECTORY	(1u<<2u)

typedef struct
{
	bucket_t buck;
	int depth;	/*shallower files will remove deeper files*/
} fsbucket_t;
extern hashtable_t filesystemhash;	//this table is the one to build your hash references into
extern int fs_hash_dups;	//for tracking efficiency. no functional use.
extern int fs_hash_files;	//for tracking efficiency. no functional use.
extern qboolean fs_readonly;	//if true, fopen(, "w") should always fail.
extern void *fs_thread_mutex;
extern float fs_accessed_time;
extern cvar_t	fs_dlURL;

struct searchpath_s;
struct searchpathfuncs_s
{
	int				fsver;
	void			(QDECL *ClosePath)(searchpathfuncs_t *handle);		//removes a reference, kills it when it reaches 0. the package can only actually be killed once all contained files are closed.
	void			(QDECL *AddReference)(searchpathfuncs_t *handle);	//adds an extra reference, so we survive closes better.

	void			(QDECL *GetPathDetails)(searchpathfuncs_t *handle, char *outdetails, size_t sizeofdetails);
	void			(QDECL *BuildHash)(searchpathfuncs_t *handle, int depth, void (QDECL *FS_AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle));
	unsigned int	(QDECL *FindFile)(searchpathfuncs_t *handle, flocation_t *loc, const char *name, void *hashedresult);	//true if found (hashedresult can be NULL)
		//note that if rawfile and offset are set, many Com_FileOpens will read the raw file
		//otherwise ReadFile will be called instead.
	void			(QDECL *ReadFile)(searchpathfuncs_t *handle, flocation_t *loc, char *buffer);	//reads the entire file in one go (size comes from loc, so make sure the loc is valid, this is for performance with compressed archives)
	int				(QDECL *EnumerateFiles)(searchpathfuncs_t *handle, const char *match, int (QDECL *func)(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath), void *parm);

	int				(QDECL *GeneratePureCRC) (searchpathfuncs_t *handle, const int *seed);

	vfsfile_t *		(QDECL *OpenVFS)(searchpathfuncs_t *handle, flocation_t *loc, const char *mode);

	qboolean		(QDECL *PollChanges)(searchpathfuncs_t *handle);	//returns true if there were changes

	qboolean		(QDECL *FileStat)(searchpathfuncs_t *handle, flocation_t *loc, time_t *mtime);
	qboolean		(QDECL *CreateFile)(searchpathfuncs_t *handle, flocation_t *loc, const char *filename);		//like FindFile, but returns a usable loc even if the file does not exist yet (may also create requisite directories too)
	qboolean		(QDECL *RenameFile)(searchpathfuncs_t *handle, const char *oldname, const char *newname);	//returns true on success, false if source doesn't exist, or if dest does (cached locs may refer to either new or old name).
	qboolean		(QDECL *RemoveFile)(searchpathfuncs_t *handle, const char *filename);	//returns true on success, false if it wasn't found or is readonly.
};
//searchpathfuncs_t *(QDECL *OpenNew)(vfsfile_t *file, const char *desc);	//returns a handle to a new pak/path

//the stdio filesystem is special as that's the starting point of the entire filesystem
//warning: the handle is known to be a string pointer to the dir name
extern searchpathfuncs_t *(QDECL VFSOS_OpenPath)	(vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix);
extern searchpathfuncs_t *(QDECL FSZIP_LoadArchive) (vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix);
extern searchpathfuncs_t *(QDECL FSPAK_LoadArchive) (vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix);
extern searchpathfuncs_t *(QDECL FSDWD_LoadArchive) (vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix);
extern searchpathfuncs_t *(QDECL FSDZ_LoadArchive)	(vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix);
vfsfile_t *QDECL VFSOS_Open(const char *osname, const char *mode);
vfsfile_t *FS_DecompressGZip(vfsfile_t *infile, vfsfile_t *outfile);

int FS_RegisterFileSystemType(void *module, const char *extension, searchpathfuncs_t *(QDECL *OpenNew)(vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix), qboolean loadscan);
void FS_UnRegisterFileSystemType(int idx);
void FS_UnRegisterFileSystemModule(void *module);

void FS_AddHashedPackage(searchpath_t **oldpaths, const char *parent_pure, const char *parent_logical, searchpath_t *search, unsigned int loadstuff, const char *pakpath, const char *qhash, const char *pakprefix, unsigned int packageflags);
void PM_LoadPackages(searchpath_t **oldpaths, const char *parent_pure, const char *parent_logical, searchpath_t *search, unsigned int loadstuff, int minpri, int maxpri);
qboolean PM_HandleRedirect(const char *package, char *url, size_t urlsize);
void PM_ManifestChanged(ftemanifest_t *man);
void *PM_GeneratePackageFromMeta(vfsfile_t *file, char *fname, size_t fnamesize, enum fs_relative *fsroot);
void PM_FileInstalled(const char *filename, enum fs_relative fsroot, void *metainfo, qboolean enable); //we finished installing a file via some other mechanism (drag+drop or from server. insert it into the updates menu.
void PM_EnumeratePlugins(void (*callback)(const char *name, qboolean blocked));
struct xcommandargcompletioncb_s;
void PM_EnumerateMaps(const char *partial, struct xcommandargcompletioncb_s *ctx);
void PM_LoadMap(const char *package, const char *map);
unsigned int PM_IsApplying(void);
unsigned int PM_MarkUpdates (void);	//mark new/updated packages as needing install.
void PM_ApplyChanges(void);	//for -install/-doinstall args
qboolean PM_AreSourcesNew(qboolean doprompt);
qboolean PM_FindUpdatedEngine(char *syspath, size_t syspathsize);	//names the engine we should be running
void PM_AddManifestPackages(ftemanifest_t *man, qboolean mayapply);
void Menu_Download_Update(void);

typedef struct
{
	char *description;
	void (*Update)			(const char *url, vfsfile_t *out, qboolean favourcache);
#define plugupdatesourcefuncs_name "UpdateSource"
} plugupdatesourcefuncs_t;
qboolean PM_RegisterUpdateSource(void *module, plugupdatesourcefuncs_t *funcs);

enum modsourcetype_e
{
	MST_SYSTEM,		//found via an fmf installed at system level (eg part of a flatpak app)
	MST_DEFAULT,	//the default.fmf in the working directory
	MST_BASEDIR,	//other fmf files in the basedir
	MST_HOMEDIR,	//other fmf files in the homedir
	MST_GAMEDIR,	//mod found from just looking for gamedirs.
	MST_INTRINSIC,	//knowledge of the mod came one of the games we're hardcoded with.

	MST_UNKNOWN,	//forgot where it came from...
};
int FS_EnumerateKnownGames(qboolean (*callback)(void *usr, ftemanifest_t *man, enum modsourcetype_e sourcetype), void *usr, qboolean fixedbasedir);

struct modlist_s
{
	ftemanifest_t *manifest;
	enum modsourcetype_e sourcetype;
	char *gamedir;
	char *description;
};
struct modlist_s *Mods_GetMod(size_t diridx);

#define SPF_REFERENCED		1		//something has been loaded from this path. should filter out client references...
#define SPF_COPYPROTECTED	2		//downloads are not allowed fom here.
#define SPF_TEMPORARY		4		//a map-specific path, purged at map change.
#define SPF_EXPLICIT		8		//a root gamedir (bumps depth on gamedir depth checks).
#define SPF_UNTRUSTED		16		//has been downloaded from somewhere. configs inside it should never be execed with local access rights.
#define SPF_PRIVATE			32		//private to the client. ie: the fte dir. name is not networked.
#define SPF_WRITABLE		64		//safe to write here. lots of weird rules etc.
#define SPF_BASEPATH		128		//part of the basegames, and not the mod gamedir(s).
#define SPF_QSHACK			256		//a bit of a hack, allows scanning for $rootdir/quakespasm.pak
#define SPF_SERVER			512		//a package that was loaded to match the server's packages
#define SPF_ISDIR			1024	//is an actual directory (not itself a package).
#define SPF_VIRTUAL			2048	//path is virtual in some form, and the logicalpath is NOT a path..
qboolean FS_LoadPackageFromFile(vfsfile_t *vfs, char *pname, char *localname, int *crc, unsigned int flags);

#ifdef AVAIL_XZDEC
vfsfile_t *FS_XZ_DecompressWriteFilter(vfsfile_t *infile);
vfsfile_t *FS_XZ_DecompressReadFilter(vfsfile_t *srcfile);
#endif
#ifdef AVAIL_GZDEC
vfsfile_t *FS_GZ_WriteFilter(vfsfile_t *outfile, qboolean autoclosefile, qboolean compress);
#endif
