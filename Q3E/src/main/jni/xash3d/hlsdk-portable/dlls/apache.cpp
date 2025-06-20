/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
#if !OEM_BUILD

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"

extern DLL_GLOBAL int		g_iSkillLevel;

#define SF_WAITFORTRIGGER	(0x04 | 0x40) // UNDONE: Fix!
#define SF_NOWRECKAGE		0x08

class CApache : public CBaseMonster
{
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int Classify( void ) { return CLASS_HUMAN_MILITARY; };
	int BloodColor( void ) { return DONT_BLEED; }
	void Killed( entvars_t *pevAttacker, int iGib );
	void GibMonster( void );

	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -300.0f, -300.0f, -172.0f );
		pev->absmax = pev->origin + Vector( 300.0f, 300.0f, 8.0f );
	}

	void EXPORT HuntThink( void );
	void EXPORT FlyTouch( CBaseEntity *pOther );
	void EXPORT CrashTouch( CBaseEntity *pOther );
	void EXPORT DyingThink( void );
	void EXPORT StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT NullThink( void );

	void ShowDamage( void );
	void Flight( void );
	void FireRocket( void );
	BOOL FireGun( void );

	int  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );

	int m_iRockets;
	float m_flForce;
	float m_flNextRocket;

	Vector m_vecTarget;
	Vector m_posTarget;

	Vector m_vecDesired;
	Vector m_posDesired;

	Vector m_vecGoal;

	Vector m_angGun;
	float m_flLastSeen;
	float m_flPrevSeen;

	int m_iSoundState; // don't save this

	int m_iSpriteTexture;
	int m_iExplode;
	int m_iBodyGibs;

	float m_flGoalSpeed;

	int m_iDoSmokePuff;
	CBeam *m_pBeam;
};

LINK_ENTITY_TO_CLASS( monster_apache, CApache )

TYPEDESCRIPTION	CApache::m_SaveData[] =
{
	DEFINE_FIELD( CApache, m_iRockets, FIELD_INTEGER ),
	DEFINE_FIELD( CApache, m_flForce, FIELD_FLOAT ),
	DEFINE_FIELD( CApache, m_flNextRocket, FIELD_TIME ),
	DEFINE_FIELD( CApache, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_posTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CApache, m_vecDesired, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_posDesired, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CApache, m_vecGoal, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_angGun, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_flLastSeen, FIELD_TIME ),
	DEFINE_FIELD( CApache, m_flPrevSeen, FIELD_TIME ),
	//DEFINE_FIELD( CApache, m_iSoundState, FIELD_INTEGER ),		// Don't save, precached
	//DEFINE_FIELD( CApache, m_iSpriteTexture, FIELD_INTEGER ),
	//DEFINE_FIELD( CApache, m_iExplode, FIELD_INTEGER ),
	//DEFINE_FIELD( CApache, m_iBodyGibs, FIELD_INTEGER ),
	DEFINE_FIELD( CApache, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CApache, m_flGoalSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CApache, m_iDoSmokePuff, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CApache, CBaseMonster )

void CApache::Spawn( void )
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pev ), "models/apache.mdl" );
	UTIL_SetSize( pev, Vector( -32.0f, -32.0f, -64.0f ), Vector( 32.0f, 32.0f, 0.0f ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;
	pev->health = gSkillData.apacheHealth;

	m_flFieldOfView = -0.707f; // 270 degrees

	pev->sequence = 0;
	ResetSequenceInfo();
	pev->frame = RANDOM_LONG( 0, 0xFF );

	InitBoneControllers();

	if( pev->spawnflags & SF_WAITFORTRIGGER )
	{
		SetUse( &CApache::StartupUse );
	}
	else
	{
		SetThink( &CApache::HuntThink );
		SetTouch( &CApache::FlyTouch );
		pev->nextthink = gpGlobals->time + 1.0f;
	}

	m_iRockets = 10;
}

void CApache::Precache( void )
{
	PRECACHE_MODEL( "models/apache.mdl" );

	PRECACHE_SOUND( "apache/ap_rotor1.wav" );
	PRECACHE_SOUND( "apache/ap_rotor2.wav" );
	PRECACHE_SOUND( "apache/ap_rotor3.wav" );
	PRECACHE_SOUND( "apache/ap_whine1.wav" );

	PRECACHE_SOUND( "weapons/mortarhit.wav" );

	m_iSpriteTexture = PRECACHE_MODEL( "sprites/white.spr" );

	PRECACHE_SOUND( "turret/tu_fire1.wav" );

	PRECACHE_MODEL( "sprites/lgtning.spr" );

	m_iExplode = PRECACHE_MODEL( "sprites/fexplo.spr" );
	m_iBodyGibs = PRECACHE_MODEL( "models/metalplategibs_green.mdl" );

	UTIL_PrecacheOther( "hvr_rocket" );
}

void CApache::NullThink( void )
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.5f;
}

