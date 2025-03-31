/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef GEPROPERTYPAGE_H_
#define GEPROPERTYPAGE_H_

class rvGEPropertyPage
{
public:
	
	rvGEPropertyPage ( );

	virtual bool	Init			( void ) { return true; }
	virtual bool	Apply			( void ) { return true; }
	virtual bool	SetActive		( void ) { return true; }
	virtual bool	KillActive		( void ) { return true; }
	virtual int		HandleMessage	( UINT msg, WPARAM wParam, LPARAM lParam );

	static INT_PTR CALLBACK WndProc ( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	
protected:

	HWND		mPage;	
};

#endif // GEPROPERTYPAGE_H_