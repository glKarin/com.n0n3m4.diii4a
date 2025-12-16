/*
MessageBox.h -- simple messagebox with text
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
#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include "BaseWindow.h"
#include "Action.h"

class CMenuMessageBox : public CMenuBaseWindow
{
public:
	typedef CMenuBaseWindow BaseClass;

	CMenuMessageBox( const char *name = "Unnamed MessageBox" );

	void SetMessage( const char *sz );
private:
	void _Init() override;

	CMenuBackgroundBitmap background;
	CMenuAction dlgMessage;
};

#endif // MESSAGEBOX_H
