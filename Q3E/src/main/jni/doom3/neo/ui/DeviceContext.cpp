/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "DeviceContext.h"

idVec4 idDeviceContext::colorPurple;
idVec4 idDeviceContext::colorOrange;
idVec4 idDeviceContext::colorYellow;
idVec4 idDeviceContext::colorGreen;
idVec4 idDeviceContext::colorBlue;
idVec4 idDeviceContext::colorRed;
idVec4 idDeviceContext::colorBlack;
idVec4 idDeviceContext::colorWhite;
idVec4 idDeviceContext::colorNone;


idCVar gui_smallFontLimit("gui_smallFontLimit", "0.30", CVAR_GUI | CVAR_ARCHIVE, "");
idCVar gui_mediumFontLimit("gui_mediumFontLimit", "0.60", CVAR_GUI | CVAR_ARCHIVE, "");
#ifdef _WCHAR_LANG
idCVar harm_gui_wideCharLang("harm_gui_wideCharLang", "0", CVAR_GUI | CVAR_BOOL | CVAR_ARCHIVE, "enable wide-character language support");
static bool _hasWideCharFont = false;
#define AsASCIICharLang(text_, len_) ( !_hasWideCharFont || !harm_gui_wideCharLang.GetBool() || idStr::IsPureASCII(text_, len_) )
#endif
#ifdef _D3BFG_FONT
idCVar harm_gui_useD3BFGFont("harm_gui_useD3BFGFont", "", CVAR_GUI | CVAR_INIT | CVAR_ARCHIVE, "using DOOM3-BFG new fonts instead of old fonts.\n"
		"    0 or \"\": disable\n"
        "    1: make DOOM3 old fonts mapping to DOOM3-BFG new fonts automatic"
#ifdef _RAVEN
        "(`r_strogg` and `strogg` fonts always disable)"
#elif defined(_HUMANHEAD)
        "(`alien` font always disable)"
#endif
        ". e.g. \n"
#ifdef _RAVEN
        "        'fonts/chain_**.dat' -> 'newfonts/Chainlink_Semi_Bold/48.dat'\n"
		"        'fonts/lowpixel_**.dat' -> 'newfonts/microgrammadbolext/48.dat'\n"
        "        'fonts/marine_**.dat' -> 'newfonts/Arial_Narrow/48.dat'\n"
        "        'fonts/profont_**.dat' -> 'newfonts/BankGothic_Md_BT/48.dat'\n"
#elif defined(_HUMANHEAD)
        "        'fonts/fontImage_**.dat' -> 'newfonts/Chainlink_Semi_Bold/48.dat'\n"
        "        'fonts/menu/fontImage_**.dat' -> 'newfonts/Arial_Narrow/48.dat'\n"
#else
		"        'fonts/fontImage_**.dat' -> 'newfonts/Chainlink_Semi_Bold/48.dat'\n"
		"        'fonts/an/fontImage_**.dat' -> 'newfonts/Arial_Narrow/48.dat'\n"
		"        'fonts/arial/fontImage_**.dat' -> 'newfonts/Arial_Narrow/48.dat'\n"
		"        'fonts/bank/fontImage_**.dat' -> 'newfonts/BankGothic_Md_BT/48.dat'\n"
		"        'fonts/micro/fontImage_**.dat' -> 'newfonts/microgrammadbolext/48.dat'\n"
#endif
		"    Otherwise you can setup DOOM3-BFG new font name to override all DOOM 3/Quake 4/Prey old fonts. e.g. \n"
		"        Chainlink_Semi_Bold\n"
		"        Arial_Narrow\n"
		"        BankGothic_Md_BT\n"
		"        microgrammadbolext\n"
		"        DFPHeiseiGothicW7\n"
		"        Sarasori_Rg\n"
		);
#endif


idList<fontInfoEx_t> idDeviceContext::fonts;

#ifdef _RAVEN //k: I am not find Quake4 default font named "", so using cvar to control
const char	*harm_gui_defaultFontArgs[]	= {
	"chain", 
	"lowpixel", 
	"marine", 
	"profont", 
	"r_strogg", 
	"strogg", 
	NULL };
static idCVar harm_gui_defaultFont("harm_gui_defaultFont", harm_gui_defaultFontArgs[0], CVAR_ARCHIVE | CVAR_GUI, "Setup default GUI font. It will be available in next running.", harm_gui_defaultFontArgs, idCmdSystem::ArgCompletion_String<harm_gui_defaultFontArgs>);
#endif
int idDeviceContext::FindFont(const char *name)
{
	int c = fonts.Num();

	for (int i = 0; i < c; i++) {
		if (idStr::Icmp(name, fonts[i].name) == 0) {
			return i;
		}
	}

	// If the font was not found, try to register it
	idStr fileName = name;
#ifdef _RAVEN //k: Quake4 default font
	if(!idStr::Icmp(fileName, "fonts"))
	{
		fileName = "fonts/";
		const char *defFontName = harm_gui_defaultFont.GetString();
		if(!defFontName || !defFontName[0])
			defFontName = harm_gui_defaultFontArgs[0];
		fileName += defFontName;
	}
#endif
	fileName.Replace("fonts", va("fonts/%s", fontLang.c_str()));

	fontInfoEx_t fontInfo;
    memset(&fontInfo, 0, sizeof(fontInfoEx_t)); // DG: initialize this //k 2025
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
#ifdef _RAVEN
            if(!idStr::Icmp("chain", fname))
                d3bfgFontName = "Chainlink_Semi_Bold";
            else if(!idStr::Icmp("lowpixel", fname))
                d3bfgFontName = "microgrammadbolext";
            else if(!idStr::Icmp("marine", fname))
                d3bfgFontName = "Arial_Narrow";
            else if(!idStr::Icmp("profont", fname))
                d3bfgFontName = "BankGothic_Md_BT";
            else if(!idStr::Icmp("r_strogg", fname))
                d3bfgFontName = NULL;
            else if(!idStr::Icmp("strogg", fname))
                d3bfgFontName = NULL;
            else
                d3bfgFontName = "Chainlink_Semi_Bold";
#elif defined(_HUMANHEAD)
            if(!idStr::Icmp("menu", fname))
                d3bfgFontName = "Arial_Narrow";
            else if(!idStr::Icmp("alien", fname))
                d3bfgFontName = NULL;
            else
                d3bfgFontName = "Chainlink_Semi_Bold";
#else
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
#endif
		}
        if(d3bfgFontName && d3bfgFontName[0])
        {
            idStr newFileName = fileName;
            newFileName.Replace(va("fonts/%s", fontLang.c_str()), "newfonts/");
            newFileName.StripFilename();
            newFileName.AppendPath(d3bfgFontName);
            if (renderSystem->RegisterFont(newFileName, fonts[index]))
            {
                common->Printf("Font '%s' using DOOM3-BFG new font '%s'.\n", name, newFileName.c_str());
                fontLoaded = true;
            }
            else // load default if fail
            {
                common->Printf("Font '%s' load DOOM3-BFG new font '%s' fail, try using default font.\n", name, newFileName.c_str());
                fontLoaded = renderSystem->RegisterFont(fileName, fonts[index]);
            }
        }
        else
        {
            common->Printf("Font '%s' not use DOOM3-BFG new font.\n", name);
            fontLoaded = renderSystem->RegisterFont(fileName, fonts[index]);
        }
	}
	else
