// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#if defined( _DEBUG )
#define SD_LOAD_STATISTICS
#endif

#include "../libs/filelib/File.h"

/*class sdFileIOLog {
public:
	void					PrintFileHandles();

	FILE*					fopen( const char* filename, const char* mode );
	int						fclose( FILE* stream );

private:
	struct fileHandle_t {
		FILE*	stream;
		idStr	fileName;
	};

	void					AddFileHandle( fileHandle_t& handle );
	void					RemoveFileHandle( FILE* stream );

private:
	idList< fileHandle_t >	fileHandles;
};

extern sdFileIOLog* fileIOLog;*/

#if defined ( SVN_LOCK )
struct svnLock_t {
	bool	created;
	char	osPath[ MAX_STRING_CHARS ];
};

extern svnLock_t svnLock;
#endif /* SVN_LOCK */

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
	"fs_savepath"		path to extracted dlls, downloaded game data, etc. files read & write
	"fs_userpath"		path to config, save game, etc. files, read & write
	"fs_cdpath"			path to cd, read-only
	"fs_devpath"		path to files created during development, read & write

	The base path for file saving can be set to "fs_savepath" or "fs_devpath".

===============================================================================
*/

const unsigned	FILE_NOT_FOUND_TIMESTAMP	= 0xFFFFFFFF;
const int		MAX_PURE_PAKS				= 128;
const int		MAX_OSPATH					= 256;

typedef enum {
	PURE_OK,		// we are good to connect as-is
	PURE_RESTART,	// restart required
	PURE_MISSING,	// pak files missing on the client
	PURE_NODLL		// no DLL could be extracted
} fsPureReply_t;

typedef enum {
	DLTYPE_URL,
	DLTYPE_FILE,
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
	idStr					url;
	idStr					urlAuth;
	idStr					referer;
	char					dlerror[ MAX_STRING_CHARS ];
	int						dltotal;
	int						dlnow;
	int						curlstatus;
	dlStatus_t				status;
} urlDownload_t;

typedef struct fileDownload_s {
	int					offset;
	int					length;
	void *				buffer;
} fileDownload_t;

typedef struct proxyDownload_s {
    idStr               proxyUrl;
    idStr               proxyAuth;
} proxyDownload_t;

struct backgroundDownload_t {
	backgroundDownload_t*	next;	// set by the fileSystem
	dlType_t				opcode;
	idFile*					fh;
	fileDownload_t			file;
	urlDownload_t			url;
	proxyDownload_t			proxy;
	volatile bool			completed;
};

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

struct metaDataContext_t {
	const idDict*	meta;
	bool			addon;
	idStr			pak;
};

class sdAddonMetaDataList {
	friend class idFileSystemLocal;
public:

							~sdAddonMetaDataList( void ) { 
								for( int i = 0; i < meta.Num(); i++ ) {
									delete meta[ i ].meta;
								}
							}

	int						GetNumMetaData() const { return meta.Num(); }
	const idDict&			GetMetaData( int index ) const { return *meta[ index ].meta; }
	const metaDataContext_t&GetMetaDataContext( int index ) const { return meta[ index ]; }
	const idDict*			FindMetaData( const char* name, const idDict* defaultDict = NULL ) {
								for( int i = 0; i < meta.Num(); i++ ) {
									const char* value = meta[ i ].meta->GetString( "metadata_name" );
									if( !idStr::Icmp( name, value ) ) {
										return meta[ i ].meta;
									}
								}
								return defaultDict;
							}
	int						FindMetaDataIndex( const char* name ) {
								for( int i = 0; i < meta.Num(); i++ ) {
									const char* value = meta[ i ].meta->GetString( "metadata_name" );
									if( !idStr::Icmp( name, value ) ) {
										return i;
									}
								}
								return -1;
							}

	const metaDataContext_t*	FindMetaDataContext( const char* name ) const {
								for( int i = 0; i < meta.Num(); i++ ) {
									const char* value = meta[ i ].meta->GetString( "metadata_name" );
									if( !idStr::Icmp( name, value ) ) {
										return &meta[ i ];
									}
								}
								return NULL;
							}

private:
	idList<metaDataContext_t>	meta;
};

