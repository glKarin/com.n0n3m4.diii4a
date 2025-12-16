/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#if !OEM_BUILD && !HLDEMO_BUILD

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "game.h"

enum satchel_state
{
	SATCHEL_IDLE = 0,
	SATCHEL_READY,
	SATCHEL_RELOAD
};

enum satchel_e
{
	SATCHEL_IDLE1 = 0,
	SATCHEL_FIDGET1,
	SATCHEL_DRAW,
	SATCHEL_DROP
};

enum satchel_radio_e
{
	SATCHEL_RADIO_IDLE1 = 0,
	SATCHEL_RADIO_FIDGET1,
	SATCHEL_RADIO_DRAW,
	SATCHEL_RADIO_FIRE,
	SATCHEL_RADIO_HOLSTER
};

class CSatchelCharge : public CGrenade
{
	Vector m_lastBounceOrigin;	// Used to fix a bug in engine: when object isn't moving, but its speed isn't 0 and on ground isn't set
	void Spawn( void );
	void Precache( void );
	void BounceSound( void );

	void EXPORT SatchelSlide( CBaseEntity *pOther );
	void EXPORT SatchelThink( void );

public:
	void Deactivate( void );
};

LINK_ENTITY_TO_CLASS( monster_satchel, CSatchelCharge )

//=========================================================
// Deactivate - do whatever it is we do to an orphaned 
// satchel when we don't want it in the world anymore.
//=========================================================
void CSatchelCharge::Deactivate( void )
{
	pev->solid = SOLID_NOT;
	UTIL_Remove( this );
}

void CSatchelCharge::Spawn( void )
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pev ), "models/w_satchel.mdl" );
	//UTIL_SetSize( pev, Vector( -16, -16, -4 ), Vector( 16, 16, 32 ) );	// Old box -- size of headcrab monsters/players get blocked by this
	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );	// Uses point-sized, and can be stepped over
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CSatchelCharge::SatchelSlide );
	SetUse( &CGrenade::DetonateUse );
	SetThink( &CSatchelCharge::SatchelThink );
	pev->nextthink = gpGlobals->time + 0.1f;

	pev->gravity = 0.5f;
	pev->friction = 0.8f;

	pev->dmg = gSkillData.plrDmgSatchel;
	// ResetSequenceInfo();
	pev->sequence = 1;
}

void CSatchelCharge::SatchelSlide( CBaseEntity *pOther )
{
	//entvars_t *pevOther = pOther->pev;

	// don't hit the guy that launched this grenade
	if( pOther->edict() == pev->owner )
		return;

	// pev->avelocity = Vector( 300, 300, 300 );
	pev->gravity = 1;// normal gravity now

	// HACKHACK - On ground isn't always set, so look for ground underneath
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 10 ), ignore_monsters, edict(), &tr );

	if( tr.flFraction < 1.0f )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95;
		pev->avelocity = pev->avelocity * 0.9;
		// play sliding sound, volume based on velocity
	}
	if( !( pev->flags & FL_ONGROUND ) && pev->velocity.Length2D() > 10.0f )
	{
		// Fix for a bug in engine: when object isn't moving, but its speed isn't 0 and on ground isn't set
		if( pev->origin != m_lastBounceOrigin )
		BounceSound();
	}
	m_lastBounceOrigin = pev->origin;
	// There is no model animation so commented this out to prevent net traffic
	// StudioFrameAdvance();
}

void CSatchelCharge::SatchelThink( void )
{
	// There is no model animation so commented this out to prevent net traffic
	// StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;

	if( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	if( pev->waterlevel == 3 )
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = pev->velocity * 0.8f;
		pev->avelocity = pev->avelocity * 0.9f;
		pev->velocity.z += 8;
	}
	else if( pev->waterlevel == 0 )
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}
	else
	{
		pev->velocity.z -= 8.0f;
	}	
}

void CSatchelCharge::Precache( void )
{
	PRECACHE_MODEL( "models/w_satchel.mdl" );
	PRECACHE_SOUND( "weapons/g_bounce1.wav" );
	PRECACHE_SOUND( "weapons/g_bounce2.wav" );
	PRECACHE_SOUND( "weapons/g_bounce3.wav" );
}

void CSatchelCharge::BounceSound( void )
{
	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/g_bounce1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/g_bounce2.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/g_bounce3.wav", 1, ATTN_NORM );
		break;
	}
}

LINK_ENTITY_TO_CLASS( weapon_satchel, CSatchel )

