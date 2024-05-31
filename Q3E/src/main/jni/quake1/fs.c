/*
	DarkPlaces file system

	Copyright (C) 2003-2006 Mathieu Olivier

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA
*/

#include <limits.h>
#include <fcntl.h>

#ifdef WIN32
# include <direct.h>
# include <io.h>
# include <shlobj.h>
# include <sys/stat.h>
# include <share.h>
#else
# include <pwd.h>
# include <sys/stat.h>
# include <unistd.h>
#endif

#include "quakedef.h"

#if TARGET_OS_IPHONE
// include SDL for IPHONEOS code
# include <SDL.h>
#endif

#include "thread.h"

#include "fs.h"
#include "wad.h"

#ifdef WIN32
#include "utf8lib.h"
#endif

// Win32 requires us to add O_BINARY, but the other OSes don't have it
#ifndef O_BINARY
# define O_BINARY 0
#endif

// In case the system doesn't support the O_NONBLOCK flag
#ifndef O_NONBLOCK
# define O_NONBLOCK 0
#endif

// largefile support for Win32
#ifdef WIN32
#undef lseek
# define lseek _lseeki64
#endif

// suppress deprecated warnings
#if _MSC_VER >= 1400
# define read _read
# define write _write
# define close _close
# define unlink _unlink
# define dup _dup
#endif

#if USE_RWOPS
# include <SDL.h>
typedef SDL_RWops *filedesc_t;
# define FILEDESC_INVALID NULL
# define FILEDESC_ISVALID(fd) ((fd) != NULL)
# define FILEDESC_READ(fd,buf,count) ((fs_offset_t)SDL_RWread(fd, buf, 1, count))
# define FILEDESC_WRITE(fd,buf,count) ((fs_offset_t)SDL_RWwrite(fd, buf, 1, count))
# define FILEDESC_CLOSE SDL_RWclose
# define FILEDESC_SEEK SDL_RWseek
static filedesc_t FILEDESC_DUP(const char *filename, filedesc_t fd) {
	filedesc_t new_fd = SDL_RWFromFile(filename, "rb");
	if (SDL_RWseek(new_fd, SDL_RWseek(fd, 0, RW_SEEK_CUR), RW_SEEK_SET) < 0) {
		SDL_RWclose(new_fd);
		return NULL;
	}
	return new_fd;
}
# define unlink(name) Con_DPrintf("Sorry, no unlink support when trying to unlink %s.\n", (name))
#else
typedef int filedesc_t;
# define FILEDESC_INVALID -1
# define FILEDESC_ISVALID(fd) ((fd) != -1)
# define FILEDESC_READ read
# define FILEDESC_WRITE write
# define FILEDESC_CLOSE close
# define FILEDESC_SEEK lseek
static filedesc_t FILEDESC_DUP(const char *filename, filedesc_t fd) {
	return dup(fd);
}
#endif


/* This code seems to have originally been written with the assumption that
 * read(..., n) returns n on success. This is not the case (refer to
 * <https://pubs.opengroup.org/onlinepubs/9699919799/functions/read.html>).
 * Ditto for write.
 */

/*
====================
ReadAll

Read exactly length bytes from fd into buf. If end of file is reached,
the number of bytes read is returned. If an error occurred, that error
is returned. Note that if an error is returned, any previously read
data is lost.
====================
*/
static fs_offset_t ReadAll(const filedesc_t fd, void *const buf, const size_t length)
{
	char *const p = (char *)buf;
	size_t cursor = 0;
	do
	{
		const fs_offset_t result = FILEDESC_READ(fd, p + cursor, length - cursor);
		if (result < 0) // Error
			return result;
		if (result == 0) // EOF
			break;
		cursor += result;
	} while (cursor < length);
	return cursor;
}

/*
====================
WriteAll

Write exactly length bytes to fd from buf.
If an error occurred, that error is returned.
====================
*/
static fs_offset_t WriteAll(const filedesc_t fd, const void *const buf, const size_t length)
{
	const char *const p = (const char *)buf;
	size_t cursor = 0;
	do
	{
		const fs_offset_t result = FILEDESC_WRITE(fd, p + cursor, length - cursor);
		if (result < 0) // Error
			return result;
		cursor += result;
	} while (cursor < length);
	return cursor;
}

#undef FILEDESC_READ
#define FILEDESC_READ ReadAll
#undef FILEDESC_WRITE
#define FILEDESC_WRITE WriteAll

/** \page fs File System

All of Quake's data access is through a hierarchical file system, the contents
of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and
all game directories.  The sys_* files pass this to host_init in
quakeparms_t->basedir.  This can be overridden with the "-basedir" command
line parm to allow code debugging in a different directory.  The base
directory is only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that
all generated files (savegames, screenshots, demos, config files) will be
saved to.  This can be overridden with the "-game" command line parameter.
If multiple "-game <gamedir>" args are passed the last one is the "primary"
and files will be saved there, the rest are read-only.

*/


/*
=============================================================================

CONSTANTS

=============================================================================
*/

// Magic numbers of a ZIP file (big-endian format)
#define ZIP_DATA_HEADER	0x504B0304  // "PK\3\4"
#define ZIP_CDIR_HEADER	0x504B0102  // "PK\1\2"
#define ZIP_END_HEADER	0x504B0506  // "PK\5\6"

// Other constants for ZIP files
#define ZIP_MAX_COMMENTS_SIZE		((unsigned short)0xFFFF)
#define ZIP_END_CDIR_SIZE			22
#define ZIP_CDIR_CHUNK_BASE_SIZE	46
#define ZIP_LOCAL_CHUNK_BASE_SIZE	30

#ifdef LINK_TO_ZLIB
#include <zlib.h>

#define qz_inflate inflate
#define qz_inflateEnd inflateEnd
#define qz_inflateInit2_ inflateInit2_
#define qz_inflateReset inflateReset
#define qz_deflateInit2_ deflateInit2_
#define qz_deflateEnd deflateEnd
#define qz_deflate deflate
#define Z_MEMLEVEL_DEFAULT 8
#else

// Zlib constants (from zlib.h)
#define Z_SYNC_FLUSH	2
#define MAX_WBITS		15
#define Z_OK			0
#define Z_STREAM_END	1
#define Z_STREAM_ERROR  (-2)
#define Z_DATA_ERROR    (-3)
#define Z_MEM_ERROR     (-4)
#define Z_BUF_ERROR     (-5)
#define ZLIB_VERSION	"1.2.3"

#define Z_BINARY 0
#define Z_DEFLATED 8
#define Z_MEMLEVEL_DEFAULT 8

#define Z_NULL 0
#define Z_DEFAULT_COMPRESSION (-1)
#define Z_NO_FLUSH 0
#define Z_SYNC_FLUSH 2
#define Z_FULL_FLUSH 3
#define Z_FINISH 4

// Uncomment the following line if the zlib DLL you have still uses
// the 1.1.x series calling convention on Win32 (WINAPI)
//#define ZLIB_USES_WINAPI


/*
=============================================================================

TYPES

=============================================================================
*/

/*! Zlib stream (from zlib.h)
 * \warning: some pointers we don't use directly have
 * been cast to "void*" for a matter of simplicity
 */
typedef struct
{
	unsigned char			*next_in;	///< next input byte
	unsigned int	avail_in;	///< number of bytes available at next_in
	unsigned long	total_in;	///< total nb of input bytes read so far

	unsigned char			*next_out;	///< next output byte should be put there
	unsigned int	avail_out;	///< remaining free space at next_out
	unsigned long	total_out;	///< total nb of bytes output so far

	char			*msg;		///< last error message, NULL if no error
	void			*state;		///< not visible by applications

	void			*zalloc;	///< used to allocate the internal state
	void			*zfree;		///< used to free the internal state
	void			*opaque;	///< private data object passed to zalloc and zfree

	int				data_type;	///< best guess about the data type: ascii or binary
	unsigned long	adler;		///< adler32 value of the uncompressed data
	unsigned long	reserved;	///< reserved for future use
} z_stream;
#endif


/// inside a package (PAK or PK3)
#define QFILE_FLAG_PACKED (1 << 0)
/// file is compressed using the deflate algorithm (PK3 only)
#define QFILE_FLAG_DEFLATED (1 << 1)
/// file is actually already loaded data
#define QFILE_FLAG_DATA (1 << 2)
/// real file will be removed on close
#define QFILE_FLAG_REMOVE (1 << 3)

#define FILE_BUFF_SIZE 2048
typedef struct
{
	z_stream	zstream;
	size_t		comp_length;			///< length of the compressed file
	size_t		in_ind, in_len;			///< input buffer current index and length
	size_t		in_position;			///< position in the compressed file
	unsigned char		input [FILE_BUFF_SIZE];
} ztoolkit_t;

struct qfile_s
{
	int				flags;
	filedesc_t			handle;					///< file descriptor
	fs_offset_t		real_length;			///< uncompressed file size (for files opened in "read" mode)
	fs_offset_t		position;				///< current position in the file
	fs_offset_t		offset;					///< offset into the package (0 if external file)
	int				ungetc;					///< single stored character from ungetc, cleared to EOF when read

	// Contents buffer
	fs_offset_t		buff_ind, buff_len;		///< buffer current index and length
	unsigned char			buff [FILE_BUFF_SIZE];

	ztoolkit_t*		ztk;	///< For zipped files.

	const unsigned char *data;	///< For data files.

	const char *filename; ///< Kept around for QFILE_FLAG_REMOVE, unused otherwise
};


// ------ PK3 files on disk ------ //

// You can get the complete ZIP format description from PKWARE website

typedef struct pk3_endOfCentralDir_s
{
	unsigned int signature;
	unsigned short disknum;
	unsigned short cdir_disknum;	///< number of the disk with the start of the central directory
	unsigned short localentries;	///< number of entries in the central directory on this disk
	unsigned short nbentries;		///< total number of entries in the central directory on this disk
	unsigned int cdir_size;			///< size of the central directory
	unsigned int cdir_offset;		///< with respect to the starting disk number
	unsigned short comment_size;
	fs_offset_t prepended_garbage;
} pk3_endOfCentralDir_t;


// ------ PAK files on disk ------ //
typedef struct dpackfile_s
{
	char name[56];
	int filepos, filelen;
} dpackfile_t;

typedef struct dpackheader_s
{
	char id[4];
	int dirofs;
	int dirlen;
} dpackheader_t;


/*! \name Packages in memory
 * @{
 */
/// the offset in packfile_t is the true contents offset
#define PACKFILE_FLAG_TRUEOFFS (1 << 0)
/// file compressed using the deflate algorithm
#define PACKFILE_FLAG_DEFLATED (1 << 1)
/// file is a symbolic link
#define PACKFILE_FLAG_SYMLINK (1 << 2)

typedef struct packfile_s
{
	char name [MAX_QPATH];
	int flags;
	fs_offset_t offset;
	fs_offset_t packsize;	///< size in the package
	fs_offset_t realsize;	///< real file size (uncompressed)
} packfile_t;

typedef struct pack_s
{
	char filename [MAX_OSPATH];
	char shortname [MAX_QPATH];
	filedesc_t handle;
	qbool ignorecase;  ///< PK3 ignores case
	int numfiles;
	qbool vpack;
	qbool dlcache;
	packfile_t *files;
} pack_t;
//@}

/// Search paths for files (including packages)
typedef struct searchpath_s
{
	// only one of filename / pack will be used
	char filename[MAX_OSPATH];
	pack_t *pack;
	struct searchpath_s *next;
} searchpath_t;


/*
=============================================================================

FUNCTION PROTOTYPES

=============================================================================
*/

void FS_Dir_f(cmd_state_t *cmd);
void FS_Ls_f(cmd_state_t *cmd);
void FS_Which_f(cmd_state_t *cmd);

static searchpath_t *FS_FindFile (const char *name, int *index, const char **canonicalname, qbool quiet);
static packfile_t* FS_AddFileToPack (const char* name, pack_t* pack,
									fs_offset_t offset, fs_offset_t packsize,
									fs_offset_t realsize, int flags);


/*
=============================================================================

VARIABLES

=============================================================================
*/

mempool_t *fs_mempool;
void *fs_mutex = NULL;

searchpath_t *fs_searchpaths = NULL;
const char *const fs_checkgamedir_missing = "missing";

#define MAX_FILES_IN_PACK	65536

char fs_userdir[MAX_OSPATH];
char fs_gamedir[MAX_OSPATH];
char fs_basedir[MAX_OSPATH];
static pack_t *fs_selfpack = NULL;

// list of active game directories (empty if not running a mod)
int fs_numgamedirs = 0;
char fs_gamedirs[MAX_GAMEDIRS][MAX_QPATH];

// list of all gamedirs with modinfo.txt
gamedir_t *fs_all_gamedirs = NULL;
int fs_all_gamedirs_count = 0;

cvar_t scr_screenshot_name = {CF_CLIENT | CF_PERSISTENT, "scr_screenshot_name","dp", "prefix name for saved screenshots (changes based on -game commandline, as well as which game mode is running; the date is encoded using strftime escapes)"};
cvar_t fs_empty_files_in_pack_mark_deletions = {CF_CLIENT | CF_SERVER, "fs_empty_files_in_pack_mark_deletions", "0", "if enabled, empty files in a pak/pk3 count as not existing but cancel the search in further packs, effectively allowing patch pak/pk3 files to 'delete' files"};
cvar_t fs_unload_dlcache = {CF_CLIENT, "fs_unload_dlcache", "1", "if enabled, unload dlcache's loaded pak/pk3 files when changing server and/or map WARNING: disabling unloading can cause servers to override assets of other servers, \"memory leaking\" by dlcache assets never unloading and many more issues"};
cvar_t cvar_fs_gamedir = {CF_CLIENT | CF_SERVER | CF_READONLY | CF_PERSISTENT, "fs_gamedir", "", "the list of currently selected gamedirs (use the 'gamedir' command to change this)"};


/*
=============================================================================

PRIVATE FUNCTIONS - PK3 HANDLING

=============================================================================
*/

#ifndef LINK_TO_ZLIB
// Functions exported from zlib
#if defined(WIN32) && defined(ZLIB_USES_WINAPI)
# define ZEXPORT WINAPI
#else
# define ZEXPORT
#endif

static int (ZEXPORT *qz_inflate) (z_stream* strm, int flush);
static int (ZEXPORT *qz_inflateEnd) (z_stream* strm);
static int (ZEXPORT *qz_inflateInit2_) (z_stream* strm, int windowBits, const char *version, int stream_size);
static int (ZEXPORT *qz_inflateReset) (z_stream* strm);
static int (ZEXPORT *qz_deflateInit2_) (z_stream* strm, int level, int method, int windowBits, int memLevel, int strategy, const char *version, int stream_size);
static int (ZEXPORT *qz_deflateEnd) (z_stream* strm);
static int (ZEXPORT *qz_deflate) (z_stream* strm, int flush);
#endif

#define qz_inflateInit2(strm, windowBits) \
        qz_inflateInit2_((strm), (windowBits), ZLIB_VERSION, sizeof(z_stream))
#define qz_deflateInit2(strm, level, method, windowBits, memLevel, strategy) \
        qz_deflateInit2_((strm), (level), (method), (windowBits), (memLevel), (strategy), ZLIB_VERSION, sizeof(z_stream))

#ifndef LINK_TO_ZLIB
//        qz_deflateInit_((strm), (level), ZLIB_VERSION, sizeof(z_stream))

static dllfunction_t zlibfuncs[] =
{
	{"inflate",			(void **) &qz_inflate},
	{"inflateEnd",		(void **) &qz_inflateEnd},
	{"inflateInit2_",	(void **) &qz_inflateInit2_},
	{"inflateReset",	(void **) &qz_inflateReset},
	{"deflateInit2_",   (void **) &qz_deflateInit2_},
	{"deflateEnd",      (void **) &qz_deflateEnd},
	{"deflate",         (void **) &qz_deflate},
	{NULL, NULL}
};

/// Handle for Zlib DLL
static dllhandle_t zlib_dll = NULL;
#endif

#ifdef WIN32
static HRESULT (WINAPI *qSHGetFolderPath) (HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath);
static dllfunction_t shfolderfuncs[] =
{
	{"SHGetFolderPathW", (void **) &qSHGetFolderPath},
	{NULL, NULL}
};
static const char* shfolderdllnames [] =
{
	"shfolder.dll",  // IE 4, or Win NT and higher
	NULL
};
static dllhandle_t shfolder_dll = NULL;

