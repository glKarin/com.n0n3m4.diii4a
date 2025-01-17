#include <sys/types.h>

#include "quakedef.h"
#include "client.h"
#include "cap_ogg.h"

// video capture cvars
static cvar_t cl_capturevideo_ogg_theora_vp3compat = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_ogg_theora_vp3compat", "1", "make VP3 compatible theora streams"};
static cvar_t cl_capturevideo_ogg_theora_quality = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_ogg_theora_quality", "48", "video quality factor (0 to 63), or -1 to use bitrate only; higher is better; setting both to -1 achieves unlimited quality"};
static cvar_t cl_capturevideo_ogg_theora_bitrate = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_ogg_theora_bitrate", "-1", "video bitrate (45 to 2000 kbps), or -1 to use quality only; higher is better; setting both to -1 achieves unlimited quality"};
static cvar_t cl_capturevideo_ogg_theora_keyframe_bitrate_multiplier = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_ogg_theora_keyframe_bitrate_multiplier", "1.5", "how much more bit rate to use for keyframes, specified as a factor of at least 1"};
static cvar_t cl_capturevideo_ogg_theora_keyframe_maxinterval = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_ogg_theora_keyframe_maxinterval", "64", "maximum keyframe interval (1 to 1000)"};
static cvar_t cl_capturevideo_ogg_theora_keyframe_mininterval = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_ogg_theora_keyframe_mininterval", "8", "minimum keyframe interval (1 to 1000)"};
static cvar_t cl_capturevideo_ogg_theora_keyframe_auto_threshold = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_ogg_theora_keyframe_auto_threshold", "80", "threshold for key frame decision (0 to 100)"};
static cvar_t cl_capturevideo_ogg_theora_noise_sensitivity = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_ogg_theora_noise_sensitivity", "1", "video noise sensitivity (0 to 6); lower is better"};
static cvar_t cl_capturevideo_ogg_theora_sharpness = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_ogg_theora_sharpness", "0", "sharpness (0 to 2); lower is sharper"};
static cvar_t cl_capturevideo_ogg_vorbis_quality = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_ogg_vorbis_quality", "3", "audio quality (-1 to 10); higher is better"};

typedef struct {
  long endbyte;
  int  endbit;

  unsigned char *buffer;
  unsigned char *ptr;
  long storage;
} oggpack_buffer;

/* ogg_page is used to encapsulate the data in one Ogg bitstream page *****/

typedef struct {
  unsigned char *header;
  long header_len;
  unsigned char *body;
  long body_len;
} ogg_page;

/* ogg_stream_state contains the current encode/decode state of a logical
   Ogg bitstream **********************************************************/

typedef struct {
  unsigned char   *body_data;    /* bytes from packet bodies */
  long    body_storage;          /* storage elements allocated */
  long    body_fill;             /* elements stored; fill mark */
  long    body_returned;         /* elements of fill returned */


  int     *lacing_vals;      /* The values that will go to the segment table */
  int64_t *granule_vals; /* granulepos values for headers. Not compact
				this way, but it is simple coupled to the
				lacing fifo */
  long    lacing_storage;
  long    lacing_fill;
  long    lacing_packet;
  long    lacing_returned;

  unsigned char    header[282];      /* working space for header encode */
  int              header_fill;

  int     e_o_s;          /* set when we have buffered the last packet in the
                             logical bitstream */
  int     b_o_s;          /* set after we've written the initial page
                             of a logical bitstream */
  long    serialno;
  long    pageno;
  int64_t  packetno;      /* sequence number for decode; the framing
                             knows where there's a hole in the data,
                             but we need coupling so that the codec
                             (which is in a seperate abstraction
                             layer) also knows about the gap */
  int64_t   granulepos;

} ogg_stream_state;

/* ogg_packet is used to encapsulate the data and metadata belonging
   to a single raw Ogg/Vorbis packet *************************************/

typedef struct {
  unsigned char *packet;
  long  bytes;
  long  b_o_s;
  long  e_o_s;

  int64_t  granulepos;
  
  int64_t  packetno;     /* sequence number for decode; the framing
				knows where there's a hole in the data,
				but we need coupling so that the codec
				(which is in a seperate abstraction
				layer) also knows about the gap */
} ogg_packet;

typedef struct {
  unsigned char *data;
  int storage;
  int fill;
  int returned;

  int unsynced;
  int headerbytes;
  int bodybytes;
} ogg_sync_state;

/* Ogg BITSTREAM PRIMITIVES: encoding **************************/

static int      (*qogg_stream_packetin) (ogg_stream_state *os, ogg_packet *op);
static int      (*qogg_stream_pageout) (ogg_stream_state *os, ogg_page *og);
static int      (*qogg_stream_flush) (ogg_stream_state *os, ogg_page *og);

/* Ogg BITSTREAM PRIMITIVES: general ***************************/

static int      (*qogg_stream_init) (ogg_stream_state *os,int serialno);
static int      (*qogg_stream_clear) (ogg_stream_state *os);
static int64_t  (*qogg_page_granulepos) (ogg_page *og);

// end of ogg.h stuff

// vorbis/codec.h stuff
typedef struct vorbis_info{
  int version;
  int channels;
  long rate;

  /* The below bitrate declarations are *hints*.
     Combinations of the three values carry the following implications:

     all three set to the same value:
       implies a fixed rate bitstream
     only nominal set:
       implies a VBR stream that averages the nominal bitrate.  No hard
       upper/lower limit
     upper and or lower set:
       implies a VBR bitstream that obeys the bitrate limits. nominal
       may also be set to give a nominal rate.
     none set:
       the coder does not care to speculate.
  */

  long bitrate_upper;
  long bitrate_nominal;
  long bitrate_lower;
  long bitrate_window;

  void *codec_setup;
} vorbis_info;

