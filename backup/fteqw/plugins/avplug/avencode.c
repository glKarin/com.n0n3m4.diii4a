#include "../plugin.h"
#include "../engine.h"

#include "libavformat/avformat.h"
//#include "libavformat/avio.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"

#ifdef _DIII4A //karin: dynamic load ffmpeg
#include "qffmpeg_inc.h"
#endif

#define TARGET_FFMPEG (LIBAVFORMAT_VERSION_MICRO >= 100)

#if TARGET_FFMPEG
#define ENCODERNAME "ffmpeg"
#else
#define ENCODERNAME "libav"
#endif

#define HAVE_DECOUPLED_API (LIBAVCODEC_VERSION_MAJOR>57 || (LIBAVCODEC_VERSION_MAJOR==57&&LIBAVCODEC_VERSION_MINOR>=36))

//crappy compat crap
#ifndef AV_CODEC_FLAG_GLOBAL_HEADER
#define AV_CODEC_FLAG_GLOBAL_HEADER CODEC_FLAG_GLOBAL_HEADER
#endif
#ifndef AV_ERROR_MAX_STRING_SIZE
#define AV_ERROR_MAX_STRING_SIZE 64
#endif

static plugfsfuncs_t *filefuncs;

/*
Most of the logic in here came from here:
http://svn.gnumonks.org/tags/21c3-video/upstream/ffmpeg-0.4.9-pre1/output_example.c
*/

static cvar_t *ffmpeg_format_force;
static cvar_t *ffmpeg_videocodec;
static cvar_t *ffmpeg_videobitrate;
static cvar_t *ffmpeg_videoforcewidth;
static cvar_t *ffmpeg_videoforceheight;
static cvar_t *ffmpeg_videopreset;
static cvar_t *ffmpeg_video_crf;
static cvar_t *ffmpeg_audiocodec;
static cvar_t *ffmpeg_audiobitrate;

struct encctx
{
	char abspath[MAX_OSPATH];
	AVFormatContext *fc;
	qboolean doneheaders;

	AVCodecContext *video_codec;
	AVStream *video_st;
	struct SwsContext *scale_ctx;
	AVFrame *picture;
	uint8_t *video_outbuf;
	int video_outbuf_size;

	AVCodecContext *audio_codec;
	AVStream *audio_st;
	AVFrame *audio;
	uint8_t *audio_outbuf;
	uint32_t audio_outcount;
	int64_t audio_pts;
};

#define VARIABLE_AUDIO_FRAME_MIN_SIZE 512	//audio frames smaller than a certain size are just wasteful
#define VARIABLE_AUDIO_FRAME_MAX_SIZE 1024

#if !TARGET_FFMPEG
#define av_make_error_string qav_make_error_string
static inline char *av_make_error_string(char *errbuf, size_t errbuf_size, int errnum)
{
	av_strerror(errnum, errbuf, errbuf_size);
	return errbuf;
}
#endif

static void AVEnc_End (void *ctx);

