/*
BaseFontBackend.cpp - common font renderer backend code
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
#include "BaseFontBackend.h"
#include "FontManager.h"
#include <math.h>
#include "Utils.h"
#include "miniutl/utlbuffer.h"

CBaseFont::CBaseFont()
	: m_iTall(), m_iWeight(), m_iFlags(),
	m_iHeight(), m_iMaxCharWidth(), m_iAscent(),
	m_iBlur(), m_fBrighten(),
	m_iEllipsisWide( 0 ),
	m_glyphs(0, 0), m_ABCCache( 0, 0 )
{
	m_szTextureName[0] = m_szName[0] = 0;
	SetDefLessFunc( m_glyphs );
	SetDefLessFunc( m_ABCCache );
}


/*
=========================
CBaseFont::GetTextureName

Mangle texture name, so using same font names with different attributes will not confuse engine or font renderer
=========================
+*/
void CBaseFont::GetTextureName( char *dst, size_t len ) const
{
	char attribs[256];
	int i = 0;
	if( GetFlags() & FONT_ITALIC ) attribs[i++] = 'i'; // 1 parameter
	if( GetFlags() & FONT_UNDERLINE ) attribs[i++] = 'u'; // 1 parameter
	if( m_iBlur )
	{
		int chars = snprintf( attribs + i, sizeof( attribs ) - 1 - i, "g%i%.2f", m_iBlur, m_fBrighten );
		i += chars;
	}
	if( m_iOutlineSize )
	{
		int chars = snprintf( attribs + i, sizeof( attribs ) - 1 - i, "o%i", m_iOutlineSize );
		i += chars;
	}
	if( m_iScanlineOffset )
	{
		int chars = snprintf( attribs + i, sizeof( attribs ) - 1 - i, "s%i%.2f", m_iScanlineOffset, m_fScanlineScale );
		i += chars;
	}
	attribs[i] = 0;

	// faster loading: don't query filesystem, tell engine to skip everything and load only from buffer
	if( i == 0 )
	{
		snprintf( dst, len - 1, "#%s_%i_%i_%s_font.bmp", GetName(), GetTall(), GetWeight(), GetBackendName( ));
		dst[len - 1] = 0;
	}
	else
	{
		attribs[i] = 0;
		snprintf( dst, len - 1, "#%s_%i_%i_%s_%s_font.bmp", GetName(), GetTall(), GetWeight(), attribs, GetBackendName( ));
		dst[len - 1] = 0;
	}
}

#define MAX_PAGE_SIZE 256

