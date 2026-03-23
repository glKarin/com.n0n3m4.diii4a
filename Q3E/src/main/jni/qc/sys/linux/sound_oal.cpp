/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#include "../../idlib/precompiled.h"
#include "../../sound/snd_local.h"

class idAudioHardwareLinux : public idAudioHardware {
public:
	idAudioHardwareLinux();
    ~idAudioHardwareLinux();

    bool	Initialize( );

	// driver doesn't support memory map API
	bool	Lock( void **pDSLockedBuffer, ulong *dwDSLockedBufferSize ) { return false; }
	bool	Unlock( void *pDSLockedBuffer, dword dwDSLockedBufferSize ) { return false; }
	bool	GetCurrentPosition( ulong *pdwCurrentWriteCursor ) { return false; }
	int		GetMixBufferSize( void )  { return 0; }
	
	int		GetNumberOfSpeakers( void ) {
		return idSoundSystemLocal::s_numberOfSpeakers.GetInteger();
	}

	// doesn't support write API
	bool	Flush( void ) { return false; }
	void	Write( bool ) { }
	short*	GetMixBuffer( void ) { return NULL; }
};

/*
==========
iAudioHardware::Alloc
==========
*/
idAudioHardware *idAudioHardware::Alloc() { return new idAudioHardwareLinux; }

/*
==========
idAudioHardware::~idAudioHardware
==========
*/
idAudioHardware::~idAudioHardware() { }

/*
 ===============
 Sys_LoadOpenAL
 -===============
 */
bool Sys_LoadOpenAL( void ) {
	return true;
}

/*
==========
idAudioHardwareLinux::idAudioHardwareLinux
==========
*/
idAudioHardwareLinux::idAudioHardwareLinux() {
}

/*
==========
idAudioHardwareLinux::~idAudioHardwareLinux
==========
*/
idAudioHardwareLinux::~idAudioHardwareLinux() {
}

/*
==========
idAudioHardwareLinux::Initialize
==========
*/
bool idAudioHardwareLinux::Initialize( ) {
	return false;
}
