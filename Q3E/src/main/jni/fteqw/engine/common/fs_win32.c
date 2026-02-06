#include "quakedef.h"
#include "fs.h"
#include "winquake.h"

#ifdef FTE_SDL
#define WinNT true
#endif

//FIXME: find somewhere better for this win32 utility code.
//(its here instead of sys_win.c because dedicated servers don't use sys_win.c)

//outlen is the size of out in _BYTES_.
wchar_t *widen(wchar_t *out, size_t outbytes, const char *utf8)
{
	size_t outlen;
	wchar_t *ret = out;
	//utf-8 to utf-16, not ucs-2.
	unsigned int codepoint;
	int error;
	outlen = outbytes/sizeof(wchar_t);
	if (!outlen)
		return L"";
	outlen--;
	while (*utf8)
	{
		codepoint = utf8_decode(&error, utf8, (void*)&utf8);
		if (error || codepoint > 0x10FFFFu)
			codepoint = 0xFFFDu;
		if (codepoint > 0xffff)
		{
			if (outlen < 2)
				break;
			outlen -= 2;
			codepoint -= 0x10000u;
			*out++ = 0xD800 | (codepoint>>10);
			*out++ = 0xDC00 | (codepoint&0x3ff);
		}
		else
		{
			if (outlen < 1)
				break;
			outlen -= 1;
			*out++ = codepoint;
		}
	}
	*out = 0;
	return ret;
}

char *narrowen(char *out, size_t outlen, wchar_t *wide)
{
	char *ret = out;
	int bytes;
	unsigned int codepoint;
	if (!outlen)
		return "";
	outlen--;
	//utf-8 to utf-16, not ucs-2.
	while (*wide)
	{
		codepoint = *wide++;
		if (codepoint >= 0xD800u && codepoint <= 0xDBFFu)
		{	//handle utf-16 surrogates
			if (*wide >= 0xDC00u && *wide <= 0xDFFFu)
			{
				codepoint = (codepoint&0x3ff)<<10;
				codepoint |= *wide++ & 0x3ff;
			}
			else
				codepoint = 0xFFFDu;
		}
		bytes = utf8_encode(out, codepoint, outlen);
		if (bytes <= 0)
			break;
		out += bytes;
		outlen -= bytes;
	}
	*out = 0;
	return ret;
}

int MyRegGetIntValue(void *base, const char *keyname, const char *valuename, int defaultval)
{
	int result = defaultval;
	DWORD datalen = sizeof(result);
	HKEY subkey;
	DWORD type = REG_NONE;
	wchar_t wide[MAX_PATH];
	if (RegOpenKeyExW(base, widen(wide, sizeof(wide), keyname), 0, KEY_READ, &subkey) == ERROR_SUCCESS)
	{
		if (ERROR_SUCCESS != RegQueryValueExW(subkey, widen(wide, sizeof(wide), valuename), NULL, &type, (void*)&result, &datalen) || type != REG_DWORD)
			result = defaultval;
		RegCloseKey (subkey);
	}
	return result;
}
//result is utf-8
qboolean MyRegGetStringValue(void *base, const char *keyname, const char *valuename, void *data, size_t datalen)
{
	qboolean result = false;
	HKEY subkey;
	DWORD type = REG_NONE;
	wchar_t wide[MAX_PATH];
	wchar_t wdata[2048];
	DWORD dwlen = sizeof(wdata) - sizeof(wdata[0]);
	if (RegOpenKeyExW(base, widen(wide, sizeof(wide), keyname), 0, KEY_READ, &subkey) == ERROR_SUCCESS)
	{
		result = ERROR_SUCCESS == RegQueryValueExW(subkey, widen(wide, sizeof(wide), valuename), NULL, &type, (BYTE*)wdata, &dwlen);
		RegCloseKey (subkey);
	}

	if (result && (type == REG_SZ || type == REG_EXPAND_SZ))
	{
		wdata[dwlen/sizeof(wchar_t)] = 0;
		narrowen(data, datalen, wdata);
	}
	else
		((char*)data)[0] = 0;
	return result;
}
qboolean MyRegGetStringValueMultiSz(void *base, const char *keyname, const char *valuename, void *data, int datalen)
{
	qboolean result = false;
	HKEY subkey;
	wchar_t wide[MAX_PATH];
	DWORD type = REG_NONE;
	if (RegOpenKeyExW(base, widen(wide, sizeof(wide), keyname), 0, KEY_READ, &subkey) == ERROR_SUCCESS)
	{
		DWORD dwlen = datalen;
		result = ERROR_SUCCESS == RegQueryValueEx(subkey, valuename, NULL, &type, data, &dwlen);
		datalen = dwlen;
		RegCloseKey (subkey);
	}

	if (type == REG_MULTI_SZ)
		return result;
	return false;
}