/* vorbis_dsp_state buffers the current vorbis audio
   analysis/synthesis state.  The DSP state belongs to a specific
   logical bitstream ****************************************************/
typedef struct vorbis_dsp_state{
  int analysisp;
  vorbis_info *vi;

  float **pcm;
  float **pcmret;
  int      pcm_storage;
  int      pcm_current;
  int      pcm_returned;

  int  preextrapolate;
  int  eofflag;

  long lW;
  long W;
  long nW;
  long centerW;

  int64_t granulepos;
  int64_t sequence;

  int64_t glue_bits;
  int64_t time_bits;
  int64_t floor_bits;
  int64_t res_bits;

  void       *backend_state;
} vorbis_dsp_state;

typedef struct vorbis_block{
  /* necessary stream state for linking to the framing abstraction */
  float  **pcm;       /* this is a pointer into local storage */
  oggpack_buffer opb;

  long  lW;
  long  W;
  long  nW;
  int   pcmend;
  int   mode;

  int         eofflag;
  int64_t granulepos;
  int64_t sequence;
  vorbis_dsp_state *vd; /* For read-only access of configuration */

  /* local storage to avoid remallocing; it's up to the mapping to
     structure it */
  void               *localstore;
  long                localtop;
  long                localalloc;
  long                totaluse;
  struct alloc_chain *reap;

  /* bitmetrics for the frame */
  long glue_bits;
  long time_bits;
  long floor_bits;
  long res_bits;

  void *internal;

} vorbis_block;

/* vorbis_block is a single block of data to be processed as part of
the analysis/synthesis stream; it belongs to a specific logical
bitstream, but is independant from other vorbis_blocks belonging to
that logical bitstream. *************************************************/

struct alloc_chain{
  void *ptr;
  struct alloc_chain *next;
};

/* vorbis_info contains all the setup information specific to the
   specific compression/decompression mode in progress (eg,
   psychoacoustic settings, channel setup, options, codebook
   etc). vorbis_info and substructures are in backends.h.
*********************************************************************/

/* the comments are not part of vorbis_info so that vorbis_info can be
   static storage */
typedef struct vorbis_comment{
  /* unlimited user comment fields.  libvorbis writes 'libvorbis'
     whatever vendor is set to in encode */
  char **user_comments;
  int   *comment_lengths;
  int    comments;
  char  *vendor;

} vorbis_comment;


/* libvorbis encodes in two abstraction layers; first we perform DSP
   and produce a packet (see docs/analysis.txt).  The packet is then
   coded into a framed OggSquish bitstream by the second layer (see
   docs/framing.txt).  Decode is the reverse process; we sync/frame
   the bitstream and extract individual packets, then decode the
   packet back into PCM audio.

   The extra framing/packetizing is used in streaming formats, such as
   files.  Over the net (such as with UDP), the framing and
   packetization aren't necessary as they're provided by the transport
   and the streaming layer is not used */

/* Vorbis PRIMITIVES: general ***************************************/

static void     (*qvorbis_info_init) (vorbis_info *vi);
static void     (*qvorbis_info_clear) (vorbis_info *vi);
static void     (*qvorbis_comment_init) (vorbis_comment *vc);
static void     (*qvorbis_comment_clear) (vorbis_comment *vc);

static int      (*qvorbis_block_init) (vorbis_dsp_state *v, vorbis_block *vb);
static int      (*qvorbis_block_clear) (vorbis_block *vb);
static void     (*qvorbis_dsp_clear) (vorbis_dsp_state *v);
static double   (*qvorbis_granule_time) (vorbis_dsp_state *v,
				    int64_t granulepos);

/* Vorbis PRIMITIVES: analysis/DSP layer ****************************/

static int      (*qvorbis_analysis_init) (vorbis_dsp_state *v,vorbis_info *vi);
static int      (*qvorbis_commentheader_out) (vorbis_comment *vc, ogg_packet *op);
static int      (*qvorbis_analysis_headerout) (vorbis_dsp_state *v,
					  vorbis_comment *vc,
					  ogg_packet *op,
					  ogg_packet *op_comm,
					  ogg_packet *op_code);
static float ** (*qvorbis_analysis_buffer) (vorbis_dsp_state *v,int vals);
static int      (*qvorbis_analysis_wrote) (vorbis_dsp_state *v,int vals);
static int      (*qvorbis_analysis_blockout) (vorbis_dsp_state *v,vorbis_block *vb);
static int      (*qvorbis_analysis) (vorbis_block *vb,ogg_packet *op);

static int      (*qvorbis_bitrate_addblock) (vorbis_block *vb);
static int      (*qvorbis_bitrate_flushpacket) (vorbis_dsp_state *vd,
					   ogg_packet *op);

// end of vorbis/codec.h stuff

// vorbisenc.h stuff
static int (*qvorbis_encode_init_vbr) (vorbis_info *vi,
				  long channels,
				  long rate,

				  float base_quality /* quality level from 0. (lo) to 1. (hi) */
				  );
// end of vorbisenc.h stuff

// theora.h stuff

#define TH_ENCCTL_SET_VP3_COMPATIBLE (10)

typedef struct {
    int   y_width;      /**< Width of the Y' luminance plane */
    int   y_height;     /**< Height of the luminance plane */
    int   y_stride;     /**< Offset in bytes between successive rows */

    int   uv_width;     /**< Width of the Cb and Cr chroma planes */
    int   uv_height;    /**< Height of the chroma planes */
    int   uv_stride;    /**< Offset between successive chroma rows */
    unsigned char *y;   /**< Pointer to start of luminance data */
    unsigned char *u;   /**< Pointer to start of Cb data */
    unsigned char *v;   /**< Pointer to start of Cr data */

} yuv_buffer;

