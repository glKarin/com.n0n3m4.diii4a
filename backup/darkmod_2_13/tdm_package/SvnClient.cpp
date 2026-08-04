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

#include "SvnClient.h"

#include "TraceLog.h"
#include "StdString.h"
#include <string.h>

namespace tdm
{

bool SvnClient::SetActive(const fs::path& repoRoot)
{
	_repoRoot = repoRoot;
	_versionedSet.clear();
	if (repoRoot.empty())
		return false;
	TraceLog::WriteLine(LOG_STANDARD, "Activating SVN client checks in " + repoRoot.string());

	char cmdline[1024];
	sprintf(cmdline, "cd %s && svn list -R . >versioned_files_list.txt", repoRoot.string().c_str());
	TraceLog::WriteLine(LOG_VERBOSE, std::string("SVN command line: ") + cmdline);
	int err = system(cmdline);
	if (err) {
		TraceLog::WriteLine(LOG_STANDARD, "Invoking SVN failed with error code " + std::to_string(err));
		return false;
	}

	fs::path filelist_fn = repoRoot / "versioned_files_list.txt";
	FILE *f = fopen(filelist_fn.string().c_str(), "r");
	char line[1024];
	while (fgets(line, sizeof(line), f))
	{
		int l = strlen(line);
		while (l > 0 && isspace(line[l-1]))
			l--;

		if (l == 0)
			break;		//empty line / EOL
		if (line[l-1] == '/')
			l--;		//directory

		line[l] = 0;
		_versionedSet.insert(line);
	}
	fclose(f);

	return true;
}

bool SvnClient::FileIsUnderVersionControl(const fs::path& path)
{
	if (_repoRoot.empty())
		return true;		//deactivated

	std::string target = path.string();
	if (!stdext::starts_with(target, _repoRoot.string()))
		throw std::runtime_error("SvnClientImpl::FileIsUnderVersionControl: path format problem");
	target.erase(0, _repoRoot.string().length() + 1);	//include next slash

	return _versionedSet.count(target) > 0;
}

} // namespace
