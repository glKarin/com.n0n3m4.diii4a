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

#ifndef __SND_LOCAL_H__
#define __SND_LOCAL_H__

// you need the OpenAL headers for build, even if AL is not enabled - http://www.openal.org/
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include "../framework/UsercmdGen.h"
#include "sound.h"
#include "efxlib.h"

// demo sound commands
typedef enum {
	SCMD_STATE,				// followed by a load game state
	SCMD_PLACE_LISTENER,
	SCMD_ALLOC_EMITTER,

	SCMD_FREE,
	SCMD_UPDATE,
	SCMD_START,
	SCMD_MODIFY,
	SCMD_STOP,
	SCMD_FADE
} soundDemoCommand_t;

const int SOUND_MAX_CHANNELS		= 8;
const int SOUND_DECODER_FREE_DELAY	= 1000 * MIXBUFFER_SAMPLES / USERCMD_MSEC;		// four seconds

const int PRIMARYFREQ				= 44100;			// samples per second
const float SND_EPSILON				= 1.0f / 32768.0f;	// if volume is below this, it will always multiply to zero

const int ROOM_SLICES_IN_BUFFER		= 10;

const float UNDERWATER_VOLUME_LOSS  = 17.0f; // grayman #3556

class idAudioHardware;
class idAudioBuffer;
class idWaveFile;
class idSoundCache;
class idSoundSample;
class idSampleDecoder;
class idSoundChannel;
class idSoundWorldLocal;

//stgatilov #2454: atomic piece of subtitle text data
struct Subtitle {
	int offsetStart;		/* number of samples from start of sound to subtitle start */
	int offsetEnd;			/* number of samples from start of sound to subtitle end */
	idStr text;				/* text to be displayed (may contain \n being multiline) */
};
//stgatilov #2454: reference to subtitle which should be displayed now
struct SubtitleMatch {
	const Subtitle *subtitle;		/* subtitle to be shown */
	const idSoundSample *sample;	/* which sample? */
	const idSoundChannel *channel;	/* which channel? */
	const idSoundEmitter *emitter;	/* which emitter? */
	SubtitleLevel verbosity;		/* how important is this subtitle */
	idVec3 spatializedDirection;	/* direction vector in listener's local space */
	float volume;					/* volume of sound (0..1) */
	int status;						/* temporary: used during slot assignment */
};

/*
===================================================================================

	General extended waveform format structure.
	Use this for all NON PCM formats.

===================================================================================
*/

#ifdef WIN32
#pragma pack(1)
#endif
#ifdef __MWERKS__
#pragma pack (push, 1)
#endif
struct waveformatex_s {
	word    wFormatTag;        /* format type */
	word    nChannels;         /* number of channels (i.e. mono, stereo...) */
	dword   nSamplesPerSec;    /* sample rate */
	dword   nAvgBytesPerSec;   /* for buffer estimation */
	word    nBlockAlign;       /* block size of data */
	word    wBitsPerSample;    /* Number of bits per sample of mono data */
	word    cbSize;            /* The count in bytes of the size of
									extra information (after cbSize) */
} PACKED;

typedef waveformatex_s waveformatex_t;

/* OLD general waveform format structure (information common to all formats) */
struct waveformat_s {
	word    wFormatTag;        /* format type */
	word    nChannels;         /* number of channels (i.e. mono, stereo, etc.) */
	dword   nSamplesPerSec;    /* sample rate */
	dword   nAvgBytesPerSec;   /* for buffer estimation */
	word    nBlockAlign;       /* block size of data */
} PACKED;

typedef waveformat_s waveformat_t;

/* flags for wFormatTag field of WAVEFORMAT */
enum {
	WAVE_FORMAT_TAG_PCM		= 1,
	WAVE_FORMAT_TAG_OGG		= 2,
	WAVE_FORMAT_TAG_STREAM_CINEMATICS = 3		//#4534
};

/* specific waveform format structure for PCM data */
struct pcmwaveformat_s {
	waveformat_t	wf;
	word			wBitsPerSample;
} PACKED;

typedef pcmwaveformat_s pcmwaveformat_t;

#ifndef mmioFOURCC
#define mmioFOURCC( ch0, ch1, ch2, ch3 )				\
		( (dword)(byte)(ch0) | ( (dword)(byte)(ch1) << 8 ) |	\
		( (dword)(byte)(ch2) << 16 ) | ( (dword)(byte)(ch3) << 24 ) )
