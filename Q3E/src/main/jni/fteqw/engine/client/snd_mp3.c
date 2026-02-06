//source file by david walton, derived from minimad.c
//modified slightly to use new library functions instead for single threaded asyncronous mp3 sound.
//in Quake!

//Be aware that MP3s are patented and thus not GPLable. This file is an interface to madlib which contains the decoder.
//This file is excluded from the main source tree because of this, and madlib is not distributed - in binary or source form.
//The GPL disallows distribution of patented software. You may reactive, but do not distribute binaries.

//Really madlib should come with the exception of the mp3 patent.

#include "quakedef.h"

#ifdef AVAIL_MP3

#include "winquake.h"
#undef channels


#ifdef _WIN32
#define inline _inline
#endif

//the problem with this is that we store the entire decoded file in memory.
//this takes a lot of mem.

//uses madlib



/*
 * mad - MPEG audio decoder
 * Copyright (C) 2000-2001 Robert Leslie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

# include "libmad/mad.h"
#define USE_MADLIB
#include "mymad.c"

/*
 * This is perhaps the simplest example use of the MAD high-level API.
 * Standard input is mapped into memory via mmap(), then the high-level API
 * is invoked with three callbacks: input, output, and error. The output
 * callback converts MAD's high-resolution PCM samples to 16 bits, then
 * writes them to standard output in little-endian, stereo-interleaved
 * format.
 */



/*
 * This is a private message structure. A generic pointer to this structure
 * is passed to each of the callback functions. Put here any data you need
 * to access from within the callbacks.
 */

typedef struct {
	unsigned char *start;
	unsigned long length;
	qboolean failed;

	sfxcache_t	mp3sc;
	char *mp3aswavdata;	
	int mp3aswavpos;
	int mp3aswavbuflen;

	struct mad_decoder decoder;

	sfx_t *s;
} decoderbuffer_t;

int mymad_run(struct mad_decoder *decoder);
int mymad_reset(struct mad_decoder *decoder);
void mymad_finish(struct mad_decoder *decoder);
static int startdecode(unsigned char *start, unsigned long length, decoderbuffer_t *buffer);

#define BUFFERSIZEINC (2*1024*1024)

int DecodeSomeMP3(sfx_t *s, int minlength);
qbyte *COM_LoadFile (char *path, int usehunk);
int MP3_decode(unsigned char *start, unsigned long length, decoderbuffer_t *buffer);
void CancelDecoder(sfx_t *s);
sfxcache_t *S_LoadMP3Sound (sfx_t *s)
{
	char	namebuffer[MAX_OSPATH];
	char	*name;
	decoderbuffer_t *buffer;

	char *data;
	qboolean telluser;
	int len;

	name = s->name;

	if (name[0] == '#')
		strcpy(namebuffer, &name[1]);
	else
		Q_snprintfz (namebuffer, sizeof(namebuffer), "sound/%s", name);

	len = strlen(namebuffer);
	telluser = strcmp(namebuffer+len-4, ".wav");
	if (!telluser)
		strcpy(namebuffer+len-4, ".mp3");	

	//try opening from a quake path
	data = COM_LoadFile(namebuffer, 5);
	if (!data)
	{//if that didn't work, try opening direct from exe - this is media after all.
		FILE *f;

		if (!telluser)
			return NULL;	//never go out of the quake path for a wav replacement.
#ifndef _WIN32
		char unixname[128];
		if (name[1] == ':' && name[2] == '\\')	//convert from windows to a suitable alternative.
		{			
			snprintf(unixname, sizeof(unixname), "/mnt/%c/%s", name[0]-'A'+'a', name+3);
			name = unixname;
			while (*name)
			{
				if (*name == '\\')
					*name = '/';
				name++;
			}			
			name = unixname;			
		}
#endif
		if ((f = fopen(name, "rb")))
		{
			com_filesize = COM_filelength(f);
			data = BZ_Malloc(com_filesize);
			fread(data, 1, com_filesize, f);
			fclose(f);
		}
		else
		{
			Con_SafePrintf ("Couldn't load %s\n", namebuffer);
			return NULL;
		}
	}

	if (s->decoder)
		Sys_Error("Decoding already decoding file\n");
	s->decoder = Z_Malloc(sizeof(decoderbuffer_t) + sizeof(sfxdecode_t));
	buffer = (decoderbuffer_t*)(s->decoder+1);

	buffer->mp3aswavpos=0;
	buffer->mp3sc.length=0;
	buffer->s = s;
	s->decoder->buf = buffer;
	s->decoder->decodemore = DecodeSomeMP3;
	s->decoder->abort = CancelDecoder;

	if (!startdecode(data, com_filesize, buffer))
	{
		if (buffer->mp3aswavdata)
		{
			BZ_Free(buffer->mp3aswavdata);

			buffer->mp3aswavdata=NULL;
		}
		Con_SafePrintf ("Couldn't load %s - corrupt.\n", namebuffer);
		return NULL;
	}

	s->decoder->decodemore(s, 100);
	if (!s->decoder)	//wow, short file. :/
		return s->cache.data;

	s->cache.fake=true;
	return buffer->s->cache.data;
}