const GUID qFOLDERID_SavedGames = {0x4C5C32FF, 0xBB9D, 0x43b0, {0xB5, 0xB4, 0x2D, 0x72, 0xE5, 0x4E, 0xAA, 0xA4}};
#define qREFKNOWNFOLDERID const GUID *
#define qKF_FLAG_CREATE 0x8000
#define qKF_FLAG_NO_ALIAS 0x1000
static HRESULT (WINAPI *qSHGetKnownFolderPath) (qREFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath);
static dllfunction_t shell32funcs[] =
{
	{"SHGetKnownFolderPath", (void **) &qSHGetKnownFolderPath},
	{NULL, NULL}
};
static const char* shell32dllnames [] =
{
	"shell32.dll",  // Vista and higher
	NULL
};
static dllhandle_t shell32_dll = NULL;

static HRESULT (WINAPI *qCoInitializeEx)(LPVOID pvReserved, DWORD dwCoInit);
static void (WINAPI *qCoUninitialize)(void);
static void (WINAPI *qCoTaskMemFree)(LPVOID pv);
static dllfunction_t ole32funcs[] =
{
	{"CoInitializeEx", (void **) &qCoInitializeEx},
	{"CoUninitialize", (void **) &qCoUninitialize},
	{"CoTaskMemFree", (void **) &qCoTaskMemFree},
	{NULL, NULL}
};
static const char* ole32dllnames [] =
{
	"ole32.dll", // 2000 and higher
	NULL
};
static dllhandle_t ole32_dll = NULL;
#endif

/*
====================
PK3_CloseLibrary

Unload the Zlib DLL
====================
*/
static void PK3_CloseLibrary (void)
{
#ifndef LINK_TO_ZLIB
	Sys_FreeLibrary (&zlib_dll);
#endif
}


/*
====================
PK3_OpenLibrary

Try to load the Zlib DLL
====================
*/
static qbool PK3_OpenLibrary (void)
{
#ifdef LINK_TO_ZLIB
	return true;
#else
	const char* dllnames [] =
	{
#if defined(WIN32)
# ifdef ZLIB_USES_WINAPI
		"zlibwapi.dll",
		"zlib.dll",
# else
		"zlib1.dll",
# endif
#elif defined(MACOSX)
		"libz.dylib",
#else
		"libz.so.1",
		"libz.so",
#endif
		NULL
	};

	// Already loaded?
	if (zlib_dll)
		return true;

	// Load the DLL
	return Sys_LoadDependency (dllnames, &zlib_dll, zlibfuncs);
#endif
}

/*
====================
FS_HasZlib

See if zlib is available
====================
*/
qbool FS_HasZlib(void)
{
#ifdef LINK_TO_ZLIB
	return true;
#else
	PK3_OpenLibrary(); // to be safe
	return (zlib_dll != 0);
#endif
}

/*
====================
PK3_GetEndOfCentralDir

Extract the end of the central directory from a PK3 package
====================
*/
static qbool PK3_GetEndOfCentralDir (const char *packfile, filedesc_t packhandle, pk3_endOfCentralDir_t *eocd)
{
	fs_offset_t filesize, maxsize;
	unsigned char *buffer, *ptr;
	int ind;

	// Get the package size
	filesize = FILEDESC_SEEK (packhandle, 0, SEEK_END);
	if (filesize < ZIP_END_CDIR_SIZE)
		return false;

	// Load the end of the file in memory
	if (filesize < ZIP_MAX_COMMENTS_SIZE + ZIP_END_CDIR_SIZE)
		maxsize = filesize;
	else
		maxsize = ZIP_MAX_COMMENTS_SIZE + ZIP_END_CDIR_SIZE;
	buffer = (unsigned char *)Mem_Alloc (tempmempool, maxsize);
	FILEDESC_SEEK (packhandle, filesize - maxsize, SEEK_SET);
	if (FILEDESC_READ (packhandle, buffer, maxsize) != (fs_offset_t) maxsize)
	{
		Mem_Free (buffer);
		return false;
	}

	// Look for the end of central dir signature around the end of the file
	maxsize -= ZIP_END_CDIR_SIZE;
	ptr = &buffer[maxsize];
	ind = 0;
	while (BuffBigLong (ptr) != ZIP_END_HEADER)
	{
		if (ind == maxsize)
		{
			Mem_Free (buffer);
			return false;
		}

		ind++;
		ptr--;
	}

	memcpy (eocd, ptr, ZIP_END_CDIR_SIZE);
	eocd->signature = LittleLong (eocd->signature);
	eocd->disknum = LittleShort (eocd->disknum);
	eocd->cdir_disknum = LittleShort (eocd->cdir_disknum);
	eocd->localentries = LittleShort (eocd->localentries);
	eocd->nbentries = LittleShort (eocd->nbentries);
	eocd->cdir_size = LittleLong (eocd->cdir_size);
	eocd->cdir_offset = LittleLong (eocd->cdir_offset);
	eocd->comment_size = LittleShort (eocd->comment_size);
	eocd->prepended_garbage = filesize - (ind + ZIP_END_CDIR_SIZE) - eocd->cdir_offset - eocd->cdir_size; // this detects "SFX" zip files
	eocd->cdir_offset += eocd->prepended_garbage;

	Mem_Free (buffer);

	if (
			eocd->cdir_size > filesize ||
			eocd->cdir_offset >= filesize ||
			eocd->cdir_offset + eocd->cdir_size > filesize
	   )
	{
		// Obviously invalid central directory.
		return false;
	}

	return true;
}


/*
====================
PK3_BuildFileList

Extract the file list from a PK3 file
====================
*/
static int PK3_BuildFileList (pack_t *pack, const pk3_endOfCentralDir_t *eocd)
{
	unsigned char *central_dir, *ptr;
	unsigned int ind;
	fs_offset_t remaining;

	// Load the central directory in memory
	central_dir = (unsigned char *)Mem_Alloc (tempmempool, eocd->cdir_size);
	if (FILEDESC_SEEK (pack->handle, eocd->cdir_offset, SEEK_SET) == -1)
	{
		Mem_Free (central_dir);
		return -1;
	}
	if(FILEDESC_READ (pack->handle, central_dir, eocd->cdir_size) != (fs_offset_t) eocd->cdir_size)
	{
		Mem_Free (central_dir);
		return -1;
	}

	// Extract the files properties
	// The parsing is done "by hand" because some fields have variable sizes and
	// the constant part isn't 4-bytes aligned, which makes the use of structs difficult
	remaining = eocd->cdir_size;
	pack->numfiles = 0;
	ptr = central_dir;
	for (ind = 0; ind < eocd->nbentries; ind++)
	{
		fs_offset_t namesize, count;

		// Checking the remaining size
		if (remaining < ZIP_CDIR_CHUNK_BASE_SIZE)
		{
			Mem_Free (central_dir);
			return -1;
		}
		remaining -= ZIP_CDIR_CHUNK_BASE_SIZE;

		// Check header
		if (BuffBigLong (ptr) != ZIP_CDIR_HEADER)
		{
			Mem_Free (central_dir);
			return -1;
		}

		namesize = (unsigned short)BuffLittleShort (&ptr[28]);	// filename length

		// Check encryption, compression, and attributes
		// 1st uint8  : general purpose bit flag
		//    Check bits 0 (encryption), 3 (data descriptor after the file), and 5 (compressed patched data (?))
		//
		// LadyHavoc: bit 3 would be a problem if we were scanning the archive
		// but is not a problem in the central directory where the values are
		// always real.
		//
		// bit 3 seems to always be set by the standard Mac OSX zip maker
		//
		// 2nd uint8 : external file attributes
		//    Check bits 3 (file is a directory) and 5 (file is a volume (?))
		if ((ptr[8] & 0x21) == 0 && (ptr[38] & 0x18) == 0)
		{
			// Still enough bytes for the name?
			if (remaining < namesize || namesize >= (int)sizeof (*pack->files))
			{
				Mem_Free (central_dir);
				return -1;
			}

			// WinZip doesn't use the "directory" attribute, so we need to check the name directly
			if (ptr[ZIP_CDIR_CHUNK_BASE_SIZE + namesize - 1] != '/')
			{
				char filename [sizeof (pack->files[0].name)];
				fs_offset_t offset, packsize, realsize;
				int flags;

				// Extract the name (strip it if necessary)
				namesize = min(namesize, (int)sizeof (filename) - 1);
				memcpy (filename, &ptr[ZIP_CDIR_CHUNK_BASE_SIZE], namesize);
				filename[namesize] = '\0';

				if (BuffLittleShort (&ptr[10]))
					flags = PACKFILE_FLAG_DEFLATED;
				else
					flags = 0;
				offset = (unsigned int)(BuffLittleLong (&ptr[42]) + eocd->prepended_garbage);
				packsize = (unsigned int)BuffLittleLong (&ptr[20]);
				realsize = (unsigned int)BuffLittleLong (&ptr[24]);

				switch(ptr[5]) // C_VERSION_MADE_BY_1
				{
					case 3: // UNIX_
					case 2: // VMS_
					case 16: // BEOS_
						if((BuffLittleShort(&ptr[40]) & 0120000) == 0120000)
							// can't use S_ISLNK here, as this has to compile on non-UNIX too
							flags |= PACKFILE_FLAG_SYMLINK;
						break;
				}

				FS_AddFileToPack (filename, pack, offset, packsize, realsize, flags);
			}
		}

		// Skip the name, additionnal field, and comment
		// 1er uint16 : extra field length
		// 2eme uint16 : file comment length
		count = namesize + (unsigned short)BuffLittleShort (&ptr[30]) + (unsigned short)BuffLittleShort (&ptr[32]);
		ptr += ZIP_CDIR_CHUNK_BASE_SIZE + count;
		remaining -= count;
	}

	// If the package is empty, central_dir is NULL here
	if (central_dir != NULL)
		Mem_Free (central_dir);
	return pack->numfiles;
}


/*
====================
FS_LoadPackPK3

Create a package entry associated with a PK3 file
====================
*/
static pack_t *FS_LoadPackPK3FromFD (const char *packfile, filedesc_t packhandle, qbool silent)
{
	pk3_endOfCentralDir_t eocd;
	pack_t *pack;
	int real_nb_files;

	if (! PK3_GetEndOfCentralDir (packfile, packhandle, &eocd))
	{
		if(!silent)
			Con_Printf ("%s is not a PK3 file\n", packfile);
		FILEDESC_CLOSE(packhandle);
		return NULL;
	}

	// Multi-volume ZIP archives are NOT allowed
	if (eocd.disknum != 0 || eocd.cdir_disknum != 0)
	{
		Con_Printf ("%s is a multi-volume ZIP archive\n", packfile);
		FILEDESC_CLOSE(packhandle);
		return NULL;
	}

	// We only need to do this test if MAX_FILES_IN_PACK is lesser than 65535
	// since eocd.nbentries is an unsigned 16 bits integer
#if MAX_FILES_IN_PACK < 65535
	if (eocd.nbentries > MAX_FILES_IN_PACK)
	{
		Con_Printf ("%s contains too many files (%hu)\n", packfile, eocd.nbentries);
		FILEDESC_CLOSE(packhandle);
		return NULL;
	}
#endif

	// Create a package structure in memory
	pack = (pack_t *)Mem_Alloc(fs_mempool, sizeof (pack_t));
	pack->ignorecase = true; // PK3 ignores case
	dp_strlcpy (pack->filename, packfile, sizeof (pack->filename));
	pack->handle = packhandle;
	pack->numfiles = eocd.nbentries;
	pack->files = (packfile_t *)Mem_Alloc(fs_mempool, eocd.nbentries * sizeof(packfile_t));

	real_nb_files = PK3_BuildFileList (pack, &eocd);
	if (real_nb_files < 0)
	{
		Con_Printf ("%s is not a valid PK3 file\n", packfile);
		FILEDESC_CLOSE(pack->handle);
		Mem_Free(pack);
		return NULL;
	}

	Con_DPrintf("Added packfile %s (%i files)\n", packfile, real_nb_files);
	return pack;
}

static filedesc_t FS_SysOpenFiledesc(const char *filepath, const char *mode, qbool nonblocking);
static pack_t *FS_LoadPackPK3 (const char *packfile)
{
	filedesc_t packhandle;
	packhandle = FS_SysOpenFiledesc (packfile, "rb", false);
	if (!FILEDESC_ISVALID(packhandle))
		return NULL;
	return FS_LoadPackPK3FromFD(packfile, packhandle, false);
}


/*
====================
PK3_GetTrueFileOffset

Find where the true file data offset is
====================
*/
static qbool PK3_GetTrueFileOffset (packfile_t *pfile, pack_t *pack)
{
	unsigned char buffer [ZIP_LOCAL_CHUNK_BASE_SIZE];
	fs_offset_t count;

	// Already found?
	if (pfile->flags & PACKFILE_FLAG_TRUEOFFS)
		return true;

	// Load the local file description
	if (FILEDESC_SEEK (pack->handle, pfile->offset, SEEK_SET) == -1)
	{
		Con_Printf ("Can't seek in package %s\n", pack->filename);
		return false;
	}
	count = FILEDESC_READ (pack->handle, buffer, ZIP_LOCAL_CHUNK_BASE_SIZE);
	if (count != ZIP_LOCAL_CHUNK_BASE_SIZE || BuffBigLong (buffer) != ZIP_DATA_HEADER)
	{
		Con_Printf ("Can't retrieve file %s in package %s\n", pfile->name, pack->filename);
		return false;
	}

	// Skip name and extra field
	pfile->offset += BuffLittleShort (&buffer[26]) + BuffLittleShort (&buffer[28]) + ZIP_LOCAL_CHUNK_BASE_SIZE;

	pfile->flags |= PACKFILE_FLAG_TRUEOFFS;
	return true;
}


/*
=============================================================================

OTHER PRIVATE FUNCTIONS

=============================================================================
*/


/*
====================
FS_AddFileToPack

Add a file to the list of files contained into a package
====================
*/
static packfile_t* FS_AddFileToPack (const char* name, pack_t* pack,
									 fs_offset_t offset, fs_offset_t packsize,
									 fs_offset_t realsize, int flags)
{
	int (*strcmp_funct) (const char* str1, const char* str2);
	int left, right, middle;
	packfile_t *pfile;

	strcmp_funct = pack->ignorecase ? strcasecmp : strcmp;

	// Look for the slot we should put that file into (binary search)
	left = 0;
	right = pack->numfiles - 1;
	while (left <= right)
	{
		int diff;

		middle = (left + right) / 2;
		diff = strcmp_funct (pack->files[middle].name, name);

		// If we found the file, there's a problem
		if (!diff)
			Con_Printf ("Package %s contains the file %s several times\n", pack->filename, name);

		// If we're too far in the list
		if (diff > 0)
			right = middle - 1;
		else
			left = middle + 1;
	}

	// We have to move the right of the list by one slot to free the one we need
	pfile = &pack->files[left];
	memmove (pfile + 1, pfile, (pack->numfiles - left) * sizeof (*pfile));
	pack->numfiles++;

	dp_strlcpy (pfile->name, name, sizeof (pfile->name));
	pfile->offset = offset;
	pfile->packsize = packsize;
	pfile->realsize = realsize;
	pfile->flags = flags;

	return pfile;
}

#if WIN32
#define WSTRBUF 4096
static inline int wstrlen(wchar *wstr)
{
	int len = 0;
	while (wstr[len] != 0 && len < WSTRBUF)
		++len;
	return len;
}
#define widen(str, wstr) fromwtf8(str, strlen(str), wstr, WSTRBUF)
#define narrow(wstr, str) towtf8(wstr, wstrlen(wstr), str, WSTRBUF)
#endif

static void FS_mkdir (const char *path)
{
#if WIN32
	wchar pathw[WSTRBUF] = {0};
#endif
	if(Sys_CheckParm("-readonly"))
		return;

#if WIN32
	widen(path, pathw);
	if (_wmkdir (pathw) == -1)
#else
	if (mkdir (path, 0777) == -1)
#endif
	{
		// No logging for this. The only caller is FS_CreatePath (which
		// calls it in ways that will intentionally produce EEXIST),
		// and its own callers always use the directory afterwards and
		// thus will detect failure that way.
	}
}

/*
============
FS_CreatePath

Only used for FS_OpenRealFile.
============
*/
void FS_CreatePath (char *path)
{
	char *ofs, save;

	for (ofs = path+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/' || *ofs == '\\')
		{
			// create the directory
			save = *ofs;
			*ofs = 0;
			FS_mkdir (path);
			*ofs = save;
		}
	}
}


