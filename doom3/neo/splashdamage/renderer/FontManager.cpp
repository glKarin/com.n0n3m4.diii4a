// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "DeviceContext_local.h"
#include "FontManager_local.h"
#include "renderer/tr_local.h"

#define DEFAULT_FONT_TEXTURE_SIZE 1024
#define FONT_CHECKSUM_FILE_NAME "font_checksum.txt"

#define AsASCIICharLang(text_, len_) ( !_hasWideCharFont || idStr::IsPureASCII(text_, len_) )

#define COLOR_FTOUB(x) ((byte)((x) * 255.0f))

#define CALC_SIZE(x) idMath::FtoiFast((float)round(idMath::Ceil(x) + 1e-6))

#define DRAW_TEXT_LINE_SPACING 2 // 5 doom3

extern idCVar harm_gui_useD3BFGFont;
extern idCVar gui_smallFontLimit;
extern idCVar gui_mediumFontLimit;
static bool _hasWideCharFont = false;

extern bool R_ExportTrueTypeFont(const char *fontPath, const char *fontType, const char *language, int width);

idList<fontInfoEx_t> sdFontManagerLocal::fonts;
idList<sdLocFont_t> sdFontManagerLocal::fontConfigs;

static idCVar harm_r_fontDefaultScale("harm_r_fontDefaultScale", "0.27", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_RENDERER, "default font scale in GUIs");
#define DC_DEFAULT_FONT_SCALE (harm_r_fontDefaultScale.GetFloat())

sdFontManagerLocal::sdFontManagerLocal()
	: activeFont(NULL),
		useFont(NULL),
		overStrikeMode(true)
{
}

void sdFontManagerLocal::Init(void) {
	SetupFonts();
}

void sdFontManagerLocal::Shutdown(void) {
	fontLang.Clear();
	activeFont = NULL;
	useFont = NULL;
	for(int i = 0; i < fonts.Num(); i++)
	{
		common->Printf("Free font '%s'.\n", fonts[i].name);
		R_Font_FreeFontInfoEx(&fonts[i]);
	}
	fontConfigs.Clear();
	fonts.Clear();
	overStrikeMode = true;
}

qhandle_t sdFontManagerLocal::FindFont( const char* name ) {
#if 0
	name = "fonts";
#endif
	int c = fonts.Num();

	for (int i = 0; i < c; i++) {
		if (idStr::Icmp(name, fonts[i].name) == 0) {
			return i;
		}
	}

	// If the font was not found, try to register it
	idStr fileName;
	if(!idStr::Icmpn(name, "fonts", 5))
		fileName = name;
	else
	{
		fileName = "fonts";
		fileName.AppendPath(name);
	}
	fileName.Replace("fonts", va("fonts/%s", fontLang.c_str()));

	sdLocFont_t *fc = FindFontConfig(name);

	fontInfoEx_t fontInfo;
    memset(&fontInfo, 0, sizeof(fontInfoEx_t));
	int index = fonts.Append(fontInfo);

	bool fontLoaded = false;
#ifdef _D3BFG_FONT
	const char *d3bfgFontName = harm_gui_useD3BFGFont.GetString();
	if(d3bfgFontName && d3bfgFontName[0] && idStr::Cmp(d3bfgFontName, "0") != 0)
	{
		if(idStr::Cmp(d3bfgFontName, "1") == 0)
		{
			idStr fname(name);
			fname.StripPath();
			if(!idStr::Icmp("an", fname))
				d3bfgFontName = "Arial_Narrow";
			else if(!idStr::Icmp("arial", fname))
				d3bfgFontName = "Arial_Narrow";
			else if(!idStr::Icmp("bank", fname))
				d3bfgFontName = "BankGothic_Md_BT";
			else if(!idStr::Icmp("micro", fname))
				d3bfgFontName = "microgrammadbolext";
			else
				d3bfgFontName = "Chainlink_Semi_Bold";
		}
        if(d3bfgFontName && d3bfgFontName[0])
        {
            idStr newFileName = fileName;
            newFileName.Replace(va("fonts/%s", fontLang.c_str()), "newfonts/");
            newFileName.StripFilename();
            newFileName.AppendPath(d3bfgFontName);
            if (RegisterFont(index, newFileName.c_str()))
            {
                common->Printf("Font '%s' using DOOM3-BFG new font '%s'.\n", name, newFileName.c_str());
                fontLoaded = true;
            }
            else // load default if fail
            {
                common->Printf("Font '%s' load DOOM3-BFG new font '%s' fail, try using default font.\n", name, newFileName.c_str());
                fontLoaded = RegisterFont(index, fileName.c_str());
            }
        }
        else
        {
            common->Printf("Font '%s' not use DOOM3-BFG new font.\n", name);
            fontLoaded = RegisterFont(index, fileName.c_str());
        }
	}
	else
#endif	
	fontLoaded = RegisterFont(index, fileName.c_str(), fc ? fc->checksum : 0);
	if(!fontLoaded)
	{
		if(fc)
		{
			if(ConvertFont(fc, name, fontLang.c_str(), fileName.c_str()))
			{
				fontLoaded = RegisterFont(index, fileName.c_str());
			}
		}
	}

	if (fontLoaded) {
		idStr::Copynz(fonts[index].name, name, sizeof(fonts[index].name));
#ifdef _WCHAR_LANG
		if(!_hasWideCharFont)
		{
			const fontInfoEx_t *f = &fonts[index];
			if(f->fontInfoSmall.numIndexes > 0 || f->fontInfoMedium.numIndexes > 0 || f->fontInfoLarge.numIndexes > 0)
				_hasWideCharFont = true;
		}
#endif
		if(fc)
			fc->fontId = index;
		return index;
	} 

	return -1;
}

