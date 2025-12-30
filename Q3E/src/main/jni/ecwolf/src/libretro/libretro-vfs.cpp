/*
** libretro-vfs.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
** Copyright 2020 Google
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "wl_def.h"
#include "c_cvars.h"
#include "streams/file_stream.h" // Must be before id_sd.h
#include "id_sd.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "config.h"
#include "wl_play.h"
#include "wl_net.h"
#include "libretro.h"
#include "state_machine.h"
#include "wl_def.h"
#include "wl_menu.h"
#include "id_ca.h"
#include "id_sd.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "wl_atmos.h"
#include "m_classes.h"
#include "m_random.h"
#include "config.h"
#include "w_wad.h"
#include "language.h"
#include "textures/textures.h"
#include "c_cvars.h"
#include "thingdef/thingdef.h"
#include "v_font.h"
#include "v_palette.h"
#include "v_video.h"
#include "r_data/colormaps.h"
#include "wl_agent.h"
#include "doomerrors.h"
#include "lumpremap.h"
#include "scanner.h"
#include "g_shared/a_keys.h"
#include "g_mapinfo.h"
#include "wl_draw.h"
#include "wl_inter.h"
#include "wl_iwad.h"
#include "wl_play.h"
#include "wl_game.h"
#include "wl_loadsave.h"
#include "wl_net.h"
#include "dobject.h"
#include "colormatcher.h"
#include "version.h"
#include "r_2d/r_main.h"
#include "filesys.h"
#include "g_conversation.h"
#include "g_intermission.h"
#include "am_map.h"
#include "wl_loadsave.h"
#include "retro_dirent.h"
#include "vfs/vfs_implementation.h"

struct retro_vfs_interface *vfs_interface;

static const char * wrap_retro_vfs_readdir(RDIR *dirstream)
{
	if (!retro_readdir(dirstream))
		return NULL;
	return retro_dirent_get_name (dirstream);
}

static int wrap_retro_vfs_stat(const char *path, int32_t *size) {
	if (vfs_interface)
		return vfs_interface->stat(path, size);
	else
		return retro_vfs_stat_impl(path, size);
}

struct FileMemoryStore
{
	void *contents;
	long length;
	FString filename;
};

static TArray<FileMemoryStore> MemoryFiles;

struct retro_vfs_wrapped_file_handle
{
	enum { UNKNOWN, LIBRETRO_VFS, MEM } type;
	RFILE *lr;
	struct FileMemoryStore *memstore;
	long mem_pos;
};

bool store_files_in_memory;

static struct retro_vfs_wrapped_file_handle *
retro_vfs_open_ro(const char *filename) {
	struct retro_vfs_wrapped_file_handle *ret = (struct retro_vfs_wrapped_file_handle *) malloc (sizeof (*ret));
	CHECKMALLOCRESULT(ret);
	memset(ret, 0, sizeof(*ret));
	for (size_t i = 0; i < MemoryFiles.Size(); i++) {
		if (MemoryFiles[i].filename.CompareNoCase(filename) == 0) {
			ret->type = retro_vfs_wrapped_file_handle::MEM;
			ret->memstore = &MemoryFiles[i];
			printf("Reusing loaded file %s\n", filename);
			return ret;
		}
	}

	ret->lr = filestream_open(filename, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
	if (!ret->lr) {
		free(ret);
		return NULL;
	}
	ret->type = retro_vfs_wrapped_file_handle::LIBRETRO_VFS;
	if (store_files_in_memory) {
		struct FileMemoryStore *store = new FileMemoryStore();
		store->length = filestream_get_size(ret->lr);
		store->contents = malloc (store->length);
		store->filename = filename;
		if (store->contents) {
			filestream_read(ret->lr, store->contents, store->length);
			filestream_close(ret->lr);
			ret->type = retro_vfs_wrapped_file_handle::MEM;
			ret->memstore = store;
			ret->lr = NULL;
			MemoryFiles.Push(*store);
		}
	}
	return ret;
}

static int64_t retro_vfs_size(struct retro_vfs_wrapped_file_handle *stream)
{
	switch (stream->type) {
	case retro_vfs_wrapped_file_handle::LIBRETRO_VFS:
		return filestream_get_size(stream->lr);
	case retro_vfs_wrapped_file_handle::MEM:
		return stream->memstore->length;
	default:
	  	return -1;
	}
}

static int64_t retro_vfs_tell(struct retro_vfs_wrapped_file_handle *stream)
{
	switch (stream->type) {
	case retro_vfs_wrapped_file_handle::LIBRETRO_VFS:
		return filestream_tell(stream->lr);
	case retro_vfs_wrapped_file_handle::MEM:
		return stream->mem_pos;
	default:
	  	assert(0);
		return -1;
	}
}

static void retro_vfs_close(struct retro_vfs_wrapped_file_handle *stream)
{
	switch (stream->type) {
	case retro_vfs_wrapped_file_handle::LIBRETRO_VFS:
		filestream_close(stream->lr);
		break;
	case retro_vfs_wrapped_file_handle::MEM:
		break;
	default:
		assert(0);
	}

	free(stream);
}

static bool retro_vfs_seek(struct retro_vfs_wrapped_file_handle *stream, int64_t pos)
{
	switch (stream->type) {
	case retro_vfs_wrapped_file_handle::LIBRETRO_VFS:
		return filestream_seek(stream->lr, pos, RETRO_VFS_SEEK_POSITION_START) >= 0;
	case retro_vfs_wrapped_file_handle::MEM:
		if (pos < 0 || pos > stream->memstore->length)
			return false;
		stream->mem_pos = pos;
		return true;
	default:
		assert(0);
	}
	return false;
}

static int64_t retro_vfs_read(struct retro_vfs_wrapped_file_handle *stream, void *s, uint64_t len)
{
	int64_t ret = -1;
	switch (stream->type) {
	case retro_vfs_wrapped_file_handle::LIBRETRO_VFS:
		ret = filestream_read(stream->lr, s, len);
		break;
	case retro_vfs_wrapped_file_handle::MEM:
		ret = len;
		if (ret > stream->memstore->length - stream->mem_pos)
			ret = stream->memstore->length - stream->mem_pos;
		if (ret < 0)
			ret = 0;
		memcpy(s, (char *) stream->memstore->contents + stream->mem_pos, ret);
		stream->mem_pos += ret;
		break;
	default:
		assert(0);
	}		
	return ret;
}

int FDirectory::AddDirectory(const char *dirpath)
{
	int count = 0;
	TArray<FString> scanDirectories;
	scanDirectories.Push(dirpath);
	for(unsigned int i = 0;i < scanDirectories.Size();i++)
	{
		RDIR* directory = retro_opendir(scanDirectories[i].GetChars());
		if (directory == NULL)
		{
			Printf("Could not read directory\n");
			return 0;
		}

		const char *file;
		while((file = wrap_retro_vfs_readdir(directory)) != NULL)
		{
			if(file[0] == '.') //File is hidden or ./.. directory so ignore it.
				continue;

			FString fullFileName = scanDirectories[i] + file;

			int32_t size = -1;
			int statflags = wrap_retro_vfs_stat(fullFileName.GetChars(), &size);

			if(statflags & RETRO_VFS_STAT_IS_DIRECTORY)
			{
				scanDirectories.Push(scanDirectories[i] + file + "/");
				continue;
			}
			AddEntry(scanDirectories[i] + file, size);
			count++;
		}
		retro_closedir(directory);
	}
	return count;
}


FileReader::FileReader ()
: File(NULL), Length(0), StartPos(0), FilePos(0), CloseOnDestruct(false)
{
}

FileReader::FileReader (const FileReader &other, long length)
: File(other.File), Length(length), CloseOnDestruct(false)
{
	FilePos = StartPos = retro_vfs_tell (other.File);
}

FileReader::FileReader (const char *filename)
: File(NULL), Length(0), StartPos(0), FilePos(0), CloseOnDestruct(false)
{
	if (!Open(filename))
	{
		I_Error ("Could not open %s", filename);
	}
}

FileReader *FileReader::SafeOpen(const char *filename)
{
	FileReader *ret = new FileReader();
	if (!ret->Open(filename))
		return NULL;
	return ret;
}

FileReader::FileReader (struct retro_vfs_wrapped_file_handle *file)
: File(file), Length(0), StartPos(0), FilePos(0), CloseOnDestruct(false)
{
	Length = retro_vfs_size(file);
}

FileReader::FileReader (struct retro_vfs_wrapped_file_handle *file, long length)
: File(file), Length(length), CloseOnDestruct(true)
{
	FilePos = StartPos = retro_vfs_tell (file);
}

FileReader::~FileReader ()
{
	if (CloseOnDestruct && File != NULL)
	{
		retro_vfs_close (File);
		File = NULL;
	}
}

bool FileReader::Open (const char *filename)
{
	File = retro_vfs_open_ro(filename);
	if (File == NULL) return false;
	FilePos = 0;
	StartPos = 0;
	CloseOnDestruct = true;
	Length = retro_vfs_size(File);
	return true;
}


void FileReader::ResetFilePtr ()
{
	FilePos = retro_vfs_tell (File);
}

long FileReader::Tell () const
{
	return FilePos - StartPos;
}

long FileReader::Seek (long offset, int origin)
{
	if (origin == SEEK_SET)
	{
		offset += StartPos;
	}
	else if (origin == SEEK_CUR)
	{
		offset += FilePos;
	}
	else if (origin == SEEK_END)
	{
		offset += StartPos + Length;
	}
	if (retro_vfs_seek (File, offset))
	{
		FilePos = offset;
		return 0;
	}
	return -1;
}

long FileReader::Read (void *buffer, long len)
{
	assert(len >= 0);
	if (len <= 0) return 0;
	if (FilePos + len > StartPos + Length)
	{
		len = Length - FilePos + StartPos;
	}
	len = (long)retro_vfs_read (File, buffer, len);
	FilePos += len;
	return len;
}

char *FileReader::Gets(char *strbuf, int len)
{
	if (len <= 0 || FilePos >= StartPos + Length) return NULL;
	long pos = Tell();
	retro_vfs_read (File, strbuf, len);
	char *p = strbuf;
	for (p = strbuf; p < strbuf + len - 1 && p - strbuf < Length - pos  && *p && *p != '\n'; p++);
	*p = 0;
	Seek(pos + p - strbuf, SEEK_SET);
	return strbuf;
}

static TMap<unsigned int, FString> VirtualRenameTable;
// The reverse table was setup for the GOG version of Spear of Destiny.
// It seems awfully fragile in comparison to the above.
static TMap<unsigned int, FString> VirtualRenameReverse;

File::File(const FString &filename)
{
	init(filename);
}

File::File(const File &dir, const FString &filename)
{
	init(dir.getDirectory() + PATH_SEPARATOR + filename);
}

void File::init(FString filename)
{
	this->filename = filename;
	directory = false;
	existing = false;
	// We don't want to write from this libretro core
	// standalone write for savefiles but we use libretro serialization instead
	writable = false;

	// Are we trying to reference a renamed file?
	if(FString *fname = VirtualRenameTable.CheckKey(MakeKey(filename)))
		filename = *fname;

	int32_t size;
	int statflags = wrap_retro_vfs_stat(filename.GetChars(), &size);
	if(statflags & RETRO_VFS_STAT_IS_VALID)
		existing = true;

	if(existing)
	{
		if((statflags & RETRO_VFS_STAT_IS_DIRECTORY))
		{
			directory = true;

			// Populate a base list.
			RDIR *direct = retro_opendir(filename);
			if(direct != NULL)
			{
				const char *file = NULL;
				while((file = wrap_retro_vfs_readdir(direct)) != NULL)
					files.Push(file);
			}
			retro_closedir(direct);
		}
	}

	// Check for any virtual renames
	for(unsigned int i = 0;i < files.Size();++i)
	{
		FString *renamed = VirtualRenameReverse.CheckKey(MakeKey(getDirectory() + PATH_SEPARATOR + files[i]));
		if(renamed)
			files[i] = *renamed;
	}
}

FString File::getDirectory() const
{
	if(directory)
	{
		if(filename[filename.Len()-1] == '\\' || filename[filename.Len()-1] == '/')
			return filename.Left(filename.Len()-1);
		return filename;
	}

	long dirSepPos = filename.LastIndexOfAny("/\\");
	if(dirSepPos != -1)
		return filename.Left(dirSepPos);
	return FString(".");
}

FString File::getFileName() const
{
	if(directory)
		return FString();

	long dirSepPos = filename.LastIndexOfAny("/\\");
	if(dirSepPos != -1)
		return filename.Mid(dirSepPos+1);
	return filename;
}

FString File::getInsensitiveFile(const FString &filename, bool sensitiveExtension) const
{
	const TArray<FString> &files = getFileList();
	FString extension = filename.Mid(filename.LastIndexOf('.')+1);

	for(unsigned int i = 0;i < files.Size();++i)
	{
		if(files[i].CompareNoCase(filename) == 0)
		{
			if(!sensitiveExtension || files[i].Mid(files[i].LastIndexOf('.')+1).Compare(extension) == 0)
				return files[i];
		}
	}
	return filename;
}

/**
 * Perform a virtual rename on the file. This function is designed to handle
 * renaming improperly set copies of Spear of Destiny. Since we can't assume
 * that the user can physically rename the file, we need to do the remapping
 * in memory. We could just load the SD1 files, but that may be undesirable
 * for save compatibility and/or multiplayer.
 */
void File::rename(const FString &newname)
{
//	char path[MAX_PATH];
//	FileSys::FullFileName(filename, path);
//	filename = path;

	VirtualRenameReverse[MakeKey(filename)] = newname;
	VirtualRenameTable[MakeKey(getDirectory() + PATH_SEPARATOR + newname)] = filename;
}
