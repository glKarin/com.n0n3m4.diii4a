/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */

#ifndef SE_INCL_FILESYSTEM_H
#define SE_INCL_FILESYSTEM_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Engine.h>

// !!! FIXME: rcg10142001 This should really be using CTStrings...

/*
 * This class handles filesystem differences between platforms.
 */
class ENGINE_API CFileSystem {
public:
    // Construct a system-dependent version of CFileSystem.
    // (argv0) is argv[0] in your mainline.
    // gamename is a simple token that identifies your game.
  static CFileSystem *GetInstance(const char *argv0, const char *gamename);

  virtual ~CFileSystem(void) {}

    // Get the platform-specific directory separator. This could be
    //  "\\" on win32, "/" on unix, and ":" on MacOS Classic.
    //  Consider the returned pointer to be READ ONLY, as it points to a
    //  static, internal literal string on most platforms.
    //  Some platforms may define a dir separator that is MORE THAN ONE
    //  CHARACTER LONG. You have been warned.
  static const char *GetDirSeparator(void);

    // Returns TRUE if (fname) is not a real file ("." and ".." on win32, etc.)
    //  THIS DOES NOT CHECK IF A SPECIFIC FILE EXISTS IN THE FILESYSTEM!
  static BOOL IsDummyFile(const char *fname);

    // Returns TRUE if (fname) exists at all. May be a symlink, dir, file, etc.
  static BOOL Exists(const char *fname);

    // Returns TRUE if (fname) is a directory.
  static BOOL IsDirectory(const char *fname);

    // Get the path of the binary (.EXE file) being run.
    //  (buf) is where to store the info, and (bufSize) is the size, in bytes,
    //  of what's pointed to by (buf). The buffer is always promised to be
    //  null-terminated.
  virtual void GetExecutablePath(char *buf, ULONG bufSize) = 0;

    // Get the user directory. This is the user's home directory on systems
    //  with that concept, and the base (buf) is where to store the info, and
    //  (bufSize) is the size, in bytes, of what's pointed to by (buf). The
    //  buffer is always promised to be null-terminated, and, if there's room,
    //  with have a trailing dir separator. It is likely that you will have
    //  write permission in the user directory tree, and will NOT have write
    //  permission in the base directory. You have been warned.
  virtual void GetUserDirectory(char *buf, ULONG bufSize) = 0;

    // Get an array of CTStrings containing the names of files in (dir) that
    //  match (wildcard).
  virtual CDynamicArray<CTString> *FindFiles(const char *dir,
                                             const char *wildcard) = 0;

protected:
    // use GetInstance(), instead.
  CFileSystem() {}
};

ENGINE_API extern CFileSystem *_pFileSystem;

#endif

// end of FileSystem.h ...


