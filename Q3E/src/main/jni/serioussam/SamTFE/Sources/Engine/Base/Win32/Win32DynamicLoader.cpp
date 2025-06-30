/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */

/* rcg10072001 Implemented. */

#include "Engine/StdH.h"
#include <Engine/Engine.h>
#include <Engine/Base/DynamicLoader.h>
#include <Engine/Base/Shell.h>
#include <Engine/Base/Console.h>

class CWin32DynamicLoader : public CDynamicLoader
{
public:
    CWin32DynamicLoader(const char *libname);
    virtual ~CWin32DynamicLoader(void);
    virtual void *FindSymbol(const char *sym);
    virtual const char *GetError(void);

protected:
    void SetError(void);
    HMODULE module;
    CTString *err;
};


CDynamicLoader *CDynamicLoader::GetInstance(const char *libname)
{
    return(new CWin32DynamicLoader(libname));
}


void CWin32DynamicLoader::SetError(void)
{
	const char * errmsg = ::GetWindowsError(::GetLastError());
    delete err;
    err = NULL;

    if (errmsg != NULL)
        err = new CTString(errmsg);
}


const char *CWin32DynamicLoader::GetError(void)
{
    return((err) ? (const char *) (*err) : NULL);
}


void *CWin32DynamicLoader::FindSymbol(const char *sym)
{
    void *retval = NULL;
    if (module != NULL) {
        retval = ::GetProcAddress(module, sym);
        if (retval == NULL)
            SetError();
    }

    return(retval);
}


CTFileName CDynamicLoader::ConvertLibNameToPlatform(const char *libname)
{
    return(CTFileName(CTString(libname)));
}


CWin32DynamicLoader::CWin32DynamicLoader(const char *libname)
    : module(NULL),
      err(NULL)
{
	module = ::LoadLibraryA((const char *)libname);
	if (module == NULL)
		SetError();
/*
	CTFileName _fnm = (CTFileName(CTString(libname)));
	_fnm = _fnm.FileName();
    if (stricmp(_fnm.FileExt(), ".dll") != 0)
        _fnm += ".dll";

	CPrintF(TRANSV("CWin32DynamicLoader _fnm: %s\n"), (const char *)_fnm);
    module = ::LoadLibraryA((const char *) _fnm);
    if (module == NULL)
        SetError();
*/
}


CWin32DynamicLoader::~CWin32DynamicLoader(void)
{
    delete err;
    if (module != NULL)
        ::FreeLibrary(module);
}


// end of Win32DynamicLoader.cpp ...