#endif

#define fourcc_riff     mmioFOURCC('R', 'I', 'F', 'F')

struct waveformatextensible_s {
	waveformatex_t    Format;
	union {
		word wValidBitsPerSample;       /* bits of precision  */
		word wSamplesPerBlock;          /* valid if wBitsPerSample==0*/
		word wReserved;                 /* If neither applies, set to zero*/
	} Samples;
	dword           dwChannelMask;      /* which channels are */
										/* present in stream  */
	int            SubFormat;
} PACKED;

typedef waveformatextensible_s waveformatextensible_t;

typedef dword fourcc;

/* RIFF chunk information data structure */
struct mminfo_s {
	fourcc			ckid;           /* chunk ID */
	dword			cksize;         /* chunk size */
	fourcc			fccType;        /* form type or list type */
	dword			dwDataOffset;   /* offset of data portion of chunk */
} PACKED;

typedef mminfo_s mminfo_t;

#ifdef WIN32
#pragma pack()
#endif
#ifdef __MWERKS__
#pragma pack (pop)
#endif

/*
===================================================================================

idWaveFile

===================================================================================
*/

class idWaveFile {
public:
					idWaveFile( void );
					~idWaveFile( void );

	int				Open( const char* strFileName, waveformatex_t* pwfx = NULL );
	int				OpenFromMemory( short* pbData, int ulDataSize, waveformatextensible_t* pwfx );
	int				Read( byte* pBuffer, int dwSizeToRead, int *pdwSizeRead );
	int				Seek( int offset );
	int				Close( void );
	int				ResetFile( void );
	ID_TIME_T		Timestamp( void ) const { return mfileTime; }

	int				GetOutputSize( void ) { return mdwSize; }
	int				GetMemorySize( void ) { return mMemSize; }

	waveformatextensible_t	mpwfx;        // Pointer to waveformatex structure

private:
	idFile *		mhmmio;			// I/O handle for the WAVE
	mminfo_t		mck;			// Multimedia RIFF chunk
	mminfo_t		mckRiff;		// used when opening a WAVE file
	dword			mdwSize;		// size in samples
	dword			mMemSize;		// size of the wave data in memory
	dword			mseekBase;
	ID_TIME_T		mfileTime;

	bool			mbIsReadingFromMemory;
	short *			mpbData;
	short *			mpbDataCur;
	dword			mulDataSize;

	void *			ogg;			// only !NULL when !s_realTimeDecoding
	int				oggStream;		// stgatilov: ogg->stream in original D3 with hacked libogg
	bool			isOgg;

private:
	int				ReadMMIO( void );

	int				OpenOGG( const char* strFileName, waveformatex_t* pwfx = NULL );
	int				ReadOGG( byte* pBuffer, int dwSizeToRead, int *pdwSizeRead );
	int				CloseOGG( void );
};


/*
===================================================================================

Encapsulates functionality of a DirectSound buffer.

===================================================================================
*/

class idAudioBuffer {
public:
	virtual int 		Play( dword dwPriority=0, dword dwFlags=0 ) = 0;
	virtual int			Stop( void ) = 0;
	virtual int			Reset( void ) = 0;
	virtual bool		IsSoundPlaying( void ) = 0;
	virtual void	 	SetVolume( float x ) = 0;
};


/*
===================================================================================

idSoundEmitterLocal

===================================================================================
*/

typedef enum {
	REMOVE_STATUS_INVALID				= -1,
	REMOVE_STATUS_ALIVE					=  0,
	REMOVE_STATUS_WAITSAMPLEFINISHED	=  1,	//stgatilov: plays now, but will be stopped & freed when it ends
	REMOVE_STATUS_SAMPLEFINISHED		=  2
} removeStatus_t;

class idSoundFade {
public:
	int					fadeStart44kHz;
	int					fadeEnd44kHz;
	float				fadeStartVolume;		// in dB
	float				fadeEndVolume;			// in dB

	void				Clear();
	float				FadeDbAt44kHz( int current44kHz );
};

class SoundFX {
protected:
	bool				initialized;

	int					channel;
	int					maxlen;

	float*				buffer;
	float				continuitySamples[4];

