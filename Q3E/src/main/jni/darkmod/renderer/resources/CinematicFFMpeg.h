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

#pragma once

#include "renderer/resources/Cinematic.h"

#ifdef __ANDROID__ //karin: without ffmpeg
#include "CinematicID.h"
class idCinematicFFMpeg : public idCinematicLocal
{
public:
	static void InitCinematic() {}
	static void ShutdownCinematic() {}
};
#else
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libavutil/audio_fifo.h>
}

/**
 * Cinematic driven by the ffmpeg libavcodec library.
 */
class idCinematicFFMpeg : public idCinematic
{
public:
	idCinematicFFMpeg();
	virtual					~idCinematicFFMpeg() override;

	static void			InitCinematic(void);
	static void			ShutdownCinematic(void);

	virtual bool			InitFromFile(const char *qpath, bool looping, bool withAudio) override;
	virtual int				AnimationLength() override;
	virtual cinData_t		ImageForTime(int milliseconds) override;
	virtual bool SoundForTimeInterval(int sampleOffset44k, int *sampleSize, float *output) override;
	virtual int GetRealSoundOffset(int sampleOffset44k) const override;
	virtual cinStatus_t GetStatus() const override;
	virtual const char *GetFilePath() const override;


	virtual void			ResetTime(int time) override;
	virtual void			Close() override;

private: // methods
	void _ResetTime();

	// Logging callback used by FFmpeg library
	static void LogCallback(void* avcl, int level, const char *fmt, va_list vl);

	// initializes and allocates all decoder data (see section below)
	// this includes: file handle, AV libs contexts, packets queue
	// note: also called when resetting time
	bool OpenDecoder();
	// (internal implementation)
	bool _OpenDecoder();
	// frees and deallocates all decoder data
	void CloseDecoder();
	// is decoder properly opened now?
	bool IsDecoderOpened() const;
	bool IsDecoderOpened_Locking() const;


	// finds and returns the index of best suitable stream of given type
	// opens this stream with appropriate decoder (codec)
	// newly allocated decoder is stored to the context pointer
	// note: format context must be already opened
	int OpenBestStreamOfType(AVMediaType type, AVCodecContext* &context);

	// increments current lap (loop) number
	// reopens decoder to repeat video from the beginning
	bool IncrementLap(double videoTime);

private: // members

	static const int CRITICAL_SECTION_PACKETS = CRITICAL_SECTION_ONE;
	static const int CRITICAL_SECTION_CLOCK = CRITICAL_SECTION_ONE;
	static const int CRITICAL_SECTION_DECODER = CRITICAL_SECTION_ONE;

	//=== input parameters:

	// The path of the cinematic (VFS path)
	idStr _path;
	// Whether this cinematic is supposed to loop
	bool _looping;
	// Whether we have to decode and playback audio from the file
	bool _withAudio;

	//=== some constant info (get once per video)

	// duration of this cinematic in seconds
	double _duration;
	// The frame rate this cinematic was encoded in (frames per sec)
	double _frameRate;
	// size of the RGBA buffer in decoded frames
	int _framebufferWidth;
	int _framebufferHeight;
	// how many audio channels we produce as output (currently it is 2 = stereo)
	int _channels;

	//=== video clock

	// renderer's backend time when the video started (in sec)
	double _startTime;

	// at what moment (in video time - relative to video start) first sample was decoded (in sec)
	double _soundTimeOffset;

	// last time video was decoded for this time (relative to video start, in sec)
	double _lastVideoTime;

	// how many full laps have already been played
	int _loopNumber;
	// the highest calculated frame ending time of all time
	// time is determined by video packets, so it does not take previous laps into account
	// this value stops changing after single full lap, and it equals loop duration afterwards
	double _loopDuration;

	//set to true when video has ended
	bool _finished;

	//=== decoder data (including AV libs)

	// The virtual file we're streaming from
	idFile* _file;
	// file reading wrapper for AV libs
	class VFSIOContext;
	VFSIOContext* _customIOContext;

