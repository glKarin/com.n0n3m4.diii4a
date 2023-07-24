// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_TRIGGER_H__
#define __GAME_TRIGGER_H__

#include "Entity.h"
#include "roles/RoleManager.h"

extern const idEventDef EV_Enable;
extern const idEventDef EV_Disable;

class sdDeclToolTip;

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

	virtual void		Enable( void );
	virtual void		Disable( void );

protected:
	void				Event_Enable( void );
	void				Event_Disable( void );

protected:
	sdRequirementContainer			requirements;
	const sdProgram::sdFunction*	onTouchFunc;
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

	virtual void		OnTouch( idEntity *other, const trace_t& trace );

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

	virtual bool		WantsTouch( void ) const { return true; }

	bool				CheckFacing( idEntity *activator );
	void				TriggerAction( idEntity *activator );
	void				Event_TriggerAction( idEntity *activator );
	void				Event_Trigger( idEntity *activator );
};


/*
===============================================================================

  Trigger which hurts touching entities.

===============================================================================
*/

typedef sdPair< idEntityPtr<idEntity>, int > entityTriggerTime_t;

class idTrigger_Hurt : public idTrigger {
public:
	CLASS_PROTOTYPE( idTrigger_Hurt );

						idTrigger_Hurt( void );

	void				Spawn( void );

	void				PlayPassSound( void );
	void				PlayFailSound( void );

	void				UpdateTriggerEntities( idEntity *ent );
	bool				HasTriggered( idEntity *ent );

	virtual bool		WantsTouch( void ) const { return on; }
	virtual void		OnTouch( idEntity *other, const trace_t& trace );

private:
	bool				on;
	int					delay;
	const sdDeclDamage*	damageDecl;

	int					nextPassSoundTime;
	int					nextFailSoundTime;

	idList< entityTriggerTime_t >	triggerTimeList;

	void				Event_Toggle( idEntity *activator );
};

#endif /* !__GAME_TRIGGER_H__ */
