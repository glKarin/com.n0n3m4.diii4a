/*
CheckBox.h - checkbox
Copyright (C) 2010 Uncle Mike
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

#ifndef MENU_CHECKBOX_H
#define MENU_CHECKBOX_H

#include "Editable.h"

#define UI_CHECKBOX_EMPTY		"gfx/shell/cb_empty"
#define UI_CHECKBOX_GRAYED		"gfx/shell/cb_disabled"
#define UI_CHECKBOX_FOCUS		"gfx/shell/cb_over"
#define UI_CHECKBOX_PRESSED		"gfx/shell/cb_down"
#define UI_CHECKBOX_ENABLED		"gfx/shell/cb_checked"

class CMenuCheckBox : public CMenuEditable
{
public:
	typedef CMenuEditable BaseClass;

	CMenuCheckBox();
	void VidInit() override;
	bool KeyUp( int key ) override;
	bool KeyDown( int key ) override;
	void Draw( void ) override;
	void UpdateEditable() override;
	void LinkCvar( const char *name ) override
	{
		CMenuEditable::LinkCvar( name, CMenuEditable::CVAR_VALUE );
	}

	void SetPicture( const char *empty, const char *focus, const char *press, const char *check, const char *grayed )
	{
		szEmptyPic = empty;
		szFocusPic = focus;
		szPressPic = press;
		szCheckPic = check;
		szGrayedPic = grayed;
	}

	bool bChecked;
	bool bInvertMask;
	bool bChangeOnPressed;

	CImage szEmptyPic;
	CImage szFocusPic;
	CImage szPressPic;
	CImage szCheckPic;
	CImage szGrayedPic;	// when QMF_GRAYED is set

	unsigned int iMask; // used only for BitMaskCb
	static void BitMaskCb( CMenuBaseItem *pSelf, void *pExtra )
	{
		CMenuCheckBox *self = (CMenuCheckBox*)pSelf;

		if( !self->bInvertMask == self->bChecked )
		{
			*(unsigned int*)pExtra |= self->iMask;
		}
		else
		{
			*(unsigned int*)pExtra &= ~self->iMask;
		}
	}

	CColor colorText;
private:
	Point m_scTextPos;
	Size m_scTextSize;
};

#endif // MENU_CHECKBOX_H
