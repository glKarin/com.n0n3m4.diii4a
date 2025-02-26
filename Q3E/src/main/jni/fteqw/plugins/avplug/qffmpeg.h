#ifndef QFFMPEGPROC
#error "you must define QFFMPEGPROC before including this file"
#endif

#if !defined(QFFMPEG_MODULE)
#define QFFMPEG_AVCODEC
#define QFFMPEG_AVFORMAT
#define QFFMPEG_SWRESAMPLE
#define QFFMPEG_SWRSCALE
#define QFFMPEG_AVUTIL
#endif

#ifdef QFFMPEG_AVCODEC
QFFMPEGPROC(av_frame_alloc, AVFrame *, (void))
QFFMPEGPROC(av_frame_free, void, (AVFrame **frame))
QFFMPEGPROC(avcodec_open2, int, (AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options))
QFFMPEGPROC(avcodec_parameters_to_context, int, (AVCodecContext *codec,
        const AVCodecParameters *par))
QFFMPEGPROC(avcodec_alloc_context3, AVCodecContext *, (const AVCodec *codec))
QFFMPEGPROC(av_image_fill_arrays, int, (uint8_t *dst_data[4], int dst_linesize[4],
        const uint8_t *src,
        enum AVPixelFormat pix_fmt, int width, int height, int align))
QFFMPEGPROC(avcodec_close, int, (AVCodecContext *avctx))
QFFMPEGPROC(avcodec_free_context, void, (AVCodecContext **avctx))
QFFMPEGPROC(avcodec_send_packet, int, (AVCodecContext *avctx, const AVPacket *avpkt))
QFFMPEGPROC(av_packet_unref, void, (AVPacket *pkt))
QFFMPEGPROC(avcodec_receive_frame, int, (AVCodecContext *avctx, AVFrame *frame))
QFFMPEGPROC(avcodec_flush_buffers, void, (AVCodecContext *avctx))
QFFMPEGPROC(avcodec_find_encoder_by_name, const AVCodec *, (const char *name))
QFFMPEGPROC(avcodec_find_encoder, const AVCodec *, (enum AVCodecID id))
QFFMPEGPROC(avcodec_find_decoder, const AVCodec *, (enum AVCodecID id))
QFFMPEGPROC(av_init_packet, void, (AVPacket *pkt))
QFFMPEGPROC(av_packet_rescale_ts, void, (AVPacket *pkt, AVRational tb_src, AVRational tb_dst))
QFFMPEGPROC(avcodec_receive_packet, int, (AVCodecContext *avctx, AVPacket *avpkt))
QFFMPEGPROC(avcodec_fill_audio_frame, int, (AVFrame *frame, int nb_channels,
			enum AVSampleFormat sample_fmt, const uint8_t *buf,
			int buf_size, int align)
		)
QFFMPEGPROC(avcodec_send_frame, int, (AVCodecContext *avctx, const AVFrame *frame))
QFFMPEGPROC(avcodec_parameters_from_context, int, (AVCodecParameters *par,
const AVCodecContext *codec)
)
#endif

#ifdef QFFMPEG_AVFORMAT
#if !defined(ff_const59)
#define ff_const59 const
#endif
QFFMPEGPROC(avformat_open_input, int, (AVFormatContext **ps, const char *url, ff_const59 AVInputFormat *fmt, AVDictionary **options))
QFFMPEGPROC(avformat_find_stream_info, int, (AVFormatContext *ic, AVDictionary **options))
QFFMPEGPROC(av_find_best_stream, int, (AVFormatContext *ic,
                        enum AVMediaType type,
                        int wanted_stream_nb,
                        int related_stream,
#if LIBAVCODEC_VERSION_MAJOR > 58
                        const
#endif
                        AVCodec **decoder_ret,
                        int flags))
QFFMPEGPROC(av_seek_frame, int, (AVFormatContext *s, int stream_index, int64_t timestamp,
                  int flags))
QFFMPEGPROC(avformat_close_input, void, (AVFormatContext **s))
QFFMPEGPROC(av_read_frame, int, (AVFormatContext *s, AVPacket *pkt))
QFFMPEGPROC(avformat_alloc_context, AVFormatContext *, (void))
QFFMPEGPROC(avformat_new_stream, AVStream *, (AVFormatContext *s, const AVCodec *c))
QFFMPEGPROC(av_guess_format, const AVOutputFormat *, (const char *short_name,
                                      const char *filename,
                                      const char *mime_type)
			)
