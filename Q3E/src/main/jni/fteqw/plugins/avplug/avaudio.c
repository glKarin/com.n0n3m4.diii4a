#include "../plugin.h"
#include "../engine.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

static size_t activedecoders;
static cvar_t *ffmpeg_audiodecoder, *pdeveloper;

#define HAVE_DECOUPLED_API (LIBAVCODEC_VERSION_MAJOR>57 || (LIBAVCODEC_VERSION_MAJOR==57&&LIBAVCODEC_VERSION_MINOR>=36))

struct avaudioctx
{
	//raw file
	uint8_t *filedata;
	size_t fileofs;
	size_t filesize;

	//avformat stuff
	AVFormatContext *pFormatCtx;
	int audioStream;

	AVCodecContext *pACodecCtx;
	AVFrame *pAFrame;

	//decoding
	int64_t lasttime;

	//output audio
	//we throw away data if the format changes. which is awkward, but gah.
	int64_t samples_framestart;
	int samples_channels;
	int samples_speed;
	qaudiofmt_t samples_format;
	qbyte *samples_buffer;
	size_t samples_framecount;
	size_t samples_maxbytes;
};

static void S_AV_Purge(sfx_t *s)
{
	struct avaudioctx *ctx = (struct avaudioctx*)s->decoder.buf;

	s->loadstate = SLS_NOTLOADED;

	// Free the audio decoder
	if (ctx->pACodecCtx)
		avcodec_close(ctx->pACodecCtx);
	av_free(ctx->pAFrame);

	// Close the video file
	avformat_close_input(&ctx->pFormatCtx);

	//free the decoded buffer
	free(ctx->samples_buffer);

	//file storage will be cleared here too
	free(ctx);

	if (s->decoder.ended)
		activedecoders--;
	memset(&s->decoder, 0, sizeof(s->decoder));
}
#define QAF_U8 0x81
#define QAF_S32 0x04
#ifndef MIXER_F32
#define QAF_F32 0x84
#endif
#define QAF_F64 0x88
static void S_AV_ReadFrame(struct avaudioctx *ctx)
{	//reads an audioframe and spits its data into the output sound file for the game engine to use.
	qaudiofmt_t outformat = QAF_S16, informat=QAF_S16;
	int channels = ctx->pACodecCtx->channels;
	int planes = 1, p;
	unsigned int auddatasize = av_samples_get_buffer_size(NULL, ctx->pACodecCtx->channels, ctx->pAFrame->nb_samples, ctx->pACodecCtx->sample_fmt, 1);
	switch(ctx->pACodecCtx->sample_fmt)
	{	//we don't support planar audio. we just treat it as mono instead.
	default:
		auddatasize = 0;
		break;
	case AV_SAMPLE_FMT_U8P:
		planes = channels;
		outformat = QAF_S8;
		informat = QAF_U8;
		break;
	case AV_SAMPLE_FMT_U8:
		planes = 1;
		outformat = QAF_S8;
		informat = QAF_U8;
		break;
	case AV_SAMPLE_FMT_S16P:
		planes = channels;
		outformat = QAF_S16;
		informat = QAF_S16;
		break;
	case AV_SAMPLE_FMT_S16:
		planes = 1;
		outformat = QAF_S16;
		informat = QAF_S16;
		break;

	case AV_SAMPLE_FMT_S32P:
		planes = channels;
		outformat = QAF_S16;
		informat = QAF_S32;
		break;
	case AV_SAMPLE_FMT_S32:
		planes = 1;
		outformat = QAF_S16;
		informat = QAF_S32;
		break;

#ifdef MIXER_F32
	case AV_SAMPLE_FMT_FLTP:
		planes = channels;
		outformat = QAF_F32;
		informat = QAF_F32;
		break;
	case AV_SAMPLE_FMT_FLT:
		planes = 1;
		outformat = QAF_F32;
		informat = QAF_F32;
		break;

	case AV_SAMPLE_FMT_DBLP:
		planes = channels;
		outformat = QAF_F32;
		informat = QAF_F64;
		break;
	case AV_SAMPLE_FMT_DBL:
		planes = 1;
		outformat = QAF_F32;
		informat = QAF_F64;
		break;
#else
	case AV_SAMPLE_FMT_FLTP:
		planes = channels;
		outformat = QAF_S16;
		informat = QAF_F32;
		break;
	case AV_SAMPLE_FMT_FLT:
		planes = 1;
		outformat = QAF_S16;
		informat = QAF_F32;
		break;

	case AV_SAMPLE_FMT_DBLP:
		planes = channels;
		outformat = QAF_S16;
		informat = QAF_F64;
		break;
	case AV_SAMPLE_FMT_DBL:
		planes = 1;
		outformat = QAF_S16;
		informat = QAF_F64;
		break;
#endif
	}

	if (ctx->samples_channels != channels || ctx->samples_speed != ctx->pACodecCtx->sample_rate || ctx->samples_format != outformat)
	{	//something changed, update
		ctx->samples_channels = channels;
		ctx->samples_speed = ctx->pACodecCtx->sample_rate;
		ctx->samples_format = outformat;

		//and discard any decoded audio. this might loose some.
		ctx->samples_framestart += ctx->samples_framecount;
		ctx->samples_framecount = 0;
	}
	if (ctx->samples_maxbytes < (ctx->samples_framecount*QAF_BYTES(ctx->samples_format)*ctx->samples_channels)+auddatasize)
	{
		ctx->samples_maxbytes = (ctx->samples_framecount*QAF_BYTES(ctx->samples_format)*ctx->samples_channels)+auddatasize;
		ctx->samples_maxbytes *= 2;	//slop
		ctx->samples_buffer = realloc(ctx->samples_buffer, ctx->samples_maxbytes);
	}
	if (planes==1 && outformat != QAF_S8 && informat==outformat)
		memcpy(ctx->samples_buffer + ctx->samples_framecount*(QAF_BYTES(ctx->samples_format)*ctx->samples_channels), ctx->pAFrame->data[0], auddatasize);
	else
	{
		void *fte_restrict outv = (ctx->samples_buffer + ctx->samples_framecount*(QAF_BYTES(ctx->samples_format)*ctx->samples_channels));
		size_t i, samples = auddatasize / (planes*QAF_BYTES(informat));
		if (outformat == QAF_S8 && informat == QAF_U8)
		{
			char *out = outv;
			for (p = 0; p < planes; p++, out++)
			{
				unsigned char *in = ctx->pAFrame->data[p];
				for (i = 0; i < samples; i++)
					out[i*planes] = in[i]-128;	//convert from u8 to s8.
			}
		}
		else if (outformat == QAF_S16 && informat == QAF_S16)
		{
			signed short *out = outv;
			for (p = 0; p < planes; p++, out++)
			{
				signed short *in = (signed short *)ctx->pAFrame->data[p];
				for (i = 0; i < samples; i++)
					out[i*planes] = in[i];	//no conversion needed
			}
		}
		else if (outformat == QAF_S16 && informat == QAF_S32)
		{
			signed short *out = outv;
			for (p = 0; p < planes; p++, out++)
			{
				signed int *in = (signed int *)ctx->pAFrame->data[p];
				for (i = 0; i < samples; i++)
					out[i*planes] = in[i]>>16;	//just use the MSBs, no clamping needed.
			}
		}
#ifdef MIXER_F32
		else if (outformat == QAF_F32 && informat == QAF_F32)
		{
			float *out = outv;
			for (p = 0; p < planes; p++, out++)
			{
				float *in = (float *)ctx->pAFrame->data[p];
				for (i = 0; i < samples; i++)
					out[i*planes] = in[i];	//no conversion needed.
			}
		}
		else if (outformat == QAF_F32 && informat == QAF_F64)
		{
			float *out = outv;
			for (p = 0; p < planes; p++, out++)
			{
				double *in = (double *)ctx->pAFrame->data[p];
				for (i = 0; i < samples; i++)
					out[i*planes] = in[i];	//no clamping needed.
			}
		}
#else
		else if (outformat == QAF_S16 && informat == QAF_F32)
		{
			signed short *out = outv;
			for (p = 0; p < planes; p++, out++)
			{
				float *in = (float *)ctx->pAFrame->data[p];
				for (i = 0; i < samples; i++)
				{
					int v = in[i] * 32767;
					if (v < -32768)
						v = -32768;
					if (v > 32767)
						v = 32767;
					out[i*planes] = v;
				}
			}
		}
		else if (outformat == QAF_S16 && informat == QAF_F64)
		{
			signed short *out = outv;
			for (p = 0; p < planes; p++, out++)
			{
				double *in = (double *)ctx->pAFrame->data[p];
				for (i = 0; i < samples; i++)
				{
					int v = in[i] * 32767;
					if (v < -32768)
						v = -32768;
					if (v > 32767)
						v = 32767;
					out[i*planes] = v;
				}
			}
		}
#endif
	}
	ctx->samples_framecount += auddatasize/(QAF_BYTES(informat)*ctx->samples_channels);
}
static sfxcache_t *S_AV_Locate(sfx_t *sfx, sfxcache_t *buf, ssamplepos_t start, int length)
{	//warning: can be called on a different thread.
	struct avaudioctx *ctx = (struct avaudioctx*)sfx->decoder.buf;
	AVPacket		packet;
	int64_t			curtime;

	if (!buf)
		return NULL;

	curtime = start + length;

	while (1)
	{
		if (start < ctx->samples_framestart)
			break;	//o.O rewind!

		if (ctx->samples_framestart+ctx->samples_framecount > curtime)
			break;	//no need yet.

#ifdef HAVE_DECOUPLED_API
		if(0==avcodec_receive_frame(ctx->pACodecCtx, ctx->pAFrame))
		{
			S_AV_ReadFrame(ctx);
			continue;
		}
#endif

		// We're ahead of the previous frame. try and read the next.
		if (av_read_frame(ctx->pFormatCtx, &packet) < 0)
			break;

		// Is this a packet from the video stream?
		if(packet.stream_index==ctx->audioStream)
		{
#ifdef HAVE_DECOUPLED_API
			avcodec_send_packet(ctx->pACodecCtx, &packet);
#else
			int okay;
			int len;
			void *odata = packet.data;
			while (packet.size > 0)
			{	//this old api only decodes part of the packet with each itteration, so keep reading until we decoded the entire thing.
				okay = false;
				len = avcodec_decode_audio4(ctx->pACodecCtx, ctx->pAFrame, &okay, &packet);
				if (len < 0)
					break;
				packet.size -= len;
				packet.data += len;
				if (okay)
					S_AV_ReadFrame(ctx);
			}
			packet.data = odata;
#endif
		}

		// Free the packet that was allocated by av_read_frame
		av_packet_unref(&packet);
	}

	buf->length = ctx->samples_framecount;
	buf->speed = ctx->samples_speed;
	buf->format = ctx->samples_format;
	buf->numchannels = ctx->samples_channels;
	buf->soundoffset = ctx->samples_framestart;
	buf->data = ctx->samples_buffer;

	//if we couldn't return any new data, then we're at an eof, return NULL to signal that.
	if (start == buf->soundoffset + buf->length && length > 0)
		return NULL;

	return buf;
}
static float S_AV_Query(struct sfx_s *sfx, struct sfxcache_s *buf, char *title, size_t titlesize)
{
	struct avaudioctx *ctx = (struct avaudioctx*)sfx->decoder.buf;
	if (!ctx)
		return -1;
	if (buf)
	{
		buf->data = NULL;
		buf->soundoffset = 0;
		buf->length = 0;
		buf->numchannels = ctx->samples_channels;
		buf->speed = ctx->samples_speed;
		buf->format = ctx->samples_format;
	}
	return ctx->pFormatCtx->duration / (float)AV_TIME_BASE;
}