static AVFrame *alloc_frame(enum AVPixelFormat pix_fmt, int width, int height)
{
	AVFrame *picture;
	uint8_t *picture_buf;
	int size;

	picture = av_frame_alloc();
	if(!picture)
		return NULL;
#if TARGET_FFMPEG
	size = av_image_get_buffer_size(pix_fmt, width, height, 1);
#else
	size = avpicture_get_size(pix_fmt, width, height);
#endif
	picture_buf = (uint8_t*)(av_malloc(size));
	if (!picture_buf)
	{
		av_free(picture);
		return NULL;
	}
#if TARGET_FFMPEG
	av_image_fill_arrays(picture->data, picture->linesize, picture_buf, pix_fmt, width, height, 1/*fixme: align*/);
#else
	avpicture_fill((AVPicture*)picture, picture_buf, pix_fmt, width, height);
#endif
	picture->width = width;
	picture->height = height;
	return picture;
}
static AVStream *add_video_stream(struct encctx *ctx, AVCodec *codec, int fps, int width, int height)
{
	AVCodecContext *c;
	AVStream *st;
	int bitrate = ffmpeg_videobitrate->value;
	int forcewidth = ffmpeg_videoforcewidth->value;
	int forceheight = ffmpeg_videoforceheight->value;

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
	st = avformat_new_stream(ctx->fc, NULL);
	if (!st)
		return NULL;
	st->id = ctx->fc->nb_streams-1;
	c = avcodec_alloc_context3(codec);
#else
	st = avformat_new_stream(ctx->fc, codec);
	if (!st)
		return NULL;

	c = st->codec;
	st->id = ctx->fc->nb_streams-1;
#endif
	ctx->video_codec = c;
	c->codec_id = codec->id;
	c->codec_type = codec->type;

	/* put sample parameters */
	if (bitrate)
		c->bit_rate = bitrate;
//	c->rc_max_rate = bitrate;
//	c->rc_min_rate = bitrate;
	/* resolution must be a multiple of two */
	c->width = forcewidth?forcewidth:width;
	c->height = forceheight?forceheight:height;
	/* frames per second */
	c->time_base.num = 1;
	c->time_base.den = fps;
	//c->gop_size = 12; /* emit one intra frame every twelve frames at most */
	c->pix_fmt       = AV_PIX_FMT_YUV420P;
	if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO)
	{
		/* just for testing, we also add B frames */
		c->max_b_frames = 2;
	}
	if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO)
	{
		/* needed to avoid using macroblocks in which some coeffs overflow 
		   this doesnt happen with normal video, it just happens here as the 
		   motion of the chroma plane doesnt match the luma plane */
//		c->mb_decision=2;
	}
	// some formats want stream headers to be seperate
	if (ctx->fc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	if (*ffmpeg_videopreset->string)
		av_opt_set(c->priv_data, "preset", ffmpeg_videopreset->string, AV_OPT_SEARCH_CHILDREN);
	if (*ffmpeg_video_crf->string)
		av_opt_set(c->priv_data, "crf", ffmpeg_video_crf->string, AV_OPT_SEARCH_CHILDREN);

	return st;
}
static void close_video(struct encctx *ctx)
{
	if (!ctx->video_st)
		return;

	avcodec_close(ctx->video_codec);
	if (ctx->picture)
	{
		av_free(ctx->picture->data[0]);
		av_free(ctx->picture);
	}
	av_free(ctx->video_outbuf);
}

#if HAVE_DECOUPLED_API
//frame can be null on eof.
static void AVEnc_DoEncode(AVFormatContext *fc, AVStream *stream, AVCodecContext *codec, AVFrame *frame)
{
	AVPacket pkt;
	int err = avcodec_send_frame(codec, frame);
	if (err)
	{
		char buf[512];
		Con_Printf("avcodec_send_frame: error: %s\n", av_make_error_string(buf, sizeof(buf), err));
	}

	av_init_packet(&pkt);
	while (!(err=avcodec_receive_packet(codec, &pkt)))
	{
		av_packet_rescale_ts(&pkt, codec->time_base, stream->time_base);
		pkt.stream_index = stream->index;
		err = av_interleaved_write_frame(fc, &pkt);
		if (err)
		{
			char buf[512];
			Con_Printf("av_interleaved_write_frame: error: %s\n", av_make_error_string(buf, sizeof(buf), err));
		}
		av_packet_unref(&pkt);
	}
	if (err && err != AVERROR(EAGAIN) && err != AVERROR_EOF)
	{
		char buf[512];
		Con_Printf("avcodec_receive_packet: error: %s\n", av_make_error_string(buf, sizeof(buf), err));
	}
}
#endif

