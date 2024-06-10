/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


#pragma hdrstop

#include "RenderSystem_local.h"
//
#define CIN_system	1
#define CIN_loop	2
#define	CIN_hold	4
#define CIN_silent	8
#define CIN_shader	16

#ifdef __ANDROID__ //karin: using ffmpeg for play bink video from RBDoom3-BFG
#if 0
struct HBINK_s
{
	int Width, Height;
	int FrameNum, Frames;
};
typedef HBINK_s * HBINK;
typedef void * HBINKBUFFER;

#define BinkOpen(...) 0
#define BinkClose(...)
#define BinkCopyToBufferRect(...)
#define BinkNextFrame(...)
#define BinkDoFrame(...)
#define BinkWait(...) 1
#endif

// Carl: ffmpg for bink video files
extern "C"
{

//#ifdef WIN32
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif
//#include <inttypes.h>
//#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
}
#include "../sound/CinematicAudio.h"
#include "../sound/OpenAL/AL_CinematicAudio.h"
#define NUM_LAG_FRAMES 15	// SRS - Lag audio by 15 frames (~1/2 sec at 30 fps) for ffmpeg bik decoder AV sync

const int DEFAULT_CIN_WIDTH		= 1280;
const int DEFAULT_CIN_HEIGHT	= 720;

#else

#include "../external/bink/bink.h"

#endif

class idCinematicLocal : public idCinematic {
public:
	idCinematicLocal();
	~idCinematicLocal();


	virtual bool			InitFromFile( const char *qpath, bool looping );
	virtual cinData_t		ImageForTime( int milliseconds );
	virtual void			Close();
	virtual void			ResetTime(int time);
	virtual bool			IsDone() { return isDone; }

	virtual idImage*		GetRenderImage() { return renderImage; }
private:
#if !defined(__ANDROID__)
	HBINK Bink = 0;
	HBINKBUFFER Bink_buffer = 0;
	cinData_t currentFrameData;
#endif
	bool isLooping = false;
	bool isDone = false;

	idImage* renderImage;
#ifdef __ANDROID__
	int						animationLength;
	idStr					fileName;
	cinStatus_t				status;
	int						startTime;
	int						CIN_WIDTH, CIN_HEIGHT;
	float					frameRate;
	byte* 					image;
	byte* 					buf;

	int						video_stream_index;
	int						audio_stream_index; //GK: Make extra indexer for audio
	AVFormatContext*		fmt_ctx;
	AVFrame*				frame;
	AVFrame*				frame2;
	AVFrame*				frame3; //GK: make extra frame for audio
#if LIBAVCODEC_VERSION_MAJOR > 58
	const AVCodec*			dec;
	const AVCodec*			dec2;	// SRS - Separate decoder for audio
#else
	AVCodec*				dec;
	AVCodec*				dec2;	// SRS - Separate decoder for audio
#endif
	AVCodecContext*			dec_ctx;
	AVCodecContext*			dec_ctx2;
	SwsContext*				img_convert_ctx;
	bool					hasFrame;
	long					framePos;
	AVSampleFormat			dst_smp;
	bool					hasplanar;
	SwrContext*				swr_ctx;
	cinData_t				ImageForTimeFFMPEG( int milliseconds );
	bool					InitFromFFMPEGFile( const char* qpath, bool looping );
	void					FFMPEGReset();
	uint8_t*				lagBuffer[NUM_LAG_FRAMES] = {};
	int						lagBufSize[NUM_LAG_FRAMES] = {};
	int						lagIndex;
	bool					skipLag;

	//GK:Also init variables for XAudio2 or OpenAL (SRS - this must be an instance variable)
	CinematicAudio*			cinematicAudio;
#endif
};

//===========================================

/*
====================
idCinematic::InitCinematic
====================
*/
void idCinematic::InitCinematic(void) {

}

/*
====================
idCinematic::ShutdownCinematic
====================
*/
void idCinematic::ShutdownCinematic(void) {

}

