
#ifndef __SOUND__
#define __SOUND__

// Resolution of sound shakes in fps
#define SHAKE_FPS			30

/*
===============================================================================

	SOUND WORLD IDS

===============================================================================
*/

#define SOUNDWORLD_ANY		-1
#define SOUNDWORLD_NONE		0
#define SOUNDWORLD_GAME		1
#define SOUNDWORLD_MENU		2
#define SOUNDWORLD_EDITOR	3
#define SOUNDWORLD_MAX		4

/*
===============================================================================

	SOUND SHADER DECL

===============================================================================
*/

// RAVEN BEGIN
class rvCommonSample
{
public:
							rvCommonSample( void );
	virtual					~rvCommonSample( void ) {}

	virtual const byte		*GetSampleData( void ) const { return( NULL ); }
	virtual int				GetNumChannels( void ) const { return( 0 ); }
	virtual int				GetNumSamples( void ) const { return( 0 ); }
	virtual int				GetSampleRate( void ) const { return( 0 ); }
	virtual int				GetMemoryUsed( void ) const { return( 0 ); }
	virtual	int				GetDurationMS( void ) const { return( 0 ); }
	virtual	float			GetDuration( void ) const { return( 0.0f ); }
	virtual	void			Load( int langIndex = -1 ) {}
	virtual void			PurgeSoundSample( void ) {}
	virtual	bool			IsOgg( void ) const { return false; }		// is false for an expanded ogg
	virtual	void			Expand( bool force ) {}						// expand oggs to pcm

			void			SetReferencedThisLevel( void ) { levelLoadReferenced = true; }

	idStr					name;								// name of the sample file
	unsigned int 			timestamp;							// the most recent of all images used in creation, for reloadImages command

	bool					defaultSound;						// error during loading, now a beep
	bool					purged;
	bool					levelLoadReferenced;				// so we can tell which samples aren't needed any more
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	bool					referencedOutsideLevelLoad;
#endif
// RAVEN END
};
// RAVEN END

// sound shader flags
static const int	SSF_PRIVATE_SOUND =		BIT(0);		// only plays for the current listenerId
static const int	SSF_ANTI_PRIVATE_SOUND =BIT(1);		// plays for everyone but the current listenerId
static const int	SSF_NO_OCCLUSION =		BIT(2);		// don't flow through portals, only use straight line
static const int	SSF_GLOBAL =			BIT(3);		// play full volume to all speakers and all listeners
static const int	SSF_OMNIDIRECTIONAL =	BIT(4);		// fall off with distance, but play same volume in all speakers
static const int	SSF_LOOPING =			BIT(5);		// repeat the sound continuously
static const int	SSF_PLAY_ONCE =			BIT(6);		// never restart if already playing on any channel of a given emitter
static const int	SSF_UNCLAMPED =			BIT(7);		// don't clamp calculated volumes at 1.0
static const int	SSF_NO_FLICKER =		BIT(8);		// always return 1.0 for volume queries
static const int	SSF_NO_DUPS =			BIT(9);		// try not to play the same sound twice in a row
// RAVEN BEGIN
static const int	SSF_USEDOPPLER =		BIT(10);	// allow doppler pitch shifting effects
static const int	SSF_NO_RANDOMSTART =	BIT(11);	// don't offset the start position for looping sounds
static const int	SSF_VO_FOR_PLAYER =		BIT(12);	// Notifies a funcRadioChatter that this shader is directed at the player
static const int	SSF_IS_VO =				BIT(13);	// this sound is VO
static const int	SSF_CAUSE_RUMBLE =		BIT(14);	// causes joystick rumble
static const int	SSF_CENTER =			BIT(15);	// sound through center channel only
static const int	SSF_HILITE =			BIT(16);	// display debug info for this emitter
// RAVEN END

// these options can be overriden from sound shader defaults on a per-emitter and per-channel basis
typedef struct {
	float					minDistance;
	float					maxDistance;
	float					volume;
	float					attenuatedVolume;
	float					shakes;
	int						soundShaderFlags;		// SSF_* bit flags
	int						soundClass;				// for global fading of sounds
// RAVEN BEGIN
// bdube: frequency shift
	float					frequencyShift;
	float					wetLevel;
	float					dryLevel;
// RAVEN END	
} soundShaderParms_t;

const int		SOUND_MAX_LIST_WAVS		= 32;

// sound classes are used to fade most sounds down inside cinematics, leaving dialog
// flagged with a non-zero class full volume
const int		SOUND_CLASS_MUSICAL		= 3;
const int		SOUND_MAX_CLASSES		= 4;

