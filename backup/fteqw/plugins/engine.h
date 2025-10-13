#ifndef FTEPLUGIN
#ifndef VARGS
#define VARGS QDECL
#endif
typedef enum uploadfmt_e
{
	TF_INVALID,
	TF_RGBA32,
	TF_BGRA32,
	TF_RGBX32,
	TF_BGRX32,
	TF_RGB24,
	TF_BGR24
} uploadfmt_t;

typedef struct
{
	size_t structsize;
	const char *drivername;
	void *(VARGS *createdecoder)(const char *name);
	qboolean (VARGS *decodeframe)(void *ctx, qboolean nosound, qboolean forcevideo, double mediatime, void (QDECL *uploadtexture)(void *ectx, uploadfmt_t fmt, int width, int height, void *data, void *palette), void *ectx);
	void (VARGS *shutdown)(void *ctx);
	void (VARGS *rewind)(void *ctx);

	//these are any interactivity functions you might want...
	void (VARGS *cursormove) (void *ctx, float posx, float posy);	//pos is 0-1
	void (VARGS *key) (void *ctx, int code, int unicode, int event);
	qboolean (VARGS *setsize) (void *ctx, int width, int height);
	void (VARGS *getsize) (void *ctx, int *width, int *height);
	void (VARGS *changestream) (void *ctx, const char *streamname);

	size_t (VARGS *gettext) (void *ctx, const char *field, char *out, size_t outlen);	//if out is null, returns required buffer size. returns 0 on failure / buffer too small
} media_decoder_funcs_t;
typedef struct
{
	size_t structsize;
	const char *drivername;
	const char *description;
	const char *defaultextension;
	void *(VARGS *capture_begin) (char *streamname, int videorate, int width, int height, int *sndkhz, int *sndchannels, int *sndbits);
	void (VARGS *capture_video) (void *ctx, void *data, int frame, int width, int height, enum uploadfmt fmt);
	void (VARGS *capture_audio) (void *ctx, void *data, int bytes);
	void (VARGS *capture_end) (void *ctx);
} media_encoder_funcs_t;
#endif