void CBaseFont::UploadGlyphsForRanges(charRange_t *range, int rangeSize)
{
	const int maxWidth = GetMaxCharWidth();
	const int height = GetHeight();
	const int tempSize = maxWidth * height * 4; // allocate temporary buffer for max possible glyph size
	const Point nullPt( 0, 0 );

	GetTextureName( m_szTextureName, sizeof( m_szTextureName ));

	if( ReadFromCache( m_szTextureName, range, rangeSize ))
	{
		int dotWideA, dotWideB, dotWideC;
		GetCharABCWidths( '.', dotWideA, dotWideB, dotWideC );
		m_iEllipsisWide = ( dotWideA + dotWideB + dotWideC ) * 3;

		return;
	}

	CBMP bmp( MAX_PAGE_SIZE, MAX_PAGE_SIZE );
	byte *rgbdata = bmp.GetTextureData();
	bmp_t *hdr = bmp.GetBitmapHdr();

	Size tempDrawSize( maxWidth, height );
	byte *temp = new byte[tempSize];

	// abscissa atlas optimization
	CUtlVector<uint> lines;
	int line = 0;

	// texture is reversed by Y coordinates
	int xstart = 0, ystart = hdr->height-1;
	for( int iRange = 0; iRange < rangeSize; iRange++ )
	{
		size_t size = range[iRange].Length();

		for( size_t i = 0; i < size; i++ )
		{
			int ch = range[iRange].Character( i );

			// clear temporary buffer
			memset( temp, 0, tempSize );

			// draw it to temp buffer
			Size drawSize;
			GetCharRGBA( ch, nullPt, tempDrawSize, temp, drawSize );

			// see if we need to go down or create a new page
			if( xstart + drawSize.w > (int)hdr->width )
			{
				// update or push
				if( lines.IsValidIndex( line ) )
				{
					lines[line] = xstart;
					line++;
					// do we have next or don't have it yet?
					if( lines.IsValidIndex( line ) )
						xstart = lines[line];
					else
						xstart = 0;
				}
				else
				{
					lines.AddToTail(xstart);
					line++;
					// obviously we don't have next
					xstart = 0;

				}
				ystart -= height + 1; // HACKHACK: Add more space between rows, this removes ugly 1 height pixel rubbish

				// No free space now
				if( ystart - height - 1 <= 0 )
				{
					if( hdr->height <= (int)hdr->width ) // prioritize height grow
					{
						int oldheight = hdr->height - ystart;
						bmp.Increase( hdr->width, hdr->height * 2 );
						hdr = bmp.GetBitmapHdr();
						ystart = hdr->height - oldheight - 1;
					}
					else
					{
						bmp.Increase( hdr->width * 2, hdr->height );
						hdr = bmp.GetBitmapHdr();
						line = 0;
						xstart = lines[line];
						ystart = hdr->height - 1;
					}

					// update pointers
					rgbdata = bmp.GetTextureData();
				}
			}

			// set rgbdata rect
			wrect_t rect;
			rect.top    = hdr->height - ystart;
			rect.bottom = hdr->height - ystart + height;
			rect.left   = xstart;
			rect.right  = xstart + drawSize.w;

			// copy glyph to rgbdata

			for( int y = 0; y < height - 1; y++ )
			{
				byte *dst = &rgbdata[(ystart - y) * hdr->width * 4];
				byte *src = &temp[y * maxWidth * 4];
				for( int x = 0; x < drawSize.w; x++ )
				{
					byte *xdst = &dst[ ( xstart + x ) * 4 ];
					byte *xsrc = &src[ x * 4 ];

					// copy 4 bytes: R, G, B and A
					memcpy( xdst, xsrc, 4 );
				}
			}

			// move xstart
			xstart += drawSize.w;

			glyph_t glyph;
			glyph.ch = ch;
			glyph.rect = rect;
			glyph.texture = 0; // will be acquired later

			m_glyphs.Insert( glyph );
		}
	}

	HIMAGE hImage = EngFuncs::PIC_Load( m_szTextureName, bmp.GetBitmap(), bmp.GetBitmapHdr()->fileSize, 0 );

	SaveToCache( m_szTextureName, range, rangeSize, &bmp );

	delete[] temp;

	for( int i = m_glyphs.FirstInorder();; i = m_glyphs.NextInorder( i ) )
	{
		m_glyphs[i].texture = hImage;
		if( i == m_glyphs.LastInorder() )
			break;
	}

	int dotWideA, dotWideB, dotWideC;
	GetCharABCWidths( '.', dotWideA, dotWideB, dotWideC );
	m_iEllipsisWide = ( dotWideA + dotWideB + dotWideC ) * 3;
}


CBaseFont::~CBaseFont()
{
	if( m_szTextureName[0] != 0 )
		EngFuncs::PIC_Free( m_szTextureName );
}

void CBaseFont::GetCharABCWidths( int ch, int &a, int &b, int &c )
{
	abc_t find;
	find.ch = ch;

	const int i = m_ABCCache.Find( find );
	if( m_ABCCache.IsValidIndex( i ))
	{
		find = m_ABCCache[i];
		a = find.a;
		b = find.b;
		c = find.c;
		return;
	}

	// not found in cache
	GetCharABCWidthsNoCache( ch, find.a, find.b, find.c );

	find.a -= m_iBlur;
	find.b += m_iBlur + m_iOutlineSize;

	a = find.a;
	b = find.b;
	c = find.c;

	m_ABCCache.Insert( find );
}

