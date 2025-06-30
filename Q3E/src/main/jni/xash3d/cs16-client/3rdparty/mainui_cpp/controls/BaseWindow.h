/*
BaseWindow.h -- base menu window
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
#ifndef BASEWINDOW_H
#define BASEWINDOW_H

#include "ItemsHolder.h"
#include "BackgroundBitmap.h"

// Base class for windows.
// Should be used for message boxes, dialogs, root menus(e.g. frameworks)
class CMenuBaseWindow : public CMenuItemsHolder
{
public:
	typedef CMenuItemsHolder BaseClass;
	CMenuBaseWindow( const char *name = "Unnamed Window", CWindowStack *pStack = &uiStatic.menu );

	// Overloaded functions
	// Window visibility is switched through window stack
	void Hide() override;
	void Show() override;
	bool IsVisible() const override;

	bool KeyUp( int key ) override;
	bool KeyDown( int key ) override;
	void Draw() override;

	bool KeyValueData(const char *key, const char *data) override;

	enum EAnimation
	{
		ANIM_NO = 0,  // no animation
		ANIM_CLOSING, // window closing animation
		ANIM_OPENING, // window showing animation
	};

	// Override this method to draw custom animations
	// For example, during transitions
	// Return false when animation is still going
	// Otherwise return true, so window will be marked as "no animation"
	// and this method will not be called anymore(until next menu transition)
	virtual bool DrawAnimation();

	// Check current window is a root
	virtual bool IsRoot() const { return false; }

	// Hide current window and save changes
	virtual void SaveAndPopMenu();

	bool IsWindow() override { return true; }

	void EnableTransition( EAnimation type );
	void DisableTransition() { eTransitionType = ANIM_NO; }

	// set parent of window
	void Link( CMenuItemsHolder *h )
	{
		m_pParent = h;
	}

	bool bAllowDrag;
	EAnimation eTransitionType; // valid only when in transition

	const CWindowStack *WindowStack() const
	{
		return m_pStack;
	}

protected:
	int m_iTransitionStartTime;

	CWindowStack *m_pStack;
private:
	CMenuBaseWindow(); // remove

	friend void UI_DrawMouseCursor( void ); // HACKHACK: Cursor should be set by menu item
	friend void UI_UpdateMenu( float flTime );

	bool IsAbsolutePositioned( void ) const override { return true; }
	void DragDrop( int down );

	bool m_bHolding;
	Point m_bHoldOffset;
};

#endif // BASEWINDOW_H
