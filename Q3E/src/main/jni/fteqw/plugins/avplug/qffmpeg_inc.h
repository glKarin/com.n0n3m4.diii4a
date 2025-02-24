#ifndef _ANDROID_FFMPEG_H
#define _ANDROID_FFMPEG_H

#ifdef _DIII4A //karin: dynamic load ffmpeg

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"

#define av_frame_alloc qav_frame_alloc
#define av_frame_free qav_frame_free
#define avformat_open_input qavformat_open_input
#define avformat_find_stream_info qavformat_find_stream_info
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
#define av_malloc qav_malloc
#define av_log_set_level qav_log_set_level
#define avio_alloc_context qavio_alloc_context
#define av_free qav_free
#define av_log_set_callback qav_log_set_callback
#define avformat_alloc_context qavformat_alloc_context
#define avcodec_find_decoder qavcodec_find_decoder
#define av_samples_get_buffer_size qav_samples_get_buffer_size
#define av_image_alloc qav_image_alloc
#define sws_getCachedContext qsws_getCachedContext
#define avcodec_flush_buffers qavcodec_flush_buffers
#define av_guess_format qav_guess_format
#define avcodec_find_encoder_by_name qavcodec_find_encoder_by_name
#define avcodec_find_encoder qavcodec_find_encoder
#define av_strdup qav_strdup
#define avformat_new_stream qavformat_new_stream
#define av_opt_set qav_opt_set
#define av_get_default_channel_layout qav_get_default_channel_layout
#define av_image_get_buffer_size qav_image_get_buffer_size
//#define av_strerror qav_strerror
#define avcodec_parameters_from_context qavcodec_parameters_from_context
#define av_get_bytes_per_sample qav_get_bytes_per_sample
#define av_dump_format qav_dump_format
#define avio_open qavio_open
#define avformat_write_header qavformat_write_header
#define avcodec_fill_audio_frame qavcodec_fill_audio_frame
#define av_write_trailer qav_write_trailer
#define avio_close qavio_close
#define avformat_free_context qavformat_free_context
#define avcodec_send_frame qavcodec_send_frame
#define av_init_packet qav_init_packet
#define avcodec_receive_packet qavcodec_receive_packet
#define av_packet_rescale_ts qav_packet_rescale_ts
#define av_interleaved_write_frame qav_interleaved_write_frame

#define QFFMPEGPROC(name, rettype, args) extern rettype (* q##name) args;
#include "qffmpeg.h"

#endif

#endif //_ANDROID_FFMPEG_H
