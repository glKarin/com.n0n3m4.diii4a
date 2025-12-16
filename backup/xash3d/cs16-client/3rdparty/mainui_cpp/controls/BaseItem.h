/*
BaseItem.h -- base menu item
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
#ifndef BASEITEM_H
#define BASEITEM_H

#include "BaseMenu.h"
#include "Color.h"
#include "cursor_type.h"

class CMenuItemsHolder;
class CMenuBaseItem
{
public:
	friend class CMenuItemsHolder;

	// The main constructor
	CMenuBaseItem();
	virtual ~CMenuBaseItem();

	// Init is called when Item is added to Framework
	// Called once by Framework
	virtual void Init( void );

	// VidInit is called after VidInit method of Framework
	// VidInit can be called multiple times
	virtual void VidInit( void );

	// Reload is called after VidInit item method
	// should be used for reloading internal data, like cvar values
	virtual void Reload( void );

	// Key is called every key press
	// returns true if handled or false if ignored
	virtual bool KeyUp( int key );
	virtual bool KeyDown( int key );

	// Draw is called when screen must be updated
	virtual void Draw( void );

	// Think is called every frame, before drawing
	virtual void Think( void );

	// Char is a special key press event for text input
	virtual void Char( int key );

	// Called every mouse movement got from engine.
	// Should return true, if
	virtual bool MouseMove( int x, int y ) { return true; }

	// Toggle inactivity of item
	virtual void ToggleInactive( void )
	{
		iFlags ^= QMF_INACTIVE;
	}

	// Direct inacivity set
	virtual void SetInactive( bool visible )
	{
		if( visible ) iFlags |= QMF_INACTIVE;
		else iFlags &= ~QMF_INACTIVE;
	}

	// Cause item to be shown.
	// Simple items will be drawn
	// Window will be added to current window stack
	virtual void Show() { iFlags &= ~QMF_HIDDEN; }

	// Cause item to be hidden
	// Simple item will be hidden
	// Window will be removed from current window stack
	virtual void Hide() { iFlags |= QMF_HIDDEN;  }

	// Determine if this item is visible
	virtual bool IsVisible() const { return !(iFlags & QMF_HIDDEN); }

	// Key value data reading, both parameters are zero-terminated string
	virtual bool KeyValueData( const char *key, const char *data );

	// Determine if this item can be activated using a hotkey
	virtual bool HotKey( int key ) { return false; }

	// Get item default cursor (flags may override this)
	virtual VGUI_DefaultCursor CursorAction() { return dc_arrow; }

	// Toggle visibiltiy.
	inline void ToggleVisibility()
	{
		if( IsVisible() ) Hide();
		else Show();
	}

	// Direct visibility set
	inline void SetVisibility( const bool show )
	{
		if( show ) Show();
		else Hide();
	}

	inline void SetGrayed( const bool grayed )
	{
		if( grayed ) iFlags |= QMF_GRAYED;
		else iFlags &= ~(QMF_GRAYED);
	}

	inline void ToggleGrayed( )
	{
		iFlags ^= QMF_GRAYED;
	}

	// Checks item is current selected in parent Framework
	bool IsCurrentSelected( void ) const;

	// Calculate render positions based on relative positions.
	void CalcPosition( void );

	// Calculate scale size(item size, char size)
	void CalcSizes( void );

	// Play sound
	void PlayLocalSound( const char *name )
	{
		if( iFlags & QMF_SILENT )
			return;

		EngFuncs::PlayLocalSound( name );
	}


	CEventCallback onGotFocus;
	CEventCallback onLostFocus;
	CEventCallback onReleased;
	CEventCallback onChanged;
	CEventCallback onPressed;

	// called when CL_IsActive returns true, otherwise onActivate
	CEventCallback onReleasedClActive;

	inline void SetCoord( int x, int y )                { pos.x = x; pos.y = y; }
	inline void SetSize( int w, int h )                 { size.w = w; size.h = h; }
	inline void SetRect( int x, int y, int w, int h )   { SetCoord( x, y ); SetSize( w, h ); }
	inline Point GetRenderPosition() const { return m_scPos; }
	inline Size  GetRenderSize()     const { return m_scSize; }

	void SetCharSize( EFontSizes fs );

	inline void SetNameAndStatus( const char *name, const char *status, const char *tag = NULL )
	{
		szName = name;
		szStatusText = status;
		szTag = tag;
	}

	CMenuItemsHolder* Parent() const			{ return m_pParent; }

	#ifndef MY_COMPILER_SUCKS
	template <class T> T* _Parent() const	{ return static_cast<T*>(m_pParent); } // a shortcut to parent
	#define GetParent(type) _Parent<type>()
	#else
	template <class T> T* _Parent(T*) const	{ return static_cast<T*>(m_pParent); } // a shortcut to parent
	#define GetParent(type) _Parent((type*)(NULL))
	#endif
	bool IsPressed() const { return m_bPressed; }
	int LastFocusTime() const { return m_iLastFocusTime; }

	unsigned int iFlags;

	Point pos;
	Size size;
	int charSize;

	const char *szName;
	const char *szStatusText;
	const char *szTag; // tag for searching in res file

	CColor colorBase;
	CColor colorFocus;

	unsigned int eTextAlignment;
	EFocusAnimation eFocusAnimation;
	ELetterCase eLetterCase;

	HFont font;

	bool   bDrawStroke;
	CColor colorStroke;
	int    iStrokeWidth;

	int		m_iLastFocusTime;

	// calls specific EventCallback
	virtual void _Event( int ev );
protected:

	// Determine, is this item is absolute positioned
	// If false, it will be positiond relative to it's parent
	virtual bool IsAbsolutePositioned( void ) const { return false; }

	CMenuItemsHolder	*m_pParent;
	bool	m_bPressed;

	bool	m_bAllocName;

	Point m_scPos;
	Size m_scSize;
	int m_scChSize;
};

#include "ItemsHolder.h"

#endif // BASEITEM_H
