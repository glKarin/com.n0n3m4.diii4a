/*
Table.cpp
Copyright (C) 2010 Uncle Mike
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

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Table.h"
#include "Utils.h"
#include "Scissor.h"

#define HEADER_HEIGHT_FRAC 1.75f

CMenuTable::CMenuTable() : BaseClass(),
	bFramedHintText( false ),
	bAllowSorting( false ),
	bShowScrollBar( true ),
	szBackground(),
	szUpArrow( UI_UPARROW ), szUpArrowFocus( UI_UPARROWFOCUS ), szUpArrowPressed( UI_UPARROWPRESSED ),
	szDownArrow( UI_DOWNARROW ), szDownArrowFocus( UI_DOWNARROWFOCUS ), szDownArrowPressed( UI_DOWNARROWPRESSED ),
	iTopItem( 0 ),
	iScrollBarSliding( false ),
	iHighlight( -1 ), iCurItem( 0 ), iNumRows( 0 ),
	m_iLastItemMouseChange( 0 ),
	m_iSortingColumn( -1 ),
	m_pModel( NULL )
{
	memset( szHeaderTexts, 0, sizeof(szHeaderTexts) );
	memset( columns, 0, sizeof(columns) );
	eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	SetCharSize( QM_SMALLFONT );
	bDrawStroke = true;
}

void CMenuTable::VidInit()
{
	BaseClass::VidInit();

	iBackgroundColor.SetDefault( uiColorBlack );
	iHeaderColor.SetDefault( uiColorHelp );
	colorStroke.SetDefault( uiInputFgColor );
	iStrokeFocusedColor.SetDefault( uiInputTextColor );

	iStrokeWidth = uiStatic.outlineWidth;

	iNumRows = ( m_scSize.h - iStrokeWidth * 2 ) / m_scChSize - 1;

	if( !iCurItem )
	{
		if( iCurItem < iTopItem )
			iTopItem = iCurItem;
		if( iCurItem > iTopItem + iNumRows - 1 )
			iTopItem = iCurItem - iNumRows + 1;
		if( iTopItem > m_pModel->GetRows() - iNumRows )
			iTopItem = m_pModel->GetRows() - iNumRows;
		if( iTopItem < 0 )
			iTopItem = 0;
	}

	flFixedSumm = 0.0f;
	flDynamicSumm = 0.0f;

	for( int i = 0; i < m_pModel->GetColumns(); i++ )
	{
		// this isn't valid behaviour, but still enough for tables without
		// set columns width
		if( !columns[i].flWidth )
		{
			SetColumnWidth( i, 1 / m_pModel->GetColumns(), false );
		}

		if( columns[i].fStaticWidth )
			flFixedSumm += columns[i].flWidth;
		else
			flDynamicSumm += columns[i].flWidth;
	}

	flFixedSumm *= uiStatic.scaleX;

	// at first, determine header height
	headerSize.h = m_scChSize * HEADER_HEIGHT_FRAC;

	// then determine arrow position and sizes
	if( bShowScrollBar )
		arrow.w = arrow.h = 24;
	else
		arrow.w = arrow.h = 0;
	arrow = arrow.Scale();
	downArrow.x = upArrow.x = m_scPos.x + m_scSize.w - arrow.w + iStrokeWidth * 1;
	upArrow.y = m_scPos.y - iStrokeWidth;
	downArrow.y = upArrow.y + m_scSize.h - arrow.h + iStrokeWidth * 2;
	if( !bFramedHintText )
	{
		upArrow.y += headerSize.h;
	}

	// calculate header size(position is table position)
	headerSize.w = m_scSize.w - arrow.w + iStrokeWidth * 2;

	// box is lower than header
	boxPos.x = m_scPos.x;
	boxPos.y = m_scPos.y + headerSize.h;
	boxSize.w = headerSize.w;
	boxSize.h = m_scSize.h - headerSize.h;
}

bool CMenuTable::MouseMove( int x, int y )
{
	if( !iScrollBarSliding && FBitSet( iFlags, QMF_HASMOUSEFOCUS ))
	{
		float step = Step();

		if( UI_CursorInRect( boxPos, boxSize ))
		{
			static float ac_y = 0;

			ac_y += cursorDY;
			cursorDY = 0;
			if( ac_y > m_scChSize / 2.0f )
			{
				iTopItem -= ac_y / m_scChSize - 0.5f;
				ac_y = 0;
			}
			if( ac_y < -m_scChSize / 2.0f )
			{
				iTopItem -= ac_y / m_scChSize - 0.5f;
				ac_y = 0;
			}
		}
		else if( UI_CursorInRect( sbarPos, sbarSize ))
		{
			static float ac_y = 0;

			ac_y += cursorDY;
			cursorDY = 0;
			if( ac_y < -step )
			{
				iTopItem += ac_y / step + 0.5f;
				ac_y = 0;
			}
			if( ac_y > step )
			{
				iTopItem += ac_y / step + 0.5f;
				ac_y = 0;
			}
		}
	}

	if( iScrollBarSliding )
	{
		int dist = uiStatic.cursorY - sbarPos.y - (sbarSize.h / 2);

		if((((dist / 2) > (m_scChSize / 2)) || ((dist / 2) < (m_scChSize / 2))) && iTopItem <= (m_pModel->GetRows() - iNumRows ) && iTopItem >= 0)
		{
			//_Event( QM_CHANGED );

			if((dist / 2) > ( m_scChSize / 2 ) && iTopItem < ( m_pModel->GetRows() - iNumRows - 1 ))
			{
				iTopItem++;
			}

			if((dist / 2) < -(m_scChSize / 2) && iTopItem > 0 )
			{
				iTopItem--;
			}
		}
	}

	iTopItem = bound( 0, iTopItem, m_pModel->GetRows() - iNumRows );

	return true;
}

bool CMenuTable::MoveView(int delta )
{
	iTopItem += delta;

	if( iTopItem < abs(delta) )
	{
		iTopItem = 0;
		return false;
	}
	else if( iTopItem > m_pModel->GetRows() - iNumRows )
	{
		if( m_pModel->GetRows() - iNumRows < 0 )
			iTopItem = 0;
		else
			iTopItem = m_pModel->GetRows() - iNumRows;
		return false;
	}

	return true;
}

bool CMenuTable::MoveCursor(int delta)
{
	iCurItem += delta;

	if( iCurItem < 0 )
	{
		iCurItem = 0;
		return false;
	}
	else if( iCurItem > m_pModel->GetRows() - 1 )
	{
		iCurItem = m_pModel->GetRows() - 1;
		return false;
	}
	return true;
}

void CMenuTable::SetCurrentIndex( int idx )
{
	iCurItem = bound( 0, idx, m_pModel->GetRows() );

	if( iCurItem < iTopItem )
		iTopItem = iCurItem;
	if( iNumRows ) // check if already vidinit
	{
		if( iCurItem > iTopItem + iNumRows - 1 )
			iTopItem = iCurItem - iNumRows + 1;
		if( iTopItem > m_pModel->GetRows() - iNumRows )
			iTopItem = m_pModel->GetRows() - iNumRows;
		if( iTopItem < 0 )
			iTopItem = 0;
	}
	else
	{
		iTopItem = 0; // will be recalculated on vidinit
	}
}

float CMenuTable::Step()
{
	float step = (m_pModel->GetRows() <= 1 ) ? 1 : (downArrow.y - upArrow.y - arrow.h) / (float)(m_pModel->GetRows() - 1);

	return step;
}

bool CMenuTable::KeyUp( int key )
{
	const char *sound = 0;
	int i;
	bool noscroll = false;

	iScrollBarSliding = false;

	if( UI::Key::IsLeftMouse( key ))
	{
		noscroll = true; // don't scroll to current when mouse used

		if( FBitSet( iFlags, QMF_HASMOUSEFOCUS ))
		{
			// test for arrows
			if( UI_CursorInRect( upArrow, arrow ) )
			{
				if( MoveView( -5 ) )
					sound = uiStatic.sounds[SND_MOVE];
				else sound = uiStatic.sounds[SND_BUZZ];
			}
			else if( UI_CursorInRect( downArrow, arrow ))
			{
				if( MoveView( 5 ) )
					sound = uiStatic.sounds[SND_MOVE];
				else sound = uiStatic.sounds[SND_BUZZ];
			}
			else if( UI_CursorInRect( boxPos, boxSize ))
			{
				// test for item select
				int starty = boxPos.y + iStrokeWidth;
				int endy = starty + iNumRows * m_scChSize;
				if( uiStatic.cursorY > starty && uiStatic.cursorY < endy )
				{
					int offsety = uiStatic.cursorY - starty;
					int newCur = iTopItem + offsety / m_scChSize;

					if( newCur < m_pModel->GetRows() )
					{
						if( newCur == iCurItem )
						{
							if( uiStatic.realTime - m_iLastItemMouseChange < 200 ) // 200 msec to double click
							{
								m_pModel->OnActivateEntry( iCurItem );
							}
						}
						else
						{
							iCurItem = newCur;
							sound = uiStatic.sounds[SND_NULL];
						}

						m_iLastItemMouseChange = uiStatic.realTime;
					}
				}
			}
			else if( bAllowSorting && UI_CursorInRect( m_scPos, headerSize ))
			{
				Point p = m_scPos;
				Size sz;
				sz.h = headerSize.h;

				for( i = 0; i < m_pModel->GetColumns(); i++, p.x += sz.w )
				{
					if( columns[i].fStaticWidth )
						sz.w = columns[i].flWidth * uiStatic.scaleX;
					else
						sz.w = ((float)headerSize.w - flFixedSumm) * columns[i].flWidth / flDynamicSumm;

					if( UI_CursorInRect( p, sz ))
					{
						if( GetSortingColumn() != i )
						{
							SetSortingColumn( i );
						}
						else
						{
							SwapOrder();
						}

						_Event( QM_CHANGED );
					}
				}
			}
		}
	}
	else if( UI::Key::IsEnter( key ))
	{
		if( m_pModel->GetRows() )
			m_pModel->OnActivateEntry( iCurItem ); // activate only on release
		else
			sound = uiStatic.sounds[SND_BUZZ]; // list is empty, can't activate anything
	}

	if( !noscroll )
	{
		if( iCurItem < iTopItem )
			iTopItem = iCurItem;
		if( iCurItem > iTopItem + iNumRows - 1 )
			iTopItem = iCurItem - iNumRows + 1;
		if( iTopItem > m_pModel->GetRows() - iNumRows )
			iTopItem = m_pModel->GetRows() - iNumRows;
		if( iTopItem < 0 )
			iTopItem = 0;
	}

	if( sound )
	{
		if( sound != uiStatic.sounds[SND_BUZZ] )
			_Event( QM_CHANGED );

		PlayLocalSound( sound );
	}

	return sound != NULL;
}

bool CMenuTable::KeyDown( int key )
{
	const char *sound = 0;
	bool noscroll = false;

	if( UI::Key::IsUpArrow( key ))
		sound = MoveCursor( -1 ) ? uiStatic.sounds[SND_MOVE] : 0;
	else if( UI::Key::IsDownArrow( key ))
		sound = MoveCursor( 1 ) ? uiStatic.sounds[SND_MOVE] : 0;
	else if( key == K_MWHEELUP )
		sound = MoveCursor( -1 ) ? uiStatic.sounds[SND_MOVE] : uiStatic.sounds[SND_BUZZ];
	else if( key == K_MWHEELDOWN )
		sound = MoveCursor( 1 ) ? uiStatic.sounds[SND_MOVE] : uiStatic.sounds[SND_BUZZ];
	else if( UI::Key::IsPageUp( key ))
		sound = MoveCursor( -2 ) ? uiStatic.sounds[SND_MOVE] : uiStatic.sounds[SND_BUZZ];
	else if( UI::Key::IsPageDown( key ))
		sound = MoveCursor( 2 ) ? uiStatic.sounds[SND_MOVE] : uiStatic.sounds[SND_BUZZ];
	else if( UI::Key::IsHome( key ))
	{
		sound = iCurItem > 0 ? uiStatic.sounds[SND_MOVE] : uiStatic.sounds[SND_BUZZ];
		iCurItem = 0;
	}
	else if( UI::Key::IsEnd( key ))
	{
		int lastItem = Q_max( m_pModel->GetRows() - 1, 0 );
		sound = iCurItem < lastItem ? uiStatic.sounds[SND_MOVE] : uiStatic.sounds[SND_BUZZ];
		iCurItem = lastItem;
	}
	else if( UI::Key::IsDelete( key ))
	{
		if( m_pModel->GetRows() )
			m_pModel->OnDeleteEntry( iCurItem ); // allow removing entries on repeating
		else
			sound = uiStatic.sounds[SND_BUZZ];
	}
	else if( UI::Key::IsLeftMouse( key ))
	{
		noscroll = true; // don't scroll to current when mouse used

		if( FBitSet( iFlags, QMF_HASMOUSEFOCUS ))
		{
			// test for scrollbar
			if( UI_CursorInRect( sbarPos, sbarSize ))
				iScrollBarSliding = true;
		}
	}

	if( !noscroll )
	{
		if( iCurItem < iTopItem )
			iTopItem = iCurItem;
		if( iCurItem > iTopItem + iNumRows - 1 )
			iTopItem = iCurItem - iNumRows + 1;
		if( iTopItem > m_pModel->GetRows() - iNumRows )
			iTopItem = m_pModel->GetRows() - iNumRows;
		if( iTopItem < 0 )
			iTopItem = 0;
	}

	if( sound )
	{
		if( sound != uiStatic.sounds[SND_BUZZ] )
			_Event( QM_CHANGED );

		PlayLocalSound( sound );
	}

	return sound != NULL;
}

void CMenuTable::DrawLine( Point p, const char **psz, size_t size, uint textColor, bool forceCol, uint fillColor )
{
	size_t i;
	Size sz;
	uint textflags = 0;

	textflags |= iFlags & QMF_DROPSHADOW ? ETF_SHADOW : 0;
	textflags |= forceCol ? ETF_FORCECOL : 0;

	sz.h = headerSize.h;

	if( fillColor )
	{
		sz.w = headerSize.w;
		UI_FillRect( p, sz, fillColor );
	}

	for( i = 0; i < size; i++, p.x += sz.w )
	{
		Point pt = p;

		if( columns[i].fStaticWidth )
			sz.w = columns[i].flWidth * uiStatic.scaleX;
		else
			sz.w = ((float)headerSize.w - flFixedSumm) * columns[i].flWidth / flDynamicSumm;

		if( !psz[i] || !sz.w ) // headers may be null, cells too
			continue;

		if( bAllowSorting && i == GetSortingColumn() )
		{
			HIMAGE hPic;

			if( IsAscend() )
				hPic = EngFuncs::PIC_Load( UI_ASCEND );
			else hPic = EngFuncs::PIC_Load( UI_DESCEND );

			if( hPic )
			{
				Point picPos = pt;
				Size picSize = EngFuncs::PIC_Size( hPic ) * uiStatic.scaleX;

				picPos.y += g_FontMgr->GetFontAscent( font );

				if( IsAscend() )
					picPos.y -= picSize.h;

				EngFuncs::PIC_Set( hPic, 255, 255, 255 );
				EngFuncs::PIC_DrawAdditive( picPos, picSize );
				pt.x += picSize.w;
			}
		}

		UI_DrawString( font, pt, sz, psz[i], textColor, m_scChSize,
			m_pModel->GetAlignmentForColumn( i ), textflags );
	}
}

void CMenuTable::DrawLine( Point p, int line, uint textColor, bool forceCol, uint fillColor )
{
	int i;
	Size sz;

	sz.h = m_scChSize;

	unsigned int newFillColor;
	bool forceFillColor = false;
	if( m_pModel->GetLineColor( line, newFillColor, forceFillColor ))
	{
		if( !fillColor || forceFillColor )
			fillColor = newFillColor;
	}

	if( fillColor )
	{
		sz.w = headerSize.w;
		UI_FillRect( p, sz, fillColor );
	}

	for( i = 0; i < m_pModel->GetColumns(); i++, p.x += sz.w )
	{
		uint textflags = 0;

		textflags |= iFlags & QMF_DROPSHADOW ? ETF_SHADOW : 0;

		if( columns[i].fStaticWidth )
			sz.w = columns[i].flWidth * uiStatic.scaleX;
		else
			sz.w = ((float)boxSize.w - flFixedSumm) * columns[i].flWidth / flDynamicSumm;

		const char *str = m_pModel->GetCellText( line, i );
		const ECellType type = m_pModel->GetCellType( line, i );

		if( !str /* && type != CELL_ITEM  */) // headers may be null, cells too
			continue;

		bool useCustomColors = m_pModel->GetCellColors( line, i, newFillColor, forceFillColor );

		if( useCustomColors )
		{
			if( forceFillColor || forceCol )
			{
				textflags |= ETF_FORCECOL;
			}
			textColor = newFillColor;
		}

		switch( type )
		{
		case CELL_TEXT:
			UI_DrawString( font, p, sz, str, textColor, m_scChSize, m_pModel->GetAlignmentForColumn( i ),
				textflags | ( m_pModel->IsCellTextWrapped( line, i ) ? 0 : ETF_NOSIZELIMIT ) );
			break;
		case CELL_IMAGE_ADDITIVE:
		case CELL_IMAGE_DEFAULT:
		case CELL_IMAGE_HOLES:
		case CELL_IMAGE_TRANS:
		{
			HIMAGE pic = EngFuncs::PIC_Load( str );

			if( !pic )
				continue;

			Point picPos = p;
			Size picSize = EngFuncs::PIC_Size( pic );
			float scale = (float)m_scChSize/(float)picSize.h;

			picSize = picSize * scale;

			switch( m_pModel->GetAlignmentForColumn( i ) )
			{
			case QM_RIGHT: picPos.x += ( sz.w - picSize.w ); break;
			case QM_CENTER: picPos.x += ( sz.w - picSize.w ) / 2; break;
			default: break;
			}

			if( useCustomColors )
			{
				int r, g, b, a;
				UnpackRGBA( r, g, b, a, newFillColor );
				EngFuncs::PIC_Set( pic, r, g, b, a );
			}
			else
			{
				EngFuncs::PIC_Set( pic, 255, 255, 255 );
			}

			switch( type )
			{
			case CELL_IMAGE_ADDITIVE:
				EngFuncs::PIC_DrawAdditive( picPos, picSize );
				break;
			case CELL_IMAGE_DEFAULT:
				EngFuncs::PIC_Draw( picPos, picSize );
				break;
			case CELL_IMAGE_HOLES:
				EngFuncs::PIC_DrawHoles( picPos, picSize );
				break;
			case CELL_IMAGE_TRANS:
				EngFuncs::PIC_DrawTrans( picPos, picSize );
				break;
			default: break; // shouldn't happen
			}

			break;
		}
		}
	}
}

