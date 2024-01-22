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

#ifndef __SOUND__
#define __SOUND__

#ifdef _RAVEN
#define SOUNDWORLD_ANY		-1
#define SOUNDWORLD_NONE		0
#define SOUNDWORLD_GAME		1
#define SOUNDWORLD_MENU		2
#define SOUNDWORLD_EDITOR	3
#define SOUNDWORLD_MAX		4

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
#endif
/*
===============================================================================

	SOUND SHADER DECL

===============================================================================
*/

// unfortunately, our minDistance / maxDistance is specified in meters, and
// we have far too many of them to change at this time.
const float DOOM_TO_METERS = 0.0254f;					// doom to meters
const float METERS_TO_DOOM = (1.0f/DOOM_TO_METERS);	// meters to doom

class idSoundSample;

// sound shader flags
static const int	SSF_PRIVATE_SOUND =		BIT(0);	// only plays for the current listenerId
static const int	SSF_ANTI_PRIVATE_SOUND =BIT(1);	// plays for everyone but the current listenerId
static const int	SSF_NO_OCCLUSION =		BIT(2);	// don't flow through portals, only use straight line
static const int	SSF_GLOBAL =			BIT(3);	// play full volume to all speakers and all listeners
static const int	SSF_OMNIDIRECTIONAL =	BIT(4);	// fall off with distance, but play same volume in all speakers
static const int	SSF_LOOPING =			BIT(5);	// repeat the sound continuously
static const int	SSF_PLAY_ONCE =			BIT(6);	// never restart if already playing on any channel of a given emitter
static const int	SSF_UNCLAMPED =			BIT(7);	// don't clamp calculated volumes at 1.0
static const int	SSF_NO_FLICKER =		BIT(8);	// always return 1.0 for volume queries
static const int	SSF_NO_DUPS =			BIT(9);	// try not to play the same sound twice in a row

#ifdef _RAVEN
// RAVEN BEGIN
static const int        SSF_USEDOPPLER = BIT(10);       // allow doppler pitch shifting effects
static const int        SSF_NO_RANDOMSTART = BIT(11);   // don't offset the start position for looping sounds
static const int        SSF_VO_FOR_PLAYER = BIT(12);    // Notifies a funcRadioChatter that this shader is directed at the player
static const int        SSF_IS_VO = BIT(13);    // this sound is VO
static const int        SSF_CAUSE_RUMBLE = BIT(14);     // causes joystick rumble
static const int        SSF_CENTER = BIT(15);   // sound through center channel only
static const int        SSF_HILITE = BIT(16);   // display debug info for this emitter
// RAVEN END
#endif
#ifdef _HUMANHEAD
static const int	SSF_VOICEAMPLITUDE =	BIT(10);// HUMANHEAD pdm: include in findamplitude queries
static const int	SSF_OMNI_WHEN_CLOSE =	BIT(11);// HUMANHEAD pdm: make omni when within the mindistance
static const int	SSF_NOREVERB =			BIT(12);// HUMANHEAD pdm: reverb exclusion

//HUMANHEAD rww - subtitle functionality
typedef struct soundSubtitle_s {
	idStr			subText;
	float			subTime;
	int				subChannel;
} soundSub_t;

typedef struct soundSubtitleList_s {
	idStr				soundName;
	idList<soundSub_t>	subList;
} soundSubtitleList_t;
//HUMANHEAD END
#endif

// these options can be overriden from sound shader defaults on a per-emitter and per-channel basis
typedef struct {
	float					minDistance;
	float					maxDistance;
	float					volume;					// in dB, unfortunately.  Negative values get quieter
	float					shakes;
	int						soundShaderFlags;		// SSF_* bit flags
	int						soundClass;				// for global fading of sounds
													
#ifdef _RAVEN
	// RAVEN BEGIN
// bdube: frequency shift
	float					frequencyShift;
	float					wetLevel;
	float					dryLevel;
// RAVEN END
#endif

#ifdef _HUMANHEAD
	int						subIndex;               //HUMANHEAD rww - indices into the subtitle table
	int						profanityIndex;			// HUMANHEAD pdm
	float					profanityDelay;			// HUMANHEAD pdm
	float					profanityDuration;		// HUMANHEAD pdm
#endif
} soundShaderParms_t;

