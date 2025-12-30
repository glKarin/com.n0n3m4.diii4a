/*
** filesys.h
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
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

#ifndef __FILESYS_H__
#define __FILESYS_H__

#include "tarray.h"
#include "zstring.h"

#ifdef _WIN32
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

namespace FileSys
{
	enum ESpecialDirectory
	{
		DIR_Program,
		DIR_Configuration,
		DIR_Saves,
		DIR_ApplicationSupport,
		DIR_Documents,
		DIR_Screenshots,

		NUM_SPECIAL_DIRECTORIES
	};

	enum ESteamApp
	{
		APP_Wolfenstein3D,
		APP_SpearOfDestiny,
		APP_ThrowbackPack,
		APP_NoahsArk,

		NUM_STEAM_APPS
	};

	FString GetDirectoryPath(ESpecialDirectory dir);
	FString GetSteamPath(ESteamApp game);
	FString GetGOGPath(ESteamApp game);
	void SetDirectoryPath(ESpecialDirectory dir, const FString &path);
	void SetupPaths(int argc, const char* const *argv);

#ifdef __APPLE__
	FString OSX_FindFolder(ESpecialDirectory dir);
#endif
}

class File
{
	public:
		File(const FString &filename);
		File(const File &dir, const FString &filename);
		~File() {}

		bool					exists() const { return existing; }
		FString					getDirectory() const;
		FString					getFileName() const;
		const TArray<FString>	&getFileList() const { return files; }
		FString					getInsensitiveFile(const FString &filename, bool sensitiveExtension) const;
		FString					getPath() const { return filename; }
		bool					isDirectory() const { return directory; }
		bool					isFile() const { return !directory; }
		bool					isWritable() const { return writable; }
		FILE					*open(const char* mode) const;
		void					rename(const FString &newname);
		bool					remove();

	protected:
		void					init(FString filename);

		FString	filename;

		TArray<FString>	files;
		bool			directory;
		bool			existing;
		bool			writable;
};

#endif /* __FILESYS_H__ */