void CMenuTable::Draw()
{
	int i, y;
	int selColor = PackRGB( 80, 56, 24 );
	int upFocus, downFocus, scrollbarFocus;

	// HACKHACK: recalc iNumRows, to be not greater than iNumItems
	iNumRows = ( m_scSize.h - iStrokeWidth * 2 ) / m_scChSize - 1;
	if( iNumRows > m_pModel->GetRows() )
		iNumRows = m_pModel->GetRows();

	// HACKHACK: normalize iTopItem
	// remove when there will be per-pixel scrolling
	iTopItem = bound( 0, iTopItem, m_pModel->GetRows() - 1 );

	if( UI_CursorInRect( boxPos, boxSize ) )
	{
		int newCur = iTopItem + ( uiStatic.cursorY - boxPos.y ) / m_scChSize;

		if( newCur < m_pModel->GetRows() )
			iHighlight = newCur;
		else iHighlight = -1;
	}
	else iHighlight = -1;

	if( szBackground )
	{
		UI_DrawPic( m_scPos, m_scSize, uiColorWhite, szBackground );
	}
	else
	{
		// draw the opaque outlinebox first
		if( bFramedHintText )
		{
			UI_FillRect( m_scPos, headerSize.AddVertical( boxSize ), iBackgroundColor );
		}
		else
		{
			UI_FillRect( boxPos, boxSize, iBackgroundColor );
		}
	}

	int columns = Q_min( m_pModel->GetColumns(), MAX_TABLE_COLUMNS );

	DrawLine( m_scPos, szHeaderTexts, columns, iHeaderColor, true );

	if( !szBackground )
	{
		int color;

		if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS && iFlags & QMF_HASKEYBOARDFOCUS )
			color = iStrokeFocusedColor;
		else
			color = colorStroke;

		if( bFramedHintText )
		{
			UI_DrawRectangleExt( m_scPos, headerSize, color, iStrokeWidth, QM_LEFT | QM_TOP | QM_RIGHT );
		}

		if( bDrawStroke )
			UI_DrawRectangleExt( boxPos, boxSize, color, iStrokeWidth );
	}


	sbarPos.x = upArrow.x + arrow.w * 0.125f;
	sbarSize.w = arrow.w * 0.75f;

	float step = Step();

	if(((downArrow.y - upArrow.y - arrow.h) - (((m_pModel->GetRows()-1)*m_scChSize)/2)) < 2)
	{
		sbarSize.h = (downArrow.y - upArrow.y - arrow.h) - (step * (m_pModel->GetRows() - iNumRows));
		sbarPos.y = upArrow.y + arrow.h + (step*iTopItem);
	}
	else
	{
		sbarSize.h = downArrow.y - upArrow.y - arrow.h - (((m_pModel->GetRows()- iNumRows) * m_scChSize) / 2);
		sbarPos.y = upArrow.y + arrow.h + (((iTopItem) * m_scChSize)/2);
	}

	// draw the arrows base
	UI_FillRect( upArrow.x, upArrow.y + arrow.h,
		arrow.w, downArrow.y - upArrow.y - arrow.h, uiInputFgColor );

	// ADAMIX
	if( iScrollBarSliding )
	{
		// Draw scrollbar background
		UI_FillRect( sbarPos.x, upArrow.y + arrow.h, sbarSize.w, downArrow.y - upArrow.y - arrow.h, uiColorBlack);
	}

	// ADAMIX END
	// draw the arrows
	if( iFlags & QMF_GRAYED )
	{
		UI_DrawPic( upArrow, arrow, uiColorDkGrey, szUpArrow );
		UI_DrawPic( downArrow, arrow, uiColorDkGrey, szDownArrow );
	}
	else
	{
		scrollbarFocus = UI_CursorInRect( sbarPos, sbarSize );

		// special case if we sliding but lost focus
		if( iScrollBarSliding ) scrollbarFocus = true;

		// Draw scrollbar itself
		UI_FillRect( sbarPos, sbarSize, scrollbarFocus ? uiInputTextColor : uiColorBlack );

		if(this != m_pParent->ItemAtCursor())
		{
			UI_DrawPic( upArrow, arrow, uiColorWhite, szUpArrow );
			UI_DrawPic( downArrow, arrow, uiColorWhite, szDownArrow );
		}
		else
		{
			// see which arrow has the mouse focus
			upFocus = UI_CursorInRect( upArrow, arrow );
			downFocus = UI_CursorInRect( downArrow, arrow );

			if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS )
			{
				UI_DrawPic( upArrow, arrow, uiColorWhite, (upFocus) ? szUpArrowFocus : szUpArrow );
				UI_DrawPic( downArrow, arrow, uiColorWhite, (downFocus) ? szDownArrowFocus : szDownArrow );
			}
			else if( eFocusAnimation == QM_PULSEIFFOCUS )
			{
				int	color;

				color = PackAlpha( colorBase, 255 * (0.5f + 0.5f * sin( (float)uiStatic.realTime / UI_PULSE_DIVISOR )));

				UI_DrawPic( upArrow, arrow, (upFocus) ? color : (int)colorBase, (upFocus) ? szUpArrowFocus : szUpArrow );
				UI_DrawPic( downArrow, arrow, (downFocus) ? color : (int)colorBase, (downFocus) ? szDownArrowFocus : szDownArrow );
			}
		}
	}


	// prevent the columns out of rectangle bounds
	UI::Scissor::PushScissor( boxPos, boxSize );
	y = boxPos.y;

	for( i = iTopItem; i < m_pModel->GetRows() && i < iNumRows + iTopItem; i++, y += m_scChSize )
	{
		int color = colorBase; // predict state
		bool forceCol = false;
		int fillColor = 0;

		if( iFlags & QMF_GRAYED )
		{
			color = uiColorDkGrey;
			forceCol = true;
		}
		else if( !(iFlags & QMF_INACTIVE) )
		{
			if( i == iCurItem )
			{
				if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS )
					color = colorFocus;
				else if( eFocusAnimation == QM_PULSEIFFOCUS )
					color = PackAlpha( colorBase, 255 * (0.5f + 0.5f * sin( (float)uiStatic.realTime / UI_PULSE_DIVISOR )));

				fillColor = selColor;
			}
			else if( i == iHighlight )
			{
				fillColor = 0x80383838;
			}
		}

		DrawLine( Point( boxPos.x, y ), i, color, forceCol, fillColor );
	}

	UI::Scissor::PopScissor();
}