/*
============
FS_Path_f

============
*/
static void FS_Path_f(cmd_state_t *cmd)
{
	searchpath_t *s;

	Con_Print("Current search path:\n");
	for (s=fs_searchpaths ; s ; s=s->next)
	{
		if (s->pack)
		{
			if(s->pack->vpack)
				Con_Printf("%sdir (virtual pack)\n", s->pack->filename);
			else
				Con_Printf("%s (%i files)\n", s->pack->filename, s->pack->numfiles);
		}
		else
			Con_Printf("%s\n", s->filename);
	}
}


/*
=================
FS_LoadPackPAK
=================
*/
/*! Takes an explicit (not game tree related) path to a pak file.
 *Loads the header and directory, adding the files at the beginning
 *of the list so they override previous pack files.
 */
static pack_t *FS_LoadPackPAK (const char *packfile)
{
	dpackheader_t header;
	int i, numpackfiles;
	filedesc_t packhandle;
	pack_t *pack;
	dpackfile_t *info;

	packhandle = FS_SysOpenFiledesc(packfile, "rb", false);
	if (!FILEDESC_ISVALID(packhandle))
		return NULL;
	if(FILEDESC_READ (packhandle, (void *)&header, sizeof(header)) != sizeof(header))
	{
		Con_Printf ("%s is not a packfile\n", packfile);
		FILEDESC_CLOSE(packhandle);
		return NULL;
	}
	if (memcmp(header.id, "PACK", 4))
	{
		Con_Printf ("%s is not a packfile\n", packfile);
		FILEDESC_CLOSE(packhandle);
		return NULL;
	}
	header.dirofs = LittleLong (header.dirofs);
	header.dirlen = LittleLong (header.dirlen);

	if (header.dirlen % sizeof(dpackfile_t))
	{
		Con_Printf ("%s has an invalid directory size\n", packfile);
		FILEDESC_CLOSE(packhandle);
		return NULL;
	}

	numpackfiles = header.dirlen / sizeof(dpackfile_t);

	if (numpackfiles < 0 || numpackfiles > MAX_FILES_IN_PACK)
	{
		Con_Printf ("%s has %i files\n", packfile, numpackfiles);
		FILEDESC_CLOSE(packhandle);
		return NULL;
	}

	info = (dpackfile_t *)Mem_Alloc(tempmempool, sizeof(*info) * numpackfiles);
	FILEDESC_SEEK (packhandle, header.dirofs, SEEK_SET);
	if(header.dirlen != FILEDESC_READ (packhandle, (void *)info, header.dirlen))
	{
		Con_Printf("%s is an incomplete PAK, not loading\n", packfile);
		Mem_Free(info);
		FILEDESC_CLOSE(packhandle);
		return NULL;
	}

	pack = (pack_t *)Mem_Alloc(fs_mempool, sizeof (pack_t));
	pack->ignorecase = true; // PAK is sensitive in Quake1 but insensitive in Quake2
	dp_strlcpy (pack->filename, packfile, sizeof (pack->filename));
	pack->handle = packhandle;
	pack->numfiles = 0;
	pack->files = (packfile_t *)Mem_Alloc(fs_mempool, numpackfiles * sizeof(packfile_t));

	// parse the directory
	for (i = 0;i < numpackfiles;i++)
	{
		fs_offset_t offset = (unsigned int)LittleLong (info[i].filepos);
		fs_offset_t size = (unsigned int)LittleLong (info[i].filelen);

		// Ensure a zero terminated file name (required by format).
		info[i].name[sizeof(info[i].name) - 1] = 0;

		FS_AddFileToPack (info[i].name, pack, offset, size, size, PACKFILE_FLAG_TRUEOFFS);
	}

	Mem_Free(info);

	Con_DPrintf("Added packfile %s (%i files)\n", packfile, numpackfiles);
	return pack;
}

/*
====================
FS_LoadPackVirtual

Create a package entry associated with a directory file
====================
*/
static pack_t *FS_LoadPackVirtual (const char *dirname)
{
	pack_t *pack;
	pack = (pack_t *)Mem_Alloc(fs_mempool, sizeof (pack_t));
	pack->vpack = true;
	pack->ignorecase = false;
	dp_strlcpy (pack->filename, dirname, sizeof(pack->filename));
	pack->handle = FILEDESC_INVALID;
	pack->numfiles = -1;
	pack->files = NULL;
	Con_DPrintf("Added packfile %s (virtual pack)\n", dirname);
	return pack;
}

/*
================
FS_AddPack_Fullpath
================
*/
/*! Adds the given pack to the search path.
 * The pack type is autodetected by the file extension.
 *
 * Returns true if the file was successfully added to the
 * search path or if it was already included.
 *
 * If keep_plain_dirs is set, the pack will be added AFTER the first sequence of
 * plain directories.
 *
 */
static qbool FS_AddPack_Fullpath(const char *pakfile, const char *shortname, qbool *already_loaded, qbool keep_plain_dirs, qbool dlcache)
{
	searchpath_t *search;
	pack_t *pak = NULL;
	const char *ext = FS_FileExtension(pakfile);
	size_t l;

	for(search = fs_searchpaths; search; search = search->next)
	{
		if(search->pack && !strcasecmp(search->pack->filename, pakfile))
		{
			if(already_loaded)
				*already_loaded = true;
			return true; // already loaded
		}
	}

	if(already_loaded)
		*already_loaded = false;

	if(!strcasecmp(ext, "pk3dir") || !strcasecmp(ext, "dpkdir"))
		pak = FS_LoadPackVirtual (pakfile);
	else if(!strcasecmp(ext, "pak"))
		pak = FS_LoadPackPAK (pakfile);
	else if(!strcasecmp(ext, "pk3") || !strcasecmp(ext, "dpk"))
		pak = FS_LoadPackPK3 (pakfile);
	else if(!strcasecmp(ext, "obb")) // android apk expansion
		pak = FS_LoadPackPK3 (pakfile);
	else
		Con_Printf("\"%s\" does not have a pack extension\n", pakfile);

	if(pak)
	{
		dp_strlcpy(pak->shortname, shortname, sizeof(pak->shortname));

		//Con_DPrintf("  Registered pack with short name %s\n", shortname);
		if(keep_plain_dirs)
		{
			// find the first item whose next one is a pack or NULL
			searchpath_t *insertion_point = 0;
			if(fs_searchpaths && !fs_searchpaths->pack)
			{
				insertion_point = fs_searchpaths;
				for(;;)
				{
					if(!insertion_point->next)
						break;
					if(insertion_point->next->pack)
						break;
					insertion_point = insertion_point->next;
				}
			}
			// If insertion_point is NULL, this means that either there is no
			// item in the list yet, or that the very first item is a pack. In
			// that case, we want to insert at the beginning...
			if(!insertion_point)
			{
				search = (searchpath_t *)Mem_Alloc(fs_mempool, sizeof(searchpath_t));
				search->next = fs_searchpaths;
				fs_searchpaths = search;
			}
			else
			// otherwise we want to append directly after insertion_point.
			{
				search = (searchpath_t *)Mem_Alloc(fs_mempool, sizeof(searchpath_t));
				search->next = insertion_point->next;
				insertion_point->next = search;
			}
		}
		else
		{
			search = (searchpath_t *)Mem_Alloc(fs_mempool, sizeof(searchpath_t));
			search->next = fs_searchpaths;
			fs_searchpaths = search;
		}
		search->pack = pak;
		search->pack->dlcache = dlcache;
		if(pak->vpack)
		{
			dpsnprintf(search->filename, sizeof(search->filename), "%s/", pakfile);
			// if shortname ends with "pk3dir" or "dpkdir", strip that suffix to make it just "pk3" or "dpk"
			// same goes for the name inside the pack structure
			l = strlen(pak->shortname);
			if(l >= 7)
				if(!strcasecmp(pak->shortname + l - 7, ".pk3dir") || !strcasecmp(pak->shortname + l - 7, ".dpkdir"))
					pak->shortname[l - 3] = 0;
			l = strlen(pak->filename);
			if(l >= 7)
				if(!strcasecmp(pak->filename + l - 7, ".pk3dir") || !strcasecmp(pak->filename + l - 7, ".dpkdir"))
					pak->filename[l - 3] = 0;
		}
		return true;
	}
	else
	{
		Con_Printf(CON_ERROR "unable to load pak \"%s\"\n", pakfile);
		return false;
	}
}


/*
================
FS_AddPack
================
*/
/*! Adds the given pack to the search path and searches for it in the game path.
 * The pack type is autodetected by the file extension.
 *
 * Returns true if the file was successfully added to the
 * search path or if it was already included.
 *
 * If keep_plain_dirs is set, the pack will be added AFTER the first sequence of
 * plain directories.
 */
qbool FS_AddPack(const char *pakfile, qbool *already_loaded, qbool keep_plain_dirs, qbool dlcache)
{
	char fullpath[MAX_OSPATH];
	int index;
	searchpath_t *search;

	if(already_loaded)
		*already_loaded = false;

	// then find the real name...
	search = FS_FindFile(pakfile, &index, NULL, true);
	if(!search || search->pack)
	{
		Con_Printf("could not find pak \"%s\"\n", pakfile);
		return false;
	}

	dpsnprintf(fullpath, sizeof(fullpath), "%s%s", search->filename, pakfile);

	return FS_AddPack_Fullpath(fullpath, pakfile, already_loaded, keep_plain_dirs, dlcache);
}


/*
================
FS_AddGameDirectory

Sets fs_gamedir, adds the directory to the head of the path,
then loads and adds pak1.pak pak2.pak ...
================
*/
static void FS_AddGameDirectory (const char *dir)
{
	int i;
	stringlist_t list;
	searchpath_t *search;

	dp_strlcpy (fs_gamedir, dir, sizeof (fs_gamedir));

	stringlistinit(&list);
	listdirectory(&list, "", dir);
	stringlistsort(&list, false);

	// add any PAK package in the directory
	for (i = 0;i < list.numstrings;i++)
	{
		if (!strcasecmp(FS_FileExtension(list.strings[i]), "pak"))
		{
			FS_AddPack_Fullpath(list.strings[i], list.strings[i] + strlen(dir), NULL, false, false);
		}
	}

	// add any PK3 package in the directory
	for (i = 0;i < list.numstrings;i++)
	{
		if (!strcasecmp(FS_FileExtension(list.strings[i]), "pk3") || !strcasecmp(FS_FileExtension(list.strings[i]), "obb") || !strcasecmp(FS_FileExtension(list.strings[i]), "pk3dir")
			|| !strcasecmp(FS_FileExtension(list.strings[i]), "dpk") || !strcasecmp(FS_FileExtension(list.strings[i]), "dpkdir"))
		{
			FS_AddPack_Fullpath(list.strings[i], list.strings[i] + strlen(dir), NULL, false, false);
		}
	}

	stringlistfreecontents(&list);

	// Add the directory to the search path
	// (unpacked files have the priority over packed files)
	search = (searchpath_t *)Mem_Alloc(fs_mempool, sizeof(searchpath_t));
	dp_strlcpy (search->filename, dir, sizeof (search->filename));
	search->next = fs_searchpaths;
	fs_searchpaths = search;
}


/*
================
FS_AddGameHierarchy
================
*/
static void FS_AddGameHierarchy (const char *dir)
{
	char vabuf[1024];
	// Add the common game directory
	FS_AddGameDirectory (va(vabuf, sizeof(vabuf), "%s%s/", fs_basedir, dir));

	if (*fs_userdir)
		FS_AddGameDirectory(va(vabuf, sizeof(vabuf), "%s%s/", fs_userdir, dir));
}


/*
============
FS_FileExtension
============
*/
const char *FS_FileExtension (const char *in)
{
	const char *separator, *backslash, *colon, *dot;

	dot = strrchr(in, '.');
	if (dot == NULL)
		return "";

	separator = strrchr(in, '/');
	backslash = strrchr(in, '\\');
	if (!separator || separator < backslash)
		separator = backslash;
	colon = strrchr(in, ':');
	if (!separator || separator < colon)
		separator = colon;

	if (separator && (dot < separator))
		return "";

	return dot + 1;
}


/*
============
FS_FileWithoutPath
============
*/
const char *FS_FileWithoutPath (const char *in)
{
	const char *separator, *backslash, *colon;

	separator = strrchr(in, '/');
	backslash = strrchr(in, '\\');
	if (!separator || separator < backslash)
		separator = backslash;
	colon = strrchr(in, ':');
	if (!separator || separator < colon)
		separator = colon;
	return separator ? separator + 1 : in;
}


/*
================
FS_ClearSearchPath
================
*/
static void FS_ClearSearchPath (void)
{
	// unload all packs and directory information, close all pack files
	// (if a qfile is still reading a pack it won't be harmed because it used
	//  dup() to get its own handle already)
	while (fs_searchpaths)
	{
		searchpath_t *search = fs_searchpaths;
		fs_searchpaths = search->next;
		if (search->pack && search->pack != fs_selfpack)
		{
			if(!search->pack->vpack)
			{
				// close the file
				FILEDESC_CLOSE(search->pack->handle);
				// free any memory associated with it
				if (search->pack->files)
					Mem_Free(search->pack->files);
			}
			Mem_Free(search->pack);
		}
		Mem_Free(search);
	}
}

/*
================
FS_UnloadPacks_dlcache

Like FS_ClearSearchPath() but unloads only the packs loaded from dlcache
so we don't need to use a full FS_Rescan() to prevent
content from the previous server and/or map from interfering with the next
================
*/
void FS_UnloadPacks_dlcache(void)
{
	searchpath_t *search = fs_searchpaths, *searchprev = fs_searchpaths, *searchnext;

	if (!fs_unload_dlcache.integer)
		return;

	while (search)
	{
		searchnext = search->next;
		if (search->pack && search->pack->dlcache)
		{
			Con_DPrintf("Unloading pack: %s\n", search->pack->shortname);

			// remove it from the search path list
			if (search == fs_searchpaths)
				fs_searchpaths = search->next;
			else
				searchprev->next = search->next;

			// close the file
			FILEDESC_CLOSE(search->pack->handle);
			// free any memory associated with it
			if (search->pack->files)
				Mem_Free(search->pack->files);
			Mem_Free(search->pack);
			Mem_Free(search);
		}
		else
			searchprev = search;
		search = searchnext;
	}
}

static void FS_AddSelfPack(void)
{
	if(fs_selfpack)
	{
		searchpath_t *search;
		search = (searchpath_t *)Mem_Alloc(fs_mempool, sizeof(searchpath_t));
		search->next = fs_searchpaths;
		search->pack = fs_selfpack;
		fs_searchpaths = search;
	}
}


