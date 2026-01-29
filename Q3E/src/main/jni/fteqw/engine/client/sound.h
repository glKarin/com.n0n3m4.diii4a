/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sound.h -- client sound i/o functions

#ifndef __SOUND__
#define __SOUND__

//#define MIXER_F32
#define MAXSOUNDCHANNELS 8	//on a per device basis

//pitch/rate changes require that we track stuff with subsample precision.
//this can result in some awkward overflows.
#define ssamplepos_t qintptr_t
#define usamplepos_t quintptr_t
#define PITCHSHIFT 6	/*max audio file length = ((1<<32)>>PITCHSHIFT)/KHZ*/

struct sfx_s;

typedef struct {
	struct sfxcache_s *(QDECL *decodedata) (struct sfx_s *sfx, struct sfxcache_s *buf, ssamplepos_t start, int length);	//return true when done.
	float (QDECL *querydata) (struct sfx_s *sfx, struct sfxcache_s *buf, char *title, size_t titlesize);	//reports length + original format info without actually decoding anything.
	void (QDECL *ended) (struct sfx_s *sfx);	//sound stopped playing and is now silent (allow rewinding or something).
	void (QDECL *purge) (struct sfx_s *sfx);	//sound is being purged from memory. destroy everything.
	void *buf;
} sfxdecode_t;

enum
{
	SLS_NOTLOADED,	//not tried to load it
	SLS_LOADING,	//loading it on a worker thread.
	SLS_LOADED,		//currently in memory and usable.
	SLS_FAILED		//already tried to load it. it won't work. not found, invalid format, etc
};
typedef struct sfx_s
{
	char 	name[MAX_OSPATH];
	sfxdecode_t		decoder;

	int loadstate; //no more super-spammy
	qboolean touched:1; //if the sound is still relevent
	qboolean syspath:1; //if the sound is still relevent

	int loopstart;	//-1 or sample index to begin looping at once the sample ends
} sfx_t;

typedef enum
{
#ifdef FTE_TARGET_WEB
		QAF_BLOB=0,
#endif
		QAF_S8=1,
		//QAF_U8=0x80|1,
		QAF_S16=2,
		//QAF_S32=4,
#ifdef MIXER_F32
		QAF_F32=0x80|4,
#endif
#define QAF_BYTES(v) (v&0x7f)	//to make memory allocation easier.
} qaudiofmt_t;

// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct sfxcache_s
{
	usamplepos_t length;	//sample count
	unsigned int speed;
	qaudiofmt_t format;
	unsigned int numchannels;
	usamplepos_t soundoffset;	//byte index into the sound
	qbyte	*data;		// variable sized
} sfxcache_t;

typedef struct
{
	int				numchannels;			// this many samples per frame
	int				samples;				// mono samples in buffer (individual, non grouped)
	int				samplepos;				// in mono samples
	int				samplebytes;			// per channel (NOT per frame)
	enum
	{
		QSF_INVALID,	//not selected yet...
		QSF_EXTERNALMIXER,	//this sample format is totally irrelevant as this device uses some sort of external mixer.
		QSF_U8,		//FIXME: more unsigned formats need changes to S_ClearBuffer
		QSF_S8,		//signed 8bit format is actually quite rare.
		QSF_S16,	//normal format
//		QSF_X8_S24,	//upper 8 bits unused. hopefully we don't need any packed thing
//		QSF_S32,	//lower 8 bits probably unused. this makes overflow detection messy.
		QSF_F32,	//modern mixers can use SSE/SIMD stuff, and we can skip clamping so this can be quite nippy.
	} sampleformat;
	int				speed;					// this many frames per second
	unsigned char	*buffer;				// pointer to mixed pcm buffer (not directly used by mixer)
} dma_t;

