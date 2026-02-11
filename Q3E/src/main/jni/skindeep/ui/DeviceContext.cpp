/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

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

#include "sys/platform.h"
#include "idlib/geometry/DrawVert.h"

#include "ui/DeviceContext.h"

idVec4 idDeviceContext::colorPurple;
idVec4 idDeviceContext::colorOrange;
idVec4 idDeviceContext::colorYellow;
idVec4 idDeviceContext::colorGreen;
idVec4 idDeviceContext::colorBlue;
idVec4 idDeviceContext::colorRed;
idVec4 idDeviceContext::colorBlack;
idVec4 idDeviceContext::colorWhite;
idVec4 idDeviceContext::colorNone;

idCVar lang_enable_breakbefore(	"lang_enable_breakbefore",	"1", CVAR_GAME | CVAR_BOOL,	"enables jp can break before char ruleset" );

// SM: Removed with BFG changes
// idList<fontInfoEx_t> idDeviceContext::fonts;
// 
// int idDeviceContext::FindFont( const char *name ) {
// 	int c = fonts.Num();
// 	for (int i = 0; i < c; i++) {
// 		if (idStr::Icmp(name, fonts[i].name) == 0) {
// 			return i;
// 		}
// 	}
// 
// 	// If the font was not found, try to register it
// 	idStr fileName = name;
// 	fileName.Replace("fonts", va("fonts/%s", fontLang.c_str()) );
// 
// 	fontInfoEx_t fontInfo;
// 		int index = fonts.Append( fontInfo );
// 		if ( renderSystem->RegisterFont( fileName, fonts[index] ) ){
// 		idStr::Copynz( fonts[index].name, name, sizeof( fonts[index].name ) );
// 		return index;
// 		} else {
// 		common->Printf( "Could not register font %s [%s]\n", name, fileName.c_str() );
// 		return -1;
// }
// }
// 
// void idDeviceContext::SetupFonts() {
// 	fonts.SetGranularity( 1 );
// 
// 	fontLang = cvarSystem->GetCVarString( "sys_lang" );
// 
// 	// western european languages can use the english font
// 	if ( fontLang == "french" || fontLang == "german" || fontLang == "spanish" || fontLang == "italian" ) {
// 		fontLang = "english";
// 	}
// 
// 	// Default font has to be added first
// 	FindFont( "fonts" );
// }
// 
// void idDeviceContext::SetFont( int num ) {
// 	if ( num >= 0 && num < fonts.Num() ) {
// 		activeFont = &fonts[num];
// 	} else {
// 		activeFont = &fonts[0];
// 	}
// }


// blendo eric: ordered common break opportunity characters in unicode-32 languages
// these break AFTER
enum UNI_CANBREAK_CHARS
{   // 0000 ascii start
	UNI_0009_TAB = 0x00000009,
	UNI_0020_SPACE = 0x00000020,
	UNI_003A_COLON = 0x0000003A,
	UNI_0097_EM_DASH = 0x00000097,

	// 2000 general punctuation block
	UNI_2000_EM_SPACE = 0x00002000,
	UNI_2001_EM_SPACE = 0x00002001,
	UNI_2002_EM_SPACE = 0x00002002,
	UNI_2003_EM_SPACE = 0x00002003,
	UNI_2004_EM_SPACE = 0x00002004,
	UNI_2005_EM_SPACE = 0x00002005,
	UNI_2006_EM_SPACE = 0x00002006,
	// UNI_2007_EM_BREAK = 0x00002007,
	UNI_2008_EM_SPACE = 0x00002008,
	UNI_2009_EM_SPACE = 0x00002009,
	// UNI_2010_EM_BREAK = 0x00002010,
	UNI_200A_EM_SPACE = 0x0000200A,
	UNI_200B_ZERO_SPACE = 0x0000200B,
	UNI_200C_ZERO_SPACE = 0x0000200C,
	UNI_200D_ZERO_SPACE = 0x0000200D,
	UNI_2014_EM_DASH = 0x00002014,
	UNI_2024_ELLIPSIS1 = 0x00002024,
	UNI_2025_ELLIPSIS2 = 0x00002025,
	UNI_2026_ELLIPSIS3 = 0x00002026,
	UNI_2027_ELLIPSIS4 = 0x00002027,
	UNI_2028_HYPHENPOINT = 0x00002028,

	// 3000 CJK block (chinese japanese korean)
	UNI_3000_IDEO_SPACE = 0x00003000, // JP full-width space
	UNI_3001_IDEO_COMMA = 0x00003001, // JP full-width comma
	UNI_3002_IDEO_FULLSTOP = 0x00003002, // JP full-width period
	UNI_3009_IDEO_RIGHT_ANGLEBRACKET = 0x00003009,
	UNI_300B_RIGHT_DOUBLEANGLEBRACKET = 0x0000300B, 
	UNI_300D_RIGHT_CORNERBRACKET = 0x0000300D,
	UNI_300F_RIGHT_WHITEBRACKET = 0x0000300F,
	UNI_3011_RIGHT_LENBRACKET = 0x00003011,
	UNI_3015_RIGHT_SHELLBRACKET = 0x00003015,
	UNI_3017_RIGHT_LENWHITEBRACKET = 0x00003017,
	UNI_3019_RIGHT_SHELLWHITEBRACKET = 0x00003019,
	UNI_301B_RIGHT_WHITESQUAREBRACKET = 0x0000301B,
	UNI_301E_RIGHT_PRIME = 0x0000301E, // quotation
	UNI_301F_RIGHT_LOWPRIME = 0x0000301F, // low quotation