#endif
	fontLoaded = renderSystem->RegisterFont(fileName, fonts[index]);
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
		return index;
	} else {
		common->Printf("Could not register font %s [%s]\n", name, fileName.c_str());
		return -1;
	}
}

void idDeviceContext::SetupFonts()
{
	fonts.SetGranularity(1);

	fontLang = cvarSystem->GetCVarString("sys_lang");

	// western european languages can use the english font
	if (fontLang == "french" || fontLang == "german" || fontLang == "spanish" || fontLang == "italian") {
		fontLang = "english";
	}

	// Default font has to be added first
	FindFont("fonts");
}

void idDeviceContext::SetFont(int num)
{
	if (num >= 0 && num < fonts.Num()) {
		activeFont = &fonts[num];
	} else {
		activeFont = &fonts[0];
	}
}


void idDeviceContext::Init()
{
	xScale = 0.0;
	SetSize(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
#ifdef _RAVEN // quake4 assets
	whiteImage = declManager->FindMaterial("gfx/guis/white.tga");
#else
	whiteImage = declManager->FindMaterial("guis/assets/white.tga");
#endif
	whiteImage->SetSort(SS_GUI);
	mbcs = false;
	SetupFonts();
	activeFont = &fonts[0];
	colorPurple = idVec4(1, 0, 1, 1);
	colorOrange = idVec4(1, 1, 0, 1);
	colorYellow = idVec4(0, 1, 1, 1);
	colorGreen = idVec4(0, 1, 0, 1);
	colorBlue = idVec4(0, 0, 1, 1);
	colorRed = idVec4(1, 0, 0, 1);
	colorWhite = idVec4(1, 1, 1, 1);
	colorBlack = idVec4(0, 0, 0, 1);
	colorNone = idVec4(0, 0, 0, 0);
#ifdef _RAVEN // quake4 assets
	cursorImages[CURSOR_ARROW] = declManager->FindMaterial("gfx/guis/guicursor_arrow.tga");
	cursorImages[CURSOR_HAND] = declManager->FindMaterial("gfx/guis/guicursor_hand.tga");
	scrollBarImages[SCROLLBAR_HBACK] = declManager->FindMaterial("gfx/guis/scrollbarh.tga");
	scrollBarImages[SCROLLBAR_VBACK] = declManager->FindMaterial("gfx/guis/scrollbarv.tga");
	scrollBarImages[SCROLLBAR_THUMB] = declManager->FindMaterial("gfx/guis/scrollbar_thumb.tga");
	scrollBarImages[SCROLLBAR_RIGHT] = declManager->FindMaterial("gfx/guis/scrollbar_right.tga");
	scrollBarImages[SCROLLBAR_LEFT] = declManager->FindMaterial("gfx/guis/scrollbar_left.tga");
	scrollBarImages[SCROLLBAR_UP] = declManager->FindMaterial("gfx/guis/scrollbar_up.tga");
	scrollBarImages[SCROLLBAR_DOWN] = declManager->FindMaterial("gfx/guis/scrollbar_down.tga");
#elif defined(_HUMANHEAD)
	cursorImages[CURSOR_ARROW] = declManager->FindMaterial("guis/assets/guicursor_arrow.tga");
	cursorImages[CURSOR_HAND] = declManager->FindMaterial("guis/assets/guicursor_hand.tga");
	scrollBarImages[SCROLLBAR_HBACK] = declManager->FindMaterial("ui/assets/scrollbarh.tga");
	scrollBarImages[SCROLLBAR_VBACK] = declManager->FindMaterial("guis/assets/scrollbarv.tga");
	scrollBarImages[SCROLLBAR_THUMB] = declManager->FindMaterial("guis/assets/scrollbar_thumb.tga");
	scrollBarImages[SCROLLBAR_RIGHT] = declManager->FindMaterial("ui/assets/scrollbar_right.tga");
	scrollBarImages[SCROLLBAR_LEFT] = declManager->FindMaterial("ui/assets/scrollbar_left.tga");
	scrollBarImages[SCROLLBAR_UP] = declManager->FindMaterial("ui/assets/scrollbar_up.tga");
	scrollBarImages[SCROLLBAR_DOWN] = declManager->FindMaterial("ui/assets/scrollbar_down.tga");
#else
	cursorImages[CURSOR_ARROW] = declManager->FindMaterial("ui/assets/guicursor_arrow.tga");
	cursorImages[CURSOR_HAND] = declManager->FindMaterial("ui/assets/guicursor_hand.tga");
	scrollBarImages[SCROLLBAR_HBACK] = declManager->FindMaterial("ui/assets/scrollbarh.tga");
	scrollBarImages[SCROLLBAR_VBACK] = declManager->FindMaterial("ui/assets/scrollbarv.tga");
	scrollBarImages[SCROLLBAR_THUMB] = declManager->FindMaterial("ui/assets/scrollbar_thumb.tga");
	scrollBarImages[SCROLLBAR_RIGHT] = declManager->FindMaterial("ui/assets/scrollbar_right.tga");
	scrollBarImages[SCROLLBAR_LEFT] = declManager->FindMaterial("ui/assets/scrollbar_left.tga");
	scrollBarImages[SCROLLBAR_UP] = declManager->FindMaterial("ui/assets/scrollbar_up.tga");
	scrollBarImages[SCROLLBAR_DOWN] = declManager->FindMaterial("ui/assets/scrollbar_down.tga");
#endif
	cursorImages[CURSOR_ARROW]->SetSort(SS_GUI);
	cursorImages[CURSOR_HAND]->SetSort(SS_GUI);
	scrollBarImages[SCROLLBAR_HBACK]->SetSort(SS_GUI);
	scrollBarImages[SCROLLBAR_VBACK]->SetSort(SS_GUI);
	scrollBarImages[SCROLLBAR_THUMB]->SetSort(SS_GUI);
	scrollBarImages[SCROLLBAR_RIGHT]->SetSort(SS_GUI);
	scrollBarImages[SCROLLBAR_LEFT]->SetSort(SS_GUI);
	scrollBarImages[SCROLLBAR_UP]->SetSort(SS_GUI);
	scrollBarImages[SCROLLBAR_DOWN]->SetSort(SS_GUI);
	cursor = CURSOR_ARROW;
	enableClipping = true;
	overStrikeMode = true;
	mat.Identity();
	origin.Zero();
	initialized = true;

	// DG: this is used for the "make sure menus are rendered as 4:3" hack
	fixScaleForMenu.Set(1, 1);
	fixOffsetForMenu.Set(0, 0);
    scaleMenusTo43 = false;
}

void idDeviceContext::Shutdown()
{
	fontName.Clear();
	clipRects.Clear();
#ifdef _WCHAR_LANG
	for(int i = 0; i < fonts.Num(); i++)
	{
		printf("Free font '%s'.\n", fonts[i].name);
		R_Font_FreeFontInfoEx(&fonts[i]);
	}
#endif
	fonts.Clear();
	Clear();
}

void idDeviceContext::Clear()
{
	initialized = false;
	useFont = NULL;
	activeFont = NULL;
	mbcs = false;
}

idDeviceContext::idDeviceContext()
{
	Clear();
}

void idDeviceContext::SetTransformInfo(const idVec3 &org, const idMat3 &m)
{
	origin = org;
	mat = m;
}

//
//  added method
void idDeviceContext::GetTransformInfo(idVec3 &org, idMat3 &m)
{
	m = mat;
	org = origin;
}
//

void idDeviceContext::PopClipRect()
{
	if (clipRects.Num()) {
		clipRects.RemoveIndex(clipRects.Num()-1);
	}
}

void idDeviceContext::PushClipRect(idRectangle r)
{
	clipRects.Append(r);
}

void idDeviceContext::PushClipRect(float x, float y, float w, float h)
{
	clipRects.Append(idRectangle(x, y, w, h));
}

bool idDeviceContext::ClippedCoords(float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2)
{

	if (enableClipping == false || clipRects.Num() == 0) {
		return false;
	}

	int c = clipRects.Num();

	while (--c > 0) {
		idRectangle *clipRect = &clipRects[c];

		float ox = *x;
		float oy = *y;
		float ow = *w;
		float oh = *h;

		if (ow <= 0.0f || oh <= 0.0f) {
			break;
		}

		if (*x < clipRect->x) {
			*w -= clipRect->x - *x;
			*x = clipRect->x;
		} else if (*x > clipRect->x + clipRect->w) {
			*x = *w = *y = *h = 0;
		}

		if (*y < clipRect->y) {
			*h -= clipRect->y - *y;
			*y = clipRect->y;
		} else if (*y > clipRect->y + clipRect->h) {
			*x = *w = *y = *h = 0;
		}

		if (*w > clipRect->w) {
			*w = clipRect->w - *x + clipRect->x;
		} else if (*x + *w > clipRect->x + clipRect->w) {
			*w = clipRect->Right() - *x;
		}

		if (*h > clipRect->h) {
			*h = clipRect->h - *y + clipRect->y;
		} else if (*y + *h > clipRect->y + clipRect->h) {
			*h = clipRect->Bottom() - *y;
		}

		if (s1 && s2 && t1 && t2 && ow > 0.0f) {
			float ns1, ns2, nt1, nt2;
			// upper left
			float u = (*x - ox) / ow;
			ns1 = *s1 * (1.0f - u) + *s2 * (u);

			// upper right
			u = (*x + *w - ox) / ow;
			ns2 = *s1 * (1.0f - u) + *s2 * (u);

			// lower left
			u = (*y - oy) / oh;
			nt1 = *t1 * (1.0f - u) + *t2 * (u);

			// lower right
			u = (*y + *h - oy) / oh;
			nt2 = *t1 * (1.0f - u) + *t2 * (u);

			// set values
			*s1 = ns1;
			*s2 = ns2;
			*t1 = nt1;
			*t2 = nt2;
		}
	}

	return (*w == 0 || *h == 0) ? true : false;
}


void idDeviceContext::AdjustCoords(float *x, float *y, float *w, float *h)
{
	if (x) {
		*x *= xScale;
		if(scaleMenusTo43)
		{
			*x *= fixScaleForMenu.x; // DG: for "render menus as 4:3" hack
			*x += fixOffsetForMenu.x;
		}
	}

	if (y) {
		*y *= yScale;
		if(scaleMenusTo43)
		{
			*y *= fixScaleForMenu.y; // DG: for "render menus as 4:3" hack
			*y += fixOffsetForMenu.y;
		}
	}

	if (w) {
		*w *= xScale;
		if(scaleMenusTo43)
		{
			*w *= fixScaleForMenu.x; // DG: for "render menus as 4:3" hack
		}
	}

	if (h) {
		*h *= yScale;
		if(scaleMenusTo43)
		{
			*h *= fixScaleForMenu.y; // DG: for "render menus as 4:3" hack
		}
	}
}

void idDeviceContext::DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *shader)
{
	idDrawVert verts[4];
	glIndex_t indexes[6];
	indexes[0] = 3;
	indexes[1] = 0;
	indexes[2] = 2;
	indexes[3] = 2;
	indexes[4] = 0;
	indexes[5] = 1;
	verts[0].xyz[0] = x;
	verts[0].xyz[1] = y;
	verts[0].xyz[2] = 0;
	verts[0].st[0] = s1;
	verts[0].st[1] = t1;
	verts[0].normal[0] = 0;
	verts[0].normal[1] = 0;
	verts[0].normal[2] = 1;
	verts[0].tangents[0][0] = 1;
	verts[0].tangents[0][1] = 0;
	verts[0].tangents[0][2] = 0;
	verts[0].tangents[1][0] = 0;
	verts[0].tangents[1][1] = 1;
	verts[0].tangents[1][2] = 0;
	verts[1].xyz[0] = x + w;
	verts[1].xyz[1] = y;
	verts[1].xyz[2] = 0;
	verts[1].st[0] = s2;
	verts[1].st[1] = t1;
	verts[1].normal[0] = 0;
	verts[1].normal[1] = 0;
	verts[1].normal[2] = 1;
	verts[1].tangents[0][0] = 1;
	verts[1].tangents[0][1] = 0;
	verts[1].tangents[0][2] = 0;
	verts[1].tangents[1][0] = 0;
	verts[1].tangents[1][1] = 1;
	verts[1].tangents[1][2] = 0;
	verts[2].xyz[0] = x + w;
	verts[2].xyz[1] = y + h;
	verts[2].xyz[2] = 0;
	verts[2].st[0] = s2;
	verts[2].st[1] = t2;
	verts[2].normal[0] = 0;
	verts[2].normal[1] = 0;
	verts[2].normal[2] = 1;
	verts[2].tangents[0][0] = 1;
	verts[2].tangents[0][1] = 0;
	verts[2].tangents[0][2] = 0;
	verts[2].tangents[1][0] = 0;
	verts[2].tangents[1][1] = 1;
	verts[2].tangents[1][2] = 0;
	verts[3].xyz[0] = x;
	verts[3].xyz[1] = y + h;
	verts[3].xyz[2] = 0;
	verts[3].st[0] = s1;
	verts[3].st[1] = t2;
	verts[3].normal[0] = 0;
	verts[3].normal[1] = 0;
	verts[3].normal[2] = 1;
	verts[3].tangents[0][0] = 1;
	verts[3].tangents[0][1] = 0;
	verts[3].tangents[0][2] = 0;
	verts[3].tangents[1][0] = 0;
	verts[3].tangents[1][1] = 1;
	verts[3].tangents[1][2] = 0;

	bool ident = !mat.IsIdentity();

	if (ident) {
		verts[0].xyz -= origin;
		verts[0].xyz *= mat;
		verts[0].xyz += origin;
		verts[1].xyz -= origin;
		verts[1].xyz *= mat;
		verts[1].xyz += origin;
		verts[2].xyz -= origin;
		verts[2].xyz *= mat;
		verts[2].xyz += origin;
		verts[3].xyz -= origin;
		verts[3].xyz *= mat;
		verts[3].xyz += origin;
	}

	renderSystem->DrawStretchPic(&verts[0], &indexes[0], 4, 6, shader, ident);

}


