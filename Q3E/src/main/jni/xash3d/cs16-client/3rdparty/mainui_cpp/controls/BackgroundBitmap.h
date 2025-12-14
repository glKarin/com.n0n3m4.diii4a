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
	struct bimage_t
	{
		HIMAGE hImage;
		Point coord;
		Size size;
	};

	enum bstate_e
	{
		DRAW_COLOR,
		DRAW_WON,
		DRAW_STEAM,
	};

	void DrawBackgroundPiece( const bimage_t &image, Point p, int xOffset, int yOffset, float xScale, float yScale );
	void DrawSteamBackgroundLayout( Point p, int xOffset, int yOffset, float xScale, float yScale );
	void DrawColor();
	void DrawInGameBackground();

	static bool LoadSteamBackground( const bool gamedirOnly ); // Steam background loader
	static bool LoadWONBackground( const bool gamedirOnly ); // WON background loader
	static void UpdatePreference();

	static bool s_bEnableLogoMovie, s_bGameHasSteamBackground, s_bGameHasWONBackground;

	static bstate_e s_state;

	static bimage_t s_WONBackground;

	static Size s_SteamBackgroundImageSize;
	static CUtlVector<bimage_t> s_SteamBackground;
};

#endif // MENU_BACKGROUNDBITMAP_H
