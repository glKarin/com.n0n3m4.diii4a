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

// SM: Removed all the freetype stuff and replaced register font from BFG

#include "sys/platform.h"

#include "renderer/tr_local.h"
#include "renderer/Font.h"

/*
============
RegisterFont

Loads 3 point sizes, 12, 24, and 48
============
*/
idFont * idRenderSystemLocal::RegisterFont( const char *fontName ) {
	// SM: Replaced completely with BFG version
	idStr baseFontName = fontName;
	baseFontName.Replace( "fonts/", "" );
	if (baseFontName.Icmp("fonts") == 0) {
		baseFontName = idFont::DEFAULT_FONT;
	}
	for (int i = 0; i < fonts.Num(); i++) {
		if (idStr::Icmp( fonts[i]->GetName(), baseFontName ) == 0) {
			fonts[i]->Touch();
			return fonts[i];
		}
	}
	idFont * newFont = new idFont( baseFontName );
	fonts.Append( newFont );
	return newFont;
}

// SM: ResetFonts is copied over from BFG
/*
========================
idRenderSystemLocal::ResetFonts
========================
*/
void idRenderSystemLocal::ResetFonts() {
	fonts.DeleteContents( true );
}

/*
============
R_InitFreeType
============
*/
void R_InitFreeType( void ) {
//	registeredFontCount = 0;
}

/*
============
R_DoneFreeType
============
*/
void R_DoneFreeType( void ) {
//	registeredFontCount = 0;
}