	float				param;

public:
						SoundFX()										{ channel = 0; buffer = NULL; initialized = false; maxlen = 0; memset( continuitySamples, 0, sizeof( float ) * 4 ); };
	virtual				~SoundFX()										{ if ( buffer ) delete buffer; };

	virtual void		Initialize()									{ };
	virtual void		ProcessSample( float* in, float* out ) = 0;

	void				SetChannel( int chan )							{ channel = chan; };
	int					GetChannel()									{ return channel; };

	void				SetContinuitySamples( float in1, float in2, float out1, float out2 )		{ continuitySamples[0] = in1; continuitySamples[1] = in2; continuitySamples[2] = out1; continuitySamples[3] = out2; };		// FIXME?
	void				GetContinuitySamples( float& in1, float& in2, float& out1, float& out2 )	{ in1 = continuitySamples[0]; in2 = continuitySamples[1]; out1 = continuitySamples[2]; out2 = continuitySamples[3]; };

	void				SetParameter( float val )						{ param = val; };
};

class SoundFX_Lowpass : public SoundFX {
public:
	virtual void		ProcessSample( float* in, float* out ) override;
};

class SoundFX_LowpassFast : public SoundFX {
	float				freq;
	float				res;
	float				a1, a2, a3;
	float				b1, b2;

public:
	virtual void		ProcessSample( float* in, float* out ) override;
	void				SetParms( float p1 = 0, float p2 = 0, float p3 = 0 );
};

class SoundFX_Comb : public SoundFX {
	int					currentTime;

public:
	virtual void		Initialize() override;
	virtual void		ProcessSample( float* in, float* out ) override;
};

class FracTime {
public:
	int			time;
	float		frac;

	void		Set( int val )					{ time = val; frac = 0; };
	void		Increment( float val )			{ frac += val; while ( frac >= 1.f ) { time++; frac--; } };
};

enum {
	PLAYBACK_RESET,
	PLAYBACK_ADVANCING
};

class idSoundChannel;

class idSlowChannel {
	bool					active;
	const idSoundChannel*	chan;

	int						playbackState;
	int						triggerOffset;

	FracTime				newPosition;
	int						newSampleOffset;

	FracTime				curPosition;
	int						curSampleOffset;

	SoundFX_LowpassFast		lowpass;

	// functions
	void					GenerateSlowChannel( FracTime& playPos, int sampleCount44k, float* finalBuffer );

	float					GetSlowmoSpeed();

public:

	void					AttachSoundChannel( const idSoundChannel *chan );
	void					Reset();

	void					GatherChannelSamples( int sampleOffset44k, int sampleCount44k, float *dest );

	bool					IsActive()				{ return active; };
	FracTime				GetCurrentPosition()	{ return curPosition; };
};

class idSoundChannel {
public:
						idSoundChannel( void );
						~idSoundChannel( void );

	void				Clear( void );
	void				Start( void );
	void				Stop( void );
	void				GatherChannelSamples( int sampleOffset44k, int sampleCount44k, float *dest ) const;
	int					GatherSubtitles( int sampleOffset44k, idList<SubtitleMatch> &matches, int level ) const;	//stgatilov #2454
	void				ALStop( void );			// free OpenAL resources if any

	bool				triggerState;			// stgatilov: is it enabled according to Start/Stop methods?
	int					trigger44kHzTime;		// hardware time sample the channel started
	int					triggerGame44kHzTime;	// game time sample time the channel started
	soundShaderParms_t	parms;					// combines the shader parms and the per-channel overrides
	idSoundSample *		leadinSample;			// if not looped, this is the only sample
	s_channelType		triggerChannel;
	const idSoundShader *soundShader;
	idSampleDecoder *	decoder;
	float				diversity;
	float				lastVolume;				// last calculated volume based on distance
	float				lastV[6];				// last calculated volume for each speaker, so we can smoothly fade
	idSoundFade			channelFade;
	bool				triggered;				// stgatilov: true means all OpenAL buffers should be recreated
	ALuint				openalSource;
	ALuint				openalStreamingOffset;
	ALuint				openalStreamingBuffer[3];
	ALuint				lastopenalStreamingBuffer[3];
	float				currentSampleVolume;	// stgatilov #6491: estimated from currently streamed data

	bool				disallowSlow;
};

