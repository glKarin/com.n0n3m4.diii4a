// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SOUNDEMITTER_H__
#define __SOUNDEMITTER_H__

/*
===============================================================================

	SOUND EMITTER

===============================================================================
*/

class idSoundShader;
struct soundShaderParms_t;

#include "../renderer/Cinematic.h"

// sound channels
static const int SCHANNEL_ANY = 0;	// used in queries and commands to effect every channel at once, in
									// startSound to have it not override any other channel
static const int SCHANNEL_ONE = 1;	// any following integer can be used as a channel number

typedef int soundChannel_t;			// the game uses its own series of enums, and we don't want to require casts

class idSoundEmitter {
public:
	virtual								~idSoundEmitter( void ) {}

	// for save games.  Index will always be > 0
	virtual	int							Index( void ) const = 0;

	// a non-immediate free will let all currently playing sounds complete
	// soundEmitters are not actually deleted, they are just marked as
	// reusable by the soundWorld
	virtual void						Free( bool immediate ) = 0;

	// the parms specified will be the default overrides for all sounds started on this emitter.
	// NULL is acceptable for parms
	virtual void						UpdateEmitter( const idVec3 &origin, int listenerId, const soundShaderParms_t *parms ) = 0;

	// returns the length of the started sound in msec
	virtual int							StartSound( const idSoundShader* shader, const soundChannel_t channelStart, soundChannel_t channelEnd, float diversity = 0, int shaderFlags = 0 ) = 0;

	// pass SCHANNEL_ANY to effect all channels
	virtual const soundShaderParms_t&	GetChannelParms( const soundChannel_t channel ) = 0;
	virtual void						ModifySound( const soundChannel_t channel, const soundShaderParms_t& parms ) = 0;
	virtual void						StopSound( const soundChannel_t channel ) = 0;
	virtual void						FadeSound( const soundChannel_t channel, float to, float over ) = 0;

	// returns true if there are any sounds playing from this emitter.  There is some conservative
	// slop at the end to remove inconsistent race conditions with the sound thread updates.
	// FIXME: network game: on a dedicated server, this will always be false
	virtual bool						CurrentlyPlaying( void ) const = 0;

	// returns a 0.0 to 1.0 value based on the current sound amplitude, allowing
	// graphic effects to be modified in time with the audio.
	// just samples the raw wav file, it doesn't account for volume overrides in the
	virtual	float						CurrentAmplitude( void ) = 0;

	virtual cinData_t					ImageForTime( const int milliseconds ) = 0;

	virtual void						SetChannelOffset( const soundChannel_t channel, int ms ) = 0;
};

#endif /* !__SOUNDEMITTER_H__ */
