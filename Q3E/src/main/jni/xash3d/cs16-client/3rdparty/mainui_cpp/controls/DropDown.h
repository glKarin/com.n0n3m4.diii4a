/*
DropDown.h - simple drop down menus
Copyright (C) 2023 numas13

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef DROP_DOWN_H
#define DROP_DOWN_H

#include "Editable.h"
#include "utlvector.h"

// TODO: make CMenuDropDown compatible with model-view pattern
// TODO: convert Str/Float/Int classes to templates

class CMenuDropDown : public CMenuEditable
{
public:
	typedef CMenuEditable BaseClass;

	enum valType_e
	{
		VALUE_STRING = 0,
		VALUE_INT,
		VALUE_FLOAT,
	};

	bool KeyDown( int key ) override;
	bool KeyUp( int key ) override;
	void VidInit() override;
	void Draw() override;

	int GetCount()
	{
		return m_szNames.Count();
	}

	void SelectItem( int i, bool event = true )
	{
		if( !m_szNames.IsValidIndex( i ))
			return;

		if( i != m_iState )
		{
			m_iState = i;
			if( event )
				_Event( QM_CHANGED );
		}
		else
			m_iState = i;
	}

	void SelectLast( bool event = true )
	{
		SelectItem( GetCount( ) - 1, event );
	}

	void SetMenuOpen( bool state );
	void MenuOpen() { SetMenuOpen( true ); }
	void MenuClose() { SetMenuOpen( false ); }
	void MenuToggle() { SetMenuOpen( !m_isOpen ); }

	// recalculate size
	void ForceRecalc()
	{
		MenuToggle( );
		MenuToggle( );
	}

	virtual void SetCvar() = 0;

	CColor iBackgroundColor;
	CColor iFgTextColor;
	CColor iBgTextColor;

	bool bDropUp;

protected:
	CMenuDropDown();

	void Clear()
	{
		m_iState = 0;
		m_szNames.RemoveAll();
	}

	void AddItemName ( const char *text )
	{
		m_szNames.AddToTail( text );
		ForceRecalc();
	}

	int m_iState;
	bool m_isOpen;

private:
	int IsNewStateByMouseClick();

	CUtlVector<const char *> m_szNames;
	Size m_scItemSize;

	CImage m_ArrowClosed;
	CImage m_ArrowOpened;
	Size m_ArrowSize;
};

class CMenuDropDownStr : public CMenuDropDown
{
public:
	CMenuDropDownStr() : CMenuDropDown() {}
	void UpdateEditable() override;

	void SetCvar() override
	{
		SetCvarString( m_Values[m_iState] );
	}

	void Clear()
	{
		CMenuDropDown::Clear();
		m_Values.RemoveAll();
	}

	void AddItem( const char *text, const char *value )
	{
		m_Values.AddToTail( value );
		AddItemName( text );
	}

	const char *GetItem()
	{
		return GetItem( m_iState );
	}

	const char *GetItem( int i )
	{
		return m_Values.IsValidIndex( i ) ? m_Values[i] : nullptr;
	}

private:
	CUtlVector<const char *> m_Values;
};

class CMenuDropDownInt : public CMenuDropDown
{
public:
	CMenuDropDownInt() : CMenuDropDown() {}
	void UpdateEditable() override;

	void SetCvar() override
	{
		SetCvarValue( m_Values[m_iState] );
	}

	void Clear()
	{
		CMenuDropDown::Clear();
		m_Values.RemoveAll();
	}

	void AddItem( const char *text, int value )
	{
		m_Values.AddToTail( value );
		AddItemName( text );
	}

	int GetItem()
	{
		return GetItem( m_iState );
	}

	int GetItem( int i ) const
	{
		return m_Values.IsValidIndex( i ) ? m_Values[i] : 0;
	}

private:
	CUtlVector<int> m_Values;
};

class CMenuDropDownFloat : public CMenuDropDown
{
public:
	CMenuDropDownFloat() : CMenuDropDown() {}
	void UpdateEditable() override;

	void SetCvar() override
	{
		SetCvarValue( m_Values[m_iState] );
	}

	void Clear()
	{
		CMenuDropDown::Clear();
		m_Values.RemoveAll();
	}

	void AddItem( const char *text, float value )
	{
		m_Values.AddToTail( value );
		AddItemName( text );
	}

	float GetItem()
	{
		return GetItem( m_iState );
	}

	float GetItem( int i )
	{
		return m_Values.IsValidIndex( i ) ? m_Values[i] : 0;
	}

private:
	CUtlVector<float> m_Values;
};

#endif // DROP_DOWN_H