#ifdef _HUMANHEAD
//HUMANHEAD: aob
class hhSoundShaderParmsModifier {
public:
	hhSoundShaderParmsModifier() {
		memset(&parms, 0, sizeof(soundShaderParms_t));
        parms.subIndex = -1;
		minDistanceIsSet = false;
		maxDistanceIsSet = false;
		volumeIsSet = false;
		shakesIsSet = false;
		soundShaderFlagsIsSet = false;
	}


	void				ModifyParms(soundShaderParms_t& parmsToModify) const {
		if (MinDistanceIsSet()) {
			parmsToModify.minDistance = parms.minDistance;
		}

		if (MaxDistanceIsSet()) {
			parmsToModify.maxDistance = parms.maxDistance;
		}

		if (VolumeIsSet()) {
			parmsToModify.volume = parms.volume;
		}

		if (ShakesIsSet()) {
			parmsToModify.shakes = parms.shakes;
		}

		if (SoundShaderFlagsIsSet()) {
			parmsToModify.soundShaderFlags = parms.soundShaderFlags;
		}
	}


	void				SetMinDistance(const float minDistance) { parms.minDistance = minDistance; minDistanceIsSet = true; }
	void				SetMaxDistance(const float maxDistance) { parms.maxDistance = maxDistance; maxDistanceIsSet = true; }
	void				SetVolume(const float volume) { parms.volume = volume; volumeIsSet = true; }
	void				SetShakes(const float shakes) { parms.shakes = shakes; shakesIsSet = true; }
	void				SetSoundShaderFlags(const int soundShaderFlags) { parms.soundShaderFlags = soundShaderFlags; soundShaderFlagsIsSet = true; }

	bool				MinDistanceIsSet() const { return minDistanceIsSet; }
	bool				MaxDistanceIsSet() const { return maxDistanceIsSet; }
	bool				VolumeIsSet() const { return volumeIsSet; }
	bool				ShakesIsSet() const { return shakesIsSet; }
	bool				SoundShaderFlagsIsSet() const { return soundShaderFlagsIsSet; }

protected:
	soundShaderParms_t	parms;
	bool				minDistanceIsSet;
	bool				maxDistanceIsSet;
	bool				volumeIsSet;
	bool				shakesIsSet;
	bool				soundShaderFlagsIsSet;

};
//HUMANHEAD END
#endif


const int		SOUND_MAX_LIST_WAVS		= 32;

#ifdef _HUMANHEAD
// HUMANHEAD pdm: sound classes
const int		SOUNDCLASS_NORMAL		= 0;
const int		SOUNDCLASS_VOICEDUCKER	= 1;
const int		SOUNDCLASS_SPIRITWALK	= 2;
const int		SOUNDCLASS_VOICE		= 3;
const int		SOUNDCLASS_MUSIC		= 4;
// HUMANHEAD END
#endif

// sound classes are used to fade most sounds down inside cinematics, leaving dialog
// flagged with a non-zero class full volume
#ifdef _HUMANHEAD
const int		SOUND_MAX_CLASSES		= 5;
#else
const int		SOUND_MAX_CLASSES		= 4;
#endif

#ifdef _RAVEN
const int		SOUND_CLASS_MUSICAL		= 3;
#endif

// it is somewhat tempting to make this a virtual class to hide the private
// details here, but that doesn't fit easily with the decl manager at the moment.
class idSoundShader : public idDecl
{
	public:
		idSoundShader(void);
		virtual					~idSoundShader(void);

		virtual size_t			Size(void) const;
		virtual bool			SetDefaultText(void);
		virtual const char 	*DefaultDefinition(void) const;
#ifdef _RAVEN
		virtual bool			Parse(const char *text, const int textLength, bool noCaching = false);
#else
		virtual bool			Parse(const char *text, const int textLength);
#endif
		virtual void			FreeData(void);
		virtual void			List(void) const;

		virtual const char 	*GetDescription() const;

		// so the editor can draw correct default sound spheres
		// this is currently defined as meters, which sucks, IMHO.
		virtual float			GetMinDistance() const;		// FIXME: replace this with a GetSoundShaderParms()
		virtual float			GetMaxDistance() const;

