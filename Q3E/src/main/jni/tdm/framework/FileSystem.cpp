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

#include "precompiled.h"
#pragma hdrstop



#include <minizip/unzip.h>
#include "minizip/minizip_extra.h"	//unzReOpen
//stgatilov: for pk4 repacking
#include <minizip/zip.h>

#ifdef WIN32
	#include <io.h>	// for _read
    #include <sys/utime.h> // for _utime
#else
	#if !__MACH__ && __MWERKS__
		#include <types.h>
		#include <stat.h>
	#else
		#include <sys/types.h>
		#include <sys/stat.h>
	#endif
	#include <unistd.h>
    #include <utime.h> // for utime
#endif

#if ID_ENABLE_CURL
	#include "curl/curl.h"
	#define ExtLibs
#endif

/*
=============================================================================

DOOM FILESYSTEM

All of Doom's data access is through a hierarchical file system, but the contents of 
the file system can be transparently merged from several sources.

A "relativePath" is a reference to game file data, which must include a terminating zero.
"..", "\\", and ":" are explicitly illegal in qpaths to prevent any references
outside the Doom directory system.

The "base path" is the path to the directory holding all the game directories and
usually the executable. It defaults to the current directory, but can be overridden
with "+set fs_basepath c:\doom" on the command line. The base path cannot be modified
at all after startup.

The "save path" is the path to the directory where game files will be saved. It defaults
to the base path, but can be overridden with a "+set fs_savepath c:\doom" on the
command line. Any files that are created during the game (demos, screenshots, etc.) will
be created reletive to the save path.

The "dev path" is the path to an alternate hierarchy where the editors and tools used
during development (Radiant, AF editor, dmap, runAAS) will write files to. It defaults to
the cd path, but can be overridden with a "+set fs_devpath c:\doom" on the command line.

If a user runs the game directly from a CD, the base path would be on the CD. This
should still function correctly, but all file writes will fail (harmlessly).

The "base game" is the directory under the paths where data comes from by default "base".

The "current game" may be the same as the base game, or it may be the name of another
directory under the paths that should be searched for files before looking in the base
game. The game directory is set with "+set fs_mod myaddon" on the command line. This is
the basis for addons.

No other directories outside of the base game and current game will ever be referenced by
filesystem functions.

To save disk space and speed up file loading, directory trees can be collapsed into zip
files. The files use a ".pk4" extension to prevent users from unzipping them accidentally,
but otherwise they are simply normal zip files. A game directory can have multiple zip
files of the form "pak0.pk4", "pak1.pk4", etc. Zip files are searched in decending order
from the highest number to the lowest, and will always take precedence over the filesystem.
This allows a pk4 distributed as a patch to override all existing data.

The relative path "sound/newstuff/test.wav" would be searched for in the following places:

for save path, dev path, base path, cd path:
	for current game, base game:
		search directory
		search zip files

downloaded files, to be written to save path + current game's directory

The filesystem can be safely shutdown and reinitialized with different
basedir / cddir / game combinations, but all other subsystems that rely on it
(sound, video) must also be forced to restart.

"fs_caseSensitiveOS":
This cvar is set on operating systems that use case sensitive filesystems (Linux and OSX)
It is a common situation to have the media reference filenames, whereas the file on disc 
only matches in a case-insensitive way. When "fs_caseSensitiveOS" is set, the filesystem
will always do a case insensitive search.
IMPORTANT: This only applies to files, and not to directories. There is no case-insensitive
matching of directories. All directory names should be lowercase, when "com_developer" is 1,
the filesystem will warn when it catches bad directory situations (regardless of the
"fs_caseSensitiveOS" setting)
When bad casing in directories happen and "fs_caseSensitiveOS" is set, BuildOSPath will
attempt to correct the situation by forcing the path to lowercase. This assumes the media
is stored all lowercase.

"additional mod path search":
fs_mod can be used to set an additional search path
in search order, fs_mod > BASE_TDM

=============================================================================
*/

#define MAX_ZIPPED_FILE_NAME	2048
#define FILE_HASH_SIZE			1024

typedef struct fileInPack_s {
	idStr				name;						// name of the file
	ZPOS64_T			pos;						// file info position in zip
	struct fileInPack_s * next;						// next file in the hash
} fileInPack_t;

typedef enum {
	BINARY_UNKNOWN = 0,
	BINARY_YES,
	BINARY_NO
} binaryStatus_t;

typedef struct {
	idList<int>			depends;
	idList<idDict *>	mapDecls;
} addonInfo_t;

typedef struct {
	idStr				pakFilename;				// c:\doom\base\pak0.pk4
	unzFile				handle;
	int					checksum;
	int					numfiles;
	int					length;
	bool				referenced;
	binaryStatus_t		binary;
	bool				addon;						// this is an addon pack - addon_search tells if it's 'active'
	bool				addon_search;				// is in the search list
	addonInfo_t			*addon_info;
	bool				isNew;						// for downloaded paks
	fileInPack_t		*hashTable[FILE_HASH_SIZE];
	fileInPack_t		*buildBuffer;
} pack_t;

typedef struct {
	idStr				path;						// c:\doom
	idStr				gamedir;					// base
} directory_t;

typedef struct searchpath_s {
	pack_t *			pack;						// only one of pack / dir will be non NULL
	directory_t *		dir;
	domainStatus_t		domain;						// stgatilov #5766: TDM core / FM
	struct searchpath_s *next;
} searchpath_t;

// search flags when opening a file
#define FSFLAG_SEARCH_DIRS		( 1 << 0 )
#define FSFLAG_SEARCH_PAKS		( 1 << 1 )
#define FSFLAG_BINARY_ONLY		( 1 << 2 )
#define FSFLAG_SEARCH_ADDONS	( 1 << 3 )

// 2 search path (fs_savepath fs_basepath)
// + .jpg and .tga
#define MAX_CACHED_DIRS 6

// how many OSes to handle game paks for ( we don't have to know them precisely )
#define MAX_GAME_OS	6
#define BINARY_CONFIG "binary.conf"
#define ADDON_CONFIG "addon.conf"

class idDEntry : public idStrList {
public:
						idDEntry() {}
	virtual				~idDEntry() {}

	bool				Matches( const char *directory, const char *extension ) const;
	void				Init( const char *directory, const char *extension, const idStrList &list );
	void				Clear( void );

private:
	idStr				directory;
	idStr				extension;
};

class idFileSystemLocal : public idFileSystem {
public:
							idFileSystemLocal( void );

	virtual void			Init( void ) override;
	void					StartBackgroundDownloadThread( void );
	virtual void			Restart( void ) override;
	virtual void			Shutdown( bool reloading ) override;
	virtual bool			IsInitialized( void ) const override;
	virtual idModList *		ListMods( void ) override;
	virtual void			FreeModList( idModList *modList ) override;
	virtual idFileList *	ListFiles( const char *relativePath, const char *extension, bool sort = false, bool fullRelativePath = false, const char* gamedir = NULL ) const override;
	virtual idFileList *	ListFilesTree( const char *relativePath, const char *extension, bool sort = false, const char* gamedir = NULL ) const override;
	virtual void			FreeFileList( idFileList *fileList ) const override;
	virtual const char *	OSPathToRelativePath( const char *OSPath ) override;
	virtual const char *	RelativePathToOSPath( const char *relativePath, const char *basePath, const char *gamedir = NULL ) override;
	virtual const char *	BuildOSPath( const char *base, const char *game, const char *relativePath ) const override;
	virtual void			CreateOSPath( const char *OSPath ) override;
	virtual bool			FileIsInPAK( const char *relativePath ) const override;
	bool					UpdateGamePakChecksums( void );
	virtual int				GetOSMask( void ) override;
	virtual int				ReadFile( const char *relativePath, void **buffer, ID_TIME_T *timestamp ) override;
	virtual void			FreeFile( void *buffer ) override;
	virtual int				WriteFile( const char *relativePath, const void *buffer, int size, const char *basePath = "fs_modSavePath", const char *gamedir = NULL ) override;
	virtual void			RemoveFile( const char *relativePath, const char *gamedir = NULL) override;
    idFile *				OpenFileReadFlags( const char *relativePath, int searchFlags, pack_t **foundInPak = NULL, const char* gamedir = NULL );	//Note: thread-unsafe!
    virtual idFile *		OpenFileRead( const char *relativePath, const char* gamedir = NULL ) override;
    virtual idFile *		OpenFileReadPrefetch( const char *relativePath, const char* gamedir = NULL ) override;
	virtual idFile *		OpenFileWrite( const char *relativePath, const char *basePath = "fs_modSavePath", const char *gamedir = NULL ) override;
	virtual idFile *		OpenFileAppend( const char *relativePath, bool sync = false, const char *basePath = "fs_modSavePath", const char *gamedir = NULL ) override;
	virtual idFile *		OpenFileByMode( const char *relativePath, fsMode_t mode ) override;
	virtual idFile *		OpenExplicitFileRead( const char *OSPath ) override;
	virtual idFile *		OpenExplicitFileWrite( const char *OSPath ) override;
	virtual void			CloseFile( idFile *f ) override;
	virtual void			BackgroundDownload( backgroundDownload_t *bgl ) override;
	virtual void			ResetReadCount( void ) override { readCount.SetValue(0); }
	virtual void			AddToReadCount( int c ) override { readCount.Add(c); }
	virtual int				GetReadCount( void ) const override { return readCount.GetValue(); }
	virtual void			FindDLL( const char *basename, char dllPath[ MAX_OSPATH ], bool updateChecksum ) override;
	virtual void			ClearDirCache( void ) const override;
	virtual bool			CopyFile( const char *fromOSPath, const char *toOSPath ) override;
	virtual int				ValidateDownloadPakForChecksum( int checksum, char path[ MAX_STRING_CHARS ], bool isBinary ) override;
	virtual idFile *		MakeTemporaryFile( void ) override;
	virtual int				AddZipFile( const char *path ) override;
	virtual findFile_t		FindFile( const char *path, bool scheduleAddons ) override;
	virtual int				GetNumMaps() override;
	virtual const idDict *	GetMapDecl( int i ) override;
	virtual void			FindMapScreenshot( const char *path, char *buf, int len ) override;
	virtual bool			FilenameCompare( const char *s1, const char *s2 ) const override;

	virtual const char*		ModPath() const override;

	static void				Dir_f( const idCmdArgs &args );
	static void				DirTree_f( const idCmdArgs &args );
	static void				Path_f( const idCmdArgs &args );
	static void				TouchFile_f( const idCmdArgs &args );
	static void				TouchFileList_f( const idCmdArgs &args );
	static void				TestThreads_f( const idCmdArgs &args );

private:
    friend void				BackgroundDownloadThread(void *parms);

	mutable idSysMutex		globalMutex;		// locked on most filesystem operations

	searchpath_t *			searchPaths;
	idSysInterlockedInteger	readCount;			// total bytes read
	idSysInterlockedInteger	loadCount;			// total files read
	idSysInterlockedInteger	loadStack;			// total files in memory
	idStr					gameFolder;			// this will be a single name without separators

	searchpath_t			*addonPaks;			// not loaded up, but we saw them

	idDict					mapDict;			// for GetMapDecl

	static idCVar			fs_debug;
	static idCVar			fs_basepath;
	static idCVar			fs_savepath;
	static idCVar			fs_devpath;
	static idCVar			fs_caseSensitiveOS;
	static idCVar			fs_searchAddons;

    // taaaki: fs_game and fs_game_base have been removed as TDM is no longer a mod and these fs cvars were causing
    // confusion due to inconsistent usage. fs_mod has been added to allow for mods of TDM.
    static idCVar           fs_mod;
    static idCVar           fs_currentfm;

	// greebo: For regular TDM missions, all savegames/demos/etc. should be written to darkmod/fms/<fs_currentfm>/... 
	// instead of creating a folder in fs_basePath. So, use "fs_modSavePath" as argument to filesystem->OpenFileWrite()
    // The value of modSavePath is something like C:\Games\TDM\darkmod\fms\ in Win32, or /home/user/games/tdm/darkmod/fms/ in Linux.
	static idCVar			fs_modSavePath;

	backgroundDownload_t *	backgroundDownloads;
	backgroundDownload_t	defaultBackgroundDownload;
	uintptr_t				backgroundThread;

	idList<pack_t *>		serverPaks;
	bool					loadedFileFromDir;		// set to true once a file was loaded from a directory
	idList<int>				restartChecksums;		// used during a restart to set things in right order
	idList<int>				addonChecksums;			// list of checksums that should go to the search list directly ( for restarts )
	int						restartGamePakChecksum;
	int						gameDLLChecksum;		// the checksum of the last loaded game DLL
	int						gamePakChecksum;		// the checksum of the pak holding the loaded game DLL

	int						gamePakForOS[ MAX_GAME_OS ];

	mutable idSysMutex		dir_cache_mutex;
	mutable idDEntry		dir_cache[ MAX_CACHED_DIRS ]; // fifo
	mutable int				dir_cache_index;
	mutable int				dir_cache_count;

private:
	static void				ReplaceSeparators( idStr &path, char sep = PATHSEPERATOR_CHAR );
    static int 				HashFileName(const char *fname);
	int						ListOSFiles( const char *directory, const char *extension, idStrList &list ) const;
	FILE *					OpenOSFile( const char *name, const char *mode, idStr *caseSensitiveName = NULL ) const;
	FILE *					OpenOSFileCorrectName( idStr &path, const char *mode ) const;
	static int				DirectFileLength( FILE *o );
	bool					CopyFile( idFile *src, const char *toOSPath );
	static int				AddUnique( const char *name, idStrList &list, idHashIndex &hashIndex );
	static void				GetExtensionList( const char *extension, idStrList &extensionList );
	int						GetFileList( const char *relativePath, const idStrList &extensions, idStrList &list, idHashIndex &hashIndex, bool fullRelativePath, const char* gamedir = NULL ) const; //note: thread-unsafe!

	int						GetFileListTree( const char *relativePath, const idStrList &extensions, idStrList &list, idHashIndex &hashIndex, const char* gamedir = NULL ) const; //note: thread-unsafe!
	pack_t *				LoadZipFile( const char *zipfile ); //note: thread-unsafe!
	void					AddGameDirectory( const char *path, const char *dir, domainStatus_t domain ); //note: thread-unsafe!
	void					SetupGameDirectories( const char *gameName ); //note: thread-unsafe!
	void					Startup( void ); //note: thread-unsafe!

	static bool				FileAllowedFromDir( const char *path );
							// searches all the paks
	pack_t *				GetPackForChecksum( int checksum, bool searchAddons = false ); //note: thread-unsafe!
							// searches all the paks
	pack_t *				FindPakForFileChecksum( const char *relativePath, int fileChecksum, bool bReference ); //note: thread-unsafe!
	idFile_InZip *			ReadFileFromZip( pack_t *pak, fileInPack_t *pakFile, const char *relativePath ); //note: thread-unsafe!
	static int				GetFileChecksum( idFile *file );
	static addonInfo_t *	ParseAddonDef( const char *buf, const int len );
	void					FollowAddonDependencies( pack_t *pak );

	static size_t			CurlWriteFunction( void *ptr, size_t size, size_t nmemb, void *stream );
							// curl_progress_callback in curl.h
	static int				CurlProgressFunction( void *clientp, double dltotal, double dlnow, double ultotal, double ulnow );
};

idCVar	idFileSystemLocal::fs_debug( "fs_debug", "0", CVAR_SYSTEM | CVAR_INTEGER, "", 0, 2, idCmdSystem::ArgCompletion_Integer<0,2> );
idCVar	idFileSystemLocal::fs_basepath( "fs_basepath", "", CVAR_SYSTEM | CVAR_INIT, "" );
idCVar	idFileSystemLocal::fs_savepath( "fs_savepath", "", CVAR_SYSTEM | CVAR_INIT, "" );
idCVar	idFileSystemLocal::fs_devpath( "fs_devpath", "", CVAR_SYSTEM | CVAR_INIT, "" );