static void AVEnc_Video (void *vctx, int frameno, void *data, int bytestride, int width, int height, enum uploadfmt qpfmt)
{
	struct encctx *ctx = vctx;
	const uint8_t *srcslices[4];
	int srcstride[4];
	int avpfmt;

	if (!ctx->video_st)
		return;

	switch(qpfmt)
	{
	case TF_BGRA32: avpfmt = AV_PIX_FMT_BGRA; break;
	case TF_RGBA32: avpfmt = AV_PIX_FMT_RGBA; break;
#if TARGET_FFMPEG
	case TF_BGRX32: avpfmt = AV_PIX_FMT_BGR0; break;
	case TF_RGBX32: avpfmt = AV_PIX_FMT_RGB0; break;
#endif
	case TF_BGR24: avpfmt = AV_PIX_FMT_BGR24; break;
	case TF_RGB24: avpfmt = AV_PIX_FMT_RGB24; break;
	default:
		return;
	}

	if (bytestride < 0)	//fix up the buffers so callers don't have to.
		data = (char*)data - bytestride*(height-1);

	//weird maths to flip it.
	srcslices[0] = (uint8_t*)data;
	srcstride[0] = bytestride;
	srcslices[1] = NULL;
	srcstride[1] = 0;
	srcslices[2] = NULL;	//libav's version probably needs this excess
	srcstride[2] = 0;
	srcslices[3] = NULL;
	srcstride[3] = 0;

	//fixme: it would be nice to avoid copies here if possible...
	//convert RGB to whatever the codec needs (ie: yuv...).
	//also rescales, but only if the user resizes the video while recording. which is a stupid thing to do.
	ctx->scale_ctx = sws_getCachedContext(ctx->scale_ctx, width, height, avpfmt, ctx->picture->width, ctx->picture->height, ctx->video_codec->pix_fmt, SWS_POINT, 0, 0, 0);
	sws_scale(ctx->scale_ctx, srcslices, srcstride, 0, height, ctx->picture->data, ctx->picture->linesize);

	ctx->picture->pts = frameno;
	ctx->picture->format = ctx->video_codec->pix_fmt;
#if HAVE_DECOUPLED_API
	AVEnc_DoEncode(ctx->fc, ctx->video_st, ctx->video_codec, ctx->picture);
#else
	{
		AVPacket pkt;
		int success;
		int err;

		av_init_packet(&pkt);
		pkt.data = ctx->video_outbuf;
		pkt.size = ctx->video_outbuf_size;
		success = 0;
		err = avcodec_encode_video2(ctx->video_codec, &pkt, ctx->picture, &success);
		if (err)
		{
			char buf[512];
			Con_Printf("avcodec_encode_video2: error: %s\n", av_make_error_string(buf, sizeof(buf), err));
		}
		else if (err == 0 && success)
		{
			av_packet_rescale_ts(&pkt, ctx->video_codec->time_base, ctx->video_st->time_base);
			pkt.stream_index = ctx->video_st->index;
			err = av_interleaved_write_frame(ctx->fc, &pkt);
		
			if (err)
			{
				char buf[512];
				Con_Printf("av_interleaved_write_frame: error: %s\n", av_make_error_string(buf, sizeof(buf), err));
			}
		}
	}
#endif
}

static AVStream *add_audio_stream(struct encctx *ctx, AVCodec *codec, int *samplerate, int *bits, int channels)
{
	AVCodecContext *c;
	AVStream *st;
	int bitrate = ffmpeg_audiobitrate->value;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 48, 101)
	st = avformat_new_stream(ctx->fc, codec);
	if (!st)
		return NULL;

	c = st->codec;
#else
	st = avformat_new_stream(ctx->fc, NULL);
	if (!st)
		return NULL;

	c = avcodec_alloc_context3(codec);
	if(avcodec_parameters_to_context(c, st->codecpar))
		return NULL;
#endif
	st->id = ctx->fc->nb_streams-1;
	ctx->audio_codec = c;
	c->codec_id = codec->id;
	c->codec_type = codec->type;

//	if (c->codec_id == AV_CODEC_ID_OPUS)	//opus is strictly 48khz. force that here.
//		*samplerate = 48000;				//FIXME: the engine can't cope with this.

	/* put sample parameters */
	c->bit_rate = bitrate;
	/* frames per second */
	c->time_base.num = 1;
	c->time_base.den = *samplerate;
	c->sample_rate = *samplerate;
	c->channels = channels;
	c->channel_layout = av_get_default_channel_layout(c->channels);
	c->sample_fmt = codec->sample_fmts[0];

