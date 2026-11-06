#ifndef _DL_FFMPEG_H
#define _DL_FFMPEG_H

#ifdef DL_FFMPEG

#define av_frame_alloc qav_frame_alloc
#define av_frame_free qav_frame_free
#define avformat_open_input qavformat_open_input
#define avformat_find_stream_info qavformat_find_stream_info
#define av_strerror qav_strerror
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

#define avcodec_open2 qavcodec_open2
#define swr_get_delay qswr_get_delay
#define av_malloc qav_malloc
#define av_packet_alloc qav_packet_alloc
#define av_guess_sample_aspect_ratio qav_guess_sample_aspect_ratio
#define av_packet_free qav_packet_free
#define avformat_open_input qavformat_open_input
#define avformat_alloc_context qavformat_alloc_context
#define avio_alloc_context qavio_alloc_context
#define av_rescale_rnd qav_rescale_rnd
#define avcodec_find_decoder qavcodec_find_decoder
#define avio_context_free qavio_context_free
#define av_channel_layout_uninit qav_channel_layout_uninit
#define swr_alloc_set_opts2 qswr_alloc_set_opts2
#define av_channel_layout_copy qav_channel_layout_copy
#define av_channel_layout_default qav_channel_layout_default
#define av_frame_get_buffer qav_frame_get_buffer

#define QFFMPEGPROC(name, rettype, args) extern rettype (* q##name) args;
#include "qffmpeg.h"

#endif

#endif //_DL_FFMPEG_H
