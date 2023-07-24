// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SOUNDWORLD_H__
#define __SOUNDWORLD_H__

#include "SoundEmitter.h"

/*
===============================================================================

	SOUND WORLD

There can be multiple independent sound worlds, just as there can be multiple
independent render worlds.  The prime example is the editor sound preview
option existing simultaneously with a live game.
===============================================================================
*/

class idDemoFile;

class idSoundWorld {
public:
	virtual					~idSoundWorld( void ) {}

	// call at each map start
	virtual void			ClearAllSoundEmitters( void ) = 0;
	virtual void			StopAllSounds( void ) = 0;

	// get a new emitter that can play sounds in this world
	virtual idSoundEmitter*	AllocSoundEmitter( void ) = 0;

	// for load games, index 0 will return NULL
	virtual idSoundEmitter*	EmitterForIndex( int index ) = 0;

	// build list of contributing audio channels
	virtual void			MixLoop( int current44kHz ) = 0;

	// query sound samples from all emitters reaching a given position
	virtual	float			CurrentShakeAmplitudeForPosition( const int time, const idVec3& listenerPosition ) = 0;

	// where is the camera/microphone
	// listenerId allows listener-private and antiPrivate sounds to be filtered
	// gameTime is in msec, and is used to time sound queries and removals so that they are independent
	// of any race conditions with the async update
	virtual	void			PlaceListener( const idVec3& origin, const idMat3& axis, const int listenerId, const int gameTime ) = 0;

	virtual const idVec3&	GetListenerPosition() const = 0;
	virtual const idMat3&	GetListenerAxis() const = 0;

	// fade all sounds in the world with a given shader soundClass
	// to is in Db (sigh), over is in seconds
	virtual void			FadeSoundClasses( const int soundClass, const float to, const float over ) = 0;

	// background music
	virtual	void			PlayShaderDirectly( const idSoundShader* shader, const soundChannel_t channel = SCHANNEL_ANY, int* length = NULL ) = 0;

	// dumps the current state and begins archiving commands
	virtual void			StartWritingDemo( idDemoFile *demo ) = 0;
	virtual void			StopWritingDemo( void ) = 0;

	// read a sound command from a demo file
	virtual void			ProcessDemoCommand( idDemoFile *demo ) = 0;

	// pause and unpause the sound world
	virtual void			Pause( void ) = 0;
	virtual void			UnPause( void ) = 0;
	virtual bool			IsPaused( void ) = 0;

	virtual bool			IsMuted() const = 0;

	// Write the sound output to multiple wav files.  Note that this does not use the
	// work done by AsyncUpdate, it mixes explicitly in the foreground every PlaceOrigin(),
	// under the assumption that we are rendering out screenshots and the gameTime is going
	// much slower than real time.
	// path should not include an extension, and the generated filenames will be:
	// <path>_left.raw, <path>_right.raw, or <path>_51left.raw, <path>_51right.raw, 
	// <path>_51center.raw, <path>_51lfe.raw, <path>_51backleft.raw, <path>_51backright.raw, 
	// If only two channel mixing is enabled, the left and right .raw files will also be
	// combined into a stereo .wav file.
	virtual void			AVIOpen( const char* path, const char* name ) = 0;
	virtual void			AVIClose( void ) = 0;

	// SaveGame / demo Support
	virtual void			WriteToSaveGame( idFile* savefile ) = 0;
	virtual void			ReadFromSaveGame( idFile* savefile ) = 0;

	virtual void			BeginLevelLoad( void ) = 0;
};

#endif /* !__SOUNDWORLD_H__ */
