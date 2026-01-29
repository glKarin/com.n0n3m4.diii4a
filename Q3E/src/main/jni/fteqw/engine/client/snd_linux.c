#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#if defined(__OpenBSD__)
	#include <soundcard.h>	//OpenBSD emulates this, so its no longer sys/.
#else
	#include <sys/soundcard.h>
#endif
#include <stdio.h>
#include "quakedef.h"

#ifdef HAVE_MIXER

#ifdef __linux__
#include <sys/stat.h>
#endif

#ifndef AFMT_FLOAT
	#define AFMT_FLOAT 0x00004000	//OSS4 supports it, but linux is too shit to define it.
#endif


static int tryrates[] = { 11025, 22051, 44100, 8000, 48000 };

static unsigned int OSS_MMap_GetDMAPos(soundcardinfo_t *sc)
{
	struct count_info count;

	if (sc->audio_fd != -1)
	{
		if (ioctl(sc->audio_fd, SNDCTL_DSP_GETOPTR, &count)==-1)
		{
			perror("/dev/dsp");
			Con_Printf("Uh, sound dead.\n");
			close(sc->audio_fd);
			sc->audio_fd = -1;
			return 0;
		}
//		shm->samplepos = (count.bytes / shm->samplebytes) & (shm->samples-1);
//		fprintf(stderr, "%d    \r", count.ptr);
		sc->sn.samplepos = count.ptr / sc->sn.samplebytes;
	}
	return sc->sn.samplepos;

}
static void OSS_MMap_Submit(soundcardinfo_t *sc, int start, int end)
{
}

static unsigned int OSS_Alsa_GetDMAPos(soundcardinfo_t *sc)
{
	struct audio_buf_info info;
	unsigned int bytes;
	if (ioctl (sc->audio_fd, SNDCTL_DSP_GETOSPACE, &info) != -1)
	{
		bytes = sc->snd_sent + info.bytes;
		sc->sn.samplepos = bytes / sc->sn.samplebytes;
	}
	return sc->sn.samplepos;
}


static void OSS_Alsa_Submit(soundcardinfo_t *sc, int start, int end)
{
	unsigned int bytes, offset, ringsize;
	unsigned chunk;
	int result;

	/*we can't change the data that was already written*/
	bytes = end * sc->sn.numchannels * sc->sn.samplebytes;
	bytes -= sc->snd_sent;
	if (!bytes)
		return;

	ringsize = sc->sn.samples * sc->sn.samplebytes;

	chunk = bytes;
	offset = sc->snd_sent % ringsize;

	if (offset + chunk >= ringsize)
		chunk = ringsize - offset;
	result = write(sc->audio_fd, sc->sn.buffer + offset, chunk);
	if (result < chunk)
	{
		if (result >= 0)
			sc->snd_sent += result;
//		printf("full?\n");
		return;
	}
	sc->snd_sent += chunk;

	chunk = bytes - chunk;
	if (chunk)
	{
		result = write(sc->audio_fd, sc->sn.buffer, chunk);
		if (result > 0)
			sc->snd_sent += result;
	}
}

static void OSS_Shutdown(soundcardinfo_t *sc)
{
	if (sc->sn.buffer)	//close it properly, so we can go and restart it later.
	{
		if (sc->Submit == OSS_Alsa_Submit)
			free(sc->sn.buffer); /*if using alsa-compat, just free the buffer*/
		else
			munmap(sc->sn.buffer, sc->sn.samples * sc->sn.samplebytes);
	}
	if (sc->audio_fd != -1)
		close(sc->audio_fd);
	*sc->name = '\0';
}

static void *OSS_Lock(soundcardinfo_t *sc, unsigned int *sampidx)
{
	return sc->sn.buffer;
}

static void OSS_Unlock(soundcardinfo_t *sc, void *buffer)
{
}

