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
// snd_dma.c -- main control for any streaming sound output devices

#include "quakedef.h"

#ifdef __GNUC__
	#define fte_weakstruct __attribute__((weak))
#else
	//msvc's uninitialised symbols are always weak, so this is fine.
	#define fte_weakstruct
#endif

#ifdef CSQC_DAT
//for sounds following csqc ents
	#include "pr_common.h"
	extern world_t csqc_world;
#endif

static void S_Play_f(void);
static void S_SoundList_f(void);
#ifdef HAVE_MIXER
static void S_Update_(soundcardinfo_t *sc);
#endif
void S_StopAllSounds(qboolean clear);
static void S_StopAllSounds_f (void);

static void S_UpdateCard(soundcardinfo_t *sc);
static void S_ClearBuffer (soundcardinfo_t *sc);

// =======================================================================
// Internal sound data & structures
// =======================================================================

soundcardinfo_t *sndcardinfo;	//the master card.

int				snd_blocked = 0;
static qboolean	snd_ambient = 1;
qboolean		snd_initialized = false;
int				snd_speed;
float			voicevolumemod = 1;

static struct listener_s
{
	int		entnum;
	vec3_t	origin;
	vec3_t	velocity;
	vec3_t	forward;
	vec3_t	right;
	vec3_t	up;
} listener[MAX_SPLITS];
cvar_t snd_nominaldistance		= CVARAFD("s_nominaldistance", "1000", "snd_soundradius", CVAR_CHEAT, "This cvar defines how far an attenuation=1 sound can be heard.");

#define	MAX_SFX		8192
sfx_t		*known_sfx;		// hunk allocated [MAX_SFX]
int			num_sfx;

sfx_t		*ambient_sfx[NUM_AMBIENTS];

//int 		desired_speed = 44100;
int 		desired_bits = 16;

int sound_started=0;

cvar_t mastervolume				= CVARFD(	"mastervolume", "1", CVAR_ARCHIVE, "Additional multiplier for all other sounds.");
cvar_t bgmvolume				= CVARAFD(	"musicvolume", "0.3", "bgmvolume", CVAR_ARCHIVE,
											"Volume level for background music.");
cvar_t volume					= CVARAFD(	"volume", "0.7", /*q3*/"s_volume",CVAR_ARCHIVE,
											"Volume level for game sounds (does not affect music, voice, or cinematics).");

cvar_t nosound					= CVARFD(	"nosound", "0", CVAR_ARCHIVE,
											"Disable all sound from the engine. Cannot be overriden by configs or anything if set via the -nosound commandline argument.");
cvar_t snd_precache				= CVARAF(	"s_precache", "1",
											"precache", 0);
cvar_t snd_loadas8bit			= CVARAFD(	"s_loadas8bit", "0",
											"loadas8bit", CVAR_ARCHIVE,
											"Downsample sounds on load as lower quality 8-bit sound, to save memory.");
cvar_t snd_loadasstereo			= CVARD(	"snd_loadasstereo", "0",
											"Force mono sounds to load as if stereo ones, to waste memory. Not normally useful.");
cvar_t ambient_level			= CVARAFD(	"s_ambientlevel", "0.3",
											"ambient_level", CVAR_ARCHIVE,
											"This controls the volume levels of automatic area-based sounds (like water or sky), and is quite annoying. If you're playing deathmatch you'll definitely want this OFF.");
cvar_t ambient_fade				= CVARAF(	"s_ambientfade", "100",
											"ambient_fade", CVAR_ARCHIVE);
cvar_t snd_noextraupdate		= CVARAF(	"s_noextraupdate", "0",
											"snd_noextraupdate", 0);
cvar_t snd_show					= CVARAF(	"s_show", "0",
											"snd_show", 0);
#ifdef __DJGPP__
#define DEFAULT_SND_KHZ "11"
#else
//fixme: are android devices more likely to use 44.1khz?
#define DEFAULT_SND_KHZ "48"	//most modern systems should go with 48khz audio (dvd quality). various hardware codecs support nothing else.
#endif
cvar_t snd_khz					= CVARAFD(	"s_khz", DEFAULT_SND_KHZ,
											"snd_khz", CVAR_ARCHIVE, "Sound speed, in kilohertz. Common values are 11, 22, 44, 48. Values above 1000 are explicitly in hertz.");
cvar_t	snd_inactive			= CVARAFD(	"s_inactive", "1",
											"snd_inactive", CVAR_ARCHIVE,
											"Play sound while application is inactive (ie: tabbed out). Needs a snd_restart if changed."
											);	//set if you want sound even when tabbed out.
cvar_t _snd_mixahead			= CVARAFD(	"s_mixahead", "0.1",
											"_snd_mixahead", CVAR_ARCHIVE, "Specifies how many seconds to prebuffer audio. Lower values give less latency, but might result in crackling. Different hardware/drivers have different tolerances, and this setting may be ignored completely where drivers are expected to provide their own tolerances.");
cvar_t snd_leftisright			= CVARAF(	"s_swapstereo", "0",
											"snd_leftisright", CVAR_ARCHIVE);
cvar_t snd_eax					= CVARAF(	"s_eax", "0",
											"snd_eax", 0);
cvar_t snd_speakers				= CVARAFD(	"s_numspeakers", "2",
											"snd_numspeakers", CVAR_ARCHIVE, "Number of hardware audio channels to use. "FULLENGINENAME" supports up to 6.");
cvar_t snd_buffersize			= CVARAF(	"s_buffersize", "0",
											"snd_buffersize", 0);
#if defined(MIXER_F32) && defined(FTE_TARGET_WEB)	//let emscripten have float audio
cvar_t snd_samplebits			= CVARAF(	"s_bits", "32",
											"snd_samplebits", CVAR_ARCHIVE);
#else
cvar_t snd_samplebits			= CVARAF(	"s_bits", "16",
											"snd_samplebits", CVAR_ARCHIVE);
#endif
cvar_t snd_playersoundvolume	= CVARAFD(	"s_localvolume", "1",
											"snd_localvolume", CVAR_ARCHIVE,
											"Sound level for sounds local or originating from the player such as firing and pain sounds.");	//sugested by crunch
cvar_t snd_doppler				= CVARAFD(	"s_doppler", "0",
											"snd_doppler", CVAR_ARCHIVE,
											"Enables doppler, with a multiplier for the scale.");
cvar_t snd_doppler_min			= CVARAFD(	"s_doppler_min", "0.5",
											"snd_doppler_min", CVAR_ARCHIVE,
											"Slowest allowed doppler scale.");
cvar_t snd_doppler_max			= CVARAFD(	"s_doppler_max", "2",
											"snd_doppler_max", CVAR_ARCHIVE,
											"Highest allowed doppler scale, to avoid things getting too weird.");
cvar_t snd_playbackrate			= CVARFD(	"snd_playbackrate", "1", CVAR_CHEAT, "Debugging cvar that changes the playback rate of all new sounds.");
cvar_t snd_ignoregamespeed		= CVARFD(	"snd_ignoregamespeed", "0", 0, "When set, allows sounds to desynchronise with game time or demo speeds.");

cvar_t snd_ignorecueloops		= CVARD(	"snd_ignorecueloops", "0", "Ignores cue commands in wav files, for q3 compat.");
cvar_t snd_linearresample		= CVARAF(	"s_linearresample", "1",
											"snd_linearresample", 0);
cvar_t snd_linearresample_stream = CVARAF(	"s_linearresample_stream", "0",
											"snd_linearresample_stream", 0);

cvar_t snd_mixerthread			= CVARAD(	"s_mixerthread", "1",
											"snd_mixerthread", "When enabled sound mixing will be run on a separate thread. Currently supported only by directsound. Other drivers may unconditionally thread audio. Set to 0 only if you have issues.");
cvar_t snd_device				= CVARAFD(	"s_device", "",
										  "snd_device", CVAR_ARCHIVE, "This is the sound device(s) to use, in the form of driver:device.\nIf desired, multiple devices can be listed in space-seperated (quoted) tokens. _s_device_opts contains any enumerated options.\nIn all seriousness, use the menu to set this if you wish to keep your hair.");
cvar_t snd_device_opts			= CVARFD(	"_s_device_opts", "", CVAR_NOSET|CVAR_NOSAVE, "The possible audio output devices, in \"value\" \"description\" pairs, for gamecode to read.");

#ifdef VOICECHAT
static void QDECL S_Voip_Play_Callback(cvar_t *var, char *oldval);
cvar_t snd_voip_capturedevice	= CVARF("cl_voip_capturedevice", "", CVAR_ARCHIVE);
cvar_t snd_voip_capturedevice_opts	= CVARFD("_cl_voip_capturedevice_opts", "", CVAR_NOSET|CVAR_NOSAVE, "The possible audio capture devices, in \"value\" \"description\" pairs, for gamecode to read.");
int voipbutton;	//+voip, no longer part of cl_voip_send to avoid it getting saved
cvar_t snd_voip_send			= CVARFD("cl_voip_send", "0", CVAR_ARCHIVE|CVAR_NOTFROMSERVER, "Sends voice-over-ip data to the server whenever it is set.\n0: only send voice if +voip is pressed.\n1: voice activation.\n2: constantly send.\n+4: Do not send to game, only to rtp sessions.");
cvar_t snd_voip_test			= CVARD("cl_voip_test", "0", "If 1, enables you to hear your own voice directly, bypassing the server and thus without networking latency, but is fine for checking audio levels. Note that sv_voip_echo can be set if you want to include latency and packetloss considerations, but setting that cvar requires server admin access and is thus much harder to use.");
cvar_t snd_voip_vad_threshhold	= CVARFD("cl_voip_vad_threshhold", "15", CVAR_ARCHIVE, "This is the threshhold for voice-activation-detection when sending voip data");
cvar_t snd_voip_vad_delay		= CVARD("cl_voip_vad_delay", "0.3", "Keeps sending voice data for this many seconds after voice activation would normally stop");
cvar_t snd_voip_capturingvol	= CVARAFD("cl_voip_capturingvol", "0.5", NULL, CVAR_ARCHIVE, "Volume multiplier applied while capturing, to avoid your audio from being heard by others. Does not affect game volume when others speak (minimum of cl_voip_capturingvol and cl_voip_ducking is used).");
cvar_t snd_voip_showmeter		= CVARAFD("cl_voip_showmeter", "1", NULL, CVAR_ARCHIVE, "Shows your speech volume above the standard hud. 0=hide, 1=show when transmitting, 2=ignore voice-activation disable");

cvar_t snd_voip_play			= CVARAFCD("cl_voip_play", "1", NULL, CVAR_ARCHIVE, S_Voip_Play_Callback, "Enables voip playback. Value is a volume scaler.");
cvar_t snd_voip_ducking			= CVARAFD("cl_voip_ducking", "0.5", NULL, CVAR_ARCHIVE, "Scales game audio by this much when someone is talking to you. Does not affect your speaker volume when you speak (minimum of cl_voip_capturingvol and cl_voip_ducking is used).");
cvar_t snd_voip_micamp			= CVARAFD("cl_voip_micamp", "2", NULL, CVAR_ARCHIVE, "Amplifies your microphone when using voip.");
cvar_t snd_voip_codec			= CVARAFD("cl_voip_codec", "", NULL, CVAR_ARCHIVE, "0: speex(@11khz). 1: raw. 2: opus. 3: speex(@8khz). 4: speex(@16). 5:speex(@32). 6: pcma. 7: pcmu.");
#ifdef HAVE_SPEEX
cvar_t snd_voip_noisefilter		= CVARAFD("cl_voip_noisefilter", "1", NULL, CVAR_ARCHIVE, "Enable the use of the noise cancelation filter.");
cvar_t snd_voip_autogain		= CVARAFD("cl_voip_autogain", "0", NULL, CVAR_ARCHIVE, "Attempts to normalize your voice levels to a standard level. Useful for lazy people, but interferes with voice activation levels.");
#endif
cvar_t snd_voip_bitrate			= CVARAFD("cl_voip_bitrate", "3000", NULL, CVAR_ARCHIVE, "For codecs with non-specific bitrates, this specifies the target bitrate to use.");
#endif

extern vfsfile_t *rawwritefile;
#ifdef MULTITHREAD
void *mixermutex;
void S_LockMixer(void)
{
	Sys_LockMutex(mixermutex);
}
void S_UnlockMixer(void)
{
	Sys_UnlockMutex(mixermutex);
}
#else
void S_LockMixer(void)
{
}
void S_UnlockMixer(void)
{
}
#endif

void S_AmbientOff (void)
{
	snd_ambient = false;
}


void S_AmbientOn (void)
{
	snd_ambient = true;
}

qboolean S_HaveOutput(void)
{
	return sound_started && sndcardinfo;
}


void S_SoundInfo_f(void)
{
	int i, j;
	int active, known;
	soundcardinfo_t *sc;
	if (!sound_started)
	{
		Con_Printf ("sound system not started\n");
		return;
	}

	if (!sndcardinfo)
	{
		Con_Printf ("No sound cards\n");
		return;
	}
	for (sc = sndcardinfo; sc; sc = sc->next)
	{
		Con_Printf("Audio Device: %s\n", sc->name);
		Con_Printf(" %d channels, %gkhz, %d bit audio%s\n", sc->sn.numchannels, sc->sn.speed/1000.0, sc->sn.samplebytes*8, sc->selfpainting?", threaded":"");
		Con_Printf(" %d samples in buffer\n", sc->sn.samples);
		for (i = 0, active = 0, known = 0; i < sc->total_chans; i++)
		{
			if (sc->channel[i].sfx)
			{
				known++;
				for (j = 0; j < MAXSOUNDCHANNELS; j++)
				{
					if (sc->channel[i].vol[j])
					{
						active++;
						break;
					}
				}
				if (j<MAXSOUNDCHANNELS)
					Con_Printf(" %s (%i %i, %g %g %g, active)\n", sc->channel[i].sfx->name, sc->channel[i].entnum, sc->channel[i].entchannel, sc->channel[i].origin[0], sc->channel[i].origin[1], sc->channel[i].origin[2]);
				else
					Con_DPrintf(" %s (%i %i, %g %g %g, inactive)\n", sc->channel[i].sfx->name, sc->channel[i].entnum, sc->channel[i].entchannel, sc->channel[i].origin[0], sc->channel[i].origin[1], sc->channel[i].origin[2]);
			}
		}
		Con_Printf(" %d/%d/%"PRIiSIZE"/%"PRIiSIZE" active/known/highest/max\n", active, known, sc->total_chans, sc->max_chans);
		for (i = 0; i < sc->sn.numchannels; i++)
		{
			Con_Printf(" chan %i: fwd:%-4g rt:%-4g up:%-4g dist:%-4g\n", i, sc->speakerdir[i][0], sc->speakerdir[i][1], sc->speakerdir[i][2], sc->dist[i]);
		}
	}
}

#ifdef VOICECHAT
#ifdef SPEEX_STATIC
#include <speex/speex.h>
#include <speex/speex_preprocess.h>
#else
typedef struct {int stuff[15];} SpeexBits;
typedef struct SpeexMode SpeexMode;
typedef struct SpeexPreprocessState SpeexPreprocessState;
typedef qint16_t spx_int16_t;

#define SPEEX_MODEID_NB 0
#define SPEEX_MODEID_WB 1
#define SPEEX_MODEID_UWB 2
#define SPEEX_GET_FRAME_SIZE 3

#define SPEEX_SET_SAMPLING_RATE 24
#define SPEEX_GET_SAMPLING_RATE 25


#define SPEEX_PREPROCESS_SET_DENOISE 0
#define SPEEX_PREPROCESS_SET_AGC 2
#define SPEEX_PREPROCESS_SET_AGC_MAX_GAIN 30
#endif

enum
{
	VOIP_SPEEX_OLD	= 0,	//original supported codec (with needless padding and at the wrong rate to keep quake implementations easy)
	VOIP_RAW16		= 1,	//support is not recommended.
	VOIP_OPUS		= 2,	//supposed to be better than speex.
	VOIP_SPEEX_NARROW = 3,	//narrowband speex. packed data.
	VOIP_SPEEX_WIDE = 4,	//wideband speex. packed data.
	VOIP_SPEEX_ULTRAWIDE = 5,//wideband speex. packed data.
	VOIP_PCMA		= 6,	//G711 is kinda shit, encoding audio at 8khz with funny truncation for 13bit to 8bit
	VOIP_PCMU		= 7,	//ulaw version of g711 (instead of alaw)

	VOIP_INVALID = 16	//not currently generating audio.
};
#if defined(HAVE_LEGACY) && defined(HAVE_OPUS) && defined(HAVE_SPEEX)
	#define VOIP_DEFAULT_CODEC ((cls.protocol==CP_QUAKEWORLD && !(cls.fteprotocolextensions2&PEXT2_REPLACEMENTDELTAS))?VOIP_SPEEX_OLD:VOIP_OPUS)	//opus is preferred, but ezquake is still common and only supports my first attempt at voice compression so favour that for mvdsv servers.
#elif defined(HAVE_OPUS)
	#define VOIP_DEFAULT_CODEC VOIP_OPUS
#elif defined(HAVE_SPEEX)
	#define VOIP_DEFAULT_CODEC VOIP_SPEEX_OLD
#else
	#define VOIP_DEFAULT_CODEC VOIP_PCMA
#endif

static struct
{
#ifdef HAVE_SPEEX
	struct
	{
		qboolean inited;
		qboolean loaded;
		dllhandle_t *speexlib;

		SpeexBits encbits;
		SpeexBits decbits[MAX_CLIENTS];

		const SpeexMode *modenb;
		const SpeexMode *modewb;
		const SpeexMode *modeuwb;
	} speex;

	struct
	{
		qboolean inited;
		qboolean loaded;
		dllhandle_t *speexdsplib;

		SpeexPreprocessState *preproc;	//filter out noise
		int curframesize;
		int cursamplerate;
	} speexdsp;
#endif

#ifdef HAVE_OPUS
	struct
	{
		qboolean inited;
		qboolean loaded;
		dllhandle_t *opuslib;
	} opus;
#endif

	unsigned char enccodec;
	void *encoder;
	unsigned int encframesize;
	unsigned int encsamplerate;

	void *decoder[MAX_CLIENTS];
	float declevel[MAX_CLIENTS];
	unsigned char deccodec[MAX_CLIENTS];
	unsigned char decseq[MAX_CLIENTS];	/*sender's sequence, to detect+cover minor packetloss*/
	unsigned char decgen[MAX_CLIENTS];	/*last generation. if it changes, we flush speex to reset packet loss*/
	unsigned int decsamplerate[MAX_CLIENTS];
	unsigned int decframesize[MAX_CLIENTS];
	float lastspoke[MAX_CLIENTS];	/*time when they're no longer considered talking. if future, they're talking (timeout avoids flickering, and harder to troll with fake-tourettes when noone is looking)*/
	float lastspoke_any;

	unsigned char capturebuf[32768]; /*pending data*/
	unsigned int capturepos;/*amount of pending data*/
	unsigned int encsequence;/*the outgoing sequence count*/
	unsigned int enctimestamp;/*for rtp streaming*/
	unsigned int generation;/*incremented whenever capture is restarted*/
	qboolean wantsend;	/*set if we're capturing data to send*/
	float voiplevel;	/*your own voice level*/
	unsigned int dumps;	/*trigger a new generation thing after a bit*/
	unsigned int keeps;	/*for vad_delay*/
	int curbitrate;

	snd_capture_driver_t *cdriver;/*capture driver's functions*/
	void *cdriverctx;	/*capture driver context*/
} s_voip;

