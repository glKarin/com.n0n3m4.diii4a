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
#ifndef OPENFILEDIALOG_H_
#define OPENFILEDIALOG_H_

#define OFD_MUSTEXIST	0x00000001

class rvOpenFileDialog
{
public:

	rvOpenFileDialog ( void );
	~rvOpenFileDialog ( void );

	bool			DoModal		( HWND parent );
	const char*		GetFilename	( void );

	void			SetFilter		( const char* filter );
	void			SetTitle		( const char* title );
	void			SetOKTitle		( const char* title );
	void			SetInitialPath	( const char* path );
	void			SetFlags		( int flags );

	const char*		GetInitialPath  ( void );

protected:

	void			UpdateFileList	( void );
	void			UpdateLookIn	( void );

	HWND			mWnd;
	HWND			mWndFileList;
	HWND			mWndLookin;
	
	HINSTANCE		mInstance;
	
	HIMAGELIST		mImageList;
	HBITMAP			mBackBitmap;
	
	static char		mLookin[ MAX_OSPATH ];
	idStr			mFilename;
	idStr			mTitle;
	idStr			mOKTitle;
	idStrList		mFilters;
	
	int				mFlags;

private:
	
	void	HandleCommandOK			( void );
	void	HandleLookInChange		( void );
	void	HandleInitDialog		( void );

	static INT_PTR CALLBACK DlgProc ( HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam );
};

ID_INLINE const char* rvOpenFileDialog::GetFilename ( void )
{
	return mFilename.c_str ( );
}

ID_INLINE void rvOpenFileDialog::SetTitle ( const char* title )
{
	mTitle = title;
}

ID_INLINE void rvOpenFileDialog::SetOKTitle ( const char* title )
{
	mOKTitle = title;
}

ID_INLINE void rvOpenFileDialog::SetInitialPath ( const char* path )
{
    if (!idStr::Cmpn(mLookin, path, static_cast<int>(strlen(path))))
	{
		return;
	}

	idStr::Copynz( mLookin, path, sizeof( mLookin ) );
}

ID_INLINE void rvOpenFileDialog::SetFlags ( int flags )
{
	mFlags = flags;
}

ID_INLINE const char* rvOpenFileDialog::GetInitialPath ( void )
{
	return mLookin;
}

#endif // OPENFILEDIALOG_H_