/*
====================
idCinematicLocal::idCinematicLocal
====================
*/
idCinematicLocal::idCinematicLocal() {
	renderImage = nullptr;
#if !defined(__ANDROID__)
	Bink = nullptr;
#else
	// Carl: ffmpeg stuff, for bink and normal video files:

	image = NULL;
	status = FMV_EOF;
	buf = NULL;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
	frame = av_frame_alloc();
	frame2 = av_frame_alloc();
	frame3 = av_frame_alloc();
#else
	frame = avcodec_alloc_frame();
	frame2 = avcodec_alloc_frame();
	frame3 = avcodec_alloc_frame();
#endif // LIBAVCODEC_VERSION_INT

	dec_ctx = NULL;
	dec_ctx2 = NULL;
	fmt_ctx = NULL;
	video_stream_index = -1;
	audio_stream_index = -1;
	hasplanar = false;
	swr_ctx = NULL;
	img_convert_ctx = NULL;
	hasFrame = false;
	framePos = -1;
	lagIndex = 0;
	skipLag = false;
	
	cinematicAudio = new CinematicAudio_OpenAL;
#endif
}

/*
====================
idCinematicLocal::~idCinematicLocal
====================
*/
idCinematicLocal::~idCinematicLocal() {
	if (renderImage)
	{
		renderImage->PurgeImage();
		renderImage = nullptr;
	}

#if !defined(__ANDROID__)
	if (Bink)
	{
		BinkClose(Bink);
		Bink = nullptr;
	}
#else
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
	av_frame_free( &frame );
	av_frame_free( &frame2 );
	av_frame_free( &frame3 );
#else
	av_freep( &frame );
	av_freep( &frame2 );
	av_freep( &frame3 );
#endif
	//GK: Properly close local XAudio2 or OpenAL voice
	cinematicAudio->ShutdownAudio();
#endif
}

/*
====================
idCinematic::Alloc
====================
*/
idCinematic* idCinematic::Alloc() {
	return new idCinematicLocal();
}

/*
====================
idCinematic::InitFromFile
====================
*/
bool idCinematicLocal::InitFromFile(const char* qpath, bool looping) {
	idStr osPath = fileSystem->RelativePathToOSPath(qpath);

#if !defined(__ANDROID__)
	// Open the movie file
	Bink = BinkOpen(osPath.c_str(), 0);
	if (!Bink)
	{
		Bink = nullptr;
		common->Warning("Failed to open %s", qpath);
		return false;
	}

	currentFrameData.imageWidth = Bink->Width;
	currentFrameData.imageHeight = Bink->Height;
	currentFrameData.image = new byte[Bink->Width * Bink->Height * 4];
	currentFrameData.status = FMV_PLAY;

	{
		idImageOpts opts;
		static int numRenderImages = 0;

		opts.format = FMT_RGBA8;
		opts.colorFormat = CFM_DEFAULT;
		opts.numLevels = 1;
		opts.textureType = TT_2D;
		opts.isPersistant = true;
		opts.width = Bink->Width;
		opts.height = Bink->Height;
		opts.numMSAASamples = 0;

		renderImage = renderSystem->CreateImage(va("_binkMovie%d", numRenderImages++), &opts, TF_NEAREST);
	}
#endif

	

	isDone = false;
	isLooping = looping;

#ifdef __ANDROID__
	fileName = qpath;
	{
		// SRS End

		animationLength = 0;
		//idLib::Warning( "New filename: '%s'\n", fileName.c_str() );
		return InitFromFFMPEGFile( fileName.c_str(), looping );
	}
#else
	return true;
#endif
}

/*
====================
idCinematic::ImageForTime
====================
*/
cinData_t idCinematicLocal::ImageForTime(int milliseconds) {
#if !defined(__ANDROID__)
	if (!BinkWait(Bink))
	{
		if (Bink->FrameNum == Bink->Frames)
		{
			if (!isLooping)
			{
				currentFrameData.status = FMV_EOF;
				isDone = true;
				return currentFrameData;
			}

			Bink->FrameNum = 0;
		}

		BinkDoFrame(Bink);

		BinkCopyToBufferRect(Bink,
			(void *)currentFrameData.image,
			Bink->Width * 4,
			Bink->Height,
			0, 0,
			0, 0, Bink->Width, Bink->Height,
			BINKSURFACE32 |
			BINKCOPYALL);

		// RGB to BGR
		byte* image_data = (byte*)currentFrameData.image;
		for (int i = 0; i < Bink->Width * Bink->Height * 4; i += 4)
		{
			byte r = image_data[i + 0];
			image_data[i + 0] = image_data[i + 2];
			image_data[i + 2] = r;
		}

		BinkNextFrame(Bink);
	}

	return currentFrameData;
#else
	return ImageForTimeFFMPEG( milliseconds );
#endif
}


