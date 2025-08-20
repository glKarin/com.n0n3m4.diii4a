/*
ui_menu.c -- main menu interface
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2017 a1batross

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
#include "extdll_menu.h"
#include "BaseMenu.h"
#include "PicButton.h"
#include "keydefs.h"
#include "Utils.h"
#include "BtnsBMPTable.h"
#include "YesNoMessageBox.h"
#include "BackgroundBitmap.h"
#include "FontManager.h"
#include "cursor_type.h"

cvar_t		*ui_showmodels;
cvar_t		*ui_show_window_stack;
cvar_t		*ui_borderclip;
cvar_t		*ui_language;
cvar_t		*ui_menu_style;

uiStatic_t	uiStatic;
static CMenuEntry	*s_pEntries = NULL;

const char	*uiSoundOldPrefix	= "media/";
const char	*uiSoundNewPrefix	= "sound/common/";
const char	*uiSounds[] = {
	"media/launch_upmenu1.wav",
	"media/launch_dnmenu1.wav",
	"media/launch_select2.wav",
	"",
	"media/launch_glow1.wav",
	"media/launch_deny2.wav",
	"media/launch_select1.wav",
	"media/launch_deny1.wav",
	"",
	""
};

// they match default WON colors.lst now, except alpha
// a1ba: made uiColorHelp brighter so it's easier to read
unsigned int uiColorHelp        = 0xFFA0A0A0; // 160, 160, 160, 255 // hint letters color
unsigned int uiPromptBgColor    = 0xFF383838; // 56,  56,  56,  255 // dialog background color
unsigned int uiPromptTextColor  = 0xFFF0B418; // 240, 180, 24,  255 // dialog or button letters color
unsigned int uiPromptFocusColor = 0xFFFFFF00; // 255, 255,  0,  255 // dialog or button focus letters color
unsigned int uiInputTextColor   = 0xFFF0B418; // 240, 180, 24,  255
unsigned int uiInputBgColor     = 0x80383838; // 56,  56,  56,  128 // field, scrollist, checkbox background color
unsigned int uiInputFgColor     = 0xFF555555; // 85,  85,  85,  255 // field, scrollist, checkbox foreground color
unsigned int uiColorWhite       = 0xFFFFFFFF; // 255, 255, 255, 255 // useful for bitmaps
unsigned int uiColorDkGrey      = 0x80404040; // 64,  64,  64,  128 // shadow and grayed items
unsigned int uiColorBlack       = 0x80000000; //  0,   0,   0,  128 // some controls background
unsigned int uiColorConsole     = 0xFFF0B418; // just for reference

// color presets (this is nasty hack to allow color presets to part of text)
const unsigned int g_iColorTable[8] =
{
0xFF000000, // black
0xFFFF0000, // red
0xFF00FF00, // green
0xFFFFFF00, // yellow
0xFF0000FF, // blue
0xFF00FFFF, // cyan
0xFFF0B418, // dialog or button letters color
0xFFFFFFFF, // white
};

CMenuEntry::CMenuEntry(const char *cmd, void (*pfnPrecache)(), void (*pfnShow)(), void (*pfnShutdown)() ) :
	m_szCommand( cmd ),
	m_pfnPrecache( pfnPrecache ),
	m_pfnShow( pfnShow ),
	m_pfnShutdown( pfnShutdown ),
	m_pNext( s_pEntries )
{
	s_pEntries = this;
}

/*
=================
UI_ScaleCoords

Any parameter can be NULL if you don't want it
=================
*/
void UI_ScaleCoords( int *x, int *y, int *w, int *h )
{
	if( x ) *x *= uiStatic.scaleX;
	if( y ) *y *= uiStatic.scaleY;
	if( w ) *w *= uiStatic.scaleX;
	if( h ) *h *= uiStatic.scaleY;
}

void UI_ScaleCoords( int &x, int &y )
{
	x *= uiStatic.scaleX;
	y *= uiStatic.scaleY;
}

void UI_ScaleCoords( int &x, int &y, int &w, int &h )
{
	UI_ScaleCoords( x, y );
	UI_ScaleCoords( w, h );
}