void idDeviceContext::DrawMaterial(float x, float y, float w, float h, const idMaterial *mat, const idVec4 &color, float scalex, float scaley)
{

	renderSystem->SetColor(color);

	float	s0, s1, t0, t1;

//
//  handle negative scales as well
	if (scalex < 0) {
		w *= -1;
		scalex *= -1;
	}

	if (scaley < 0) {
		h *= -1;
		scaley *= -1;
	}

//
	if (w < 0) {	// flip about vertical
		w  = -w;
		s0 = 1 * scalex;
		s1 = 0;
	} else {
		s0 = 0;
		s1 = 1 * scalex;
	}

	if (h < 0) {	// flip about horizontal
		h  = -h;
		t0 = 1 * scaley;
		t1 = 0;
	} else {
		t0 = 0;
		t1 = 1 * scaley;
	}

	if (ClippedCoords(&x, &y, &w, &h, &s0, &t0, &s1, &t1)) {
		return;
	}

	AdjustCoords(&x, &y, &w, &h);

	DrawStretchPic(x, y, w, h, s0, t0, s1, t1, mat);
}

void idDeviceContext::DrawMaterialRotated(float x, float y, float w, float h, const idMaterial *mat, const idVec4 &color, float scalex, float scaley, float angle)
{

	renderSystem->SetColor(color);

	float	s0, s1, t0, t1;

	//
	//  handle negative scales as well
	if (scalex < 0) {
		w *= -1;
		scalex *= -1;
	}

	if (scaley < 0) {
		h *= -1;
		scaley *= -1;
	}

	//
	if (w < 0) {	// flip about vertical
		w  = -w;
		s0 = 1 * scalex;
		s1 = 0;
	} else {
		s0 = 0;
		s1 = 1 * scalex;
	}

	if (h < 0) {	// flip about horizontal
		h  = -h;
		t0 = 1 * scaley;
		t1 = 0;
	} else {
		t0 = 0;
		t1 = 1 * scaley;
	}

	if (angle == 0.0f && ClippedCoords(&x, &y, &w, &h, &s0, &t0, &s1, &t1)) {
		return;
	}

	AdjustCoords(&x, &y, &w, &h);

	DrawStretchPicRotated(x, y, w, h, s0, t0, s1, t1, mat, angle);
}