class idFileSystem {
public:
	virtual					~idFileSystem() {}
							// Initializes the file system.
	virtual void			Init( bool quiet = false ) = 0;
							// Restarts the file system.
	virtual void			Restart( void ) = 0;
							// Shutdown the file system.
	virtual void			Shutdown( bool reloading ) = 0;
							// Returns true if the file system is initialized.
	virtual bool			IsInitialized( void ) const = 0;
							// Enables/Disables quiet mode.
	virtual void			SetQuietMode( bool enable ) = 0;
							// Returns quiet mode status.
	virtual bool			GetQuietMode( void ) const = 0;
							// Returns true if we are doing an fs_copyfiles.
	virtual bool			PerformingCopyFiles( void ) const = 0;
							// Returns a list of mods found along with descriptions
							// 'mods' contains the directory names to be passed to fs_game
							// 'descriptions' contains a free form string to be used in the UI
	virtual idModList *		ListMods( void ) = 0;
							// Returns a list of metadata specified in addon.conf (and pakmeta.conf)
	virtual sdAddonMetaDataList* ListAddonMetaData( const char* metaDataTag ) = 0;
							// Frees the given metadata list
	virtual void			FreeAddonMetaDataList( sdAddonMetaDataList* list ) = 0;
							// Frees the given mod list
	virtual void			FreeModList( idModList *modList ) = 0;
							// Lists dll files with the directories specified by fs_toolsPath
							// Directory should not have either a leading or trailing '/'														
	virtual idFileList *	ListTools() = 0;
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
							// Returns a full OS path where the initial base of the file system exists
	virtual const char *	GetBasePath() const = 0;
							// Returns a full OS path where files can be written to
	virtual const char *	GetSavePath() const = 0;
							// Returns a full OS path where user visible files can be written to
	virtual const char *	GetUserPath() const = 0;
							// Returns the current game path
	virtual const char *	GetGamePath() const = 0;
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
							// the process is verbosive when fs_debug 1
	virtual fsPureReply_t	SetPureServerChecksums( const int pureChecksums[ MAX_PURE_PAKS ], int gamePakChecksum, int missingChecksums[ MAX_PURE_PAKS ], int *missingGamePakChecksum, const char *serverVersion = NULL ) = 0;
							// fills a 0-terminated list of pak checksums for a client
							// if OS is -1, give the current game pak checksum. if >= 0, lookup the game pak table (server only)
	virtual void			GetPureServerChecksums( int checksums[ MAX_PURE_PAKS ], int OS, int *gamePakChecksum ) = 0;
							// before doing a restart, force the pure list and the search order
							// if the given checksum list can't be completely processed and set, will error out
	virtual void			SetRestartChecksums( const int pureChecksums[ MAX_PURE_PAKS ], int gamePakChecksum ) = 0;
							// equivalent to calling SetPureServerChecksums with an empty list
	virtual	void			ClearPureChecksums( void ) = 0;
							// get a mask of supported OSes. if not pure, returns -1
	virtual unsigned int	GetOSMask( void ) = 0;
							// returns true if the file exists
	virtual bool			FileExists( const char *relativePath ) = 0;
							// returns true if the file exists
	virtual bool			FileExistsExplicit( const char *OSPath ) = 0;
							// the timestamp or FILE_NOT_FOUND_TIMESTAMP
	virtual unsigned int	GetTimestamp( const char *relativePath ) = 0;
							// Reads a complete file.
							// Returns the length of the file, or -1 on failure.
							// A null buffer will just return the file length without loading.
							// A null timestamp will be ignored.
							// As a quick check for existence. -1 length == not present.
							// A 0 byte will always be appended at the end, so string ops are safe.
							// The buffer should be considered read-only, because it may be cached for other uses.
							// when markPaksReferenced is false the file's pk4 (if any) will not be added to the referenced list
							// (this is useful for reading metadata for things like the server browser)
	virtual int				ReadFile( const char *relativePath, void **buffer, unsigned int *timestamp = NULL, bool markPaksReferenced = true ) = 0;
							// Frees the memory allocated by ReadFile.
	virtual void			FreeFile( void *buffer ) = 0;
							// Writes a complete file, will create any needed subdirectories.
							// Returns the length of the file, or -1 on failure.
	virtual int				WriteFile( const char *relativePath, const void *buffer, int size, const char *basePath = "fs_savepath" ) = 0;
							// Removes the given file.
	virtual void			RemoveFile( const char *relativePath ) = 0;
							// Opens a file for reading.
	virtual idFile *		OpenFileRead( const char *relativePath, bool allowCopyFiles = true, const char* gamedir = NULL, bool markPaksReferenced = true ) = 0;
							// Opens a file for writing, will create any needed subdirectories.
	virtual idFile *		OpenFileWrite( const char *relativePath, const char *basePath = "fs_savepath" ) = 0;
							// Opens a file for writing at the end.
	virtual idFile *		OpenFileAppend( const char *filename, bool sync = false, const char *basePath = "fs_savepath" ) = 0;
							// Opens a file for reading, writing, or appending depending on the value of mode.
	virtual idFile *		OpenFileByMode( const char *relativePath, fsMode_t mode ) = 0;
							// Opens a file for reading from a full OS path.
	virtual idFile *		OpenExplicitFileRead( const char *OSPath ) = 0;
							// Opens a file for writing to a full OS path.
	virtual idFile *		OpenExplicitFileWrite( const char *OSPath ) = 0;
							// Removes the given file from a full OS path
	virtual void			RemoveExplicitFile( const char* OSPath ) = 0;
							// Removes the given directory from a full OS path
	virtual bool			RemoveExplicitDir( const char* OSPath, bool nonEmpty, bool recursive ) = 0;
							// Closes a file.
	virtual void			CloseFile( idFile *f ) = 0;
							// Returns immediately, performing the read from a background thread.
	virtual void			BackgroundDownload( backgroundDownload_t *bgl ) = 0;
							// Cancel the pending background download
	virtual bool			CancelBackgroundDownload( backgroundDownload_t *bgl ) = 0;
							// resets the bytes read counter
	virtual void			ResetReadCount( void ) = 0;
							// retrieves the current read count
	virtual int				GetReadCount( void ) = 0;
							// adds to the read count
	virtual void			AddToReadCount( int c ) = 0;
							// look for a dynamic module
	virtual void			FindDLL( const char *basename, char dllPath[ MAX_OSPATH ], bool updateChecksum, bool pureCheck ) = 0;
							// case sensitive filesystems use an internal directory cache
							// the cache is cleared when calling OpenFileWrite and RemoveFile
							// in some cases you may need to use this directly
	virtual void			ClearDirCache( void ) = 0;