//=========================================================
// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
//=========================================================
int CSatchel::AddDuplicate( CBasePlayerItem *pOriginal )
{
#if !CLIENT_DLL
	CSatchel *pSatchel;
	int nNumSatchels, nSatchelsInPocket;
	CBaseEntity *ent;

	if( g_pGameRules->IsMultiplayer() )
	{
		if( satchelfix.value )
		{
			if( !pOriginal->m_pPlayer )
				return TRUE;

			nNumSatchels = 0;
			nSatchelsInPocket = pOriginal->m_pPlayer->m_rgAmmo[pOriginal->PrimaryAmmoIndex()];
			ent = NULL;

			while( ( ent = UTIL_FindEntityInSphere( ent, pOriginal->m_pPlayer->pev->origin, 4096 )) != NULL )
			{
				if( FClassnameIs( ent->pev, "monster_satchel" ))
					nNumSatchels += ent->pev->owner == pOriginal->m_pPlayer->edict();
			}
		}

		pSatchel = (CSatchel *)pOriginal;

		if( pSatchel->m_chargeReady != SATCHEL_IDLE
		    && ( satchelfix.value && nSatchelsInPocket + nNumSatchels > SATCHEL_MAX_CARRY - 1 ))
		{
			// player has some satchels deployed. Refuse to add more.
			return FALSE;
		}
	}
#endif
	return CBasePlayerWeapon::AddDuplicate( pOriginal );
}

//=========================================================
//=========================================================
int CSatchel::AddToPlayer( CBasePlayer *pPlayer )
{
	int bResult = CBasePlayerItem::AddToPlayer( pPlayer );

	pPlayer->pev->weapons |= ( 1 << m_iId );
	m_chargeReady = SATCHEL_IDLE;// this satchel charge weapon now forgets that any satchels are deployed by it.

	if( bResult )
	{
		return AddWeapon();
	}
	return FALSE;
}

void CSatchel::Spawn()
{
	Precache();
	m_iId = WEAPON_SATCHEL;
	SET_MODEL( ENT( pev ), "models/w_satchel.mdl" );

	m_iDefaultAmmo = SATCHEL_DEFAULT_GIVE;
		
	FallInit();// get ready to fall down.
}

void CSatchel::Precache( void )
{
	PRECACHE_MODEL( "models/v_satchel.mdl" );
	PRECACHE_MODEL( "models/v_satchel_radio.mdl" );
	PRECACHE_MODEL( "models/w_satchel.mdl" );
	PRECACHE_MODEL( "models/p_satchel.mdl" );
	PRECACHE_MODEL( "models/p_satchel_radio.mdl" );

	UTIL_PrecacheOther( "monster_satchel" );
}

int CSatchel::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "Satchel Charge";
	p->iMaxAmmo1 = SATCHEL_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 1;
	p->iFlags = ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->iId = m_iId = WEAPON_SATCHEL;
	p->iWeight = SATCHEL_WEIGHT;

	return 1;
}

//=========================================================
//=========================================================
BOOL CSatchel::IsUseable( void )
{
	return CanDeploy();
}

BOOL CSatchel::CanDeploy( void )
{
	if( m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0 ) 
	{
		// player is carrying some satchels
		return TRUE;
	}

	if( m_chargeReady )
	{
		// player isn't carrying any satchels, but has some out
		return TRUE;
	}

	return FALSE;
}

BOOL CSatchel::Deploy()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
	BOOL result;

	if( m_chargeReady )
		result = DefaultDeploy( "models/v_satchel_radio.mdl", "models/p_satchel_radio.mdl", SATCHEL_RADIO_DRAW, "hive" );
	else
		result = DefaultDeploy( "models/v_satchel.mdl", "models/p_satchel.mdl", SATCHEL_DRAW, "trip" );
	
#if WEAPONS_ANIMATION_TIMES_FIX
	if ( result )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
	}
#endif
	return result;
}

void CSatchel::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;

	if( m_chargeReady )
	{
		SendWeaponAnim( SATCHEL_RADIO_HOLSTER );
	}
	else
	{
		SendWeaponAnim( SATCHEL_DROP );
	}
	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "common/null.wav", 1.0f, ATTN_NORM );

	if( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] && m_chargeReady != SATCHEL_READY )
	{
		m_pPlayer->pev->weapons &= ~( 1 << WEAPON_SATCHEL );
		DestroyItem();
	}
}

void CSatchel::PrimaryAttack( void )
{
#if SATCHEL_OLD_BEHAVIOUR
	switch( m_chargeReady )
	{
	case SATCHEL_IDLE:
		{
			Throw();
		}
		break;
	case SATCHEL_READY:
		{
			SendWeaponAnim( SATCHEL_RADIO_FIRE );

			edict_t *pPlayer = m_pPlayer->edict();

			CBaseEntity *pSatchel = NULL;

			while( ( pSatchel = UTIL_FindEntityInSphere( pSatchel, m_pPlayer->pev->origin, 4096 )) != NULL )
			{
				if( FClassnameIs( pSatchel->pev, "monster_satchel" ))
				{
					if( pSatchel->pev->owner == pPlayer )
					{
						pSatchel->Use( m_pPlayer, m_pPlayer, USE_ON, 0 );
					}
				}
			}

			m_chargeReady = SATCHEL_RELOAD;
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.5f );
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
			break;
		}
	case SATCHEL_RELOAD:
		// we're reloading, don't allow fire
		break;
	}