	UNI_303F_HALF_FILL_VIS = 0x0000303F, // visible half-width space

	UNI_30FB_INTERPUNCT = 0x000030FB, // JP word separator (rarely kanji number separator?)

	// FF00 full & half-width JP/KR block
	UNI_FF01_FULL_EXCLAIMATION = 0x0000FF01,
	UNI_FF09_FULL_RIGHT_PARENTHESIS = 0x0000FF09,
	UNI_FF0C_FULL_COMMA = 0x0000FF0C,
	UNI_FF0E_FULL_FULLSTOP = 0x0000FF0E,
	UNI_FF1A_FULL_COLON = 0x0000FF1A,
	UNI_FF1B_FULL_SEMICOLON = 0x0000FF1B,
	UNI_FF1F_FULL_QUESTION = 0x0000FF1F,
	UNI_FF3D_FULL_RIGHT_SQUAREBRACKET = 0x0000FF3D,
	UNI_FF60_FULL_RIGHT_WHITEPARENTHESIS = 0x0000FF60,

	UNI_FF61_HALF_FULLSTOP = 0x0000FF61,
	UNI_FF63_HALF_RIGHT_CORNERBRACKET = 0x0000FF63, 
	UNI_FF64_HALF_COMMA = 0x0000FF64,
	UNI_FF65_HALF_INTERPUNCT = 0x0000FF65, // JP word separator

	UNI_FFA0_HALF_WIDTH_FILL = 0x0000FFA0, // KR separator

	UNI_FFFF_LAST = 0x0000FFFF //  U+FFFF undefined char
};

const uint32 UNI_BLOCK_CODE_SIZE = 0x00001000; // unicode defined U+1000 block groups
const int UNI_CANBREAK_LIST_COUNT = 52;
const uint32 UNI_CANBREAK_LIST[UNI_CANBREAK_LIST_COUNT] = {
		UNI_0009_TAB,
		UNI_0020_SPACE,
		UNI_003A_COLON,
		UNI_0097_EM_DASH,
		UNI_2000_EM_SPACE,
		UNI_2001_EM_SPACE,
		UNI_2002_EM_SPACE,
		UNI_2003_EM_SPACE,
		UNI_2004_EM_SPACE,
		UNI_2005_EM_SPACE,
		UNI_2006_EM_SPACE,
		UNI_2008_EM_SPACE,
		UNI_2009_EM_SPACE,
		UNI_200A_EM_SPACE,
		UNI_200B_ZERO_SPACE,
		UNI_200C_ZERO_SPACE,
		UNI_200D_ZERO_SPACE,
		UNI_2014_EM_DASH,
		UNI_2024_ELLIPSIS1,
		UNI_2025_ELLIPSIS2,
		UNI_2026_ELLIPSIS3,
		UNI_2027_ELLIPSIS4,
		UNI_2028_HYPHENPOINT,
		UNI_3000_IDEO_SPACE,
		UNI_3001_IDEO_COMMA,
		UNI_3002_IDEO_FULLSTOP,
		UNI_300B_RIGHT_DOUBLEANGLEBRACKET,
		UNI_300D_RIGHT_CORNERBRACKET,
		UNI_300F_RIGHT_WHITEBRACKET,
		UNI_3011_RIGHT_LENBRACKET,
		UNI_3015_RIGHT_SHELLBRACKET,
		UNI_3017_RIGHT_LENWHITEBRACKET,
		UNI_3019_RIGHT_SHELLWHITEBRACKET,
		UNI_301B_RIGHT_WHITESQUAREBRACKET,
		UNI_301E_RIGHT_PRIME,
		UNI_301F_RIGHT_LOWPRIME,
		UNI_303F_HALF_FILL_VIS,
		UNI_30FB_INTERPUNCT,
		UNI_FF01_FULL_EXCLAIMATION,
		UNI_FF09_FULL_RIGHT_PARENTHESIS,
		UNI_FF0C_FULL_COMMA,
		UNI_FF0E_FULL_FULLSTOP,
		UNI_FF1A_FULL_COLON,
		UNI_FF1B_FULL_SEMICOLON,
		UNI_FF1F_FULL_QUESTION,
		UNI_FF3D_FULL_RIGHT_SQUAREBRACKET,
		UNI_FF60_FULL_RIGHT_WHITEPARENTHESIS,
		UNI_FF61_HALF_FULLSTOP,
		UNI_FF63_HALF_RIGHT_CORNERBRACKET,
		UNI_FF64_HALF_COMMA,
		UNI_FF65_HALF_INTERPUNCT,
		UNI_FFA0_HALF_WIDTH_FILL,
	};

const int UNI_CANBREAK_BLOCK_START_COUNT = 5;
const uint32 UNI_CANBREAK_LAST_BLOCK = UNI_BLOCK_CODE_SIZE * UNI_CANBREAK_BLOCK_START_COUNT;