Point Point::Scale()
{
	return Point( x * uiStatic.scaleX, y * uiStatic.scaleY );
}

Size Size::Scale()
{
	return Size( w * uiStatic.scaleX, h * uiStatic.scaleY );
}

/*
=================
UI_CursorInRect
=================
*/
bool UI_CursorInRect( int x, int y, int w, int h )
{
	if( uiStatic.cursorX < x || uiStatic.cursorX > x + w )
		return false;
	if( uiStatic.cursorY < y || uiStatic.cursorY > y + h )
		return false;
	return true;
}

/*
=================
UI_EnableAlphaFactor
=================
*/
void UI_EnableAlphaFactor( float a )
{
	uiStatic.enableAlphaFactor = true;
	uiStatic.alphaFactor = bound( 0.0f, a, 1.0f );
}

/*
=================
UI_DisableAlphaFactor
=================
*/
void UI_DisableAlphaFactor()
{
	uiStatic.enableAlphaFactor = false;
}

/*
=================
UI_DrawPic
=================
*/
void UI_DrawPic( int x, int y, int width, int height, const unsigned int color, CImage &pic, const ERenderMode eRenderMode )
{
	HIMAGE hPic = pic.Handle();

	if( !hPic )
		return;

	int r, g, b, a;
	UnpackRGBA( r, g, b, a, color );

	EngFuncs::PIC_Set( hPic, r, g, b, a );
	switch( eRenderMode )
	{
	case QM_DRAWNORMAL:
		if( !uiStatic.enableAlphaFactor )
		{
			EngFuncs::PIC_Draw( x, y, width, height );
			break;
		}
		// intentional fallthrough
	case QM_DRAWTRANS:
		EngFuncs::PIC_DrawTrans( x, y, width, height );
		break;
	case QM_DRAWADDITIVE:
		EngFuncs::PIC_DrawAdditive( x, y, width, height );
		break;
	case QM_DRAWHOLES:
		EngFuncs::PIC_DrawHoles( x, y, width, height );
		break;
	}
}

/*
=================
UI_FillRect
=================
*/
void UI_FillRect( int x, int y, int width, int height, const unsigned int color )
{
	int r, g, b, a;
	UnpackRGBA( r, g, b, a, color );

	EngFuncs::FillRGBA( x, y, width, height, r, g, b, a );
}

/*
=================
UI_DrawRectangleExt
=================
*/
void UI_DrawRectangleExt( int in_x, int in_y, int in_w, int in_h, const unsigned int color, int outlineWidth, int flag )
{
	int	x, y, w, h;

	if( outlineWidth == 0 ) outlineWidth = uiStatic.outlineWidth;

	if( flag & QM_LEFT )
	{
		x = in_x - outlineWidth;
		y = in_y - outlineWidth;
		w = outlineWidth;
		h = in_h + outlineWidth + outlineWidth;

		// draw left
		UI_FillRect( x, y, w, h, color );
	}

	if( flag & QM_RIGHT )
	{
		x = in_x + in_w;
		y = in_y - outlineWidth;
		w = outlineWidth;
		h = in_h + outlineWidth + outlineWidth;

		// draw right
		UI_FillRect( x, y, w, h, color );
	}

	if( flag & QM_TOP )
	{
		x = in_x;
		y = in_y - outlineWidth;
		w = in_w;
		h = outlineWidth;

		// draw top
		UI_FillRect( x, y, w, h, color );
	}

	if( flag & QM_BOTTOM )
	{
		// draw bottom
		x = in_x;
		y = in_y + in_h;
		w = in_w;
		h = outlineWidth;

		UI_FillRect( x, y, w, h, color );
	}
}