/*
==============
idSndWindow::Close
==============
*/
void idCinematicLocal::Close() {
#if !defined(__ANDROID__)
	if (Bink == nullptr)
		return;

	delete currentFrameData.image;

	BinkClose(Bink);
#else
	if( image )
	{
		Mem_Free( ( void* )image );
		image = NULL;
		buf = NULL;
		status = FMV_EOF;
	}

	if( img_convert_ctx )
	{
		sws_freeContext( img_convert_ctx );
		img_convert_ctx = NULL;
	}

	// SRS - Free audio codec context, resample context, and any lagged audio buffers
	if( dec_ctx2 )
	{
		avcodec_free_context( &dec_ctx2 );

		// SRS - Free resample context if we were decoding planar audio
		if( swr_ctx )
		{
			swr_free( &swr_ctx );
		}

		for( int i = 0; i < NUM_LAG_FRAMES; i++ )
		{
			lagBufSize[ i ] = 0;
			if( lagBuffer[ i ] )
			{
				av_freep( &lagBuffer[ i ] );
			}
		}
	}

	if( dec_ctx )
	{
		avcodec_free_context( &dec_ctx );
	}

	if( fmt_ctx )
	{
		avformat_close_input( &fmt_ctx );
	}
	status = FMV_EOF;
#endif
}

/*
==============
idSndWindow::ResetTime
==============
*/
void idCinematicLocal::ResetTime(int time) {
#if !defined(__ANDROID__)
	if(Bink)
		Bink->FrameNum = 0;
#endif
	isDone = false;
#ifdef __ANDROID__
	startTime = time; //originally this was: ( backEnd.viewDef ) ? 1000 * backEnd.viewDef->floatTime : -1;
	status = FMV_PLAY;
#endif
}

#ifdef __ANDROID__
/*
==============
idCinematicLocal::GetSampleFormat
==============
*/
const char* GetSampleFormat( AVSampleFormat sample_fmt )
{
	switch( sample_fmt )
	{
		case AV_SAMPLE_FMT_U8:
		case AV_SAMPLE_FMT_U8P:
		{
			return "8-bit";
		}
		case AV_SAMPLE_FMT_S16:
		case AV_SAMPLE_FMT_S16P:
		{
			return "16-bit";
		}
		case AV_SAMPLE_FMT_S32:
		case AV_SAMPLE_FMT_S32P:
		{
			return "32-bit";
		}
		case AV_SAMPLE_FMT_FLT:
		case AV_SAMPLE_FMT_FLTP:
		{
			return "Float";
		}
		case AV_SAMPLE_FMT_DBL:
		case AV_SAMPLE_FMT_DBLP:
		{
			return "Double";
		}
		default:
		{
			return "Unknown";
		}
	}
}

