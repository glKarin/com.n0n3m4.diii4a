/*
MessageBox.cpp -- simple messagebox with text
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
#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Utils.h"
#include "Action.h"
#include "ItemsHolder.h"
#include "MessageBox.h"

CMenuMessageBox::CMenuMessageBox(const char *name) : BaseClass( name )
{
	iFlags |= QMF_INACTIVE;
}

void CMenuMessageBox::_Init()
{
	background.bForceColor = true;
	background.colorBase = uiPromptBgColor;

	dlgMessage.eTextAlignment = QM_CENTER; // center
	dlgMessage.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	dlgMessage.SetCoord( 0, 0 );
	dlgMessage.size = size;

	AddItem( background );
	AddItem( dlgMessage );
}

void CMenuMessageBox::SetMessage( const char *sz )
{
	dlgMessage.szName = sz;
}