#ifdef HAVE_OPUS
#define OPUS_APPLICATION_VOIP				2048
#define OPUS_SET_BITRATE_REQUEST			4002
#define OPUS_RESET_STATE					4028
#ifdef OPUS_STATIC
#include "opus.h"
#define qopus_encoder_create	opus_encoder_create
#define qopus_encoder_destroy	opus_encoder_destroy
#define qopus_encoder_ctl		opus_encoder_ctl
#define qopus_encode			opus_encode
#define qopus_decoder_create	opus_decoder_create
#define qopus_decoder_destroy	opus_decoder_destroy
#define qopus_decoder_ctl		opus_decoder_ctl
#define qopus_decode			opus_decode
#else
#define opus_int32 int
#define opus_int16 short
#define OpusEncoder void
#define OpusDecoder void
static OpusEncoder *(VARGS *qopus_encoder_create)(opus_int32 Fs, int channels, int application, int *error);
static void (VARGS *qopus_encoder_destroy)(OpusEncoder *st);
static int (VARGS *qopus_encoder_ctl)(OpusEncoder *st, int request, ...);
static opus_int32 (VARGS *qopus_encode)(OpusEncoder *st, const opus_int16 *pcm, int frame_size, unsigned char *data, opus_int32 max_data_bytes);
static OpusDecoder *(VARGS *qopus_decoder_create)(opus_int32 Fs, int channels, int *error);
static void (VARGS *qopus_decoder_destroy)(OpusDecoder *st);
static int (VARGS *qopus_decoder_ctl)(OpusDecoder *st, int request, ...);
static int (VARGS *qopus_decode)(OpusDecoder *st, const unsigned char *data, opus_int32 len, opus_int16 *pcm, int frame_size, int decode_fec);
static dllfunction_t qopusfuncs[] =
{
	{(void*)&qopus_encoder_create,	"opus_encoder_create"},
	{(void*)&qopus_encoder_destroy,	"opus_encoder_destroy"},
	{(void*)&qopus_encoder_ctl,		"opus_encoder_ctl"},
	{(void*)&qopus_encode,			"opus_encode"},

	{(void*)&qopus_decoder_create,	"opus_decoder_create"},
	{(void*)&qopus_decoder_destroy,	"opus_decoder_destroy"},
	{(void*)&qopus_decoder_ctl,		"opus_decoder_ctl"},
	{(void*)&qopus_decode,			"opus_decode"},

	{NULL}
};
#endif

static qboolean S_Opus_Init(void)
{
#ifndef OPUS_STATIC
#ifdef _WIN32
	char *modulename = "libopus-0" ARCH_DL_POSTFIX;
#else
	char *modulename = "libopus"ARCH_DL_POSTFIX".0";
#endif

	if (s_voip.opus.inited)
		return s_voip.opus.loaded;
	s_voip.opus.inited = true;

	s_voip.opus.opuslib = Sys_LoadLibrary(modulename, qopusfuncs);
	if (!s_voip.opus.opuslib)
	{
		Con_Printf("%s not found. Voice chat is not available.\n", modulename);
		return false;
	}
#endif

	s_voip.opus.loaded = true;
	return s_voip.opus.loaded;
}
#endif

#ifdef HAVE_SPEEX
#ifdef SPEEX_STATIC
#define qspeex_lib_get_mode speex_lib_get_mode
#define qspeex_bits_init speex_bits_init
#define qspeex_bits_reset speex_bits_reset
#define qspeex_bits_write speex_bits_write

#define qspeex_preprocess_state_init speex_preprocess_state_init
#define qspeex_preprocess_state_destroy speex_preprocess_state_destroy
#define qspeex_preprocess_ctl speex_preprocess_ctl
#define qspeex_preprocess_run speex_preprocess_run

#define qspeex_encoder_init speex_encoder_init
#define qspeex_encoder_destroy speex_encoder_destroy
#define qspeex_encoder_ctl speex_encoder_ctl
#define qspeex_encode_int speex_encode_int

#define qspeex_decoder_init speex_decoder_init
#define qspeex_decoder_destroy speex_decoder_destroy
#define qspeex_decode_int speex_decode_int
#define qspeex_bits_read_from speex_bits_read_from
#else
static const SpeexMode *(VARGS *qspeex_lib_get_mode)(int mode);
static void (VARGS *qspeex_bits_init)(SpeexBits *bits);
static void (VARGS *qspeex_bits_reset)(SpeexBits *bits);
static int (VARGS *qspeex_bits_write)(SpeexBits *bits, char *bytes, int max_len);

static SpeexPreprocessState *(VARGS *qspeex_preprocess_state_init)(int frame_size, int sampling_rate);
static void (VARGS *qspeex_preprocess_state_destroy)(SpeexPreprocessState *st);
static int (VARGS *qspeex_preprocess_ctl)(SpeexPreprocessState *st, int request, void *ptr);
static int (VARGS *qspeex_preprocess_run)(SpeexPreprocessState *st, spx_int16_t *x);

static void * (VARGS *qspeex_encoder_init)(const SpeexMode *mode);
static int (VARGS *qspeex_encoder_ctl)(void *state, int request, void *ptr);
static int (VARGS *qspeex_encode_int)(void *state, spx_int16_t *in, SpeexBits *bits);

static void *(VARGS *qspeex_decoder_init)(const SpeexMode *mode);
static void (VARGS *qspeex_decoder_destroy)(void *state);
static int (VARGS *qspeex_decode_int)(void *state, SpeexBits *bits, spx_int16_t *out);
static void (VARGS *qspeex_bits_read_from)(SpeexBits *bits, char *bytes, int len);

static dllfunction_t qspeexfuncs[] =
{
	{(void*)&qspeex_lib_get_mode, "speex_lib_get_mode"},
	{(void*)&qspeex_bits_init, "speex_bits_init"},
	{(void*)&qspeex_bits_reset, "speex_bits_reset"},
	{(void*)&qspeex_bits_write, "speex_bits_write"},

	{(void*)&qspeex_encoder_init, "speex_encoder_init"},
	{(void*)&qspeex_encoder_ctl, "speex_encoder_ctl"},
	{(void*)&qspeex_encode_int, "speex_encode_int"},

	{(void*)&qspeex_decoder_init, "speex_decoder_init"},
	{(void*)&qspeex_decoder_destroy, "speex_decoder_destroy"},
	{(void*)&qspeex_decode_int, "speex_decode_int"},
	{(void*)&qspeex_bits_read_from, "speex_bits_read_from"},

	{NULL}
};
static dllfunction_t qspeexdspfuncs[] =
{
	{(void*)&qspeex_preprocess_state_init, "speex_preprocess_state_init"},
	{(void*)&qspeex_preprocess_state_destroy, "speex_preprocess_state_destroy"},
	{(void*)&qspeex_preprocess_ctl, "speex_preprocess_ctl"},
	{(void*)&qspeex_preprocess_run, "speex_preprocess_run"},

	{NULL}
};
#endif

static qboolean S_SpeexDSP_Init(void)
{
#ifndef SPEEX_STATIC
	if (s_voip.speexdsp.inited)
		return s_voip.speexdsp.loaded;
	s_voip.speexdsp.inited = true;


	s_voip.speexdsp.speexdsplib = Sys_LoadLibrary("libspeexdsp", qspeexdspfuncs);
	if (!s_voip.speexdsp.speexdsplib)
	{
		Con_Printf("libspeexdsp not found. Your mic may be noisy.\n");
		return false;
	}
#endif

	s_voip.speexdsp.loaded = true;
	return s_voip.speexdsp.loaded;
}

static qboolean S_Speex_Init(void)
{
#ifndef SPEEX_STATIC
	if (s_voip.speex.inited)
		return s_voip.speex.loaded;
	s_voip.speex.inited = true;

	s_voip.speex.speexlib = Sys_LoadLibrary("libspeex", qspeexfuncs);
	if (!s_voip.speex.speexlib)
	{
		Con_Printf("libspeex not found. Voice chat is not available.\n");
		return false;
	}
#endif

	s_voip.speex.modenb = qspeex_lib_get_mode(SPEEX_MODEID_NB);
	s_voip.speex.modewb = qspeex_lib_get_mode(SPEEX_MODEID_WB);
	s_voip.speex.modeuwb = qspeex_lib_get_mode(SPEEX_MODEID_UWB);

	s_voip.speex.loaded = true;
	return s_voip.speex.loaded;
}
#endif

#ifdef AVAIL_OPENAL
extern snd_capture_driver_t OPENAL_Capture;
#endif
#ifdef _WIN32
snd_capture_driver_t fte_weakstruct DSOUND_Capture;
#endif
snd_capture_driver_t fte_weakstruct OSS_Capture;
snd_capture_driver_t fte_weakstruct SDL_Capture;

snd_capture_driver_t *capturedrivers[] =
{
#ifdef _WIN32
	&DSOUND_Capture,
#endif
	&SDL_Capture,
	&OSS_Capture,
#ifdef AVAIL_OPENAL
	&OPENAL_Capture,
#endif
	NULL
};

size_t PCMA_Decode(short *out, unsigned char *in, size_t samples)
{
	size_t i = 0;
	for (i = 0; i < samples; i++)
	{
		unsigned char inv = in[i]^0x55;	//g711 alaw inverts every other bit
		int m = inv&0xf;
		int e = (inv&0x70)>>4;
		if (e)
			m = (((m)<<1)|0x21) << (e-1);
		else
			m = (((m)<<1)|1);
		if (inv & 0x80)
			out[i] = -m;
		else
			out[i] = m;
	}
	return i;
}
size_t PCMA_Encode(unsigned char *out, size_t outsize, short *in, size_t samples)
{
	size_t i = 0;
	for (i = 0; i < samples; i++)
	{
		int o = in[i];
		unsigned char b;
		if (o < 0)
		{
			o = -o;
			b = 0x80;
		}
		else
			b = 0;

		if (o >= 0x0800)
			b |= ((o>>7)&0xf) | 0x70;
		else if (o >= 0x0400)
			b |= ((o>>6)&0xf) | 0x60;
		else if (o >= 0x0200)
			b |= ((o>>5)&0xf) | 0x50;
		else if (o >= 0x0100)
			b |= ((o>>4)&0xf) | 0x40;
		else if (o >= 0x0080)
			b |= ((o>>3)&0xf) | 0x30;
		else if (o >= 0x0040)
			b |= ((o>>2)&0xf) | 0x20;
		else if (o >= 0x0020)
			b |= ((o>>1)&0xf) | 0x10;
		else
			b |= ((o>>1)&0xf) | 0x00;
		out[i] = b^0x55;	//invert every-other bit.
	}

	return samples;
}
size_t PCMU_Decode(short *out, unsigned char *in, size_t samples)
{
	size_t i = 0;
	for (i = 0; i < samples; i++)
	{
		unsigned char inv = in[i]^0xff;
		int m = (((inv&0xf)<<1)|0x21) << ((inv&0x70)>>4);
		m -= 33;
		if (inv & 0x80)
			out[i] = -m;
		else
			out[i] = m;
	}
	return i;
}
size_t PCMU_Encode(unsigned char *out, size_t outsize, short *in, size_t samples)
{
	size_t i = 0;
	for (i = 0; i < samples; i++)
	{
		int o = in[i];
		unsigned char b;
		if (o < 0)
		{
			o = ~o;
			b = 0x80;
		}
		else
			b = 0;
		o+=33;

		if (o >= 0x1000)
			b |= ((o>>8)&0xf) | 0x70;
		else if (o >= 0x0800)
			b |= ((o>>7)&0xf) | 0x60;
		else if (o >= 0x0400)
			b |= ((o>>6)&0xf) | 0x50;
		else if (o >= 0x0200)
			b |= ((o>>5)&0xf) | 0x40;
		else if (o >= 0x0100)
			b |= ((o>>4)&0xf) | 0x30;
		else if (o >= 0x0080)
			b |= ((o>>3)&0xf) | 0x20;
		else if (o >= 0x0040)
			b |= ((o>>2)&0xf) | 0x10;
		else
			b |= ((o>>1)&0xf) | 0x00;
		out[i] = b^0xff;
	}

	return samples;
}

void S_Voip_Decode(unsigned int sender, unsigned int codec, unsigned int gen, unsigned char seq, unsigned int bytes, unsigned char *data)
{
	unsigned char *start;
	short decodebuf[8192];
	unsigned int decodesamps, len, drops;
	int r;

	if (sender >= MAX_CLIENTS)
		return;

	decodesamps = 0;
	drops = 0;
	start = data;

	s_voip.lastspoke[sender] = realtime + 0.5;
	if (s_voip.lastspoke[sender] > s_voip.lastspoke_any)
		s_voip.lastspoke_any = s_voip.lastspoke[sender];

	//if they re-started speaking, flush any old state to avoid things getting weirdly delayed and reset the codec properly.
	if (s_voip.decgen[sender] != gen || s_voip.deccodec[sender] != codec)
	{
		S_RawAudio(sender, NULL, s_voip.decsamplerate[sender], 0, 1, 2, 0);

		if (s_voip.deccodec[sender] != codec)
		{
			//make sure old state is closed properly.
			switch(s_voip.deccodec[sender])
			{
#ifdef HAVE_SPEEX
			case VOIP_SPEEX_OLD:
			case VOIP_SPEEX_NARROW:
			case VOIP_SPEEX_WIDE:
			case VOIP_SPEEX_ULTRAWIDE:
				qspeex_decoder_destroy(s_voip.decoder[sender]);
				break;
#endif
			case VOIP_RAW16:
				break;
#ifdef HAVE_OPUS
			case VOIP_OPUS:
				qopus_decoder_destroy(s_voip.decoder[sender]);
				break;
#endif
			}
			s_voip.decoder[sender] = NULL;
			s_voip.deccodec[sender] = VOIP_INVALID;
		}

		switch(codec)
		{
		default:	//codec not supported.
			return;
		case VOIP_RAW16:
			s_voip.decsamplerate[sender] = 11025;
			break;
		case VOIP_PCMA:
		case VOIP_PCMU:
			s_voip.decsamplerate[sender] = 8000;
			s_voip.decframesize[sender] = 8000/20;
			break;
#ifdef HAVE_SPEEX
		case VOIP_SPEEX_OLD:
		case VOIP_SPEEX_NARROW:
		case VOIP_SPEEX_WIDE:
		case VOIP_SPEEX_ULTRAWIDE:
			{
				const SpeexMode *smode;
				if (!S_Speex_Init())
					return;	//speex not usable.
				if (codec == VOIP_SPEEX_NARROW)
				{
					s_voip.decsamplerate[sender] = 8000;
					s_voip.decframesize[sender] = 160;
					smode = s_voip.speex.modenb;
				}
				else if (codec == VOIP_SPEEX_WIDE)
				{
					s_voip.decsamplerate[sender] = 16000;
					s_voip.decframesize[sender] = 320;
					smode = s_voip.speex.modewb;
				}
				else if (codec == VOIP_SPEEX_ULTRAWIDE)
				{
					s_voip.decsamplerate[sender] = 32000;
					s_voip.decframesize[sender] = 640;
					smode = s_voip.speex.modeuwb;
				}
				else
				{
					s_voip.decsamplerate[sender] = 11025;
					s_voip.decframesize[sender] = 160;
					smode = s_voip.speex.modenb;
				}
				if (!s_voip.decoder[sender])
				{
					qspeex_bits_init(&s_voip.speex.decbits[sender]);
					qspeex_bits_reset(&s_voip.speex.decbits[sender]);
					s_voip.decoder[sender] = qspeex_decoder_init(smode);
					if (!s_voip.decoder[sender])
						return;
				}
				else
					qspeex_bits_reset(&s_voip.speex.decbits[sender]);
			}
			break;
#endif
#ifdef HAVE_OPUS
		case VOIP_OPUS:
			if (!S_Opus_Init())
				return;

			//the lazy way to reset the codec!
			if (!s_voip.decoder[sender])
			{
				//opus outputs to 8, 12, 16, 24, or 48khz. pick whichever has least excess samples and resample to fit it.
				if (snd_speed <= 8000)
					s_voip.decsamplerate[sender] = 8000;
				else if (snd_speed <= 12000)
					s_voip.decsamplerate[sender] = 12000;
				else if (snd_speed <= 16000)
					s_voip.decsamplerate[sender] = 16000;
				else if (snd_speed <= 24000)
					s_voip.decsamplerate[sender] = 24000;
				else
					s_voip.decsamplerate[sender] = 48000;
				s_voip.decoder[sender] = qopus_decoder_create(s_voip.decsamplerate[sender], 1/*FIXME: support stereo where possible*/, NULL);
				if (!s_voip.decoder[sender])
					return;

				s_voip.decframesize[sender] = s_voip.decsamplerate[sender]/400;	//this is the maximum size in a single frame.
			}
			else
				qopus_decoder_ctl(s_voip.decoder[sender], OPUS_RESET_STATE);
			break;
#endif
		}
		s_voip.deccodec[sender] = codec;
		s_voip.decgen[sender] = gen;
		s_voip.decseq[sender] = seq;
		s_voip.declevel[sender] = 0;
	}


	//if there's packetloss, tell the decoder about the missing parts.
	//no infinite loops please.
	if ((unsigned)(seq - s_voip.decseq[sender]) > 10)
		s_voip.decseq[sender] = seq - 10;
	while(s_voip.decseq[sender] != seq)
	{
		if (decodesamps + s_voip.decframesize[sender] > sizeof(decodebuf)/sizeof(decodebuf[0]))
		{
			S_RawAudio(sender, (qbyte*)decodebuf, s_voip.decsamplerate[sender], decodesamps, 1, 2, snd_voip_play.value);
			decodesamps = 0;
		}
		switch(codec)
		{
		case VOIP_RAW16:
		case VOIP_PCMA:
		case VOIP_PCMU:
			break;
#ifdef HAVE_SPEEX
		case VOIP_SPEEX_OLD:
		case VOIP_SPEEX_NARROW:
		case VOIP_SPEEX_WIDE:
		case VOIP_SPEEX_ULTRAWIDE:
			qspeex_decode_int(s_voip.decoder[sender], NULL, decodebuf + decodesamps);
			decodesamps += s_voip.decframesize[sender];
			break;
#endif
#ifdef HAVE_OPUS
		case VOIP_OPUS:
			r = qopus_decode(s_voip.decoder[sender], NULL, 0, decodebuf + decodesamps, min(s_voip.decframesize[sender], sizeof(decodebuf)/sizeof(decodebuf[0]) - decodesamps), false);
			if (r > 0)
				decodesamps += r;
			break;
#endif
		}
		s_voip.decseq[sender]++;
	}

	while (bytes > 0)
	{
		if (decodesamps + s_voip.decframesize[sender] >= sizeof(decodebuf)/sizeof(decodebuf[0]))
		{
			S_RawAudio(sender, (qbyte*)decodebuf, s_voip.decsamplerate[sender], decodesamps, 1, 2, snd_voip_play.value);
			decodesamps = 0;
		}
		switch(codec)
		{
		default:
			bytes = 0;
			break;
#ifdef HAVE_SPEEX
		case VOIP_SPEEX_OLD:
		case VOIP_SPEEX_NARROW:
		case VOIP_SPEEX_WIDE:
		case VOIP_SPEEX_ULTRAWIDE:
			if (codec == VOIP_SPEEX_OLD)
			{	//older versions support only this, and require this extra bit.
				bytes--;
				len = *start++;
				if (bytes < len)
					break;
			}
			else
				len = bytes;
			qspeex_bits_read_from(&s_voip.speex.decbits[sender], start, len);
			bytes -= len;
			start += len;
			while (qspeex_decode_int(s_voip.decoder[sender], &s_voip.speex.decbits[sender], decodebuf + decodesamps) == 0)
			{
				decodesamps += s_voip.decframesize[sender];
				s_voip.decseq[sender]++;
				seq++;
				if (decodesamps + s_voip.decframesize[sender] >= sizeof(decodebuf)/sizeof(decodebuf[0]))
				{
					S_RawAudio(sender, (qbyte*)decodebuf, s_voip.decsamplerate[sender], decodesamps, 1, 2, snd_voip_play.value);
					decodesamps = 0;
				}
			}
			break;
#endif
		case VOIP_RAW16:
			len = min(bytes, sizeof(decodebuf)-(sizeof(decodebuf[0])*decodesamps));
			memcpy(decodebuf+decodesamps, start, len);
			decodesamps += len / sizeof(decodebuf[0]);
			s_voip.decseq[sender]++;
			bytes -= len;
			start += len;
			break;
		case VOIP_PCMA:
		case VOIP_PCMU:
			len = min(bytes, sizeof(decodebuf)-(sizeof(decodebuf[0])*decodesamps));
			if (len > s_voip.decframesize[sender]*2)
				len = s_voip.decframesize[sender]*2;
			if (codec == VOIP_PCMA)
				decodesamps += PCMA_Decode(decodebuf+decodesamps, start, len);
			else
				decodesamps += PCMU_Decode(decodebuf+decodesamps, start, len);
			s_voip.decseq[sender]++;
			bytes -= len;
			start += len;
			break;
#ifdef HAVE_OPUS
		case VOIP_OPUS:
			len = bytes;
			if (decodesamps > 0)
			{
				S_RawAudio(sender, (qbyte*)decodebuf, s_voip.decsamplerate[sender], decodesamps, 1, 2, snd_voip_play.value);
				decodesamps = 0;
			}
			r = qopus_decode(s_voip.decoder[sender], start, len, decodebuf + decodesamps, sizeof(decodebuf)/sizeof(decodebuf[0]) - decodesamps, false);
//			Con_Printf("Decoded %i frames from %i bytes\n", r, len);
			if (r > 0)
			{
				int frames = r / s_voip.decframesize[sender];
				decodesamps += r;
				s_voip.decseq[sender] = (s_voip.decseq[sender] + frames) & 0xff;
				seq = (seq+frames)&0xff;
			}
			else if (r < 0)
				Con_Printf("Opus decoding error %i\n", r);

			bytes -= len;
			start += len;
			break;
#endif
		}
	}

	if (drops)
		Con_DPrintf("%i dropped audio frames\n", drops);

	if (decodesamps > 0)
	{	//calculate levels of other people. eukara demanded this.
		float level = 0;
		float f;
		for (len = 0; len < decodesamps; len++)
		{
			f = decodebuf[len];
			level += f*f;
		}
		level = (3000*level) / (32767.0f*32767*decodesamps);
		s_voip.declevel[sender] = (s_voip.declevel[sender]*7 + level)/8;

		S_RawAudio(sender, (qbyte*)decodebuf, s_voip.decsamplerate[sender], decodesamps, 1, 2, snd_voip_play.value);
	}
}

