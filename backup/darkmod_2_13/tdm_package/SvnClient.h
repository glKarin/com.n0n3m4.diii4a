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

#include <memory>
#include <string>
#include <set>
#include "StdFilesystem.h"

namespace fs = stdext;

namespace tdm
{

class SvnClient;

/**
 * An object providing a few SVN client methods.
 * This is used to query file states.
 */
class SvnClient
{
public:
	// Try to activate the client in the specified SVN repository.
	// Deactivated clients will return true in FileIsUnderVersionControl().
	bool SetActive(const fs::path& repoRoot);

	// Returns true if the given file is under version control, false in all other cases
	bool FileIsUnderVersionControl(const fs::path& path);

private:
	fs::path _repoRoot;
	std::set<std::string> _versionedSet;
};

} // namespace