void sdFontManagerLocal::FreeFont( const qhandle_t font ) {
}

const int sdFontManagerLocal::GetFontHeight( const qhandle_t font, const int pointSize ) {
	fontInfoEx_t *af;
	fontInfo_t *uf;
	if (font >= 0 && font < fonts.Num()) {
		af = &fonts[font];
	} else {
		af = &fonts[0];
	}
	float scale = DC_DEFAULT_FONT_SCALE;
	int maxHeight;
	if (scale <= gui_smallFontLimit.GetFloat()) {
		uf = &af->fontInfoSmall;
		maxHeight = af->maxHeightSmall;
	} else if (scale <= gui_mediumFontLimit.GetFloat()) {
		uf = &af->fontInfoMedium;
		maxHeight = af->maxHeightMedium;
	} else {
		uf = &af->fontInfoLarge;
		maxHeight = af->maxHeightLarge;
	}
	float useScale = scale * uf->glyphScale;
	return idMath::FtoiFast(maxHeight * useScale);
}

void sdFontManagerLocal::SetFont( const qhandle_t num ) {
	if (num >= 0 && num < fonts.Num()) {
		activeFont = &fonts[num];
	} else {
		activeFont = &fonts[0];
	}
}

void sdFontManagerLocal::SetFontSize( const int pointSize ) {
}

void sdFontManagerLocal::DrawText( const wchar_t* text, const sdBounds2D& rect, int textAlign, bool wrap, bool noclipping, const idVec4 &color ) {
	DrawText(text, DC_DEFAULT_FONT_SCALE, textAlign, color, rect, wrap, noclipping, -1, false, NULL, 0);
}

void sdFontManagerLocal::GetTextDimensions( const wchar_t* text, const sdBounds2D& rect, int textAlign, bool wrap, bool noclipping, const qhandle_t font, const int pointSize, int& width, int& height, float* scale, int** charAdvances, idList< int >* lineBreaks ) {
	float fontScale = DC_DEFAULT_FONT_SCALE;
	SetFont(font);
	int size[2] = {0};
	DrawText(text, fontScale, textAlign, colorWhite, rect, wrap, noclipping, -1, true, lineBreaks, 0, charAdvances, size);
	width = size[0];
	height = size[1];

	if (scale)
		*scale = fontScale;
}

void sdFontManagerLocal::SetupFonts() {
	fonts.SetGranularity(1);

	fontLang = cvarSystem->GetCVarString("sys_lang");

	// western european languages can use the english font
	if (fontLang == "french" || fontLang == "german" || fontLang == "spanish" || fontLang == "italian") {
		fontLang = "english";
	}

	// Default font has to be added first
	FindFont("fonts");

	common->Printf("Loading font configs......\n");
	LoadFontConfigs("english");
	if(idStr::Icmp(fontLang, "english"))
		LoadFontConfigs(fontLang);
	common->Printf("%d font configs found.\n", fontConfigs.Num());
}

void sdFontManagerLocal::SetFontByScale(float scale)
{
	if (scale <= gui_smallFontLimit.GetFloat()) {
		useFont = &activeFont->fontInfoSmall;
		activeFont->maxHeight = activeFont->maxHeightSmall;
		activeFont->maxWidth = activeFont->maxWidthSmall;
	} else if (scale <= gui_mediumFontLimit.GetFloat()) {
		useFont = &activeFont->fontInfoMedium;
		activeFont->maxHeight = activeFont->maxHeightMedium;
		activeFont->maxWidth = activeFont->maxWidthMedium;
	} else {
		useFont = &activeFont->fontInfoLarge;
		activeFont->maxHeight = activeFont->maxHeightLarge;
		activeFont->maxWidth = activeFont->maxWidthLarge;
	}
}

void sdFontManagerLocal::PaintChar(float x,float y,float width,float height,float scale,float	s,float	t,float	s2,float t2,const idMaterial *hShader, const idVec4 *color)
{
	float	w, h;
	w = width * scale;
	h = height * scale;

	if (deviceContextLocal.ClippedCoords(&x, &y, &w, &h, &s, &t, &s2, &t2)) {
		return;
	}

	deviceContextLocal.AdjustCoords(&x, &y, &w, &h);
	deviceContextLocal.DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader, color);
}

void sdFontManagerLocal::DrawEditCursor(float x, float y, float scale, const idVec4 *color)
{
	if ((int)(com_ticNumber >> 4) & 1) {
		return;
	}

	SetFontByScale(scale);
	float useScale = scale * useFont->glyphScale;
	const glyphInfo_t *glyph2 = &useFont->glyphs[(overStrikeMode) ? '_' : '|'];
	float	yadj = useScale * glyph2->top;
	PaintChar(x, y - yadj,glyph2->imageWidth,glyph2->imageHeight,useScale,glyph2->s,glyph2->t,glyph2->s2,glyph2->t2,glyph2->glyph, color);
}