/*
================
FS_Rescan
================
*/
void FS_Rescan (void)
{
	int i;
	qbool fs_modified = false;
	qbool reset = false;
	char gamedirbuf[MAX_INPUTLINE];
	char vabuf[1024];

	if (fs_searchpaths)
		reset = true;
	FS_ClearSearchPath();

	// automatically activate gamemode for the gamedirs specified
	if (reset)
		COM_ChangeGameTypeForGameDirs();

	// add the game-specific paths
	// gamedirname1 (typically id1)
	FS_AddGameHierarchy (gamedirname1);
	// update the com_modname (used for server info)
	if (gamedirname2 && gamedirname2[0])
		dp_strlcpy(com_modname, gamedirname2, sizeof(com_modname));
	else
		dp_strlcpy(com_modname, gamedirname1, sizeof(com_modname));

	// add the game-specific path, if any
	// (only used for mission packs and the like, which should set fs_modified)
	if (gamedirname2 && gamedirname2[0])
	{
		fs_modified = true;
		FS_AddGameHierarchy (gamedirname2);
	}

	// -game <gamedir>
	// Adds basedir/gamedir as an override game
	// LadyHavoc: now supports multiple -game directories
	// set the com_modname (reported in server info)
	*gamedirbuf = 0;
	for (i = 0;i < fs_numgamedirs;i++)
	{
		fs_modified = true;
		FS_AddGameHierarchy (fs_gamedirs[i]);
		// update the com_modname (used server info)
		dp_strlcpy (com_modname, fs_gamedirs[i], sizeof (com_modname));
		if(i)
			dp_strlcat(gamedirbuf, va(vabuf, sizeof(vabuf), " %s", fs_gamedirs[i]), sizeof(gamedirbuf));
		else
			dp_strlcpy(gamedirbuf, fs_gamedirs[i], sizeof(gamedirbuf));
	}
	Cvar_SetQuick(&cvar_fs_gamedir, gamedirbuf); // so QC or console code can query it

	// add back the selfpack as new first item
	FS_AddSelfPack();

	if (cls.state != ca_dedicated)
	{
		// set the default screenshot name to either the mod name or the
		// gamemode screenshot name
		if (strcmp(com_modname, gamedirname1))
			Cvar_SetQuick (&scr_screenshot_name, com_modname);
		else
			Cvar_SetQuick (&scr_screenshot_name, gamescreenshotname);
	}

	if((i = Sys_CheckParm("-modname")) && i < sys.argc - 1)
		dp_strlcpy(com_modname, sys.argv[i+1], sizeof(com_modname));

	// If "-condebug" is in the command line, remove the previous log file
	if (Sys_CheckParm ("-condebug") != 0)
		unlink (va(vabuf, sizeof(vabuf), "%s/qconsole.log", fs_gamedir));

	// look for the pop.lmp file and set registered to true if it is found
	if (FS_FileExists("gfx/pop.lmp"))
		Cvar_SetValueQuick(&registered, 1);
	switch(gamemode)
	{
	case GAME_NORMAL:
	case GAME_HIPNOTIC:
	case GAME_ROGUE:
		if (!registered.integer)
		{
			if (fs_modified)
				Con_Print("Playing shareware version, with modification.\nwarning: most mods require full quake data.\n");
			else
				Con_Print("Playing shareware version.\n");
		}
		else
			Con_Print("Playing registered version.\n");
		break;
	case GAME_STEELSTORM:
		if (registered.integer)
			Con_Print("Playing registered version.\n");
		else
			Con_Print("Playing shareware version.\n");
		break;
	default:
		break;
	}

	// unload all wads so that future queries will return the new data
	W_UnloadAll();
}

static void FS_Rescan_f(cmd_state_t *cmd)
{
	FS_Rescan();
}

/*
================
FS_ChangeGameDirs
================
*/
extern qbool vid_opened;
qbool FS_ChangeGameDirs(int numgamedirs, char gamedirs[][MAX_QPATH], qbool complain, qbool failmissing)
{
	int i;
	const char *p;

	if (fs_numgamedirs == numgamedirs)
	{
		for (i = 0;i < numgamedirs;i++)
			if (strcasecmp(fs_gamedirs[i], gamedirs[i]))
				break;
		if (i == numgamedirs)
			return true; // already using this set of gamedirs, do nothing
	}

	if (numgamedirs > MAX_GAMEDIRS)
	{
		if (complain)
			Con_Printf("That is too many gamedirs (%i > %i)\n", numgamedirs, MAX_GAMEDIRS);
		return false; // too many gamedirs
	}

	for (i = 0;i < numgamedirs;i++)
	{
		// if string is nasty, reject it
		p = FS_CheckGameDir(gamedirs[i]);
		if(!p)
		{
			if (complain)
				Con_Printf("Nasty gamedir name rejected: %s\n", gamedirs[i]);
			return false; // nasty gamedirs
		}
		if(p == fs_checkgamedir_missing && failmissing)
		{
			if (complain)
				Con_Printf("Gamedir missing: %s%s/\n", fs_basedir, gamedirs[i]);
			return false; // missing gamedirs
		}
	}

	Host_SaveConfig(CONFIGFILENAME);

	fs_numgamedirs = numgamedirs;
	for (i = 0;i < fs_numgamedirs;i++)
		dp_strlcpy(fs_gamedirs[i], gamedirs[i], sizeof(fs_gamedirs[i]));

	// reinitialize filesystem to detect the new paks
	FS_Rescan();

	if (cls.demoplayback)
	{
		CL_Disconnect();
		cls.demonum = 0;
	}

	// unload all sounds so they will be reloaded from the new files as needed
	S_UnloadAllSounds_f(cmd_local);

	// reset everything that can be and reload configs
	Cbuf_InsertText(cmd_local, "\nloadconfig\n");

	return true;
}

/*
================
FS_GameDir_f
================
*/
static void FS_GameDir_f(cmd_state_t *cmd)
{
	int i;
	int numgamedirs;
	char gamedirs[MAX_GAMEDIRS][MAX_QPATH];

	if (Cmd_Argc(cmd) < 2)
	{
		Con_Printf("gamedirs active:");
		for (i = 0;i < fs_numgamedirs;i++)
			Con_Printf(" %s", fs_gamedirs[i]);
		Con_Printf("\n");
		return;
	}

	numgamedirs = Cmd_Argc(cmd) - 1;
	if (numgamedirs > MAX_GAMEDIRS)
	{
		Con_Printf("Too many gamedirs (%i > %i)\n", numgamedirs, MAX_GAMEDIRS);
		return;
	}

	for (i = 0;i < numgamedirs;i++)
		dp_strlcpy(gamedirs[i], Cmd_Argv(cmd, i+1), sizeof(gamedirs[i]));

	if ((cls.state == ca_connected && !cls.demoplayback) || sv.active)
	{
		// actually, changing during game would work fine, but would be stupid
		Con_Printf("Can not change gamedir while client is connected or server is running!\n");
		return;
	}

	// halt demo playback to close the file
	CL_Disconnect();

	FS_ChangeGameDirs(numgamedirs, gamedirs, true, true);
}

static const char *FS_SysCheckGameDir(const char *gamedir, char *buf, size_t buflength)
{
	qbool success;
	qfile_t *f;
	stringlist_t list;
	fs_offset_t n;
	char vabuf[1024];

	stringlistinit(&list);
	listdirectory(&list, gamedir, "");
	success = list.numstrings > 0;
	stringlistfreecontents(&list);

	if(success)
	{
		f = FS_SysOpen(va(vabuf, sizeof(vabuf), "%smodinfo.txt", gamedir), "r", false);
		if(f)
		{
			n = FS_Read (f, buf, buflength - 1);
			if(n >= 0)
				buf[n] = 0;
			else
				*buf = 0;
			FS_Close(f);
		}
		else
			*buf = 0;
		return buf;
	}

	return NULL;
}

/*
================
FS_CheckGameDir
================
*/
const char *FS_CheckGameDir(const char *gamedir)
{
	const char *ret;
	static char buf[8192];
	char vabuf[1024];

	if (FS_CheckNastyPath(gamedir, true))
		return NULL;

	ret = FS_SysCheckGameDir(va(vabuf, sizeof(vabuf), "%s%s/", fs_userdir, gamedir), buf, sizeof(buf));
	if(ret)
	{
		if(!*ret)
		{
			// get description from basedir
			ret = FS_SysCheckGameDir(va(vabuf, sizeof(vabuf), "%s%s/", fs_basedir, gamedir), buf, sizeof(buf));
			if(ret)
				return ret;
			return "";
		}
		return ret;
	}

	ret = FS_SysCheckGameDir(va(vabuf, sizeof(vabuf), "%s%s/", fs_basedir, gamedir), buf, sizeof(buf));
	if(ret)
		return ret;

	return fs_checkgamedir_missing;
}

static void FS_ListGameDirs(void)
{
	stringlist_t list, list2;
	int i;
	const char *info;
	char vabuf[1024];

	fs_all_gamedirs_count = 0;
	if(fs_all_gamedirs)
		Mem_Free(fs_all_gamedirs);

	stringlistinit(&list);
	listdirectory(&list, va(vabuf, sizeof(vabuf), "%s/", fs_basedir), "");
	listdirectory(&list, va(vabuf, sizeof(vabuf), "%s/", fs_userdir), "");
	stringlistsort(&list, false);

	stringlistinit(&list2);
	for(i = 0; i < list.numstrings; ++i)
	{
		if(i)
			if(!strcmp(list.strings[i-1], list.strings[i]))
				continue;
		info = FS_CheckGameDir(list.strings[i]);
		if(!info)
			continue;
		if(info == fs_checkgamedir_missing)
			continue;
		if(!*info)
			continue;
		stringlistappend(&list2, list.strings[i]);
	}
	stringlistfreecontents(&list);

	fs_all_gamedirs = (gamedir_t *)Mem_Alloc(fs_mempool, list2.numstrings * sizeof(*fs_all_gamedirs));
	for(i = 0; i < list2.numstrings; ++i)
	{
		info = FS_CheckGameDir(list2.strings[i]);
		// all this cannot happen any more, but better be safe than sorry
		if(!info)
			continue;
		if(info == fs_checkgamedir_missing)
			continue;
		if(!*info)
			continue;
		dp_strlcpy(fs_all_gamedirs[fs_all_gamedirs_count].name, list2.strings[i], sizeof(fs_all_gamedirs[fs_all_gamedirs_count].name));
		dp_strlcpy(fs_all_gamedirs[fs_all_gamedirs_count].description, info, sizeof(fs_all_gamedirs[fs_all_gamedirs_count].description));
		++fs_all_gamedirs_count;
	}
}

/*
#ifdef WIN32
#pragma comment(lib, "shell32.lib")
#include <ShlObj.h>
#endif
*/

static void COM_InsertFlags(const char *buf) {
	const char *p;
	char *q;
	const char **new_argv;
	int i = 0;
	int args_left = 256;
	new_argv = (const char **)Mem_Alloc(fs_mempool, sizeof(*sys.argv) * (sys.argc + args_left + 2));
	if(sys.argc == 0)
		new_argv[0] = "dummy";  // Can't really happen.
	else
		new_argv[0] = sys.argv[0];
	++i;
	p = buf;
	while(COM_ParseToken_Console(&p))
	{
		size_t sz = strlen(com_token) + 1; // shut up clang
		if(i > args_left)
			break;
		q = (char *)Mem_Alloc(fs_mempool, sz);
		dp_strlcpy(q, com_token, sz);
		new_argv[i] = q;
		++i;
	}
	// Now: i <= args_left + 1.
	if (sys.argc >= 1)
	{
		memcpy((char *)(&new_argv[i]), &sys.argv[1], sizeof(*sys.argv) * (sys.argc - 1));
		i += sys.argc - 1;
	}
	// Now: i <= args_left + (sys.argc || 1).
	new_argv[i] = NULL;
	sys.argv = new_argv;
	sys.argc = i;
}

static int FS_ChooseUserDir(userdirmode_t userdirmode, char *userdir, size_t userdirsize)
{
#if defined(__IPHONEOS__)
	if (userdirmode == USERDIRMODE_HOME)
	{
		// fs_basedir is "" by default, to utilize this you can simply add your gamedir to the Resources in xcode
		// fs_userdir stores configurations to the Documents folder of the app
		dp_strlcpy(userdir, "../Documents/", MAX_OSPATH);
		return 1;
	}
	return -1;

#elif defined(WIN32)
	char homedir[WSTRBUF];
	wchar *homedirw;
#if _MSC_VER >= 1400
	size_t homedirwlen;
#endif
	wchar_t mydocsdirw[WSTRBUF];
	char mydocsdir[WSTRBUF];
	wchar_t *savedgamesdirw;
	char savedgamesdir[WSTRBUF] = {0};
	int fd;
	char vabuf[1024];

	userdir[0] = 0;
	switch(userdirmode)
	{
	default:
		return -1;
	case USERDIRMODE_NOHOME:
		dp_strlcpy(userdir, fs_basedir, userdirsize);
		break;
	case USERDIRMODE_MYGAMES:
		if (!shfolder_dll)
			Sys_LoadDependency(shfolderdllnames, &shfolder_dll, shfolderfuncs);
		mydocsdir[0] = 0;
		if (qSHGetFolderPath && qSHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, mydocsdirw) == S_OK)
		{
			narrow(mydocsdirw, mydocsdir);
			dpsnprintf(userdir, userdirsize, "%s/My Games/%s/", mydocsdir, gameuserdirname);
			break;
		}
#if _MSC_VER >= 1400
		_wdupenv_s(&homedirw, &homedirwlen, L"USERPROFILE");
		narrow(homedirw, homedir);
		if(homedir[0])
		{
			dpsnprintf(userdir, userdirsize, "%s/.%s/", homedir, gameuserdirname);
			free(homedirw);
			break;
		}
#else
		homedirw = _wgetenv(L"USERPROFILE");
		narrow(homedirw, homedir);
		if(homedir[0])
		{
			dpsnprintf(userdir, userdirsize, "%s/.%s/", homedir, gameuserdirname);
			break;
		}
#endif
		return -1;
	case USERDIRMODE_SAVEDGAMES:
		if (!shell32_dll)
			Sys_LoadDependency(shell32dllnames, &shell32_dll, shell32funcs);
		if (!ole32_dll)
			Sys_LoadDependency(ole32dllnames, &ole32_dll, ole32funcs);
		if (qSHGetKnownFolderPath && qCoInitializeEx && qCoTaskMemFree && qCoUninitialize)
		{
			savedgamesdir[0] = 0;
			qCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
/*
#ifdef __cplusplus
			if (SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_CREATE | KF_FLAG_NO_ALIAS, NULL, &savedgamesdirw) == S_OK)
#else
			if (SHGetKnownFolderPath(&FOLDERID_SavedGames, KF_FLAG_CREATE | KF_FLAG_NO_ALIAS, NULL, &savedgamesdirw) == S_OK)
#endif
*/
			if (qSHGetKnownFolderPath(&qFOLDERID_SavedGames, qKF_FLAG_CREATE | qKF_FLAG_NO_ALIAS, NULL, &savedgamesdirw) == S_OK)
			{
				narrow(savedgamesdirw, savedgamesdir);
				qCoTaskMemFree(savedgamesdirw);
			}
			qCoUninitialize();
			if (savedgamesdir[0])
			{
				dpsnprintf(userdir, userdirsize, "%s/%s/", savedgamesdir, gameuserdirname);
				break;
			}
		}
		return -1;
	}
#else
	int fd;
	char *homedir;
	char vabuf[1024];
	userdir[0] = 0;
	switch(userdirmode)
	{
	default:
		return -1;
	case USERDIRMODE_NOHOME:
		dp_strlcpy(userdir, fs_basedir, userdirsize);
		break;
	case USERDIRMODE_HOME:
		homedir = getenv("HOME");
		if(homedir)
		{
			dpsnprintf(userdir, userdirsize, "%s/.%s/", homedir, gameuserdirname);
			break;
		}
		return -1;
	case USERDIRMODE_SAVEDGAMES:
		homedir = getenv("HOME");
		if(homedir)
		{
#ifdef MACOSX
			dpsnprintf(userdir, userdirsize, "%s/Library/Application Support/%s/", homedir, gameuserdirname);
#else
			// the XDG say some files would need to go in:
			// XDG_CONFIG_HOME (or ~/.config/%s/)
			// XDG_DATA_HOME (or ~/.local/share/%s/)
			// XDG_CACHE_HOME (or ~/.cache/%s/)
			// and also search the following global locations if defined:
			// XDG_CONFIG_DIRS (normally /etc/xdg/%s/)
			// XDG_DATA_DIRS (normally /usr/share/%s/)
			// this would be too complicated...
			return -1;
#endif
			break;
		}
		return -1;
	}
#endif


#if !defined(__IPHONEOS__)

#ifdef WIN32
	// historical behavior...
	if (userdirmode == USERDIRMODE_NOHOME && strcmp(gamedirname1, "id1"))
		return 0; // don't bother checking if the basedir folder is writable, it's annoying...  unless it is Quake on Windows where NOHOME is the default preferred and we have to check for an error case
#endif

	// see if we can write to this path (note: won't create path)
#ifdef WIN32
	// no access() here, we must try to open the file for appending
	fd = FS_SysOpenFiledesc(va(vabuf, sizeof(vabuf), "%s%s/config.cfg", userdir, gamedirname1), "a", false);
	if(fd >= 0)
		FILEDESC_CLOSE(fd);
#else
	// on Unix, we don't need to ACTUALLY attempt to open the file
	if(access(va(vabuf, sizeof(vabuf), "%s%s/", userdir, gamedirname1), W_OK | X_OK) >= 0)
		fd = 1;
	else
		fd = -1;
#endif
	if(fd >= 0)
	{
		return 1; // good choice - the path exists and is writable
	}
	else
	{
		if (userdirmode == USERDIRMODE_NOHOME)
			return -1; // path usually already exists, we lack permissions
		else
			return 0; // probably good - failed to write but maybe we need to create path
	}
#endif
}