#ifdef SUPPORT_ICE
static int S_Voip_NameToId(const char *codec)
{
	if (!Q_strcasecmp(codec, "speex@8000"))
		return VOIP_SPEEX_NARROW;
	else if (!Q_strcasecmp(codec, "speex@11025"))
		return VOIP_SPEEX_OLD;
	else if (!Q_strcasecmp(codec, "speex@16000"))
		return VOIP_SPEEX_WIDE;
	else if (!Q_strcasecmp(codec, "speex@32000"))
		return VOIP_SPEEX_ULTRAWIDE;
	else if (!Q_strcasecmp(codec, "opus") || !strcmp(codec, "opus@48000"))
		return VOIP_OPUS;
	else if (!Q_strcasecmp(codec, "pcma@8000"))
		return VOIP_PCMA;
	else if (!Q_strcasecmp(codec, "pcmu@8000"))
		return VOIP_PCMU;
	else
		return VOIP_INVALID;
}
qboolean S_Voip_RTP_CodecOkay(const char *codec)
{
	switch(S_Voip_NameToId(codec))
	{
#ifdef HAVE_SPEEX
	case VOIP_SPEEX_NARROW:
	case VOIP_SPEEX_OLD:
	case VOIP_SPEEX_WIDE:
	case VOIP_SPEEX_ULTRAWIDE:
		return S_Speex_Init();
#endif
	case VOIP_PCMA:
	case VOIP_PCMU:
		return true;
#ifdef HAVE_OPUS
	case VOIP_OPUS:
		return S_Opus_Init();
#endif
	default:
		return false;
	}
}
void S_Voip_RTP_Parse(unsigned short sequence, char *codec, unsigned char *data, unsigned int datalen)
{
	S_Voip_Decode(MAX_CLIENTS-1, S_Voip_NameToId(codec), 0, sequence&0xff, datalen, data);
}
qboolean NET_RTP_Transmit(unsigned int sequence, unsigned int timestamp, const char *codec, char *cdata, int clength);
qboolean NET_RTP_Active(void);
#else
#define NET_RTP_Active() false
#endif

void S_Voip_Parse(void)
{
	unsigned int sender;
	unsigned int bytes;
	unsigned char data[1024];
	unsigned char seq, gen;
	unsigned char codec;

	sender = MSG_ReadByte();
	gen = MSG_ReadByte();
	codec = gen>>4;
	gen &= 0x0f;
	seq = MSG_ReadByte();
	bytes = MSG_ReadShort();
	if (bytes > sizeof(data) || snd_voip_play.value <= 0)
	{
		MSG_ReadSkip(bytes);
		return;
	}
	MSG_ReadData(data, bytes);

	sender %= MAX_CLIENTS;

	//if testing, don't get confused if the server is echoing voice too!
	if (snd_voip_test.ival)
		if (sender == cl.playerview[0].playernum)
			return;

	S_Voip_Decode(sender, codec, gen, seq, bytes, data);
}
static float S_Voip_Preprocess(short *start, unsigned int samples, float micamp)
{
	int i;
	float level = 0, f;
	int framesize = s_voip.encframesize;
	while(samples >= framesize)
	{
#ifdef HAVE_SPEEX
		if (s_voip.speexdsp.preproc)
			qspeex_preprocess_run(s_voip.speexdsp.preproc, start);
#endif
		for (i = 0; i < framesize; i++)
		{
			f = start[i] * micamp;
			start[i] = bound(-32768, f, 32767);	//clamp it carefully, so it doesn't go to crap when given far too high a mic amp
			level += f*f;
		}

		start += framesize;
		samples -= framesize;
	}
	return level;
}
static void S_Voip_TryInitCaptureContext(char *driver, char *device, int rate)
{
	static float throttle;
	int i;

	s_voip.cdriver = NULL;

	/*Add new drivers in order of priority*/
	for (i = 0; capturedrivers[i]; i++)
	{
		if (capturedrivers[i]->Init && (!driver || !strcmp(capturedrivers[i]->drivername, driver)))
		{
			s_voip.cdriver = capturedrivers[i];

			s_voip.cdriverctx = s_voip.cdriver->Init(s_voip.encsamplerate, device);
			if (s_voip.cdriverctx)
			{
				//success!
				return;
			}
		}
	}

	if (!s_voip.cdriver)
	{
		if (!driver)
			Con_ThrottlePrintf(&throttle, 0, CON_ERROR"No microphone drivers supported\n");
		else
			Con_ThrottlePrintf(&throttle, 0, CON_ERROR"Microphone driver \"%s\" is not valid\n", driver);
	}
	else
		Con_ThrottlePrintf(&throttle, 0, CON_ERROR"No microphone detected\n");
	s_voip.cdriver = NULL;
}

static void S_Voip_InitCaptureContext(int rate)
{
	char *s;

	s_voip.cdriver = NULL;
	s_voip.cdriverctx = NULL;

	for (s = snd_voip_capturedevice.string; ; )
	{
		char *sep;
		s = COM_Parse(s);
		if (!*com_token)
			break;

		sep = strchr(com_token, ':');
		if (sep)
			*sep++ = 0;
		S_Voip_TryInitCaptureContext(com_token, sep, rate);
	}
	if (!s_voip.cdriver)
		S_Voip_TryInitCaptureContext(NULL, NULL, rate);
}

void S_Voip_Transmit(unsigned char clc, sizebuf_t *buf)
{
	unsigned char outbuf[8192];
	unsigned int outpos;//in bytes
	unsigned int encpos;//in bytes
	short *start;
	unsigned int initseq;//in frames
#ifdef SUPPORT_ICE
	unsigned int inittimestamp;//in samples
#endif
	unsigned int samps;
	float level;
	int len;
	float micamp = snd_voip_micamp.value;
	qboolean voipsendenable = true;
	int voipcodec = *snd_voip_codec.string?snd_voip_codec.ival:VOIP_DEFAULT_CODEC;
	qboolean rtpstream = NET_RTP_Active();

	if (buf)
	{
		/*if you're sending sound, you should be prepared to accept others yelling at you to shut up*/
		if (snd_voip_play.value <= 0)
			voipsendenable = false;
		/*don't send sound if its not supported. that'll break stuff*/
		if (!(cls.fteprotocolextensions2 & PEXT2_VOICECHAT))
			voipsendenable = false;
	}
	else
	{
		/*we're not sending it to a server. the above considerations don't matter*/
		voipsendenable = snd_voip_test.ival;
	}
	/*don't send sound if mic volume won't send anything anyway*/
	if (micamp <= 0)
		voipsendenable = false;

	if (rtpstream)
	{
		voipsendenable = true;
		//if rtp streaming is enabled, hack the codec to something better supported
#ifdef HAVE_SPEEX
		if (voipcodec == VOIP_SPEEX_OLD)
			voipcodec = VOIP_SPEEX_WIDE;
#endif
	}


	voicevolumemod = s_voip.lastspoke_any > realtime?snd_voip_ducking.value:1;
	voicevolumemod *= mastervolume.value;

	if (!voipsendenable || (voipcodec != s_voip.enccodec && s_voip.cdriver))
	{
		if (s_voip.cdriver)
		{
			if (s_voip.cdriverctx)
			{
				if (s_voip.wantsend)
				{
					s_voip.cdriver->Stop(s_voip.cdriverctx);
					s_voip.wantsend = false;
				}
				s_voip.cdriver->Shutdown(s_voip.cdriverctx);
				s_voip.cdriverctx = NULL;
			}
			s_voip.cdriver = NULL;
		}
		switch(s_voip.enccodec)
		{
#ifdef HAVE_SPEEX
		case VOIP_SPEEX_OLD:
		case VOIP_SPEEX_NARROW:
		case VOIP_SPEEX_WIDE:
		case VOIP_SPEEX_ULTRAWIDE:
			break;
#endif
#ifdef HAVE_OPUS
		case VOIP_OPUS:
			qopus_encoder_destroy(s_voip.encoder);
			break;
#endif
		}
		s_voip.encoder = NULL;
		s_voip.enccodec = VOIP_INVALID;

		if (!voipsendenable)
			return;
	}

	voipsendenable = voipbutton || (snd_voip_send.ival>0);

	if (!s_voip.cdriver)
	{
		s_voip.voiplevel = -1;
		/*only init the first time capturing is requested*/
		if (!voipsendenable)
			return;

		/*see if we can init our encoding codec...*/
		switch(voipcodec)
		{
#ifdef HAVE_SPEEX
		case VOIP_SPEEX_OLD:
		case VOIP_SPEEX_NARROW:
		case VOIP_SPEEX_WIDE:
		case VOIP_SPEEX_ULTRAWIDE:
			{
				const SpeexMode *smode;
				if (!S_Speex_Init())
				{
					Con_Printf("Unable to use speex codec - not installed\n");
					return;
				}

				if (voipcodec == VOIP_SPEEX_ULTRAWIDE)
					smode = s_voip.speex.modeuwb;
				else if (voipcodec == VOIP_SPEEX_WIDE)
					smode = s_voip.speex.modewb;
				else
					smode = s_voip.speex.modenb;
				qspeex_bits_init(&s_voip.speex.encbits);
				qspeex_bits_reset(&s_voip.speex.encbits);
				s_voip.encoder = qspeex_encoder_init(smode);
				if (!s_voip.encoder)
					return;
				qspeex_encoder_ctl(s_voip.encoder, SPEEX_GET_FRAME_SIZE, &s_voip.encframesize);
				qspeex_encoder_ctl(s_voip.encoder, SPEEX_GET_SAMPLING_RATE, &s_voip.encsamplerate);
				if (voipcodec == VOIP_SPEEX_NARROW)
					s_voip.encsamplerate = 8000;
				else if (voipcodec == VOIP_SPEEX_WIDE)
					s_voip.encsamplerate = 16000;
				else if (voipcodec == VOIP_SPEEX_ULTRAWIDE)
					s_voip.encsamplerate = 32000;
				else
					s_voip.encsamplerate = 11025;
				qspeex_encoder_ctl(s_voip.encoder, SPEEX_SET_SAMPLING_RATE, &s_voip.encsamplerate);
			}
			break;
#endif
		case VOIP_PCMA:
		case VOIP_PCMU:
			s_voip.encsamplerate = 8000;
			s_voip.encframesize = 8000/20;
			break;
		case VOIP_RAW16:
			s_voip.encsamplerate = 11025;
			s_voip.encframesize = 256;
			break;
#ifdef HAVE_OPUS
		case VOIP_OPUS:
			if (!S_Opus_Init())
			{
				Con_Printf("Unable to use opus codec - not installed\n");
				return;
			}

			//use whatever is convienient.
			s_voip.encsamplerate = 48000;
			s_voip.encframesize = s_voip.encsamplerate / 400;	//2.5ms frames, at a minimum.
			s_voip.encoder = qopus_encoder_create(s_voip.encsamplerate, 1, OPUS_APPLICATION_VOIP, NULL);
			if (!s_voip.encoder)
				return;

			s_voip.curbitrate = 0;

//			opus_encoder_ctl(s_voip.encoder, OPUS_SET_BITRATE(bitrate_bps));
//			opus_encoder_ctl(s_voip.encoder, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_NARROWBAND));
//			opus_encoder_ctl(s_voip.encoder, OPUS_SET_VBR(use_vbr));
//			opus_encoder_ctl(s_voip.encoder, OPUS_SET_VBR_CONSTRAINT(cvbr));
//			opus_encoder_ctl(s_voip.encoder, OPUS_SET_COMPLEXITY(complexity));
//			opus_encoder_ctl(s_voip.encoder, OPUS_SET_INBAND_FEC(use_inbandfec));
//			opus_encoder_ctl(s_voip.encoder, OPUS_SET_FORCE_CHANNELS(forcechannels));
//			opus_encoder_ctl(s_voip.encoder, OPUS_SET_DTX(use_dtx));
//			opus_encoder_ctl(s_voip.encoder, OPUS_SET_PACKET_LOSS_PERC(packet_loss_perc));

//			opus_encoder_ctl(s_voip.encoder, OPUS_GET_LOOKAHEAD(&skip));
//			opus_encoder_ctl(s_voip.encoder, OPUS_SET_LSB_DEPTH(16));
			break;
#endif
		default:
			Con_Printf("Unable to use that codec - not implemented\n");
			//can't start up other coedcs, cos we're too lame.
			return;
		}
		s_voip.enccodec = voipcodec;

		S_Voip_InitCaptureContext(s_voip.encsamplerate);	//sets cdriver+cdriverctx
	}

	/*couldn't init a driver?*/
	if (!s_voip.cdriverctx || !s_voip.cdriver)
	{
		return;
	}

	if (!voipsendenable && s_voip.wantsend)
	{
		s_voip.wantsend = false;
		s_voip.capturepos += s_voip.cdriver->Update(s_voip.cdriverctx, (unsigned char*)s_voip.capturebuf + s_voip.capturepos, 1, sizeof(s_voip.capturebuf) - s_voip.capturepos);
		s_voip.cdriver->Stop(s_voip.cdriverctx);
		/*note: we still grab audio to flush everything that was captured while it was active*/
	}
	else if (voipsendenable && !s_voip.wantsend)
	{
		s_voip.wantsend = true;
		if (!s_voip.capturepos)
		{	/*if we were actually still sending, it was probably only off for a single frame, in which case don't reset it*/
			s_voip.dumps = 0;
			s_voip.generation++;
			s_voip.encsequence = 0;

			//reset codecs so they start with a clean slate when new audio blocks are generated.
			switch(s_voip.enccodec)
			{
#ifdef HAVE_SPEEX
			case VOIP_SPEEX_OLD:
			case VOIP_SPEEX_NARROW:
			case VOIP_SPEEX_WIDE:
			case VOIP_SPEEX_ULTRAWIDE:
				qspeex_bits_reset(&s_voip.speex.encbits);
				break;
#endif
			case VOIP_RAW16:
				break;
#ifdef HAVE_OPUS
			case VOIP_OPUS:
				qopus_encoder_ctl(s_voip.encoder, OPUS_RESET_STATE);
				break;
#endif
			}
		}
		else
		{
			s_voip.capturepos += s_voip.cdriver->Update(s_voip.cdriverctx, (unsigned char*)s_voip.capturebuf + s_voip.capturepos, 1, sizeof(s_voip.capturebuf) - s_voip.capturepos);
		}
		s_voip.cdriver->Start(s_voip.cdriverctx);
	}

	if (s_voip.wantsend)
		voicevolumemod = min(voicevolumemod, snd_voip_capturingvol.value);

	s_voip.capturepos += s_voip.cdriver->Update(s_voip.cdriverctx, (unsigned char*)s_voip.capturebuf + s_voip.capturepos, s_voip.encframesize*2, sizeof(s_voip.capturebuf) - s_voip.capturepos);

	if (!voipsendenable || (!s_voip.wantsend && s_voip.capturepos < s_voip.encframesize*2))
	{
		s_voip.voiplevel = -1;
		s_voip.capturepos = 0;
		return;
	}

	initseq = s_voip.encsequence;
#ifdef SUPPORT_ICE
	inittimestamp = s_voip.enctimestamp;
#endif
	level = 0;
	samps=0;
	//*2 for 16bit audio input.
	for (encpos = 0, outpos = 0; encpos+s_voip.encframesize*2 <= s_voip.capturepos && outpos+256 < sizeof(outbuf); )
	{
		start = (short*)(s_voip.capturebuf + encpos);

#ifdef HAVE_SPEEX
		if (snd_voip_noisefilter.ival || snd_voip_autogain.ival)
		{
			if (!s_voip.speexdsp.preproc || snd_voip_noisefilter.modified || snd_voip_noisefilter.modified || s_voip.speexdsp.curframesize != s_voip.encframesize || s_voip.speexdsp.cursamplerate != s_voip.encsamplerate)
			{
				if (s_voip.speexdsp.preproc)
					qspeex_preprocess_state_destroy(s_voip.speexdsp.preproc);
				s_voip.speexdsp.preproc = NULL;
				if (S_SpeexDSP_Init())
				{
					int i;
					s_voip.speexdsp.preproc = qspeex_preprocess_state_init(s_voip.encframesize, s_voip.encsamplerate);
					i = snd_voip_noisefilter.ival;
					qspeex_preprocess_ctl(s_voip.speexdsp.preproc, SPEEX_PREPROCESS_SET_DENOISE, &i);
					i = snd_voip_autogain.ival;
					qspeex_preprocess_ctl(s_voip.speexdsp.preproc, SPEEX_PREPROCESS_SET_AGC, &i);

					s_voip.speexdsp.curframesize = s_voip.encframesize;
					s_voip.speexdsp.cursamplerate = s_voip.encsamplerate;
				}
			}
		}
		else if (s_voip.speexdsp.preproc)
		{
			qspeex_preprocess_state_destroy(s_voip.speexdsp.preproc);
			s_voip.speexdsp.preproc = NULL;
		}
#endif

		switch(s_voip.enccodec)
		{
#ifdef HAVE_SPEEX
		case VOIP_SPEEX_OLD:
			//this is from before I understood speex properly.
			level += S_Voip_Preprocess(start, s_voip.encframesize, micamp);
			qspeex_bits_reset(&s_voip.speex.encbits);
			qspeex_encode_int(s_voip.encoder, start, &s_voip.speex.encbits);
			len = qspeex_bits_write(&s_voip.speex.encbits, outbuf+(outpos+1), sizeof(outbuf) - (outpos+1));
			if (len < 0 || len > 255)
				len = 0;
			outbuf[outpos] = len;
			outpos += 1+len;
			s_voip.encsequence++;
			samps+=s_voip.encframesize;
			encpos += s_voip.encframesize*2;
			break;
		case VOIP_SPEEX_NARROW:
		case VOIP_SPEEX_WIDE:
		case VOIP_SPEEX_ULTRAWIDE:
			//write multiple speex frames into a single merged frame
			qspeex_bits_reset(&s_voip.speex.encbits);
			for (; encpos+s_voip.encframesize*2 <= s_voip.capturepos; )
			{
				start = (short*)(s_voip.capturebuf + encpos);
				level += S_Voip_Preprocess(start, s_voip.encframesize, micamp);
				qspeex_encode_int(s_voip.encoder, start, &s_voip.speex.encbits);
				s_voip.encsequence++;
				samps+=s_voip.encframesize;
				encpos += s_voip.encframesize*2;

				if (rtpstream)	//FIXME: why?
					break;
			}
			len = qspeex_bits_write(&s_voip.speex.encbits, outbuf+outpos, sizeof(outbuf) - outpos);
			outpos += len;
			break;
#endif
		case VOIP_RAW16:
			len = s_voip.capturepos-encpos;	//amount of data to be eaten in this frame
			len = min(len, sizeof(outbuf)-outpos);
			len &= ~((s_voip.encframesize*2)-1);
			level += S_Voip_Preprocess(start, len/2, micamp);
			memcpy(outbuf+outpos, start, len);	//'encode'
			outpos += len;			//bytes written to output
			encpos += len;			//number of bytes consumed

			s_voip.encsequence++;	//increment number of packets, for packetloss detection.
			samps+=len / 2;	//number of samplepairs eaten in this packet. for stats.
			break;
		case VOIP_PCMA:
		case VOIP_PCMU:
			//FIXME: what's with this /2? these are just 8-bit mono (logarithmic) pcm...
			len = s_voip.capturepos-encpos;	//amount of data to be eaten in this frame
			len = min(len, sizeof(outbuf)-outpos);
			len = min(len, s_voip.encframesize*2);
			level += S_Voip_Preprocess(start, len/2, micamp);
			if (s_voip.enccodec == VOIP_PCMA)
				outpos += PCMA_Encode(outbuf+outpos, sizeof(outbuf)-outpos, start, len/2);
			else
				outpos += PCMU_Encode(outbuf+outpos, sizeof(outbuf)-outpos, start, len/2);
			encpos += len;			//number of bytes consumed
			s_voip.encsequence++;	//increment number of packets, for packetloss detection.
			samps+=len / 2;	//number of samplepairs eaten in this packet. for stats.
			break;
#ifdef HAVE_OPUS
		case VOIP_OPUS:
			{
				//opus rtp only supports/allows a single chunk in each packet.
				int frames;
				int nrate;
				//densely pack the frames.
				start = (short*)(s_voip.capturebuf + encpos);
				frames = (s_voip.capturepos-encpos)/2;

				nrate = snd_voip_bitrate.value;
				if (nrate != s_voip.curbitrate)
				{
					s_voip.curbitrate = nrate;
					if (nrate == 0)
						nrate = -1000;
					qopus_encoder_ctl(s_voip.encoder, OPUS_SET_BITRATE_REQUEST, (int)nrate);
					nrate = 10000;
				}

				if (frames >= 2880)
					frames = 2880;
				else if (frames >= 1920 && nrate > 100)
					frames = 1920;
				else if (frames >= 960 && nrate > 500)
					frames = 960;
				else if (frames >= 480 && nrate > 1000)
					frames = 480;
				else if (snd_voip_send.ival & 4)
					break;	//don't send small rtp packets, its abusive.
				else if (frames >= 240 && nrate > 2000)
					frames = 240;
				else if (frames >= 120 && nrate > 4000)
					frames = 120;
				else
					break;	//invalid size, wait for more.

				level += S_Voip_Preprocess(start, frames, micamp);
				len = qopus_encode(s_voip.encoder, start, frames, outbuf+outpos, sizeof(outbuf) - outpos);
				if (len >= 0)
				{
					s_voip.encsequence += frames / s_voip.encframesize;
					outpos += len;
					samps+=frames;
					encpos += frames*2;
				}
				else
				{
					Con_Printf("Opus encoding error: %i\n", len);
					encpos = s_voip.capturepos;
				}
			}
			break;
#endif
		default:
			outbuf[outpos] = 0;
			break;
		}

		//opus has no way to detect the end properly.
		//standard rtp favours many small packets.
		if (rtpstream || s_voip.enccodec == VOIP_OPUS)
			break;
	}
	if (samps)
	{
		float nl;
		s_voip.enctimestamp += samps;
		nl = (3000*level) / (32767.0f*32767*samps);
		s_voip.voiplevel = (s_voip.voiplevel*7 + nl)/8;
		if (s_voip.voiplevel < snd_voip_vad_threshhold.ival && !voipbutton && !(snd_voip_send.ival & 6))
		{
			/*try and dump it, it was too quiet, and they're not pressing +voip*/
			if (s_voip.keeps > samps)
			{
				/*but not instantly*/
				s_voip.keeps -= samps;
			}
			else
			{
				outpos = 0;
				s_voip.dumps += samps;
				s_voip.keeps = 0;
			}
		}
		else
			s_voip.keeps = s_voip.encsamplerate * snd_voip_vad_delay.value;
		if (outpos)
		{
			if (s_voip.dumps > s_voip.encsamplerate/4)
				s_voip.generation++;
			s_voip.dumps = 0;
		}
	}

	if (outpos)
	{
		if (buf && !(snd_voip_send.ival & 4))
		{
			if (buf->maxsize - buf->cursize >= 5+outpos)
			{
				qbyte cgen = ((s_voip.enccodec&0x7)<<4) | (s_voip.generation & 0x0f);
				if (s_voip.enccodec >= 8 || 0)
					cgen |= 0x80;

				MSG_WriteByte(buf, clc);
				MSG_WriteByte(buf, cgen);
				MSG_WriteByte(buf, initseq&0xff);
				/*if (cgen & 0x80)
				{
					MSG_WriteShort(buf, 1+outpos);
					MSG_WriteByte(buf, s_voip.enccodec>>3);
				}
				else*/
					MSG_WriteShort(buf, outpos);	//even with codecs where the size is easy to determine, this is still useful for servers (which are unaware of the actual codec)
				SZ_Write(buf, outbuf, outpos);
			}
			else
				Con_Printf("Audio frame too small %i vs %i\n", outpos+4, buf->maxsize - buf->cursize);
		}

#ifdef SUPPORT_ICE
		if (rtpstream)
		{
			switch(s_voip.enccodec)
			{
#ifdef HAVE_SPEEX
			case VOIP_SPEEX_NARROW:
			case VOIP_SPEEX_WIDE:
			case VOIP_SPEEX_ULTRAWIDE:
			case VOIP_SPEEX_OLD:
				NET_RTP_Transmit(initseq, inittimestamp, va("speex@%i", s_voip.encsamplerate), outbuf, outpos);
				break;
#endif
			case VOIP_PCMA:
				NET_RTP_Transmit(initseq, inittimestamp, "pcma@8000", outbuf, outpos);
				break;
			case VOIP_PCMU:
				NET_RTP_Transmit(initseq, inittimestamp, "pcmu@8000", outbuf, outpos);
				break;
#ifdef HAVE_OPUS
			case VOIP_OPUS:
				NET_RTP_Transmit(initseq, inittimestamp, "opus@48000", outbuf, outpos);
				break;
#endif
			}
		}
#endif

		if (snd_voip_test.ival)
			S_Voip_Decode(cl.playerview[0].playernum, s_voip.enccodec, s_voip.generation & 0x0f, initseq&0xff, outpos, outbuf);

		//update our own lastspoke, so queries shows that we're speaking when we're speaking in a generic way, even if we can't hear ourselves.
		//but don't update general lastspoke, so ducking applies only when others speak. use capturingvol for yourself. they're more explicit that way.
		s_voip.lastspoke[cl.playerview[0].playernum] = realtime + 0.5;
	}

	/*remove sent data*/
	if (encpos)
	{
		memmove(s_voip.capturebuf, s_voip.capturebuf + encpos, s_voip.capturepos-encpos);
		s_voip.capturepos -= encpos;
	}
}
void S_Voip_Ignore(unsigned int slot, qboolean ignore)
{
	CL_SendClientCommand(true, "vignore %i %i", slot, ignore);
}
static void S_Voip_Enable_f(void)
{
	if (Cmd_IsInsecure())
		return;
	voipbutton = true;
}
static void S_Voip_Disable_f(void)
{
	voipbutton = false;
}
static void S_Voip_f(void)
{
#ifdef HAVE_SPEEX
	if (!strcmp(Cmd_Argv(1), "maxgain"))
	{
		int i = atoi(Cmd_Argv(2));
		if (s_voip.speexdsp.preproc)
			qspeex_preprocess_ctl(s_voip.speexdsp.preproc, SPEEX_PREPROCESS_SET_AGC_MAX_GAIN, &i);
	}
	else
#endif
	{
		Con_Printf("unrecognised parameter \"%s\"\n", Cmd_Argv(1));
	}
}
static void QDECL S_Voip_Play_Callback(cvar_t *var, char *oldval)
{
	if (cls.fteprotocolextensions2 & PEXT2_VOICECHAT)
	{
		if (var->value > 0)
			CL_SendClientCommand(true, "unmuteall");
		else
			CL_SendClientCommand(true, "muteall");
	}
}
void S_Voip_MapChange(void)
{
	voipbutton = false;
	Cvar_ForceCallback(&snd_voip_play);
}
int S_Voip_Loudness(qboolean ignorevad)
{
	if (!s_voip.cdriverctx || (!ignorevad && s_voip.dumps))
		return -1;
	if (s_voip.voiplevel > 100)
		return 100;
	return s_voip.voiplevel;
}
int S_Voip_ClientLoudness(unsigned int plno)
{
	if (plno >= MAX_CLIENTS)
		return 0;
	if (s_voip.lastspoke[plno] > realtime)
		return s_voip.declevel[plno];
	return -1;
}
qboolean S_Voip_Speaking(unsigned int plno)
{
	if (plno >= MAX_CLIENTS)
		return false;
	return s_voip.lastspoke[plno] > realtime;
}

