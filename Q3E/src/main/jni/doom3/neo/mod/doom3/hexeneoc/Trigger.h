/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __GAME_TRIGGER_H__
#define __GAME_TRIGGER_H__

#include "gamesys/Event.h"
#include "Entity.h"

extern const idEventDef EV_Enable;
extern const idEventDef EV_Disable;

/*
===============================================================================

  Trigger base.

===============================================================================
*/

class idTrigger : public idEntity {
public:
	CLASS_PROTOTYPE( idTrigger );

	static void			DrawDebugInfo( void );

						idTrigger();
	void				Spawn( void );

	const function_t *	GetScriptFunction( void ) const;

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual void		Enable( void );
	virtual void		Disable( void );

protected:
	void				CallScript( void ) const;

	void				Event_Enable( void );
	void				Event_Disable( void );

	// void				check_gravity();
	void				eoc_Check_Automap( idEntity *activator ); // HEXEN : Zeroth

	const function_t *	scriptFunction;
};


/*
===============================================================================

  Trigger which can be activated multiple times.

===============================================================================
*/

class idTrigger_Multi : public idTrigger {
public:
	CLASS_PROTOTYPE( idTrigger_Multi );

						idTrigger_Multi( void );

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

private:
	float				wait;
	float				random;
	float				delay;
	float				random_delay;
	int					nextTriggerTime;
	idStr				requires;
	int					removeItem;
	bool				touchClient;
	bool				touchOther;
	bool				triggerFirst;
	bool				triggerWithSelf;

	bool				CheckFacing( idEntity *activator );
	void				TriggerAction( idEntity *activator );
	void				Event_TriggerAction( idEntity *activator );
	void				Event_Trigger( idEntity *activator );
	void				Event_Touch( idEntity *other, trace_t *trace );
};


/*
===============================================================================

  Trigger which can only be activated by an entity with a specific name.

===============================================================================
*/

class idTrigger_EntityName : public idTrigger {
public:
	CLASS_PROTOTYPE( idTrigger_EntityName );

						idTrigger_EntityName( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );

private:
	float				wait;
	float				random;
	float				delay;
	float				random_delay;
	int					nextTriggerTime;
	bool				triggerFirst;
	idStr				entityName;

	void				TriggerAction( idEntity *activator );
	void				Event_TriggerAction( idEntity *activator );
	void				Event_Trigger( idEntity *activator );
	void				Event_Touch( idEntity *other, trace_t *trace );
};

/*
===============================================================================

  Trigger which repeatedly fires targets.

===============================================================================
*/

class idTrigger_Timer : public idTrigger {
public:
	CLASS_PROTOTYPE( idTrigger_Timer );

						idTrigger_Timer( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );

	virtual void		Enable( void );
	virtual void		Disable( void );

private:
	float				random;
	float				wait;
	bool				on;
	float				delay;
	idStr				onName;
	idStr				offName;

	void				Event_Timer( void );
	void				Event_Use( idEntity *activator );
};


/*
===============================================================================

  Trigger which fires targets after being activated a specific number of times.

===============================================================================
*/

class idTrigger_Count : public idTrigger {
public:
	CLASS_PROTOTYPE( idTrigger_Count );

						idTrigger_Count( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	void				SetCount( int c ) { count = c; };
	void				SetGoal( int c ) { goal = c; };

	void				Spawn( void );

private:
	int					goal;
	int					count;
	float				delay;

	void				Event_Trigger( idEntity *activator );
	void				Event_TriggerAction( idEntity *activator );
};


/*
===============================================================================

  Trigger which hurts touching entities.

===============================================================================
*/

class idTrigger_Hurt : public idTrigger {
public:
	CLASS_PROTOTYPE( idTrigger_Hurt );

						idTrigger_Hurt( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );

private:
	bool				on;
	float				delay;
	idList<idDict>		nextTime;

	void				Event_Touch( idEntity *other, trace_t *trace );
	void				Event_Toggle( idEntity *activator );

	// HEXEN : Zeroth
private:
	bool				dontTripby_LocalPlayer;
	bool				limitEntityType;
	bool				tripby_idPlayer;
	bool				tripby_idAI;
	bool				tripby_idMoveable;
	bool				tripby_idItem;
	bool				tripby_idActor;
	bool				tripby_idProjectile;
	bool				tripby_LocalPlayer;
};


/*
===============================================================================

  Trigger which fades the player view.

===============================================================================
*/

class idTrigger_Fade : public idTrigger {
public:

	CLASS_PROTOTYPE( idTrigger_Fade );

private:
	void				Event_Trigger( idEntity *activator );
};


/*
===============================================================================

  Trigger which continuously tests whether other entities are touching it.

===============================================================================
*/

class idTrigger_Touch : public idTrigger {
public:

	CLASS_PROTOTYPE( idTrigger_Touch );

						idTrigger_Touch( void );

	void				Spawn( void );
	virtual void		Think( void );

	void				Save( idSaveGame *savefile );
	void				Restore( idRestoreGame *savefile );

	virtual void		Enable( void );
	virtual void		Disable( void );

	void				TouchEntities( void );

private:
	idClipModel *		clipModel;

	void				Event_Trigger( idEntity *activator );

// HEXEN : Zeroth - for trigger_enter
private:
	bool				onEntrance;
	idList<idStr>		touchingEntities;
	idList<bool>		flagEntities;
	idList<int>			resistTimeEntities;

// HEXEN : Zeroth - for limit entity types
private:
	bool				dontTripby_LocalPlayer;
	bool				limitEntityType;
	bool				tripby_idPlayer;
	bool				tripby_idAI;
	bool				tripby_idMoveable;
	bool				tripby_idItem;
	bool				tripby_idActor;
	bool				tripby_idProjectile;

// HEXEN : Zeroth - for trigger_hurtmulti
private:
	bool				hurtmulti;

// HEXEN : Zeroth - primarily for trigger_hurtmulti
private:
	float				delay;

// HEXEN : Zeroth - for trigger_hurtmulti
private:
	idList<idDict>		nextTime; // HEXEN : Zeroth. changed to idlist. not really useful anymore.
};

#endif /* !__GAME_TRIGGER_H__ */