int sdFontManagerLocal::DrawText(float x, float y, float scale, idVec4 color, const char *text, float adjust, int limit, int style, int cursor, bool calcOnly, int *rWidth)
{
	int			len, count;
	idVec4		newColor;
	const glyphInfo_t *glyph;
	float		useScale;
	SetFontByScale(scale);
	useScale = scale * useFont->glyphScale;
	count = 0;
	float start = x;

	if ((text && color.w != 0.0f) || calcOnly) {
		const unsigned char	*s = (const unsigned char *)text;
		memcpy(&newColor[0], &color[0], sizeof(idVec4));
		len = strlen(text);

		if (limit > 0 && len > limit) {
			len = limit;
		}

#ifdef _WCHAR_LANG
        if(AsASCIICharLang(text, (int)len))
		{
#endif
			while (s && *s && count < len) {
				if (*s < GLYPH_START || *s > GLYPH_END) {
					s++;
					continue;
				}

				glyph = &useFont->glyphs[*s];

				//
				// int yadj = Assets.textFont.glyphs[text[i]].bottom +
				// Assets.textFont.glyphs[text[i]].top; float yadj = scale *
				// (Assets.textFont.glyphs[text[i]].imageHeight -
				// Assets.textFont.glyphs[text[i]].height);
				//
				if (idStr::IsColor((const char *)s)) {
					if (*(s+1) == C_COLOR_DEFAULT) {
						newColor = color;
					} else {
						newColor = idStr::ColorForIndex(*(s+1));
						newColor[3] = color[3];
					}

					if (cursor == count || cursor == count+1) {
						float partialSkip = ((glyph->xSkip * useScale) + adjust) / 5.0f;

						if (cursor == count) {
							partialSkip *= 2.0f;
						}

						if(!calcOnly)
							DrawEditCursor(x - partialSkip, y, scale, &newColor);
					}

					s += 2;
					count += 2;
					continue;
				} else {
					float yadj = useScale * glyph->top;
					if(!calcOnly)
						PaintChar(x,y - yadj,glyph->imageWidth,glyph->imageHeight,useScale,glyph->s,glyph->t,glyph->s2,glyph->t2,glyph->glyph, &newColor);

					if (cursor == count) {
						if(!calcOnly)
							DrawEditCursor(x, y, scale, &newColor);
					}

					x += (glyph->xSkip * useScale) + adjust;
					s++;
					count++;
				}
			}
#ifdef _WCHAR_LANG
		}
		else
        {
            idStr drawText = text;
            int charIndex = 0;
            int lastCharIndex = 0;

            while( charIndex < len ) {
                lastCharIndex = charIndex;
                uint32_t textChar = drawText.UTF8Char( charIndex );

                glyph = R_Font_GetGlyphInfo(useFont, textChar);
                if (!glyph) {
                    continue;
                }

                //karin: charIndex will increment when read UTF8 character, so use last charIndex
                if( textChar == C_COLOR_ESCAPE && idStr::IsColor( drawText.c_str() + lastCharIndex ) ) {
                    // textChar == '^' and charIndex is color value current
                    if( drawText[ charIndex ] == C_COLOR_DEFAULT ) {
                        newColor = color;
                    } else {
                        newColor = idStr::ColorForIndex( drawText[ charIndex ] );
                        newColor[3] = color[3];
                    }
                    if( cursor == charIndex - 1 || cursor == charIndex ) {
                        float partialSkip = ((glyph->xSkip * useScale) + adjust) / 5.0f;

                        if (cursor == count) {
                            partialSkip *= 2.0f;
                        }

						if(!calcOnly)
                        DrawEditCursor(x - partialSkip, y, scale, &newColor);
                    }
                    charIndex++; //karin: skip color value character
                    continue;
                } else {
                    float yadj = useScale * glyph->top;
					if(!calcOnly)
                    PaintChar(x,y - yadj,glyph->imageWidth,glyph->imageHeight,useScale,glyph->s,glyph->t,glyph->s2,glyph->t2,glyph->glyph, &newColor);

                    if( cursor == charIndex - 1 ) {
						if(!calcOnly)
                        DrawEditCursor( x, y, scale, &newColor );
                    }

                    x += (glyph->xSkip * useScale) + adjust;
                }
            }
        }
#endif

		if (cursor == len) {
			if(!calcOnly)
			DrawEditCursor(x, y, scale, &newColor);
		}
	}

	if(rWidth)
		*rWidth = CALC_SIZE(x - start);
	if(calcOnly)
		count = 0;

	return count;
}

int sdFontManagerLocal::MaxCharWidth(float scale)
{
	SetFontByScale(scale);
	float useScale = scale * useFont->glyphScale;
	return idMath::FtoiFast(activeFont->maxWidth * useScale);
}

int sdFontManagerLocal::MaxCharHeight(float scale)
{
	SetFontByScale(scale);
	float useScale = scale * useFont->glyphScale;
	return idMath::FtoiFast(activeFont->maxHeight * useScale);
}

int sdFontManagerLocal::CharWidth(const char c, float scale)
{
	glyphInfo_t *glyph;
	float		useScale;
	SetFontByScale(scale);
	fontInfo_t	*font = useFont;
	useScale = scale * font->glyphScale;
	glyph = &font->glyphs[(const unsigned char)c];
	return idMath::FtoiFast(glyph->xSkip * useScale);
}