void CApache::StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CApache::HuntThink );
	SetTouch( &CApache::FlyTouch );
	pev->nextthink = gpGlobals->time + 0.1f;
	SetUse( NULL );
}

void CApache::Killed( entvars_t *pevAttacker, int iGib )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3f;

	STOP_SOUND( ENT( pev ), CHAN_STATIC, "apache/ap_rotor2.wav" );

	UTIL_SetSize( pev, Vector( -32.0f, -32.0f, -64.0f ), Vector( 32.0f, 32.0f, 0.0f ) );
	SetThink( &CApache::DyingThink );
	SetTouch( &CApache::CrashTouch );
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;

	if( pev->spawnflags & SF_NOWRECKAGE )
	{
		m_flNextRocket = gpGlobals->time + 4.0f;
	}
	else
	{
		m_flNextRocket = gpGlobals->time + 15.0f;
	}
}

void CApache::DyingThink( void )
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;

	pev->avelocity = pev->avelocity * 1.02f;

	// still falling?
	if( m_flNextRocket > gpGlobals->time )
	{
		// random explosions
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );		// This just makes a dynamic light now
			WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -150.0f, 150.0f ) );
			WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -150.0f, 150.0f ) );
			WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -150.0f, -50.0f ) );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( RANDOM_LONG( 0, 29 ) + 30 ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();

		// lots of smoke
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -150.0f, 150.0f ) );
			WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -150.0f, 150.0f ) );
			WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -150.0f, -50.0f ) );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 100 ); // scale * 10
			WRITE_BYTE( 10 ); // framerate
		MESSAGE_END();

		Vector vecSpot = pev->origin + ( pev->mins + pev->maxs ) * 0.5f;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL );

			// position
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z );

			// size
			WRITE_COORD( 400 );
			WRITE_COORD( 400 );
			WRITE_COORD( 132 );

			// velocity
			WRITE_COORD( pev->velocity.x ); 
			WRITE_COORD( pev->velocity.y );
			WRITE_COORD( pev->velocity.z );

			// randomization
			WRITE_BYTE( 50 ); 

			// Model
			WRITE_SHORT( m_iBodyGibs );	//model id#

			// # of shards
			WRITE_BYTE( 4 );	// let client decide

			// duration
			WRITE_BYTE( 30 );// 3.0 seconds

			// flags
			WRITE_BYTE( BREAK_METAL );
		MESSAGE_END();

		// don't stop it we touch a entity
		pev->flags &= ~FL_ONGROUND;
		pev->nextthink = gpGlobals->time + 0.2f;
		return;
	}
	else
	{
		Vector vecSpot = pev->origin + ( pev->mins + pev->maxs ) * 0.5f;

		/*
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_EXPLOSION);		// This just makes a dynamic light now
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 300.0f );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( 250 ); // scale * 10
			WRITE_BYTE( 8 ); // framerate
		MESSAGE_END();
		*/

		// fireball
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_SPRITE );
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 256.0f );
			WRITE_SHORT( m_iExplode );
			WRITE_BYTE( 120 ); // scale * 10
			WRITE_BYTE( 255 ); // brightness
		MESSAGE_END();

		// big smoke
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 512.0f );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 250 ); // scale * 10
			WRITE_BYTE( 5 ); // framerate
		MESSAGE_END();

		// blast circle
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_BEAMCYLINDER );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z + 2000 ); // reach damage radius over .2 seconds
			WRITE_SHORT( m_iSpriteTexture );
			WRITE_BYTE( 0 ); // startframe
			WRITE_BYTE( 0 ); // framerate
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 32 );  // width
			WRITE_BYTE( 0 );   // noise
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 192 );   // r, g, b
			WRITE_BYTE( 128 ); // brightness
			WRITE_BYTE( 0 );		// speed
		MESSAGE_END();

		EMIT_SOUND( ENT( pev ), CHAN_STATIC, "weapons/mortarhit.wav", 1.0f, 0.3f );

		RadiusDamage( pev->origin, pev, pev, 300, CLASS_NONE, DMG_BLAST );

		if(/*!( pev->spawnflags & SF_NOWRECKAGE ) && */( pev->flags & FL_ONGROUND ) )
		{
			CBaseEntity *pWreckage = Create( "cycler_wreckage", pev->origin, pev->angles );
			// SET_MODEL( ENT( pWreckage->pev ), STRING( pev->model ) );
			UTIL_SetSize( pWreckage->pev, Vector( -200.0f, -200.0f, -128.0f ), Vector( 200.0f, 200.0f, -32.0f ) );
			pWreckage->pev->frame = pev->frame;
			pWreckage->pev->sequence = pev->sequence;
			pWreckage->pev->framerate = 0;
			pWreckage->pev->dmgtime = gpGlobals->time + 5.0f;
		}

		// gibs
		vecSpot = pev->origin + ( pev->mins + pev->maxs ) * 0.5f;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL);

			// position
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 64.0f );

			// size
			WRITE_COORD( 400 );
			WRITE_COORD( 400 );
			WRITE_COORD( 128 );

			// velocity
			WRITE_COORD( 0 );
			WRITE_COORD( 0 );
			WRITE_COORD( 200 );

			// randomization
			WRITE_BYTE( 30 );

			// Model
			WRITE_SHORT( m_iBodyGibs );	//model id#

			// # of shards
			WRITE_BYTE( 200 );

			// duration
			WRITE_BYTE( 200 );// 10.0 seconds

			// flags
			WRITE_BYTE( BREAK_METAL );
		MESSAGE_END();

		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