static qboolean OSS_InitCard(soundcardinfo_t *sc, const char *snddev)
{	//FIXME: implement snd_multipledevices somehow.
	int rc;
	int fmt;
	int tmp;
	int i;
	struct audio_buf_info info;
	int caps;
	qboolean alsadetected = false;

#ifdef __linux__
	struct stat sb;
	if (stat("/proc/asound", &sb) != -1)
		alsadetected = true;
#endif

	if (COM_CheckParm("-nooss"))
		return false;

	if (!snddev || !*snddev)
		snddev = "/dev/dsp";
	else if (strncmp(snddev, "/dev/dsp", 8))
	{
		Con_Printf("Refusing to use non-dsp device\n");
		return false;
	}

	sc->inactive_sound = true;	//linux sound devices always play sound, even when we're not the active app...

// open the sound device, confirm capability to mmap, and get size of dma buffer

	Con_Printf("Initing OSS sound device %s\n", snddev);

#ifdef __linux__
	//linux is a pile of shit.
	//nonblock is needed to get around issues with the old/buggy linux oss3 clone implementation, as well as because this code is too lame to thread audio.
	sc->audio_fd = open(snddev, O_RDWR | O_NONBLOCK);	//try the primary device
	//fixme: the following is desired once threading is supported.
	//int flags = fcntl(fd, F_GETFL, 0);
	//fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
#else
	//FIXME: remove non-block if we're using threads.
	sc->audio_fd = open(snddev, O_WRONLY | O_NONBLOCK);	//try the primary device
#endif
	if (sc->audio_fd < 0)
	{
		perror(snddev);
		Con_Printf(CON_ERROR "OSS: Could not open %s\n", snddev);
		OSS_Shutdown(sc);
		return false;
	}
	Q_strncpyz(sc->name, snddev, sizeof(sc->name));

//reset it
	rc = ioctl(sc->audio_fd, SNDCTL_DSP_RESET, 0);
	if (rc < 0)
	{
		perror(snddev);
		Con_Printf(CON_ERROR "OSS: Could not reset %s\n", snddev);
		OSS_Shutdown(sc);
		return false;
	}

//check its general capabilities, we need trigger+mmap
	if (ioctl(sc->audio_fd, SNDCTL_DSP_GETCAPS, &caps)==-1)
	{
		perror(snddev);
		Con_Printf(CON_ERROR "OSS: Sound driver too old\n");
		OSS_Shutdown(sc);
		return false;
	}

//choose channels
#ifdef SNDCTL_DSP_CHANNELS /*I'm paranoid, okay?*/
	tmp = sc->sn.numchannels;
	rc = ioctl(sc->audio_fd, SNDCTL_DSP_CHANNELS, &tmp);
	if (rc < 0)
	{
		perror(snddev);
		Con_Printf(CON_ERROR "OSS: Could not set %s to channels=%d\n", snddev, sc->sn.numchannels);
		OSS_Shutdown(sc);
		return false;
	}
	sc->sn.numchannels = tmp;
#else
	tmp = 0;
	if (sc->sn.numchannels == 2)
		tmp = 1;
	rc = ioctl(sc->audio_fd, SNDCTL_DSP_STEREO, &tmp);
	if (rc < 0)
	{
		perror(snddev);
		Con_Printf(CON_ERROR "OSS: Could not set %s to stereo=%d\n", snddev, sc->sn.numchannels);
		OSS_Shutdown(sc);
		return false;
	}
	if (tmp)
		sc->sn.numchannels = 2;
	else
		sc->sn.numchannels = 1;
#endif

	// ask the device what it supports
	ioctl(sc->audio_fd, SNDCTL_DSP_GETFMTS, &fmt);

	//choose a format
	if (sc->sn.samplebytes >= 4 && (fmt & AFMT_FLOAT))
	{
		sc->sn.samplebytes = 4;
		sc->sn.sampleformat = QSF_F32;
		rc = AFMT_FLOAT;
		rc = ioctl(sc->audio_fd, SNDCTL_DSP_SETFMT, &rc);
		if (rc < 0)
		{
			perror(snddev);
			Con_Printf(CON_ERROR "OSS: Could not support 16-bit data.  Try 8-bit.\n");
			OSS_Shutdown(sc);
			return false;
		}
	}
	else if (sc->sn.samplebytes >= 2 && (fmt & AFMT_S16_NE))
	{
		sc->sn.samplebytes = 2;
		sc->sn.sampleformat = QSF_S16;
		rc = AFMT_S16_NE;
		rc = ioctl(sc->audio_fd, SNDCTL_DSP_SETFMT, &rc);
		if (rc < 0)
		{
			perror(snddev);
			Con_Printf(CON_ERROR "OSS: Could not support 16-bit data.  Try 8-bit.\n");
			OSS_Shutdown(sc);
			return false;
		}
	}
	else if (/*sc->sn.samplebytes == 1 && */(fmt & AFMT_U8))
	{
		sc->sn.samplebytes = 1;
		sc->sn.sampleformat = QSF_U8;
		rc = AFMT_U8;
		rc = ioctl(sc->audio_fd, SNDCTL_DSP_SETFMT, &rc);
		if (rc < 0)
		{
			perror(snddev);
			Con_Printf(CON_ERROR "OSS: Could not support 8-bit data.\n");
			OSS_Shutdown(sc);
			return false;
		}
	}
	else if (/*sc->sn.samplebytes == 1 && */(fmt & AFMT_S8))
	{
		sc->sn.samplebytes = 1;
		sc->sn.sampleformat = QSF_S8;
		rc = AFMT_S8;
		rc = ioctl(sc->audio_fd, SNDCTL_DSP_SETFMT, &rc);
		if (rc < 0)
		{
			perror(snddev);
			Con_Printf(CON_ERROR "OSS: Could not support 8-bit data.\n");
			OSS_Shutdown(sc);
			return false;
		}
	}
	else
	{
		perror(snddev);
		Con_Printf(CON_ERROR "OSS: %d-bit sound not supported.\n", sc->sn.samplebytes*8);
		OSS_Shutdown(sc);
		return false;
	}

//choose speed
	//use the default - menu set value.
	tmp = sc->sn.speed;
	if (ioctl(sc->audio_fd, SNDCTL_DSP_SPEED, &tmp) != 0)
	{	//humph, default didn't work. Go for random preset ones that should work.
		for (i=0 ; i<sizeof(tryrates)/4 ; i++)
		{
			tmp = tryrates[i];
			if (!ioctl(sc->audio_fd, SNDCTL_DSP_SPEED, &tmp)) break;
		}
		if (i == (sizeof(tryrates)/4))
		{
			perror(snddev);
			Con_Printf(CON_ERROR "OSS: Failed to obtain a suitable rate\n");
			OSS_Shutdown(sc);
			return false;
		}
	}
	sc->sn.speed = tmp;

//figure out buffer size
	if (ioctl(sc->audio_fd, SNDCTL_DSP_GETOSPACE, &info)==-1)
	{
		perror("GETOSPACE");
		Con_Printf(CON_ERROR "OSS: Um, can't do GETOSPACE?\n");
		OSS_Shutdown(sc);
		return false;
	}
	sc->sn.samples = info.fragstotal * info.fragsize;
	sc->sn.samples /= sc->sn.samplebytes;
	/*samples is the number of samples*channels */

// memory map the dma buffer
	sc->sn.buffer = MAP_FAILED;
	if (alsadetected)
	{
		Con_Printf("Refusing to mmap oss device in case alsa's oss emulation crashes.\n");
	}
	else if ((caps & DSP_CAP_TRIGGER) && (caps & DSP_CAP_MMAP))
	{
		sc->sn.buffer = (unsigned char *) mmap(NULL, sc->sn.samples*sc->sn.samplebytes, PROT_WRITE, MAP_FILE|MAP_SHARED, sc->audio_fd, 0);
		if (sc->sn.buffer == MAP_FAILED)
		{
			Con_Printf("%s: device reported mmap capability, but mmap failed.\n", snddev);
			if (alsadetected)
			{
				char *f, *n;
				f = (char *)com_argv[0];
				while((n = strchr(f, '/')))
					f = n + 1;
				Con_Printf("Your system is running alsa.\nTry: sudo echo \"%s 0 0 direct\" > /proc/asound/card0/pcm0p/oss\n", f);
			}
		}
	}
	if (sc->sn.buffer == MAP_FAILED)
	{
		sc->sn.buffer = NULL;

		sc->samplequeue = info.bytes / sc->sn.samplebytes;
		sc->sn.samples*=2;
		sc->sn.buffer = malloc(sc->sn.samples*sc->sn.samplebytes);
		sc->Submit		= OSS_Alsa_Submit;
		sc->GetDMAPos	= OSS_Alsa_GetDMAPos;
	}
	else
	{
		// toggle the trigger & start her up
		tmp = 0;
		rc  = ioctl(sc->audio_fd, SNDCTL_DSP_SETTRIGGER, &tmp);
		if (rc < 0)
		{
			perror(snddev);
			Con_Printf(CON_ERROR "OSS: Could not toggle.\n");
			OSS_Shutdown(sc);
			return false;
		}
		tmp = PCM_ENABLE_OUTPUT;
		rc = ioctl(sc->audio_fd, SNDCTL_DSP_SETTRIGGER, &tmp);
		if (rc < 0)
		{
			perror(snddev);
			Con_Printf(CON_ERROR "OSS: Could not toggle.\n");
			OSS_Shutdown(sc);
			return false;
		}
		sc->Submit		= OSS_MMap_Submit;
		sc->GetDMAPos	= OSS_MMap_GetDMAPos;
	}

	sc->sn.samplepos = 0;

	sc->Lock		= OSS_Lock;
	sc->Unlock		= OSS_Unlock;
	sc->Shutdown	= OSS_Shutdown;

	return true;
}