/**
 * A Colorspace.
 */
typedef enum {
  OC_CS_UNSPECIFIED,    /**< The colorspace is unknown or unspecified */
  OC_CS_ITU_REC_470M,   /**< This is the best option for 'NTSC' content */
  OC_CS_ITU_REC_470BG,  /**< This is the best option for 'PAL' content */
  OC_CS_NSPACES         /**< This marks the end of the defined colorspaces */
} theora_colorspace;

/**
 * A Chroma subsampling
 *
 * These enumerate the available chroma subsampling options supported
 * by the theora format. See Section 4.4 of the specification for
 * exact definitions.
 */
typedef enum {
  OC_PF_420,    /**< Chroma subsampling by 2 in each direction (4:2:0) */
  OC_PF_RSVD,   /**< Reserved value */
  OC_PF_422,    /**< Horizonatal chroma subsampling by 2 (4:2:2) */
  OC_PF_444     /**< No chroma subsampling at all (4:4:4) */
} theora_pixelformat;
/**
 * Theora bitstream info.
 * Contains the basic playback parameters for a stream,
 * corresponding to the initial 'info' header packet.
 * 
 * Encoded theora frames must be a multiple of 16 in width and height.
 * To handle other frame sizes, a crop rectangle is specified in
 * frame_height and frame_width, offset_x and * offset_y. The offset
 * and size should still be a multiple of 2 to avoid chroma sampling
 * shifts. Offset values in this structure are measured from the
 * upper left of the image.
 *
 * Frame rate, in frames per second, is stored as a rational
 * fraction. Aspect ratio is also stored as a rational fraction, and
 * refers to the aspect ratio of the frame pixels, not of the
 * overall frame itself.
 * 
 * See <a href="http://svn.xiph.org/trunk/theora/examples/encoder_example.c">
 * examples/encoder_example.c</a> for usage examples of the
 * other paramters and good default settings for the encoder parameters.
 */
typedef struct {
  uint32_t  width;      /**< encoded frame width  */
  uint32_t  height;     /**< encoded frame height */
  uint32_t  frame_width;    /**< display frame width  */
  uint32_t  frame_height;   /**< display frame height */
  uint32_t  offset_x;   /**< horizontal offset of the displayed frame */
  uint32_t  offset_y;   /**< vertical offset of the displayed frame */
  uint32_t  fps_numerator;      /**< frame rate numerator **/
  uint32_t  fps_denominator;    /**< frame rate denominator **/
  uint32_t  aspect_numerator;   /**< pixel aspect ratio numerator */
  uint32_t  aspect_denominator; /**< pixel aspect ratio denominator */
  theora_colorspace colorspace;     /**< colorspace */
  int           target_bitrate;     /**< nominal bitrate in bits per second */
  int           quality;  /**< Nominal quality setting, 0-63 */
  int           quick_p;  /**< Quick encode/decode */

  /* decode only */
  unsigned char version_major;
  unsigned char version_minor;
  unsigned char version_subminor;

  void *codec_setup;

  /* encode only */
  int           dropframes_p;
  int           keyframe_auto_p;
  uint32_t  keyframe_frequency;
  uint32_t  keyframe_frequency_force;  /* also used for decode init to
                                              get granpos shift correct */
  uint32_t  keyframe_data_target_bitrate;
  int32_t   keyframe_auto_threshold;
  uint32_t  keyframe_mindistance;
  int32_t   noise_sensitivity;
  int32_t   sharpness;

  theora_pixelformat pixelformat;   /**< chroma subsampling mode to expect */

} theora_info;

/** Codec internal state and context.
 */
typedef struct{
  theora_info *i;
  int64_t granulepos;

  void *internal_encode;
  void *internal_decode;

} theora_state;

/** 
 * Comment header metadata.
 *
 * This structure holds the in-stream metadata corresponding to
 * the 'comment' header packet.
 *
 * Meta data is stored as a series of (tag, value) pairs, in
 * length-encoded string vectors. The first occurence of the 
 * '=' character delimits the tag and value. A particular tag
 * may occur more than once. The character set encoding for
 * the strings is always UTF-8, but the tag names are limited
 * to case-insensitive ASCII. See the spec for details.
 *
 * In filling in this structure, qtheora_decode_header() will
 * null-terminate the user_comment strings for safety. However,
 * the bitstream format itself treats them as 8-bit clean,
 * and so the length array should be treated as authoritative
 * for their length.
 */
typedef struct theora_comment{
  char **user_comments;         /**< An array of comment string vectors */
  int   *comment_lengths;       /**< An array of corresponding string vector lengths in bytes */
  int    comments;              /**< The total number of comment string vectors */
  char  *vendor;                /**< The vendor string identifying the encoder, null terminated */

} theora_comment;
static int (*qtheora_encode_init) (theora_state *th, theora_info *ti);
static int (*qtheora_encode_YUVin) (theora_state *t, yuv_buffer *yuv);
static int (*qtheora_encode_packetout) ( theora_state *t, int last_p,
                                    ogg_packet *op);
static int (*qtheora_encode_header) (theora_state *t, ogg_packet *op);
static int (*qtheora_encode_comment) (theora_comment *tc, ogg_packet *op);
static int (*qtheora_encode_tables) (theora_state *t, ogg_packet *op);
static void (*qtheora_info_init) (theora_info *c);
static void (*qtheora_info_clear) (theora_info *c);
static void (*qtheora_clear) (theora_state *t);
static void (*qtheora_comment_init) (theora_comment *tc);
static void  (*qtheora_comment_clear) (theora_comment *tc);
static double (*qtheora_granule_time) (theora_state *th,int64_t granulepos);
static int (*qtheora_control) (theora_state *th,int req,void *buf,size_t buf_sz);
// end of theora.h stuff

