/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

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
	"fs_devpath"		path to files created during development, read & write

	The base path for file saving can be set to "fs_savepath" or "fs_devpath".

===============================================================================
*/

static const ID_TIME_T	FILE_NOT_FOUND_TIMESTAMP	= -1;   //0xFFFFFFFF;  stgatilov: in x64 mode it is not -1
static const int		MAX_PURE_PAKS				= 128;
static const int		MAX_OSPATH					= 256;

// modes for OpenFileByMode. used as bit mask internally
typedef enum {
	FS_READ		= 0,
	FS_WRITE	= 1,
	FS_APPEND	= 2
} fsMode_t;

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

typedef struct backgroundDownload_s {
	struct backgroundDownload_s	*next;	// set by the fileSystem
	dlType_t			opcode;
	idFile *			f;
	fileDownload_t		file;
	urlDownload_t		url;
	volatile bool		completed;
} backgroundDownload_t;

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
							// Returns a list of mods found along with descriptions
							// 'mods' contains the directory names to be passed to fs_mod
							// 'descriptions' contains a free form string to be used in the UI
	virtual idModList *		ListMods( void ) = 0;
							// Frees the given mod list
	virtual void			FreeModList( idModList *modList ) = 0;
							// Lists files with the given extension in the given directory.
							// Directory should not have either a leading or trailing '/'
							// The returned files will not include any directories or '/' unless fullRelativePath is set.
							// The extension must include a leading dot and may not contain wildcards.
							// If extension is "/", only subdirectories will be returned.
	virtual idFileList *	ListFiles( const char *relativePath, const char *extension, bool sort = false, bool fullRelativePath = false, const char* gamedir = NULL ) const = 0;
							// Lists files in the given directory and all subdirectories with the given extension.
							// Directory should not have either a leading or trailing '/'
							// The returned files include a full relative path.
							// The extension must include a leading dot and may not contain wildcards.
	virtual idFileList *	ListFilesTree( const char *relativePath, const char *extension, bool sort = false, const char* gamedir = NULL ) const = 0;
							// Frees the given file list.
	virtual void			FreeFileList( idFileList *fileList ) const = 0;
							// Converts a relative path to a full OS path.
	virtual const char *	OSPathToRelativePath( const char *OSPath ) = 0;
							// Converts a full OS path to a relative path.
	virtual const char *	RelativePathToOSPath( const char *relativePath, const char *basePath = "fs_devpath", const char *gamedir = NULL ) = 0;
							// Builds a full OS path from the given components.
	virtual const char *	BuildOSPath( const char *base, const char *game, const char *relativePath ) const = 0;
							// Creates the given OS path for as far as it doesn't exist already.
	virtual void			CreateOSPath( const char *OSPath ) = 0;
							// Returns true if a file is in a pak file.
	virtual bool			FileIsInPAK( const char *relativePath ) const = 0;
							// Returns a space separated string containing the checksums of all referenced pak files.
	virtual int				GetOSMask( void ) = 0;
							// Reads a complete file.
							// Returns the length of the file, or -1 on failure.
							// A null buffer will just return the file length without loading.
							// A null timestamp will be ignored.
							// As a quick check for existance. -1 length == not present.
							// A 0 byte will always be appended at the end, so string ops are safe.
							// The buffer should be considered read-only, because it may be cached for other uses.
	virtual int				ReadFile( const char *relativePath, void **buffer, ID_TIME_T *timestamp = NULL ) = 0;
							// Frees the memory allocated by ReadFile.
	virtual void			FreeFile( void *buffer ) = 0;
							// Writes a complete file, will create any needed subdirectories.
							// Returns the length of the file, or -1 on failure. 
							// greebo: By default use the mod save path to write stuff
	virtual int				WriteFile( const char *relativePath, const void *buffer, int size, const char *basePath = "fs_modSavePath", const char *gamedir = NULL ) = 0;
							// Removes the given file.
	virtual void			RemoveFile( const char *relativePath, const char *gamedir = NULL ) = 0;
							// Opens a file for reading.
    virtual idFile *		OpenFileRead( const char *relativePath, const char* gamedir = NULL ) = 0;
							// Prefetches the entire file to memory and returns an in-memory file object
	virtual idFile *		OpenFileReadPrefetch( const char *relativePath, const char *gamedir = NULL ) = 0;
							// Opens a file for writing, will create any needed subdirectories.
	virtual idFile *		OpenFileWrite( const char *relativePath, const char *basePath = "fs_modSavePath", const char *gamedir = NULL ) = 0;
							// Opens a file for writing at the end.
	virtual idFile *		OpenFileAppend( const char *filename, bool sync = false, const char *basePath = "fs_modSavePath", const char *gamedir = NULL ) = 0;
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
	virtual int				GetReadCount( void ) const = 0;
							// adds to the read count
	virtual void			AddToReadCount( int c ) = 0;
							// look for a dynamic module
	virtual void			FindDLL( const char *basename, char dllPath[ MAX_OSPATH ], bool updateChecksum ) = 0;
							// case sensitive filesystems use an internal directory cache
							// the cache is cleared when calling OpenFileWrite and RemoveFile
							// in some cases you may need to use this directly
	virtual void			ClearDirCache( void ) const = 0;

							// don't use for large copies - allocates a single memory block for the copy
	virtual bool			CopyFile( const char *fromOSPath, const char *toOSPath ) = 0;

							// lookup a relative path, return the size or 0 if not found
	virtual int				ValidateDownloadPakForChecksum( int checksum, char path[ MAX_STRING_CHARS ], bool isGamePak ) = 0;

	virtual idFile *		MakeTemporaryFile( void ) = 0;

							// make downloaded pak files known
	virtual int				AddZipFile( const char *path ) = 0;

							// look for a file in the loaded paks or the addon paks
							// if the file is found in addons, FS's internal structures are ready for a reloadEngine
	virtual findFile_t		FindFile( const char *path, bool scheduleAddons = false ) = 0;

							// get map/addon decls and take into account addon paks that are not on the search list
							// the decl 'name' is in the "path" entry of the dict
	virtual int				GetNumMaps() = 0;
	virtual const idDict *	GetMapDecl( int i ) = 0;
	virtual void			FindMapScreenshot( const char *path, char *buf, int len ) = 0;

							// ignore case and seperator char distinctions
	virtual bool			FilenameCompare( const char *s1, const char *s2 ) const = 0;

	// greebo: Returns the full path to the darkmod/ folder or custom_mod/ if set.
	virtual const char*		ModPath() const = 0;
};

extern idFileSystem *		fileSystem;

#endif /* !__FILESYSTEM_H__ */
