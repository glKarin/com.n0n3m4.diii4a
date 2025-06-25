/*
BitmapFont.h - bitmap font backend
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
#pragma once
#ifndef BITMAPFONT_H
#define BITMAPFONT_H

#include "BaseFontBackend.h"

class CBitmapFont : public CBaseFont
{
public:
	CBitmapFont();
	~CBitmapFont() override;

	bool Create( const char *name,
						 int tall, int weight,
						 int blur, float brighten,
						 int outlineSize,
						 int scanlineOffset, float scanlineScale,
						 int flags ) override;
	void GetCharRGBA( int ch, Point pt, Size sz, byte *rgba, Size &drawSize ) override;
	void GetCharABCWidthsNoCache( int ch, int &a, int &b, int &c ) override;
	void GetCharABCWidths( int ch, int &a, int &b, int &c ) override;
	bool HasChar( int ch ) const override;
	const char *GetBackendName() const override { return "bitmap"; }

	void UploadGlyphsForRanges( charRange_t *range, int rangeSize ) override;
	int DrawCharacter(int ch, Point pt, int charH, const unsigned int color, bool forceAdditive = false) override;
private:
	HIMAGE hImage;
	int iImageWidth, iImageHeight;
};

#endif // BITMAPFONT_H
