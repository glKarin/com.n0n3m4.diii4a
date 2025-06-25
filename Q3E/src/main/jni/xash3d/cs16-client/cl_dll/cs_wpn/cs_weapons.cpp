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

#include "port.h"

#include "wrect.h"
#include "cl_dll.h"
#include "cl_util.h"
#include "hud.h"

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"

#define PLAYER_H
#include "weapons.h"
#undef PLAYER_H

#include "nodes.h"
#include "player.h"

#include "usercmd.h"
#include "entity_state.h"
#include "demo_api.h"
#include "pm_defs.h"
#include "event_api.h"
#include "r_efx.h"

#include "hud_iface.h"
#include "com_weapons.h"
#include "demo.h"

#include "cl_entity.h"

#include "pm_shared.h"
#include "GameStudioModelRenderer.h"
#include "ammohistory.h"
#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

// Globals used by game logic
const Vector g_vecZero = Vector( 0, 0, 0 );
enginefuncs_t g_engfuncs;
globalvars_t  *gpGlobals;
ItemInfo CBasePlayerItem::ItemInfoArray[MAX_WEAPONS];

// Pool of client side entities/entvars_t
static entvars_t	ev[ 32 ];
static int			num_ents = 0;

// The entity we'll use to represent the local client
static CBasePlayer	player;

// Local version of game .dll global variables ( time, etc. )
static globalvars_t	Globals = { };

static CBasePlayerWeapon *g_pWpns[ 32 ];


// CS Weapon placeholder entities
static CAK47 g_AK47;
static CAUG g_AUG;
static CAWP g_AWP;
static CC4 g_C4;
static CDEAGLE g_DEAGLE;
static CELITE g_ELITE;
static CFamas g_Famas;
static CFiveSeven g_FiveSeven;
static CFlashbang g_Flashbang;
static CG3SG1 g_G3SG1;
static CGalil g_Galil;
static CGLOCK18 g_GLOCK18;
static CHEGrenade g_HEGrenade;
static CKnife g_Knife;
static CM249 g_M249;
static CM3 g_M3;
static CM4A1 g_M4A1;
static CMAC10 g_MAC10;
static CMP5N g_MP5N;
static CP228 g_P228;
static CP90 g_P90;
static CSCOUT g_SCOUT;
static CSG550 g_SG550;
static CSG552 g_SG552;
static CSmokeGrenade g_SmokeGrenade;
static CTMP g_TMP;
static CUMP45 g_UMP45;
static CUSP g_USP;
static CXM1014 g_XM1014;

int    g_iWeaponFlags;
bool   g_bInBombZone;
int    g_iFreezeTimeOver;
bool   g_bHoldingShield;
bool   g_bHoldingKnife;
float  g_flPlayerSpeed;
int    g_iPlayerFlags;
Vector g_vPlayerVelocity;
int    g_iWaterLevel;

/*
======================
AlertMessage

Print debug messages to console
======================
*/
void AlertMessage( ALERT_TYPE atype, const char *szFmt, ... )
{
	va_list		argptr;
	static char	string[1024];

	va_start (argptr, szFmt);
	vsprintf (string, szFmt,argptr);
	va_end (argptr);

	gEngfuncs.Con_Printf( "cl:  " );
	gEngfuncs.Con_Printf( string );
}

/*
=====================
HUD_PrepEntity

Links the raw entity to an entvars_s holder.  If a player is passed in as the owner, then
we set up the m_pPlayer field.
=====================
*/
edict_t *EHANDLE::Get(void)
{
	if (!m_pent)
		return NULL;

	if (m_pent->serialnumber != m_serialnumber)
		return NULL;

	return m_pent;
}

edict_t *EHANDLE::Set(edict_t *pent)
{
	m_pent = pent;

	if (pent)
		m_serialnumber = m_pent->serialnumber;

	return pent;
}

EHANDLE::operator CBaseEntity *(void)
{
	return (CBaseEntity *)GET_PRIVATE(Get());
}

CBaseEntity *EHANDLE::operator = (CBaseEntity *pEntity)
{
	if (pEntity)
	{
		m_pent = ENT(pEntity->pev);

		if (m_pent)
			m_serialnumber = m_pent->serialnumber;
	}
	else
	{
		m_pent = NULL;
		m_serialnumber = 0;
	}

	return pEntity;
}

EHANDLE::operator int(void)
{
	return Get() != NULL;
}

CBaseEntity *EHANDLE::operator ->(void)
{
	return (CBaseEntity *)GET_PRIVATE(Get());
}

void HUD_PrepEntity( CBaseEntity *pEntity )
{
	memset( &ev[ num_ents ], 0, sizeof( entvars_t ) );
	pEntity->pev = &ev[ num_ents++ ];

	pEntity->Precache();
	pEntity->Spawn();
}


void HUD_PrepEntity( CBasePlayerWeapon *pEntity, CBasePlayer *pWeaponOwner )
{
	ItemInfo info = {};

	memset( &ev[ num_ents ], 0, sizeof( entvars_t ) );
	pEntity->pev = &ev[ num_ents++ ];

	pEntity->Precache();
	pEntity->Spawn();
	pEntity->m_pPlayer = pWeaponOwner;
	pEntity->GetItemInfo( &info );

	g_pWpns[ info.iId ] = pEntity;
	CBasePlayerItem::ItemInfoArray[ info.iId ] = info;
}

/*
=====================
CBaseEntity :: Killed

If weapons code "kills" an entity, just set its effects to EF_NODRAW
=====================
*/
void CBaseEntity :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->effects |= EF_NODRAW;
}

