// JAM format decoder, used by Blood Omnicide

#ifdef LIBAVCODEC
//#define JAM_USELIBAVCODECSCALE
#endif

typedef struct jamdecodestream_s
{
	qfile_t       *file;
	double         info_framerate;
	unsigned int   info_frames;
	unsigned int   info_imagewidth;
	unsigned int   info_imageheight;
	double         info_aspectratio;
	float          colorscale;
	unsigned char  colorsub;

	// info used during decoding
	unsigned char *frame;
	unsigned char *frame_prev;
	unsigned char  frame_palette[768];
	unsigned char *frame_compressed;
	unsigned int   framesize;
	unsigned int   framenum;

	// libavcodec scaling
#ifdef JAM_USELIBAVCODECSCALE
	unsigned char *frame_output_buffer;
	AVFrame       *frame_output;
	AVFrame       *frame_output_scale;
	unsigned int   framewidth;
	unsigned int   frameheight;
#endif

	// channel the sound file is being played on
	int sndchan;
}
jamdecodestream_t;

// opens a stream
void jam_close(void *stream);
unsigned int jam_getwidth(void *stream);
unsigned int jam_getheight(void *stream);
double jam_getframerate(void *stream);
double jam_getaspectratio(void *stream);
int jam_video(void *stream, void *imagedata, unsigned int Rmask, unsigned int Gmask, unsigned int Bmask, unsigned int bytesperpixel, int imagebytesperrow);
void *jam_open(clvideo_t *video, char *filename, const char **errorstring)
{
	char jamHead[16];
	jamdecodestream_t *s;
	char *wavename;

	// allocate stream structure
	s = (jamdecodestream_t *)Z_Malloc(sizeof(jamdecodestream_t));
	memset(s, 0, sizeof(jamdecodestream_t));
	if (s == NULL)
	{
		*errorstring = "unable to allocate memory for stream info structure";
		return NULL;
	}
	s->sndchan = -1;

	// open file
	s->file = FS_OpenVirtualFile(filename, true);
	if (!s->file)
	{
		*errorstring = "unable to open videofile";
		jam_close(s);
		return NULL;
	}

	// read header
	if (!FS_Read(s->file, jamHead, 16))
	{
		*errorstring = "JamDecoder: unexpected EOF reading header";
		jam_close(s);
		return NULL;
	}
	if (memcmp(jamHead, "JAM", 4))
	{
		*errorstring = "JamDecoder: not a JAM file";
		jam_close(s);
		return NULL;
	}

	s->info_imagewidth = LittleLong(*(jamHead + 4));
	s->info_imageheight = LittleLong(*(jamHead + 8));
	s->info_frames = LittleLong(*(jamHead + 12)) - 1;
	s->info_framerate = 15;
	s->info_aspectratio = (double)s->info_imagewidth / (double)s->info_imageheight;
	s->colorscale = 0.90;
	s->colorsub = 4;
	s->framesize = s->info_imagewidth * s->info_imageheight;

	// allocate frame input/output
	if (s->framesize < 0)
	{
		*errorstring = "JamDecoder: bad framesize";
		jam_close(s);
		return NULL;
	}
	s->frame = (unsigned char *)Z_Malloc(s->framesize * 2);
	s->frame_prev = (unsigned char *)Z_Malloc(s->framesize * 2);
	s->frame_compressed = (unsigned char *)Z_Malloc(s->framesize);
	if (s->frame_compressed == NULL || s->frame == NULL || s->frame_prev == NULL)
	{
		*errorstring = "JamDecoder: unable to allocate memory for video decoder";
		jam_close(s);
		return NULL;
	}

	// scale support provided by libavcodec
#ifdef JAM_USELIBAVCODECSCALE
	s->framewidth = s->info_imagewidth;
	s->frameheight = s->info_imageheight;

	// min size
	if (cl_video_libavcodec_minwidth.integer > 0)
		s->info_imagewidth = max(s->info_imagewidth, (unsigned int)cl_video_libavcodec_minwidth.integer);
	if (cl_video_libavcodec_minheight.integer > 0)
		s->info_imageheight = max(s->info_imageheight, (unsigned int)cl_video_libavcodec_minheight.integer);

	// allocate output
	s->frame_output_buffer = (unsigned char *)Z_Malloc(s->framesize * 4);
	s->frame_output = AvCodec_AllocFrame();
	s->frame_output_scale = AvCodec_AllocFrame();
	if (!s->frame_output_buffer || !s->frame_output || !s->frame_output_scale)
    {
		*errorstring = "JamDecoder: failed to allocate LibAvcodec frame";
		jam_close(s);
		Z_Free(s);
        return NULL;
	}
#endif

	// everything is ok
	// set the module functions
	s->framenum = 0;
	video->close = jam_close;
	video->getwidth = jam_getwidth;
	video->getheight = jam_getheight;
	video->getframerate = jam_getframerate;
	video->decodeframe = jam_video;
	video->getaspectratio = jam_getaspectratio;

	// set sound
	size_t namelen;
	namelen = strlen(filename) + 10;
	wavename = (char *)Z_Malloc(namelen);
	if (wavename)
	{
		sfx_t* sfx;
		FS_StripExtension(filename, wavename, namelen);
		dp_strlcat(wavename, ".wav", namelen);
		sfx = S_PrecacheSound(wavename, false, false);
		if (sfx != NULL)
			s->sndchan = S_StartSound (-1, 0, sfx, vec3_origin, 1.0f, 0);
		else
			s->sndchan = -1;
		Z_Free(wavename);
	}

	return s;
}

