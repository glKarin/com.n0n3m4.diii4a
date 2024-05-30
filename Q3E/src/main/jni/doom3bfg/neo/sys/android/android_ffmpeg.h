#ifndef _ANDROID_FFMPEG_H
#define _ANDROID_FFMPEG_H

#if defined(USE_FFMPEG)
#ifdef _DL_FFMPEG

#define av_frame_alloc qav_frame_alloc
#define av_frame_free qav_frame_free
#define avformat_open_input qavformat_open_input
#define avformat_find_stream_info qavformat_find_stream_info
#define av_strerror qav_strerror
#define avcodec_open2 qavcodec_open2
#define avcodec_parameters_to_context qavcodec_parameters_to_context
#define avcodec_alloc_context3 qavcodec_alloc_context3
#define av_find_best_stream qav_find_best_stream
#define swr_alloc_set_opts qswr_alloc_set_opts
#define swr_init qswr_init
#define av_image_fill_arrays qav_image_fill_arrays
#define sws_getContext qsws_getContext
#define av_freep qav_freep
#define av_seek_frame qav_seek_frame
#define avcodec_close qavcodec_close
#define sws_freeContext qsws_freeContext
#define avcodec_free_context qavcodec_free_context
#define swr_free qswr_free
#define avformat_close_input qavformat_close_input
#define avcodec_send_packet qavcodec_send_packet
#define av_packet_unref qav_packet_unref
#define av_read_frame qav_read_frame
#define avcodec_receive_frame qavcodec_receive_frame
#define av_samples_alloc qav_samples_alloc
#define swr_convert qswr_convert
#define sws_scale qsws_scale

#define QFFMPEGPROC(name, rettype, args) extern rettype (* q##name) args;
#include "qffmpeg.h"

extern bool ffmpeg_available;
#define FFMPEG_AVAILABLE() (ffmpeg_available)
#define FFMPEG_IF_AVAILABLE if(FFMPEG_AVAILABLE())
#define FFMPEG_IF_NOT_AVAILABLE(x) if(!FFMPEG_AVAILABLE()) { x }

#else

#define FFMPEG_AVAILABLE() (true)
#define FFMPEG_IF_AVAILABLE
#define FFMPEG_IF_NOT_AVAILABLE(x)

#endif

#endif

#endif //_ANDROID_FFMPEG_H