int sdFontManagerLocal::DrawText(const char *text, float textScale, int textAlign, idVec4 color, const sdBounds2D &rectDraw, bool wrap, bool noclipping, int cursor, bool calcOnly, idList<int> *breaks, int limit, int** charAdvances, int rSize[])
{
	const char	*p, *textPtr, *newLinePtr;
	char		buff[1024];
	int			len, newLine, newLineWidth, count;
	float		y;
	float		textWidth;

	float		charSkip = MaxCharWidth(textScale) + 1;
	float		lineSkip = MaxCharHeight(textScale);

	float		cursorSkip = (cursor >= 0 ? charSkip : 0);

	bool		lineBreak, wordBreak;
	int tWidth = CALC_SIZE(charSkip);
	float tHeight = 0;

	SetFontByScale(textScale);

	y = lineSkip + rectDraw.GetTop();
#if 0
	if(!wrap) // single line only
	{
		if (textAlign & ALIGN_BOTTOM) {
			y = rectDraw.GetBottom() - lineSkip;
		} else if (textAlign & ALIGN_MIDDLE) {
			y = lineSkip + rectDraw.GetTop() + (rectDraw.GetHeight() - lineSkip) * 0.5f;
			//y = rectDraw.GetBottom() - (rectDraw.GetHeight() - lineSkip) * 0.5f;
		}
	}
#endif

	textWidth = 0;
	newLinePtr = NULL;
	if (charAdvances && text && text[0])
		memset(*charAdvances, 0, idStr::Length(text));

	if (!calcOnly && !(text && *text)) {
		if (cursor == 0) {
			DrawEditCursor(rectDraw.GetLeft(), y, textScale, &color);
		}

		if(rSize)
		{
			rSize[0] = tWidth;
			rSize[1] = CALC_SIZE(lineSkip);
		}
		return idMath::FtoiFast(rectDraw.GetWidth() / charSkip);
	}

	tHeight = lineSkip;
	textPtr = text;
	len = 0;
	buff[0] = '\0';
	newLine = 0;
	newLineWidth = 0;
	p = textPtr;

	if (breaks) {
		breaks->Append(0);
	}

	count = 0;
	textWidth = 0;
	lineBreak = false;
	wordBreak = false;

#ifdef _WCHAR_LANG
    if(AsASCIICharLang(text, (int)strlen(text)))
	{
#endif
		while (p) {

			if (*p == '\n' || *p == '\r' || *p == '\0') {
				lineBreak = true;

				if ((*p == '\n' && *(p + 1) == '\r') || (*p == '\r' && *(p + 1) == '\n')) {
					p++;
				}
			}

			int nextCharWidth = (idStr::CharIsPrintable(*p) ? CharWidth(*p, textScale) : cursorSkip);
			const int charWidth = nextCharWidth;
			// FIXME: this is a temp hack until the guis can be fixed not not overflow the bounding rectangles
			//		  the side-effect is that list boxes and edit boxes will draw over their scroll bars
			//	The following line and the !linebreak in the if statement below should be removed
			nextCharWidth = 0;

			if (!lineBreak && (textWidth + nextCharWidth) > rectDraw.GetWidth()) {
				// The next character will cause us to overflow, if we haven't yet found a suitable
				// break spot, set it to be this character
				if ((!calcOnly || wrap) && !noclipping) //karin: continue if in calc mode and single line
				{
					if (len > 0 && newLine == 0) {
						newLine = len;
						newLinePtr = p;
						newLineWidth = textWidth;
					}

					wordBreak = true;
				}
			} else if (lineBreak || (wrap && (*p == ' ' || *p == '\t'))) {
				// The next character is in view, so if we are a break character, store our position
				newLine = len;
				newLinePtr = p + 1;
				newLineWidth = textWidth;
			}

			if (lineBreak || wordBreak) {
				float x = rectDraw.GetLeft();

				if (textAlign & ALIGN_RIGHT) {
					x = rectDraw.GetRight() - newLineWidth;
				} else if (textAlign & ALIGN_CENTER) {
					x = rectDraw.GetLeft() + (rectDraw.GetWidth() - newLineWidth) / 2;
				}

				if (wrap || newLine > 0) {
					buff[newLine] = '\0';

					// This is a special case to handle breaking in the middle of a word.
					// if we didn't do this, the cursor would appear on the end of this line
					// and the beginning of the next.
					if (wordBreak && cursor >= newLine && newLine == len) {
						cursor++;
					}
				}

				//if (!calcOnly) 
				{
					int tw = 0;
					count += DrawText(x, y, textScale, color, buff, 0, 0, 0, cursor, calcOnly, &tw);
					if(tw > tWidth)
						tWidth = tw;
				}

				if (cursor < newLine) {
					cursor = -1;
				} else if (cursor >= 0) {
					cursor -= (newLine + 1);
				}

				if (!wrap && !calcOnly) {
					if(rSize)
					{
						rSize[0] = tWidth;
						rSize[1] = CALC_SIZE(tHeight);
					}
					return newLine;
				}

				if ((limit && count > limit) || *p == '\0') {
					break;
				}

				y += lineSkip + DRAW_TEXT_LINE_SPACING;
				tHeight += lineSkip + DRAW_TEXT_LINE_SPACING;

				if (!calcOnly && y > rectDraw.GetBottom()) {
					break;
				}

				p = newLinePtr;

				if (breaks) {
					breaks->Append(p - text);
				}

				len = 0;
				newLine = 0;
				newLineWidth = 0;
				textWidth = 0;
				lineBreak = false;
				wordBreak = false;
				continue;
			}

			if (charAdvances)
				(*charAdvances)[p - text] = charWidth;
			buff[len++] = *p++;
			buff[len] = '\0';

			// update the width
			if (*(buff + len - 1) != C_COLOR_ESCAPE && (len <= 1 || *(buff + len - 2) != C_COLOR_ESCAPE)) {
				textWidth += textScale * useFont->glyphScale * useFont->glyphs[(const unsigned char)*(buff + len - 1)].xSkip;
			}
		}
#ifdef _WCHAR_LANG
	}
	else
    {
        idStr drawText = text;
        int			charIndex = 0;
        idStr textBuffer;
        int			lastBreak = 0;
        float		textWidthAtLastBreak = 0.0f;
		int index = 0;

        while( charIndex < drawText.Length() ) {
            uint32_t textChar = drawText.UTF8Char( charIndex );
        	index++;

            // See if we need to start a new line.
            if( textChar == '\n' || textChar == '\r' || charIndex == drawText.Length() ) {
                lineBreak = true;
                if( charIndex < drawText.Length() ) {
                    // New line character and we still have more text to read.
                    char nextChar = drawText[ charIndex + 1 ];
                    if( ( textChar == '\n' && nextChar == '\r' ) || ( textChar == '\r' && nextChar == '\n' ) ) {
                        // Just absorb extra newlines.
                        textChar = drawText.UTF8Char( charIndex );
        				index++;
                    }
                }
            }

            // Check for escape colors if not then simply get the glyph width.
            if( textChar == C_COLOR_ESCAPE && charIndex < drawText.Length() ) {
                textBuffer.AppendUTF8Char( textChar );
                textChar = drawText.UTF8Char( charIndex );
        		index++;
            }

            // If the character isn't a new line then add it to the text buffer.
            if( textChar != '\n' && textChar != '\r' ) {
            	const int charWidth = R_Font_GetCharWidth( useFont, textChar, textScale );
            	textWidth += charWidth;
            	if (charAdvances)
            		(*charAdvances)[index - 1] = charWidth;
                textBuffer.AppendUTF8Char( textChar );
            }

            if( !lineBreak && ( textWidth > rectDraw.GetWidth() ) ) {
                // The next character will cause us to overflow, if we haven't yet found a suitable
            	// break spot, set it to be this character
				if ((!calcOnly || wrap) && !noclipping) //karin: continue if in calc mode and single line
            	{
					if( textBuffer.Length() > 0 && lastBreak == 0 ) {
						lastBreak = textBuffer.Length();
						textWidthAtLastBreak = textWidth;
					}
					wordBreak = true;
				}
            } else if( lineBreak || ( wrap && ( textChar == ' ' || textChar == '\t' ) ) ) {
                // The next character is in view, so if we are a break character, store our position
                lastBreak = textBuffer.Length();
                textWidthAtLastBreak = textWidth;
            }

            // We need to go to a new line
            if( lineBreak || wordBreak ) {
                float x = rectDraw.GetLeft();

                if( textWidthAtLastBreak > 0 ) {
                    textWidth = textWidthAtLastBreak;
                }

                // Align text if needed
            	if( textAlign & ALIGN_RIGHT ) {
                    x = rectDraw.GetRight() - textWidth;
                } else if( textAlign & ALIGN_CENTER ) {
                    x = rectDraw.GetLeft() + ( rectDraw.GetWidth() - textWidth ) / 2;
                }

                if( wrap || lastBreak > 0 ) {
                    // This is a special case to handle breaking in the middle of a word.
                    // if we didn't do this, the cursor would appear on the end of this line
                    // and the beginning of the next.
                    if( wordBreak && cursor >= lastBreak && lastBreak == textBuffer.Length() ) {
                        cursor++;
                    }
                }

                // Draw what's in the current text buffer.
                //if( !calcOnly ) 
				{
					int tw = 0;
                    if( lastBreak > 0 ) {
                        count += DrawText( x, y, textScale, color, textBuffer.Left( lastBreak ).c_str(), 0, 0, 0, cursor, calcOnly, &tw );
                        textBuffer = textBuffer.Right( textBuffer.Length() - lastBreak );
                    } else {
                        count += DrawText( x, y, textScale, color, textBuffer.c_str(), 0, 0, 0, cursor, calcOnly, &tw );
                        textBuffer.Clear();
                    }
					if(tw > tWidth)
						tWidth = tw;
                }

                if( cursor < lastBreak ) {
                    cursor = -1;
                } else if( cursor >= 0 ) {
                    cursor -= ( lastBreak + 1 );
                }

                // If wrap is disabled return at this point.
                if( !wrap && !calcOnly ) {
					if(rSize)
					{
						rSize[0] = tWidth;
						rSize[1] = CALC_SIZE(tHeight);
					}
                    return lastBreak;
                }

                // If we've hit the allowed character limit then break.
                if( limit && count > limit ) {
                    break;
                }

                y += lineSkip + DRAW_TEXT_LINE_SPACING;
				tHeight += lineSkip + DRAW_TEXT_LINE_SPACING;

                if( !calcOnly && y > rectDraw.GetBottom() ) {
                    break;
                }

                // If breaks were requested then make a note of this one.
                if( breaks ) {
                    breaks->Append( drawText.Length() - charIndex );
                }

                // Reset necessary parms for next line.
                lastBreak = 0;
                textWidth = 0;
                textWidthAtLastBreak = 0;
                lineBreak = false;
                wordBreak = false;

                // Reassess the remaining width
                for( int i = 0; i < textBuffer.Length(); ) {
                    if( textChar != C_COLOR_ESCAPE ) {
                        textWidth += R_Font_GetCharWidth( useFont, textBuffer.UTF8Char( i ), textScale );
                    }
                }

                continue;
            }
        }
    }
#endif

	if(rSize)
	{
		rSize[0] = tWidth;
		rSize[1] = CALC_SIZE(tHeight);
	}
	return idMath::FtoiFast(rectDraw.GetWidth() / charSkip);
}