void S_Voip_Init(void)
{
	int i;
	for (i = 0; i < MAX_CLIENTS; i++)
		s_voip.deccodec[i] = VOIP_INVALID;
	s_voip.enccodec = VOIP_INVALID;

	Cvar_Register(&snd_voip_capturedevice,		"Voice Chat");
	Cvar_Register(&snd_voip_capturedevice_opts,		"Voice Chat");
	Cvar_Register(&snd_voip_send,		"Voice Chat");
	Cvar_Register(&snd_voip_vad_threshhold,	"Voice Chat");
	Cvar_Register(&snd_voip_vad_delay,	"Voice Chat");
	Cvar_Register(&snd_voip_capturingvol,	"Voice Chat");
	Cvar_Register(&snd_voip_showmeter,	"Voice Chat");
	Cvar_Register(&snd_voip_play,		"Voice Chat");
	Cvar_Register(&snd_voip_test,		"Voice Chat");
	Cvar_Register(&snd_voip_ducking,		"Voice Chat");
	Cvar_Register(&snd_voip_micamp,		"Voice Chat");
	Cvar_Register(&snd_voip_codec,		"Voice Chat");
#ifdef HAVE_SPEEX
	Cvar_Register(&snd_voip_noisefilter,		"Voice Chat");
	Cvar_Register(&snd_voip_autogain,		"Voice Chat");
#endif
	Cvar_Register(&snd_voip_bitrate,		"Voice Chat");
	Cmd_AddCommand("+voip", S_Voip_Enable_f);
	Cmd_AddCommand("-voip", S_Voip_Disable_f);
	Cmd_AddCommand("voip", S_Voip_f);
}
#else
void S_Voip_Parse(void)
{
	unsigned int bytes;

	MSG_ReadByte();
	MSG_ReadByte();
	MSG_ReadByte();
	bytes = MSG_ReadShort();
	MSG_ReadSkip(bytes);
}
int S_Voip_ClientLoudness(unsigned int plno)
{
	return -1;
}
#endif



void S_DefaultSpeakerConfiguration(soundcardinfo_t *sc)
{
	sc->dist[0] = 1;
	sc->dist[1] = 1;
	sc->dist[2] = 1;
	sc->dist[3] = 1;
	sc->dist[4] = 1;
	sc->dist[5] = 1;

	switch (sc->sn.numchannels)
	{
	case 1:
		VectorSet(sc->speakerdir[0], 0, 0, 0);
		break;
	case 2:
	case 3:
		VectorSet(sc->speakerdir[0], 0, -1, 0);
		VectorSet(sc->speakerdir[1], 0, 1, 0);
		VectorSet(sc->speakerdir[2], 0, 0, 0);
		break;
	case 4: // quad
	case 5:
		VectorSet(sc->speakerdir[0], 0.7, -0.7, 0);
		VectorSet(sc->speakerdir[1], 0.7, 0.7, 0);
		VectorSet(sc->speakerdir[2], -0.7, -0.7, 0);
		VectorSet(sc->speakerdir[3], -0.7, 0.7, 0);
		VectorSet(sc->speakerdir[4], 0, 0, 0);
		break;
	case 6: // 5.1
	case 7:
		VectorSet(sc->speakerdir[0], 0.7, -0.7, 0);	//front-left
		VectorSet(sc->speakerdir[1], 0.7, 0.7, 0);	//front-right
		VectorSet(sc->speakerdir[2], 1, 0, 0);		//center
		VectorSet(sc->speakerdir[3], 0, 0, 0);		//bass
		VectorSet(sc->speakerdir[4], -0.7, -0.7, 0);//back-left
		VectorSet(sc->speakerdir[5], -0.7, 0.7, 0);	//back-right
		VectorSet(sc->speakerdir[6], 0, 0, 0);
		break;
	case 8: // 7.1
	default:
		VectorSet(sc->speakerdir[0], 0.7, -0.7, 0);
		VectorSet(sc->speakerdir[1], 0.7, 0.7, 0);
		VectorSet(sc->speakerdir[2], 1, 0, 0);
		VectorSet(sc->speakerdir[3], 0, 0, 0);
		VectorSet(sc->speakerdir[4], -0.7, -0.7, 0);
		VectorSet(sc->speakerdir[5], -0.7, 0.7, 0);
		VectorSet(sc->speakerdir[6], 0, -1, 0);
		VectorSet(sc->speakerdir[7], 0, 1, 0);
		break;
	}
}

#ifdef AVAIL_WASAPI
extern sounddriver_t WASAPI_Output;
#endif
#ifdef AVAIL_XAUDIO2
extern sounddriver_t XAUDIO2_Output;
#endif
#ifdef AVAIL_DSOUND
extern sounddriver_t DSOUND_Output;
#endif
sounddriver_t fte_weakstruct SDL_Output;
#if defined(__unix__) && !defined(__APPLE__) // Alsa, OSS and PulseAudio can all be installed on most Unixes these days; They can fall back to OSS_Output if needed - Brad
extern sounddriver_t ALSA_Output;
extern sounddriver_t Pulse_Output;
#endif
sounddriver_t fte_weakstruct OSS_Output;
#ifdef AVAIL_OPENAL
extern sounddriver_t OPENAL_Output;
extern sounddriver_t OPENAL_Output_Lame;
#endif
#ifdef __DJGPP__
extern sounddriver_t SBLASTER_Output;
#endif
#if defined(_WIN32) && !defined(WINRT) && !defined(FTE_SDL)
extern sounddriver_t WaveOut_Output;
#endif

#ifdef MACOSX
sounddriver_t fte_weakstruct MacOS_AudioOutput;	//prefered on mac
#endif
#ifdef ANDROID
sounddriver_t fte_weakstruct OSL_Output;			//general audio library, but android has all kinds of quirks.
sounddriver_t fte_weakstruct Droid_AudioOutput;
#endif
#if defined(__MORPHOS__)
sounddriver_t fte_weakstruct AHI_AudioOutput;		//prefered on morphos
#endif
sounddriver_t fte_weakstruct SNDIO_AudioOutput;	//bsd

//in order of preference
static sounddriver_t *outputdrivers[] =
{
#ifdef AVAIL_OPENAL
	&OPENAL_Output,	//refuses to run as the default device, at least until its perfected.
#endif

#ifdef HAVE_MIXER
#ifdef AVAIL_DSOUND
	&DSOUND_Output,
#endif
#ifdef AVAIL_XAUDIO2
	&XAUDIO2_Output,
#endif
#ifdef AVAIL_WASAPI
	&WASAPI_Output,	//this is last, so that we can default to exclusive. woot.
#endif

	&SDL_Output,		//prefered on linux. distros can ensure that its configured correctly.
#ifdef AUDIO_PULSE
	&Pulse_Output,		//wasteful, and availability generally means Alsa is broken/defective.
#endif
#ifdef AUDIO_ALSA
	&ALSA_Output,		//pure shite, and availability generally means OSS is broken/defective.
#endif
#ifdef AUDIO_OSS
	&OSS_Output,		//good for low latency audio, but not likely to work any more on linux (unlike every other unix system with a decent opengl driver)
#endif
#ifdef __DJGPP__
	&SBLASTER_Output,	//zomgwtfdos?
#endif
#if defined(_WIN32) && !defined(WINRT) && !defined(FTE_SDL)
	&WaveOut_Output,	//doesn't work properly in vista, etc.
#endif

#ifdef MACOSX
	&MacOS_AudioOutput,	//prefered on mac
#endif
#ifdef ANDROID
	&OSL_Output,		//opensl(es)
#endif
#if defined(__MORPHOS__)
	&AHI_AudioOutput,	//prefered on morphos
#endif
	&SNDIO_AudioOutput,	//prefered on OpenBSD

#ifdef AVAIL_OPENAL
	&OPENAL_Output_Lame,//streaming quake's audio via openal instead of using openal properly. used in our browser port to work around issues with webaudio (at least in chromium).
#endif
#endif
	NULL
};

