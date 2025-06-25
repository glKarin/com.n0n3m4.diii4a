/*
Slider.h - slider
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

#ifndef MENU_SLIDER_H
#define MENU_SLIDER_H
#define UI_SLIDER_MAIN		"gfx/shell/slider"

#include "Editable.h"

class CMenuSlider : public CMenuEditable
{
public:
	typedef CMenuEditable BaseClass;

	CMenuSlider();
	void VidInit( void ) override;
	bool KeyUp( int key ) override;
	bool KeyDown( int key ) override;
	void Draw( void ) override;
	void UpdateEditable() override;
	void LinkCvar(const char *name) override
	{
		CMenuEditable::LinkCvar(name, CVAR_VALUE);
	}

	void Setup( float minValue, float maxValue, float range )
	{
		m_flMinValue = minValue;
		m_flMaxValue = maxValue;
		m_flRange = range;
	}
	void SetCurrentValue( float curValue )
	{
		m_flCurValue = curValue > m_flMaxValue ? m_flMaxValue :
						( curValue < m_flMinValue ? m_flMinValue : curValue );
	}

	float GetCurrentValue() { return m_flCurValue; }
	// void SetDrawStep( float drawStep, int numSteps );

	void SetKeepSlider( int keepSlider ) { m_iKeepSlider = keepSlider; }

	CImage imgSlider;
private:
	float	m_flMinValue;
	float	m_flMaxValue;
	float	m_flCurValue;
	float	m_flDrawStep;
	int		m_iNumSteps;
	float	m_flRange;
	int		m_iKeepSlider;	// when mouse button is holds

	int m_iSliderOutlineWidth;
	Size m_scCenterBox;
};

#endif // MENU_SLIDER_H
