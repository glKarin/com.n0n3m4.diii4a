/*
===========================================================================

Icarus Starship Command Simulator GPL Source Code
Copyright (C) 2017 Steven Eric Boyette.

This file is part of the Icarus Starship Command Simulator GPL Source Code (?Icarus Starship Command Simulator GPL Source Code?).

Icarus Starship Command Simulator GPL Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Icarus Starship Command Simulator GPL Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Icarus Starship Command Simulator GPL Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef __SBCONSOLE_H__
#define __SBCONSOLE_H__

#include "sbModule.h"

class sbConsole : public idEntity {

public:
			CLASS_PROTOTYPE( sbConsole ); //the necessary idClass prototypes

						sbConsole();

	sbModule*			ControlledModule;

	virtual void		Spawn( void );
	virtual void		Save( idSaveGame *savefile ) const;
	virtual void		Restore( idRestoreGame *savefile );

	sbShip*				ParentShip;

	int					GetCaptainTestNumber(); // boyette mod
	virtual void		RecieveVolley();		// boyette mod

	virtual void		DoStuffAfterAllMapEntitiesHaveSpawned();

	// gui
	virtual bool		HandleSingleGuiCommand( idEntity *entityGui, idLexer *src );

	virtual	void		Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
						// adds a damage effect like overlays, blood, sparks, debris etc.

	virtual	void		SetRenderEntityGuisStrings( const char* varName, const char* value );
	virtual	void		SetRenderEntityGuisBools( const char* varName, bool value );
	virtual	void		SetRenderEntityGuisInts( const char* varName, int value );
	virtual	void		SetRenderEntityGuisFloats( const char* varName, float value );
	virtual	void		HandleNamedEventOnGuis( const char* eventName );

	virtual	void		SetRenderEntityGui0String( const char* varName, const char* value );
	virtual	void		SetRenderEntityGui0Bool( const char* varName, bool value );
	virtual	void		SetRenderEntityGui0Int( const char* varName, int value );
	virtual	void		SetRenderEntityGui0Float( const char* varName, float value );
	virtual	void		HandleNamedEventOnGui0( const char* eventName );

	int					gui_minigame_sequence_counter;

	idAngles			min_view_angles;
	idAngles			max_view_angles;
	void				ReleasePlayerCaptain();

	// BOYETTE NOTE TODO - might be a good idea to get rid of this console_occupied bool - buffing detection might serve the purpose better.
	bool				console_occupied;



protected:
	int					testNumber;


};


#endif /* __GAME_SBCONSOLE_H__ */