// it is somewhat tempting to make this a virtual class to hide the private
// details here, but that doesn't fit easily with the decl manager at the moment.
// RAVEN BEGIN
// jsinger: added to support serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
class idSoundShader : public idDecl, public Serializable<'ISS '> {
public:
	virtual void			Write( SerialOutputStream &stream ) const;
	virtual void			AddReferences() const;
							idSoundShader( SerialInputStream &stream );
#else
class idSoundShader : public idDecl {
#endif
public:
							idSoundShader( void ) { Init(); }
	virtual					~idSoundShader( void ) {}

	virtual size_t			Size( void ) const;
	virtual bool			SetDefaultText( void );
	virtual const char *	DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength, bool noCaching );
	virtual void			FreeData( void );
// RAVEN BEGIN
	virtual	void			SetReferencedThisLevel( void );
// RAVEN END
	virtual void			List( void ) const;

	virtual const char *	GetDescription( void ) const { return( desc ); }

	// so the editor can draw correct default sound spheres
	// this is currently defined as meters, which sucks, IMHO.
	virtual float			GetMinDistance( void ) const { return( parms.minDistance ); }
	virtual float			GetMaxDistance( void ) const { return( parms.maxDistance ); }

// RAVEN BEGIN
// scork: for detailed error-reporting
	virtual bool			Validate( const char *psText, int iTextLength, idStr &strReportTo ) const;

// jscott: implemented id's code
	virtual	bool			RebuildTextSource( void );

// jscott: required access functions
			bool			IsPrivateSound( void ) const { return( !!( parms.soundShaderFlags & SSF_PRIVATE_SOUND ) ); }
			bool			IsAntiPrivateSound( void ) const { return( !!( parms.soundShaderFlags & SSF_ANTI_PRIVATE_SOUND ) ); }
			bool			IsNoOcclusion( void ) const { return( !!( parms.soundShaderFlags & SSF_NO_OCCLUSION ) ); }
			bool			IsGlobal( void ) const { return( !!( parms.soundShaderFlags & SSF_GLOBAL ) ); }
			bool			IsOmnidirectional( void ) const { return( !!( parms.soundShaderFlags & SSF_OMNIDIRECTIONAL ) ); }
			bool			IsLooping( void ) const { return( !!( parms.soundShaderFlags & SSF_LOOPING ) ); }
			bool			IsPlayOnce( void ) const { return( !!( parms.soundShaderFlags & SSF_PLAY_ONCE ) ); }
			bool			IsUnclamped( void ) const { return( !!( parms.soundShaderFlags & SSF_UNCLAMPED ) ); }
			bool			IsNoFlicker( void ) const { return( !!( parms.soundShaderFlags & SSF_NO_FLICKER ) ); }
			bool			IsNoDupes( void ) const { return( !!( parms.soundShaderFlags & SSF_NO_DUPS ) ); }
			bool			IsDoppler( void ) const { return( !!( parms.soundShaderFlags & SSF_USEDOPPLER ) ); }
			bool			IsNoRandomStart( void ) const { return( !!( parms.soundShaderFlags & SSF_NO_RANDOMSTART ) ); }
			bool			IsVO_ForPlayer( void ) const { return( !!( parms.soundShaderFlags & SSF_VO_FOR_PLAYER ) ); }
			bool			IsVO( void ) const { return( !!( parms.soundShaderFlags & SSF_IS_VO ) ); }
			bool			IsCauseRumble( void ) const { return( !!( parms.soundShaderFlags & SSF_CAUSE_RUMBLE ) ); }
			bool			IsCenter( void ) const { return( !!( parms.soundShaderFlags & SSF_CENTER ) ); }

			float			GetVolume( void ) const { return( parms.volume ); }
			float			GetShakes( void ) const { return( parms.shakes ); }
			float			GetTimeLength( void ) const;
			void			GetParms( soundShaderParms_t *out ) const { *out = parms; }
			void			SetNoShakes( bool in ) { noShakes = in; }
			bool			GetNoShakes( void ) const { return( noShakes ); }

			void			IncPlayCount( void ) { playCount++; }
			int				GetPlayCount( void ) const { return( playCount ); }
// RAVEN END

			float			GetLeadinVolume( void ) const { return( leadinVolume ); }
			int				GetNumEntries( void ) const { return( numEntries ); }
			rvCommonSample	*GetLeadin( int index ) const { return( leadins[index] ); }
			rvCommonSample	*GetEntry( int index ) const { return( entries[index] ); }
			const char		*GetShakeData( int index ) const;
			void			SetShakeData( int index, const char *ampData );
			void			Purge( bool freeBaseBlocks );
			void			LoadSampleData( int langIndex = -1 );

