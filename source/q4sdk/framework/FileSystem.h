// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

/*
===============================================================================

	File System

	No stdio calls should be used by any part of the game, because of all sorts
	of directory and separator char issues. Throughout the game a forward slash
	should be used as a separator. The file system takes care of the conversion
	to an OS specific separator. The file system treats all file and directory
	names as case insensitive.

	The following cvars store paths used by the file system:

	"fs_basepath"		path to local install, read-only
	"fs_savepath"		path to config, save game, etc. files, read & write
	"fs_cdpath"			path to cd, read-only
	"fs_devpath"		path to files created during development, read & write

	The base path for file saving can be set to "fs_savepath" or "fs_devpath".

===============================================================================
*/

static const unsigned	FILE_NOT_FOUND_TIMESTAMP	= 0xFFFFFFFF;
static const int		MAX_PURE_PAKS				= 128;
// master server can keep server updated with a list of allowed paks per OS
static const int		MAX_GAMEPAK_PER_OS			= 10;
static const int		MAX_OSPATH					= 256;

// modes for OpenFileByMode. used as bit mask internally
typedef enum {
	FS_READ		= 0,
	FS_WRITE	= 1,
	FS_APPEND	= 2
} fsMode_t;

typedef enum {
	PURE_OK,		// we are good to connect as-is
	PURE_RESTART,	// restart required
	PURE_MISSING,	// pak files missing on the client
	PURE_NODLL,		// no DLL could be extracted
	//	PURE_DIFFERENT, // differing checksums
} fsPureReply_t;

typedef enum {
	DLTYPE_URL,
	DLTYPE_FILE
} dlType_t;

typedef enum {
	DL_WAIT,		// waiting in the list for beginning of the download
	DL_INPROGRESS,	// in progress
	DL_DONE,		// download completed, success
	DL_ABORTING,	// this one can be set during a download, it will force the next progress callback to abort - then will go to DL_FAILED
	DL_FAILED
} dlStatus_t;

typedef enum {
	FILE_EXEC,
	FILE_OPEN
} dlMime_t;

typedef enum {
	FIND_NO,
	FIND_YES,
	FIND_ADDON
} findFile_t;

typedef struct urlDownload_s {
	idStr				url;
	idStr				urlAuth;
	idStr				referer;
	char				dlerror[ MAX_STRING_CHARS ];
	int					dltotal;
	int					dlnow;
	int					dlstatus;
	dlStatus_t			status;
} urlDownload_t;

typedef struct fileDownload_s {
	int					position;
	int					length;
	void *				buffer;
} fileDownload_t;

typedef struct proxyDownload_s {
	idStr				proxyUrl;
	idStr				proxyAuth;
} proxyDownload_t;

// RAVEN BEGIN
// mwhitlock: changes for Xenon to enable us to use texture resources from .xpr
// bundles.
#if defined(_XENON)	
#include "Xtl.h"
#endif
// RAVEN END

// RAVEN BEGIN
// mwhitlock: changes for Xenon to enable us to use texture resources from .xpr
// bundles.
#if defined(_XENON)

// Define this to enable background streaming via Xenon's native file I/O
// routines (unbuffered reads).
#define _XENON_USE_NATIVE_FILEIO

typedef struct backgroundDownload_s
{
	static const int MAX_SEGMENTS = 2;

	struct backgroundDownload_s	*next;					// Set by the fileSystem.
	dlType_t					opcode;
	idFile *					f;
	int							numSegments;
	fileDownload_t				file[MAX_SEGMENTS];
	urlDownload_t				url;
	volatile bool				completed;
	double						ticksToLoad;			// Used for performance profiling.

public:
	void Clear(void)
	{
		numSegments=0;
		completed=false;
		f=0;
	}

	void AddSegment(int	position, int length, void* buffer)
	{
		assert(position>0);
		assert(length>0);
		assert(buffer!=0);
		assert(numSegments<MAX_SEGMENTS);
		if(numSegments<MAX_SEGMENTS)
		{
			file[numSegments].position=position;
			file[numSegments].length=length;
			file[numSegments].buffer=buffer;
			numSegments++;
		}
#if defined(_DEBUG)
		// Sanity: segements should not overlap.
		for(int s=0;s<numSegments-1;s++)
		{
			assert(file[s].position+file[s].length-1 < file[s+1].position);
		}
#endif
	}

	int DownloadSize(void) const
	{
		int size=0;
		for(int s=0;s<numSegments;s++)
		{
			size+=file[s].length;
		}
		return size;
	}
} backgroundDownload_t;

