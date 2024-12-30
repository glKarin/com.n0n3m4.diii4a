#include "quakedef.h"
#include "cap_avi.h"

#define AVI_MASTER_INDEX_SIZE 640 // GB ought to be enough for anyone

typedef struct capturevideostate_avi_formatspecific_s
{
	// AVI stuff
	fs_offset_t videofile_firstchunkframes_offset;
	fs_offset_t videofile_totalframes_offset1;
	fs_offset_t videofile_totalframes_offset2;
	fs_offset_t videofile_totalsampleframes_offset;
	int videofile_ix_master_audio_inuse;
	fs_offset_t videofile_ix_master_audio_inuse_offset;
	fs_offset_t videofile_ix_master_audio_start_offset;
	int videofile_ix_master_video_inuse;
	fs_offset_t videofile_ix_master_video_inuse_offset;
	fs_offset_t videofile_ix_master_video_start_offset;
	fs_offset_t videofile_ix_movistart;
	fs_offset_t position;
	qbool canseek;
	sizebuf_t riffbuffer;
	unsigned char riffbufferdata[128];
	sizebuf_t riffindexbuffer;
	int riffstacklevel;
	fs_offset_t riffstackstartoffset[4];
	fs_offset_t riffstacksizehint[4];
	const char *riffstackfourcc[4];
}
capturevideostate_avi_formatspecific_t;
#define LOAD_FORMATSPECIFIC_AVI() capturevideostate_avi_formatspecific_t *format = (capturevideostate_avi_formatspecific_t *) cls.capturevideo.formatspecific

static void SCR_CaptureVideo_RIFF_Start(void)
{
	LOAD_FORMATSPECIFIC_AVI();
	memset(&format->riffbuffer, 0, sizeof(sizebuf_t));
	format->riffbuffer.maxsize = sizeof(format->riffbufferdata);
	format->riffbuffer.data = format->riffbufferdata;
	format->position = 0;
}

static void SCR_CaptureVideo_RIFF_Flush(void)
{
	LOAD_FORMATSPECIFIC_AVI();
	if (format->riffbuffer.cursize > 0)
	{
		if (!FS_Write(cls.capturevideo.videofile, format->riffbuffer.data, format->riffbuffer.cursize))
			cls.capturevideo.error = true;
		format->position += format->riffbuffer.cursize;
		format->riffbuffer.cursize = 0;
		format->riffbuffer.overflowed = false;
	}
}

static void SCR_CaptureVideo_RIFF_FlushNoIncrease(void)
{
	LOAD_FORMATSPECIFIC_AVI();
	if (format->riffbuffer.cursize > 0)
	{
		if (!FS_Write(cls.capturevideo.videofile, format->riffbuffer.data, format->riffbuffer.cursize))
			cls.capturevideo.error = true;
		format->riffbuffer.cursize = 0;
		format->riffbuffer.overflowed = false;
	}
}

static void SCR_CaptureVideo_RIFF_WriteBytes(const unsigned char *data, size_t size)
{
	LOAD_FORMATSPECIFIC_AVI();
	SCR_CaptureVideo_RIFF_Flush();
	if (!FS_Write(cls.capturevideo.videofile, data, size))
		cls.capturevideo.error = true;
	format->position += size;
}

static void SCR_CaptureVideo_RIFF_Write32(int n)
{
	LOAD_FORMATSPECIFIC_AVI();
	if (format->riffbuffer.cursize + 4 > format->riffbuffer.maxsize)
		SCR_CaptureVideo_RIFF_Flush();
	MSG_WriteLong(&format->riffbuffer, n);
}

static void SCR_CaptureVideo_RIFF_Write16(int n)
{
	LOAD_FORMATSPECIFIC_AVI();
	if (format->riffbuffer.cursize + 2 > format->riffbuffer.maxsize)
		SCR_CaptureVideo_RIFF_Flush();
	MSG_WriteShort(&format->riffbuffer, n);
}

static void SCR_CaptureVideo_RIFF_WriteFourCC(const char *chunkfourcc)
{
	LOAD_FORMATSPECIFIC_AVI();
	if (format->riffbuffer.cursize + (int)strlen(chunkfourcc) > format->riffbuffer.maxsize)
		SCR_CaptureVideo_RIFF_Flush();
	MSG_WriteUnterminatedString(&format->riffbuffer, chunkfourcc);
}

static void SCR_CaptureVideo_RIFF_WriteTerminatedString(const char *string)
{
	LOAD_FORMATSPECIFIC_AVI();
	if (format->riffbuffer.cursize + (int)strlen(string) > format->riffbuffer.maxsize)
		SCR_CaptureVideo_RIFF_Flush();
	MSG_WriteString(&format->riffbuffer, string);
}

static fs_offset_t SCR_CaptureVideo_RIFF_GetPosition(void)
{
	LOAD_FORMATSPECIFIC_AVI();
	SCR_CaptureVideo_RIFF_Flush();
	//return FS_Tell(cls.capturevideo.videofile);
	return format->position;
}

