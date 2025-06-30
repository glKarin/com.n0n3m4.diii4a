/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */

#ifndef SE_INCL_DYNAMICLOADER_H
#define SE_INCL_DYNAMICLOADER_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

/*
 * This class handles DLL loading in a platform-abstracted manner.
 */
class ENGINE_API CDynamicLoader {
public:
    // Construct a CDynamicLoader to handle symbol lookups in a shared lib.
    // (libname) is a platform-independent filename. If you specify "game",
    //  then you get "Game.dll" on windows, and "libgame.so" on Linux.
    //  You should ALWAYS use lowercase, unless you know what you're doing.
  static CDynamicLoader *GetInstance(const char *libname);

    // Convert a win32 DLL path to a platform-specific equivalent.
    //  ("Bin\\Entities.dll" becomes "Bin/libEntities.so" on Linux, etc.)
  static CTFileName ConvertLibNameToPlatform(const char *libname);

  virtual ~CDynamicLoader(void) {}

    // Lookup (sym) and return a pointer to it.
    //  Returns NULL on error/symbol not found.
  virtual void *FindSymbol(const char *sym) = 0;

    // return a human-readable error message. NULL if there's no error.
  virtual const char *GetError(void) = 0;

protected:
    // use GetInstance(), instead.
  CDynamicLoader() {}
};

#endif

// end of DynamicLoader.h ...