class SoundChainResults // grayman #3042
{
public:
						SoundChainResults( void ) { }
	float				loss; // accumulated sound loss
	idVec3				spatializedOrigin; // where the sound seems to come from
	float				distance; // path distance back to the sound emitter
	float				spatialDistance; // distance back to the spacializedOrigin
};

class idSoundEmitterLocal : public idSoundEmitter {
public:

						idSoundEmitterLocal( void );
	virtual				~idSoundEmitterLocal( void ) override;

	//----------------------------------------------

	// the "time" parameters should be game time in msec, which is used to make queries
	// return deterministic values regardless of async buffer scheduling

	// a non-immediate free will let all currently playing sounds complete
	virtual void		Free( bool immediate ) override;

	// the parms specified will be the default overrides for all sounds started on this emitter.
	// NULL is acceptable for parms
	virtual void		UpdateEmitter( const idVec3 &origin, int listenerId, const soundShaderParms_t *parms ) override;

	// returns the length of the started sound in msec
	virtual int			StartSound( const idSoundShader *shader, const s_channelType channel, float diversity = 0, int shaderFlags = 0, bool allowSlow = true /* D3XP */ ) override;

	float				GetEffectiveVolume(idVec3 spatializedOrigin, float distance, float volumeLoss); // grayman #4882
	
	// can pass SCHANNEL_ANY
	virtual void		ModifySound( const s_channelType channel, const soundShaderParms_t *parms ) override;
	virtual void		StopSound( const s_channelType channel ) override;
	virtual void		FadeSound( const s_channelType channel, float to, float over ) override;

	virtual bool		CurrentlyPlaying( void ) const override;

	// can pass SCHANNEL_ANY
	virtual	float		CurrentAmplitude( void ) override;

	// used for save games
	virtual	int			Index( void ) const override;

	//----------------------------------------------

	void				Clear( void );

	void				OverrideParms( const soundShaderParms_t *base, const soundShaderParms_t *over, soundShaderParms_t *out, const char *comment );
	void				CheckForCompletion( int current44kHzTime );
	void				Spatialize( bool primary, idVec3 listenerPos, int listenerArea, idRenderWorld *rw ); // grayman #4882

	idSoundWorldLocal *	soundWorld;				// the world that holds this emitter

	int					index;						// in world emitter list
	removeStatus_t		removeStatus;

	idVec3				origin;
	int					listenerId;		
	soundShaderParms_t	parms;						// default overrides for all channels


	// the following are calculated in UpdateEmitter, and don't need to be archived
	float				maxDistance;				// greatest of all playing channel max distances
	float				minDistance;				// smallest of all playing channel min distances // grayman #3042
	float				volumeLoss;					// grayman #3042 - accumulated volume loss while traversing portals
	int					lastValidPortalArea;		// so an emitter that slides out of the world continues playing
	bool				playing;					// true if any channel is active according to our timings (OpenAL status does not matter)
	bool				hasShakes;
	idVec3				spatializedOrigin;			// the virtual sound origin, either the real sound origin,
													// or a point through a portal chain
	float				realDistance;				// in meters
	float				distance;					// in meters, this may be the straight-line distance, or
													// it may go through a chain of portals.  If there
													// is not an open-portal path, distance will be > maxDistance

	// a single soundEmitter can have many channels playing from the same point
	idSoundChannel		channels[SOUND_MAX_CHANNELS];

	idSlowChannel		slowChannels[SOUND_MAX_CHANNELS];

	idSlowChannel		GetSlowChannel( const idSoundChannel *chan );
	void				SetSlowChannel( const idSoundChannel *chan, idSlowChannel slow );
	void				ResetSlowChannel( const idSoundChannel *chan );

	// this is just used for feedback to the game or rendering system:
	// flashing lights and screen shakes.  Because the material expression
	// evaluation doesn't do common subexpression removal, we cache the
	// last generated value
	int					ampTime;
	float				amplitude;
};


/*
===================================================================================

idSoundWorldLocal

===================================================================================
*/

class s_stats {
public:
	s_stats( void ) {
		rinuse = 0;
		runs = 1;
		timeinprocess = 0;
		missedWindow = 0;
		missedUpdateWindow = 0;
		activeSounds = 0;
	}
	int		rinuse;
	int		runs;
	int		timeinprocess;
	int		missedWindow;
	int		missedUpdateWindow;
	int		activeSounds;
};