/*
=====================
CBasePlayerWeapon :: DefaultReload
=====================
*/
BOOL CBasePlayerWeapon :: DefaultReload( int iClipSize, int iAnim, float fDelay, int body )
{
	if( !m_pPlayer->m_pActiveItem )
		return FALSE;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return FALSE;

	int j = min(iClipSize - m_iClip, player.m_rgAmmo[m_iPrimaryAmmoType]);

	if (j == 0)
		return FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + fDelay;

	//!!UNDONE -- reload sound goes here !!!
	SendWeaponAnim( iAnim, UseDecrement() );

	m_fInReload = TRUE;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + fDelay + 0.5f;
	return TRUE;
}

/*
=====================
CBasePlayerWeapon :: CanDeploy
=====================
*/
BOOL CBasePlayerWeapon :: CanDeploy( void )
{
	return TRUE;
}
/*
=====================
CBasePlayer :: HasShield

=====================
*/
bool CBasePlayer::HasShield()
{
	return g_bHoldingShield;
}

/*
=====================
CBasePlayerWeapon::HasSecondaryAttack()

=====================
*/
bool CBasePlayerWeapon::HasSecondaryAttack()
{
	if (m_pPlayer->HasShield())
	{
		return true;
	}

	switch (m_iId)
	{
	case WEAPON_AK47:
	case WEAPON_XM1014:
	case WEAPON_MAC10:
	case WEAPON_ELITE:
	case WEAPON_FIVESEVEN:
	case WEAPON_MP5N:
	case WEAPON_M249:
	case WEAPON_M3:
	case WEAPON_TMP:
	case WEAPON_DEAGLE:
	case WEAPON_P228:
	case WEAPON_P90:
	case WEAPON_C4:
	case WEAPON_GALIL:
		return false;
	default:
		break;
	}

	return true;
}

void CBasePlayerWeapon::FireRemaining(int &shotsFired, float &shootTime, BOOL isGlock18)
{
	m_iClip--;

	if (m_iClip < 0)
	{
		m_iClip = 0;
		shotsFired = 3;
		shootTime = 0;
		return;
	}

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecDir;

	if (isGlock18)
	{
		vecDir = m_pPlayer->FireBullets3(vecSrc, gpGlobals->v_forward, 0.05, 8192, 1, BULLET_PLAYER_9MM, 18, 0.9, m_pPlayer->pev, TRUE, m_pPlayer->random_seed);
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, ENT(m_pPlayer->pev), m_usFireGlock18, 0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, (int)(m_pPlayer->pev->punchangle.x * 10000), (int)(m_pPlayer->pev->punchangle.y * 10000), m_iClip == 0, FALSE);
		m_pPlayer->ammo_9mm--;
	}
	else
	{
		vecDir = m_pPlayer->FireBullets3(vecSrc, gpGlobals->v_forward, m_fBurstSpread, 8192, 2, BULLET_PLAYER_556MM, 30, 0.96, m_pPlayer->pev, TRUE, m_pPlayer->random_seed);
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, ENT(m_pPlayer->pev), m_usFireFamas, 0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, (int)(m_pPlayer->pev->punchangle.x * 10000000), (int)(m_pPlayer->pev->punchangle.y * 10000000), FALSE, FALSE);
		m_pPlayer->ammo_556nato--;
	}

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;

	if (++shotsFired != 3)
		shootTime = gpGlobals->time + 0.1;
	else
		shootTime = 0;
}

bool CBasePlayerWeapon::ShieldSecondaryFire(int iUpAnim, int iDownAnim)
{
	if (!m_pPlayer->HasShield())
		return false;

	if (m_iWeaponState & WPNSTATE_SHIELD_DRAWN)
	{
		m_iWeaponState &= ~WPNSTATE_SHIELD_DRAWN;
		SendWeaponAnim(iDownAnim, UseDecrement() != FALSE);
		strcpy(m_pPlayer->m_szAnimExtention, "shieldgun");
		m_fMaxSpeed = 250.0f;
		m_pPlayer->m_bShieldDrawn = false;
	}
	else
	{
		m_iWeaponState |= WPNSTATE_SHIELD_DRAWN;
		SendWeaponAnim(iUpAnim, UseDecrement() != FALSE);
		strcpy(m_pPlayer->m_szAnimExtention, "shielded");
		m_fMaxSpeed = 180.0f;
		m_pPlayer->m_bShieldDrawn = true;
	}

	m_flNextSecondaryAttack = 0.4f;
	m_flNextPrimaryAttack = 0.4f;
	m_flTimeWeaponIdle = 0.6f;

	return true;
}

void CBasePlayerWeapon::KickBack(float up_base, float lateral_base, float up_modifier, float lateral_modifier, float up_max, float lateral_max, int direction_change)
{

}

void CBasePlayerWeapon::SetPlayerShieldAnim(void)
{
	if (!m_pPlayer->HasShield())
		return;

	if (m_iWeaponState & WPNSTATE_SHIELD_DRAWN)
	{
		strcpy(m_pPlayer->m_szAnimExtention, "shield");
	}
	else
	{
		strcpy(m_pPlayer->m_szAnimExtention, "shieldgun");
	}
}

void CBasePlayerWeapon::ResetPlayerShieldAnim(void)
{
	if (m_pPlayer->HasShield())
	{
		if (m_iWeaponState & WPNSTATE_SHIELD_DRAWN)
		{
			strcpy(m_pPlayer->m_szAnimExtention, "shieldgun");
		}
	}
}