static soundcardinfo_t *SNDDMA_Init(char *driver, char *device, int seat)
{
	soundcardinfo_t *sc = Z_Malloc(sizeof(soundcardinfo_t));
	sounddriver_t *sd;
	int i;
	int st;

	memset(sc, 0, sizeof(*sc));
	sc->seat = seat;

	sc->next = sndcardinfo;
	sndcardinfo = sc;

	// set requested rate
	if (snd_khz.ival >= 1000)
		sc->sn.speed = snd_khz.ival;
	else if (snd_khz.ival <= 0)
		sc->sn.speed = 22050;
/*	else if (snd_khz.ival >= 195)
		sc->sn.speed = 200000;
	else if (snd_khz.ival >= 180)
		sc->sn.speed = 192000;
	else if (snd_khz.ival >= 90)
		sc->sn.speed = 96000; */
	else if (snd_khz.ival >= 45)
		sc->sn.speed = 48000;
	else if (snd_khz.ival >= 30)
		sc->sn.speed = 44100;
	else if (snd_khz.ival >= 20)
		sc->sn.speed = 22050;
	else if (snd_khz.ival >= 10)
		sc->sn.speed = 11025;
	else
		sc->sn.speed = 8000;

	// set requested speaker count
	if (snd_speakers.ival > MAXSOUNDCHANNELS)
		sc->sn.numchannels = MAXSOUNDCHANNELS;
	else if (snd_speakers.ival > 1)
		sc->sn.numchannels = (int)snd_speakers.ival;
	else
		sc->sn.numchannels = 1;

	// set requested sample bits
	if (snd_samplebits.ival >= 32)
		sc->sn.samplebytes = 4;
	else if (snd_samplebits.ival >= 16)
		sc->sn.samplebytes = 2;
	else
		sc->sn.samplebytes = 1;

	// set requested buffer size
	if (snd_buffersize.ival > 0)
		sc->sn.samples = snd_buffersize.ival * sc->sn.numchannels;
	else
		sc->sn.samples = 0;

	for (i = 0; outputdrivers[i]; i++)
	{
		sd = outputdrivers[i];
		if (sd && sd->name && (!driver || !Q_strcasecmp(sd->name, driver)))
		{
			//skip drivers which are not present.
			if (!sd->InitCard)
				continue;

			st = (**sd->InitCard)(sc, device);
			if (st)
			{
				if (!sc->sn.sampleformat)
				{
					Con_TPrintf("S_Startup: Ignoring soundcard %s due to unspecified sample format.\n", sc->name);
					S_ShutdownCard(sc);
					continue;
				}
				S_DefaultSpeakerConfiguration(sc);
				if (snd_speed)
				{	//if the sample speeds of multiple soundcards do not match, it'll fail.
					if (snd_speed != sc->sn.speed)
					{
						Con_TPrintf("S_Startup: Ignoring soundcard %s due to mismatched sample speeds.\n", sc->name);
						S_ShutdownCard(sc);
						return NULL;
					}
				}
				else
					snd_speed = sc->sn.speed;

				if (sc->seat == -1 && sc->ListenerUpdate)
					sc->seat = 0;	//hardware rendering won't cope with seat=-1

				Z_ReallocElements((void**)&sc->channel, &sc->max_chans, NUM_AMBIENTS+NUM_MUSICS, sizeof(*sc->channel));
				return sc;
			}
		}
	}

	S_ShutdownCard(sc);

	if (!driver)
		Con_TPrintf("Could not start audio device \"%s\"\n", device?device:"default");
	else
		Con_TPrintf("Could not start \"%s\" device \"%s\"\n", driver, device?device:"default");
	return NULL;
}

soundcardinfo_t *S_SetupDeviceSeat(char *driver, char *device, int seat)
{
	return SNDDMA_Init(driver, device, seat);
	/*
	soundcardinfo_t *sc;
	for (sc = sndcardinfo; sc; sc = sc->next)
	{
		sc->seat = seat;
	}*/
}

static void QDECL S_EnumeratedOutDevice(const char *driver, const char *devicecode, const char *readabledevice)
{
	const char *fullintname;
	char opts[8192];
	char nbuf[1024];
	char dbuf[1024];
	
	if (devicecode)
		fullintname = va("%s:%s", driver, devicecode);
	else
		fullintname = driver;

	Q_snprintfz(opts, sizeof(opts), "%s%s%s %s", snd_device_opts.string, *snd_device_opts.string?" ":"", COM_QuotedString(fullintname, nbuf, sizeof(nbuf), false), COM_QuotedString(readabledevice, dbuf, sizeof(dbuf), false));
	Cvar_ForceSet(&snd_device_opts, opts);
}
#ifdef VOICECHAT
static void QDECL S_Voip_EnumeratedCaptureDevice(const char *driver, const char *devicecode, const char *readabledevice)
{
	const char *fullintname;
	char opts[8192];
	char nbuf[1024];
	char dbuf[1024];

	if (devicecode)
		fullintname = va("%s:%s", driver, devicecode);
	else
		fullintname = driver;
	
	Q_snprintfz(opts, sizeof(opts), "%s%s%s %s", snd_voip_capturedevice_opts.string, *snd_voip_capturedevice_opts.string?" ":"", COM_QuotedString(fullintname, nbuf, sizeof(nbuf), false), COM_QuotedString(readabledevice, dbuf, sizeof(dbuf), false));
	Cvar_ForceSet(&snd_voip_capturedevice_opts, opts);
}
#endif
void S_EnumerateDevices(void)
{
	int i;
	sounddriver_t *sd;
	qboolean safe = COM_CheckParm("-noenumerate") || COM_CheckParm("-safe");

	Cvar_ForceSet(&snd_device_opts, "");
	S_EnumeratedOutDevice("", NULL, "Default");
	S_EnumeratedOutDevice("none", NULL, "None");

	for (i = 0; outputdrivers[i]; i++)
	{
		sd = outputdrivers[i];
		if (sd && sd->name)
		{
			if (safe || !sd->Enumerate || !sd->Enumerate(S_EnumeratedOutDevice))
				S_EnumeratedOutDevice(sd->name, "", va("Default %s", sd->name));
		}
	}

#ifdef VOICECHAT
	Cvar_ForceSet(&snd_voip_capturedevice_opts, "");
	S_Voip_EnumeratedCaptureDevice("", NULL, "Default");
	for (i = 0; capturedrivers[i]; i++)
	{
		if (!capturedrivers[i]->Init)
			continue;
		if (safe || !capturedrivers[i]->Enumerate || !capturedrivers[i]->Enumerate(S_Voip_EnumeratedCaptureDevice))
			S_Voip_EnumeratedCaptureDevice(capturedrivers[i]->drivername, NULL, va("Default %s", capturedrivers[i]->drivername));
	}
#endif
}

/*
================
S_Startup
================
*/

void S_ClearRaw(void);
void S_Startup (void)
{
	qboolean nodefault = false;
	char *s;

	if (!snd_initialized)
		return;

	if (sound_started)
		S_Shutdown(false);

	snd_blocked = 0;
	snd_speed = 0;

	S_UpdateReverb(0, NULL, 0);
	{	//we can actually use underwater hints automatically easily enough. q3 also does this.
		//its other things that are more awkward.
		struct reverbproperties_s underwater = REVERB_PRESET_UNDERWATER;
		S_UpdateReverb(1, &underwater, sizeof(underwater));
	}

	for (s = snd_device.string; ; )
	{
		char *sep;
		s = COM_Parse(s);
		if (!*com_token)
			break;

		if (!Q_strcasecmp(com_token, "none"))
			nodefault = true;
		else
		{
			sep = strchr(com_token, ':');
			if (sep)
				*sep++ = 0;
			SNDDMA_Init(com_token, sep, -1);
		}
	}
	if (!sndcardinfo && !nodefault)
	{
#if defined(_WIN32) && !defined(FTE_SDL)
		INS_SetupControllerAudioDevices(true);
#endif
		if (!sndcardinfo)
			SNDDMA_Init(NULL, NULL, -1);
	}

	sound_started = true;

	S_ClearRaw();

	if (!known_sfx)
		known_sfx = Z_Malloc(MAX_SFX*sizeof(sfx_t));
	num_sfx = 0;

	CL_InitTEntSounds();

	ambient_sfx[AMBIENT_WATER] = S_PrecacheSound ("ambience/water1.wav");
	ambient_sfx[AMBIENT_SKY] = S_PrecacheSound ("ambience/wind2.wav");
}

//why isn't this part of S_Restart_f anymore?
//so that the video code can call it directly without flushing the models it's just loaded.
void S_DoRestart (qboolean onlyifneeded)
{
	int i;
	if (onlyifneeded && sound_started)
		return;	//don't need to if its already running.

	S_StopAllSounds (true);
	S_Shutdown(false);

	if (nosound.ival)
		return;

	S_Startup();

	S_StopAllSounds (true);


	for (i=1 ; i<MAX_PRECACHE_SOUNDS ; i++)
	{
		if (!cl.sound_name[i])
			break;
		cl.sound_precache[i] = S_FindName (cl.sound_name[i], true, false);
	}
}

void S_Restart_f (void)
{
	S_EnumerateDevices();

	S_DoRestart(false);
}

void S_Control_f (void)
{
	int i;
	char *command;

	command = Cmd_Argv (1);

	if (!Q_strcasecmp(command, "off"))
	{
		Cache_Flush();//forget the old sounds.

		S_StopAllSounds (true);

		S_Shutdown(false);
		sound_started = 0;
	}

	if (!Q_strcasecmp(command, "rate") || !Q_strcasecmp(command, "speed"))
	{
		Cvar_SetValue(&snd_khz, atof(Cmd_Argv (2))/1000);
		S_Restart_f();
		return;
	}

	//individual device control
	if (!Q_strncasecmp(command, "card", 4))
	{
		int card;
		soundcardinfo_t *sc;
		card = atoi(command+4);

		for (i = 0, sc = sndcardinfo; i < card && sc; i++,sc=sc->next)
			;

		if (!sc)
		{
			Con_Printf("Sound card %i is invalid (try resetting first)\n", card);
			return;
		}
		if (Cmd_Argc() < 3)
		{
			Con_Printf("Scard %i is %s\n", card, sc->name);
			return;
		}
		command = Cmd_Argv (2);
		if (!Q_strcasecmp(command, "mono"))
		{
			for (i = 0; i < MAXSOUNDCHANNELS; i++)
			{
				VectorSet(sc->speakerdir[i], 0, 0, 0);
				sc->dist[i] = 1;
			}
		}
		else if (!Q_strcasecmp(command, "standard") || !Q_strcasecmp(command, "stereo"))
		{
			for (i = 0; i < MAXSOUNDCHANNELS; i++)
			{
				VectorSet(sc->speakerdir[i], 0, (i&1)?1:-1, 0);
				sc->dist[i] = 1;
			}
		}
		else if (!Q_strcasecmp(command, "swap"))
		{
			for (i = 0; i < MAXSOUNDCHANNELS; i++)
			{
				sc->speakerdir[i][1] *= -1;
			}
		}
		else if (!Q_strcasecmp(command, "front"))
		{
			for (i = 0; i < MAXSOUNDCHANNELS; i++)
			{
				VectorSet(sc->speakerdir[i], 0.7, (i&1)?-0.7:0.7, 0);
				sc->dist[i] = 1;
			}
		}
		else if (!Q_strcasecmp(command, "back"))
		{
			for (i = 0; i < MAXSOUNDCHANNELS; i++)
			{
				VectorSet(sc->speakerdir[i], -0.7, (i&1)?-0.7:0.7, 0);
				sc->dist[i] = 1;
			}
		}
		return;
	}
	else
		Con_Printf("valid commands are: off, single, multi, cardX mono, cardX stereo, cardX front, cardX back\n");
}

/*
================
S_Init
================
*/
void S_Init (void)
{
	int p, i;

	Con_DPrintf("\nSound Initialization\n");

	Cmd_AddCommand("play", S_Play_f);	//sound that doesn't follow the player
	Cmd_AddCommand("play2", S_Play_f);	//sound that DOES follow the player
	Cmd_AddCommand("playvol", S_Play_f);
	Cmd_AddCommand("stopsound", S_StopAllSounds_f);
	Cmd_AddCommand("soundlist", S_SoundList_f);
	Cmd_AddCommand("soundinfo", S_SoundInfo_f);

	Cmd_AddCommand("snd_restart", S_Restart_f);

	Cmd_AddCommand("soundcontrol", S_Control_f);

	Cvar_Register(&nosound,				"Sound controls");
	Cvar_Register(&mastervolume,		"Sound controls");
	Cvar_Register(&volume,				"Sound controls");
	Cvar_Register(&snd_precache,		"Sound controls");
	Cvar_Register(&snd_loadas8bit,		"Sound controls");
	Cvar_Register(&snd_loadasstereo,	"Sound controls");
	Cvar_Register(&bgmvolume,			"Sound controls");
	Cvar_Register(&snd_nominaldistance,	"Sound controls");
	Cvar_Register(&ambient_level,		"Sound controls");
	Cvar_Register(&ambient_fade,		"Sound controls");
	Cvar_Register(&snd_noextraupdate,	"Sound controls");
	Cvar_Register(&snd_show,			"Sound controls");
	Cvar_Register(&_snd_mixahead,		"Sound controls");
	Cvar_Register(&snd_khz,				"Sound controls");
	Cvar_Register(&snd_leftisright,		"Sound controls");
	Cvar_Register(&snd_eax,				"Sound controls");
	Cvar_Register(&snd_speakers,		"Sound controls");
	Cvar_Register(&snd_buffersize,		"Sound controls");
	Cvar_Register(&snd_samplebits,		"Sound controls");
	Cvar_Register(&snd_playbackrate,	"Sound controls");
	Cvar_Register(&snd_ignoregamespeed,	"Sound controls");
	Cvar_Register(&snd_doppler,			"Sound controls");
	Cvar_Register(&snd_doppler_min,		"Sound controls");
	Cvar_Register(&snd_doppler_max,		"Sound controls");

	Cvar_Register(&snd_inactive,		"Sound controls");

#ifdef MULTITHREAD
	Cvar_Register(&snd_mixerthread,				"Sound controls");
#endif
	Cvar_Register(&snd_playersoundvolume,		"Sound controls");
	Cvar_Register(&snd_device,		"Sound controls");
	Cvar_Register(&snd_device_opts,		"Sound controls");

	Cvar_Register(&snd_ignorecueloops, "Sound controls");
	Cvar_Register(&snd_linearresample, "Sound controls");
	Cvar_Register(&snd_linearresample_stream, "Sound controls");

#ifdef VOICECHAT
	S_Voip_Init();
#endif

#ifdef MULTITHREAD
	mixermutex = Sys_CreateMutex();
#endif

	for (i = 0; outputdrivers[i]; i++)
	{
		sounddriver_t *sd = outputdrivers[i];
		if (sd && sd->name && sd->RegisterCvars)
			sd->RegisterCvars();
	}

	if (COM_CheckParm("-nosound"))
	{
		Cvar_ForceSet(&nosound, "1");
		nosound.flags |= CVAR_NOSET;
		return;
	}

	S_EnumerateDevices();

	p = COM_CheckParm ("-soundspeed");
	if (!p)
		p = COM_CheckParm ("-sspeed");
	if (!p)
		p = COM_CheckParm ("-sndspeed");
	if (p)
	{
		if (p < com_argc-1)
			Cvar_SetValue(&snd_khz, atof(com_argv[p+1]));
		else
			Sys_Error ("S_Init: you must specify a speed in KB after -soundspeed");
	}

	snd_initialized = true;

	known_sfx = Z_Malloc(MAX_SFX*sizeof(sfx_t));
	num_sfx = 0;
}


// =======================================================================
// Shutdown sound engine
// =======================================================================

void S_ShutdownCard(soundcardinfo_t *sc)
{
	soundcardinfo_t **link;

	for (link = &sndcardinfo; *link; link = &(*link)->next)
	{
		if (*link == sc)
		{
			*link = sc->next;
			if (sc->Shutdown)
				sc->Shutdown(sc);
			Z_Free(sc->channel);
			Z_Free(sc);
			break;
		}
	}
}
void S_Shutdown(qboolean final)
{
	soundcardinfo_t *sc, *next;

#if defined(_WIN32) && !defined(FTE_SDL)
	INS_SetupControllerAudioDevices(false);
#endif

	for (sc = sndcardinfo; sc; sc=next)
	{
		next = sc->next;
		sc->Shutdown(sc);
		Z_Free(sc->channel);
		Z_Free(sc);
		sndcardinfo = next;
	}

	sound_started = 0;
	S_Purge(false);

	Z_Free(known_sfx);
	known_sfx = NULL;
	num_sfx = 0;

	if (final)
	{
		Z_Free(reverbproperties);
		reverbproperties = NULL;
		numreverbproperties = 0;
	}

#ifdef MULTITHREAD
	if (final && mixermutex)
	{
		Sys_DestroyMutex(mixermutex);
		mixermutex = NULL;
	}
#endif
}


// =======================================================================
// Load a sound
// =======================================================================

/*
==================
S_FindName

also touches it
==================
*/
sfx_t *S_FindName (const char *name, qboolean create, qboolean syspath)
{
	int		i;
	sfx_t	*sfx;

	if (!name)
		Sys_Error ("S_FindName: NULL\n");

	if (Q_strlen(name) >= MAX_OSPATH)
		Sys_Error ("Sound name too long: %s", name);

// see if already loaded
	for (i=0 ; i < num_sfx ; i++)
		if (!Q_strcmp(known_sfx[i].name, name) && known_sfx[i].syspath == syspath)
		{
			known_sfx[i].touched = true;
			return &known_sfx[i];
		}

	if (num_sfx == MAX_SFX)
		Sys_Error ("S_FindName: out of sfx_t");

	if (create && known_sfx)
	{
		sfx = &known_sfx[i];
		strcpy (sfx->name, name);
		sfx->syspath = syspath;
		sfx->touched = true;

		num_sfx++;
	}
	else
		sfx = NULL;

	return sfx;
}

void S_Purge(qboolean retaintouched)
{
	sfx_t	*sfx;
	int i;

	//make sure ambients are kept. silly ambients.
	if (retaintouched)
	{
		ambient_sfx[AMBIENT_WATER] = S_PrecacheSound ("ambience/water1.wav");
		ambient_sfx[AMBIENT_SKY] = S_PrecacheSound ("ambience/wind2.wav");
	}

	if (!num_sfx)
		return;

	S_LockMixer();
	for (i=0 ; i < num_sfx ; i++)
	{
		sfx = &known_sfx[i];
		/*don't hurt sounds if they're being processed by a worker thread*/
		if (sfx->loadstate == SLS_LOADING)
		{
			if (retaintouched)
				continue;	//don't bother waiting

			//trying to shut down or something.
			//make sure there's no worker about to write to sfx after the memory is freed
			COM_WorkerPartialSync(sfx, &sfx->loadstate, SLS_LOADING);
		}

		/*don't purge the file if its still relevent*/
		if (retaintouched && sfx->touched)
			continue;

		if (S_IsPlayingSomewhere(sfx))
			continue;	//eep?!?

		sfx->loadstate = SLS_NOTLOADED;
		/*nothing to do if there's no data within*/
		if (!sfx->decoder.buf)
			continue;
		/*stop the decoder first*/
		if (sfx->decoder.purge)
			sfx->decoder.purge(sfx);
		else if (sfx->decoder.ended)
			sfx->decoder.ended(sfx);

		/*if there's any data associated still, kill it. if present, it should be a single sfxcache_t (with data in same alloc)*/
		if (sfx->decoder.buf)
			BZ_Free(sfx->decoder.buf);
		memset(&sfx->decoder, 0, sizeof(sfx->decoder));
	}
	S_UnlockMixer();
}

void S_ResetFailedLoad(void)
{
	int i;
	for (i=0 ; i < num_sfx ; i++)
	{
		if (known_sfx[i].loadstate == SLS_FAILED)
			known_sfx[i].loadstate = SLS_NOTLOADED;
	}
}

void S_UntouchAll(void)
{
	int i;
	for (i=0 ; i < num_sfx ; i++)
		known_sfx[i].touched = false;
}

/*
==================
S_PrecacheSound

==================
*/
sfx_t *S_PrecacheSound2 (const char *name, qboolean syspath)
{
	sfx_t	*sfx;

	if (nosound.ival || !known_sfx || !*name)
		return NULL;

	sfx = S_FindName (name, true, syspath);

// cache it in
	if (snd_precache.ival && snd_precache.ival != 2 && sndcardinfo)
		S_LoadSound (sfx, true);

	return sfx;
}


//=============================================================================