static void SCR_CaptureVideo_RIFF_Push(const char *chunkfourcc, const char *listtypefourcc, fs_offset_t sizeHint)
{
	LOAD_FORMATSPECIFIC_AVI();
	if (listtypefourcc && sizeHint >= 0)
		sizeHint += 4; // size hint is for INNER size
	SCR_CaptureVideo_RIFF_WriteFourCC(chunkfourcc);
	SCR_CaptureVideo_RIFF_Write32(sizeHint);
	SCR_CaptureVideo_RIFF_Flush();
	format->riffstacksizehint[format->riffstacklevel] = sizeHint;
	format->riffstackstartoffset[format->riffstacklevel] = SCR_CaptureVideo_RIFF_GetPosition();
	format->riffstackfourcc[format->riffstacklevel] = chunkfourcc;
	++format->riffstacklevel;
	if (listtypefourcc)
		SCR_CaptureVideo_RIFF_WriteFourCC(listtypefourcc);
}

static void SCR_CaptureVideo_RIFF_Pop(void)
{
	LOAD_FORMATSPECIFIC_AVI();
	fs_offset_t offset, sizehint;
	int x;
	unsigned char sizebytes[4];
	// write out the chunk size and then return to the current file position
	format->riffstacklevel--;
	offset = SCR_CaptureVideo_RIFF_GetPosition();

	sizehint = format->riffstacksizehint[format->riffstacklevel];
	x = (int)(offset - (format->riffstackstartoffset[format->riffstacklevel]));

	if(x != sizehint)
	{
		if(sizehint != -1)
		{
			int i;
			Con_Printf(CON_WARN "WARNING: invalid size hint %d when writing video data (actual size: %d)\n", (int) sizehint, x);
			for(i = 0; i <= format->riffstacklevel; ++i)
			{
				Con_Printf("  RIFF level %d = %s\n", i, format->riffstackfourcc[i]);
			}
		}
		sizebytes[0] = (x) & 0xff;sizebytes[1] = (x >> 8) & 0xff;sizebytes[2] = (x >> 16) & 0xff;sizebytes[3] = (x >> 24) & 0xff;
		if(FS_Seek(cls.capturevideo.videofile, -(x + 4), SEEK_END) >= 0)
		{
			FS_Write(cls.capturevideo.videofile, sizebytes, 4);
		}
		FS_Seek(cls.capturevideo.videofile, 0, SEEK_END);
	}

	if (offset & 1)
	{
		SCR_CaptureVideo_RIFF_WriteBytes((unsigned char *) "\0", 1);
	}
}

static void GrowBuf(sizebuf_t *buf, int extralen)
{
	if(buf->cursize + extralen > buf->maxsize)
	{
		int oldsize = buf->maxsize;
		unsigned char *olddata;
		olddata = buf->data;
		buf->maxsize = max(buf->maxsize * 2, 4096);
		buf->data = (unsigned char *) Mem_Alloc(tempmempool, buf->maxsize);
		if(olddata)
		{
			memcpy(buf->data, olddata, oldsize);
			Mem_Free(olddata);
		}
	}
}

static void SCR_CaptureVideo_RIFF_IndexEntry(const char *chunkfourcc, int chunksize, int flags)
{
	LOAD_FORMATSPECIFIC_AVI();
	if(!format->canseek)
		Sys_Error("SCR_CaptureVideo_RIFF_IndexEntry called on non-seekable AVI");

	if (format->riffstacklevel != 2)
		Sys_Error("SCR_Capturevideo_RIFF_IndexEntry: RIFF stack level is %i (should be 2)\n", format->riffstacklevel);
	GrowBuf(&format->riffindexbuffer, 16);
	SCR_CaptureVideo_RIFF_Flush();
	MSG_WriteUnterminatedString(&format->riffindexbuffer, chunkfourcc);
	MSG_WriteLong(&format->riffindexbuffer, flags);
	MSG_WriteLong(&format->riffindexbuffer, (int)FS_Tell(cls.capturevideo.videofile) - format->riffstackstartoffset[1]);
	MSG_WriteLong(&format->riffindexbuffer, chunksize);
}