/*
=====================
CBasePlayerWeapon :: DefaultDeploy

=====================
*/
BOOL CBasePlayerWeapon :: DefaultDeploy( const char *szViewModel, const char *szWeaponModel, int iAnim, const char *szAnimExt, int skiplocal )
{
	if ( !CanDeploy() )
		return FALSE;

	gEngfuncs.CL_LoadModel( szViewModel, &m_pPlayer->pev->viewmodel );

	SendWeaponAnim( iAnim, skiplocal );

	m_pPlayer->m_flNextAttack = 0.75f;
	m_flTimeWeaponIdle = 1.5f;
	return TRUE;
}

/*
=====================
CBasePlayerWeapon :: PlayEmptySound

=====================
*/
BOOL CBasePlayerWeapon :: PlayEmptySound( void )
{
#if 0
	if (m_iPlayEmptySound)
	{
		switch (m_iId)
		{
		case WEAPON_USP:
		case WEAPON_GLOCK18:
		case WEAPON_P228:
		case WEAPON_DEAGLE:
		case WEAPON_ELITE:
		case WEAPON_FIVESEVEN:
			HUD_PlaySound("weapons/dryfire_pistol.wav", 0.8);
			break;
		default:
			HUD_PlaySound("weapons/dryfire_rifle.wav",  0.8);
			break;
		}
	}
#endif
	return 0;
}

/*
=====================
CBasePlayerWeapon :: ResetEmptySound

=====================
*/
void CBasePlayerWeapon :: ResetEmptySound( void )
{
	m_iPlayEmptySound = 1;
}

/*
=====================
CBasePlayerWeapon::Holster

Put away weapon
=====================
*/
void CBasePlayerWeapon::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE; // cancel any reload in progress.
	m_pPlayer->pev->viewmodel = 0;
}

/*
=====================
CBasePlayerWeapon::SendWeaponAnim

Animate weapon model
=====================
*/
void CBasePlayerWeapon::SendWeaponAnim( int iAnim, int skiplocal )
{
	//gEngfuncs.Con_DPrintf( "Predict::SendWeaponAnim( %i )\n", iAnim );

	m_pPlayer->pev->weaponanim = iAnim;
	HUD_SendWeaponAnim( iAnim, m_iId, 0, 0 );
}

void CBasePlayerWeapon::RetireWeapon()
{
	// TODO: Implement
	// UTIL_GetNextBestWeapon( m_pPlayer, this );
}

Vector CBaseEntity::FireBullets3 ( Vector vecSrc, Vector vecDirShooting, float flSpread, float flDistance, int iPenetration, int iBulletType, int iDamage, float flRangeModifier, entvars_t *pevAttacker, bool bPistol, int shared_rand )
{
	float x, y, z;

	if ( pevAttacker )
	{
		x = UTIL_SharedRandomFloat(shared_rand, -0.5, 0.5) + UTIL_SharedRandomFloat(shared_rand + 1, -0.5, 0.5);
		y = UTIL_SharedRandomFloat(shared_rand + 2, -0.5, 0.5) + UTIL_SharedRandomFloat(shared_rand + 3, -0.5, 0.5);
	}
	else
	{
		do
		{
			x = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
			y = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
			z = x * x + y * y;
		}
		while (z > 1);
	}

	return Vector(x * flSpread, y * flSpread, 0.0f);
}
/*
=====================
CBasePlayerWeapon::ItemPostFrame

Handles weapon firing, reloading, etc.
=====================
*/
void CBasePlayerWeapon::ItemPostFrame( void )
{
	int button = m_pPlayer->pev->button;

	if (!HasSecondaryAttack())
		button &= ~IN_ATTACK2;

	if (m_flGlock18Shoot != 0)
	{
		m_iClip--;
		if( m_iClip < 0 )
		{
			m_iClip = m_iGlock18ShotsFired = 0;
		}
		FireRemaining(m_iGlock18ShotsFired, m_flGlock18Shoot, TRUE);
	}
	else if (gpGlobals->time > m_flFamasShoot && m_flFamasShoot != 0)
	{
		m_iClip--;
		if( m_iClip < 0 )
		{
			m_iClip = m_iFamasShotsFired = 0;
		}
		FireRemaining(m_iFamasShotsFired, m_flFamasShoot, FALSE);
	}

	if (m_flNextPrimaryAttack <= UTIL_WeaponTimeBase() )
	{
		if (m_pPlayer->m_bResumeZoom)
		{
			m_pPlayer->pev->fov = m_pPlayer->m_iFOV = m_pPlayer->m_iLastZoom;

			if (m_pPlayer->m_iFOV == m_pPlayer->m_iLastZoom)
			{
				m_pPlayer->m_bResumeZoom = false;
				// viewmodel hide is implemented elsewhere
			}
		}
	}

	if ( m_pPlayer->HasShield() )
	{
		if (m_fInReload && m_pPlayer->pev->button & IN_ATTACK2)
		{
			SecondaryAttack();
			m_pPlayer->pev->button &= ~IN_ATTACK2;
			m_fInReload = FALSE;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase();
		}
	}

	if ((m_fInReload) && m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase())
	{
		int j = min(iMaxClip() - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);

		m_iClip += j;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j;
		m_fInReload = FALSE;
	}

	if ((button & IN_ATTACK2) && m_flNextSecondaryAttack <= UTIL_WeaponTimeBase())
	{
		if (pszAmmo2() && !m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType])
			m_fFireOnEmpty = TRUE;

		SecondaryAttack();
		m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	else if ((m_pPlayer->pev->button & IN_ATTACK) && m_flNextPrimaryAttack <= UTIL_WeaponTimeBase())
	{
		if ((!m_iClip && pszAmmo1()) || (iMaxClip() == WEAPON_NOCLIP && !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]))
			m_fFireOnEmpty = TRUE;

		if (m_pPlayer->m_bCanShoot == true)
			PrimaryAttack();
	}
	else if (m_pPlayer->pev->button & IN_RELOAD && iMaxClip() != WEAPON_NOCLIP && !m_fInReload)
	{
		if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		{
			if (m_flFamasShoot == 0 && m_flGlock18Shoot == 0)
			{
				if (!(m_iWeaponState & WPNSTATE_SHIELD_DRAWN))
					Reload();
			}
		}
	}
	else if (!(button & (IN_ATTACK | IN_ATTACK2)))
	{
		if (m_bDelayFire == true)
		{
			m_bDelayFire = false;

			if (m_iShotsFired > 15)
				m_iShotsFired = 15;

			m_flDecreaseShotsFired = gpGlobals->time + 0.4;
		}

		m_fFireOnEmpty = FALSE;

		if (m_iId != WEAPON_USP && m_iId != WEAPON_GLOCK18 && m_iId != WEAPON_P228 && m_iId != WEAPON_DEAGLE && m_iId != WEAPON_ELITE && m_iId != WEAPON_FIVESEVEN)
		{
			if (m_iShotsFired > 0)
			{
				if (gpGlobals->time > m_flDecreaseShotsFired)
				{
					m_iShotsFired--;
					m_flDecreaseShotsFired = gpGlobals->time + 0.0225;
				}
			}
		}
		else
			m_iShotsFired = 0;


		if (!(m_iWeaponState & WPNSTATE_SHIELD_DRAWN))
		{

			if (m_iClip == 0 && !(iFlags() & ITEM_FLAG_NOAUTORELOAD)
					&& m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
			{
				if (m_flFamasShoot == 0 && m_flGlock18Shoot == 0)
				{
					Reload();
					return;
				}
			}
		}

		WeaponIdle();
		return;
	}
}

