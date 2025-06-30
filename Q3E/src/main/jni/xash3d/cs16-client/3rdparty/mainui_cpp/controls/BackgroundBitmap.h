/*
BackgroundBitmap.h -- background menu item
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

#ifndef MENU_BACKGROUNDBITMAP_H
#define MENU_BACKGROUNDBITMAP_H

#include "Bitmap.h"
#include "utlvector.h"

#define ART_BACKGROUND		"gfx/shell/splash.bmp"

// Ultimate class that support multiple types of background: fillColor, WON-style, GameUI-style
class CMenuBackgroundBitmap: public CMenuBitmap
{
public:
	CMenuBackgroundBitmap();

	void VidInit( void ) override;
	void Draw( void ) override;
	void SetInactive(bool) override { }
	void ToggleInactive() override { }

	bool bForceColor; // dialogs should set this

	static void LoadBackground();
	static bool ShouldDrawLogoMovie() { return s_bEnableLogoMovie; }
private:
	void DrawBackgroundLayout( Point p, int xOffset, int yOffset, float xScale, float yScale );
	void DrawColor();
	void DrawInGameBackground();

	static bool LoadBackgroundImage( const bool gamedirOnly ); // Steam background loader
	static bool CheckBackgroundSplash( const bool gamedirOnly ); // WON background loader

	//==========
	// WON-style
	//==========
	static bool s_bEnableLogoMovie;
	static Size s_BackgroundImageSize;
	static bool s_bLoadedSplash;
	static Size s_SteamBackgroundSize;

	//=============
	// GameUI-style
	//=============
	typedef struct
	{
		HIMAGE hImage;
		Point coord;
		Size size;
	} bimage_t;

	static CUtlVector<bimage_t> s_Backgrounds;
};

#endif // MENU_BACKGROUNDBITMAP_H