void CApache::FlyTouch( CBaseEntity *pOther )
{
	// bounce if we hit something solid
	if( pOther->pev->solid == SOLID_BSP )
	{
		TraceResult tr = UTIL_GetGlobalTrace();

		// UNDONE, do a real bounce
		pev->velocity = pev->velocity + tr.vecPlaneNormal * ( pev->velocity.Length() + 200.0f );
	}
}

void CApache::CrashTouch( CBaseEntity *pOther )
{
	// only crash if we hit something solid
	if( pOther->pev->solid == SOLID_BSP )
	{
		SetTouch( NULL );
		m_flNextRocket = gpGlobals->time;
		pev->nextthink = gpGlobals->time;
	}
}

void CApache::GibMonster( void )
{
	// EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "common/bodysplat.wav", 0.75f, ATTN_NORM, 0, 200 );
}

void CApache::HuntThink( void )
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;

	ShowDamage();

	if( m_pGoalEnt == NULL && !FStringNull( pev->target ) )// this monster has a target
	{
		m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
		if( m_pGoalEnt )
		{
			m_posDesired = m_pGoalEnt->pev->origin;
			UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
			m_vecGoal = gpGlobals->v_forward;
		}
	}

	// if( m_hEnemy == NULL )
	{
		Look( 4092 );
		m_hEnemy = BestVisibleEnemy();
	}

	// generic speed up
	if( m_flGoalSpeed < 800.0f )
		m_flGoalSpeed += 5.0f;

	if( m_hEnemy != 0 )
	{
		// ALERT( at_console, "%s\n", STRING( m_hEnemy->pev->classname ) );
		if( FVisible( m_hEnemy ) )
		{
			if( m_flLastSeen < gpGlobals->time - 5.0f )
				m_flPrevSeen = gpGlobals->time;
			m_flLastSeen = gpGlobals->time;
			m_posTarget = m_hEnemy->Center();
		}
		else
		{
			m_hEnemy = NULL;
		}
	}

	m_vecTarget = ( m_posTarget - pev->origin ).Normalize();

	float flLength = ( pev->origin - m_posDesired ).Length();

	if( m_pGoalEnt )
	{
		// ALERT( at_console, "%.0f\n", flLength );

		if( flLength < 128.0f )
		{
			m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( m_pGoalEnt->pev->target ) );
			if( m_pGoalEnt )
			{
				m_posDesired = m_pGoalEnt->pev->origin;
				UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
				m_vecGoal = gpGlobals->v_forward;
				flLength = ( pev->origin - m_posDesired ).Length();
			}
		}
	}
	else
	{
		m_posDesired = pev->origin;
	}

	if( flLength > 250.0f ) // 500
	{
		// float flLength2 = ( m_posTarget - pev->origin ).Length() * ( 1.5f - DotProduct( ( m_posTarget - pev->origin ).Normalize(), pev->velocity.Normalize() ) );
		// if( flLength2 < flLength )
		if( m_flLastSeen + 90.0f > gpGlobals->time && DotProduct( ( m_posTarget - pev->origin ).Normalize(), ( m_posDesired - pev->origin ).Normalize() ) > 0.25f )
		{
			m_vecDesired = ( m_posTarget - pev->origin ).Normalize();
		}
		else
		{
			m_vecDesired = ( m_posDesired - pev->origin ).Normalize();
		}
	}
	else
	{
		m_vecDesired = m_vecGoal;
	}

	Flight();

	// ALERT( at_console, "%.0f %.0f %.0f\n", gpGlobals->time, m_flLastSeen, m_flPrevSeen );
	if( ( m_flLastSeen + 1.0f > gpGlobals->time ) && ( m_flPrevSeen + 2.0f < gpGlobals->time ) )
	{
		if( FireGun() )
		{
			// slow down if we're fireing
			if( m_flGoalSpeed > 400.0f )
				m_flGoalSpeed = 400.0f;
		}

		// don't fire rockets and gun on easy mode
		if( g_iSkillLevel == SKILL_EASY )
			m_flNextRocket = gpGlobals->time + 10.0f;
	}

	UTIL_MakeAimVectors( pev->angles );
	Vector vecEst = ( gpGlobals->v_forward * 800.0f + pev->velocity ).Normalize();
	// ALERT( at_console, "%d %d %d %4.2f\n", pev->angles.x < 0.0f, DotProduct( pev->velocity, gpGlobals->v_forward ) > -100.0f, m_flNextRocket < gpGlobals->time, DotProduct( m_vecTarget, vecEst ) );

	if( ( m_iRockets % 2 ) == 1 )
	{
		FireRocket();
		m_flNextRocket = gpGlobals->time + 0.5f;
		if( m_iRockets <= 0 )
		{
			m_flNextRocket = gpGlobals->time + 10.0f;
			m_iRockets = 10;
		}
	}
	else if( pev->angles.x < 0.0f && DotProduct( pev->velocity, gpGlobals->v_forward ) > -100.0f && m_flNextRocket < gpGlobals->time )
	{
		if( m_flLastSeen + 60.0f > gpGlobals->time )
		{
			if( m_hEnemy != 0 )
			{
				// make sure it's a good shot
				if( DotProduct( m_vecTarget, vecEst ) > .965f )
				{
					TraceResult tr;

					UTIL_TraceLine( pev->origin, pev->origin + vecEst * 4096.0f, ignore_monsters, edict(), &tr );
					if( (tr.vecEndPos - m_posTarget ).Length() < 512.0f )
						FireRocket();
				}
			}
			else
			{
				TraceResult tr;

				UTIL_TraceLine( pev->origin, pev->origin + vecEst * 4096.0f, dont_ignore_monsters, edict(), &tr );
				// just fire when close
				if( ( tr.vecEndPos - m_posTarget ).Length() < 512.0f )
					FireRocket();
			}
		}
	}
}