float CBasePlayerWeapon::GetNextAttackDelay(float delay)
{
	float flNextAttack = UTIL_WeaponTimeBase() + delay;
	return flNextAttack;
}

/*
=====================
CBasePlayer::SelectLastItem

=====================
*/
void CBasePlayer::SelectLastItem(void)
{
	if (!m_pLastItem)
	{
		return;
	}

	if ( m_pActiveItem && !m_pActiveItem->CanHolster() )
	{
		return;
	}

	if (m_pActiveItem)
		m_pActiveItem->Holster( );

	CBasePlayerItem *pTemp = m_pActiveItem;
	m_pActiveItem = m_pLastItem;
	m_pLastItem = pTemp;
	m_pActiveItem->Deploy( );
}

/*
=====================
CBasePlayer::Killed

=====================
*/
void CBasePlayer::Killed( entvars_t *pevAttacker, int iGib )
{
	// Holster weapon immediately, to allow it to cleanup
	if ( m_pActiveItem )
		 m_pActiveItem->Holster( );
}

/*
=====================
CBasePlayer::Spawn

=====================
*/
void CBasePlayer::Spawn( void )
{
	if (m_pActiveItem)
		m_pActiveItem->Deploy( );
}

Vector CBasePlayer::GetGunPosition()
{
	Vector origin = pev->origin;
	Vector view_ofs;

	gEngfuncs.pEventAPI->EV_LocalPlayerViewheight(view_ofs);

	return origin + view_ofs;
}

/*
=====================
UTIL_TraceLine

Don't actually trace, but act like the trace didn't hit anything.
=====================
*/
void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr )
{
	memset( ptr, 0, sizeof( *ptr ) );
#if 0
	static float flLastFraction = 1.0f;

	if( g_runfuncs )
	{
		Vector vStart = vecStart, vEnd = vecEnd;
		pmtrace_t pmtrace;

		gEngfuncs.pEventAPI->EV_SetTraceHull( 0 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vStart, vEnd, 0, -1, &pmtrace );
		flLastFraction = ptr->flFraction = pmtrace.fraction;
		ptr->vecEndPos = pmtrace.endpos;
	}
	else
	{
		ptr->flFraction = flLastFraction;
	}
#else
	ptr->flFraction = 1.0f;
#endif
}