//	if (c->sample_fmt == AV_SAMPLE_FMT_FLTP || c->sample_fmt == AV_SAMPLE_FMT_FLT)
//		*bits = 32;	//get the engine to mix 32bit audio instead of whatever its currently set to.
//	else if (c->sample_fmt == AV_SAMPLE_FMT_U8P || c->sample_fmt == AV_SAMPLE_FMT_U8)
//		*bits = 8;	//get the engine to mix 32bit audio instead of whatever its currently set to.
//	else if (c->sample_fmt == AV_SAMPLE_FMT_S16P || c->sample_fmt == AV_SAMPLE_FMT_S16)
//		*bits = 16;
//	else
		*bits = 32;	//ask for float audio.

	c->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	// some formats want stream headers to be seperate
	if (ctx->fc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

//	avcodec_parameters_from_context(st->codecpar, c);

	return st;
}
static void close_audio(struct encctx *ctx)
{
	if (!ctx->audio_st)
		return;

	avcodec_close(ctx->audio_codec);
}
static void AVEnc_Audio (void *vctx, void *data, int bytes)
{
	struct encctx *ctx = vctx;

	if (!ctx->audio_st)
		return;

	while (bytes)
	{
		int i, p, chans = ctx->audio_codec->channels;
		int blocksize = sizeof(float)*chans;
		int count = bytes / blocksize;
		int planesize = ctx->audio_codec->frame_size;
		float *in;
		int offset;

		if (!planesize)	//variable-sized frames. yay
		{
			planesize = count;
			if (count > VARIABLE_AUDIO_FRAME_MAX_SIZE - ctx->audio_outcount)
				count = VARIABLE_AUDIO_FRAME_MAX_SIZE - ctx->audio_outcount;
		}
		else if (count > ctx->audio_codec->frame_size - ctx->audio_outcount)
			count = ctx->audio_codec->frame_size - ctx->audio_outcount;

		in = (float*)data;
		offset = ctx->audio_outcount;
		ctx->audio_outcount += count;
		data = (qbyte*)data + count * blocksize;
		bytes -= count * blocksize;

		//input is always float audio, because I'm lazy.
		//output is whatever the codec needs (may be packed or planar, gah).
		//the engine's mixer will do all rate scaling for us, as well as channel selection
		switch(ctx->audio_codec->sample_fmt)
		{
		case AV_SAMPLE_FMT_DBL:
			offset *= chans;
			count *= chans;
			planesize *= chans;
			chans = 1;
		case AV_SAMPLE_FMT_DBLP:
			for (p = 0; p < chans; p++)
			{
				double *f = (double*)ctx->audio_outbuf + p*planesize + offset;
				for (i = 0; i < count*chans; i+=chans)
					*f++ = in[i];
				in++;
			}
			break;
		case AV_SAMPLE_FMT_FLT:
			offset *= chans;
			count *= chans;
			planesize *= chans;
			chans = 1;
		case AV_SAMPLE_FMT_FLTP:
			for (p = 0; p < chans; p++)
			{
				float *f = (float *)ctx->audio_outbuf + p*planesize + offset;
				for (i = 0; i < count*chans; i+=chans)
					*f++ = in[i];
				in++;
			}
			break;
		case AV_SAMPLE_FMT_S32:
			offset *= chans;
			count *= chans;
			planesize *= chans;
			chans = 1;
		case AV_SAMPLE_FMT_S32P:
			for (p = 0; p < chans; p++)
			{
				int32_t *f = (int32_t *)ctx->audio_outbuf + p*planesize + offset;
				for (i = 0; i < count*chans; i+=chans)
					*f++ = bound(0x80000000, (int)(in[i] * (float)0x7fffffff), 0x7fffffff);
				in++;
			}
			break;
		case AV_SAMPLE_FMT_S16:
			offset *= chans;
			count *= chans;
			planesize *= chans;
			chans = 1;
		case AV_SAMPLE_FMT_S16P:
			for (p = 0; p < chans; p++)
			{
				int16_t *f = (int16_t *)ctx->audio_outbuf + p*planesize + offset;
				for (i = 0; i < count*chans; i+=chans)
					*f++ = bound(-32768, (int)(in[i] * 32767), 32767);

				//sin((ctx->audio_pts+ctx->audio_outcount-count+i/chans)*0.1) * 32767;//
				in++;
			}
			break;
		case AV_SAMPLE_FMT_U8:
			offset *= chans;
			count *= chans;
			planesize *= chans;
			chans = 1;
		case AV_SAMPLE_FMT_U8P:
			for (p = 0; p < chans; p++)
			{
				uint8_t *f = (uint8_t*)ctx->audio_outbuf + p*planesize + offset;
				for (i = 0; i < count*chans; i+=chans)
					*f++ = bound(0, 128+(int)(in[i] * 127), 255);
				in++;
			}
			break;
		default:
			return;
		}

		if (ctx->audio_codec->frame_size)
		{
			if (ctx->audio_outcount < ctx->audio_codec->frame_size)
				break;	//not enough data yet.
		}
		else
		{
			if (ctx->audio_outcount < VARIABLE_AUDIO_FRAME_MIN_SIZE)
				break;	//not enough data yet.
		}

		ctx->audio->nb_samples = ctx->audio_outcount;
		avcodec_fill_audio_frame(ctx->audio, ctx->audio_codec->channels, ctx->audio_codec->sample_fmt, ctx->audio_outbuf, av_get_bytes_per_sample(ctx->audio_codec->sample_fmt)*ctx->audio_outcount*ctx->audio_codec->channels, 1);
		ctx->audio->pts = ctx->audio_pts;
		ctx->audio_pts += ctx->audio_outcount;
		ctx->audio_outcount = 0;

#if HAVE_DECOUPLED_API
		AVEnc_DoEncode(ctx->fc, ctx->audio_st, ctx->audio_codec, ctx->audio);
#else
		{
			AVPacket pkt;
			int success;
			int err;
			av_init_packet(&pkt);
			pkt.data = NULL;
			pkt.size = 0;
			success = 0;
			err = avcodec_encode_audio2(ctx->audio_codec, &pkt, ctx->audio, &success);

			if (err)
			{
				char buf[512];
				Con_Printf("avcodec_encode_audio2: error: %s\n", av_make_error_string(buf, sizeof(buf), err));
			}
			else if (success)
			{
		//		pkt.pts = ctx->audio_codec->coded_frame->pts;
		//		if(ctx->audio_codec->coded_frame->key_frame)
		//			pkt.flags |= AV_PKT_FLAG_KEY;

				av_packet_rescale_ts(&pkt, ctx->audio_codec->time_base, ctx->audio_st->time_base);
				pkt.stream_index = ctx->audio_st->index;
				err = av_interleaved_write_frame(ctx->fc, &pkt);
				if (err)
				{
					char buf[512];
					Con_Printf("av_interleaved_write_frame: error: %s\n", av_make_error_string(buf, sizeof(buf), err));
				}
			}
		}
#endif
	}
}

