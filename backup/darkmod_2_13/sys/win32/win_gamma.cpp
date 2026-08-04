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
/*
** WIN_GAMMA.C
*/
#include <assert.h>
#include "win_local.h"
#include "../../renderer/tr_local.h"

static unsigned short s_oldHardwareGamma[3][256];

/*
** WG_GetOldGammaRamp
**
*/
void WG_GetOldGammaRamp( void )
{
	HDC			hDC;

	hDC = GetDC( GetDesktopWindow() );
	GetDeviceGammaRamp( hDC, s_oldHardwareGamma );
	ReleaseDC( GetDesktopWindow(), hDC );


/*
** GLimp_SetGamma
**
*/
void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
	unsigned short table[3][256];
	int i;

	if ( !glw_state.hDC )
	{
		return;
	}

	for ( i = 0; i < 256; i++ )
	{
		table[0][i] = ( ( ( unsigned short ) red[i] ) << 8 ) | red[i];
		table[1][i] = ( ( ( unsigned short ) green[i] ) << 8 ) | green[i];
		table[2][i] = ( ( ( unsigned short ) blue[i] ) << 8 ) | blue[i];
	}

	if ( !SetDeviceGammaRamp( glw_state.hDC, table ) ) {
		common->Printf( "WARNING: SetDeviceGammaRamp failed.\n" );
	}
}

/*
** WG_RestoreGamma
*/
void WG_RestoreGamma( void )
{
	HDC hDC;

	// if we never read in a reasonable looking
	// table, don't write it out
	if ( s_oldHardwareGamma[0][255] == 0 ) {
		return;
	}

	hDC = GetDC( GetDesktopWindow() );
	SetDeviceGammaRamp( hDC, s_oldHardwareGamma );
	ReleaseDC( GetDesktopWindow(), hDC );
}

