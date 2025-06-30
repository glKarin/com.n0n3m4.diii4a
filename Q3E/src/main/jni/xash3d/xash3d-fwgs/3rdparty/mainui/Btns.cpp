/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Utils.h"
#include "BtnsBMPTable.h"
#include <string.h>

#define ART_BUTTONS_MAIN		"gfx/shell/btns_main.bmp"	// we support bmp only

static int UI_GetBlockHeight( int button_height, int points[3], int &empty_height )
{
	empty_height = 1; // don't add pixels in-between
	int height = ( button_height + empty_height ) * 2 + button_height;

	points[0] = 0;
	points[1] = button_height + empty_height;
	points[2] = ( button_height + empty_height ) * 2; // 102:

	return height;
}

/*
=================
UI_LoadBmpButtons
=================
*/
void UI_LoadBmpButtons()
{
	memset( uiStatic.buttonsPics, 0, sizeof( uiStatic.buttonsPics ));

	if( uiStatic.lowmemory )
		return;

	CBMP *bmp = CBMP::LoadFile( ART_BUTTONS_MAIN );

	if( bmp == nullptr )
		return;

	// get default block size
	uiStatic.buttons_width = bmp->GetBitmapHdr()->width; // pass to the other parts of ui
	uiStatic.buttons_height = 26; // hardcoded!

	// calculate original bmp size
	const int stride = (( bmp->GetBitmapHdr()->width * bmp->GetBitmapHdr()->bitsPerPixel / 8 ) + 3 ) & ~3;
	const size_t btn_sz = stride * uiStatic.buttons_height; // one button size

	// get our cutted bmp sizes
	int empty_height;
	const int blockHeight = UI_GetBlockHeight( uiStatic.buttons_height, uiStatic.buttons_points, empty_height );
	const int empty_sz = empty_height * stride;

	// init CBMP and copy data
	CBMP cutted_bmp( bmp->GetBitmapHdr(), stride * blockHeight );
	cutted_bmp.GetBitmapHdr()->height = blockHeight;

	byte *src = bmp->GetTextureData();
	byte *dst = cutted_bmp.GetTextureData();

	int pic_count = bmp->GetBitmapHdr()->height / uiStatic.buttons_height / 3;
	int src_pos = bmp->GetBitmapHdr()->height * stride;

	for( int i = 0; i < pic_count; i++ )
	{
		int dst_pos = blockHeight * stride;
		char fname[256];

		V_snprintf( fname, sizeof( fname ), "#btns_%d.bmp", i );

		for( int btn = 0; btn < 3; btn++ )
		{
			src_pos -= btn_sz;
			dst_pos -= btn_sz;
			memcpy( &dst[dst_pos], &src[src_pos], btn_sz );

			// fixup alpha for half-life infected mod
			if( bmp->GetBitmapHdr()->bitsPerPixel == 32 )
			{
				for( int j = dst_pos; j < dst_pos + btn_sz; j += 4 )
					dst[j+3] = 255;
			}

			// fix misaligned gearbox btns
			memset( &dst[dst_pos], 0, stride );

			// fill the empty space
			if( btn < 2 && empty_sz != 0 )
			{
				dst_pos -= empty_sz;
				memset( &dst[dst_pos], 0, empty_sz );
			}
		}

		// upload image into video memory
		uiStatic.buttonsPics[i] = EngFuncs::PIC_Load( fname, cutted_bmp.GetBitmap(), cutted_bmp.GetBitmapHdr()->fileSize );
	}

	delete bmp;
}