static void *AVEnc_Begin (char *streamname, int videorate, int width, int height, int *sndkhz, int *sndchannels, int *sndbits)
{
	struct encctx *ctx;
	AVOutputFormat *fmt = NULL;
	AVCodec *videocodec = NULL;
	AVCodec *audiocodec = NULL;
	int err;
	char errtxt[AV_ERROR_MAX_STRING_SIZE] = {0};

	if (*ffmpeg_format_force->string)
	{
		fmt = av_guess_format(ffmpeg_format_force->string, NULL, NULL);
		if (!fmt)
		{
			Con_Printf("Unknown format specified: %s.\n", ffmpeg_format_force->string);
			return NULL;
		}
	}
	if (!fmt)
		fmt = av_guess_format(NULL, streamname, NULL);
	if (!fmt)
	{
		Con_DPrintf("Could not deduce output format from file extension: using MPEG.\n");
		fmt = av_guess_format("mpeg", NULL, NULL);
	}
	if (!fmt)
	{
		Con_Printf("Format not known\n");
		return NULL;
	}

	if (videorate)
	{
		if (strcmp(ffmpeg_videocodec->string, "none"))
		{
			if (ffmpeg_videocodec->string[0])
			{
				videocodec = avcodec_find_encoder_by_name(ffmpeg_videocodec->string);
				if (!videocodec)
				{
					Con_Printf("Unsupported %s \"%s\"\n", ffmpeg_videocodec->name, ffmpeg_videocodec->string);
					return NULL;
				}
			}
			if (!videocodec && fmt->video_codec != AV_CODEC_ID_NONE)
				videocodec = avcodec_find_encoder(fmt->video_codec);
		}
	}
	if (*sndkhz)
	{
		if (strcmp(ffmpeg_audiocodec->string, "none"))
		{
			if (ffmpeg_audiocodec->string[0])
			{
				audiocodec = avcodec_find_encoder_by_name(ffmpeg_audiocodec->string);
				if (!audiocodec)
				{
					Con_Printf(ENCODERNAME": Unsupported %s \"%s\"\n", ffmpeg_audiocodec->name, ffmpeg_audiocodec->string);
					return NULL;
				}
			}
			if (!audiocodec && fmt->audio_codec != AV_CODEC_ID_NONE)
				audiocodec = avcodec_find_encoder(fmt->audio_codec);
		}
	}

	Con_DPrintf(ENCODERNAME": Using format \"%s\"\n", fmt->name);
	if (videocodec)
		Con_DPrintf(ENCODERNAME": Using Video Codec \"%s\"\n", videocodec->name);
	else
		Con_DPrintf(ENCODERNAME": Not encoding video\n");
	if (audiocodec)
		Con_DPrintf(ENCODERNAME": Using Audio Codec \"%s\"\n", audiocodec->name);
	else
		Con_DPrintf(ENCODERNAME": Not encoding audio\n");

	if (!videocodec && !audiocodec)
	{
		Con_DPrintf(ENCODERNAME": Nothing to encode!\n");
		return NULL;
	}

	if (!audiocodec)
		*sndkhz = 0;

	ctx = malloc(sizeof(*ctx));
	if (!ctx)
		return NULL;
	memset(ctx, 0, sizeof(*ctx));

	ctx->fc = avformat_alloc_context();
	ctx->fc->oformat = fmt;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 6, 100)
	Q_strncatz(ctx->fc->filename, streamname, sizeof(ctx->fc->filename));
