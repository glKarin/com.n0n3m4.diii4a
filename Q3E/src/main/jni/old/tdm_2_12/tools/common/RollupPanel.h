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

#ifndef ROLLUPPANEL_H_
#define ROLLUPPANEL_H_

#define RPITEM_MAX_NAME	64

struct RPITEM
{
	HWND		mDialog;
	HWND		mButton;
	HWND		mGroupBox;
	bool		mExpanded;
	bool		mEnable;
	bool		mAutoDestroy;
	WNDPROC		mOldDlgProc;
	WNDPROC		mOldButtonProc;
	char		mCaption[RPITEM_MAX_NAME];
};

class rvRollupPanel
{
public:

	rvRollupPanel ( void );
	virtual ~rvRollupPanel ( void );

	bool	Create			( DWORD dwStyle, const RECT& rect, HWND parent, unsigned int id );

	int		InsertItem		( const char* caption, HWND dialog, bool autoDestroy, int index = -1);

	void	RemoveItem		( int index );
	void	RemoveAllItems	( void );

	void	ExpandItem		( int index, bool expand = true );
	void	ExpandAllItems	( bool expand = true );

	void	EnableItem		( int index, bool enabled = true );
	void	EnableAllItems	( bool enable = true );

	int		GetItemCount	( void );

	RPITEM*	GetItem			( int index );

	int		GetItemIndex	( const char* caption );
	int		GetItemIndex	( HWND hwnd );

	void	ScrollToItem	( int index, bool top = true );
	int		MoveItemAt		( int index, int newIndex );
	bool	IsItemExpanded	( int index );
	bool	IsItemEnabled	( int index );
	
	HWND	GetWindow		( void );
	
	void	AutoSize		( void );

protected:

	void	RecallLayout	( void );	
	void	_RemoveItem		( int index );
	void	_ExpandItem		( RPITEM* item, bool expand );
	void	_EnableItem		( RPITEM* item, bool enable );

	int		HandleCommand		( WPARAM wParam, LPARAM lParam );
	int		HandlePaint			( WPARAM wParam, LPARAM lParam );
	int		HandleSize			( WPARAM wParam, LPARAM lParam );
	int		HandleLButtonDown	( WPARAM wParam, LPARAM lParam );
	int		HandleLButtonUp		( WPARAM wParam, LPARAM lParam );
	int		HandleMouseMove		( WPARAM wParam, LPARAM lParam );
	int		HandleMouseWheel	( WPARAM wParam, LPARAM lParam );
	int		HandleMouseActivate	( WPARAM wParam, LPARAM lParam );	
	int		HandleContextMenu	( WPARAM wParam, LPARAM lParam );	

	// Datas
	idList<RPITEM*>	mItems;
	int				mStartYPos;
	int				mItemHeight;
	int				mOldMouseYPos;
	int				mSBOffset;
	HWND			mWindow;

	// Window proc
	static LRESULT CALLBACK		WindowProc	( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static LRESULT CALLBACK		DialogProc	( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static LRESULT CALLBACK		ButtonProc	( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	
	static LRESULT FAR PASCAL	GetMsgProc	( int nCode, WPARAM wParam, LPARAM lParam );
	static idList<HWND>	mDialogs;	
	static HHOOK		mDialogHook;
};

ID_INLINE int rvRollupPanel::GetItemCount ( void )
{
	return mItems.Num(); 
}

ID_INLINE bool rvRollupPanel::IsItemExpanded ( int index )
{
	if ( index >= mItems.Num() || index < 0 )
	{
		return false;
	}
	return mItems[index]->mExpanded;
}

ID_INLINE bool rvRollupPanel::IsItemEnabled( int index )
{
	if ( index >= mItems.Num() || index < 0 )
	{
		return false;
	}
	return mItems[index]->mEnable;
}

ID_INLINE HWND rvRollupPanel::GetWindow ( void )
{
	return mWindow;
}

#endif // ROLLUPPANEL_H_