/*
=================
UI_DrawString
=================
*/
int UI_DrawString( HFont font, int x, int y, int w, int h,
		const char *string, const unsigned int color,
		int charH, uint justify, uint flags )
{
	uint	modulate, shadowModulate = 0;
	int	xx = 0, yy, shadowOffset = 0, ch;
	int maxX = x;

	if( !string || !string[0] )
		return x;

	if( flags & ETF_SHADOW )
	{
		shadowModulate = PackAlpha( uiColorBlack, UnpackAlpha( color ));
		shadowOffset = Q_max( 1, uiStatic.scaleX * 3 );
	}

	modulate = color;

	if( justify & QM_TOP )
	{
		yy = y;
		h -= h % charH;
	}
	else if( justify & QM_BOTTOM )
	{
		yy = y + h - charH;
		h -= h % charH;
	}
	else
	{
		yy = y + (h - charH)/2;
		h -= charH;
	}

	if( flags & ETF_NO_WRAP )
		h = charH;

	int i = 0;
	int ellipsisWide = g_FontMgr->GetEllipsisWide( font );
	bool giveup = false;

	while( string[i] && !giveup )
	{
		char line[1024], *l;
		int j = i, len = 0;
		int pixelWide = 0;
		int save_pixelWide = 0;
		int save_j = 0;

		EngFuncs::UtfProcessChar( 0 );
		while( string[j] )
		{
			if( string[j] == '\n' )
			{
				j++;
				break;
			}

			if( len == sizeof( line ) - 1 )
				break;

			line[len] = string[j];

			int uch = EngFuncs::UtfProcessChar( ( unsigned char )string[j] );

			if( IsColorString( string + j )) // don't calc wides for colorstrings
			{
				line[len+1] = string[j+1];
				len += 2;
				j += 2;
			}
			else if( !uch ) // don't calc wides for invalid codepoints
			{
				len++;
				j++;
			}
			else
			{
				int charWide;

				// does we have free space for new line?
				if( yy < (y + h) - charH )
				{
					if( uch == ' ' && pixelWide < w ) // remember last whitespace
					{
						save_pixelWide = pixelWide;
						save_j = j;
					}
				}
				else
				{
					// remember last position, when we still fit
					if( pixelWide + ellipsisWide < w && j > 0 )
					{
						save_pixelWide = pixelWide;
						save_j = j;
					}
				}

				charWide = g_FontMgr->GetCharacterWidthScaled( font, uch, charH );

				if( !(flags & ETF_NOSIZELIMIT) && pixelWide + charWide > w )
				{
					// do we have free space for new line?
					if( yy < (y + h) - charH )
					{
						// try to word wrap
						if( save_j != 0 && save_pixelWide != 0 )
						{
							pixelWide = save_pixelWide;
							len -= j - save_j; // skip whitespace
							j = save_j + 1; // skip whitespace
						}

						break;
					}
					else
					{

						if( save_j != 0 && save_pixelWide != 0 )
						{
							pixelWide = save_pixelWide;
							len -= j - save_j;
							j = save_j;


							if( len > 0 )
							{
								line[len] = '.';
								line[len+1] = '.';
								line[len+2] = '.';
								len += 3;
							}
						}

						// we don't have free space anymore, so just stop drawing
						giveup = true;

						break;
					}
				}
				else
				{
					pixelWide += charWide;
					j++;
					len++;
				}
			}
		}
		line[len] = 0;

		// align the text as appropriate
		if( justify & QM_LEFT  )
		{
			xx = x;
		}
		else if( justify & QM_RIGHT )
		{
			xx = x + (w - pixelWide);
		}
		else // QM_LEFT
		{
			xx = x + (w - pixelWide) / 2.0f;
		}

		// draw it
		l = line;
		EngFuncs::UtfProcessChar( 0 );
		while( *l )
		{
			if( IsColorString( l ))
			{
				int colorNum = ColorIndex( *(l+1) );

				if( colorNum == 7 && color != 0 )
				{
					modulate = color;
				}
				else if( !(flags & ETF_FORCECOL) )
				{
					modulate = PackAlpha( g_iColorTable[colorNum], UnpackAlpha( color ));
				}

				l += 2;
				continue;
			}

			ch = *l++;
			ch &= 255;

// when using custom font render, we use utf-8
			ch = EngFuncs::UtfProcessChar( (unsigned char) ch );
			if( !ch )
				continue;

			if( flags & ETF_SHADOW )
				g_FontMgr->DrawCharacter( font, ch, Point( xx + shadowOffset, yy + shadowOffset ), charH, shadowModulate, flags & ETF_ADDITIVE );

#ifdef DEBUG_WHITESPACE
			if( ch == ' ' )
			{
				g_FontMgr->DrawCharacter( font, '_', Point( xx, yy ), charH, modulate, flags & ETF_ADDITIVE );
				xx += g_FontMgr->GetCharacterWidthScaled( font, ch, charH );
				continue;
			}
#endif

			xx += g_FontMgr->DrawCharacter( font, ch, Point( xx, yy ), charH, modulate, flags & ETF_ADDITIVE );

			maxX = Q_max( xx, maxX );
		}
		yy += charH;

		i = j;
	}

	EngFuncs::UtfProcessChar( 0 );

	return maxX;
}