/*
=================
SND_PickChannel
=================
*/
channel_t *SND_PickChannel(soundcardinfo_t *sc, int entnum, int entchannel)
{
	int ch_idx;
	int oldest;

// Check for replacement sound, or an idle channel
	oldest = -1;
	for (ch_idx=DYNAMIC_FIRST; ch_idx < sc->max_chans ; ch_idx++)
	{
		if (entchannel != 0		// channel 0 never overrides
		&& sc->channel[ch_idx].entnum == entnum
		&& sc->channel[ch_idx].entchannel == entchannel)
		{	// always override sound from same entity
			oldest = ch_idx;
			break;
		}

		if (!sc->channel[ch_idx].sfx)
			oldest = ch_idx;
	}

	if (oldest == -1)
	{
		oldest = sc->max_chans;
		Z_ReallocElements((void**)&sc->channel, &sc->max_chans, oldest+1, sizeof(*sc->channel));
	}

	sc->channel[oldest].sfx = NULL;

	if (sc->total_chans <= oldest)
		sc->total_chans = oldest+1;
#ifdef Q3CLIENT	//presumably we should be using this instead of pos for oldest, but blurgh.
	sc->channel[oldest].starttime = Sys_Milliseconds();
#endif
	return &sc->channel[oldest];
}

static void SND_AccumulateSpacialization(soundcardinfo_t *sc, channel_t *ch, vec3_t origin)
{
	vec3_t listener_vec;
	vec_t dist;
	vec_t scale;
	vec3_t world_vec;
	int i, v;
	float volscale;
	int seat;

	if (ch->flags & CF_CL_ABSVOLUME)
		volscale = mastervolume.value;
	else
		volscale = volume.value * voicevolumemod;

	if (sc->seat == -1)
	{
		seat = 0;
		VectorSubtract(origin, listener[seat].origin, world_vec);
		dist = DotProduct(world_vec,world_vec);
		for (i = 1; i < cl.splitclients; i++)
		{
			VectorSubtract(origin, listener[i].origin, world_vec);
			scale = DotProduct(world_vec,world_vec);
			if (scale < dist)
			{
				dist = scale;
				seat = i;
			}
		}
	}
	else
	{
		seat = sc->seat;
	}

	// anything coming from the view entity will always be full volume
	if (ch->entnum == listener[seat].entnum)
	{
		v = ch->master_vol * (ruleset_allow_localvolume.value ? snd_playersoundvolume.value : 1) * volscale;
		v = bound(0, v, 255);
		for (i = 0; i < sc->sn.numchannels; i++)
			ch->vol[i] = v;
		return;
	}

// calculate stereo seperation and distance attenuation
	VectorSubtract(origin, listener[seat].origin, world_vec);

	dist = VectorNormalize(world_vec) * ch->dist_mult;

	if ((ch->flags & CF_NOSPACIALISE) || !ch->dist_mult)
	{
		scale = 1;
		scale = (1.0 - dist) * scale;
		v = ch->master_vol * scale * volscale;
		for (i = 0; i < sc->sn.numchannels; i++)
			ch->vol[i] += bound(0, v, 255);
		return;
	}

	//rotate the world_vec into listener space, so that the audio direction stored in the speakerdir array can be used directly.
	listener_vec[0] = DotProduct(listener[seat].forward, world_vec);
	listener_vec[1] = DotProduct(listener[seat].right, world_vec);
	listener_vec[2] = DotProduct(listener[seat].up, world_vec);

	if (snd_leftisright.ival^r_xflip.ival)
		listener_vec[1] = -listener_vec[1];

	for (i = 0; i < sc->sn.numchannels; i++)
	{
		scale = 1 + DotProduct(listener_vec, sc->speakerdir[i]);
		scale = (1.0 - dist) * scale * sc->dist[i];
		v = ch->master_vol * scale * volscale;
		ch->vol[i] += bound(0, v, 255);
	}
}
/*
=================
SND_Spatialize
=================
*/
static void SND_Spatialize(soundcardinfo_t *sc, channel_t *ch)
{
	vec3_t listener_vec, sound_vel;
	vec_t dist;
	vec_t scale;
	vec3_t world_vec;
	int i, v;
	float volscale;
	int seat;

	if (ch->flags & CF_FOLLOW)
	{
		//sounds following ents should update their position to match that ent's position.
		//its important that they do not snap back to where they were if the entity vanishes, so we just overwrite the channel origin for that. its simpler.
#ifdef CSQC_DAT
		if (ch->entnum < 0 && -ch->entnum < csqc_world.num_edicts)
		{
			wedict_t *ed = WEDICT_NUM_PB(csqc_world.progs, -ch->entnum);
			if (ed->ereftype == ER_ENTITY)
			{
				VectorCopy(ed->v->origin, ch->origin);
				VectorCopy(ed->v->velocity, ch->velocity);

				if (ed->v->solid == SOLID_BSP)
				{
					VectorMA(ch->origin, 0.5, ed->v->absmin, ch->origin);
					VectorMA(ch->origin, 0.5, ed->v->absmax, ch->origin);
				}
			}
		}
		else
#endif
		if (ch->entnum > 0 && ch->entnum < cl.maxlerpents && cl.lerpents[ch->entnum].sequence == cl.lerpentssequence)
		{
			lerpents_t *le = cl.lerpents+ch->entnum;
			int midx = le->entstate->modelindex;
			
			VectorCopy(le->origin, ch->origin);
			//VectorCopy(le->velocity, ch->velocity);	//fixme: bmodels should use their center rather than their origin. check le->state->solid?

			//bmodels should report the center of the entity rather than the origin (which is frequently at 0 0 0 or merely used as a pivot)
			if (le->entstate->solidsize == ES_SOLID_BSP && midx > 0 && midx < countof(cl.model_precache))
			{
				if (cl.model_precache[midx] && cl.model_precache[midx]->loadstate == MLS_LOADED && cl.model_precache[midx]->type == mod_brush)
				{
					//fixme: should probably deal with rotations.
					VectorMA(ch->origin, 0.5, cl.model_precache[midx]->mins, ch->origin);
					VectorMA(ch->origin, 0.5, cl.model_precache[midx]->maxs, ch->origin);
				}
			}
		}
		//FIXME: update rate to provide doppler
	}

	//sounds with absvolume ignore all volume etc cvars+settings
	if (ch->flags & CF_CL_ABSVOLUME)
		volscale = mastervolume.value;
	else
		volscale = volume.value * voicevolumemod;

	if (!vid.activeapp && !snd_inactive.ival && !(ch->flags & CF_CLI_INACTIVE))
		volscale = 0;

	if (sc->seat == -1)
	{
		seat = 0;
		VectorSubtract(ch->origin, listener[seat].origin, world_vec);
		dist = DotProduct(world_vec,world_vec);
#if MAX_SPLITS > 1
		for (i = 1; i < cl.splitclients; i++)
		{
			VectorSubtract(ch->origin, listener[i].origin, world_vec);
			scale = DotProduct(world_vec,world_vec);
			if (scale < dist)
			{
				dist = scale;
				seat = i;
			}
		}
#endif
	}
	else
	{
		seat = sc->seat;
	}

	// anything coming from the view entity will always be full volume
	// (no, I don't like this hack)
	if (ch->entnum == listener[seat].entnum && ch->entnum)
	{
		v = ch->master_vol * (ruleset_allow_localvolume.value ? snd_playersoundvolume.value : 1) * volscale;
		v = bound(0, v, 255);
		for (i = 0; i < sc->sn.numchannels; i++)
			ch->vol[i] = v;
		return;
	}

// calculate stereo seperation and distance attenuation
	VectorSubtract(ch->origin, listener[seat].origin, world_vec);

	dist = VectorNormalize(world_vec) * ch->dist_mult;

	if ((ch->flags & CF_NOSPACIALISE) || !ch->dist_mult)
	{
		scale = 1;
		scale = (1.0 - dist) * scale;
		v = ch->master_vol * scale * volscale;
		v = bound(0, v, 255);
		for (i = 0; i < sc->sn.numchannels; i++)
			ch->vol[i] = v;
		return;
	}

	//an attempt at doppler.
	if (snd_doppler.value)
	{
		//according to feh, the speed of sound is about 9000 qu/s.
		VectorAdd(listener[seat].velocity, ch->velocity, sound_vel);
		scale = 1 + snd_doppler.value * DotProduct(world_vec, sound_vel) / (9000.0);
		if (scale > snd_doppler_max.value)
			scale = snd_doppler_max.value;
		if (scale < snd_doppler_min.value)
			scale = snd_doppler_min.value;
		ch->rate = (1<<PITCHSHIFT) * scale + 0.5;
		if (ch->rate < 1)	//too small values result in crashes.
			ch->rate = 1;
	}

	//rotate the world_vec into listener space, so that the audio direction stored in the speakerdir array can be used directly.
	listener_vec[0] = DotProduct(listener[seat].forward, world_vec);
	listener_vec[1] = DotProduct(listener[seat].right, world_vec);
	listener_vec[2] = DotProduct(listener[seat].up, world_vec);

	if (snd_leftisright.ival^r_xflip.ival)
		listener_vec[1] = -listener_vec[1];

	for (i = 0; i < sc->sn.numchannels; i++)
	{
		scale = 1 + DotProduct(listener_vec, sc->speakerdir[i]);
		scale = (1.0 - dist) * scale * sc->dist[i];
		v = ch->master_vol * scale * volscale;
		v = bound(0, v, 255);
		ch->vol[i] = v;
	}
}

// =======================================================================
// Start a sound effect
// =======================================================================
static void S_UpdateSoundCard(soundcardinfo_t *sc, qboolean updateonly, channel_t *target_chan, int entnum, int entchannel, sfx_t *sfx, vec3_t origin, vec3_t velocity, float fvol, float attenuation, float timeoffset, float ratemul, unsigned int flags)
{
	channel_t *check;
	int		vol;
	int		ch_idx;
	int		skip;
	int		absstartpos = updateonly?sc->GetChannelPos?sc->GetChannelPos(sc, target_chan)<<PITCHSHIFT:target_chan->pos:0;
	extern cvar_t cl_demospeed;
	chanupdatereason_t chanupdatetype = updateonly?CUR_UPDATE:CUR_EVERYTHING;

	if (!sfx)
		sfx = target_chan->sfx;

	if (fvol < 0 || !sfx)
	{	//stopsound, apparently.
		target_chan->sfx = NULL;
		return;
	}

	if (timeoffset != 0.0)
		chanupdatetype |= CUR_OFFSET;

	if (!ratemul)	//rate of 0
		ratemul = 1;
	ratemul *= snd_playbackrate.value;
	if (!snd_ignoregamespeed.ival)
		ratemul *= (cls.state?cl.gamespeed:1) * (cls.demoplayback?cl_demospeed.value:1);
	if (ratemul <= 0)	//in case the user set the cvars weirdly
		ratemul = 1;

	vol = fvol*255;

// spatialize
	if (target_chan->sfx != sfx)
		chanupdatetype |= CUR_SOUNDCHANGE;

	memset (target_chan, 0, sizeof(*target_chan));
	if (!origin)
	{
		if (sc->seat == -1)
		{
			VectorClear(target_chan->origin);
			attenuation = 0;
			flags |= CF_NOSPACIALISE;
		}
		else
			VectorCopy(listener[sc->seat].origin, target_chan->origin);
	}
	else
	{
		VectorCopy(origin, target_chan->origin);
	}
	if (velocity)
		VectorCopy(velocity, target_chan->velocity);
	else
		VectorClear(target_chan->velocity);
	target_chan->flags = flags;
	target_chan->dist_mult = attenuation / snd_nominaldistance.value;
	target_chan->master_vol = vol;
	target_chan->entnum = entnum;
	target_chan->entchannel = entchannel;
	SND_Spatialize(sc, target_chan);

	if (!S_LoadSound (sfx, false))
	{
		target_chan->sfx = NULL;
		return;		// couldn't load the sound's data
	}

	//FIXME: why does this only filter for openal devices? its weird.
	if (!updateonly && !target_chan->vol[0] && !target_chan->vol[1] && !target_chan->vol[2] && !target_chan->vol[3] && !target_chan->vol[4] && !target_chan->vol[5] && sc->ChannelUpdate)
	if (sfx->loopstart == -1 && !(flags&CF_FORCELOOP))	//only skip if its not looping.
	{
		target_chan->sfx = NULL;
		goto updatechannel;
	}

	target_chan->sfx = sfx;

	target_chan->rate = ((1<<PITCHSHIFT) * ratemul); //*sfx->rate/sc->sn.speed;
	if (target_chan->rate < 1)	/*make sure the rate won't crash us*/
		target_chan->rate = 1;
	target_chan->pos = absstartpos + (int)(timeoffset*sc->sn.speed*target_chan->rate);

	if (!updateonly)
	{
// if an identical sound has also been started this frame, offset the pos
// a bit to keep it from just making the first one louder
		check = &sc->channel[DYNAMIC_FIRST];
		for (ch_idx=DYNAMIC_FIRST; ch_idx < sc->total_chans; ch_idx++, check++)
		{
			if (check == target_chan)
				continue;
			if (check->sfx == sfx && !check->pos)
			{
				skip = rand () % (int)(0.1*sc->sn.speed);
				target_chan->pos -= skip*target_chan->rate;
				break;
			}
		}
	}

updatechannel:

	if (sc->ChannelUpdate)
		sc->ChannelUpdate(sc, target_chan, chanupdatetype);
}

float S_UpdateSound(int entnum, int entchannel, sfx_t *sfx, vec3_t origin, vec3_t velocity, float fvol, float attenuation, float timeofs, float pitchadj, unsigned int flags)
{
	int i;
	int result = 0;
	int cards = 0;
	soundcardinfo_t *sc;
	channel_t *chan;

	if (cls.demoseeking)
		return result;
	S_LockMixer();
	for (sc = sndcardinfo; sc; sc = sc->next)
	{
		cards++;
		for (i = 0; i < sc->total_chans; i++)
		{
			if (sc->channel[i].entnum == entnum && sc->channel[i].entchannel == entchannel && sc->channel[i].sfx)
			{
				S_UpdateSoundCard(sc, true, &sc->channel[i], entnum, entchannel, sfx, origin, velocity, fvol, attenuation, timeofs, pitchadj, flags);
				result++;
				break;
			}
		}

		//start it if we couldn't find it.
		if (i == sc->total_chans && sfx)
		{
			chan = SND_PickChannel(sc, entnum, entchannel);
			if (chan)
				S_UpdateSoundCard(sc, false, chan, entnum, entchannel, sfx, origin, velocity, fvol, attenuation, timeofs, pitchadj, flags);
		}
	}
	S_UnlockMixer();
	if (!cards)
		cards=1;
	return result / (float)cards;
}

void S_StartSound(int entnum, int entchannel, sfx_t *sfx, vec3_t origin, vec3_t velocity, float fvol, float attenuation, float timeofs, float pitchadj, unsigned int flags)
{
	soundcardinfo_t *sc;
	channel_t *target_chan;

	if (!sfx || !*sfx->name)	//no named sounds would need specific starting.
		return;

	if (cls.demoseeking)
		return;

	if (!sound_started)
		return;

	if (nosound.ival)
		return;

	S_LockMixer();
	for (sc = sndcardinfo; sc; sc = sc->next)
	{
		if (flags & CF_NOREPLACE)
		{
			int i;
			for (i = 0; i < sc->total_chans; i++)
				if (sc->channel[i].entnum == entnum && sc->channel[i].entchannel == entchannel)
					break;
			if (i < sc->total_chans)
				continue;
		}
#ifdef Q3CLIENT
		if (flags & CF_CLI_NODUPES)
		{	//don't start too many simultaneous sounds. q3 sucks or something.
			int active = 0, i;
			unsigned int time = Sys_Milliseconds();
			for (i = 0; i < sc->total_chans; i++)
			{	//as per q3, channel isn't important.
				if (sc->channel[i].entnum == entnum && sc->channel[i].sfx == sfx)
				{
					//never allow a new sound within 50ms of the previous one
					if (time - sc->channel[i].starttime < 50)
						break;
					active++;
				}
			}
			if (active >= 4 || i < sc->total_chans)
			{
				Con_DPrintf("CF_CLI_NODUPES strikes again!\n");
				break;
			}
		}
#endif
		// pick a channel to play on
		target_chan = SND_PickChannel(sc, entnum, entchannel);
		if (!target_chan)
			break;

		S_UpdateSoundCard(sc, false, target_chan, entnum, entchannel, sfx, origin, velocity, fvol, attenuation, timeofs, pitchadj, flags);
	}
	S_UnlockMixer();
}

qboolean S_GetMusicInfo(int musicchannel, float *time, float *duration, char *title, size_t titlesize)
{
	qboolean result = false;
	soundcardinfo_t *sc;
	sfx_t *sfx;
	*time = 0;
	*duration = 0;

	if (titlesize)
		*title = 0;

	musicchannel += MUSIC_FIRST;

	S_LockMixer();
	for (sc = sndcardinfo; sc; sc = sc->next)
	{
		sfx = sc->channel[musicchannel].sfx;
		if (sfx)
		{
			Q_strncpyz(title, COM_SkipPath(sfx->name), titlesize);
			if (sfx->loadstate == SLS_LOADED)
			{
				if (sfx->decoder.querydata)
					*duration = sfx->decoder.querydata(sfx, NULL, title, titlesize);
				else if (sfx->decoder.buf)
				{
					sfxcache_t *c = sfx->decoder.buf;
					*duration = (float)c->length / c->speed;
				}
				else
					*duration = 0;

				//FIXME: openal doesn't report the actual time.
				*time = (sc->channel[musicchannel].pos>>PITCHSHIFT) / (float)snd_speed;	//the time into the sound, ignoring play rate.
				result = true;
			}
		}
	}
	S_UnlockMixer();

	return result;
}

float S_GetSoundTime(int entnum, int entchannel)
{
	int i;
	float result = -1;	//if we didn't find one
	soundcardinfo_t *sc;
	S_LockMixer();
	for (sc = sndcardinfo; sc && result == -1; sc = sc->next)
	{
		for (i = 0; i < sc->total_chans; i++)
		{
			if (sc->channel[i].entnum == entnum && sc->channel[i].entchannel == entchannel && sc->channel[i].sfx)
			{
				ssamplepos_t spos = sc->GetChannelPos?sc->GetChannelPos(sc, &sc->channel[i]):(sc->channel[i].pos>>PITCHSHIFT);
				result = spos / (float)snd_speed;	//the time into the sound, ignoring play rate.
				break;
			}
		}
		//we found one on this sound device card, ignore others.
		if (result != -1)
			break;
	}
	S_UnlockMixer();
	return result;
}
float S_GetChannelLevel(int entnum, int entchannel)
{
	int i, j;
	float result = -1;	//if we didn't find one
	soundcardinfo_t *sc;
	sfxcache_t scachebuf, *scache;
	S_LockMixer();
	for (sc = sndcardinfo; sc && result == -1; sc = sc->next)
	{
		for (i = 0; i < sc->total_chans; i++)
		{
			if (sc->channel[i].entnum == entnum && sc->channel[i].entchannel == entchannel && sc->channel[i].sfx)
			{
				ssamplepos_t spos = sc->GetChannelPos?sc->GetChannelPos(sc, &sc->channel[i]):(sc->channel[i].pos>>PITCHSHIFT);
				if (sc->channel[i].sfx->decoder.decodedata)
					scache = sc->channel[i].sfx->decoder.decodedata(sc->channel[i].sfx, &scachebuf, spos, 1);
				else 
					scache = NULL;
				if (!scache)
					scache = sc->channel[i].sfx->decoder.buf;

				if (scache && spos >= scache->soundoffset && spos < scache->soundoffset+scache->length)
				{
					spos -= scache->soundoffset;
					spos *= scache->numchannels;
					switch(scache->format)
					{
#ifdef FTE_TARGET_WEB
					case QAF_BLOB:
						result = 0;	//sorry. you're going to have to use .wav :(
						break;
#endif
					case QAF_S8:
						for (j = 0; j < scache->numchannels; j++)	//average the channels
							result += abs(((signed char*)scache->data)[spos+j]);
						result /= scache->numchannels*127.0;
						break;
					case QAF_S16:
						for (j = 0; j < scache->numchannels; j++)	//average the channels
							result += abs(((signed short*)scache->data)[spos+j]);
						result /= scache->numchannels*32767.0;
						break;
#ifdef MIXER_F32
					case QAF_F32:
						for (j = 0; j < scache->numchannels; j++)	//average the channels
							result += fabs(((float*)scache->data)[spos+j]);
						result /= scache->numchannels;
						break;
#endif
					}
				}
				else
					result = 0;
				break;
			}
		}
		//we found one on this sound device card, ignore others.
		if (result != -1)
			break;
	}
	S_UnlockMixer();
	return result;
}

