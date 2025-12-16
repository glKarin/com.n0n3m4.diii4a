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
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "rain.h"
#include "com_model.h"
#include "studio.h"
#include "studio_util.h"
#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"
#include "com_weapons.h"

#include <cstring>

#include "events.h"

#define MAX_CLIENTS 32

extern float g_flRoundTime;

/// USER-DEFINED SERVER MESSAGE HANDLERS

int CHud :: MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf )
{
	// clear all hud data
	HUDLIST *pList = m_pHudList;

	while ( pList )
	{
		if ( pList->p )
			pList->p->Reset();
		pList = pList->pNext;
	}

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	char szMapPrefix[64] = { 0 };
	char szMapName[64] = { 0 };
	const char *szFullMapName = gEngfuncs.pfnGetLevelName();
	if ( szFullMapName && szFullMapName[0] )
	{
		strncpy( szMapName, szFullMapName + 5, sizeof( szMapName ) );
		szMapName[strlen( szMapName ) - 4] = '\0';

		int i = 0;
		while ( szMapName[i] != '_' && szMapName[i] != '\0' && i < sizeof( szMapPrefix ) - 1 )
		{
			szMapPrefix[i] = szMapName[i];
			i++;
		}
		szMapPrefix[i] = '_';
		szMapPrefix[i + 1] = '\0';
	}
	gEngfuncs.Cvar_Set( gHUD.cscl_currentmap->name, szMapName );
	gEngfuncs.Cvar_Set( gHUD.cscl_mapprefix->name, szMapPrefix );

	// reinitialize models. We assume that server already precached all models.
	// NOTE: we're doing this in ResetHUD instead of InitHUD because it's not being
	// sent in demos, as it's not part of the signon packet and it's not sent with
	// fullupdate. Doing this in InitHUD essentially caches invalid model indices
	g_iRShell       = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/rshell.mdl" );
	g_iPShell       = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/pshell.mdl" );
	g_iShotgunShell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shotgunshell.mdl" );
	
	return 1;
}

void CAM_ToFirstPerson(void);

int CHud :: MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	CAM_ToFirstPerson();
	return 1;
}

int CHud :: MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while (pList)
	{
		if ( pList->p )
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

	g_iFreezeTimeOver = 0;

	g_FogParameters.density = 0.0f;
	g_FogParameters.affectsSkyBox = 0;
	g_FogParameters.color[0] = 0.0f;
	g_FogParameters.color[1] = 0.0f;
	g_FogParameters.color[2] = 0.0f;

	if( cl_fog_density )
		gEngfuncs.Cvar_SetValue( cl_fog_density->name, 0.0f );
	
	if( cl_fog_r )
		gEngfuncs.Cvar_SetValue( cl_fog_r->name, g_FogParameters.color[0] );
	
	if( cl_fog_g )
		gEngfuncs.Cvar_SetValue( cl_fog_g->name, g_FogParameters.color[1] );
	
	if( cl_fog_b )
		gEngfuncs.Cvar_SetValue( cl_fog_b->name, g_FogParameters.color[2] );

	memset( g_PlayerExtraInfo, 0, sizeof(g_PlayerExtraInfo) );

	ResetRain();

	// reset round time
	g_flRoundTime   = 0.0f;

	return 1;
}


int CHud :: MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	m_Teamplay = reader.ReadByte();

	return 1;
}

int CHud :: MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	m_iConcussionEffect = reader.ReadByte();
	if (m_iConcussionEffect)
		this->m_StatusIcons.EnableIcon("dmg_concuss",255,160,0);
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return 1;
}

int CHud::MsgFunc_ShadowIdx(const char *pszName, int iSize, void *pbuf)
{
	BufferReader reader( pszName, pbuf, iSize );

	int idx = reader.ReadLong();
	g_StudioRenderer.StudioSetShadowSprite(idx);
	return 1;
}

int CHud::MsgFunc_ServerName( const char *name, int size, void *buf )
{
	BufferReader reader( name, buf, size );
	strncpy( gHUD.m_szServerName, reader.ReadString(), 64 );
	gHUD.m_szServerName[63] = 0;
	return 1;
}

int CHud::MsgFunc_Fog( const char *pszName, int iSize, void *pbuf )
{
	//int flags;

	memset( &g_FogParameters, 0, sizeof(FogParameters));

	BufferReader reader( pszName, pbuf, iSize );

	g_FogParameters.affectsSkyBox = false;
	g_FogParameters.color[0] = reader.ReadByte();
	g_FogParameters.color[1] = reader.ReadByte();
	g_FogParameters.color[2] = reader.ReadByte();

	union
	{
		float f;
		char b[4];
	} density;

	density.b[0] = reader.ReadByte();
	density.b[1] = reader.ReadByte();
	density.b[2] = reader.ReadByte();
	density.b[3] = reader.ReadByte();

	g_FogParameters.density = density.f;

	if( cl_fog_density )
		gEngfuncs.Cvar_SetValue( cl_fog_density->name, g_FogParameters.density );
	
	if( cl_fog_r )
		gEngfuncs.Cvar_SetValue( cl_fog_r->name, g_FogParameters.color[0] );
	
	if( cl_fog_g )
		gEngfuncs.Cvar_SetValue( cl_fog_g->name, g_FogParameters.color[1] );
	
	if( cl_fog_b )
		gEngfuncs.Cvar_SetValue( cl_fog_b->name, g_FogParameters.color[2] );
	
	return 1;
}