void idDeviceContext::DrawStretchPicRotated(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *shader, float angle)
{

	idDrawVert verts[4];
	glIndex_t indexes[6];
	indexes[0] = 3;
	indexes[1] = 0;
	indexes[2] = 2;
	indexes[3] = 2;
	indexes[4] = 0;
	indexes[5] = 1;
	verts[0].xyz[0] = x;
	verts[0].xyz[1] = y;
	verts[0].xyz[2] = 0;
	verts[0].st[0] = s1;
	verts[0].st[1] = t1;
	verts[0].normal[0] = 0;
	verts[0].normal[1] = 0;
	verts[0].normal[2] = 1;
	verts[0].tangents[0][0] = 1;
	verts[0].tangents[0][1] = 0;
	verts[0].tangents[0][2] = 0;
	verts[0].tangents[1][0] = 0;
	verts[0].tangents[1][1] = 1;
	verts[0].tangents[1][2] = 0;
	verts[1].xyz[0] = x + w;
	verts[1].xyz[1] = y;
	verts[1].xyz[2] = 0;
	verts[1].st[0] = s2;
	verts[1].st[1] = t1;
	verts[1].normal[0] = 0;
	verts[1].normal[1] = 0;
	verts[1].normal[2] = 1;
	verts[1].tangents[0][0] = 1;
	verts[1].tangents[0][1] = 0;
	verts[1].tangents[0][2] = 0;
	verts[1].tangents[1][0] = 0;
	verts[1].tangents[1][1] = 1;
	verts[1].tangents[1][2] = 0;
	verts[2].xyz[0] = x + w;
	verts[2].xyz[1] = y + h;
	verts[2].xyz[2] = 0;
	verts[2].st[0] = s2;
	verts[2].st[1] = t2;
	verts[2].normal[0] = 0;
	verts[2].normal[1] = 0;
	verts[2].normal[2] = 1;
	verts[2].tangents[0][0] = 1;
	verts[2].tangents[0][1] = 0;
	verts[2].tangents[0][2] = 0;
	verts[2].tangents[1][0] = 0;
	verts[2].tangents[1][1] = 1;
	verts[2].tangents[1][2] = 0;
	verts[3].xyz[0] = x;
	verts[3].xyz[1] = y + h;
	verts[3].xyz[2] = 0;
	verts[3].st[0] = s1;
	verts[3].st[1] = t2;
	verts[3].normal[0] = 0;
	verts[3].normal[1] = 0;
	verts[3].normal[2] = 1;
	verts[3].tangents[0][0] = 1;
	verts[3].tangents[0][1] = 0;
	verts[3].tangents[0][2] = 0;
	verts[3].tangents[1][0] = 0;
	verts[3].tangents[1][1] = 1;
	verts[3].tangents[1][2] = 0;

	bool ident = !mat.IsIdentity();

	if (ident) {
		verts[0].xyz -= origin;
		verts[0].xyz *= mat;
		verts[0].xyz += origin;
		verts[1].xyz -= origin;
		verts[1].xyz *= mat;
		verts[1].xyz += origin;
		verts[2].xyz -= origin;
		verts[2].xyz *= mat;
		verts[2].xyz += origin;
		verts[3].xyz -= origin;
		verts[3].xyz *= mat;
		verts[3].xyz += origin;
	}

	//Generate a translation so we can translate to the center of the image rotate and draw
	idVec3 origTrans;
	origTrans.x = x+(w/2);
	origTrans.y = y+(h/2);
	origTrans.z = 0;


	//Rotate the verts about the z axis before drawing them
	idMat4 rotz;
	rotz.Identity();
	float sinAng = idMath::Sin(angle);
	float cosAng = idMath::Cos(angle);
	rotz[0][0] = cosAng;
	rotz[0][1] = sinAng;
	rotz[1][0] = -sinAng;
	rotz[1][1] = cosAng;

	for (int i = 0; i < 4; i++) {
		//Translate to origin
		verts[i].xyz -= origTrans;

		//Rotate
		verts[i].xyz = rotz * verts[i].xyz;

		//Translate back
		verts[i].xyz += origTrans;
	}


	renderSystem->DrawStretchPic(&verts[0], &indexes[0], 4, 6, shader, (angle == 0.0) ? false : true);
}

