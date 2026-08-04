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

#include "RevisionTracker.h"

#include <climits>
#include <vector>
#include <string>

//this auto-generated file contains svnversion output in string constant
#include "svnversion.h"


RevisionTracker::RevisionTracker() :
	_highestRevision(0),
	_lowestRevision(INT_MAX)
{
	_version = SVN_WORKING_COPY_VERSION;

	idStr buff = _version;
	for (int i = 0; i < buff.Length(); i++)
		if (!idStr::CharIsNumeric(buff[i]))
			buff[i] = ' ';
	idLexer lexer(buff.c_str(), buff.Length(), "svnversion");

	idToken token;
	if (lexer.ReadToken(&token) && token.IsNumeric()) {
		_lowestRevision = token.GetIntValue();
		if (lexer.ReadToken(&token) && token.IsNumeric())
			_highestRevision = token.GetIntValue();		//min:max
		else
			_highestRevision = _lowestRevision;				//rev
	}
	else {
		common->Warning("Cannot detect SVN version!");
		common->Printf("Make sure 'svnversion' command works in console on the build machine.\n\n");
	}
}

const char *RevisionTracker::GetRevisionString() const {
	return _version.c_str();
}

int RevisionTracker::GetHighestRevision() const
{
	return _highestRevision;
}

int RevisionTracker::GetLowestRevision() const
{
	return _lowestRevision;
}

int RevisionTracker::GetSavegameRevision() const
{
	// stgatilov: If you are doing hotfix release,
	// then override revision with hardcoded number here:
	//return 9108;

	return GetHighestRevision();
}

// Accessor to the singleton
RevisionTracker& RevisionTracker::Instance()
{
	static RevisionTracker _instance;
	return _instance;
}