//client and server
#define CF_SV_RELIABLE		1	// send reliably
#define CF_NET_SENTVELOCITY	CF_SV_RELIABLE
#define CF_FORCELOOP		2	// forces looping. set on static sounds.
#define CF_NOSPACIALISE		4	// these sounds are played at a fixed volume in both speakers, but still gets quieter with distance.
//#define CF_PAUSED			8	// rate = 0. or something.
#define CF_CL_ABSVOLUME		16	// ignores volume cvar (but not mastervolume). this is ignored if received from the server because there's no practical way for the server to respect the client's preferences.
//#define CF_SV_RESERVED	CF_CL_ABSVOLUME
#define CF_NOREVERB			32	// disables reverb on this channel, if possible.
#define CF_FOLLOW			64	// follows the owning entity (stops moving if we lose track)
#define CF_NOREPLACE		128	// start sound event is ignored if there's already a sound playing on that entchannel (probably paired with CF_FORCELOOP).

#define CF_SV_UNICAST		256 // serverside only. the sound is sent to msg_entity only.
#define CF_SV_SENDVELOCITY	512	// serverside hint that velocity is important
#define CF_CLI_AUTOSOUND	1024	// generated from q2 entities, which avoids breaking regular sounds, using it outside the sound system will probably break things.
#define CF_CLI_INACTIVE		2048	// try to play even when inactive
#ifdef Q3CLIENT
#define CF_CLI_NODUPES		4096	// block multiple identical sounds being started on the same entity within rapid succession (regardless of channel). required by quake3.
#endif
#define CF_CLI_STATIC		8192	//started via ambientsound/svc_spawnstaticsound
#define CF_NETWORKED (CF_NOSPACIALISE|CF_NOREVERB|CF_FORCELOOP|CF_FOLLOW|CF_NOREPLACE)

typedef struct
{
	sfx_t	*sfx;			// sfx number
	int		vol[MAXSOUNDCHANNELS];		// volume, 0.8 fixed point.
	ssamplepos_t pos;		// sample position in sfx, <0 means delay sound start (shifted up by PITCHSHIFT)
	int		rate;			// fixed point rate scaling
	int		flags;			// cf_ flags
	int		entnum;			// to allow overriding a specific sound
	int		entchannel;		// to avoid overriding a specific sound too easily
	vec3_t	origin;			// origin of sound effect
	vec3_t	velocity;		// velocity of sound effect
	vec_t	dist_mult;		// distance multiplier (attenuation/clipK)
	int		master_vol;		// 0-255 master volume
#ifdef Q3CLIENT
	unsigned int starttime;	// start time, to replicate q3's 50ms embargo on duped sounds.
#endif
} channel_t;

struct soundcardinfo_s;
typedef struct soundcardinfo_s soundcardinfo_t;

extern struct sndreverbproperties_s
{
	int modificationcount;
	struct reverbproperties_s 
	{	//note: this struct originally comes from openal's eaxreverb
		//it is shared with gamecode
		float flDensity;
		float flDiffusion;
		float flGain;
		float flGainHF;
		float flGainLF;
		float flDecayTime;
		float flDecayHFRatio;
		float flDecayLFRatio;
		float flReflectionsGain;
		float flReflectionsDelay;
		float flReflectionsPan[3];
		float flLateReverbGain;
		float flLateReverbDelay;
		float flLateReverbPan[3];
		float flEchoTime;	
		float flEchoDepth;
		float flModulationTime;
		float flModulationDepth;
		float flAirAbsorptionGainHF;
		float flHFReference;
		float flLFReference;
		float flRoomRolloffFactor;
		int   iDecayHFLimit;
	} props;
} *reverbproperties;
extern size_t numreverbproperties;

//reverbproperties_s presets, from efx-presets.h
//mostly for testing
#define REVERB_PRESET_PSYCHOTIC \
    { 0.0625f, 0.5000f, 0.3162f, 0.8404f, 1.0000f, 7.5600f, 0.9100f, 1.0000f, 0.4864f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f }, 2.4378f, 0.0300f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 4.0000f, 1.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x0 }