bool sdFontManagerLocal::ParseFontConfig(const char *path, sdLocFont_t &config) {
	idLexer src;
	src.SetFlags(LEXFL_ALLOWPATHNAMES);
	if(!src.LoadFile(path))
		return false;

	config.checksum = 0;
	config.fontId = -1;
	idToken token;
	while(true)
	{
		if(!src.ReadToken(&token))
			break;

		if(!idStr::Icmp(token, "file"))
		{
			if(!src.ReadToken(&token))
			{
				src.Error( "Parse font config: failed to parse file" );
				return false;
			}
			config.file = token.c_str();
			config.checksum = TrueTypeFontFileChecksum(token.c_str());
			continue;
		}

		if(!idStr::Icmp(token, "faceIndex"))
		{
			config.faceIndex = src.ParseInt();
			continue;
		}

		src.Warning( "Parse font config: unexpected token '%s'.", token.c_str() );
	}
	return true;
}

sdLocFont_t * sdFontManagerLocal::FindFontConfig(const char *name)
{
	for(int i = 0; i < fontConfigs.Num(); i++)
	{
		if(!idStr::Icmp(fontConfigs[i].name, name))
			return &fontConfigs[i];
	}
	return NULL;
}

void sdFontManagerLocal::LoadFontConfigs(const char *lang) {
	idStr path("localization");
	if(!lang || !lang[0])
		lang = cvarSystem->GetCVarString("sys_lang");
	path.AppendPath(lang);
	path.AppendPath("fonts");

	idFileList* fileList = fileSystem->ListFiles(path.c_str(), ".font");

	common->Printf("Load font config on %s.....\n", path.c_str());

	for (int i = 0; i < fileList->GetNumFiles(); i++)
	{
		idLexer src;
		idToken	token;
		idStr fileName = fileList->GetList()[i];

		idStr str(path);
		str.AppendPath(fileName);

		sdLocFont_t config;
		if(!ParseFontConfig(str.c_str(), config))
			continue;

		fileName.StripFileExtension();
		sdLocFont_t *exists = FindFontConfig(fileName.c_str());
		if(exists)
		{
			exists->file = config.file;
			exists->faceIndex = config.faceIndex;
			exists->checksum = config.checksum;
			common->Printf("Override %s font config '%s'.\n", lang, fileName.c_str());
		}
		else
		{
			config.name = fileName;
			config.fontId = -1;
			fontConfigs.Append(config);
			common->Printf("Add %s font config '%s'.\n", lang, fileName.c_str());
		}
	}

	fileSystem->FreeFileList(fileList);
}