static dllfunction_t oggfuncs[] =
{
	{"ogg_stream_packetin", (void **) &qogg_stream_packetin},
	{"ogg_stream_pageout", (void **) &qogg_stream_pageout},
	{"ogg_stream_flush", (void **) &qogg_stream_flush},
	{"ogg_stream_init", (void **) &qogg_stream_init},
	{"ogg_stream_clear", (void **) &qogg_stream_clear},
	{"ogg_page_granulepos", (void **) &qogg_page_granulepos},
	{NULL, NULL}
};

static dllfunction_t vorbisencfuncs[] =
{
	{"vorbis_encode_init_vbr", (void **) &qvorbis_encode_init_vbr},
	{NULL, NULL}
};

static dllfunction_t vorbisfuncs[] =
{
	{"vorbis_info_init", (void **) &qvorbis_info_init},
	{"vorbis_info_clear", (void **) &qvorbis_info_clear},
	{"vorbis_comment_init", (void **) &qvorbis_comment_init},
	{"vorbis_comment_clear", (void **) &qvorbis_comment_clear},
	{"vorbis_block_init", (void **) &qvorbis_block_init},
	{"vorbis_block_clear", (void **) &qvorbis_block_clear},
	{"vorbis_dsp_clear", (void **) &qvorbis_dsp_clear},
	{"vorbis_analysis_init", (void **) &qvorbis_analysis_init},
	{"vorbis_commentheader_out", (void **) &qvorbis_commentheader_out},
	{"vorbis_analysis_headerout", (void **) &qvorbis_analysis_headerout},
	{"vorbis_analysis_buffer", (void **) &qvorbis_analysis_buffer},
	{"vorbis_analysis_wrote", (void **) &qvorbis_analysis_wrote},
	{"vorbis_analysis_blockout", (void **) &qvorbis_analysis_blockout},
	{"vorbis_analysis", (void **) &qvorbis_analysis},
	{"vorbis_bitrate_addblock", (void **) &qvorbis_bitrate_addblock},
	{"vorbis_bitrate_flushpacket", (void **) &qvorbis_bitrate_flushpacket},
	{"vorbis_granule_time", (void **) &qvorbis_granule_time},
	{NULL, NULL}
};

static dllfunction_t theorafuncs[] =
{
	{"theora_info_init", (void **) &qtheora_info_init},
	{"theora_info_clear", (void **) &qtheora_info_clear},
	{"theora_comment_init", (void **) &qtheora_comment_init},
	{"theora_comment_clear", (void **) &qtheora_comment_clear},
	{"theora_encode_init", (void **) &qtheora_encode_init},
	{"theora_encode_YUVin", (void **) &qtheora_encode_YUVin},
	{"theora_encode_packetout", (void **) &qtheora_encode_packetout},
	{"theora_encode_header", (void **) &qtheora_encode_header},
	{"theora_encode_comment", (void **) &qtheora_encode_comment},
	{"theora_encode_tables", (void **) &qtheora_encode_tables},
	{"theora_clear", (void **) &qtheora_clear},
	{"theora_granule_time", (void **) &qtheora_granule_time},
	{"theora_control", (void **) &qtheora_control},
	{NULL, NULL}
};

static dllhandle_t og_dll = NULL, vo_dll = NULL, ve_dll = NULL, th_dll = NULL;

static qbool SCR_CaptureVideo_Ogg_OpenLibrary(void)
{
	const char* dllnames_og [] =
	{
#if defined(WIN32)
		"libogg-0.dll",
		"libogg.dll",
		"ogg.dll",
#elif defined(MACOSX)
		"libogg.dylib",
#else
		"libogg.so.0",
		"libogg.so",
#endif
		NULL
	};
	const char* dllnames_vo [] =
	{
#if defined(WIN32)
		"libvorbis-0.dll",
		"libvorbis.dll",
		"vorbis.dll",
#elif defined(MACOSX)
		"libvorbis.dylib",
#else
		"libvorbis.so.0",
		"libvorbis.so",
#endif
		NULL
	};
	const char* dllnames_ve [] =
	{
#if defined(WIN32)
		"libvorbisenc-2.dll",
		"libvorbisenc.dll",
		"vorbisenc.dll",
#elif defined(MACOSX)
		"libvorbisenc.dylib",
#else
		"libvorbisenc.so.2",
		"libvorbisenc.so",
#endif
		NULL
	};
	const char* dllnames_th [] =
	{
#if defined(WIN32)
		"libtheora-0.dll",
		"libtheora.dll",
		"theora.dll",
#elif defined(MACOSX)
		"libtheora.dylib",
#else
		"libtheora.so.0",
		"libtheora.so",
#endif
		NULL
	};

	return
		Sys_LoadDependency (dllnames_og, &og_dll, oggfuncs)
		&&
		Sys_LoadDependency (dllnames_th, &th_dll, theorafuncs)
		&&
		Sys_LoadDependency (dllnames_vo, &vo_dll, vorbisfuncs)
		&&
		Sys_LoadDependency (dllnames_ve, &ve_dll, vorbisencfuncs);
}

