/*
draw_util.cpp - Draw Utils
Copyright (C) 2016 a1batross

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

In addition, as a special exception, the author gives permission to
link the code of this program with the Half-Life Game Engine ("HL
Engine") and Modified Game Libraries ("MODs") developed by Valve,
L.L.C ("Valve").  You must obey the GNU General Public License in all
respects for all of the code used other than the HL Engine and MODs
from Valve.  If you modify this file, you may extend this exception
to your version of the file, but you are not obligated to do so.  If
you do not wish to do so, delete this exception statement from your
version.
*/

#include "hud.h"
#include "cl_util.h"
#include "draw_util.h"
#include "triangleapi.h"
#include <string.h>
#include <ctype.h>
#include "utflib.h"

float DrawUtils::color[3];

#define IsColorString( p )	( p && *( p ) == '^' && *(( p ) + 1) && *(( p ) + 1) >= '0' && *(( p ) + 1 ) <= '9' )
#define ColorIndex( c )	((( c ) - '0' ) & 7 )

// console color typeing
byte g_color_table[][4] =
{
{100, 100, 100, 255},	// should be black, but hud font is additive, so printing black characters is impossible
{255,   0,   0, 255},	// red
{  0, 255,   0, 255},	// green
{255, 255,   0, 255},	// yellow
{  0,   0, 255, 255},	// blue
{  0, 255, 255, 255},	// cyan
{255,   0, 255, 255},	// magenta
{240, 180,  24, 255},	// default color (can be changed by user)
};

int g_codepage = 0;
qboolean g_accept_utf8;

cvar_t *con_charset;
cvar_t *cl_charset;

/*
============================
Con_UtfProcessChar

Convert utf char to current font's single-byte encoding
============================
*/
int Con_UtfProcessCharForce( int in )
{
	// TODO: get rid of global state where possible
	static utfstate_t state = { 0 };

	uint32_t ch = Q_DecodeUTF8( &state, in );

	if( g_codepage == 1251 )
		return Q_UnicodeToCP1251( ch );
	if( g_codepage == 1252 )
		return Q_UnicodeToCP1252( ch );

	return '?'; // not implemented yet
}

int Con_UtfProcessChar( int in )
{
	if( !g_accept_utf8 ) // incoming character is not a UTF-8 sequence
		return in;

	// otherwise, decode it and convert to selected codepage
	return Con_UtfProcessCharForce( in );
}

int DrawUtils::DrawHudString( int xpos, int ypos, int iMaxX, const char *str, int r, int g, int b, float scale, bool drawing )
{
	int first_xpos = xpos;
	char *szIt = (char *)str;

	Con_UtfProcessChar( 0 );

	// draw the string until we hit the null character or a newline character
	for ( ; *szIt != 0 && *szIt != '\n'; szIt++ )
	{
		if ( *szIt == '\\' && *( szIt + 1 ) != '\n' && *( szIt + 1 ) != 0 )
		{
			// an escape character

			switch ( *( ++szIt ) )
			{
			case 'y':
				UnpackRGB( r, g, b, RGB_YELLOWISH );
				continue;
			case 'r':
				UnpackRGB( r, g, b, RGB_REDISH );
				continue;
			case 'w':
				UnpackRGB( r, g, b, RGB_WHITE );
				continue;
			case 'd':
				UnpackRGB( r, g, b, RGB_GRAY );
				continue;
			case 'R':
				//if ( drawing )
					//return xpos;
				//return DrawHudStringReverse( iMaxX, ypos, first_xpos, ++szIt, r, g, b, scale, true ); // set 'drawing' to true, to stop when '\R' is catched
				xpos = iMaxX - gHUD.GetCharWidth( 'M' ) * 10;
				++szIt;
			}
		}
		else if( IsColorString( szIt ) )
		{
			szIt++;
			if( gHUD.hud_colored->value )
			{
				r = g_color_table[ColorIndex( *szIt )][0];
				g = g_color_table[ColorIndex( *szIt )][1];
				b = g_color_table[ColorIndex( *szIt )][2];
			}
			continue;
		}

		int uch = Con_UtfProcessChar( (unsigned char)*szIt );

		//if ( !uch )
			//continue;

		int next = xpos + gHUD.GetCharWidth( uch ); // variable-width fonts look cool

		if ( next > iMaxX )
			return xpos;

		xpos += TextMessageDrawChar( xpos, ypos, ( unsigned char )*szIt, r, g, b, scale );
	}

	return xpos;
}


int DrawUtils::DrawHudStringReverse( int xpos, int ypos, int iMinX, const char *szString, int r, int g, int b, float scale, bool drawing )
{
	int first_xpos = xpos;

	// iterate through the string in reverse
	for ( signed int i = strlen( szString ); i >= 0; i-- )
	{
		if ( i > 1 )
		{
			if( szString[i - 1] == '\\' )
			{
				// an escape character

				switch ( szString[i] )
				{
				case 'y':
					UnpackRGB( r, g, b, RGB_YELLOWISH );
					break;
				case 'r':
					UnpackRGB( r, g, b, RGB_REDISH );
					break;
				case 'w':
					UnpackRGB( r, g, b, RGB_WHITE );
					break;
				case 'R':
					if ( drawing )
						return xpos;

					return DrawHudString( iMinX, ypos, first_xpos, &szString[i - 1], r, g, b, scale, true ); // set 'drawing' to true, to stop when '\R' is catched
				case 'd':
					UnpackRGB( r, g, b, RGB_GRAY );
					break;
				}
				continue;
			}
			else if( IsColorString( szString - 1 ) )
			{
				if( gHUD.hud_colored->value )
				{
					r = g_color_table[ColorIndex( *szString )][0];
					g = g_color_table[ColorIndex( *szString )][1];
					b = g_color_table[ColorIndex( *szString )][2];
				}
				i--;
				continue;
			}
		}

		int uch = Con_UtfProcessChar( (unsigned char)szString[i] );

		/*if ( !uch )
			continue;*/

		int next = xpos - gHUD.GetCharWidth( uch ); // variable-width fonts look cool

		if ( next < iMinX )
			return xpos;

		xpos = next;

		TextMessageDrawChar( xpos, ypos, (unsigned char)szString[i], r, g, b );
	}

	return xpos;
}