int sdFontManagerLocal::DrawText(float x, float y, float scale, idVec4 color, const wchar_t *text, float adjust, int limit, int style, int cursor, bool calcOnly, int *rWidth)
{
	int			len, count;
	idVec4		newColor;
	const glyphInfo_t *glyph;
	float		useScale;
	SetFontByScale(scale);
	useScale = scale * useFont->glyphScale;
	count = 0;
	float start = x;

	if ((text && color.w != 0.0f) || calcOnly) {
		const wchar_t *s = text;
		memcpy(&newColor[0], &color[0], sizeof(idVec4));
		len = idWStr::Length(text);

		if (limit > 0 && len > limit) {
			len = limit;
		}

        while( s && *s ) {
            uint32_t textChar = *s;

            glyph = R_Font_GetGlyphInfo(useFont, textChar);
            if (!glyph) {
            	s++;
                continue;
            }

            if( idWStr::IsColor( s ) ) {
                // textChar == '^' and charIndex is color value current
				if (*(s+1) == WC_COLOR_DEFAULT) {
                    newColor = color;
                } else {
                    newColor = idWStr::ColorForIndex( *(s+1) );
                    newColor[3] = color[3];
                }
                if( cursor == count || cursor == count+1 ) {
                    float partialSkip = ((glyph->xSkip * useScale) + adjust) / 5.0f;

                    if (cursor == count) {
                        partialSkip *= 2.0f;
                    }

					if(!calcOnly)
                    DrawEditCursor(x - partialSkip, y, scale, &newColor);
                }
            	s += 2;
            	count += 2;
                continue;
            } else {
                float yadj = useScale * glyph->top;
				if(!calcOnly)
                PaintChar(x,y - yadj,glyph->imageWidth,glyph->imageHeight,useScale,glyph->s,glyph->t,glyph->s2,glyph->t2,glyph->glyph, &newColor);

                if( cursor == count ) {
					if(!calcOnly)
                    DrawEditCursor( x, y, scale, &newColor );
                }

            	x += (glyph->xSkip * useScale) + adjust;
            	s++;
            	count++;
            }
        }

		if (cursor == len) {
			if(!calcOnly)
			DrawEditCursor(x, y, scale, &newColor);
		}
	}

	if(rWidth)
		*rWidth = CALC_SIZE(x - start);
	if(calcOnly)
		count = 0;

	return count;
}