void SCR_CaptureVideo_Ogg_Init(void)
{
	SCR_CaptureVideo_Ogg_OpenLibrary();

	Cvar_RegisterVariable(&cl_capturevideo_ogg_theora_vp3compat);
	Cvar_RegisterVariable(&cl_capturevideo_ogg_theora_quality);
	Cvar_RegisterVariable(&cl_capturevideo_ogg_theora_bitrate);
	Cvar_RegisterVariable(&cl_capturevideo_ogg_theora_keyframe_bitrate_multiplier);
	Cvar_RegisterVariable(&cl_capturevideo_ogg_theora_keyframe_maxinterval);
	Cvar_RegisterVariable(&cl_capturevideo_ogg_theora_keyframe_mininterval);
	Cvar_RegisterVariable(&cl_capturevideo_ogg_theora_keyframe_auto_threshold);
	Cvar_RegisterVariable(&cl_capturevideo_ogg_theora_noise_sensitivity);
	Cvar_RegisterVariable(&cl_capturevideo_ogg_theora_sharpness);
	Cvar_RegisterVariable(&cl_capturevideo_ogg_vorbis_quality);
}

qbool SCR_CaptureVideo_Ogg_Available(void)
{
	return og_dll && th_dll && vo_dll && ve_dll;
}

void SCR_CaptureVideo_Ogg_CloseDLL(void)
{
	Sys_FreeLibrary (&ve_dll);
	Sys_FreeLibrary (&vo_dll);
	Sys_FreeLibrary (&th_dll);
	Sys_FreeLibrary (&og_dll);
}

// this struct should not be needed
// however, libogg appears to pull the ogg_page's data element away from our
// feet before we get to write the data due to interleaving
// so this struct is used to keep the page data around until it actually gets
// written
typedef struct allocatedoggpage_s
{
	size_t len;
	double time;
	unsigned char data[65307];
	// this number is from RFC 3533. In case libogg writes more, we'll have to increase this
	// but we'll get a Host_Error in this case so we can track it down
}
allocatedoggpage_t;

typedef struct capturevideostate_ogg_formatspecific_s
{
	ogg_stream_state to, vo;
	int serial1, serial2;
	theora_state ts;
	vorbis_dsp_state vd;
	vorbis_block vb;
	vorbis_info vi;
	yuv_buffer yuv[2];
	int yuvi;
	int lastnum;
	int channels;

	allocatedoggpage_t videopage, audiopage;
}
capturevideostate_ogg_formatspecific_t;
#define LOAD_FORMATSPECIFIC_OGG() capturevideostate_ogg_formatspecific_t *format = (capturevideostate_ogg_formatspecific_t *) cls.capturevideo.formatspecific

static void SCR_CaptureVideo_Ogg_Interleave(void)
{
	LOAD_FORMATSPECIFIC_OGG();
	ogg_page pg;

	if(!cls.capturevideo.soundrate)
	{
		while(qogg_stream_pageout(&format->to, &pg) > 0)
		{
			FS_Write(cls.capturevideo.videofile, pg.header, pg.header_len);
			FS_Write(cls.capturevideo.videofile, pg.body, pg.body_len);
		}
		return;
	}

	for(;;)
	{
		// first: make sure we have a page of both types
		if(!format->videopage.len)
			if(qogg_stream_pageout(&format->to, &pg) > 0)
			{
				format->videopage.len = pg.header_len + pg.body_len;
				format->videopage.time = qtheora_granule_time(&format->ts, qogg_page_granulepos(&pg));
				if(format->videopage.len > sizeof(format->videopage.data))
					Sys_Error("video page too long");
				memcpy(format->videopage.data, pg.header, pg.header_len);
				memcpy(format->videopage.data + pg.header_len, pg.body, pg.body_len);
			}
		if(!format->audiopage.len)
			if(qogg_stream_pageout(&format->vo, &pg) > 0)
			{
				format->audiopage.len = pg.header_len + pg.body_len;
				format->audiopage.time = qvorbis_granule_time(&format->vd, qogg_page_granulepos(&pg));
				if(format->audiopage.len > sizeof(format->audiopage.data))
					Sys_Error("audio page too long");
				memcpy(format->audiopage.data, pg.header, pg.header_len);
				memcpy(format->audiopage.data + pg.header_len, pg.body, pg.body_len);
			}

		if(format->videopage.len && format->audiopage.len)
		{
			// output the page that ends first
			if(format->videopage.time < format->audiopage.time)
			{
				FS_Write(cls.capturevideo.videofile, format->videopage.data, format->videopage.len);
				format->videopage.len = 0;
			}
			else
			{
				FS_Write(cls.capturevideo.videofile, format->audiopage.data, format->audiopage.len);
				format->audiopage.len = 0;
			}
		}
		else
			break;
	}
}

static void SCR_CaptureVideo_Ogg_FlushInterleaving(void)
{
	LOAD_FORMATSPECIFIC_OGG();

	if(cls.capturevideo.soundrate)
	if(format->audiopage.len)
	{
		FS_Write(cls.capturevideo.videofile, format->audiopage.data, format->audiopage.len);
		format->audiopage.len = 0;
	}

	if(format->videopage.len)
	{
		FS_Write(cls.capturevideo.videofile, format->videopage.data, format->videopage.len);
		format->videopage.len = 0;
	}
}

