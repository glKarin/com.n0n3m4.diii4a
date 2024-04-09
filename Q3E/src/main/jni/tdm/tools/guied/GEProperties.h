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

#ifndef GEPROPERTIES_H_
#define GEPROPERTIES_H_

#ifndef PROPERTYGRID_H_
#include "../common/PropertyGrid.h"
#endif

class rvGEWorkspace;
class rvGEWindowWrapper;

class rvGEProperties
{
public:

	rvGEProperties ( );
	
	bool	Create				( HWND parent, bool visible );
	void	Show				( bool visibile );

	void	SetWorkspace		( rvGEWorkspace* workspace );

	void	Update				( void );	

	HWND	GetWindow			( void );
	
protected:

	bool	AddModifier			( const char* name, const char* value );

	static LRESULT CALLBACK WndProc ( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

	HWND				mWnd;
	rvPropertyGrid		mGrid;
	rvGEWindowWrapper*	mWrapper;
	rvGEWorkspace*		mWorkspace;	
};

ID_INLINE HWND rvGEProperties::GetWindow ( void )
{
	return mWnd;
}

ID_INLINE void rvGEProperties::SetWorkspace ( rvGEWorkspace* workspace )
{
	mWorkspace = workspace;
	Update ( );
}

#endif // GEPROPERTIES_H_