#else

typedef struct backgroundDownload_s {
	struct backgroundDownload_s	*next;	// set by the fileSystem
	dlType_t				opcode;
	idFile *				f;
	fileDownload_t			file;
	int						numSegments;
	urlDownload_t			url;
	proxyDownload_t			proxy;
	volatile bool			completed;
} backgroundDownload_t;

#endif
// RAVEN END

// file list for directory listings
class idFileList {
	friend class idFileSystemLocal;
public:
	const char *			GetBasePath( void ) const { return basePath; }
	int						GetNumFiles( void ) const { return list.Num(); }
	const char *			GetFile( int index ) const { return list[index]; }
	const idStrList &		GetList( void ) const { return list; }

private:
	idStr					basePath;
	idStrList				list;
};

// mod list
class idModList {
	friend class idFileSystemLocal;
public:
	int						GetNumMods( void ) const { return mods.Num(); }
	const char *			GetMod( int index ) const { return mods[index]; }
	const char *			GetDescription( int index ) const { return descriptions[index]; }

private:
	idStrList				mods;
	idStrList				descriptions;
};

class idFileSystem {
public:
	virtual					~idFileSystem() {}
							// Initializes the file system.
	virtual void			Init( void ) = 0;
							// Restarts the file system.
	virtual void			Restart( void ) = 0;
							// Shutdown the file system.
	virtual void			Shutdown( bool reloading ) = 0;
							// Returns true if the file system is initialized.
	virtual bool			IsInitialized( void ) const = 0;
							// Returns true if we are doing an fs_copyfiles.
	virtual bool			PerformingCopyFiles( void ) const = 0;
							// Returns a list of mods found along with descriptions
							// 'mods' contains the directory names to be passed to fs_game
							// 'descriptions' contains a free form string to be used in the UI
	virtual idModList *		ListMods( void ) = 0;
							// Frees the given mod list
	virtual void			FreeModList( idModList *modList ) = 0;
							// Lists files with the given extension in the given directory.
							// Directory should not have either a leading or trailing '/'
							// The returned files will not include any directories or '/' unless fullRelativePath is set.
							// The extension must include a leading dot and may not contain wildcards.
							// If extension is "/", only subdirectories will be returned.
	virtual idFileList *	ListFiles( const char *relativePath, const char *extension, bool sort = false, bool fullRelativePath = false, const char* gamedir = NULL ) = 0;
							// Lists files in the given directory and all subdirectories with the given extension.
							// Directory should not have either a leading or trailing '/'
							// The returned files include a full relative path.
							// The extension must include a leading dot and may not contain wildcards.
	virtual idFileList *	ListFilesTree( const char *relativePath, const char *extension, bool sort = false, const char* gamedir = NULL ) = 0;
							// Frees the given file list.
	virtual void			FreeFileList( idFileList *fileList ) = 0;
							// Converts a relative path to a full OS path.
	virtual const char *	OSPathToRelativePath( const char *OSPath ) = 0;
							// Converts a full OS path to a relative path.
	virtual const char *	RelativePathToOSPath( const char *relativePath, const char *basePath = "fs_devpath" ) = 0;
							// Builds a full OS path from the given components.
	virtual const char *	BuildOSPath( const char *base, const char *game, const char *relativePath ) = 0;
							// Creates the given OS path for as far as it doesn't exist already.
	virtual void			CreateOSPath( const char *OSPath ) = 0;
							// Returns true if a file is in a pak file.
	virtual bool			FileIsInPAK( const char *relativePath ) = 0;
							// Returns a space separated string containing the checksums of all referenced pak files.
							// will call SetPureServerChecksums internally to restrict itself
	virtual void			UpdatePureServerChecksums( void ) = 0;
							// setup the mapping of OS -> game pak checksum
	virtual bool			UpdateGamePakChecksums( void ) = 0;
							// 0-terminated list of pak checksums
							// if pureChecksums[ 0 ] == 0, all data sources will be allowed
							// otherwise, only pak files that match one of the checksums will be checked for files
							// with the sole exception of .cfg files.
							// the function tries to configure pure mode from the paks already referenced and this new list
							// it returns wether the switch was successfull, and sets the missing checksums
							// use fs_debug 1 to verbose
	virtual fsPureReply_t	SetPureServerChecksums( const int pureChecksums[ MAX_PURE_PAKS ], int gamePakChecksum[ MAX_GAMEPAK_PER_OS ], int missingChecksums[ MAX_PURE_PAKS ], int *missingGamePakChecksum, int *restartGamePakChecksu ) = 0;
							// fills a 0-terminated list of pak checksums for a client
							// if OS is -1, give the current game pak checksum. if >= 0, lookup the game pak table (server only)
							// indexes > 0 may be provided on servers when the master is giving additional game paks to let in
	virtual void			GetPureServerChecksums( int checksums[ MAX_PURE_PAKS ], int OS, int gamePakChecksum[ MAX_GAMEPAK_PER_OS ] ) = 0;
							// before doing a restart, force the pure list and the search order
							// if the given checksum list can't be completely processed and set, will error out
	virtual void			SetRestartChecksums( const int pureChecksums[ MAX_PURE_PAKS ], int gamePakChecksum ) = 0;
							// equivalent to calling SetPureServerChecksums with an empty list
	virtual	void			ClearPureChecksums( void ) = 0;
							// get a mask of supported OSes. if not pure, returns -1
	virtual unsigned int	GetOSMask( void ) = 0;
							// Reads a complete file.
							// Returns the length of the file, or -1 on failure.
							// A null buffer will just return the file length without loading.
							// A null timestamp will be ignored.
							// As a quick check for existance. -1 length == not present.
							// A 0 byte will always be appended at the end, so string ops are safe.
							// The buffer should be considered read-only, because it may be cached for other uses.
	virtual int				ReadFile( const char *relativePath, void **buffer, unsigned *timestamp = NULL ) = 0;
							// Frees the memory allocated by ReadFile.
	virtual void			FreeFile( void *buffer ) = 0;
							// Writes a complete file, will create any needed subdirectories.
							// Returns the length of the file, or -1 on failure.
	virtual int				WriteFile( const char *relativePath, const void *buffer, int size, const char *basePath = "fs_savepath" ) = 0;
							// Removes the given file.
// RAVEN BEGIN
// rjohnson: can specify the path cvar
	virtual void			RemoveFile( const char *relativePath, const char *basePath = "fs_savepath" ) = 0;
// bdube: added method
							// Removes the given file and returns the filesystem status of the removal
	virtual int				RemoveExplicitFile ( const char *OSPath ) = 0;

// mekberg: is file loading allowed?
	virtual void			SetIsFileLoadingAllowed(bool mode) = 0;

// dluetscher: returns file loading status
	virtual bool			GetIsFileLoadingAllowed() const = 0;

// amccarthy: set the current asset log name.
	virtual void			SetAssetLogName(const char *logName) = 0;
// amccarthy: write out a list of all files loaded.
	virtual void			WriteAssetLog() = 0;
// jnewquist: clear list of all files loaded.
	virtual void			ClearAssetLog() = 0;
// jnewquist: Accessor for asset log name (with filter)
	virtual const char*		GetAssetLogName() = 0;

// jscott: new functions for tools
	virtual idFile *		GetNewFileMemory( void ) = 0;
	virtual idFile *		GetNewFilePermanent( void ) = 0;

// RAVEN END
							// Opens a file for reading.
	virtual idFile *		OpenFileRead( const char *relativePath, bool allowCopyFiles = true, const char* gamedir = NULL ) = 0;
							// Opens a file for writing, will create any needed subdirectories.
	virtual idFile *		OpenFileWrite( const char *relativePath, const char *basePath = "fs_savepath", bool ASCII = false ) = 0;
							// Opens a file for writing at the end.
	virtual idFile *		OpenFileAppend( const char *filename, bool sync = false, const char *basePath = "fs_basepath" ) = 0;
							// Opens a file for reading, writing, or appending depending on the value of mode.
	virtual idFile *		OpenFileByMode( const char *relativePath, fsMode_t mode ) = 0;
							// Opens a file for reading from a full OS path.
	virtual idFile *		OpenExplicitFileRead( const char *OSPath ) = 0;
							// Opens a file for writing to a full OS path.
	virtual idFile *		OpenExplicitFileWrite( const char *OSPath ) = 0;
							// Closes a file.
	virtual void			CloseFile( idFile *f ) = 0;
							// Returns immediately, performing the read from a background thread.
	virtual void			BackgroundDownload( backgroundDownload_t *bgl ) = 0;
							// resets the bytes read counter
	virtual void			ResetReadCount( void ) = 0;
							// retrieves the current read count
	virtual int				GetReadCount( void ) = 0;
							// adds to the read count
	virtual void			AddToReadCount( int c ) = 0;
							// look for a dynamic module
	virtual void			FindDLL( const char *basename, char dllPath[ MAX_OSPATH ], bool updateChecksum ) = 0;
							// case sensitive filesystems use an internal directory cache
							// the cache is cleared when calling OpenFileWrite and RemoveFile
							// in some cases you may need to use this directly
	virtual void			ClearDirCache( void ) = 0;