static void SCR_CaptureVideo_Ogg_EndVideo(void)
{
	LOAD_FORMATSPECIFIC_OGG();
	ogg_page pg;
	ogg_packet pt;

	if(format->yuvi >= 0)
	{
		// send the previous (and last) frame
		while(format->lastnum-- > 0)
		{
			qtheora_encode_YUVin(&format->ts, &format->yuv[format->yuvi]);

			while(qtheora_encode_packetout(&format->ts, !format->lastnum, &pt))
				qogg_stream_packetin(&format->to, &pt);

			SCR_CaptureVideo_Ogg_Interleave();
		}
	}

	if(cls.capturevideo.soundrate)
	{
		qvorbis_analysis_wrote(&format->vd, 0);
		while(qvorbis_analysis_blockout(&format->vd, &format->vb) == 1)
		{
			qvorbis_analysis(&format->vb, NULL);
			qvorbis_bitrate_addblock(&format->vb);
			while(qvorbis_bitrate_flushpacket(&format->vd, &pt))
				qogg_stream_packetin(&format->vo, &pt);
			SCR_CaptureVideo_Ogg_Interleave();
		}
	}

	SCR_CaptureVideo_Ogg_FlushInterleaving();

	while(qogg_stream_pageout(&format->to, &pg) > 0)
	{
		FS_Write(cls.capturevideo.videofile, pg.header, pg.header_len);
		FS_Write(cls.capturevideo.videofile, pg.body, pg.body_len);
	}

	if(cls.capturevideo.soundrate)
	{
		while(qogg_stream_pageout(&format->vo, &pg) > 0)
		{
			FS_Write(cls.capturevideo.videofile, pg.header, pg.header_len);
			FS_Write(cls.capturevideo.videofile, pg.body, pg.body_len);
		}
	}
		
	while (1) {
		int result = qogg_stream_flush (&format->to, &pg);
		if (result < 0)
			fprintf (stderr, "Internal Ogg library error.\n"); // TODO Sys_Error
		if (result <= 0)
			break;
		FS_Write(cls.capturevideo.videofile, pg.header, pg.header_len);
		FS_Write(cls.capturevideo.videofile, pg.body, pg.body_len);
	}

	if(cls.capturevideo.soundrate)
	{
		while (1) {
			int result = qogg_stream_flush (&format->vo, &pg);
			if (result < 0)
				fprintf (stderr, "Internal Ogg library error.\n"); // TODO Sys_Error
			if (result <= 0)
				break;
			FS_Write(cls.capturevideo.videofile, pg.header, pg.header_len);
			FS_Write(cls.capturevideo.videofile, pg.body, pg.body_len);
		}

		qogg_stream_clear(&format->vo);
		qvorbis_block_clear(&format->vb);
		qvorbis_dsp_clear(&format->vd);
	}

	qogg_stream_clear(&format->to);
	qtheora_clear(&format->ts);
	qvorbis_info_clear(&format->vi);

	Mem_Free(format->yuv[0].y);
	Mem_Free(format->yuv[0].u);
	Mem_Free(format->yuv[0].v);
	Mem_Free(format->yuv[1].y);
	Mem_Free(format->yuv[1].u);
	Mem_Free(format->yuv[1].v);
	Mem_Free(format);

	FS_Close(cls.capturevideo.videofile);
	cls.capturevideo.videofile = NULL;
}

static void SCR_CaptureVideo_Ogg_ConvertFrame_BGRA_to_YUV(u8 *in)
{
	LOAD_FORMATSPECIFIC_OGG();
	yuv_buffer *yuv;
	int x, y;
	int blockr, blockg, blockb;
	unsigned char *b;
	int w = cls.capturevideo.width;
	int h = cls.capturevideo.height;
	int inpitch = w*4;

	yuv = &format->yuv[format->yuvi];

	for(y = 0; y < h; ++y)
	{
		for(b = in + (h-1-y)*w*4, x = 0; x < w; ++x)
		{
			blockr = b[2];
			blockg = b[1];
			blockb = b[0];
			yuv->y[x + yuv->y_stride * y] =
				cls.capturevideo.yuvnormalizetable[0][cls.capturevideo.rgbtoyuvscaletable[0][0][blockr] + cls.capturevideo.rgbtoyuvscaletable[0][1][blockg] + cls.capturevideo.rgbtoyuvscaletable[0][2][blockb]];
			b += 4;
		}

		if ((y & 1) == 0 && y/2 < h/2) // if h is odd, this skips the last row
		{
			for(b = in + (h-2-y)*w*4, x = 0; x < w/2; ++x)
			{
				blockr = (b[2] + b[6] + b[inpitch+2] + b[inpitch+6]) >> 2;
				blockg = (b[1] + b[5] + b[inpitch+1] + b[inpitch+5]) >> 2;
				blockb = (b[0] + b[4] + b[inpitch+0] + b[inpitch+4]) >> 2;
				yuv->u[x + yuv->uv_stride * (y/2)] =
					cls.capturevideo.yuvnormalizetable[1][cls.capturevideo.rgbtoyuvscaletable[1][0][blockr] + cls.capturevideo.rgbtoyuvscaletable[1][1][blockg] + cls.capturevideo.rgbtoyuvscaletable[1][2][blockb] + 128];
				yuv->v[x + yuv->uv_stride * (y/2)] =
					cls.capturevideo.yuvnormalizetable[2][cls.capturevideo.rgbtoyuvscaletable[2][0][blockr] + cls.capturevideo.rgbtoyuvscaletable[2][1][blockg] + cls.capturevideo.rgbtoyuvscaletable[2][2][blockb] + 128];
				b += 8;
			}
		}
	}
}

static void SCR_CaptureVideo_Ogg_VideoFrames(int num, u8 *in)
{
	LOAD_FORMATSPECIFIC_OGG();
	ogg_packet pt;

	// data is in cls.capturevideo.outbuffer as BGRA and has size width*height

	if(format->yuvi >= 0)
	{
		// send the previous frame
		while(format->lastnum-- > 0)
		{
			qtheora_encode_YUVin(&format->ts, &format->yuv[format->yuvi]);

			while(qtheora_encode_packetout(&format->ts, false, &pt))
				qogg_stream_packetin(&format->to, &pt);

			SCR_CaptureVideo_Ogg_Interleave();
		}
	}

	format->yuvi = (format->yuvi + 1) % 2;
	SCR_CaptureVideo_Ogg_ConvertFrame_BGRA_to_YUV(in);
	format->lastnum = num;

	// TODO maybe send num-1 frames from here already
}