typedef struct soundPortalTrace_s {
	int		portalArea;
	const struct soundPortalTrace_s	*prevStack;
} soundPortalTrace_t;

class idSoundWorldLocal : public idSoundWorld {
public:
	virtual					~idSoundWorldLocal( void ) override;

	// call at each map start
	virtual void			ClearAllSoundEmitters( void ) override;
	virtual void			StopAllSounds( void ) override;

	// get a new emitter that can play sounds in this world
	virtual idSoundEmitter *AllocSoundEmitter(idVec3 loc) override; // grayman #4882

	// for load games
	virtual idSoundEmitter *EmitterForIndex( int index ) override;

	// query data from all emitters in the world
	virtual float			CurrentShakeAmplitudeForPosition( const int time, const idVec3 &listererPosition ) override;

	// where is the camera/microphone
	// listenerId allows listener-private sounds to be added
	virtual void			PlaceListener( const idVec3 &origin, const idMat3 &axis, const int listenerId, const int gameTime, const idStr& areaName, const idStr& efxPreset) override;

	// fade all sounds in the world with a given shader soundClass
	// to is in Db (sigh), over is in seconds
	virtual void			FadeSoundClasses( const int soundClass, const float to, const float over ) override;

	// dumps the current state and begins archiving commands
	virtual void			StartWritingDemo( idDemoFile *demo ) override;
	virtual void			StopWritingDemo( void ) override;

	// read a sound command from a demo file
	virtual void			ProcessDemoCommand( idDemoFile *readDemo ) override;

	// background music
	virtual void			PlayShaderDirectly( const char *name, int channel = -1 ) override;

	// pause and unpause the sound world
	virtual void			Pause( void ) override;
	virtual void			UnPause( void ) override;
	virtual bool			IsPaused( void ) override;

	// avidump
	virtual void			AVIOpen( const char *path, const char *name ) override;
	virtual void			AVIClose( void ) override;

	// SaveGame Support
	virtual void			WriteToSaveGame( idFile *savefile ) override;
	virtual void			ReadFromSaveGame( idFile *savefile ) override;
	
	void					ReadFromSaveGameSoundChannel( idFile *saveGame, idSoundChannel *ch );
	void					ReadFromSaveGameSoundShaderParams( idFile *saveGame, soundShaderParms_t *params );
	void					WriteToSaveGameSoundChannel( idFile *saveGame, idSoundChannel *ch );
	void					WriteToSaveGameSoundShaderParams( idFile *saveGame, soundShaderParms_t *params );
	
	virtual void			SetSlowmo( bool active ) override;
	virtual void			SetSlowmoSpeed( float speed ) override;
	virtual void			SetEnviroSuit( bool active ) override;

	//=======================================

							idSoundWorldLocal( void );

	void					Shutdown( void );
	void					Init( idRenderWorld *rw );

	// update
	void					ForegroundUpdate( int currentTime );
	void					OffsetSoundTime( int offset44kHz );

	idSoundEmitterLocal *	AllocLocalSoundEmitter(idVec3 loc); // grayman #4882
	void					CalcEars( int numSpeakers, idVec3 realOrigin, idVec3 listenerPos, idMat3 listenerAxis, float ears[6], float spatialize );
	void					AddChannelContribution( idSoundEmitterLocal *sound, idSoundChannel *chan,
												int current44kHz, int numSpeakers, float *finalMixBuffer );
	void					MixLoop( int current44kHz, int numSpeakers, float *finalMixBuffer );
	void					MixLoopInternal( int current44kHz, int numSpeakers, float *finalMixBuffer );
	void					AVIUpdate( void );
	float					GetDiffractionLoss(const idVec3 p1, const idVec3 p2, const idVec3 p3); // grayman #4219
	bool					ResolveOrigin( bool primary, const int stackDepth, const soundPortalTrace_t *prevStack, const int soundArea, const float dist, const float loss, const idVec3& soundOrigin, const idVec3& prevSoundOrigin, idSoundEmitterLocal *def , SoundChainResults *results); // grayman #3042 // grayman #4219 // grayman #4882
	float					FindAmplitude( idSoundEmitterLocal *sound, const int localTime, const idVec3 *listenerPosition, const s_channelType channel, bool shakesOnly );
	void					GetSubtitles( idList<SubtitleMatch> &dest ) override;	//stgatilov #2454