QFFMPEGPROC(av_dump_format, void, (AVFormatContext *ic,
                    int index,
                    const char *url,
                    int is_output)
			)
QFFMPEGPROC(avformat_free_context, void, (AVFormatContext *s))
QFFMPEGPROC(av_write_trailer, int, (AVFormatContext *s))
QFFMPEGPROC(avformat_write_header, int, (AVFormatContext *s, AVDictionary **options))
QFFMPEGPROC(avio_open, int, (AVIOContext **s, const char *url, int flags))
QFFMPEGPROC(avio_alloc_context, AVIOContext *, (
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int64_t (*seek)(void *opaque, int64_t offset, int whence))
		)
QFFMPEGPROC(avio_close, int, (AVIOContext *s))
QFFMPEGPROC(av_write_trailer, int, (AVFormatContext *s))
QFFMPEGPROC(av_interleaved_write_frame, int, (AVFormatContext *s, AVPacket *pkt))
#endif

#ifdef QFFMPEG_SWRSCALE
QFFMPEGPROC(sws_getContext, struct SwsContext *, (int srcW, int srcH, enum AVPixelFormat srcFormat,
        int dstW, int dstH, enum AVPixelFormat dstFormat,
        int flags, SwsFilter *srcFilter,
                SwsFilter *dstFilter, const double *param))
QFFMPEGPROC(sws_freeContext, void, (struct SwsContext *swsContext))
QFFMPEGPROC(sws_scale, int, (struct SwsContext *c, const uint8_t *const srcSlice[],
        const int srcStride[], int srcSliceY, int srcSliceH,
        uint8_t *const dst[], const int dstStride[]))
QFFMPEGPROC(sws_getCachedContext, struct SwsContext *, (struct SwsContext *context,
                                        int srcW, int srcH, enum AVPixelFormat srcFormat,
                                        int dstW, int dstH, enum AVPixelFormat dstFormat,
                                        int flags, SwsFilter *srcFilter,
                                        SwsFilter *dstFilter, const double *param)
			)
#endif

#ifdef QFFMPEG_SWRESAMPLE
QFFMPEGPROC(swr_alloc_set_opts, struct SwrContext *, (struct SwrContext *s,
        int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
        int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
        int log_offset, void *log_ctx))
QFFMPEGPROC(swr_init, int, (struct SwrContext *s))
QFFMPEGPROC(swr_free, void, (struct SwrContext **s))
QFFMPEGPROC(swr_convert, int, (struct SwrContext *s, uint8_t **out, int out_count,
        const uint8_t **in , int in_count))
#endif

#ifdef QFFMPEG_AVUTIL
QFFMPEGPROC(av_freep, void, (void *ptr))
QFFMPEGPROC(av_malloc, void *, (size_t size))
QFFMPEGPROC(av_log_set_level, void, (int level))
QFFMPEGPROC(av_free, void, (void *ptr))
QFFMPEGPROC(av_strdup, char *, (const char *s))
QFFMPEGPROC(av_log_set_callback, void, (void (*callback)(void*, int, const char*, va_list)))
QFFMPEGPROC(av_strerror, int, (int errnum, char *errbuf, size_t errbuf_size))
QFFMPEGPROC(av_samples_get_buffer_size, int, (int *linesize, int nb_channels, int nb_samples,
                               enum AVSampleFormat sample_fmt, int align)
			)
QFFMPEGPROC(av_opt_set, int, (void *obj, const char *name, const char *val, int search_flags))
QFFMPEGPROC(av_image_alloc, int, (uint8_t *pointers[4], int linesizes[4],
                   int w, int h, enum AVPixelFormat pix_fmt, int align)
			)
QFFMPEGPROC(av_image_get_buffer_size, int, (enum AVPixelFormat pix_fmt, int width, int height, int align))
QFFMPEGPROC(av_get_default_channel_layout, int64_t, (int nb_channels))
QFFMPEGPROC(av_get_bytes_per_sample, int, (enum AVSampleFormat sample_fmt))
QFFMPEGPROC(av_samples_alloc, int, (uint8_t **audio_data, int *linesize, int nb_channels,
int nb_samples, enum AVSampleFormat sample_fmt, int align))
#endif

#undef QFFMPEGPROC

#undef QFFMPEG_AVCODEC
#undef QFFMPEG_AVFORMAT
#undef QFFMPEG_SWRESAMPLE
#undef QFFMPEG_SWRSCALE
#undef QFFMPEG_AVUTIL

#undef QFFMPEG_MODULE