#define SDRVNAME "OSS"
#if defined(__linux__) && !defined(SNDCTL_SYSINFO)
typedef struct oss_sysinfo {
	char product[32];   /* E.g. SunOS Audio */
	char version[32];   /* E.g. 4.0a */
	int versionnum;     /* See OSS_GETVERSION */
	char options[128];  /* NOT SUPPORTED */
	int numaudios;      /* # of audio/dsp devices */
	int openedaudio[8]; /* Reserved, always 0 */
	int numsynths;        /* NOT SUPPORTED, always 0 */
	int nummidis;         /* NOT SUPPORTED, always 0 */
	int numtimers;        /* NOT SUPPORTED, always 0 */
	int nummixers;        /* # of mixer devices */
	int openedmidi[8];    /* Mask of midi devices are busy */
	int numcards;         /* Number of sound cards in the system */
	int numaudioengines;  /* Number of audio engines in the system */
	char license[16];     /* E.g. "GPL" or "CDDL" */
	char revision_info[256];  /* Reserved */
	int filler[172];          /* Reserved */
} oss_sysinfo;
#define SNDCTL_SYSINFO          _IOR ('X', 1, oss_sysinfo)
#endif

#if defined(__linux__) && !defined(SNDCTL_AUDIOINFO)
typedef struct oss_audioinfo {
	int dev;  /* Device to query */
	char name[64];  /* Human readable name */
	int busy;  /* reserved */
	int pid;  /* reserved */
	int caps;  /* PCM_CAP_INPUT, PCM_CAP_OUTPUT */
	int iformats;  /* Supported input formats */
	int oformats;  /* Supported output formats */
	int magic;  /* reserved */
	char cmd[64];  /* reserved */
	int card_number;
	int port_number;  /* reserved */
	int mixer_dev;
	int legacy_device; /* Obsolete field. Replaced by devnode */
	int enabled;  /* reserved */
	int flags;  /* reserved */
	int min_rate;  /* Minimum sample rate */
	int max_rate;  /* Maximum sample rate */
	int min_channels;  /* Minimum number of channels */
	int max_channels;  /* Maximum number of channels */
	int binding;  /* reserved */
	int rate_source;  /* reserved */
	char handle[32];  /* reserved */
	unsigned int nrates;  /* reserved */
	unsigned int rates[20];  /* reserved */
	char song_name[64];  /* reserved */
	char label[16];  /* reserved */
	int latency;  /* reserved */
	char devnode[32];  /* Device special file name (absolute path) */
	int next_play_engine;  /* reserved */
	int next_rec_engine;  /* reserved */
	int filler[184];  /* reserved */
} oss_audioinfo;
#define SNDCTL_AUDIOINFO      _IOWR('X', 7, oss_audioinfo)
#endif