		// returns NULL if an AltSound isn't defined in the shader.
		// we use this for pairing a specific broken light sound with a normal light sound
		virtual const idSoundShader *GetAltSound() const;

		virtual bool			HasDefaultSound() const;

		virtual const soundShaderParms_t *GetParms() const;
		virtual int				GetNumSounds() const;
		virtual const char 	*GetSound(int index) const;

		virtual bool			CheckShakesAndOgg(void) const;
#ifdef _RAVEN
// RAVEN BEGIN
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
			void			GetParms( soundShaderParms_t *out ) const { *out = parms; }
			void			SetNoShakes( bool in ) { noShakes = in; }
			bool			GetNoShakes( void ) const { return( noShakes ); }

			void			IncPlayCount( void ) { playCount++; }
			int				GetPlayCount( void ) const { return( playCount ); }
// RAVEN END
#endif

#ifdef _HUMANHEAD
	float					GetVolume(void) const { return parms.volume; }
#endif

	private:
		friend class idSoundWorldLocal;
		friend class idSoundEmitterLocal;
		friend class idSoundChannel;
		friend class idSoundCache;

		// options from sound shader text
		soundShaderParms_t		parms;						// can be overriden on a per-channel basis

		bool					onDemand;					// only load when played, and free when finished
		int						speakerMask;
		const idSoundShader 	*altSound;
		idStr					desc;						// description
		bool					errorDuringParse;
		float					leadinVolume;				// allows light breaking leadin sounds to be much louder than the broken loop

		idSoundSample 	*leadins[SOUND_MAX_LIST_WAVS];
		int						numLeadins;
		idSoundSample 	*entries[SOUND_MAX_LIST_WAVS];
		int						numEntries;
#ifdef _RAVEN
// RAVEN BEGIN
	bool					noShakes;					// Don't generate shake data
	bool					frequentlyUsed;				// Expand this to pcm data no matter how long it is
// RAVEN END
// RAVEN BEGIN
// bdube: frequency shift code from splash
	float					minFrequencyShift;
	float					maxFrequencyShift;

	int						playCount;					// For profiling
// RAVEN END
#endif

	private:
		void					Init(void);
		bool					ParseShader(idLexer &src);
};

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


class idSoundEmitter
{
	public:
		virtual					~idSoundEmitter(void) {}

		// a non-immediate free will let all currently playing sounds complete
		// soundEmitters are not actually deleted, they are just marked as
		// reusable by the soundWorld
		virtual void			Free(bool immediate) = 0;

		// the parms specified will be the default overrides for all sounds started on this emitter.
		// NULL is acceptable for parms
		virtual void			UpdateEmitter(const idVec3 &origin, int listenerId, const soundShaderParms_t *parms) = 0;
#ifdef _RAVEN
// jmarshall - todo add velocity
	virtual void			UpdateEmitter(const idVec3& origin, const idVec3& velocity, int listenerId, const soundShaderParms_t* parms) = 0;
// jmarshalll end
#endif

		// returns the length of the started sound in msec
		virtual int				StartSound(const idSoundShader *shader, const s_channelType channel, float diversity = 0, int shaderFlags = 0, bool allowSlow = true) = 0;

		// pass SCHANNEL_ANY to effect all channels
		virtual void			ModifySound(const s_channelType channel, const soundShaderParms_t *parms) = 0;
		virtual void			StopSound(const s_channelType channel) = 0;
		// to is in Db (sigh), over is in seconds
		virtual void			FadeSound(const s_channelType channel, float to, float over) = 0;

		// returns true if there are any sounds playing from this emitter.  There is some conservative
		// slop at the end to remove inconsistent race conditions with the sound thread updates.
		// FIXME: network game: on a dedicated server, this will always be false
		virtual bool			CurrentlyPlaying(void) const = 0;

		// returns a 0.0 to 1.0 value based on the current sound amplitude, allowing
		// graphic effects to be modified in time with the audio.
		// just samples the raw wav file, it doesn't account for volume overrides in the
		virtual	float			CurrentAmplitude(void) = 0;

