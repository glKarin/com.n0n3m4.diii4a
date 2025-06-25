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
//
// Health.cpp
//
// implementation of CHudHealth class
//

#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include "eventscripts.h"

#include "draw_util.h"

#include "ev_hldm.h"
#include "com_weapons.h"

#define PAIN_NAME "sprites/%d_pain.spr"
#define DAMAGE_NAME "sprites/%d_dmg.spr"
#define EPSILON 0.4f

int giDmgHeight, giDmgWidth;

int giDmgFlags[NUM_DMG_TYPES] = 
{
	DMG_POISON,
	DMG_ACID,
	DMG_FREEZE|DMG_SLOWFREEZE,
	DMG_DROWN,
	DMG_BURN|DMG_SLOWBURN,
	DMG_NERVEGAS,
	DMG_RADIATION,
	DMG_SHOCK,
	DMG_CALTROP,
	DMG_TRANQ,
	DMG_CONCUSS,
	DMG_HALLUC
};

enum
{
	ATK_FRONT = 0,
	ATK_RIGHT,
	ATK_REAR,
	ATK_LEFT
};

int CHudHealth::Init(void)
{
	HOOK_MESSAGE(gHUD.m_Health, Health);
	HOOK_MESSAGE(gHUD.m_Health, Damage);
	HOOK_MESSAGE(gHUD.m_Health, ScoreAttrib);
	HOOK_MESSAGE(gHUD.m_Health, ClCorpse);
	HOOK_MESSAGE( gHUD.m_Health, HealthInfo );
	HOOK_MESSAGE( gHUD.m_Health, Account );

	m_iHealth = 100;
	m_fFade = 0;
	m_iFlags = 0;
	m_bitsDamage = 0;
	giDmgHeight = 0;
	giDmgWidth = 0;

	for( int i = 0; i < 4; i++ )
		m_fAttack[i] = 0;

	memset(m_dmg, 0, sizeof(DAMAGE_IMAGE) * NUM_DMG_TYPES);

	CVAR_CREATE("cl_corpsestay", "600", FCVAR_ARCHIVE);
	gHUD.AddHudElem(this);
	return 1;
}

void CHudHealth::Reset( void )
{
	// make sure the pain compass is cleared when the player respawns
	for( int i = 0; i < 4; i++ )
		m_fAttack[i] = 0;


	// force all the flashing damage icons to expire
	m_bitsDamage = 0;
	for ( int i = 0; i < NUM_DMG_TYPES; i++ )
	{
		m_dmg[i].fExpire = 0;
	}
}

int CHudHealth::VidInit(void)
{
	m_hSprite = LoadSprite(PAIN_NAME);

	m_vAttackPos[ATK_FRONT].x = ScreenWidth  / 2.0 - SPR_Width ( m_hSprite, 0 ) / 2.0;
	m_vAttackPos[ATK_FRONT].y = ScreenHeight / 2.0 - SPR_Height( m_hSprite, 0 ) * 3;

	m_vAttackPos[ATK_RIGHT].x = ScreenWidth  / 2.0 + SPR_Width ( m_hSprite, 1 ) * 2;
	m_vAttackPos[ATK_RIGHT].y = ScreenHeight / 2.0 - SPR_Height( m_hSprite, 1 ) / 2.0;

	m_vAttackPos[ATK_REAR ].x = ScreenWidth  / 2.0 - SPR_Width ( m_hSprite, 2 ) / 2.0;
	m_vAttackPos[ATK_REAR ].y = ScreenHeight / 2.0 + SPR_Height( m_hSprite, 2 ) * 2;

	m_vAttackPos[ATK_LEFT ].x = ScreenWidth  / 2.0 - SPR_Width ( m_hSprite, 3 ) * 3;
	m_vAttackPos[ATK_LEFT ].y = ScreenHeight / 2.0 - SPR_Height( m_hSprite, 3 ) / 2.0;


	m_HUD_dmg_bio = gHUD.GetSpriteIndex( "dmg_bio" ) + 1;
	m_HUD_cross = gHUD.GetSpriteIndex( "cross" );

	giDmgHeight = gHUD.GetSpriteRect(m_HUD_dmg_bio).Width();
	giDmgWidth = gHUD.GetSpriteRect(m_HUD_dmg_bio).Height();

	return 1;
}

