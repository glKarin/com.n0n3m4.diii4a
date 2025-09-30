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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

#include <sndio.h>

#include "../qcommon/q_shared.h"
#include "../client/snd_local.h"

static struct sio_hdl *hdl;
static int snd_inited;

unsigned char *dma_buffer;
size_t dma_buffer_size, dma_ptr;


qboolean
SNDDMA_Init(void)
{
	struct sio_par par;
	int i;

	if (snd_inited)
		return qtrue;

	hdl = sio_open(NULL, SIO_PLAY, 1);
	if (hdl == NULL) {
		Com_Printf("Could not open sndio device\n");
		return qfalse;
	}

	sio_initpar(&par);

	par.bits = 16;
	par.sig = 1;
	par.pchan = 2;
	par.rate = 44100;
	par.le = SIO_LE_NATIVE;
	par.appbufsz = par.rate / 10;	/* 1/10 second latency */

	if (!sio_setpar(hdl, &par) || !sio_getpar(hdl, &par)) {
		Com_Printf("Error setting audio parameters\n");
		sio_close(hdl);
		return qfalse;
	}
	if ((par.pchan != 1 && par.pchan != 2) ||
	    !((par.bits == 16 && par.sig == 1) ||
	      (par.bits == 8 && par.sig == 0))) {
		Com_Printf("Could not set appropriate audio parameters\n");
		sio_close(hdl);
		return qfalse;
	}
	dma.speed = par.rate;
	dma.channels = par.pchan;
	dma.samplebits = par.bits;
	dma.submission_chunk = 1;

	/*
	 * find the smallest power of two larger than the buffer size
	 * and use it as the internal buffer's size
	 */
	for (i = 1; i < par.appbufsz; i <<= 2)
		; /* nothing */
	dma.samples = i * par.pchan;

	dma_buffer_size = dma.samples * dma.samplebits / 8;
	dma_buffer = calloc(1, dma_buffer_size);
	if (dma_buffer == NULL) {
		Com_Printf("Could not allocate audio ring buffer\n");
		return qfalse;
	}
	dma_ptr = 0;
	dma.buffer = dma_buffer;
	if (!sio_start(hdl)) {
		Com_Printf("Could not start audio\n");
		sio_close(hdl);
		return qfalse;
	}

	snd_inited = qtrue;
	return qtrue;
}

void
SNDDMA_Shutdown(void)
{
	if (snd_inited == qtrue) {
		sio_close(hdl);
		snd_inited = qfalse;
	}
	free(dma_buffer);
}

int
SNDDMA_GetDMAPos(void)
{
	if (!snd_inited)
		return (0);

	return (dma_ptr / (dma.samplebits / 8));
}

void
SNDDMA_Submit(void)
{
	struct pollfd pfd;
	size_t count, todo, avail;
	int n;

	n = sio_pollfd(hdl, &pfd, POLLOUT);
	while (poll(&pfd, n, 0) < 0 && errno == EINTR)
		;
	if (!(sio_revents(hdl, &pfd) & POLLOUT))
		return;
	avail = dma_buffer_size;
	while (avail > 0) {
		todo = dma_buffer_size - dma_ptr;
		if (todo > avail)
			todo = avail;
		count = sio_write(hdl, dma_buffer + dma_ptr, todo);
		if (count == 0)
			break;
		dma_ptr += count;
		if (dma_ptr >= dma_buffer_size)
			dma_ptr -= dma_buffer_size;
		avail -= count;
	}
}

void
SNDDMA_BeginPainting(void)
{
}
