// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_MISC_DEFENCETURRET_H__
#define __GAME_MISC_DEFENCETURRET_H__

#include "../ScriptEntity.h"
#include "../script/ScriptEntityHelpers.h"
#include "../botai/Bot_Common.h"

class sdDefenceTurret : public sdScriptEntity {
	CLASS_PROTOTYPE( sdDefenceTurret );

	enum {
		EVENT_SETTARGET = sdScriptEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

										sdDefenceTurret( void );
	virtual								~sdDefenceTurret( void );

	void								Spawn( void );
	virtual void						PostThink( void );

	void								SetTargetEntity( idEntity* entity );
	
	idEntity *							GetTargetEntity( void ) const;
	float								GetTurretMinRange( void ) const;
	float								GetTurretMaxRange( void ) const;
	bool								IsDeployed( void );
	int									GetTurretType( void ) { return deployableType; }
	idEntity *							GetTurretOwner( void ) const;

	void								UpdateTarget( void );
	void								UpdatePlayerTarget( idPlayer* player );

	bool								ValidateTarget( void ) const;
	bool								InFiringRange( const idVec3& targetPos ) const;

	void								AcquireTarget();
	void								EndAttack( void );
	void								BeginAttack( void );

	void								SetDisabled( bool value );
	bool								IsDisabled( void );
	idVec3								GetTargetPosition( idEntity* entity );

	static void							InitAngleInfo( const char* name, angleClamp_t& info, const sdDeclStringMap* map );

	void								Event_SetDisabled( bool value );
	void								Event_IsDisabled( void );
	void								Event_GetTargetPosition( idEntity* entity );
	void								Event_GetEnemy( void );
	void								Event_SetEnemy( idEntity* other, float turnDelay );

	virtual void						WriteDemoBaseData( idFile* file ) const;
	virtual void						ReadDemoBaseData( idFile* file );

	virtual void						OnPlayerEntered( idPlayer* player, int index );
	virtual void						OnPlayerExited( idPlayer* player, int index );

	virtual bool						ClientReceiveEvent( int event, int time, const idBitMsg& msg );

	virtual void						DamageFeedback( idEntity *victim, idEntity *inflictor, int oldHealth, int newHealth, const sdDeclDamage* damageDecl, bool headshot );

protected:

	struct turretFlags_t {
		bool							disabled	: 1;
		bool							hasTarget	: 1;
		bool							attacking	: 1;
	};

	sdScriptedEntityHelper_Aimer		aimer;

	const sdProgram::sdFunction*		isDeployedFunc;
	const sdProgram::sdFunction*		getOwnerFunc;
	const sdProgram::sdFunction*		validateTargetFunc;
	const sdProgram::sdFunction*		getMinRangeFunc;
	const sdProgram::sdFunction*		getMaxRangeFunc;

	int									deployableType;

	int									acquireWaitTime;
	int									startTurn;
	int									nextTargetAcquireTime;

	idEntityPtr< idEntity >				target;

	turretFlags_t						turretFlags;
};

#endif // __GAME_MISC_DEFENCETURRET_H__
