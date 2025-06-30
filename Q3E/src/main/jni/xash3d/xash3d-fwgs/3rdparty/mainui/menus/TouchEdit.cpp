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

/*
 * This is empty menu that allows engine to draw touch editor
 */

#include "Framework.h"
#include "keydefs.h"

class CMenuTouchEdit : public CMenuFramework
{
public:
	CMenuTouchEdit() : CMenuFramework( "CMenuTouchEdit" ) { }

	void Show() override;
	void Hide() override;
	void Draw() override;
	bool DrawAnimation() override;
	bool KeyDown( int key ) override;
private:
	float saveTouchEnable;
};

void CMenuTouchEdit::Show()
{
	saveTouchEnable = EngFuncs::GetCvarFloat( "touch_enable" );

	EngFuncs::CvarSetValue( "touch_enable", 1 );
	EngFuncs::CvarSetValue( "touch_in_menu", 1 );
	EngFuncs::ClientCmd(FALSE, "touch_enableedit");

	CMenuFramework::Show();
}

void CMenuTouchEdit::Hide()
{
	EngFuncs::CvarSetValue( "touch_enable", saveTouchEnable );
	EngFuncs::CvarSetValue( "touch_in_menu", 0 );
	EngFuncs::ClientCmd(FALSE, "touch_disableedit");

	CMenuFramework::Hide();
}

bool CMenuTouchEdit::DrawAnimation()
{
	return true;
}

/*
=================
UI_TouchEdit_DrawFunc
=================
*/
void CMenuTouchEdit::Draw( void )
{
	if( !EngFuncs::GetCvarFloat("touch_in_menu") )
	{
		Hide();
		UI_TouchButtons_GetButtonList();
	}
}

/*
=================
UI_TouchEdit_KeyFunc
=================
*/
bool CMenuTouchEdit::KeyDown( int key )
{
	if( UI::Key::IsEscape( key ) )
	{
		Hide();
		PlayLocalSound( uiStatic.sounds[SND_OUT] );
		return true;
	}
	return false;
}

ADD_MENU( menu_touchedit, CMenuTouchEdit, UI_TouchEdit_Menu );
