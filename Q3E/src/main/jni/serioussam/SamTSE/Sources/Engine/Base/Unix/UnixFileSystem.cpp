/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

/* rcg10142001 Implemented. */


// !!! FIXME: rcg10142001 This should really be using CTStrings...


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/param.h>
#include <unistd.h>

#include <Engine/Engine.h>
#include <Engine/Base/FileSystem.h>

#if !defined(_DIII4A) //karin: no SDL
#include "SDL.h"
#endif

ENGINE_API CFileSystem *_pFileSystem = NULL;


class CUnixFileSystem : public CFileSystem
{
public:
    CUnixFileSystem(const char *argv0, const char *gamename);
    virtual ~CUnixFileSystem(void);
    virtual void GetExecutablePath(char *buf, ULONG bufSize);
    virtual void GetUserDirectory(char *buf, ULONG bufSize);
    virtual CDynamicArray<CTString> *FindFiles(const char *dir,
                                               const char *wildcard);
protected:
    char *exePath;
    char *userDir;
};

CFileSystem *CFileSystem::GetInstance(const char *argv0, const char *gamename)
{
    return(new CUnixFileSystem(argv0, gamename));
}


const char *CFileSystem::GetDirSeparator(void)
{
    return("/");
}


BOOL CFileSystem::IsDummyFile(const char *fname)
{
    return( (strcmp(fname, ".") == 0) || (strcmp(fname, "..") == 0) );
}


BOOL CFileSystem::Exists(const char *fname)
{
    struct stat s;
    if (stat(fname, &s) == -1)
        return(FALSE);

    return(TRUE);
}


BOOL CFileSystem::IsDirectory(const char *fname)
{
    struct stat s;
    if (stat(fname, &s) == -1){
        #ifdef NDEBUG
        //CPrintF(TRANSV("Debug: CFileSystem::IsDirectory error'%s' : \n"), (const char *)fname);
        #endif //_DEBUG
        return(FALSE);
    }
    #ifdef NDEBUG
	/*CPrintF(TRANSV("Debug: '%s' : "), (const char *)fname);
    switch (s.st_mode & S_IFMT) {
     case S_IFBLK:  CPrintF(TRANSV("block device\n"));            break;
     case S_IFCHR:  CPrintF(TRANSV("character device\n"));        break;
     case S_IFDIR:  CPrintF(TRANSV("directory\n"));               break;
     case S_IFIFO:  CPrintF(TRANSV("FIFO/pipe\n"));               break;
     case S_IFLNK:  CPrintF(TRANSV("symlink\n"));                 break;
     case S_IFREG:  CPrintF(TRANSV("regular file\n"));            break;
     case S_IFSOCK: CPrintF(TRANSV("socket\n"));                  break;
    default:        CPrintF(TRANSV("unknown?\n"));                break;
    }*/
    #endif //_DEBUG

    return(S_ISDIR(s.st_mode) ? TRUE : FALSE);
}


CUnixFileSystem::CUnixFileSystem(const char *argv0, const char *gamename)
{
#ifdef _DIII4A //karin: using cwd
	char cwd[MAX_PATH];
	if ( getcwd( cwd, sizeof( cwd ) - 1 ) == NULL )
	{
		strcpy(cwd, ".");
		cwd[1] = '\0';
	}
	else
		cwd[MAX_PATH-1] = '\0';
	strcat(cwd, "/"); // must append sep
	exePath = strdup(cwd);
	//strcat(cwd, ".local/");
	userDir = strdup(cwd);
#else
    exePath = SDL_GetBasePath();
    userDir = SDL_GetPrefPath("Serious-Engine", gamename);
#endif
}


CUnixFileSystem::~CUnixFileSystem(void)
{
#ifdef _DIII4A //karin: no SDL
    free(userDir);
    free(exePath);
#else
    SDL_free(userDir);
    SDL_free(exePath);
#endif
}


void CUnixFileSystem::GetExecutablePath(char *buf, ULONG bufSize)
{
#ifdef _DIII4A //karin: no SDL
    snprintf(buf, bufSize, "%s", exePath);
#else
    SDL_snprintf(buf, bufSize, "%s", exePath);
#endif
}


void CUnixFileSystem::GetUserDirectory(char *buf, ULONG bufSize)
{
#ifdef _DIII4A //karin: no SDL
    snprintf(buf, bufSize, "%s", userDir);
#else
    SDL_snprintf(buf, bufSize, "%s", userDir);
#endif
}


CDynamicArray<CTString> *CUnixFileSystem::FindFiles(const char *dir,
                                                   const char *wildcard)
{
    CDynamicArray<CTString> *retval = new CDynamicArray<CTString>;
    DIR *d = opendir(dir);

    if (d != NULL)
    {
        struct dirent *dent;
        while ((dent = readdir(d)) != NULL)
        {
            CTString str(dent->d_name);
            if (str.Matches(wildcard))
                *retval->New() = str;
        }
        closedir(d);
    }

    return(retval);
}

// end of UnixFileSystem.cpp ...