static int AVIO_Mem_Read(void *opaque, uint8_t *buf, int buf_size)
{
	struct avaudioctx *ctx = opaque;
	if (ctx->fileofs > ctx->filesize)
		buf_size = 0;
	if (buf_size > ctx->filesize-ctx->fileofs)
		buf_size = ctx->filesize-ctx->fileofs;
	if (buf_size > 0)
	{
		memcpy(buf, ctx->filedata + ctx->fileofs, buf_size);
		ctx->fileofs += buf_size;
		return buf_size;
	}
	return 0;
}
static int64_t AVIO_Mem_Seek(void *opaque, int64_t offset, int whence)
{
	struct avaudioctx *ctx = opaque;
	whence &= ~AVSEEK_FORCE;
	switch(whence)
	{
	default:
		return -1;
	case SEEK_SET:
		ctx->fileofs = offset;
		break;
	case SEEK_CUR:
		ctx->fileofs += offset;
		break;
	case SEEK_END:
		ctx->fileofs = ctx->filesize + offset;
		break;
	case AVSEEK_SIZE:
		return ctx->filesize;
	}
	if (ctx->fileofs < 0)
		ctx->fileofs = 0;
	return ctx->fileofs;
}

/*const char *COM_GetFileExtension (const char *in)
{
	const char *dot;

	for (dot = in + strlen(in); dot >= in && *dot != '.'; dot--)
		;
	if (dot < in)
		return "";
	in = dot+1;
	return in;
}*/
static qboolean QDECL S_LoadAVSound (sfx_t *s, qbyte *data, size_t datalen, int sndspeed)
{
	struct avaudioctx *ctx;
	int i;
	AVCodec *pCodec;
	const int iBufSize = 4 * 1024;

	if (!ffmpeg_audiodecoder)
		return false;
	if (!ffmpeg_audiodecoder->ival /* && *ffmpeg_audiodecoder.string */)
		return false;


	if (!data || !datalen)
		return false;

	//ignore it if it looks like a wav file. that means we don't need to figure out how to calculate loopstart.
	//FIXME: this also blocks playing the audio from avi files too!
	if (datalen >= 4 && !strncmp(data, "RIFF", 4))
		return false;

//	if (strcasecmp(COM_GetFileExtension(s->name), "wav"))	//don't do .wav - I've no idea how to read the loopstart tag with ffmpeg.
//		return false;

	s->decoder.buf = ctx = malloc(sizeof(*ctx) + datalen);
	if (!ctx)
		return false;	//o.O
	memset(ctx, 0, sizeof(*ctx));

	// Create internal io buffer for FFmpeg
	ctx->filedata = data;	//defer that copy
	ctx->filesize = datalen;	//defer that copy
	ctx->pFormatCtx = avformat_alloc_context();
	ctx->pFormatCtx->pb = avio_alloc_context(av_malloc(iBufSize), iBufSize, 0, ctx, AVIO_Mem_Read, 0, AVIO_Mem_Seek);

	// Open file
	if(avformat_open_input(&ctx->pFormatCtx, s->name, NULL, NULL)==0)
	{
		// Retrieve stream information
		if(avformat_find_stream_info(ctx->pFormatCtx, NULL)>=0)
		{
			ctx->audioStream=-1;
			for(i=0; i<ctx->pFormatCtx->nb_streams; i++)
#if LIBAVFORMAT_VERSION_MAJOR >= 57
				if(ctx->pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO)
#else
				if(ctx->pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO)
#endif
				{
					ctx->audioStream=i;
					break;
				}
			if(ctx->audioStream!=-1)
			{
#if LIBAVFORMAT_VERSION_MAJOR >= 57
				pCodec=avcodec_find_decoder(ctx->pFormatCtx->streams[ctx->audioStream]->codecpar->codec_id);
				ctx->pACodecCtx = avcodec_alloc_context3(pCodec);
				if (avcodec_parameters_to_context(ctx->pACodecCtx, ctx->pFormatCtx->streams[ctx->audioStream]->codecpar) < 0)
				{
					avcodec_free_context(&ctx->pACodecCtx);
					pCodec = NULL;
				}
#else
				ctx->pACodecCtx=ctx->pFormatCtx->streams[ctx->audioStream]->codec;
				pCodec=avcodec_find_decoder(ctx->pACodecCtx->codec_id);
#endif
				ctx->pAFrame=av_frame_alloc();
				if(pCodec!=NULL && ctx->pAFrame && avcodec_open2(ctx->pACodecCtx, pCodec, NULL) >= 0)
				{	//success
				}
				else
					ctx->audioStream = -1;
			}
		}

		if (ctx->audioStream != -1)
		{
			//sucky copy
			ctx->filedata = (uint8_t*)(ctx+1);
			memcpy(ctx->filedata, data, datalen);

			s->decoder.ended = S_AV_Purge;
			s->decoder.purge = S_AV_Purge;
			s->decoder.decodedata = S_AV_Locate;
			s->decoder.querydata = S_AV_Query;
			activedecoders++;
			return true;
		}
	}
	S_AV_Purge(s);
	return false;
}
qboolean AVAudio_MayUnload(void)
{
	return activedecoders==0;
}
static qboolean AVAudio_Init(void)
{
	if (!plugfuncs->ExportFunction("MayUnload", AVAudio_MayUnload) ||
		!plugfuncs->ExportFunction("S_LoadSound", S_LoadAVSound))
	{
		Con_Printf("ffmpeg: Engine doesn't support audio decoder plugins\n");
		return false;
	}
	ffmpeg_audiodecoder = cvarfuncs->GetNVFDG("ffmpeg_audiodecoder_wip", "1", 0, "Enables the use of ffmpeg's decoder for pure audio files.", "ffmpeg");
	if (!ffmpeg_audiodecoder->ival)
		Con_Printf("ffmpeg: audio decoding disabled, use \"set %s 1\" to enable ffmpeg audio decoding\n", ffmpeg_audiodecoder->name);
	return true;
}


