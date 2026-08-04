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

// Copyright (C) 2006 Chris Sarantos <csarantos@gmail.com>

#ifndef PROJECTILE_RESULT_H
#define PROJECTILE_RESULT_H

/**
*	CProjectileResult is a dummy object that may be spawned when a projectile hits
* a surface.  This object handles things like determining whether the projectile
* sticks in to the surface and may be retrieved, or if it breaks.  This object
* is also used to store any stim/response information on the projectile (e.g., 
* water arrow puts out torches when it detonates), and run other scripts if desired.
*
* NOTE: This object MUST be destroyed/removed in the scripts that are run
**/


class CProjectileResult : public idEntity {
public:
	CLASS_PROTOTYPE( CProjectileResult );

	CProjectileResult(void);
	virtual ~CProjectileResult(void) override;

/**
* Initialize the projectile result, called by the projectile
**/
	void Init
		(
			SFinalProjData *pData, const trace_t &collision,
			idProjectile *pProj, bool bActivate
		);

protected:
	/**
	* Chooses whether to run the "active" or "dud" script, based on the material
	* type hit and the activating material types.
	**/
	void RunResultScript( void );

protected:
	/**
	* Collision data from the impacted projectile
	**/
	trace_t			m_Collision;

	/**
	* Other data from the impacted projectile
	**/
	SFinalProjData	m_ProjData;

	/**
	* True => projectile activated, false => projectile is a dud
	**/
	bool			m_bActivated;

	/**
	* Getter events for scripting
	**/
	void Event_GetFinalVel( void );

	void Event_GetFinalAngVel( void );

	void Event_GetAxialDir( void );

	void Event_GetProjMass( void );

	void Event_GetSurfType( void );

	void Event_GetSurfNormal( void );

	void Event_GetStruckEnt( void );

	void Event_GetIncidenceAngle( void );

	void Event_GetActualStruckEnt( void ); // grayman #837

	void Event_IsVineFriendly( void ); // grayman #2787 
};

#endif
