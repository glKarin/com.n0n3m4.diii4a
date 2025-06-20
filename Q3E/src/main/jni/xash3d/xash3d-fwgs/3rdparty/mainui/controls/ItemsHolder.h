/*
ItemsHolder.h -- an item that can contain and operate with other items
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
#ifndef EMBEDITEM_H
#define EMBEDITEM_H

#include "BaseItem.h"
#include "utlvector.h"

class CMenuItemsHolder : public CMenuBaseItem
{
public:
	typedef CMenuBaseItem BaseClass;

	CMenuItemsHolder();

	// Overload _Init, _VidInit instead of these methods
	void Init() override;
	void VidInit() override;

	void Reload() override;
	bool KeyUp( int key ) override;
	bool KeyDown( int key ) override;
	void Char( int key ) override;
	void ToggleInactive( void ) override;
	void SetInactive( bool visible ) override;
	void Draw( void ) override;
	void Think( void ) override;

	bool MouseMove( int x, int y ) override;

	bool KeyValueData(const char *key, const char *data) override;

	// returns a position where actual items holder is located
	virtual Point GetPositionOffset() const;
	virtual bool IsWindow() { return false; }

	void CursorMoved( void );
	void SetCursor( int newCursor, bool notify = true );
	void SetCursorToItem( CMenuBaseItem &item, bool notify = true );
	bool AdjustCursor( int dir );

	void AddItem( CMenuBaseItem &item );
	void RemoveItem( CMenuBaseItem &item );
	CMenuBaseItem *ItemAtCursor( void );
	CMenuBaseItem *ItemAtCursorPrev( void );
	CMenuBaseItem *FindItemByTag( const char *tag );
	inline CMenuBaseItem *GetItemByIndex( int idx )
	{
		if( m_pItems.IsValidIndex( idx ))
			return m_pItems[idx];
		return NULL;
	}

	void CalcItemsPositions();
	void CalcItemsSizes();

	inline void AddItem( CMenuBaseItem *item ) { AddItem( *item ); }
	inline int GetCursor() const { return m_iCursor; }
	inline int GetCursorPrev() const { return m_iCursorPrev; }
	inline int ItemCount() const { return m_pItems.Count(); }
	inline bool WasInit() const { return m_bInit; }

	void SetResourceFilename( const char *filename ) { m_szResFile = filename; }

	void RegisterNamedEvent( CEventCallback ev, const char *name );
	CEventCallback FindEventByName( const char *name );

protected:
	virtual void _Init() {}
	virtual void _VidInit() {}
	void VidInitItems();

	bool LoadRES( const char *filename );

	int m_iCursor;
	int m_iCursorPrev;

	CUtlVector<CMenuBaseItem *> m_pItems;

	// it's unnecessary to register here, it's only for searching events by res file
	CUtlVector<CEventCallback> m_events;

	bool m_bInit;
	bool m_bWrapCursor;

	const char *m_szResFile;
private:
	bool Key( const int key, const bool down );
	CMenuBaseItem *m_pItemAtCursorOnDown;

	int m_iHotKeyDown;
};

#endif // EMBEDITEM_H
