/*
BackgroundBitmap.cpp -- background menu item
Copyright (C) 2010 Uncle Mike
Copyright (C) 2017 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "BackgroundBitmap.h"
#include "Utils.h"
#include "BaseWindow.h"

bool CMenuBackgroundBitmap::s_bEnableLogoMovie = false;
bool CMenuBackgroundBitmap::s_bGameHasSteamBackground = false;
bool CMenuBackgroundBitmap::s_bGameHasWONBackground = false;
CMenuBackgroundBitmap::bstate_e CMenuBackgroundBitmap::s_state;

CMenuBackgroundBitmap::bimage_t CMenuBackgroundBitmap::s_WONBackground;

Size CMenuBackgroundBitmap::s_SteamBackgroundImageSize;
CUtlVector<CMenuBackgroundBitmap::bimage_t> CMenuBackgroundBitmap::s_SteamBackground;

CMenuBackgroundBitmap::CMenuBackgroundBitmap() : CMenuBitmap()
{
	szPic = 0;
	iFlags = QMF_INACTIVE|QMF_DISABLESCAILING;
	bForceColor = false;
}

void CMenuBackgroundBitmap::VidInit()
{
	pos.x = pos.y = 0;

	if( m_pParent )
	{
		// fill parent
		if( m_pParent->iFlags & QMF_DISABLESCAILING )
		{
			size = m_pParent->size;
		}
		else
		{
			size = m_pParent->size.Scale();
		}
	}
	else
	{
		size = Size( ScreenWidth, ScreenHeight );
	}

	colorBase.SetDefault( 0xFF505050 );

	CMenuBaseItem::VidInit();
}

void CMenuBackgroundBitmap::DrawInGameBackground()
{
	UI_FillRect( m_scPos, m_scSize, uiColorBlack );
}

void CMenuBackgroundBitmap::DrawColor()
{
	if( bDrawStroke )
	{
		UI_DrawRectangleExt( m_scPos, m_scSize, colorStroke, 1 );
	}

	UI_FillRect( m_scPos, m_scSize, colorBase );
}

void CMenuBackgroundBitmap::DrawBackgroundPiece( const bimage_t &image, Point p, int xOffset, int yOffset, float xScale, float yScale )
{
	EngFuncs::PIC_Set( image.hImage, 255, 255, 255, 255 );

	int dx = (int)ceil( image.coord.x * xScale );
	int dy = (int)ceil( image.coord.y * yScale );
	int dw = (int)ceil( image.size.w * xScale );
	int dt = (int)ceil( image.size.h * yScale );

	EngFuncs::PIC_Draw( p.x + dx + xOffset, p.y + dy + yOffset, dw, dt );
}

void CMenuBackgroundBitmap::DrawSteamBackgroundLayout( Point p, int xOffset, int yOffset, float xScale, float yScale )
{
	// iterate and draw all the background pieces
	for( int i = 0; i < s_SteamBackground.Count(); i++ )
		DrawBackgroundPiece( s_SteamBackground[i], p, xOffset, yOffset, xScale, yScale );
}

/*
=================
CMenuBackgroundBitmap::Draw
=================
*/
void CMenuBackgroundBitmap::Draw()
{
	if( bForceColor )
	{
		DrawColor();
		return;
	}

	if( EngFuncs::ClientInGame() )
	{
		if( EngFuncs::GetCvarFloat( "cl_background" ) )
		{
			return;
		}

		if( EngFuncs::GetCvarFloat( "ui_renderworld" ) )
		{
			DrawInGameBackground();
			return;
		}
	}

	if( szPic )
	{
		UI_DrawPic( m_scPos, m_scSize, uiColorWhite, szPic );
		return;
	}

	if( FBitSet( ui_prefer_won_background->flags, FCVAR_CHANGED ))
	{
		UpdatePreference();

		// because the cvar is set by user, tell them if chosen background is not available
		if( ui_prefer_won_background->value && s_state != DRAW_WON )
			UI_ShowMessageBox( L( "WON background is not available" ));
	}

	Point p;
	float xScale, yScale;
	Size s;

	if( s_state == DRAW_COLOR )
	{
		DrawColor();
		return;
	}
	else if( s_state == DRAW_WON )
		s = s_WONBackground.size;
	else
		s = s_SteamBackgroundImageSize;

	// Disable parallax effect. It's just funny, but not really needed
#if 0
	float flParallaxScale = 0.02;
	p.x = (uiStatic.cursorX - ScreenWidth) * flParallaxScale;
	p.y = (uiStatic.cursorY - ScreenHeight) * flParallaxScale;

	// work out scaling factors
	// work out scaling factors
	if( ScreenWidth * s.h > ScreenHeight * s.w )
	{
		xScale = ScreenWidth / s.w * (1 + flParallaxScale);
		yScale = xScale;
	}
	else
	{
		yScale = ScreenHeight / s.h * (1 + flParallaxScale);
		xScale = yScale;
	}
#else
	p.x = p.y = 0;

	// work out scaling factors
	if( ScreenWidth * s.h > ScreenHeight * s.w )
	{
		xScale = ScreenWidth / s.w;
		yScale = xScale;
	}
	else
	{
		yScale = ScreenHeight / s.h;
		xScale = yScale;
	}
#endif

	// center wide background (for example if background is wider than our window)
	int xOffset = 0, yOffset = 0;
	if( s.w * xScale > ScreenWidth )
		xOffset = ( ScreenWidth - s.w * xScale ) / 2;
	else if( s.h * yScale > ScreenHeight )
		yOffset = ( ScreenHeight - s.h * yScale ) / 2;

	if( s_state == DRAW_WON )
		DrawBackgroundPiece( s_WONBackground, p, xOffset, yOffset, xScale, yScale );
	else
		DrawSteamBackgroundLayout( p, xOffset, yOffset, xScale, yScale );

	// print CS16Client version
	char version[32];
	int stringLen, charH;

	snprintf( version, 32, "cs16-client build %s", EngFuncs::GetCvarString( "cscl_ver" ) );

	EngFuncs::engfuncs.pfnDrawConsoleStringLen( version, &stringLen, &charH );
	EngFuncs::engfuncs.pfnDrawConsoleString( stringLen * 0.05f, ScreenHeight - charH * 1.05f, version );

}

