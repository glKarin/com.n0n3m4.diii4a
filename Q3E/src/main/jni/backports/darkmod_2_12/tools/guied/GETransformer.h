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

#ifndef GETRANSFORMER_H_
#define GETRANSFORMER_H_

class rvGETransformer
{
public:

	rvGETransformer ( );

	bool		Create			( HWND parent, bool visible );		
	void		Show			( bool show );
	
	void		SetWorkspace	( rvGEWorkspace* workspace );
	void		Update			( void );
	
	HWND		GetWindow		( void );
	
protected:

	HWND			mWnd;
	HWND			mDlg;
	rvGEWorkspace*	mWorkspace;	
	idWindow*		mRelative;
	
private:

	static LRESULT CALLBACK		WndProc		( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static INT_PTR CALLBACK		DlgProc		( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT FAR PASCAL	GetMsgProc	( int nCode, WPARAM wParam, LPARAM lParam );
};

ID_INLINE HWND rvGETransformer::GetWindow ( void )
{
	return mWnd;
}

#endif // GETRANSFORMER_H_