// closes a stream
void jam_close(void *stream)
{
	jamdecodestream_t *s = (jamdecodestream_t *)stream;
	if (s == NULL)
		return;
	if (s->frame_compressed)
		Z_Free(s->frame_compressed);
	s->frame_compressed = NULL;
	if (s->frame)
		Z_Free(s->frame);
	s->frame = NULL;
	if (s->frame_prev)
		Z_Free(s->frame_prev);
	s->frame_prev = NULL;
	if (s->sndchan != -1)
		S_StopChannel(s->sndchan, true, true);
	s->sndchan = -1;
	if (s->file)
		FS_Close(s->file);
	s->file = NULL;
#ifdef JAM_USELIBAVCODECSCALE
	if (s->frame_output_buffer)
		Z_Free(s->frame_output_buffer);
	s->frame_output_buffer = NULL;
	if (s->frame_output)
		AvUtil_Free(s->frame_output);
	s->frame_output = NULL;
	if (s->frame_output_scale)
		AvUtil_Free(s->frame_output_scale);
	s->frame_output_scale = NULL;
#endif
	Z_Free(s);
}

// returns the width of the image data
unsigned int jam_getwidth(void *stream)
{
	jamdecodestream_t *s = (jamdecodestream_t *)stream;
	return s->info_imagewidth;
}

// returns the height of the image data
unsigned int jam_getheight(void *stream)
{
	jamdecodestream_t *s = (jamdecodestream_t *)stream;
	return s->info_imageheight;
}

// returns the framerate of the stream
double jam_getframerate(void *stream)
{
	jamdecodestream_t *s = (jamdecodestream_t *)stream;
	return s->info_framerate;
}

// returns aspect ration of the stream
double jam_getaspectratio(void *stream)
{
	jamdecodestream_t *s = (jamdecodestream_t *)stream;
	return s->info_aspectratio;
}

// decode JAM frame
static void jam_decodeframe(unsigned char *inbuf, unsigned char *outbuf, unsigned char *prevbuf, int outsize, int frametype)
{
	unsigned char *srcptr, *destptr, *prevptr;
	int bytesleft;
	unsigned int mark;
	unsigned short int bits;
	int rep;
	int backoffs;
	unsigned char *back;
	int i;

	srcptr = inbuf;
	destptr = outbuf;
	prevptr = prevbuf;
	bytesleft = outsize;

	if (frametype == 2)
	{
		memcpy(outbuf, inbuf, outsize);
		return;
	}
	while(bytesleft > 0)
	{
		memcpy(&mark, srcptr, 4);
		srcptr += 4;
		for(i=0; i<32 && bytesleft > 0; i++,mark=mark>>1)
		{
			if(mark & 1)
			{
				*destptr = *srcptr;
				destptr ++;
				prevptr ++;
				srcptr ++;
				bytesleft --;
			}
			else
			{
				bits = srcptr[0] + 256*srcptr[1];
				rep = (bits >> 11) + 3;
				if(frametype == 1)
				{
					backoffs = 0x821 - (bits & 0x7ff);
					back = destptr - backoffs;
				}
				else
				{
					backoffs = 0x400 - (bits & 0x7ff);
					back = prevptr - backoffs;
				}
				srcptr += 2;
				memcpy(destptr, back, rep);
				destptr += rep;
				prevptr += rep;
				bytesleft -= rep;
			}
		}
	}
}

