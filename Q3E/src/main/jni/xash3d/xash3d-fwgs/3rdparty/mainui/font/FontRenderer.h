#pragma once
#ifndef FONT_RENDERER_H
#define FONT_RENDERER_H

typedef int HFont; // handle to a font

enum EFontFlags
{
	FONT_NONE      = 0,
	FONT_ITALIC    = 1 << 0,
	FONT_UNDERLINE = 1 << 1,
	FONT_STRIKEOUT = 1 << 2
};
#ifndef MAINUI_SMALL_SCREEN
#define UI_CONSOLE_CHAR_WIDTH	9
#define UI_CONSOLE_CHAR_HEIGHT  18

#define UI_SMALL_CHAR_WIDTH		10
#define UI_SMALL_CHAR_HEIGHT	20

#define UI_MED_CHAR_WIDTH		13
#define UI_MED_CHAR_HEIGHT		26

#ifdef CS16CLIENT
#define UI_BIG_CHAR_WIDTH		25
#define UI_BIG_CHAR_HEIGHT		50
#else
#define UI_BIG_CHAR_WIDTH		20
#define UI_BIG_CHAR_HEIGHT		40
#endif // CS16CLIENT
#else // MAINUI_SMALL_SCREEN

#define UI_CONSOLE_CHAR_WIDTH	30
#define UI_CONSOLE_CHAR_HEIGHT  60

#define UI_SMALL_CHAR_WIDTH		30
#define UI_SMALL_CHAR_HEIGHT	60

#define UI_MED_CHAR_WIDTH		35
#define UI_MED_CHAR_HEIGHT		70

#define UI_BIG_CHAR_WIDTH		35
#define UI_BIG_CHAR_HEIGHT		70
#endif


enum EFontSizes
{
#ifdef CLIENT_DLL // hack!
	QM_DEFAULTFONT = 1, // medium size font
#else
	QM_DEFAULTFONT = 0, // medium size font
#endif
	QM_SMALLFONT,       // small
	QM_BIGFONT,         // big
	QM_BOLDFONT,
};

class CFontBuilder
{
public:
	CFontBuilder( const char *name, int tall, int weight )
	{
		m_szName = name;
		m_iTall = tall;
		m_iWeight = weight;

		m_iFlags = FONT_NONE;
		m_iBlur = m_iScanlineOffset = m_iOutlineSize = 0;
		m_hForceHandle = -1;

		m_fScanlineScale = 0.7f;
		m_fBrighten = 1.0f;
	}

	CFontBuilder &SetBlurParams( int blur, float brighten = 1.0f )
	{
		m_iBlur = blur;
		m_fBrighten = brighten;
		return *this;
	}

	CFontBuilder &SetOutlineSize( int outlineSize = 1 )
	{
		m_iOutlineSize = outlineSize;
		return *this;
	}

	CFontBuilder &SetScanlineParams( int offset = 2, float scale = 0.7f )
	{
		m_iScanlineOffset = offset;
		m_fScanlineScale = scale;
		return *this;
	}

	CFontBuilder &SetFlags( int flags )
	{
		m_iFlags = flags;
		return *this;
	}

	HFont Create();

private:
	CFontBuilder &SetHandleNum( HFont num ) // restricted only for FontManager
	{
		m_hForceHandle = num;
		return *this;
	}

	const char *m_szName;
	int m_iTall, m_iWeight, m_iFlags;
	int m_iBlur;
	float m_fBrighten;

	int m_iOutlineSize;

	int m_iScanlineOffset;
	float m_fScanlineScale;
	HFont m_hForceHandle;
	friend class CFontManager;
};


#endif // FONT_RENDERER_H