// find the first index of character codes in or higher than each block
ID_INLINE int* UnicodeCanBreakBlockStartSetup()
{
	static int blockStartIndices[UNI_CANBREAK_BLOCK_START_COUNT];
	blockStartIndices[0] = 0;
	int curBlock = 0;
	uint32 nextBlockCode = UNI_BLOCK_CODE_SIZE;
	for (int idx = 1; idx < UNI_CANBREAK_LIST_COUNT; ++idx) {
		assert(UNI_CANBREAK_LIST[idx - 1] < UNI_CANBREAK_LIST[idx]); // verify order and uniqueness
		while (UNI_CANBREAK_LIST[idx] >= nextBlockCode && curBlock < UNI_CANBREAK_BLOCK_START_COUNT - 1) {
			curBlock++;
			blockStartIndices[curBlock] = idx;
			nextBlockCode += UNI_BLOCK_CODE_SIZE;
		}
	}
	return blockStartIndices;
}

// indices of first can break char in or above U+1000/0x1000 blocks
const int * UNI_CANBREAK_BLOCK_START_INDICES = UnicodeCanBreakBlockStartSetup(); 

// blendo eric: detect common break opportunity unicode-32 characters
ID_INLINE bool UnicodeCharacterCanBreakAfter(uint32 ch) {
	// start at the smallest break char index of U+1000/0x1000 blocks in break char list to speed up search
	int startChIdx = ch < UNI_CANBREAK_LAST_BLOCK ? UNI_CANBREAK_BLOCK_START_INDICES[(ch/UNI_BLOCK_CODE_SIZE)] : UNI_CANBREAK_BLOCK_START_INDICES[UNI_CANBREAK_BLOCK_START_COUNT-1];
	for (int chIdx = startChIdx; chIdx < UNI_CANBREAK_LIST_COUNT; chIdx++) {
		if (ch < UNI_CANBREAK_LIST[chIdx]) {
			return false; // try to exit out early
		} else if (ch == UNI_CANBREAK_LIST[chIdx]) {
			return true;
		}
	}
	return false;
}

// JP, allow break on phonemes/ideographs
// these can break BEFORE
const int UNI_CANBREAKBEFORE_RANGES_COUNT = 8;
const int UNI_CANBREAKBEFORE_RANGES[UNI_CANBREAKBEFORE_RANGES_COUNT][2] = {
	{0x00003041,0x00003096}, // jp-hiragana, excludes voiced marks
	{0x000030A1,0x000031FF}, // jp-katakana, excludes conjunction
	{0x000032D0,0x000032FE}, // jp-katakana, circled
	{0x00003300,0x00003357}, // jp-katakana, square
	{0x0000FF66,0x0000FF9D}, // jp-katakana halfwidth
	{0x000031F0,0x00003357}, // jp-katakana 2
	{0x00004E00,0x00009FFF}, // cjk-unified-ideograph (hanzi/kanji)
	{0x0000FF66,0x0000FF9D}, // jp-katakana fullwidth
};

ID_INLINE bool UnicodeCharacterCanBreakBefore(uint32 ch) {
	if (ch < UNI_CANBREAKBEFORE_RANGES[0][0] || ch > UNI_CANBREAKBEFORE_RANGES[UNI_CANBREAKBEFORE_RANGES_COUNT - 1][1]) {
		return false; // fast out of range of all blocks, useful for non jp lang
	}

	for (int idx = 0; idx < UNI_CANBREAKBEFORE_RANGES_COUNT; idx++) {
		if ( ch < UNI_CANBREAKBEFORE_RANGES[idx][0] ) { // out of range, early out
			return false;
		} else if ( ch <= UNI_CANBREAKBEFORE_RANGES[idx][1]) { // in range
			return true;
		}
	}
	return false;
}


const int UNI_CANBREAK_ZEROSPACE_COUNT = 4;
const uint32 UNI_CANBREAK_ZEROSPACE_LIST[UNI_CANBREAK_ZEROSPACE_COUNT] = {
		UNI_200B_ZERO_SPACE,
		UNI_200C_ZERO_SPACE,
		UNI_200D_ZERO_SPACE,
		UNI_303F_HALF_FILL_VIS, // usually visible, but invis here
};

// blendo eric: special language characters that do not affect rendering
bool UnicodeCharacterZeroSpace(uint32 ch) {
	if (ch < UNI_CANBREAK_ZEROSPACE_LIST[0]) {
		return false; // fast out, if it's below the first item
	}

	for (int idx = 0; idx < UNI_CANBREAK_ZEROSPACE_COUNT; idx++) {
		if (ch == UNI_CANBREAK_ZEROSPACE_LIST[idx]) {
			return true;
		}
	}
	return false;
}

