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

#if !USE_VGUI || USE_NOVGUI_MOTD
DECLARE_MESSAGE( m_MOTD, MOTD )
#endif

int CHudMOTD::Init( void )
{
	gHUD.AddHudElem( this );

#if !USE_VGUI || USE_NOVGUI_MOTD
	HOOK_MESSAGE( MOTD );
#endif

	m_bShow = false;

	m_iFlags &= ~HUD_ACTIVE;  // start out inactive
	m_szMOTD[0] = 0;

	return 1;
}

int CHudMOTD::VidInit( void )
{
	// Load sprites here
	return 1;
}

void CHudMOTD::Reset( void )
{
	m_iFlags &= ~HUD_ACTIVE;  // start out inactive
	m_szMOTD[0] = 0;
	m_iLines = 0;
	m_bShow = 0;
}

#define LINE_HEIGHT  (gHUD.m_scrinfo.iCharHeight)
#define ROW_GAP  13
#define ROW_RANGE_MIN 30
#define ROW_RANGE_MAX ( ScreenHeight - 100 )
int CHudMOTD::Draw( float fTime )
{
	gHUD.m_iNoConsolePrint &= ~( 1 << 1 );
	if( !m_bShow )
		return 1;
	gHUD.m_iNoConsolePrint |= 1 << 1;
	//bool bScroll;
	// find the top of where the MOTD should be drawn,  so the whole thing is centered in the screen
	int ypos = ( ScreenHeight - LINE_HEIGHT * m_iLines ) / 2; // shift it up slightly
	unsigned char *ch = (unsigned char*)m_szMOTD;
	int xpos = ( ScreenWidth - gHUD.m_scrinfo.charWidths['M'] * m_iMaxLength ) / 2;
	if( xpos < 30 )
		xpos = 30;
	int xmax = xpos + gHUD.m_scrinfo.charWidths['M'] * m_iMaxLength;
	int height = LINE_HEIGHT * m_iLines;
	int ypos_r=ypos;
	if( height > ROW_RANGE_MAX )
	{
		ypos = ROW_RANGE_MIN + 7 + scroll;
		if( ypos  > ROW_RANGE_MIN + 4 )
			scroll-= ( ypos - ( ROW_RANGE_MIN + 4 ) ) / 3.0f;
		if( ypos + height < ROW_RANGE_MAX )
			scroll+= ( ROW_RANGE_MAX - ( ypos + height ) ) / 3.0f;
		ypos_r = ROW_RANGE_MIN;
		height = ROW_RANGE_MAX;
	}
	// int ymax = ypos + height;
	if( xmax > ScreenWidth - 30 ) xmax = ScreenWidth - 30;
	gHUD.DrawDarkRectangle( xpos - 5, ypos_r - 5, xmax - xpos + 10, height + 10 );
	while( *ch )
	{
		unsigned char *next_line;
		for( next_line = ch; *next_line != '\n' && *next_line != 0; next_line++ )
			;
		// int line_length = 0;  // count the length of the current line
		// for( next_line = ch; *next_line != '\n' && *next_line != 0; next_line++ )
		//	line_length += gHUD.m_scrinfo.charWidths[*next_line];
		unsigned char *top = next_line;
		if( *top == '\n' )
			*top = 0;
		else
			top = NULL;

		// find where to start drawing the line
		if( ( ypos > ROW_RANGE_MIN ) && ( ypos + LINE_HEIGHT <= ypos_r + height ) )
			DrawUtfString( xpos, ypos, xmax, (const char*)ch, 255, 180, 0 );

		ypos += LINE_HEIGHT;

		if( top )  // restore
			*top = '\n';
		ch = next_line;
		if( *ch == '\n' )
			ch++;

		if( ypos > ( ScreenHeight - 20 ) )
			break;  // don't let it draw too low
	}

	return 1;
}

int CHudMOTD::MsgFunc_MOTD( const char *pszName, int iSize, void *pbuf )
{
	if( m_iFlags & HUD_ACTIVE )
	{
		Reset(); // clear the current MOTD in prep for this one
	}

	BEGIN_READ( pbuf, iSize );

	int is_finished = READ_BYTE();
	strlcat( m_szMOTD, READ_STRING(), sizeof( m_szMOTD ));

	if( is_finished )
	{
		int length = 0;

		m_iMaxLength = 0;
		m_iFlags |= HUD_ACTIVE;

		for( char *sz = m_szMOTD; *sz != 0; sz++ )  // count the number of lines in the MOTD
		{
			if( *sz == '\n' )
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
			// length = 0;
		}
		m_bShow = true;
	}

	return 1;
}
