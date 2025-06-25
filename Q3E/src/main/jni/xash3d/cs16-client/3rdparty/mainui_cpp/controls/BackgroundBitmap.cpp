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
Size CMenuBackgroundBitmap::s_BackgroundImageSize;
CUtlVector<CMenuBackgroundBitmap::bimage_t> CMenuBackgroundBitmap::s_Backgrounds;
bool CMenuBackgroundBitmap::s_bLoadedSplash = false;
Size CMenuBackgroundBitmap::s_SteamBackgroundSize;

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

void CMenuBackgroundBitmap::DrawBackgroundLayout( Point p, int xOffset, int yOffset, float xScale, float yScale )
{
	int start, end;

	if ( ui_menu_style->value && s_bLoadedSplash )
	{
		end = s_Backgrounds.Count();
		start = end - 1;
		s_bEnableLogoMovie = true;
		s_BackgroundImageSize = s_Backgrounds.Tail().size;
	}
	else
	{
		start = 0;
		end = s_bLoadedSplash ? s_Backgrounds.Count() - 1 : s_Backgrounds.Count();
		s_bEnableLogoMovie = false;
		s_BackgroundImageSize = s_SteamBackgroundSize;
	}

	// iterate and draw all the background pieces
	for (int i = start; i < end; i++)
	{
		bimage_t &bimage = s_Backgrounds[i];

		int dx = (int)ceil(bimage.coord.x * xScale);
		int dy = (int)ceil(bimage.coord.y * yScale);
		int dw = (int)ceil(bimage.size.w * xScale);
		int dt = (int)ceil(bimage.size.h * yScale);

		EngFuncs::PIC_Set( bimage.hImage, 255, 255, 255, 255 );
		EngFuncs::PIC_Draw( p.x + dx + xOffset, p.y + dy + yOffset, dw, dt );
	}
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

	if( s_Backgrounds.Count() == 0 )
	{
		DrawColor();
		return;
	}

	if( szPic )
	{
		UI_DrawPic( m_scPos, m_scSize, uiColorWhite, szPic );
		return;
	}

	Point p;
	float xScale, yScale;

	// Disable parallax effect. It's just funny, but not really needed
#if 0
	float flParallaxScale = 0.02;
	p.x = (uiStatic.cursorX - ScreenWidth) * flParallaxScale;
	p.y = (uiStatic.cursorY - ScreenHeight) * flParallaxScale;

	// work out scaling factors
	// work out scaling factors
	if( ScreenWidth * s_BackgroundImageSize.h > ScreenHeight * s_BackgroundImageSize.w )
	{
		xScale = ScreenWidth / s_BackgroundImageSize.w * (1 + flParallaxScale);
		yScale = xScale;
	}
	else
	{
		yScale = ScreenHeight / s_BackgroundImageSize.h * (1 + flParallaxScale);
		xScale = yScale;
	}
#else
	p.x = p.y = 0;

	// work out scaling factors
	if( ScreenWidth * s_BackgroundImageSize.h > ScreenHeight * s_BackgroundImageSize.w )
	{
		xScale = ScreenWidth / s_BackgroundImageSize.w;
		yScale = xScale;
	}
	else
	{
		yScale = ScreenHeight / s_BackgroundImageSize.h;
		xScale = yScale;
	}
#endif

	// center wide background (for example if background is wider than our window)
	int xOffset = 0, yOffset = 0;
	if( s_BackgroundImageSize.w * xScale > ScreenWidth )
		xOffset = ( ScreenWidth - s_BackgroundImageSize.w * xScale ) / 2;
	else if( s_BackgroundImageSize.h * yScale > ScreenHeight )
		yOffset = ( ScreenHeight - s_BackgroundImageSize.h * yScale ) / 2;

	DrawBackgroundLayout( p, xOffset, yOffset, xScale, yScale );

	// print CS16Client version
	char version[32];
	int stringLen, charH;

	snprintf( version, 32, "cs16-client build %s", EngFuncs::GetCvarString( "cscl_ver" ) );

	EngFuncs::engfuncs.pfnDrawConsoleStringLen( version, &stringLen, &charH );
	EngFuncs::engfuncs.pfnDrawConsoleString( stringLen * 0.05f, ScreenHeight - charH * 1.05f, version );
}

bool CMenuBackgroundBitmap::LoadBackgroundImage( bool gamedirOnly )
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

	s_SteamBackgroundSize.w = atoi( token );

	pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );
	if( !pfile ) goto freefile;

	s_SteamBackgroundSize.h = atoi( token );

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

		s_Backgrounds.AddToTail( img );
	}

	loaded = true;

freefile:
	EngFuncs::COM_FreeFile( afile );
	return loaded;
}

bool CMenuBackgroundBitmap::CheckBackgroundSplash( bool gamedirOnly )
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
		s_BackgroundImageSize = img.size;

		s_Backgrounds.AddToTail( img );

		if( gamedirOnly )
		{
			// if we doesn't have logo.avi in gamedir we don't want to draw it
			s_bEnableLogoMovie = EngFuncs::FileExists( "media/logo.avi", TRUE );
		}

		return true;
	}

	return false;
}

void CMenuBackgroundBitmap::LoadBackground()
{
	if( s_Backgrounds.Count() != 0 || uiStatic.lowmemory )
		return;

	// try to load backgrounds from mod
	LoadBackgroundImage( true ); // at first check new gameui backgrounds

	s_bLoadedSplash = CheckBackgroundSplash( true ); // then check won-style

	/*
	// try from base directory
	if( LoadBackgroundImage( false ) )
		return; // gameui bgs are allowed to be inherited

	if( CheckBackgroundSplash( false ) )
		return;
	*/
}
