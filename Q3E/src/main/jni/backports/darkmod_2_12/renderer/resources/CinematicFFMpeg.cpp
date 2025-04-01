/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "renderer/resources/CinematicFFMpeg.h"
#include "renderer/resources/Image.h"
#include "renderer/tr_local.h"

#if !defined(__ANDROID__) //karin: without ffmpeg

//note: equals PRIMARYFREQ from sound headers
#define FREQ44K 44100

//when audio clock and video clock deviate from each other by more that this number of seconds
//then audio clock is resynced to video clock (possibly introducing a crack/jump in sound)
static const double SYNC_CLOCKS_TOLERANCE = 0.3;

//maximal allowed error in packet timestamp recorded in video file
//most known case of timestamp jitter is Matroska container, which stores all timestamps as integer number of milliseconds
//audio FIFO for decoded samples would fix jitter up to this tolerance, anything greater would result in sound "cracks"
static const double TIMESTAMP_JITTER_TOLERANCE = 0.01;


idCVar r_cinematic_log("r_cinematic_log", 0, CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, 
	"Dump logs from cinematic into \"log_cinematics.txt\" file."
);
idCVar r_cinematic_log_ffmpeg("r_cinematic_log_ffmpeg", 0, CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, 
	"Dump logs from FFmpeg libraries into \"log_cinematics.txt\" file."
);
idCVar r_cinematic_log_flush("r_cinematic_log_flush", 0, CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, 
	"Flush log file for cinematics: make sure all messages are written on crash."
);

idCVar r_cinematic_checkImmediately("r_cinematic_checkImmediately", 0, CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, 
	"Immediately check that video file can be opened when its material is parsed. "
	"Note: by default (when = 0) it is only checked that file exists."
);


//note: global variables are used only for developer-level logging
static idFile *logFile = NULL;
static double InvClockTicksPerSecond = -1.0;
static const int MAX_LOG_LEN = 1024;

static void LogPostMessage(const char *message) {
	if (!logFile)
		return;
	Sys_EnterCriticalSection(CRITICAL_SECTION_THREE);

	static double StartClockTicks = idLib::sys->GetClockTicks();
	double timestamp = (idLib::sys->GetClockTicks() - StartClockTicks) * InvClockTicksPerSecond;
	int64_t microsecs = int64_t(timestamp * 1e+6);
	logFile->Printf("%4d.%03d.%03d: %s\n",
		int(microsecs / 1000000), int(microsecs / 1000 % 1000), int(microsecs % 1000),
		message
	);

	if (r_cinematic_log_flush.GetBool())
		logFile->Flush();

	Sys_LeaveCriticalSection(CRITICAL_SECTION_THREE);
}

void idCinematicFFMpeg::InitCinematic( void ) {
	InvClockTicksPerSecond = 1.0 / idLib::sys->ClockTicksPerSecond();
	//Note: we cannot init logfile, because we cannot read cvars yet (see constructor)
}

static void InitLogFile() {
	// Create log file (only once) --- called from constructor
	if (!logFile)
		logFile = fileSystem->OpenFileWrite("log_cinematics.txt", "fs_devpath", "");
}

void idCinematicFFMpeg::ShutdownCinematic( void ) {
	if (logFile) {
		fileSystem->CloseFile(logFile);
		logFile = NULL;
	}
}

static void LogVPrintf(const char *format, va_list args) {
	char messageBuf[MAX_LOG_LEN];
	idStr::vsnPrintf(messageBuf, MAX_LOG_LEN, format, args);
	LogPostMessage(messageBuf);
}

static void LogPrintf(const char *format, ...) {
	va_list args;
	va_start(args, format);
	LogVPrintf(format, args);
	va_end(args);
}
//in each idCinematic call, this macro is used to start logging 
static void LogCallStart(const char *format, ...) {
	char formatBuf[MAX_LOG_LEN];
	idStr::snPrintf(formatBuf, MAX_LOG_LEN, "\n\n   ====== %s ======", format);
	va_list args;
	va_start(args, format);
	LogVPrintf(formatBuf, args);
	va_end(args);
}

//for logging various timings:
#define TIMER_START(name) double timerStart_##name = idLib::sys->GetClockTicks()
#define TIMER_END(name) ((idLib::sys->GetClockTicks() - timerStart_##name) * InvClockTicksPerSecond)
#define TIMER_END_LOG(name, description) LogPrintf(description " in %0.3lf ms", TIMER_END(name) * 1000.0);

//making sections for calls
#define CALL_START(...) TIMER_START(CALL); LogCallStart(__VA_ARGS__);
#define CALL_END_LOG() TIMER_END_LOG(CALL, "Call in total:");

void idCinematicFFMpeg::LogCallback(void* avcl, int level, const char *fmt, va_list vl) {
	static const char PREFIX[] = "[FFmpeg]   ";
	static const int PREFIX_LEN = sizeof(PREFIX) - 1;
	char messageBuf[MAX_LOG_LEN];
	strcpy(messageBuf, PREFIX);
	vsnprintf(messageBuf + PREFIX_LEN, MAX_LOG_LEN - PREFIX_LEN, fmt, vl);
	int len = (int)strlen(messageBuf);
	if (messageBuf[len-1] == '\n')
		messageBuf[--len] = 0;
	LogPostMessage(messageBuf);
}