static void SCR_CaptureVideo_RIFF_MakeIxChunk(const char *fcc, const char *dwChunkId, fs_offset_t masteridx_counter, int *masteridx_count, fs_offset_t masteridx_start)
{
	LOAD_FORMATSPECIFIC_AVI();
	int nMatching;
	int i;
	fs_offset_t ix = SCR_CaptureVideo_RIFF_GetPosition();
	fs_offset_t pos, sz;
	
	if(!format->canseek)
		Sys_Error("SCR_CaptureVideo_RIFF_MakeIxChunk called on non-seekable AVI");

	if(*masteridx_count >= AVI_MASTER_INDEX_SIZE)
		return;

	nMatching = 0; // go through index and enumerate them
	for(i = 0; i < format->riffindexbuffer.cursize; i += 16)
		if(!memcmp(format->riffindexbuffer.data + i, dwChunkId, 4))
			++nMatching;

	sz = 2+2+4+4+4+4+4;
	for(i = 0; i < format->riffindexbuffer.cursize; i += 16)
		if(!memcmp(format->riffindexbuffer.data + i, dwChunkId, 4))
			sz += 8;

	SCR_CaptureVideo_RIFF_Push(fcc, NULL, sz);
	SCR_CaptureVideo_RIFF_Write16(2); // wLongsPerEntry
	SCR_CaptureVideo_RIFF_Write16(0x0100); // bIndexType=1, bIndexSubType=0
	SCR_CaptureVideo_RIFF_Write32(nMatching); // nEntriesInUse
	SCR_CaptureVideo_RIFF_WriteFourCC(dwChunkId); // dwChunkId
	SCR_CaptureVideo_RIFF_Write32(format->videofile_ix_movistart & (fs_offset_t) 0xFFFFFFFFu);
	SCR_CaptureVideo_RIFF_Write32(((fs_offset_t) format->videofile_ix_movistart) >> 32);
	SCR_CaptureVideo_RIFF_Write32(0); // dwReserved

	for(i = 0; i < format->riffindexbuffer.cursize; i += 16)
		if(!memcmp(format->riffindexbuffer.data + i, dwChunkId, 4))
		{
			unsigned int *p = (unsigned int *) (format->riffindexbuffer.data + i);
			unsigned int flags = p[1];
			unsigned int rpos = p[2];
			unsigned int size = p[3];
			size &= ~0x80000000;
			if(!(flags & 0x10)) // no keyframe?
				size |= 0x80000000;
			SCR_CaptureVideo_RIFF_Write32(rpos + 8);
			SCR_CaptureVideo_RIFF_Write32(size);
		}

	SCR_CaptureVideo_RIFF_Flush();
	SCR_CaptureVideo_RIFF_Pop();
	pos = SCR_CaptureVideo_RIFF_GetPosition();

	if(FS_Seek(cls.capturevideo.videofile, masteridx_start + 16 * *masteridx_count, SEEK_SET) >= 0)
	{
		SCR_CaptureVideo_RIFF_Write32(ix & (fs_offset_t) 0xFFFFFFFFu);
		SCR_CaptureVideo_RIFF_Write32(((fs_offset_t) ix) >> 32);
		SCR_CaptureVideo_RIFF_Write32(pos - ix);
		SCR_CaptureVideo_RIFF_Write32(nMatching);
		SCR_CaptureVideo_RIFF_FlushNoIncrease();
	}

	if(FS_Seek(cls.capturevideo.videofile, masteridx_counter, SEEK_SET) >= 0)
	{
		SCR_CaptureVideo_RIFF_Write32(++*masteridx_count);
		SCR_CaptureVideo_RIFF_FlushNoIncrease();
	}

	FS_Seek(cls.capturevideo.videofile, 0, SEEK_END); // return value doesn't matter here
}

static void SCR_CaptureVideo_RIFF_Finish(qbool final)
{
	LOAD_FORMATSPECIFIC_AVI();
	// close the "movi" list
	SCR_CaptureVideo_RIFF_Pop();
	if(format->videofile_ix_master_video_inuse_offset)
		SCR_CaptureVideo_RIFF_MakeIxChunk("ix00", "00dc", format->videofile_ix_master_video_inuse_offset, &format->videofile_ix_master_video_inuse, format->videofile_ix_master_video_start_offset);
	if(format->videofile_ix_master_audio_inuse_offset)
		SCR_CaptureVideo_RIFF_MakeIxChunk("ix01", "01wb", format->videofile_ix_master_audio_inuse_offset, &format->videofile_ix_master_audio_inuse, format->videofile_ix_master_audio_start_offset);
	// write the idx1 chunk that we've been building while saving the frames (for old style players)
	if(final && format->videofile_firstchunkframes_offset)
	// TODO replace index creating by OpenDML ix##/##ix/indx chunk so it works for more than one AVI part too
	{
		SCR_CaptureVideo_RIFF_Push("idx1", NULL, format->riffindexbuffer.cursize);
		SCR_CaptureVideo_RIFF_WriteBytes(format->riffindexbuffer.data, format->riffindexbuffer.cursize);
		SCR_CaptureVideo_RIFF_Pop();
	}
	format->riffindexbuffer.cursize = 0;
	// pop the RIFF chunk itself
	while (format->riffstacklevel > 0)
		SCR_CaptureVideo_RIFF_Pop();
	SCR_CaptureVideo_RIFF_Flush();
	if(format->videofile_firstchunkframes_offset)
	{
		Con_DPrintf("Finishing first chunk (%d frames)\n", cls.capturevideo.frame);
		if(FS_Seek(cls.capturevideo.videofile, format->videofile_firstchunkframes_offset, SEEK_SET) >= 0)
		{
			SCR_CaptureVideo_RIFF_Write32(cls.capturevideo.frame);
			SCR_CaptureVideo_RIFF_FlushNoIncrease();
		}
		FS_Seek(cls.capturevideo.videofile, 0, SEEK_END);
		format->videofile_firstchunkframes_offset = 0;
	}
	else
		Con_DPrintf("Finishing another chunk (%d frames)\n", cls.capturevideo.frame);
}