/*
==============
idCinematicLocal::InitFromFFMPEGFile
==============
*/
bool idCinematicLocal::InitFromFFMPEGFile( const char* qpath, bool looping )
{
	int ret;
	int ret2;
	int file_size;
	isLooping = looping;
	startTime = 0;
	CIN_HEIGHT = DEFAULT_CIN_HEIGHT;
	CIN_WIDTH  =  DEFAULT_CIN_WIDTH;

	idStr fullpath;
	idFile* testFile = fileSystem->OpenFileRead( qpath );
	if( testFile )
	{
		fullpath = testFile->GetFullPath();
		file_size = testFile->Length();
		fileSystem->CloseFile( testFile );
	}
	// RB: case sensitivity HACK for Linux
	
	//idStr fullpath = fileSystem->RelativePathToOSPath( qpath, "fs_basepath" );

	if( ( ret = avformat_open_input( &fmt_ctx, fullpath, NULL, NULL ) ) < 0 )
	{
		// SRS - another case sensitivity hack for Linux, this time for ffmpeg and RoQ files
		idStr ext;
		fullpath.ExtractFileExtension( ext );
		if( idStr::Cmp( ext.c_str(), "roq" ) == 0 )
		{
			// SRS - If ffmpeg can't open .roq file, then try again with .RoQ extension instead
			fullpath.Replace( ".roq", ".RoQ" );
			ret = avformat_open_input( &fmt_ctx, fullpath, NULL, NULL );
		}
		if( ret < 0 )
		{
			common->Warning( "idCinematic: Cannot open FFMPEG video file: '%s', %d\n", qpath, looping );
			return false;
		}
	}
	if( ( ret = avformat_find_stream_info( fmt_ctx, NULL ) ) < 0 )
	{
		common->Warning( "idCinematic: Cannot find stream info: '%s', %d\n", qpath, looping );
		return false;
	}
	/* select the video stream */
	ret = av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0 );
	if( ret < 0 )
	{
		common->Warning( "idCinematic: Cannot find a video stream in: '%s', %d\n", qpath, looping );
		return false;
	}
	video_stream_index = ret;
	dec_ctx = avcodec_alloc_context3( dec );
	if( ( ret = avcodec_parameters_to_context( dec_ctx, fmt_ctx->streams[video_stream_index]->codecpar ) ) < 0 )
	{
		char* error = new char[256];
		av_strerror( ret, error, 256 );
		common->Warning( "idCinematic: Failed to create video codec context from codec parameters with error: %s\n", error );
	}
	dec_ctx->time_base = fmt_ctx->streams[video_stream_index]->time_base;
	dec_ctx->framerate = fmt_ctx->streams[video_stream_index]->avg_frame_rate;
	dec_ctx->pkt_timebase = fmt_ctx->streams[video_stream_index]->time_base;
	/* init the video decoder */
	if( ( ret = avcodec_open2( dec_ctx, dec, NULL ) ) < 0 )
	{
		char* error = new char[256];
		av_strerror( ret, error, 256 );
		common->Warning( "idCinematic: Cannot open video decoder for: '%s', %d, with error: %s\n", qpath, looping, error );
		return false;
	}
	//GK:Begin
	//After the video decoder is open then try to open audio decoder
	ret2 = av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &dec2, 0 );
	if( ret2 >= 0 )  //Make audio optional (only intro video has audio no other)
	{
		audio_stream_index = ret2;
		dec_ctx2 = avcodec_alloc_context3( dec2 );
		if( ( ret2 = avcodec_parameters_to_context( dec_ctx2, fmt_ctx->streams[audio_stream_index]->codecpar ) ) < 0 )
		{
			char* error = new char[256];
			av_strerror( ret2, error, 256 );
			common->Warning( "idCinematic: Failed to create audio codec context from codec parameters with error: %s\n", error );
		}
		dec_ctx2->time_base = fmt_ctx->streams[audio_stream_index]->time_base;
		dec_ctx2->framerate = fmt_ctx->streams[audio_stream_index]->avg_frame_rate;
		dec_ctx2->pkt_timebase = fmt_ctx->streams[audio_stream_index]->time_base;
		if( ( ret2 = avcodec_open2( dec_ctx2, dec2, NULL ) ) < 0 )
		{
			common->Warning( "idCinematic: Cannot open audio decoder for: '%s', %d\n", qpath, looping );
			//return false;
		}
		if( dec_ctx2->sample_fmt >= AV_SAMPLE_FMT_U8P )											// SRS - Planar formats start at AV_SAMPLE_FMT_U8P
		{
			dst_smp = static_cast<AVSampleFormat>( dec_ctx2->sample_fmt - AV_SAMPLE_FMT_U8P );	// SRS - Setup context to convert from planar to packed
			swr_ctx = swr_alloc_set_opts( NULL, dec_ctx2->channel_layout, dst_smp, dec_ctx2->sample_rate, dec_ctx2->channel_layout, dec_ctx2->sample_fmt, dec_ctx2->sample_rate, 0, NULL );
			int res = swr_init( swr_ctx );
			hasplanar = true;
		}
		else
		{
			dst_smp = dec_ctx2->sample_fmt;														// SRS - Must always define the destination format
			hasplanar = false;
		}
		common->Printf( "Cinematic audio stream found: Sample Rate=%d Hz, Channels=%d, Format=%s, Planar=%d\n", dec_ctx2->sample_rate, dec_ctx2->channels, GetSampleFormat( dec_ctx2->sample_fmt ), hasplanar );
		cinematicAudio->InitAudio( dec_ctx2 );
	}
	else
	{
		// SRS - Most cinematics have no audio, so disable the warning to reduce distracting log messages
		//common->Warning("idCinematic: Cannot find an audio stream in: '%s', %d\n", qpath, looping);
	}
	//GK:End
	CIN_WIDTH = dec_ctx->width;
	CIN_HEIGHT = dec_ctx->height;
	/** Calculate Duration in seconds
	  * This is the fundamental unit of time (in seconds) in terms
	  * of which frame timestamps are represented. For fixed-fps content,
	  * timebase should be 1/framerate and timestamp increments should be
	  * identically 1.
	  * - encoding: MUST be set by user.
	  * - decoding: Set by libavcodec.
	  */
	AVRational avr = dec_ctx->time_base;
	/**
	 * For some codecs, the time base is closer to the field rate than the frame rate.
	 * Most notably, H.264 and MPEG-2 specify time_base as half of frame duration
	 * if no telecine is used ...
	 *
	 * Set to time_base ticks per frame. Default 1, e.g., H.264/MPEG-2 set it to 2.
	 */
	int ticksPerFrame = dec_ctx->ticks_per_frame;
	// SRS - In addition to ticks, must also use time_base numerator (not always 1) and denominator in the duration calculation
	float durationSec = static_cast<double>( fmt_ctx->streams[video_stream_index]->duration ) * static_cast<double>( ticksPerFrame ) * static_cast<double>( avr.num ) / static_cast<double>( avr.den );
	//GK: No duration is given. Check if we get at least bitrate to calculate the length, otherwise set it to a fixed 100 seconds (should it be lower ?)
	if( durationSec < 0 )
	{
		// SRS - First check the file context bit rate and estimate duration using file size and overall bit rate
		if( fmt_ctx->bit_rate > 0 )
		{
			durationSec = file_size * 8.0 / fmt_ctx->bit_rate;
		}
		// SRS - Likely an RoQ file, so use the video bit rate tolerance plus audio bit rate to estimate duration, then add 10% to correct for variable bit rate
		else if( dec_ctx->bit_rate_tolerance > 0 )
		{
			durationSec = file_size * 8.0 / ( dec_ctx->bit_rate_tolerance + ( dec_ctx2 ? dec_ctx2->bit_rate : 0 ) ) * 1.1;
		}
		// SRS - Otherwise just set a large max duration
		else
		{
			durationSec = 100.0;
		}
	}
	animationLength = durationSec * 1000;
	frameRate = av_q2d( fmt_ctx->streams[video_stream_index]->avg_frame_rate );
	common->Printf( "Loaded FFMPEG file: '%s', looping=%d, %dx%d, %3.2f FPS, %4.1f sec\n", qpath, looping, CIN_WIDTH, CIN_HEIGHT, frameRate, durationSec );

	// SRS - Get number of image bytes needed by querying with NULL first, then allocate image and fill with correct parameters
	int img_bytes = av_image_fill_arrays( frame2->data, frame2->linesize, NULL, AV_PIX_FMT_BGR32, CIN_WIDTH, CIN_HEIGHT, 1 );
	image = ( byte* )Mem_Alloc( img_bytes );
	av_image_fill_arrays( frame2->data, frame2->linesize, image, AV_PIX_FMT_BGR32, CIN_WIDTH, CIN_HEIGHT, 1 ); //GK: Straight out of the FFMPEG source code
	img_convert_ctx = sws_getContext( dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt, CIN_WIDTH, CIN_HEIGHT, AV_PIX_FMT_BGR32, SWS_BICUBIC, NULL, NULL, NULL );

	{
		idImageOpts opts;
		static int numRenderImages = 0;

		opts.format = FMT_RGBA8;
		opts.colorFormat = CFM_DEFAULT;
		opts.numLevels = 1;
		opts.textureType = TT_2D;
		opts.isPersistant = true;
		opts.width = CIN_WIDTH;
		opts.height = CIN_HEIGHT;
		opts.numMSAASamples = 0;

		renderImage = renderSystem->CreateImage(va("_binkMovie%d", numRenderImages++), &opts, TF_NEAREST);
	}

	status = FMV_PLAY;
	hasFrame = false;
	framePos = -1;
	ImageForTime( 0 );
	status = ( looping ) ? FMV_PLAY : FMV_IDLE;

	return true;
}