#else
	ctx->fc->url = av_strdup(streamname);
#endif

	//pick default codecs
	ctx->video_st = NULL;
	if (videocodec)
		ctx->video_st = add_video_stream(ctx, videocodec, videorate, width, height);
	if (audiocodec)
		ctx->audio_st = add_audio_stream(ctx, audiocodec, sndkhz, sndbits, *sndchannels);

	if (ctx->video_st)
	{
		AVCodecContext *c = ctx->video_codec;
		err = avcodec_open2(c, videocodec, NULL);
		if (err < 0)
		{
			Con_Printf(ENCODERNAME": Could not init codec instance \"%s\" - %s\nMaybe try a different framerate/resolution/bitrate\n", videocodec->name, av_make_error_string(errtxt, sizeof(errtxt), err));
			AVEnc_End(ctx);
			return NULL;
		}

		ctx->picture = alloc_frame(c->pix_fmt, c->width, c->height);

		ctx->video_outbuf_size = 200000;
		ctx->video_outbuf = av_malloc(ctx->video_outbuf_size);
		if (!ctx->video_outbuf)
			ctx->video_outbuf_size = 0;

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
		//copy the avcodec parameters over to avformat
		err = avcodec_parameters_from_context(ctx->video_st->codecpar, c);
		if(err < 0)
		{
			AVEnc_End(ctx);
			return NULL;
		}
#endif
	}
	if (ctx->audio_st)
	{
		int sz;
		AVCodecContext *c = ctx->audio_codec;
		err = avcodec_open2(c, audiocodec, NULL);
		if (err < 0)
		{
			Con_Printf(ENCODERNAME": Could not init codec instance \"%s\" - %s\n", audiocodec->name, av_make_error_string(errtxt, sizeof(errtxt), err));
			AVEnc_End(ctx);
			return NULL;
		}

		ctx->audio = av_frame_alloc();
		sz = ctx->audio_codec->frame_size;
		if (!sz)
			sz = VARIABLE_AUDIO_FRAME_MAX_SIZE;
		sz *= av_get_bytes_per_sample(ctx->audio_codec->sample_fmt) * ctx->audio_codec->channels;
		ctx->audio_outbuf = av_malloc(sz);

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
		//copy the avcodec parameters over to avformat
		err = avcodec_parameters_from_context(ctx->audio_st->codecpar, c);
		if(err < 0)
		{
			AVEnc_End(ctx);
			return NULL;
		}
#endif
	}

	av_dump_format(ctx->fc, 0, streamname, 1);

	if (!(fmt->flags & AVFMT_NOFILE))
	{
		//okay, this is annoying, but I'm too lazy to figure out the issue I was having with avio stuff.
		if (!filefuncs->NativePath(streamname, FS_GAMEONLY, ctx->abspath, sizeof(ctx->abspath)))
		{
			Con_Printf("Couldn't find system path for '%s'\n", streamname);
			AVEnc_End(ctx);
			return NULL;
		}
		err = avio_open(&ctx->fc->pb, ctx->abspath, AVIO_FLAG_WRITE);
		if (err < 0)
		{
			Con_Printf("Could not open '%s' - %s\n", ctx->abspath, av_make_error_string(errtxt, sizeof(errtxt), err));
			AVEnc_End(ctx);
			return NULL;
		}
	}
	else
	{
		strncpy(ctx->abspath, "<STREAM>", sizeof(ctx->abspath)-1);
	}

	//different formats have different metadata formats. there's no standards here.
	//av_dict_set(&ctx->fc-	>metadata, "TPFL", "testtest", 0);
	//FIXME: use ffmpeg's sidedata stuff, which should handle it in a generic way

	//nearly complete, can make the file dirty now.
	err = avformat_write_header(ctx->fc, NULL);
	if (err < 0)
	{
		Con_Printf("avformat_write_header(%s): failed %s\n", ctx->abspath, av_make_error_string(errtxt, sizeof(errtxt), err));
		if (ctx->video_st)
			Con_Printf("  Video %s: %i * %i\n", ctx->video_codec->codec->name, width, height);
		if (ctx->audio_st)
			Con_Printf("  Audio %s: %i channels, %ibit @ %ikhz\n", ctx->audio_codec->codec->name, *sndchannels, *sndbits, *sndkhz);
		AVEnc_End(ctx);
		return NULL;
	}
	ctx->doneheaders = true;
	return ctx;
}
static void AVEnc_End (void *vctx)
{
	struct encctx *ctx = vctx;
	unsigned int i;

#if HAVE_DECOUPLED_API
	if (ctx->doneheaders)
	{
		//terminate the codecs properly, flushing all unwritten packets
		if (ctx->video_st)
			AVEnc_DoEncode(ctx->fc, ctx->video_st, ctx->video_codec, NULL);
		if (ctx->audio_st)
			AVEnc_DoEncode(ctx->fc, ctx->audio_st, ctx->audio_codec, NULL);
	}
#endif

	//don't write trailers if this is an error case and we never even wrote the headers.
	if (ctx->doneheaders)
	{
		av_write_trailer(ctx->fc);
		if (*ctx->abspath)
			Con_Printf("Finished writing %s\n", ctx->abspath);
	}

	close_video(ctx);
	close_audio(ctx);

	for(i = 0; i < ctx->fc->nb_streams; i++)
		av_freep(&ctx->fc->streams[i]);
//	if (!(fmt->flags & AVFMT_NOFILE))
		avio_close(ctx->fc->pb);
	av_free(ctx->audio_outbuf);
	avformat_free_context(ctx->fc);
	free(ctx);
}
static media_encoder_funcs_t encoderfuncs =
{
	sizeof(media_encoder_funcs_t),
	"ffmpeg",
	"Use ffmpeg's various codecs. Various settings are configured with the "ENCODERNAME"_* cvars.",
	".mp4;.*",
	AVEnc_Begin,
	AVEnc_Video,
	AVEnc_Audio,
	AVEnc_End
};