static void SCR_CaptureVideo_RIFF_OverflowCheck(int framesize)
{
	LOAD_FORMATSPECIFIC_AVI();
	fs_offset_t cursize;
	//fs_offset_t curfilesize;
	if (format->riffstacklevel != 2)
		Sys_Error("SCR_CaptureVideo_RIFF_OverflowCheck: chunk stack leakage!\n");
	
	if(!format->canseek)
		return;

	// check where we are in the file
	SCR_CaptureVideo_RIFF_Flush();
	cursize = SCR_CaptureVideo_RIFF_GetPosition() - format->riffstackstartoffset[0];
	//curfilesize = SCR_CaptureVideo_RIFF_GetPosition();

	// if this would overflow the windows limit of 1GB per RIFF chunk, we need
	// to close the current RIFF chunk and open another for future frames
	if (8 + cursize + framesize + format->riffindexbuffer.cursize + 8 + format->riffindexbuffer.cursize + 64 > 1<<30) // note that the Ix buffer takes less space... I just don't dare to / 2 here now... sorry, maybe later
	{
		SCR_CaptureVideo_RIFF_Finish(false);
		// begin a new 1GB extended section of the AVI
		SCR_CaptureVideo_RIFF_Push("RIFF", "AVIX", -1);
		SCR_CaptureVideo_RIFF_Push("LIST", "movi", -1);
		format->videofile_ix_movistart = format->riffstackstartoffset[1];
	}
}

// converts from BGRA32 to I420 colorspace (identical to YV12 except chroma plane order is reversed), this colorspace is handled by the Intel(r) 4:2:0 codec on Windows
static void SCR_CaptureVideo_ConvertFrame_BGRA_to_I420_flip(int width, int height, unsigned char *instart, unsigned char *outstart)
{
	int x, y;
	int blockr, blockg, blockb;
	int outoffset = (width/2)*(height/2);
	unsigned char *b, *out;
	// process one line at a time, and CbCr every other line at 2 pixel intervals
	for (y = 0;y < height;y++)
	{
		// 1x1 Y
		for (b = instart + (height-1-y)*width*4, out = outstart + y*width, x = 0;x < width;x++, b += 4, out++)
		{
			blockr = b[2];
			blockg = b[1];
			blockb = b[0];
			*out = cls.capturevideo.yuvnormalizetable[0][cls.capturevideo.rgbtoyuvscaletable[0][0][blockr] + cls.capturevideo.rgbtoyuvscaletable[0][1][blockg] + cls.capturevideo.rgbtoyuvscaletable[0][2][blockb]];
		}
		if ((y & 1) == 0 && y/2 < height/2) // if h is odd, this skips the last row
		{
			// 2x2 Cr and Cb planes
			int inpitch = width*4;
			for (b = instart + (height-2-y)*width*4, out = outstart + width*height + (y/2)*(width/2), x = 0;x < width/2;x++, b += 8, out++)
			{
				blockr = (b[2] + b[6] + b[inpitch+2] + b[inpitch+6]) >> 2;
				blockg = (b[1] + b[5] + b[inpitch+1] + b[inpitch+5]) >> 2;
				blockb = (b[0] + b[4] + b[inpitch+0] + b[inpitch+4]) >> 2;
				// Cr
				out[0        ] = cls.capturevideo.yuvnormalizetable[1][cls.capturevideo.rgbtoyuvscaletable[1][0][blockr] + cls.capturevideo.rgbtoyuvscaletable[1][1][blockg] + cls.capturevideo.rgbtoyuvscaletable[1][2][blockb] + 128];
				// Cb
				out[outoffset] = cls.capturevideo.yuvnormalizetable[2][cls.capturevideo.rgbtoyuvscaletable[2][0][blockr] + cls.capturevideo.rgbtoyuvscaletable[2][1][blockg] + cls.capturevideo.rgbtoyuvscaletable[2][2][blockb] + 128];
			}
		}
	}
}

static void SCR_CaptureVideo_Avi_VideoFrames(int num, u8 *in)
{
	LOAD_FORMATSPECIFIC_AVI();
	int x = 0, width = cls.capturevideo.width, height = cls.capturevideo.height;
	unsigned char *out = cls.capturevideo.outbuffer;

	SCR_CaptureVideo_ConvertFrame_BGRA_to_I420_flip(width, height, in, out);
	x = width*height+(width/2)*(height/2)*2;
	while(num-- > 0)
	{
		if(format->canseek)
		{
			SCR_CaptureVideo_RIFF_OverflowCheck(8 + x);
			SCR_CaptureVideo_RIFF_IndexEntry("00dc", x, 0x10); // AVIIF_KEYFRAME
		}

		if(!format->canseek)
		{
			SCR_CaptureVideo_RIFF_Push("RIFF", "AVIX", 12+8+x);
			SCR_CaptureVideo_RIFF_Push("LIST", "movi", 8+x);
		}
		SCR_CaptureVideo_RIFF_Push("00dc", NULL, x);
		SCR_CaptureVideo_RIFF_WriteBytes(out, x);
		SCR_CaptureVideo_RIFF_Pop();
		if(!format->canseek)
		{
			SCR_CaptureVideo_RIFF_Pop();
			SCR_CaptureVideo_RIFF_Pop();
		}
	}
}