int sdFontManagerLocal::DrawText(const wchar_t *text, float textScale, int textAlign, idVec4 color, const sdBounds2D &rectDraw, bool wrap, bool noclipping, int cursor, bool calcOnly, idList<int> *breaks, int limit, int** charAdvances, int rSize[])
{
	const wchar_t	*p, *textPtr, *newLinePtr;
	wchar_t		buff[1024];
	int			len, newLine, newLineWidth, count;
	float		y;
	float		textWidth;

	float		charSkip = MaxCharWidth(textScale) + 1;
	float		lineSkip = MaxCharHeight(textScale);

	float		cursorSkip = (cursor >= 0 ? charSkip : 0);

	bool		lineBreak, wordBreak;
	int tWidth = CALC_SIZE(charSkip);
	float tHeight = 0;

	SetFontByScale(textScale);

	y = lineSkip + rectDraw.GetTop();
#if 0
	if(!wrap) // single line only
	{
		if (textAlign & ALIGN_BOTTOM) {
			y = rectDraw.GetBottom() - lineSkip;
		} else if (textAlign & ALIGN_MIDDLE) {
			y = lineSkip + rectDraw.GetTop() + (rectDraw.GetHeight() - lineSkip) * 0.5f;
			//y = rectDraw.GetBottom() - (rectDraw.GetHeight() - lineSkip) * 0.5f;
		}
	}
#endif

	textWidth = 0;
	newLinePtr = NULL;
	if (charAdvances && text && text[0])
		memset(*charAdvances, 0, idWStr::Length(text));

	if (!calcOnly && !(text && *text)) {
		if (cursor == 0) {
			DrawEditCursor(rectDraw.GetLeft(), y, textScale, &color);
		}

		if(rSize)
		{
			rSize[0] = tWidth;
			rSize[1] = CALC_SIZE(lineSkip);
		}
		return idMath::FtoiFast(rectDraw.GetWidth() / charSkip);
	}

	tHeight = lineSkip;
	textPtr = text;

	len = 0;
	buff[0] = L'\0';
	newLine = 0;
	newLineWidth = 0;
	p = textPtr;

	if (breaks) {
		breaks->Append(0);
	}

	count = 0;
	lineBreak = false;
	wordBreak = false;

	while (p) {
		if (*p == L'\n' || *p == L'\r' || *p == L'\0') {
			lineBreak = true;

			if ((*p == L'\n' && *(p + 1) == L'\r') || (*p == L'\r' && *(p + 1) == L'\n')) {
				p++;
			}
		}

		int nextCharWidth = (idStr::CharIsPrintable(*p) ? R_Font_GetCharWidth(useFont, *p, textScale) : cursorSkip);
		const int charWidth = nextCharWidth;
		// FIXME: this is a temp hack until the guis can be fixed not not overflow the bounding rectangles
		//		  the side-effect is that list boxes and edit boxes will draw over their scroll bars
		//	The following line and the !linebreak in the if statement below should be removed
		nextCharWidth = 0;

		if( !lineBreak && ( (textWidth + nextCharWidth) > rectDraw.GetWidth() ) ) {
			// The next character will cause us to overflow, if we haven't yet found a suitable
			// break spot, set it to be this character
			if ((!calcOnly || wrap) && !noclipping) //karin: continue if in calc mode and single line
			{
				if (len > 0 && newLine == 0) {
					newLine = len;
					newLinePtr = p;
					newLineWidth = textWidth;
				}

				wordBreak = true;
			}
		} else if( lineBreak || ( wrap && ( *p == L' ' || *p == L'\t' ) ) ) {
			// The next character is in view, so if we are a break character, store our position
			newLine = len;
			newLinePtr = p + 1;
			newLineWidth = textWidth;
        }

        if( lineBreak || wordBreak ) {
        	float x = rectDraw.GetLeft();

        	if (textAlign & ALIGN_RIGHT) {
        		x = rectDraw.GetRight() - newLineWidth;
        	} else if (textAlign & ALIGN_CENTER) {
        		x = rectDraw.GetLeft() + (rectDraw.GetWidth() - newLineWidth) / 2;
        	}

        	if (wrap || newLine > 0) {
        		buff[newLine] = L'\0';

        		// This is a special case to handle breaking in the middle of a word.
        		// if we didn't do this, the cursor would appear on the end of this line
        		// and the beginning of the next.
        		if (wordBreak && cursor >= newLine && newLine == len) {
        			cursor++;
        		}
        	}

            // Draw what's in the current text buffer.
            //if( !calcOnly ) 
			{
				int tw = 0;
				count += DrawText(x, y, textScale, color, buff, 0, 0, 0, cursor, calcOnly, &tw);
				if(tw > tWidth)
					tWidth = tw;
            }

        	if (cursor < newLine) {
        		cursor = -1;
        	} else if (cursor >= 0) {
        		cursor -= (newLine + 1);
        	}

        	if (!wrap && !calcOnly) {
				if(rSize)
				{
					rSize[0] = tWidth;
					rSize[1] = CALC_SIZE(tHeight);
				}
        		return newLine;
        	}

        	if ((limit && count > limit) || *p == L'\0') {
        		break;
        	}

        	y += lineSkip + DRAW_TEXT_LINE_SPACING;
			tHeight += lineSkip + DRAW_TEXT_LINE_SPACING;

        	if (!calcOnly && y > rectDraw.GetBottom()) {
        		break;
        	}

        	p = newLinePtr;

        	if (breaks) {
        		breaks->Append(p - text);
        	}

        	len = 0;
        	newLine = 0;
        	newLineWidth = 0;
        	textWidth = 0;
        	lineBreak = false;
        	wordBreak = false;
        	continue;
        }

		if (charAdvances)
			(*charAdvances)[p - text] = charWidth;
		buff[len++] = *p++;
		buff[len] = L'\0';

		// update the width
		if (*(buff + len - 1) != C_COLOR_ESCAPE && (len <= 1 || *(buff + len - 2) != C_COLOR_ESCAPE)) {
            textWidth += R_Font_GetCharWidth( useFont, *(buff + len - 1), textScale );
		}
    }

	if(rSize)
	{
		rSize[0] = tWidth;
		rSize[1] = CALC_SIZE(tHeight);
	}
	return idMath::FtoiFast(rectDraw.GetWidth() / charSkip);
}