// Custom AVIOContext wrapper to allow loading directly from an idFile
class idCinematicFFMpeg::VFSIOContext
{
private:
	idFile* _file;
	int _bufferSize;

	AVIOContext* _context;

	// Noncopyable
	VFSIOContext(const VFSIOContext&);
	VFSIOContext& operator=(const VFSIOContext&);

public:
	VFSIOContext(idFile* file) :
		_file(file),
		_bufferSize(4096),
		_context(NULL)
	{
		unsigned char* buffer = static_cast<unsigned char*>(ExtLibs::av_malloc(_bufferSize));
		bool noseek = false;
		if (_file->IsCompressed()) {
			common->Warning("Opening video file \"%s\", which is compressed inside PK4.\n  Seeking function is disabled", _file->GetName());
			noseek = true;
		}
		_context = ExtLibs::avio_alloc_context(buffer, _bufferSize, 0, this,
			&VFSIOContext::read, NULL, (noseek ? NULL : &VFSIOContext::seek));
	}

	~VFSIOContext()
	{
		ExtLibs::av_free(_context->buffer);
		ExtLibs::av_free(_context);
	}

	static int read(void* opaque, unsigned char* buf, int buf_size)
	{
		VFSIOContext* self = static_cast<VFSIOContext*>(opaque);

		int bytes = self->_file->Read(buf, buf_size);
		if (bytes == 0 && buf_size > 0)
			return AVERROR_EOF;
		return bytes;
	}

	static int64_t seek(void *opaque, int64_t offset, int whence)
	{
		VFSIOContext* self = static_cast<VFSIOContext*>(opaque);

		switch (whence)
		{
		case AVSEEK_SIZE:
			return self->_file->Length();
		case SEEK_SET:
			return self->_file->Seek(offset, FS_SEEK_SET);
		case SEEK_CUR:
			return self->_file->Seek(offset, FS_SEEK_CUR);
		case SEEK_END:
			return self->_file->Seek(offset, FS_SEEK_END);
		default:
			return AVERROR(EINVAL);
		};
	}

	AVIOContext* getContext()
	{
		return _context;
	}
};


bool idCinematicFFMpeg::IsDecoderOpened() const {
	//note: normally, either all these things are present, or all are absent
	return _file && _customIOContext && _formatContext && _videoDecoderContext && _swScaleContext;
}
bool idCinematicFFMpeg::IsDecoderOpened_Locking() const {
	Sys_EnterCriticalSection(CRITICAL_SECTION_DECODER);
	bool ok = IsDecoderOpened();
	Sys_LeaveCriticalSection(CRITICAL_SECTION_DECODER);
	return ok;
}

bool idCinematicFFMpeg::OpenDecoder() {
	bool ok = _OpenDecoder();
	if (!ok) {
		//partially initialized decoder data -> clean it up
		CloseDecoder();
	}
	return ok;
}