/*
=================
UI_DrawMouseCursor
=================
*/
void UI_DrawMouseCursor( void )
{
	CMenuBaseItem	*item;
	void *hCursor = (void *)dc_arrow;

#if 0
	if( !UI_IsXashFWGS( ))
	{
#ifdef _WIN32
		EngFuncs::SetCursor((HICON)LoadCursor( NULL, (LPCTSTR)OCR_NORMAL ));
#endif // _WIN32
		return;
	}
#endif // 0

	int cursor = uiStatic.menu.Current()->GetCursor();
	item = uiStatic.menu.Current()->GetItemByIndex( cursor );

	if( item && FBitSet( item->iFlags, QMF_HASMOUSEFOCUS )) 	// fast approach
	{
		if( FBitSet( item->iFlags, QMF_GRAYED ))
			hCursor = (void *)dc_no;
		else hCursor = (void *)item->CursorAction();
	}
	else
	{
		FOR_EACH_VEC( uiStatic.menu.Current()->m_pItems, i )
		{
			item = (CMenuBaseItem *)uiStatic.menu.Current()->GetItemByIndex(cursor);

			if( !item->IsVisible( ))
				continue;

			if( !FBitSet( item->iFlags, QMF_HASMOUSEFOCUS ))
				continue;

			if( FBitSet( item->iFlags, QMF_GRAYED ))
				hCursor = (void *)dc_no;
			else hCursor = (void *)item->CursorAction();

			break;
		}
	}

	EngFuncs::SetCursor( hCursor );
}

// =====================================================================

/*
=================
UI_StartBackGroundMap
=================
*/
bool UI_StartBackGroundMap( void )
{
	static bool	first = TRUE;

	if( !first ) return FALSE;

	first = FALSE;

	// some map is already running
	if( uiStatic.bgmaps.IsEmpty() || CL_IsActive() || gpGlobals->demoplayback )
		return FALSE;

	int bgmapid = EngFuncs::RandomLong( 0, uiStatic.bgmaps.Count() - 1 );

	char cmd[128];
	snprintf( cmd, sizeof( cmd ), "maps/%s.bsp", uiStatic.bgmaps[bgmapid].Get( ));
	if( !EngFuncs::FileExists( cmd, TRUE )) return FALSE;

	snprintf( cmd, sizeof( cmd ), "map_background %s\n", uiStatic.bgmaps[bgmapid].Get( ));
	EngFuncs::ClientCmd( FALSE, cmd );

	return TRUE;
}

// =====================================================================

/*
=================
UI_CloseMenu
=================
*/
void UI_CloseMenu( void )
{
	uiStatic.menu.Clean();

//	EngFuncs::KEY_ClearStates ();
	if( !uiStatic.client.IsActive() )
		EngFuncs::KEY_SetDest( KEY_GAME );
}

// =====================================================================


