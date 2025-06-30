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
// death notice
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#if USE_VGUI
#include "vgui_TeamFortressViewport.h"
#endif

DECLARE_MESSAGE( m_DeathNotice, DeathMsg )

struct DeathNoticeItem {
	char szKiller[MAX_PLAYER_NAME_LENGTH * 2];
	char szVictim[MAX_PLAYER_NAME_LENGTH * 2];
	int iId;	// the index number of the associated sprite
	int iSuicide;
	int iTeamKill;
	int iNonPlayerKill;
	float flDisplayTime;
	float *KillerColor;
	float *VictimColor;
};

#define MAX_DEATHNOTICES	4
static int DEATHNOTICE_DISPLAY_TIME = 6;

#define DEATHNOTICE_TOP		32

DeathNoticeItem rgDeathNoticeList[MAX_DEATHNOTICES + 1];

float g_ColorBlue[3]	= { 0.6, 0.8, 1.0 };
float g_ColorRed[3]	= { 1.0, 0.25, 0.25 };
float g_ColorGreen[3]	= { 0.6, 1.0, 0.6 };
float g_ColorYellow[3]	= { 1.0, 0.7, 0.0 };
float g_ColorGrey[3]	= { 0.8, 0.8, 0.8 };

float *GetClientColor( int clientIndex )
{
	switch( g_PlayerExtraInfo[clientIndex].teamnumber )
	{
	case 1:	return g_ColorBlue;
	case 2: return g_ColorRed;
	case 3: return g_ColorYellow;
	case 4: return g_ColorGreen;
	case 0: return g_ColorYellow;
	default: return g_ColorGrey;
	}

	return NULL;
}

int CHudDeathNotice::Init( void )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( DeathMsg );

	CVAR_CREATE( "hud_deathnotice_time", "6", FCVAR_ARCHIVE );

	return 1;
}

void CHudDeathNotice::InitHUDData( void )
{
	memset( rgDeathNoticeList, 0, sizeof(rgDeathNoticeList) );
}

int CHudDeathNotice::VidInit( void )
{
	m_HUD_d_skull = gHUD.GetSpriteIndex( "d_skull" );

	return 1;
}

int CHudDeathNotice::Draw( float flTime )
{
	int x, y, r, g, b;

	int gap = 20;

	const wrect_t& sprite = gHUD.GetSpriteRect(m_HUD_d_skull);
	gap = sprite.bottom - sprite.top;

	SCREENINFO screenInfo;

	screenInfo.iSize = sizeof(SCREENINFO);
	gEngfuncs.pfnGetScreenInfo(&screenInfo);
	gap = Q_max( gap, screenInfo.iCharHeight );

	for( int i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if( rgDeathNoticeList[i].iId == 0 )
			break;  // we've gone through them all

		if( rgDeathNoticeList[i].flDisplayTime < flTime )
		{
			// display time has expired
			// remove the current item from the list
			memmove( &rgDeathNoticeList[i], &rgDeathNoticeList[i + 1], sizeof(DeathNoticeItem) * ( MAX_DEATHNOTICES - i ) );
			i--;  // continue on the next item;  stop the counter getting incremented
			continue;
		}

		rgDeathNoticeList[i].flDisplayTime = Q_min( rgDeathNoticeList[i].flDisplayTime, gHUD.m_flTime + DEATHNOTICE_DISPLAY_TIME );

		// Only draw if the viewport will let me
		// vgui dropped out
#if USE_VGUI
		if( gViewPort && gViewPort->AllowedToPrintText() )
#endif
		{
			// Draw the death notice
			y = YRES( DEATHNOTICE_TOP ) + 2 + ( gap * i );  //!!!

			int id = ( rgDeathNoticeList[i].iId == -1 ) ? m_HUD_d_skull : rgDeathNoticeList[i].iId;
			x = ScreenWidth - ConsoleStringLen( rgDeathNoticeList[i].szVictim ) - ( gHUD.GetSpriteRect(id).right - gHUD.GetSpriteRect(id).left ) - 4;

			if( !rgDeathNoticeList[i].iSuicide )
			{
				x -= ( 5 + ConsoleStringLen( rgDeathNoticeList[i].szKiller ) );

				// Draw killers name
				if( rgDeathNoticeList[i].KillerColor )
					DrawSetTextColor( rgDeathNoticeList[i].KillerColor[0], rgDeathNoticeList[i].KillerColor[1], rgDeathNoticeList[i].KillerColor[2] );
				x = 5 + DrawConsoleString( x, y + 4, rgDeathNoticeList[i].szKiller );
			}

			r = 255; g = 80; b = 0;
			if( rgDeathNoticeList[i].iTeamKill )
			{
				r = 10;	g = 240; b = 10;  // display it in sickly green
			}

			// Draw death weapon
			SPR_Set( gHUD.GetSprite(id), r, g, b );
			SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect(id) );

			x += ( gHUD.GetSpriteRect(id).right - gHUD.GetSpriteRect(id).left );

			// Draw victims name (if it was a player that was killed)
			if( rgDeathNoticeList[i].iNonPlayerKill == FALSE )
			{
				if( rgDeathNoticeList[i].VictimColor )
					DrawSetTextColor( rgDeathNoticeList[i].VictimColor[0], rgDeathNoticeList[i].VictimColor[1], rgDeathNoticeList[i].VictimColor[2] );
				x = DrawConsoleString( x, y + 4, rgDeathNoticeList[i].szVictim );
			}
		}
	}

	return 1;
}