void FS_Init_Commands(void)
{
	Cvar_RegisterVariable (&scr_screenshot_name);
	Cvar_RegisterVariable (&fs_empty_files_in_pack_mark_deletions);
	Cvar_RegisterVariable (&fs_unload_dlcache);
	Cvar_RegisterVariable (&cvar_fs_gamedir);

	Cmd_AddCommand(CF_SHARED, "gamedir", FS_GameDir_f, "changes active gamedir list, can take multiple arguments which shouldn't include the base directory, the last gamedir is the \"primary\" and files will be saved there (example usage: gamedir ctf id1)");
	Cmd_AddCommand(CF_SHARED, "fs_rescan", FS_Rescan_f, "rescans filesystem for new pack archives and any other changes");
	Cmd_AddCommand(CF_SHARED, "path", FS_Path_f, "print searchpath (game directories and archives)");
	Cmd_AddCommand(CF_SHARED, "dir", FS_Dir_f, "list files in searchpath matching an * filename pattern, one per line");
	Cmd_AddCommand(CF_SHARED, "ls", FS_Ls_f, "list files in searchpath matching an * filename pattern, multiple per line");
	Cmd_AddCommand(CF_SHARED, "which", FS_Which_f, "accepts a file name as argument and reports where the file is taken from");
}

static void FS_Init_Dir (void)
{
	const char *p;
	int i;

	*fs_basedir = 0;
	*fs_userdir = 0;
	*fs_gamedir = 0;

	// -basedir <path>
	// Overrides the system supplied base directory (under GAMENAME)
// COMMANDLINEOPTION: Filesystem: -basedir <path> chooses what base directory the game data is in, inside this there should be a data directory for the game (for example id1)
	i = Sys_CheckParm ("-basedir");
	if (i && i < sys.argc-1)
	{
		dp_strlcpy (fs_basedir, sys.argv[i+1], sizeof (fs_basedir));
		i = (int)strlen (fs_basedir);
		if (i > 0 && (fs_basedir[i-1] == '\\' || fs_basedir[i-1] == '/'))
			fs_basedir[i-1] = 0;
	}
	else
	{
// If the base directory is explicitly defined by the compilation process
#ifdef DP_FS_BASEDIR
		dp_strlcpy(fs_basedir, DP_FS_BASEDIR, sizeof(fs_basedir));
#elif defined(__ANDROID__)
#ifdef _DIII4A
		extern const char * Sys_GameDataDefaultPath(void);
		dpsnprintf(fs_basedir, sizeof(fs_basedir), "%s/%s/", Sys_GameDataDefaultPath(), gameuserdirname);
#else
		dpsnprintf(fs_basedir, sizeof(fs_basedir), "/sdcard/%s/", gameuserdirname);
#endif
#elif defined(MACOSX)
		// FIXME: is there a better way to find the directory outside the .app, without using Objective-C?
		if (strstr(sys.argv[0], ".app/"))
		{
			char *split;
			dp_strlcpy(fs_basedir, sys.argv[0], sizeof(fs_basedir));
			split = strstr(fs_basedir, ".app/");
			if (split)
			{
				struct stat statresult;
				char vabuf[1024];
				// truncate to just after the .app/
				split[5] = 0;
				// see if gamedir exists in Resources
				if (stat(va(vabuf, sizeof(vabuf), "%s/Contents/Resources/%s", fs_basedir, gamedirname1), &statresult) == 0)
				{
					// found gamedir inside Resources, use it
					dp_strlcat(fs_basedir, "Contents/Resources/", sizeof(fs_basedir));
				}
				else
				{
					// no gamedir found in Resources, gamedir is probably
					// outside the .app, remove .app part of path
					while (split > fs_basedir && *split != '/')
						split--;
					*split = 0;
				}
			}
		}
#else
		// use the working directory
		getcwd(fs_basedir, sizeof(fs_basedir));
#endif
	}

	// make sure the appending of a path separator won't create an unterminated string
	memset(fs_basedir + sizeof(fs_basedir) - 2, 0, 2);
	// add a path separator to the end of the basedir if it lacks one
	if (fs_basedir[0] && fs_basedir[strlen(fs_basedir) - 1] != '/' && fs_basedir[strlen(fs_basedir) - 1] != '\\')
		dp_strlcat(fs_basedir, "/", sizeof(fs_basedir));

	// Add the personal game directory
	if((i = Sys_CheckParm("-userdir")) && i < sys.argc - 1)
		dpsnprintf(fs_userdir, sizeof(fs_userdir), "%s/", sys.argv[i+1]);
	else if (Sys_CheckParm("-nohome"))
		*fs_userdir = 0; // user wants roaming installation, no userdir
	else
	{
#ifdef DP_FS_USERDIR
		dp_strlcpy(fs_userdir, DP_FS_USERDIR, sizeof(fs_userdir));
#else
		int dirmode;
		int highestuserdirmode = USERDIRMODE_COUNT - 1;
		int preferreduserdirmode = USERDIRMODE_COUNT - 1;
		int userdirstatus[USERDIRMODE_COUNT];
# ifdef WIN32
		// historical behavior...
		if (!strcmp(gamedirname1, "id1"))
			preferreduserdirmode = USERDIRMODE_NOHOME;
# endif
		// check what limitations the user wants to impose
		if (Sys_CheckParm("-home")) preferreduserdirmode = USERDIRMODE_HOME;
		if (Sys_CheckParm("-mygames")) preferreduserdirmode = USERDIRMODE_MYGAMES;
		if (Sys_CheckParm("-savedgames")) preferreduserdirmode = USERDIRMODE_SAVEDGAMES;
		// gather the status of the possible userdirs
		for (dirmode = 0;dirmode < USERDIRMODE_COUNT;dirmode++)
		{
			userdirstatus[dirmode] = FS_ChooseUserDir((userdirmode_t)dirmode, fs_userdir, sizeof(fs_userdir));
			if (userdirstatus[dirmode] == 1)
				Con_DPrintf("userdir %i = %s (writable)\n", dirmode, fs_userdir);
			else if (userdirstatus[dirmode] == 0)
				Con_DPrintf("userdir %i = %s (not writable or does not exist)\n", dirmode, fs_userdir);
			else
				Con_DPrintf("userdir %i (not applicable)\n", dirmode);
		}
		// some games may prefer writing to basedir, but if write fails we
		// have to search for a real userdir...
		if (preferreduserdirmode == 0 && userdirstatus[0] < 1)
			preferreduserdirmode = highestuserdirmode;
		// check for an existing userdir and continue using it if possible...
		for (dirmode = USERDIRMODE_COUNT - 1;dirmode > 0;dirmode--)
			if (userdirstatus[dirmode] == 1)
				break;
		// if no existing userdir found, make a new one...
		if (dirmode == 0 && preferreduserdirmode > 0)
			for (dirmode = preferreduserdirmode;dirmode > 0;dirmode--)
				if (userdirstatus[dirmode] >= 0)
					break;
		// and finally, we picked one...
		FS_ChooseUserDir((userdirmode_t)dirmode, fs_userdir, sizeof(fs_userdir));
		Con_DPrintf("userdir %i is the winner\n", dirmode);
#endif
	}

	// if userdir equal to basedir, clear it to avoid confusion later
	if (!strcmp(fs_basedir, fs_userdir))
		fs_userdir[0] = 0;

	FS_ListGameDirs();

	p = FS_CheckGameDir(gamedirname1);
	if(!p || p == fs_checkgamedir_missing)
		Con_Printf(CON_WARN "WARNING: base gamedir %s%s/ not found!\n", fs_basedir, gamedirname1);

	if(gamedirname2)
	{
		p = FS_CheckGameDir(gamedirname2);
		if(!p || p == fs_checkgamedir_missing)
			Con_Printf(CON_WARN "WARNING: base gamedir %s%s/ not found!\n", fs_basedir, gamedirname2);
	}

	// -game <gamedir>
	// Adds basedir/gamedir as an override game
	// LadyHavoc: now supports multiple -game directories
	for (i = 1;i < sys.argc && fs_numgamedirs < MAX_GAMEDIRS;i++)
	{
		if (!sys.argv[i])
			continue;
		if (!strcmp (sys.argv[i], "-game") && i < sys.argc-1)
		{
			i++;
			p = FS_CheckGameDir(sys.argv[i]);
			if(!p)
				Con_Printf("WARNING: Nasty -game name rejected: %s\n", sys.argv[i]);
			if(p == fs_checkgamedir_missing)
				Con_Printf(CON_WARN "WARNING: -game %s%s/ not found!\n", fs_basedir, sys.argv[i]);
			// add the gamedir to the list of active gamedirs
			dp_strlcpy (fs_gamedirs[fs_numgamedirs], sys.argv[i], sizeof(fs_gamedirs[fs_numgamedirs]));
			fs_numgamedirs++;
		}
	}

	// generate the searchpath
	FS_Rescan();

	if (Thread_HasThreads())
		fs_mutex = Thread_CreateMutex();
}

/*
================
FS_Init_SelfPack
================
*/
void FS_Init_SelfPack (void)
{
	char *buf;

	// Load darkplaces.opt from the FS.
	if (!Sys_CheckParm("-noopt"))
	{
		buf = (char *) FS_SysLoadFile("darkplaces.opt", tempmempool, true, NULL);
		if(buf)
		{
			COM_InsertFlags(buf);
			Mem_Free(buf);
		}
	}

#ifndef USE_RWOPS
	// Provide the SelfPack.
	if (!Sys_CheckParm("-noselfpack") && sys.selffd >= 0)
	{
		fs_selfpack = FS_LoadPackPK3FromFD(sys.argv[0], sys.selffd, true);
		if(fs_selfpack)
		{
			FS_AddSelfPack();
			if (!Sys_CheckParm("-noopt"))
			{
				buf = (char *) FS_LoadFile("darkplaces.opt", tempmempool, true, NULL);
				if(buf)
				{
					COM_InsertFlags(buf);
					Mem_Free(buf);
				}
			}
		}
	}
#endif
}

/*
================
FS_Init
================
*/

void FS_Init(void)
{
	fs_mempool = Mem_AllocPool("file management", 0, NULL);

	FS_Init_Commands();

	PK3_OpenLibrary ();

	// initialize the self-pack (must be before COM_InitGameType as it may add command line options)
	FS_Init_SelfPack();

	// detect gamemode from commandline options or executable name
	COM_InitGameType();

	FS_Init_Dir();
}

/*
================
FS_Shutdown
================
*/
void FS_Shutdown (void)
{
	// close all pack files and such
	// (hopefully there aren't any other open files, but they'll be cleaned up
	//  by the OS anyway)
	FS_ClearSearchPath();
	Mem_FreePool (&fs_mempool);
	PK3_CloseLibrary ();

#ifdef WIN32
	Sys_FreeLibrary (&shfolder_dll);
	Sys_FreeLibrary (&shell32_dll);
	Sys_FreeLibrary (&ole32_dll);
#endif

	if (fs_mutex)
		Thread_DestroyMutex(fs_mutex);
}

static filedesc_t FS_SysOpenFiledesc(const char *filepath, const char *mode, qbool nonblocking)
{
	filedesc_t handle = FILEDESC_INVALID;
	int mod, opt;
	unsigned int ind;
	qbool dolock = false;
	#ifdef WIN32
	wchar filepathw[WSTRBUF] = {0};
	#endif

	// Parse the mode string
	switch (mode[0])
	{
		case 'r':
			mod = O_RDONLY;
			opt = 0;
			break;
		case 'w':
			mod = O_WRONLY;
			opt = O_CREAT | O_TRUNC;
			break;
		case 'a':
			mod = O_WRONLY;
			opt = O_CREAT | O_APPEND;
			break;
		default:
			Con_Printf(CON_ERROR "FS_SysOpen(%s, %s): invalid mode\n", filepath, mode);
			return FILEDESC_INVALID;
	}
	for (ind = 1; mode[ind] != '\0'; ind++)
	{
		switch (mode[ind])
		{
			case '+':
				mod = O_RDWR;
				break;
			case 'b':
				opt |= O_BINARY;
				break;
			case 'l':
				dolock = true;
				break;
			default:
				Con_Printf(CON_ERROR "FS_SysOpen(%s, %s): unknown character in mode (%c)\n",
							filepath, mode, mode[ind]);
		}
	}

	if (nonblocking)
		opt |= O_NONBLOCK;

	if(Sys_CheckParm("-readonly") && mod != O_RDONLY)
		return FILEDESC_INVALID;

#if USE_RWOPS
	if (dolock)
		return FILEDESC_INVALID;
	handle = SDL_RWFromFile(filepath, mode);
#else
# ifdef WIN32
	widen(filepath, filepathw);
#  if _MSC_VER >= 1400
	_wsopen_s(&handle, filepathw, mod | opt, (dolock ? ((mod == O_RDONLY) ? _SH_DENYRD : _SH_DENYRW) : _SH_DENYNO), _S_IREAD | _S_IWRITE);
#  else
	handle = _wsopen (filepathw, mod | opt, (dolock ? ((mod == O_RDONLY) ? _SH_DENYRD : _SH_DENYRW) : _SH_DENYNO), _S_IREAD | _S_IWRITE);
#  endif
# else
	handle = open (filepath, mod | opt, 0666);
	if(handle >= 0 && dolock)
	{
		struct flock l;
		l.l_type = ((mod == O_RDONLY) ? F_RDLCK : F_WRLCK);
		l.l_whence = SEEK_SET;
		l.l_start = 0;
		l.l_len = 0;
		if(fcntl(handle, F_SETLK, &l) == -1)
		{
			FILEDESC_CLOSE(handle);
			handle = -1;
		}
	}
# endif
#endif

	return handle;
}

int FS_SysOpenFD(const char *filepath, const char *mode, qbool nonblocking)
{
#ifdef USE_RWOPS
	return -1;
#else
	return FS_SysOpenFiledesc(filepath, mode, nonblocking);
#endif
}

/*
====================
FS_SysOpen

Internal function used to create a qfile_t and open the relevant non-packed file on disk
====================
*/
qfile_t* FS_SysOpen (const char* filepath, const char* mode, qbool nonblocking)
{
	qfile_t* file;

	file = (qfile_t *)Mem_Alloc (fs_mempool, sizeof (*file));
	file->ungetc = EOF;
	file->handle = FS_SysOpenFiledesc(filepath, mode, nonblocking);
	if (!FILEDESC_ISVALID(file->handle))
	{
		Mem_Free (file);
		return NULL;
	}

	file->filename = Mem_strdup(fs_mempool, filepath);

	file->real_length = FILEDESC_SEEK (file->handle, 0, SEEK_END);

	// For files opened in append mode, we start at the end of the file
	if (mode[0] == 'a')
		file->position = file->real_length;
	else
		FILEDESC_SEEK (file->handle, 0, SEEK_SET);

	return file;
}


/*
===========
FS_OpenPackedFile

Open a packed file using its package file descriptor
===========
*/
static qfile_t *FS_OpenPackedFile (pack_t* pack, int pack_ind)
{
	packfile_t *pfile;
	filedesc_t dup_handle;
	qfile_t* file;

	pfile = &pack->files[pack_ind];

	// If we don't have the true offset, get it now
	if (! (pfile->flags & PACKFILE_FLAG_TRUEOFFS))
		if (!PK3_GetTrueFileOffset (pfile, pack))
			return NULL;

#ifndef LINK_TO_ZLIB
	// No Zlib DLL = no compressed files
	if (!zlib_dll && (pfile->flags & PACKFILE_FLAG_DEFLATED))
	{
		Con_Printf(CON_WARN "WARNING: can't open the compressed file %s\n"
					"You need the Zlib DLL to use compressed files\n",
					pfile->name);
		return NULL;
	}
#endif

	// LadyHavoc: FILEDESC_SEEK affects all duplicates of a handle so we do it before
	// the dup() call to avoid having to close the dup_handle on error here
	if (FILEDESC_SEEK (pack->handle, pfile->offset, SEEK_SET) == -1)
	{
		Con_Printf ("FS_OpenPackedFile: can't lseek to %s in %s (offset: %08x%08x)\n",
					pfile->name, pack->filename, (unsigned int)(pfile->offset >> 32), (unsigned int)(pfile->offset));
		return NULL;
	}

	dup_handle = FILEDESC_DUP (pack->filename, pack->handle);
	if (!FILEDESC_ISVALID(dup_handle))
	{
		Con_Printf ("FS_OpenPackedFile: can't dup package's handle (pack: %s)\n", pack->filename);
		return NULL;
	}

	file = (qfile_t *)Mem_Alloc (fs_mempool, sizeof (*file));
	memset (file, 0, sizeof (*file));
	file->handle = dup_handle;
	file->flags = QFILE_FLAG_PACKED;
	file->real_length = pfile->realsize;
	file->offset = pfile->offset;
	file->position = 0;
	file->ungetc = EOF;

	if (pfile->flags & PACKFILE_FLAG_DEFLATED)
	{
		ztoolkit_t *ztk;

		file->flags |= QFILE_FLAG_DEFLATED;

		// We need some more variables
		ztk = (ztoolkit_t *)Mem_Alloc (fs_mempool, sizeof (*ztk));

		ztk->comp_length = pfile->packsize;

		// Initialize zlib stream
		ztk->zstream.next_in = ztk->input;
		ztk->zstream.avail_in = 0;

		/* From Zlib's "unzip.c":
		 *
		 * windowBits is passed < 0 to tell that there is no zlib header.
		 * Note that in this case inflate *requires* an extra "dummy" byte
		 * after the compressed stream in order to complete decompression and
		 * return Z_STREAM_END.
		 * In unzip, i don't wait absolutely Z_STREAM_END because I known the
		 * size of both compressed and uncompressed data
		 */
		if (qz_inflateInit2 (&ztk->zstream, -MAX_WBITS) != Z_OK)
		{
			Con_Printf ("FS_OpenPackedFile: inflate init error (file: %s)\n", pfile->name);
			FILEDESC_CLOSE(dup_handle);
			Mem_Free(file);
			return NULL;
		}

		ztk->zstream.next_out = file->buff;
		ztk->zstream.avail_out = sizeof (file->buff);

		file->ztk = ztk;
	}

	return file;
}