	// context for demuxing container (avi/mp4/whatever)
	AVFormatContext* _formatContext;
	// context for decoding video frames (xvid/mpeg2/whatever)
	AVCodecContext* _videoDecoderContext;
	// context for decoding audio samples (mp3/ogg/whatever)
	AVCodecContext* _audioDecoderContext;
	// context for scaling and color space conversions (e.g. YUV -> RGB)
	SwsContext* _swScaleContext;
	// context for sound samples conversion
	SwrContext* _swResampleContext;

	// we are decoding video from this stream
	int _videoStreamIndex;
	// we are decoding audio from this stream (-1 if none)
	int _audioStreamIndex;

	// single frame used for intermediate storage of video decoder
	AVFrame *_tempVideoFrame;
	AVFrame *_tempAudioFrame;

	struct PacketNode {
		// the actual packet in this node
		AVPacket *_packet;
		//packets comprise a linked list
		PacketNode* _next;
	};
	// queues of packets fetched from file but not yet decoded
	typedef idQueue(PacketNode, _next) PacketQueue;
	PacketQueue _videoPackets;
	PacketQueue _audioPackets;

	// read one packet from file, put it into proper queue
	bool FetchPacket();
	bool FetchPacket_Locking();
	// returns first packet from packet queue, and removes it from queue
	// if no packets are available, fetches one from file automatically
	PacketNode *GetPacket(PacketQueue &queue);
	PacketNode *GetPacket_Locking(PacketQueue &queue);
	// kill first packet in the specified queue
	bool DropPacket(PacketQueue &queue);
	// free specified packet node (internal)
	void FreePacket(PacketNode *packetNode);

	//=== decoding: general

	// decode at least one video/audio frame, reading packets from queue
	// returns false if there are no more video packets to decode
	// note: decoded frame may be discarded immediately, if it ends earlier than specified time
	bool FetchFrames(AVMediaType type, double discardTime);
	// decode exactly one packet from queue (may not produce a frame immediately)
	// returns number of frames decoded
	int DecodePacket(AVMediaType type, const AVPacket &packet, double discardTime);
	// process a given video/audio frame just decoded
	void ProcessDecodedFrame(AVMediaType type, AVFrame *decodedFrame, double discardTime);

	//=== decoded video frames

	//single completely decoded frame
	struct DecodedFrame {
		// true only for the very first frame in video
		bool _first;
		// video time in seconds
		double _timestamp;
		// frame duration in seconds
		double _duration;
		// byte array holding RGBA image
		byte *_image;

		//frames belong to linked list
		DecodedFrame *_next;

		DecodedFrame() : _first(false), _timestamp(DBL_MAX), _duration(-DBL_MAX), _image(NULL), _next(NULL) {}
	};

	struct FramesStorage {
		//all decoded frames which are not yet obsolete
		idQueue(DecodedFrame, _next) _alive;
		//obsolete frames: they can be reused without reallocation
		idStack(DecodedFrame, _next) _dead;
	};
	FramesStorage _videoFrames;

	// process a given video frame just decoded:
	// convert it to RGBA and put into frames storage
	void ProcessDecodedVideoFrame(AVFrame *decodedFrame, double timestamp, double duration, bool first);
	// return two consecutive frames: prev happens before specified time, next happens after specified time
	// also returns interpolation ratio in [0, 1]
	// note: fetches frames automatically if necessary
	bool GetFrame(double videoTime, DecodedFrame* &prevFrame, DecodedFrame* &nextFrame, double &ratio);
	// remove all frames with timestamp older that the specified time
	void DiscardOldFrames(double videoTime);
	// frees all memory taken by frames
	void DestroyAllFrames();

	//=== decoded audio samples

	struct SamplesStorage {
		AVAudioFifo *_fifo;
		double _startTime;

		SamplesStorage() : _fifo(NULL), _startTime(DBL_MAX) {}
	};
	SamplesStorage _audioSamples;

	// process a given audio frame just decoded:
	// convert it to 44K frequency with predefined channel layout
	// and put into samples storage
	void ProcessDecodedAudioFrame(AVFrame *decodedFrame, double timestamp, double duration);
	// return decoded samples in the given time interval (video clock time + samples count)
	// note: fetches audio frames automatically if necessary
	bool GetAudioInterval(double startTime, int samplesCount, float *output);
	// remove all frames with timestamp older that the specified time
	void DiscardOldSamples(double videoTime);
	// free memory taken by sound samples
	void DestroyAllSamples();

};
#endif