void CApache::Flight( void )
{
	// tilt model 5 degrees
	Vector vecAdj = Vector( 5.0f, 0.0f, 0.0f );

	// estimate where I'll be facing in one seconds
	UTIL_MakeAimVectors( pev->angles + pev->avelocity * 2.0f + vecAdj );
	// Vector vecEst1 = pev->origin + pev->velocity + gpGlobals->v_up * m_flForce - Vector( 0.0f, 0.0f, 384.0f );
	// float flSide = DotProduct( m_posDesired - vecEst1, gpGlobals->v_right );
	
	float flSide = DotProduct( m_vecDesired, gpGlobals->v_right );

	if( flSide < 0.0f )
	{
		if( pev->avelocity.y < 60.0f )
		{
			pev->avelocity.y += 8.0f; // 9 * ( 3.0 / 2.0 );
		}
	}
	else
	{
		if( pev->avelocity.y > -60.0f )
		{
			pev->avelocity.y -= 8.0f; // 9 * ( 3.0 / 2.0 );
		}
	}
	pev->avelocity.y *= 0.98f;

	// estimate where I'll be in two seconds
	UTIL_MakeAimVectors( pev->angles + pev->avelocity * 1.0f + vecAdj );
	Vector vecEst = pev->origin + pev->velocity * 2.0f + gpGlobals->v_up * m_flForce * 20.0f - Vector( 0.0f, 0.0f, 384.0f * 2.0f );

	// add immediate force
	UTIL_MakeAimVectors( pev->angles + vecAdj );
	pev->velocity.x += gpGlobals->v_up.x * m_flForce;
	pev->velocity.y += gpGlobals->v_up.y * m_flForce;
	pev->velocity.z += gpGlobals->v_up.z * m_flForce;
	// add gravity
	pev->velocity.z -= 38.4f; // 32ft/sec


	float flSpeed = pev->velocity.Length();
	float flDir = DotProduct( Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, 0.0f ), Vector( pev->velocity.x, pev->velocity.y, 0.0f ) );
	if( flDir < 0 )
		flSpeed = -flSpeed;

	float flDist = DotProduct( m_posDesired - vecEst, gpGlobals->v_forward );

	// float flSlip = DotProduct( pev->velocity, gpGlobals->v_right );
	float flSlip = -DotProduct( m_posDesired - vecEst, gpGlobals->v_right );

	// fly sideways
	if( flSlip > 0.0f )
	{
		if( pev->angles.z > -30.0f && pev->avelocity.z > -15.0f )
			pev->avelocity.z -= 4.0f;
		else
			pev->avelocity.z += 2.0f;
	}
	else
	{
		if( pev->angles.z < 30 && pev->avelocity.z < 15.0f )
			pev->avelocity.z += 4.0f;
		else
			pev->avelocity.z -= 2.0f;
	}

	// sideways drag
	pev->velocity.x = pev->velocity.x * ( 1.0f - fabs( gpGlobals->v_right.x ) * 0.05f );
	pev->velocity.y = pev->velocity.y * ( 1.0f - fabs( gpGlobals->v_right.y ) * 0.05f );
	pev->velocity.z = pev->velocity.z * ( 1.0f - fabs( gpGlobals->v_right.z ) * 0.05f );

	// general drag
	pev->velocity = pev->velocity * 0.995f;

	// apply power to stay correct height
	if( m_flForce < 80.0f && vecEst.z < m_posDesired.z )
	{
		m_flForce += 12.0f;
	}
	else if( m_flForce > 30.0f )
	{
		if( vecEst.z > m_posDesired.z )
			m_flForce -= 8.0f;
	}

	// pitch forward or back to get to target
	if( flDist > 0.0f && flSpeed < m_flGoalSpeed /* && flSpeed < flDist */ && pev->angles.x + pev->avelocity.x > -40.0f )
	{
		// ALERT( at_console, "F " );
		// lean forward
		pev->avelocity.x -= 12.0f;
	}
	else if( flDist < 0.0f && flSpeed > -50.0f && pev->angles.x + pev->avelocity.x < 20.0f )
	{
		// ALERT( at_console, "B " );
		// lean backward
		pev->avelocity.x += 12.0f;
	}
	else if( pev->angles.x + pev->avelocity.x > 0.0f )
	{
		// ALERT( at_console, "f " );
		pev->avelocity.x -= 4.0f;
	}
	else if( pev->angles.x + pev->avelocity.x < 0.0f )
	{
		// ALERT( at_console, "b " );
		pev->avelocity.x += 4.0f;
	}

	// ALERT( at_console, "%.0f %.0f : %.0f %.0f : %.0f %.0f : %.0f\n", pev->origin.x, pev->velocity.x, flDist, flSpeed, pev->angles.x, pev->avelocity.x, m_flForce );
	// ALERT( at_console, "%.0f %.0f : %.0f %0.f : %.0f\n", pev->origin.z, pev->velocity.z, vecEst.z, m_posDesired.z, m_flForce );

	// make rotor, engine sounds
	if( m_iSoundState == 0 )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, "apache/ap_rotor2.wav", 1.0f, 0.3f, 0, 110 );
		// EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, "apache/ap_whine1.wav", 0.5f, 0.2f, 0, 110 );

		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions
	}
	else
	{
		CBaseEntity *pPlayer = NULL;

		pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
		// UNDONE: this needs to send different sounds to every player for multiplayer.	
		if( pPlayer )
		{
			float pitch = DotProduct( pev->velocity - pPlayer->pev->velocity, ( pPlayer->pev->origin - pev->origin ).Normalize() );

			pitch = (int)( 100.0f + pitch / 50.0f );

			if( pitch > 250.0f )
				pitch = 250.0f;
			if( pitch < 50.0f )
				pitch = 50.0f;
			if( pitch == 100.0f )
				pitch = 101.0f;

			/*float flVol = ( m_flForce / 100.0f ) + 0.1f;
			if( flVol > 1.0f ) 
				flVol = 1.0f;*/

			EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, "apache/ap_rotor2.wav", 1.0f, 0.3f, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch );
		}
		// EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, "apache/ap_whine1.wav", flVol, 0.2f, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch );
	
		// ALERT( at_console, "%.0f %.2f\n", pitch, flVol );
	}
}

