/*
BaseWindow.h -- base client menu window
Copyright (C) 2018 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#ifndef BASECLIENTWINDOW_H
#define BASECLIENTWINDOW_H

#include "BaseWindow.h"

class CMenuBaseClientWindow : public CMenuBaseWindow
{
public:
	typedef CMenuBaseWindow BaseClass;
	CMenuBaseClientWindow( const char *name = "BaseClientWindow" );

	bool KeyDown( int key ) override;
};

#endif // BASECLIENTWINDOW_H