void idDeviceContext::DrawFilledRect(float x, float y, float w, float h, const idVec4 &color)
{

	if (color.w == 0.0f) {
		return;
	}

	renderSystem->SetColor(color);

	if (ClippedCoords(&x, &y, &w, &h, NULL, NULL, NULL, NULL)) {
		return;
	}

	AdjustCoords(&x, &y, &w, &h);
	DrawStretchPic(x, y, w, h, 0, 0, 0, 0, whiteImage);
}


void idDeviceContext::DrawRect(float x, float y, float w, float h, float size, const idVec4 &color)
{

	if (color.w == 0.0f) {
		return;
	}

	renderSystem->SetColor(color);

	if (ClippedCoords(&x, &y, &w, &h, NULL, NULL, NULL, NULL)) {
		return;
	}

	AdjustCoords(&x, &y, &w, &h);
	DrawStretchPic(x, y, size, h, 0, 0, 0, 0, whiteImage);
	DrawStretchPic(x + w - size, y, size, h, 0, 0, 0, 0, whiteImage);
	DrawStretchPic(x, y, w, size, 0, 0, 0, 0, whiteImage);
	DrawStretchPic(x, y + h - size, w, size, 0, 0, 0, 0, whiteImage);
}

void idDeviceContext::DrawMaterialRect(float x, float y, float w, float h, float size, const idMaterial *mat, const idVec4 &color)
{

	if (color.w == 0.0f) {
		return;
	}

	renderSystem->SetColor(color);
	DrawMaterial(x, y, size, h, mat, color);
	DrawMaterial(x + w - size, y, size, h, mat, color);
	DrawMaterial(x, y, w, size, mat, color);
	DrawMaterial(x, y + h - size, w, size, mat, color);
}


void idDeviceContext::SetCursor(int n)
{
	cursor = (n < CURSOR_ARROW || n >= CURSOR_COUNT) ? CURSOR_ARROW : n;
}

void idDeviceContext::DrawCursor(float *x, float *y, float size)
{
	if (*x < 0) {
		*x = 0;
	}

	if (*x >= vidWidth) {
		*x = vidWidth;
	}

	if (*y < 0) {
		*y = 0;
	}

	if (*y >= vidHeight) {
		*y = vidHeight;
	}

	renderSystem->SetColor(colorWhite);
	if(scaleMenusTo43)
	{
		// DG: I use this instead of plain AdjustCursorCoords and the following lines
		//     to scale menus and other fullscreen GUIs to 4:3 aspect ratio
		AdjustCursorCoords(x, y, &size, &size);
		float sizeW = size * fixScaleForMenu.x;
		float sizeH = size * fixScaleForMenu.y;
		float fixedX = *x * fixScaleForMenu.x + fixOffsetForMenu.x;
		float fixedY = *y * fixScaleForMenu.y + fixOffsetForMenu.y;
		DrawStretchPic(fixedX, fixedY, sizeW, sizeH, 0, 0, 1, 1, cursorImages[cursor]);
	}
	else
	{
		AdjustCoords(x, y, &size, &size);
		DrawStretchPic(*x, *y, size, size, 0, 0, 1, 1, cursorImages[cursor]);
	}
}
/*
 =======================================================================================================================
 =======================================================================================================================
 */

