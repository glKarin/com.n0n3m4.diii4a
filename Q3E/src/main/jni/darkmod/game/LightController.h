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

// Copyright (C) 2010 Tels (Donated to The Dark Mod)

#ifndef __DARKMOD_LIGHTCONTROLLER_H__
#define __DARKMOD_LIGHTCONTROLLER_H__

/*
===============================================================================

  Light Controller -  control local ambient lights depending on which lights
  are in the same area/close to them.

  This class is a singleton and initiated/destroyed from gameLocal.

===============================================================================
*/

// Defines data for each ambient light the light controller controls:
typedef struct {
	idVec3				origin;
	idVec3				color;			// current color of the light
	idVec3				target_color;	// the target coloer
} light_controller_ambient_t;

// Defines data for each light the light controller uses to control the ambients:
typedef struct {
	idVec3				origin;
	idVec3				color;			// current color of the light
	float				radius;			// average of the radius
} light_controller_light_t;


class CLightController {
public:
	//CLASS_PROTOTYPE( CModelGenerator );

						CLightController( void );

						~CLightController();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	/**
	* Called by gameLocal.
	*/
	void				Init ( void );
	void				Clear ( void );

	/**
	* Register a light with the controller.
	*/
	void				RegisterLight ( void );

	/**
	* De-register a light with the controller.
	*/
	void				UnregisterLight ( void );

	/**
	* Register a local ambient light with the controller.
	*/
	void				RegisterAmbient ( void );

	/**
	* Unregister a local ambient light with the controller.
	*/
	void				UnregisterAmbient ( void );

	/**
	* Update the local ambient lights because the light has changed.
	*/
	void				LightChanged( const int entityNum );

private:
	// Called by the dtor
	void				Shutdown();

private:

	idList< light_controller_ambient_t >	m_Ambients;
	idList< light_controller_light_t >		m_Lights;

	bool				m_bActive;
};

#endif /* !__DARKMOD_LIGHTCONTROLLER_H__ */