#else
	if( m_chargeReady != SATCHEL_RELOAD )
	{
		Throw();
	}
#endif
}

void CSatchel::SecondaryAttack( void )
{
#if SATCHEL_OLD_BEHAVIOUR
	if( m_chargeReady != SATCHEL_RELOAD )
	{
		Throw();
	}
#else
	switch( m_chargeReady )
	{
	case SATCHEL_IDLE:
		break;
	case SATCHEL_READY:
		{
			SendWeaponAnim( SATCHEL_RADIO_FIRE );

			edict_t *pPlayer = m_pPlayer->edict();

			CBaseEntity *pSatchel = NULL;

			while( ( pSatchel = UTIL_FindEntityInSphere( pSatchel, m_pPlayer->pev->origin, 4096 )) != NULL )
			{
				if( FClassnameIs( pSatchel->pev, "monster_satchel" ))
				{
					if( pSatchel->pev->owner == pPlayer )
					{
						pSatchel->Use( m_pPlayer, m_pPlayer, USE_ON, 0 );
					}
				}
			}

			m_chargeReady = SATCHEL_RELOAD;
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.5f );
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
			break;
		}
	case SATCHEL_RELOAD:
		// we're reloading, don't allow fire
		break;
	}
#endif
}

void CSatchel::Throw( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
#if !CLIENT_DLL
		Vector vecSrc = m_pPlayer->pev->origin;

		Vector vecThrow = gpGlobals->v_forward * 274 + m_pPlayer->pev->velocity;

		CBaseEntity *pSatchel = Create( "monster_satchel", vecSrc, Vector( 0, 0, 0 ), m_pPlayer->edict() );
		pSatchel->pev->velocity = vecThrow;
		pSatchel->pev->avelocity.y = 400;

		m_pPlayer->pev->viewmodel = MAKE_STRING( "models/v_satchel_radio.mdl" );
		m_pPlayer->pev->weaponmodel = MAKE_STRING( "models/p_satchel_radio.mdl" );
#else
		LoadVModel( "models/v_satchel_radio.mdl", m_pPlayer );
#endif

		SendWeaponAnim( SATCHEL_RADIO_DRAW );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_chargeReady = SATCHEL_READY;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		m_flNextPrimaryAttack = GetNextAttackDelay( 1.0f );
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	}
}

void CSatchel::WeaponIdle( void )
{
	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	switch( m_chargeReady )
	{
	case SATCHEL_IDLE:
		SendWeaponAnim( SATCHEL_FIDGET1 );
		// use tripmine animations
		strcpy( m_pPlayer->m_szAnimExtention, "trip" );
		break;
	case SATCHEL_READY:
		SendWeaponAnim( SATCHEL_RADIO_FIDGET1 );
		// use hivehand animations
		strcpy( m_pPlayer->m_szAnimExtention, "hive" );
		break;
	case SATCHEL_RELOAD:
		if( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			m_chargeReady = 0;
			RetireWeapon();
			return;
		}

#if !CLIENT_DLL
		m_pPlayer->pev->viewmodel = MAKE_STRING( "models/v_satchel.mdl" );
		m_pPlayer->pev->weaponmodel = MAKE_STRING( "models/p_satchel.mdl" );
#else
		LoadVModel( "models/v_satchel.mdl", m_pPlayer );
#endif
		SendWeaponAnim( SATCHEL_DRAW );

		// use tripmine animations
		strcpy( m_pPlayer->m_szAnimExtention, "trip" );

		m_flNextPrimaryAttack = GetNextAttackDelay( 0.5f );
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		m_chargeReady = SATCHEL_IDLE;
		break;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );// how long till we do this again.
}

//=========================================================
// DeactivateSatchels - removes all satchels owned by
// the provided player. Should only be used upon death.
//
// Made this global on purpose.
//=========================================================
void DeactivateSatchels( CBasePlayer *pOwner )
{
	edict_t *pFind; 

	pFind = FIND_ENTITY_BY_CLASSNAME( NULL, "monster_satchel" );

	while( !FNullEnt( pFind ) )
	{
		CBaseEntity *pEnt = CBaseEntity::Instance( pFind );
		CSatchelCharge *pSatchel = (CSatchelCharge *)pEnt;

		if( pSatchel )
		{
			if( pSatchel->pev->owner == pOwner->edict() )
			{
				pSatchel->Deactivate();
			}
		}

		pFind = FIND_ENTITY_BY_CLASSNAME( pFind, "monster_satchel" );
	}
}
#endif