int CHudHealth:: MsgFunc_Health(const char *pszName,  int iSize, void *pbuf )
{
	// TODO: update local health data
	BufferReader reader( pszName, pbuf, iSize );
	int x = reader.ReadByte();

	m_iFlags |= HUD_DRAW;

	// Only update the fade if we've changed health
	if (x != m_iHealth)
	{
		m_fFade = FADE_TIME;
		m_iHealth = x;
	}

	return 1;
}


int CHudHealth:: MsgFunc_Damage(const char *pszName,  int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	int armor = reader.ReadByte();	// armor
	int damageTaken = reader.ReadByte();	// health
	long bitsDamage = reader.ReadLong(); // damage bits

	vec3_t vecFrom;

	for ( int i = 0 ; i < 3 ; i++)
		vecFrom[i] = reader.ReadCoord();

	UpdateTiles(gHUD.m_flTime, bitsDamage);

	// Actually took damage?
	if ( damageTaken > 0 || armor > 0 )
	{
		CalcDamageDirection(vecFrom);
		if( g_iXash )
		{
			float time = damageTaken * 4.0f + armor * 2.0f;

			if( time > 200.0f ) time = 200.0f;
			gMobileAPI.pfnVibrate( time, 0 );
		}
	}
	return 1;
}

int CHudHealth:: MsgFunc_ScoreAttrib(const char *pszName,  int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	int index = reader.ReadByte();
	unsigned char flags = reader.ReadByte();
	g_PlayerExtraInfo[index].dead   = !!(flags & PLAYER_DEAD);
	g_PlayerExtraInfo[index].has_c4 = !!(flags & PLAYER_HAS_C4);
	g_PlayerExtraInfo[index].vip    = !!(flags & PLAYER_VIP);
	return 1;
}
// Returns back a color from the
// Green <-> Yellow <-> Red ramp
void CHudHealth::GetPainColor( int &r, int &g, int &b, int &a )
{
#if 0
	int iHealth = m_iHealth;

	if (iHealth > 25)
		iHealth -= 25;
	else if ( iHealth < 0 )
		iHealth = 0;
	g = iHealth * 255 / 100;
	r = 255 - g;
	b = 0;
#else
	if( m_iHealth <= 15 )
	{
		a = 255; // If health is getting low, make it bright red
	}
	else
	{
		// Has health changed? Flash the health #
		if (m_fFade)
		{
			m_fFade -= (gHUD.m_flTimeDelta * 20);

			if (m_fFade <= 0)
			{
				m_fFade = 0;
				a = MIN_ALPHA;
			}
			else
			{
				// Fade the health number back to dim
				a = MIN_ALPHA +  (m_fFade/FADE_TIME) * 128;
			}
		}
		else
		{
			a = MIN_ALPHA;
		}
	}

	if (m_iHealth > 25)
	{
		DrawUtils::UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );
	}
	else
	{
		r = 250;
		g = 0;
		b = 0;
	}
#endif 
}


int CHudHealth::Draw(float flTime)
{
	if( !(gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH ) && !gEngfuncs.IsSpectateOnly() )
	{
		DrawHealthBar( flTime );
		DrawDamage( flTime );
		DrawPain( flTime );
	}

	return 1;
}

void CHudHealth::DrawHealthBar( float flTime )
{
	int r, g, b;
	int a = 0, x, y;
	int HealthWidth;

	GetPainColor( r, g, b, a );
	DrawUtils::ScaleColors(r, g, b, a );

	// Only draw health if we have the suit.
	if (gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)))
	{
		HealthWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).Width();
		int CrossWidth = gHUD.GetSpriteRect(m_HUD_cross).Width();

		y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
		x = CrossWidth /2;

		SPR_Set(gHUD.GetSprite(m_HUD_cross), r, g, b);
		SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_cross));

		x = CrossWidth + HealthWidth / 2;

		x = DrawUtils::DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iHealth, r, g, b);
	}
}