/*
==============
idCinematicLocal::FFMPEGReset
==============
*/
void idCinematicLocal::FFMPEGReset()
{
	// RB: don't reset startTime here because that breaks video replays in the PDAs
	//startTime = 0;

	framePos = -1;

	// SRS - If we have an ffmpeg audio context and are not looping, or skipLag is true, reset audio to release any stale buffers
	if( dec_ctx2 && ( !( isLooping && status == FMV_EOF ) || skipLag ) )
	{
		cinematicAudio->ResetAudio();

		for( int i = 0; i < NUM_LAG_FRAMES; i++ )
		{
			lagBufSize[ i ] = 0;
			if( lagBuffer[ i ] )
			{
				av_freep( &lagBuffer[ i ] );
			}
		}
	}

	// SRS - For non-RoQ (i.e. bik) files, use standard frame seek to rewind the stream
	if( dec_ctx->codec_id != AV_CODEC_ID_ROQ && av_seek_frame( fmt_ctx, video_stream_index, 0, 0 ) >= 0 )
	{
		status = FMV_LOOPED;
	}
	// SRS - Special handling for RoQ files: only byte seek works and ffmpeg RoQ decoder needs reset
	else if( dec_ctx->codec_id == AV_CODEC_ID_ROQ && av_seek_frame( fmt_ctx, video_stream_index, 0, AVSEEK_FLAG_BYTE ) >= 0 )
	{
		// Close and reopen the ffmpeg RoQ codec without clearing the context - this seems to reset the decoder properly
		avcodec_close( dec_ctx );
		avcodec_open2( dec_ctx, dec, NULL );

		status = FMV_LOOPED;
	}
	// SRS - Can't rewind the stream so we really are at EOF
	else
	{
		status = FMV_EOF;
	}
}