char UTIL_TextureHit(TraceResult *ptr, Vector vecSrc, Vector vecEnd)
{
	char chTextureType;
	float rgfl1[3], rgfl2[3];
	const char *pTextureName;
	char szbuffer[64];
	CBaseEntity *pEntity;

	if( ptr->pHit == NULL )
		return CHAR_TEX_FLESH;

	pEntity = CBaseEntity::Instance(ptr->pHit);

	if (pEntity && pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
		return CHAR_TEX_FLESH;

	vecSrc.CopyToArray(rgfl1);
	vecEnd.CopyToArray(rgfl2);

	if (pEntity)
		pTextureName = TRACE_TEXTURE(ENT(pEntity->pev), rgfl1, rgfl2);
	else
		pTextureName = TRACE_TEXTURE(ENT(0), rgfl1, rgfl2);

	if (pTextureName)
	{
		if (*pTextureName == '-' || *pTextureName == '+')
			pTextureName += 2;

		if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
			pTextureName++;

		strncpy(szbuffer, pTextureName, sizeof(szbuffer));
		szbuffer[CBTEXTURENAMEMAX - 1] = 0;
		chTextureType = PM_FindTextureType(szbuffer);
	}
	else
		chTextureType = 0;

	return chTextureType;
}

CBasePlayer *UTIL_PlayerByIndex(int playerIndex)
{
#if 0
	CBaseEntity *pPlayer = NULL;

	if (playerIndex > 0 && playerIndex <= gpGlobals->maxClients)
	{
		edict_t *pPlayerEdict = INDEXENT(playerIndex);

		if (pPlayerEdict && !pPlayerEdict->free)
			pPlayer = CBaseEntity::Instance(pPlayerEdict);
	}

	return pPlayer;
#else
	return &player;
#endif
}

void Broadcast( const char *msg, int pitch ); // hud/radio
void CBasePlayer::Radio(const char *msg_id, const char *msg_verbose, int pitch, bool showIcon)
{
	Broadcast( msg_id, pitch );
}

void UTIL_MakeVectors( const Vector &vec )
{
	gEngfuncs.pfnAngleVectors( vec, gpGlobals->v_forward, gpGlobals->v_right, gpGlobals->v_up );
}

/*
=====================
HUD_InitClientWeapons

Set up weapons, player and functions needed to run weapons code client-side.
=====================
*/
void HUD_InitClientWeapons( void )
{
	static int initialized = 0;
	if ( initialized )
		return;

	initialized = 1;

	// Set up pointer ( dummy object )
	gpGlobals = &Globals;

	// Fill in current time ( probably not needed )
	gpGlobals->time = gEngfuncs.GetClientTime();

	// Fake functions
	//g_engfuncs.pfnSetClientMaxspeed = HUD_SetMaxSpeed;

	// Handled locally
	g_engfuncs.pfnPlaybackEvent		= HUD_PlaybackEvent;
	g_engfuncs.pfnAlertMessage		= AlertMessage;

	// Pass through to engine
	g_engfuncs.pfnPrecacheEvent		= gEngfuncs.pfnPrecacheEvent;
	g_engfuncs.pfnRandomFloat		= gEngfuncs.pfnRandomFloat;
	g_engfuncs.pfnRandomLong		= gEngfuncs.pfnRandomLong;

	// Allocate a slot for the local player
	HUD_PrepEntity( &player	);

	// Allocate slot(s) for each weapon that we are going to be predicting
	if( gHUD.GetGameType() != GAME_CZERODS )
	{
		HUD_PrepEntity( &g_P228, &player);
		HUD_PrepEntity( &g_SCOUT, &player);
		HUD_PrepEntity( &g_HEGrenade, &player);
		HUD_PrepEntity( &g_XM1014, &player);
		HUD_PrepEntity( &g_C4, &player);
		HUD_PrepEntity( &g_MAC10, &player);
		HUD_PrepEntity( &g_AUG, &player);
		HUD_PrepEntity( &g_SmokeGrenade, &player);
		HUD_PrepEntity( &g_ELITE, &player);
		HUD_PrepEntity( &g_FiveSeven, &player);
		HUD_PrepEntity( &g_UMP45, &player);
		HUD_PrepEntity( &g_SG550, &player);
		HUD_PrepEntity( &g_Galil, &player);
		HUD_PrepEntity( &g_Famas, &player);
		HUD_PrepEntity( &g_USP, &player);
		HUD_PrepEntity( &g_GLOCK18, &player);
		HUD_PrepEntity( &g_AWP, &player);
		HUD_PrepEntity( &g_MP5N, &player);
		HUD_PrepEntity( &g_M249, &player);
		HUD_PrepEntity( &g_M4A1, &player);
		HUD_PrepEntity( &g_M3, &player );
		HUD_PrepEntity( &g_TMP, &player);
		HUD_PrepEntity( &g_G3SG1, &player);
		HUD_PrepEntity( &g_Flashbang, &player);
		HUD_PrepEntity( &g_DEAGLE, &player);
		HUD_PrepEntity( &g_SG552, &player);
		HUD_PrepEntity( &g_AK47, &player);
		HUD_PrepEntity( &g_Knife, &player);
		HUD_PrepEntity( &g_P90, &player );
	}
}


int GetWeaponAccuracyFlags( int weaponid )
{
	int result = 0;

	if( weaponid <= WEAPON_P90 )
	{
		switch( weaponid )
		{
		case WEAPON_AUG:
		case WEAPON_GALIL:
		case WEAPON_M249:
		case WEAPON_SG552:
		case WEAPON_AK47:
		case WEAPON_P90:
			result = ACCURACY_AIR | ACCURACY_SPEED;
			break;
		case WEAPON_P228:
		case WEAPON_FIVESEVEN:
		case WEAPON_DEAGLE:
			result = ACCURACY_AIR | ACCURACY_SPEED | ACCURACY_DUCK;
			break;
		case WEAPON_GLOCK18:
			if( g_iWeaponFlags & WPNSTATE_GLOCK18_BURST_MODE)
			{
				result = ACCURACY_AIR | ACCURACY_SPEED | ACCURACY_DUCK;
			}
			else
			{
				result = ACCURACY_AIR | ACCURACY_SPEED | ACCURACY_DUCK | ACCURACY_MULTIPLY_BY_14_2;
			}
			break;
		case WEAPON_MAC10:
		case WEAPON_UMP45:
		case WEAPON_MP5N:
		case WEAPON_TMP:
			result = ACCURACY_AIR;
			break;
		case WEAPON_M4A1:
			if(g_iWeaponFlags & WPNSTATE_USP_SILENCED)
			{
				result = ACCURACY_AIR | ACCURACY_SPEED;
			}
			else
			{
				result = ACCURACY_AIR | ACCURACY_SPEED | ACCURACY_MULTIPLY_BY_14;
			}
			break;
		case WEAPON_FAMAS:
			if(g_iWeaponFlags & WPNSTATE_FAMAS_BURST_MODE)
			{
				result = ACCURACY_AIR | ACCURACY_SPEED;
			}
			else
			{
				result = ACCURACY_AIR | ACCURACY_SPEED | (1<<4);
			}
			break;
		case WEAPON_USP:
			if(g_iWeaponFlags & WPNSTATE_USP_SILENCED)
			{
				result = ACCURACY_AIR | ACCURACY_SPEED | ACCURACY_DUCK;
			}
			else
			{
				result = ACCURACY_AIR | ACCURACY_SPEED | ACCURACY_DUCK | ACCURACY_MULTIPLY_BY_14;
			}
			break;
		}
	}

	return result;
}

/*
=====================
HUD_WeaponsPostThink

Run Weapon firing code on client
=====================
*/
void HUD_WeaponsPostThink( local_state_s *from, local_state_s *to, usercmd_t *cmd, double time, unsigned int random_seed )
{
	int i;
	int buttonsChanged;
	CBasePlayerWeapon *pWeapon = NULL;
	static int lasthealth;
	int flags;

	HUD_InitClientWeapons();

	// Get current clock
	gpGlobals->time = time;

	// Fill in data based on selected weapon
	switch ( from->client.m_iId )
	{
		case WEAPON_P228:
			pWeapon = &g_P228;
			break;

		case WEAPON_SCOUT:
			pWeapon = &g_SCOUT;
			break;

		case WEAPON_HEGRENADE:
			pWeapon = &g_HEGrenade;
			break;

		case WEAPON_XM1014:
			pWeapon = &g_XM1014;
			break;

		case WEAPON_C4:
			pWeapon = &g_C4;
			break;

		case WEAPON_MAC10:
			pWeapon = &g_MAC10;
			break;

		case WEAPON_AUG:
			pWeapon = &g_AUG;
			break;

		case WEAPON_SMOKEGRENADE:
			pWeapon = &g_SmokeGrenade;
			break;

		case WEAPON_ELITE:
			pWeapon = &g_ELITE;
			break;

		case WEAPON_FIVESEVEN:
			pWeapon = &g_FiveSeven;
			break;

		case WEAPON_UMP45:
			pWeapon = &g_UMP45;
			break;

		case WEAPON_SG550:
			pWeapon = &g_SG550;
			break;

		case WEAPON_GALIL:
			pWeapon = &g_Galil;
			break;

		case WEAPON_FAMAS:
			pWeapon = &g_Famas;
			break;

		case WEAPON_USP:
			pWeapon = &g_USP;
			break;

		case WEAPON_GLOCK18:
			pWeapon = &g_GLOCK18;
			break;

		case WEAPON_AWP:
			pWeapon = &g_AWP;
			break;

		case WEAPON_MP5N:
			pWeapon = &g_MP5N;
			break;

		case WEAPON_M249:
			pWeapon = &g_M249;
			break;

		case WEAPON_M3:
			pWeapon = &g_M3;
			break;

		case WEAPON_M4A1:
			pWeapon = &g_M4A1;
			break;

		case WEAPON_TMP:
			pWeapon = &g_TMP;
			break;

		case WEAPON_G3SG1:
			pWeapon = &g_G3SG1;
			break;

		case WEAPON_FLASHBANG:
			pWeapon = &g_Flashbang;
			break;

		case WEAPON_DEAGLE:
			pWeapon = &g_DEAGLE;
			break;

		case WEAPON_SG552:
			pWeapon = &g_SG552;
			break;

		case WEAPON_AK47:
			pWeapon = &g_AK47;
			break;

		case WEAPON_KNIFE:
			pWeapon = &g_Knife;
			break;

		case WEAPON_P90:
			pWeapon = &g_P90;
			break;

		/*case WEAPON_NONE:
			break;

		case WEAPON_GLOCK:
		default:
			gEngfuncs.Con_Printf("VALVEWHY: Unknown Weapon %i is active.\n", from->client.m_iId );
			break;*/
	}

	// Store pointer to our destination entity_state_t so we can get our origin, etc. from it
	//  for setting up events on the client
	g_finalstate = to;

	// If we are running events/etc. go ahead and see if we
	//  managed to die between last frame and this one
	// If so, run the appropriate player killed or spawn function
	if ( g_runfuncs )
	{
		if ( to->client.health <= 0 && lasthealth > 0 )
			player.Killed( NULL, 0 );
		else if ( to->client.health > 0 && lasthealth <= 0 )
			player.Spawn();

		lasthealth = to->client.health;
	}

	// We are not predicting the current weapon, just bow out here.
	if ( !pWeapon )
		return;

	for ( i = 0; i < MAX_WEAPONS; i++ )
	{
		CBasePlayerWeapon *pCurrent = g_pWpns[ i ];
		if ( !pCurrent )
			continue;

		weapon_data_t *pfrom = from->weapondata + i;

		pCurrent->m_fInReload			= pfrom->m_fInReload;
		pCurrent->m_fInSpecialReload	= pfrom->m_fInSpecialReload;
		pCurrent->m_iClip				= pfrom->m_iClip;
		pCurrent->m_flNextPrimaryAttack	= pfrom->m_flNextPrimaryAttack;
		pCurrent->m_flNextSecondaryAttack = pfrom->m_flNextSecondaryAttack;
		pCurrent->m_flTimeWeaponIdle	= pfrom->m_flTimeWeaponIdle;
		pCurrent->m_flStartThrow		= pfrom->fuser2;
		pCurrent->m_flReleaseThrow		= pfrom->fuser3;
		pCurrent->m_iSwing				= pfrom->iuser1;
		pCurrent->m_iWeaponState		= pfrom->m_iWeaponState;
		pCurrent->m_flLastFire			= pfrom->m_fAimedDamage;
		pCurrent->m_iShotsFired			= pfrom->m_fInZoom;
	}

	if( from->client.vuser4.x < 0 || from->client.vuser4.x > MAX_AMMO_TYPES )
		pWeapon->m_iPrimaryAmmoType = 0;
	else
	{
		pWeapon->m_iPrimaryAmmoType = (int)from->client.vuser4.x;
		player.m_rgAmmo[ pWeapon->m_iPrimaryAmmoType ]  = (int)from->client.vuser4.y;
	}


	g_iWeaponFlags = pWeapon->m_iWeaponState;

	// For random weapon events, use this seed to seed random # generator
	player.random_seed = random_seed;

	// Get old buttons from previous state.
	player.m_afButtonLast = from->playerstate.oldbuttons;

	// Which buttsons chave changed
	buttonsChanged = (player.m_afButtonLast ^ cmd->buttons);	// These buttons have changed this frame

	// Debounced button codes for pressed/released
	// The changed ones still down are "pressed"
	player.m_afButtonPressed =  buttonsChanged & cmd->buttons;
	// The ones not down are "released"
	player.m_afButtonReleased = buttonsChanged & (~cmd->buttons);
	player.pev->v_angle = cmd->viewangles;
	player.pev->origin = from->client.origin;

	// Set player variables that weapons code might check/alter
	player.pev->button = cmd->buttons;

	player.pev->velocity = from->client.velocity;

	player.pev->deadflag   = from->client.deadflag;
	player.pev->waterlevel = from->client.waterlevel;
	player.pev->maxspeed   = from->client.maxspeed;
	player.pev->punchangle = from->client.punchangle;
	player.pev->fov        = from->client.fov;
	player.pev->weaponanim = from->client.weaponanim;
	player.pev->viewmodel  = from->client.viewmodel;
	player.m_flNextAttack  = from->client.m_flNextAttack;

	g_iPlayerFlags    = player.pev->flags = from->client.flags;
	g_vPlayerVelocity = player.pev->velocity;
	g_flPlayerSpeed	  = player.pev->velocity.Length();
	g_iWaterLevel     = player.pev->waterlevel;

	//Stores all our ammo info, so the client side weapons can use them.
	player.ammo_9mm			= from->client.ammo_nails;
	player.ammo_556nato		= from->client.ammo_cells;
	player.ammo_buckshot	= from->client.ammo_shells;
	player.ammo_556natobox	= from->client.ammo_rockets;
	player.ammo_762nato		= (int)from->client.vuser2.x;
	player.ammo_45acp		= (int)from->client.vuser2.y;
	player.ammo_50ae		= (int)from->client.vuser2.z;
	player.ammo_338mag		= (int)from->client.vuser3.x;
	player.ammo_57mm		= (int)from->client.vuser3.y;
	player.ammo_357sig		= (int)from->client.vuser3.z;

	cl_entity_t *pplayer = gEngfuncs.GetLocalPlayer();
	if( pplayer )
	{
		player.pev->origin = from->client.origin;
		player.pev->angles	= pplayer->angles;
		player.pev->v_angle = v_angles;
	}

	flags = from->client.iuser3;
	g_bHoldingKnife		= pWeapon->m_iId == WEAPON_KNIFE;
	player.m_bCanShoot	= (flags & PLAYER_CAN_SHOOT) != 0;
	g_iFreezeTimeOver	= !(flags & PLAYER_FREEZE_TIME_OVER);
	g_bInBombZone		= (flags & PLAYER_IN_BOMB_ZONE) != 0;

	// validate if we can hold shield with specified weapon
	switch( from->client.m_iId )
	{
	case WEAPON_KNIFE:
	case WEAPON_GLOCK18:
	case WEAPON_USP:
	case WEAPON_P228:
	case WEAPON_DEAGLE:
	case WEAPON_FIVESEVEN:
	case WEAPON_HEGRENADE:
	case WEAPON_FLASHBANG:
	case WEAPON_SMOKEGRENADE:
		g_bHoldingShield	= (flags & PLAYER_HOLDING_SHIELD) != 0;
		break;
	default:
		g_bHoldingShield = false;
		break;
	}

	// Point to current weapon object
	if ( from->client.m_iId )
		player.m_pActiveItem = pWeapon;

	// Don't go firing anything if we have died.
	// Or if we don't have a weapon model deployed
	if ( ( player.pev->deadflag != ( DEAD_DISCARDBODY + 1 ) ) &&
		 !CL_IsDead() && player.pev->viewmodel && !g_iUser1 )
	{
		if( g_bHoldingKnife && pWeapon->m_iClientWeaponState &&
				player.pev->button & IN_FORWARD )
			player.m_flNextAttack = 0;
		else if( player.m_flNextAttack <= 0 )
		{
			pWeapon->ItemPostFrame();
		}
	}

	// Assume that we are not going to switch weapons
	to->client.m_iId					= from->client.m_iId;

	// Now see if we issued a changeweapon command ( and we're not dead )
	if ( cmd->weaponselect && ( player.pev->deadflag != ( DEAD_DISCARDBODY + 1 ) ) )
	{
		// Switched to a different weapon?
		if ( from->weapondata[ cmd->weaponselect ].m_iId == cmd->weaponselect )
		{
			CBasePlayerWeapon *pNew = g_pWpns[ cmd->weaponselect ];
			if ( pNew && ( pNew != pWeapon ) )
			{
				// Put away old weapon
				if (player.m_pActiveItem)
					player.m_pActiveItem->Holster( );

				player.m_pLastItem = player.m_pActiveItem;
				player.m_pActiveItem = pNew;

				// Deploy new weapon
				if (player.m_pActiveItem)
				{
					player.m_pActiveItem->Deploy( );
				}

				// Update weapon id so we can predict things correctly.
				to->client.m_iId = cmd->weaponselect;
			}
		}
	}

	// Copy in results of prediction code
	to->client.viewmodel				= player.pev->viewmodel;
	to->client.fov						= player.pev->fov;
	to->client.weaponanim				= player.pev->weaponanim;
	to->client.m_flNextAttack			= player.m_flNextAttack;
	to->client.maxspeed					= player.pev->maxspeed;
	to->client.punchangle				= player.pev->punchangle;


	to->client.ammo_nails = player.ammo_9mm;
	to->client.ammo_cells = player.ammo_556nato;
	to->client.ammo_shells = player.ammo_buckshot;
	to->client.ammo_rockets = player.ammo_556natobox;
	to->client.vuser2.x = player.ammo_762nato;
	to->client.vuser2.y = player.ammo_45acp;
	to->client.vuser2.z = player.ammo_50ae;
	to->client.vuser3.x = player.ammo_338mag;
	to->client.vuser3.y = player.ammo_57mm;
	to->client.vuser3.z = player.ammo_357sig;
	to->client.iuser3 = flags;




	// Make sure that weapon animation matches what the game .dll is telling us
	//  over the wire ( fixes some animation glitches )
	if ( g_runfuncs && ( HUD_GetWeaponAnim() != to->client.weaponanim ) )
		// Force a fixed anim down to viewmodel
		HUD_SendWeaponAnim( to->client.weaponanim, to->client.m_iId, 2, 1 );

	if (pWeapon->m_iPrimaryAmmoType < MAX_AMMO_TYPES)
	{
		to->client.vuser4.x = pWeapon->m_iPrimaryAmmoType;
		to->client.vuser4.y = player.m_rgAmmo[ pWeapon->m_iPrimaryAmmoType ];
	}
	else
	{
		to->client.vuser4.x = -1.0;
		to->client.vuser4.y = 0;
	}

	for ( i = 0; i < MAX_WEAPONS; i++ )
	{
		CBasePlayerWeapon *pCurrent = g_pWpns[ i ];

		weapon_data_t *pto = to->weapondata + i;

		if ( !pCurrent )
		{
			memset( pto, 0, sizeof( weapon_data_t ) );
			continue;
		}

		pto->m_iClip					= pCurrent->m_iClip;

		pto->m_flNextPrimaryAttack		= pCurrent->m_flNextPrimaryAttack;
		pto->m_flNextSecondaryAttack	= pCurrent->m_flNextSecondaryAttack;
		pto->m_flTimeWeaponIdle			= pCurrent->m_flTimeWeaponIdle;

		pto->m_fInReload				= pCurrent->m_fInReload;
		pto->m_fInSpecialReload			= pCurrent->m_fInSpecialReload;
		pto->m_flNextReload				= pCurrent->m_flNextReload;
		pto->fuser2						= pCurrent->m_flStartThrow;
		pto->fuser3						= pCurrent->m_flReleaseThrow;
		pto->iuser1						= pCurrent->m_iSwing;
		pto->m_iWeaponState				= pCurrent->m_iWeaponState;
		pto->m_fInZoom					= pCurrent->m_iShotsFired;
		pto->m_fAimedDamage				= pCurrent->m_flLastFire;

		// Decrement weapon counters, server does this at same time ( during post think, after doing everything else )
		pto->m_flNextReload				-= cmd->msec / 1000.0f;
		pto->m_fNextAimBonus			-= cmd->msec / 1000.0f;
		pto->m_flNextPrimaryAttack		-= cmd->msec / 1000.0f;
		pto->m_flNextSecondaryAttack	-= cmd->msec / 1000.0f;
		pto->m_flTimeWeaponIdle			-= cmd->msec / 1000.0f;


		if( pto->m_flPumpTime != -9999.0f )
		{
			pto->m_flPumpTime -= cmd->msec / 1000.0f;
			if( pto->m_flPumpTime < -1.0f )
				pto->m_flPumpTime = 1.0f;
		}

		if ( pto->m_fNextAimBonus < -1.0 )
		{
			pto->m_fNextAimBonus = -1.0;
		}

		if ( pto->m_flNextPrimaryAttack < -1.0 )
		{
			pto->m_flNextPrimaryAttack = -1.0;
		}

		if ( pto->m_flNextSecondaryAttack < -0.001 )
		{
			pto->m_flNextSecondaryAttack = -0.001;
		}

		if ( pto->m_flTimeWeaponIdle < -0.001 )
		{
			pto->m_flTimeWeaponIdle = -0.001;
		}

		if ( pto->m_flNextReload < -0.001 )
		{
			pto->m_flNextReload = -0.001;
		}

		/*if ( pto->fuser1 < -0.001 )
		{
			pto->fuser1 = -0.001;
		}*/
	}

	// m_flNextAttack is now part of the weapons, but is part of the player instead
	to->client.m_flNextAttack -= cmd->msec / 1000.0f;
	if ( to->client.m_flNextAttack < -0.001 )
	{
		to->client.m_flNextAttack = -0.001;
	}

	// Wipe it so we can't use it after this frame
	g_finalstate = NULL;
}

/*
=====================
HUD_PostRunCmd

Client calls this during prediction, after it has moved the player and updated any info changed into to->
time is the current client clock based on prediction
cmd is the command that caused the movement, etc
runfuncs is 1 if this is the first time we've predicted this command.  If so, sounds and effects should play, otherwise, they should
be ignored
=====================
*/
void DLLEXPORT HUD_PostRunCmd( local_state_t *from, local_state_t *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed )
{
	g_runfuncs = runfuncs;

	HUD_WeaponsPostThink( from, to, cmd, time, random_seed );
	to->client.fov = g_lastFOV;

	if ( g_runfuncs )
	{
		g_gaitseq	= to->playerstate.gaitsequence;
		g_rseq		= to->playerstate.sequence;
		g_clang		= cmd->viewangles;
		g_clorg		= to->playerstate.origin;
	}
}