static void SCR_CaptureVideo_Avi_EndVideo(void)
{
	LOAD_FORMATSPECIFIC_AVI();

	if(format->canseek)
	{
		// close any open chunks
		SCR_CaptureVideo_RIFF_Finish(true);

		// go back and fix the video frames and audio samples fields
		if(format->videofile_totalframes_offset1)
			if(FS_Seek(cls.capturevideo.videofile, format->videofile_totalframes_offset1, SEEK_SET) >= 0)
			{
				SCR_CaptureVideo_RIFF_Write32(cls.capturevideo.frame);
				SCR_CaptureVideo_RIFF_FlushNoIncrease();
			}
		if(format->videofile_totalframes_offset2)
			if(FS_Seek(cls.capturevideo.videofile, format->videofile_totalframes_offset2, SEEK_SET) >= 0)
			{
				SCR_CaptureVideo_RIFF_Write32(cls.capturevideo.frame);
				SCR_CaptureVideo_RIFF_FlushNoIncrease();
			}
		if (cls.capturevideo.soundrate)
		{
			if(format->videofile_totalsampleframes_offset)
				if(FS_Seek(cls.capturevideo.videofile, format->videofile_totalsampleframes_offset, SEEK_SET) >= 0)
				{
					SCR_CaptureVideo_RIFF_Write32(cls.capturevideo.soundsampleframe);
					SCR_CaptureVideo_RIFF_FlushNoIncrease();
				}
		}
	}

	if (format->riffindexbuffer.data)
	{
		Mem_Free(format->riffindexbuffer.data);
		format->riffindexbuffer.data = NULL;
	}

	FS_Close(cls.capturevideo.videofile);
	cls.capturevideo.videofile = NULL;

	Mem_Free(format);
}

static void SCR_CaptureVideo_Avi_SoundFrame(const portable_sampleframe_t *paintbuffer, size_t length)
{
	LOAD_FORMATSPECIFIC_AVI();
	int x;
	unsigned char bufstereo16le[PAINTBUFFER_SIZE * 4];
	unsigned char* out_ptr;
	size_t i;

	// write the sound buffer as little endian 16bit interleaved stereo
	for(i = 0, out_ptr = bufstereo16le; i < length; i++, out_ptr += 4)
	{
		int n0, n1;

		n0 = paintbuffer[i].sample[0] * 32768.0f;
		n0 = bound(-32768, n0, 32767);
		out_ptr[0] = (unsigned char)n0;
		out_ptr[1] = (unsigned char)(n0 >> 8);

		n1 = paintbuffer[i].sample[1] * 32768.0f;
		n1 = bound(-32768, n1, 32767);
		out_ptr[2] = (unsigned char)n1;
		out_ptr[3] = (unsigned char)(n1 >> 8);
	}

	x = (int)length*4;
	if(format->canseek)
	{
		SCR_CaptureVideo_RIFF_OverflowCheck(8 + x);
		SCR_CaptureVideo_RIFF_IndexEntry("01wb", x, 0x10); // AVIIF_KEYFRAME
	}

	if(!format->canseek)
	{
		SCR_CaptureVideo_RIFF_Push("RIFF", "AVIX", 12+8+x);
		SCR_CaptureVideo_RIFF_Push("LIST", "movi", 8+x);
	}
	SCR_CaptureVideo_RIFF_Push("01wb", NULL, x);
	SCR_CaptureVideo_RIFF_WriteBytes(bufstereo16le, x);
	SCR_CaptureVideo_RIFF_Pop();
	if(!format->canseek)
	{
		SCR_CaptureVideo_RIFF_Pop();
		SCR_CaptureVideo_RIFF_Pop();
	}
}