/*
=================
UI_UpdateMenu
=================
*/
void UI_UpdateMenu( float flTime )
{
	if( !uiStatic.initialized )
		return;

	static bool loadStuff = true;

	// can't do this in Init, since these are dependent on cvar values
	// set from user configs
	if( loadStuff )
	{
		// load localized strings
		UI_LoadCustomStrings();

		// load scr
		UI_LoadScriptConfig();

		// load background track
		if( !CL_IsActive( ))
			EngFuncs::PlayBackgroundTrack( "media/gamestartup", "media/gamestartup" );

		loadStuff = false;
	}

	UI_DrawFinalCredits ();

	// also moved opening main menu here from SetActiveMenu, so
	// translation strings could will be loaded at this moment
	if( uiStatic.nextFrameActive )
	{
		if( !uiStatic.menu.IsActive() )
			UI_Main_Menu();

		uiStatic.nextFrameActive = false;
	}

	// advance global time
	uiStatic.realTime = flTime * 1000;

	// let's use engine credits "feature" for drawing client windows
	if( uiStatic.client.IsActive( ))
		uiStatic.client.Update();

	if( !uiStatic.menu.IsActive( ))
		return;

	if( !EngFuncs::ClientInGame() && EngFuncs::GetCvarFloat( "cl_background" ) != 0.0f )
		return;	// don't draw menu while level is loading

	if( uiStatic.firstDraw )
	{
		// we loading background so skip SCR_Update
		if( UI_StartBackGroundMap( ))
			return;

		uiStatic.firstDraw = false;
	}

	// draw cursor
	UI_DrawMouseCursor();

	// background doesn't need to be drawn with transparency
	bool enableAlphaFactor = uiStatic.enableAlphaFactor;
	uiStatic.enableAlphaFactor = false;
	uiStatic.background->Draw();
	uiStatic.enableAlphaFactor = enableAlphaFactor;

	uiStatic.menu.Update();
}

/*
=================
UI_KeyEvent
=================
*/
void UI_KeyEvent( int key, int down )
{
	bool clientActive, menuActive;

	if( !uiStatic.initialized )
		return;

	if( key == K_MOUSE1 )
	{
		g_bCursorDown = !!down;
	}

	clientActive = uiStatic.client.IsActive();
	menuActive = uiStatic.menu.IsActive();

	if( clientActive && !menuActive )
		down ? uiStatic.client.KeyDownEvent( key ) :
			uiStatic.client.KeyUpEvent( key );

	if( menuActive )
		down ? uiStatic.menu.KeyDownEvent( key ) :
			uiStatic.menu.KeyUpEvent( key );
}

/*
=================
UI_CharEvent
=================
*/
void UI_CharEvent( int key )
{
	if( !uiStatic.initialized )
		return;

	bool clientActive = uiStatic.client.IsActive();
	bool menuActive = uiStatic.menu.IsActive();

	if( clientActive && !menuActive )
		uiStatic.client.CharEvent( key );

	if( menuActive )
		uiStatic.menu.CharEvent( key );
}

bool g_bCursorDown;
float cursorDY;

/*
=================
UI_MouseMove
=================
*/
void UI_MouseMove( int x, int y )
{
	bool clientActive, menuActive;

	if( !uiStatic.initialized )
		return;

	clientActive = uiStatic.client.IsActive();
	menuActive = uiStatic.menu.IsActive();

	if( !clientActive && !menuActive )
		return;

	if( uiStatic.cursorX == x && uiStatic.cursorY == y )
		return;

	if( g_bCursorDown )
	{
		static bool prevDown = false;

		if( !prevDown )
		{
			prevDown = true;
			cursorDY = 0;
		}
		else if( y - uiStatic.cursorY )
		{
			cursorDY += y - uiStatic.cursorY;
		}
	}
	else
		cursorDY = 0;
	//Con_Printf("%d %d %f\n",x, y, cursorDY);

	// now menu uses absolute coordinates
	uiStatic.cursorX = x;
	uiStatic.cursorY = y;

	if( UI_CursorInRect( 1, 1, ScreenWidth - 1, ScreenHeight - 1 ))
		uiStatic.mouseInRect = true;
	else uiStatic.mouseInRect = false;

	uiStatic.cursorX = bound( 0, uiStatic.cursorX, ScreenWidth );
	uiStatic.cursorY = bound( 0, uiStatic.cursorY, ScreenHeight );

	if( clientActive && !menuActive )
		uiStatic.client.MouseEvent( x, y );

	if( menuActive )
		uiStatic.menu.MouseEvent( x, y );
}


