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

#ifndef __SCRIPT_DOC_EXPORT_H__
#define __SCRIPT_DOC_EXPORT_H__

#include <map>
#include <string>
#include "../../idlib/Str.h"

class idFile;
class idEventDef;

class ScriptEventDocGenerator
{
protected:
	struct CompareCaseInsensitively
	{
		bool operator() (const std::string & s1, const std::string & s2) const
		{
			return idStr::Icmp(s1.c_str(), s2.c_str()) < 0;
		}
	};

	// Alphabetically sorted event map
	typedef std::map<std::string, const idEventDef*, CompareCaseInsensitively> EventMap;
	EventMap _events;

	idStr _dateStr;

public:
	ScriptEventDocGenerator();

	virtual void WriteDoc(idFile& outputFile) = 0;
};

// D3Script exporter
class ScriptEventDocGeneratorD3Script :
	public ScriptEventDocGenerator
{
public:
	virtual void WriteDoc(idFile& outputFile) override;

private:
	idStr GetEventDocumentation(const idEventDef& ev);
};

// Mediawiki exporter
class ScriptEventDocGeneratorMediaWiki :
	public ScriptEventDocGenerator
{
public:
	virtual void WriteDoc(idFile& outputFile) override;

private:
	idStr GetEventDescription(const idEventDef& ev);
	idStr GetEventDoc(const idEventDef* ev, bool includeSpawnclassInfo);
};

// XML exporter
class ScriptEventDocGeneratorXml :
	public ScriptEventDocGenerator
{
public:
	virtual void WriteDoc(idFile& outputFile) override;

private:
	idStr GetEventDescription(const idEventDef& ev);
	idStr GetEventDoc(const idEventDef* ev, bool includeSpawnclassInfo);
};

#endif