	//============================================

	idRenderWorld *			rw;				// for portals and debug drawing
	idDemoFile *			writeDemo;			// if not NULL, archive commands here

	idMat3					listenerAxis;
	idVec3					listenerPos;		// position in meters
	int						listenerPrivateId;
	idVec3					listenerQU;			// position in "quake units"
	int						listenerArea;
	idStr					listenerAreaName;
	idStr					listenerAreaEfxPreset;	// #6273
	ALuint					listenerEffect;
	ALuint					listenerSlot;
	ALuint					listenerFilter;
	// nbohr1more: #5587 Reverb volume control
	float					listenerSlotReverbGain;

	int						gameMsec;
	int						game44kHz;
	int						pause44kHz;
	int						lastAVI44kHz;		// determine when we need to mix and write another block

	idList<idSoundEmitterLocal *>emitters;

	idSoundFade				soundClassFade[SOUND_MAX_CLASSES];	// for global sound fading

	// avi stuff
	idFile *				fpa[6];
	idStr					aviDemoPath;
	idStr					aviDemoName;

	idSoundEmitterLocal *	localSound;		// just for playShaderDirectly()
	idSoundEmitterLocal *	secondarySound;		// just for playShaderDirectly() // grayman #4882

	bool					slowmoActive;
	float					slowmoSpeed;
	bool					enviroSuitActive;

	//stgatilov #2454: array of active subtitles is generated by sound thread but used by backend/frontend thread
	//  activeSubtitles[activeSubtitlesFrame]  is fully created and publicly visible
	//  activeSubtitles[!activeSubtitlesFrame] is currently appended to by sound thread
	idList<SubtitleMatch>	activeSubtitles[2];		// double-buffered list of active subtitles
	int						activeSubtitlesFrame;	// which buffer in activeSubtitles is valid now
	mutable idSysMutex		activeSubtitlesMutex;	
};

/*
===================================================================================

idSoundSystemLocal

===================================================================================
*/

typedef struct {
	ALuint			handle;
	int				startTime;
	idSoundChannel	*chan;
	bool			inUse;
	bool			looping;
	bool			stereo;
} openalSource_t;

class idSoundSystemLocal : public idSoundSystem {
public:
	idSoundSystemLocal( ) {
		isInitialized = false;
	}

	// all non-hardware initialization
	virtual void			Init( void ) override;

	// shutdown routine
	virtual	void			Shutdown( void ) override;

	// sound is attached to the window, and must be recreated when the window is changed
	virtual bool			ShutdownHW( void ) override;
	virtual bool			InitHW( void ) override;

	// async loop, called at 60Hz
	virtual int				AsyncUpdate( int time ) override;
	// async loop, when the sound driver uses a write strategy
	virtual int				AsyncUpdateWrite( int time ) override;
	// direct mixing called from the sound driver thread for OSes that support it
	virtual int				AsyncMix( int soundTime, float *mixBuffer ) override;

	virtual void			SetMute( bool mute ) override;

	virtual cinData_t		ImageForTime( const int milliseconds, const bool waveform ) override;

	int						GetSoundDecoderInfo( int index, soundDecoderInfo_t &decoderInfo ) override;

	// if rw == NULL, no portal occlusion or rendered debugging is available
	virtual idSoundWorld	*AllocSoundWorld( idRenderWorld *rw ) override;

	// specifying NULL will cause silence to be played
	virtual void			SetPlayingSoundWorld( idSoundWorld *soundWorld ) override;

	// some tools, like the sound dialog, may be used in both the game and the editor
	// This can return NULL, so check!
	virtual idSoundWorld	*GetPlayingSoundWorld( void ) override;

	virtual	void			BeginLevelLoad( void ) override;
	virtual	void			EndLevelLoad( const char *mapString ) override;

	virtual void			PrintMemInfo( MemInfo_t *mi ) override;

	virtual int				IsEFXAvailable(void) override;

	//-------------------------

	int						GetCurrent44kHzTime( void ) const;
	float					dB2Scale( const float val ) const;
	int						SamplesToMilliseconds( int samples ) const;
	int						MillisecondsToSamples( int ms ) const;

	void					DoEnviroSuit( float* samples, int numSamples, int numSpeakers );

