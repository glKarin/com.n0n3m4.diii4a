// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __FONTMANAGERLOCAL_H__
#define __FONTMANAGERLOCAL_H__

#include "FontManager.h"

/*
 * file "localization/fonts/lcdsh-regular.ttf"
faceIndex 0
 */
struct sdLocFont_t {
	idStr name;
	idStr file;
	int faceIndex;
	int fontId;
	unsigned int checksum;
};

enum {
	ALIGN_LEFT = DTF_LEFT,
	ALIGN_CENTER = DTF_CENTER,
	ALIGN_RIGHT = DTF_RIGHT,
	ALIGN_TOP = DTF_TOP,
	ALIGN_MIDDLE = DTF_VCENTER,
	ALIGN_BOTTOM = DTF_BOTTOM,
};

class sdFontManagerLocal : public sdFontManager
{
public:
	sdFontManagerLocal(void);
	~sdFontManagerLocal(void) {}

	virtual void            Init(void);
	virtual void            Shutdown(void);

    virtual qhandle_t		FindFont( const char* fontName );
    virtual void			FreeFont( const qhandle_t font );

    virtual const int		GetFontHeight( const qhandle_t font, const int pointSize );

    virtual void			SetFont( const qhandle_t font );
    virtual void			SetFontSize( const int pointSize );

    void					DrawText( const wchar_t* text, const sdBounds2D& rect, int align, bool wrap, bool noclipping, const idVec4 &color );
    void					GetTextDimensions( const wchar_t* text, const sdBounds2D& rect, int align, bool wrap, bool noclipping, const qhandle_t font, const int pointSize, int& width, int& height, float* scale = NULL, int** charAdvances = NULL, idList< int >* lineBreaks = NULL );


	void					DrawText(const char* text, const sdBounds2D& rect, int align, bool wrap, bool noclipping, const idVec4 &color );
	void					GetTextDimensions( const char* text, const sdBounds2D& rect, int textAlign, bool wrap, bool noclipping, const qhandle_t font, const int pointSize, int& width, int& height, float* scale = NULL, int** charAdvances = NULL, idList< int >* lineBreaks = NULL );

private:
	bool					ParseFontConfig(const char *path, sdLocFont_t &config);
	sdLocFont_t *			FindFontConfig(const char *name);
	void					LoadFontConfigs(const char *lang);
	void					SetupFonts();
	bool					RegisterFont(int index, const char *fileName, unsigned int check = 0);
	bool					ConvertFont(const sdLocFont_t *fc, const char *name, const char *lang, const char *fileName) const;
	unsigned int			ReadChecksum(const char *fileName) const;
	void					WriteChecksum(const char *fileName, unsigned int checksum) const;
	void					RemoveChecksum(const char *fileName) const;
	void					ChecksumFileName(idStr &out, const char *fileName) const;
	unsigned int			TrueTypeFontFileChecksum(const char *file) const;

private:
	void					SetFontByScale(float scale);
	int						DrawText(float x, float y, float scale, idVec4 color, const char *text, float adjust, int limit, int style, int cursor = -1, bool calcOnly = false, int *rWidth = NULL);
	void					PaintChar(float x,float y,float width,float height,float scale,float	s,float	t,float	s2,float t2,const idMaterial *hShader, const idVec4 *color = NULL);
	void					DrawEditCursor(float x, float y, float scale, const idVec4 *color = NULL);
	int						DrawText(const char *text, float textScale, int textAlign, idVec4 color, const sdBounds2D &rectDraw, bool wrap, bool noclipping, int cursor = -1, bool calcOnly = false, idList<int> *breaks = NULL, int limit = 0, int** charAdvances = NULL, int rSize[] = NULL);
	int						MaxCharHeight(float scale);
	int						MaxCharWidth(float scale);
	int						CharWidth(const char c, float scale);
	int						DrawText(float x, float y, float scale, idVec4 color, const wchar_t *text, float adjust, int limit, int style, int cursor, bool calcOnly = false, int *rWidth = NULL);
	int						DrawText(const wchar_t *text, float textScale, int textAlign, idVec4 color, const sdBounds2D &rectDraw, bool wrap, bool noclipping, int cursor, bool calcOnly, idList<int> *breaks, int limit, int** charAdvances = NULL, int rSize[] = NULL);

private:
	idStr					fontName;
	idStr					fontLang;

	fontInfoEx_t			*activeFont;
	fontInfo_t				*useFont;

	static idList<fontInfoEx_t> fonts;
	static idList<sdLocFont_t> fontConfigs;
	bool					overStrikeMode;
};

extern sdFontManagerLocal fontManagerLocal;

#endif /* !__FONTMANAGERLOCAL_H__ */