//default reverb 1
#define REVERB_PRESET_UNDERWATER \
 { 0.3645f, 1.0000f, 0.3162f, 0.0100f, 1.0000f, 1.4900f, 0.1000f, 1.0000f, 0.5963f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f }, 7.0795f, 0.0110f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 1.1800f, 0.3480f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }

void S_Init (void);
void S_Startup (void);
void S_EnumerateDevices(void);
void S_Shutdown (qboolean final);
float S_GetSoundTime(int entnum, int entchannel);
float S_GetChannelLevel(int entnum, int entchannel);
void S_StartSound (int entnum, int entchannel, sfx_t *sfx, vec3_t origin, vec3_t velocity, float fvol, float attenuation, float timeofs, float pitchadj, unsigned int flags);
float S_UpdateSound(int entnum, int entchannel, sfx_t *sfx, vec3_t origin, vec3_t velocity, float fvol, float attenuation, float timeofs, float pitchadj, unsigned int flags);
void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation);
void S_StopSound (int entnum, int entchannel);
void S_StopAllSounds(qboolean clear);
void S_UpdateListener(int seat, int entnum, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, size_t reverbtype, vec3_t velocity);
qboolean S_UpdateReverb(size_t reverbtype, void *reverb, size_t reverbsize);
void S_GetListenerInfo(int seat, float *origin, float *forward, float *right, float *up);
void S_Update (void);
void S_ExtraUpdate (void);
void S_MixerThread(soundcardinfo_t *sc);
void S_Purge(qboolean retaintouched);

void S_LockMixer(void);
void S_UnlockMixer(void);

qboolean S_HaveOutput(void);

void S_Music_Clear(sfx_t *onlyifsample);
void S_Music_Seek(float time);
qboolean S_GetMusicInfo(int musicchannel, float *time, float *duration, char *title, size_t titlesize);
qboolean S_Music_Playing(int musicchannel);
float Media_CrossFade(int musicchanel, float vol, float time);	//queries the volume we're meant to be playing (checks for fade out). -1 for no more, otherwise returns vol.
sfx_t *Media_NextTrack(int musicchanel, float *time);	//queries the track we're meant to be playing now.

sfx_t *S_FindName (const char *name, qboolean create, qboolean syspath);
sfx_t *S_PrecacheSound2 (const char *sample, qboolean syspath);
#define S_PrecacheSound(s) S_PrecacheSound2(s,false)
void S_UntouchAll(void);
void S_ClearPrecache (void);
void S_BeginPrecaching (void);
void S_EndPrecaching (void);

void S_PaintChannels(soundcardinfo_t *sc, int endtime);
void S_InitPaintChannels (soundcardinfo_t *sc);

soundcardinfo_t *S_SetupDeviceSeat(char *driver, char *device, int seat);
void S_ShutdownCard (soundcardinfo_t *sc);

void S_DefaultSpeakerConfiguration(soundcardinfo_t *sc);
void S_ResetFailedLoad(void);

#ifdef PEXT2_VOICECHAT
void S_Voip_Parse(void);
#endif
int S_Voip_ClientLoudness(unsigned int plno);
#ifdef VOICECHAT
extern cvar_t snd_voip_showmeter;
void S_Voip_Transmit(unsigned char clc, sizebuf_t *buf);
void S_Voip_MapChange(void);
int S_Voip_Loudness(qboolean ignorevad);	//-1 for not capturing, otherwise between 0 and 100
qboolean S_Voip_Speaking(unsigned int plno);
void S_Voip_Ignore(unsigned int plno, qboolean ignore);
#else
#define S_Voip_Loudness() -1
#define S_Voip_Speaking(p) false
#define S_Voip_Ignore(p,s)
#endif

qboolean S_IsPlayingSomewhere(sfx_t *s);
//qboolean ResampleSfx (sfx_t *sfx, int inrate, int inchannels, int inwidth, int insamps, int inloopstart, qbyte *data);

