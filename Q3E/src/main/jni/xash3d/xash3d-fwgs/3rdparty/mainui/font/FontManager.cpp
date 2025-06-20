/*
FontManager.cpp - font manager
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
#include <locale.h>
#include "FontManager.h"
#include "BaseMenu.h"
#include "Utils.h"

#include "BaseFontBackend.h"

#if defined(MAINUI_USE_FREETYPE)
#include "FreeTypeFont.h"
#elif defined(MAINUI_USE_STB)
#include "StbFont.h"
#elif defined(_WIN32)
#include "WinAPIFont.h"
#endif

#include "BitmapFont.h"

#define DEFAULT_MENUFONT "Trebuchet MS"
#define DEFAULT_CONFONT  "Tahoma"
#define DEFAULT_WEIGHT   500

CFontManager *g_FontMgr;

CFontManager::CFontManager()
{
#ifdef MAINUI_USE_FREETYPE
	FT_Init_FreeType( &CFreeTypeFont::m_Library );
#endif
	m_Fonts.EnsureCapacity( 4 );
	m_FontFiles.EnsureCapacity( 2 );
}

CFontManager::~CFontManager()
{
	DeleteAllFonts();
#ifdef MAINUI_USE_FREETYPE
	FT_Done_FreeType( CFreeTypeFont::m_Library );
	CFreeTypeFont::m_Library = NULL;
#endif

	FOR_EACH_HASHMAP( m_FontFiles, i )
	{
		byte *p = m_FontFiles.Element( i ).data;
		EngFuncs::COM_FreeFile( p );
	}

	m_FontFiles.Purge();
}

void CFontManager::VidInit( void )
{
	static float prevScale = 0;

	float scale = uiStatic.scaleY;

	if( !prevScale
#ifndef SCALE_FONTS // complete disables font re-rendering
	|| fabs( scale - prevScale ) > 0.1f
#endif
	)
	{
		DeleteAllFonts();
		uiStatic.hDefaultFont = CFontBuilder( DEFAULT_MENUFONT, UI_MED_CHAR_HEIGHT * scale, DEFAULT_WEIGHT )
			.SetHandleNum( QM_DEFAULTFONT )
			.Create();
		uiStatic.hSmallFont   = CFontBuilder( DEFAULT_MENUFONT, UI_SMALL_CHAR_HEIGHT * scale, DEFAULT_WEIGHT )
			.SetHandleNum( QM_SMALLFONT )
			.Create();
		uiStatic.hBigFont     = CFontBuilder( DEFAULT_MENUFONT, UI_BIG_CHAR_HEIGHT * scale, DEFAULT_WEIGHT )
			.SetHandleNum( QM_BIGFONT )
			.Create();
		uiStatic.hBoldFont = CFontBuilder( DEFAULT_MENUFONT, UI_MED_CHAR_HEIGHT * scale, 1000 )
			.SetHandleNum( QM_BOLDFONT )
			.Create();

		if( !uiStatic.lowmemory )
		{
			uiStatic.hLightBlur = CFontBuilder( DEFAULT_MENUFONT, UI_MED_CHAR_HEIGHT * scale, DEFAULT_WEIGHT )
				.SetBlurParams( 2 * scale, 1.25f )
				.Create();

			uiStatic.hHeavyBlur = CFontBuilder( DEFAULT_MENUFONT, UI_MED_CHAR_HEIGHT * scale, DEFAULT_WEIGHT )
				.SetBlurParams( 8 * scale, 2.0f )
				.Create();
		}

		uiStatic.hConsoleFont = CFontBuilder( DEFAULT_CONFONT, UI_CONSOLE_CHAR_HEIGHT * scale, 500 )
			.SetOutlineSize()
			.Create();
		prevScale = scale;
	}
}

void CFontManager::DeleteAllFonts()
{
	for( int i = 0; i < m_Fonts.Count(); i++ )
	{
		delete m_Fonts[i];
	}
	m_Fonts.RemoveAll();
}

void CFontManager::DeleteFont(HFont hFont)
{
	CBaseFont *font = GetIFontFromHandle(hFont);
	if( font )
	{
		m_Fonts[hFont-1] = NULL;

		delete font;
	}
}

CBaseFont *CFontManager::GetIFontFromHandle(HFont font)
{
	if( m_Fonts.IsValidIndex( font - 1 ) )
		return m_Fonts[font-1];

	return NULL;
}

int CFontManager::GetEllipsisWide(HFont font)
{
	if( m_Fonts.IsValidIndex( font - 1 ) )
		return m_Fonts[font-1]->GetEllipsisWide();
	return 0;
}

void CFontManager::GetCharABCWide(HFont font, int ch, int &a, int &b, int &c)
{
	CBaseFont *pFont = GetIFontFromHandle( font );
	if( pFont )
		pFont->GetCharABCWidths( ch, a, b, c );
	else
		a = b = c = 0;
}

int CFontManager::GetCharacterWidth(HFont font, int ch)
{
	int a, b, c;
	GetCharABCWide( font, ch, a, b, c );
	return a + b + c;
}

int CFontManager::GetCharacterWidthScaled(HFont font, int ch, int height)
{
	return GetCharacterWidth( font, ch )
#ifdef SCALE_FONTS
		* ((float)height / (float)GetFontTall( font ))
#endif
	;
}

HFont CFontManager::GetFontByName(const char *name)
{
	for( int i = 0; i < m_Fonts.Count(); i++ )
	{
		if( !stricmp( name, m_Fonts[i]->GetName() ) )
			return i;
	}
	return -1;
}

int CFontManager::GetFontTall(HFont font)
{
	CBaseFont *pFont = GetIFontFromHandle( font );
	if( pFont )
		return pFont->GetTall();
	return 0;
}

int CFontManager::GetFontAscent(HFont font)
{
	CBaseFont *pFont = GetIFontFromHandle( font );
	if( pFont )
		return pFont->GetAscent();
	return 0;
}

bool CFontManager::GetFontUnderlined(HFont font)
{
	CBaseFont *pFont = GetIFontFromHandle( font );
	if( pFont )
		return pFont->GetFlags() & FONT_UNDERLINE;
	return false;
}

void CFontManager::GetTextSize(HFont fontHandle, const char *text, int *wide, int *tall, int size )
{
	CBaseFont *font = GetIFontFromHandle( fontHandle );

	if( !font || !text || !text[0] )
	{
		if( wide ) *wide = 0;
		if( tall ) *tall = 0;
		return;
	}

	int fontTall = font->GetHeight(), x = 0;
	int _wide = 0, _tall;
	const char *ch = text;
	_tall = fontTall;
	int i = 0;

	EngFuncs::UtfProcessChar( 0 );

	while( *ch && ( size < 0 || i < size ) )
	{
		// Skip colorcodes
		if( IsColorString( ch ) )
		{
			ch += 2;
			continue;
		}

		int uch;

		uch = EngFuncs::UtfProcessChar( (unsigned char)*ch );
		if( uch )
		{
			if( uch == '\n' && *( ch + 1 ) != '\0' )
			{
				_tall += fontTall;
				x = 0;
			}
			else
			{
				int a, b, c;
				font->GetCharABCWidths( uch, a, b, c );
				x += a + b + c;
				if( x > _wide )
					_wide = x;
			}
		}
		i++;
		ch++;
	}
	EngFuncs::UtfProcessChar( 0 );

	if( tall ) *tall = _tall;
	if( wide ) *wide = _wide;
}

int CFontManager::CutText(HFont fontHandle, const char *text, int height, int visibleSize, bool reverse, bool stopAtWhitespace, int *wide, bool *remaining )
{
	CBaseFont *font = GetIFontFromHandle( fontHandle );

	if( remaining )
		*remaining = false;

	if( !font || !text || !text[0] || visibleSize <= 0 )
		return 0;

	int _wide = 0;
	const char *ch = text;

#ifdef SCALE_FONTS
	visibleSize  = (float)visibleSize / (float)height * (float)font->GetTall();
#endif

	EngFuncs::UtfProcessChar( 0 );

	int whiteSpacePos = 0;

	// calculate full text wide
	while( *ch )
	{
		// skip colorcodes
		if( IsColorString( ch ) )
		{
			ch += 2;
			continue;
		}

		int uch = EngFuncs::UtfProcessChar( (unsigned char)*ch );
		int x = 0;
		if( uch )
		{
			if( uch == '\n' )
			{
				ch++;
				break; //
			}

			int a, b, c;
			font->GetCharABCWidths( uch, a, b, c );
			x = a + b + c;

			if( uch == ' ' )
			{
				whiteSpacePos = ch - text;
			}
		}

		if( !reverse && _wide + x >= visibleSize )
			break;

		ch++;
		_wide += x;
	}

	EngFuncs::UtfProcessChar( 0 );

	if( !reverse )
	{
		if( *ch && remaining ) *remaining = true;
		if( wide ) *wide = _wide;
		if( stopAtWhitespace && whiteSpacePos )
			return whiteSpacePos;
		return ch - text;
	}

	if( _wide < visibleSize )
	{
		if( remaining ) *remaining = false;
		if( wide ) *wide = _wide;
		return 0;
	}

	ch = text;

	whiteSpacePos = 0;

	// now remove character one by one to fit
	while( *ch && _wide > visibleSize )
	{
		// skip colorcodes
		if( IsColorString( ch ) )
		{
			ch += 2;
			continue;
		}

		int uch = EngFuncs::UtfProcessChar( (unsigned char)*ch );
		if( uch )
		{
			// we don't need check for newlines here, it's only done for oneline Field widget
			int a, b, c;
			font->GetCharABCWidths( uch, a, b, c );
			_wide -= a + b + c;

			if( uch == ' ' )
			{
				whiteSpacePos = ch - text;
			}
		}
		ch++;
	}

	EngFuncs::UtfProcessChar( 0 );

	if( remaining ) *remaining = true;
	if( wide ) *wide = _wide;
	if( stopAtWhitespace && whiteSpacePos ) return whiteSpacePos;
	return ch - text;

}

int CFontManager::GetTextWide(HFont font, const char *text, int size)
{
	int wide;

	GetTextSize( font, text, &wide, NULL, size );

	return wide;
}

int CFontManager::GetTextHeight(HFont fontHandle, const char *text, int size )
{
	CBaseFont *font = GetIFontFromHandle( fontHandle );
	if( !font || !text || !text[0] )
	{
		return 0;
	}

	int height = font->GetHeight();

	// lightweight variant only for getting text height
	int i = 0;
	while( *text&&( size < 0 || i < size ) )
	{
		if( *text == '\n' )
			height += height;

		text++;
		i++;
	}
	return height;
}

int CFontManager::GetTextHeightExt( HFont fontHandle, const char *text, int height, int w, int size )
{
	CBaseFont *font = GetIFontFromHandle( fontHandle );
	if( !font || !text || !text[0] || !w )
	{
		return 0;
	}

	const char *text2 = text;
	int y = 0;

	while( *text2 && ( size < 0 || text2 - text < size ) )
	{
		int pos = CutText( fontHandle, text2, height, w, false, true );
		if( pos == 0 )
			break;

		y += height;
		text2 += pos;
	}

	return y;

}

int CFontManager::GetTextWideScaled(HFont font, const char *text, const int height, int size)
{
	CBaseFont *pFont = GetIFontFromHandle( font );
	if( pFont )
	{
		return GetTextWide( font, text, size )
#ifdef SCALE_FONTS
			* ((float)height / (float)pFont->GetTall())
#endif
		;
	}

	return 0;
}

void CFontManager::UploadTextureForFont(CBaseFont *font)
{
	// upload only latin needed for english and cyrillic needed for russian
	// maybe it would be extended someday...

	charRange_t range[] =
	{
	{ 0x0021, 0x007E, NULL, 0 }, // ascii printable range
	{ 0x00C0, 0x00FF, NULL, 0 }, // latin-1 supplement (letters only)
	{ 0x0400, 0x045F, NULL, 0 }, // cyrillic range
	{ 0, 0, table_cp1251, V_ARRAYSIZE( table_cp1251 ) }, // cp1251
	};

	font->UploadGlyphsForRanges( range, V_ARRAYSIZE( range ) );
}

int CFontManager::DrawCharacter(HFont fontHandle, int ch, Point pt, int charH, const unsigned int color, bool forceAdditive )
{
	CBaseFont *font = GetIFontFromHandle( fontHandle );

	if( !font )
		return 0;

	return font->DrawCharacter( ch, pt, charH, color, forceAdditive );
}

void CFontManager::DebugDraw(HFont fontHandle)
{
	CBaseFont *font = GetIFontFromHandle(fontHandle);

	font->DebugDraw();
}


HFont CFontBuilder::Create()
{
	CBaseFont *font;

	// check existing font at first
	if( !m_hForceHandle )
	{
		for( int i = 0; i < g_FontMgr->m_Fonts.Count(); i++ )
		{
			font = g_FontMgr->m_Fonts[i];

			if( font->IsEqualTo( m_szName, m_iTall, m_iWeight, m_iBlur, m_iFlags ) )
				return i + 1;
		}
	}

#if defined(MAINUI_USE_FREETYPE)
	font = new CFreeTypeFont();
#elif defined(MAINUI_USE_STB)
	font = new CStbFont();
#elif defined(_WIN32) && defined(MAINUI_USE_CUSTOM_FONT_RENDER)
	font = new CWinAPIFont();
#else
	font = new CBitmapFont();
#endif

	double starttime = EngFuncs::DoubleTime();

	if( !font->Create( m_szName, m_iTall, m_iWeight, m_iBlur, m_fBrighten, m_iOutlineSize, m_iScanlineOffset, m_fScanlineScale, m_iFlags ) )
	{
		delete font;

		// fallback to bitmap font
		font = new CBitmapFont();

		// should never fail
		if( !font->Create( "Bitmap Font", m_iTall, m_iWeight, m_iBlur, m_fBrighten, m_iOutlineSize, m_iScanlineOffset, m_fScanlineScale, m_iFlags ) )
		{
			delete font;
			return -1;
		}
	}

	g_FontMgr->UploadTextureForFont( font );

	double endtime = EngFuncs::DoubleTime();

	Con_DPrintf( "Rendering %s(%i, %i) took %f seconds\n", font->GetName(), m_iTall, m_iWeight, endtime - starttime );

	if( m_hForceHandle != -1 && g_FontMgr->m_Fonts.Count() != m_hForceHandle )
	{
		if( g_FontMgr->m_Fonts.IsValidIndex( m_hForceHandle ) )
		{
			g_FontMgr->m_Fonts.FastRemove( m_hForceHandle );
			return g_FontMgr->m_Fonts.InsertBefore( m_hForceHandle, font );
		}
	}

	return g_FontMgr->m_Fonts.AddToTail(font) + 1;
}

bool CFontManager::FindFontDataFile( const char *name, int tall, int weight, int flags, char *dataFile, size_t dataFileChars )
{
	if( !strcmp( name, "Trebuchet MS" ))
	{
		Q_strncpy( dataFile, "gfx/fonts/FiraSans-Regular.ttf", dataFileChars );
		return true;
	}
	else if( !strcmp( name, "Tahoma" ))
	{
		Q_strncpy( dataFile, "gfx/fonts/tahoma.ttf", dataFileChars );
		return true;
	}

	return false;
}

byte *CFontManager::LoadFontDataFile( const char *vfspath, int *plen )
{
	int i = m_FontFiles.Find( vfspath );
	if( i != m_FontFiles.InvalidIndex( ))
	{
		if( plen )
			*plen = m_FontFiles[i].length;

		return m_FontFiles[i].data;
	}
	int len = 0;
	byte *p = EngFuncs::COM_LoadFile( vfspath, &len );
	if( p != nullptr )
	{
		if( plen )
			*plen = len;

		font_file file = { len, p };
		m_FontFiles.Insert( vfspath, file );
	}

	return p;
}