bool CMenuBackgroundBitmap::LoadSteamBackground( bool gamedirOnly )
{
	char *afile = NULL, *pfile;
	char token[4096];

	bool loaded = false;

	s_bEnableLogoMovie = false; // no logos for Steam background

	// try 25'th anniversary update background first
	if( FBitSet( gMenu.m_gameinfo.flags, GFL_HD_BACKGROUND ))
		afile = (char *)EngFuncs::COM_LoadFile( "resource/HD_BackgroundLayout.txt" );

	if( !afile )
		afile = (char *)EngFuncs::COM_LoadFile( "resource/BackgroundLayout.txt" );

	if( !afile )
		return false;

	pfile = afile;

	pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );
	if( !pfile || strcmp( token, "resolution" )) // resolution at first!
		goto freefile;

	pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );
	if( !pfile ) goto freefile;

	s_SteamBackgroundImageSize.w = atoi( token );

	pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );
	if( !pfile ) goto freefile;

	s_SteamBackgroundImageSize.h = atoi( token );

	// Now read all tiled background list
	while(( pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) )))
	{
		bimage_t img;

		if( !EngFuncs::FileExists( token, gamedirOnly ))
			goto freefile;

		img.hImage = EngFuncs::PIC_Load( token, PIC_NOFLIP_TGA );

		if( !img.hImage ) goto freefile;

		// ignore "scaled" attribute. What does it mean?
		pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );
		if( !pfile ) goto freefile;

		pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );
		if( !pfile ) goto freefile;
		img.coord.x = atoi( token );

		pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );
		if( !pfile ) goto freefile;
		img.coord.y = atoi( token );

		img.size.w = EngFuncs::PIC_Width( img.hImage );
		img.size.h = EngFuncs::PIC_Height( img.hImage );

		s_SteamBackground.AddToTail( img );
	}

	loaded = true;

freefile:
	EngFuncs::COM_FreeFile( afile );
	return loaded;
}

bool CMenuBackgroundBitmap::LoadWONBackground( bool gamedirOnly )
{
	s_bEnableLogoMovie = false;

	if( EngFuncs::FileExists( ART_BACKGROUND, gamedirOnly ))
	{
		bimage_t img;

		img.hImage = EngFuncs::PIC_Load( ART_BACKGROUND );

		if( !img.hImage )
			return false;

		img.coord.x = img.coord.y = 0;
		img.size.w = EngFuncs::PIC_Width( img.hImage );
		img.size.h = EngFuncs::PIC_Height( img.hImage );
		s_WONBackground = img;

		if( gamedirOnly )
		{
			// if we doesn't have logo.avi in gamedir we don't want to draw it
			s_bEnableLogoMovie = EngFuncs::FileExists( "media/logo.avi", true );
		}

		return true;
	}

	return false;
}

void CMenuBackgroundBitmap::UpdatePreference()
{
	if( ui_prefer_won_background->value )
	{
		if( s_bGameHasWONBackground )
			s_state = DRAW_WON;
		else if( s_bGameHasSteamBackground )
			s_state = DRAW_STEAM;
		else if( s_WONBackground.hImage )
			s_state = DRAW_WON;
		else if( s_SteamBackground.Count() > 0 )
			s_state = DRAW_STEAM;
		else
			s_state = DRAW_COLOR;
	}
	else
	{
		if( s_bGameHasSteamBackground )
			s_state = DRAW_STEAM;
		else if( s_bGameHasWONBackground )
			s_state = DRAW_WON;
		else if( s_SteamBackground.Count() > 0 )
			s_state = DRAW_STEAM;
		else if( s_WONBackground.hImage )
			s_state = DRAW_WON;
		else
			s_state = DRAW_COLOR;
	}

	ClearBits( ui_prefer_won_background->flags, FCVAR_CHANGED );
}

void CMenuBackgroundBitmap::LoadBackground()
{
	s_bEnableLogoMovie = false;
	s_bGameHasSteamBackground = false;
	s_bGameHasWONBackground = false;

	if( uiStatic.lowmemory )
		return;

	if( LoadSteamBackground( true ))
	{
		Con_DPrintf( "%s: found %s background in %s directory\n", __func__, "steam", "game" );
		s_bGameHasSteamBackground = true;
	}
	else if( LoadSteamBackground( false ))
		Con_DPrintf( "%s: found %s background in %s directory\n", __func__, "steam", "base" );

	if( LoadWONBackground( true ))
	{
		Con_DPrintf( "%s: found %s background in %s directory\n", __func__, "won", "game" );
		s_bGameHasWONBackground = true;
	}
	else if( LoadWONBackground( false ))
		Con_DPrintf( "%s: found %s background in %s directory\n", __func__, "won", "game" );

	UpdatePreference();
}