// picks a channel based on priorities, empty slots, number of channels
channel_t *SND_PickChannel(soundcardinfo_t *sc, int entnum, int entchannel);

void SND_ResampleStream (const void *in, int inrate, qaudiofmt_t inwidth, int inchannels, int insamps, void *out, int outrate, qaudiofmt_t outwidth, int outchannels, int resampstyle);

// restart entire sound subsystem (doesn't flush old sounds, so make sure that happens)
void S_DoRestart (qboolean onlyifneeded);

void S_Restart_f (void);

//plays streaming audio
#define SOURCEID_MENUQC -3
#define SOURCEID_CSQC -2
#define SOURCEID_CINEMATIC -1
#define SOURCEID_VOIP_FIRST 0
#define SOURCEID_VOIP_MAX MAX_CLIENTS-1
void S_RawAudio(int sourceid, const qbyte *data, int speed, int samples, int channels, qaudiofmt_t width, float volume);
float S_RawAudioQueued(int sourceid);

void CLVC_Poll (void);

void SNDVC_MicInput(qbyte *buffer, int samples, int freq, int width);



// ====================================================================
// User-setable variables
// ====================================================================

#define NUM_MUSICS				1

#define AMBIENT_FIRST 0
#define AMBIENT_STOP NUM_AMBIENTS
#define MUSIC_FIRST AMBIENT_STOP
#define MUSIC_STOP (MUSIC_FIRST + NUM_MUSICS)
#define DYNAMIC_FIRST MUSIC_STOP

//
// Fake dma is a synchronous faking of the DMA progress used for
// isolating performance in the renderer.  The fakedma_updates is
// number of times S_Update() is called per second.
//

extern int				snd_speed;

extern cvar_t snd_nominaldistance;

extern	cvar_t snd_loadas8bit;
extern	cvar_t bgmvolume;
extern	cvar_t volume, mastervolume;
extern	cvar_t snd_capture;
extern	cvar_t nosound;

extern float voicevolumemod;

extern qboolean	snd_initialized;
extern cvar_t snd_mixerthread;

extern int		snd_blocked;

void S_LocalSound (const char *s);
void S_LocalSound2 (const char *sound, int channel, float volume);
qboolean S_LoadSound (sfx_t *s, qboolean forcedecode);

typedef qboolean (QDECL *S_LoadSound_t) (sfx_t *s, qbyte *data, size_t datalen, int sndspeed, qboolean forcedecode);
qboolean S_RegisterSoundInputPlugin(void *module, S_LoadSound_t loadfnc); //called to register additional sound input plugins
void S_UnregisterSoundInputModule(void *module);

void S_AmbientOff (void);
void S_AmbientOn (void);


//inititalisation functions.
typedef struct
{
	const char *name;	//must be a single token, with no :
	qboolean (QDECL *InitCard) (soundcardinfo_t *sc, const char *cardname);	//NULL for default device.
	qboolean (QDECL *Enumerate) (void (QDECL *callback) (const char *drivername, const char *devicecode, const char *readablename));
	void (QDECL *RegisterCvars) (void);
} sounddriver_t;
/*typedef int (*sounddriver) (soundcardinfo_t *sc, int cardnum);
extern sounddriver pOPENAL_InitCard;
extern sounddriver pDSOUND_InitCard;
extern sounddriver pALSA_InitCard;
extern sounddriver pSNDIO_InitCard;
extern sounddriver pOSS_InitCard;
extern sounddriver pSDL_InitCard;
extern sounddriver pWAV_InitCard;
extern sounddriver pAHI_InitCard;
*/

typedef enum
{
	CUR_SPACIALISEONLY	= 0,			//for ticking over, respacialising, etc
	CUR_UPDATE			= (1u<<1),		//flags/rate changed without changing the sound itself
	CUR_SOUNDCHANGE		= (1u<<2),		//the audio file changed too. reset everything.
	CUR_OFFSET			= (1u<<3),
	CUR_EVERYTHING		= CUR_UPDATE|CUR_SOUNDCHANGE|CUR_OFFSET
} chanupdatereason_t;