void CHudHealth::CalcDamageDirection( Vector vecFrom )
{
	Vector	forward, right, up;
	float	side, front, flDistToTarget;

	if( vecFrom.IsNull() )
	{
		for( int i = 0; i < 4; i++ )
			m_fAttack[i] = 0;
		return;
	}

	vecFrom = vecFrom - gHUD.m_vecOrigin;
	flDistToTarget = vecFrom.Length();
	vecFrom = vecFrom.Normalize();
	AngleVectors (gHUD.m_vecAngles, forward, right, up);

	front = DotProduct (vecFrom, right);
	side = DotProduct (vecFrom, forward);

	if (flDistToTarget <= 50)
	{
		for( int i = 0; i < 4; i++ )
			m_fAttack[i] = 1;
	}
	else
	{
		if (side > EPSILON)
			m_fAttack[0] = max(m_fAttack[0], side);
		if (side < -EPSILON)
			m_fAttack[1] = max(m_fAttack[1], 0 - side );
		if (front > EPSILON)
			m_fAttack[2] = max(m_fAttack[2], front);
		if (front < -EPSILON)
			m_fAttack[3] = max(m_fAttack[3], 0 - front );
	}
}

void CHudHealth::DrawPain(float flTime)
{
	if (m_fAttack[0] == 0 &&
		m_fAttack[1] == 0 &&
		m_fAttack[2] == 0 &&
		m_fAttack[3] == 0)
		return;

	float a, fFade = gHUD.m_flTimeDelta * 2;

	for( int i = 0; i < 4; i++ )
	{
		if( m_fAttack[i] > EPSILON )
		{
			/*GetPainColor(r, g, b);
			shade = a * max( m_fAttack[i], 0.5 );
			DrawUtils::ScaleColors(r, g, b, shade);*/

			a = max( m_fAttack[i], 0.5 );

			SPR_Set( m_hSprite, 255 * a, 255 * a, 255 * a);
			SPR_DrawAdditive( i, m_vAttackPos[i].x, m_vAttackPos[i].y, NULL );
			m_fAttack[i] = max( 0, m_fAttack[i] - fFade );
		}
		else
			m_fAttack[i] = 0;
	}
}

void CHudHealth::DrawDamage(float flTime)
{
	int r, g, b, a;

	if (!m_bitsDamage)
		return;

	DrawUtils::UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );

	a = (int)( fabs(sin(flTime*2)) * 256.0);

	DrawUtils::ScaleColors(r, g, b, a);
	int i;
	// Draw all the items
	for (i = 0; i < NUM_DMG_TYPES; i++)
	{
		if (m_bitsDamage & giDmgFlags[i])
		{
			DAMAGE_IMAGE *pdmg = &m_dmg[i];
			SPR_Set(gHUD.GetSprite(m_HUD_dmg_bio + i), r, g, b );
			SPR_DrawAdditive(0, pdmg->x, pdmg->y, &gHUD.GetSpriteRect(m_HUD_dmg_bio + i));
		}
	}


	// check for bits that should be expired
	for ( i = 0; i < NUM_DMG_TYPES; i++ )
	{
		DAMAGE_IMAGE *pdmg = &m_dmg[i];

		if ( m_bitsDamage & giDmgFlags[i] )
		{
			pdmg->fExpire = min( flTime + DMG_IMAGE_LIFE, pdmg->fExpire );

			if ( pdmg->fExpire <= flTime		// when the time has expired
				 && a < 40 )						// and the flash is at the low point of the cycle
			{
				pdmg->fExpire = 0;

				int y = pdmg->y;
				pdmg->x = pdmg->y = 0;

				// move everyone above down
				for (int j = 0; j < NUM_DMG_TYPES; j++)
				{
					pdmg = &m_dmg[j];
					if ((pdmg->y) && (pdmg->y < y))
						pdmg->y += giDmgHeight;

				}

				m_bitsDamage &= ~giDmgFlags[i];  // clear the bits
			}
		}
	}
}


