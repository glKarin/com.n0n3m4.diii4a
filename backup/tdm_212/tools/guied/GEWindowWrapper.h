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

#ifndef GEWINDOWWRAPPER_H_
#define GEWINDOWWRAPPER_H_

class idWindow;
class rvGEWindowWrapper;

typedef bool (*PFNENUMCHILDRENPROC) ( rvGEWindowWrapper* wrapper, void* data );

class rvGEWindowWrapper
{
public:

	enum EWindowType
	{
		WT_UNKNOWN,
		WT_NORMAL,
		WT_EDIT,
		WT_HTML,
		WT_CHOICE,
		WT_SLIDER,
		WT_BIND,
		WT_LIST,
		WT_RENDER,
		WT_TRANSITION
	};

	rvGEWindowWrapper ( idWindow* window, EWindowType type );

	static rvGEWindowWrapper*	GetWrapper ( idWindow* window );	
		
	idWindow*			GetWindow			( void );
	idDict&				GetStateDict		( void );
	idDict&				GetVariableDict		( void );
	idDict&				GetScriptDict		( void );
	idRectangle&		GetClientRect		( void );
	idRectangle&		GetScreenRect		( void );
	EWindowType			GetWindowType		( void );
	int					GetChildCount		( void );
	int					GetDepth			( void );
	idWindow*			GetChild			( int index );
	
	void				SetRect				( idRectangle& rect );
	void				SetState			( const idDict& dict );
	void				SetStateKey			( const char* key, const char* value, bool update = true );
	void				DeleteStateKey		( const char* key );
	bool				VerfiyStateKey		( const char* name, const char* value, idStr* result = NULL );
	
	bool				IsFlippedHorz		( void );
	bool				IsFlippedVert		( void );
	bool				IsHidden			( void );
	bool				IsDeleted			( void );
	bool				IsSelected			( void );
	bool				IsExpanded			( void );

	bool				CanHaveChildren		( void );
	bool				CanMoveAndSize		( void );
	
	void				SetFlippedHorz		( bool f );
	void				SetFlippedVert		( bool f );
	void				SetHidden			( bool v );
	void				SetDeleted			( bool del );
	void				SetSelected			( bool sel );
	void				SetWindowType		( EWindowType type );

	bool				Expand				( void );
	bool				Collapse			( void );

	bool				EnumChildren		( PFNENUMCHILDRENPROC proc, void* data );

	idWindow*			WindowFromPoint		( float x, float y, bool visibleOnly = true );
	
	void				Finish				( void );
	
	static EWindowType	StringToWindowType		( const char* string );
	static const char*	WindowTypeToString		( EWindowType type );
	
protected:

	void				CalcScreenRect		( void );
	void				UpdateRect			( void );
	void				UpdateWindowState 	( void );	

	idRectangle		mClientRect;
	idRectangle		mScreenRect;
	idWindow*		mWindow;
	idDict			mState;
	idDict			mVariables;
	idDict			mScripts;
	bool			mFlippedHorz;
	bool			mFlippedVert;
	bool			mHidden;
	bool			mDeleted;
	bool			mSelected;
	bool			mExpanded;
	bool			mOldVisible;
	bool			mMoveable;
	EWindowType		mType;
};

ID_INLINE idDict& rvGEWindowWrapper::GetStateDict ( void )
{
	return mState;
}

ID_INLINE idDict& rvGEWindowWrapper::GetVariableDict ( void )
{
	return mVariables;
}

ID_INLINE idDict& rvGEWindowWrapper::GetScriptDict ( void )
{
	return mScripts;
}

ID_INLINE bool rvGEWindowWrapper::IsFlippedHorz ( void )
{
	return mFlippedHorz;
}

ID_INLINE bool rvGEWindowWrapper::IsFlippedVert ( void ) 
{
	return mFlippedVert;
}

ID_INLINE bool rvGEWindowWrapper::IsExpanded ( void ) 
{
	return mExpanded;
}
	
ID_INLINE void rvGEWindowWrapper::SetFlippedHorz ( bool f )
{
	mFlippedHorz = f;
}

ID_INLINE void rvGEWindowWrapper::SetFlippedVert ( bool f )
{
	mFlippedVert = f;
}

ID_INLINE idRectangle& rvGEWindowWrapper::GetClientRect ( void )
{
	return mClientRect;
}

ID_INLINE idRectangle& rvGEWindowWrapper::GetScreenRect ( void )
{
	return mScreenRect;
}

ID_INLINE bool rvGEWindowWrapper::IsHidden ( void )
{
	return mHidden;
}

ID_INLINE bool rvGEWindowWrapper::IsDeleted ( void )
{
	return mDeleted;
}

ID_INLINE bool rvGEWindowWrapper::IsSelected ( void )
{
	return mSelected;
}

ID_INLINE void rvGEWindowWrapper::SetSelected ( bool sel )
{
	mSelected = sel;
}

ID_INLINE rvGEWindowWrapper::EWindowType rvGEWindowWrapper::GetWindowType ( void )
{
	return mType;
}

ID_INLINE void rvGEWindowWrapper::SetWindowType ( rvGEWindowWrapper::EWindowType type )
{
	mType = type;
}

ID_INLINE idWindow* rvGEWindowWrapper::GetChild ( int index )
{
	return mWindow->GetChild ( index );
}

ID_INLINE idWindow* rvGEWindowWrapper::GetWindow ( void )
{
	return mWindow;
}

ID_INLINE bool rvGEWindowWrapper::CanMoveAndSize ( void )
{
	return mMoveable;
}

#endif // GEWINDOWWRAPPER_H_
