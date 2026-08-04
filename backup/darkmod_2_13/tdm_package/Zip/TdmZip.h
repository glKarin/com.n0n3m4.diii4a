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

#pragma once

#include <set>
#include <list>
#include <vector>
#include <time.h>
#include <cstdint>
#include <memory>
#include "../StdFilesystem.h"

namespace fs = stdext;

typedef void* zipFile;
typedef void* unzFile;

namespace tdm
{

/**
 * A class wrapping around a minizip file handle, providing
 * some convenience method to easily extract data from PK4s.
 */
class ZipFileRead
{
public:
	struct CompressedFile
	{
		std::vector<unsigned char>	data;
		time_t						changeTime;
		unsigned long				uncompressedSize;
		unsigned long				crc32;

		enum Method
		{
			STORED,
			DEFLATED,
		};

		Method						compressionMethod;
		int							compressionLevel;

		std::vector<unsigned char>	extraField;
		std::vector<char>			comment;
		std::vector<unsigned char>	localExtraField;
	};
	typedef std::shared_ptr<CompressedFile> CompressedFilePtr;

	struct MemberInfo
	{
		std::string filename;
		uint32_t crc;
		std::size_t uncompressedSize;
		std::size_t compressedSize;
	};

	class Visitor
	{
	public:
		virtual void VisitFile(const MemberInfo& info) = 0;
	};

private:
	// The handle for the zip archive
	unzFile _handle;

public:
	ZipFileRead(unzFile handle);

	~ZipFileRead();

	/** 
	 * Returns the number of files in this archive
	 */
	std::size_t GetNumFiles() const;

	/**
	 * Visit each archive member using the given visitor class.
	 */
	void ForeachFile(Visitor& visitor);

	/**
	 * Returns the global comment string of this archive.
	 */
	std::string GetGlobalComment();

	/**
	 * greebo: returns TRUE when this archive contains the given file.
	 */
	bool ContainsFile(const std::string& filename);

	/**
	 * Attempts to load the given text file. The string will be empty
	 * if the file failed to load.
	 */
	std::string LoadTextFile(const std::string& filename);

	/**
	 * greebo: Extracts the given file to the given destination path.
	 * @returns: TRUE on success, FALSE otherwise.
	 */
	bool ExtractFileTo(const std::string& filename, const fs::path& destPath);

	/**
	 * greebo: Extracts all contained files to the given destination path.
	 * Files found in the ignore list are not extracted.
	 *
	 * @ignoreIfExisting contains the filenames which should not be extracted if their
	 * destination file already exists.
	 * @ignoreList contains all the files which should not be extracted whatsoever.
	 *
	 * @throws: std::runtime_error if anything bad happens.
	 * @returns: the list of files which have been extracted.
	 */
	std::list<fs::path> ExtractAllFilesTo(const fs::path& destPath, 
						   const std::set<std::string>& ignoreIfExisting = std::set<std::string>(),
						   const std::set<std::string>& ignoreList = std::set<std::string>());

	/**
	 * greebo: Reads the compressed data from this zip file. This is used for 
	 * copying files between Zip archives without inflating them.
	 */
	CompressedFilePtr ReadCompressedFile(const std::string& filename);

	/**
	 * greebo: Iterates over all archive members, and XORs all CRCs, returns the result.
	 *
	 * @returns: the cumulative CRC.
	 * @throws: std::runtime_error on any failures.
	 */
	uint32_t GetCumulativeCrc();

    /**
    * Iterates over all archive members and checks for files with dates outside the acceptable range
    *
    * @returns: true if bad dates are found, false otherwise
    * @throws: std::runtime_error on any failures.
    */
    bool ContainsBadDate();
};
typedef std::shared_ptr<ZipFileRead> ZipFileReadPtr;

/**
 * A class wrapping around a minizip file handle, for storing data
 * into ZIP files.
 */
class ZipFileWrite
{
public:
	enum CompressionMethod
	{
		STORE,
		DEFLATE,
		DEFLATE_MAX,	// maximum level
	};

private:
	// The handle for the zip archive
	zipFile _handle;

	std::string _globalComment;

public:
	ZipFileWrite(zipFile handle);

	~ZipFileWrite();

	/**
	 * Sets the global comment of this ZIP file, will be written to the archive on close.
	 */
	void SetGlobalComment(const std::string& comment);

	/**
	 * greebo: Deflates the given file to the given destination path within the ZIP file.
	 * @returns: TRUE on success, FALSE otherwise.
	 */
	bool DeflateFile(const fs::path& fileToCompress, const std::string& destPath, CompressionMethod method = DEFLATE);

	/**
	 * greebo: Copy a file from another Zip file.
	 *
	 * @fromZip: The zip archive to copy from.
	 * @fromPath: The filename within @fromZip defining the source file.
	 * @toPath: The destination filename within this archive.
	 */
	bool CopyFileFromZip(const ZipFileReadPtr& fromZip, const std::string& fromPath, const std::string& toPath);
};
typedef std::shared_ptr<ZipFileWrite> ZipFileWritePtr;

/**
 * greebo: This service class can be used to load and inspect zip files and
 * retrieve specific files from the archive.
 */
class Zip
{
public:
	enum WriteMode
	{
		CREATE,
		APPEND
	};

	/**
	 * Tries to open the given file for read-only access.
	 * 
	 * @returns: NULL on failure, the file object on success.
	 */
	static ZipFileReadPtr OpenFileRead(const fs::path& fullPath);

	/**
	 * Tries to open the given file for write access.
	 * Pass the desired mode, either CREATE or APPEND.
	 *
	 * @returns: NULL on failure, the file object on success.
	 */
	static ZipFileWritePtr OpenFileWrite(const fs::path& fullPath, WriteMode mode);

	/**
	 * Removes the specified members from the given archive. As minizip doesn't provide a remove
	 * algorithm, this step is done by copying all files from one archive into another (in compressed form)
	 * and leaving those out which should be removed.
	 */
	static void RemoveFilesFromArchive(const fs::path& fullPath, const std::set<std::string>& membersToRemove);

    /**
    * Copy all files from one archive into another (in compressed form) and replace the original file with
    * the new version of the archive.
    */
    static void RecreateArchive(const fs::path& fullPath);

private:
    /**
    * Copy all files from one archive into another (in compressed form), leaving out those which should  
    * be removed. Replace the original file with the new version of the archive.
    */
    static void RecreateArchive(const fs::path& fullPath, const std::set<std::string>& membersToRemove);
};

} // namespace