//generic module stuff. this has to go somewhere.
static void AVLogCallback(void *avcl, int level, const char *fmt, va_list vl)
{	//needs to be reenterant
#ifdef _DEBUG
	char		string[1024];
	if (level >= AV_LOG_INFO)
		return;	//don't care if its just going to be spam.
	Q_vsnprintf (string, sizeof(string), fmt, vl);
	if (level >= AV_LOG_WARNING)
	{
		if (pdeveloper && pdeveloper->ival)
			Con_Printf("ffmpeg: %s", string);
	}
	else if (level >= AV_LOG_ERROR)
		Con_Printf(CON_WARNING"ffmpeg: %s", string);
	else
		Con_Printf(CON_ERROR"ffmpeg: %s", string);
#endif
}

//get the encoder/decoders to register themselves with the engine, then make sure avformat/avcodec have registered all they have to give.
qboolean AVEnc_Init(void);
qboolean AVDec_Init(void);
qboolean Plug_Init(void)
{
	qboolean okay = false;

	okay |= AVAudio_Init();
	okay |= AVDec_Init();
	okay |= AVEnc_Init();
	if (okay)
	{
#if ( LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100) )
		av_register_all();
		avcodec_register_all();
#endif

		pdeveloper = cvarfuncs->GetNVFDG("developer", "0", 0, "Developer spam.", "ffmpeg");
		av_log_set_level(AV_LOG_WARNING);
		av_log_set_callback(AVLogCallback);
	}
	return okay;
}