			const char *	GetSampleName( int index ) const;
			int				GetSamplesPerSec( int index ) const;
			int				GetNumChannels( int index ) const;
			int				GetNumSamples( int index ) const;
			int				GetMemorySize( int index ) const;
			const byte *	GetNonCacheData( int index ) const;

			void			ExpandSmallOggs( bool force );
// RAVEN END

	// returns NULL if an AltSound isn't defined in the shader.
	// we use this for pairing a specific broken light sound with a normal light sound
	virtual const idSoundShader *GetAltSound( void ) const { return( altSound ); }

	virtual bool			HasDefaultSound( void ) const;

	virtual const soundShaderParms_t *GetParms( void ) const { return( &parms ); }
	virtual int				GetNumSounds( void ) const;
	virtual const char *	GetSound( int index ) const;

private:
	friend class idSoundEmitterLocal;
	friend class idSoundChannel;
	friend class idSoundCache;

	// options from sound shader text
	soundShaderParms_t		parms;						// can be overriden on a per-channel basis

	const idSoundShader *	altSound;
	idStr					desc;						// description
	bool					errorDuringParse;
// RAVEN BEGIN
	bool					noShakes;					// Don't generate shake data
	bool					frequentlyUsed;				// Expand this to pcm data no matter how long it is
// RAVEN END
	float					leadinVolume;				// allows light breaking leadin sounds to be much louder than the broken loop

// RAVEN BEGIN
	rvCommonSample *		leadins[SOUND_MAX_LIST_WAVS];
// RAVEN END
	int						numLeadins;
// RAVEN BEGIN
	rvCommonSample *		entries[SOUND_MAX_LIST_WAVS];
	idStrList				shakes;
// RAVEN END
	int						numEntries;

// RAVEN BEGIN
// bdube: frequency shift code from splash
	float					minFrequencyShift;	
	float					maxFrequencyShift;

	int						playCount;					// For profiling
// RAVEN END

private:
	void					Init( void );
	bool					ParseShader( idLexer &src );
};

class rvSoundShaderEdit
{
public:
	virtual ~rvSoundShaderEdit() {}
	virtual	const char *	GetSampleName( const idSoundShader *sound, int index ) const = 0;
	virtual	int				GetSamplesPerSec( const idSoundShader *sound, int index ) const = 0;
	virtual	int				GetNumChannels( const idSoundShader *sound, int index ) const = 0;
	virtual	int				GetNumSamples( const idSoundShader *sound, int index ) const = 0;
	virtual	int				GetMemorySize( const idSoundShader *sound, int index ) const = 0;
	virtual	const byte *	GetNonCacheData( const idSoundShader *sound, int index ) const = 0;
	virtual void			LoadSampleData( idSoundShader *sound, int langIndex = -1 ) = 0;
	virtual	void			ExpandSmallOggs( idSoundShader *sound, bool force ) = 0;
	virtual	const char		*GetShakeData( idSoundShader *sound, int index ) = 0;
	virtual	void			SetShakeData( idSoundShader *sound, int index, const char *ampData ) = 0;
	virtual	void			Purge( idSoundShader *sound, bool freeBaseBlocks ) = 0;
};

extern rvSoundShaderEdit	*soundShaderEdit;

/*
===============================================================================

	SOUND EMITTER

===============================================================================
*/

// sound channels
static const int SCHANNEL_ANY = 0;	// used in queries and commands to effect every channel at once, in
									// startSound to have it not override any other channel
static const int SCHANNEL_ONE = 1;	// any following integer can be used as a channel number
typedef int s_channelType;	// the game uses its own series of enums, and we don't want to require casts


class idSoundEmitter {
public:
	virtual					~idSoundEmitter( void ) {}

	// the parms specified will be the default overrides for all sounds started on this emitter.
	// NULL is acceptable for parms
	virtual void			UpdateEmitter( const idVec3 &origin, const idVec3 &velocity, int listenerId, const soundShaderParms_t *parms ) = 0;

	// returns the length of the started sound in msec
	virtual int				StartSound( const idSoundShader *shader, const s_channelType channel, float diversity = 0.0f, int shaderFlags = 0 ) = 0;

	// pass SCHANNEL_ANY to effect all channels
	virtual void			ModifySound( const s_channelType channel, const soundShaderParms_t *parms ) = 0;
	virtual void			StopSound( const s_channelType channel ) = 0;
	// to is in Db (sigh), over is in seconds
	virtual void			FadeSound( const s_channelType channel, float to, float over ) = 0;