bool CBaseFont::IsEqualTo(const char *name, int tall, int weight, int blur, int flags)  const
{
	if( stricmp( name, m_szName ))
		return false;

	if( m_iTall != tall )
		return false;

	if( m_iWeight != weight )
		return false;

	if( m_iBlur != blur )
		return false;

	if( m_iFlags != flags )
		return false;

	return true;
}

void CBaseFont::DebugDraw()
{
	HIMAGE const hImage = EngFuncs::PIC_Load( m_szTextureName );
	const int w = EngFuncs::PIC_Width( hImage );
	const int h = EngFuncs::PIC_Height( hImage );

	int x = 0;
	EngFuncs::PIC_Set( hImage, 255, 255, 255 );
	EngFuncs::PIC_DrawTrans( Point( x, 0 ), Size( w, h ));

	for( int i = m_glyphs.FirstInorder();; i = m_glyphs.NextInorder( i ) )
	{
		if( m_glyphs[i].texture == hImage )
		{
			Point pt;
			Size sz;
			pt.x = x + m_glyphs[i].rect.left;
			pt.y = m_glyphs[i].rect.top;
			sz.w = m_glyphs[i].rect.right - m_glyphs[i].rect.left;
			sz.h = m_glyphs[i].rect.bottom - pt.y;
			UI_DrawRectangleExt( pt, sz, PackRGBA( 255, 0, 0, 255 ), 1 );

			int a, b, c;
			GetCharABCWidths( m_glyphs[i].ch, a, b, c );

			pt.x -= a;
			sz.w += c + a;
			UI_DrawRectangleExt( pt, sz, PackRGBA( 0, 255, 0, 255 ), 1, QM_LEFT | QM_RIGHT );

			const int ascender = GetAscent();
			pt.y += ascender;
			UI_DrawRectangleExt( pt, sz, PackRGBA( 0, 0, 255, 255 ), 1, QM_TOP );
		}

		if( i == m_glyphs.LastInorder() )
			break;
	}
}

void CBaseFont::ApplyBlur(Size rgbaSz, byte *rgba)
{
	if( !m_iBlur )
		return;

	const int size = rgbaSz.w * rgbaSz.h * 4;
	byte *src = new byte[size];
	double sigma2;
	memcpy( src, rgba, size );

	sigma2 = 0.5 * m_iBlur;
	sigma2 *= sigma2;
	double *distribution = new double[m_iBlur * 2 + 1];
	for( int x = 0; x <= m_iBlur * 2; x++ )
	{
		int val = x - m_iBlur;

		distribution[x] = (1.0 / sqrt(2 * 3.14 * sigma2)) * pow(2.7, -1 * (val * val) / (2 * sigma2));

		// brightening factor
		distribution[x] *= m_fBrighten;
	}


	for( int y = 0; y < rgbaSz.h; y++ )
	{
		for( int x = 0; x < rgbaSz.w; x++ )
		{
			GetBlurValueForPixel( distribution, src, Point(x, y), rgbaSz, rgba );

			rgba += 4;
		}
	}

	delete[] distribution;
	delete[] src;
}

void CBaseFont::GetBlurValueForPixel( double *distribution, const byte *src, Point srcPt, Size srcSz, byte *dest )
{
	double accum = 0.0f;

	// scan the positive x direction
	int maxX = Q_min( srcPt.x + m_iBlur, srcSz.w );
	int minX = Q_max( srcPt.x - m_iBlur, 0 );
	for( int x = minX; x < maxX; x++ )
	{
		int maxY = Q_min( srcPt.y + m_iBlur, srcSz.h );
		int minY = Q_max( srcPt.y - m_iBlur, 0);
		for (int y = minY; y < maxY; y++)
		{
			const byte *srcPos = src + ((x + (y * srcSz.w)) * 4);

			// muliply by the value matrix
			double weight = distribution[(x - srcPt.x) + m_iBlur];
			double weight2 = distribution[(y - srcPt.y) + m_iBlur];
			accum += ( srcPos[3] ) * (weight * weight2);
		}
	}

	// all the values are the same for fonts, just use the calculated alpha
	dest[0] = dest[1] = dest[2] = 255;
	dest[3] = Q_min( (int)(accum + 0.5), 255);
}