/*
====================
FS_CheckNastyPath

Return true if the path should be rejected due to one of the following:
1: path elements that are non-portable
2: path elements that would allow access to files outside the game directory,
   or are just not a good idea for a mod to be using.
====================
*/
int FS_CheckNastyPath (const char *path, qbool isgamedir)
{
	// all: never allow an empty path, as for gamedir it would access the parent directory and a non-gamedir path it is just useless
	if (!path[0])
		return 2;

	// Windows: don't allow \ in filenames (windows-only), period.
	// (on Windows \ is a directory separator, but / is also supported)
	if (strstr(path, "\\"))
		return 1; // non-portable

	// Mac: don't allow Mac-only filenames - : is a directory separator
	// instead of /, but we rely on / working already, so there's no reason to
	// support a Mac-only path
	// Amiga and Windows: : tries to go to root of drive
	if (strstr(path, ":"))
		return 1; // non-portable attempt to go to root of drive

	// Amiga: // is parent directory
	if (strstr(path, "//"))
		return 1; // non-portable attempt to go to parent directory

	// all: don't allow going to parent directory (../ or /../)
	if (strstr(path, ".."))
		return 2; // attempt to go outside the game directory

	// Windows and UNIXes: don't allow absolute paths
	if (path[0] == '/')
		return 2; // attempt to go outside the game directory

	// all: don't allow . character immediately before a slash, this catches all imaginable cases of ./, ../, .../, etc
	if (strstr(path, "./"))
		return 2; // possible attempt to go outside the game directory

	// all: forbid trailing slash on gamedir
	if (isgamedir && path[strlen(path)-1] == '/')
		return 2;

	// all: forbid leading dot on any filename for any reason
	if (strstr(path, "/."))
		return 2; // attempt to go outside the game directory

	// after all these checks we're pretty sure it's a / separated filename
	// and won't do much if any harm
	return false;
}

/*
====================
FS_SanitizePath

Sanitize path (replace non-portable characters
with portable ones in-place, etc)
====================
*/
void FS_SanitizePath(char *path)
{
	for (; *path; path++)
		if (*path == '\\')
			*path = '/';
}

/*
====================
FS_FindFile

Look for a file in the packages and in the filesystem

Return the searchpath where the file was found (or NULL)
and the file index in the package if relevant
====================
*/
static searchpath_t *FS_FindFile (const char *name, int *index, const char **canonicalname, qbool quiet)
{
	searchpath_t *search;
	pack_t *pak;

	// search through the path, one element at a time
	for (search = fs_searchpaths;search;search = search->next)
	{
		// is the element a pak file?
		if (search->pack && !search->pack->vpack)
		{
			int (*strcmp_funct) (const char* str1, const char* str2);
			int left, right, middle;

			pak = search->pack;
			strcmp_funct = pak->ignorecase ? strcasecmp : strcmp;

			// Look for the file (binary search)
			left = 0;
			right = pak->numfiles - 1;
			while (left <= right)
			{
				int diff;

				middle = (left + right) / 2;
				diff = strcmp_funct (pak->files[middle].name, name);

				// Found it
				if (!diff)
				{
					if (fs_empty_files_in_pack_mark_deletions.integer && pak->files[middle].realsize == 0)
					{
						// yes, but the first one is empty so we treat it as not being there
						if (!quiet && developer_extra.integer)
							Con_DPrintf("FS_FindFile: %s is marked as deleted\n", name);

						if (index != NULL)
							*index = -1;
						if (canonicalname)
							*canonicalname = NULL;
						return NULL;
					}

					if (!quiet && developer_extra.integer)
						Con_DPrintf("FS_FindFile: %s in %s\n", pak->files[middle].name, pak->filename);

					if (index != NULL)
						*index = middle;
					if (canonicalname)
						*canonicalname = pak->files[middle].name;
					return search;
				}

				// If we're too far in the list
				if (diff > 0)
					right = middle - 1;
				else
					left = middle + 1;
			}
		}
		else
		{
			char netpath[MAX_OSPATH];
			dpsnprintf(netpath, sizeof(netpath), "%s%s", search->filename, name);
			if (FS_SysFileExists (netpath))
			{
				if (!quiet && developer_extra.integer)
					Con_DPrintf("FS_FindFile: %s\n", netpath);

				if (index != NULL)
					*index = -1;
				if (canonicalname)
					*canonicalname = name;
				return search;
			}
		}
	}

	if (!quiet && developer_extra.integer)
		Con_DPrintf("FS_FindFile: can't find %s\n", name);

	if (index != NULL)
		*index = -1;
	if (canonicalname)
		*canonicalname = NULL;
	return NULL;
}


/*
===========
FS_OpenReadFile

Look for a file in the search paths and open it in read-only mode
===========
*/
static qfile_t *FS_OpenReadFile (const char *filename, qbool quiet, qbool nonblocking, int symlinkLevels)
{
	searchpath_t *search;
	int pack_ind;

	search = FS_FindFile (filename, &pack_ind, NULL, quiet);

	// Not found?
	if (search == NULL)
		return NULL;

	// Found in the filesystem?
	if (pack_ind < 0)
	{
		// this works with vpacks, so we are fine
		char path [MAX_OSPATH];
		dpsnprintf (path, sizeof (path), "%s%s", search->filename, filename);
		return FS_SysOpen (path, "rb", nonblocking);
	}

	// So, we found it in a package...

	// Is it a PK3 symlink?
	// TODO also handle directory symlinks by parsing the whole structure...
	// but heck, file symlinks are good enough for now
	if(search->pack->files[pack_ind].flags & PACKFILE_FLAG_SYMLINK)
	{
		if(symlinkLevels <= 0)
		{
			Con_Printf("symlink: %s: too many levels of symbolic links\n", filename);
			return NULL;
		}
		else
		{
			char linkbuf[MAX_QPATH];
			fs_offset_t count;
			qfile_t *linkfile = FS_OpenPackedFile (search->pack, pack_ind);
			const char *mergeslash;
			char *mergestart;

			if(!linkfile)
				return NULL;
			count = FS_Read(linkfile, linkbuf, sizeof(linkbuf) - 1);
			FS_Close(linkfile);
			if(count < 0)
				return NULL;
			linkbuf[count] = 0;

			// Now combine the paths...
			mergeslash = strrchr(filename, '/');
			mergestart = linkbuf;
			if(!mergeslash)
				mergeslash = filename;
			while(!strncmp(mergestart, "../", 3))
			{
				mergestart += 3;
				while(mergeslash > filename)
				{
					--mergeslash;
					if(*mergeslash == '/')
						break;
				}
			}
			// Now, mergestart will point to the path to be appended, and mergeslash points to where it should be appended
			if(mergeslash == filename)
			{
				// Either mergeslash == filename, then we just replace the name (done below)
			}
			else
			{
				// Or, we append the name after mergeslash;
				// or rather, we can also shift the linkbuf so we can put everything up to and including mergeslash first
				int spaceNeeded = mergeslash - filename + 1;
				int spaceRemoved = mergestart - linkbuf;
				if(count - spaceRemoved + spaceNeeded >= MAX_QPATH)
				{
					Con_DPrintf("symlink: too long path rejected\n");
					return NULL;
				}
				memmove(linkbuf + spaceNeeded, linkbuf + spaceRemoved, count - spaceRemoved);
				memcpy(linkbuf, filename, spaceNeeded);
				linkbuf[count - spaceRemoved + spaceNeeded] = 0;
				mergestart = linkbuf;
			}
			if (!quiet && developer_loading.integer)
				Con_DPrintf("symlink: %s -> %s\n", filename, mergestart);
			if(FS_CheckNastyPath (mergestart, false))
			{
				Con_DPrintf("symlink: nasty path %s rejected\n", mergestart);
				return NULL;
			}
			return FS_OpenReadFile(mergestart, quiet, nonblocking, symlinkLevels - 1);
		}
	}

	return FS_OpenPackedFile (search->pack, pack_ind);
}


/*
=============================================================================

MAIN PUBLIC FUNCTIONS

=============================================================================
*/

/*
====================
FS_OpenRealFile

Open a file in the userpath. The syntax is the same as fopen
Used for savegame scanning in menu, and all file writing.
====================
*/
qfile_t* FS_OpenRealFile (const char* filepath, const char* mode, qbool quiet)
{
	char real_path [MAX_OSPATH];

	if (FS_CheckNastyPath(filepath, false))
	{
		Con_Printf("FS_OpenRealFile(\"%s\", \"%s\", %s): nasty filename rejected\n", filepath, mode, quiet ? "true" : "false");
		return NULL;
	}

	dpsnprintf (real_path, sizeof (real_path), "%s/%s", fs_gamedir, filepath); // this is never a vpack

	// If the file is opened in "write", "append", or "read/write" mode,
	// create directories up to the file.
	if (mode[0] == 'w' || mode[0] == 'a' || strchr (mode, '+'))
		FS_CreatePath (real_path);
	return FS_SysOpen (real_path, mode, false);
}


/*
====================
FS_OpenVirtualFile

Open a file. The syntax is the same as fopen
====================
*/
qfile_t* FS_OpenVirtualFile (const char* filepath, qbool quiet)
{
	qfile_t *result = NULL;
	if (FS_CheckNastyPath(filepath, false))
	{
		Con_Printf("FS_OpenVirtualFile(\"%s\", %s): nasty filename rejected\n", filepath, quiet ? "true" : "false");
		return NULL;
	}

	if (fs_mutex) Thread_LockMutex(fs_mutex);
	result = FS_OpenReadFile (filepath, quiet, false, 16);
	if (fs_mutex) Thread_UnlockMutex(fs_mutex);
	return result;
}


/*
====================
FS_FileFromData

Open a file. The syntax is the same as fopen
====================
*/
qfile_t* FS_FileFromData (const unsigned char *data, const size_t size, qbool quiet)
{
	qfile_t* file;
	file = (qfile_t *)Mem_Alloc (fs_mempool, sizeof (*file));
	memset (file, 0, sizeof (*file));
	file->flags = QFILE_FLAG_DATA;
	file->ungetc = EOF;
	file->real_length = size;
	file->data = data;
	return file;
}

/*
====================
FS_Close

Close a file
====================
*/
int FS_Close (qfile_t* file)
{
	if(file->flags & QFILE_FLAG_DATA)
	{
		Mem_Free(file);
		return 0;
	}

	if (FILEDESC_CLOSE (file->handle))
		return EOF;

	if (file->filename)
	{
		if (file->flags & QFILE_FLAG_REMOVE)
		{
			if (remove(file->filename) == -1)
			{
				// No need to report this. If removing a just
				// written file failed, this most likely means
				// someone else deleted it first - which we
				// like.
			}
		}

		Mem_Free((void *) file->filename);
	}

	if (file->ztk)
	{
		qz_inflateEnd (&file->ztk->zstream);
		Mem_Free (file->ztk);
	}

	Mem_Free (file);
	return 0;
}

void FS_RemoveOnClose(qfile_t* file)
{
	file->flags |= QFILE_FLAG_REMOVE;
}

/*
====================
FS_Write

Write "datasize" bytes into a file
====================
*/
fs_offset_t FS_Write (qfile_t* file, const void* data, size_t datasize)
{
	fs_offset_t written = 0;

	// If necessary, seek to the exact file position we're supposed to be
	if (file->buff_ind != file->buff_len)
	{
		if (FILEDESC_SEEK (file->handle, file->buff_ind - file->buff_len, SEEK_CUR) == -1)
		{
			Con_Printf(CON_WARN "WARNING: could not seek in %s.\n", file->filename);
		}
	}

	// Purge cached data
	FS_Purge (file);

	// Write the buffer and update the position
	// LadyHavoc: to hush a warning about passing size_t to an unsigned int parameter on Win64 we do this as multiple writes if the size would be too big for an integer (we never write that big in one go, but it's a theory)
	while (written < (fs_offset_t)datasize)
	{
		// figure out how much to write in one chunk
		fs_offset_t maxchunk = 1<<30; // 1 GiB
		int chunk = (int)min((fs_offset_t)datasize - written, maxchunk);
		int result = (int)FILEDESC_WRITE (file->handle, (const unsigned char *)data + written, chunk);
		// if at least some was written, add it to our accumulator
		if (result > 0)
			written += result;
		// if the result is not what we expected, consider the write to be incomplete
		if (result != chunk)
			break;
	}
	file->position = FILEDESC_SEEK (file->handle, 0, SEEK_CUR);
	if (file->real_length < file->position)
		file->real_length = file->position;

	// note that this will never be less than 0 even if the write failed
	return written;
}