bool idCinematicFFMpeg::_OpenDecoder() {

	//=== Open all the necessary stuff

	_file = fileSystem->OpenFileRead(_path.c_str());
	if (_file == NULL) {
		common->Warning("Couldn't open video file: %s", _path.c_str());
		return false;
	}
	LogPrintf("Opened file %s", _path.c_str());

	if (r_cinematic_log_ffmpeg.GetBool())
		ExtLibs::av_log_set_callback(idCinematicFFMpeg::LogCallback);

	TIMER_START(ctxAlloc);
	// Use libavformat to detect the video type and stream
	_formatContext = ExtLibs::avformat_alloc_context();
	// To use the VFS we need to set up a custom AV I/O context
	_customIOContext = new VFSIOContext(_file);
	_formatContext->pb = _customIOContext->getContext();
	TIMER_END_LOG(ctxAlloc, "AVFormat context allocated");

	TIMER_START(formatOpen);
	AVDictionary *options = NULL;
	//accelerate ExtLibs::avformat_find_stream_info by setting tighter limits
	ExtLibs::av_dict_set_int(&options, "probesize", 1<<20, 0);
	ExtLibs::av_dict_set_int(&options, "analyzeduration", int(1.0 * AV_TIME_BASE), 0);		//ROQ is bound by this one
	bool ok = ExtLibs::avformat_open_input(&_formatContext, NULL, NULL, &options) >= 0;
	ExtLibs::av_dict_free(&options);
	if (!ok) {
		common->Warning("Could not open %s\n", _path.c_str());
		return false;
	}
	TIMER_END_LOG(formatOpen, "AVFormat input opened");

	TIMER_START(findStream);
	if (ExtLibs::avformat_find_stream_info(_formatContext, NULL) < 0) {
		common->Warning("Could not find stream info %s\n", _path.c_str());
		return false;
	}
	TIMER_END_LOG(findStream, "Found stream info");

	// Find the most suitable video stream and open decoder for it
	_videoStreamIndex = OpenBestStreamOfType(AVMEDIA_TYPE_VIDEO, _videoDecoderContext);
	if (_videoStreamIndex < 0) {
		common->Warning("Could not find video stream in %s\n", _path.c_str());
		return false;
	}
	AVStream* videoStream = _formatContext->streams[_videoStreamIndex];
	AVRational videoTBase = _videoDecoderContext->pkt_timebase;
	LogPrintf("Video stream timebase: %d/%d = %0.6lf", videoTBase.num, videoTBase.den, ExtLibs::av_q2d(videoTBase));


	if (_withAudio) {
		// Find the most suitable audio stream and open decoder for it
		_audioStreamIndex = OpenBestStreamOfType(AVMEDIA_TYPE_AUDIO, _audioDecoderContext);
		if (_audioStreamIndex < 0) {
			common->Warning("Could not find audio stream in %s\n", _path.c_str());
			return false;
		}
		AVRational audioTBase = _audioDecoderContext->pkt_timebase;
		LogPrintf("Audio stream timebase: %d/%d = %0.6lf", audioTBase.num, audioTBase.den, ExtLibs::av_q2d(audioTBase));
	}

	TIMER_START(createSwsCtx);
	// Set up the scaling context used to convert the images to RGBA
	_swScaleContext = ExtLibs::sws_getContext(
		_videoDecoderContext->width, _videoDecoderContext->height,
		_videoDecoderContext->pix_fmt,
		_videoDecoderContext->width, _videoDecoderContext->height,
		AV_PIX_FMT_RGBA,
		SWS_BICUBIC, NULL, NULL, NULL
	);
	TIMER_END_LOG(createSwsCtx, "Created swScale context");

	if (_withAudio) {
		TIMER_START(createSwrCtx);
		// Set up the scaling context used to convert the images to RGBA
		_swResampleContext = ExtLibs::swr_alloc_set_opts(NULL,
			(_channels == 2 ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO), AV_SAMPLE_FMT_FLT, FREQ44K,
			_audioDecoderContext->channel_layout, _audioDecoderContext->sample_fmt, _audioDecoderContext->sample_rate,
			0, _customIOContext
		);
		ExtLibs::swr_init(_swResampleContext);
		if (!ExtLibs::swr_is_initialized(_swResampleContext)) {
			common->Warning("Could not initialize audio resample context for %s\n", _path.c_str());
			return false;
		}
		TIMER_END_LOG(createSwrCtx, "Created swResample context");

		//note: we make sure audio storage is allocated
		//but it is not part of decoder's data structures
		//and it is NOT destroyed when decoder is closed
		if (!_audioSamples._fifo) {
			_audioSamples._fifo = ExtLibs::av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, _channels, FREQ44K);
			_audioSamples._startTime = 0.0;
		}
	}

	_tempVideoFrame = ExtLibs::av_frame_alloc();
	if (_withAudio)
		_tempAudioFrame = ExtLibs::av_frame_alloc();

	//=== get some information we might need

	// Some video formats (like the beloved ROQ) don't provider a sane duration value, so let's check
	if (videoStream->duration != AV_NOPTS_VALUE && videoStream->duration >= 0)
		_duration = videoStream->duration * ExtLibs::av_q2d(videoStream->time_base);
	else
		_duration = 100.000; // use a hardcoded value, just like the good old idCinematicLocal

	_frameRate = ExtLibs::av_q2d(_videoDecoderContext->framerate);

	_framebufferWidth = _videoDecoderContext->width;
	_framebufferHeight = _videoDecoderContext->height;

	LogPrintf("Video in stream %d of duration %0.3lf secs", _videoStreamIndex, _duration);
	LogPrintf("Video has resolution %d x %d and framerate %0.2f", _framebufferWidth, _framebufferHeight, _frameRate);

	assert(!_videoPackets.Peek());
	assert(!_audioPackets.Peek());

	return true;
}

void idCinematicFFMpeg::CloseDecoder() {
	TIMER_START(closeAll);

	//TODO: do we actually want to drop packets on restart?...
	while (DropPacket(_videoPackets));
	while (DropPacket(_audioPackets));

	if (_tempVideoFrame)
		ExtLibs::av_frame_free(&_tempVideoFrame);
	_tempVideoFrame = NULL;
	if (_tempAudioFrame)
		ExtLibs::av_frame_free(&_tempAudioFrame);
	_tempAudioFrame = NULL;

	_videoStreamIndex = -1;
	_audioStreamIndex = -1;

	if (_swScaleContext)
		ExtLibs::sws_freeContext(_swScaleContext);
	_swScaleContext = NULL;
	if (_swResampleContext)
		ExtLibs::swr_free(&_swResampleContext);
	_swResampleContext = NULL;

	if (_videoDecoderContext)
		avcodec_free_context(&_videoDecoderContext);

	if (_audioDecoderContext)
		avcodec_free_context(&_audioDecoderContext);

	if (_formatContext)
		ExtLibs::avformat_close_input(&_formatContext);
	_formatContext = NULL;

	if (_customIOContext)
		delete _customIOContext;
	_customIOContext = NULL;

	if (_file)
		fileSystem->CloseFile(_file);
	_file = NULL;

	TIMER_END_LOG(closeAll, "Freed all FFmpeg resources");
}

