#ifndef QFFMPEGPROC
#error "you must define QFFMPEGPROC before including this file"
#endif

#if !defined(QFFMPEG_MODULE)
#define QFFMPEG_AVCODEC
#define QFFMPEG_AVFORMAT
#define QFFMPEG_SWRESAMPLE
#define QFFMPEG_SWRSCALE
#endif

#ifdef QFFMPEG_AVCODEC
QFFMPEGPROC(av_frame_alloc, AVFrame *, (void))
QFFMPEGPROC(av_frame_free, void, (AVFrame **frame))
QFFMPEGPROC(av_strerror, int, (int errnum, char *errbuf, size_t errbuf_size))
QFFMPEGPROC(avcodec_open2, int, (AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options))
QFFMPEGPROC(avcodec_parameters_to_context, int, (AVCodecContext *codec,
        const AVCodecParameters *par))
QFFMPEGPROC(avcodec_alloc_context3, AVCodecContext *, (const AVCodec *codec))
QFFMPEGPROC(av_image_fill_arrays, int, (uint8_t *dst_data[4], int dst_linesize[4],
        const uint8_t *src,
        enum AVPixelFormat pix_fmt, int width, int height, int align))
QFFMPEGPROC(av_freep, void, (void *ptr))
QFFMPEGPROC(avcodec_close, int, (AVCodecContext *avctx))
QFFMPEGPROC(avcodec_free_context, void, (AVCodecContext **avctx))
QFFMPEGPROC(avcodec_send_packet, int, (AVCodecContext *avctx, const AVPacket *avpkt))
QFFMPEGPROC(av_packet_unref, void, (AVPacket *pkt))
QFFMPEGPROC(avcodec_receive_frame, int, (AVCodecContext *avctx, AVFrame *frame))
QFFMPEGPROC(av_samples_alloc, int, (uint8_t **audio_data, int *linesize, int nb_channels,
        int nb_samples, enum AVSampleFormat sample_fmt, int align))
#endif

#ifdef QFFMPEG_AVFORMAT
QFFMPEGPROC(avformat_open_input, int, (AVFormatContext **ps, const char *url, ff_const59 AVInputFormat *fmt, AVDictionary **options))
QFFMPEGPROC(avformat_find_stream_info, int, (AVFormatContext *ic, AVDictionary **options))
QFFMPEGPROC(av_find_best_stream, int, (AVFormatContext *ic,
                        enum AVMediaType type,
                        int wanted_stream_nb,
                        int related_stream,
                        AVCodec **decoder_ret,
                        int flags))
QFFMPEGPROC(av_seek_frame, int, (AVFormatContext *s, int stream_index, int64_t timestamp,
                  int flags))
QFFMPEGPROC(avformat_close_input, void, (AVFormatContext **s))
QFFMPEGPROC(av_read_frame, int, (AVFormatContext *s, AVPacket *pkt))
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

#undef QFFMPEGPROC

#undef QFFMPEG_AVCODEC
#undef QFFMPEG_AVFORMAT
#undef QFFMPEG_SWRESAMPLE
#undef QFFMPEG_SWRSCALE

#undef QFFMPEG_MODULE