void CBaseFont::ApplyOutline(Point pt, Size rgbaSz, byte *rgba)
{
	if( !m_iOutlineSize )
		return;

	uint *tmp = new uint[rgbaSz.w * rgbaSz.h]; // matrix where we accumulate alpha values
	memset( tmp, 0, sizeof( *tmp ) * rgbaSz.w * rgbaSz.h );

	for( int y = pt.x; y < rgbaSz.h; y++ )
	{
		for( int x = pt.y; x < rgbaSz.w; x++ )
		{
			byte *src = &rgba[(x + (y * rgbaSz.w)) * 4];

			for( int shadowX = -m_iOutlineSize; shadowX <= m_iOutlineSize; shadowX++ )
			{
				for( int shadowY = -m_iOutlineSize; shadowY <= m_iOutlineSize; shadowY++ )
				{
					int testX = shadowX + x, testY = shadowY + y;
					if( testX < 0 || testX >= rgbaSz.w || testY < 0 || testY >= rgbaSz.h )
						continue;

					uint *dst = &tmp[(testX + (testY * rgbaSz.w))];
					*dst += src[3];
				}
			}
		}
	}

	// find total amount of adjacent pixels
	int total = m_iOutlineSize * 4 + m_iOutlineSize * m_iOutlineSize * 4;

	for( int y = pt.x; y < rgbaSz.h; y++ )
	{
		for( int x = pt.y; x < rgbaSz.w; x++ )
		{
			byte *src = &rgba[(x + (y * rgbaSz.w)) * 4];
			uint *dst = &tmp[(x + (y * rgbaSz.w))];

			if( *dst == 0 )
				continue;

			uint val = *dst / (double)total;

			val *= ( m_iOutlineSize + 1 ); // make it darker

			// is this pixel painted by font renderer
			if( src[3] > 0 )
			{
				// blend it with outline
				src[0] = src[1] = src[2] = src[3];
				src[3] = Q_min( src[3] + val, 255 );
			}
			else
			{
				src[0] = src[1] = src[2] = 0; // black outline
				src[3] = Q_min( val, 255 );
			}
		}
	}

	delete[] tmp;
}

void CBaseFont::ApplyScanline(Size rgbaSz, byte *rgba)
{
	if( m_iScanlineOffset < 2 )
		return;

	for( int y = 0; y < rgbaSz.h; y++ )
	{
		if( y % m_iScanlineOffset == 0 )
			continue;

		byte *src = &rgba[(y * rgbaSz.w) * 4];
		for( int x = 0; x < rgbaSz.w; x++, src += 4 )
		{
			src[0] *= m_fScanlineScale;
			src[1] *= m_fScanlineScale;
			src[2] *= m_fScanlineScale;
		}
	}
}

void CBaseFont::ApplyStrikeout(Size rgbaSz, byte *rgba)
{
	if( !(m_iFlags & FONT_STRIKEOUT) )
		return;

	const int y = rgbaSz.h * 0.5f;

	byte *src = &rgba[(y*rgbaSz.w) * 4];

	for( int x = 0; x < rgbaSz.w; x++, src += 4 )
	{
		src[0] = src[1] = src[2] = 127;
		src[3] = 255;
	}
}