							// lookup a relative path, return the size or 0 if not found
	virtual int				RelativeDownloadPathForChecksum( int checksum, char path[ MAX_STRING_CHARS ] ) = 0;

							// verify the file can be downloaded, lookup a relative path, return the size or 0 if not found
	virtual int				ValidateDownloadPakForChecksum( int checksum, char path[ MAX_STRING_CHARS ], bool isGamePak ) = 0;

							// verify the file can be downloaded, lookup an absolute (OS) path, return the size or 0 if not found
	virtual int				ValidateDownloadPakForRelativePath( const char *relativePath, char path[ MAX_STRING_CHARS ], bool &isGamePakReturn ) = 0;

	virtual idFile *		MakeTemporaryFile( void ) = 0;

							// make downloaded pak files known so pure negociation works next time
	virtual int				AddZipFile( const char *path ) = 0;

// RAVEN BEGIN
// nrausch: explicit pak add/removal
#ifdef _XENON	
	virtual bool			AddDownloadedPak( const char *path ) = 0;
	virtual void			RemoveDownloadedPak( const char *path ) = 0;

	virtual bool			AddExplicitPak( const char *path ) = 0;
	virtual void			RemoveExplicitPak( const char *path ) = 0;
	virtual bool			IsPakLoaded( const char *path ) = 0;
#endif
// RAVEN END

