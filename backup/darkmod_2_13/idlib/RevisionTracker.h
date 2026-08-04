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
#ifndef __TDM_REVISION_TRACKER_H__
#define __TDM_REVISION_TRACKER_H__

/**
 * greebo: This class is a simple singleton keeping track of the highest and 
 * lowest SVN revision numbers. 
 *
 * The global function FileRevisionList() is taking care of registering 
 * the revision numbers to this class.
 */
class RevisionTracker
{
private:
	// Some stats
	int _highestRevision;
	int _lowestRevision;
	idStr _version;

public:
	/**
	 * Constructor is taking care of initialising the members.
	 */
	RevisionTracker();

	/**
	 * greebo: Accessor methods. Retrieves the highest and lowest revision
	 * and the number of registered files.
	 */
	int GetHighestRevision() const;
	int GetLowestRevision() const;

	/**
	 * Returns string representation of the current revision.
	 */
	const char *GetRevisionString() const;

	/**
	 * stgatilov: Returns revision of this executable for savegame checks.
	 * Note: it is overriden with hardcoded number for hotfix releases!
	 */
	int GetSavegameRevision() const;

	// Accessor to the singleton
	static RevisionTracker& Instance();
};

#endif /* __TDM_REVISION_TRACKER_H__ */