int CBaseFont::DrawCharacter(int ch, Point pt, int charH, const unsigned int color, bool forceAdditive)
{
	Size charSize;
	int a, b, c, width;

#ifdef SCALE_FONTS
	float factor = (float)charH / (float)GetTall();
#endif

	GetCharABCWidths( ch, a, b, c );
	width = a + b + c;

	// skip whitespace
	if( ch == ' ' )
	{
#ifdef SCALE_FONTS
		if( charH > 0 )
		{
			return width * factor + 0.5f;
		}
		else
#endif
		{
			return width;
		}
	}

	CBaseFont::glyph_t find( ch );
	int idx = m_glyphs.Find( find );

	if( m_glyphs.IsValidIndex( idx ) )
	{
		CBaseFont::glyph_t &glyph = m_glyphs[idx];

		int r, g, b, alpha;

		UnpackRGBA(r, g, b, alpha, color );

#ifdef SCALE_FONTS	// Scale font
		if( charH > 0 )
		{
			charSize.w = (glyph.rect.right - glyph.rect.left) * factor + 0.5f;
			charSize.h = GetHeight() * factor + 0.5f;
		}
		else
#endif
		{
			charSize.w = glyph.rect.right - glyph.rect.left;
			charSize.h = GetHeight();
		}

		pt.x += a;

		EngFuncs::PIC_Set( glyph.texture, r, g, b, alpha );
		if( forceAdditive )
			EngFuncs::PIC_DrawAdditive( pt, charSize, &glyph.rect );
		else
			EngFuncs::PIC_DrawTrans( pt, charSize, &glyph.rect );
	}

#ifdef SCALE_FONTS
	if( charH > 0 )
	{
		return width * factor + 0.5f;
	}
#endif
	return width;
}

#define CACHED_FONT_IDENT \
	(('T'<<24)+('F'<<16)+('I'<<8)+'U') // little-endian "UIFT"

// Version 2. Font blur has been changed, force font regeneration
#define CACHED_FONT_VERSION 2

struct char_data_t
{
	uint32_t ch;
	int32_t a, b, c;
	uint32_t left, right, top, bottom;
};

struct cached_font_t
{
	uint32_t ident;
	uint32_t version;
	uint32_t charsCount;
};

bool CBaseFont::ReadFromCache( const char *filename, charRange_t *range, size_t rangeSize )
{
	char path[512];
	int size, i, j, charsCount = 0;
	byte *data;
	cached_font_t *hdr;
	char_data_t *ch;
	bmp_t *bmp;

	// skip special symbol used for engine
	V_snprintf( path, sizeof( path ), ".fontcache/%s", filename[0] == '#' ? filename + 1 : filename );

	if( !EngFuncs::FileExists( path ))
		return false;

	data = EngFuncs::COM_LoadFile( path, &size );

	if( !data )
	{
		Con_Printf( "Failed to load font cache file\n" );
		return false;
	}

	for( i = 0; i < rangeSize; i++ )
		charsCount += range[i].Length();

	hdr = reinterpret_cast<cached_font_t *>( data );

	if( size < sizeof( cached_font_t ) )
	{
		Con_Printf( "Font cache file is too short\n" );
		EngFuncs::COM_FreeFile( data );
		EngFuncs::DeleteFile( filename );
		return false;
	}

	if( hdr->ident != CACHED_FONT_IDENT )
	{
		Con_Printf( "Wrong font cache file format\n" );
		EngFuncs::COM_FreeFile( data );
		EngFuncs::DeleteFile( filename );
		return false;
	}

	if( hdr->version != CACHED_FONT_VERSION )
	{
		Con_Printf( "Wrong font cache file version. Expected %d, got %d\n", CACHED_FONT_VERSION, hdr->version );
		EngFuncs::COM_FreeFile( data );
		EngFuncs::DeleteFile( filename );
		return false;
	}

	if( hdr->charsCount != charsCount )
	{
		Con_Printf( "Font cache file has different character set. Expected %d characters in set, got %d\n", charsCount, hdr->charsCount );
		EngFuncs::COM_FreeFile( data );
		EngFuncs::DeleteFile( filename );
		return false;
	}

	if( size < sizeof( cached_font_t ) + hdr->charsCount * sizeof( char_data_t ))
	{
		Con_Printf( "Font cache file is too short (2nd check)\n" );
		EngFuncs::COM_FreeFile( data );
		EngFuncs::DeleteFile( filename );
		return false;
	}

	bmp = reinterpret_cast<bmp_t *>(data + sizeof( cached_font_t ) + hdr->charsCount * sizeof( char_data_t ));

	if( bmp->id[0] != 'B' && bmp->id[1] != 'M' )
	{
		Con_Printf( "Font cache BMP file id check failed\n" );
		EngFuncs::COM_FreeFile( data );
		EngFuncs::DeleteFile( filename );
		return false;
	}

	if( size != sizeof( cached_font_t ) + hdr->charsCount * sizeof( char_data_t ) + bmp->fileSize )
	{
		Con_Printf( "Font cache file is too short or too long (3rd check)\n" );
		EngFuncs::COM_FreeFile( data );
		EngFuncs::DeleteFile( filename );
		return false;
	}

	HIMAGE hImage = EngFuncs::PIC_Load( filename, (const byte*)bmp, bmp->fileSize, 0 );

	if( !hImage )
	{
		Con_Printf( "Failed to load font cache BMP\n" );
		EngFuncs::COM_FreeFile( data );
		EngFuncs::DeleteFile( filename );
		return false;
	}

	ch = reinterpret_cast<char_data_t *>(data + sizeof( cached_font_t ));

	for( i = 0; i < rangeSize; i++ )
	{
		charsCount = range[i].Length();

		for( j = 0; j < charsCount; j++ )
		{
			if( ch->ch != range[i].Character( j ))
			{
				Con_Printf( "Font cache file has different character set. Expected %d, got %d", range[i].Character( j ), ch->ch );
				EngFuncs::COM_FreeFile( data );
				EngFuncs::PIC_Free( filename );
				EngFuncs::DeleteFile( filename );
				return false;
			}

			glyph_t glyph( ch->ch );
			abc_t abc;

			glyph.rect.left = ch->left;
			glyph.rect.bottom = ch->bottom;
			glyph.rect.right = ch->right;
			glyph.rect.top = ch->top;
			glyph.texture = hImage;

			m_glyphs.Insert( glyph );

			abc.ch = ch->ch;
			abc.a = ch->a;
			abc.b = ch->b;
			abc.c = ch->c;

			m_ABCCache.Insert( abc );

			ch++;
		}
	}

	EngFuncs::COM_FreeFile( data );
	return true;
}

