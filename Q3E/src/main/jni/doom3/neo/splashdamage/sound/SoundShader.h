// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SOUNDSHADER_H__
#define __SOUNDSHADER_H__

#include "../framework/declManager.h"

/*
===============================================================================

	Sound Shader Decl

===============================================================================
*/

class idSoundSample;

// sound shader flags
static const int	SSF_PRIVATE_SOUND =			BIT(0);		// only plays for the current listenerId
static const int	SSF_ANTI_PRIVATE_SOUND =	BIT(1);		// plays for everyone but the current listenerId
static const int	SSF_NO_OCCLUSION =			BIT(2);		// don't flow through portals, only use straight line
static const int	SSF_GLOBAL =				BIT(3);		// play full volume to all speakers and all listeners
static const int	SSF_OMNIDIRECTIONAL =		BIT(4);		// fall off with distance, but play same volume in all speakers
static const int	SSF_LOOPING =				BIT(5);		// repeat the sound continuously
static const int	SSF_PLAY_ONCE =				BIT(6);		// never restart if already playing on any channel of a given emitter
static const int	SSF_UNCLAMPED =				BIT(7);		// don't clamp calculated volumes at 1.0
static const int	SSF_NO_FLICKER =			BIT(8);		// always return 1.0 for volume queries
static const int	SSF_NO_DUPS =				BIT(9);		// try not to play the same sound twice in a row
static const int	SSF_RANDOMIZE =				BIT(10);	// randomly cycle the looped sample
static const int	SSF_OCCLUDE_ONCE =			BIT(11);	// only occlude once in its lifetime

// these options can be overriden from sound shader defaults on a per-emitter and per-channel basis
struct soundShaderParms_t {
	float					minDistance;
	float					maxDistance;
	float					farDistance;			// sound is only played when beyond this distance
	float					volume;					// in dB, unfortunately.  Negative values get quieter
	float					shakes;
	int						soundShaderFlags;		// SSF_* bit flags
	float					pitchShift;
	int						soundClass;				// for global fading of sounds
	int						soundArea;
};

const int		SOUND_MAX_LIST_WAVS		= 32;

// sound classes are used to fade most sounds down inside cinematics, leaving dialog
// flagged with a non-zero class full volume
const int		SOUND_MAX_CLASSES		= 4;

// it is somewhat tempting to make this a virtual class to hide the private
// details here, but that doesn't fit easily with the decl manager at the moment.
class idSoundShader : public idDecl {
public:
							idSoundShader( void );
	virtual					~idSoundShader( void );

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
	virtual void			List( void ) const;

	static void				CacheFromDict( const idDict& dict );

	virtual const char *	GetDescription() const;

	// so the editor can draw correct default sound spheres
	// this is currently defined as meters, which sucks, IMHO.
	virtual float			GetMinDistance() const;		// FIXME: replace this with a GetSoundShaderParms()
	virtual float			GetMaxDistance() const;

	bool					RebuildTextSource( void );

	bool					IsPrivateSound( void ) const { return( !!( parms.soundShaderFlags & SSF_PRIVATE_SOUND ) ); }
	bool					IsAntiPrivateSound( void ) const { return( !!( parms.soundShaderFlags & SSF_ANTI_PRIVATE_SOUND ) ); }
	bool					IsNoOcclusion( void ) const { return( !!( parms.soundShaderFlags & SSF_NO_OCCLUSION ) ); }
	bool					IsGlobal( void ) const { return( !!( parms.soundShaderFlags & SSF_GLOBAL ) ); }
	bool					IsOmnidirectional( void ) const { return( !!( parms.soundShaderFlags & SSF_OMNIDIRECTIONAL ) ); }
	bool					IsLooping( void ) const { return( !!( parms.soundShaderFlags & SSF_LOOPING ) ); }
	bool					IsPlayOnce( void ) const { return( !!( parms.soundShaderFlags & SSF_PLAY_ONCE ) ); }
	bool					IsOnDemand() const { return onDemand; }
	bool					IsLowPriority( void ) const { return lowPriority; }

	float					GetVolume( void ) const { return parms.volume; }
	float					GetShakes( void ) const { return parms.shakes; }
	virtual int				GetTimeLength( void ) const;	// This is virtual because the effects editor needs it
	//int						GetAudibleLength( void ) const;
	void					GetParms( soundShaderParms_t* out ) const { *out = parms; }
	idSoundSample*			GetEntry( int index ) const { return( entries[index] ); }

	int						GetSpeakerMask() const { return speakerMask; }
	float					GetLeadinVolume() const { return leadinVolume; }

	// returns NULL if an AltSound isn't defined in the shader.
	// we use this for pairing a specific broken light sound with a normal light sound
	virtual const idSoundShader *GetAltSound() const;

	virtual bool			HasDefaultSound() const;

	virtual const soundShaderParms_t *GetParms() const;
	virtual int				GetNumSounds() const;
	virtual const char *	GetSound( int index ) const;

	virtual bool			CheckShakesAndOgg( void ) const;

	virtual bool			IsOGGCompressed( void ) const;

private:
	friend class idSoundWorldLocal;
	friend class idSoundEmitterLocal;
	friend class idSoundChannel;
	friend class idSoundCache;

	// options from sound shader text
	soundShaderParms_t		parms;						// can be overriden on a per-channel basis

	bool					onDemand;					// only load when played, and free when finished
	bool					lowPriority;
	int						speakerMask;
	const idSoundShader*	altSound;
	idStr					desc;						// description
	bool					errorDuringParse;
	float					leadinVolume;				// allows light breaking leadin sounds to be much louder than the broken loop

	idSoundSample*			leadins[SOUND_MAX_LIST_WAVS];
	int						numLeadins;
	idSoundSample*			entries[SOUND_MAX_LIST_WAVS];
	int						numEntries;

	int						compressionMode;

private:
	void					Init( void );
	bool					ParseShader( idParser &src );
};

#endif /* !__SOUNDSHADER_H__ */