void CHudHealth::UpdateTiles(float flTime, long bitsDamage)
{	
	DAMAGE_IMAGE *pdmg;

	// Which types are new?
	long bitsOn = ~m_bitsDamage & bitsDamage;
	
	for (int i = 0; i < NUM_DMG_TYPES; i++)
	{
		pdmg = &m_dmg[i];

		// Is this one already on?
		if (m_bitsDamage & giDmgFlags[i])
		{
			pdmg->fExpire = flTime + DMG_IMAGE_LIFE; // extend the duration
			if (!pdmg->fBaseline)
				pdmg->fBaseline = flTime;
		}

		// Are we just turning it on?
		if (bitsOn & giDmgFlags[i])
		{
			// put this one at the bottom
			pdmg->x = giDmgWidth/8;
			pdmg->y = ScreenHeight - giDmgHeight * 2;
			pdmg->fExpire=flTime + DMG_IMAGE_LIFE;
			
			// move everyone else up
			for (int j = 0; j < NUM_DMG_TYPES; j++)
			{
				if (j == i)
					continue;

				pdmg = &m_dmg[j];
				if (pdmg->y)
					pdmg->y -= giDmgHeight;

			}
			pdmg = &m_dmg[i];
		}
	}

	// damage bits are only turned on here;  they are turned off when the draw time has expired (in DrawDamage())
	m_bitsDamage |= bitsDamage;
}


int CHudHealth :: MsgFunc_ClCorpse(const char *pszName, int iSize, void *pbuf)
{
	BufferReader reader(pbuf, iSize);
	char *pModel, szModel[64];
	Vector origin, angles;
	float delay;
	int seq, classID, teamID, playerID;

	pModel		= reader.ReadString();
	origin.x	= reader.ReadLong() / 128.0f;
	origin.y	= reader.ReadLong() / 128.0f;
	origin.z	= reader.ReadLong() / 128.0f;
	angles		= reader.ReadCoordVector();
	delay		= gEngfuncs.GetClientTime() + (reader.ReadLong() / 100.0f);
	seq			= reader.ReadByte();
	classID		= reader.ReadByte();
	teamID		= reader.ReadByte();
	playerID	= reader.ReadByte();

	if( !gHUD.cl_minmodels->value )
	{
		if( !strstr(pModel, "models/") )
			snprintf( szModel, sizeof(szModel), "models/player/%s/%s.mdl", pModel, pModel );
		else
			strncpy( szModel, pModel, sizeof( szModel ));
	}
	else
	{
		int modelidx;
		if( teamID == TEAM_TERRORIST ) // terrorists
		{
			modelidx = gHUD.cl_min_t->value;
			if( !BIsValidTModelIndex(modelidx) )
				modelidx = PLAYERMODEL_LEET;
		}
		else if( teamID == TEAM_CT ) // ct
		{
			if( g_PlayerExtraInfo[playerID].vip )
				modelidx = PLAYERMODEL_VIP;
			else if( !BIsValidCTModelIndex( gHUD.cl_min_ct->value ))
				modelidx = PLAYERMODEL_GIGN;
			else modelidx = gHUD.cl_min_ct->value;
		}
		else modelidx = PLAYERMODEL_PLAYER;

		strncpy( szModel, sPlayerModelFiles[modelidx], sizeof( szModel ) );
	}
	CreateCorpse( origin, angles, szModel, delay, seq, classID );
	return 0;
}

/*
============
CL_IsDead

Returns 1 if health is <= 0
============
*/
bool CL_IsDead()
{
	if( gHUD.m_Health.m_iHealth <= 0 )
		return true;
	return false;
}

int CHudHealth::MsgFunc_HealthInfo( const char *pszName, int iSize, void *buf )
{
	BufferReader reader( pszName, buf, iSize );

	int idx = reader.ReadByte();
	int health = reader.ReadLong();

	if ( idx < MAX_PLAYERS )
		g_PlayerExtraInfo[idx].sb_health = health;

	return 1;
}

int CHudHealth::MsgFunc_Account( const char *pszName, int iSize, void *buf )
{
	BufferReader reader( pszName, buf, iSize );

	int idx = reader.ReadByte();
	int account = reader.ReadLong();

	if ( idx < MAX_PLAYERS )
		g_PlayerExtraInfo[idx].sb_account = account;

	return 1;
}