// taaaki: Removed fs_game and fs_game_base and replaced with fs_mod and fs_currentfm
idCVar  idFileSystemLocal::fs_mod( "fs_mod", "darkmod", CVAR_SYSTEM | CVAR_INIT | CVAR_SERVERINFO, "Path to custom TDM mod." );
idCVar  idFileSystemLocal::fs_currentfm( "fs_currentfm", "", CVAR_SYSTEM | CVAR_INIT | CVAR_SERVERINFO, "The currently installed fan mission." );
#ifdef WIN32
idCVar	idFileSystemLocal::fs_caseSensitiveOS( "fs_caseSensitiveOS", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
#else
idCVar	idFileSystemLocal::fs_caseSensitiveOS( "fs_caseSensitiveOS", "1", CVAR_SYSTEM | CVAR_BOOL, "" );
#endif
idCVar	idFileSystemLocal::fs_searchAddons( "fs_searchAddons", "0", CVAR_SYSTEM | CVAR_BOOL, "search all addon pk4s ( disables addon functionality )" );

// greebo: Custom savepath in darkmod/fms/
idCVar	idFileSystemLocal::fs_modSavePath( "fs_modSavePath", "", CVAR_SYSTEM | CVAR_INIT, "This is where all screenshots and savegames will be written to." );

idFileSystemLocal	fileSystemLocal;
idFileSystem *		fileSystem = &fileSystemLocal;

/*
================
idFileSystemLocal::idFileSystemLocal
================
*/
idFileSystemLocal::idFileSystemLocal( void ) {
	searchPaths = NULL;
	readCount.SetValue(0);
	loadCount.SetValue(0);
	loadStack.SetValue(0);
	dir_cache_index = 0;
	dir_cache_count = 0;
	loadedFileFromDir = false;
	restartGamePakChecksum = 0;
	memset( &backgroundThread, 0, sizeof( backgroundThread ) );
	addonPaks = NULL;
}

/*
================
idFileSystemLocal::HashFileName

return a hash value for the filename
================
*/
int idFileSystemLocal::HashFileName( const char *fname ) {
	int		i;
	int	hash;
	char	letter;

	hash = 0;
	i = 0;
	while( fname[i] != '\0' ) {
		letter = idStr::ToLower( fname[i] );
		if ( letter == '.' ) {
			break;				// don't include extension
		}
		if ( letter == '\\' ) {
			letter = '/';		// damn path names
		}
		hash += (int)(letter * (i+119));
		i++;
	}
	hash &= (FILE_HASH_SIZE-1);
	return hash;
}

/*
===========
idFileSystemLocal::FilenameCompare

Ignore case and separator char distinctions

RETURNS false WHEN EQUAL THIS IS SO STUPID IT HURTS!
===========
*/
bool idFileSystemLocal::FilenameCompare( const char *s1, const char *s2 ) const {
	int		c1, c2;
	
	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( c1 >= 'a' && c1 <= 'z' ) {
			c1 -= ('a' - 'A');
		}
		if ( c2 >= 'a' && c2 <= 'z' ) {
			c2 -= ('a' - 'A');
		}

		if ( c1 == '\\' || c1 == ':' ) {
			c1 = '/';
		}
		if ( c2 == '\\' || c2 == ':' ) {
			c2 = '/';
		}
		
		if ( c1 != c2 ) {
			return true;		// strings not equal
		}
	} while( c1 );
	
	return false;		// strings are equal
}

/*
================
idFileSystemLocal::OpenOSFile
optional caseSensitiveName is set to case sensitive file name as found on disc (fs_caseSensitiveOS only)
================
*/
FILE *idFileSystemLocal::OpenOSFile( const char *fileName, const char *mode, idStr *caseSensitiveName ) const {
	//supposedly, no lock required (fopen and ListOSFiles should be relatively threadsafe)
	FILE *fp;
	idStr fpath, entry;
	idStrList list;

#ifndef __MWERKS__
#ifndef WIN32 
	// some systems will let you fopen a directory
	struct stat buf;
	if ( stat( fileName, &buf ) != -1 && !S_ISREG(buf.st_mode) ) {
		return NULL;
	}
#endif
#endif
	fp = fopen( fileName, mode );
	if ( !fp && fs_caseSensitiveOS.GetBool() ) {
		fpath = fileName;
		fpath.StripFilename();
		fpath.StripTrailing( PATHSEPERATOR_CHAR );
		if ( ListOSFiles( fpath, NULL, list ) == -1 ) {
			return NULL;
		}
		
		for ( int i = 0; i < list.Num(); i++ ) {
			entry = fpath + PATHSEPERATOR_CHAR + list[i];
			if ( !entry.Icmp( fileName ) ) {
				fp = fopen( entry, mode );
				if ( fp ) {
					if ( caseSensitiveName ) {
						*caseSensitiveName = entry;
						caseSensitiveName->StripPath();
					}
					if ( fs_debug.GetInteger() ) {
						common->Printf( "idFileSystemLocal::OpenFileRead: changed %s to %s\n", fileName, entry.c_str() );
					}
					break;
				} else {
					// not supposed to happen if ListOSFiles is doing it's job correctly
					common->Warning( "idFileSystemLocal::OpenFileRead: fs_caseSensitiveOS 1 could not open %s", entry.c_str() );
				}
			}
		}
	} else if ( caseSensitiveName ) {
		*caseSensitiveName = fileName;
		caseSensitiveName->StripPath();
	}
	return fp;
}

/*
================
idFileSystemLocal::OpenOSFileCorrectName
================
*/
FILE *idFileSystemLocal::OpenOSFileCorrectName( idStr &path, const char *mode ) const {
	idStr caseName;
	FILE *f = OpenOSFile( path.c_str(), mode, &caseName );
	if ( f ) {
		path.StripFilename();
		path += PATHSEPERATOR_STR;
		path += caseName;
	}
	return f;
}

/*
================
idFileSystemLocal::DirectFileLength
================
*/
int idFileSystemLocal::DirectFileLength( FILE *o ) {
	const int pos = ftell( o );
	fseek( o, 0, SEEK_END );
	const int end = ftell( o );
	fseek( o, pos, SEEK_SET );
	return end;
}

/*
============
idFileSystemLocal::CreateOSPath

Creates any directories needed to store the given filename
============
*/
void idFileSystemLocal::CreateOSPath( const char *OSPath ) {
	char	*ofs;
	
	// make absolutely sure that it can't back up the path
	// FIXME: what about c: ?
	// duzenko: allow .. in the middle
	if (strstr( OSPath, ".." ) == OSPath || strstr( OSPath, "::" ) ) {
#ifdef _DEBUG		
		common->DPrintf( "refusing to create relative path \"%s\"\n", OSPath );
#endif
		return;
	}

	idStr path( OSPath );
	for( ofs = &path[ 1 ]; *ofs ; ofs++ ) {
		if ( *ofs == PATHSEPERATOR_CHAR ) {	
			// create the directory
			*ofs = 0;
			Sys_Mkdir( path );
			*ofs = PATHSEPERATOR_CHAR;
		}
	}
}

/*
=================
idFileSystemLocal::CopyFile

Copy a fully specified file from one place to another
=================
*/
bool idFileSystemLocal::CopyFile( const char *fromOSPath, const char *toOSPath ) {
	//supposedly, no lock required (also, this might be a time-consuming method)
	FILE	*f;
	byte	*buf;

	common->Printf( "copy %s to %s\n", fromOSPath, toOSPath );
	f = OpenOSFile( fromOSPath, "rb" );
	if ( !f ) {
		return false;
	}
	fseek( f, 0, SEEK_END );
	const int len = ftell( f );
	fseek( f, 0, SEEK_SET );

	buf = (byte *)Mem_Alloc( len );
	if ( fread( buf, 1, len, f ) != (unsigned int)len ) {
		common->FatalError( "short read in idFileSystemLocal::CopyFile()\n" );
	}
	fclose( f );

	CreateOSPath( toOSPath );
	f = OpenOSFile( toOSPath, "wb" );
	if ( !f ) {
		common->Printf( "could not create destination file\n" );
		Mem_Free( buf );
		return false;
	}
	if ( fwrite( buf, 1, len, f ) != (unsigned int)len ) {
		common->FatalError( "short write in idFileSystemLocal::CopyFile()\n" );
	}
	fclose( f );
	Mem_Free( buf );
	return true;
}

/*
=================
idFileSystemLocal::CopyFile
=================
*/
bool idFileSystemLocal::CopyFile( idFile *src, const char *toOSPath ) {
	FILE	*f;
	byte	*buf;

	common->Printf( "copy %s to %s\n", src->GetName(), toOSPath );
	src->Seek( 0, FS_SEEK_END );
	const int len = src->Tell();
	src->Seek( 0, FS_SEEK_SET );

	buf = (byte *)Mem_Alloc( len );
	if ( src->Read( buf, len ) != len ) {
		common->FatalError( "Short read in idFileSystemLocal::CopyFile()\n" );
	}

	CreateOSPath( toOSPath );
	f = OpenOSFile( toOSPath, "wb" );
	if ( !f ) {
		common->Printf( "could not create destination file\n" );
		Mem_Free( buf );
		return false;
	}
	if ( fwrite( buf, 1, len, f ) != (unsigned int)len ) {
		common->FatalError( "Short write in idFileSystemLocal::CopyFile()\n" );
	}
	fclose( f );
	Mem_Free( buf );
	return true;
}

/*
====================
idFileSystemLocal::ReplaceSeparators

Fix things up differently for win/unix/mac
====================
*/
void idFileSystemLocal::ReplaceSeparators( idStr &path, char sep ) {
	char *s;

	for( s = &path[ 0 ]; *s ; s++ ) {
		if ( *s == '/' || *s == '\\' ) {
			*s = sep;
		}
	}
}

/*
===================
idFileSystemLocal::BuildOSPath
===================
*/
const char *idFileSystemLocal::BuildOSPath( const char *base, const char *game, const char *relativePath ) const {
	thread_local static char OSPath[MAX_STRING_CHARS];
	idStr newPath;

	if ( fs_caseSensitiveOS.GetBool() || com_developer.GetBool() ) {
		// extract the path, make sure it's all lowercase
		idStr testPath, fileName;

        if ( strlen(game) == 0) {
		    sprintf( testPath, "%s", relativePath );
        } else {
            sprintf( testPath, "%s/%s", game , relativePath );
        }
		testPath.StripFilename();

		if ( testPath.HasUpper() ) {

			// grayman #4327 - stop console spam when folders
			// have uppercase letters in them
			// common->Warning( "Non-portable: path contains uppercase characters: %s", testPath.c_str() );

			// attempt a fixup on the fly
			if ( fs_caseSensitiveOS.GetBool() ) {
				testPath.ToLower();
				fileName = relativePath;
				fileName.StripPath();
                if ( testPath.Length() == 0) {
				    sprintf( newPath, "%s/%s", base, fileName.c_str() );
                } else {
                    sprintf( newPath, "%s/%s/%s", base, testPath.c_str(), fileName.c_str() );
                }
				ReplaceSeparators( newPath );
				common->DPrintf( "Fixed up to %s\n", newPath.c_str() );
				idStr::Copynz( OSPath, newPath, sizeof( OSPath ) );
				return OSPath;
			}
		}
	}

	idStr strBase = base;
	strBase.StripTrailing( '/' );
	strBase.StripTrailing( '\\' );
    if ( strlen(game) == 0) {
        sprintf( newPath, "%s/%s", strBase.c_str(), relativePath );
    } else {
        sprintf( newPath, "%s/%s/%s", strBase.c_str(), game, relativePath );
    }
	ReplaceSeparators( newPath );
	idStr::Copynz( OSPath, newPath, sizeof( OSPath ) );
	return OSPath;
}

/*
================
idFileSystemLocal::OSPathToRelativePath

takes a full OS path, as might be found in data from a media creation
program, and converts it to a relativePath by stripping off directories

Returns false if the osPath tree doesn't match any of the existing
search paths.

================
*/
#define GPATH_COUNT 4
const char *idFileSystemLocal::OSPathToRelativePath( const char *OSPath ) {
	thread_local static char relativePath[MAX_STRING_CHARS];
	const char *s, *base = NULL;
     
	// skip a drive letter?

	// search for anything with "base" in it
	// Ase files from max may have the form of:
	// "//Purgatory/purgatory/doom/base/models/mapobjects/bitch/hologirl.tga"
	// which won't match any of our drive letter based search paths
	bool ignoreWarning = false;

    // we need take the basepath and strip off all but the last directory so that
    // it can be used as a reference point. this should allow users to install TDM
    // in any directory they wish
    idStr dynbase = fs_basepath.GetString();
    dynbase.StripPath();

    const char * gamePath = NULL;
    for ( int gpath = 0; gpath < GPATH_COUNT; gpath++) {
        switch (gpath) {
            case 0: gamePath = BASE_GAMEDIR; break; // taaaki - seems to be some issues with removing this - need to look into it further
            case 1: gamePath = dynbase.c_str(); break; // base the tdm directory off the bin path instead of hardcoding it
            case 2: gamePath = fs_mod.GetString(); break;
            case 3: gamePath = fs_currentfm.GetString(); break; // taaaki - may need to check this if my assumptions are incorrect
            default: gamePath = NULL; break;
        }

        if ( base == NULL && gamePath && strlen( gamePath ) ) {
            base = strstr( OSPath, gamePath );
            while ( base ) {
				char c1 = '\0', c2;
				if ( base > OSPath ) {
					c1 = *(base - 1);
				}
				c2 = *( base + strlen( gamePath ) );
				if ( ( c1 == '/' || c1 == '\\' ) && ( c2 == '/' || c2 == '\\' ) ) {
					break;
				}
				base = strstr( base + 1, gamePath );
			}
		}
    }

	if ( base ) {
		s = strstr( base, "/" );
		if ( !s ) {
			s = strstr( base, "\\" );
		} 
		if ( s ) {
			strcpy( relativePath, s + 1 );
			if ( fs_debug.GetInteger() > 1 ) {
				common->Printf( "idFileSystem::OSPathToRelativePath: %s becomes %s\n", OSPath, relativePath );
			}
			return relativePath;
		}
	}

	if ( !ignoreWarning ) {
		common->Warning( "idFileSystem::OSPathToRelativePath failed on %s", OSPath );
	}

	strcpy( relativePath, "" );
	return relativePath;
}

/*
=====================
idFileSystemLocal::RelativePathToOSPath

Returns a fully qualified path that can be used with stdio libraries
=====================
*/
const char *idFileSystemLocal::RelativePathToOSPath( const char *relativePath, const char *basePath, const char *gamedir ) {
	const char *path = cvarSystem->GetCVarString( basePath );
	if ( !path[0] ) {
		path = fs_savepath.GetString(); // taaaki - need to re-evaluate this as the default
	}

	return BuildOSPath( path, gamedir ? gamedir : gameFolder.c_str(), relativePath );
}

/*
=================
idFileSystemLocal::RemoveFile
=================
*/
void idFileSystemLocal::RemoveFile( const char *relativePath, const char *gamedir ) {
	//better forbid doing other modifications in parallel
	idScopedCriticalSection lock(globalMutex);

	idStr OSPath;
    int removeResult = -1;

	if ( fs_devpath.GetString()[0] ) {
		OSPath = BuildOSPath( fs_devpath.GetString(), gamedir ? gamedir : gameFolder.c_str(), relativePath );
        removeResult = remove( OSPath );
	}

    if (removeResult != 0)
    {
        OSPath = BuildOSPath(fs_modSavePath.GetString(), gamedir ? gamedir : gameFolder.c_str(), relativePath);
        remove(OSPath);
    }

	ClearDirCache();
}

