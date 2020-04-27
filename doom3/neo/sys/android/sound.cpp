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

const char	*s_driverArgs[]	= { NULL };

extern void (*initAudio)(void *buffer, int size);
extern void (*writeAudio)(int offset, int length);

int m_buffer_size;

class idAudioHardwareAndroid: public idAudioHardware
{
	public:
		unsigned int m_channels;
		unsigned int m_speed;
		void *m_buffer;
		
		idAudioHardwareAndroid(){m_buffer=0;};

		~idAudioHardwareAndroid(){};

		bool Initialize()
		{
		common->Printf("------ Android Sound Initialization ------\n");
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
		bool Flush(void){return true;};
		void Write(bool flushing){};

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

idAudioHardware *idAudioHardware::Alloc()
{
	return new idAudioHardwareAndroid;
}

/*
 ===============
 Sys_LoadOpenAL
 -===============
 */
bool Sys_LoadOpenAL(void)
{
	return false;
}

