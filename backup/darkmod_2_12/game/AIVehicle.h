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

// Copyright (C) 2007 The Dark Mod Authors
//

#ifndef AIVEHICLE_H
#define AIVEHICLE_H

/**
 * AIVehicle is a derived class of idAI meant for AI that can be ridden around
 * by players as a vehicle, but can also act independently.
 * Players must be on the same team and frob them to start riding
 * Update: Players can now control them without being bound to them,
 * e.g., controlling them from a horse-drawn coach
 */

/**
* Contains information for a given vehicle speed/animation
**/
struct SAIVehicleSpeed
{
	idStr	Anim; // animation to play at this speed
	float	MinAnimRate; // anim rate modifier when we just barely switch into this speed
	float	MaxAnimRate; // anim rate modifier when we are about to switch to higher speed
	float	NextSpeedFrac; // fraction of max control speed at which we switch to the NEXT speed
};


class CAIVehicle : public idAI {
public:
	CLASS_PROTOTYPE( CAIVehicle );

							CAIVehicle( void );
	virtual					~CAIVehicle( void ) override;
	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void ) override;

	/**
	* Updates the facing angle
	**/
	void					UpdateSteering( void );

	/**
	* Updates requested speed.
	* For now, returns true if the player is pressing forward and movement is requested
	* Otherwise, returns false and only turning is requested
	**/
	bool					UpdateSpeed( void );

	/**
	* Executed when frobbed by the player: toggle mount/dismount
	**/
	void					PlayerFrob(idPlayer *player);

	/**
	* Returns the player that is controlling this AI's movement
	**/
	idPlayer *				GetController( void ) { return m_Controller.GetEntity(); };
	/**
	* Starts reading control input from the player and stops thinking independently
	* If player argument is NULL, returns control back to AI mind
	* This does not handle immobilizing the player, that is done elsewhere.
	**/
	void					SetController( idPlayer *player );

	// Script events
	void					Event_SetController( idPlayer *player );
	/**
	* This needs to be a separate script event since scripting didn't like
	* passing in $null_entity for some reason.
	**/
	void					Event_ClearController( void );
	void					Event_FrobRidable(idPlayer *player);
	/**
	* Get the current movement animation name if controlled
	**/
	void					Event_GetMoveAnim( void );

	/** Overload idAI::LinkScriptVariables to link new variables **/
	virtual void			LinkScriptVariables( void ) override;

public:
	/** Tell scripts we are under player control **/
	idScriptBool			AI_CONTROLLED;

protected:
	idEntityPtr<idPlayer>	m_Controller;
	/**
	* Joint to which the player is attached
	**/
	jointHandle_t			m_RideJoint;
	idVec3					m_RideOffset;
	idAngles				m_RideAngles;

	/**
	* Current world yaw we are pointing along [deg?]
	**/
	float					m_CurAngle;
	/**
	* Current leg animation (set by requested move speed)
	**/
	idStr					m_CurMoveAnim;
	/**
	* Requested speed, as a fraction of max speed
	**/
	float					m_SpeedFrac;
	/**
	* Speed at which the pointing angle can be changed (should be speed dependent)
	**/
	float					m_SteerSpeed;
	/**
	* Assuming a constant acceleration, how many seconds does it take to get to max speed?
	**/
	float					m_SpeedTimeToMax;

	// Arbitrary number of speeds
	idList<SAIVehicleSpeed> m_Speeds;

	// animation to play when jumping (no jumping if this is empty) NYI!
	idStr					m_JumpAnim;
};


#endif /* !AIVEHICLE_H */