static qboolean QDECL OSS_Enumerate(void (QDECL *cb) (const char *drivername, const char *devicecode, const char *readablename))
{
#if defined(SNDCTL_SYSINFO) && defined(SNDCTL_AUDIOINFO)
	int i;
	int fd;
	oss_sysinfo si;
	const char *devmixer;

	if (COM_CheckParm("-nooss"))
		return true;

	devmixer = getenv("OSS_MIXERDEV");
	if (!devmixer)
		devmixer = "/dev/mixer";
	fd = open(devmixer, O_RDWR|O_NONBLOCK, 0);

	if (fd == -1)
		return true;	//oss not supported. don't list any devices.

	memset(&si, 0, sizeof(si));	//just in case the driver is really dodgy...
	if (ioctl(fd, SNDCTL_SYSINFO, &si) >= 0)
	{
		if ((si.versionnum>>16) >= 4 || si.numaudios > 128)
		{	//only trust all the fields if its recent enough and doesn't look dodgy.
			for(i = 0; i < si.numaudios; i++)
			{
				oss_audioinfo ai;
				memset(&ai, 0, sizeof(ai));	//just in case the driver is really dodgy...
				ai.dev = i;
				if (ioctl(fd, SNDCTL_AUDIOINFO, &ai) >= 0)
					cb(SDRVNAME, ai.devnode, ai.name);
			}
			close(fd);
			return true;
		}
		else
			printf("Not enumerating OSS %u.%u.%u devices.\n", (si.versionnum>>16)&0xffff, (si.versionnum>>8)&0xff, si.versionnum&0xff);
	}
	else
		printf("OSS driver is too old to support device enumeration.\n");
	close(fd);
#endif
	return false;	//enumeration failed, will show only a default device.
}

