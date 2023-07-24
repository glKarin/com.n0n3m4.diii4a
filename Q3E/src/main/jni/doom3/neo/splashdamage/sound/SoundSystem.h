// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SOUNDSYSTEM_H__
#define __SOUNDSYSTEM_H__

#include "../renderer/Cinematic.h"

/*
===============================================================================

	SOUND SYSTEM

===============================================================================
*/

struct soundDecoderInfo_t {
	idStr					name;
	idStr					format;
	int						numChannels;
	int						numSamplesPerSecond;
	int						num44kHzSamples;
	int						numBytes;
	bool					looping;
	float					lastVolume;
	int						elapsed44kHzTime;
	int						start44kHzTime;
	int						current44kHzTime;
};

class idSoundSystem {
public:
	virtual					~idSoundSystem( void ) {}

	// all non-hardware initialization
	virtual void			Init( void ) = 0;

	// shutdown routine
	virtual	void			Shutdown( void ) = 0;

	// call ClearBuffer if there is a chance that the AsyncUpdate won't get called
	// for 20+ msec, which would cause a stuttering repeat of the current
	// buffer contents
	virtual void			ClearBuffer( void ) = 0;

	// sound is attached to the window, and must be recreated when the window is changed
	virtual bool			InitHW( void ) = 0;
	virtual bool			ShutdownHW( void ) = 0;

	virtual sdLock&			GetLock() = 0;

	virtual int				AsyncUpdate( int time ) = 0;

	// it is a good idea to mute everything when starting a new level,
	// because sounds may be started before a valid listener origin
	// is specified
	virtual void			SetMute( bool mute ) = 0;

	virtual bool			IsMuted() const = 0;

	// for the sound level meter window
	virtual cinData_t		ImageForTime( const int milliseconds, const bool waveform ) = 0;

	// get sound decoder info
	virtual int				GetSoundDecoderInfo( int index, soundDecoderInfo_t& decoderInfo ) = 0;

	// if rw == NULL, no portal occlusion or rendered debugging is available
	virtual idSoundWorld*	AllocSoundWorld( idRenderWorld* rw ) = 0;

	virtual void			FreeSoundWorld( idSoundWorld* sw ) = 0;

	// specifying NULL will cause silence to be played
	virtual void			SetPlayingSoundWorld( idSoundWorld* soundWorld ) = 0;

	// some tools, like the sound dialog, may be used in both the game and the editor
	// This can return NULL, so check!
	virtual idSoundWorld *	GetPlayingSoundWorld( void ) = 0;

	// Mark all soundSamples as currently unused,
	// but don't free anything.
	virtual	void			BeginLevelLoad( void ) = 0;

	// Free all soundSamples marked as unused
	// We might want to defer the loading of new sounds to this point,
	// as we do with images, to avoid having a union in memory at one time.
	virtual	void			EndLevelLoad( void ) = 0;

	virtual void			StartCapture() = 0;
	virtual void			StopCapture() = 0;
	virtual int				GetCaptureRate( void ) const = 0;

	virtual bool			QuerySpeakers( int numSpeakers ) const = 0;

	virtual void			RefreshSoundDevices( void ) = 0; // Updates the list of sound devices available

	virtual const idWStrList*	ListSoundPlaybackDevices() const = 0;
	virtual const idWStrList*	ListSoundCaptureDevices() const = 0;
	virtual void				FreeDeviceList( const idWStrList* list ) const = 0;

	virtual int					GetAudioDeviceHash( const wchar_t* name ) const = 0;
	virtual int					GetAudioDeviceHash( const char* name ) const = 0;
};

extern idSoundSystem	*soundSystem;

#endif /* !__SOUNDSYSTEM_H__ */
