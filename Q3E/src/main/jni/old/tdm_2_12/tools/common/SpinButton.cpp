/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "SpinButton.h"

void SpinButton_SetIncrement ( HWND hWnd, float inc )
{
	SetWindowLongPtr ( hWnd, GWLP_USERDATA, (long)(inc * 100.0f) );
}

void SpinButton_SetRange ( HWND hWnd, float minRange, float maxRange )
{
	SendMessage ( hWnd, UDM_SETRANGE32, (WPARAM)(minRange*100.0f), (LPARAM)(maxRange*100.0f) );	
}

void SpinButton_HandleNotify ( NMHDR* hdr )
{
	// Return if incorrect data in edit box 
	NM_UPDOWN* udhdr= (NM_UPDOWN*)hdr;

	// Change with 0.1 on each click 
	char strValue[64];
	float value;				
	GetWindowText ( (HWND)SendMessage ( hdr->hwndFrom, UDM_GETBUDDY, 0, 0 ), strValue, 63 );
	
	float inc = (float)GetWindowLongPtr ( hdr->hwndFrom, GWLP_USERDATA );
	if ( inc == 0 )
	{
		inc = 100.0f;
		SetWindowLongPtr ( hdr->hwndFrom, GWLP_USERDATA, 100 );
	}
	inc /= 100.0f;
	
	if ( GetAsyncKeyState ( VK_SHIFT ) & 0x8000 )
	{
		inc *= 10.0f;
	}
	
	value  = atof(strValue);				
	value += (udhdr->iDelta)*(inc);

	// Avoid round-off errors 
	value = floor(value*1e3+0.5)/1e3;
	
	LONG minRange;
	LONG maxRange; 
	SendMessage ( hdr->hwndFrom, UDM_GETRANGE32, (WPARAM)&minRange, (LPARAM)&maxRange );
	if ( minRange !=  0 || maxRange != 0 )
	{
		float minRangef = (float)(long)minRange / 100.0f;
		float maxRangef = (float)maxRange / 100.0f;
		if ( value > maxRangef )
		{
			value = maxRangef;
		}
		if ( value < minRangef )
		{
			value = minRangef;
		}		
	}
	
	SetWindowText ( (HWND)SendMessage ( hdr->hwndFrom, UDM_GETBUDDY, 0, 0 ), va("%g",value) );
}
