/*
	snd_alsa.c

	Support for the ALSA 1.0.1 sound driver

	Copyright (C) 1999,2000  contributors of the QuakeForge project
	Please see the file "AUTHORS" for a list of contributors

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA

*/
//actually stolen from darkplaces.
//I guess noone can be arsed to write it themselves. :/
//
//This file is otherwise known as 'will the linux jokers please stop fucking over the open sound system please'


#include "quakedef.h"
#ifdef AUDIO_ALSA
#include <alsa/asoundlib.h>
#include <dlfcn.h>

static void *alsasharedobject;

static int (*psnd_pcm_open)				(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode);
static int (*psnd_pcm_close)				(snd_pcm_t *pcm);
static int (*psnd_config_update_free_global)(void);
static const char *(*psnd_strerror)			(int errnum);
static int (*psnd_pcm_hw_params_any)			(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
static int (*psnd_pcm_hw_params_set_access)		(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access);
static int (*psnd_pcm_hw_params_set_format)		(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val);
static int (*psnd_pcm_hw_params_set_channels)		(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val);
static int (*psnd_pcm_hw_params_set_rate_near)		(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
static int (*psnd_pcm_hw_params_set_period_size_near)	(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val, int *dir);
static int (*psnd_pcm_hw_params)			(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
static int (*psnd_pcm_sw_params_current)		(snd_pcm_t *pcm, snd_pcm_sw_params_t *params);
static int (*psnd_pcm_sw_params_set_start_threshold)	(snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_uframes_t val);
static int (*psnd_pcm_sw_params_set_stop_threshold)	(snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_uframes_t val);
static int (*psnd_pcm_sw_params)			(snd_pcm_t *pcm, snd_pcm_sw_params_t *params);
static int (*psnd_pcm_hw_params_get_buffer_size)	(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val);
static int (*psnd_pcm_hw_params_set_buffer_size_near)	(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val);
static int (*psnd_pcm_set_params)			(snd_pcm_t *pcm, snd_pcm_format_t format, snd_pcm_access_t access, unsigned int channels, unsigned int rate, int soft_resample, unsigned int latency);
static snd_pcm_sframes_t (*psnd_pcm_avail_update)	(snd_pcm_t *pcm);
static snd_pcm_state_t (*psnd_pcm_state)		(snd_pcm_t *pcm);
static int (*psnd_pcm_start)				(snd_pcm_t *pcm);
static int (*psnd_pcm_recover)				(snd_pcm_t *pcm, int err, int silent);

static size_t (*psnd_pcm_hw_params_sizeof)		(void);
static size_t (*psnd_pcm_sw_params_sizeof)		(void);

static int (*psnd_pcm_mmap_begin)			(snd_pcm_t *pcm, const snd_pcm_channel_area_t **areas, snd_pcm_uframes_t *offset, snd_pcm_uframes_t *frames);
static snd_pcm_sframes_t (*psnd_pcm_mmap_commit)	(snd_pcm_t *pcm, snd_pcm_uframes_t offset, snd_pcm_uframes_t frames);

static snd_pcm_sframes_t (*psnd_pcm_writei)		(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size);
static int (*psnd_pcm_prepare)				(snd_pcm_t *pcm);

static int 	(*psnd_device_name_hint)		(int card, const char *iface, void ***hints);
static char * 	(*psnd_device_name_get_hint)	(const void *hint, const char *id);
static int		(*psnd_device_name_free_hint)	(void **hints);


static unsigned int ALSA_MMap_GetDMAPos (soundcardinfo_t *sc)
{
	const snd_pcm_channel_area_t *areas;
	snd_pcm_uframes_t offset;
	snd_pcm_uframes_t nframes = sc->sn.samples / sc->sn.numchannels;

	psnd_pcm_avail_update (sc->handle);
	psnd_pcm_mmap_begin (sc->handle, &areas, &offset, &nframes);
	offset *= sc->sn.numchannels;
	nframes *= sc->sn.numchannels;
	sc->sn.samplepos = offset;
	sc->sn.buffer = areas->addr;
	return sc->sn.samplepos;
}

static void ALSA_MMap_Submit (soundcardinfo_t *sc, int start, int end)
{
	int			state;
	int			count = end - start;
	const snd_pcm_channel_area_t *areas;
	snd_pcm_uframes_t nframes;
	snd_pcm_uframes_t offset;

	nframes = count / sc->sn.numchannels;

	psnd_pcm_avail_update (sc->handle);
	psnd_pcm_mmap_begin (sc->handle, &areas, &offset, &nframes);

	state = psnd_pcm_state (sc->handle);

	switch (state) {
		case SND_PCM_STATE_PREPARED:
			psnd_pcm_mmap_commit (sc->handle, offset, nframes);
			psnd_pcm_start (sc->handle);
			break;
		case SND_PCM_STATE_RUNNING:
			psnd_pcm_mmap_commit (sc->handle, offset, nframes);
			break;
		default:
			break;
	}
}
static unsigned int ALSA_RW_GetDMAPos (soundcardinfo_t *sc)
{
	int frames;
	frames = psnd_pcm_avail_update(sc->handle);
	if (frames < 0)
	{
		psnd_pcm_start (sc->handle);
		psnd_pcm_recover(sc->handle, frames, true);
		frames = psnd_pcm_avail_update(sc->handle);
	}
	if (frames >= 0)
	{
		sc->sn.samplepos = (sc->snd_sent + frames) * sc->sn.numchannels;
	}
	return sc->sn.samplepos;
}
static void ALSA_RW_Submit (soundcardinfo_t *sc, int start, int end)
{
//	int			state;
	unsigned int frames, offset, ringsize;
	unsigned chunk;
	int result;
	int stride = sc->sn.numchannels * sc->sn.samplebytes;

	while(1)
	{
		/*we can't change the data that was already written*/
		frames = end - sc->snd_sent;
		if (frames <= 0)
			return;

//		state = psnd_pcm_state (sc->handle);

		ringsize = sc->sn.samples / sc->sn.numchannels;

		chunk = frames;
		offset = sc->snd_sent % ringsize;

		if (offset + chunk >= ringsize)
			chunk = ringsize - offset;
		result = psnd_pcm_writei(sc->handle, sc->sn.buffer + offset*stride, chunk);
		if (result < chunk)
		{
			if (result < 0)
				return;
		}
		sc->snd_sent += chunk;

		chunk = frames - chunk;
		if (chunk)
		{
			result = psnd_pcm_writei(sc->handle, sc->sn.buffer, chunk);
			if (result > 0)
				sc->snd_sent += result;
		}

//		if (state == SND_PCM_STATE_PREPARED)
//			psnd_pcm_start (sc->handle);
	};
}

static void ALSA_Shutdown (soundcardinfo_t *sc)
{
	psnd_pcm_close (sc->handle);
	psnd_config_update_free_global();	//and try to reduce leaks

	if (sc->Submit == ALSA_RW_Submit)
		free(sc->sn.buffer);

	Con_DPrintf("Alsa closed\n");
}

static void *ALSA_LockBuffer(soundcardinfo_t *sc, unsigned int *sampidx)
{
	return sc->sn.buffer;
}

static void ALSA_UnlockBuffer(soundcardinfo_t *sc, void *buffer)
{
}

static qboolean Alsa_InitAlsa(void)
{
	static qboolean tried;
	static qboolean alsaworks;

	static dllfunction_t funcs[] =
	{
		{(void**)&psnd_pcm_open,							"snd_pcm_open"},
		{(void**)&psnd_pcm_close,							"snd_pcm_close"},
		{(void**)&psnd_config_update_free_global,			"snd_config_update_free_global"},
		{(void**)&psnd_strerror,							"snd_strerror"},
		{(void**)&psnd_pcm_hw_params_any,					"snd_pcm_hw_params_any"},
		{(void**)&psnd_pcm_hw_params_set_access,			"snd_pcm_hw_params_set_access"},
		{(void**)&psnd_pcm_hw_params_set_format,			"snd_pcm_hw_params_set_format"},
		{(void**)&psnd_pcm_hw_params_set_channels,			"snd_pcm_hw_params_set_channels"},
		{(void**)&psnd_pcm_hw_params_set_rate_near,			"snd_pcm_hw_params_set_rate_near"},
		{(void**)&psnd_pcm_hw_params_set_period_size_near,	"snd_pcm_hw_params_set_period_size_near"},
		{(void**)&psnd_pcm_hw_params,						"snd_pcm_hw_params"},
		{(void**)&psnd_pcm_sw_params_current,				"snd_pcm_sw_params_current"},
		{(void**)&psnd_pcm_sw_params_set_start_threshold,	"snd_pcm_sw_params_set_start_threshold"},
		{(void**)&psnd_pcm_sw_params_set_stop_threshold,	"snd_pcm_sw_params_set_stop_threshold"},
		{(void**)&psnd_pcm_sw_params,						"snd_pcm_sw_params"},
		{(void**)&psnd_pcm_hw_params_get_buffer_size,		"snd_pcm_hw_params_get_buffer_size"},
		{(void**)&psnd_pcm_avail_update,					"snd_pcm_avail_update"},
		{(void**)&psnd_pcm_state,							"snd_pcm_state"},
		{(void**)&psnd_pcm_start,							"snd_pcm_start"},
		{(void**)&psnd_pcm_recover,							"snd_pcm_recover"},
		{(void**)&psnd_pcm_set_params,						"snd_pcm_set_params"},
		{(void**)&psnd_pcm_hw_params_sizeof,				"snd_pcm_hw_params_sizeof"},
		{(void**)&psnd_pcm_sw_params_sizeof,				"snd_pcm_sw_params_sizeof"},
		{(void**)&psnd_pcm_hw_params_set_buffer_size_near,	"snd_pcm_hw_params_set_buffer_size_near"},

		{(void**)&psnd_pcm_mmap_begin,						"snd_pcm_mmap_begin"},
		{(void**)&psnd_pcm_mmap_commit,						"snd_pcm_mmap_commit"},

		{(void**)&psnd_pcm_writei,							"snd_pcm_writei"},
		{(void**)&psnd_pcm_prepare,							"snd_pcm_prepare"},

		{(void**)&psnd_device_name_hint,					"snd_device_name_hint"},
		{(void**)&psnd_device_name_get_hint,				"snd_device_name_get_hint"},
		{(void**)&psnd_device_name_free_hint,				"snd_device_name_free_hint"},
		{NULL,NULL}
	};

	if (tried)
		return alsaworks;
	tried = true;

	//pulseaudio's wrapper library fucks with alsa in bad ways, making it unusable on some systems.
	if (COM_CheckParm("-noalsa"))
		return false;

	// Try alternative names of libasound, sometimes it is not linked correctly.
	alsasharedobject = Sys_LoadLibrary("libasound.so.2", funcs);
	if (!alsasharedobject)
		alsasharedobject = Sys_LoadLibrary("libasound.so", funcs);
	if (!alsasharedobject)
		return false;

	alsaworks = true;
	return alsaworks;
}

static qboolean QDECL ALSA_InitCard (soundcardinfo_t *sc, const char *pcmname)
{
	snd_pcm_t   *pcm;
	snd_pcm_uframes_t buffer_size;

	int					 err;
	snd_pcm_hw_params_t	*hw;
	snd_pcm_sw_params_t	*sw;
#if 0
	int					 bps, stereo;
	unsigned int		 rate;
	snd_pcm_uframes_t	 frag_size;
#endif
	qboolean mmap = false;

	if (!Alsa_InitAlsa())
	{
		Con_Printf(CON_ERROR "Alsa does not appear to be installed or compatible\n");
		return false;
	}

	hw = alloca(psnd_pcm_hw_params_sizeof());
	sw = alloca(psnd_pcm_sw_params_sizeof());
	memset(sw, 0, psnd_pcm_sw_params_sizeof());
	memset(hw, 0, psnd_pcm_hw_params_sizeof());

//WARNING: 'default' as the default sucks arse. it adds about a second's worth of lag.
	if (!pcmname)
		pcmname = "default";

	sc->inactive_sound = true;	//linux sound devices always play sound, even when we're not the active app...

	Con_Printf("Initing ALSA sound device \"%s\"\n", pcmname);

	err = psnd_pcm_open (&pcm, pcmname, SND_PCM_STREAM_PLAYBACK,
						  SND_PCM_NONBLOCK);
	if (0 > err)
	{
		Con_Printf (CON_ERROR "ALSA Error: open error (%s): %s\n", pcmname, psnd_strerror (err));
		return 0;
	}
	Con_Printf ("ALSA: Using PCM %s.\n", pcmname);

#if 1
	if (!sc->sn.sampleformat)
	{
		if (sc->sn.samplebytes >= 4)
			sc->sn.sampleformat = QSF_F32;
		else if (sc->sn.samplebytes != 1)
			sc->sn.sampleformat = QSF_S16;
		else
			sc->sn.sampleformat = QSF_U8;
	}
	switch(sc->sn.sampleformat)
	{
	case QSF_U8:	err = SND_PCM_FORMAT_U8;	sc->sn.samplebytes=1; break;
	case QSF_S8:	err = SND_PCM_FORMAT_S8;	sc->sn.samplebytes=1; break;
	case QSF_S16:	err = SND_PCM_FORMAT_S16;	sc->sn.samplebytes=2; break;
	case QSF_F32:	err = SND_PCM_FORMAT_FLOAT;	sc->sn.samplebytes=4; break;
	default:
		Con_Printf (CON_ERROR "ALSA: unsupported sample format %i\n", sc->sn.sampleformat);
		goto error;
	}

	err = psnd_pcm_set_params(pcm, err, (mmap?SND_PCM_ACCESS_MMAP_INTERLEAVED:SND_PCM_ACCESS_RW_INTERLEAVED), sc->sn.numchannels, sc->sn.speed, true, 0.04*1000000);
	if (0 > err)
	{
		Con_Printf (CON_ERROR "ALSA: error setting params. %s\n", psnd_strerror (err));
		goto error;
	}

//	sc->sn.numchannels = stereo;
//	sc->sn.samplepos = 0;
//	sc->sn.samplebytes = bps/8;

	sc->samplequeue = buffer_size = 2048;
#else	
	err = psnd_pcm_hw_params_any (pcm, hw);
	if (0 > err)
	{
		Con_Printf (CON_ERROR "ALSA: error setting hw_params_any. %s\n", psnd_strerror (err));
		goto error;
	}

	err = psnd_pcm_hw_params_set_access (pcm, hw,  mmap?SND_PCM_ACCESS_MMAP_INTERLEAVED:SND_PCM_ACCESS_RW_INTERLEAVED);
	if (0 > err)
	{
		Con_Printf (CON_ERROR "ALSA: Failure to set interleaved PCM access. %s\n", psnd_strerror (err));
		goto error;
	}

	// get sample bit size
	bps = sc->sn.samplebytes*8;
	{
		snd_pcm_format_t spft;
		if (bps == 16)
			spft = SND_PCM_FORMAT_S16;
		else
			spft = SND_PCM_FORMAT_U8;

		err = psnd_pcm_hw_params_set_format (pcm, hw, spft);
		while (err < 0)
		{
			if (spft == SND_PCM_FORMAT_S16)
			{
				bps = 8;
				spft = SND_PCM_FORMAT_U8;
			}
			else
			{
				Con_Printf (CON_ERROR "ALSA: no usable formats. %s\n", psnd_strerror (err));
				goto error;
			}
			err = psnd_pcm_hw_params_set_format (pcm, hw, spft);
		}
	}

	// get speaker channels
	stereo = sc->sn.numchannels;
	err = psnd_pcm_hw_params_set_channels (pcm, hw, stereo);
	while (err < 0)
	{
		if (stereo > 2)
			stereo = 2;
		else if (stereo > 1)
			stereo = 1;
		else
		{
			Con_Printf (CON_ERROR "ALSA: no usable number of channels. %s\n", psnd_strerror (err));
			goto error;
		}
		err = psnd_pcm_hw_params_set_channels (pcm, hw, stereo);
	}

	// get rate
	rate = sc->sn.speed;
	err = psnd_pcm_hw_params_set_rate_near (pcm, hw, &rate, 0);
	while (err < 0)
	{
		if (rate > 48000)
			rate = 48000;
		else if (rate > 44100)
			rate = 44100;
		else if (rate > 22150)
			rate = 22150;
		else if (rate > 11025)
			rate = 11025;
		else if (rate > 800)
			rate = 800;
		else
		{
			Con_Printf (CON_ERROR "ALSA: no usable rates. %s\n", psnd_strerror (err));
			goto error;
		}
		err = psnd_pcm_hw_params_set_rate_near (pcm, hw, &rate, 0);
	}

	if (rate > 11025)
		frag_size = 8 * bps * rate / 11025;
	else
		frag_size = 8 * bps;

	err = psnd_pcm_hw_params_set_period_size_near (pcm, hw, &frag_size, 0);
	if (0 > err)
	{
		Con_Printf (CON_ERROR "ALSA: unable to set period size near %i. %s\n", (int) frag_size, psnd_strerror (err));
		goto error;
	}
	err = psnd_pcm_hw_params (pcm, hw);
	if (0 > err) {
		Con_Printf (CON_ERROR "ALSA: unable to install hw params: %s\n",
					psnd_strerror (err));
		goto error;
	}
	err = psnd_pcm_sw_params_current (pcm, sw);
	if (0 > err) {
		Con_Printf (CON_ERROR "ALSA: unable to determine current sw params. %s\n", psnd_strerror (err));
		goto error;
	}
	err = psnd_pcm_sw_params_set_start_threshold (pcm, sw, ~0U);
	if (0 > err) {
		Con_Printf (CON_ERROR "ALSA: unable to set playback threshold. %s\n", psnd_strerror (err));
		goto error;
	}
	err = psnd_pcm_sw_params_set_stop_threshold (pcm, sw, ~0U);
	if (0 > err) {
		Con_Printf (CON_ERROR "ALSA: unable to set playback stop threshold. %s\n", psnd_strerror (err));
		goto error;
	}
	err = psnd_pcm_sw_params (pcm, sw);
	if (0 > err) {
		Con_Printf (CON_ERROR "ALSA: unable to install sw params. %s\n", psnd_strerror (err));
		goto error;
	}

	sc->sn.numchannels = stereo;
	sc->sn.samplepos = 0;
	sc->sn.samplebytes = bps/8;

	buffer_size = sc->sn.samples / stereo;
	if (buffer_size)
	{
		err = psnd_pcm_hw_params_set_buffer_size_near(pcm, hw, &buffer_size);
		if (err < 0)
		{
			Con_Printf (CON_ERROR "ALSA: unable to set buffer size. %s\n", psnd_strerror (err));
			goto error;
		}
	}

	err = psnd_pcm_hw_params_get_buffer_size (hw, &buffer_size);
	if (0 > err) {
		Con_Printf (CON_ERROR "ALSA: unable to get buffer size. %s\n", psnd_strerror (err));
		goto error;
	}
	sc->sn.speed = rate;
#endif

	sc->sn.samples = buffer_size * sc->sn.numchannels;		// mono samples in buffer
	sc->handle = pcm;

	sc->Lock		= ALSA_LockBuffer;
	sc->Unlock		= ALSA_UnlockBuffer;
	sc->Shutdown		= ALSA_Shutdown;
	if (mmap)
	{
		sc->GetDMAPos	= ALSA_MMap_GetDMAPos;
		sc->Submit	= ALSA_MMap_Submit;
		sc->GetDMAPos(sc);		// sets shm->buffer

		//alsa doesn't seem to like high mixahead values
		//(maybe it tells us above somehow...)
		//so force it lower
		//quake's default of 0.2 was for 10fps rendering anyway
		//so force it down to 0.1 which is the default for halflife at least, and should give better latency
		{
			extern cvar_t _snd_mixahead;
			if (_snd_mixahead.value >= 0.2)
			{
				Con_Printf("Alsa Hack: _snd_mixahead forced lower\n");
				_snd_mixahead.value = 0.1;
			}
		}
	}
	else
	{
		sc->GetDMAPos	= ALSA_RW_GetDMAPos;
		sc->Submit	= ALSA_RW_Submit;

		sc->samplequeue = sc->sn.samples;
		sc->sn.buffer = malloc(sc->sn.samples * sc->sn.samplebytes);

		err = psnd_pcm_prepare(pcm);
		if (0 > err)
		{
			Con_Printf (CON_ERROR "ALSA: unable to prepare for use. %s\n", psnd_strerror (err));
			goto error;
		}
	}

	return true;

error:
	psnd_pcm_close (pcm);
	return false;
}
#define SDRVNAME "ALSA"
static qboolean QDECL ALSA_Enumerate(void (QDECL *cb) (const char *drivername, const char *devicecode, const char *readablename))
{
	size_t i;
	void **hints;

	if (Alsa_InitAlsa())
	{
		if (!psnd_device_name_hint(-1, "pcm", &hints))
		{
			for (i = 0; hints[i]; i++)
			{
				char *n = psnd_device_name_get_hint(hints[i], "NAME");
				if (n)
				{
					char *t = psnd_device_name_get_hint(hints[i], "IOID");
					if (!t || strcasecmp(t, "Input"))
					{
						char *d = psnd_device_name_get_hint(hints[i], "DESC");
						if (d)
							cb(SDRVNAME, n, va("ALSA:%s", d));
						else
							cb(SDRVNAME, n, n);
						free(d);
					}
					free(t);
					free(n);	//dangerous to free things across boundaries.
				}
			}
			psnd_device_name_free_hint(hints);
		}
		else
			return false;
	}
	return true;
}
sounddriver_t ALSA_Output =
{
	SDRVNAME,
	ALSA_InitCard,
	ALSA_Enumerate
};
#endif
