/*
BaseFontBackend.h - common font renderer backend code
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
#ifndef BASEFONT_H
#define BASEFONT_H

// #include "port.h" // defines XASH_MOBILE_PLATFORM
#include "BaseMenu.h"
#include "utlrbtree.h"

// #ifdef XASH_MOBILE_PLATFORM
#if defined(__ANDROID__) || TARGET_OS_IPHONE || defined(XASH_SAILFISH) || defined(MAINUI_FONT_SCALE)
#define SCALE_FONTS
#endif

struct charRange_t
{
	int chMin;
	int chMax;
	const int *sequence;
	int size;

	size_t Length() const
	{
		if( sequence )
			return size;
		return chMax - chMin;
	}

	int Character( size_t pos )
	{
		if( sequence )
			return sequence[pos];
		return chMin + pos;
	}
};

class CBaseFont
{
public:
	CBaseFont();
	virtual ~CBaseFont( );

	virtual bool Create(
		const char *name,
		int tall, int weight,
		int blur, float brighten,
		int outlineSize,
		int scanlineOffset, float scanlineScale,
		int flags ) = 0;
	virtual void GetCharRGBA( int ch, Point pt, Size sz, byte *rgba, Size &drawSize ) = 0;
	virtual void GetCharABCWidthsNoCache( int ch, int &a, int &b, int &c ) = 0;
	virtual bool HasChar( int ch ) const = 0;
	virtual const char *GetBackendName() const = 0;
	virtual void GetCharABCWidths( int ch, int &a, int &b, int &c );
	virtual void UploadGlyphsForRanges( charRange_t *range, int rangeSize );
	virtual int  DrawCharacter(int ch, Point pt, int charH, const unsigned int color, bool forceAdditive = false);

	inline int GetHeight() const       { return m_iHeight + GetEfxOffset(); }
	inline int GetTall() const         { return m_iTall; }
	inline const char *GetName() const { return m_szName; }
	inline int GetAscent() const       { return m_iAscent; }
	inline int GetMaxCharWidth() const { return m_iMaxCharWidth; }
	inline int GetFlags() const        { return m_iFlags; }
	inline int GetWeight() const       { return m_iWeight; }
	inline int GetEfxOffset() const    { return m_iBlur + m_iOutlineSize; }

	bool IsEqualTo( const char *name, int tall, int weight, int blur, int flags ) const;

	void DebugDraw();

	void GetTextureName(char *dst, size_t len) const;

	inline int GetEllipsisWide( ) { return m_iEllipsisWide; }

protected:
	void ApplyBlur( Size rgbaSz, byte *rgba );
	void ApplyOutline(Point pt, Size rgbaSz, byte *rgba );
	void ApplyScanline( Size rgbaSz, byte *rgba );
	void ApplyStrikeout( Size rgbaSz, byte *rgba );

	char m_szName[32];
	int	 m_iTall, m_iWeight, m_iFlags, m_iHeight, m_iMaxCharWidth;
	int  m_iAscent;
	bool m_bAdditive;

	// blurring
	int  m_iBlur;
	float m_fBrighten;

	// Scanlines
	int  m_iScanlineOffset;
	float m_fScanlineScale;

	// Outlines
	int  m_iOutlineSize;
	int m_iEllipsisWide;

private:
	bool ReadFromCache( const char *filename, charRange_t *range, size_t rangeSize );
	void SaveToCache( const char *filename, charRange_t *range, size_t rangeSize, CBMP *bmp );

	void GetBlurValueForPixel( double *distribution, const byte *src, Point srcPt, Size srcSz, byte *dest );

	struct glyph_t
	{
		glyph_t() : ch( 0 ), texture( 0 ), rect() { }
		glyph_t( int ch ) : ch( ch ), texture( 0 ), rect() { }
		int ch;
		HIMAGE texture;
		wrect_t rect;

		bool operator< (const glyph_t &a) const
		{
			return ch < a.ch;
		}
	};

	struct abc_t
	{
		int ch;
		int a, b, c;

		bool operator< ( const abc_t &a ) const
		{
			return ch < a.ch;
		}
	};

	CUtlRBTree<glyph_t, int> m_glyphs;
	CUtlRBTree<abc_t, int>   m_ABCCache;

	char m_szTextureName[256];
	friend class CFontManager;
};


#endif // BASEFONT_H