void CApache::FireRocket( void )
{
	static float side = 1.0f;

	if( m_iRockets <= 0 )
		return;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + 1.5f * ( gpGlobals->v_forward * 21.0f + gpGlobals->v_right * 70.0f * side + gpGlobals->v_up * -79.0f );

	switch( m_iRockets % 5 )
	{
	case 0:
		vecSrc = vecSrc + gpGlobals->v_right * 10.0f;
		break;
	case 1:
		vecSrc = vecSrc - gpGlobals->v_right * 10.0f;
		break;
	case 2:
		vecSrc = vecSrc + gpGlobals->v_up * 10.0f;
		break;
	case 3:
		vecSrc = vecSrc - gpGlobals->v_up * 10.0f;
		break;
	case 4:
		break;
	}

	CBaseEntity *pRocket = CBaseEntity::Create( "hvr_rocket", vecSrc, pev->angles, edict() );
	if( pRocket )
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecSrc.x );
			WRITE_COORD( vecSrc.y );
			WRITE_COORD( vecSrc.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 20 ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
		MESSAGE_END();

		pRocket->pev->velocity = pev->velocity + gpGlobals->v_forward * 100.0f;

		m_iRockets--;

		side = - side;
	}
}

BOOL CApache::FireGun()
{
	UTIL_MakeAimVectors( pev->angles );

	Vector posGun, angGun;
	GetAttachment( 1, posGun, angGun );

	Vector vecTarget = ( m_posTarget - posGun ).Normalize();

	Vector vecOut;

	vecOut.x = DotProduct( gpGlobals->v_forward, vecTarget );
	vecOut.y = -DotProduct( gpGlobals->v_right, vecTarget );
	vecOut.z = DotProduct( gpGlobals->v_up, vecTarget );

	Vector angles = UTIL_VecToAngles( vecOut );

	angles.x = -angles.x;
	if( angles.y > 180.0f )
		angles.y = angles.y - 360.0f;
	if( angles.y < -180.0f )
		angles.y = angles.y + 360.0f;
	if( angles.x > 180.0f )
		angles.x = angles.x - 360.0f;
	if( angles.x < -180.0f )
		angles.x = angles.x + 360.0f;

	if( angles.x > m_angGun.x )
		m_angGun.x = Q_min( angles.x, m_angGun.x + 12.0f );
	if( angles.x < m_angGun.x )
		m_angGun.x = Q_max( angles.x, m_angGun.x - 12.0f );
	if( angles.y > m_angGun.y )
		m_angGun.y = Q_min( angles.y, m_angGun.y + 12.0f );
	if( angles.y < m_angGun.y )
		m_angGun.y = Q_max( angles.y, m_angGun.y - 12.0f );

	m_angGun.y = SetBoneController( 0, m_angGun.y );
	m_angGun.x = SetBoneController( 1, m_angGun.x );

	Vector posBarrel, angBarrel;
	GetAttachment( 0, posBarrel, angBarrel );
	Vector vecGun = ( posBarrel - posGun ).Normalize();

	if( DotProduct( vecGun, vecTarget ) > 0.98f )
	{
#if 1
		FireBullets( 1, posGun, vecGun, VECTOR_CONE_4DEGREES, 8192, BULLET_MONSTER_12MM, 1 );
		EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "turret/tu_fire1.wav", 1, 0.3f );
#else
		static float flNext;
		TraceResult tr;
		UTIL_TraceLine( posGun, posGun + vecGun * 8192, dont_ignore_monsters, ENT( pev ), &tr );

		if( !m_pBeam )
		{
			m_pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 80 );
			m_pBeam->PointEntInit( pev->origin, entindex() );
			m_pBeam->SetEndAttachment( 1 );
			m_pBeam->SetColor( 255, 180, 96 );
			m_pBeam->SetBrightness( 192 );
		}

		if( flNext < gpGlobals->time )
		{
			flNext = gpGlobals->time + 0.5f;
			m_pBeam->SetStartPos( tr.vecEndPos );
		}
