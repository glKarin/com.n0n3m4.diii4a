#if _MSC_VER != 1900 || !defined(_USING_V110_SDK71_)
#error This hack is only needed with v140_xp to fix stat bug in Windows XP
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <sys/stat.h>

#define USE_WINDOWS_DWORD
#include "wl_def.h"

//==========================================================================
//
// VS14Stat
//
// Work around an issue where stat doesn't work with v140_xp. This was
// supposedly fixed, but as of Update 1 continues to not function on XP.
//
//==========================================================================

int VS14Stat(const char *path, struct _stat64i32 *buffer)
{
	WIN32_FILE_ATTRIBUTE_DATA data;
	if(!GetFileAttributesEx(path, GetFileExInfoStandard, &data))
		return -1;

	buffer->st_ino = 0;
	buffer->st_mode = ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? S_IFDIR : S_IFREG)|
		((data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? S_IREAD : S_IREAD|S_IWRITE);
	buffer->st_dev = buffer->st_rdev = 0;
	buffer->st_nlink = 1;
	buffer->st_uid = 0;
	buffer->st_gid = 0;
	buffer->st_size = data.nFileSizeLow;
	buffer->st_atime = (*(QWORD*)&data.ftLastAccessTime) / 10000000 - 11644473600LL;
	buffer->st_mtime = (*(QWORD*)&data.ftLastWriteTime) / 10000000 - 11644473600LL;
	buffer->st_ctime = (*(QWORD*)&data.ftCreationTime) / 10000000 - 11644473600LL;
	return 0;
}
