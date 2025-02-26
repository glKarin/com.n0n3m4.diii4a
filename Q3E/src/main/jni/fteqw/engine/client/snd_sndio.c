/*
* Copyright (c) 2010 Jacob Meuser <jakemsr@sdf.lonestar.org>
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
/* Modifified for FTEQW by Alf Schlichting, a.schlichting@lemarit.com */
/* note: this is for OpenBSD */

#include "quakedef.h"
#include "sound.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

#include <sndio.h>

struct sndio_private
{
	struct sio_hdl *hdl;

	unsigned char *dma_buffer;
	size_t dma_buffer_size, dma_ptr;
};

static qboolean sndio_init(soundcardinfo_t *, const char *);
static void *sndio_lock(soundcardinfo_t *);
static void sndio_unlock(soundcardinfo_t *, void *);
static void sndio_shutdown(soundcardinfo_t *);
static unsigned int sndio_getdmapos(soundcardinfo_t *);
static void sndio_submit(soundcardinfo_t *, int, int);
static void sndio_setunderwater(soundcardinfo_t *sc, qboolean underwater);      //simply a stub. Any ideas how to actually implement this properly?

static void sndio_setunderwater(soundcardinfo_t *sc, qboolean underwater)       //simply a stub. Any ideas how to actually implement this properly?
{
}

static qboolean sndio_init(soundcardinfo_t *sc, const char *cardname)
{
	struct sndio_private *sp;
	struct sio_par par;
	unsigned samp_per_buf;
	char *s;
	int i;

	Con_DPrintf("sndio_init called\n");
	if (cardname && *cardname)
		return false;	//only support the default device for now.

	sp = calloc(sizeof(struct sndio_private), 1);
	if (sp == NULL)
	{
		Con_Printf("Could not get mem");
		return false;
	}

	Con_DPrintf("trying to open sp->hdl\n");
	sp->hdl = sio_open(SIO_DEVANY, SIO_PLAY, 1);
	if (sp->hdl == NULL)
	{
		Con_Printf("Could not open sndio device\n");
		return false;
	}
	Con_DPrintf("Opened sndio\n");
	sc->GetDMAPos = sndio_getdmapos;
	sc->Submit = sndio_submit;
	sc->Shutdown = sndio_shutdown;
	sc->Lock = sndio_lock;
	sc->Unlock = sndio_unlock;
	sc->SetWaterDistortion = sndio_setunderwater;
	sc->handle = sp;

	sio_initpar(&par);
	par.rate = sc->sn.speed;
	par.bits = (sc->sn.samplebytes==1)?8:16;
	par.sig = 1;
	par.le = SIO_LE_NATIVE;
	par.pchan = sc->sn.numchannels;
	par.appbufsz = par.rate / 20;   /* 1/20 second latency */

	if (!sio_setpar(sp->hdl, &par) || !sio_getpar(sp->hdl, &par))
	{
		Con_Printf("Error setting audio parameters\n");
		sio_close(sp->hdl);
		return false;
	}
	if ((par.pchan != 1 && par.pchan != 2) ||
		(par.bits != 16 || par.sig != 1))
	{
		Con_Printf("Could not set appropriate audio parameters\n");
		sio_close(sp->hdl);
		return false;
	}

	if (par.bits == 16)
	{
		sc->sn.sampleformat = QSF_S16;
		sc->sn.samplebytes = 2;
	}
	else if (par.bits == 8)
	{
		sc->sn.sampleformat = QSF_U8;
		sc->sn.samplebytes = 1;
	}
/*	sc->sn.speed = par.rate;
	sc->sn.numchannels = par.pchan;
	sc->sn.samplebits = par.bits;
*/

	/*
	* find the smallest power of two larger than the buffer size
	* and use it as the internal buffer's size
	*/
	for (i = 1; i < par.appbufsz; i <<= 1)
		; /* nothing */
	sc->sn.samples = i * par.pchan;

	sp->dma_buffer_size = sc->sn.samples * sc->sn.samplebytes;
	sc->sn.buffer = calloc(1, sp->dma_buffer_size);
	if (sc->sn.buffer == NULL)
	{
		Con_Printf("Could not allocate audio ring buffer\n");
		return false;
	}
	dma_ptr = 0;
	if (!sio_start(sp->hdl))
	{
		Con_Printf("Could not start audio\n");
		sio_close(sp->hdl);
		return false;
	}
	sc->sn.samplepos = 0;

	Con_DPrintf("sc->sn.speed = %d, par.rate = %d\n", sc->sn.speed, par.rate);
	Con_DPrintf("sc->sn.samplebits = %d, par.bits = %d\n", sc->sn.samplebytes*8, par.bits);
	Con_DPrintf("sc->sn.numchannels = %d, par.pchan = %d\n", sc->sn.numchannels, par.pchan);
	Con_DPrintf("sc->sn.samples = %d, par.pchan = %d\n", sc->sn.samples, par.pchan);
	Con_DPrintf("dma_buffer_size = %d\n", sp->dma_buffer_size);
	return true;
}


static void *
sndio_lock(soundcardinfo_t *sc, unsigned int *sampidx)
{
	return sc->sn.buffer;
}

static void
sndio_unlock(soundcardinfo_t *sci, void *p)
{
}

static void
sndio_shutdown(soundcardinfo_t *sc)
{
	struct sndio_private *sp = sc->handle;

	sio_close(sp->hdl);
	free(sc->sn.buffer);
	sc->sn.buffer = NULL;
	*sc->name = '\0';
}

static unsigned int
sndio_getdmapos(soundcardinfo_t *sc)
{
	struct sndio_private *sp = sc->handle;
	sc->sn.samplepos = dma_ptr / sc->sn.samplebytes;
	return sc->sn.samplepos;
}

static void
sndio_submit(soundcardinfo_t *sc, int startcount, int endcount)
{
	struct pollfd pfd;
	struct sndio_private *sp = sc->handle;
	size_t count, todo, avail;
	int n;

	n = sio_pollfd(sp->hdl, &pfd, POLLOUT);
	while (poll(&pfd, n, 0) < 0 && errno == EINTR)
		;
	if (!(sio_revents(sp->hdl, &pfd) & POLLOUT))
		return;
	avail = sp->dma_buffer_size;
	while (avail > 0)
	{
		todo = sp->dma_buffer_size - dma_ptr;
		if (todo > avail)
			todo = avail;
		count = sio_write(sp->hdl, sc->sn.buffer + dma_ptr, todo);
		if (count == 0)
			break;
		dma_ptr += count;
		if (dma_ptr >= sp->dma_buffer_size)
			dma_ptr -= sp->dma_buffer_size;
		avail -= count;
	}
}

sounddriver_t SNDIO_AudioOutput =
{
	"sndio",
	sndio_init,
	NULL
};