qboolean S_IsPlayingSomewhere(sfx_t *s)
{
	soundcardinfo_t *si;
	int i;
	for (si = sndcardinfo; si; si=si->next)
	{
		for (i = 0; i < si->total_chans; i++)
		if (si->channel[i].sfx == s)
			return true;
	}
	return false;
}

static void S_StopSoundCard(soundcardinfo_t *sc, int entnum, int entchannel)
{
	int i;

	for (i=0 ; i<sc->total_chans ; i++)
	{
		if (sc->channel[i].entnum == entnum
			&& (!entchannel || sc->channel[i].entchannel == entchannel))
		{
			sc->channel[i].sfx = NULL;
			if (sc->ChannelUpdate)
				sc->ChannelUpdate(sc, &sc->channel[i], CUR_EVERYTHING);
			if (entchannel)
				break;
		}
	}
}

void S_StopSound(int entnum, int entchannel)
{
	soundcardinfo_t *sc;
	S_LockMixer();
	for (sc = sndcardinfo; sc; sc = sc->next)
		S_StopSoundCard(sc, entnum, entchannel);
	S_UnlockMixer();
}

void S_StopAllSounds(qboolean clear)
{
	int		i;
	sfx_t *s;
	channel_t musics[NUM_MUSICS];

	soundcardinfo_t *sc;

	if (!sound_started)
		return;
	S_LockMixer();

	for (sc = sndcardinfo; sc; sc = sc->next)
	{
		for (i=sc->total_chans ; i --> 0 ; )
		{
			if (i >= MUSIC_FIRST && i < MUSIC_FIRST+NUM_MUSICS && sc->selfpainting)
				continue;	//don't reset music if is safe to continue playing it without stuttering
			s = sc->channel[i].sfx;
			if (s)
			{
				sc->channel[i].sfx = NULL;
				if (s->loadstate == SLS_LOADED && s->decoder.ended)
				if (!S_IsPlayingSomewhere(s))	//if we aint playing it elsewhere, free it compleatly.
				{
					if (s->decoder.ended)
						s->decoder.ended(s);
				}

				if (sc->ChannelUpdate)
					sc->ChannelUpdate(sc, &sc->channel[i], CUR_EVERYTHING);
			}
		}

		sc->total_chans = NUM_AMBIENTS + NUM_MUSICS;	// no statics
		Z_ReallocElements((void**)&sc->channel, &sc->max_chans, sc->total_chans, sizeof(*sc->channel));

		memcpy(musics, &sc->channel[MUSIC_FIRST], sizeof(musics));
		Q_memset(sc->channel, 0, sc->max_chans * sizeof(channel_t));
		memcpy(&sc->channel[MUSIC_FIRST], musics, sizeof(musics));

		if (clear && !sc->selfpainting)	//if its self-painting, then the mixer will continue painting anyway (which is important if its still painting music, but otherwise don't stutter at all when loading)
			S_ClearBuffer (sc);
	}

	S_UnlockMixer();
}

static void S_StopAllSounds_f (void)
{
	S_StopAllSounds (true);
}

static void S_ClearBuffer (soundcardinfo_t *sc)
{
	void *buffer;
	unsigned int dummy;

	int		clear;

	if (!sound_started || !sc->sn.buffer)
		return;

	if (sc->sn.sampleformat == QSF_U8)
		clear = 0x80;
	else
		clear = 0;

	dummy = 0;
	buffer = sc->Lock(sc, &dummy);
	if (buffer)
	{
		Q_memset(buffer, clear, sc->sn.samples * sc->sn.samplebytes);
		sc->Unlock(sc, buffer);
	}
}

/*
=================
S_StaticSound
=================
*/
void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation)
{
	channel_t	*ss;
	soundcardinfo_t *scard;

	if (!sfx)
		return;

	S_LockMixer();

	for (scard = sndcardinfo; scard; scard = scard->next)
	{
		if (scard->total_chans == scard->max_chans)
		{
			if (!ZF_ReallocElements((void**)&scard->channel, &scard->max_chans, scard->max_chans+64, sizeof(*scard->channel)))
			{
				Con_Printf ("total_channels == MAX_CHANNELS\n");
				continue;
			}
		}

		if (!S_LoadSound (sfx, true))
			break;

		ss = &scard->channel[scard->total_chans];
		scard->total_chans++;

		ss->entnum = 0;
		ss->sfx = sfx;
		ss->rate = 1<<PITCHSHIFT;
		VectorCopy (origin, ss->origin);
		ss->master_vol = vol*255;
		ss->dist_mult = attenuation / snd_nominaldistance.value;
		ss->pos = 0;
		ss->flags = CF_FORCELOOP|CF_CLI_STATIC;

		SND_Spatialize (scard, ss);

		if (scard->ChannelUpdate)
			scard->ChannelUpdate(scard, ss, CUR_EVERYTHING);
	}

	S_UnlockMixer();
}


//=============================================================================

void S_Music_Clear(sfx_t *onlyifsample)
{
	//stops the current BGM music
	//calling this will trigger Media_NextTrack later
	sfx_t *s;
	soundcardinfo_t *sc;
	int i;
	for (i = MUSIC_FIRST; i < MUSIC_STOP; i++)
	{
		for (sc = sndcardinfo; sc; sc=sc->next)
		{
			s = sc->channel[i].sfx;
			if (!s)
				continue;
			if (onlyifsample && s != onlyifsample)
				continue;

			sc->channel[i].pos = 0;
			sc->channel[i].sfx = NULL;

			if (sc->ChannelUpdate)
				sc->ChannelUpdate(sc, &sc->channel[i], CUR_EVERYTHING);

			if (s && s->decoder.ended && !S_IsPlayingSomewhere(s))	//if we aint playing it elsewhere, free it compleatly.
				s->decoder.ended(s);
		}
	}
}
void S_Music_Seek(float time)
{
	soundcardinfo_t *sc;
	int i;
	for (i = MUSIC_FIRST; i < MUSIC_STOP; i++)
	{
		for (sc = sndcardinfo; sc; sc=sc->next)
		{
			sc->channel[i].pos += sc->sn.speed*time * sc->channel[i].rate;

			if (sc->channel[i].pos < 0)
			{	//clamp to the start of the track
				sc->channel[i].pos=0;
			}
			//if we seek over the end, ignore it. The sound playing code will spot that.
		}
	}
}

//mixer must be locked
qboolean S_Music_Playing(int musicchannel)
{
	soundcardinfo_t *sc;
	musicchannel += MUSIC_FIRST;
	for (sc = sndcardinfo; sc; sc=sc->next)
	{
		if (sc->channel[musicchannel].sfx)
			return true;
	}
	return false;
}

/*
===================
S_UpdateAmbientSounds
===================
*/
mleaf_t *Q1BSP_LeafForPoint (model_t *model, vec3_t p);
void S_UpdateAmbientSounds (soundcardinfo_t *sc)
{
	float		vol;
	channel_t	*chan;
	int i;
#ifdef Q1BSPS
	mleaf_t		*l;
	float oldvol;
	int ambientlevel[NUM_AMBIENTS];
#endif

	if (!snd_ambient)
		return;

	for (i = MUSIC_FIRST; i < MUSIC_STOP; i++)
	{
		chanupdatereason_t changed = CUR_SPACIALISEONLY;
		chan = &sc->channel[i];
		if (!chan->sfx)
		{
			float time = 0;
			sfx_t *newmusic;
			if (!S_Music_Playing(i-MUSIC_FIRST))
			{
				newmusic = Media_NextTrack(i-MUSIC_FIRST, &time);
				if (newmusic && newmusic->loadstate != SLS_FAILED)
				{	//okay, now we know which track we're meant to be playing, all devices can play it at once.
					soundcardinfo_t *sc2;
					for (sc2 = sndcardinfo; sc2; sc2=sc2->next)
					{
						channel_t	*chan = &sc2->channel[i];
						chan->sfx = newmusic;
						chan->rate = 1<<PITCHSHIFT;
						chan->pos = (int)(time * sc->sn.speed) * chan->rate;
						changed = CUR_EVERYTHING;

						chan->master_vol = bound(0, 1, 255);
						chan->vol[0] = chan->vol[1] = chan->vol[2] = chan->vol[3] = chan->vol[4] = chan->vol[5] = chan->master_vol;
						if (sc->ChannelUpdate)
							sc->ChannelUpdate(sc, chan, changed);
					}
				}
			}
		}
		if (chan->sfx)
		{
			chan->flags = /*CF_CL_INACTIVE|*/CF_CL_ABSVOLUME|CF_NOSPACIALISE|CF_NOREVERB;	//bypasses volume cvar completely.
			vol = 255*bgmvolume.value*voicevolumemod;
			if (!vid.activeapp && !snd_inactive.ival && !(chan->flags & CF_CLI_INACTIVE))
				vol = 0;
			vol = bound(0, vol, 255);
			vol = Media_CrossFade(i-MUSIC_FIRST, vol, (chan->pos>>PITCHSHIFT) / (float)snd_speed);
			if (vol < 0)
			{	//cross fading wants to KILL this track now, apparently.
				sfx_t *s = chan->sfx;

				if (s->loadstate != SLS_LOADING)
				{
					chan->pos = 0;
					chan->sfx = NULL;

					if (sc->ChannelUpdate)
						sc->ChannelUpdate(sc, chan, CUR_EVERYTHING);

					if (s && s->decoder.ended && !S_IsPlayingSomewhere(s))	//if we aint playing it elsewhere, free it compleatly.
						s->decoder.ended(s);
				}
			}
			else
			{
				chan->master_vol = bound(0, vol, 255);
				chan->vol[0] = chan->vol[1] = chan->vol[2] = chan->vol[3] = chan->vol[4] = chan->vol[5] = chan->master_vol;
				if (sc->ChannelUpdate)
					sc->ChannelUpdate(sc, chan, changed);
			}
		}
	}

#ifdef Q1BSPS
// calc ambient sound levels
	for (i = 0; i < NUM_AMBIENTS; i++)
		ambientlevel[i] = 0;
	if (cl.worldmodel && cl.worldmodel->type == mod_brush && cl.worldmodel->fromgame == fg_quake && cl.worldmodel->loadstate == MLS_LOADED)
	{
		if (ambient_level.value)
		{
			if (sc->seat < 0)
			{
				int seat = max(1,cl.splitclients);
				while(seat --> 0)
				{
					l = Q1BSP_LeafForPoint(cl.worldmodel, listener[seat].origin);
					if (!l)
						continue;

					for (i = 0; i < NUM_AMBIENTS; i++)
						ambientlevel[i] = max(ambientlevel[i], l->ambient_sound_level[i]);
				}
			}
			else
			{
				l = Q1BSP_LeafForPoint(cl.worldmodel, listener[sc->seat].origin);
				if (l)
					for (i = 0; i < NUM_AMBIENTS; i++)
						ambientlevel[i] = l->ambient_sound_level[i];
			}
		}
	}

	for (i = 0 ; i< NUM_AMBIENTS ; i++)
	{
		chan = &sc->channel[AMBIENT_FIRST+i];
		chan->sfx = ambient_sfx[AMBIENT_FIRST+i];
		chan->entnum = 0;
		chan->flags = CF_FORCELOOP | CF_NOSPACIALISE;
		chan->rate = 1<<PITCHSHIFT;

		VectorClear(chan->origin);

		vol = ambient_level.value * ambientlevel[i];
		if (vol < 8)
			vol = 0;

		oldvol = sc->ambientlevels[i];

	// don't adjust volume too fast
		if (sc->ambientlevels[i] < vol)
		{
			sc->ambientlevels[i] += host_frametime * ambient_fade.value;
			if (sc->ambientlevels[i] > vol)
				sc->ambientlevels[i] = vol;
		}
		else if (chan->master_vol > vol)
		{
			sc->ambientlevels[i] -= host_frametime * ambient_fade.value;
			if (sc->ambientlevels[i] < vol)
				sc->ambientlevels[i] = vol;
		}

		chan->master_vol = sc->ambientlevels[i];
		chan->vol[0] = chan->vol[1] = chan->vol[2] = chan->vol[3] = chan->vol[4] = chan->vol[5] = bound(0, chan->master_vol * (volume.value*voicevolumemod), 255);

		if (sc->ChannelUpdate)
			sc->ChannelUpdate(sc, chan, ((oldvol == 0) ^ (sc->ambientlevels[i] == 0))?CUR_EVERYTHING:CUR_SPACIALISEONLY);
	}
#endif
}

struct sndreverbproperties_s *reverbproperties;
size_t numreverbproperties;
qboolean S_UpdateReverb(size_t slot, void *reverb, size_t reverbsize)
{
	struct reverbproperties_s newprops;
	if (slot >= 1024)
		return false;

	if (slot >= numreverbproperties)
	{
		int slots = slot+1;
		void *n = BZ_Realloc(reverbproperties, sizeof(*reverbproperties)*slots);
		if (!n)
			return false;
		reverbproperties = n;
		memset(reverbproperties+numreverbproperties, 0, sizeof(*reverbproperties) * (slots-numreverbproperties));
		numreverbproperties = slots;
	}

	memset(&newprops, 0, sizeof(newprops));
	if (reverb)
	{
		//clamp the size for possible future extensibility
		if (reverbsize > sizeof(newprops))
			reverbsize = sizeof(newprops);
		memcpy(&newprops, reverb, reverbsize);
	}

	if (memcmp(&newprops, &reverbproperties[slot].props, sizeof(newprops)))
	{
		reverbproperties[slot].props = newprops;
		reverbproperties[slot].modificationcount++;
	}
	return true;
}

/*
============
S_Update

Called once each time through the main loop
============
*/
void S_UpdateListener(int seat, int entnum, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, size_t reverbtype, vec3_t velocity)
{
	soundcardinfo_t *sc;
	listener[seat].entnum = entnum;
	VectorCopy(origin, listener[seat].origin);
	VectorCopy(forward, listener[seat].forward);
	VectorCopy(right, listener[seat].right);
	VectorCopy(up, listener[seat].up);
	VectorCopy(velocity, listener[seat].velocity);

	for (sc = sndcardinfo; sc; sc=sc->next)
		if (sc->SetEnvironmentReverb && (sc->seat == seat || (sc->seat == -1 && seat == 0)))
			sc->SetEnvironmentReverb(sc, reverbtype);
}

void S_GetListenerInfo(int seat, float *origin, float *forward, float *right, float *up)
{
	VectorCopy(listener[seat].origin, origin);
	VectorCopy(listener[seat].forward, forward);
	VectorCopy(listener[seat].right, right);
	VectorCopy(listener[seat].up, up);
}

static void S_Q2_AddEntitySounds(soundcardinfo_t *sc)
{
	vec3_t positions[2048];
	int entnums[countof(positions)];
	sfx_t *sounds[countof(positions)];
	unsigned int count;
	unsigned int j;
	channel_t *c;

#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)
		count = CLQ2_GatherSounds(positions, entnums, sounds, countof(sounds));
	else
#endif
#ifdef VM_CG
	if (cls.protocol == CP_QUAKE3 && q3)
		count = q3->cg.GatherLoopingSounds(positions, entnums, sounds, countof(sounds));
	else
#endif
		return;
	
	while(count --> 0)
	{
		sfx_t *sfx = sounds[count];
		if (!sfx)
			continue;
		if (sfx->loadstate == SLS_NOTLOADED)
			S_LoadSound(sfx, true);
		if (sfx->loadstate != SLS_LOADED)
			continue;	//not ready yet

		if (sc->ChannelUpdate)
		{
			for (c = NULL, j=DYNAMIC_FIRST; j < sc->total_chans ; j++)
			{
				if (sc->channel[j].entnum == entnums[count] && !sc->channel[j].entchannel && (sc->channel[j].flags & CF_CLI_AUTOSOUND))
				{
					c = &sc->channel[j];
					break;
				}
			}
		}
		else
		{
			for (c = NULL, j=DYNAMIC_FIRST; j < sc->total_chans ; j++)
			{
				if (sc->channel[j].sfx == sfx && (sc->channel[j].flags & CF_CLI_AUTOSOUND))
				{
					c = &sc->channel[j];
					break;
				}
			}
		}
		if (!c)
		{
			c = SND_PickChannel(sc, 0, 0);
			if (!c)
				continue;
			c->flags = CF_CLI_AUTOSOUND|CF_FORCELOOP;
			c->entnum = sc->ChannelUpdate?entnums[count]:0;
			c->entchannel = 0;
			c->dist_mult = 3 / snd_nominaldistance.value;
			c->master_vol = 255 * 1;
			c->pos = 0<<PITCHSHIFT;	//q2 does weird stuff with the pos. we just forceloop and detect when it became irrelevant. this is required for stream decoding or openal
			c->rate = 1<<PITCHSHIFT;
			for (j = 0; j < countof(c->vol); j++)
				c->vol[j] = 0;
			c->sfx = NULL;
		}
		if (sc->ChannelUpdate)
		{	//hardware mixing doesn't support merging
			VectorCopy(positions[count], c->origin);
			SND_Spatialize(sc, c);

			if (c->sfx)
				sc->ChannelUpdate(sc, c, CUR_SPACIALISEONLY);
		}
		else
		{	//merge with any other ents, if we can
			for (j = 0; j <= count; j++)
			{
				if (sounds[j] == sfx)
				{
					sounds[j] = NULL;
					SND_AccumulateSpacialization(sc, c, positions[j]);
				}
			}
		}
		if (!c->sfx)
		{
			for (j = 0; j < countof(c->vol); j++)
				if (c->vol[j])
					break;
			if (j == countof(c->vol))
				c->sfx = NULL;	//err, never mind
			else
			{
				c->sfx = sfx;
				if (sc->ChannelUpdate)
					sc->ChannelUpdate(sc, c, CUR_EVERYTHING);
			}
		}
	}
}