int idCinematicFFMpeg::OpenBestStreamOfType(AVMediaType type, AVCodecContext* &context) {
	if (context)
		avcodec_free_context(&context);

	TIMER_START(findBestStream);
	int streamIndex = ExtLibs::av_find_best_stream(_formatContext, type, -1, -1, NULL, 0);
	if (streamIndex < 0) {
		common->Warning("Could not find %s stream in input.\n", ExtLibs::av_get_media_type_string(type));
		return -1;
	}
	TIMER_END_LOG(findBestStream, "Found best stream");
	AVStream* st = _formatContext->streams[streamIndex];

	AVCodecID codecId = st->codecpar->codec_id;
	LogPrintf("Stream %d is encoded with codec %d: %s", streamIndex, codecId, ExtLibs::avcodec_get_name(codecId));
	// find decoder for the stream
	TIMER_START(findDecoder);
	AVCodec* dec = ExtLibs::avcodec_find_decoder(codecId);
	if (!dec) {
		common->Warning("Failed to find %s:%s decoder\n", ExtLibs::av_get_media_type_string(type), ExtLibs::avcodec_get_name(codecId));
		return AVERROR(EINVAL);
	}
	TIMER_END_LOG(findDecoder, "Found decoder");

	TIMER_START(openCodec);
	context = avcodec_alloc_context3(dec);
	avcodec_parameters_to_context(context, st->codecpar);
	context->pkt_timebase = st->time_base;

	//note: "0" means "auto" (usually equal to number of CPU cores)
	//this overrides the default value "1"
	context->thread_count = 0;
	//note: this is the default value
	//frame-threading is preferred, slice-threading is used only if codec does not support frame-threading
	//context->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

	AVDictionary *opts = NULL;
	if (ExtLibs::avcodec_open2(context, dec, &opts) < 0) {
		avcodec_free_context(&context);
		common->Warning("Failed to open %s:%s codec\n", ExtLibs::av_get_media_type_string(type), ExtLibs::avcodec_get_name(codecId));
		return -1;
	}
	TIMER_END_LOG(openCodec, "Opened decoder");

	// success
	return streamIndex;
}


bool idCinematicFFMpeg::FetchPacket() {
	while (true) {
		TIMER_START(readPacket);
		AVPacket *newPacket = av_packet_alloc();
		int ret = ExtLibs::av_read_frame(_formatContext, newPacket);
		if (ret < 0) {
			ExtLibs::av_packet_free(&newPacket);
			return false;
		}
		TIMER_END_LOG(readPacket, "Read packet");

		int stream = newPacket->stream_index;
		AVStream* st = _formatContext->streams[stream];
		AVMediaType type = ExtLibs::avcodec_get_type(st->codecpar->codec_id);
		LogPrintf("Packet: stream = %d (%s)  size = %d  DTS = %lld  PTS = %lld  dur = %d",
			newPacket->stream_index, ExtLibs::av_get_media_type_string(type), newPacket->size,
			newPacket->dts, newPacket->pts, newPacket->duration
		);

		if (stream != _videoStreamIndex && stream != _audioStreamIndex) {
			ExtLibs::av_packet_free(&newPacket);
			continue;
		}

		PacketQueue &queue = (stream == _videoStreamIndex ? _videoPackets : _audioPackets);
		PacketNode *packetNode = new PacketNode();
		packetNode->_packet = newPacket;
		packetNode->_next = nullptr;
		queue.Add(packetNode);

		return ret >= 0;
	}
}

idCinematicFFMpeg::PacketNode *idCinematicFFMpeg::GetPacket(PacketQueue &queue) {
	while (!queue.Peek()) {
		bool ok = FetchPacket();
		if (!ok)
			return NULL;
	}
	assert(queue.Peek());

	PacketNode *packetNode = queue.Get();
	return packetNode;
}

bool idCinematicFFMpeg::DropPacket(PacketQueue &queue) {
	if (PacketNode *packetNode = queue.Get()) {
		FreePacket(packetNode);
		return true;
	}
	return false;
}

void idCinematicFFMpeg::FreePacket(PacketNode *packetNode) {
	if (!packetNode) return;
	ExtLibs::av_packet_free(&packetNode->_packet);
	delete packetNode;
}

bool idCinematicFFMpeg::FetchPacket_Locking() {
	Sys_EnterCriticalSection(CRITICAL_SECTION_PACKETS);
	bool res = FetchPacket();
	Sys_LeaveCriticalSection(CRITICAL_SECTION_PACKETS);
	return res;
}

idCinematicFFMpeg::PacketNode *idCinematicFFMpeg::GetPacket_Locking(PacketQueue &queue) {
	Sys_EnterCriticalSection(CRITICAL_SECTION_PACKETS);
	PacketNode *res = GetPacket(queue);
	Sys_LeaveCriticalSection(CRITICAL_SECTION_PACKETS);
	return res;
}

bool idCinematicFFMpeg::FetchFrames(AVMediaType type, double discardTime) {
	if (type == AVMEDIA_TYPE_VIDEO)
		DiscardOldFrames(discardTime);
	else
		DiscardOldSamples(discardTime);

	int framesDecoded = 0;
	do {
		PacketQueue &queue = (type == AVMEDIA_TYPE_VIDEO ? _videoPackets : _audioPackets);
		PacketNode *packetNode = GetPacket_Locking(queue);
		if (!packetNode)
			break;	//end of stream
		const AVPacket &packet = *packetNode->_packet;

		framesDecoded += DecodePacket(type, packet, discardTime);

		FreePacket(packetNode);
	} while (framesDecoded == 0);

	if (framesDecoded == 0) {
		LogPrintf("Flushing codec internal buffer...");
		//try to flush codec's internal buffer
		AVPacket nullPacket;
		memset(&nullPacket, 0, sizeof(nullPacket));
		framesDecoded += DecodePacket(type, nullPacket, discardTime);
	}
		
	return (framesDecoded > 0);
}

