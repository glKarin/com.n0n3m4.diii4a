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
#ifndef __FORCE_PUSH_H__
#define __FORCE_PUSH_H__

#include "Force.h"

/**
 * greebo: This class should represent the push force as applied by the player
 * to large game world objects.
 */
class CForcePush : 
	public idForce
{
public:
	CLASS_PROTOTYPE( CForcePush );

						CForcePush();

	void				SetOwner(idEntity* ownerEnt);

	idEntity*			GetPushEntity(); // grayman #4603

	// Set physics object which is about to be pushed
	void				SetPushEntity(idEntity* pushEnt, int id = -1);

	// Set the push parameters for the next evaluation frame
	void				SetContactInfo(const trace_t& contactInfo, const idVec3& impactVelocity);

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

public: // common force interface
	virtual void		Evaluate( int time ) override;

private:
	void				SetOwnerIsPushing(bool isPushing);

private:
	idEntity*				pushEnt;		// entity being pushed
	idEntityPtr<idEntity>	lastPushEnt;	// the entity we pushed last frame

	int					id;				// clip model id of physics object
	trace_t				contactInfo;	// the contact info of the object we're pushing
	idVec3				impactVelocity;	// the velocity the owner had at impact time
	int					startPushTime;	// the time we started to push the physics object

	idEntity*			owner;			// the owning entity
};
typedef std::shared_ptr<CForcePush> CForcePushPtr;

#endif /* __FORCE_PUSH_H__ */