static void AVEnc_Preset_Nvidia_f(void)
{
	cvarfuncs->SetString("capturedriver", ENCODERNAME);	//be sure to use our encoder
	cvarfuncs->SetString(ENCODERNAME"_videocodec", "h264_nvenc");
	cvarfuncs->SetString("capturerate", "60");	//we should be able to cope with it, and the default of 30 sucks
	cvarfuncs->SetString("capturedemowidth", "1920");	//force a specific size, some codecs need multiples of 16 or whatever.
	cvarfuncs->SetString("capturedemoheight", "1080");	//so this avoids issues with various video codecs.

	cvarfuncs->SetString("capturesound", "1");
	cvarfuncs->SetString("capturesoundchannels", "2");
	cvarfuncs->SetString("capturesoundbits", "16");

	Con_Printf(ENCODERNAME": now configured for nvidia's hardware encoder\n");
	Con_Printf(ENCODERNAME": use ^[/capture foo.mp4^] or ^[/capturedemo foo.mvd foo.mkv^] commands to begin capturing\n");
}
void AVEnc_Preset_Defaults_f(void)
{	//most formats will end up using the x264 encoder or something
	cvarfuncs->SetString(ENCODERNAME"_format_force", "");
	cvarfuncs->SetString(ENCODERNAME"_videocodec", "");
	cvarfuncs->SetString(ENCODERNAME"_videobitrate", "");
	cvarfuncs->SetString(ENCODERNAME"_videoforcewidth", "");
	cvarfuncs->SetString(ENCODERNAME"_videoforceheight", "");
	cvarfuncs->SetString(ENCODERNAME"_videopreset", "veryfast");
	cvarfuncs->SetString(ENCODERNAME"_video_crf", "");
	cvarfuncs->SetString(ENCODERNAME"_audiocodec", "");
	cvarfuncs->SetString(ENCODERNAME"_audiobitrate", "");

	cvarfuncs->SetString("capturedriver", ENCODERNAME);
	cvarfuncs->SetString("capturerate", "30");
	cvarfuncs->SetString("capturedemowidth", "0");
	cvarfuncs->SetString("capturedemoheight", "0");
	cvarfuncs->SetString("capturesound", "1");
	cvarfuncs->SetString("capturesoundchannels", "2");
	cvarfuncs->SetString("capturesoundbits", "16");

	Con_Printf(ENCODERNAME": capture settings reset to "ENCODERNAME" defaults\n");
	Con_Printf(ENCODERNAME": Note that some codecs may have restrictions on video sizes\n");
}