void idDeviceContext::Init() {
	xScale = 0.0;
	SetSize(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
	whiteImage = declManager->FindMaterial("guis/assets/white.tga");
	whiteImage->SetSort( SS_GUI );
	mbcs = false;
	// SM: From BFG
	activeFont = renderSystem->RegisterFont( "" );
// 	SetupFonts();
// 	activeFont = &fonts[0];
	colorPurple = idVec4(1, 0, 1, 1);
	colorOrange = idVec4(1, 1, 0, 1);
	colorYellow = idVec4(0, 1, 1, 1);
	colorGreen = idVec4(0, 1, 0, 1);
	colorBlue = idVec4(0, 0, 1, 1);
	colorRed = idVec4(1, 0, 0, 1);
	colorWhite = idVec4(1, 1, 1, 1);
	colorBlack = idVec4(0, 0, 0, 1);
	colorNone = idVec4(0, 0, 0, 0);
	cursorImages[CURSOR_ARROW] = declManager->FindMaterial("ui/assets/guicursor_arrow");
	cursorImages[CURSOR_HAND] = declManager->FindMaterial("ui/assets/guicursor_hand");
	scrollBarImages[SCROLLBAR_HBACK] = declManager->FindMaterial("ui/assets/scrollbarh");
	scrollBarImages[SCROLLBAR_VBACK] = declManager->FindMaterial("ui/assets/scrollbarv");
	scrollBarImages[SCROLLBAR_THUMB] = declManager->FindMaterial("ui/assets/scrollbar_thumb");
	scrollBarImages[SCROLLBAR_RIGHT] = declManager->FindMaterial("ui/assets/scrollbar_right");
	scrollBarImages[SCROLLBAR_LEFT] = declManager->FindMaterial("ui/assets/scrollbar_left");
	scrollBarImages[SCROLLBAR_UP] = declManager->FindMaterial("ui/assets/scrollbar_up");
	scrollBarImages[SCROLLBAR_DOWN] = declManager->FindMaterial("ui/assets/scrollbar_down");
	cursorImages[CURSOR_ARROW]->SetSort( SS_GUI );
	cursorImages[CURSOR_HAND]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_HBACK]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_VBACK]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_THUMB]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_RIGHT]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_LEFT]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_UP]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_DOWN]->SetSort( SS_GUI );
	cursor = CURSOR_ARROW;
	enableClipping = true;
	overStrikeMode = true;
	mat.Identity();
	origin.Zero();
	initialized = true;

	activeFontMaterial = NULL; // blendo eric

	// DG: this is used for the "make sure menus are rendered as 4:3" hack
	fixScaleForMenu.Set(1, 1);
	fixOffsetForMenu.Set(0, 0);
}

void idDeviceContext::Shutdown() {
	fontName.Clear();
	clipRects.Clear();
	//fonts.Clear();
	Clear();
}

void idDeviceContext::Clear() {
	initialized = false;
	//useFont = NULL;
	activeFont = NULL;
	mbcs = false;
	activeFontMaterial = NULL; // blendo eric
}

idDeviceContext::idDeviceContext() {
	Clear();
}

void idDeviceContext::SetTransformInfo(const idVec3 &org, const idMat3 &m) {
	origin = org;
	mat = m;
}

//
//  added method
void idDeviceContext::GetTransformInfo(idVec3& org, idMat3& m )
{
	m = mat;
	org = origin;
}
//

void idDeviceContext::PopClipRect() {
	if (clipRects.Num()) {
		clipRects.RemoveIndex(clipRects.Num()-1);
	}
}

void idDeviceContext::PushClipRect(idRectangle r) {
	clipRects.Append(r);
}

void idDeviceContext::PushClipRect(float x, float y, float w, float h) {
	clipRects.Append(idRectangle(x, y, w, h));
}