// This message handler may be better off elsewhere
int CHudDeathNotice::MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf )
{
	int i;
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ( pbuf, iSize );

	int killer = READ_BYTE();
	int victim = READ_BYTE();

	char killedwith[32];
	strcpy( killedwith, "d_" );
	strlcat( killedwith, READ_STRING(), sizeof( killedwith ));

#if USE_VGUI && !USE_NOVGUI_SCOREBOARD
	if (gViewPort)
		gViewPort->DeathMsg( killer, victim );
#else
	gHUD.m_Scoreboard.DeathMsg( killer, victim );
#endif

	gHUD.m_Spectator.DeathMessage( victim );

	for( i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if( rgDeathNoticeList[i].iId == 0 )
			break;
	}
	if( i == MAX_DEATHNOTICES )
	{
		// move the rest of the list forward to make room for this item
		memmove( rgDeathNoticeList, rgDeathNoticeList + 1, sizeof(DeathNoticeItem) * MAX_DEATHNOTICES );
		i = MAX_DEATHNOTICES - 1;
	}

	gHUD.GetAllPlayersInfo();

	// Get the Killer's name
	const char *killer_name = "";
	killer_name = g_PlayerInfoList[killer].name;
	if( !killer_name )
	{
		killer_name = "";
		rgDeathNoticeList[i].szKiller[0] = 0;
	}
	else
	{
		rgDeathNoticeList[i].KillerColor = GetClientColor( killer );
		strlcpy( rgDeathNoticeList[i].szKiller, killer_name, MAX_PLAYER_NAME_LENGTH );
	}

	// Get the Victim's name
	const char *victim_name = "";
	// If victim is -1, the killer killed a specific, non-player object (like a sentrygun)
	if( ( (signed char)victim ) != -1 )
		victim_name = g_PlayerInfoList[victim].name;
	if( !victim_name )
	{
		victim_name = "";
		rgDeathNoticeList[i].szVictim[0] = 0;
	}
	else
	{
		rgDeathNoticeList[i].VictimColor = GetClientColor( victim );
		strlcpy( rgDeathNoticeList[i].szVictim, victim_name, MAX_PLAYER_NAME_LENGTH );
	}

	// Is it a non-player object kill?
	if( ( (signed char)victim ) == -1 )
	{
		rgDeathNoticeList[i].iNonPlayerKill = TRUE;

		// Store the object's name in the Victim slot (skip the d_ bit)
		strcpy( rgDeathNoticeList[i].szVictim, killedwith + 2 );
	}
	else
	{
		if( killer == victim || killer == 0 )
			rgDeathNoticeList[i].iSuicide = TRUE;

		if( !strcmp( killedwith, "d_teammate" ) )
			rgDeathNoticeList[i].iTeamKill = TRUE;
	}

	// Find the sprite in the list
	int spr = gHUD.GetSpriteIndex( killedwith );

	rgDeathNoticeList[i].iId = spr;

	DEATHNOTICE_DISPLAY_TIME = CVAR_GET_FLOAT( "hud_deathnotice_time" );
	rgDeathNoticeList[i].flDisplayTime = gHUD.m_flTime + DEATHNOTICE_DISPLAY_TIME;

	if( rgDeathNoticeList[i].iNonPlayerKill )
	{
		ConsolePrint( rgDeathNoticeList[i].szKiller );
		ConsolePrint( " killed a " );
		ConsolePrint( rgDeathNoticeList[i].szVictim );
		ConsolePrint( "\n" );
	}
	else
	{
		// record the death notice in the console
		if( rgDeathNoticeList[i].iSuicide )
		{
			ConsolePrint( rgDeathNoticeList[i].szVictim );

			if( !strcmp( killedwith, "d_world" ) )
			{
				ConsolePrint( " died" );
			}
			else
			{
				ConsolePrint( " killed self" );
			}
		}
		else if( rgDeathNoticeList[i].iTeamKill )
		{
			ConsolePrint( rgDeathNoticeList[i].szKiller );
			ConsolePrint( " killed his teammate " );
			ConsolePrint( rgDeathNoticeList[i].szVictim );
		}
		else
		{
			ConsolePrint( rgDeathNoticeList[i].szKiller );
			ConsolePrint( " killed " );
			ConsolePrint( rgDeathNoticeList[i].szVictim );
		}

		if( *killedwith && (*killedwith > 13 ) && strcmp( killedwith, "d_world" ) && !rgDeathNoticeList[i].iTeamKill )
		{
			ConsolePrint( " with " );

			// replace the code names with the 'real' names
			if( !strcmp( killedwith + 2, "egon" ) )
				strcpy( killedwith, "d_gluon gun" );
			if( !strcmp( killedwith + 2, "gauss" ) )
				strcpy( killedwith, "d_tau cannon" );

			ConsolePrint( killedwith + 2 ); // skip over the "d_" part
		}

		ConsolePrint( "\n" );
	}

	return 1;
}