	// returns true if there are any sounds playing from this emitter.  There is some conservative
	// slop at the end to remove inconsistent race conditions with the sound thread updates.
	// FIXME: network game: on a dedicated server, this will always be false
	virtual bool			CurrentlyPlaying( const s_channelType channel = SCHANNEL_ANY ) const = 0;

	// returns a 0.0 to 1.0 value based on the current sound amplitude, allowing
	// graphic effects to be modified in time with the audio.
	// just samples the raw wav file, it doesn't account for volume overrides in the
	virtual	float			CurrentAmplitude( int channelFlags = -1, bool factorDistance = false ) = 0;

	// Returns true if the emitter is in the passed in world
	virtual bool			AttachedToWorld( int id ) const = 0;

	// for save games.  Index will always be > 0
	virtual	int				Handle( void ) const = 0;
};

/*
===============================================================================

	SOUND SYSTEM

===============================================================================
*/

typedef struct {
	idStr					name;
	idStr					format;
	int						numChannels;
	int						numSamplesPerSecond;
	int						num44kHzSamples;
	int						numBytes;
	bool					looping;
	float					lastVolume;
	int						start44kHzTime;
	int						current44kHzTime;
} soundDecoderInfo_t;

typedef struct soundPortalTrace_s {
	int		portalArea;
	const struct soundPortalTrace_s	*prevStack;
} soundPortalTrace_t;

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
	virtual void			InitVoiceComms( void ) = 0;
	virtual bool			ShutdownHW( void ) = 0;
	virtual void			ShutdownVoiceComms( void ) = 0;

	// Called once per common frame to check on changes to the sound system
	virtual void			Frame( void ) = 0;
	// Service the sound system
	virtual void			ForegroundUpdate( void ) = 0;

	// asyn loop, called at 60Hz
	virtual int				AsyncUpdate( int time ) = 0;

	// direct mixing for OSes that support it
	virtual int				AsyncMix( int soundTime, float *mixBuffer ) = 0;

	// async loop, when the sound driver uses a write strategy
	virtual int				AsyncUpdateWrite( int time ) = 0;

	// it is a good idea to mute everything when starting a new level,
	// because sounds may be started before a valid listener origin
	// is specified
	virtual void			SetMute( bool mute ) = 0;

	// for the sound level meter window
	virtual cinData_t		ImageForTime( const int milliseconds, const bool waveform ) = 0;

	// get sound decoder info
	virtual int				GetSoundDecoderInfo( int index, soundDecoderInfo_t &decoderInfo ) = 0;

	// Mark all soundSamples as currently unused,
	// but don't free anything.
	virtual	void			BeginLevelLoad( const char *mapName ) = 0;

	// Free all soundSamples marked as unused
	// We might want to defer the loading of new sounds to this point,
	// as we do with images, to avoid having a union in memory at one time.
	virtual	void			EndLevelLoad( const char *mapName ) = 0;

// RAVEN BEGIN
// jnewquist: Free all sounds
#ifdef _XENON
	virtual void			FlushLevelSoundSamples( void ) = 0;
#endif
// RAVEN END

	// Frees the empty base blocks in the appropriate soundCache
	virtual void			CleanCache( void ) = 0;

	// prints memory info
	virtual void			PrintMemInfo( MemInfo *mi ) = 0;
#ifdef _USE_OPENAL
	// is EAX support present - -1: disabled at compile time. 0: no suitable hardwre 1: ok
	virtual int				IsEAXAvailable( void ) = 0;
	virtual const char *	GetDeviceName( int index ) = 0;
	virtual const char *	GetDefaultDeviceName( void ) = 0;
#endif

	// SoundWorld stuff

	// call at each map start
	virtual void			SetRenderWorld( idRenderWorld *rw ) = 0;
	virtual void			StopAllSounds( int worldId ) = 0;

// RAVEN BEGIN
	// dissociate all virtual channels from hardware
	virtual	void			DisableAllSounds( void ) = 0;

	// get a new emitter that can play sounds in this world
    virtual int				AllocSoundEmitter( int worldId ) = 0;
	virtual void			FreeSoundEmitter( int worldId, int handle, bool immediate ) = 0;