/*
====================
FS_Read

Read up to "buffersize" bytes from a file
====================
*/
fs_offset_t FS_Read (qfile_t* file, void* buffer, size_t buffersize)
{
	fs_offset_t count, done;

	if (buffersize == 0 || !buffer)
		return 0;

	// Get rid of the ungetc character
	if (file->ungetc != EOF)
	{
		((char*)buffer)[0] = file->ungetc;
		buffersize--;
		file->ungetc = EOF;
		done = 1;
	}
	else
		done = 0;

	if(file->flags & QFILE_FLAG_DATA)
	{
		size_t left = file->real_length - file->position;
		if(buffersize > left)
			buffersize = left;
		memcpy(buffer, file->data + file->position, buffersize);
		file->position += buffersize;
		return buffersize;
	}

	// First, we copy as many bytes as we can from "buff"
	if (file->buff_ind < file->buff_len)
	{
		count = file->buff_len - file->buff_ind;
		count = ((fs_offset_t)buffersize > count) ? count : (fs_offset_t)buffersize;
		done += count;
		memcpy (buffer, &file->buff[file->buff_ind], count);
		file->buff_ind += count;

		buffersize -= count;
		if (buffersize == 0)
			return done;
	}

	// NOTE: at this point, the read buffer is always empty

	// If the file isn't compressed
	if (! (file->flags & QFILE_FLAG_DEFLATED))
	{
		fs_offset_t nb;

		// We must take care to not read after the end of the file
		count = file->real_length - file->position;

		// If we have a lot of data to get, put them directly into "buffer"
		if (buffersize > sizeof (file->buff) / 2)
		{
			if (count > (fs_offset_t)buffersize)
				count = (fs_offset_t)buffersize;
			if (FILEDESC_SEEK (file->handle, file->offset + file->position, SEEK_SET) == -1)
			{
				// Seek failed. When reading from a pipe, and
				// the caller never called FS_Seek, this still
				// works fine.  So no reporting this error.
			}
			nb = FILEDESC_READ (file->handle, &((unsigned char*)buffer)[done], count);
			if (nb > 0)
			{
				done += nb;
				file->position += nb;

				// Purge cached data
				FS_Purge (file);
			}
		}
		else
		{
			if (count > (fs_offset_t)sizeof (file->buff))
				count = (fs_offset_t)sizeof (file->buff);
			if (FILEDESC_SEEK (file->handle, file->offset + file->position, SEEK_SET) == -1)
			{
				// Seek failed. When reading from a pipe, and
				// the caller never called FS_Seek, this still
				// works fine.  So no reporting this error.
			}
			nb = FILEDESC_READ (file->handle, file->buff, count);
			if (nb > 0)
			{
				file->buff_len = nb;
				file->position += nb;

				// Copy the requested data in "buffer" (as much as we can)
				count = (fs_offset_t)buffersize > file->buff_len ? file->buff_len : (fs_offset_t)buffersize;
				memcpy (&((unsigned char*)buffer)[done], file->buff, count);
				file->buff_ind = count;
				done += count;
			}
		}

		return done;
	}

	// If the file is compressed, it's more complicated...
	// We cycle through a few operations until we have read enough data
	while (buffersize > 0)
	{
		ztoolkit_t *ztk = file->ztk;
		int error;

		// NOTE: at this point, the read buffer is always empty

		// If "input" is also empty, we need to refill it
		if (ztk->in_ind == ztk->in_len)
		{
			// If we are at the end of the file
			if (file->position == file->real_length)
				return done;

			count = (fs_offset_t)(ztk->comp_length - ztk->in_position);
			if (count > (fs_offset_t)sizeof (ztk->input))
				count = (fs_offset_t)sizeof (ztk->input);
			FILEDESC_SEEK (file->handle, file->offset + (fs_offset_t)ztk->in_position, SEEK_SET);
			if (FILEDESC_READ (file->handle, ztk->input, count) != count)
			{
				Con_Printf ("FS_Read: unexpected end of file\n");
				break;
			}

			ztk->in_ind = 0;
			ztk->in_len = count;
			ztk->in_position += count;
		}

		ztk->zstream.next_in = &ztk->input[ztk->in_ind];
		ztk->zstream.avail_in = (unsigned int)(ztk->in_len - ztk->in_ind);

		// Now that we are sure we have compressed data available, we need to determine
		// if it's better to inflate it in "file->buff" or directly in "buffer"

		// Inflate the data in "file->buff"
		if (buffersize < sizeof (file->buff) / 2)
		{
			ztk->zstream.next_out = file->buff;
			ztk->zstream.avail_out = sizeof (file->buff);
			error = qz_inflate (&ztk->zstream, Z_SYNC_FLUSH);
			if (error != Z_OK && error != Z_STREAM_END)
			{
				Con_Printf ("FS_Read: Can't inflate file\n");
				break;
			}
			ztk->in_ind = ztk->in_len - ztk->zstream.avail_in;

			file->buff_len = (fs_offset_t)sizeof (file->buff) - ztk->zstream.avail_out;
			file->position += file->buff_len;

			// Copy the requested data in "buffer" (as much as we can)
			count = (fs_offset_t)buffersize > file->buff_len ? file->buff_len : (fs_offset_t)buffersize;
			memcpy (&((unsigned char*)buffer)[done], file->buff, count);
			file->buff_ind = count;
		}

		// Else, we inflate directly in "buffer"
		else
		{
			ztk->zstream.next_out = &((unsigned char*)buffer)[done];
			ztk->zstream.avail_out = (unsigned int)buffersize;
			error = qz_inflate (&ztk->zstream, Z_SYNC_FLUSH);
			if (error != Z_OK && error != Z_STREAM_END)
			{
				Con_Printf ("FS_Read: Can't inflate file\n");
				break;
			}
			ztk->in_ind = ztk->in_len - ztk->zstream.avail_in;

			// How much data did it inflate?
			count = (fs_offset_t)(buffersize - ztk->zstream.avail_out);
			file->position += count;

			// Purge cached data
			FS_Purge (file);
		}

		done += count;
		buffersize -= count;
	}

	return done;
}


/*
====================
FS_Print

Print a string into a file
====================
*/
int FS_Print (qfile_t* file, const char *msg)
{
	return (int)FS_Write (file, msg, strlen (msg));
}

/*
====================
FS_Printf

Print a string into a file
====================
*/
int FS_Printf(qfile_t* file, const char* format, ...)
{
	int result;
	va_list args;

	va_start (args, format);
	result = FS_VPrintf (file, format, args);
	va_end (args);

	return result;
}


/*
====================
FS_VPrintf

Print a string into a file
====================
*/
int FS_VPrintf (qfile_t* file, const char* format, va_list ap)
{
	int len;
	fs_offset_t buff_size = MAX_INPUTLINE;
	char *tempbuff;

	for (;;)
	{
		tempbuff = (char *)Mem_Alloc (tempmempool, buff_size);
		len = dpvsnprintf (tempbuff, buff_size, format, ap);
		if (len >= 0 && len < buff_size)
			break;
		Mem_Free (tempbuff);
		buff_size *= 2;
	}

	len = FILEDESC_WRITE (file->handle, tempbuff, len);
	Mem_Free (tempbuff);

	return len;
}


/*
====================
FS_Getc

Get the next character of a file
====================
*/
int FS_Getc (qfile_t* file)
{
	unsigned char c;

	if (FS_Read (file, &c, 1) != 1)
		return EOF;

	return c;
}


/*
====================
FS_UnGetc

Put a character back into the read buffer (only supports one character!)
====================
*/
int FS_UnGetc (qfile_t* file, unsigned char c)
{
	// If there's already a character waiting to be read
	if (file->ungetc != EOF)
		return EOF;

	file->ungetc = c;
	return c;
}


/*
====================
FS_Seek

Move the position index in a file
====================
*/
int FS_Seek (qfile_t* file, fs_offset_t offset, int whence)
{
	ztoolkit_t *ztk;
	unsigned char* buffer;
	fs_offset_t buffersize;

	// Compute the file offset
	switch (whence)
	{
		case SEEK_CUR:
			offset += file->position - file->buff_len + file->buff_ind;
			break;

		case SEEK_SET:
			break;

		case SEEK_END:
			offset += file->real_length;
			break;

		default:
			return -1;
	}
	if (offset < 0 || offset > file->real_length)
		return -1;

	if(file->flags & QFILE_FLAG_DATA)
	{
		file->position = offset;
		return 0;
	}

	// If we have the data in our read buffer, we don't need to actually seek
	if (file->position - file->buff_len <= offset && offset <= file->position)
	{
		file->buff_ind = offset + file->buff_len - file->position;
		return 0;
	}

	// Purge cached data
	FS_Purge (file);

	// Unpacked or uncompressed files can seek directly
	if (! (file->flags & QFILE_FLAG_DEFLATED))
	{
		if (FILEDESC_SEEK (file->handle, file->offset + offset, SEEK_SET) == -1)
			return -1;
		file->position = offset;
		return 0;
	}

	// Seeking in compressed files is more a hack than anything else,
	// but we need to support it, so here we go.
	ztk = file->ztk;

	// If we have to go back in the file, we need to restart from the beginning
	if (offset <= file->position)
	{
		ztk->in_ind = 0;
		ztk->in_len = 0;
		ztk->in_position = 0;
		file->position = 0;
		if (FILEDESC_SEEK (file->handle, file->offset, SEEK_SET) == -1)
			Con_Printf("IMPOSSIBLE: couldn't seek in already opened pk3 file.\n");

		// Reset the Zlib stream
		ztk->zstream.next_in = ztk->input;
		ztk->zstream.avail_in = 0;
		qz_inflateReset (&ztk->zstream);
	}

	// We need a big buffer to force inflating into it directly
	buffersize = 2 * sizeof (file->buff);
	buffer = (unsigned char *)Mem_Alloc (tempmempool, buffersize);

	// Skip all data until we reach the requested offset
	while (offset > (file->position - file->buff_len + file->buff_ind))
	{
		fs_offset_t diff = offset - (file->position - file->buff_len + file->buff_ind);
		fs_offset_t count, len;

		count = (diff > buffersize) ? buffersize : diff;
		len = FS_Read (file, buffer, count);
		if (len != count)
		{
			Mem_Free (buffer);
			return -1;
		}
	}

	Mem_Free (buffer);
	return 0;
}


/*
====================
FS_Tell

Give the current position in a file
====================
*/
fs_offset_t FS_Tell (qfile_t* file)
{
	return file->position - file->buff_len + file->buff_ind;
}


/*
====================
FS_FileSize

Give the total size of a file
====================
*/
fs_offset_t FS_FileSize (qfile_t* file)
{
	return file->real_length;
}


/*
====================
FS_Purge

Erases any buffered input or output data
====================
*/
void FS_Purge (qfile_t* file)
{
	file->buff_len = 0;
	file->buff_ind = 0;
	file->ungetc = EOF;
}


/*
============
FS_LoadAndCloseQFile

Loads full content of a qfile_t and closes it.
Always appends a 0 byte.
============
*/
static unsigned char *FS_LoadAndCloseQFile (qfile_t *file, const char *path, mempool_t *pool, qbool quiet, fs_offset_t *filesizepointer)
{
	unsigned char *buf = NULL;
	fs_offset_t filesize = 0;

	if (file)
	{
		filesize = file->real_length;
		if(filesize < 0)
		{
			Con_Printf("FS_LoadFile(\"%s\", pool, %s, filesizepointer): trying to open a non-regular file\n", path, quiet ? "true" : "false");
			FS_Close(file);
			return NULL;
		}

		buf = (unsigned char *)Mem_Alloc (pool, filesize + 1);
		buf[filesize] = '\0';
		FS_Read (file, buf, filesize);
		FS_Close (file);
		if (developer_loadfile.integer)
			Con_Printf("loaded file \"%s\" (%u bytes)\n", path, (unsigned int)filesize);
	}

	if (filesizepointer)
		*filesizepointer = filesize;
	return buf;
}


/*
============
FS_LoadFile

Filename are relative to the quake directory.
Always appends a 0 byte.
============
*/
unsigned char *FS_LoadFile (const char *path, mempool_t *pool, qbool quiet, fs_offset_t *filesizepointer)
{
	qfile_t *file = FS_OpenVirtualFile(path, quiet);
	return FS_LoadAndCloseQFile(file, path, pool, quiet, filesizepointer);
}


/*
============
FS_SysLoadFile

Filename are OS paths.
Always appends a 0 byte.
============
*/
unsigned char *FS_SysLoadFile (const char *path, mempool_t *pool, qbool quiet, fs_offset_t *filesizepointer)
{
	qfile_t *file = FS_SysOpen(path, "rb", false);
	return FS_LoadAndCloseQFile(file, path, pool, quiet, filesizepointer);
}


/*
============
FS_WriteFile

The filename will be prefixed by the current game directory
============
*/
qbool FS_WriteFileInBlocks (const char *filename, const void *const *data, const fs_offset_t *len, size_t count)
{
	qfile_t *file;
	size_t i;
	fs_offset_t lentotal;

	file = FS_OpenRealFile(filename, "wb", false);
	if (!file)
	{
		Con_Printf("FS_WriteFile: failed on %s\n", filename);
		return false;
	}

	lentotal = 0;
	for(i = 0; i < count; ++i)
		lentotal += len[i];
	Con_DPrintf("FS_WriteFile: %s (%u bytes)\n", filename, (unsigned int)lentotal);
	for(i = 0; i < count; ++i)
		FS_Write (file, data[i], len[i]);
	FS_Close (file);
	return true;
}

qbool FS_WriteFile (const char *filename, const void *data, fs_offset_t len)
{
	return FS_WriteFileInBlocks(filename, &data, &len, 1);
}


/*
=============================================================================

OTHERS PUBLIC FUNCTIONS

=============================================================================
*/

/*
============
FS_StripExtension
============
*/
void FS_StripExtension (const char *in, char *out, size_t size_out)
{
	char *last = NULL;
	char currentchar;

	if (size_out == 0)
		return;

	while ((currentchar = *in) && size_out > 1)
	{
		if (currentchar == '.')
			last = out;
		else if (currentchar == '/' || currentchar == '\\' || currentchar == ':')
			last = NULL;
		*out++ = currentchar;
		in++;
		size_out--;
	}
	if (last)
		*last = 0;
	else
		*out = 0;
}


/*
==================
FS_DefaultExtension
==================
*/
void FS_DefaultExtension (char *path, const char *extension, size_t size_path)
{
	const char *src;

	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	src = path + strlen(path);

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	dp_strlcat (path, extension, size_path);
}


/*
==================
FS_FileType

Look for a file in the packages and in the filesystem
==================
*/
int FS_FileType (const char *filename)
{
	searchpath_t *search;
	char fullpath[MAX_OSPATH];

	search = FS_FindFile (filename, NULL, NULL, true);
	if(!search)
		return FS_FILETYPE_NONE;

	if(search->pack && !search->pack->vpack)
		return FS_FILETYPE_FILE; // TODO can't check directories in paks yet, maybe later

	dpsnprintf(fullpath, sizeof(fullpath), "%s%s", search->filename, filename);
	return FS_SysFileType(fullpath);
}


/*
==================
FS_FileExists

Look for a file in the packages and in the filesystem
Returns its canonical name (VFS path with correct capitalisation) if found, else NULL.
If the file is found outside a pak, this will be the same pointer as passed in.
==================
*/
const char *FS_FileExists (const char *filename)
{
	const char *canonicalname;

	return FS_FindFile(filename, NULL, &canonicalname, true) ? canonicalname : NULL;
}


/*
==================
FS_SysFileExists

Look for a file in the filesystem only
==================
*/
int FS_SysFileType (const char *path)
{
#if WIN32
// Sajt - some older sdks are missing this define
# ifndef INVALID_FILE_ATTRIBUTES
#  define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
# endif
	wchar pathw[WSTRBUF] = {0};
	DWORD result;
	widen(path, pathw);
	result = GetFileAttributesW(pathw);

	if(result == INVALID_FILE_ATTRIBUTES)
		return FS_FILETYPE_NONE;

	if(result & FILE_ATTRIBUTE_DIRECTORY)
		return FS_FILETYPE_DIRECTORY;

	return FS_FILETYPE_FILE;
#else
	struct stat buf;

	if (stat (path,&buf) == -1)
		return FS_FILETYPE_NONE;

#ifndef S_ISDIR
#define S_ISDIR(a) (((a) & S_IFMT) == S_IFDIR)
#endif
	if(S_ISDIR(buf.st_mode))
		return FS_FILETYPE_DIRECTORY;

	return FS_FILETYPE_FILE;
#endif
}

qbool FS_SysFileExists (const char *path)
{
	return FS_SysFileType (path) != FS_FILETYPE_NONE;
}

