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

#ifndef __AREA_MANAGER_H__
#define __AREA_MANAGER_H__

#include <map>
#include <set>

class idAI;

namespace ai
{

class AreaManager
{
private:
	// angua: Forbidden areas (e.g. areas with locked doors) are excluded from path finding 
	// for specific AI
	// ForbiddenAreasMap: multimap of area number and the AI for which this area should be excluded
	typedef std::multimap<int, const idAI*> ForbiddenAreasMap;
	ForbiddenAreasMap _forbiddenAreas;

	// angua: AiAreasMap: gives a set of areas for each AI (for faster lookup)
	typedef std::set<int> AreaSet;
	typedef std::map<const idAI*, AreaSet> AiAreasMap;
	AiAreasMap _aiAreas;

public:
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

	bool AddForbiddenArea(int areanum, const idAI* ai);
	bool AreaIsForbidden(int areanum, const idAI* ai) const;
	void RemoveForbiddenArea(int areanum, const idAI* ai);

	void DisableForbiddenAreas(const idAI* ai);
	void EnableForbiddenAreas(const idAI* ai);

	void Clear();
};

} // namespace ai

#endif /* __AREA_MANAGER_H__ */