qboolean MyRegSetValue(void *base, const char *keyname, const char *valuename, int type, const void *data, int datalen)
{
	qboolean result = false;
	HKEY subkey;
	wchar_t wide[MAX_PATH];
	wchar_t wided[2048];

	if (type == REG_SZ)
	{
		data = widen(wided, sizeof(wided), data);
		datalen = wcslen(wided)*2;
	}

	//'trivially' return success if its already set.
	//this allows success even when we don't have write access.
	if (RegOpenKeyExW(base, widen(wide, sizeof(wide), keyname), 0, KEY_READ, &subkey) == ERROR_SUCCESS)
	{
		DWORD oldtype;
		char olddata[2048];
		DWORD olddatalen = sizeof(olddata);
		result = ERROR_SUCCESS == RegQueryValueExW(subkey, widen(wide, sizeof(wide), valuename), NULL, &oldtype, olddata, &olddatalen);
		RegCloseKey (subkey);

		if (oldtype == REG_SZ || oldtype == REG_EXPAND_SZ)
		{	//ignore any null terminators that may have come along for the ride
			while(olddatalen > 1 && olddata[olddatalen-2] && olddata[olddatalen-1] == 0)
				olddatalen-=2;
		}

		if (result && datalen == olddatalen && type == oldtype && !memcmp(data, olddata, datalen))
			return result;
		result = false;
	}

	if (RegCreateKeyExW(base, widen(wide, sizeof(wide), keyname), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &subkey, NULL) == ERROR_SUCCESS)
	{
		result = ERROR_SUCCESS == RegSetValueExW(subkey, widen(wide, sizeof(wide), valuename), 0, type, data, datalen);
		RegCloseKey (subkey);
	}
	return result;
}
void MyRegDeleteKeyValue(void *base, const char *keyname, const char *valuename)
{
	HKEY subkey;
	wchar_t wide[MAX_PATH];
	if (RegOpenKeyExW(base, widen(wide, sizeof(wide), keyname), 0, KEY_WRITE, &subkey) == ERROR_SUCCESS)
	{
		RegDeleteValueW(subkey, widen(wide, sizeof(wide), valuename));
		RegCloseKey (subkey);
	}
}





#ifndef WINRT	//winrt is too annoying. lets just use stdio.

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ~0
#endif

//read-only memory mapped files.
//for write access, we use the stdio module as a fallback.
//do you think anyone will ever notice that utf8 filenames work even in windows? probably not. oh well, worth a try.

#define VFSW32_Open VFSOS_Open
#define VFSW32_OpenPath VFSOS_OpenPath
#define VFSW32_OpenTemp FS_OpenTemp

typedef struct {
	searchpathfuncs_t pub;
	HANDLE changenotification;
	void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle);
	int hashdepth;
	char rootpath[1];
} vfsw32path_t;
typedef struct {
	vfsfile_t funcs;
	HANDLE hand;
	HANDLE mmh;
	void *mmap;
	unsigned int length;
	unsigned int offset;
} vfsw32file_t;