int DrawUtils::DrawHudNumber( int x, int y, int iFlags, int iNumber, int r, int g, int b )
{
	int iWidth = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).Width();
	int k;

	if ( iNumber > 0 )
	{
		// SPR_Draw 100's
		if ( iNumber >= 100 )
		{
			k = iNumber / 100;
			SPR_Set( gHUD.GetSprite( gHUD.m_HUD_number_0 + k ), r, g, b );
			SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( gHUD.m_HUD_number_0 + k ) );
			x += iWidth;
		}
		else if ( iFlags & ( DHN_3DIGITS ) )
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw 10's
		if ( iNumber >= 10 )
		{
			k = ( iNumber % 100 ) / 10;
			SPR_Set( gHUD.GetSprite( gHUD.m_HUD_number_0 + k ), r, g, b );
			SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( gHUD.m_HUD_number_0 + k ) );
			x += iWidth;
		}
		else if ( iFlags & ( DHN_3DIGITS | DHN_2DIGITS ) )
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones
		k = iNumber % 10;
		SPR_Set( gHUD.GetSprite( gHUD.m_HUD_number_0 + k ), r, g, b );
		SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( gHUD.m_HUD_number_0 + k ) );
		x += iWidth;
	}
	else if ( iFlags & DHN_DRAWZERO )
	{
		SPR_Set( gHUD.GetSprite( gHUD.m_HUD_number_0 ), r, g, b );

		// SPR_Draw 100's
		if ( iFlags & ( DHN_3DIGITS ) )
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		if ( iFlags & ( DHN_3DIGITS | DHN_2DIGITS ) )
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones

		SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ) );
		x += iWidth;
	}

	return x;
}

int DrawUtils::DrawHudNumber2( int x, int y, bool DrawZero, int iDigits, int iNumber, int r, int g, int b )
{
	int iWidth = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).Width();
	x += ( iDigits - 1 ) * iWidth;

	int ResX = x + iWidth;
	do
	{
		int k = iNumber % 10;
		iNumber /= 10;
		SPR_Set( gHUD.GetSprite( gHUD.m_HUD_number_0 + k ), r, g, b );
		SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( gHUD.m_HUD_number_0 + k ) );
		x -= iWidth;
		iDigits--;
	} while ( iNumber > 0 || ( iDigits > 0 && DrawZero ) );

	return ResX;
}

int DrawUtils::DrawHudNumber2( int x, int y, int iNumber, int r, int g, int b )
{
	int iWidth = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).Width();

	int iDigits = 0;
	int temp    = iNumber;
	do
	{
		iDigits++;
		temp /= 10;
	} while ( temp > 0 );

	x += ( iDigits - 1 ) * iWidth;

	int ResX = x + iWidth;
	do
	{
		int k = iNumber % 10;
		iNumber /= 10;
		SPR_Set( gHUD.GetSprite( gHUD.m_HUD_number_0 + k ), r, g, b );
		SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( gHUD.m_HUD_number_0 + k ) );
		x -= iWidth;
	} while ( iNumber > 0 );

	return ResX;
}

void DrawUtils::Draw2DQuad( float x1, float y1, float x2, float y2 )
{
	// REMOVE WHEN NANOGL BUG WILL BE FIXED
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );
	// REMOVE WHEN NANOGL BUG WILL BE FIXED

	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Vertex3f( x1, y1, 0 );

	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Vertex3f( x1, y2, 0 );

	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Vertex3f( x2, y2, 0 );

	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Vertex3f( x2, y1, 0 );

	// REMOVE WHEN NANOGL BUG WILL BE FIXED
	gEngfuncs.pTriAPI->End( );
	// REMOVE WHEN NANOGL BUG WILL BE FIXED

}

int DrawUtils::HudStringLen( const char *szIt, float scale )
{
	int l;

	Con_UtfProcessChar( 0 );

	// count length until we hit the null character or a newline character
	for ( l = 0; *szIt != 0 && *szIt != '\n'; szIt++ )
	{
		if( szIt[0] == '\\' && szIt[1] != '\n' &&
			(szIt[1] == 'y' || szIt[1] == 'w' || szIt[1] == 'd' || szIt[1] == 'R') ) // not sure is reversing handled correctly
		{
			szIt++;
			continue;
		}

		if( szIt[0] == '^' && isdigit( szIt[1] ) ) // suck down, unreadable nicknames. Check even if hud_colored is off
		{
			szIt++;
			continue;
		}

		int uch = Con_UtfProcessChar( (unsigned char)*szIt );

		if ( !uch )
			continue;

		l += gHUD.GetCharWidth( uch );
	}

	return l;
}