// decodes a video frame to the supplied output pixels
int jam_video(void *stream, void *imagedata, unsigned int Rmask, unsigned int Gmask, unsigned int Bmask, unsigned int bytesperpixel, int imagebytesperrow)
{
	unsigned char frameHead[16], *b;
	unsigned int compsize, outsize, i;
	jamdecodestream_t *s = (jamdecodestream_t *)stream;

	// EOF
	if (s->framenum >= s->info_frames)
		return 1;
	s->framenum++;

readframe:
	// read frame header
	if (!FS_Read(s->file, &frameHead, 16))
	{
		Con_Printf("JamDecoder: unexpected EOF on frame %i\n", s->framenum);
		return 1;
	}
	compsize = LittleLong(*(frameHead + 8)) - 16;
	outsize = LittleLong(*(frameHead + 12));
	if (compsize > s->framesize || outsize > s->framesize)
	{
		Con_Printf("JamDecoder: got bogus header on frame %i\n", s->framenum);
		return 1;
	}

	// read frame contents
	if (!FS_Read(s->file, s->frame_compressed, compsize))
	{
		Con_Printf("JamDecoder: unexpected EOF on frame %i\n", s->framenum);
		return 1;
	}

	// palette goes interleaved with special flag
	if (frameHead[0] == 2)
	{
		if (compsize == 768)
		{
			memcpy(s->frame_palette, s->frame_compressed, 768);
			for(i = 0; i < 768; i++)
				s->frame_palette[i] = (unsigned char)(bound(0, (s->frame_palette[i] * s->colorscale) - s->colorsub, 255));
			goto readframe;
		}
	}
	else
	{
		// decode frame
		// shift buffers to provide current and previous one, decode
		b = s->frame_prev;
		s->frame_prev = s->frame;
		s->frame = b;
		jam_decodeframe(s->frame_compressed, s->frame, s->frame_prev, outsize, frameHead[4]);
#ifdef JAM_USELIBAVCODECSCALE
		// make BGRA imagepixels from 8bit palettized frame
		b = (unsigned char *)s->frame_output_buffer;
		for(i = 0; i < s->framesize; i++)
		{
			*b++ = s->frame_palette[s->frame[i]*3 + 2];
			*b++ = s->frame_palette[s->frame[i]*3 + 1];
			*b++ = s->frame_palette[s->frame[i]*3];
			*b++ = 255;
		}
		// scale
		AvCodec_FillPicture((AVPicture *)s->frame_output, (uint8_t *)s->frame_output_buffer, PIX_FMT_BGRA, s->framewidth, s->frameheight);
		AvCodec_FillPicture((AVPicture *)s->frame_output_scale, (uint8_t *)imagedata, PIX_FMT_BGRA, s->info_imagewidth, s->info_imageheight);
		SwsContext *scale_context = SwScale_GetCachedContext(NULL, s->framewidth, s->frameheight, PIX_FMT_BGRA, s->info_imagewidth, s->info_imageheight, PIX_FMT_BGRA, libavcodec_scalers[max(0, min(LIBAVCODEC_SCALERS, cl_video_libavcodec_scaler.integer))], NULL, NULL, NULL); 
		if (!scale_context)
		{
			Con_Printf("JamDecoder: LibAvcodec: error creating scale context frame %i\n", s->framenum);
			return 1;
		}
		if (!SwScale_Scale(scale_context, s->frame_output->data, s->frame_output->linesize, 0, s->frameheight, s->frame_output_scale->data, s->frame_output_scale->linesize))
			Con_Printf("JamDecoder: LibAvcodec : error scaling frame\n", s->framenum);
		SwScale_FreeContext(scale_context); 
#else
		// make BGRA imagepixels from 8bit palettized frame
		b = (unsigned char *)imagedata;
		for(i = 0; i < s->framesize; i++)
		{
			// bgra
			*b++ = s->frame_palette[s->frame[i]*3 + 2];
			*b++ = s->frame_palette[s->frame[i]*3 + 1];
			*b++ = s->frame_palette[s->frame[i]*3];
			*b++ = 255;
		}
#endif
	}
	return 0;
}