void idDeviceContext::PaintChar(float x,float y,float width,float height,float scale,float	s,float	t,float	s2,float t2,const idMaterial *hShader)
{
	float	w, h;
	w = width * scale;
	h = height * scale;

	if (ClippedCoords(&x, &y, &w, &h, &s, &t, &s2, &t2)) {
		return;
	}

	AdjustCoords(&x, &y, &w, &h);
	DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
}


void idDeviceContext::SetFontByScale(float scale)
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

int idDeviceContext::DrawText(float x, float y, float scale, idVec4 color, const char *text, float adjust, int limit, int style, int cursor)
{
	int			len, count;
	idVec4		newColor;
	const glyphInfo_t *glyph;
	float		useScale;
	SetFontByScale(scale);
	useScale = scale * useFont->glyphScale;
	count = 0;

	if (text && color.w != 0.0f) {
		const unsigned char	*s = (const unsigned char *)text;
		renderSystem->SetColor(color);
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
					} else {
						renderSystem->SetColor(newColor);
					}

					DrawEditCursor(x - partialSkip, y, scale);
				}

				renderSystem->SetColor(newColor);
				s += 2;
				count += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
#ifdef _RAVEN //karin: 2025 Q4D font->horiBearingY - 1.0
                yadj = yadj - 1.0f;
#endif
				PaintChar(x,y - yadj,glyph->imageWidth,glyph->imageHeight,useScale,glyph->s,glyph->t,glyph->s2,glyph->t2,glyph->glyph);

				if (cursor == count) {
					DrawEditCursor(x, y, scale);
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
                        } else {
                            renderSystem->SetColor(newColor);
                        }

                        DrawEditCursor(x - partialSkip, y, scale);
                    }
                    renderSystem->SetColor( newColor );
                    charIndex++; //karin: skip color value character
                    continue;
                } else {
                    float yadj = useScale * glyph->top;
#ifdef _RAVEN //karin: 2025 Q4D font->horiBearingY - 1.0
                    yadj = yadj - 1.0f;
#endif
                    PaintChar(x,y - yadj,glyph->imageWidth,glyph->imageHeight,useScale,glyph->s,glyph->t,glyph->s2,glyph->t2,glyph->glyph);

                    if( cursor == charIndex - 1 ) {
                        DrawEditCursor( x, y, scale );
                    }

                    x += (glyph->xSkip * useScale) + adjust;
                }
            }
        }
#endif

		if (cursor == len) {
			DrawEditCursor(x, y, scale);
		}
	}

	return count;
}

void idDeviceContext::SetSize(float width, float height)
{
	vidWidth = VIRTUAL_WIDTH;
	vidHeight = VIRTUAL_HEIGHT;
	xScale = yScale = 0.0f;

	if (width != 0.0f && height != 0.0f) {
		xScale = vidWidth * (1.0f / width);
		yScale = vidHeight * (1.0f / height);
	}
}

int idDeviceContext::CharWidth(const char c, float scale)
{
	glyphInfo_t *glyph;
	float		useScale;
	SetFontByScale(scale);
	fontInfo_t	*font = useFont;
	useScale = scale * font->glyphScale;
	glyph = &font->glyphs[(const unsigned char)c];
	return idMath::FtoiFast(glyph->xSkip * useScale);
}

int idDeviceContext::TextWidth(const char *text, float scale, int limit)
{
	int i, width;

	SetFontByScale(scale);
	const glyphInfo_t *glyphs = useFont->glyphs;

	if (text == NULL) {
		return 0;
	}

	width = 0;

#ifdef _WCHAR_LANG
    int len = (int)strlen(text);
    if(AsASCIICharLang(text, len))
    {
#endif
	if (limit > 0) {
		for (i = 0; text[i] != '\0' && i < limit; i++) {
			if (idStr::IsColor(text + i)) {
				i++;
			} else {
				width += glyphs[((const unsigned char *)text)[i]].xSkip;
			}
		}
	} else {
		for (i = 0; text[i] != '\0'; i++) {
			if (idStr::IsColor(text + i)) {
				i++;
			} else {
				width += glyphs[((const unsigned char *)text)[i]].xSkip;
			}
		}
	}

	return idMath::FtoiFast(scale * useFont->glyphScale * width);
#ifdef _WCHAR_LANG
    }
    else
    {
        idStr drawText = text;
        int charIndex = 0;
        float f = 0.0f;

        if (limit > 0) {
            while( charIndex < len ) {
                if(charIndex >= limit)
                    break;

                if( idStr::IsColor( drawText.c_str() + charIndex ) ) {
                    charIndex += 2; //skip 2 characters, because color is ^x format
                } else {
                    uint32_t textChar = drawText.UTF8Char( charIndex );
                    f += R_Font_GetCharWidth(useFont, textChar, scale);
                }
            }
        } else {
            while( charIndex < len ) {
                if( idStr::IsColor( drawText.c_str() + charIndex ) ) {
                    charIndex += 2; //skip 2 characters, because color is ^x format
                } else {
                    uint32_t textChar = drawText.UTF8Char( charIndex );
                    f += R_Font_GetCharWidth(useFont, textChar, scale);
                }
            }
        }
        return idMath::FtoiFast(f);
    }
#endif
}