// RAVEN END

	// for load games, index 0 will return NULL
	virtual idSoundEmitter *EmitterForIndex( int worldId, int index ) = 0;
	virtual int				GetNumEmitters( void ) const = 0;

	// query sound samples from all emitters reaching a given position
	virtual	float			CurrentShakeAmplitudeForPosition( int worldId, const int time, const idVec3 &listenerPosition ) = 0;

	// where is the camera/microphone
	// listenerId allows listener-private and antiPrivate sounds to be filtered
	// gameTime is in msec, and is used to time sound queries and removals so that they are independent
	// of any race conditions with the async update
	virtual	void			PlaceListener( const idVec3 &origin, const idMat3 &axis, const int listenerId, const int gameTime, const idStr &areaName ) = 0;

	// reset the listener portal to invalid during level transitions
	virtual void			ResetListener( void ) = 0;

	// fade all sounds in the world with a given shader soundClass
	// to is in Db (sigh), over is in seconds
	virtual void			FadeSoundClasses( int worldId, const int soundClass, float to, const float over ) = 0;

	// background music
	virtual	void			PlayShaderDirectly( int worldId, const char *name, int channel = -1 ) = 0;

	// dumps the current state and begins archiving commands
	virtual void			StartWritingDemo( int worldId, idDemoFile *demo ) = 0;
	virtual void			StopWritingDemo( int worldId ) = 0;

	// read a sound command from a demo file
	virtual void			ProcessDemoCommand( int worldId, idDemoFile *demo ) = 0;

	virtual int				GetHardwareTime( void ) const = 0;

	// unpauses the selected soundworld, pauses all others
	virtual int				SetActiveSoundWorld( bool on ) = 0;
// RAVEN BEGIN
// jnewquist: Accessor for active sound world
	virtual int				GetActiveSoundWorld( void ) = 0;
// RAVEN END

	// Write the sound output to multiple wav files.  Note that this does not use the
	// work done by AsyncUpdate, it mixes explicitly in the foreground every PlaceOrigin(),
	// under the assumption that we are rendering out screenshots and the gameTime is going
	// much slower than real time.
	// path should not include an extension, and the generated filenames will be:
	// <path>_left.raw, <path>_right.raw, or <path>_51left.raw, <path>_51right.raw, 
	// <path>_51center.raw, <path>_51lfe.raw, <path>_51backleft.raw, <path>_51backright.raw, 
	// If only two channel mixing is enabled, the left and right .raw files will also be
	// combined into a stereo .wav file.
	virtual void			AVIOpen( int worldId, const char *path, const char *name ) = 0;
	virtual void			AVIClose( int worldId ) = 0;

	// SaveGame / demo Support
	virtual void			WriteToSaveGame( int worldId, idFile *savefile ) = 0;
	virtual void			ReadFromSaveGame( int worldId, idFile *savefile ) = 0;

// RAVEN BEGIN
// rjohnson: added list active sounds
	virtual void			ListActiveSounds( int worldId ) = 0;
// RAVEN END
	// End SoundWorld stuff

// RAVEN BEGIN
// jscott: added
	virtual size_t			ListSoundSummary( void ) = 0;

	virtual bool			HasCache( void ) const = 0;
	virtual rvCommonSample	*FindSample( const idStr &filename ) = 0;
	virtual	int				SamplesToMilliseconds( int samples ) const = 0;
	virtual void *			AllocSoundSample( int size ) = 0;
	virtual void			FreeSoundSample( const byte *address ) = 0;

	virtual bool			GetInsideLevelLoad( void ) const = 0;
	virtual	bool			ValidateSoundShader( idSoundShader *shader ) = 0;

// jscott: voice comm support
	virtual	bool			EnableRecording( bool enable, bool test, float &micLevel ) = 0;
	virtual int				GetVoiceData( byte *buffer, int maxSize ) = 0;
	virtual void			PlayVoiceData( int clientNum, const byte *buffer, int bytes ) = 0;
	virtual void			BufferVoiceData( void ) = 0;
	virtual void			MixVoiceData( float *finalMixBuffer, int numSpeakers, int newTime ) = 0;
// ddynerman: voice comm utility
	virtual	int				GetCommClientNum( int channel ) const = 0;
	virtual int				GetNumVoiceChannels( void ) const = 0;

// jscott: reverb editor support
	virtual	const char		*GetReverbName( int reverb ) = 0;
	virtual	int				GetNumAreas( void ) = 0;
	virtual	int				GetReverb( int area ) = 0;
	virtual	bool			SetReverb( int area, const char *reverbName, const char *fileName ) = 0;
	
	virtual void			EndCinematic() = 0;
	
// RAVEN END
};

extern idSoundSystem	*soundSystem;

void S_InitSoundSystem( void );

#endif /* !__SOUND__ */
