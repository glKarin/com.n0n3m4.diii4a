/*
Table.h - Table
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

#ifndef MENU_TABLE_H
#define MENU_TABLE_H

#include "BaseItem.h"
#include "BaseModel.h"

#define MAX_TABLE_COLUMNS 16

/*
 * CMenuTable
 *
 * In order to keep simplicity, there is some limitations.
 * If you will keep them in mind, you will find tables simple, easy and fast to add on your window
 *
 * 1. CMenuTable uses a very simple MVC pattern, but there controller is merged with model(OnDelete, OnActivate)
 * It means, that you have to inherit from CMenuBaseModel, implement all pure methods and only then you can put your data on table.
 *
 * 2. CMenuTable will call Update() only when you will pass a model pointer to table.
 *
 * 3. Adding table on items holder before you have passed a model pointer is PROHIBITED and will lead to crash.
 *
 * 4. You can't use columns more than MAX_TABLE_COLUMNS
 *
 * 5. Header text is set per Table instance.
 *
 * 6. Column widths are constant(at this moment). You should not exceed 1.0 in total columns width
 *
 */

class CMenuTable : public CMenuBaseItem
{
public:
	typedef CMenuBaseItem BaseClass;

	CMenuTable();


	bool KeyUp( int key ) override;
	bool KeyDown( int key ) override;
	void Draw() override;
	void VidInit() override;
	bool MouseMove( int x, int y ) override;
	bool MoveView( int delta );
	bool MoveCursor( int delta );
	int GetCurrentIndex() { return iCurItem; }
	void SetCurrentIndex( int idx );
	int GetSortingColumn( void ) { return m_iSortingColumn; }
	bool IsAscend( void ) { return m_bAscend; }
	void SetSortingColumn( int column, bool ascend = true )
	{
		m_iSortingColumn = column;
		m_bAscend = ascend;
		if( !m_pModel->Sort( column, ascend ) )
			m_iSortingColumn = -1; // sorting is not supported
	}

	void SwapOrder( void )
	{
		SetSortingColumn( m_iSortingColumn, !m_bAscend );
	}

	void DisableSorting( void )
	{
		SetSortingColumn( -1 );
	}

	void SetUpArrowPicture( const char *upArrow, const char *upArrowFocus, const char *upArrowPressed )
	{
		szUpArrow = upArrow;
		szUpArrowFocus = upArrowFocus;
		szUpArrowPressed = upArrowPressed;
	}

	void SetDownArrowPicture( const char *downArrow, const char *downArrowFocus, const char *downArrowPressed )
	{
		szDownArrow = downArrow;
		szDownArrowFocus = downArrowFocus;
		szDownArrowPressed = downArrowPressed;
	}

	void SetModel( CMenuBaseModel *model )
	{
		m_pModel = model;
		m_pModel->Update();
	}

	void SetHeaderText( int num, const char *text )
	{
		if( num < MAX_TABLE_COLUMNS && num >= 0 )
			szHeaderTexts[num] = text;
	}

	// widths are in [0.0; 1.0]
	// Total of all widths should be 1.0f, but not necessary.
	// to keep everything simple, if first few columns exceeds 1.0, the other will not be shown
	// if you have set fixed == true, then column size is set in logical units
	void SetColumnWidth( int num, float width, bool fixed = false )
	{
		if( num < MAX_TABLE_COLUMNS && num >= 0 )
		{
			columns[num].flWidth = width;
			columns[num].fStaticWidth = fixed;
		}
	}

	// shortcut for SetHeaderText && SetColumnWidth
	inline void SetupColumn( int num, const char *text, float width, bool fixed = false )
	{
		SetHeaderText( num, text );
		SetColumnWidth( num, width, fixed );
	}

	bool bFramedHintText;
	bool bAllowSorting;
	bool bShowScrollBar;

	CColor iStrokeFocusedColor;

	CColor iBackgroundColor;
	CColor iHeaderColor;

private:
	float Step( void );

	void DrawLine(Point p, const char **psz, size_t size, uint textColor, bool forceCol, uint fillColor = 0);
	void DrawLine(Point p, int line, uint textColor, bool forceCol, uint fillColor = 0);

	const char	*szHeaderTexts[MAX_TABLE_COLUMNS];
	struct
	{
		float flWidth;
		bool fStaticWidth;
	} columns[MAX_TABLE_COLUMNS];

	float flFixedSumm, flDynamicSumm;

	CImage szBackground;
	CImage szUpArrow;
	CImage szUpArrowFocus;
	CImage szUpArrowPressed;
	CImage szDownArrow;
	CImage szDownArrowFocus;
	CImage szDownArrowPressed;

	int		iTopItem;
	int     iNumRows;
// scrollbar stuff // ADAMIX
	Point	sbarPos;
	Size	sbarSize;
	bool	iScrollBarSliding;
// highlight // mittorn
	int		iHighlight;
	int		iCurItem;

	int		m_iLastItemMouseChange;

	// sorting
	int m_iSortingColumn;
	bool m_bAscend;

	// header
	Size headerSize;

	// arrows
	Point upArrow;
	Point downArrow;
	Size arrow;

	// actual table
	Point boxPos;
	Size boxSize;

	CMenuBaseModel *m_pModel;
};


#endif // MENU_TABLE_H