void CBaseFont::SaveToCache( const char *filename, charRange_t *range, size_t rangeSize, CBMP *bmp )
{
	char path[512];
	int i, j;
	uint32_t charsCount = 0;
	byte *data, *buf_p;
	size_t size = 0, bmpSize = bmp->GetBitmapHdr()->fileSize;

	// skip special symbol used for engine
	if( filename[0] == '#' )
		filename++;

	for( i = 0; i < rangeSize; i++ )
		charsCount += range[i].Length();

	size = sizeof( cached_font_t ) +
			charsCount * sizeof( char_data_t ) +
			bmpSize;

	buf_p = data = new byte[size];

	((cached_font_t *)buf_p)->ident = CACHED_FONT_IDENT;
	((cached_font_t *)buf_p)->version = CACHED_FONT_VERSION;
	((cached_font_t *)buf_p)->charsCount = charsCount;

	buf_p += sizeof( cached_font_t );

	for( i = 0; i < rangeSize; i++ )
	{
		charsCount = range[i].Length();

		for( j = 0; j < charsCount; j++ )
		{
			char_data_t ch;


			ch.ch = range[i].Character( j );
			GetCharABCWidths( ch.ch, ch.a, ch.b, ch.c );

			glyph_t glyph( ch.ch );
			int idx = m_glyphs.Find( glyph );

			glyph = m_glyphs[idx];

			ch.left   = glyph.rect.left;
			ch.right  = glyph.rect.right;
			ch.bottom = glyph.rect.bottom;
			ch.top    = glyph.rect.top;

			memcpy( buf_p, &ch, sizeof( ch ));
			buf_p += sizeof( ch );
		}
	}

	memcpy( buf_p, bmp->GetBitmapHdr(), bmpSize );

	if( buf_p + bmpSize - data != size )
		Host_Error( "%s: %i: buf_p + bmpSize - data != size", __FILE__, __LINE__ );

	V_snprintf( path, sizeof( path ), ".fontcache/%s", filename );
	EngFuncs::COM_SaveFile( path, data, size );

	delete[] data;
}
