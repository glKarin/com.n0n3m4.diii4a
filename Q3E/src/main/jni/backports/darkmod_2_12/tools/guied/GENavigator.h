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
#ifndef GENAVIGATOR_H_
#define GENAVIGATOR_H_

class rvGEWorkspace;
class idWindow;

class rvGENavigator
{
public:

	rvGENavigator ( );
	
	bool	Create				( HWND parent, bool visible );
	void	Show				( bool visibile );

	void	Refresh				( void );

	void	SetWorkspace		( rvGEWorkspace* workspace );
	
	void	Update				( void );	
	void	UpdateSelections	( void );
	
	HWND	GetWindow			( void );

protected:

	void	AddWindow			( idWindow* window );

	static LRESULT CALLBACK WndProc ( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT CALLBACK ListWndProc ( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT FAR PASCAL GetMsgProc ( int nCode, WPARAM wParam, LPARAM lParam );

	HWND			mWnd;
	HWND			mTree;
	HICON			mVisibleIcon;
	HICON			mVisibleIconDisabled;
	HICON			mScriptsIcon;
	HICON			mScriptsLightIcon;
	HICON			mCollapseIcon;
	HICON			mExpandIcon;
	rvGEWorkspace*	mWorkspace;
	WNDPROC			mListWndProc;
};

ID_INLINE HWND rvGENavigator::GetWindow ( void )
{
	return mWnd;
}

#endif // GENAVIGATOR_H_