int idDeviceContext::TextHeight(const char *text, float scale, int limit)
{
	int			len, count;
	float		max;
	glyphInfo_t *glyph;
	float		useScale;
	const char	*s = text;
	SetFontByScale(scale);
	fontInfo_t	*font = useFont;

	useScale = scale * font->glyphScale;
	max = 0;

	if (text) {
		len = strlen(text);

		if (limit > 0 && len > limit) {
			len = limit;
		}

		count = 0;

#ifdef _WCHAR_LANG
        if(AsASCIICharLang(text, len))
        {
#endif
		while (s && *s && count < len) {
			if (idStr::IsColor(s)) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[*(const unsigned char *)s];

				if (max < glyph->height) {
					max = glyph->height;
				}

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
            float f = 0.0f;

            while( charIndex < len ) {
                if ( idStr::IsColor( drawText.c_str() + charIndex ) ) {
                    charIndex += 2;
                    continue;
                } else {
                    uint32_t textChar = drawText.UTF8Char( charIndex );
                    f = R_Font_GetCharHeight(useFont, textChar, scale);

                    if (max < f) {
                        max = f;
                    }
                }
            }

            return idMath::FtoiFast(max);
        }
#endif
	}

	return idMath::FtoiFast(max * useScale);
}

int idDeviceContext::MaxCharWidth(float scale)
{
	SetFontByScale(scale);
	float useScale = scale * useFont->glyphScale;
	return idMath::FtoiFast(activeFont->maxWidth * useScale);
}

int idDeviceContext::MaxCharHeight(float scale)
{
	SetFontByScale(scale);
	float useScale = scale * useFont->glyphScale;
	return idMath::FtoiFast(activeFont->maxHeight * useScale);
}

const idMaterial *idDeviceContext::GetScrollBarImage(int index)
{
	if (index >= SCROLLBAR_HBACK && index < SCROLLBAR_COUNT) {
		return scrollBarImages[index];
	}

	return scrollBarImages[SCROLLBAR_HBACK];
}

// this only supports left aligned text
idRegion *idDeviceContext::GetTextRegion(const char *text, float textScale, idRectangle rectDraw, float xStart, float yStart)
{
#if 0
	const char	*p, *textPtr, *newLinePtr;
	char		buff[1024];
	int			len, textWidth, newLine, newLineWidth;
	float		y;

	float charSkip = MaxCharWidth(textScale) + 1;
	float lineSkip = MaxCharHeight(textScale);

	textWidth = 0;
	newLinePtr = NULL;
#endif
	return NULL;
	/*
		if (text == NULL) {
			return;
		}

		textPtr = text;
		if (*textPtr == '\0') {
			return;
		}

		y = lineSkip + rectDraw.y + yStart;
		len = 0;
		buff[0] = '\0';
		newLine = 0;
		newLineWidth = 0;
		p = textPtr;

		textWidth = 0;
		while (p) {
			if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\0') {
				newLine = len;
				newLinePtr = p + 1;
				newLineWidth = textWidth;
			}

			if ((newLine && textWidth > rectDraw.w) || *p == '\n' || *p == '\0') {
				if (len) {

					float x = rectDraw.x ;

					buff[newLine] = '\0';
					DrawText(x, y, textScale, color, buff, 0, 0, 0);
					if (!wrap) {
						return;
					}
				}

				if (*p == '\0') {
					break;
				}

				y += lineSkip + 5;
				p = newLinePtr;
				len = 0;
				newLine = 0;
				newLineWidth = 0;
				continue;
			}

			buff[len++] = *p++;
			buff[len] = '\0';
			textWidth = TextWidth( buff, textScale, -1 );
		}
	*/
}

void idDeviceContext::DrawEditCursor(float x, float y, float scale)
{
	if ((int)(com_ticNumber >> 4) & 1) {
		return;
	}

	SetFontByScale(scale);
	float useScale = scale * useFont->glyphScale;
	const glyphInfo_t *glyph2 = &useFont->glyphs[(overStrikeMode) ? '_' : '|'];
	float	yadj = useScale * glyph2->top;
#ifdef _RAVEN //karin: 2025 Q4D font->horiBearingY - 1.0
    yadj = yadj - 1.0f;
#endif
	PaintChar(x, y - yadj,glyph2->imageWidth,glyph2->imageHeight,useScale,glyph2->s,glyph2->t,glyph2->s2,glyph2->t2,glyph2->glyph);
}

int idDeviceContext::DrawText(const char *text, float textScale, int textAlign, idVec4 color, idRectangle rectDraw, bool wrap, int cursor, bool calcOnly, idList<int> *breaks, int limit)
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

	SetFontByScale(textScale);

	textWidth = 0;
	newLinePtr = NULL;

	if (!calcOnly && !(text && *text)) {
		if (cursor == 0) {
			renderSystem->SetColor(color);
			DrawEditCursor(rectDraw.x, lineSkip + rectDraw.y, textScale);
		}

		return idMath::FtoiFast(rectDraw.w / charSkip);
	}

	textPtr = text;

	y = lineSkip + rectDraw.y;
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
		// FIXME: this is a temp hack until the guis can be fixed not not overflow the bounding rectangles
		//		  the side-effect is that list boxes and edit boxes will draw over their scroll bars
		//	The following line and the !linebreak in the if statement below should be removed
		nextCharWidth = 0;

		if (!lineBreak && (textWidth + nextCharWidth) > rectDraw.w) {
			// The next character will cause us to overflow, if we haven't yet found a suitable
			// break spot, set it to be this character
			if (len > 0 && newLine == 0) {
				newLine = len;
				newLinePtr = p;
				newLineWidth = textWidth;
			}

			wordBreak = true;
		} else if (lineBreak || (wrap && (*p == ' ' || *p == '\t'))) {
			// The next character is in view, so if we are a break character, store our position
			newLine = len;
			newLinePtr = p + 1;
			newLineWidth = textWidth;
		}

		if (lineBreak || wordBreak) {
			float x = rectDraw.x;

			if (textAlign == ALIGN_RIGHT) {
				x = rectDraw.x + rectDraw.w - newLineWidth;
			} else if (textAlign == ALIGN_CENTER) {
				x = rectDraw.x + (rectDraw.w - newLineWidth) / 2;
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

			if (!calcOnly) {
				count += DrawText(x, y, textScale, color, buff, 0, 0, 0, cursor);
			}

			if (cursor < newLine) {
				cursor = -1;
			} else if (cursor >= 0) {
				cursor -= (newLine + 1);
			}

			if (!wrap) {
				return newLine;
			}

			if ((limit && count > limit) || *p == '\0') {
				break;
			}

			y += lineSkip + 5;

			if (!calcOnly && y > rectDraw.Bottom()) {
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

        while( charIndex < drawText.Length() ) {
            uint32_t textChar = drawText.UTF8Char( charIndex );

            // See if we need to start a new line.
            if( textChar == '\n' || textChar == '\r' || charIndex == drawText.Length() ) {
                lineBreak = true;
                if( charIndex < drawText.Length() ) {
                    // New line character and we still have more text to read.
                    char nextChar = drawText[ charIndex + 1 ];
                    if( ( textChar == '\n' && nextChar == '\r' ) || ( textChar == '\r' && nextChar == '\n' ) ) {
                        // Just absorb extra newlines.
                        textChar = drawText.UTF8Char( charIndex );
                    }
                }
            }

            // Check for escape colors if not then simply get the glyph width.
            if( textChar == C_COLOR_ESCAPE && charIndex < drawText.Length() ) {
                textBuffer.AppendUTF8Char( textChar );
                textChar = drawText.UTF8Char( charIndex );
            }

            // If the character isn't a new line then add it to the text buffer.
            if( textChar != '\n' && textChar != '\r' ) {
                textWidth += R_Font_GetCharWidth( useFont, textChar, textScale );
                textBuffer.AppendUTF8Char( textChar );
            }

            if( !lineBreak && ( textWidth > rectDraw.w ) ) {
                // The next character will cause us to overflow, if we haven't yet found a suitable
                // break spot, set it to be this character
                if( textBuffer.Length() > 0 && lastBreak == 0 ) {
                    lastBreak = textBuffer.Length();
                    textWidthAtLastBreak = textWidth;
                }
                wordBreak = true;
            } else if( lineBreak || ( wrap && ( textChar == ' ' || textChar == '\t' ) ) ) {
                // The next character is in view, so if we are a break character, store our position
                lastBreak = textBuffer.Length();
                textWidthAtLastBreak = textWidth;
            }

            // We need to go to a new line
            if( lineBreak || wordBreak ) {
                float x = rectDraw.x;

                if( textWidthAtLastBreak > 0 ) {
                    textWidth = textWidthAtLastBreak;
                }

                // Align text if needed
                if( textAlign == ALIGN_RIGHT ) {
                    x = rectDraw.x + rectDraw.w - textWidth;
                } else if( textAlign == ALIGN_CENTER ) {
                    x = rectDraw.x + ( rectDraw.w - textWidth ) / 2;
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
                if( !calcOnly ) {
                    if( lastBreak > 0 ) {
                        count += DrawText( x, y, textScale, color, textBuffer.Left( lastBreak ).c_str(), 0, 0, 0, cursor );
                        textBuffer = textBuffer.Right( textBuffer.Length() - lastBreak );
                    } else {
                        count += DrawText( x, y, textScale, color, textBuffer.c_str(), 0, 0, 0, cursor );
                        textBuffer.Clear();
                    }
                }

                if( cursor < lastBreak ) {
                    cursor = -1;
                } else if( cursor >= 0 ) {
                    cursor -= ( lastBreak + 1 );
                }

                // If wrap is disabled return at this point.
                if( !wrap ) {
                    return lastBreak;
                }

                // If we've hit the allowed character limit then break.
                if( limit && count > limit ) {
                    break;
                }

                y += lineSkip + 5;

                if( !calcOnly && y > rectDraw.Bottom() ) {
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

	return idMath::FtoiFast(rectDraw.w / charSkip);
}

/*
=============
idRectangle::String
=============
*/
char *idRectangle::String(void) const
{
	static	int		index = 0;
	static	char	str[ 8 ][ 48 ];
	char	*s;

	// use an array so that multiple toString's won't collide
	s = str[ index ];
	index = (index + 1)&7;

	sprintf(s, "%.2f %.2f %.2f %.2f", x, y, w, h);

	return s;
}

// DG: this is used for the "make sure menus are rendered as 4:3" hack
void idDeviceContext::SetMenuScaleFix(bool enable) {
    scaleMenusTo43 = enable;
    
	if(enable) {
		float w = renderSystem->GetScreenWidth();
		float h = renderSystem->GetScreenHeight();
		float aspectRatio = w/h;
		static const float virtualAspectRatio = float(VIRTUAL_WIDTH)/float(VIRTUAL_HEIGHT); // 4:3
		if(aspectRatio > 1.4f) {
			// widescreen (4:3 is 1.333 3:2 is 1.5, 16:10 is 1.6, 16:9 is 1.7778)
			// => we need to scale and offset X
			// All the coordinates here assume 640x480 (VIRTUAL_WIDTH x VIRTUAL_HEIGHT)
			// screensize, so to fit a 4:3 menu into 640x480 stretched to a widescreen,
			// we need do decrease the width to something smaller than 640 and center
			// the result with an offset
			float scaleX = virtualAspectRatio/aspectRatio;
			float offsetX = (1.0f-scaleX)*(VIRTUAL_WIDTH*0.5f); // (640 - scale*640)/2
			fixScaleForMenu.Set(scaleX, 1);
			fixOffsetForMenu.Set(offsetX, 0);
		} else if(aspectRatio < 1.24f) {
			// portrait-mode, "thinner" than 5:4 (which is 1.25)
			// => we need to scale and offset Y
			// it's analogue to the other case, but inverted and with height and Y
			float scaleY = aspectRatio/virtualAspectRatio;
			float offsetY = (1.0f - scaleY)*(VIRTUAL_HEIGHT*0.5f); // (480 - scale*480)/2
			fixScaleForMenu.Set(1, scaleY);
			fixOffsetForMenu.Set(0, offsetY);
		}
	} else {
		fixScaleForMenu.Set(1, 1);
		fixOffsetForMenu.Set(0, 0);
	}
}

// DG: same as AdjustCoords, but ignore fixupMenus because for the cursor that must be handled seperately
void idDeviceContext::AdjustCursorCoords(float *x, float *y, float *w, float *h) {
	if (x) {
		*x *= xScale;
	}
	if (y) {
		*y *= yScale;
	}
	if (w) {
		*w *= xScale;
	}
	if (h) {
		*h *= yScale;
	}
}
