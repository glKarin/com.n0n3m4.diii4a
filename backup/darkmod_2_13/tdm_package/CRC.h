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

#ifndef _TDM_CRC_H_
#define _TDM_CRC_H_

#include <string>
#include <stdexcept>
#include <sstream>
#include <cstdio>
#include <cstdint>

#include "StdString.h"
#include "StdFormat.h"
#include "StdFilesystem.h"

#include "File.h"
#include "Zip/TdmZip.h"
#include "TraceLog.h"
#include "zlib.h"

namespace fs = stdext;

namespace tdm
{

/**
 * Service class for CRC handling.
 */
class CRC
{
public:
	static uint32_t ParseFromString(const std::string& hexaStr)
	{
		uint32_t out;

		std::stringstream ss;

		if (!stdext::starts_with(hexaStr, "0x"))
		{
			ss << ("0x" + hexaStr);
		}
		else
		{
			ss << hexaStr;
		}

		ss >> std::hex >> out;
		
		return out;
	}

	static std::string ToString(uint32_t crc)
	{
		return stdext::format("%x", crc);
	}

	/**
	 * greebo: Returns the CRC for the given file. ZIP-archives (PK4s too) will 
	 * be opened and a cumulative CRC over the archive members will be returned.
	 *
	 * @throws: std::runtime_error if something goes wrong.
	 */
	static uint32_t GetCrcForFile(const fs::path& file)
	{
		try
		{
			if (File::IsArchive(file))
			{
				return GetCrcForZip(file);
			}
			else
			{
				return GetCrcForNonZipFile(file);
			}
		}
		catch (std::runtime_error& ex)
		{
			TraceLog::Write(LOG_ERROR, ex.what());
			throw ex;
		}
	}
	
	static uint32_t GetCrcForNonZipFile(const fs::path& file)
	{
		// Open the file for reading
		FILE* fh = fopen(file.string().c_str(), "rb");

		if (fh == NULL) throw std::runtime_error("Could not open file: " + file.string());

		uint32_t crc = crc32(0, NULL, 0);
		
		while (true)
		{
			// Read the file in 32kb chunks
			char buf[32*1024];

			size_t bytesRead = fread(buf, 1, sizeof(buf), fh);

			if (bytesRead > 0)
			{
				crc = crc32(crc, (Bytef*)buf, bytesRead);
				continue;
			}
			
			break;
		}

		TraceLog::WriteLine(LOG_VERBOSE, "CRC calculated for file " + file.string() + " = " + stdext::format("%x", crc));

		fclose(fh);

		return crc;
	}

	static uint32_t GetCrcForZip(const fs::path& file)
	{
		// Open the file for reading
		ZipFileReadPtr zipFile = Zip::OpenFileRead(file);

		if (zipFile == NULL) throw std::runtime_error("Could not open ZIP file: " + file.string());

		uint32_t crc = zipFile->GetCumulativeCrc();

		TraceLog::WriteLine(LOG_VERBOSE, "CRC calculated for zip file " + file.string() + " = " + stdext::format("%x", crc));

		return crc;
	}
};

} // namespace

#endif /* _TDM_CRC_H_ */