struct soundcardinfo_s { //windows has one defined AFTER directsound
	char name[256];	//a description of the card.
	char guid[256];	//device name as detected (so input code can create sound devices without bugging out too much)
	struct soundcardinfo_s *next;
	int seat;

//speaker orientations for spacialisation.
	float dist[MAXSOUNDCHANNELS];

	vec3_t speakerdir[MAXSOUNDCHANNELS];

//info on which sound effects are playing
	//FIXME: use a linked list
	channel_t	*channel;
	size_t		total_chans;
	size_t		max_chans;

	float	ambientlevels[NUM_AMBIENTS];	//we use a float instead of the channel's int volume value to avoid framerate dependancies with slow transitions.

//mixer
	volatile dma_t sn;	//why is this volatile?
	qboolean inactive_sound;	//continue mixing for this card even when the window isn't active.
	qboolean selfpainting;	//allow the sound code to call the right functions when it feels the need (not properly supported).

	int	paintedtime;	//used in the mixer as last-written pos (in frames)
	int	oldsamplepos;	//this is used to track buffer wraps
	int	buffers;	//used to keep track of how many buffer wraps for consistant sound
	int	samplequeue;	//this is the number of samples the device can enqueue. if set, DMAPos returns the write point (rather than hardware read point) (in samplepairs).

//callbacks
	void *(*Lock) (soundcardinfo_t *sc, unsigned int *startoffset);	//grab a pointer to the hardware ringbuffer or whatever. startoffset is the starting offset. you can set it to 0 and bump the start offset if you need.
	void (*Unlock) (soundcardinfo_t *sc, void *buffer);				//release the hardware ringbuffer memory
	void (*Submit) (soundcardinfo_t *sc, int start, int end);		//if the ringbuffer is emulated, this is where you should push it to the device.
	void (*Shutdown) (soundcardinfo_t *sc);							//kill the device
	unsigned int (*GetDMAPos) (soundcardinfo_t *sc);				//get the current point that the hardware is reading from (the return value should not wrap, at least not very often)
	void (*SetEnvironmentReverb) (soundcardinfo_t *sc, size_t reverb);	//if you have eax enabled, change the environment. generally this is a stub. optional.
	void (*Restore) (soundcardinfo_t *sc);							//called before lock/unlock/lock/unlock/submit. optional
	void (*ChannelUpdate) (soundcardinfo_t *sc, channel_t *channel, chanupdatereason_t schanged);	//properties of a sound effect changed. this is to notify hardware mixers. optional.
	void (*ListenerUpdate) (soundcardinfo_t *sc, int entnum, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, vec3_t velocity);	//player moved or something. this is to notify hardware mixers. optional.
	ssamplepos_t (*GetChannelPos) (soundcardinfo_t *sc, channel_t *channel);	//queries a hardware mixer's channel position (essentially returns channel->pos, except more up to date)

//driver-specific - if you need more stuff, you should just shove it in the handle pointer
	void *thread;
	void *handle;
	int snd_sent;
	int snd_completed;
	int audio_fd;
};

extern soundcardinfo_t *sndcardinfo;

typedef struct
{
	int apiver;
	char *drivername;
	qboolean (QDECL *Enumerate) (void (QDECL *callback) (const char *drivername, const char *devicecode, const char *readablename));
	void *(QDECL *Init) (int samplerate, const char *device);			/*create a new context*/
	void (QDECL *Start) (void *ctx);		/*begin grabbing new data, old data is potentially flushed*/
	unsigned int (QDECL *Update) (void *ctx, unsigned char *buffer, unsigned int minbytes, unsigned int maxbytes);	/*grab the data into a different buffer*/
	void (QDECL *Stop) (void *ctx);		/*stop grabbing new data, old data may remain*/
	void (QDECL *Shutdown) (void *ctx);	/*destroy everything*/
} snd_capture_driver_t;

#endif