							// look for a file in the loaded paks or the addon paks
							// if the file is found in addons, FS's internal structures are ready for a reloadEngine
	virtual findFile_t		FindFile( const char *path ) = 0;

							// get map/addon decls and take into account addon paks that are not on the search list
							// the decl 'name' is in the "path" entry of the dict
	virtual int				GetNumMaps() = 0;
	virtual int				GetMapDeclIndex( const char *mapName ) = 0;
	virtual const idDict *	GetMapDecl( int i ) = 0;
	virtual const idDict *	GetMapDecl( const char *mapName ) = 0;
	virtual void			FindMapScreenshot( const char *path, char *buf, int len ) = 0;

// RAVEN BEGIN
// rjohnson: added new functions
							// Converts a full OS path to an import path.
	virtual bool			OSpathToImportPath( const char *osPath, idStr &iPath, bool stripTemp = false ) = 0;
							// Opens a file for reading from the fs_importpath directory
	virtual idFile *		OpenImportFileRead( const char *filename ) = 0;
							// Copy a file 
	virtual void			CopyOSFile( const char *fromOSPath, const char *toOSPath ) = 0;
	virtual void			CopyOSFile( idFile *src, const char *toOSPath ) = 0;
// RAVEN END

	// demo functions - only for use by the core ( and not DLL boundary / memory safe anyway )
	// ReadDemoHeader: returns -1 if a reloadEngine is requested before trying again, 0 if we failed with no backup, and 1 if we can continue
	// if demo_enforceFS is 0, will always ret 1
	virtual void			WriteDemoHeader( idFile *file ) = 0;
	virtual int				ReadDemoHeader( idFile *file ) = 0;

	// new in 1.3
	// indicates if the filesystem is currently running with pak files restrictions or addons
	virtual bool			IsRunningWithRestrictions( void ) = 0;

	// new in 1.4
	virtual void			ReadCodePakLists( const idBitMsg &msg ) = 0;
	virtual bool			HaveCodePakLists( void ) const = 0;

	// pick best language - used by the core to pick a good default language based on present zpaks
	virtual void			SelectDefaultLanguage( void ) = 0;

	virtual void			ClearAddonList( void ) = 0;
};

extern idFileSystem *		fileSystem;

#endif /* !__FILESYSTEM_H__ */
