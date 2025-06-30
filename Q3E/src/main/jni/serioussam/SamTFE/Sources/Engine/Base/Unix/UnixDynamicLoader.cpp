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

/* rcg10072001 Implemented. */

#include <dlfcn.h>
#include <locale.h>
#include <dirent.h>
#include <errno.h>

#include <Engine/Engine.h>
#include <Engine/Base/DynamicLoader.h>

CTString realname;

int default_filter(const struct dirent *ent)
{
    /* Keep only entries that do not begin with a '.'. */
    return (ent->d_name[0] != '.');
}

CTString SearchLib(const char *dirname)
{
    int  arg;
    /* Use user's locale rules (for alphasort). */
    setlocale(LC_ALL, "");
    struct dirent **list = NULL;
    int i, count;

    count = scandir((const char *)dirname, &list, default_filter, alphasort);
    if (count == -1) {
        CPrintF("CUnixDynamicLoader error: %s\n", strerror(errno));
        return CTString("");
    }

    for (i = 0; i < count; i++) {
        int _file = strncmp((const char *)list[i]->d_name, (const char *)"libvorbisfile.so", (size_t) 16 ); 
        if(_file == 0) {
           CPrintF("CUnixDynamicLoader: found %s\n", (const char *)list[i]->d_name);
            break;
        }
    }
    free(list);

    if (i+1 >= count) {
        CPrintF("CUnixDynamicLoader error: libvorbisfile not fiund\n");   
        return CTString("");
    }
    return CTString((const char *)list[i]->d_name);
}
class CUnixDynamicLoader : public CDynamicLoader
{
public:
    CUnixDynamicLoader(const char *libname);
    virtual ~CUnixDynamicLoader(void);
    virtual void *FindSymbol(const char *sym);
    virtual const char *GetError(void);

protected:
    void DoOpen(const char *lib);
    void SetError(void);
    void *module;
    CTString *err;
};


CDynamicLoader *CDynamicLoader::GetInstance(const char *libname)
{
    return(new CUnixDynamicLoader(libname));
}

void CUnixDynamicLoader::SetError(void)
{
    const char *errmsg = ::dlerror();
    delete err;
    err = NULL;

    if (errmsg != NULL)
    {
        CPrintF("CUnixDynamicLoader error: %s\n", errmsg);
        err = new CTString(errmsg);
    }
}


const char *CUnixDynamicLoader::GetError(void)
{
    return((err) ? (const char *) (*err) : NULL);
}


void *CUnixDynamicLoader::FindSymbol(const char *sym)
{
    //printf("Looking for symbol %s\n", sym);
    void *retval = NULL;
#if !defined(STATICALLY_LINKED)
    if (module != NULL) 
#endif
	{
        retval = ::dlsym(module, sym);
        SetError();
    }

    return(retval);
}


void CUnixDynamicLoader::DoOpen(const char *lib)
{
	if(!lib) // always find symbols
		return;
    #ifdef PLATFORM_FREEBSD
    dlerror(); // need for clean Undefined symbol "_nss_cache_cycle_prevention_function" message
    #endif

    CTFileName fnmLib = CTString(lib);
    CTFileName fnmLibname = fnmLib.FileName();
    int _libvorbisfile  = strncmp((const char *)fnmLibname, (const char *) "libvorbisfile", (size_t) 13 ); // skip
    if( _pShell->GetINDEX("sys_iSysPath") == 1 && _libvorbisfile !=0 ) {
        fnmLib = _fnmModLibPath + _fnmMod + fnmLib.FileName() + fnmLib.FileExt();
    }
    module = ::dlopen((const char *)fnmLib, RTLD_LAZY | RTLD_GLOBAL);
    #ifndef PLATFORM_MACOSX
    if (_libvorbisfile == 0 && module == NULL) { // if libvorbisfile.so not open trying libvorbisfile.so.3
        CPrintF("Trying load libvorbisfile.so.3 ...\n");
        fnmLib = fnmLibname + CTString(".so.3"); // libvorbisfile.so.3
        module = ::dlopen((const char *)fnmLib, RTLD_LAZY | RTLD_GLOBAL);
    }
    #endif
    #ifdef PLATFORM_FREEBSD
    if (_libvorbisfile == 0 && module == NULL) { // if libvorbisfile.so not open trying searching libvorbisfile.so.xxx
        CPrintF("Trying to search libvorbisfile.so.xxx ...\n");
        #if defined(__NetBSD__)
          realname = SearchLib((const char *)"/usr/pkg/lib");
        #else
          realname = SearchLib((const char *)"/usr/local/lib");
        #endif
        fnmLib = CTString(realname); // libvorbisfile.so.xxx
        module = ::dlopen((const char *)fnmLib, RTLD_LAZY | RTLD_GLOBAL);
    }
    #endif
    if (module == NULL) {
        SetError();
    }
}


CTFileName CDynamicLoader::ConvertLibNameToPlatform(const char *libname)
{
    #if PLATFORM_MACOSX
    const char *DLLEXTSTR = ".dylib";
    #else
    const char *DLLEXTSTR = ".so";
    #endif
    CTFileName fnm = CTString(libname);
    CTString libstr((strncmp("lib", fnm.FileName(), 3) == 0) ? "" : "lib");
    return(fnm.FileDir() + libstr + fnm.FileName() + DLLEXTSTR);
}


CUnixDynamicLoader::CUnixDynamicLoader(const char *libname)
    : module(NULL),
      err(NULL)
{
    if (libname == NULL) {
        DoOpen(NULL);
    } else {
        CTFileName fnm = ConvertLibNameToPlatform(libname);

        // Always try to dlopen from inside the game dirs before trying
        //  system libraries...
        if (fnm.FileDir() == "") {
          char buf[MAX_PATH];
          _pFileSystem->GetExecutablePath(buf, sizeof (buf));
          CTFileName fnmDir = CTString(buf);
        #ifdef PLATFORM_FREEBSD
          CTFileName fnmLib = CTString(libname);
          CTFileName fnmLibname = fnmLib.FileName();
          int _libvorbisfile   = strncmp((const char *)fnmLibname, (const char *) "libvorbisfile", (size_t) 13 );
          if( _pShell->GetINDEX("sys_iSysPath") == 1 && _libvorbisfile ==0 ) {
          #if defined(__NetBSD__)
            fnmDir = CTString("/usr/pkg/lib");
          #else
            fnmDir = CTString("/usr/local/lib");
          #endif
          } 
        #endif
            fnmDir = fnmDir.FileDir() + fnm;
            DoOpen(fnmDir);
            if (module != NULL) {
                return;
            }
        }

        DoOpen(fnm);
    }
}


CUnixDynamicLoader::~CUnixDynamicLoader(void)
{
    delete err;
    if (module != NULL)
        ::dlclose(module);
}


// end of UnixDynamicLoader.cpp ...


