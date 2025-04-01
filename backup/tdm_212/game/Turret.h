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

#ifndef __GAME_TURRET_H__
#define __GAME_TURRET_H__

/*
===================================================================================

	Turret

===================================================================================
*/

class idTurret : public idEntity {
public:
	CLASS_PROTOTYPE( idTurret );

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	virtual void			Think( void ) override;

private:

	enum
	{
		STATE_PASSIVE,
		STATE_IDLE,
		STATE_SUSPICIOUS,
		STATE_ALERTED,
		STATE_OFF,
		STATE_DEAD
	};

	virtual void			UpdateState( void );
	virtual void			UpdateEnemies( void );
	virtual void			UpdateColors( void );
	bool					CanAttack( idEntity* ent, bool useCurrentAngles );
	int						GetTurretState( void );
	idVec3					GetEnemyPosition( idEntity *ent );
	idAngles				GetAttackAngles( idVec3 enemyPos );
	void					ShootCannon( idVec3 enemyPos );
	void					RoutEnemies( void );
	void					SetPower( bool newState );
	virtual void			Activate( idEntity* activator ) override;
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
								const char *damageDefName, const float damageScale,	const int location, trace_t *tr) override;
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) override;
	void					AddSparks( void );
	void					TriggerSparks( void );

	void					Event_RegisterCams( void );
	void					Event_GetTurretState( void );
	void					Event_Attack( idEntity *e, bool ignoreCollisions );
	void					Event_AttackPosition( idVec3 &targetPos, bool ignoreCollisions );
	void					Event_DisableManualAttack( void );

	int						state;
	bool					intact;
	bool					flinderized;
	bool					m_bPower;
	bool					m_bManualModeEnemy;
	bool					m_bManualModePosition;
	bool					m_bManualModeIgnoreCollisions;

	idList< idEntityPtr<idEntity> >	cams;
	idEntityPtr<idEntity>	enemy;
	idEntityPtr<idEntity>	cameraDisplay;
	idEntityPtr<idEntity>	sparks;
	idVec3					attackPos;
	int						nextAttackCheck;
	int						nextEnemiesUpdate;

	idAngles				m_startAngles;
	idAngles				targetAngles;
	idAngles				currentAngles;
	float					speedVertical;
	float					speedHorizontal;
	int						timeElapsed;
	int						prevTime;

	float					nextSparkTime;
	bool					sparksOn;
	bool					sparksPowerDependent;
	bool					sparksPeriodic;
	float					sparksInterval;
	float					sparksIntervalRand;
};

#endif /* !__GAME_TURRET_H__ */
