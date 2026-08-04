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
#include "../idlib/precompiled.h"
#include "../sound/snd_local.h"

AL_API ALenum AL_APIENTRY alGetError() {
	return AL_NO_ERROR;
}

AL_API ALboolean AL_APIENTRY alIsSource( ALuint sid ) {
	return AL_FALSE;
}

AL_API void AL_APIENTRY alGenBuffers( ALsizei n, ALuint* buffers ) { }

AL_API void AL_APIENTRY alSourceStop( ALuint sid ) { }

AL_API void AL_APIENTRY alGetSourcei( ALuint sid,  ALenum pname, ALint* value ) { }

AL_API ALint AL_APIENTRY alGetInteger( ALenum param ) { return 0; }

ALC_API void ALC_APIENTRY alcSuspendContext( ALCcontext *alcHandle ) { }

ALC_API ALCdevice * ALC_APIENTRY alcOpenDevice( const ALchar *tokstr ) {
	return NULL;
}

AL_API void AL_APIENTRY alDeleteBuffers( ALsizei n, const ALuint* buffers ) { }

AL_API ALboolean AL_APIENTRY alIsExtensionPresent( const ALchar* fname ) {
	return AL_FALSE;
}

AL_API void AL_APIENTRY alBufferData( ALuint   buffer,
									ALenum   format,
									const ALvoid*    data,
									ALsizei  size,
									ALsizei  freq ) { }

ALC_API ALCboolean ALC_APIENTRY alcMakeContextCurrent( ALCcontext *alcHandle ) {
	return 0;
}

ALC_API ALCvoid ALC_APIENTRY alcProcessContext( ALCcontext *alcHandle ) {
	return;
}

ALC_API ALCvoid ALC_APIENTRY alcDestroyContext( ALCcontext *alcHandle ) {
	return;
}

ALC_API const ALCchar * ALC_APIENTRY alcGetString( ALCdevice *deviceHandle, ALCenum token ) {
	return NULL;
}

AL_API void AL_APIENTRY alBufferData( ALuint   buffer,
									ALenum   format,
									ALvoid*    data,
									ALsizei  size,
									ALsizei  freq ) { }

AL_API void AL_APIENTRY alDeleteBuffers( ALsizei n, ALuint* buffers ) { }

AL_API ALboolean AL_APIENTRY alIsExtensionPresent( ALubyte* fname ) {
	return AL_FALSE;
}

AL_API void AL_APIENTRY alDeleteSources( ALsizei n, const ALuint* sources ) { }

AL_API ALenum AL_APIENTRY alGetEnumValue( const ALchar* ename ) {
	return 0;
}

AL_API void* AL_APIENTRY alGetProcAddress( const ALchar* fname ) {
	return NULL;
}

ALC_API ALCcontext * ALC_APIENTRY alcCreateContext( ALCdevice *dev,
												  const ALCint* attrlist ) { 
	return NULL;
}

ALC_API ALCdevice * ALC_APIENTRY alcOpenDevice( ALubyte *tokstr ) {
	return NULL;
}

AL_API void AL_APIENTRY alListenerfv( ALenum pname, const ALfloat* param ) { }

AL_API void AL_APIENTRY alSourceQueueBuffers( ALuint sid, ALsizei numEntries, const ALuint *bids ) { }

AL_API void AL_APIENTRY alSourcei( ALuint sid, ALenum param, ALint value ) { }

AL_API void AL_APIENTRY alListenerf( ALenum pname, ALfloat param ) { }

ALC_API ALCboolean ALC_APIENTRY alcCloseDevice( ALCdevice *dev ) { return false; }

AL_API ALboolean AL_APIENTRY alIsBuffer( ALuint buffer ) {
	return AL_FALSE;
}

AL_API void AL_APIENTRY alSource3f( ALuint sid, ALenum param,
                                  ALfloat f1, ALfloat f2, ALfloat f3 ) { }

AL_API void AL_APIENTRY alGenSources( ALsizei n, ALuint* sources ) { }

AL_API void AL_APIENTRY alSourcef( ALuint sid, ALenum param, ALfloat value ) { }

AL_API void AL_APIENTRY alSourceUnqueueBuffers( ALuint sid, ALsizei numEntries, ALuint *bids ) { }

AL_API void AL_APIENTRY alSourcePlay( ALuint sid ) { }