/*
===========
FS_Search

Allocate and fill a search structure with information on matching filenames.
===========
*/
fssearch_t *FS_Search(const char *pattern, int caseinsensitive, int quiet, const char *packfile)
{
	fssearch_t *search;
	searchpath_t *searchpath;
	pack_t *pak;
	int i, basepathlength, numfiles, numchars, resultlistindex, dirlistindex;
	stringlist_t resultlist;
	stringlist_t dirlist;
	stringlist_t matchedSet, foundSet;
	const char *start, *slash, *backslash, *colon, *separator;
	char *basepath;

	for (i = 0;pattern[i] == '.' || pattern[i] == ':' || pattern[i] == '/' || pattern[i] == '\\';i++)
		;

	if (i > 0)
	{
		Con_Printf("Don't use punctuation at the beginning of a search pattern!\n");
		return NULL;
	}

	stringlistinit(&resultlist);
	stringlistinit(&dirlist);
	search = NULL;
	slash = strrchr(pattern, '/');
	backslash = strrchr(pattern, '\\');
	colon = strrchr(pattern, ':');
	separator = max(slash, backslash);
	separator = max(separator, colon);
	basepathlength = separator ? (separator + 1 - pattern) : 0;
	basepath = (char *)Mem_Alloc (tempmempool, basepathlength + 1);
	if (basepathlength)
		memcpy(basepath, pattern, basepathlength);
	basepath[basepathlength] = 0;

	// search through the path, one element at a time
	for (searchpath = fs_searchpaths;searchpath;searchpath = searchpath->next)
	{
		// is the element a pak file?
		if (searchpath->pack && !searchpath->pack->vpack)
		{
			// look through all the pak file elements
			pak = searchpath->pack;
			if(packfile)
			{
				if(strcmp(packfile, pak->shortname))
					continue;
			}
			for (i = 0;i < pak->numfiles;i++)
			{
				char temp[MAX_OSPATH];
				dp_strlcpy(temp, pak->files[i].name, sizeof(temp));
				while (temp[0])
				{
					if (matchpattern(temp, (char *)pattern, true))
					{
						for (resultlistindex = 0;resultlistindex < resultlist.numstrings;resultlistindex++)
							if (!strcmp(resultlist.strings[resultlistindex], temp))
								break;
						if (resultlistindex == resultlist.numstrings)
						{
							stringlistappend(&resultlist, temp);
							if (!quiet && developer_loading.integer)
								Con_Printf("SearchPackFile: %s : %s\n", pak->filename, temp);
						}
					}
					// strip off one path element at a time until empty
					// this way directories are added to the listing if they match the pattern
					slash = strrchr(temp, '/');
					backslash = strrchr(temp, '\\');
					colon = strrchr(temp, ':');
					separator = temp;
					if (separator < slash)
						separator = slash;
					if (separator < backslash)
						separator = backslash;
					if (separator < colon)
						separator = colon;
					*((char *)separator) = 0;
				}
			}
		}
		else
		{
			if(packfile)
				continue;

			start = pattern;

			stringlistinit(&matchedSet);
			stringlistinit(&foundSet);
			// add a first entry to the set
			stringlistappend(&matchedSet, "");
			// iterate through pattern's path
			while (*start)
			{
				const char *asterisk, *wildcard, *nextseparator, *prevseparator;
				char subpath[MAX_OSPATH];
				char subpattern[MAX_OSPATH];

				// find the next wildcard
				wildcard = strchr(start, '?');
				asterisk = strchr(start, '*');
				if (asterisk && (!wildcard || asterisk < wildcard))
				{
					wildcard = asterisk;
				}

				if (wildcard)
				{
					nextseparator = strchr( wildcard, '/' );
				}
				else
				{
					nextseparator = NULL;
				}

				if( !nextseparator ) {
					nextseparator = start + strlen( start );
				}

				// prevseparator points past the '/' right before the wildcard and nextseparator at the one following it (or at the end of the string)
				// copy everything up except nextseperator
				dp_ustr2stp(subpattern, sizeof(subpattern), pattern, nextseparator - pattern);
				// find the last '/' before the wildcard
				prevseparator = strrchr( subpattern, '/' );
				if (!prevseparator)
					prevseparator = subpattern;
				else
					prevseparator++;
				// copy everything from start to the previous including the '/' (before the wildcard)
				// everything up to start is already included in the path of matchedSet's entries
				dp_ustr2stp(subpath, sizeof(subpath), start, (prevseparator - subpattern) - (start - pattern));

				// for each entry in matchedSet try to open the subdirectories specified in subpath
				for( dirlistindex = 0 ; dirlistindex < matchedSet.numstrings ; dirlistindex++ ) {
					char temp[MAX_OSPATH];
					dp_strlcpy( temp, matchedSet.strings[ dirlistindex ], sizeof(temp) );
					dp_strlcat( temp, subpath, sizeof(temp) );
					listdirectory( &foundSet, searchpath->filename, temp );
				}
				if( dirlistindex == 0 ) {
					break;
				}
				// reset the current result set
				stringlistfreecontents( &matchedSet );
				// match against the pattern
				for( dirlistindex = 0 ; dirlistindex < foundSet.numstrings ; dirlistindex++ ) {
					const char *direntry = foundSet.strings[ dirlistindex ];
					if (matchpattern(direntry, subpattern, true)) {
						stringlistappend( &matchedSet, direntry );
					}
				}
				stringlistfreecontents( &foundSet );

				start = nextseparator;
			}

			for (dirlistindex = 0;dirlistindex < matchedSet.numstrings;dirlistindex++)
			{
				const char *matchtemp = matchedSet.strings[dirlistindex];
				if (matchpattern(matchtemp, (char *)pattern, true))
				{
					for (resultlistindex = 0;resultlistindex < resultlist.numstrings;resultlistindex++)
						if (!strcmp(resultlist.strings[resultlistindex], matchtemp))
							break;
					if (resultlistindex == resultlist.numstrings)
					{
						stringlistappend(&resultlist, matchtemp);
						if (!quiet && developer_loading.integer)
							Con_Printf("SearchDirFile: %s\n", matchtemp);
					}
				}
			}
			stringlistfreecontents( &matchedSet );
		}
	}

	if (resultlist.numstrings)
	{
		stringlistsort(&resultlist, true);
		numfiles = resultlist.numstrings;
		numchars = 0;
		for (resultlistindex = 0;resultlistindex < resultlist.numstrings;resultlistindex++)
			numchars += (int)strlen(resultlist.strings[resultlistindex]) + 1;
		search = (fssearch_t *)Z_Malloc(sizeof(fssearch_t) + numchars + numfiles * sizeof(char *));
		search->filenames = (char **)((char *)search + sizeof(fssearch_t));
		search->filenamesbuffer = (char *)((char *)search + sizeof(fssearch_t) + numfiles * sizeof(char *));
		search->numfilenames = (int)numfiles;
		numfiles = 0;
		numchars = 0;
		for (resultlistindex = 0;resultlistindex < resultlist.numstrings;resultlistindex++)
		{
			size_t textlen;
			search->filenames[numfiles] = search->filenamesbuffer + numchars;
			textlen = strlen(resultlist.strings[resultlistindex]) + 1;
			memcpy(search->filenames[numfiles], resultlist.strings[resultlistindex], textlen);
			numfiles++;
			numchars += (int)textlen;
		}
	}
	stringlistfreecontents(&resultlist);

	Mem_Free(basepath);
	return search;
}

void FS_FreeSearch(fssearch_t *search)
{
	Z_Free(search);
}

extern int con_linewidth;
static int FS_ListDirectory(const char *pattern, int oneperline)
{
	int numfiles;
	int numcolumns;
	int numlines;
	int columnwidth;
	int linebufpos;
	int i, j, k, l;
	const char *name;
	char linebuf[MAX_INPUTLINE];
	fssearch_t *search;
	search = FS_Search(pattern, true, true, NULL);
	if (!search)
		return 0;
	numfiles = search->numfilenames;
	if (!oneperline)
	{
		// FIXME: the names could be added to one column list and then
		// gradually shifted into the next column if they fit, and then the
		// next to make a compact variable width listing but it's a lot more
		// complicated...
		// find width for columns
		columnwidth = 0;
		for (i = 0;i < numfiles;i++)
		{
			l = (int)strlen(search->filenames[i]);
			if (columnwidth < l)
				columnwidth = l;
		}
		// count the spacing character
		columnwidth++;
		// calculate number of columns
		numcolumns = con_linewidth / columnwidth;
		// don't bother with the column printing if it's only one column
		if (numcolumns >= 2)
		{
			numlines = (numfiles + numcolumns - 1) / numcolumns;
			for (i = 0;i < numlines;i++)
			{
				linebufpos = 0;
				for (k = 0;k < numcolumns;k++)
				{
					l = i * numcolumns + k;
					if (l < numfiles)
					{
						name = search->filenames[l];
						for (j = 0;name[j] && linebufpos + 1 < (int)sizeof(linebuf);j++)
							linebuf[linebufpos++] = name[j];
						// space out name unless it's the last on the line
						if (k + 1 < numcolumns && l + 1 < numfiles)
							for (;j < columnwidth && linebufpos + 1 < (int)sizeof(linebuf);j++)
								linebuf[linebufpos++] = ' ';
					}
				}
				linebuf[linebufpos] = 0;
				Con_Printf("%s\n", linebuf);
			}
		}
		else
			oneperline = true;
	}
	if (oneperline)
		for (i = 0;i < numfiles;i++)
			Con_Printf("%s\n", search->filenames[i]);
	FS_FreeSearch(search);
	return (int)numfiles;
}

static void FS_ListDirectoryCmd (cmd_state_t *cmd, const char* cmdname, int oneperline)
{
	const char *pattern;
	if (Cmd_Argc(cmd) >= 3)
	{
		Con_Printf("usage:\n%s [path/pattern]\n", cmdname);
		return;
	}
	if (Cmd_Argc(cmd) == 2)
		pattern = Cmd_Argv(cmd, 1);
	else
		pattern = "*";
	if (!FS_ListDirectory(pattern, oneperline))
		Con_Print("No files found.\n");
}

void FS_Dir_f(cmd_state_t *cmd)
{
	FS_ListDirectoryCmd(cmd, "dir", true);
}

void FS_Ls_f(cmd_state_t *cmd)
{
	FS_ListDirectoryCmd(cmd, "ls", false);
}

void FS_Which_f(cmd_state_t *cmd)
{
	const char *filename;
	int index;
	searchpath_t *sp;
	if (Cmd_Argc(cmd) != 2)
	{
		Con_Printf("usage:\n%s <file>\n", Cmd_Argv(cmd, 0));
		return;
	}
	filename = Cmd_Argv(cmd, 1);
	sp = FS_FindFile(filename, &index, NULL, true);
	if (!sp) {
		Con_Printf("%s isn't anywhere\n", filename);
		return;
	}
	if (sp->pack)
	{
		if(sp->pack->vpack)
			Con_Printf("%s is in virtual package %sdir\n", filename, sp->pack->shortname);
		else
			Con_Printf("%s is in package %s\n", filename, sp->pack->shortname);
	}
	else
		Con_Printf("%s is file %s%s\n", filename, sp->filename, filename);
}


const char *FS_WhichPack(const char *filename)
{
	int index;
	searchpath_t *sp = FS_FindFile(filename, &index, NULL, true);
	if(sp && sp->pack)
		return sp->pack->shortname;
	else if(sp)
		return "";
	else
		return 0;
}

/*
====================
FS_IsRegisteredQuakePack

Look for a proof of purchase file file in the requested package

If it is found, this file should NOT be downloaded.
====================
*/
qbool FS_IsRegisteredQuakePack(const char *name)
{
	searchpath_t *search;
	pack_t *pak;

	// search through the path, one element at a time
	for (search = fs_searchpaths;search;search = search->next)
	{
		if (search->pack && !search->pack->vpack && !strcasecmp(FS_FileWithoutPath(search->filename), name))
			// TODO do we want to support vpacks in here too?
		{
			int (*strcmp_funct) (const char* str1, const char* str2);
			int left, right, middle;

			pak = search->pack;
			strcmp_funct = pak->ignorecase ? strcasecmp : strcmp;

			// Look for the file (binary search)
			left = 0;
			right = pak->numfiles - 1;
			while (left <= right)
			{
				int diff;

				middle = (left + right) / 2;
				diff = strcmp_funct (pak->files[middle].name, "gfx/pop.lmp");

				// Found it
				if (!diff)
					return true;

				// If we're too far in the list
				if (diff > 0)
					right = middle - 1;
				else
					left = middle + 1;
			}

			// we found the requested pack but it is not registered quake
			return false;
		}
	}

	return false;
}

int FS_CRCFile(const char *filename, size_t *filesizepointer)
{
	int crc = -1;
	unsigned char *filedata;
	fs_offset_t filesize;
	if (filesizepointer)
		*filesizepointer = 0;
	if (!filename || !*filename)
		return crc;
	filedata = FS_LoadFile(filename, tempmempool, true, &filesize);
	if (filedata)
	{
		if (filesizepointer)
			*filesizepointer = filesize;
		crc = CRC_Block(filedata, filesize);
		Mem_Free(filedata);
	}
	return crc;
}

unsigned char *FS_Deflate(const unsigned char *data, size_t size, size_t *deflated_size, int level, mempool_t *mempool)
{
	z_stream strm;
	unsigned char *out = NULL;
	unsigned char *tmp;

	*deflated_size = 0;
#ifndef LINK_TO_ZLIB
	if(!zlib_dll)
		return NULL;
#endif

	memset(&strm, 0, sizeof(strm));
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	if(level < 0)
		level = Z_DEFAULT_COMPRESSION;

	if(qz_deflateInit2(&strm, level, Z_DEFLATED, -MAX_WBITS, Z_MEMLEVEL_DEFAULT, Z_BINARY) != Z_OK)
	{
		Con_Printf("FS_Deflate: deflate init error!\n");
		return NULL;
	}

	strm.next_in = (unsigned char*)data;
	strm.avail_in = (unsigned int)size;

	tmp = (unsigned char *) Mem_Alloc(tempmempool, size);
	if(!tmp)
	{
		Con_Printf("FS_Deflate: not enough memory in tempmempool!\n");
		qz_deflateEnd(&strm);
		return NULL;
	}

	strm.next_out = tmp;
	strm.avail_out = (unsigned int)size;

	if(qz_deflate(&strm, Z_FINISH) != Z_STREAM_END)
	{
		Con_Printf("FS_Deflate: deflate failed!\n");
		qz_deflateEnd(&strm);
		Mem_Free(tmp);
		return NULL;
	}

	if(qz_deflateEnd(&strm) != Z_OK)
	{
		Con_Printf("FS_Deflate: deflateEnd failed\n");
		Mem_Free(tmp);
		return NULL;
	}

	if(strm.total_out >= size)
	{
		Con_Printf("FS_Deflate: deflate is useless on this data!\n");
		Mem_Free(tmp);
		return NULL;
	}

	out = (unsigned char *) Mem_Alloc(mempool, strm.total_out);
	if(!out)
	{
		Con_Printf("FS_Deflate: not enough memory in target mempool!\n");
		Mem_Free(tmp);
		return NULL;
	}

	*deflated_size = (size_t)strm.total_out;

	memcpy(out, tmp, strm.total_out);
	Mem_Free(tmp);

	return out;
}

static void AssertBufsize(sizebuf_t *buf, int length)
{
	if(buf->cursize + length > buf->maxsize)
	{
		int oldsize = buf->maxsize;
		unsigned char *olddata;
		olddata = buf->data;
		buf->maxsize += length;
		buf->data = (unsigned char *) Mem_Alloc(tempmempool, buf->maxsize);
		if(olddata)
		{
			memcpy(buf->data, olddata, oldsize);
			Mem_Free(olddata);
		}
	}
}

unsigned char *FS_Inflate(const unsigned char *data, size_t size, size_t *inflated_size, mempool_t *mempool)
{
	int ret;
	z_stream strm;
	unsigned char *out = NULL;
	unsigned char tmp[2048];
	unsigned int have;
	sizebuf_t outbuf;

	*inflated_size = 0;
#ifndef LINK_TO_ZLIB
	if(!zlib_dll)
		return NULL;
#endif

	memset(&outbuf, 0, sizeof(outbuf));
	outbuf.data = (unsigned char *) Mem_Alloc(tempmempool, sizeof(tmp));
	outbuf.maxsize = sizeof(tmp);

	memset(&strm, 0, sizeof(strm));
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	if(qz_inflateInit2(&strm, -MAX_WBITS) != Z_OK)
	{
		Con_Printf("FS_Inflate: inflate init error!\n");
		Mem_Free(outbuf.data);
		return NULL;
	}

	strm.next_in = (unsigned char*)data;
	strm.avail_in = (unsigned int)size;

	do
	{
		strm.next_out = tmp;
		strm.avail_out = sizeof(tmp);
		ret = qz_inflate(&strm, Z_NO_FLUSH);
		// it either returns Z_OK on progress, Z_STREAM_END on end
		// or an error code
		switch(ret)
		{
			case Z_STREAM_END:
			case Z_OK:
				break;

			case Z_STREAM_ERROR:
				Con_Print("FS_Inflate: stream error!\n");
				break;
			case Z_DATA_ERROR:
				Con_Print("FS_Inflate: data error!\n");
				break;
			case Z_MEM_ERROR:
				Con_Print("FS_Inflate: mem error!\n");
				break;
			case Z_BUF_ERROR:
				Con_Print("FS_Inflate: buf error!\n");
				break;
			default:
				Con_Print("FS_Inflate: unknown error!\n");
				break;

		}
		if(ret != Z_OK && ret != Z_STREAM_END)
		{
			Con_Printf("Error after inflating %u bytes\n", (unsigned)strm.total_in);
			Mem_Free(outbuf.data);
			qz_inflateEnd(&strm);
			return NULL;
		}
		have = sizeof(tmp) - strm.avail_out;
		AssertBufsize(&outbuf, max(have, sizeof(tmp)));
		SZ_Write(&outbuf, tmp, have);
	} while(ret != Z_STREAM_END);

	qz_inflateEnd(&strm);

	out = (unsigned char *) Mem_Alloc(mempool, outbuf.cursize);
	if(!out)
	{
		Con_Printf("FS_Inflate: not enough memory in target mempool!\n");
		Mem_Free(outbuf.data);
		return NULL;
	}

	memcpy(out, outbuf.data, outbuf.cursize);
	Mem_Free(outbuf.data);

	*inflated_size = (size_t)outbuf.cursize;

	return out;
}
