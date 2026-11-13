// SPDX-License-Identifier: GPL-3.0-or-later
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

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a
copy of these additional terms immediately following the terms and conditions of the GNU General Public
License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software
at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing
id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#include <SDL.h>
#include "../../idlib/precompiled.h"
#include "../../sound/snd_local.h"

// SDL's queued audio API supplies the portable fallback mixer. OpenAL Soft
// remains the preferred spatial-audio path and is loaded below.
class idAudioHardwareSDL : public idAudioHardware {
    SDL_AudioDeviceID device;
    short samples[MIXBUFFER_SAMPLES * 2];

  public:
    idAudioHardwareSDL() : device(0) {
        memset(samples, 0, sizeof(samples));
    }
    ~idAudioHardwareSDL() {
        if (device)
            SDL_CloseAudioDevice(device);
    }
    bool Initialize() {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
            return false;
        SDL_AudioSpec want;
        SDL_zero(want);
        want.freq = PRIMARYFREQ;
        want.format = AUDIO_S16SYS;
        want.channels = 2;
        want.samples = MIXBUFFER_SAMPLES;
        device = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
        if (!device) {
            common->Warning("SDL audio: %s", SDL_GetError());
            return false;
        }
        idSoundSystemLocal::s_numberOfSpeakers.SetInteger(2);
        SDL_PauseAudioDevice(device, 0);
        return true;
    }
    bool Lock(void **, ulong *) {
        return false;
    }
    bool Unlock(void *, dword) {
        return false;
    }
    bool GetCurrentPosition(ulong *) {
        return false;
    }
    bool Flush() {
        return device && SDL_GetQueuedAudioSize(device) < sizeof(samples) * 2;
    }
    void Write(bool) {
        if (device && SDL_GetQueuedAudioSize(device) < sizeof(samples) * 3)
            SDL_QueueAudio(device, samples, sizeof(samples));
    }
    int GetNumberOfSpeakers() {
        return 2;
    }
    int GetMixBufferSize() {
        return sizeof(samples);
    }
    short *GetMixBuffer() {
        return samples;
    }
};
idAudioHardware::~idAudioHardware() {}
idAudioHardware *idAudioHardware::Alloc() {
    return new idAudioHardwareSDL;
}

#ifdef _OPENAL
#include <dlfcn.h>
typedef void *HMODULE;
static HMODULE hOpenAL = NULL;
#define GetProcAddress(x, a) Sys_DLL_GetProcAddress((uintptr_t)x, a)
#include "../../openal/idal.cpp"
#endif

/*
 ===============
 Sys_LoadOpenAL
 -===============
 */
bool Sys_LoadOpenAL(void) {
#ifdef _OPENAL
    const char *sym;

    if (hOpenAL) {
        return true;
    }

    const char *path = cvarSystem->GetCVarString("s_libOpenAL");
    if (!path || !path[0])
        return false;

    hOpenAL = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (!hOpenAL) {
        common->Warning("LoadLibrary %s failed.", path);
        return false;
    }
    if ((sym = InitializeIDAL(hOpenAL))) {
        common->Warning("GetProcAddress %s failed.", sym);
        dlclose(hOpenAL);
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
    if (hOpenAL) {
        dlclose(hOpenAL);
        hOpenAL = NULL;
    }
#endif
}

#undef GetProcAddress
