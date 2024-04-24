/*
Copyright (C) 2020 David Knapp (Cloudwalk)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "darkplaces.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

// Decoding functions
AVCodec                    *(*qavcodec_find_decoder)                            (enum AVCodecID id);
AVCodec                    *(*qavcodec_find_decoder_by_name)                    (const char *name);
int                         (*qavcodec_default_get_buffer2)                     (AVCodecContext *s, AVFrame *frame, int flags);
void                        (*qavcodec_align_dimensions)                        (AVCodecContext *s, int *width, int *height);
void                        (*qavcodec_align_dimensions2)                       (AVCodecContext *s, int *width, int *height, int linesize_align[AV_NUM_DATA_POINTERS]);
int                         (*qavcodec_enum_to_chroma_pos)                      (int *xpos, int *ypos, enum AVChromaLocation pos);
enum AVChromaLocation       (*qavcodec_chroma_pos_to_enum)                      (int xpos, int ypos);
int                         (*qavcodec_send_packet)                             (AVCodecContext *avctx, const AVPacket *avpkt);
int                         (*qavcodec_receive_frame)                           (AVCodecContext *avctx, AVFrame *frame);
int                         (*qavcodec_send_frame)                              (AVCodecContext *avctx, const AVFrame *frame);
int                         (*qavcodec_receive_packet)                          (AVCodecContext *avctx, AVPacket *avpkt);
int                         (*qavcodec_get_hw_frames_parameters)                (AVCodecContext *avctx, AVBufferRef *device_ref, enum AVPixelFormat hw_pix_fmt, AVBufferRef **out_frames_ref);

// Encoding functions
AVCodec                    *(*qavcodec_find_encoder)                            (enum AVCodecID id);
AVCodec                    *(*qavcodec_find_encoder_by_name)                    (const char *name);
int                         (*qavcodec_encode_subtitle)                         (AVCodecContext *avctx, uint8_t *buf, int buf_size, const AVSubtitle *sub);

// Core functions
const AVCodecHWConfig      *(*qavcodec_get_hw_config)                           (const AVCodec *codec, int index);
const AVCodec              *(*qav_codec_iterate)                                (void **opaque);
unsigned                    (*qavcodec_version)                                 (void);
const char                 *(*qavcodec_configuration)                           (void);
const char                 *(*qavcodec_license)                                 (void);
AVCodecContext             *(*qavcodec_alloc_context3)                          (const AVCodec *codec);
void                        (*qavcodec_free_context)                            (AVCodecContext **avctx);
int                         (*qavcodec_get_context_defaults3)                   (AVCodecContext *s, const AVCodec *codec);
const AVClass              *(*qavcodec_get_class)                               (void);
const AVClass              *(*qavcodec_get_frame_class)                         (void);
const AVClass              *(*qavcodec_get_subtitle_rect_class)                 (void);
AVCodecParameters          *(*qavcodec_parameters_alloc)                        (void);
void                        (*qavcodec_parameters_free)                         (AVCodecParameters **par);
int                         (*qavcodec_parameters_copy)                         (AVCodecParameters *dst, const AVCodecParameters *src);
int                         (*qavcodec_parameters_from_context)                 (AVCodecParameters *par, const AVCodecContext *codec);
int                         (*qavcodec_parameters_to_context)                   (AVCodecContext *codec, const AVCodecParameters *par);
int                         (*qavcodec_open2)                                   (AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options);
int                         (*qavcodec_close)                                   (AVCodecContext *avctx);
void                        (*qavsubtitle_free)                                 (AVSubtitle *sub);

// Demuxing functions
AVInputFormat              *(*qav_find_input_format)                            (const char *short_name);
AVInputFormat              *(*qav_probe_input_format)                           (AVProbeData *pd, int is_opened);
AVInputFormat              *(*qav_probe_input_format2)                          (AVProbeData *pd, int is_opened, int *score_max);
AVInputFormat              *(*qav_probe_input_format3)                          (AVProbeData *pd, int is_opened, int *score_ret);
int                         (*qav_probe_input_buffer2)                          (AVIOContext *pb, AVInputFormat **fmt, const char *url, void *logctx, unsigned int offset, unsigned int max_probe_size);
int                         (*qav_probe_input_buffer)                           (AVIOContext *pb, AVInputFormat **fmt, const char *url, void *logctx, unsigned int offset, unsigned int max_probe_size);
int                         (*qavformat_open_input)                             (AVFormatContext **ps, const char *url, AVInputFormat *fmt, AVDictionary **options);
int                         (*qavformat_find_stream_info)                       (AVFormatContext *ic, AVDictionary **options);
AVProgram                  *(*qav_find_program_from_stream)                     (AVFormatContext *ic, AVProgram *last, int s);
void                        (*qav_program_add_stream_index)                     (AVFormatContext *ac, int progid, unsigned int idx);
int                         (*qav_find_best_stream)                             (AVFormatContext *ic, enum AVMediaType type, int wanted_stream_nb, int related_stream, AVCodec **decoder_ret, int flags);
int                         (*qav_read_frame)                                   (AVFormatContext *s, AVPacket *pkt);
int                         (*qav_seek_frame)                                   (AVFormatContext *s, int stream_index, int64_t timestamp, int flags);
int                         (*qavformat_seek_file)                              (AVFormatContext *s, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags);
int                         (*qavformat_flush)                                  (AVFormatContext *s);
int                         (*qav_read_play)                                    (AVFormatContext *s);
int                         (*qav_read_pause)                                   (AVFormatContext *s);
void                        (*qavformat_close_input)                            (AVFormatContext **s);

// Muxing functions
av_warn_unused_result int   (*qavformat_write_header)                           (AVFormatContext *s, AVDictionary **options);
av_warn_unused_result int   (*qavformat_init_output)                            (AVFormatContext *s, AVDictionary **options);
int                         (*qav_write_frame)                                  (AVFormatContext *s, AVPacket *pkt);
int                         (*qav_interleaved_write_frame)                      (AVFormatContext *s, AVPacket *pkt);
int                         (*qav_write_uncoded_frame)                          (AVFormatContext *s, int stream_index, AVFrame *frame);
int                         (*qav_interleaved_write_uncoded_frame)              (AVFormatContext *s, int stream_index, AVFrame *frame);
int                         (*qav_write_uncoded_frame_query)                    (AVFormatContext *s, int stream_index);
int                         (*qav_write_trailer)                                (AVFormatContext *s);
AVOutputFormat             *(*qav_guess_format)                                 (const char *short_name, const char *filename, const char *mime_type);
enum AVCodecID              (*qav_guess_codec)                                  (AVOutputFormat *fmt, const char *short_name, const char *filename, const char *mime_type, enum AVMediaType type);
int                         (*qav_get_output_timestamp)                         (struct AVFormatContext *s, int stream, int64_t *dts, int64_t *wall);

// Core functions
unsigned                    (*qavformat_version)                                (void);
const char                 *(*qavformat_configuration)                          (void);
const char                 *(*qavformat_license)                                (void);
void                        (*qav_register_all)                                 (void);
void                        (*qav_register_input_format)                        (AVInputFormat *format);
void                        (*qav_register_output_format)                       (AVOutputFormat *format);
int                         (*qavformat_network_init)                           (void);
int                         (*qavformat_network_deinit)                         (void);
AVInputFormat              *(*qav_iformat_next)                                 (const AVInputFormat *f);
AVOutputFormat             *(*qav_oformat_next)                                 (const AVOutputFormat *f);
AVFormatContext            *(*qavformat_alloc_context)                          (void);
void                        (*qavformat_free_context)                           (AVFormatContext *s);
const AVClass              *(*qavformat_get_class)                              (void);
AVStream                   *(*qavformat_new_stream)                             (AVFormatContext *s, const AVCodec *c);
uint8_t                    *(*qav_stream_new_side_data)                         (AVStream *stream, enum AVPacketSideDataType type, int size);
uint8_t                    *(*qav_stream_get_side_data)                         (const AVStream *stream, enum AVPacketSideDataType type, int *size);
AVProgram                  *(*qav_new_program)                                  (AVFormatContext *s, int id);

// Utility functions
void                        (*qav_hex_dump)                                     (FILE *f, const uint8_t *buf, int size);
void                        (*qav_hex_dump_log)                                 (void *avcl, int level, const uint8_t *buf, int size);
void                        (*qav_pkt_dump2)                                    (FILE *f, const AVPacket *pkt, int dump_payload, const AVStream *st);
void                        (*qav_pkt_dump_log2)                                (void *avcl, int level, const AVPacket *pkt, int dump_payload, const AVStream *st);
enum AVCodecID              (*qav_codec_get_id)                                 (const struct AVCodecTag *const *tags, unsigned int tag);
unsigned int                (*qav_codec_get_tag)                                (const struct AVCodecTag *const *tags, enum AVCodecID id);
int                         (*qav_codec_get_tag2)                               (const struct AVCodecTag *const *tags, enum AVCodecID id, unsigned int *tag);
int                         (*qav_find_default_stream_index)                    (AVFormatContext *s);
int                         (*qav_index_search_timestamp)                       (AVStream *st, int64_t timestamp, int flags);
int                         (*qav_add_index_entry)                              (AVStream *st, int64_t pos, int64_t timestamp, int size, int distance, int flags);
void                        (*qav_url_split)                                    (char *proto, int proto_size, char *authorization, int authorization_size, char *hostname, int hostname_size, int *port_ptr, char *path, int path_size, const char *url);
void                        (*qav_dump_format)                                  (AVFormatContext *ic, int index, const char *url, int is_output);
int                         (*qav_get_frame_filename2)                          (char *buf, int buf_size, const char *path, int number, int flags);
int                         (*qav_get_frame_filename)                           (char *buf, int buf_size, const char *path, int number);
int                         (*qav_filename_number_test)                         (const char *filename);
int                         (*qav_sdp_create)                                   (AVFormatContext *ac[], int n_files, char *buf, int size);
int                         (*qav_match_ext)                                    (const char *filename, const char *extensions);
int                         (*qavformat_query_codec)                            (const AVOutputFormat *ofmt, enum AVCodecID codec_id, int std_compliance);
AVRational                  (*qav_guess_sample_aspect_ratio)                    (AVFormatContext *format, AVStream *stream, AVFrame *frame);
AVRational                  (*qav_guess_frame_rate)                             (AVFormatContext *ctx, AVStream *stream, AVFrame *frame);
int                         (*qavformat_match_stream_specifier)                 (AVFormatContext *s, AVStream *st, const char *spec);
int                         (*qavformat_queue_attached_pictures)                (AVFormatContext *s);
int                         (*qavformat_transfer_internal_stream_timing_info)   (const AVOutputFormat *ofmt, AVStream *ost, const AVStream *ist, enum AVTimebaseSource copy_tb);
AVRational                  (*qav_stream_get_codec_timebase)                    (const AVStream *st);

static dllfunction_t libavcodecfuncs[] =
{
	{"avcodec_find_decoder",                             (void **) &qavcodec_find_decoder},
	{"avcodec_find_decoder_by_name",                     (void **) &qavcodec_find_decoder_by_name},
	{"avcodec_default_get_buffer2",                      (void **) &qavcodec_default_get_buffer2},
	{"avcodec_align_dimensions",                         (void **) &qavcodec_align_dimensions},
	{"avcodec_align_dimensions2",                        (void **) &qavcodec_align_dimensions},
	{"avcodec_enum_to_chroma_pos",                       (void **) &qavcodec_enum_to_chroma_pos},
	{"avcodec_chroma_pos_to_enum",                       (void **) &qavcodec_chroma_pos_to_enum},
	{"avcodec_send_packet",                              (void **) &qavcodec_send_packet},
	{"avcodec_receive_frame",                            (void **) &qavcodec_receive_frame},
	{"avcodec_send_frame",                               (void **) &qavcodec_send_frame},
	{"avcodec_receive_packet",                           (void **) &qavcodec_receive_packet},
	{"avcodec_get_hw_frames_parameters",                 (void **) &qavcodec_get_hw_frames_parameters},
	{"avcodec_get_hw_config",                            (void **) &qavcodec_get_hw_config},
	{"av_codec_iterate",                                 (void **) &qav_codec_iterate},
	{"avcodec_version",                                  (void **) &qavcodec_version},
	{"avcodec_configuration",                            (void **) &qavcodec_configuration},
	{"avcodec_license",                                  (void **) &qavcodec_license},
	{"avcodec_alloc_context3",                           (void **) &qavcodec_alloc_context3},
	{"avcodec_free_context",                             (void **) &qavcodec_free_context},
	{"avcodec_get_context_defaults3",                    (void **) &qavcodec_get_context_defaults3},
	{"avcodec_get_class",                                (void **) &qavcodec_get_class},
	{"avcodec_get_frame_class",                          (void **) &qavcodec_get_frame_class},
	{"avcodec_get_subtitle_rect_class",                  (void **) &qavcodec_get_subtitle_rect_class},
	{"avcodec_parameters_alloc",                         (void **) &qavcodec_parameters_alloc},
	{"avcodec_parameters_free",                          (void **) &qavcodec_parameters_free},
	{"avcodec_parameters_copy",                          (void **) &qavcodec_parameters_copy},
	{"avcodec_parameters_from_context",                  (void **) &qavcodec_parameters_from_context},
	{"avcodec_parameters_to_context",                    (void **) &qavcodec_parameters_to_context},
	{"avcodec_open2",                                    (void **) &qavcodec_open2},
	{"avcodec_close",                                    (void **) &qavcodec_close},
	{"avsubtitle_free",                                  (void **) &qavsubtitle_free},
	{NULL, NULL}
};

static dllfunction_t libavformatfuncs[] =
{
	{"av_find_input_format",                             (void **) &qav_find_input_format},
	{"av_probe_input_format",                            (void **) &qav_probe_input_format},
	{"av_probe_input_format2",                           (void **) &qav_probe_input_format2},
	{"av_probe_input_format3",                           (void **) &qav_probe_input_format3},
	{"av_probe_input_buffer2",                           (void **) &qav_probe_input_buffer2},
	{"av_probe_input_buffer",                            (void **) &qav_probe_input_buffer},
	{"avformat_open_input",                              (void **) &qavformat_open_input},
	{"avformat_find_stream_info",                        (void **) &qavformat_find_stream_info},
	{"av_find_program_from_stream",                      (void **) &qav_find_program_from_stream},
	{"av_program_add_stream_index",                      (void **) &qav_program_add_stream_index},
	{"av_find_best_stream",                              (void **) &qav_find_best_stream},
	{"av_read_frame",                                    (void **) &qav_read_frame},
	{"av_seek_frame",                                    (void **) &qav_seek_frame},
	{"avformat_seek_file",                               (void **) &qavformat_seek_file},
	{"avformat_flush",                                   (void **) &qavformat_flush},
	{"av_read_play",                                     (void **) &qav_read_play},
	{"av_read_pause",                                    (void **) &qav_read_pause},
	{"avformat_close_input",                             (void **) &qavformat_close_input},
	{"avformat_write_header",                            (void **) &qavformat_write_header},
	{"avformat_init_output",                             (void **) &qavformat_init_output},
	{"av_write_frame",                                   (void **) &qav_write_frame},
	{"av_interleaved_write_frame",                       (void **) &qav_interleaved_write_frame},
	{"av_write_uncoded_frame",                           (void **) &qav_write_uncoded_frame},
	{"av_interleaved_write_uncoded_frame",               (void **) &qav_interleaved_write_uncoded_frame},
	{"av_write_uncoded_frame_query",                     (void **) &qav_write_uncoded_frame_query},
	{"av_write_trailer",                                 (void **) &qav_write_trailer},
	{"av_guess_format",                                  (void **) &qav_guess_format},
	{"av_guess_codec",                                   (void **) &qav_guess_codec},
	{"av_get_output_timestamp",                          (void **) &qav_get_output_timestamp},
	{"avformat_version",                                 (void **) &qavformat_version},
	{"avformat_configuration",                           (void **) &qavformat_configuration},
	{"avformat_license",                                 (void **) &qavformat_license},
	{"av_register_all",                                  (void **) &qav_register_all},
	{"av_register_input_format",                         (void **) &qav_register_input_format},
	{"av_register_output_format",                        (void **) &qav_register_output_format},
	{"avformat_network_init",                            (void **) &qavformat_network_init},
	{"avformat_network_deinit",                          (void **) &qavformat_network_deinit},
	{"av_iformat_next",                                  (void **) &qav_iformat_next},
	{"av_oformat_next",                                  (void **) &qav_oformat_next},
	{"avformat_alloc_context",                           (void **) &qavformat_alloc_context},
	{"avformat_free_context",                            (void **) &qavformat_free_context},
	{"avformat_get_class",                               (void **) &qavformat_get_class},
	{"avformat_new_stream",                              (void **) &qavformat_new_stream},
	{"av_stream_new_side_data",                          (void **) &qav_stream_new_side_data},
	{"av_stream_get_side_data",                          (void **) &qav_stream_get_side_data},
	{"av_new_program",                                   (void **) &qav_new_program},
	{"av_hex_dump",                                      (void **) &qav_hex_dump},
	{"av_hex_dump_log",                                  (void **) &qav_hex_dump_log},
	{"av_pkt_dump2",                                     (void **) &qav_pkt_dump2},
	{"av_pkt_dump_log2",                                 (void **) &qav_pkt_dump_log2},
	{"av_codec_get_id",                                  (void **) &qav_codec_get_id},
	{"av_codec_get_tag",                                 (void **) &qav_codec_get_tag},
	{"av_codec_get_tag2",                                (void **) &qav_codec_get_tag2},
	{"av_find_default_stream_index",                     (void **) &qav_find_default_stream_index},
	{"av_index_search_timestamp",                        (void **) &qav_index_search_timestamp},
	{"av_add_index_entry",                               (void **) &qav_add_index_entry},
	{"av_url_split",                                     (void **) &qav_url_split},
	{"av_dump_format",                                   (void **) &qav_dump_format},
	{"av_get_frame_filename2",                           (void **) &qav_get_frame_filename2},
	{"av_get_frame_filename",                            (void **) &qav_get_frame_filename},
	{"av_filename_number_test",                          (void **) &qav_filename_number_test},
	{"av_sdp_create",                                    (void **) &qav_sdp_create},
	{"av_match_ext",                                     (void **) &qav_match_ext},
	{"avformat_query_codec",                             (void **) &qavformat_query_codec},
	{"av_guess_sample_aspect_ratio",                     (void **) &qav_guess_sample_aspect_ratio},
	{"av_guess_frame_rate",                              (void **) &qav_guess_frame_rate},
	{"avformat_match_stream_specifier",                  (void **) &qavformat_match_stream_specifier},
	{"avformat_queue_attached_pictures",                 (void **) &qavformat_queue_attached_pictures},
	{"avformat_transfer_internal_stream_timing_info",    (void **) &qavformat_transfer_internal_stream_timing_info},
	{"av_stream_get_codec_timebase",                     (void **) &qav_stream_get_codec_timebase},
	{NULL, NULL}
};

static dllhandle_t libavcodec_dll = NULL;
static dllhandle_t libavformat_dll = NULL;

qbool LibAV_LoadLibrary(void)
{
	const char* dllnames_libavcodec [] =
	{
#if defined(WIN32)
		"libavcodec.dll",
#elif defined(MACOSX)
		"libavcodec.dylib",
#else
		"libavcodec.so",
#endif
		NULL
	};

	const char* dllnames_libavformat [] =
	{
#if defined(WIN32)
		"libavformat.dll",
#elif defined(MACOSX)
		"libavformat.dylib",
#else
		"libavformat.so",
#endif
		NULL
	};

	if (libavcodec_dll && libavformat_dll) // Already loaded?
		return true;

// COMMANDLINEOPTION: -nolibav disables libav support
	if (Sys_CheckParm("-nolibav"))
		return false;

	// Load the DLL
	if (Sys_LoadDependency (dllnames_libavcodec, &libavcodec_dll, libavcodecfuncs))
	{
		if(Sys_LoadDependency(dllnames_libavformat, &libavformat_dll, libavformatfuncs))
			return true;
		Con_Printf(CON_ERROR "Failed to load the libavformat library. Cannot use libavcodec without it\n");
		Sys_FreeLibrary(&libavcodec_dll);
	}

	return false;
}