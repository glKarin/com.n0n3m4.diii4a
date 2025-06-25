/*
Zoo.cpp -- examples
Copyright (C) 2017 a1batross

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
#include "Framework.h"
#include "Action.h"
#include "Field.h"
#include "ScrollView.h"

class CMenuZoo : public CMenuFramework
{
public:
	CMenuZoo() : CMenuFramework( "Example" ) { }

	virtual void _Init();
	virtual void _VidInit();

	static void OKButtonCommand( CMenuBaseItem *pSelf, void *pExtra )
	{
		pSelf->Parent()->Hide();
	}

	static void CancelButtonCommand( CMenuBaseItem *pSelf, void *pExtra )
	{
		pSelf->Parent()->Hide();
	}

private:
	CMenuAction OKButton;
	CMenuAction CancelButton;
	CMenuAction Label1;
	CMenuAction Label2;
	CMenuAction Label3;
	CMenuAction Label4;
	CMenuField Entry1;
	CMenuField Entry2;
	CMenuField Entry3;
	CMenuField Entry4;
	CMenuField Entry5;

	CMenuAction bmp[35];
	CMenuScrollView scrollView;
};

#define SET_TAG_BY_MEMBER_NAME( member ) \
	member.szTag = STR( member ); \
	AddItem( member );

void CMenuZoo::_Init()
{
	/*
	szTag = "CDKeyEntryDialog";
	iFlags = QMF_DIALOG;

	RegisterNamedEvent( OKButtonCommand, "Ok" );
	RegisterNamedEvent( CancelButtonCommand, "Cancel" );

	background.bForceColor = true;

	SET_TAG_BY_MEMBER_NAME( OKButton );
	SET_TAG_BY_MEMBER_NAME( CancelButton );
	SET_TAG_BY_MEMBER_NAME( Label1 );
	SET_TAG_BY_MEMBER_NAME( Label2 );
	SET_TAG_BY_MEMBER_NAME( Label3 );
	SET_TAG_BY_MEMBER_NAME( Label4 );
	SET_TAG_BY_MEMBER_NAME( Entry1 );
	SET_TAG_BY_MEMBER_NAME( Entry2 );
	SET_TAG_BY_MEMBER_NAME( Entry3 );
	//SET_TAG_BY_MEMBER_NAME( Entry4 );
	//SET_TAG_BY_MEMBER_NAME( Entry5 );
	*/

	const char *pics[] =
	{
"gfx/shell/head_advanced.tga",
"gfx/shell/head_advoptions.tga",
"gfx/shell/head_audio.tga",
"gfx/shell/head_config.tga",
"gfx/shell/head_controls.tga",
"gfx/shell/head_creategame.tga",
"gfx/shell/head_createroom.tga",
"gfx/shell/head_customize.tga",
"gfx/shell/head_custom.tga",
"gfx/shell/head_filter.tga",
"gfx/shell/head_gameopts.tga",
"gfx/shell/head_gamepad.tga",
"gfx/shell/head_gore.tga",
"gfx/shell/head_inetgames.tga",
"gfx/shell/head_keyboard.tga",
"gfx/shell/head_lan.tga",
"gfx/shell/head_load.tga",
"gfx/shell/head_multi.tga",
"gfx/shell/head_newgame.tga",
"gfx/shell/head_playrec.tga",
"gfx/shell/head_play.tga",
"gfx/shell/head_readme.tga",
"gfx/shell/head_record.tga",
"gfx/shell/head_rooms.tga",
"gfx/shell/head_room.tga",
"gfx/shell/head_saveload.tga",
"gfx/shell/head_save.tga",
"gfx/shell/head_single.tga",
"gfx/shell/head_specgames.tga",
"gfx/shell/head_touch_buttons.tga",
"gfx/shell/head_touch_options.tga",
"gfx/shell/head_touch.tga",
"gfx/shell/head_video.tga",
"gfx/shell/head_vidmodes.tga",
"gfx/shell/head_vidoptions.tga"
	};

	for( int i = 0; i < 35; i++ )
	{
		HIMAGE pic = EngFuncs::PIC_Load( pics[i] );

		bmp[i].SetBackground( pics[i] );
		bmp[i].iFlags |= QMF_DISABLESCAILING;
		bmp[i].szName = pics[i];
		bmp[i].eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
		bmp[i].colorFocus = PackRGBA( 255, 255, 255, 255 );
		bmp[i].colorBase = PackRGBA( 128, 128, 128, 255 );
		if( i == 0 )
			bmp[i].SetCoord( 0, 0 );
		else
			bmp[i].SetCoord( bmp[i-1].pos.x, bmp[i-1].pos.y + EngFuncs::PIC_Height( pic ) );
		scrollView.AddItem( bmp[i] );
	}

	scrollView.SetRect( 0, 0, 1024, 768 );

	AddItem( scrollView );
}

void CMenuZoo::_VidInit()
{

}

ADD_MENU( menu_zoo, CMenuZoo, UI_Zoo_Menu )