static int QDECL VFSW32_ReadBytes (struct vfsfile_s *file, void *buffer, int bytestoread)
{
	DWORD read;
	vfsw32file_t *intfile = (vfsw32file_t*)file;
	if (intfile->mmap)
	{
		if (intfile->offset+bytestoread > intfile->length)
			bytestoread = intfile->length-intfile->offset;
		if (bytestoread < 0)
			bytestoread = 0;	//shouldn't happen...

		memcpy(buffer, (char*)intfile->mmap + intfile->offset, bytestoread);
		intfile->offset += bytestoread;
		return bytestoread;
	}
	if (!ReadFile(intfile->hand, buffer, bytestoread, &read, NULL))
		return 0;
	return read;
}
static int QDECL VFSW32_WriteBytes (struct vfsfile_s *file, const void *buffer, int bytestoread)
{
	DWORD written;
	vfsw32file_t *intfile = (vfsw32file_t*)file;
	if (intfile->mmap)
	{
		if (intfile->offset+bytestoread > intfile->length)
			bytestoread = intfile->length-intfile->offset;

		memcpy((char*)intfile->mmap + intfile->offset, buffer, bytestoread);
		intfile->offset += bytestoread;
		return bytestoread;
	}

	if (!WriteFile(intfile->hand, buffer, bytestoread, &written, NULL))
	{
//		DWORD err = GetLastError();
		// ERROR_INVALID_USER_BUFFER or ERROR_NOT_ENOUGH_MEMORY 
		return 0;
	}
	return written;
}
static qboolean QDECL VFSW32_Seek (struct vfsfile_s *file, qofs_t pos)
{
	DWORD hi, lo;
	vfsw32file_t *intfile = (vfsw32file_t*)file;
	if (intfile->mmap)
	{
		intfile->offset = pos;
		return true;
	}

	lo = qofs_Low(pos);
	hi = qofs_High(pos);
	return SetFilePointer(intfile->hand, lo, &hi, FILE_BEGIN) != INVALID_SET_FILE_POINTER;
}
static qofs_t QDECL VFSW32_Tell (struct vfsfile_s *file)
{
	DWORD hi = 0, lo;
	vfsw32file_t *intfile = (vfsw32file_t*)file;
	if (intfile->mmap)
		return intfile->offset;
	lo = SetFilePointer(intfile->hand, 0, &hi, FILE_CURRENT);
	return qofs_Make(lo,hi);
}
static void QDECL VFSW32_Flush(struct vfsfile_s *file)
{
	vfsw32file_t *intfile = (vfsw32file_t*)file;
	if (intfile->mmap)
		FlushViewOfFile(intfile->mmap, intfile->length);

	//we only really flush things to ensure that we don't get a stall later.
	//in windows, FlushFileBuffers can have significant costs, so lets see if anyone complains about us not flushing.
//	FlushFileBuffers(intfile->hand);
}
static qofs_t QDECL VFSW32_GetSize (struct vfsfile_s *file)
{
	DWORD lo, hi = 0;
	vfsw32file_t *intfile = (vfsw32file_t*)file;

	if (intfile->mmap)
		return intfile->length;
	lo = GetFileSize(intfile->hand, &hi);
	return qofs_Make(lo,hi);
}
static qboolean QDECL VFSW32_Close(vfsfile_t *file)
{
	vfsw32file_t *intfile = (vfsw32file_t*)file;
	if (intfile->mmap)
	{
		UnmapViewOfFile(intfile->mmap);
		CloseHandle(intfile->mmh);
	}
	CloseHandle(intfile->hand);
	Z_Free(file);
	return true;
}
static qboolean QDECL VFSW32_CloseTemp(vfsfile_t *file)
{
	vfsw32file_t *intfile = (vfsw32file_t*)file;
	if (intfile->mmap)
	{
		UnmapViewOfFile(intfile->mmap);
		CloseHandle(intfile->mmh);
	}
	CloseHandle(intfile->hand);
	DeleteFileA((char*)(intfile+1));
	Z_Free(file);
	return true;
}