	ALuint					AllocOpenALSource( idSoundChannel *chan, bool looping, bool stereo );
	void					FreeOpenALSource( ALuint handle );

	idSoundCache *			soundCache;

	idSoundWorldLocal *		currentSoundWorld;	// the one to mix each async tic

	int						olddwCurrentWritePos;	// statistics
	int						buffers;				// statistics
	int						CurrentSoundTime;		// set by the async thread and only used by the main thread

	unsigned int			nextWriteBlock;

	float 					realAccum[6*MIXBUFFER_SAMPLES+16];
	float *					finalMixBuffer;			// points inside realAccum at a 16 byte aligned boundary

	bool					isInitialized;
	bool					muted;
	bool					shutdown;

	s_stats					soundStats;				// NOTE: updated throughout the code, not displayed anywhere

	int						meterTops[256];
	int						meterTopsTime[256];

	dword *					graph;

	float					volumesDB[1200];		// dB to float volume conversion

	idList<SoundFX*>		fxList;

	ALCdevice				*openalDevice;
	ALCcontext				*openalContext;
	ALsizei					openalSourceCount;
	openalSource_t			openalSources[256];

	LPALGENEFFECTS			alGenEffects;
	LPALDELETEEFFECTS		alDeleteEffects;
	LPALISEFFECT			alIsEffect;
	LPALEFFECTI				alEffecti;
	LPALEFFECTF				alEffectf;
	LPALEFFECTFV			alEffectfv;
	LPALGENFILTERS			alGenFilters;
	LPALDELETEFILTERS		alDeleteFilters;
	LPALISFILTER			alIsFilter;
	LPALFILTERI				alFilteri;
	LPALFILTERF				alFilterf;
	LPALGENAUXILIARYEFFECTSLOTS		alGenAuxiliaryEffectSlots;
	LPALDELETEAUXILIARYEFFECTSLOTS	alDeleteAuxiliaryEffectSlots;
	LPALISAUXILIARYEFFECTSLOT		alIsAuxiliaryEffectSlot;
	LPALAUXILIARYEFFECTSLOTI		alAuxiliaryEffectSloti;
	// nbohr1more: #5587 Reverb volume control
	LPALAUXILIARYEFFECTSLOTF		alAuxiliaryEffectSlotf;

	idEFXFile				EFXDatabase;
	bool					efxloaded;
							// latches
	static bool				useEFXReverb;
							// mark available during initialization, or through an explicit test
	static int				EFXAvailable;

	static bool				HRTFAvailable;	// can be used on this OpenAL implementation?
	static bool				useHRTF;		// actually used in this runtime?


	static idCVar			s_noSound;
	static idCVar			s_diffractionMax; // grayman #4219
	static idCVar			s_device;
	static idCVar			s_quadraticFalloff;
	static idCVar			s_minVolume6;
	static idCVar			s_dotbias6;
	static idCVar			s_minVolume2;
	static idCVar			s_dotbias2;
	static idCVar			s_spatializationDecay;
	static idCVar			s_showStartSound;
	static idCVar			s_maxSoundsPerShader;
	static idCVar			s_reverse;
	static idCVar			s_showLevelMeter;
	static idCVar			s_meterTopTime;
	static idCVar			s_volume;
	static idCVar			s_constantAmplitude;
	static idCVar			s_playDefaultSound;
	static idCVar			s_useOcclusion;
	static idCVar			s_subFraction;
	static idCVar			s_globalFraction;
	static idCVar			s_doorDistanceAdd;
	static idCVar			s_singleEmitter;
	static idCVar			s_numberOfSpeakers;
	static idCVar			s_force22kHz;
	static idCVar			s_clipVolumes;
	static idCVar			s_realTimeDecoding;
	static idCVar			s_useEAXReverb;
	static idCVar			s_useHRTF;
	static idCVar			s_decompressionLimit;
	// nbohr1more: #5587 Reverb volume control
	static idCVar			s_alReverbGain;

	static idCVar			s_slowAttenuate;

	static idCVar			s_enviroSuitCutoffFreq;
	static idCVar			s_enviroSuitCutoffQ;
	static idCVar			s_enviroSuitSkipLowpass;
	static idCVar			s_enviroSuitSkipReverb;