sounddriver_t OSS_Output =
{
	SDRVNAME,
	OSS_InitCard,
	OSS_Enumerate
};

#endif
#ifdef VOICECHAT	//this does apparently work after all.
#include <stdint.h>

static qboolean QDECL OSS_Capture_Enumerate (void (QDECL *callback) (const char *drivername, const char *devicecode, const char *readablename))
{
	if (COM_CheckParm("-nooss"))
		return true;	//no default devices or anything

	//open /dev/dsp or /dev/mixer or env("OSS_MIXERDEV") or something
	//SNDCTL_SYSINFO to get sysinfo.numcards
	//for i=0; i<sysinfo.numcards
	//SNDCTL_CARDINFO
	return false;
}
static void *OSS_Capture_Init(int rate, const char *snddev)
{
	int tmp;
	intptr_t fd;
	if (!snddev || !*snddev)
		snddev = "/dev/dsp";
	if (COM_CheckParm("-nooss"))
		return NULL;
	fd = open(snddev, O_RDONLY | O_NONBLOCK);       //try the primary device
	if (fd == -1)
		return NULL;

#ifdef SNDCTL_DSP_CHANNELS
	tmp = 1;
	if (ioctl(fd, SNDCTL_DSP_CHANNELS, &tmp) != 0)
#else
	tmp = 0;
	if (ioctl(fd, SNDCTL_DSP_STEREO, &tmp) != 0)
#endif
	{
		Con_Printf("Couldn't set mono\n");
		perror(snddev);
	}

	tmp = AFMT_S16_LE;
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &tmp) != 0)
	{
		Con_Printf("Couldn't set sample bits\n");
		perror(snddev);
	}

	tmp = rate;
	if (ioctl(fd, SNDCTL_DSP_SPEED, &tmp) != 0)
	{
		Con_Printf("Couldn't set capture rate\n");
		perror(snddev);
	}

	fd++;
	return (void*)fd;
}
static void OSS_Capture_Start(void *ctx)
{
	/*oss will automagically restart it when we next read*/
}

static void OSS_Capture_Stop(void *ctx)
{
	intptr_t fd = ((intptr_t)ctx)-1;

	ioctl(fd, SNDCTL_DSP_RESET, NULL);
}

static void OSS_Capture_Shutdown(void *ctx)
{
	intptr_t fd = ((intptr_t)ctx)-1;

	close(fd);
}

static unsigned int OSS_Capture_Update(void *ctx, unsigned char *buffer, unsigned int minbytes, unsigned int maxbytes)
{
	intptr_t fd = ((intptr_t)ctx)-1;
	ssize_t res;

	res = read(fd, buffer, maxbytes);
	if (res < 0)
		return 0;
	return res;
}

snd_capture_driver_t OSS_Capture =
{
	1,
	"OSS",
	OSS_Capture_Enumerate,
	OSS_Capture_Init,
	OSS_Capture_Start,
	OSS_Capture_Update,
	OSS_Capture_Stop,
	OSS_Capture_Shutdown
};
#endif