		// for save games.  Index will always be > 0
		virtual	int				Index(void) const = 0;

#ifdef _HUMANHEAD
	//HUMANHEAD: aob
	virtual void		ModifySound(idSoundShader* shader, const s_channelType channel, const hhSoundShaderParmsModifier& parmModifier) = 0;
	virtual soundShaderParms_t* GetSoundParms(idSoundShader* shader, const s_channelType channel) = 0;
	virtual float			CurrentAmplitude( const s_channelType channel ) = 0;
	virtual float			CurrentVoiceAmplitude( const s_channelType channel ) = 0;
	//HUMANHEAD END
#endif
};

/*
===============================================================================

	SOUND WORLD

There can be multiple independent sound worlds, just as there can be multiple
independent render worlds.  The prime example is the editor sound preview
option existing simultaniously with a live game.
===============================================================================
*/

class idSoundWorld
{
	public:
		virtual					~idSoundWorld(void) {}

		// call at each map start
		virtual void			ClearAllSoundEmitters(void) = 0;
		virtual void			StopAllSounds(void) = 0;

		// get a new emitter that can play sounds in this world
		virtual idSoundEmitter *AllocSoundEmitter(void) = 0;

		// for load games, index 0 will return NULL
		virtual idSoundEmitter *EmitterForIndex(int index) = 0;

		// query sound samples from all emitters reaching a given position
		virtual	float			CurrentShakeAmplitudeForPosition(const int time, const idVec3 &listenerPosition) = 0;

		// where is the camera/microphone
		// listenerId allows listener-private and antiPrivate sounds to be filtered
		// gameTime is in msec, and is used to time sound queries and removals so that they are independent
		// of any race conditions with the async update
		virtual	void			PlaceListener(const idVec3 &origin, const idMat3 &axis, const int listenerId, const int gameTime, const idStr &areaName) = 0;

		// fade all sounds in the world with a given shader soundClass
		// to is in Db (sigh), over is in seconds
		virtual void			FadeSoundClasses(const int soundClass, const float to, const float over) = 0;

		// background music
		virtual	void			PlayShaderDirectly(const char *name, int channel = -1) = 0;

		// dumps the current state and begins archiving commands
		virtual void			StartWritingDemo(idDemoFile *demo) = 0;
		virtual void			StopWritingDemo() = 0;

		// read a sound command from a demo file
		virtual void			ProcessDemoCommand(idDemoFile *demo) = 0;

		// pause and unpause the sound world
		virtual void			Pause(void) = 0;
		virtual void			UnPause(void) = 0;
		virtual bool			IsPaused(void) = 0;

		// Write the sound output to multiple wav files.  Note that this does not use the
		// work done by AsyncUpdate, it mixes explicitly in the foreground every PlaceOrigin(),
		// under the assumption that we are rendering out screenshots and the gameTime is going
		// much slower than real time.
		// path should not include an extension, and the generated filenames will be:
		// <path>_left.raw, <path>_right.raw, or <path>_51left.raw, <path>_51right.raw,
		// <path>_51center.raw, <path>_51lfe.raw, <path>_51backleft.raw, <path>_51backright.raw,
		// If only two channel mixing is enabled, the left and right .raw files will also be
		// combined into a stereo .wav file.
		virtual void			AVIOpen(const char *path, const char *name) = 0;
		virtual void			AVIClose(void) = 0;

		// SaveGame / demo Support
		virtual void			WriteToSaveGame(idFile *savefile) = 0;
		virtual void			ReadFromSaveGame(idFile *savefile) = 0;

		virtual void			SetSlowmo(bool active) = 0;
		virtual void			SetSlowmoSpeed(float speed) = 0;
		virtual void			SetEnviroSuit(bool active) = 0;

#ifdef _HUMANHEAD
	//HUMANHEAD
	virtual void			RegisterLocation(int area, const char *locationName) = 0;
	virtual void			ClearAreaLocations() = 0;
	//HUMANHEAD END

	virtual void			SetSpiritWalkEffect( bool active ) = 0;	// HUMANHEAD pdm
	virtual void			SetVoiceDucker( bool active ) = 0;		// HUMANHEAD pdm
#endif
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


class idSoundSystem
{
	public:
		virtual					~idSoundSystem(void) {}

		// all non-hardware initialization
		virtual void			Init(void) = 0;