/*
================
idFileSystemLocal::FileIsInPAK
================
*/
bool idFileSystemLocal::FileIsInPAK( const char *relativePath ) const {
	//the only reason to lock here is to forbid adding searchPaths in parallel
	idScopedCriticalSection lock(globalMutex);

	searchpath_t	*search;
	pack_t			*pak;
	fileInPack_t	*pakFile;
	int			hash;

	if ( !searchPaths ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	} else if ( !relativePath ) {
		common->FatalError( "idFileSystemLocal::FileIsInPAK: NULL 'relativePath' parameter passed\n" );
	} else if ( relativePath[0] == '/' || relativePath[0] == '\\' ) { // paths are not supposed to have a leading slash
		relativePath++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo" 
	if ( strstr( relativePath, ".." ) || strstr( relativePath, "::" ) ) {
		return false;
	}

	//
	// search through the path, one element at a time
	//

	hash = HashFileName( relativePath );

	for ( search = searchPaths; search; search = search->next ) {
		// is the element a pak file?
		if ( search->pack && search->pack->hashTable[hash] ) {

			// look through all the pak file elements
			pak = search->pack;
			pakFile = pak->hashTable[hash];
			do {
				// case and separator insensitive comparisons
				if ( !FilenameCompare( pakFile->name, relativePath ) ) {
					return true;
				}
				pakFile = pakFile->next;
			} while( pakFile != NULL );
		}
	}
	return false;
}

/*
============
idFileSystemLocal::ReadFile

Filename are relative to the search path
a null buffer will just return the file length and time without loading
timestamp can be NULL if not required
============
*/
int idFileSystemLocal::ReadFile( const char *relativePath, void **buffer, ID_TIME_T *timestamp ) {
	idFile *	f;
	byte *		buf;
	int			len;
	bool		isConfig;

	if ( !IsInitialized() ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	} else if ( !relativePath || !relativePath[0] ) {
		common->FatalError( "idFileSystemLocal::ReadFile: NULL 'relativePath' parameter passed\n" );
	}
	
	if ( timestamp ) {
		*timestamp = FILE_NOT_FOUND_TIMESTAMP;
	}

	if ( buffer ) {
		*buffer = NULL;
	}

	//buf = NULL;	// quiet compiler warning - does this apply to GCC? nothing in MSC

	// if this is a .cfg file and we are playing back a journal, read
	// it from the journal file
	if ( strstr( relativePath, ".cfg" ) == relativePath + strlen( relativePath ) - 4 ) {
		idScopedCriticalSection lock(globalMutex);	//just in case (normally, never executed)
		isConfig = true;
		if ( eventLoop && eventLoop->JournalLevel() == 2 ) {

			loadCount.Increment();
			loadStack.Increment();

			common->DPrintf( "Loading %s from journal file.\n", relativePath );

			len = 0;
			int	r = eventLoop->com_journalDataFile->Read( &len, sizeof( len ) );
			if ( r != sizeof( len ) ) {
				*buffer = NULL;
				return -1;
			}

			buf = (byte *)Mem_ClearedAlloc(len+1);
			*buffer = buf;
			r = eventLoop->com_journalDataFile->Read( buf, len );
			if ( r != len ) {
				common->FatalError( "Read from journalDataFile failed" );
			}

			// guarantee that it will have a trailing 0 for string operations
			buf[len] = 0;

			return len;
		}
	} else {
		isConfig = false;
	}

	// look for it in the filesystem or pack files
    f = OpenFileRead( relativePath );
	if ( f == NULL ) {
		if ( buffer ) {
			*buffer = NULL;
		}
		return -1;
	}

	if ( timestamp ) {
		*timestamp = f->Timestamp();
	}

	len = f->Length();
	if ( !buffer ) {
		CloseFile( f );
		return len;
	}

	loadCount.Increment();
	loadStack.Increment();

	*buffer = buf = (byte *)Mem_ClearedAlloc(len+1);

	f->Read( buf, len );

	// guarantee that it will have a trailing 0 for string operations
	buf[len] = 0;
	CloseFile( f );

	// if we are journalling and it is a config file, write it to the journal file
	if ( isConfig && eventLoop && eventLoop->JournalLevel() == 1 ) {
		idScopedCriticalSection lock(globalMutex);	//just in case
		common->DPrintf( "Writing %s to journal file.\n", relativePath );
		eventLoop->com_journalDataFile->Write( &len, sizeof( len ) );
		eventLoop->com_journalDataFile->Write( buf, len );
		eventLoop->com_journalDataFile->Flush();
	}

	return len;
}

/*
=============
idFileSystemLocal::FreeFile
=============
*/
void idFileSystemLocal::FreeFile( void *buffer ) {
	if ( !searchPaths ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	} else if ( !buffer ) {
		common->FatalError( "idFileSystemLocal::FreeFile( NULL )" );
	}
	loadStack.Decrement();

	Mem_Free( buffer );
}

/*
============
idFileSystemLocal::WriteFile

Filenames are relative to the search path
============
*/
int idFileSystemLocal::WriteFile( const char *relativePath, const void *buffer, int size, const char *basePath, const char *gamedir ) {
	idFile *f;

	if ( !IsInitialized() ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	} else if ( !relativePath || !buffer ) {
		common->FatalError( "idFileSystemLocal::WriteFile: NULL parameter" );
	}

	f = idFileSystemLocal::OpenFileWrite( relativePath, basePath, gamedir );
	if ( !f ) {
		common->Printf( "Failed to open %s\n", relativePath );
		return -1;
	}

	size = f->Write( buffer, size );

	CloseFile( f );

	return size;
}

/*
=================
idFileSystemLocal::ParseAddonDef
=================
*/
addonInfo_t *idFileSystemLocal::ParseAddonDef( const char *buf, const int len ) {
	idLexer		src;
	idToken		token, token2;
	addonInfo_t	*info;

	src.LoadMemory( buf, len, "<addon.conf>" );
	src.SetFlags( DECL_LEXER_FLAGS );
	if ( !src.SkipUntilString( "addonDef" ) ) {
		src.Warning( "ParseAddonDef: no addonDef" );
		return NULL;
	}
	else if ( !src.ReadToken( &token ) ) {
		src.Warning( "Expected {" );
		return NULL;
	}

	info = new addonInfo_t;
	// read addonDef
	while ( 1 ) {
		if ( !src.ReadToken( &token ) ) {
			delete info;
			return NULL;
		}
		if ( !token.Icmp( "}" ) ) {
			break;
		}
		if ( token.type != TT_STRING ) {
			src.Warning( "Expected quoted string, but found '%s'", token.c_str() );
			delete info;
			return NULL;
		}
		int checksum;
		if ( sscanf( token.c_str(), "0x%x", &checksum ) != 1 && sscanf( token.c_str(), "%x", &checksum ) != 1 ) {
			src.Warning( "Could not parse checksum '%s'", token.c_str() );
			delete info;
			return NULL;
		}
		info->depends.Append( checksum );
	}
	// read any number of mapDef entries
	while ( 1 ) {
		if ( !src.SkipUntilString( "mapDef" ) ) {
			return info;
		}
		if ( !src.ReadToken( &token ) ) {
			src.Warning( "Expected map path" );
			info->mapDecls.DeleteContents( true );
			delete info;
			return NULL;
		}
		idDict *dict = new idDict;

		dict->Set( "path", token.c_str() );
		if ( !src.ReadToken( &token ) ) {
			src.Warning( "Expected {" );
			info->mapDecls.DeleteContents( true );
			delete dict;
			delete info;
			return NULL;
		}
		while ( 1 ) {
			if ( !src.ReadToken( &token ) ) {
				break;
			}
			if ( !token.Icmp( "}" ) ) {
				break;
			}
			if ( token.type != TT_STRING ) {
				src.Warning( "Expected quoted string, but found '%s'", token.c_str() );
				info->mapDecls.DeleteContents( true );
				delete dict;
				delete info;
				return NULL;
			}

			if ( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				info->mapDecls.DeleteContents( true );
				delete dict;
				delete info;
				return NULL;
			}

			if ( dict->FindKey( token ) ) {
				src.Warning( "'%s' already defined", token.c_str() );
			}
			dict->Set( token, token2 );
		}
		info->mapDecls.Append( dict );
	}
	assert( false );
	return NULL;
}


/*
stgatilov: Some types of files should not be compressed by pk4.
Otherwise reading them would be quite inefficient, usually because seeking through zlib stream is slow.
Primary examples are:
  issue 4507: ROQ videos are now read by ffmpeg, which performs tons of seeking through file;
	issue 4504: OGG sounds are read by ogg+vorbis, which also performs quite a lot of seeking;
That's why if we detect that such a file is compressed in pk4, we recompress the whole pk4 archive.
*/
#include "CompressionParameters.h"
bool DoNotCompressFile(const char *filename) {
	char ext[16] = ".";
	for (int i = 0; i < PK4_UNCOMPRESSED_EXTENSIONS_COUNT; i++) {
		strcpy(ext + 1, PK4_UNCOMPRESSED_EXTENSIONS[i]);
		if (idStr::CheckExtension(filename, ext))
			return true;
	}
	return false;
}

static const int REPACK_BUFFER_SIZE = 32768;
bool RepackPK4(unzFile uf, const char *srcZipfile) {
	common->Printf("Repacking %s...\n", srcZipfile);
#define SAFECALL(expr, ...) \
	if ((expr) != 0) { \
		common->Warning(__VA_ARGS__); \
		return false; \
	}

	//create new pk4 file
	idStr tempZipfileB = idStr(srcZipfile) + ".tmp";
	const char *tempZipfile = tempZipfileB.c_str();
	remove(tempZipfile);
	zipFile zf = zipOpen(tempZipfile, APPEND_STATUS_CREATE);
	SAFECALL(zf == NULL, "Cannot create %s", tempZipfile);

	//open old pk4 file
	unz_global_info64 gi;
	SAFECALL(unzGetGlobalInfo64( uf, &gi ), "Cannot get global info of %s", srcZipfile);

	//create new pk4.tmp file with repacked data
	char filename_inzip[MAX_ZIPPED_FILE_NAME];
	unz_file_info64	unz_file_info;
	zip_fileinfo zip_file_info;
	char buffer[REPACK_BUFFER_SIZE];
	SAFECALL(unzGoToFirstFile(uf), "Cannot get first file of %s", srcZipfile);
	for (int i = 0; i < gi.number_entry; i++) {
		SAFECALL(unzGetCurrentFileInfo64( uf, &unz_file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0 ), "Failed to read info of file in %s", srcZipfile);
		//copy necessary file info
        zip_file_info.dosDate = unz_file_info.dosDate;
		zip_file_info.external_fa = unz_file_info.external_fa;
		zip_file_info.internal_fa = unz_file_info.internal_fa;
		zip_file_info.tmz_date = reinterpret_cast<tm_zip&>(unz_file_info.tmu_date);
		//check if the file needs complete repacking (or direct copy is enough)
		bool repack_file = DoNotCompressFile(filename_inzip) && unz_file_info.compression_method != 0;
		bool raw = !repack_file;
		//open file in both pk4-s
		int method, level;
		SAFECALL(unzOpenCurrentFile2(uf, &method, &level, raw), "Cannot open file %s in %s", filename_inzip, srcZipfile);
		if (repack_file)
			method = level = 0;	//store repacked files uncompressed
		SAFECALL(zipOpenNewFileInZip2(zf, filename_inzip, &zip_file_info, NULL, 0, NULL, 0, NULL, method, level, raw), "Cannot create file %s in %s", filename_inzip, tempZipfile);
		//copy all file data from old to new pk4
		while (true) {
			int cnt = unzReadCurrentFile(uf, buffer, REPACK_BUFFER_SIZE);
			SAFECALL(cnt < 0, "Cannot read from file %s in %s", filename_inzip, srcZipfile);
			if (cnt == 0) break;
			SAFECALL(zipWriteInFileInZip(zf, buffer, cnt), "Cannot write to file %s in %s", filename_inzip, tempZipfile);
		}
		//close file in both pk4-s
		SAFECALL(unzCloseCurrentFile(uf), "Cannot close file %s in %s", filename_inzip, srcZipfile);
		SAFECALL(zipCloseFileInZipRaw(zf, unz_file_info.uncompressed_size, unz_file_info.crc), "Cannot finish file %s in %s", filename_inzip, tempZipfile);
		//iterate further
		if (i + 1 < gi.number_entry)
			SAFECALL(unzGoToNextFile(uf), "Cannot iterate to next file in %s", srcZipfile);
	}

	//close new and old pk4 files
	SAFECALL(unzClose(uf), "Cannot close %s", srcZipfile);
	SAFECALL(zipClose(zf, NULL), "Cannot finish %s", tempZipfile);

	//replace the old pk4 file
	if (!fileSystem->CopyFile(tempZipfile, srcZipfile)) {
		common->Warning("Cannot overwrite-copy %s to %s", tempZipfile, srcZipfile);
		return false;
	}
	remove(tempZipfile);

#undef SAFECALL
	return true;
}


/*
=================
idFileSystemLocal::LoadZipFile
=================
*/
pack_t *idFileSystemLocal::LoadZipFile( const char *zipfile ) {
	fileInPack_t *	buildBuffer;
	pack_t *		pack;
	unzFile			uf;
	int				err;
	unz_global_info64 gi;
	char			filename_inzip[MAX_ZIPPED_FILE_NAME];
	unz_file_info64	file_info;
	int			hash;
	int				fs_numHeaderLongs;
	int *			fs_headerLongs;
	FILE			*f;
	int				len;
	int				confHash;
	fileInPack_t	*pakFile;

	f = OpenOSFile( zipfile, "rb" );
	if ( !f ) {
		return NULL;
	}
	fseek( f, 0, SEEK_END );
	len = ftell( f );
	fclose( f );

	fs_numHeaderLongs = 0;

	uf = unzOpen( zipfile );
	err = unzGetGlobalInfo64( uf, &gi );

	if ( err != UNZ_OK ) {
		return NULL;
	}

	buildBuffer = new fileInPack_t[gi.number_entry];
	pack = new pack_t;
	for( int i = 0; i < FILE_HASH_SIZE; i++ ) {
		pack->hashTable[i] = NULL;
	}

	pack->pakFilename = zipfile;
	pack->handle = uf;
	pack->numfiles = gi.number_entry;
	pack->buildBuffer = buildBuffer;
	pack->referenced = false;
	pack->binary = BINARY_UNKNOWN;
	pack->addon = false;
	pack->addon_search = false;
	pack->addon_info = NULL;
	pack->isNew = false;

	pack->length = len;

	bool needsRepacking = false;
	unzGoToFirstFile(uf);
	fs_headerLongs = (int *)Mem_ClearedAlloc( gi.number_entry * sizeof(int) );
	for ( int i = 0; i < (int)gi.number_entry; i++ ) {
		err = unzGetCurrentFileInfo64( uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0 );
		if ( err != UNZ_OK ) {
			break;
		}
		if ( file_info.uncompressed_size > 0 ) {
			fs_headerLongs[fs_numHeaderLongs++] = LittleInt( file_info.crc );
		}
		hash = HashFileName( filename_inzip );
		buildBuffer[i].name = filename_inzip;
		buildBuffer[i].name.ToLower();
		buildBuffer[i].name.BackSlashesToSlashes();
		// store the file position in the zip
		buildBuffer[i].pos = unzGetOffset64( uf );
		// add the file to the hash
		buildBuffer[i].next = pack->hashTable[hash];
		pack->hashTable[hash] = &buildBuffer[i];
		//stgatilov: check if file is compressed but should be uncompressed
		if (DoNotCompressFile(filename_inzip) && file_info.compression_method != 0)
			needsRepacking = true;
		// go to the next file in the zip
		unzGoToNextFile(uf);
	}

	//stgatilov: repack the whole pk4 if required
	if (needsRepacking) {
		Mem_Free(fs_headerLongs);
		delete pack;
		delete[] buildBuffer;
		if (RepackPK4(uf, zipfile))
			return LoadZipFile(zipfile);
		else
			return NULL;	//repacking error
	}

	// check if this is an addon pak
	pack->addon = false;
	confHash = HashFileName( ADDON_CONFIG );
	for ( pakFile = pack->hashTable[confHash]; pakFile; pakFile = pakFile->next ) {
		if ( !FilenameCompare( pakFile->name, ADDON_CONFIG ) ) {			
			pack->addon = true;			
			idFile_InZip *file = ReadFileFromZip( pack, pakFile, ADDON_CONFIG );
			// may be just an empty file if you don't bother about the mapDef
			if ( file && file->Length() ) {
				char *buf = new char[ file->Length() + 1 ];
				file->Read( (void *)buf, file->Length() );
				buf[ file->Length() ] = '\0';
				pack->addon_info = ParseAddonDef( buf, file->Length() );
				delete[] buf;
			}
			if ( file ) {
				CloseFile( file );
			}
			break;
		}
	}

	pack->checksum = MD4_BlockChecksum( fs_headerLongs, 4 * fs_numHeaderLongs );
	pack->checksum = LittleInt( pack->checksum );

	Mem_Free( fs_headerLongs );

	return pack;
}

/*
===============
idFileSystemLocal::AddZipFile
adds a downloaded pak file to the list so we can work out what we have and what we still need
the isNew flag is set to true, indicating that we cannot add this pak to the search lists without a restart
===============
*/
int idFileSystemLocal::AddZipFile( const char *path ) {
	idScopedCriticalSection lock(globalMutex);

	idStr			fullpath = fs_savepath.GetString(); // taaaki - is savepath the correct place to look?
	pack_t			*pak;
	searchpath_t	*search, *last;

	fullpath.AppendPath( path );
	pak = LoadZipFile( fullpath );
	if ( !pak ) {
		common->Warning( "AddZipFile %s failed", path );
		return 0;
	}

	// insert the pak at the end of the search list - temporary until we restart
	pak->isNew = true;
	search = new searchpath_t;
	search->dir = NULL;
	search->pack = pak;
	search->domain = FDOM_UNKNOWN;
	search->next = NULL;
	last = searchPaths;

	while ( last->next ) {
		last = last->next;
	}

	last->next = search;
	common->Printf( "Appended %s (checksum 0x%x)\n", pak->pakFilename.c_str(), pak->checksum );
	return pak->checksum;
}

/*
===============
idFileSystemLocal::AddUnique
===============
*/
int idFileSystemLocal::AddUnique( const char *name, idStrList &list, idHashIndex &hashIndex ) {
	const int hashKey = hashIndex.GenerateKey( name, false );

	for ( int i = hashIndex.First( hashKey ); i >= 0; i = hashIndex.Next( i ) ) {
		if ( list[i].Icmp( name ) == 0 ) {
			return i;
		}
	}

	const int r = list.Append( name );
	hashIndex.Add( hashKey, r );

	return r;
}

/*
===============
idFileSystemLocal::GetExtensionList
===============
*/
void idFileSystemLocal::GetExtensionList( const char *extension, idStrList &extensionList ) {
	int s, e;
	const int l = idStr::Length( extension );

	s = 0;
	while( 1 ) {
		e = idStr::FindChar( extension, '|', s, l );
		if ( e != -1 ) {
			extensionList.Append( idStr( extension, s, e ) );
			s = e + 1;
		} else {
			extensionList.Append( idStr( extension, s, l ) );
			break;
		}
	}
}

/*
===============
idFileSystemLocal::GetFileList

Does not clear the list first so this can be used to progressively build a file list.
When 'sort' is true only the new files added to the list are sorted.
===============
*/
int idFileSystemLocal::GetFileList( const char *relativePath, const idStrList &extensions, idStrList &list, idHashIndex &hashIndex, bool fullRelativePath, const char* gamedir ) const {
	searchpath_t *	search;
	fileInPack_t *	buildBuffer;
	int				pathLength;
	int				length;
	const char *	name;
	pack_t *		pak;
	idStr			work;
	int				j;

	if ( !searchPaths ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	} else if ( !extensions.Num() || !relativePath ) {
		return 0;
	}

    pathLength = static_cast<int>(strlen(relativePath));
	if ( pathLength ) {
		pathLength++;	// for the trailing '/'
	}

	// search through the path, one element at a time, adding to list
	for ( search = searchPaths; search != NULL; search = search->next ) {
		if ( search->dir ) {
			if ( gamedir && strlen(gamedir) && (search->dir->gamedir != gamedir) ) {
					continue;
			}

			idStrList	sysFiles;
			idStr		netpath;

			netpath = BuildOSPath( search->dir->path, search->dir->gamedir, relativePath );

			for ( int i = 0; i < extensions.Num(); i++ ) {

				// scan for files in the filesystem
				ListOSFiles( netpath, extensions[i], sysFiles );

				// if we are searching for directories, remove . and ..
				if ( extensions[i][0] == '/' && extensions[i][1] == 0 ) {
					sysFiles.Remove( "." );
					sysFiles.Remove( ".." );
				}

				for( j = 0; j < sysFiles.Num(); j++ ) {
					// unique the match
					if ( fullRelativePath ) {
						work = relativePath;
						work += "/";
						work += sysFiles[j];
						AddUnique( work, list, hashIndex );
					}
					else {
						AddUnique( sysFiles[j], list, hashIndex );
					}
				}
			}
		} else if ( search->pack ) {
			// look through all the pak file elements

			pak = search->pack;
			buildBuffer = pak->buildBuffer;
			for( int i = 0; i < pak->numfiles; i++ ) {

				length = buildBuffer[i].name.Length();

				// if the name is not long anough to at least contain the path
				if ( length <= pathLength ) {
					continue;
				}

				name = buildBuffer[i].name;

				// check for a path match without the trailing '/'
				if ( pathLength && idStr::Icmpn( name, relativePath, pathLength - 1 ) != 0 ) {
					continue;
				}
 
				// ensure we have a path, and not just a filename containing the path
				if ( name[ pathLength ] == '\0' || name[pathLength - 1] != '/' ) {
					continue;
				}
 
				// make sure the file is not in a subdirectory
				for ( j = pathLength; name[j+1] != '\0'; j++ ) {
					if ( name[j] == '/' ) {
						break;
					}
				}
				if ( name[j+1] ) {
					continue;
				}

				// check for extension match
				for ( j = 0; j < extensions.Num(); j++ ) {
					if ( length >= extensions[j].Length() && extensions[j].Icmp( name + length - extensions[j].Length() ) == 0 ) {
						break;
					}
				}
				if ( j >= extensions.Num() ) {
					continue;
				}

				// unique the match
				if ( fullRelativePath ) {
					work = relativePath;
					work += "/";
					work += name + pathLength;
					work.StripTrailing( '/' );
					AddUnique( work, list, hashIndex );
				} else {
					work = name + pathLength;
					work.StripTrailing( '/' );
					AddUnique( work, list, hashIndex );
				}
			}
		}
	}

	return list.Num();
}

/*
===============
idFileSystemLocal::ListFiles
===============
*/
idFileList *idFileSystemLocal::ListFiles( const char *relativePath, const char *extension, bool sort, bool fullRelativePath, const char* gamedir ) const {
	idScopedCriticalSection lock(globalMutex);

	idHashIndex hashIndex( 4096, 4096 );
	idStrList extensionList;

	idFileList *fileList = new idFileList;
	fileList->basePath = relativePath;

	GetExtensionList( extension, extensionList );

	GetFileList( relativePath, extensionList, fileList->list, hashIndex, fullRelativePath, gamedir );

	if ( sort ) {
		idStrListSortPaths( fileList->list );
	}

	return fileList;
}

/*
===============
idFileSystemLocal::GetFileListTree
===============
*/
int idFileSystemLocal::GetFileListTree( const char *relativePath, const idStrList &extensions, idStrList &list, idHashIndex &hashIndex, const char* gamedir ) const {
	idStrList slash, folders( 128 );
	idHashIndex folderHashIndex( 1024, 128 );

	// recurse through the subdirectories
	slash.Append( "/" );
	GetFileList( relativePath, slash, folders, folderHashIndex, true, gamedir );
	for ( int i = 0; i < folders.Num(); i++ ) {
		if ( folders[i][0] == '.' ) {
			continue;
		}
		if ( folders[i].Icmp( relativePath ) == 0 ){
			continue;
		}
		GetFileListTree( folders[i], extensions, list, hashIndex, gamedir );
	}

	// list files in the current directory
	GetFileList( relativePath, extensions, list, hashIndex, true, gamedir );

	return list.Num();
}

/*
===============
idFileSystemLocal::ListFilesTree
===============
*/
idFileList *idFileSystemLocal::ListFilesTree( const char *relativePath, const char *extension, bool sort, const char* gamedir ) const {
	idScopedCriticalSection lock(globalMutex);

	idHashIndex hashIndex( 4096, 4096 );
	idStrList extensionList;

	idFileList *fileList = new idFileList();
	fileList->basePath = relativePath;
	fileList->list.SetGranularity( 4096 );

	GetExtensionList( extension, extensionList );

	GetFileListTree( relativePath, extensionList, fileList->list, hashIndex, gamedir );

	if ( sort ) {
		idStrListSortPaths( fileList->list );
	}

	return fileList;
}

/*
===============
idFileSystemLocal::FreeFileList
===============
*/
void idFileSystemLocal::FreeFileList( idFileList *fileList ) const {
	delete fileList;
}

/*
===============
idFileSystemLocal::ListMods
===============

Tels: Could entirely be removed unless we want to have "mods to darkmod".
*/
#define MAX_DESCRIPTION		256
idModList *idFileSystemLocal::ListMods( void ) {
	idScopedCriticalSection lock(globalMutex);

	char 		desc[ MAX_DESCRIPTION ];
	idStrList	dirs;
	idStrList	pk4s;

	idModList	*list = new idModList;

	const char	*search[ 3 ];

	search[0] = fs_savepath.GetString();
	search[1] = fs_devpath.GetString();
	search[2] = fs_basepath.GetString();

	for ( int isearch = 0; isearch < 3; isearch++ ) {

		dirs.Clear();
		pk4s.Clear();

		// scan for directories
		ListOSFiles( search[ isearch ], "/", dirs );

		dirs.Remove( "." );
		dirs.Remove( ".." );
		dirs.Remove( "base" );

		// see if there are any pk4 files in each directory
		for( int i = 0; i < dirs.Num(); i++ ) {
			idStr gamepath = BuildOSPath( search[ isearch ], dirs[ i ], "" );
 			ListOSFiles( gamepath, ".pk4", pk4s );
 			if ( pk4s.Num() ) {
 				if ( !list->mods.Find( dirs[ i ] ) ) {
					list->mods.Append( dirs[ i ] );
 				}
 			}
 		}
	}
	   
	list->mods.Sort();

	int isearch;
	// read the descriptions for each mod - search all paths
	for ( int i = 0; i < list->mods.Num(); i++ ) {
		for ( isearch = 0; isearch < 3; isearch++ ) {

			idStr descfile = BuildOSPath( search[ isearch ], list->mods[ i ], "description.txt" );
			FILE *f = OpenOSFile( descfile, "r" );
			if ( f ) {
				if ( fgets( desc, MAX_DESCRIPTION, f ) ) {
					list->descriptions.Append( desc );
					fclose( f );
					break;
				} else {
					common->DWarning( "Error reading %s", descfile.c_str() );
					fclose( f );
					continue;
				}
			}
		}

		if ( isearch == 3 ) {
			list->descriptions.Append( list->mods[ i ] );
		}
	}

	list->mods.Insert( "" );
	list->descriptions.Insert( GAME_NAME );

	assert( list->mods.Num() == list->descriptions.Num() );

	return list;
}

/*
===============
idFileSystemLocal::FreeModList
===============
*/
void idFileSystemLocal::FreeModList( idModList *modList ) {
	delete modList;
}

/*
===============
idDEntry::Matches
===============
*/
bool idDEntry::Matches(const char *directory, const char *extension) const {
	if ( !idDEntry::directory.Icmp( directory ) && !idDEntry::extension.Icmp( extension ) ) {
		return true;
	}
	return false;
}

/*
===============
idDEntry::Init
===============
*/
void idDEntry::Init( const char *directory, const char *extension, const idStrList &list ) {
	idDEntry::directory = directory;
	idDEntry::extension = extension;
	idStrList::operator=(list);
}

/*
===============
idDEntry::Clear
===============
*/
void idDEntry::Clear( void ) {
	directory.Clear();
	extension.Clear();
	idStrList::Clear();
}

/*
===============
idFileSystemLocal::ListOSFiles

 call to the OS for a listing of files in an OS directory
 optionally, perform some caching of the entries
===============
*/
int	idFileSystemLocal::ListOSFiles( const char *directory, const char *extension, idStrList &list ) const {
	int j, ret;

	if ( !extension ) {
		extension = "";
	}

	if ( !fs_caseSensitiveOS.GetBool() ) {
		return Sys_ListFiles( directory, extension, list );
	}

	{
		idScopedCriticalSection lock(dir_cache_mutex);
		// try in cache
		for ( int i = dir_cache_index - 1; i >= (dir_cache_index - dir_cache_count); i-- ) {
			j = (i+MAX_CACHED_DIRS) % MAX_CACHED_DIRS;
			if ( dir_cache[j].Matches( directory, extension ) ) {
				if ( fs_debug.GetInteger() ) {
					//common->Printf( "idFileSystemLocal::ListOSFiles: cache hit: %s\n", directory );
				}
				list = dir_cache[j];
				return list.Num();
			}
		}
	}

#if 0
	if ( fs_debug.GetInteger() ) {
		common->Printf( "idFileSystemLocal::ListOSFiles: cache miss: %s\n", directory );
	}
#endif

	ret = Sys_ListFiles( directory, extension, list );

	if ( ret == -1 ) {
		return -1;
	}

	{
		idScopedCriticalSection lock(dir_cache_mutex);
		// push a new entry
		dir_cache[dir_cache_index].Init( directory, extension, list );
		dir_cache_index = (dir_cache_index + 1) % MAX_CACHED_DIRS;
		if ( dir_cache_count < MAX_CACHED_DIRS ) {
			dir_cache_count++;
		}
	}

	return ret;
}

#include <thread>
void idFileSystemLocal::TestThreads_f( const idCmdArgs& args ) {
	static constexpr int B = 555;
	struct fileRec_t {
		idStr name;
		char data[B];
	};
	idList<fileRec_t> testFiles;
	extern void GetDeclLoadedFiles( idStrList &list );		
	idStrList files;
	GetDeclLoadedFiles( files );

	idTimer tOneThread, tManyThreads;
	tOneThread.Start();
	for ( auto & file: files ) {
		auto & fn = file;
		auto f = fileSystemLocal.OpenFileRead( fn );
		if ( f ) {
			if ( f->Length() >= B ) {
				fileRec_t testFile;
				testFile.name = fn;
				if ( f->Read( testFile.data, B ) != B ) {
					common->Warning( "File read error" );
					return;
				}
				testFiles.Append( testFile );
			}
			fileSystemLocal.CloseFile( f );
		}
	}
	tOneThread.Stop();
	common->Printf( "Testing %d files\n", testFiles.Num() );
	auto testFileFunc = [&] {
		auto fileList = testFiles;
		std::random_shuffle(fileList.begin(), fileList.end());
		for ( auto & testFile : fileList ) {
			auto f = fileSystemLocal.OpenFileRead( testFile.name );
			if ( !f ) {
				common->Warning( "File open error" );
				return;
			} 
			char b[B];
			if ( f->Read( b, B ) != B ) {
				common->Warning( "File read error" );
				return;
			}
			auto dataCheck = memcmp( b, testFile.data, B );
			if(dataCheck)
				common->Warning( "File read mismatch" );
			fileSystemLocal.CloseFile( f );
		}
	};

	tManyThreads.Start();
	const int N = 1<<4;
	std::thread threads[N];
	for ( int i = 0; i < N; i++ ) {
		threads[i] = std::thread( testFileFunc );
	}
	for ( auto& th : threads ) {
		th.join();
	}
	tManyThreads.Stop();

	common->Printf( "Tested %d threads\n", N );
	//note: while time is reported, it is a very bad performance test =)
	common->Printf( "Times: %0.3lf %0.3lf  (x %0.2lf)\n",
		tOneThread.Milliseconds(), tManyThreads.Milliseconds(), 
		tOneThread.Milliseconds() / tManyThreads.Milliseconds()
	);
}

/*
================
idFileSystemLocal::Dir_f
================
*/
void idFileSystemLocal::Dir_f( const idCmdArgs &args ) {
	idStr		relativePath;
	idStr		extension;
	idFileList *fileList;

	if ( args.Argc() < 2 || args.Argc() > 3 ) {
		common->Printf( "usage: dir <directory> [extension]\n" );
		return;
	}

	if ( args.Argc() == 2 ) {
		relativePath = args.Argv( 1 );
		extension = "";
	} else {
		relativePath = args.Argv( 1 );
		extension = args.Argv( 2 );
		if ( extension[0] != '.' ) {
			common->Warning( "extension should have a leading dot" );
		}
	}

	relativePath.BackSlashesToSlashes();
	relativePath.StripTrailing( '/' );

	common->Printf( "Listing of %s/*%s\n", relativePath.c_str(), extension.c_str() );
	common->Printf( "---------------\n" );

	fileList = fileSystemLocal.ListFiles( relativePath, extension );

	for ( int i = 0; i < fileList->GetNumFiles(); i++ ) {
		common->Printf( "%s\n", fileList->GetFile( i ) );
	}
	common->Printf( "%d files\n", fileList->list.Num() );

	fileSystemLocal.FreeFileList( fileList );
}

/*
================
idFileSystemLocal::DirTree_f
================
*/
void idFileSystemLocal::DirTree_f( const idCmdArgs &args ) {
	idStr		relativePath;
	idStr		extension;
	idFileList *fileList;

	if ( args.Argc() < 2 || args.Argc() > 3 ) {
		common->Printf( "usage: dirtree <directory> [extension]\n" );
		return;
	}

	if ( args.Argc() == 2 ) {
		relativePath = args.Argv( 1 );
		extension = "";
	} else {
		relativePath = args.Argv( 1 );
		extension = args.Argv( 2 );
		if ( extension[0] != '.' ) {
			common->Warning( "extension should have a leading dot" );
		}
	}

	relativePath.BackSlashesToSlashes();
	relativePath.StripTrailing( '/' );

	common->Printf( "Listing of %s/*%s /s\n", relativePath.c_str(), extension.c_str() );
	common->Printf( "---------------\n" );

	fileList = fileSystemLocal.ListFilesTree( relativePath, extension );

	for ( int i = 0; i < fileList->GetNumFiles(); i++ ) {
		common->Printf( "%s\n", fileList->GetFile( i ) );
	}
	common->Printf( "%d files\n", fileList->list.Num() );

	fileSystemLocal.FreeFileList( fileList );
}

/*
============
idFileSystemLocal::Path_f
============
*/
void idFileSystemLocal::Path_f( const idCmdArgs &args ) {
	searchpath_t *sp;
	idStr status;

	common->Printf( "Current search path:\n" );

	sp = fileSystemLocal.searchPaths;
	while ( sp ) {
		// stgatilov #5766: show domain
		char domain = '?';
		if ( sp->domain == FDOM_CORE )
			domain = 'C';
		if ( sp->domain == FDOM_FM )
			domain = 'M';

		if ( sp->pack ) {
			if ( com_developer.GetBool() ) {
				sprintf( status, "  [%c] %s (%i files - 0x%x %s",
					domain,
					sp->pack->pakFilename.c_str(),
					sp->pack->numfiles,
					sp->pack->checksum,
					sp->pack->referenced ? "referenced" : "not referenced"
				);
				if ( sp->pack->addon ) {
					status += " - addon)\n";
				} else {
					status += ")\n";
				}
				common->Printf( "%s", status.c_str() );
			} else {
				common->Printf( "  [%c] %s (%i files - 0x%x)\n",
					domain,
					sp->pack->pakFilename.c_str(),
					sp->pack->numfiles,
					sp->pack->checksum
				);
			}
		} else {
			common->Printf( "  [%c] %s/%s\n",
				domain,
				sp->dir->path.c_str(),
				sp->dir->gamedir.c_str()
			);
		}
		sp = sp->next;
	}

	if ( fileSystemLocal.gameDLLChecksum ) {
		common->Printf( "game DLL: 0x%x in pak: 0x%x\n", fileSystemLocal.gameDLLChecksum, fileSystemLocal.gamePakChecksum );
	}

	for( int i = 0; i < MAX_GAME_OS; i++ ) {
		if ( fileSystemLocal.gamePakForOS[ i ] ) {
			common->Printf( "OS %d - pak 0x%x\n", i, fileSystemLocal.gamePakForOS[ i ] );
		}
	}

	// show addon packs that are *not* in the search lists
	sp = fileSystemLocal.addonPaks;
	if ( sp ) {
		common->Printf( "Addon pk4s:\n" );
	
		while ( sp ) {
			if ( com_developer.GetBool() && sp->pack ) {
				common->Printf( "  %s (%i files - 0x%x)\n", sp->pack->pakFilename.c_str(), sp->pack->numfiles, sp->pack->checksum );
			} else if ( sp->pack ) {
				common->Printf( "  t%s (%i files)\n", sp->pack->pakFilename.c_str(), sp->pack->numfiles );
			}
			sp = sp->next;
		}
	}
}

/*
============
idFileSystemLocal::GetOSMask
============
*/
int idFileSystemLocal::GetOSMask( void ) {
	idScopedCriticalSection lock(globalMutex);

	int ret = 0;

	for( int i = 0; i < MAX_GAME_OS; i++ ) {
		if ( fileSystemLocal.gamePakForOS[ i ] ) {
			ret |= ( 1 << i );
		}
	}

	if ( !ret ) {
		return -1;
	}

	return ret;
}

/*
============
idFileSystemLocal::TouchFile_f

The only purpose of this function is to allow game script files to copy
arbitrary files furing an "fs_copyfiles 1" run. taaaki - TODO : check if I need to remove this
============
*/
void idFileSystemLocal::TouchFile_f( const idCmdArgs &args ) {

	if ( args.Argc() != 2 ) {
		common->Printf( "Usage: touchFile <file>\n" );
		return;
	}

	idFile *f = fileSystemLocal.OpenFileRead( args.Argv( 1 ) );
	if ( f ) {
		fileSystemLocal.CloseFile( f );
	}
}

/*
============
idFileSystemLocal::TouchFileList_f

Takes a text file and touches every file in it, use one file per line.
============
*/
void idFileSystemLocal::TouchFileList_f( const idCmdArgs &args ) {
	
	if ( args.Argc() != 2 ) {
		common->Printf( "Usage: touchFileList <filename>\n" );
		return;
	}

	const char *buffer = NULL;
	idParser src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
	if ( fileSystem->ReadFile( args.Argv( 1 ), ( void** )&buffer, NULL ) && buffer ) {
        src.LoadMemory(buffer, static_cast<int>(strlen(buffer)), args.Argv(1));
		if ( src.IsLoaded() ) {
			idToken token;
			while( src.ReadToken( &token ) ) {
				common->Printf( "%s\n", token.c_str() );
				session->UpdateScreen();
				idFile *f = fileSystemLocal.OpenFileRead( token );
				if ( f ) {
					fileSystemLocal.CloseFile( f );
				}
			}
		}
	}

}

/*
================
idFileSystemLocal::AddGameDirectory

Sets gameFolder, adds the directory to the head of the search paths, then loads any pk4 files.
================
*/
void idFileSystemLocal::AddGameDirectory( const char *path, const char *dir, domainStatus_t domain ) {
	searchpath_t *	search;
	pack_t *		pak;
	idStr			pakfile;
	idStrList		pakfiles;

	// check if the search path already exists
	for ( search = searchPaths; search; search = search->next ) {
		// if this element is a pak file
		if ( !search->dir ) {
			continue;
		}
		if ( search->dir->path.Cmp( path ) == 0 && search->dir->gamedir.Cmp( dir ) == 0 ) {
			return;
		}
	}

	gameFolder = dir;

	//
	// add the directory to the search path
	//
	search = new searchpath_t;
	search->dir = new directory_t;
	search->pack = NULL;
	search->domain = domain;

	search->dir->path = path;
	search->dir->gamedir = dir;
	search->next = searchPaths;
	searchPaths = search;

	// find all pak files in this directory
	pakfile = BuildOSPath( path, dir, "" );
	pakfile[ pakfile.Length() - 1 ] = 0;	// strip the trailing slash

	ListOSFiles( pakfile, ".pk4", pakfiles );

	// sort them so that later alphabetic matches override
	// earlier ones. This makes pak1.pk4 override pak0.pk4
	pakfiles.Sort();

	for ( int i = 0; i < pakfiles.Num(); i++ ) {
		pakfile = BuildOSPath( path, dir, pakfiles[i] );
		pak = LoadZipFile( pakfile );
		if ( !pak ) {
			continue;
		}

		// insert the pak after the directory it comes from
		search = new searchpath_t;
		search->dir = NULL;
		search->pack = pak;
		search->domain = domain;
		search->next = searchPaths->next;
		searchPaths->next = search;

		// This same info is printed in the next list, if you want checksums use fs_debug.
		//common->Printf( "  Loaded %s (checksum 0x%x)\n", pakfile.c_str(), pak->checksum );
	}
}

/*
================
idFileSystemLocal::SetupGameDirectories

  Takes care of the correct search order.
================
*/
void idFileSystemLocal::SetupGameDirectories( const char *gameName ) {
	// setup basepath
	if ( fs_basepath.GetString()[0] ) {
		AddGameDirectory( fs_basepath.GetString(), gameName, FDOM_UNKNOWN );
	}
}

/*
===============
idFileSystemLocal::FollowDependencies
===============
*/
void idFileSystemLocal::FollowAddonDependencies( pack_t *pak ) {
	// Serp: I've moved this into the conditional below - I dont see why it's worthy of an assert.
	//assert( pak );

	if ( !pak || !pak->addon_info || !pak->addon_info->depends.Num() ) {
		return;
	}

	int num = pak->addon_info->depends.Num();
	for ( int i = 0; i < num; i++ ) {
		pack_t *deppak = GetPackForChecksum( pak->addon_info->depends[ i ], true );
		if ( deppak ) {
			// make sure it hasn't been marked for search already
			if ( !deppak->addon_search ) {
				// must clean addonChecksums as we go
				int addon_index = addonChecksums.FindIndex( deppak->checksum );
				if ( addon_index >= 0 ) {
					addonChecksums.RemoveIndex( addon_index );
				}
				deppak->addon_search = true;
				common->Printf( "Addon pk4 %s 0x%x depends on pak %s 0x%x, will be searched\n",
								pak->pakFilename.c_str(), pak->checksum,
								deppak->pakFilename.c_str(), deppak->checksum );
				FollowAddonDependencies( deppak );
			}
		} else {
			common->Printf( "Addon pk4 %s 0x%x depends on unknown pak 0x%x\n",
							pak->pakFilename.c_str(), pak->checksum, pak->addon_info->depends[ i ] );
		}
	}
}

/*
================
idFileSystemLocal::Startup
================
*/
void idFileSystemLocal::Startup( void ) {
	searchpath_t	**search;
	pack_t			*pak;
	int				addon_index;
    bool            addFsModSave = false;

	common->Printf( "------ Initializing File System ------\n" );

	if ( restartChecksums.Num() ) {
		common->Printf( "restarting with %d pak files\n", restartChecksums.Num() );
	}
	if ( addonChecksums.Num() ) {
		common->Printf( "restarting filesystem with %d addon pak file(s) to include\n", addonChecksums.Num() );
	}

    AddGameDirectory( fs_basepath.GetString(), "", FDOM_CORE ); // always add the basepath

    // fs_mod override
    if ( fs_mod.GetString()[0] &&
         idStr::Icmp( fs_mod.GetString(), BASE_TDM ) ) {
		SetupGameDirectories( fs_mod.GetString() );
	}

#if 0
// taaaki - this code will no longer be necessary if individual user save locations are abolished.
//          remove when certain

//#ifndef _WIN32
    // taaaki - TODO - this probably doesn't need to be here anymore since we don't
    //                 appear to be using the user's homedir to store configs
    idStr homeSave = Sys_HomeSavePath();
	if ( homeSave.c_str()[0] && 
         fs_mod.GetString()[0] ) {
        AddGameDirectory( homeSave.c_str(), fs_mod.GetString() );
    }
#endif

    // taaaki: setup fm path -- 
    //   fs_modSavePath/fs_currentfm > fs_mod > BASE_TDM ("darkmod")
    // The following logic has been deprecated:
    //   Old logic: In between fs_ and fs_game_base, there is the mission folder, which is fms/<missionName>/
    //   fs_game still overrides that one, but the the mission folder should still override fs_game_base
    if ( fs_modSavePath.GetString()[0] && 
         fs_currentfm.GetString()[0] &&
         idStr::Icmp( fs_currentfm.GetString(), fs_mod.GetString() ) ) {
        
        // don't add the modSavePath just yet. wait to see if devpath has been modified
        addFsModSave = true;
        //AddGameDirectory( fs_modSavePath.GetString(), fs_currentfm.GetString() );
    }

    // if fs_devpath is set by the user, assume that this is a wip directory and add it to the top of the searchpaths
    if ( fs_devpath.GetString()[0] && 
         fs_currentfm.GetString()[0] &&
         idStr::Icmp( fs_currentfm.GetString(), fs_mod.GetString() ) &&
         idStr::Icmp( fs_devpath.GetString(), Sys_DefaultSavePath() ) ) {
        
        //AddGameDirectory( fs_devpath.GetString(), "" );
        idStr devbase ( fs_devpath.GetString() );
        idStr devfm ( fs_devpath.GetString() ); 
        devbase.StripFilename();
        devfm.StripPath();

        // if fs_modSavePath has been modified (cmdline args) and differs from the default, use the 
        // supplied version. otherwise, recreate fs_modSavePath using the devpath
        if ( addFsModSave && idStr::Icmp( fs_modSavePath.GetString(), Sys_ModSavePath() ) ) {
            AddGameDirectory( fs_modSavePath.GetString(), fs_currentfm.GetString(), FDOM_FM );
        } else {
            fs_modSavePath.SetString( devbase.c_str() );
            AddGameDirectory( fs_modSavePath.GetString(), fs_currentfm.GetString(), FDOM_FM );
            addFsModSave = false;
        }

        // check if the supplied fs_currentfm is the same as the lowest level directory
        // supplied in the fs_devpath argument. if not, use the directory from the devpath
        if ( idStr::Icmp ( devfm.c_str(), fs_currentfm.GetString() ) == 0 ) {
            AddGameDirectory( devbase.c_str(), fs_currentfm.GetString(), FDOM_FM );
        } else {
            AddGameDirectory( devbase.c_str(), devfm.c_str(), FDOM_FM );
        }
    } else if ( addFsModSave ) {
        // devpath was not modified, so we can add fs_modSavePath with the current fm to the searchpath
        AddGameDirectory( fs_modSavePath.GetString(), fs_currentfm.GetString(), FDOM_FM );
    }

    // currently all addons are in the search list - deal with filtering out and dependencies now
	// scan through and deal with dependencies
	search = &searchPaths;
	while ( *search ) {
		if ( !( *search )->pack || !( *search )->pack->addon ) {
			search = &( ( *search )->next );
			continue;
		}
		pak = ( *search )->pack;
		if ( fs_searchAddons.GetBool() ) {
			// when we have fs_searchAddons on we should never have addonChecksums
			assert( !addonChecksums.Num() );
			pak->addon_search = true;
			search = &( ( *search )->next );
			continue;
		}
		addon_index = addonChecksums.FindIndex( pak->checksum );
		if ( addon_index >= 0 ) {
			assert( !pak->addon_search );	// any pak getting flagged as addon_search should also have been removed from addonChecksums already
			pak->addon_search = true;
			addonChecksums.RemoveIndex( addon_index );
			FollowAddonDependencies( pak );
		}
		search = &( ( *search )->next );
	}

	// now scan to filter out addons not marked addon_search
	search = &searchPaths;
	while ( *search ) {
		if ( !( *search )->pack || !( *search )->pack->addon ) {
			search = &( ( *search )->next );
			continue;
		}
		assert( !( *search )->dir );
		pak = ( *search )->pack;
		if ( pak->addon_search ) {
			common->Printf( "Addon pk4 %s with checksum 0x%x is on the search list\n", pak->pakFilename.c_str(), pak->checksum );
			search = &( ( *search )->next );
		} else {
			// remove from search list, put in addons list
			searchpath_t *paksearch = *search;
			*search = ( *search )->next;
			paksearch->next = addonPaks;
			addonPaks = paksearch;
			common->Printf( "Addon pk4 %s with checksum 0x%x is on addon list\n", pak->pakFilename.c_str(), pak->checksum );				
		}
	}

	// add our commands
	cmdSystem->AddCommand( "dir", Dir_f, CMD_FL_SYSTEM, "lists a folder", idCmdSystem::ArgCompletion_FileName );
	cmdSystem->AddCommand( "dirtree", DirTree_f, CMD_FL_SYSTEM, "lists a folder with subfolders" );
	cmdSystem->AddCommand( "path", Path_f, CMD_FL_SYSTEM, "lists search paths" );
	cmdSystem->AddCommand( "touchFile", TouchFile_f, CMD_FL_SYSTEM, "touches a file" );
	cmdSystem->AddCommand( "touchFileList", TouchFileList_f, CMD_FL_SYSTEM, "touches a list of files" );
	cmdSystem->AddCommand( "fstestthreads", TestThreads_f, CMD_FL_RENDERER, "deletes all currently loaded images" );

	// print the current search paths
	Path_f( idCmdArgs() );

	common->Printf( "File System Initialized.\n" );
	common->Printf( "--------------------------------------\n" );
}

/*
=====================
idFileSystemLocal::UpdateGamePakChecksums
=====================
*/
bool idFileSystemLocal::UpdateGamePakChecksums( void ) {
	searchpath_t	*search;
	fileInPack_t	*pakFile;
	int				confHash;
	idFile			*confFile;
	char			*buf;
	idLexer			*lexConf;
	idToken			token;
	int				id;

	confHash = HashFileName( BINARY_CONFIG );

	memset( gamePakForOS, 0, sizeof( gamePakForOS ) );
	for ( search = searchPaths; search; search = search->next ) {
		if ( !search->pack ) {
			continue;
		}

		search->pack->binary = BINARY_NO;
		for ( pakFile = search->pack->hashTable[confHash]; pakFile; pakFile = pakFile->next ) {
			if ( !FilenameCompare( pakFile->name, BINARY_CONFIG ) ) {
				search->pack->binary = BINARY_YES;
				confFile = ReadFileFromZip( search->pack, pakFile, BINARY_CONFIG );

				buf = new char[ confFile->Length() + 1 ];
				confFile->Read( (void *)buf, confFile->Length() );
				buf[ confFile->Length() ] = '\0';
				lexConf = new idLexer( buf, confFile->Length(), confFile->GetFullPath() );

				while ( lexConf->ReadToken( &token ) ) {
					if ( token.IsNumeric() ) {
						id = atoi( token );
						if ( id < MAX_GAME_OS && !gamePakForOS[ id ] ) {
							if ( fs_debug.GetBool() ) {
								common->Printf( "Adding game pak checksum for OS %d: %s 0x%x\n", id, confFile->GetFullPath(), search->pack->checksum );
							}
 							gamePakForOS[ id ] = search->pack->checksum;
						}
					}
				}

				CloseFile( confFile );
				delete lexConf;
				delete[] buf;
			}
		}
	}

	// some sanity checks on the game code references
	if ( !gamePakForOS[ BUILD_OS_ID ] ) {
		common->Warning( "No game code pak reference found for the local OS" );
		return false;
	}

	if ( !cvarSystem->GetCVarBool( "net_serverAllowServerMod" ) &&
		gamePakChecksum != gamePakForOS[ BUILD_OS_ID ] ) {
		common->Warning( "The current game code doesn't match pak files (net_serverAllowServerMod is off)" );
		return false;
	}

	return true;
}

/*
=====================
idFileSystemLocal::GetPackForChecksum
=====================
*/
pack_t* idFileSystemLocal::GetPackForChecksum( int checksum, bool searchAddons ) {
	searchpath_t *search = searchPaths;
	while ( search ) {
		if ( !search->pack ) {
			continue;
		} else if ( search->pack->checksum == checksum ) {
			return search->pack;
		} else {
			search = search->next;
		}
	}

	if ( searchAddons ) {
		search = addonPaks;
		while ( search && search->pack && search->pack->addon ) {
			if ( search->pack->checksum == checksum ) {
				return search->pack;
			} else {
				search = search->next;
			}
		}
	}

	return NULL;
}

/*
===============
idFileSystemLocal::ValidateDownloadPakForChecksum
===============
*/
int idFileSystemLocal::ValidateDownloadPakForChecksum( int checksum, char path[ MAX_STRING_CHARS ], bool isBinary ) {
	idScopedCriticalSection lock(globalMutex);

	int			i;
	idStrList	testList;
	idStr		name;
	idStr		relativePath;
	bool		pakBinary;
	pack_t		*pak = GetPackForChecksum( checksum );

	if ( !pak ) {
		return 0;
	}

	// validate this pak for a potential download
	// ignore pak*.pk4 for download. those are reserved to distribution and cannot be downloaded
	name = pak->pakFilename;
	name.StripPath();
	if ( strstr( name.c_str(), "pak" ) == name.c_str() ) {
		common->DPrintf( "%s is not a donwloadable pak\n", pak->pakFilename.c_str() );
		return 0;
	}

	// check the binary
	assert( pak->binary != BINARY_UNKNOWN );
	pakBinary = ( pak->binary == BINARY_YES ) ? true : false;
	if ( isBinary != pakBinary ) {
		common->DPrintf( "%s binary flag mismatch\n", pak->pakFilename.c_str() );
		return 0;
	}

	// extract a path that includes the fs_currentfm: != OSPathToRelativePath
	testList.Append( fs_savepath.GetString() );
	testList.Append( fs_devpath.GetString() );
	testList.Append( fs_basepath.GetString() );

	for ( i = 0; i < testList.Num(); i ++ ) {
		if ( testList[ i ].Length() && !testList[ i ].Icmpn( pak->pakFilename, testList[ i ].Length() ) ) {
			relativePath = pak->pakFilename.c_str() + testList[ i ].Length() + 1;
			break;
		}
	}

	if ( i == testList.Num() ) {
		common->Warning( "idFileSystem::ValidateDownloadPak: failed to extract relative path for %s", pak->pakFilename.c_str() );
		return 0;
	} else {
		idStr::Copynz( path, relativePath, MAX_STRING_CHARS );
		return pak->length;
	}
}

/*
================
idFileSystemLocal::Init

Called only at inital startup, not when the filesystem
is resetting due to a game change
================
*/
void idFileSystemLocal::Init( void ) {
	idScopedCriticalSection lock(globalMutex);

	// allow command line parms to override our defaults
	// we have to specially handle this, because normal command
	// line variable sets don't happen until after the filesystem
	// has already been initialized
	common->StartupVariable( "fs_basepath", false );
	common->StartupVariable( "fs_savepath", false );
	common->StartupVariable( "fs_devpath", false );
	common->StartupVariable( "fs_searchAddons", false );

    // taaaki: we have replaced fs_game and fs_game_base with these
    common->StartupVariable( "fs_mod", false ); 
	common->StartupVariable( "fs_currentfm", false );

	// greebo: even the mod save path can be overriden by command line arguments
	common->StartupVariable( "fs_modSavePath", false );

	if ( fs_basepath.GetString()[0] == '\0' ) {
		fs_basepath.SetString( Sys_DefaultBasePath() );
	}
	if ( fs_savepath.GetString()[0] == '\0' ) {
		fs_savepath.SetString( Sys_DefaultSavePath() );
	}

	if ( fs_devpath.GetString()[0] == '\0' ) {
        fs_devpath.SetString( Sys_DefaultSavePath() );
	}

	// greebo: By default, set the mod save path to BASE_TDM/fms/.
	// The exact location depends on the platform we're on
	if ( fs_modSavePath.GetString()[0] == '\0' ) {
		fs_modSavePath.SetString( Sys_ModSavePath() );
	}

	// try to start up normally
	Startup( );

	// spawn a thread to handle background file reads
    StartBackgroundDownloadThread( );

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	// Dedicated servers can run with no outside files at all
	if ( eventLoop->JournalLevel() != 2 &&	//stgatilov: ignore this pre-check when reading events from journal
		ReadFile( "default.cfg", NULL, NULL ) <= 0
	) {
		common->FatalError( "Couldn't load default.cfg" );
	}
}

/*
================
idFileSystemLocal::Restart
================
*/
void idFileSystemLocal::Restart( void ) {
	idScopedCriticalSection lock(globalMutex);

	// free anything we currently have loaded
	Shutdown( true );

	Startup( );

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( ReadFile( "default.cfg", NULL, NULL ) <= 0 ) {
		common->FatalError( "Couldn't load default.cfg" );
	}
}

/*
================
idFileSystemLocal::Shutdown

Frees all resources and closes all files
================
*/
void idFileSystemLocal::Shutdown( bool reloading ) {
	idScopedCriticalSection lock(globalMutex);

	searchpath_t *sp, *next, *loop;

	gameFolder.ClearFree();
	serverPaks.ClearFree();

	if ( !reloading ) {
		restartChecksums.ClearFree();
		addonChecksums.ClearFree();
	}

	loadedFileFromDir = false;
	gameDLLChecksum = 0;
	gamePakChecksum = 0;

	ClearDirCache();

	// free everything - loop through searchPaths and addonPaks
	for ( loop = searchPaths; loop; loop == searchPaths ? loop = addonPaks : loop = NULL ) {
		for ( sp = loop; sp; sp = next ) {
			next = sp->next;

			if ( sp->pack ) {
				unzClose( sp->pack->handle );
				delete [] sp->pack->buildBuffer;
				if ( sp->pack->addon_info ) {
					sp->pack->addon_info->mapDecls.DeleteContents( true );
					delete sp->pack->addon_info;
				}
				delete sp->pack;
			}
			if ( sp->dir ) {
				delete sp->dir;
			}
			delete sp;
		}
	}

	// any FS_ calls will now be an error until reinitialized
	searchPaths = NULL;
	addonPaks = NULL;

	cmdSystem->RemoveCommand( "path" );
	cmdSystem->RemoveCommand( "dir" );
	cmdSystem->RemoveCommand( "dirtree" );
	cmdSystem->RemoveCommand( "touchFile" );

	mapDict.ClearFree();
}

/*
================
idFileSystemLocal::IsInitialized
================
*/
bool idFileSystemLocal::IsInitialized( void ) const {
	return ( searchPaths != NULL );
}

/*
=================================================================================

Opening files

=================================================================================
*/

/*
===========
idFileSystemLocal::FileAllowedFromDir
===========
*/
bool idFileSystemLocal::FileAllowedFromDir( const char *path ) {

    const unsigned int l = static_cast<unsigned int>(strlen(path));

	if  (  !strcmp( path + l - 4, ".cfg" )		// for config files
		|| !strcmp( path + l - 4, ".dat" )		// for journal files
		|| !strcmp( path + l - 4, ".dll" )		// dynamic modules are handled a different way for pure
		|| !strcmp( path + l - 3, ".so" )
		||(!strcmp( path + l - 6, ".dylib" ) && l > 6 )
		||(!strcmp( path + l - 10, ".scriptcfg") && l > 10 )	// configuration script, such as map cycle
		|| !strcmp( path + l - 4, ".dds" )
		) {
		// note: config.spec are opened through an explicit OS path and don't hit this
		return true;
	}
	// savegames
	if ( strstr( path, "savegames" ) == path &&
		( !strcmp( path + l - 4, ".tga" ) || !strcmp( path + l - 4, ".txt" ) || !strcmp( path + l - 5, ".save" ) ) ) {
		return true;
	}
	// screen shots
	if ( strstr( path, "screenshots" ) == path && 
        ( !strcmp( path + l - 4, ".tga" ) || !strcmp( path + l - 4, ".jpg" ) || 
          !strcmp( path + l - 4, ".png" ) || !strcmp( path + l - 4, ".bmp" ) ) ) {
		return true;
	}

#if 0 // likely not used in TDM
	// objective tgas
	if ( strstr( path, "maps/game" ) == path && 
		!strcmp( path + l - 4, ".tga" ) ) {
		return true;
	}
	// splash screens extracted from addons
	if ( strstr( path, "guis/assets/splash/addon" ) == path &&
		 !strcmp( path + l -4, ".tga" ) ) {
		return true;
	}
#endif

	return false;
}

/*
===========
idFileSystemLocal::ReadFileFromZip
===========
*/
idFile_InZip * idFileSystemLocal::ReadFileFromZip( pack_t *pak, fileInPack_t *pakFile, const char *relativePath ) {
	// relativePath == pakFile->name according to FilenameCompare()
	// pakFile->Pos is position of that file within the zip

	// set position in pk4 file to the file (in the zip/pk4) we want a handle on
	unzSetOffset64( pak->handle, pakFile->pos );

	// clone handle and assign a new internal filestream to zip file to it
	unzFile uf = unzReOpen( pak->pakFilename, pak->handle );
	if ( uf == NULL ) {
		common->FatalError( "ReadFileFromZip: Couldn't reopen %s", pak->pakFilename.c_str() );
	}

	// the following stuff is needed to get the uncompress filesize (for file->fileSize)
	char	filename_inzip[MAX_ZIPPED_FILE_NAME];
	unz_file_info64	file_info;
	int err = unzGetCurrentFileInfo64( uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0 );
	if ( err != UNZ_OK ) {
		common->FatalError( "Couldn't get file info for %s in %s, pos %llu", relativePath, pak->pakFilename.c_str(), pakFile->pos );
	}

	// create idFile_InZip and set fields accordingly
	idFile_InZip *file = new idFile_InZip();
	file->z = uf;
	file->name = relativePath;
	file->compressed = (file_info.compression_method != 0);
	file->fullPath = pak->pakFilename + "/" + relativePath;
	file->zipFilePos = pakFile->pos;
	file->fileSize = file_info.uncompressed_size;

	return file;
}

/*
===========
idFileSystemLocal::OpenFileReadFlags

Finds the file in the search path, following search flag recommendations
Returns filesize and an open FILE pointer.
Used for streaming data out of either a
separate file or a ZIP file.
===========
*/
idFile *idFileSystemLocal::OpenFileReadFlags( const char *relativePath, int searchFlags, pack_t **foundInPak, const char* gamedir ) {
	searchpath_t *	search;
	idStr			netpath;
	pack_t *		pak;
	fileInPack_t *	pakFile;
	directory_t *	dir;
	int			hash;
	FILE *			fp;
	
	if ( !IsInitialized() ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	} else if ( !relativePath ) {
		common->FatalError( "idFileSystemLocal::OpenFileRead: NULL 'relativePath' parameter passed\n" );
	} else if ( relativePath[0] == '\0' ) { // edge case
		// FIXME : Serp - This seems to happen a lot - find out why and see if its fixable
		common->Warning("idFileSystemLocal::OpenFileRead: Relative path was empty");
		return NULL;
	} else if ( relativePath[0] == '/' || relativePath[0] == '\\' ) { // paths are not supposed to have a leading slash
		relativePath++;
	}

	if ( foundInPak ) {
		*foundInPak = NULL;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo" 
	if ( strstr( relativePath, ".." ) || strstr( relativePath, "::" ) ) {
		return NULL;
	}
	
	// search through the path, one element at a time
	hash = HashFileName( relativePath );

	for ( search = searchPaths; search; search = search->next ) {
		if ( search->dir && ( searchFlags & FSFLAG_SEARCH_DIRS ) ) {
			// check a file in the directory tree

			// if we are running restricted, the only files we
			// will allow to come from the directory are .cfg files
			if ( serverPaks.Num() ) {
				if ( !FileAllowedFromDir( relativePath ) ) {
					continue;
				}
            }

			dir = search->dir;

			if(gamedir && strlen(gamedir)) {
				if(dir->gamedir != gamedir) {
					continue;
				}
			}
			
			netpath = BuildOSPath( dir->path, dir->gamedir, relativePath );
			fp = OpenOSFileCorrectName( netpath, "rb" );
			if ( !fp ) {
				continue;
			}

			idFile_Permanent *file = new idFile_Permanent();
			file->o = fp;
			file->name = relativePath;
			file->fullPath = netpath;
			file->mode = ( 1 << FS_READ );
			file->fileSize = DirectFileLength( file->o );
			file->domain = search->domain;
			if ( fs_debug.GetInteger() ) {
				common->Printf( "idFileSystem::OpenFileRead: %s (found in '%s/%s')\n", relativePath, dir->path.c_str(), dir->gamedir.c_str() );
			}

			return file;
		} else if ( search->pack && ( searchFlags & FSFLAG_SEARCH_PAKS ) ) {

			if ( !search->pack->hashTable[hash] ) {
				continue;
			}

			// look through all the pak file elements
			pak = search->pack;

			if ( searchFlags & FSFLAG_BINARY_ONLY ) {
				// make sure this pak is tagged as a binary file
				if ( pak->binary == BINARY_UNKNOWN ) {
					const int confHash = HashFileName( BINARY_CONFIG );
					pak->binary = BINARY_NO;
					for ( pakFile = search->pack->hashTable[confHash]; pakFile; pakFile = pakFile->next ) {
						if ( !FilenameCompare( pakFile->name, BINARY_CONFIG ) ) {
							pak->binary = BINARY_YES;
							break;
						}
					}
				}
				if ( pak->binary == BINARY_NO ) {
					continue; // not a binary pak, skip
				}
			}

			for ( pakFile = pak->hashTable[hash]; pakFile; pakFile = pakFile->next ) {
				// case and separator insensitive comparisons
				if ( !FilenameCompare( pakFile->name, relativePath ) ) {
					idFile_InZip *file = ReadFileFromZip( pak, pakFile, relativePath );
					file->domain = search->domain;

					if ( foundInPak ) {
						*foundInPak = pak;
					}

					if ( !pak->referenced ) {
						// mark this pak referenced
						if ( fs_debug.GetInteger( ) ) {
							common->Printf( "idFileSystem::OpenFileRead: %s -> adding %s to referenced paks\n", relativePath, pak->pakFilename.c_str() );
						}
						pak->referenced = true;
					}

					if ( fs_debug.GetInteger( ) ) {
						common->Printf( "idFileSystem::OpenFileRead: %s (found in '%s')\n", relativePath, pak->pakFilename.c_str() );
					}
					return file;
				}
			}
		}
	}

	if ( searchFlags & FSFLAG_SEARCH_ADDONS ) {
		search = addonPaks;
		while ( search && search->pack ) {
			pak = search->pack;
			for ( pakFile = pak->hashTable[hash]; pakFile; pakFile = pakFile->next ) {
				if ( !FilenameCompare( pakFile->name, relativePath ) ) {
					idFile_InZip *file = ReadFileFromZip( pak, pakFile, relativePath );
					if ( foundInPak ) {
						*foundInPak = pak;
					}
#ifdef _DEBUG
					if ( fs_debug.GetInteger( ) ) {
						common->Printf( "idFileSystem::OpenFileRead: %s (found in addon pk4 '%s')\n", relativePath, search->pack->pakFilename.c_str() );
					}
#endif
					return file;
				}
			}
			search = search->next;
		}
	}
	
	if ( fs_debug.GetInteger( ) ) {
		common->Printf( "Can't find %s\n", relativePath );
	}
	
	return NULL;
}

/*
===========
idFileSystemLocal::OpenFileRead
===========
*/
idFile *idFileSystemLocal::OpenFileRead( const char *relativePath, const char* gamedir ) {
	idScopedCriticalSection lock(globalMutex);
    return OpenFileReadFlags( relativePath, FSFLAG_SEARCH_DIRS | FSFLAG_SEARCH_PAKS, NULL, gamedir );
}

idFile * idFileSystemLocal::OpenFileReadPrefetch( const char *relativePath, const char *gamedir ) {
	idFile *f = OpenFileRead( relativePath, gamedir );
	if ( f == nullptr ) {
		return f;
	}
	ID_TIME_T timestamp = f->Timestamp();
	int len = f->Length();
	void *buffer = Mem_Alloc( len );
	f->Read( buffer, len );
	CloseFile( f );
	idFile_Memory *res = new idFile_Memory( relativePath, (const char *)buffer, len, true );
	res->SetTimestamp(timestamp);
	return res;
}

/*
===========
idFileSystemLocal::OpenFileWrite
===========
*/
idFile *idFileSystemLocal::OpenFileWrite( const char *relativePath, const char *basePath, const char *gamedir ) {
	idScopedCriticalSection lock(globalMutex);

	const char *path;
	idStr OSpath;
	idFile_Permanent *f;

	if ( !searchPaths ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	}

	path = cvarSystem->GetCVarString( basePath );
	if ( !path[0] ) {
		path = fs_savepath.GetString();
	}

	OSpath = BuildOSPath( path, gamedir ? gamedir : gameFolder.c_str(), relativePath );

	if ( fs_debug.GetInteger() ) {
		common->Printf( "idFileSystem::OpenFileWrite: %s\n", OSpath.c_str() );
	}

	// if the dir we are writing to is in our current list, it will be outdated
	// so just flush everything
	ClearDirCache();

	common->DPrintf( "writing to: %s\n", OSpath.c_str() );
	CreateOSPath( OSpath );

	f = new idFile_Permanent();
	f->o = OpenOSFile( OSpath, "wb" );
	if ( !f->o ) {
		delete f;
		return NULL;
	}
	f->name = relativePath;
	f->fullPath = OSpath;
	f->mode = ( 1 << FS_WRITE );
	f->handleSync = false;
	f->fileSize = 0;

	return f;
}

/*
===========
idFileSystemLocal::OpenExplicitFileRead
===========
*/
idFile *idFileSystemLocal::OpenExplicitFileRead( const char *OSPath ) {
	//no lock required here...

	idFile_Permanent *f;

	if ( !IsInitialized() ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	}

	if ( fs_debug.GetInteger() ) {
		common->Printf( "idFileSystem::OpenExplicitFileRead: %s\n", OSPath );
	}

	common->DPrintf( "idFileSystem::OpenExplicitFileRead - reading from: %s\n", OSPath );

	f = new idFile_Permanent();
	f->o = OpenOSFile( OSPath, "rb" );
	if ( !f->o ) {
		delete f;
		return NULL;
	}
	f->name = OSPath;
	f->fullPath = OSPath;
	f->mode = ( 1 << FS_READ );
	f->handleSync = false;
	f->fileSize = DirectFileLength( f->o );

	return f;
}

/*
===========
idFileSystemLocal::OpenExplicitFileWrite
===========
*/
idFile *idFileSystemLocal::OpenExplicitFileWrite( const char *OSPath ) {
	//no lock required here...
	idFile_Permanent *f;

	if ( !searchPaths ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	}

	if ( fs_debug.GetInteger() ) {
		common->Printf( "idFileSystem::OpenExplicitFileWrite: %s\n", OSPath );
	}

	common->DPrintf( "writing to: %s\n", OSPath );
	CreateOSPath( OSPath );

	f = new idFile_Permanent();
	f->o = OpenOSFile( OSPath, "wb" );
	if ( !f->o ) {
		delete f;
		return NULL;
	}
	f->name = OSPath;
	f->fullPath = OSPath;
	f->mode = ( 1 << FS_WRITE );
	f->handleSync = false;
	f->fileSize = 0;

	return f;
}

/*
===========
idFileSystemLocal::OpenFileAppend
===========
*/
idFile *idFileSystemLocal::OpenFileAppend( const char *relativePath, bool sync, const char *basePath, const char *gamedir ) {
	idScopedCriticalSection lock(globalMutex);	//actually, I think no lock required here

	const char *path;
	idStr OSpath;
	idFile_Permanent *f;

	if ( !IsInitialized() ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	}

	path = cvarSystem->GetCVarString( basePath );
	if ( !path[0] ) {
		path = fs_savepath.GetString();
	}

	OSpath = BuildOSPath( path, gamedir ? gamedir : gameFolder.c_str(), relativePath );
	CreateOSPath( OSpath );

	if ( fs_debug.GetInteger() ) {
		common->Printf( "idFileSystem::OpenFileAppend: %s\n", OSpath.c_str() );
	}

	f = new idFile_Permanent();
	f->o = OpenOSFile( OSpath, "ab" );
	if ( !f->o ) {
		delete f;
		return NULL;
	}
	f->name = relativePath;
	f->fullPath = OSpath;
	f->mode = ( 1 << FS_WRITE ) + ( 1 << FS_APPEND );
	f->handleSync = sync;
	f->fileSize = DirectFileLength( f->o );

	return f;
}

/*
================
idFileSystemLocal::OpenFileByMode
================
*/
idFile *idFileSystemLocal::OpenFileByMode( const char *relativePath, fsMode_t mode ) {
	if ( mode == FS_READ ) {
		return OpenFileRead( relativePath );
	}
	else if ( mode == FS_WRITE ) {
		return OpenFileWrite( relativePath );
	}
	else if ( mode == FS_APPEND ) {
		return OpenFileAppend( relativePath, true );
	} else {
		common->FatalError( "idFileSystemLocal::OpenFileByMode: bad mode : %i", mode );
		return NULL;
	}
}

/*
==============
idFileSystemLocal::CloseFile
==============
*/
void idFileSystemLocal::CloseFile( idFile *f ) {
	if ( !searchPaths ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	}

	delete f;
}

/*
=================================================================================

back ground loading

=================================================================================
*/

/*
=================
idFileSystemLocal::CurlWriteFunction
=================
*/
size_t idFileSystemLocal::CurlWriteFunction( void *ptr, size_t size, size_t nmemb, void *stream ) {
	backgroundDownload_t *bgl = (backgroundDownload_t *)stream;
	if ( !bgl->f ) {
		return size * nmemb;
	}
	return fwrite( ptr, size, nmemb, static_cast<idFile_Permanent*>(bgl->f)->GetFilePtr() );
}

/*
=================
idFileSystemLocal::CurlProgressFunction
=================
*/
int idFileSystemLocal::CurlProgressFunction( void *clientp, double dltotal, double dlnow, double ultotal, double ulnow ) {
	backgroundDownload_t *bgl = (backgroundDownload_t *)clientp;

	if ( bgl->url.status == DL_ABORTING ) {
		return 1;
	} else { 
		bgl->url.dltotal = dltotal;
		bgl->url.dlnow = dlnow;
		return 0;
	}
}

/*
===================
BackgroundDownload

Reads part of a file from a background thread.
===================
*/
void BackgroundDownloadThread(void *parms) {
	while( 1 ) {
		Sys_EnterCriticalSection();
		backgroundDownload_t	*bgl = fileSystemLocal.backgroundDownloads;
		if ( !bgl ) {
			Sys_LeaveCriticalSection();
			Sys_WaitForEvent();
			continue;
		}
		// remove this from the list
		fileSystemLocal.backgroundDownloads = bgl->next;
		Sys_LeaveCriticalSection();

		bgl->next = NULL;

		if ( bgl->opcode == DLTYPE_FILE ) {
			// use the low level read function, because fread may allocate memory
			size_t res = fread( bgl->file.buffer, bgl->file.length, 1, static_cast<idFile_Permanent*>(bgl->f)->GetFilePtr() );
			bgl->completed = true;
		} else {
#if ID_ENABLE_CURL
			// DLTYPE_URL
			// use a local buffer for curl error since the size define is local
			char error_buf[ CURL_ERROR_SIZE ];
			bgl->url.dlerror[ 0 ] = '\0';
			CURL *session = ExtLibs::curl_easy_init();
			CURLcode ret;
			if ( !session ) {
				bgl->url.dlstatus = CURLE_FAILED_INIT;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			ret = ExtLibs::curl_easy_setopt( session, CURLOPT_ERRORBUFFER, error_buf );
			if ( ret ) {
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			ret = ExtLibs::curl_easy_setopt( session, CURLOPT_URL, bgl->url.url.c_str() );
			if ( ret ) {
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			ret = ExtLibs::curl_easy_setopt( session, CURLOPT_FAILONERROR, 1 );
			if ( ret ) {
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			ret = ExtLibs::curl_easy_setopt( session, CURLOPT_WRITEFUNCTION, idFileSystemLocal::CurlWriteFunction );
			if ( ret ) {
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			ret = ExtLibs::curl_easy_setopt( session, CURLOPT_WRITEDATA, bgl );
			if ( ret ) {
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			ret = ExtLibs::curl_easy_setopt( session, CURLOPT_NOPROGRESS, 0 );
			if ( ret ) {
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			ret = ExtLibs::curl_easy_setopt( session, CURLOPT_PROGRESSFUNCTION, idFileSystemLocal::CurlProgressFunction );
			if ( ret ) {
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			ret = ExtLibs::curl_easy_setopt( session, CURLOPT_PROGRESSDATA, bgl );
			if ( ret ) {
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			bgl->url.dlnow = 0;
			bgl->url.dltotal = 0;
			bgl->url.status = DL_INPROGRESS;
			ret = ExtLibs::curl_easy_perform( session );
			if ( ret ) {
				Sys_Printf( "curl_easy_perform failed: %s\n", error_buf );
				idStr::Copynz( bgl->url.dlerror, error_buf, MAX_STRING_CHARS );
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			bgl->url.status = DL_DONE;
			bgl->completed = true;
#else
			bgl->url.status = DL_FAILED;
			bgl->completed = true;
#endif
		}
	}
}

/*
=================
idFileSystemLocal::StartBackgroundReadThread
=================
*/
void idFileSystemLocal::StartBackgroundDownloadThread() {
	if ( !backgroundThread ) {
		backgroundThread = Sys_CreateThread( (xthread_t)BackgroundDownloadThread, NULL, THREAD_NORMAL, "Background Download" );
		if ( !backgroundThread ) {
			common->Warning( "idFileSystemLocal::StartBackgroundDownloadThread: failed" );
		}
	} else {
		common->Printf( "background thread already running\n" );
	}
}

/*
=================
idFileSystemLocal::BackgroundDownload
=================
*/
void idFileSystemLocal::BackgroundDownload( backgroundDownload_t *bgl ) {
	if ( bgl->opcode == DLTYPE_FILE ) {
		if ( dynamic_cast<idFile_Permanent *>(bgl->f) ) {
			// add the bgl to the background download list
			Sys_EnterCriticalSection();
			bgl->next = backgroundDownloads;
			backgroundDownloads = bgl;
			Sys_TriggerEvent();
			Sys_LeaveCriticalSection();
		} else if ( bgl->f ) {
			// read zipped file directly
			bgl->f->Seek( bgl->file.position, FS_SEEK_SET );
			bgl->f->Read( bgl->file.buffer, bgl->file.length );
			bgl->completed = true;
		}
	} else {
		Sys_EnterCriticalSection();
		bgl->next = backgroundDownloads;
		backgroundDownloads = bgl;
		Sys_TriggerEvent();
		Sys_LeaveCriticalSection();
	}
}

/*
=================
idFileSystemLocal::FindPakForFileChecksum
=================
*/
pack_t *idFileSystemLocal::FindPakForFileChecksum( const char *relativePath, int findChecksum, bool bReference ) {
	searchpath_t	*search;
	pack_t			*pak;
	fileInPack_t	*pakFile;
	int				hash;

	assert( !serverPaks.Num() );

	hash = HashFileName( relativePath );
	for ( search = searchPaths; search; search = search->next ) {
		if ( search->pack && search->pack->hashTable[ hash ] ) {
			pak = search->pack;
			for ( pakFile = pak->hashTable[ hash ]; pakFile; pakFile = pakFile->next ) {
				if ( !FilenameCompare( pakFile->name, relativePath ) ) {
					idFile_InZip *file = ReadFileFromZip( pak, pakFile, relativePath );
					if ( findChecksum == GetFileChecksum( file ) ) {
						if ( fs_debug.GetBool() ) {
							common->Printf( "found '%s' with checksum 0x%x in pak '%s'\n", relativePath, findChecksum, pak->pakFilename.c_str() );
						}
						if ( bReference ) {
							pak->referenced = true;
							// FIXME: use dependencies for pak references
						}
						CloseFile( file );
						return pak;
					} else if ( fs_debug.GetBool() ) {
						common->Printf( "'%s' in pak '%s' has != checksum %x\n", relativePath, pak->pakFilename.c_str(), GetFileChecksum( file ) );
					}
					CloseFile( file );
				}
			}
		}
	}

	if ( fs_debug.GetBool() ) {
		common->Printf( "no pak file found for '%s' checksumed %x\n", relativePath, findChecksum );
	}

	return NULL;
}

/*
=================
idFileSystemLocal::GetFileChecksum
=================
*/
int idFileSystemLocal::GetFileChecksum( idFile *file ) {
	byte *buf;

	file->Seek( 0, FS_SEEK_END );
	const int len = file->Tell();
	file->Seek( 0, FS_SEEK_SET );
	buf = (byte *)Mem_Alloc( len );

	if ( file->Read( buf, len ) != len ) {
		Mem_Free( buf );
		common->FatalError( "Short read in idFileSystemLocal::GetFileChecksum()\n" );
	}

	const int ret = MD4_BlockChecksum( buf, len );
	Mem_Free( buf );

	return ret;
}

/*
=================
idFileSystemLocal::FindDLL
=================
*/
void idFileSystemLocal::FindDLL( const char *name, char _dllPath[ MAX_OSPATH ], bool updateChecksum ) {
	idScopedCriticalSection lock(globalMutex);

	idFile			*dllFile = NULL;
    idFile          *dllFileInPak = NULL;
	char			dllName[MAX_OSPATH];
	idStr			dllPath;
	int				dllHash;
	pack_t			*inPak = NULL;
	pack_t			*pak;
    time_t          timePak = 0, timeDll = 0;

#ifdef WIN32
    #define GAMEEXT "DLL"
    struct _utimbuf newTimes;
#else
    #define GAMEEXT "SO"
    struct utimbuf newTimes;
#endif

	sys->DLL_GetFileName( name, dllName, MAX_OSPATH );
	dllHash = HashFileName( dllName );

	//stgatilov #5042: timestamps of files in zip packages are no longer read
	//they are all zero now, just as in the original Doom 3
	//hence, this method can be seriously broken now...
	common->Warning("idFileSystemLocal::FindDLL: timestamps of files inside pk4 are no longer extracted");

    // try locate a game dll/so in the executable directory as well as in a pak file.
    // compare the last modified timestamps and use the latest version of the game dll/so.
	// place dev game dll/so in executable directory for development testing.
	dllPath = Sys_EXEPath( );
	dllPath.StripFilename( );
	dllPath.AppendPath( dllName );
	
	common->Printf("Searching for DLL in %s", dllPath.c_str());

	dllFile = OpenExplicitFileRead( dllPath );

    if ( !serverPaks.Num() ) {
		// try to find dll in a pak file
		dllFileInPak = OpenFileReadFlags( dllName, FSFLAG_SEARCH_PAKS | FSFLAG_BINARY_ONLY, &inPak );
    }

	if ( dllFile ) {
        timeDll = dllFile->Timestamp();
		common->Printf( "gamex86 - Found %s in EXE path with timestamp of %d - %s\n", GAMEEXT, int(timeDll), dllFile->GetFullPath() ); 
	} else {
        common->Printf( "gamex86 - No %s found in EXE path\n", GAMEEXT ); 
    }
    
    if ( dllFileInPak ) {
        timePak = dllFileInPak->Timestamp();
		common->Printf( "gamex86 - Found %s in pak file with timestamp of %d - %s\n", GAMEEXT, int(timePak), dllFileInPak->GetFullPath() );
    } else {
        common->Printf( "gamex86 - No %s found in pak file\n", GAMEEXT );
    }

    if ( dllFile && (dllFileInPak == NULL || (dllFileInPak && timeDll >= timePak)) ) {
        common->Printf( "gamex86 - %s in EXE path is newer, ignoring %s in pak file\n", GAMEEXT, GAMEEXT );
        if ( dllFileInPak ) {
            CloseFile( dllFileInPak );
		    dllFileInPak = NULL;
        }
    } else { 
		if ( dllFileInPak ) {
			common->Printf( "gamex86 - %s in pak file is newer, extracting to darkmod path\n", GAMEEXT );
			
            if ( dllFile ) {
                CloseFile( dllFile );
		        dllFile = NULL;
            }

			dllPath = ModPath();
			dllPath.Append(dllName);

			//dllPath = RelativePathToOSPath( dllName, "fs_savepath" );
			CopyFile( dllFileInPak, dllPath );

			CloseFile( dllFileInPak );
            
            // the file modification time needs to be set to that of the file in the pk4 otherwise the modification time is
            // set to when the file was extracted. this causes issues when new gamedll updates are downloaded since there is
            // a possibility that an old dll will appear to be newer than the updated dll in the new pk4 file
            
            newTimes.actime = time(NULL);     // set atime to current time
            newTimes.modtime = timePak;       // set mtime to the file time in the pk4
            
#ifdef WIN32            
            if ( _utime( dllPath.c_str(), &newTimes ) < 0 ) {
#else
            if ( utime( dllPath.c_str(), &newTimes ) < 0 ) {
#endif
                common->Warning( "gamex86 - Update of file modification time failed. This may cause issues following a TDM update." );
            } else {
                common->Printf( "gamex86 - Correction of file modification time successful\n" );
            }

			dllFile = OpenFileReadFlags( dllName, FSFLAG_SEARCH_DIRS );

			if ( !dllFile ) {
				common->Error( "%s extraction to darkmod path failed\n", GAMEEXT );
			} else if ( updateChecksum ) {
				gameDLLChecksum = GetFileChecksum( dllFile );
				gamePakChecksum = inPak->checksum;
				updateChecksum = false;	// don't try again below
			}
		} else {
			// didn't find a source in a pak file, try in the directory
			dllFile = OpenFileReadFlags( dllName, FSFLAG_SEARCH_DIRS );
			if ( dllFile ) {
				if ( updateChecksum ) {
					gameDLLChecksum = GetFileChecksum( dllFile );
					// see if we can mark a pak file
					pak = FindPakForFileChecksum( dllName, gameDLLChecksum, false );
					pak ? gamePakChecksum = pak->checksum : gamePakChecksum = 0;
					updateChecksum = false;
				}
			}
		}
	}

	if ( updateChecksum ) {
		if ( dllFile ) {
			gameDLLChecksum = GetFileChecksum( dllFile );
		} else {
			gameDLLChecksum = 0;
		}
		gamePakChecksum = 0;
	}
	if ( dllFile ) {
		dllPath = dllFile->GetFullPath( );
		CloseFile( dllFile );
		dllFile = NULL;
	} else {
		dllPath = "";
	}
	idStr::snPrintf( _dllPath, MAX_OSPATH, "%s", dllPath.c_str() );
}

/*
================
idFileSystemLocal::ClearDirCache
================
*/
void idFileSystemLocal::ClearDirCache( void ) const {
	idScopedCriticalSection lock(dir_cache_mutex);
	dir_cache_index = 0;
	dir_cache_count = 0;
	for( int i = 0; i < MAX_CACHED_DIRS; i++ ) {
		dir_cache[ i ].Clear();
	}
}

/*
===============
idFileSystemLocal::MakeTemporaryFile
===============
*/
idFile * idFileSystemLocal::MakeTemporaryFile( void ) {
	//no lock required =)
	FILE *f = tmpfile();
#ifdef __ANDROID__ //karin: tmpfile always return NULL on Android
    if(!f)
        f = Sys_tmpfile();
#endif
	if ( f ) {
		idFile_Permanent *file = new idFile_Permanent();
		file->o = f;
		file->name = "<tempfile>";
		file->fullPath = "<tempfile>";
		file->mode = ( 1 << FS_READ ) + ( 1 << FS_WRITE );
		file->fileSize = 0;
		return file;
	} else {
		common->Warning( "idFileSystem::MakeTemporaryFile failed: %s", strerror( errno ) );
		return NULL;
	}
}

/*
===============
idFileSystemLocal::FindFile
===============
*/
 findFile_t idFileSystemLocal::FindFile( const char *path, bool scheduleAddons ) {
	idScopedCriticalSection lock(globalMutex);

	pack_t *pak;
	idFile *f = OpenFileReadFlags( path, FSFLAG_SEARCH_DIRS | FSFLAG_SEARCH_PAKS | FSFLAG_SEARCH_ADDONS, &pak );
	if ( !f ) {
		return FIND_NO;
	}
	else if ( !pak ) {
		// found in FS, not even in paks
		return FIND_YES;
	}
	// marking addons for inclusion on reload - may need to do that even when already in the search path
	if ( scheduleAddons && pak->addon && addonChecksums.FindIndex( pak->checksum ) < 0 ) {
		addonChecksums.Append( pak->checksum );			
	}
	// an addon that's not on search list yet? that will require a restart
	if ( pak->addon && !pak->addon_search ) {
		delete f;
		return FIND_ADDON;
	}

	delete f;
	return FIND_YES;
}

/*
===============
idFileSystemLocal::GetNumMaps
account for actual decls and for addon maps
===============
*/
int idFileSystemLocal::GetNumMaps() {
	idScopedCriticalSection lock(globalMutex);

	searchpath_t	*search = NULL;
	int				ret = declManager->GetNumDecls( DECL_MAPDEF );
	
	// add to this all addon decls - coming from all addon packs ( searched or not )
	for ( int i = 0; i < 2; i++ ) {
		if ( i == 0 ) {
			search = searchPaths;
		} else if ( i == 1 ) {
			search = addonPaks;
		}
		for ( ; search ; search = search->next ) {
			if ( !search->pack || !search->pack->addon || !search->pack->addon_info ) {
				continue;
			}
			ret += search->pack->addon_info->mapDecls.Num();
		}
	}
	return ret;
}

/*
===============
idFileSystemLocal::GetMapDecl
retrieve the decl dictionary, add a 'path' value
===============
*/
const idDict * idFileSystemLocal::GetMapDecl( int idecl ) {
	idScopedCriticalSection lock(globalMutex);

	const idDecl			*mapDecl;
	const idDeclEntityDef	*mapDef;
	const int				numdecls = declManager->GetNumDecls( DECL_MAPDEF );
	searchpath_t			*search = NULL;
	
	if ( idecl < numdecls ) {
		mapDecl = declManager->DeclByIndex( DECL_MAPDEF, idecl );
		mapDef = static_cast<const idDeclEntityDef *>( mapDecl );
		if ( !mapDef ) {
			common->Error( "idFileSystemLocal::GetMapDecl %d: not found\n", idecl );
			return NULL;
		} else {
			mapDict = mapDef->dict;
			mapDict.Set( "path", mapDef->GetName() );
			return &mapDict;
		}
	}

	idecl -= numdecls;
	for ( int i = 0; i < 2; i++ ) {
		if ( i == 0 ) {
			search = searchPaths;
		} else if ( i == 1 ) {
			search = addonPaks;
		}
		for ( ; search ; search = search->next ) {
			if ( !search->pack || !search->pack->addon || !search->pack->addon_info ) {
				continue;
			}
			// each addon may have a bunch of map decls
			if ( idecl < search->pack->addon_info->mapDecls.Num() ) {
				mapDict = *search->pack->addon_info->mapDecls[ idecl ];
				return &mapDict;
			}
			idecl -= search->pack->addon_info->mapDecls.Num();
			assert( idecl >= 0 );
		}
	}

	return NULL;
}

/*
===============
idFileSystemLocal::FindMapScreenshot
===============
*/
void idFileSystemLocal::FindMapScreenshot( const char *path, char *buf, int len ) {
	idScopedCriticalSection lock(globalMutex);

	idFile	*file;
	idStr	mapname = path;

	mapname.StripPath();
	mapname.StripFileExtension();
	
	idStr::snPrintf( buf, len, "guis/assets/splash/%s.tga", mapname.c_str() );
	if ( ReadFile( buf, NULL, NULL ) == -1 ) {
		// try to extract from an addon
		file = OpenFileReadFlags( buf, FSFLAG_SEARCH_ADDONS );
		if ( file ) {
			// save it out to an addon splash directory
			const int dlen = file->Length();
			char *data = new char[ dlen ];
			file->Read( data, dlen );
			CloseFile( file );
			idStr::snPrintf( buf, len, "guis/assets/splash/addon/%s.tga", mapname.c_str() );
			WriteFile( buf, data, dlen );
			delete[] data;
		} else {
			idStr::Copynz( buf, "guis/assets/splash/pdtempa", len );
		}
	}
}

const char* idFileSystemLocal::ModPath() const {
    // basepath = something like c:\games\tdm, modBaseName is usually darkmod
	thread_local static char path[MAX_STRING_CHARS];
	path[0] = '\0';

	const char* modPath = fileSystem->BuildOSPath(cvarSystem->GetCVarString("fs_savepath"), "", "");

	if (modPath != NULL && modPath[0] != '\0') {
		idStr::Copynz(path, modPath, sizeof(path));
	}

	return path;
}