bool idDeviceContext::ClippedCoords(float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2) {

	if ( enableClipping == false || clipRects.Num() == 0 ) {
		return false;
	}

	int c = clipRects.Num();
	while( --c > 0 ) {
		idRectangle *clipRect = &clipRects[c];

		float ox = *x;
		float oy = *y;
		float ow = *w;
		float oh = *h;

		if ( ow <= 0.0f || oh <= 0.0f ) {
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

		if ( s1 && s2 && t1 && t2 && ow > 0.0f ) {
			float ns1, ns2, nt1, nt2;
			// upper left
			float u = ( *x - ox ) / ow;
			ns1 = *s1 * ( 1.0f - u ) + *s2 * ( u );

			// upper right
			u = ( *x + *w - ox ) / ow;
			ns2 = *s1 * ( 1.0f - u ) + *s2 * ( u );

			// lower left
			u = ( *y - oy ) / oh;
			nt1 = *t1 * ( 1.0f - u ) + *t2 * ( u );

			// lower right
			u = ( *y + *h - oy ) / oh;
			nt2 = *t1 * ( 1.0f - u ) + *t2 * ( u );

			// set values
			*s1 = ns1;
			*s2 = ns2;
			*t1 = nt1;
			*t2 = nt2;
		}
	}

	return (*w == 0 || *h == 0) ? true : false;
}

// DG: this is used for the "make sure menus are rendered as 4:3" hack
void idDeviceContext::SetMenuScaleFix(bool enable) {
	if(enable) {
		float w = renderSystem->GetScreenWidth();
		float h = renderSystem->GetScreenHeight();
		float aspectRatio = w/h;
		static const float virtualAspectRatio = 16.0f/9.0f;
		if(aspectRatio > (virtualAspectRatio + 0.1f)) {
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
		} else {
			fixScaleForMenu.Set(1, 1);
			fixOffsetForMenu.Set(0, 0);
		}
	} else {
		fixScaleForMenu.Set(1, 1);
		fixOffsetForMenu.Set(0, 0);
	}
}

void idDeviceContext::AdjustCoords(float *x, float *y, float *w, float *h) {

	if (x) {
		*x *= xScale;

		if (x && w && (*x + *w > 0))
		{
			*x *= fixScaleForMenu.x; // DG: for "render menus as 4:3" hack
			*x += fixOffsetForMenu.x;
		}
	}
	if (y) {
		*y *= yScale;

		*y *= fixScaleForMenu.y; // DG: for "render menus as 4:3" hack
		*y += fixOffsetForMenu.y;
	}
	if (w) {
		*w *= xScale;

		*w *= fixScaleForMenu.x; // DG: for "render menus as 4:3" hack
	}
	if (h) {
		*h *= yScale;

		*h *= fixScaleForMenu.y; // DG: for "render menus as 4:3" hack
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

void idDeviceContext::DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *shader) {
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
	if ( ident ) {
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

	renderSystem->DrawStretchPic( &verts[0], &indexes[0], 4, 6, shader, ident );

}

// blendo eric: allow texture coord offset, i.e allow centering
void idDeviceContext::DrawMaterial(float x, float y, float w, float h, const idMaterial *mat, const idVec4 &color, float scalex, float scaley, float offsetx, float offsety) {

	renderSystem->SetColor(color);

	float	s0, s1, t0, t1;
//
//  handle negative scales as well
	if ( scalex < 0 )
	{
		w *= -1;
		scalex *= -1;
	}
	if ( scaley < 0 )
	{
		h *= -1;
		scaley *= -1;
	}
//
	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1 * scalex;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1 * scalex;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1 * scaley;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1 * scaley;
	}

	// blendo eric: offset tex coord, i,e allow centering
	s0 += offsetx;
	s1 += offsetx;
	t0 += offsety;
	t1 += offsety;

	if ( ClippedCoords( &x, &y, &w, &h, &s0, &t0, &s1, &t1 ) ) {
		return;
	}

	AdjustCoords(&x, &y, &w, &h);

	DrawStretchPic( x, y, w, h, s0, t0, s1, t1, mat);
}

void idDeviceContext::DrawMaterialRotated(float x, float y, float w, float h, const idMaterial *mat, const idVec4 &color, float scalex, float scaley, float angle) {

	renderSystem->SetColor(color);

	float	s0, s1, t0, t1;
	//
	//  handle negative scales as well
	if ( scalex < 0 )
	{
		w *= -1;
		scalex *= -1;
	}
	if ( scaley < 0 )
	{
		h *= -1;
		scaley *= -1;
	}
	//
	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1 * scalex;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1 * scalex;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1 * scaley;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1 * scaley;
	}

	if ( angle == 0.0f && ClippedCoords( &x, &y, &w, &h, &s0, &t0, &s1, &t1 ) ) {
		return;
	}

	AdjustCoords(&x, &y, &w, &h);

	DrawStretchPicRotated( x, y, w, h, s0, t0, s1, t1, mat, angle);
}

void idDeviceContext::DrawStretchPicRotated(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *shader, float angle) {

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
	if ( ident ) {
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
	for(int i = 0; i < 4; i++) {
		//Translate to origin
		verts[i].xyz -= origTrans;

		//Rotate
		verts[i].xyz = rotz * verts[i].xyz;

		//Translate back
		verts[i].xyz += origTrans;
	}


	renderSystem->DrawStretchPic( &verts[0], &indexes[0], 4, 6, shader, (angle == 0.0) ? false : true );
}

void idDeviceContext::DrawFilledRect( float x, float y, float w, float h, const idVec4 &color) {

	if ( color.w == 0.0f ) {
		return;
	}

	renderSystem->SetColor(color);

	if (ClippedCoords(&x, &y, &w, &h, NULL, NULL, NULL, NULL)) {
		return;
	}

	AdjustCoords(&x, &y, &w, &h);
	DrawStretchPic( x, y, w, h, 0, 0, 0, 0, whiteImage);
}


void idDeviceContext::DrawRect( float x, float y, float w, float h, float size, const idVec4 &color) {

	if ( color.w == 0.0f ) {
		return;
	}

	renderSystem->SetColor(color);

	if (ClippedCoords(&x, &y, &w, &h, NULL, NULL, NULL, NULL)) {
		return;
	}

	AdjustCoords(&x, &y, &w, &h);
	DrawStretchPic( x, y, size, h, 0, 0, 0, 0, whiteImage );
	DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, whiteImage );
	DrawStretchPic( x, y, w, size, 0, 0, 0, 0, whiteImage );
	DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, whiteImage );
}

void idDeviceContext::DrawMaterialRect( float x, float y, float w, float h, float size, const idMaterial *mat, const idVec4 &color) {

	if ( color.w == 0.0f ) {
		return;
	}

	renderSystem->SetColor(color);
	DrawMaterial( x, y, size, h, mat, color );
	DrawMaterial( x + w - size, y, size, h, mat, color );
	DrawMaterial( x, y, w, size, mat, color );
	DrawMaterial( x, y + h - size, w, size, mat, color );
}


void idDeviceContext::SetCursor(int n) {
	cursor = (n < CURSOR_ARROW || n >= CURSOR_COUNT) ? CURSOR_ARROW : n;
}

