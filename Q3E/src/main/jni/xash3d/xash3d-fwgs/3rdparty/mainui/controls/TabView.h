/*
TabView.cpp -- tabbed view
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
#ifndef TABWIDGET_H
#define TABWIDGET_H

#include "ItemsHolder.h"

class CMenuTabView : public CMenuItemsHolder
{
public:
	typedef CMenuItemsHolder BaseClass;
	CMenuTabView();

	void VidInit() override;
	void Draw() override;
	Point GetPositionOffset() const override;

	inline void SetTabName( int idx, const char *name )
	{
		if( m_pItems.IsValidIndex( idx ))
			m_pItems[idx]->szName = name;
	}

	inline void AddTabItem( CMenuBaseItem &item, const char *name )
	{
		AddItem( item );
		SetTabName( m_pItems.Count() - 1, name );
	}

private:
	void DrawTab(Point pt, const char *name, bool isEnd , bool isSelected, bool isHighlighted);

	Size m_szTab;
};

#endif // TABWIDGET_H