vfsfile_t *QDECL VFSW32_OpenTemp(void)
{
	static int seq=-1;
	HANDLE h = INVALID_HANDLE_VALUE;
	vfsw32file_t *file;
	if (WinNT)
	{	//gotta use wide stuff.
		//on the plus side, FILE_SHARE_DELETE works.
		wchar_t osname[MAX_PATH];
		wchar_t tmppath[MAX_PATH];
		if (GetTempPathW(countof(tmppath), tmppath))
			if ((seq=GetTempFileNameW(tmppath, L"fte", ++seq, osname)))
				h = CreateFileW(osname, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE, NULL);
		if (!h)
			return VFSPIPE_Open(1, true);
		DeleteFileW(osname);

		file = Z_Malloc(sizeof(vfsw32file_t));
#ifdef _DEBUG
		narrowen(file->funcs.dbgname, sizeof(file->funcs.dbgname), osname);
#endif
		file->funcs.Close = VFSW32_Close;	//we already deleted it. woo.
	}
	else
	{	//can't use wide stuff.
		//FLIE_SHARE_DELETE doesn't work. we have to faff around ourselves.
		char osname[MAX_PATH];
		char tmppath[MAX_PATH];
		if (GetTempPathA(countof(tmppath), tmppath))
			if ((seq=GetTempFileNameA(tmppath, "fte", ++seq, osname)))
				h = CreateFileA(osname, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
		if (!h)
			return VFSPIPE_Open(1, true);

		file = Z_Malloc(sizeof(vfsw32file_t) + strlen(osname)+1);
		strcpy((char*)(file+1), osname);
#ifdef _DEBUG
		Q_strncpyz(file->funcs.dbgname, osname, sizeof(file->funcs.dbgname));
#endif
		file->funcs.Close = VFSW32_CloseTemp;	//gotta delete it after close. hopefully we won't crash too often...
	}
	file->funcs.ReadBytes = VFSW32_ReadBytes;
	file->funcs.WriteBytes = VFSW32_WriteBytes;
	file->funcs.Seek = VFSW32_Seek;
	file->funcs.Tell = VFSW32_Tell;
	file->funcs.GetLen = VFSW32_GetSize;
	file->funcs.Flush = VFSW32_Flush;
	file->hand = h;
	file->mmh = INVALID_HANDLE_VALUE;
	file->mmap = NULL;
	file->offset = 0;
	file->length = 0;

	return &file->funcs;
}

//WARNING: handle can be null
static vfsfile_t *QDECL VFSW32_OpenInternal(vfsw32path_t *handle, const char *quakename, const char *osname, const char *mode)
{
	HANDLE h, mh;
	unsigned int fsize;
	void *mmap;
	qboolean didexist = true;
	qboolean create;

	vfsw32file_t *file;
	qboolean read = !!strchr(mode, 'r');
	qboolean write = !!strchr(mode, 'w');
	qboolean append = !!strchr(mode, 'a');
	qboolean text = !!strchr(mode, 't');
	//qboolean persistent = !!strchr(mode, 'p');	//save to long-term storage
	write |= append;
	create = write;
	if (strchr(mode, '+'))
		read = write = true;

	if (fs_readonly && (write || append))
		return NULL;

	if (!WinNT)
	{
		//FILE_SHARE_DELETE is not supported in 9x, sorry.
		if (!create && write)
			h = CreateFileA(osname, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,	NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		else if ((write && read) || append)
			h = CreateFileA(osname, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,	NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		else if (write)
			h = CreateFileA(osname, GENERIC_READ|GENERIC_WRITE, 0,					NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		else if (read)
			h = CreateFileA(osname, GENERIC_READ,				FILE_SHARE_READ,	NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		else
			h = INVALID_HANDLE_VALUE;
	}
	else
	{
		wchar_t wide[MAX_OSPATH];
		widen(wide, sizeof(wide), osname);
		h = INVALID_HANDLE_VALUE;
		if (write || append)
		{
			//this extra block is to avoid flushing fs caches needlessly
			h = CreateFileW(wide, GENERIC_READ|GENERIC_WRITE,	FILE_SHARE_READ|FILE_SHARE_DELETE,	NULL, (!read&&!append)?CREATE_ALWAYS:OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (h == INVALID_HANDLE_VALUE)
				didexist = false;
		}

		if (h != INVALID_HANDLE_VALUE)
			;
		else if (!create && write)
			h = CreateFileW(wide, GENERIC_READ|GENERIC_WRITE,	FILE_SHARE_READ|FILE_SHARE_DELETE,	NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		else if ((write && read) || append)
			h = CreateFileW(wide, GENERIC_READ|GENERIC_WRITE,	FILE_SHARE_READ|FILE_SHARE_DELETE,	NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		else if (write)
			h = CreateFileW(wide, GENERIC_READ|GENERIC_WRITE,	FILE_SHARE_READ|FILE_SHARE_DELETE,	NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		else if (read)
			h = CreateFileW(wide, GENERIC_READ,					FILE_SHARE_READ|FILE_SHARE_DELETE,	NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		else
			h = INVALID_HANDLE_VALUE;
	}
	if (h == INVALID_HANDLE_VALUE)
		return NULL;

	if (!didexist)
	{
		if (handle && quakename && handle->AddFileHash)
			handle->AddFileHash(handle->hashdepth, quakename, NULL, handle);
		else
			FS_FlushFSHashFull();	//FIXME: no idea where this path is. if its inside a quake path, make sure it gets flushed properly. FIXME: his shouldn't be needed if we have change notifications working properly.
	}


	fsize = GetFileSize(h, NULL);
	if (write || append || text || fsize > 1024*1024*5)
	{
		fsize = 0;
		mh = INVALID_HANDLE_VALUE;
		mmap = NULL;

		/*if appending, set the access position to the end of the file*/
		if (append)
			SetFilePointer(h, 0, NULL, FILE_END);
	}
	else
	{
		mh = CreateFileMapping(h, NULL, PAGE_READONLY, 0, 0, NULL);
		if (mh == INVALID_HANDLE_VALUE)
			mmap = NULL;
		else
		{
			mmap = MapViewOfFile(mh, FILE_MAP_READ, 0, 0, fsize);
			if (mmap == NULL)
			{
				CloseHandle(mh);
				mh = INVALID_HANDLE_VALUE;
			}
		}
	}

	file = Z_Malloc(sizeof(vfsw32file_t));
#ifdef _DEBUG
	Q_strncpyz(file->funcs.dbgname, osname, sizeof(file->funcs.dbgname));
#endif
	file->funcs.ReadBytes = read?VFSW32_ReadBytes:NULL;
	file->funcs.WriteBytes = (write||append)?VFSW32_WriteBytes:NULL;
	file->funcs.Seek = VFSW32_Seek;
	file->funcs.Tell = VFSW32_Tell;
	file->funcs.GetLen = VFSW32_GetSize;
	file->funcs.Close = VFSW32_Close;
	file->funcs.Flush = VFSW32_Flush;
	file->hand = h;
	file->mmh = mh;
	file->mmap = mmap;
	file->offset = 0;
	file->length = fsize;

	return &file->funcs;
}

vfsfile_t *QDECL VFSW32_Open(const char *osname, const char *mode)
{
	//called without regard to a search path
	return VFSW32_OpenInternal(NULL, NULL, osname, mode);
}

#include <sys/stat.h>
static qboolean QDECL VFSW32_FileStat(searchpathfuncs_t *handle, flocation_t *loc, time_t *mtime)
{
	int r;
	struct _stat s;
	if (WinNT)
	{
		wchar_t wide[MAX_OSPATH];
		widen(wide, sizeof(wide), loc->rawname);
		r = _wstat(wide, &s);
	}
	else
		r = _stat(loc->rawname, &s);
	if (r)
		return false;
	*mtime = s.st_mtime;
	return true;
}

static vfsfile_t *QDECL VFSW32_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode)
{
	//path is already cleaned, as anything that gets a valid loc needs cleaning up first.
	vfsw32path_t *wp = (void*)handle;
	return VFSW32_OpenInternal(wp, loc->rawname+strlen(wp->rootpath)+1, loc->rawname, mode);
}
static void QDECL VFSW32_ClosePath(searchpathfuncs_t *handle)
{
	vfsw32path_t *wp = (void*)handle;
	if (wp->changenotification != INVALID_HANDLE_VALUE)
		FindCloseChangeNotification(wp->changenotification);
	Z_Free(wp);
}
static qboolean QDECL VFSW32_PollChanges(searchpathfuncs_t *handle)
{
	qboolean result = false;
	vfsw32path_t *wp = (void*)handle;

	if (wp->changenotification == INVALID_HANDLE_VALUE)
		return true;
	for(;;)
	{
		switch(WaitForSingleObject(wp->changenotification, 0))
		{
		case WAIT_OBJECT_0:
			result = true;
			break;
		case WAIT_TIMEOUT:
			return result;
		default:
			FindCloseChangeNotification(wp->changenotification);
			wp->changenotification = INVALID_HANDLE_VALUE;
			return true;
		}
		FindNextChangeNotification(wp->changenotification);
	}
	return result;
}
static int QDECL VFSW32_RebuildFSHash(const char *filename, qofs_t filesize, time_t mtime, void *handle, searchpathfuncs_t *spath)
{
	vfsw32path_t *wp = (void*)spath;
	if (filename[strlen(filename)-1] == '/')
	{	//this is actually a directory

		char childpath[256];
		Q_snprintfz(childpath, sizeof(childpath), "%s*", filename);
		Sys_EnumerateFiles(wp->rootpath, childpath, VFSW32_RebuildFSHash, handle, spath);
		return true;
	}

	wp->AddFileHash(wp->hashdepth, filename, NULL, wp);
	return true;
}
static void QDECL VFSW32_BuildHash(searchpathfuncs_t *handle, int hashdepth, void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle))
{
	vfsw32path_t *wp = (void*)handle;
	wp->AddFileHash = AddFileHash;
	wp->hashdepth = hashdepth;
	Sys_EnumerateFiles(wp->rootpath, "*", VFSW32_RebuildFSHash, AddFileHash, handle);
}
static qboolean QDECL VFSW32_CreateLoc(searchpathfuncs_t *handle, flocation_t *loc, const char *filename)
{
	vfsw32path_t *wp = (void*)handle;
	char *ofs;

	loc->len = 0;
	loc->offset = 0;
	loc->fhandle = handle;
	loc->rawname[sizeof(loc->rawname)-1] = 0;
	if (Q_snprintfz (loc->rawname, sizeof(loc->rawname), "%s%s", wp->rootpath, filename))
		return FF_NOTFOUND;
	for (ofs = loc->rawname+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/')
		{	// create the directory
			*ofs = 0;
			Sys_mkdir (loc->rawname);
			*ofs = '/';
		}
	}

	return FF_FOUND;
}

#include <errno.h>
static unsigned int QDECL VFSW32_FLocate(searchpathfuncs_t *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	vfsw32path_t *wp = (void*)handle;
	char netpath[MAX_OSPATH];
	wchar_t wide[MAX_OSPATH];
	qofs_t len;
	HANDLE h;
	DWORD attr;


	if (hashedresult && (void *)hashedresult != wp)
		return FF_NOTFOUND;

/*
	if (!static_registered)
	{	// if not a registered version, don't ever go beyond base
		if ( strchr (filename, '/') || strchr (filename,'\\'))
			continue;
	}
*/

// check a file in the directory tree
	if (Q_snprintfz (netpath, sizeof(netpath), "%s%s", wp->rootpath, filename))
		return FF_NOTFOUND;

	if (!WinNT)
	{
		WIN32_FIND_DATAA fda;
		h = FindFirstFileA(netpath, &fda);
		attr = fda.dwFileAttributes;
		len = (h == INVALID_HANDLE_VALUE)?0:qofs_Make(fda.nFileSizeLow, fda.nFileSizeHigh);
	}
	else
	{
		WIN32_FIND_DATAW fdw;
		h = FindFirstFileW(widen(wide, sizeof(wide), netpath), &fdw);
		attr = fdw.dwFileAttributes;
		len = (h == INVALID_HANDLE_VALUE)?0:qofs_Make(fdw.nFileSizeLow, fdw.nFileSizeHigh);
	}
	if (h == INVALID_HANDLE_VALUE)
	{
	//	int e = GetLastError();
	//  if (e == ERROR_PATH_NOT_FOUND)	//then look inside a zip
		return FF_NOTFOUND;
	}
	FindClose(h);
	if (loc)
	{
		if (attr & FILE_ATTRIBUTE_REPARSE_POINT)
		{	//when looking for reparse points, FindFirstFile only reports info about the link, not the file. which means the size is wrong.
			HANDLE f = CreateFileW(widen(wide, sizeof(wide), netpath), 0, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (f)
			{
				LARGE_INTEGER wsize;
				wsize.LowPart = GetFileSize(f, &wsize.HighPart);
				if (wsize.LowPart == INVALID_FILE_SIZE && GetLastError()!=NO_ERROR)
					wsize.LowPart = wsize.HighPart = 0;
				CloseHandle(f);
				len = qofs_Make(wsize.LowPart, wsize.HighPart);
			}
		}
		loc->len = len;
		loc->offset = 0;
		loc->fhandle = handle;
		Q_strncpyz(loc->rawname, netpath, sizeof(loc->rawname));
	}
	if (attr & FILE_ATTRIBUTE_DIRECTORY)
		return FF_DIRECTORY;	//not actually openable.
	return FF_FOUND;
}
static void QDECL VFSW32_ReadFile(searchpathfuncs_t *handle, flocation_t *loc, char *buffer)
{
//	vfsw32path_t *wp = handle;

	FILE *f;
	wchar_t wide[MAX_OSPATH];
	if (!WinNT)
		f = fopen(loc->rawname, "rb");
	else
		f = _wfopen(widen(wide, sizeof(wide), loc->rawname), L"rb");
	if (!f)	//err...
		return;
	fseek(f, loc->offset, SEEK_SET);
	fread(buffer, 1, loc->len, f);
	fclose(f);
}
static int QDECL VFSW32_EnumerateFiles (searchpathfuncs_t *handle, const char *match, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *spath), void *parm)
{
	vfsw32path_t *wp = (vfsw32path_t*)handle;
	return Sys_EnumerateFiles(wp->rootpath, match, func, parm, handle);
}

static qboolean QDECL VFSW32_RenameFile(searchpathfuncs_t *handle, const char *oldfname, const char *newfname)
{
	vfsw32path_t *wp = (vfsw32path_t*)handle;
	char oldsyspath[MAX_OSPATH];
	char newsyspath[MAX_OSPATH];
	if (fs_readonly)
		return false;
	snprintf (oldsyspath, sizeof(oldsyspath)-1, "%s%s", wp->rootpath, oldfname);
	snprintf (newsyspath, sizeof(newsyspath)-1, "%s%s", wp->rootpath, newfname);
	return Sys_Rename(oldsyspath, newsyspath);
}
static qboolean QDECL VFSW32_RemoveFile(searchpathfuncs_t *handle, const char *filename)
{
	vfsw32path_t *wp = (vfsw32path_t*)handle;
	char syspath[MAX_OSPATH];
	if (fs_readonly)
		return false;
	snprintf (syspath, sizeof(syspath)-1, "%s%s", wp->rootpath, filename);
	if (*filename && filename[strlen(filename)-1] == '/')
		return Sys_rmdir(syspath);
	return Sys_remove(syspath);
}

searchpathfuncs_t *QDECL VFSW32_OpenPath(vfsfile_t *mustbenull, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix)
{
	vfsw32path_t *np;
	int dlen = strlen(desc);
	if (mustbenull)
		return NULL;
	if (prefix && *prefix)
		return NULL;	//don't try to support this. too risky with absolute paths etc.
	np = Z_Malloc(sizeof(*np) + dlen);
	if (np)
	{
		wchar_t wide[MAX_OSPATH];
		memcpy(np->rootpath, desc, dlen+1);
		if (*np->rootpath)
			Q_strncpy(np->rootpath+dlen, "/", 2);
		if (!WinNT)
			np->changenotification = FindFirstChangeNotificationA(np->rootpath, true, FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_CREATION);
		else
			np->changenotification = FindFirstChangeNotificationW(widen(wide, sizeof(wide), np->rootpath), true, FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_CREATION);
	}

	np->pub.fsver			= FSVER;
	np->pub.ClosePath		= VFSW32_ClosePath;
	np->pub.BuildHash		= VFSW32_BuildHash;
	np->pub.FindFile		= VFSW32_FLocate;
	np->pub.ReadFile		= VFSW32_ReadFile;
	np->pub.EnumerateFiles	= VFSW32_EnumerateFiles;
	np->pub.OpenVFS			= VFSW32_OpenVFS;
	np->pub.PollChanges		= VFSW32_PollChanges;

	np->pub.FileStat		= VFSW32_FileStat;

#undef CreateFile //stoopid windows.h
	np->pub.CreateFile		= VFSW32_CreateLoc;
	np->pub.RenameFile		= VFSW32_RenameFile;
	np->pub.RemoveFile		= VFSW32_RemoveFile;

	return &np->pub;
}
#endif



qboolean Sys_GetFreeDiskSpace(const char *path, quint64_t *freespace)
{	//symlinks means the path needs to be fairly full. it may also be a file, and relative to the working directory.
	wchar_t ffs[MAX_OSPATH];
	ULARGE_INTEGER freebytes;
	unsigned int err;
	if (GetDiskFreeSpaceExW(widen(ffs, sizeof(ffs), path), &freebytes, NULL, NULL))
	{
		*freespace = freebytes.QuadPart;
		return true;
	}
	err = GetLastError();
	Con_Printf("GetDiskFreeSpaceExW(%s) failed: %x\n", path, err);
	return false;
}