/*
=================
UI_SetActiveMenu
=================
*/
void UI_SetActiveMenu( int fActive )
{
	if( !uiStatic.initialized )
		return;

	// don't continue firing if we leave game
	EngFuncs::KEY_ClearStates();

	if( fActive )
	{
		EngFuncs::KEY_SetDest( KEY_MENU );
		uiStatic.nextFrameActive = true; // main menu open moved to UI_UpdateMenu
	}
	else
	{
		UI_CloseMenu();
		uiStatic.nextFrameActive = false; // don't call main menu next frame
	}
}

/*
=================
UI_IsVisible

Some systems may need to know if it is visible or not
=================
*/
int UI_IsVisible( void )
{
	if( !uiStatic.initialized )
		return false;
	return uiStatic.menu.IsActive();
}

void UI_GetCursorPos( int *pos_x, int *pos_y )
{
	if( pos_x ) *pos_x = uiStatic.cursorX;
	if( pos_y ) *pos_y = uiStatic.cursorY;
}

// dead callback
void UI_SetCursorPos( int pos_x, int pos_y )
{
	(void)(pos_x);
	(void)(pos_y);
	uiStatic.mouseInRect = true;
}

void UI_ShowCursor( int show )
{
	uiStatic.hideCursor = (show) ? false : true;
}

int UI_MouseInRect( void )
{
	return uiStatic.mouseInRect;
}

/*
=================
UI_Precache
=================
*/
void UI_Precache( void )
{
	if( !uiStatic.initialized )
		return;

	EngFuncs::PIC_Load( UI_LEFTARROW );
	EngFuncs::PIC_Load( UI_LEFTARROWFOCUS );
	EngFuncs::PIC_Load( UI_RIGHTARROW );
	EngFuncs::PIC_Load( UI_RIGHTARROWFOCUS );
	EngFuncs::PIC_Load( UI_UPARROW );
	EngFuncs::PIC_Load( UI_UPARROWFOCUS );
	EngFuncs::PIC_Load( UI_DOWNARROW );
	EngFuncs::PIC_Load( UI_DOWNARROWFOCUS );
	EngFuncs::PIC_Load( "gfx/shell/splash" );

	for( CMenuEntry *entry = s_pEntries; entry; entry = entry->m_pNext )
	{
		if( entry->m_pfnPrecache )
			entry->m_pfnPrecache();
	}
}

void UI_ParseColor( char *&pfile, unsigned int *outColor )
{
	int color[3] = { 0xFF, 0xFF, 0xFF };
	char token[1024];

	for( int i = 0; i < 3; i++ )
	{
		pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ));
		if( !pfile ) break;
		color[i] = atoi( token );
	}

	*outColor = PackRGB( color[0], color[1], color[2] );
}

void UI_ApplyCustomColors( void )
{
	char *afile = (char *)EngFuncs::COM_LoadFile( "gfx/shell/colors.lst" );
	char *pfile = afile;
	char token[1024];

	if( !afile )
	{
		// not error, not warning, just notify
		Con_Printf( "UI_ApplyCustomColors: colors.lst not found\n" );
		return;
	}

	while(( pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ))) != NULL )
	{
		if( !stricmp( token, "HELP_COLOR" ))
		{
			UI_ParseColor( pfile, &uiColorHelp );
		}
		else if( !stricmp( token, "PROMPT_BG_COLOR" ))
		{
			UI_ParseColor( pfile, &uiPromptBgColor );
		}
		else if( !stricmp( token, "PROMPT_TEXT_COLOR" ))
		{
			UI_ParseColor( pfile, &uiPromptTextColor );
		}
		else if( !stricmp( token, "PROMPT_FOCUS_COLOR" ))
		{
			UI_ParseColor( pfile, &uiPromptFocusColor );
		}
		else if( !stricmp( token, "INPUT_TEXT_COLOR" ))
		{
			UI_ParseColor( pfile, &uiInputTextColor );
		}
		else if( !stricmp( token, "INPUT_BG_COLOR" ))
		{
			UI_ParseColor( pfile, &uiInputBgColor );
		}
		else if( !stricmp( token, "INPUT_FG_COLOR" ))
		{
			UI_ParseColor( pfile, &uiInputFgColor );
		}
		else if( !stricmp( token, "CON_TEXT_COLOR" ))
		{
			UI_ParseColor( pfile, &uiColorConsole );
		}
	}

	int	r, g, b;

	UnpackRGB( r, g, b, uiColorConsole );
	EngFuncs::SetConsoleDefaultColor( r, g, b );

	EngFuncs::COM_FreeFile( afile );
}