int idCinematicFFMpeg::DecodePacket(AVMediaType type, const AVPacket &packet, double discardTime) {
	AVCodecContext *context;
	AVFrame *frame;
	if (type == AVMEDIA_TYPE_VIDEO) {
		context = _videoDecoderContext;
		frame = _tempVideoFrame;
	}
	else {		//AVMEDIA_TYPE_AUDIO
		context = _audioDecoderContext;
		frame = _tempAudioFrame;
	}

	TIMER_START(pktsend);
	int sendRet = avcodec_send_packet(context, &packet);
	if (sendRet == AVERROR_EOF)
		return 0;	//flushing already flushed decoder
	if (sendRet < 0) {
		common->Warning("Error sending %s packet to decoder (%d)\n", ExtLibs::av_get_media_type_string(type), sendRet);
		return 0;
	}
	TIMER_END_LOG(pktsend, "Packet sent");

	int framesDecoded = 0;
	while (1) {
		TIMER_START(recvframe);
		int recvRet = avcodec_receive_frame(context, frame);
		if (recvRet == AVERROR(EAGAIN) || recvRet == AVERROR_EOF)
			break;	//no more frames in this packet / in decoder on flush
		if (recvRet < 0) {
			common->Warning("Error receiving %s frame from decoder (%d)\n", ExtLibs::av_get_media_type_string(type), recvRet);
			break;
		}

		framesDecoded++;
		ProcessDecodedFrame(type, frame, discardTime);
		TIMER_END_LOG(recvframe, "Frame received");
	}

	return framesDecoded;
}

void idCinematicFFMpeg::ProcessDecodedFrame(AVMediaType type, AVFrame *decodedFrame, double discardTime) {
	//determine good timestamp using FFmpeg magic
	int64_t pts = decodedFrame->best_effort_timestamp;
	int streamIdx = (type == AVMEDIA_TYPE_VIDEO ? _videoStreamIndex : _audioStreamIndex);
	double timebase = ExtLibs::av_q2d(_formatContext->streams[streamIdx]->time_base);
	double duration = decodedFrame->pkt_duration * timebase;
	double framestart = pts * timebase;
	framestart += _loopNumber * _loopDuration;

	//note: we discard frames older than current time
	//otherwise a pause in playback may cause a lot of frames to
	//be decoded immediately, eating much memory
	if (framestart + duration < discardTime) {
		LogPrintf("Discarded %s frame with timestamp: %0.3lf + %0.3lf  (sec)", ExtLibs::av_get_media_type_string(type), framestart, duration);
		return;
	}

	if (type == AVMEDIA_TYPE_VIDEO)
		ProcessDecodedVideoFrame(decodedFrame, framestart, duration, _loopDuration == 0.0);
	else
		ProcessDecodedAudioFrame(decodedFrame, framestart, duration);
}

void idCinematicFFMpeg::ProcessDecodedVideoFrame(AVFrame *decodedFrame, double timestamp, double duration, bool first) {
	if (!_videoFrames._dead.Peek()) {
		TIMER_START(allocImage);
		//no free frame buffers, allocate a new one
		DecodedFrame *newFrame = new DecodedFrame();
		newFrame->_image = (byte*)Mem_Alloc16(_framebufferWidth * _framebufferHeight * 4 + 16);
		_videoFrames._dead.Add(newFrame);
		TIMER_END_LOG(allocImage, "Allocated RGBA image buffer");
	}
	assert(_videoFrames._dead.Peek());

	//reuse dead (or use new) frame buffer
	DecodedFrame *frame = _videoFrames._dead.Get();
	_videoFrames._alive.Add(frame);

	TIMER_START(swsScale);
	// Note: AV_PIX_FMT_RGBA format is non-planar
	// So all the colors are stored in interleaved way: R, G, B, A, R, G, ...
	// That's why swscale expects only single destination pointer + stride
	uint8_t* const dstPtr[1] = { frame->_image };
	int lineWidth[1] = { _videoDecoderContext->width * 4 };
	ExtLibs::sws_scale(
		_swScaleContext,
		decodedFrame->data, decodedFrame->linesize, 0, decodedFrame->height,
		dstPtr, lineWidth
	);
	TIMER_END_LOG(swsScale, "Converted to RGBA");

	//save timestamp + duration in frame
	frame->_timestamp = timestamp;
	frame->_duration = duration;
	frame->_first = first;
	//keep track of the total video time (for looping)
	if (_loopNumber == 0 && _loopDuration < timestamp + duration)
		_loopDuration = timestamp + duration;

	LogPrintf("Decoded %sframe with timestamp: %0.3lf + %0.3lf  (sec)", (frame->_first ? "first " : ""), frame->_timestamp, frame->_duration);
}

