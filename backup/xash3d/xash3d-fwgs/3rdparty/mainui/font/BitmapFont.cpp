/*
BitmapFont.cpp - bitmap font backend
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

#include "BaseMenu.h"
#include "BaseFontBackend.h"
#include "BitmapFont.h"
#include "utflib.h"

CBitmapFont::CBitmapFont() : CBaseFont(), hImage( 0 ) { }
CBitmapFont::~CBitmapFont() { }
#ifndef MAINUI_FORCE_CONSOLE_BITMAP_FONT
#include "menufont.h"
#endif

#ifndef MAINUI_CONSOLE_FONT_HEIGHT
// redefine in specific ports
#define MAINUI_CONSOLE_FONT_HEIGHT 10
#endif
bool CBitmapFont::Create(const char *name, int tall, int weight, int blur, float brighten, int outlineSize, int scanlineOffset, float scanlineScale, int flags)
{
	Q_strncpy( m_szName, name, sizeof( m_szName ) );
	m_iHeight = m_iTall = tall;
	m_iWeight = weight;
	m_iFlags = flags;

	m_iBlur = blur;
	m_fBrighten = brighten;

	m_iOutlineSize = outlineSize;

	m_iScanlineOffset = scanlineOffset;
	m_fScanlineScale = scanlineScale;
	m_iAscent = 0;
	m_iMaxCharWidth = 0;
#ifndef MAINUI_FORCE_CONSOLE_BITMAP_FONT
	if( tall > UI_CONSOLE_CHAR_HEIGHT * uiStatic.scaleY )
	{
		hImage = EngFuncs::PIC_Load( "#XASH_SYSTEMFONT_001.bmp", menufont_bmp, sizeof( menufont_bmp ), 0 );
		iImageWidth = EngFuncs::PIC_Width( hImage );
		iImageHeight = EngFuncs::PIC_Height( hImage );
	}
	else
#endif
		m_iHeight = m_iTall = MAINUI_CONSOLE_FONT_HEIGHT;
	int a, c;
	GetCharABCWidths( '.', a, m_iEllipsisWide, c );
	m_iEllipsisWide *= 3;

	return true;
}

void CBitmapFont::GetCharRGBA(int ch, Point pt, Size sz, byte *rgba, Size &drawSize)
{

}

void CBitmapFont::GetCharABCWidthsNoCache( int ch, int &a, int &b, int &c )
{
	// static font not uses cache
	return;
}

void CBitmapFont::GetCharABCWidths(int ch, int &a, int &b, int &c)
{
	a = c = 0;
	if( hImage )
		b = m_iHeight/2-1;
	else
	{
		char str[2] = {(char)ch, 0};
		EngFuncs::engfuncs.pfnDrawConsoleStringLen( str, &b, NULL );
	}
}

bool CBitmapFont::HasChar(int ch) const
{
	if( ( ch >= 33 && ch <= 126 ) // ascii
		|| ( ch >= 0x0400 && ch <= 0x045F ) ) // cyrillic
		return true;
	return false;
}

void CBitmapFont::UploadGlyphsForRanges(charRange_t *range, int rangeSize)
{

}

int CBitmapFont::DrawCharacter(int ch, Point pt, int charH, const unsigned int color, bool forceAdditive)
{
	// let's say we have twice lower width from height
	// cp1251 now
	if( ch >= 0x0410 && ch <= 0x042F )
		ch = ch - 0x410 + 0xC0;
	if( ch >= 0x0430 && ch <= 0x044F )
		ch = ch - 0x430 + 0xE0;
	else
	{
		int i;
		for( i = 0; i < 64; i++ )
			if( table_cp1251[i] == ch )
				ch = i + 0x80;
	}

	// Draw character doesn't works with alpha override
	// EngFuncs::DrawCharacter( pt.x, pt.y, sz.h / 2, sz.h, ch, (int)iColor, hImage );

	if( hImage )
	{
		EngFuncs::PIC_Set( hImage, Red( color ), Green( color ), Blue( color ), Alpha( color ));

		float	row, col, size;
		col = (ch & 15) * 0.0625f + (0.5f / 256.0f);
		row = (ch >> 4) * 0.0625f + (0.5f / 256.0f);
		size = 0.0625f - (1.0f / 256.0f);


		wrect_t rc;
		int w, h;
		w = iImageWidth;
		h = iImageHeight;

		rc.top    = h * row;
		rc.left   = w * col;
		rc.bottom = rc.top + h * size;
		rc.right  = rc.left + w * size;

		if( forceAdditive )
			EngFuncs::PIC_DrawAdditive( pt.x, pt.y, charH/2, charH, &rc );
		else
			EngFuncs::PIC_DrawTrans( pt.x, pt.y, charH/2, charH, &rc );

		return charH/2-1;

	}
	else
	{
		char str[2] = {(char)ch, 0};
		EngFuncs::engfuncs.pfnDrawSetTextColor( Red( color ), Green( color ), Blue( color ), Alpha( color ) );

		return EngFuncs::engfuncs.pfnDrawConsoleString( pt.x, pt.y, str ) - pt.x;
	}
}
