/*
SpinControl.h - spin selector
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

#ifndef MENU_SPINCONTROL_H
#define MENU_SPINCONTROL_H

#include "Editable.h"
#include "BaseArrayModel.h"

class CMenuSpinControl : public CMenuEditable
{
public:
	typedef CMenuEditable BaseClass;

	CMenuSpinControl();

	void VidInit( void ) override;
	bool KeyUp( int key ) override;
	bool KeyDown( int key ) override;
	void Draw( void ) override;
	void UpdateEditable() override;

	void Setup( CMenuBaseArrayModel *model );
	void Setup( float minValue, float maxValue, float range );

	void SetDisplayPrecision( short precision );

	void SetCurrentValue( const char *stringValue );
	void SetCurrentValue( float curValue );

	float GetCurrentValue( ) { return m_flCurValue; }
	const char *GetCurrentString( ) { return m_pModel ? m_pModel->GetText( (int)m_flCurValue ) : NULL; }

	void ForceDisplayString( const char *display );

private:
	const char *MoveLeft();
	const char *MoveRight();
	void Display();

	CImage m_szBackground;
	CImage m_szLeftArrow;
	CImage m_szRightArrow;
	CImage m_szLeftArrowFocus;
	CImage m_szRightArrowFocus;
	float  m_flMinValue;
	float  m_flMaxValue;
	float  m_flCurValue;
	float  m_flRange;

	CMenuBaseArrayModel *m_pModel;
	short m_iFloatPrecision;

	char m_szDisplay[CS_SIZE];
};

#endif // MENU_SPINCONTROL_H
