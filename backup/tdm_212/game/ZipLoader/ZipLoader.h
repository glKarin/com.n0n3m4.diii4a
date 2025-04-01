/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef ZIPLOADER_H_
#define ZIPLOADER_H_

#pragma hdrstop

#include "precompiled.h"

/**
 * A class wrapping around a minizip file handle, providing
 * some convenience method to easily extract data from PK4s.
 */
class CZipFile
{
	typedef void* unzFile;	//type from minizip's unzip.h

	// The handle for the zip archive
	unzFile _handle;

public:
	CZipFile(unzFile handle);

	~CZipFile();

	/**
	 * greebo: returns TRUE when this archive contains the given file.
	 */
	bool ContainsFile(const idStr& fileName);

	/**
	 * Attempts to load the given text file. The string will be empty
	 * if the file failed to load.
	 */
	idStr LoadTextFile(const idStr& fileName);

	/**
	 * greebo: Extracts the given file to the given destination path.
	 * @returns: TRUE on success, FALSE otherwise.
	 */
	bool ExtractFileTo(const idStr& fileName, const idStr& destPath);
};
typedef std::shared_ptr<CZipFile> CZipFilePtr;

/**
 * greebo: This service class can be used to load and inspect zip files and
 * retrieve specific files from the archive. D3 filesystem doesn't expose
 * ZIP loading functionality, so this is the do-it-yourself approach.
 */
class CZipLoader
{
	// Private constructor
	CZipLoader();

public:
	/**
	 * Tries to load the given file (path is absolute, use D3 VFS functions
	 * to resolve a relative D3 path to a full OS path).
	 * 
	 * @returns: NULL on failure, the file object on success.
	 */
	CZipFilePtr OpenFile(const idStr& fullOSPath);

	// Singleton instance
	static CZipLoader& Instance();
};

#endif /* ZIPLOADER_H_ */
