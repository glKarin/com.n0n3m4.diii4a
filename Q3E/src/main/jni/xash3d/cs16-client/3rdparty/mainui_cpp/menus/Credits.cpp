/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "Framework.h"
#include "Bitmap.h"

#define UI_CREDITS_PATH		"credits.txt"
#define UI_CREDITS_MAXLINES		2048

static const char *uiCreditsDefault[] = 
{
	"",
	"Copyright XashXT Group 2017 (C)",
	"Copyright Flying With Gauss 2017 (C)",
	0
};

class CMenuCredits : public CMenuBaseWindow
{
public:
	CMenuCredits() : CMenuBaseWindow( "Credits" )
	{
		credits = NULL;
		finalCredits = false;
		active = false;
		buffer = NULL;
		numLines = 0;
		memset( index, 0, sizeof( index ));
	}
	~CMenuCredits() override;

	void Draw() override;
	bool KeyUp( int key ) override;
	bool KeyDown( int key ) override;
	bool DrawAnimation() override { return true; }
	void Show() override;

	friend void UI_DrawFinalCredits( void );
	friend void UI_FinalCredits( void );
	friend int UI_CreditsActive( void );

private:
	void _Init() override;
	void Reload() override;

	const char	**credits;
	int		startTime;
	int		showTime;
	int		fadeTime;
	int		numLines;
	bool		active;
	bool		finalCredits;
	char		*index[UI_CREDITS_MAXLINES];
	char		*buffer;
};

CMenuCredits::~CMenuCredits()
{
}

void CMenuCredits::Show()
{
	CMenuBaseWindow::Show();
	EngFuncs::KEY_SetDest( KEY_GAME );
}

/*
=================
CMenuCredits::Draw
=================
*/
void CMenuCredits::Draw( void )
{
	int	i, y;
	float	speed;
	int	h = UI_MED_CHAR_HEIGHT;
	int	color = 0;

	speed = 32.0f * ( 768.0f / ScreenHeight );	// syncronize with final background track :-)

	// now draw the credits
	UI_ScaleCoords( NULL, NULL, NULL, &h );

	y = ScreenHeight - (((gpGlobals->time * 1000) - startTime ) / speed );

	// draw the credits
	for ( i = 0; i < numLines && credits[i]; i++, y += h )
	{
		// skip not visible lines, but always draw end line
		if( y <= -h && i != numLines - 1 ) continue;

		if(( y < ( ScreenHeight - h ) / 2 ) && i == numLines - 1 )
		{
			if( !fadeTime ) fadeTime = (gpGlobals->time * 1000);
			color = UI_FadeAlpha( fadeTime, showTime );
			if( UnpackAlpha( color ))
				UI_DrawString( uiStatic.hDefaultFont, 0, ( ScreenHeight - h ) / 2, ScreenWidth, h, credits[i], color, h, QM_CENTER, ETF_SHADOW | ETF_FORCECOL );
		}
		else UI_DrawString( uiStatic.hDefaultFont, 0, y, ScreenWidth, h, credits[i], uiColorWhite, h, QM_CENTER, ETF_SHADOW );
	}

	if( y < 0 && UnpackAlpha( color ) == 0 )
	{
		active = false; // end of credits
		if( finalCredits )
			EngFuncs::HostEndGame( gMenu.m_gameinfo.title );
	}

	if( !active && !finalCredits ) // for final credits we don't show the window, just drawing
		Hide();
}

bool CMenuCredits::KeyUp( int key )
{
	return true;
}

/*
=================
CMenuCredits::Key
=================
*/
bool CMenuCredits::KeyDown( int key )
{
	// final credits can't be intterupted
	if( !finalCredits )
		active = false;

	return true;
}

/*
=================
CMenuCredits::_Init
=================
*/
void CMenuCredits::_Init( void )
{
	if( !buffer )
	{
		int	count;
		char	*p;

		// load credits if needed
		buffer = (char *)EngFuncs::COM_LoadFile( UI_CREDITS_PATH, &count );
		if( count )
		{
			if( buffer[count - 1] != '\n' && buffer[count - 1] != '\r' )
			{
				char *tmp = new char[count + 2];
				memcpy( tmp, buffer, count );
				EngFuncs::COM_FreeFile( buffer );
				buffer = tmp;
				Q_strncpy( buffer + count, "\r", 2 ); // add terminator
				count += 2; // added "\r\0"
			}
			p = buffer;

			// convert customs credits to 'ideal' strings array
			for ( numLines = 0; numLines < UI_CREDITS_MAXLINES; numLines++ )
			{
				index[numLines] = p;
				while ( *p != '\r' && *p != '\n' )
				{
					p++;
					if ( --count == 0 )
						break;
				}

				if ( *p == '\r' )
				{
					*p++ = 0;
					if( --count == 0 ) break;
				}

				*p++ = 0;
				if( --count == 0 ) break;
			}
			index[++numLines] = 0;
			credits = (const char **)index;
		}
		else
		{
			// use built-in credits
			credits =  uiCreditsDefault;
			numLines = ( sizeof( uiCreditsDefault ) / sizeof( uiCreditsDefault[0] )) - 1; // skip term
		}
	}
}

void CMenuCredits::Reload( void )
{
	// run credits
	startTime = (gpGlobals->time * 1000) + 500; // make half-seconds delay
	showTime = bound( 1000, strlen( credits[numLines - 1]) * 1000, 10000 );
	fadeTime = 0; // will be determined later
	active = true;

	CMenuBaseWindow::Reload();
}

ADD_MENU3( menu_credits, CMenuCredits, UI_FinalCredits );

void UI_DrawFinalCredits( void )
{
	if( UI_CreditsActive() )
		menu_credits->Draw ();
}

int UI_CreditsActive( void )
{
	return menu_credits->active && menu_credits->finalCredits;
}

void UI_FinalCredits( void )
{
	menu_credits->Init();
	menu_credits->VidInit();
	menu_credits->Reload(); // take a chance to reload info for items

	menu_credits->active = true;
	menu_credits->finalCredits = true;
	// don't create a window
	// menu_credits->Show();
}