/*
==============
idCinematicLocal::ImageForTimeFFMPEG
==============
*/
cinData_t idCinematicLocal::ImageForTimeFFMPEG( int thisTime )
{
	cinData_t	cinData;
	uint8_t*	audioBuffer = NULL;
	int			num_bytes = 0;

	if( thisTime <= 0 )
	{
		thisTime = Sys_Milliseconds();
	}

	memset( &cinData, 0, sizeof( cinData ) );
	if( r_skipDynamicTextures.GetBool() || status == FMV_EOF || status == FMV_IDLE )
	{
		return cinData;
	}

	if( !fmt_ctx )
	{
		// RB: .bik requested but not found
		return cinData;
	}

	if( ( !hasFrame ) || startTime == -1 )
	{
		if( startTime == -1 )
		{
			FFMPEGReset();
		}
		startTime = thisTime;
	}

	long desiredFrame = ( ( thisTime - startTime ) * frameRate ) / 1000;
	if( desiredFrame < 0 )
	{
		desiredFrame = 0;
	}

	if( desiredFrame < framePos )
	{
		FFMPEGReset();
		hasFrame = false;
		status = FMV_PLAY;
	}

	if( hasFrame && desiredFrame == framePos )
	{
		cinData.imageWidth = CIN_WIDTH;
		cinData.imageHeight = CIN_HEIGHT;
		cinData.status = status;
		cinData.image = image;
		return cinData;
	}

	AVPacket packet;
	while( framePos < desiredFrame )
	{
		int frameFinished = -1;
		//GK: Separate frame finisher for audio in order to not have the video lagging
		int frameFinished1 = -1;

		int res = 0;

		// Do a single frame by getting packets until we have a full frame
		while( frameFinished != 0 )
		{
			// if we got to the end or failed
			if( av_read_frame( fmt_ctx, &packet ) < 0 )
			{
				// can't read any more, set to EOF
				status = FMV_EOF;
				if( isLooping )
				{
					desiredFrame = 0;
					FFMPEGReset();
					hasFrame = false;
					startTime = thisTime;
					if( av_read_frame( fmt_ctx, &packet ) < 0 )
					{
						status = FMV_IDLE;
						return cinData;
					}
					status = FMV_PLAY;
				}
				else
				{
					hasFrame = false;
					status = FMV_IDLE;
					isDone = true;
					return cinData;
				}
			}
			// Is this a packet from the video stream?
			if( packet.stream_index == video_stream_index )
			{
				// Decode video frame
				if( ( res = avcodec_send_packet( dec_ctx, &packet ) ) != 0 )
				{
					char* error = new char[256];
					av_strerror( res, error, 256 );
					common->Warning( "idCinematic: Failed to send video packet for decoding with error: %s\n", error );
				}
				else
				{
					if( ( frameFinished = avcodec_receive_frame( dec_ctx, frame ) ) != 0 )
					{
						char* error = new char[256];
						av_strerror( frameFinished, error, 256 );
						common->Warning( "idCinematic: Failed to receive video frame from decoding with error: %s\n", error );
					}
				}
			}
			//GK:Begin
			else if( packet.stream_index == audio_stream_index ) //Check if it found any audio data
			{
				res = avcodec_send_packet( dec_ctx2, &packet );
				if( res != 0 && res != AVERROR( EAGAIN ) )
				{
					char* error = new char[256];
					av_strerror( res, error, 256 );
					common->Warning( "idCinematic: Failed to send audio packet for decoding with error: %s\n", error );
				}
				else
				{
					if( ( frameFinished1 = avcodec_receive_frame( dec_ctx2, frame3 ) ) != 0 )
					{
						char* error = new char[256];
						av_strerror( frameFinished1, error, 256 );
						common->Warning( "idCinematic: Failed to receive audio frame from decoding with error: %s\n", error );
					}
					else
					{
						// SRS - Since destination sample format is packed (non-planar), returned bufflinesize equals num_bytes
						res = av_samples_alloc( &audioBuffer, &num_bytes, frame3->channels, frame3->nb_samples, dst_smp, 0 );
						if( res < 0 || res != num_bytes )
						{
							common->Warning( "idCinematic: Failed to allocate audio buffer with result: %d\n", res );
						}
						if( hasplanar )
						{
							// SRS - Convert from planar to packed format keeping sample count the same
							res = swr_convert( swr_ctx, &audioBuffer, frame3->nb_samples, ( const uint8_t** )frame3->extended_data, frame3->nb_samples );
							if( res < 0 || res != frame3->nb_samples )
							{
								common->Warning( "idCinematic: Failed to convert planar audio data to packed format with result: %d\n", res );
							}
						}
						else
						{
							// SRS - Since audio is already in packed format, just copy into audio buffer
							if( num_bytes > 0 )
							{
								memcpy( audioBuffer, frame3->extended_data[0], num_bytes );
							}
						}
					}
				}
			}
			//GK:End
			// Free the packet that was allocated by av_read_frame
			av_packet_unref( &packet );
		}

		framePos++;
	}

	// We have reached the desired frame
	// Convert the image from its native format to RGB
	sws_scale( img_convert_ctx, frame->data, frame->linesize, 0, dec_ctx->height, frame2->data, frame2->linesize );
	cinData.imageWidth = CIN_WIDTH;
	cinData.imageHeight = CIN_HEIGHT;
	cinData.status = status;
	renderImage->UploadScratch( image, CIN_WIDTH, CIN_HEIGHT );
	hasFrame = true;
	cinData.image = image;

	// SRS - If we have cinematic audio data, play a lagged frame (for FFMPEG video sync) and save the current frame
	if( num_bytes > 0 )
	{
		// SRS - If we have a lagged cinematic audio frame, then play it now
		if( lagBufSize[ lagIndex ] > 0 )
		{
			// SRS - Note that PlayAudio() is responsible for releasing any audio buffers sent to it
			cinematicAudio->PlayAudio( lagBuffer[ lagIndex ], lagBufSize[ lagIndex ] );
		}

		// SRS - Save the current (new) audio buffer and its size to play NUM_LAG_FRAMES in the future
		lagBuffer[ lagIndex ] = audioBuffer;
		lagBufSize[ lagIndex ] = num_bytes;

		lagIndex = ( lagIndex + 1 ) % ( skipLag ? 1 : NUM_LAG_FRAMES );
	}

	return cinData;
}
#endif