static void UI_LoadBackgroundMapList( void )
{
	if( !EngFuncs::FileExists( "scripts/chapterbackgrounds.txt", TRUE ))
		return;

	char *afile = (char *)EngFuncs::COM_LoadFile( "scripts/chapterbackgrounds.txt", NULL );
	char *pfile = afile;
	char token[1024];

	if( !afile )
	{
		Con_Printf( "UI_LoadBackgroundMapList: chapterbackgrounds.txt not found\n" );
		return;
	}

	while(( pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ))) != NULL )
	{
		// skip the numbers (old format list)
		if( isdigit( token[0] )) continue;

		uiStatic.bgmaps.AddToTail( token );
	}

	EngFuncs::COM_FreeFile( afile );
}

static void UI_LoadSounds( void )
{
	memset( uiStatic.sounds, 0, sizeof( uiStatic.sounds ) );

	if ( uiStatic.lowmemory )
		return;

	for ( int i = 0; i < SND_COUNT; i++ )
	{
		if ( !uiSounds[i] || *uiSounds[i] == '\0' )
			continue;

		if ( !EngFuncs::FileExists( uiSounds[i] ) )
		{
			size_t len = strlen( uiSoundOldPrefix );

			if ( !strncmp( uiSounds[i], uiSoundOldPrefix, len ) )
				snprintf( uiStatic.sounds[i], sizeof( uiStatic.sounds[i] ), "%s%s", uiSoundNewPrefix, uiSounds[i] + len );
		}
		else
			Q_strncpy( uiStatic.sounds[i], uiSounds[i], sizeof( uiStatic.sounds[i] ) );
	}
}

/*
=================
UI_VidInit
=================
*/
int UI_VidInit( void )
{
	static bool calledOnce = false;
	if( uiStatic.textInput )
	{
		uiStatic.menu.InputMethodResized();

		return 0;
	}
	if(!calledOnce) UI_Precache();
	// don't allow screenwidth is slower than 4:3 screens
	// it's really not intended to use, just for keeping menu working
	if (ScreenWidth * 3 < ScreenHeight * 4)
	{
		uiStatic.scaleX = uiStatic.scaleY = ScreenWidth / 1024.0f;
		uiStatic.yOffset = ( ScreenHeight / 2.0f ) / uiStatic.scaleX - 768.0f / 2.0f;
	}
	else
	{
		// Sizes are based on screen height
		uiStatic.scaleX = uiStatic.scaleY = ScreenHeight / 768.0f;
		uiStatic.yOffset = 0;
	}


	uiStatic.width = ScreenWidth / uiStatic.scaleX;
	// move cursor to screen center
	uiStatic.cursorX = ScreenWidth / 2;
	uiStatic.cursorY = ScreenHeight / 2;
	uiStatic.outlineWidth = 4;

	// all menu buttons have the same view sizes
	uiStatic.buttons_draw_size = Size( UI_BUTTONS_WIDTH, UI_BUTTONS_HEIGHT ).Scale();

	UI_ScaleCoords( NULL, NULL, &uiStatic.outlineWidth, NULL );

	// trying to load chapterbackgrounds.txt
	UI_LoadBackgroundMapList ();

	CMenuBackgroundBitmap::LoadBackground( );

	// reload all menu buttons
	UI_LoadBmpButtons ();

	// VidInit FontManager
	g_FontMgr->VidInit();

	// load button sounds
	UI_LoadSounds();

	// init global background, root windows don't have background anymore
	if( !uiStatic.background )
		uiStatic.background = new CMenuBackgroundBitmap();
	uiStatic.background->VidInit();

	uiStatic.menu.VidInit( calledOnce );

	if( !calledOnce ) calledOnce = true;

	return 1;
}