#endif
		return TRUE;
	}
	else
	{
		if( m_pBeam )
		{
			UTIL_Remove( m_pBeam );
			m_pBeam = NULL;
		}
	}
	return FALSE;
}

void CApache::ShowDamage( void )
{
	if( m_iDoSmokePuff > 0 || RANDOM_LONG( 0, 99 ) > pev->health )
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z - 32 );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( RANDOM_LONG( 0, 9 ) + 20 ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
		MESSAGE_END();
	}
	if( m_iDoSmokePuff > 0 )
		m_iDoSmokePuff--;
}

int CApache::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if( pevInflictor->owner == edict() )
		return 0;

	if( bitsDamageType & DMG_BLAST )
	{
		flDamage *= 2.0f;
	}

	/*
	if( ( bitsDamageType & DMG_BULLET ) && flDamage > 50.0f )
	{
		// clip bullet damage at 50
		flDamage = 50.0f;
	}
	*/

	// ALERT( at_console, "%.0f\n", flDamage );
	return CBaseEntity::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CApache::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	// ALERT( at_console, "%d %.0f\n", ptr->iHitgroup, flDamage );

	// ignore blades
	if( ptr->iHitgroup == 6 && ( bitsDamageType & ( DMG_ENERGYBEAM | DMG_BULLET | DMG_CLUB ) ) )
		return;

	// hit hard, hits cockpit, hits engines
	if( flDamage > 50 || ptr->iHitgroup == 1 || ptr->iHitgroup == 2 )
	{
		// ALERT( at_console, "%.0f\n", flDamage );
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
		m_iDoSmokePuff = 3.0f + ( flDamage / 5.0f );
	}
	else
	{
		// do half damage in the body
		// AddMultiDamage( pevAttacker, this, flDamage / 2.0f, bitsDamageType );
		UTIL_Ricochet( ptr->vecEndPos, 2.0f );
	}
}

