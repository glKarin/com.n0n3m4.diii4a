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

#include "precompiled.h"
#pragma hdrstop



#include "ZipLoader.h"
#include <minizip/unzip.h>

#include <fstream>

CZipFile::CZipFile(unzFile handle) :
	_handle(handle)
{}

CZipFile::~CZipFile()
{
	unzClose(_handle);
}

bool CZipFile::ContainsFile(const idStr& fileName)
{
	int result = unzLocateFile(_handle, fileName.c_str(), 0);

	return (result == UNZ_OK);
}

idStr CZipFile::LoadTextFile(const idStr& fileName)
{
	int result = unzLocateFile(_handle, fileName.c_str(), 0);

	if (result != UNZ_OK) return "";

	unz_file_info info;
	unzGetCurrentFileInfo(_handle, &info, NULL, 0, NULL, 0, NULL, 0);

	unsigned long fileSize = info.uncompressed_size;

	int openResult = unzOpenCurrentFile(_handle);

	idStr returnValue;

	if (openResult == UNZ_OK)
	{
		char* buffer = new char[fileSize + 1];

		// Read and null-terminate the string
		unzReadCurrentFile(_handle, buffer, fileSize);
		buffer[fileSize] = '\0';

		returnValue = buffer;
		
		delete[] buffer;
	}

	unzCloseCurrentFile(_handle);

	return returnValue;
}

bool CZipFile::ExtractFileTo(const idStr& fileName, const idStr& destPath)
{
	bool returnValue = true;

	int result = unzLocateFile(_handle, fileName.c_str(), 0);

	if (result != UNZ_OK) return false;

	// Try to open the destination path before uncompressing the file
	FILE* outFile = fopen(destPath.c_str(), "wb");

	if (outFile == NULL) 
	{
		// couldn't open file for writing
		DM_LOG(LC_MAINMENU, LT_ERROR)LOGSTRING("Couldn't extract %s file to %s.\r", fileName.c_str(), destPath.c_str());
		return false; 
	}

	unz_file_info info;
	unzGetCurrentFileInfo(_handle, &info, NULL, 0, NULL, 0, NULL, 0);

	unsigned long fileSize = info.uncompressed_size;

	int openResult = unzOpenCurrentFile(_handle);

	if (openResult == UNZ_OK)
	{
		unsigned char* buffer = new unsigned char[fileSize];

		// Read and null-terminate the string
		unzReadCurrentFile(_handle, buffer, fileSize);

		fwrite(buffer, 1, fileSize, outFile);
				
		delete[] buffer;
	}
	else
	{
		returnValue = false; // fopen failed
	}

	fclose(outFile);
	unzCloseCurrentFile(_handle);

	return returnValue;
}

// --------------------------------------------------------

CZipLoader::CZipLoader()
{}

CZipFilePtr CZipLoader::OpenFile(const idStr& fullOSPath)
{
	DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Attempting to open file as ZIP: %s.\r", fullOSPath.c_str());
	unzFile handle = unzOpen(fullOSPath.c_str());

	return (handle != NULL) ? CZipFilePtr(new CZipFile(handle)) : CZipFilePtr();
}

CZipLoader& CZipLoader::Instance()
{
	static CZipLoader _instance;
	return _instance;
}