typedef int channelmapping_t[8];
channelmapping_t mapping[8] =
{
	{ 0, -1, -1, -1, -1, -1, -1, -1 }, // mono
	{ 0, 1, -1, -1, -1, -1, -1, -1 }, // stereo
	{ 0, 1, 2, -1, -1, -1, -1, -1 }, // L C R
	{ 0, 1, 2, 3, -1, -1, -1, -1 }, // surround40
	{ 0, 2, 3, 4, 1, -1, -1, -1 }, // FL FC FR RL RR
	{ 0, 2, 3, 4, 1, 5, -1, -1 }, // surround51
	{ 0, 2, 3, 4, 1, 5, 6, -1 }, // (not defined by vorbis spec)
	{ 0, 2, 3, 4, 1, 5, 6, 7 } // surround71 (not defined by vorbis spec)
};

static void SCR_CaptureVideo_Ogg_SoundFrame(const portable_sampleframe_t *paintbuffer, size_t length)
{
	LOAD_FORMATSPECIFIC_OGG();
	float **vorbis_buffer;
	size_t i;
	int j;
	ogg_packet pt;
	int *map = mapping[bound(1, cls.capturevideo.soundchannels, 8) - 1];

	vorbis_buffer = qvorbis_analysis_buffer(&format->vd, (int)length);
	for(j = 0; j < cls.capturevideo.soundchannels; ++j)
	{
		float *b = vorbis_buffer[map[j]];
		for(i = 0; i < length; ++i)
			b[i] = paintbuffer[i].sample[j];
	}
	qvorbis_analysis_wrote(&format->vd, (int)length);

	while(qvorbis_analysis_blockout(&format->vd, &format->vb) == 1)
	{
		qvorbis_analysis(&format->vb, NULL);
		qvorbis_bitrate_addblock(&format->vb);

		while(qvorbis_bitrate_flushpacket(&format->vd, &pt))
			qogg_stream_packetin(&format->vo, &pt);
	}

	SCR_CaptureVideo_Ogg_Interleave();
}