void sdFontManagerLocal::DrawText( const char* text, const sdBounds2D& rect, int textAlign, bool wrap, bool noclipping, const idVec4 &color ) {
	DrawText(text, DC_DEFAULT_FONT_SCALE, textAlign, color, rect, wrap, noclipping, -1, false, NULL, 0);
}

void sdFontManagerLocal::GetTextDimensions( const char* text, const sdBounds2D& rect, int textAlign, bool wrap, bool noclipping, const qhandle_t font, const int pointSize, int& width, int& height, float* scale, int** charAdvances, idList< int >* lineBreaks ) {
	float fontScale = DC_DEFAULT_FONT_SCALE;
	SetFont(font);
	int size[2] = {0};
	DrawText(text, fontScale, textAlign, colorWhite, rect, wrap, noclipping, -1, true, lineBreaks, 0, charAdvances, size);
	width = size[0];
	height = size[1];

	if (scale)
		*scale = fontScale;
}

bool sdFontManagerLocal::RegisterFont(int index, const char *fileName, unsigned int check)
{
	if(check)
	{
		unsigned int checksum = ReadChecksum(fileName);
		if(checksum == 0)
			return false;
		if(checksum != check)
		{
			common->Printf("Font '%s' is outdate!\n", fileName);
			return false;
		}
		else
			common->DPrintf("Font '%s' checksum pass.\n", fileName);
	}
	bool loaded = renderSystem->RegisterFont(fileName, fonts[index]);
	if(loaded)
		common->Printf("Font '%s' is loaded.\n", fileName);
	else
		common->Printf("Font '%s' load fail.\n", fileName);
	return loaded;
}

void sdFontManagerLocal::ChecksumFileName(idStr &out, const char *fileName) const
{
	out = fileName;
	//out.StripFilename();
	out.AppendPath(FONT_CHECKSUM_FILE_NAME);
}

unsigned int sdFontManagerLocal::ReadChecksum(const char *fileName) const
{
	int ret = 0;
	char *chechsum = NULL;
	idStr checksumFile;

	ChecksumFileName(checksumFile, fileName);
	int length = fileSystem->ReadFile(checksumFile, (void **)&chechsum, NULL);
	if(length > 0)
	{
		idStr str;
		str.Append(chechsum, length);
		if(sscanf(str.c_str(), "%u", &ret) != 1)
		{
			common->Printf("Unable read font checksum in file %s: %s\n", checksumFile.c_str(), str.c_str());
		}
		else
		{
			common->Printf("Read font checksum in file %s: %u\n", checksumFile.c_str(), ret);
		}
	}
	else
	{
		common->Printf("Unable read font checksum file in %s\n", checksumFile.c_str());
	}
	Mem_Free(chechsum);
	return ret;
}

void sdFontManagerLocal::WriteChecksum(const char *fileName, unsigned int checksum) const
{
	idStr checksumFile;
	ChecksumFileName(checksumFile, fileName);
	idStr checksumStr = va("%u", checksum);
	fileSystem->WriteFile(checksumFile, checksumStr.c_str(), checksumStr.Length());
	common->Printf("Write font checksum '%s' to %s.\n", checksumStr.c_str(), checksumFile.c_str());
}

void sdFontManagerLocal::RemoveChecksum(const char *fileName) const
{
	idStr checksumFile;
	ChecksumFileName(checksumFile, fileName);
	if (fileSystem->ReadFile(checksumFile.c_str(), NULL) > 0)
	{
		fileSystem->RemoveFile(checksumFile);
		common->Printf("Remove old font checksum file '%s'.\n", checksumFile.c_str());
	}
}

bool sdFontManagerLocal::ConvertFont(const sdLocFont_t *fc, const char *name, const char *lang, const char *fileName) const
{
	common->Printf("Converting and caching true type font '%s' to DOOM3 font......\n", name);
	RemoveChecksum(fileName);
	if(R_ExportTrueTypeFont(fc->file.c_str(), name, lang, DEFAULT_FONT_TEXTURE_SIZE))
	{
		common->Printf("Convert and cached true type font '%s' to DOOM3 font successful.\n", name);
		WriteChecksum(fileName, fc->checksum);
		return true;
	}
	else	
	{
		common->Warning("Couldn't convert and cache true type font '%s' to DOOM3 font.", name);
		return false;
	}
}

unsigned int sdFontManagerLocal::TrueTypeFontFileChecksum(const char *file) const
{
	unsigned int checksum;
	void *data = NULL;
	int length = fileSystem->ReadFile(file, &data, NULL);
	if(length > 0)
		checksum = MD5_BlockChecksum(data, length);
	else
		checksum = 0;
	Mem_Free(data);
	return checksum;
}



sdFontManagerLocal fontManagerLocal;

sdFontManager* fontManager = &fontManagerLocal;