class CApacheHVR : public CGrenade
{
	void Spawn( void );
	void Precache( void );
	void EXPORT IgniteThink( void );
	void EXPORT AccelerateThink( void );

	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	int m_iTrail;
	Vector m_vecForward;
};

LINK_ENTITY_TO_CLASS( hvr_rocket, CApacheHVR )

TYPEDESCRIPTION	CApacheHVR::m_SaveData[] =
{
	//DEFINE_FIELD( CApacheHVR, m_iTrail, FIELD_INTEGER ),	// Dont' save, precache
	DEFINE_FIELD( CApacheHVR, m_vecForward, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CApacheHVR, CGrenade )

void CApacheHVR::Spawn( void )
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pev ), "models/HVR.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0), Vector(0, 0, 0) );
	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CApacheHVR::IgniteThink );
	SetTouch( &CGrenade::ExplodeTouch );

	UTIL_MakeAimVectors( pev->angles );
	m_vecForward = gpGlobals->v_forward;
	pev->gravity = 0.5f;

	pev->nextthink = gpGlobals->time + 0.1f;

	pev->dmg = 150;
}

void CApacheHVR::Precache( void )
{
	PRECACHE_MODEL( "models/HVR.mdl" );
	m_iTrail = PRECACHE_MODEL( "sprites/smoke.spr" );
	PRECACHE_SOUND( "weapons/rocket1.wav" );
}

void CApacheHVR::IgniteThink( void )
{
	// pev->movetype = MOVETYPE_TOSS;

	// pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;

	// make rocket sound
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5f );

	// rocket trail
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() ); // entity
		WRITE_SHORT( m_iTrail ); // model
		WRITE_BYTE( 15 ); // life
		WRITE_BYTE( 5 );  // width
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	// set to accelerate
	SetThink( &CApacheHVR::AccelerateThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CApacheHVR::AccelerateThink( void )
{
	// check world boundaries
	if( pev->origin.x < -4096.0f || pev->origin.x > 4096.0f || pev->origin.y < -4096.0f || pev->origin.y > 4096.0f || pev->origin.z < -4096.0f || pev->origin.z > 4096.0f )
	{
		UTIL_Remove( this );
		return;
	}

	// accelerate
	float flSpeed = pev->velocity.Length();
	if( flSpeed < 1800.0f )
	{
		pev->velocity = pev->velocity + m_vecForward * 200.0f;
	}

	// re-aim
	pev->angles = UTIL_VecToAngles( pev->velocity );

	pev->nextthink = gpGlobals->time + 0.1f;
}
#endif