		// shutdown routine
		virtual	void			Shutdown(void) = 0;

		// call ClearBuffer if there is a chance that the AsyncUpdate won't get called
		// for 20+ msec, which would cause a stuttering repeat of the current
		// buffer contents
		virtual void			ClearBuffer(void) = 0;

		// sound is attached to the window, and must be recreated when the window is changed
		virtual bool			InitHW(void) = 0;
		virtual bool			ShutdownHW(void) = 0;

		// asyn loop, called at 60Hz
		virtual int				AsyncUpdate(int time) = 0;

		// async loop, when the sound driver uses a write strategy
		virtual int				AsyncUpdateWrite(int time) = 0;

		// it is a good idea to mute everything when starting a new level,
		// because sounds may be started before a valid listener origin
		// is specified
		virtual void			SetMute(bool mute) = 0;

		// for the sound level meter window
		virtual cinData_t		ImageForTime(const int milliseconds, const bool waveform) = 0;

		// get sound decoder info
		virtual int				GetSoundDecoderInfo(int index, soundDecoderInfo_t &decoderInfo) = 0;

		// if rw == NULL, no portal occlusion or rendered debugging is available
		virtual idSoundWorld 	*AllocSoundWorld(idRenderWorld *rw) = 0;

		// specifying NULL will cause silence to be played
		virtual void			SetPlayingSoundWorld(idSoundWorld *soundWorld) = 0;

		// some tools, like the sound dialog, may be used in both the game and the editor
		// This can return NULL, so check!
		virtual idSoundWorld 	*GetPlayingSoundWorld(void) = 0;

		// Mark all soundSamples as currently unused,
		// but don't free anything.
		virtual	void			BeginLevelLoad(void) = 0;

		// Free all soundSamples marked as unused
		// We might want to defer the loading of new sounds to this point,
		// as we do with images, to avoid having a union in memory at one time.
		virtual	void			EndLevelLoad(const char *mapString) = 0;

		// direct mixing for OSes that support it
		virtual int				AsyncMix(int soundTime, float *mixBuffer) = 0;

		// prints memory info
		virtual void			PrintMemInfo(MemInfo_t *mi) = 0;

		// is EAX support present - -1: disabled at compile time, 0: no suitable hardware, 1: ok, 2: failed to load OpenAL DLL
		virtual int				IsEAXAvailable(void) = 0;
#ifdef _RAVEN
		virtual idSoundWorld* GetSoundWorldFromId(int worldId) = 0;
		virtual idSoundEmitter* EmitterForIndex(int worldId, int index) = 0;
		virtual int				AllocSoundEmitter(int worldId) = 0;
		virtual void			FreeSoundEmitter(int worldId, int handle, bool immediate) = 0;
		virtual void StopAllSounds(int worldId) = 0;
		virtual void SetActiveSoundWorld(bool val) = 0;
		virtual void			FadeSoundClasses(int worldId, const int soundClass, const float to, const float over) = 0;
		virtual	float			CurrentShakeAmplitudeForPosition(int worldId, const int time, const idVec3& listenerPosition) = 0;
		virtual void			PlayShaderDirectly(int worldId, const char* name, int channel = -1) = 0;
		virtual void			PlaceListener(const idVec3& origin, const idMat3& axis, const int listenerId, const int gameTime, const idStr& areaName) = 0;
		virtual void			WriteToSaveGame(int worldId, idFile* savefile) = 0;
		virtual void			ReadFromSaveGame(int worldId, idFile* savefile) = 0;

	// reset the listener portal to invalid during level transitions
	virtual void			ResetListener( void ) = 0;

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
#endif

#ifdef _HUMANHEAD
	//HUMANHEAD rww
	virtual int					GetSubtitleIndex(const char *soundName) = 0;
	virtual void				SetSubtitleData(int subIndex, int subNum, const char *subText, float subTime, int subChannel) = 0;
	virtual soundSub_t			*GetSubtitle(int subIndex, int subNum) = 0;
	virtual soundSubtitleList_t *GetSubtitleList(int subIndex) = 0;
	//HUMANHEAD END
#endif
};

extern idSoundSystem	*soundSystem;

#endif /* !__SOUND__ */