/*
 * This is the input callback. The purpose of this callback is to (re)fill
 * the stream buffer which is to be decoded. In this example, an entire file
 * has been mapped into memory, so we just call mad_stream_buffer() with the
 * address and length of the mapping. When this callback is called a second
 * time, we are finished decoding.
 */

static
enum mad_flow input(void *data,
		    struct mad_stream *stream)
{
  decoderbuffer_t *buffer = data;

  if (!buffer->length)
    return MAD_FLOW_STOP;

  mad_stream_buffer(stream, buffer->start, buffer->length);

  buffer->length = 0;

  return MAD_FLOW_CONTINUE;
}

/*
 * The following utility routine performs simple rounding, clipping, and
 * scaling of MAD's high-resolution samples down to 16 bits. It does not
 * perform any dithering or noise shaping, which would be recommended to
 * obtain any exceptional audio quality. It is therefore not recommended to
 * use this routine if high-quality output is desired.
 */

static inline
signed int scale(mad_fixed_t sample)
{
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

/*
 * This is the output callback function. It is called after each frame of
 * MPEG audio data has been completely decoded. The purpose of this callback
 * is to output (or play) the decoded PCM audio.
 */

static
enum mad_flow output(void *data,
			struct mad_header const *header,
			struct mad_pcm *pcm)
{
	decoderbuffer_t *buffer = data;
	sfxcache_t *cache;

	unsigned int nchannels, nsamples;
	mad_fixed_t const *left_ch, *right_ch;

	char *outpos;
	signed int sample;
	float speedfactor;

	speedfactor	= (float)snd_speed/buffer->mp3sc.speed;

  /* pcm->samplerate contains the sampling frequency */

	nchannels = pcm->channels;
	nsamples  = pcm->length*speedfactor;
	left_ch   = pcm->samples[0];
	right_ch  = pcm->samples[1];

	if (buffer->mp3aswavpos + nsamples*nchannels*sizeof(short) > buffer->mp3aswavbuflen || !buffer->mp3aswavdata)
	{
		int	  newsize = buffer->mp3aswavpos + nsamples*nchannels*sizeof(short) + BUFFERSIZEINC+sizeof(sfxcache_t);
		char *newbuf = BZ_Malloc(newsize);
		if (!newbuf)
			return MAD_FLOW_STOP;	//damn and blast!

		if (buffer->mp3aswavdata)	//if we had an old buffer, fill the new one and free the old
		{
//			memset(newbuf, 0, newsize);
			memcpy(newbuf, buffer->mp3aswavdata, buffer->mp3aswavpos+sizeof(sfxcache_t));
//			newbuf[newsize] = '\0';
			BZ_Free(buffer->mp3aswavdata);
		}
		buffer->mp3aswavdata = newbuf;
		buffer->mp3aswavbuflen = newsize;

		buffer->s->cache.data = buffer->mp3aswavdata;
	}

	cache = (sfxcache_t *)buffer->mp3aswavdata;
	outpos = buffer->mp3aswavdata+sizeof(sfxcache_t) + buffer->mp3aswavpos;

	if (nchannels == 1)
	{
		if (speedfactor==1)	//fast
		{
			while (nsamples--)
			{
				sample = scale(*left_ch++);
				*outpos++ = ((sample >> 0) & 0xff);
				*outpos++ = ((sample >> 8) & 0xff);			
			}
		}
		else
		{
			int src=0;
			int pos=0;			

			while (pos < nsamples)
			{
				src = pos/speedfactor;
				sample = scale(left_ch[src]);
				*outpos++ = (((sample >> 0) & 0xff));
				*outpos++ = (((sample >> 8) & 0xff));

				pos++;
			}
		}
		cache->stereo = 0;
	}
	else
	{
		if (speedfactor==1)	//fast
		{
			while (nsamples--)
			{	//merge channels?
				sample = scale(*left_ch++);
				*outpos++ = (((sample >> 0) & 0xff));
				*outpos++ = (((sample >> 8) & 0xff));

				sample = scale(*right_ch++)/2;
				*outpos++ = (((sample >> 0) & 0xff));
				*outpos++ = (((sample >> 8) & 0xff));
			}
		}
		else
		{
			int src=0;
			int pos=0;			

			while (pos < nsamples)
			{
				src = pos/speedfactor;
				sample = scale(left_ch[src]);
				*outpos++ = (((sample >> 0) & 0xff));
				*outpos++ = (((sample >> 8) & 0xff));

				sample = scale(right_ch[src]);
				*outpos++ = (((sample >> 0) & 0xff));
				*outpos++ = (((sample >> 8) & 0xff));

				pos++;
			}
		}
		cache->stereo = 1;
	}

	buffer->mp3aswavpos += pcm->length*speedfactor*sizeof(short)*nchannels;
	buffer->mp3sc.length+= pcm->length*speedfactor;

	cache->speed = snd_speed;
	cache->length = buffer->s->decoder->decodedlen = buffer->mp3sc.length;
	cache->loopstart = -1;//buffer->mp3sc.loopstart;
	cache->width = buffer->mp3sc.width;	

	return MAD_FLOW_CONTINUE;
}

/*
 * This is the error callback function. It is called whenever a decoding
 * error occurs. The error is indicated by stream->error; the list of
 * possible MAD_ERROR_* errors can be found in the mad.h (or
 * libmad/stream.h) header file.
 */

static
enum mad_flow error(void *data,
		    struct mad_stream *stream,
		    struct mad_frame *frame)
{
  decoderbuffer_t *buffer = data;

  if (stream->error == 257)
	  return MAD_FLOW_IGNORE;

  if (developer.value)
	Con_SafePrintf("MP3Error: decoding error 0x%04x (%s) at byte offset %u\n",
	  stream->error, mad_stream_errorstr(stream),
	  stream->this_frame - buffer->start);

  buffer->failed = true;

  return MAD_FLOW_IGNORE;
}

static enum mad_flow header(void *data,
						   struct mad_header const *header)
{
	decoderbuffer_t *buffer = data;

	buffer->mp3sc.speed = header->samplerate;
	buffer->mp3sc.width = 2;
	return MAD_FLOW_CONTINUE;
}

void CancelDecoder(sfx_t *s)
{
	decoderbuffer_t *dec = s->decoder->buf;

	mymad_finish(&dec->decoder);
	mad_decoder_finish(&dec->decoder);

	s->cache.fake = false;	//give it a true cache now, and hope that we don't need to free it while it's still playing.
	s->cache.data=NULL;

	BZ_Free(dec->mp3aswavdata);
	if (dec->start)
		BZ_Free(dec->start);

	Z_Free(s->decoder);
	s->decoder = NULL;	
}

/*
 * This is the function called by main() above to perform all the
 * decoding. It instantiates a decoder object and configures it with the
 * input, output, and error callback functions above. A single call to
 * mad_decoder_run() continues until a callback function returns
 * MAD_FLOW_STOP (to stop decoding) or MAD_FLOW_BREAK (to stop decoding and
 * signal an error).
 */

int DecodeSomeMP3(sfx_t *s, int minlength)
{	
	decoderbuffer_t *dec = s->decoder->buf;

//	int oldlen;

	if (!dec->start)
		return 1;

	while(dec->mp3sc.length <= minlength)
	{
		if (!mymad_run(&dec->decoder) || dec->failed)
		{
			if (dec->mp3aswavbuflen>441000)//10 secs at highest quality
			{	//big fat heffers of mp3s are kept like this and are flushed when they finish playing fully.
				BZ_Free(dec->start);
				dec->start = NULL;
			}
			else
			{	//small mp3 files are now treated like wavs
				void *newmem;
				mymad_finish(&dec->decoder);
				mad_decoder_finish(&dec->decoder);

				s->cache.fake = false;	//give it a true cache now, and hope that we don't need to free it while it's still playing.
				s->cache.data=NULL;
				if (dec->mp3aswavdata)
				{
					newmem = Cache_Alloc(&s->cache, dec->mp3aswavbuflen+sizeof(sfxcache_t), s->name);
					memcpy(newmem, dec->mp3aswavdata, dec->mp3aswavbuflen+sizeof(sfxcache_t));
					BZ_Free(dec->mp3aswavdata);
				}
				BZ_Free(dec->start);

				Z_Free(s->decoder);
				s->decoder = NULL;
			}
			return 1;
		}
	}
	return 0;
}

int mymad_decoder_run(struct mad_decoder *decoder, enum mad_decoder_mode mode);
static
int startdecode(unsigned char *start, unsigned long length, decoderbuffer_t *buffer)
{
  /* initialize our private message structure */

  buffer->start  = start;
  buffer->length = length;
  buffer->failed = false;

  /* configure input, output, and error functions */

  mad_decoder_init(&buffer->decoder, buffer,
		   input, header, 0 /* filter */, output,
		   error, 0 /* message */);

  /* start decoding */

  mymad_reset(&buffer->decoder);

  if (buffer->failed)
	  return false;

  return true;
}
#endif