static void S_UpdateCard(soundcardinfo_t *sc)
{
	int			i, j;
	channel_t	*ch;
	channel_t	*combine;

	if (!sound_started)
		return;
	if ((snd_blocked > 0))
	{
		if (!sc->inactive_sound)
			return;
	}

#ifdef AVAIL_OPENAL
	if (sc->ListenerUpdate)
	{
		sc->ListenerUpdate(sc, listener[sc->seat].entnum, listener[sc->seat].origin, listener[sc->seat].forward, listener[sc->seat].right, listener[sc->seat].up, listener[sc->seat].velocity);
	}
#endif

// update general area ambient sound sources
	S_UpdateAmbientSounds (sc);

	combine = NULL;

// update spatialization for static and dynamic sounds
	ch = sc->channel+DYNAMIC_FIRST;
	for (i=DYNAMIC_FIRST ; i<sc->total_chans; i++, ch++)
	{
		if (!ch->sfx)
			continue;
		if (ch->flags & CF_CLI_AUTOSOUND)
		{
			if (!ch->vol[0] && !ch->vol[1] && !ch->vol[2] && !ch->vol[3] && !ch->vol[4] && !ch->vol[5])
			{
				ch->sfx = NULL;
				if (sc->ChannelUpdate)
					sc->ChannelUpdate(sc, ch, CUR_EVERYTHING);
			}
			ch->vol[0] = ch->vol[1] = ch->vol[2] = ch->vol[3] = ch->vol[4] = ch->vol[5] = 0;
			continue;
		}

		if (sc->ChannelUpdate)
		{
			if (ch->flags & CF_FOLLOW)
				SND_Spatialize(sc, ch);	//update it a little
			sc->ChannelUpdate(sc, ch, CUR_SPACIALISEONLY);
			continue;
		}

		SND_Spatialize(sc, ch);         // respatialize channel
		if (!ch->vol[0] && !ch->vol[1] && !ch->vol[2] && !ch->vol[3] && !ch->vol[4] && !ch->vol[5])
			continue;

	// try to combine static sounds with a previous channel of the same
	// sound effect so we don't mix five torches every frame

		if (ch->flags & CF_CLI_STATIC)
		{
		// see if it can just use the last one
			if (combine && combine->sfx == ch->sfx)
			{
				combine->vol[0] += ch->vol[0];
				combine->vol[1] += ch->vol[1];
				combine->vol[2] += ch->vol[2];
				combine->vol[3] += ch->vol[3];
				combine->vol[4] += ch->vol[4];
				combine->vol[5] += ch->vol[5];
				ch->vol[0] = ch->vol[1] = ch->vol[2] = ch->vol[3] = ch->vol[4] = ch->vol[5] = 0;
				continue;
			}
		// search for one
			combine = sc->channel+DYNAMIC_FIRST;
			for (j=DYNAMIC_FIRST ; j<i; j++, combine++)
				if (combine->sfx == ch->sfx)
					break;

			if (j == sc->total_chans)
			{
				combine = NULL;
			}
			else
			{
				if (combine != ch)
				{
					combine->vol[0] += ch->vol[0];
					combine->vol[1] += ch->vol[1];
					combine->vol[2] += ch->vol[2];
					combine->vol[3] += ch->vol[3];
					combine->vol[4] += ch->vol[4];
					combine->vol[5] += ch->vol[5];
					ch->vol[0] = ch->vol[1] = ch->vol[2] = ch->vol[3] = ch->vol[4] = ch->vol[5] = 0;
				}
				continue;
			}
		}
	}

	S_Q2_AddEntitySounds(sc);

//
// debugging output
//
	if (snd_show.ival)
	{
		struct listener_s *l;
		int			active, mute;
		active = 0;
		mute = 0;
		ch = sc->channel;
		for (i=0 ; i<sc->total_chans; i++, ch++)
		{
			if (ch->sfx && (ch->vol[0] || ch->vol[1]) )
			{
				if (snd_show.ival > 1)
					Con_Printf ("%i, %i/%i/%i/%i/%i/%i %s\n", i, ch->vol[0], ch->vol[1], ch->vol[2], ch->vol[3], ch->vol[4], ch->vol[5], ch->sfx->name);
				active++;
			}
			else if (ch->sfx)
				mute++;
		}

		if (sc->seat < 0)
			l = &listener[0];
		else
			l = &listener[sc->seat];
		Con_Printf ("----(%i+%i %s %i %.1f %.1f %.1f)----\n", active, mute, sc->name, l->entnum, l->origin[0], l->origin[1], l->origin[2]);
	}

#ifdef HAVE_MIXER
// mix some sound

	if (sc->selfpainting)
		return;

	if (snd_blocked > 0)
	{
		if (!sc->inactive_sound)
			return;
	}

	S_Update_(sc);
#endif
}

#ifdef HAVE_MIXER
int S_GetMixerTime(soundcardinfo_t *sc)
{
	int		samplepos;
	int		fullsamples;

	fullsamples = sc->sn.samples / sc->sn.numchannels;

// it is possible to miscount buffers if it has wrapped twice between
// calls to S_Update.  Oh well.
	samplepos = sc->GetDMAPos(sc);

	if (sc->samplequeue > 0)
		samplepos -= sc->samplequeue;

	if (samplepos < 0)
	{
		samplepos = 0;
	}
	if (samplepos < sc->oldsamplepos)
	{
		int bias;
		sc->buffers++;					// buffer wrapped

		if (sc->paintedtime > 0x40000000)
		{
			//when things get too large, we push everything back to prevent overflows
			bias = sc->paintedtime;
			bias -= bias % fullsamples;
			sc->paintedtime -= bias;
			sc->buffers -= bias / fullsamples;
		}
	}
	sc->oldsamplepos = samplepos;

	return sc->buffers*fullsamples + samplepos/sc->sn.numchannels;
}
#endif

void S_Update (void)
{
	soundcardinfo_t *sc;
	RSpeedMark();
	S_LockMixer();
	for (sc = sndcardinfo; sc; sc = sc->next)
		S_UpdateCard(sc);
	S_UnlockMixer();
	RSpeedEnd(RSPEED_AUDIO);
}

void S_ExtraUpdate (void)
{
#ifdef HAVE_MIXER
	soundcardinfo_t *sc;
#endif

	if (!sound_started)
		return;

#if defined(_WIN32) && !defined(WINRT)
	INS_Accumulate ();
#endif
#ifdef HAVE_MIXER
	if (snd_noextraupdate.ival)
		return;		// don't pollute timings

	for (sc = sndcardinfo; sc; sc = sc->next)
	{
		if (sc->selfpainting)
			continue;

		if (snd_blocked > 0)
		{
			if (!sc->inactive_sound)
				continue;
		}

		S_LockMixer();
		S_Update_(sc);
		S_UnlockMixer();
	}
#endif
}


#ifdef HAVE_MIXER
static void S_Update_(soundcardinfo_t *sc)
{
	int soundtime; /*in pairs*/
	unsigned        endtime;
	int				samps;

// Updates DMA time
	soundtime = S_GetMixerTime(sc);

	if (sc->samplequeue > 0)
	{
		/*device uses a write-once queue*/
		endtime = soundtime + sc->samplequeue/sc->sn.numchannels;
		soundtime = sc->paintedtime;
		samps = sc->samplequeue / sc->sn.numchannels;
	}
	else if (sc->samplequeue < 0)
	{	/*device is telling us the exact point that we should be mixing to*/
		endtime = soundtime;
		soundtime = sc->paintedtime;
		samps = sc->sn.samples / sc->sn.numchannels;
	}
	else
	{
		/*device uses memory-mapped output*/
		// check to make sure that we haven't overshot
		if (sc->paintedtime < soundtime)
		{
			//Con_Printf ("S_Update_ : overflow\n");
			sc->paintedtime = soundtime;
		}

		// mix ahead of current position
		endtime = soundtime + (int)(_snd_mixahead.value * sc->sn.speed);	
		samps = sc->sn.samples / sc->sn.numchannels;
	}
	if (endtime - soundtime > samps)
	{
		endtime = soundtime + samps;
	}

	/*DirectSound may have killed us to give priority to another app, ask to restore it*/
	if (sc->Restore)
		sc->Restore(sc);

	S_PaintChannels (sc, endtime);

	sc->Submit(sc, soundtime, endtime);
}

/*
called periodically by dedicated mixer threads.
do any blocking calls AFTER this returns. note that this means you can't use the Submit/unlock method to submit blocking audio.
*/
void S_MixerThread(soundcardinfo_t *sc)
{
	S_LockMixer();
	S_Update_(sc);
	S_UnlockMixer();
}
#endif

/*
===============================================================================

console functions

===============================================================================
*/

void S_Play_f(void)
{	//plays a sound located around the player
	int 	i;
	char name[256];
	sfx_t	*sfx;
	const char *cmdname = Cmd_Argv(0);
	float vol, attenuation = 0;
	unsigned int flags = CF_NOSPACIALISE;
	int entnum = 0;
	float *origin = NULL;


/*	//Vanilla compat (breaks modern QW mods):
   	if (!strcmp(cmdname, "play"))
	{
		flags = 0;
		attenuation = 1;
		origin = listener[0].origin;
		entnum = listener[0].entnum;
	}
*/

	i = 1;
	while (i<Cmd_Argc())
	{
		if (!Q_strrchr(Cmd_Argv(i), '.'))
		{
			Q_strncpyz(name, Cmd_Argv(i), sizeof(name)-4);
			Q_strcat(name, ".wav");
		}
		else
			Q_strncpyz(name, Cmd_Argv(i), sizeof(name));
		i++;
		sfx = S_PrecacheSound(name);

		if (!strcmp(cmdname, "playvol"))
			vol = Q_atof(Cmd_Argv(i++));
		else
			vol = 1.0;
		S_StartSound(entnum, 0, sfx, origin, NULL, vol, attenuation, 0, 0, flags);
	}
}

void S_SoundList_f(void)
{
	int		i;
	sfx_t	*sfx;
	sfxcache_t	*sc;
	sfxcache_t	scachebuf;
	int		size, total;
	int duration;

	S_LockMixer();


	total = 0;
	for (sfx=known_sfx, i=0 ; i<num_sfx ; i++, sfx++)
	{
		if (sfx->loadstate != SLS_LOADED)
			sc = NULL;
		else if (sfx->decoder.decodedata)
		{
			if (sfx->decoder.querydata)
				sc = (sfx->decoder.querydata(sfx, &scachebuf, NULL, 0) < 0)?NULL:&scachebuf;
			else
				sc = NULL;	//don't bother trying to actually decode anything here.
			if (!sc)
			{
				Con_Printf("S(      )            : %s\n", sfx->name);
				continue;
			}
		}
		else
			sc = sfx->decoder.buf;
		if (!sc)
		{
			Con_Printf("?(      )            : %s\n", sfx->name);
			continue;
		}
		size = (sc->soundoffset+sc->length)*QAF_BYTES(sc->format)*(sc->numchannels);
		duration = (sc->soundoffset+sc->length) / sc->speed;
		total += size;
		if (sfx->loopstart >= 0)
			Con_Printf ("L");
		else
			Con_Printf (" ");
		Con_Printf("(%2db%2ic) %6i %2is : %s\n",QAF_BYTES(sc->format)*8, sc->numchannels, size, duration, sfx->name);
	}
	Con_Printf ("Total resident: %i\n", total);

	S_UnlockMixer();
}


void S_LocalSound2 (const char *sound, int channel, float volume)
{
	sfx_t	*sfx;

	if (nosound.ival)
		return;
	if (!sound_started)
		return;

	sfx = S_PrecacheSound (sound);
	if (!sfx)
	{
		Con_Printf ("S_LocalSound: can't cache %s\n", sound);
		return;
	}
	S_StartSound (0, channel, sfx, NULL, NULL, volume, 0, 0, 0, CF_CLI_INACTIVE|CF_NOSPACIALISE|CF_NOREVERB);
}
void S_LocalSound (const char *sound)
{
	S_LocalSound2(sound, 256, 1);
}











typedef struct {
	sfxdecode_t decoder;

	qboolean inuse;
	int id;
	sfx_t *sfx;

	int numchannels;
	qaudiofmt_t format;
	int length;
	void *data;
} streaming_t;
#define MAX_RAW_SOURCES (MAX_CLIENTS+3)
streaming_t s_streamers[MAX_RAW_SOURCES];

void S_ClearRaw(void)
{
	memset(s_streamers, 0, sizeof(s_streamers));
}

//returns an sfxcache_t stating where the data is
sfxcache_t *QDECL S_Raw_Locate(sfx_t *sfx, sfxcache_t *buf, ssamplepos_t start, int length)
{
	streaming_t *s = sfx->decoder.buf;
	if (buf)
	{
		buf->data = s->data;
		buf->length = s->length;
		buf->numchannels = s->numchannels;
		buf->soundoffset = 0;
		buf->speed = snd_speed;
		buf->format = s->format;
	}
	if (start >= s->length)
		return NULL;	//eof...
	return buf;
}
void QDECL S_Raw_Ended(sfx_t *sfx)
{	//no longer playing anywhere...
	streaming_t *s = sfx->decoder.buf;
	s->inuse = false;	//let it get reused now.
}
void QDECL S_Raw_Purge(sfx_t *sfx)
{	//flush all caches, will be re-read from disk (or not, because this is streamed)
	streaming_t *s = sfx->decoder.buf;
	s->length = 0;
	s->numchannels = 0;
	BZ_Free(s->data);
	s->data = NULL;
	s->inuse = false;

	memset(&sfx->decoder, 0, sizeof(sfx->decoder));
}

float S_RawAudioQueued(int sourceid)	//returns in seconds. we don't know what the original sample count was.
{
	soundcardinfo_t *si;
	streaming_t *s;
	int i;
	float r;
	ssamplepos_t highest, pos;
	for (s = s_streamers, i = 0; i < MAX_RAW_SOURCES; i++, s++)
	{
		if (s->inuse && s->id == sourceid)
		{
			S_LockMixer();

			highest = ((~(usamplepos_t)0)>>1);
			for (si = sndcardinfo; si; si=si->next)	//make sure all cards are playing, and that we still get a prepad if just one is.
			{
				for (i = 0; i < si->total_chans; i++)
					if (si->channel[i].sfx == s->sfx)
					{
						if (si->GetChannelPos)
							pos = si->GetChannelPos(si, &si->channel[i]);
						else
							pos = si->channel[i].pos>>PITCHSHIFT;
						if (highest > pos)
							highest = pos;
						break;
					}
			}
			if (highest == ((~(usamplepos_t)0)>>1))
				r = 0;	//nothing playing it... needs to be woken up. pretend nothing is there so it gets poked a bit.
			else
				r = (s->length - highest) / (float)snd_speed;
			S_UnlockMixer();
			return r;
		}
	}
	return 0;	//not found
}

//streaming audio.	//this is useful when there is one source, and the sound is to be played with no attenuation
void S_RawAudio(int sourceid, const qbyte *data, int speed, int samples, int channels, qaudiofmt_t format, float volume)
{
	soundcardinfo_t *si;
	int i;
	int prepadl;	//this is the amount of data that was previously available, and will be removed from the buffer.
	int spare;		//the amount of existing data that is still left to be played
	int outsamples;	//the amount of data we're going to add (at the output rate)
	double speedfactor;
	qbyte *newcache;
	streaming_t *s, *free=NULL;

	if (!sound_started)
		return;

	for (s = s_streamers, i = 0; i < MAX_RAW_SOURCES; i++, s++)
	{
		if (!s->inuse)
		{
			if (!free)
				free = s;
			continue;
		}
		if (s->id == sourceid)
			break;
	}
	if (!data)
	{
		if (i == MAX_RAW_SOURCES)
			return;	//wierd, it wasn't even playing.
		s->inuse = false;

		S_LockMixer();
		for (si = sndcardinfo; si; si=si->next)
		for (i = 0; i < si->total_chans; i++)
			if (si->channel[i].sfx == s->sfx)
			{
				si->channel[i].sfx = NULL;
				break;
			}
		BZ_Free(s->data);
		s->data = NULL;
		S_UnlockMixer();
		return;
	}
	if (i == MAX_RAW_SOURCES || !s->inuse)	//whoops.
	{
		if (i == MAX_RAW_SOURCES)
		{
			if (!free)
			{
				Con_Printf("No free audio streams\n");
				return;
			}
			s = free;
		}

		if (!s->sfx)
			s->sfx = S_FindName(va("***stream_%i***", i), true, false);
		s->sfx->decoder.buf = s;
		s->sfx->decoder.decodedata = S_Raw_Locate;
		s->sfx->decoder.ended = S_Raw_Ended;
		s->sfx->decoder.purge = S_Raw_Purge;
		s->sfx->loopstart = -1; //non-looping...
		s->sfx->loadstate = SLS_LOADED;

		s->numchannels = channels;
		s->format = format;
		s->data = NULL;
		s->length = 0;

		s->id = sourceid;
		s->inuse = true;
//		Con_Printf("Added new raw stream\n");
	}
	S_LockMixer();

	if (s->format != format || s->numchannels != channels)
	{
		s->format = format;
		s->numchannels = channels;
		s->length = 0;
		Con_Printf("Restarting raw stream\n");
	}

	speedfactor	= (double)speed/snd_speed;
	outsamples = samples/speedfactor;

	prepadl = 0x7fffffff;
	for (si = sndcardinfo; si; si=si->next)	//make sure all cards are playing, and that we still get a prepad if just one is.
	{
		for (i = 0; i < si->total_chans; i++)
			if (si->channel[i].sfx == s->sfx)
			{
				if (prepadl > (si->channel[i].pos>>PITCHSHIFT))
					prepadl = (si->channel[i].pos>>PITCHSHIFT);
				break;
			}
	}

	if (prepadl == 0x7fffffff)
	{
		if (snd_show.ival)
			Con_Printf("Wasn't playing\n");
		prepadl = 0;
		spare = 0;
		if (spare > snd_speed)
		{
			Con_DPrintf("Sacrificed raw sound stream\n");
			spare = 0;	//too far out. sacrifice it all
		}
	}
	else
	{
		if (prepadl < 0)
			prepadl = 0;
		spare = s->length - prepadl;
		if (spare < 0)	//remaining samples since last time
			spare = 0;

		if (spare > snd_speed*2) // more than 2 seconds of sound. don't buffer more than 2 seconds. 1: its probably buggy if we need to. 2: takes too much memory, and we use malloc+copies.
		{
			Con_DPrintf("Sacrificed raw sound stream\n");
			spare = 0;	//too far out. sacrifice it all
		}
	}

	newcache = BZ_Malloc((spare+outsamples) * (s->numchannels) * QAF_BYTES(s->format));
	memcpy(newcache, (qbyte*)s->data + prepadl * (s->numchannels) * QAF_BYTES(s->format), spare * (s->numchannels) * QAF_BYTES(s->format));

	BZ_Free(s->data);
	s->data = newcache;

	s->length = spare + outsamples;

	{
		extern cvar_t snd_linearresample_stream;
		short *outpos = (short *)((char*)s->data + spare * (s->numchannels) * QAF_BYTES(s->format));
		SND_ResampleStream(data,
			speed,
			format,
			channels,
			samples,
			outpos,
			snd_speed,
			s->format,
			s->numchannels,
			snd_linearresample_stream.ival);
	}

	for (si = sndcardinfo; si; si=si->next)
	{
		for (i = 0; i < si->total_chans; i++)
			if (si->channel[i].sfx == s->sfx)
			{
				si->channel[i].pos -= prepadl*si->channel[i].rate;

				if (si->channel[i].pos < 0)
					si->channel[i].pos = 0;
				si->channel[i].master_vol = 255 * volume;
				if (si->ChannelUpdate)
					si->ChannelUpdate(si, &si->channel[i], CUR_SPACIALISEONLY);
				break;
			}
		if (i == si->total_chans)	//this one wasn't playing.
		{
			channel_t *c = SND_PickChannel(si, -1, 0);
			if (c)
			{
				c->flags = (sourceid>=0?CF_CLI_INACTIVE:0)|CF_CL_ABSVOLUME|CF_NOSPACIALISE;
				c->entnum = 0;
				c->entchannel = 0;
				c->dist_mult = 0;
				c->master_vol = 255 * volume;
				c->pos = 0;
				c->rate = 1<<PITCHSHIFT;
				c->sfx = s->sfx;
				SND_Spatialize(si, c);

				if (si->ChannelUpdate)
					si->ChannelUpdate(si, c, CUR_EVERYTHING);
			}
		}
	}
	S_UnlockMixer();
}