	static idCVar			s_reverbTime;
	static idCVar			s_reverbFeedback;
	static idCVar			s_enviroSuitVolumeScale;
	static idCVar			s_skipHelltimeFX;
};

extern	idSoundSystemLocal	soundSystemLocal;


/*
===================================================================================

  This class holds the actual wavefile bitmap, size, and info.

===================================================================================
*/

const int SCACHE_SIZE = MIXBUFFER_SAMPLES*20;	// 1/2 of a second (aroundabout)

class idSoundSample {
public:
							idSoundSample();
							~idSoundSample();

	idStr					name;						// name of the sample file
	ID_TIME_T		 		timestamp;					// the most recent of all images used in creation, for reloadImages command

	waveformatex_t			objectInfo;					// what are we caching
	int						objectSize;					// size of waveform in samples, excludes the header
	int						objectMemSize;				// object size in memory
	byte *					nonCacheData;				// if it's not cached
	byte *					amplitudeData;				// precomputed min,max amplitude pairs
	ALuint					openalBuffer;				// openal buffer
	bool					hardwareBuffer;
	bool					defaultSound;
	bool					purged;
	bool					levelLoadReferenced;		// so we can tell which samples aren't needed any more
	idList<Subtitle>		subtitles;
	int						subtitleTotalDuration;
	SubtitleLevel			subtitlesVerbosity;
	LoadStack *				loadStack;

	int						LengthIn44kHzSamples() const;	// BEWARE: this is 2 x duration if stereo!
	int						DurationIn44kHzSamples() const;	// stgatilov: duration multiplied by 44.1K
	ID_TIME_T		 			GetNewTimeStamp( void ) const;
	void					MakeDefault();				// turns it into a beep
	void					Load();						// loads the current sound based on name
	void					Reload( bool force );		// reloads if timestamp has changed, or always if force
	void					LoadSubtitles();			// load subtitles from .srt file if it is present (stgatilov #2454)
	void					PurgeSoundSample();			// frees all data
	void					CheckForDownSample();		// down sample if required
	bool					FetchFromCache( int offset, const byte **output, int *position, int *size, const bool allowIO );
	int						FetchSubtitles( int offset, idList<SubtitleMatch> &matches );	//stgatilov #2454
	bool					HaveSubtitlesFinished( int offset ) const;

	idStr loadError;									// loading error from stage 1 --- should be defaulted if non-empty
	void					Load_Stage1();				// can be done in parallel
	void					Load_Stage2();				// done strictly sequentally

	//stgatilov #4534: for playing sound from a video
	idCinematic *cinematic;
	void LoadFromCinematic(idCinematic *cin);
	bool FetchFromCinematic(int sampleOffset, int *sampleSize, float *output);

};


/*
===================================================================================

  Sound sample decoder.

===================================================================================
*/

class idSampleDecoder {
public:
	static void				Init( void );
	static void				Shutdown( void );
	static idSampleDecoder *Alloc( void );
	static void				Free( idSampleDecoder *decoder );
	static int				GetNumUsedBlocks( void );
	static int				GetUsedBlockMemory( void );

	virtual					~idSampleDecoder( void ) {}
	virtual void			Decode( idSoundSample *sample, int sampleOffset44k, int sampleCount44k, float *dest ) = 0;
	virtual void			ClearDecoder( void ) = 0;
	virtual idSoundSample *	GetSample( void ) const = 0;
	virtual int				GetLastDecodeTime( void ) const = 0;
};


/*
===================================================================================

  The actual sound cache.

===================================================================================
*/

class idSoundCache {
public:
							idSoundCache();
							~idSoundCache();

	idSoundSample *			FindSound( const idStr &fname );

	const int				GetNumObjects( void ) { return listCache.Num(); }
	const idSoundSample *	GetObject( const int index ) const;

	void					ReloadSounds( bool force );
	void					ReloadSubtitles();

	void					BeginLevelLoad();
	void					EndLevelLoad();

	byte*					AllocLocked( int bytes ) const;	// locks mutex if insideParallelLoading
	void					FreeLocked( byte *ptr ) const;	// ...

	void					PrintMemInfo( MemInfo_t *mi );

private:
	bool					insideLevelLoad;
	bool					insideParallelLoading;
	idList<idSoundSample*>	listCache;
	idHashIndex				cacheHash;
};

#endif /* !__SND_LOCAL_H__ */