void SCR_CaptureVideo_Ogg_BeginVideo(void)
{
	char vabuf[1024];
	cls.capturevideo.format = CAPTUREVIDEOFORMAT_OGG_VORBIS_THEORA;
	cls.capturevideo.formatextension = "ogv";
	cls.capturevideo.videofile = FS_OpenRealFile(va(vabuf, sizeof(vabuf), "%s.%s", cls.capturevideo.basename, cls.capturevideo.formatextension), "wb", false);
	if (!cls.capturevideo.videofile)
	{
		Con_Printf(CON_ERROR "Failed to open video file \"%s\", cancelling video capture.\n", vabuf);
		cls.capturevideo.error = true;
		return;
	}
	cls.capturevideo.writeEndVideo = SCR_CaptureVideo_Ogg_EndVideo;
	cls.capturevideo.writeVideoFrame = SCR_CaptureVideo_Ogg_VideoFrames;
	cls.capturevideo.writeSoundFrame = SCR_CaptureVideo_Ogg_SoundFrame;
	cls.capturevideo.formatspecific = Mem_Alloc(tempmempool, sizeof(capturevideostate_ogg_formatspecific_t));
	{
		LOAD_FORMATSPECIFIC_OGG();
		int num, denom, i;
		ogg_page pg;
		ogg_packet pt, pt2, pt3;
		theora_comment tc;
		vorbis_comment vc;
		theora_info ti;
		int vp3compat;

		format->serial1 = rand();
		qogg_stream_init(&format->to, format->serial1);

		if(cls.capturevideo.soundrate)
		{
			do
			{
				format->serial2 = rand();
			}
			while(format->serial1 == format->serial2);
			qogg_stream_init(&format->vo, format->serial2);
		}

		format->videopage.len = format->audiopage.len = 0;

		qtheora_info_init(&ti);
		ti.frame_width = cls.capturevideo.width;
		ti.frame_height = cls.capturevideo.height;
		ti.width = (ti.frame_width + 15) & ~15;
		ti.height = (ti.frame_height + 15) & ~15;
		//ti.offset_x = ((ti.width - ti.frame_width) / 2) & ~1;
		//ti.offset_y = ((ti.height - ti.frame_height) / 2) & ~1;

		for(i = 0; i < 2; ++i)
		{
			format->yuv[i].y_width = ti.width;
			format->yuv[i].y_height = ti.height;
			format->yuv[i].y_stride = ti.width;
			format->yuv[i].uv_width = ti.width / 2;
			format->yuv[i].uv_height = ti.height / 2;
			format->yuv[i].uv_stride = ti.width / 2;
			format->yuv[i].y = (unsigned char *) Mem_Alloc(tempmempool, format->yuv[i].y_stride * format->yuv[i].y_height);
			format->yuv[i].u = (unsigned char *) Mem_Alloc(tempmempool, format->yuv[i].uv_stride * format->yuv[i].uv_height);
			format->yuv[i].v = (unsigned char *) Mem_Alloc(tempmempool, format->yuv[i].uv_stride * format->yuv[i].uv_height);
		}
		format->yuvi = -1; // -1: no frame valid yet, write into 0

		FindFraction(cls.capturevideo.framerate / cls.capturevideo.framestep, &num, &denom, 1001);
		ti.fps_numerator = num;
		ti.fps_denominator = denom;

		FindFraction(1 / vid_pixelheight.value, &num, &denom, 1000);
		ti.aspect_numerator = num;
		ti.aspect_denominator = denom;

		ti.colorspace = OC_CS_UNSPECIFIED;
		ti.pixelformat = OC_PF_420;

		ti.quick_p = true; // http://mlblog.osdir.com/multimedia.ogg.theora.general/2004-07/index.shtml
		ti.dropframes_p = false;

		ti.target_bitrate = cl_capturevideo_ogg_theora_bitrate.integer * 1000;
		ti.quality = cl_capturevideo_ogg_theora_quality.integer;

		if(ti.target_bitrate <= 0)
		{
			ti.target_bitrate = -1;
			ti.keyframe_data_target_bitrate = (unsigned int)-1;
		}
		else
		{
			ti.keyframe_data_target_bitrate = (int) (ti.target_bitrate * max(1, cl_capturevideo_ogg_theora_keyframe_bitrate_multiplier.value));

			if(ti.target_bitrate < 45000 || ti.target_bitrate > 2000000)
				Con_DPrintf("WARNING: requesting an odd bitrate for theora (sensible values range from 45 to 2000 kbps)\n");
		}

		if(ti.quality < 0 || ti.quality > 63)
		{
			ti.quality = 63;
			if(ti.target_bitrate <= 0)
			{
				ti.target_bitrate = 0x7FFFFFFF;
				ti.keyframe_data_target_bitrate = 0x7FFFFFFF;
			}
		}

		// this -1 magic is because ti.keyframe_frequency and ti.keyframe_mindistance use different metrics
		ti.keyframe_frequency = bound(1, cl_capturevideo_ogg_theora_keyframe_maxinterval.integer, 1000);
		ti.keyframe_mindistance = bound(1, cl_capturevideo_ogg_theora_keyframe_mininterval.integer, (int) ti.keyframe_frequency) - 1;
		ti.noise_sensitivity = bound(0, cl_capturevideo_ogg_theora_noise_sensitivity.integer, 6);
		ti.sharpness = bound(0, cl_capturevideo_ogg_theora_sharpness.integer, 2);
		ti.keyframe_auto_threshold = bound(0, cl_capturevideo_ogg_theora_keyframe_auto_threshold.integer, 100);

		ti.keyframe_frequency_force = ti.keyframe_frequency;
		ti.keyframe_auto_p = (ti.keyframe_frequency != ti.keyframe_mindistance + 1);

		qtheora_encode_init(&format->ts, &ti);
		qtheora_info_clear(&ti);

		if(cl_capturevideo_ogg_theora_vp3compat.integer)
		{
			vp3compat = 1;
			qtheora_control(&format->ts, TH_ENCCTL_SET_VP3_COMPATIBLE, &vp3compat, sizeof(vp3compat));
			if(!vp3compat)
				Con_DPrintf("Warning: theora stream is not fully VP3 compatible\n");
		}

		// vorbis?
		if(cls.capturevideo.soundrate)
		{
			qvorbis_info_init(&format->vi);
			qvorbis_encode_init_vbr(&format->vi, cls.capturevideo.soundchannels, cls.capturevideo.soundrate, bound(-1, cl_capturevideo_ogg_vorbis_quality.value, 10) * 0.099);
			qvorbis_comment_init(&vc);
			qvorbis_analysis_init(&format->vd, &format->vi);
			qvorbis_block_init(&format->vd, &format->vb);
		}

		qtheora_comment_init(&tc);

		/* create the remaining theora headers */
		qtheora_encode_header(&format->ts, &pt);
		qogg_stream_packetin(&format->to, &pt);
		if (qogg_stream_pageout (&format->to, &pg) != 1)
			fprintf (stderr, "Internal Ogg library error.\n");
		FS_Write(cls.capturevideo.videofile, pg.header, pg.header_len);
		FS_Write(cls.capturevideo.videofile, pg.body, pg.body_len);

		qtheora_encode_comment(&tc, &pt);
		qogg_stream_packetin(&format->to, &pt);
		qtheora_encode_tables(&format->ts, &pt);
		qogg_stream_packetin (&format->to, &pt);

		qtheora_comment_clear(&tc);

		if(cls.capturevideo.soundrate)
		{
			qvorbis_analysis_headerout(&format->vd, &vc, &pt, &pt2, &pt3);
			qogg_stream_packetin(&format->vo, &pt);
			if (qogg_stream_pageout (&format->vo, &pg) != 1)
				fprintf (stderr, "Internal Ogg library error.\n");
			FS_Write(cls.capturevideo.videofile, pg.header, pg.header_len);
			FS_Write(cls.capturevideo.videofile, pg.body, pg.body_len);

			qogg_stream_packetin(&format->vo, &pt2);
			qogg_stream_packetin(&format->vo, &pt3);

			qvorbis_comment_clear(&vc);
		}

		for(;;)
		{
			int result = qogg_stream_flush (&format->to, &pg);
			if (result < 0)
				fprintf (stderr, "Internal Ogg library error.\n"); // TODO Sys_Error
			if (result <= 0)
				break;
			FS_Write(cls.capturevideo.videofile, pg.header, pg.header_len);
			FS_Write(cls.capturevideo.videofile, pg.body, pg.body_len);
		}

		if(cls.capturevideo.soundrate)
		for(;;)
		{
			int result = qogg_stream_flush (&format->vo, &pg);
			if (result < 0)
				fprintf (stderr, "Internal Ogg library error.\n"); // TODO Sys_Error
			if (result <= 0)
				break;
			FS_Write(cls.capturevideo.videofile, pg.header, pg.header_len);
			FS_Write(cls.capturevideo.videofile, pg.body, pg.body_len);
		}
	}
}
