/*
StbFont.h -- stb_truetype font renderer
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

#if defined(MAINUI_USE_CUSTOM_FONT_RENDER) && defined(MAINUI_USE_STB)
#include "BaseFontBackend.h"
#include "utlmemory.h"
#include "utlrbtree.h"
#include "stb_truetype.h"


class CStbFont : public CBaseFont
{
public:
	CStbFont();
	~CStbFont();

	bool Create(const char *name,
		int tall, int weight,
		int blur, float brighten,
		int outlineSize,
		int scanlineOffset, float scanlineScale,
		int flags) override;
	void GetCharRGBA(int ch, Point pt, Size sz, unsigned char *rgba, Size &drawSize) override;
	void GetCharABCWidthsNoCache( int ch, int &a, int &b, int &c ) override;
	bool HasChar( int ch ) const override;
	const char *GetBackendName() const override { return "stb"; }

private:
	byte *m_pFontData;
	stbtt_fontinfo m_fontInfo;

	double scale;

	friend class CFontManager;
};

#endif // defined(MAINUI_USE_CUSTOM_FONT_RENDER) && defined(MAINUI_USE_FREETYPE)
