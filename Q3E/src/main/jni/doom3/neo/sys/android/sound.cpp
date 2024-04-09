/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Krzysztof Klinikowski <kkszysiu@gmail.com>

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
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "../../idlib/precompiled.h"
#include "../../sound/snd_local.h"
#include "../posix/posix_public.h"
#include "sound.h"

const char	*s_driverArgs[]	= {
	"AudioTrack", 
#ifdef _OPENSLES
	"OpenSLES",
#endif
	// "AAudio"
	NULL };

int m_buffer_size;

static idCVar s_driver("s_driver", s_driverArgs[0], CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_ROM, "sound driver. only `AudioTrack`"
#ifdef _OPENSLES
		", `OpenSLES`"
#endif
		" be supported in this build", s_driverArgs, idCmdSystem::ArgCompletion_String<s_driverArgs>);

class idAudioHardwareAndroid: public idAudioHardware
{
	public:
		unsigned int m_channels;
		unsigned int m_speed;
		void *m_buffer;
		
		idAudioHardwareAndroid()
		: m_buffer(NULL)
		{}

		~idAudioHardwareAndroid(){
			if(m_buffer)
			{
				shutdownAudio();
				free(m_buffer);
				m_buffer = NULL;
			}
		}

		bool Initialize()
		{
			common->Printf("------ Android AudioTrack Sound Initialization ------\n");
			m_channels = 2;
			idSoundSystemLocal::s_numberOfSpeakers.SetInteger(2);
			m_speed = PRIMARYFREQ;
			m_buffer_size = MIXBUFFER_SAMPLES * m_channels * 2;
			m_buffer = malloc(m_buffer_size);
			memset(m_buffer,0,m_buffer_size);
			initAudio(m_buffer,m_buffer_size);
			return true;
		}

		bool Lock(void **pDSLockedBuffer, ulong *dwDSLockedBufferSize)
		{
			return false;
		}

		bool Unlock(void *pDSLockedBuffer, dword dwDSLockedBufferSize)
		{
			return false;
		}

		bool GetCurrentPosition(ulong *pdwCurrentWriteCursor)
		{
			return false;
		}

		// try to write as many sound samples to the device as possible without blocking and prepare for a possible new mixing call
		// returns wether there is *some* space for writing available
		bool Flush(void){
			writeAudio(-1, 0);
			//Write(true);
			return true;
		};
		void Write(bool flushing){
			writeAudio(0, flushing ? -m_buffer_size : m_buffer_size);
		};

		int GetNumberOfSpeakers(void)
		{
			return m_channels;
		}
		int GetMixBufferSize(void)
		{
			return m_buffer_size;
		}
		short *GetMixBuffer(void)
		{
			return (short *)m_buffer;
		}
};

idAudioHardware::~idAudioHardware() { }

#ifdef _OPENSLES
#include "sound_opensles.cpp"
#endif
idAudioHardware *idAudioHardware::Alloc()
{
#ifdef _OPENSLES
	if(!idStr::Icmp(s_driver.GetString(), "OpenSLES"))
		return new idAudioHardwareOpenSLES;
	else
#endif
		return new idAudioHardwareAndroid;
}


#ifdef _OPENAL
#include <dlfcn.h>
typedef void * HMODULE;
static HMODULE hOpenAL = NULL;
#define GetProcAddress( x, a ) Sys_DLL_GetProcAddress((uintptr_t)x, a)
#include "../../openal/idal.cpp"
#endif
/*
 ===============
 Sys_LoadOpenAL
 -===============
 */
bool Sys_LoadOpenAL(void)
{
#ifdef _OPENAL
	const char *sym;

	if ( hOpenAL ) {
		return true;
	}

	idStr path(Sys_DLLDefaultPath());
	path.AppendPath("libopenal.so");

	hOpenAL = dlopen( path.c_str(), RTLD_NOW | RTLD_GLOBAL );
	if ( !hOpenAL ) {
		common->Warning( "LoadLibrary %s failed.", path.c_str() );
		return false;
	}
	if ( ( sym = InitializeIDAL( hOpenAL ) ) ) {
		common->Warning( "GetProcAddress %s failed.", sym );
		dlclose( hOpenAL );
		hOpenAL = NULL;
		return false;
	}
	return true;
#else
	return false;
#endif
}

/*
===============
Sys_FreeOpenAL
===============
*/
void Sys_FreeOpenAL(void) {
#ifdef _OPENAL
	if ( hOpenAL ) {
		dlclose( hOpenAL );
		hOpenAL = NULL;
	}
#endif
}

#undef GetProcAddress