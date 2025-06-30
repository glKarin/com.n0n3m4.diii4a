/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */

/* rcg10072001 Implemented. */

// !!! FIXME: rcg10142001 This should really be using CTStrings...
#include "Engine/StdH.h"
#include <io.h>

#include <Engine/Engine.h>
#include <Engine/Base/FileSystem.h>

ENGINE_API CFileSystem *_pFileSystem = NULL;


class CWin32FileSystem : public CFileSystem
{
public:
    CWin32FileSystem(const char *argv0, const char *gamename);
    virtual ~CWin32FileSystem(void);
    virtual void GetExecutablePath(char *buf, ULONG bufSize);
    virtual void GetUserDirectory(char *buf, ULONG bufSize);
    virtual CDynamicArray<CTString> *FindFiles(const char *dir,
                                               const char *wildcard);

protected:
    char *exePath;
    char *userDir;
};


const char *CFileSystem::GetDirSeparator(void)
{
    return("\\");
}


BOOL CFileSystem::IsDummyFile(const char *fname)
{
    return( (strcmp(fname, ".") == 0) || (strcmp(fname, "..") == 0) );
}

BOOL CFileSystem::Exists(const char *fname)
{
    ASSERTALWAYS("Write me!");
	return 0;
}

BOOL CFileSystem::IsDirectory(const char *fname)
{
    ASSERTALWAYS("Write me!");
	return 0;
}

CFileSystem *CFileSystem::GetInstance(const char *argv0, const char *gamename)
{
    return(new CWin32FileSystem(argv0, gamename));
}


CWin32FileSystem::CWin32FileSystem(const char *argv0, const char *gamename)
    : exePath(NULL),
     userDir(NULL)
{
    char buf[MAX_PATH];
    memset(buf, '\0', sizeof (buf));
    GetModuleFileNameA(NULL, buf, sizeof (buf) - 1);

    exePath = new char[strlen(buf) + 1];
    strcpy(exePath, buf);

    userDir = new char[strlen(buf) + 1];
    strcpy(userDir, buf);
    ASSERTALWAYS("We need to chop \\bin\\debug off the string if it's there.\n");
}


CWin32FileSystem::~CWin32FileSystem(void)
{
    delete[] exePath;
    delete[] userDir;
}


void CWin32FileSystem::GetExecutablePath(char *buf, ULONG bufSize)
{
    strncpy(buf, exePath, bufSize);
    buf[bufSize - 1] = '\0';  // just in case.
}


void CWin32FileSystem::GetUserDirectory(char *buf, ULONG bufSize)
{
    strncpy(buf, userDir, bufSize);
    buf[bufSize - 1] = '\0';  // just in case.
}


CDynamicArray<CTString> *CWin32FileSystem::FindFiles(const char *dir,
                                                    const char *wildcard)
{
    CDynamicArray<CTString> *retval = new CDynamicArray<CTString>;

    CTString str(dir);
    if (dir[strlen(dir) - 1] != '\\')
        str += "\\";

    struct _finddata_t c_file;
    intptr_t hFile = _findfirst( (const char *)(str+wildcard), &c_file );

    for (BOOL bFileExists = hFile!=-1;
         bFileExists;
         bFileExists = _findnext( hFile, &c_file )==0)
    {
        *retval->New() = c_file.name;
    }

    _findclose(hFile);
    return(retval);
}

// end of Win32FileSystem.cpp ...