qboolean AVEnc_Init(void)
{
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	if (!cvarfuncs || !cmdfuncs || !filefuncs || !plugfuncs->ExportInterface("Media_VideoEncoder", &encoderfuncs, sizeof(encoderfuncs)))
	{
		Con_Printf(ENCODERNAME": Engine doesn't support media encoder plugins\n");
		return false;
	}

	ffmpeg_format_force		= cvarfuncs->GetNVFDG(ENCODERNAME"_format_force",		"",				0, "Forces the output container format. If blank, will guess based upon filename extension.", ENCODERNAME);
	ffmpeg_videocodec		= cvarfuncs->GetNVFDG(ENCODERNAME"_videocodec",			"",				0, "Forces which video encoder to use. If blank, guesses based upon container defaults.\nCommon names are libx264 (software), x264_nvenc (hardware accelerated)", ENCODERNAME);
	ffmpeg_videobitrate		= cvarfuncs->GetNVFDG(ENCODERNAME"_videobitrate",		"",				0, "Specifies the target video bitrate", ENCODERNAME);
	ffmpeg_videoforcewidth	= cvarfuncs->GetNVFDG(ENCODERNAME"_videoforcewidth",	"",				0, "Rescales the input video width. Best to leave blank in order to record the video at the native resolution.", ENCODERNAME);
	ffmpeg_videoforceheight	= cvarfuncs->GetNVFDG(ENCODERNAME"_videoforceheight",	"",				0, "Rescales the input video height. Best to leave blank in order to record the video at the native resolution.", ENCODERNAME);
	ffmpeg_videopreset		= cvarfuncs->GetNVFDG(ENCODERNAME"_videopreset",		"veryfast",		0, "Specifies which codec preset to use, for codecs that support such presets.", ENCODERNAME);
	ffmpeg_video_crf		= cvarfuncs->GetNVFDG(ENCODERNAME"_video_crf",			"",				0, "Specifies the 'Constant Rate Factor' codec setting.\nA value of 0 is 'lossless', a value of 51 is 'worst quality posible', a value of 23 is default.", ENCODERNAME);
	ffmpeg_audiocodec		= cvarfuncs->GetNVFDG(ENCODERNAME"_audiocodec",			"",				0, "Forces which audio encoder to use. If blank, guesses based upon container defaults.", ENCODERNAME);
	ffmpeg_audiobitrate		= cvarfuncs->GetNVFDG(ENCODERNAME"_audiobitrate",		"",				0, "Specifies the target audio bitrate", ENCODERNAME);

//	cmdfuncs->AddCommand(ENCODERNAME"_configure", AVEnc_LoadPreset_f);
	cmdfuncs->AddCommand(ENCODERNAME"_nvidia", AVEnc_Preset_Nvidia_f, "Attempts to reconfigure video capture to use nvidia's hardware encoder.");
	cmdfuncs->AddCommand(ENCODERNAME"_defaults", AVEnc_Preset_Defaults_f, "Reconfigures video capture to the "ENCODERNAME" plugin's default settings.");
	//cmdfuncs->AddCommand(ENCODERNAME"_twitch", AVEnc_Preset_Twitch_f, "Reconfigures video capture to stream to twitch.");

	return true;
}

