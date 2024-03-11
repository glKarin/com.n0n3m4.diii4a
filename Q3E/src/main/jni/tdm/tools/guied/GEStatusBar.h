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
#ifndef GESTATUSBAR_H_
#define GESTATUSBAR_H_

class rvGEStatusBar
{
public:

	rvGEStatusBar ( );

	bool	Create			( HWND parent, UINT id, bool visible = true );
	void	Resize			( int width, int height );	
	
	HWND	GetWindow		( void );

	void	SetZoom			( int zoom );
	void	SetTriangles	( int tris );
	void	SetSimple		( bool simple );
	
	void	Show			( bool state );
	void	Update			( void );
		
protected:

	HWND	mWnd;
	bool	mSimple;
	int		mZoom;
	int		mTriangles;
};

ID_INLINE HWND rvGEStatusBar::GetWindow ( void )
{
	return mWnd;
}

ID_INLINE void rvGEStatusBar::SetZoom ( int zoom )
{
	if ( mZoom != zoom )
	{
		mZoom = zoom;
		Update ( );
	}
}

ID_INLINE void rvGEStatusBar::SetTriangles ( int triangles )
{
	if ( triangles != mTriangles )
	{
		mTriangles = triangles;
		Update ( );
	}
}

ID_INLINE void rvGEStatusBar::SetSimple ( bool simple )
{
	if ( mSimple != simple )
	{
		mSimple = simple;
		Update ( );
	}
}

#endif // GESTATUSBAR_H_