bool idCinematicFFMpeg::GetFrame(double videoTime, DecodedFrame* &prevFrame, DecodedFrame* &nextFrame, double &ratio) {
	//make sure all the desired frames are in the list
	while (!_videoFrames._alive.Last() || _videoFrames._alive.Last()->_timestamp < videoTime) {
		bool ok = FetchFrames(AVMEDIA_TYPE_VIDEO, videoTime);
		if (!ok)
			return false;
	}

	prevFrame = _videoFrames._alive.Peek();
	assert(prevFrame->_timestamp + prevFrame->_duration >= videoTime);
	nextFrame = prevFrame->_next;
	if (nextFrame) {
		double nextTime = nextFrame->_timestamp, prevTime = prevFrame->_timestamp;
		ratio = (videoTime - prevTime) / (nextTime - prevTime + 1e-9);
	}
	else {
		//no previous frame? (should not happen)
		nextFrame = prevFrame;
		ratio = 1.0;
	}

	return true;
}

void idCinematicFFMpeg::DiscardOldFrames(double videoTime) {
	while (DecodedFrame *frame = _videoFrames._alive.Peek()) {
		//check if the frame ended before specified time
		if (frame->_timestamp + frame->_duration >= videoTime)
			break;
		//move frame to list of old frames (to be reused)
		_videoFrames._dead.Add(_videoFrames._alive.Get());
	}
}

void idCinematicFFMpeg::DestroyAllFrames() {
	DiscardOldFrames(DBL_MAX);
	assert(!_videoFrames._alive.Peek());

	while (DecodedFrame *frame = _videoFrames._dead.Get()) {
		Mem_Free16(frame->_image);
		delete frame;
	}
}

void idCinematicFFMpeg::ProcessDecodedAudioFrame(AVFrame *decodedFrame, double timestamp, double duration) {
	//put sound samples into internal buffer of swResample
	int decodedSamples = ExtLibs::swr_convert_frame(_swResampleContext, NULL, decodedFrame);

	int beforeCnt = ExtLibs::av_audio_fifo_size(_audioSamples._fifo);
	double newStartTime = timestamp - beforeCnt / double(FREQ44K);
	if (fabs(newStartTime - _audioSamples._startTime) > TIMESTAMP_JITTER_TOLERANCE)
		_audioSamples._startTime = newStartTime;

	while (true) {
		static const int BuffSize = FREQ44K / 10;
		float buff[2 * BuffSize];

		TIMER_START(swrResample);
		uint8_t* dstPtr = (uint8_t*)buff;
		int moreSamples = ExtLibs::swr_convert(_swResampleContext, &dstPtr, BuffSize, NULL, 0);
		if (moreSamples == 0)
			break;
		SIMDProcessor->Mul(buff, 32767.0f, buff, _channels * moreSamples);
		TIMER_END_LOG(swrResample, "Converted sound samples");

		int cnt = ExtLibs::av_audio_fifo_write(_audioSamples._fifo, (void**)&dstPtr, moreSamples);
		assert(cnt == moreSamples);
		decodedSamples += moreSamples;
	}

	LogPrintf("Decoded %d samples with timestamp: %0.3lf + %0.3lf  (sec)", decodedSamples, timestamp, duration);
}

bool idCinematicFFMpeg::GetAudioInterval(double videoTime, int samplesCount, float *output) {
	while (true) {
		double fifoEndTime = _audioSamples._startTime + ExtLibs::av_audio_fifo_size(_audioSamples._fifo) / double(FREQ44K);
		double reqEndTime = videoTime + (samplesCount + 3) / double(FREQ44K) + TIMESTAMP_JITTER_TOLERANCE;
		if (fifoEndTime >= reqEndTime)
			break;
		bool ok = FetchFrames(AVMEDIA_TYPE_AUDIO, videoTime);
		if (!ok)
			return false;
	}

	double rngStart = (videoTime - _audioSamples._startTime) * FREQ44K;
	if (rngStart >= 0.0) {
		int drop = int(rngStart + 0.5);
		if (drop > 0) {
			ExtLibs::av_audio_fifo_drain(_audioSamples._fifo, drop);
			_audioSamples._startTime += drop / double(FREQ44K);
			LogPrintf("Dropped %d sound samples", drop);
		}
	}
	else {
		int insert = int(-rngStart + 0.5);
		if (insert > 0) {
			float fill[2] = {0, 0};
			//TODO: use this code with newer FFmpeg
			/*if (ExtLibs::av_audio_fifo_size(_audioSamples._fifo) > 0) {
				float *ptr = fill;
				ExtLibs::av_audio_fifo_peek(_audioSamples._fifo, (void**)&ptr, 1);
			}*/
			//TODO: remove this crap with newer ffmpeg
			assert(ExtLibs::av_audio_fifo_size(_audioSamples._fifo) > 0);
			float *ptr = fill;
			if (ExtLibs::av_audio_fifo_read(_audioSamples._fifo, (void**)&ptr, 1)) {
				_audioSamples._startTime += 1.0 / FREQ44K;
				insert++;
			}

			insert = idMath::Imin(insert, samplesCount);
			for (int i = 0; i < insert; i++)
				for (int j = 0; j < _channels; j++)
					output[_channels * i + j] = fill[j];
			output += _channels * insert;
			samplesCount -= insert;
			LogPrintf("Inserted %d zero sound samples", insert);
		}
		if (samplesCount == 0)
			return true;
	}

	int read = ExtLibs::av_audio_fifo_read(_audioSamples._fifo, (void**)&output, samplesCount);
	assert(read == samplesCount);
	_audioSamples._startTime += samplesCount / double(FREQ44K);

	return true;
}

