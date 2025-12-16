/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
// MOTD.cpp
//
// for displaying a server-sent message of the day
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "kbutton.h"
#include "triangleapi.h"
#include <string.h>
#include <stdio.h>
#include "draw_util.h"
#include "build.h"

#if XASH_WIN32 == 1 || XASH_PSVITA == 1
#define strcasestr strstr
#endif

int CHudMOTD :: Init( void )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( gHUD.m_MOTD, MOTD );

	cl_hide_motd = CVAR_CREATE("cl_hide_motd", "0", FCVAR_ARCHIVE); // hide motd
	Reset();

	return 1;
}

int CHudMOTD :: VidInit( void )
{
	// Load sprites here
	return 1;
}

void CHudMOTD :: Reset( void )
{
	m_iFlags &= ~HUD_DRAW;  // start out inactive
	m_szMOTD[0] = 0;
	m_iLines = 0;
	m_bShow = false;
	ignoreThisMotd = false;
}

#define LINE_HEIGHT  13
#define ROW_GAP  13
#define ROW_RANGE_MIN 30
#define ROW_RANGE_MAX ( ScreenHeight - 100 )
int CHudMOTD :: Draw( float fTime )
{
	gHUD.m_iNoConsolePrint &= ~( 1 << 1 );
	if( !m_bShow )
		return 1;

	if( cl_hide_motd->value )
	{
		Reset();
		return 1;
	}

	gHUD.m_iNoConsolePrint |= 1 << 1;
	// find the top of where the MOTD should be drawn,  so the whole thing is centered in the screen
	int ypos = (ScreenHeight - LINE_HEIGHT * m_iLines)/2; // shift it up slightly
	char *ch = m_szMOTD;
	int xpos = (ScreenWidth - gHUD.GetCharWidth( 'M' ) * m_iMaxLength) / 2;
	if( xpos < 30 ) xpos = 30;
	int xmax = xpos + gHUD.GetCharWidth( 'M' ) * m_iMaxLength;
	int height = LINE_HEIGHT * m_iLines;
	int ypos_r=ypos;
	if( height > ROW_RANGE_MAX )
	{
		ypos = ROW_RANGE_MIN + 7 + scroll;
		if( ypos  > ROW_RANGE_MIN + 4 )
			scroll-= (ypos - ( ROW_RANGE_MIN + 4))/3.0;
		if( ypos + height < ROW_RANGE_MAX )
			scroll+= (ROW_RANGE_MAX - (ypos + height))/ 3.0;
		ypos_r = ROW_RANGE_MIN;
		height = ROW_RANGE_MAX;
	}
	if( xmax > ScreenWidth - 30 ) xmax = ScreenWidth - 30;
	char *next_line;
	DrawUtils::DrawRectangle(xpos-5, ypos_r - 5, xmax - xpos+10, height + 10);
	while ( *ch )
	{
		int line_length = 0;  // count the length of the current line
		for ( next_line = ch; *next_line != '\n' && *next_line != 0; next_line++ )
			line_length += gHUD.GetCharWidth( (unsigned char)*next_line );
		char *top = next_line;
		if ( *top == '\n' )
			*top = 0;
		else
			top = NULL;

		// find where to start drawing the line
		if( (ypos > ROW_RANGE_MIN) && (ypos + LINE_HEIGHT <= ypos_r + height) )
			DrawUtils::DrawHudString( xpos, ypos, xmax, ch, 255, 180, 0 );

		ypos += LINE_HEIGHT;

		if ( top )  // restore 
			*top = '\n';
		ch = next_line;
		if ( *ch == '\n' )
			ch++;

		if ( ypos > (ScreenHeight - 20) )
			break;  // don't let it draw too low
	}
	
	return 1;
}

int CHudMOTD :: MsgFunc_MOTD( const char *pszName, int iSize, void *pbuf )
{
	if( cl_hide_motd->value )
		return 1;

	if ( m_iFlags & HUD_DRAW )
	{
		Reset(); // clear the current MOTD in prep for this one
	}

	if( ignoreThisMotd )
		return 1;

	BufferReader reader( pszName, pbuf, iSize );

	int is_finished = reader.ReadByte();
	strcat( m_szMOTD, reader.ReadString() );

	// we still don't support html tags in motd :(
	if( strcasestr( m_szMOTD, "<!DOCTYPE HTML>" ) )
	{
		Reset();
		ignoreThisMotd = true;
	}

	if ( is_finished )
	{
		int length = 0;
		
		m_iMaxLength = 0;
		m_iFlags |= HUD_DRAW;


		for ( char *sz = m_szMOTD; *sz != 0; sz++ )  // count the number of lines in the MOTD
		{
			if ( *sz == '\n' )
			{
				m_iLines++;
				if( length > m_iMaxLength )
				{
					m_iMaxLength = length;
					length = 0;
				}
			}
			length++;
		}
		
		m_iLines++;
		if( length > m_iMaxLength )
		{
			m_iMaxLength = length;
			length = 0;
		}
		m_bShow = true;
	}

	return 1;
}