void SCR_CaptureVideo_Avi_BeginVideo(void)
{
	int width = cls.capturevideo.width;
	int height = cls.capturevideo.height;
	int n, d;
	unsigned int i;
	double aspect;
	char vabuf[1024];

	aspect = vid.mode.width / (vid.mode.height * vid_pixelheight.value);

	cls.capturevideo.format = CAPTUREVIDEOFORMAT_AVI_I420;
	cls.capturevideo.formatextension = "avi";
	cls.capturevideo.videofile = FS_OpenRealFile(va(vabuf, sizeof(vabuf), "%s.%s", cls.capturevideo.basename, cls.capturevideo.formatextension), "wb", false);
	if (!cls.capturevideo.videofile)
	{
		Con_Printf(CON_ERROR "Failed to open video file \"%s\", cancelling video capture.\n", vabuf);
		cls.capturevideo.error = true;
		return;
	}
	cls.capturevideo.writeEndVideo = SCR_CaptureVideo_Avi_EndVideo;
	cls.capturevideo.writeVideoFrame = SCR_CaptureVideo_Avi_VideoFrames;
	cls.capturevideo.writeSoundFrame = SCR_CaptureVideo_Avi_SoundFrame;
	cls.capturevideo.formatspecific = Mem_Alloc(tempmempool, sizeof(capturevideostate_avi_formatspecific_t));
	{
		LOAD_FORMATSPECIFIC_AVI();
		format->canseek = (FS_Seek(cls.capturevideo.videofile, 0, SEEK_SET) == 0);
		SCR_CaptureVideo_RIFF_Start();
		// enclosing RIFF chunk (there can be multiple of these in >1GB files, the later ones are "AVIX" instead of "AVI " and have no header/stream info)
		SCR_CaptureVideo_RIFF_Push("RIFF", "AVI ", format->canseek ? -1 : 12+(8+56+12+(12+52+8+40+8+68)+(cls.capturevideo.soundrate?(12+12+52+8+18):0)+12+(8+4))+12+(8+(((int) strlen(engineversion) | 1) + 1))+12);
		// AVI main header
		SCR_CaptureVideo_RIFF_Push("LIST", "hdrl", format->canseek ? -1 : 8+56+12+(12+52+8+40+8+68)+(cls.capturevideo.soundrate?(12+12+52+8+18):0)+12+(8+4));
		SCR_CaptureVideo_RIFF_Push("avih", NULL, 56);
		SCR_CaptureVideo_RIFF_Write32((int)(1000000.0 / (cls.capturevideo.framerate / cls.capturevideo.framestep))); // microseconds per frame
		SCR_CaptureVideo_RIFF_Write32(0); // max bytes per second
		SCR_CaptureVideo_RIFF_Write32(0); // padding granularity
		SCR_CaptureVideo_RIFF_Write32(0x910); // flags (AVIF_HASINDEX | AVIF_ISINTERLEAVED | AVIF_TRUSTCKTYPE)
		format->videofile_firstchunkframes_offset = SCR_CaptureVideo_RIFF_GetPosition();
		SCR_CaptureVideo_RIFF_Write32(0); // total frames
		SCR_CaptureVideo_RIFF_Write32(0); // initial frames
		if (cls.capturevideo.soundrate)
			SCR_CaptureVideo_RIFF_Write32(2); // number of streams
		else
			SCR_CaptureVideo_RIFF_Write32(1); // number of streams
		SCR_CaptureVideo_RIFF_Write32(0); // suggested buffer size
		SCR_CaptureVideo_RIFF_Write32(width); // width
		SCR_CaptureVideo_RIFF_Write32(height); // height
		SCR_CaptureVideo_RIFF_Write32(0); // reserved[0]
		SCR_CaptureVideo_RIFF_Write32(0); // reserved[1]
		SCR_CaptureVideo_RIFF_Write32(0); // reserved[2]
		SCR_CaptureVideo_RIFF_Write32(0); // reserved[3]
		SCR_CaptureVideo_RIFF_Pop();
		// video stream info
		SCR_CaptureVideo_RIFF_Push("LIST", "strl", format->canseek ? -1 : 12+52+8+40+8+68);
		SCR_CaptureVideo_RIFF_Push("strh", "vids", 52);
		SCR_CaptureVideo_RIFF_WriteFourCC("I420"); // stream fourcc (I420 colorspace, uncompressed)
		SCR_CaptureVideo_RIFF_Write32(0); // flags
		SCR_CaptureVideo_RIFF_Write16(0); // priority
		SCR_CaptureVideo_RIFF_Write16(0); // language
		SCR_CaptureVideo_RIFF_Write32(0); // initial frames
		// find an ideal divisor for the framerate
		FindFraction(cls.capturevideo.framerate / cls.capturevideo.framestep, &n, &d, 1000);
		SCR_CaptureVideo_RIFF_Write32(d); // samples/second divisor
		SCR_CaptureVideo_RIFF_Write32(n); // samples/second multiplied by divisor
		SCR_CaptureVideo_RIFF_Write32(0); // start
		format->videofile_totalframes_offset1 = SCR_CaptureVideo_RIFF_GetPosition();
		SCR_CaptureVideo_RIFF_Write32(0xFFFFFFFF); // length
		SCR_CaptureVideo_RIFF_Write32(width*height+(width/2)*(height/2)*2); // suggested buffer size
		SCR_CaptureVideo_RIFF_Write32(0); // quality
		SCR_CaptureVideo_RIFF_Write32(0); // sample size
		SCR_CaptureVideo_RIFF_Write16(0); // frame left
		SCR_CaptureVideo_RIFF_Write16(0); // frame top
		SCR_CaptureVideo_RIFF_Write16(width); // frame right
		SCR_CaptureVideo_RIFF_Write16(height); // frame bottom
		SCR_CaptureVideo_RIFF_Pop();
		// video stream format
		SCR_CaptureVideo_RIFF_Push("strf", NULL, 40);
		SCR_CaptureVideo_RIFF_Write32(40); // BITMAPINFO struct size
		SCR_CaptureVideo_RIFF_Write32(width); // width
		SCR_CaptureVideo_RIFF_Write32(height); // height
		SCR_CaptureVideo_RIFF_Write16(3); // planes
		SCR_CaptureVideo_RIFF_Write16(12); // bitcount
		SCR_CaptureVideo_RIFF_WriteFourCC("I420"); // compression
		SCR_CaptureVideo_RIFF_Write32(width*height+(width/2)*(height/2)*2); // size of image
		SCR_CaptureVideo_RIFF_Write32(0); // x pixels per meter
		SCR_CaptureVideo_RIFF_Write32(0); // y pixels per meter
		SCR_CaptureVideo_RIFF_Write32(0); // color used
		SCR_CaptureVideo_RIFF_Write32(0); // color important
		SCR_CaptureVideo_RIFF_Pop();
		// master index
		if(format->canseek)
		{
			SCR_CaptureVideo_RIFF_Push("indx", NULL, -1);
			SCR_CaptureVideo_RIFF_Write16(4); // wLongsPerEntry
			SCR_CaptureVideo_RIFF_Write16(0); // bIndexSubType=0, bIndexType=0
			format->videofile_ix_master_video_inuse_offset = SCR_CaptureVideo_RIFF_GetPosition();
			SCR_CaptureVideo_RIFF_Write32(0); // nEntriesInUse
			SCR_CaptureVideo_RIFF_WriteFourCC("00dc"); // dwChunkId
			SCR_CaptureVideo_RIFF_Write32(0); // dwReserved1
			SCR_CaptureVideo_RIFF_Write32(0); // dwReserved2
			SCR_CaptureVideo_RIFF_Write32(0); // dwReserved3
			format->videofile_ix_master_video_start_offset = SCR_CaptureVideo_RIFF_GetPosition();
			for(i = 0; i < AVI_MASTER_INDEX_SIZE * 4; ++i)
				SCR_CaptureVideo_RIFF_Write32(0); // fill up later
			SCR_CaptureVideo_RIFF_Pop();
		}
		// extended format (aspect!)
		SCR_CaptureVideo_RIFF_Push("vprp", NULL, 68);
		SCR_CaptureVideo_RIFF_Write32(0); // VideoFormatToken
		SCR_CaptureVideo_RIFF_Write32(0); // VideoStandard
		SCR_CaptureVideo_RIFF_Write32((int)(cls.capturevideo.framerate / cls.capturevideo.framestep)); // dwVerticalRefreshRate (bogus)
		SCR_CaptureVideo_RIFF_Write32(width); // dwHTotalInT
		SCR_CaptureVideo_RIFF_Write32(height); // dwVTotalInLines
		FindFraction(aspect, &n, &d, 1000);
		SCR_CaptureVideo_RIFF_Write32((n << 16) | d); // dwFrameAspectRatio // TODO a word
		SCR_CaptureVideo_RIFF_Write32(width); // dwFrameWidthInPixels
		SCR_CaptureVideo_RIFF_Write32(height); // dwFrameHeightInLines
		SCR_CaptureVideo_RIFF_Write32(1); // nFieldPerFrame
		SCR_CaptureVideo_RIFF_Write32(width); // CompressedBMWidth
		SCR_CaptureVideo_RIFF_Write32(height); // CompressedBMHeight
		SCR_CaptureVideo_RIFF_Write32(width); // ValidBMHeight
		SCR_CaptureVideo_RIFF_Write32(height); // ValidBMWidth
		SCR_CaptureVideo_RIFF_Write32(0); // ValidBMXOffset
		SCR_CaptureVideo_RIFF_Write32(0); // ValidBMYOffset
		SCR_CaptureVideo_RIFF_Write32(0); // ValidBMXOffsetInT
		SCR_CaptureVideo_RIFF_Write32(0); // ValidBMYValidStartLine
		SCR_CaptureVideo_RIFF_Pop();
		SCR_CaptureVideo_RIFF_Pop();
		if (cls.capturevideo.soundrate)
		{
			// audio stream info
			SCR_CaptureVideo_RIFF_Push("LIST", "strl", format->canseek ? -1 : 12+52+8+18);
			SCR_CaptureVideo_RIFF_Push("strh", "auds", 52);
			SCR_CaptureVideo_RIFF_Write32(1); // stream fourcc (PCM audio, uncompressed)
			SCR_CaptureVideo_RIFF_Write32(0); // flags
			SCR_CaptureVideo_RIFF_Write16(0); // priority
			SCR_CaptureVideo_RIFF_Write16(0); // language
			SCR_CaptureVideo_RIFF_Write32(0); // initial frames
			SCR_CaptureVideo_RIFF_Write32(1); // samples/second divisor
			SCR_CaptureVideo_RIFF_Write32((int)(cls.capturevideo.soundrate)); // samples/second multiplied by divisor
			SCR_CaptureVideo_RIFF_Write32(0); // start
			format->videofile_totalsampleframes_offset = SCR_CaptureVideo_RIFF_GetPosition();
			SCR_CaptureVideo_RIFF_Write32(0xFFFFFFFF); // length
			SCR_CaptureVideo_RIFF_Write32(cls.capturevideo.soundrate * 2); // suggested buffer size (this is a half second)
			SCR_CaptureVideo_RIFF_Write32(0); // quality
			SCR_CaptureVideo_RIFF_Write32(4); // sample size
			SCR_CaptureVideo_RIFF_Write16(0); // frame left
			SCR_CaptureVideo_RIFF_Write16(0); // frame top
			SCR_CaptureVideo_RIFF_Write16(0); // frame right
			SCR_CaptureVideo_RIFF_Write16(0); // frame bottom
			SCR_CaptureVideo_RIFF_Pop();
			// audio stream format
			SCR_CaptureVideo_RIFF_Push("strf", NULL, 18);
			SCR_CaptureVideo_RIFF_Write16(1); // format (uncompressed PCM?)
			SCR_CaptureVideo_RIFF_Write16(2); // channels (stereo)
			SCR_CaptureVideo_RIFF_Write32(cls.capturevideo.soundrate); // sampleframes per second
			SCR_CaptureVideo_RIFF_Write32(cls.capturevideo.soundrate * 4); // average bytes per second
			SCR_CaptureVideo_RIFF_Write16(4); // block align
			SCR_CaptureVideo_RIFF_Write16(16); // bits per sample
			SCR_CaptureVideo_RIFF_Write16(0); // size
			SCR_CaptureVideo_RIFF_Pop();
			// master index
			if(format->canseek)
			{
				SCR_CaptureVideo_RIFF_Push("indx", NULL, -1);
				SCR_CaptureVideo_RIFF_Write16(4); // wLongsPerEntry
				SCR_CaptureVideo_RIFF_Write16(0); // bIndexSubType=0, bIndexType=0
				format->videofile_ix_master_audio_inuse_offset = SCR_CaptureVideo_RIFF_GetPosition();
				SCR_CaptureVideo_RIFF_Write32(0); // nEntriesInUse
				SCR_CaptureVideo_RIFF_WriteFourCC("01wb"); // dwChunkId
				SCR_CaptureVideo_RIFF_Write32(0); // dwReserved1
				SCR_CaptureVideo_RIFF_Write32(0); // dwReserved2
				SCR_CaptureVideo_RIFF_Write32(0); // dwReserved3
				format->videofile_ix_master_audio_start_offset = SCR_CaptureVideo_RIFF_GetPosition();
				for(i = 0; i < AVI_MASTER_INDEX_SIZE * 4; ++i)
					SCR_CaptureVideo_RIFF_Write32(0); // fill up later
				SCR_CaptureVideo_RIFF_Pop();
			}
			SCR_CaptureVideo_RIFF_Pop();
		}

		format->videofile_ix_master_audio_inuse = format->videofile_ix_master_video_inuse = 0;

		// extended header (for total #frames)
		SCR_CaptureVideo_RIFF_Push("LIST", "odml", 8+4);
		SCR_CaptureVideo_RIFF_Push("dmlh", NULL, 4);
		format->videofile_totalframes_offset2 = SCR_CaptureVideo_RIFF_GetPosition();
		SCR_CaptureVideo_RIFF_Write32(0xFFFFFFFF);
		SCR_CaptureVideo_RIFF_Pop();
		SCR_CaptureVideo_RIFF_Pop();

		// close the AVI header list
		SCR_CaptureVideo_RIFF_Pop();
		// software that produced this AVI video file
		SCR_CaptureVideo_RIFF_Push("LIST", "INFO", 8+((strlen(engineversion) | 1) + 1));
		SCR_CaptureVideo_RIFF_Push("ISFT", NULL, strlen(engineversion) + 1);
		SCR_CaptureVideo_RIFF_WriteTerminatedString(engineversion);
		SCR_CaptureVideo_RIFF_Pop();
		// enable this junk filler if you like the LIST movi to always begin at 4KB in the file (why?)
#if 0
		SCR_CaptureVideo_RIFF_Push("JUNK", NULL);
		x = 4096 - SCR_CaptureVideo_RIFF_GetPosition();
		while (x > 0)
		{
			const char *junkfiller = "[ DarkPlaces junk data ]";
			int i = min(x, (int)strlen(junkfiller));
			SCR_CaptureVideo_RIFF_WriteBytes((const unsigned char *)junkfiller, i);
			x -= i;
		}
		SCR_CaptureVideo_RIFF_Pop();
#endif
		SCR_CaptureVideo_RIFF_Pop();
		// begin the actual video section now
		SCR_CaptureVideo_RIFF_Push("LIST", "movi", format->canseek ? -1 : 0);
		format->videofile_ix_movistart = format->riffstackstartoffset[1];
		// we're done with the headers now...
		SCR_CaptureVideo_RIFF_Flush();
		if (format->riffstacklevel != 2)
			Sys_Error("SCR_CaptureVideo_BeginVideo: broken AVI writing code (stack level is %i (should be 2) at end of headers)\n", format->riffstacklevel);

		if(!format->canseek)
		{
			// close the movi immediately
			SCR_CaptureVideo_RIFF_Pop();
			// close the AVI immediately (we'll put all frames into AVIX)
			SCR_CaptureVideo_RIFF_Pop();
		}
	}
}