	virtual idFile_Memory*	OpenMemoryFile( const char* name ) = 0;

	virtual idFile_Buffered*OpenBufferedFile( idFile* file ) = 0;

							// don't use for large copies - allocates a single memory block for the copy
	virtual bool			CopyFile( const char *fromOSPath, const char *toOSPath ) = 0;

							// lookup a relative path, return the size or 0 if not found
	virtual int				ValidateDownloadPakForChecksum( int checksum, char path[ MAX_STRING_CHARS ], bool isGamePak ) = 0;

							// verify the file can be downloaded, lookup an absolute (OS) path, return the size or 0 if not found
	virtual int				ValidateDownloadPakForRelativePath( const char *relativePath, char path[ MAX_STRING_CHARS ], bool &isGamePakReturn ) = 0;

	virtual idFile *		MakeTemporaryFile( void ) = 0;

							// make downloaded pak files known so pure negotiation works next time
	virtual int				AddPackFile( const char* path ) = 0;

							// look for a file in the loaded paks or the addon paks
							// if the file is found in addons, FS's internal structures are ready for a reloadEngine
	virtual findFile_t		FindFile( const char *path ) = 0;

							// ignore case and seperator char distinctions
	virtual bool			FilenameCompare( const char *s1, const char *s2 ) const = 0;

	virtual unsigned long	FileChecksum( idFile* o, unsigned char *md5digest = NULL ) = 0;
	virtual unsigned long	FileChecksum( FILE* o, unsigned char *md5digest = NULL ) = 0;

	virtual void			BeginLevelLoadStatistics() = 0;
	virtual void			AddLevelLoadStatistic( const char* extension, int numBytes ) = 0;
	virtual void			EndLevelLoadStatistics() = 0;
	virtual void			ReportLevelLoadStatistics() = 0;

							// is the pack currently referenced?
	virtual bool			IsAddonPackReferenced( const char* pak ) = 0;		
							// ensure that the pack is loaded after the next pure restart
	virtual void			ReferenceAddonPack( const char* pak ) = 0;

	// textures
	virtual void			ReadTGA( const char *name, byte **pic, int *width, int *height, unsigned *timestamp = 0, bool markPaksReferenced = true ) = 0;
	virtual void			WriteTGA( const char* name, const byte* pic, int width, int height ) = 0;
	virtual void			FreeTGA( byte* pic ) = 0;
};

extern idFileSystem *		fileSystem;

// Auto-close files on destruction
// Prefer this to using idFile* in most cases, since this is exception-safe
struct sdCleanupPolicy_CloseFile {
	static void Free( idFile* fp ) {
		fileSystem->CloseFile( fp );
	}
};
typedef sdAutoPtr< idFile, sdCleanupPolicy_CloseFile > sdFilePtr;
typedef sdAutoPtr< idFile_Memory, sdCleanupPolicy_CloseFile > sdFileMemoryPtr;


// idLib versions
struct sdCleanupPolicy_idLibCloseFile {
	static void Free( idFile* fp ) {
		idLib::fileSystem->CloseFile( fp );
	}
};
typedef sdAutoPtr< idFile, sdCleanupPolicy_idLibCloseFile > sdLibFilePtr;
typedef sdAutoPtr< idFile_Memory, sdCleanupPolicy_idLibCloseFile > sdLibFileMemoryPtr;

#endif /* !__FILESYSTEM_H__ */
