//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#pragma once
#ifndef VGUI_SCROLLBAR2_H
#define VGUI_SCROLLBAR2_H

#include <VGUI.h>
#include <VGUI_Panel.h>
#include <VGUI_Dar.h>

namespace vgui
{
class IntChangeSignal;
class Button;
class Slider2;

//-----------------------------------------------------------------------------
// Purpose: Hacked up version of the vgui scrollbar
//-----------------------------------------------------------------------------
class VGUIAPI ScrollBar2 : public Panel
{
public:
	ScrollBar2( int x, int y, int wide, int tall, bool vertical );

	virtual void	 setValue( int value );
	virtual int	 getValue();
	virtual void	 addIntChangeSignal( IntChangeSignal *s ); 
	virtual void	 setRange(int min, int max );
	virtual void	 setRangeWindow( int rangeWindow );
	virtual void	 setRangeWindowEnabled( bool state );
	virtual void	 setSize( int wide, int tall );
	virtual bool	 isVertical();
	virtual bool	 hasFullRange();
	virtual void	 setButton( Button *button,int index );
	virtual Button	*getButton( int index );
	virtual void	 setSlider( Slider2 *slider );
	virtual Slider2	*getSlider();
	virtual void	 doButtonPressed( int buttonIndex );
	virtual void	 setButtonPressedScrollValue( int value );
	virtual void	 validate();
	// bullshit public
	virtual void	 fireIntChangeSignal();
protected:
	virtual void	 performLayout();

	Button			*_button[2];
	Slider2			*_slider;
	Dar<IntChangeSignal*>	 _intChangeSignalDar;
	int			 _buttonPressedScrollValue;
};
}
#endif // VGUI_SCROLLBAR2_H