void idCinematicFFMpeg::DiscardOldSamples(double videoTime) {
	int drop = int((videoTime - _audioSamples._startTime - TIMESTAMP_JITTER_TOLERANCE) * FREQ44K);
	drop = idMath::Imin(drop, ExtLibs::av_audio_fifo_size(_audioSamples._fifo));
	if (drop > 0) {
		ExtLibs::av_audio_fifo_drain(_audioSamples._fifo, drop);
		_audioSamples._startTime += drop / double(FREQ44K);
	}
}

void idCinematicFFMpeg::DestroyAllSamples() {
	if (_audioSamples._fifo)
		ExtLibs::av_audio_fifo_free(_audioSamples._fifo);
	_audioSamples._fifo = NULL;
	_audioSamples._startTime = DBL_MAX;
}


idCinematicFFMpeg::idCinematicFFMpeg() :
	_looping(false),
	_withAudio(false),
	_duration(-1),
	_frameRate(-1),
	_framebufferWidth(-1),
	_framebufferHeight(-1),
	_channels(-1),
	_startTime(-1),
	_loopNumber(-1),
	_loopDuration(-1),
	_finished(false),
	_file(NULL),
	_customIOContext(NULL),
	_formatContext(NULL),
	_videoDecoderContext(NULL),
	_audioDecoderContext(NULL),
	_swScaleContext(NULL),
	_swResampleContext(NULL),
	_videoStreamIndex(-1),
	_audioStreamIndex(-1),
	_tempVideoFrame(NULL),
	_tempAudioFrame(NULL),
	_soundTimeOffset(DBL_MAX),
	_lastVideoTime(DBL_MAX)
{
	if (r_cinematic_log.GetBool() || r_cinematic_log_ffmpeg.GetBool())
		InitLogFile();
}

idCinematicFFMpeg::~idCinematicFFMpeg() {
	Close();
}

void idCinematicFFMpeg::Close() {
	CALL_START("Close");
	Sys_EnterCriticalSection(CRITICAL_SECTION_DECODER);

	CloseDecoder();
	DestroyAllFrames();
	DestroyAllSamples();
	_finished = false;

	Sys_LeaveCriticalSection(CRITICAL_SECTION_DECODER);
	CALL_END_LOG();
}

bool idCinematicFFMpeg::InitFromFile(const char *qpath, bool looping, bool withAudio) {
	CALL_START("InitFromFile(%s, %d, %d)", qpath, int(looping), int(withAudio));

	_path = qpath;
	_looping = looping;
	_withAudio = withAudio;
	_channels = 2;

	bool res = false;
	if (r_cinematic_checkImmediately.GetBool()) {
		res = OpenDecoder();
		CloseDecoder();
		LogPrintf("Fully checked video file: it is %s", (res ? "OK" : "BAD"));
	}
	else {
		_file = fileSystem->OpenFileRead(_path.c_str());
		if (_file != NULL) {
			res = true;
			fileSystem->CloseFile(_file);
			_file = NULL;
		}
		LogPrintf("Checked file presence: it is %s", (res ? "OK" : "BAD"));
	}

	CALL_END_LOG();
	return res;
}

const char *idCinematicFFMpeg::GetFilePath() const {
	return _path.c_str();
}

int idCinematicFFMpeg::AnimationLength() {
	return int(_duration * 1000.0);
}

cinData_t idCinematicFFMpeg::ImageForTime(int milliseconds) {
	CALL_START("ImageForTime(%d)", milliseconds);

	cinData_t res;
	memset(&res, 0, sizeof(res));
	res.imageWidth = _framebufferWidth;
	res.imageHeight = _framebufferHeight;
	res.status = FMV_EOF;

	if (!IsDecoderOpened_Locking()) {
		//never called ImageForTime or ResetTime before
		//so we open decoder and start video right now
		Sys_EnterCriticalSection(CRITICAL_SECTION_DECODER);
		bool ok = OpenDecoder();
		res.imageWidth = _framebufferWidth;
		res.imageHeight = _framebufferHeight;
		_loopNumber = 0;
		_loopDuration = 0;
		_startTime = 0.001 * milliseconds;
		_soundTimeOffset = DBL_MAX;
		_finished = false;
		Sys_LeaveCriticalSection(CRITICAL_SECTION_DECODER);
		if (!ok)
			return res;
		LogPrintf("Start time was not set, set it to now (%0.3lf sec)", _startTime);
	}
	double videoTime = 0.001 * milliseconds - _startTime;
	LogPrintf("Video time: %0.3lf sec", videoTime);

	Sys_EnterCriticalSection(CRITICAL_SECTION_CLOCK);
	_lastVideoTime = videoTime;
	Sys_LeaveCriticalSection(CRITICAL_SECTION_CLOCK);

	DiscardOldFrames(videoTime);
	DecodedFrame *prevFrame = NULL, *nextFrame = NULL;
	double ratio = 0.0;
	bool ok = GetFrame(videoTime, prevFrame, nextFrame, ratio);

	if (!ok) {
		LogPrintf("Video ended: no more frames");
		if (_looping && IncrementLap(videoTime))
			ok = GetFrame(videoTime, prevFrame, nextFrame, ratio);
	}

	if (ok) {
		res.status = FMV_PLAY;
		res.image = ratio < 0.5 ? prevFrame->_image : nextFrame->_image;
	}
	else
		_finished = true;

	CALL_END_LOG();
	return res;
}