//BC draw the mouse cursor.
void idDeviceContext::DrawCursor(float *x, float *y, float size) {
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
	AdjustCursorCoords(x, y, &size, &size);
	float sizeW = size * fixScaleForMenu.x;
	float sizeH = size * fixScaleForMenu.y;
	float fixedX = *x * fixScaleForMenu.x + fixOffsetForMenu.x;
	float fixedY = *y * fixScaleForMenu.y + fixOffsetForMenu.y;

	if (cursor == CURSOR_ARROW)
	{
		DrawStretchPic(fixedX - 1, fixedY - 1, sizeW - 4 /*BC so it's more proportional*/, sizeH, 0, 0, 1, 1, cursorImages[cursor]);
	}
	else
	{
		//For hte finger cursor.
		DrawStretchPic(fixedX - (sizeW /2), fixedY - (sizeH), sizeW - 4, sizeH * 2, 0, 0, 1, 1, cursorImages[cursor]);
	}
}
/*
 =======================================================================================================================
 =======================================================================================================================
 */
// SM: Replaced with version from BFG
void idDeviceContext::PaintChar( float x, float y, const scaledGlyphInfo_t & glyphInfo ) {
	y -= glyphInfo.top;
	x += glyphInfo.left;

	float w = glyphInfo.width;
	float h = glyphInfo.height;
	float s = glyphInfo.s1;
	float t = glyphInfo.t1;
	float s2 = glyphInfo.s2;
	float t2 = glyphInfo.t2;
	const idMaterial * hShader = glyphInfo.material;

	// blendo eric: override code gen font material
	if (activeFontMaterial) {
		hShader = activeFontMaterial;
	}

	if (ClippedCoords( &x, &y, &w, &h, &s, &t, &s2, &t2 )) {
		return;
	}

	AdjustCoords( &x, &y, &w, &h );
	DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

// SM: Removed because of BFG changes
// void idDeviceContext::SetFontByScale(float scale) {
// 	if (scale <= gui_smallFontLimit.GetFloat()) {
// 		useFont = &activeFont->fontInfoSmall;
// 		activeFont->maxHeight = activeFont->maxHeightSmall;
// 		activeFont->maxWidth = activeFont->maxWidthSmall;
// 	} else if (scale <= gui_mediumFontLimit.GetFloat()) {
// 		useFont = &activeFont->fontInfoMedium;
// 		activeFont->maxHeight = activeFont->maxHeightMedium;
// 		activeFont->maxWidth = activeFont->maxWidthMedium;
// 	} else {
// 		useFont = &activeFont->fontInfoLarge;
// 		activeFont->maxHeight = activeFont->maxHeightLarge;
// 		activeFont->maxWidth = activeFont->maxWidthLarge;
// 	}
// }

// SM: Replaced with version from BFG
int idDeviceContext::DrawText( float x, float y, float scale, idVec4 color, const char *text, float adjust, int limit, int style, int cursor ) {
	int			len;
	idVec4		newColor;

	idStr drawText = text;
	int charIndex = 0;

	if (text && color.w != 0.0f) {
		renderSystem->SetColor( color );
		memcpy( &newColor[0], &color[0], sizeof( idVec4 ) );
		len = drawText.Length();
		if (limit > 0 && len > limit) {
			len = limit;
		}

		float prevGlyphSkip = 0.0f;

		while (charIndex < len) {
			uint32 textChar = drawText.UTF8Char( charIndex );

			if ( textChar == C_COLOR_ESCAPE ) {
				uint32 colorIndex = drawText.UTF8Char( charIndex );
				if ( colorIndex == C_COLOR_DEFAULT ) {
					newColor = color;
				}
				else {
					newColor = idStr::ColorForIndex( colorIndex );
					newColor[3] = color[3];
				}
				if (cursor == charIndex - 1 || cursor == charIndex) {
					float backup = 0.0f;
					if (prevGlyphSkip > 0.0f) {
						backup = ( prevGlyphSkip + adjust ) / 5.0f;
					}
					if (cursor == charIndex - 1) {
						backup *= 2.0f;
					}
					else {
						renderSystem->SetColor( newColor );
					}
					DrawEditCursor( x - backup, y, scale );
				}
				renderSystem->SetColor( newColor );
				continue;
			}
			else {
				scaledGlyphInfo_t glyphInfo;
				activeFont->GetScaledGlyph( scale, textChar, glyphInfo );
				prevGlyphSkip = glyphInfo.xSkip;

				PaintChar( x, y, glyphInfo );

				if (cursor == charIndex - 1) {
					DrawEditCursor( x, y, scale );
				}
				x += glyphInfo.xSkip + adjust;
			}
		}
		if (cursor == len) {
			DrawEditCursor( x, y, scale );
		}
	}
	return drawText.Length();
}

void idDeviceContext::SetSize(float width, float height) {
	vidWidth = VIRTUAL_WIDTH;
	vidHeight = VIRTUAL_HEIGHT;
	xScale = yScale = 0.0f;
	if ( width != 0.0f && height != 0.0f ) {
		xScale = vidWidth * ( 1.0f / width );
		yScale = vidHeight * ( 1.0f / height );
	}
}

// SM: Replaced with version from BFG
int idDeviceContext::CharWidth( const char c, float scale ) {
	return idMath::Ftoi( activeFont->GetGlyphWidth( scale, c ) );
}

// SM: Replaced with version from BFG
int idDeviceContext::TextWidth( const char *text, float scale, int limit, float letterSpace ) {
	if (text == NULL) {
		return 0;
	}

	idStr textStr = idStr(text);
	int charIndex = 0;

	float width = 0;
	int count = 0;
	limit = limit > 0 ? limit : textStr.Length();
	while (charIndex < textStr.Length() && count < limit) {
		uint32 textChar = textStr.UTF8Char(charIndex);

		if (textChar == C_COLOR_ESCAPE && charIndex < textStr.Length()) {
			textChar = textStr.UTF8Char( charIndex );
			continue;
		}

		width += activeFont->GetGlyphWidth(scale, textChar) + scale*letterSpace;
		count++;
	}
	return idMath::Ftoi( width );
}

// SM: Replaced with version from BFG
int idDeviceContext::TextHeight(const char *text, float scale, int limit) {
	return idMath::Ftoi( activeFont->GetLineHeight( scale ) );
}

// SM: Replaced with version from BFG
int idDeviceContext::MaxCharWidth(float scale) {
	return idMath::Ftoi( activeFont->GetMaxCharWidth( scale ) );
}

// SM: Replaced with version from BFG
int idDeviceContext::MaxCharHeight(float scale) {
	return idMath::Ftoi( activeFont->GetLineHeight( scale ) );
}

const idMaterial *idDeviceContext::GetScrollBarImage(int index) {
	if (index >= SCROLLBAR_HBACK && index < SCROLLBAR_COUNT) {
		return scrollBarImages[index];
	}
	return scrollBarImages[SCROLLBAR_HBACK];
}

// this only supports left aligned text
idRegion *idDeviceContext::GetTextRegion(const char *text, float textScale, idRectangle rectDraw, float xStart, float yStart) {
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

// SM: Replaced with version from BFG
void idDeviceContext::DrawEditCursor( float x, float y, float scale ) {
	if (( int )( idLib::frameNumber >> 4 ) & 1) {
		return;
	}
	char cursorChar = ( overStrikeMode ) ? '_' : '|';
	scaledGlyphInfo_t glyphInfo;
	activeFont->GetScaledGlyph( scale, cursorChar, glyphInfo );
	PaintChar( x, y, glyphInfo );
}






// SM: Replaced with version from BFG
// blendo eric: adding letter spacing (faux kerning) option, and some cjk punctuation correction
int idDeviceContext::DrawText( const char *text, float textScale, int textAlign, idVec4 color, idRectangle rectDraw, bool wrap, int cursor, bool calcOnly, idList<int> *breaks, int limit, float letterSpace ) {
	int			count = 0;
	int			charIndex = 0;
	int			lastBreak = 0;
	float		y = 0.0f;
	float		textWidth = 0.0f;
	float		textWidthAtLastBreak = 0.0f;

	float letterSpaceWidth = letterSpace * textScale;

	float		charSkip = MaxCharWidth( textScale ) + 1 + letterSpaceWidth;
	float		lineSkip = MaxCharHeight( textScale );

	bool		lineBreak = false;
	bool		wordBreak = false;

	float		prevTextWidth = 0.0f;
	int			prevTextIndex = 0;
	uint32		prevChar = 0;
	bool		prevCharCanBreak = false;

	idStr drawText = text;
	idStr textBuffer;

	if (!calcOnly && !( text && *text )) {
		if (cursor == 0) {
			renderSystem->SetColor( color );
			DrawEditCursor( rectDraw.x, lineSkip + rectDraw.y, textScale );
		}
		return idMath::Ftoi( rectDraw.w / charSkip );
	}

	y = lineSkip + rectDraw.y;

	if (breaks) {
		breaks->Append( 0 );
	}

	while (charIndex < drawText.Length()) {
		uint32 textChar = drawText.UTF8Char( charIndex ); // blendo eric: note UTF8Char() increments the index value

		bool isNewLine = textChar == '\n' || textChar == '\r';

		// See if we need to start a new line.
		if (isNewLine || charIndex == drawText.Length()) {
			lineBreak = true;
			if (charIndex < drawText.Length()) {
				// New line character and we still have more text to read.
				char nextChar = drawText[charIndex + 1];
				if (( textChar == '\n' && nextChar == '\r' ) || ( textChar == '\r' && nextChar == '\n' )) {
					// Just absorb extra newlines.
					textChar = drawText.UTF8Char( charIndex );
				}
			}
		}

		// Check for escape colors if not then simply get the glyph width.
		if (textChar == C_COLOR_ESCAPE && charIndex < drawText.Length()) {
			textBuffer.AppendUTF8Char( textChar );
			textChar = drawText.UTF8Char( charIndex );
			textBuffer.AppendUTF8Char( textChar );
			textChar = drawText.UTF8Char( charIndex );
		}

		bool zeroWidthBreak = false;
#if 0 // this was when it was planned to use zero width white space to break
		// blendo eric: remove whitespace after cjk break chars
		const uint32 ZeroWidthBreakChar = ' ';
		const uint32 ZeroWidthAfterCharRange = 0x00003000;
		zeroWidthBreak = (textChar == ZeroWidthBreakChar) && prevCharCanBreak && (prevChar > ZeroWidthAfterCharRange);
#endif

		bool charCanBreakAfter = UnicodeCharacterCanBreakAfter(textChar);
		bool charCanBreakBefore = lang_enable_breakbefore.GetBool() && UnicodeCharacterCanBreakBefore(textChar) && textBuffer.Length() > 0;

		// If the character isn't a new line then add it to the text buffer.
		if (!isNewLine && !UnicodeCharacterZeroSpace(textChar) && !zeroWidthBreak) {
			textWidth += activeFont->GetGlyphWidth( textScale, textChar ) + letterSpaceWidth;
			textBuffer.AppendUTF8Char( textChar );
		}

		if ((!lineBreak || charIndex == drawText.Length()) && ( textWidth >= rectDraw.w )) {

			// blendo eric: edge case for a word break right on a space
			if (wrap && charCanBreakAfter)
			{
				lastBreak = textBuffer.Length();
				textWidthAtLastBreak = textWidth;
				lineBreak = true;
			}
			else if ( lang_enable_breakbefore.GetBool() && wrap && charCanBreakBefore )
			{ 
				// blendo eric: set break before this (after prev char)
				lastBreak = prevTextIndex;
				textWidthAtLastBreak = prevTextWidth;
				lineBreak = true;
			}
			else
			{
				// The next character will cause us to overflow, if we haven't yet found a suitable
				// break spot, set it to be this character
				if (textBuffer.Length() > 0 && lastBreak == 0) {
					lastBreak = textBuffer.Length();
					textWidthAtLastBreak = textWidth;
				}
				wordBreak = true;
			}
		}
		else if (lineBreak || (wrap && charCanBreakAfter)) {
			// The next character is in view, so if we are a break character, store our position
			lastBreak = textBuffer.Length();
			textWidthAtLastBreak = textWidth;
		}
		else if (lang_enable_breakbefore.GetBool() && wrap && charCanBreakBefore && textBuffer.Length() > 0 ) {
			// set break after prev char / before this
			lastBreak = prevTextIndex;
			textWidthAtLastBreak = prevTextWidth;
		}

		prevChar = textChar;
		prevTextIndex = textBuffer.Length();
		prevCharCanBreak = charCanBreakAfter;
		prevTextWidth = textWidth;

		// We need to go to a new line
		if (lineBreak || wordBreak) {
			float x = rectDraw.x;

			if (textWidthAtLastBreak > 0) {
				textWidth = textWidthAtLastBreak;
			}

			// Align text if needed
			if (textAlign == ALIGN_RIGHT) {
				x = rectDraw.x + rectDraw.w - textWidth;
			}
			else if (textAlign == ALIGN_CENTER) {
				x = rectDraw.x + ( rectDraw.w - textWidth ) / 2;
			}

			if (wrap || lastBreak > 0) {
				// This is a special case to handle breaking in the middle of a word.
				// if we didn't do this, the cursor would appear on the end of this line
				// and the beginning of the next.
				if (wordBreak && cursor >= lastBreak && lastBreak == textBuffer.Length()) {
					cursor++;
				}
			}

			// Draw what's in the current text buffer.
			if (!calcOnly) {
				if (lastBreak > 0) {
					count += DrawText( x, y, textScale, color, textBuffer.Left( lastBreak ).c_str(), letterSpaceWidth, 0, 0, cursor );
					textBuffer = textBuffer.Right( textBuffer.Length() - lastBreak );
				}
				else {
					count += DrawText( x, y, textScale, color, textBuffer.c_str(), letterSpaceWidth, 0, 0, cursor );
					textBuffer.Clear();
				}
			}
			else
			{
				if ( lastBreak > 0 ) {
					textBuffer = textBuffer.Right( textBuffer.Length() - lastBreak );
				}
				else {
					textBuffer.Clear();
				}
			}

			if (cursor < lastBreak) {
				cursor = -1;
			}
			else if (cursor >= 0) {
				cursor -= ( lastBreak + 1 );
			}

			// If wrap is disabled return at this point.
			if (!wrap) {
				return lastBreak;
			}

			// If we've hit the allowed character limit then break.
			if (limit && count > limit) {
				break;
			}

			y += lineSkip + 5;

			if (!calcOnly && y > rectDraw.Bottom()) {
				break;
			}

			// SM: When calculating the breaks we need to roll back characters or the last line
			// can get cut off
			if (calcOnly && breaks && textBuffer.Length() > 0)
			{
				charIndex -= textBuffer.Length();
				textBuffer.Clear();
			}

			// If breaks were requested then make a note of this one.
			if (breaks) {
				breaks->Append( drawText.Length() - charIndex );
			}

			// blendo eric: needed to add this to solve last char in text clipping when on border
			if (!calcOnly && !textBuffer.IsEmpty() && charIndex == drawText.Length())
			{
				DrawText(x, y, textScale, color, textBuffer.c_str(), letterSpaceWidth, 0, 0, cursor);
				break;
			}

			// Reset necessary parms for next line.
			lastBreak = 0;
			textWidth = 0;
			textWidthAtLastBreak = 0;
			lineBreak = false;
			wordBreak = false;

			// Reassess the remaining width
			for (int i = 0; i < textBuffer.Length();) {
				if (textChar != C_COLOR_ESCAPE) {
					textWidth += activeFont->GetGlyphWidth( textScale, textBuffer.UTF8Char(i) ) + letterSpaceWidth;
				}
			}

			continue;
		}
	}

	return idMath::Ftoi( rectDraw.w / charSkip );
}

/*
=============
idRectangle::String
=============
*/
char *idRectangle::String( void ) const {
	static	int		index = 0;
	static	char	str[ 8 ][ 48 ];
	char	*s;

	// use an array so that multiple toString's won't collide
	s = str[ index ];
	index = (index + 1)&7;

	sprintf( s, "%.2f %.2f %.2f %.2f", x, y, w, h );

	return s;
}