void UI_OpenUpdatePage( bool engine, bool preferstore )
{
	const char *updateUrl = NULL;

	if( engine || !gMenu.m_gameinfo.update_url[0] )
	{
		if( preferstore )
			updateUrl = PLATFORM_UPDATE_PAGE;
		else
			updateUrl = GENERIC_UPDATE_PAGE;
	}
	else
	{
		updateUrl = gMenu.m_gameinfo.update_url;
	}

	if( updateUrl )
		EngFuncs::ShellExecute( updateUrl, NULL, TRUE );
}

void UI_UpdateDialog( int preferStore )
{
	static CMenuYesNoMessageBox msgBox;
	static bool ignore = false;
	static bool staticPreferStore;

	if( ignore )
		return;

	staticPreferStore = preferStore != 0;

	msgBox.SetMessage( "A new update is available.\nPress Update to open download page." );
	msgBox.SetPositiveButton( "Update", PC_UPDATE );
	msgBox.SetNegativeButton( "Later", PC_CANCEL );

	SET_EVENT( msgBox.onPositive, UI_OpenUpdatePage( true, *(bool*)pExtra ) );
	msgBox.onPositive.pExtra = &staticPreferStore;

	SET_EVENT( msgBox.onNegative, *(bool*)pExtra = true ); // set ignore
	msgBox.onNegative.pExtra = &ignore;

	msgBox.Show();

}

static void UI_UpdateDialog_f( void )
{
	if( !strcmp( EngFuncs::CmdArgv( 1 ), "nostore" ))
		UI_UpdateDialog( false );
	else
		UI_UpdateDialog( true );
}

ADD_COMMAND( menu_updatedialog, UI_UpdateDialog_f );

/*
=================
UI_Init
=================
*/
void UI_Init( void )
{
	// register our cvars and commands
	ui_showmodels = EngFuncs::CvarRegister( "ui_showmodels", "0", FCVAR_ARCHIVE );
	ui_show_window_stack = EngFuncs::CvarRegister( "ui_show_window_stack", "0", FCVAR_ARCHIVE );
	ui_borderclip = EngFuncs::CvarRegister( "ui_borderclip", "0", FCVAR_ARCHIVE );
	ui_language = EngFuncs::CvarRegister( "ui_language", "english", FCVAR_ARCHIVE );

	// show cl_predict dialog
	EngFuncs::CvarRegister( "menu_mp_firsttime2", "1", FCVAR_ARCHIVE );

	// menu style
	ui_menu_style = EngFuncs::CvarRegister( "ui_menu_style", "1", FCVAR_ARCHIVE );

	for( CMenuEntry *entry = s_pEntries; entry; entry = entry->m_pNext )
	{
		if( entry->m_szCommand && entry->m_pfnShow )
		{
			EngFuncs::Cmd_AddCommand( entry->m_szCommand, entry->m_pfnShow );
		}
	}

	g_FontMgr = new CFontManager();

	uiStatic.initialized = true;
	uiStatic.lowmemory = (int)EngFuncs::GetCvarFloat( "host_lowmemorymode" );

	// setup game info
	gameinfo2_t *gi = EngFuncs::GetGameInfo();
	if( !gi )
		Host_Error( "pfnGetGameInfo returned NULL!\n" );

	gMenu.m_gameinfo = *gi;

	uiStatic.renderPicbuttonText = gMenu.m_gameinfo.flags & GFL_RENDER_PICBUTTON_TEXT;

	// trying to load colors.lst
	UI_ApplyCustomColors ();
}

/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown( void )
{
	if( !uiStatic.initialized )
		return;

	for( CMenuEntry *entry = s_pEntries; entry; entry = entry->m_pNext )
	{
		if( entry->m_szCommand && entry->m_pfnShow )
		{
			EngFuncs::Cmd_RemoveCommand( entry->m_szCommand );
		}

		if( entry->m_pfnShutdown )
		{
			entry->m_pfnShutdown();
		}
	}

	UI_FreeCustomStrings();

	delete uiStatic.background;
	delete g_FontMgr;

	memset( &uiStatic, 0, sizeof( uiStatic_t ));
}