bool idCinematicFFMpeg::IncrementLap(double videoTime) {
	//note: looping with both video & sound is hard to do reliably,
	//because either main or sound thread may get to EOF first...
	assert(!_withAudio);
	if (_loopDuration <= 0.001)
		return false;

	//note: no critical section is necessary, because without audio a cinematic is single-threaded
	CloseDecoder();
	do {
		_loopNumber++;
	} while (_loopDuration * (_loopNumber + 1) < videoTime);
	bool ok = OpenDecoder();
	LogPrintf("Reopened decoder for the %d-th lap (%s)", _loopNumber, (ok ? "success" : "FAIL"));

	return ok;
}

int idCinematicFFMpeg::GetRealSoundOffset(int sampleOffset44k) const {
	int soundTimeOffset44k = 0;
	if (_soundTimeOffset < DBL_MAX)
		soundTimeOffset44k = int(FREQ44K * _soundTimeOffset);
	return sampleOffset44k + soundTimeOffset44k;
}

bool idCinematicFFMpeg::SoundForTimeInterval(int sampleOffset44k, int *sampleSize, float *output) {
	CALL_START("SoundForTimeInterval(%d + %d)", sampleOffset44k, *sampleSize);
	if (!_withAudio)
		return false;
	if (!IsDecoderOpened_Locking())
		return false;
	int count = *sampleSize;
	double soundTime = sampleOffset44k / double(FREQ44K);

	/*for (int i = 0; i < count; i++)
		output[i] = 32767.0f * (float) sin( double(sampleOffset + i) / FREQ44K * M_PI * 2 * 440.0 );*/

	Sys_EnterCriticalSection(CRITICAL_SECTION_CLOCK);
	double lastVideoTime = _lastVideoTime;
	Sys_LeaveCriticalSection(CRITICAL_SECTION_CLOCK);

	double gap = lastVideoTime - (_soundTimeOffset + soundTime);
	if (fabs(gap) > SYNC_CLOCKS_TOLERANCE || _soundTimeOffset == DBL_MAX) {
		double old = _soundTimeOffset;
		_soundTimeOffset = lastVideoTime - soundTime;
		LogPrintf("Changed sound offset from %0.3lf to %0.3lf", old, _soundTimeOffset);
	}

	double videoTime = _soundTimeOffset + soundTime;
	LogPrintf("Time: sound = %0.3lf  video = %0.3lf", soundTime, videoTime);
	DiscardOldSamples(videoTime);
	bool ok = GetAudioInterval(videoTime, count, output);
	if (!ok)
		LogPrintf("Audio ended: no more frames");

	CALL_END_LOG();
	return ok;
}

cinStatus_t idCinematicFFMpeg::GetStatus() const {
	if (_finished)
		return FMV_EOF;
	if (IsDecoderOpened_Locking())
		return FMV_PLAY;
	return FMV_IDLE;
}

//note: the argument here is some stupid joke =)
//it has weird value and it is not used in original ROQ code...
void idCinematicFFMpeg::ResetTime(int someBogusTime) {
	CALL_START("ResetTime(%d)", someBogusTime);
	Sys_EnterCriticalSection(CRITICAL_SECTION_DECODER);

	_ResetTime();

	Sys_LeaveCriticalSection(CRITICAL_SECTION_DECODER);
	CALL_END_LOG();
}
void idCinematicFFMpeg::_ResetTime() {
	//note: we set start time to renderer's backend time
	//because it is always passed as argument to ImageForTime method
	if (backEnd.viewDef == NULL)
		return; //this call is non-sense

	//check if are already at the very beginning
	DecodedFrame *firstFrame = _videoFrames._alive.Peek();
	bool noReset = (IsDecoderOpened() && firstFrame && firstFrame->_first);

	_startTime = double(backEnd.viewDef->floatTime);
	_soundTimeOffset = DBL_MAX;
	_finished = false;
	LogPrintf("Backend time: %0.3lf sec", _startTime);

	//note: already locked from outside
	//Sys_EnterCriticalSection(CRITICAL_SECTION_CLOCK);
	_lastVideoTime = 0.0;
	//Sys_LeaveCriticalSection(CRITICAL_SECTION_CLOCK);


	if (noReset)
		LogPrintf("Decoder already at the beginning");
	else {
		_loopNumber = 0;
		_loopDuration = 0;

		// discard any decoded data we have
		DiscardOldFrames(DBL_MAX);
		DestroyAllSamples();

		// Just re-init the whole stuff, don't bother seeking
		// as it doesn't seem to work reliably with ROQs.
		if (IsDecoderOpened())
			CloseDecoder();
		OpenDecoder();

		LogPrintf("Purged all data and reopened decoder");
	}
}
#endif