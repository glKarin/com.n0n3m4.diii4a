/* FreeType 2 definitions from the freetype header mostly.
 */

#ifndef FT2_DEFS_H_H__
#define FT2_DEFS_H_H__

#ifdef _MSC_VER
typedef __int32 FT_Int32;
typedef unsigned __int32 FT_UInt32;
#else
# include <stdint.h>
typedef int32_t FT_Int32;
typedef uint32_t FT_UInt32;
#endif

typedef int FT_Error;

typedef signed char FT_Char;
typedef unsigned char FT_Byte;
typedef const FT_Byte *FT_Bytes;
typedef char FT_String;
typedef signed short FT_Short;
typedef unsigned short FT_UShort;
typedef signed int FT_Int;
typedef unsigned int FT_UInt;
typedef signed long FT_Long;
typedef signed long FT_Fixed;
typedef unsigned long FT_ULong;
typedef void *FT_Pointer;
typedef size_t FT_Offset;
typedef signed long FT_F26Dot6;

typedef void *FT_Stream;
typedef void *FT_Module;
typedef void *FT_Library;
typedef struct FT_FaceRec_ *FT_Face;
typedef struct FT_CharMapRec_*  FT_CharMap;
typedef struct FT_SizeRec_*  FT_Size;
typedef struct FT_Size_InternalRec_*  FT_Size_Internal;
typedef struct FT_GlyphSlotRec_*  FT_GlyphSlot;
typedef struct FT_SubGlyphRec_*  FT_SubGlyph;
typedef struct FT_Slot_InternalRec_* FT_Slot_Internal;

// Taken from the freetype headers:
typedef signed long FT_Pos;
typedef struct FT_Vector_
{
	FT_Pos x;
	FT_Pos y;
} FT_Vector;

typedef struct  FT_BBox_
{
	FT_Pos  xMin, yMin;
	FT_Pos  xMax, yMax;
} FT_BBox;

typedef enum  FT_Pixel_Mode_
{
	FT_PIXEL_MODE_NONE = 0,
	FT_PIXEL_MODE_MONO,
	FT_PIXEL_MODE_GRAY,
	FT_PIXEL_MODE_GRAY2,
	FT_PIXEL_MODE_GRAY4,
	FT_PIXEL_MODE_LCD,
	FT_PIXEL_MODE_LCD_V,
	FT_PIXEL_MODE_MAX      /* do not remove */
} FT_Pixel_Mode;
typedef enum  FT_Render_Mode_
{
	FT_RENDER_MODE_NORMAL = 0,
	FT_RENDER_MODE_LIGHT,
	FT_RENDER_MODE_MONO,
	FT_RENDER_MODE_LCD,
	FT_RENDER_MODE_LCD_V,

	FT_RENDER_MODE_MAX
} FT_Render_Mode;

#define ft_pixel_mode_none   FT_PIXEL_MODE_NONE
#define ft_pixel_mode_mono   FT_PIXEL_MODE_MONO
#define ft_pixel_mode_grays  FT_PIXEL_MODE_GRAY
#define ft_pixel_mode_pal2   FT_PIXEL_MODE_GRAY2
#define ft_pixel_mode_pal4   FT_PIXEL_MODE_GRAY4

typedef struct  FT_Bitmap_
{
	int             rows;
	int             width;
	int             pitch;
	unsigned char*  buffer;
	short           num_grays;
	char            pixel_mode;
	char            palette_mode;
	void*           palette;
} FT_Bitmap;

typedef struct  FT_Outline_
{
	short       n_contours;      /* number of contours in glyph        */
	short       n_points;        /* number of points in the glyph      */

	FT_Vector*  points;          /* the outline's points               */
	char*       tags;            /* the points flags                   */
	short*      contours;        /* the contour end points             */

	int         flags;           /* outline masks                      */
} FT_Outline;

#define FT_OUTLINE_NONE             0x0
#define FT_OUTLINE_OWNER            0x1
#define FT_OUTLINE_EVEN_ODD_FILL    0x2
#define FT_OUTLINE_REVERSE_FILL     0x4
#define FT_OUTLINE_IGNORE_DROPOUTS  0x8
#define FT_OUTLINE_SMART_DROPOUTS   0x10
#define FT_OUTLINE_INCLUDE_STUBS    0x20

#define FT_OUTLINE_HIGH_PRECISION   0x100
#define FT_OUTLINE_SINGLE_PASS      0x200

#define ft_outline_none             FT_OUTLINE_NONE
#define ft_outline_owner            FT_OUTLINE_OWNER
#define ft_outline_even_odd_fill    FT_OUTLINE_EVEN_ODD_FILL
#define ft_outline_reverse_fill     FT_OUTLINE_REVERSE_FILL
#define ft_outline_ignore_dropouts  FT_OUTLINE_IGNORE_DROPOUTS
#define ft_outline_high_precision   FT_OUTLINE_HIGH_PRECISION
#define ft_outline_single_pass      FT_OUTLINE_SINGLE_PASS

#define FT_CURVE_TAG( flag )  ( flag & 3 )

#define FT_CURVE_TAG_ON           1
#define FT_CURVE_TAG_CONIC        0
#define FT_CURVE_TAG_CUBIC        2

#define FT_CURVE_TAG_TOUCH_X      8  /* reserved for the TrueType hinter */
#define FT_CURVE_TAG_TOUCH_Y     16  /* reserved for the TrueType hinter */

#define FT_CURVE_TAG_TOUCH_BOTH  ( FT_CURVE_TAG_TOUCH_X | \
                                   FT_CURVE_TAG_TOUCH_Y )

#define FT_Curve_Tag_On       FT_CURVE_TAG_ON
#define FT_Curve_Tag_Conic    FT_CURVE_TAG_CONIC
#define FT_Curve_Tag_Cubic    FT_CURVE_TAG_CUBIC
#define FT_Curve_Tag_Touch_X  FT_CURVE_TAG_TOUCH_X
#define FT_Curve_Tag_Touch_Y  FT_CURVE_TAG_TOUCH_Y

typedef int
(*FT_Outline_MoveToFunc)( const FT_Vector*  to,
			  void*             user );
#define FT_Outline_MoveTo_Func  FT_Outline_MoveToFunc

typedef int
(*FT_Outline_LineToFunc)( const FT_Vector*  to,
			  void*             user );
#define FT_Outline_LineTo_Func  FT_Outline_LineToFunc

typedef int
(*FT_Outline_ConicToFunc)( const FT_Vector*  control,
			   const FT_Vector*  to,
			   void*             user );
#define FT_Outline_ConicTo_Func  FT_Outline_ConicToFunc

typedef int
(*FT_Outline_CubicToFunc)( const FT_Vector*  control1,
			   const FT_Vector*  control2,
			   const FT_Vector*  to,
			   void*             user );
#define FT_Outline_CubicTo_Func  FT_Outline_CubicToFunc

typedef struct  FT_Outline_Funcs_
{
	FT_Outline_MoveToFunc   move_to;
	FT_Outline_LineToFunc   line_to;
	FT_Outline_ConicToFunc  conic_to;
	FT_Outline_CubicToFunc  cubic_to;

	int                     shift;
	FT_Pos                  delta;
} FT_Outline_Funcs;

#ifndef FT_IMAGE_TAG
#define FT_IMAGE_TAG( value, _x1, _x2, _x3, _x4 )  \
          value = ( ( (unsigned long)_x1 << 24 ) | \
                    ( (unsigned long)_x2 << 16 ) | \
                    ( (unsigned long)_x3 << 8  ) | \
                      (unsigned long)_x4         )
#endif /* FT_IMAGE_TAG */

typedef enum  FT_Glyph_Format_
{
	FT_IMAGE_TAG( FT_GLYPH_FORMAT_NONE, 0, 0, 0, 0 ),

	FT_IMAGE_TAG( FT_GLYPH_FORMAT_COMPOSITE, 'c', 'o', 'm', 'p' ),
	FT_IMAGE_TAG( FT_GLYPH_FORMAT_BITMAP,    'b', 'i', 't', 's' ),
	FT_IMAGE_TAG( FT_GLYPH_FORMAT_OUTLINE,   'o', 'u', 't', 'l' ),
	FT_IMAGE_TAG( FT_GLYPH_FORMAT_PLOTTER,   'p', 'l', 'o', 't' )
} FT_Glyph_Format;
#define ft_glyph_format_none       FT_GLYPH_FORMAT_NONE
#define ft_glyph_format_composite  FT_GLYPH_FORMAT_COMPOSITE
#define ft_glyph_format_bitmap     FT_GLYPH_FORMAT_BITMAP
#define ft_glyph_format_outline    FT_GLYPH_FORMAT_OUTLINE
#define ft_glyph_format_plotter    FT_GLYPH_FORMAT_PLOTTER

typedef struct  FT_Glyph_Metrics_
{
	FT_Pos  width;
	FT_Pos  height;

	FT_Pos  horiBearingX;
	FT_Pos  horiBearingY;
	FT_Pos  horiAdvance;

	FT_Pos  vertBearingX;
	FT_Pos  vertBearingY;
	FT_Pos  vertAdvance;
} FT_Glyph_Metrics;

#define FT_EXPORT( x )  x

#define FT_OPEN_MEMORY    0x1
#define FT_OPEN_STREAM    0x2
#define FT_OPEN_PATHNAME  0x4
#define FT_OPEN_DRIVER    0x8
#define FT_OPEN_PARAMS    0x10

typedef struct  FT_Parameter_
{
	FT_ULong    tag;
	FT_Pointer  data;
} FT_Parameter;

typedef struct  FT_Open_Args_
{
	FT_UInt         flags;
	const FT_Byte*  memory_base;
	FT_Long         memory_size;
	FT_String*      pathname;
	FT_Stream       stream;
	FT_Module       driver;
	FT_Int          num_params;
	FT_Parameter*   params;
} FT_Open_Args;
typedef enum  FT_Size_Request_Type_
{
	FT_SIZE_REQUEST_TYPE_NOMINAL,
	FT_SIZE_REQUEST_TYPE_REAL_DIM,
	FT_SIZE_REQUEST_TYPE_BBOX,
	FT_SIZE_REQUEST_TYPE_CELL,
	FT_SIZE_REQUEST_TYPE_SCALES,

	FT_SIZE_REQUEST_TYPE_MAX

} FT_Size_Request_Type;
typedef struct  FT_Size_RequestRec_
{
	FT_Size_Request_Type  type;
	FT_Long               width;
	FT_Long               height;
	FT_UInt               horiResolution;
	FT_UInt               vertResolution;
} FT_Size_RequestRec;
typedef struct FT_Size_RequestRec_  *FT_Size_Request;

#define FT_LOAD_DEFAULT                      0x0
#define FT_LOAD_NO_SCALE                     0x1
#define FT_LOAD_NO_HINTING                   0x2
#define FT_LOAD_RENDER                       0x4
#define FT_LOAD_NO_BITMAP                    0x8
#define FT_LOAD_VERTICAL_LAYOUT              0x10
#define FT_LOAD_FORCE_AUTOHINT               0x20
#define FT_LOAD_CROP_BITMAP                  0x40
#define FT_LOAD_PEDANTIC                     0x80
#define FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH  0x200
#define FT_LOAD_NO_RECURSE                   0x400
#define FT_LOAD_IGNORE_TRANSFORM             0x800
#define FT_LOAD_MONOCHROME                   0x1000
#define FT_LOAD_LINEAR_DESIGN                0x2000
#define FT_LOAD_NO_AUTOHINT                  0x8000U

#define FT_LOAD_TARGET_( x )   ( (FT_Int32)( (x) & 15 ) << 16 )

#define FT_LOAD_TARGET_NORMAL  FT_LOAD_TARGET_( FT_RENDER_MODE_NORMAL )
#define FT_LOAD_TARGET_LIGHT   FT_LOAD_TARGET_( FT_RENDER_MODE_LIGHT  )
#define FT_LOAD_TARGET_MONO    FT_LOAD_TARGET_( FT_RENDER_MODE_MONO   )
#define FT_LOAD_TARGET_LCD     FT_LOAD_TARGET_( FT_RENDER_MODE_LCD    )
#define FT_LOAD_TARGET_LCD_V   FT_LOAD_TARGET_( FT_RENDER_MODE_LCD_V  )

#define FT_ENC_TAG( value, a, b, c, d )	      \
	value = ( ( (FT_UInt32)(a) << 24 ) |			  \
		  ( (FT_UInt32)(b) << 16 ) |				\
		  ( (FT_UInt32)(c) <<  8 ) |				\
		  (FT_UInt32)(d)         )

typedef enum  FT_Encoding_
{
	FT_ENC_TAG( FT_ENCODING_NONE, 0, 0, 0, 0 ),

	FT_ENC_TAG( FT_ENCODING_MS_SYMBOL, 's', 'y', 'm', 'b' ),
	FT_ENC_TAG( FT_ENCODING_UNICODE,   'u', 'n', 'i', 'c' ),

	FT_ENC_TAG( FT_ENCODING_SJIS,    's', 'j', 'i', 's' ),
	FT_ENC_TAG( FT_ENCODING_GB2312,  'g', 'b', ' ', ' ' ),
	FT_ENC_TAG( FT_ENCODING_BIG5,    'b', 'i', 'g', '5' ),
	FT_ENC_TAG( FT_ENCODING_WANSUNG, 'w', 'a', 'n', 's' ),
	FT_ENC_TAG( FT_ENCODING_JOHAB,   'j', 'o', 'h', 'a' ),

	/* for backwards compatibility */
	FT_ENCODING_MS_SJIS    = FT_ENCODING_SJIS,
	FT_ENCODING_MS_GB2312  = FT_ENCODING_GB2312,
	FT_ENCODING_MS_BIG5    = FT_ENCODING_BIG5,
	FT_ENCODING_MS_WANSUNG = FT_ENCODING_WANSUNG,
	FT_ENCODING_MS_JOHAB   = FT_ENCODING_JOHAB,

	FT_ENC_TAG( FT_ENCODING_ADOBE_STANDARD, 'A', 'D', 'O', 'B' ),
	FT_ENC_TAG( FT_ENCODING_ADOBE_EXPERT,   'A', 'D', 'B', 'E' ),
	FT_ENC_TAG( FT_ENCODING_ADOBE_CUSTOM,   'A', 'D', 'B', 'C' ),
	FT_ENC_TAG( FT_ENCODING_ADOBE_LATIN_1,  'l', 'a', 't', '1' ),

	FT_ENC_TAG( FT_ENCODING_OLD_LATIN_2, 'l', 'a', 't', '2' ),

	FT_ENC_TAG( FT_ENCODING_APPLE_ROMAN, 'a', 'r', 'm', 'n' )
} FT_Encoding;

#define ft_encoding_none            FT_ENCODING_NONE
#define ft_encoding_unicode         FT_ENCODING_UNICODE
#define ft_encoding_symbol          FT_ENCODING_MS_SYMBOL
#define ft_encoding_latin_1         FT_ENCODING_ADOBE_LATIN_1
#define ft_encoding_latin_2         FT_ENCODING_OLD_LATIN_2
#define ft_encoding_sjis            FT_ENCODING_SJIS
#define ft_encoding_gb2312          FT_ENCODING_GB2312
#define ft_encoding_big5            FT_ENCODING_BIG5
#define ft_encoding_wansung         FT_ENCODING_WANSUNG
#define ft_encoding_johab           FT_ENCODING_JOHAB

#define ft_encoding_adobe_standard  FT_ENCODING_ADOBE_STANDARD
#define ft_encoding_adobe_expert    FT_ENCODING_ADOBE_EXPERT
#define ft_encoding_adobe_custom    FT_ENCODING_ADOBE_CUSTOM
#define ft_encoding_apple_roman     FT_ENCODING_APPLE_ROMAN

typedef struct  FT_Bitmap_Size_
{
	FT_Short  height;
	FT_Short  width;

	FT_Pos    size;

	FT_Pos    x_ppem;
	FT_Pos    y_ppem;
} FT_Bitmap_Size;

typedef struct  FT_CharMapRec_
{
	FT_Face      face;
	FT_Encoding  encoding;
	FT_UShort    platform_id;
	FT_UShort    encoding_id;
} FT_CharMapRec;

typedef void  (*FT_Generic_Finalizer)(void*  object);
typedef struct  FT_Generic_
{
	void*                 data;
	FT_Generic_Finalizer  finalizer;
} FT_Generic;

typedef struct  FT_Size_Metrics_
{
	FT_UShort  x_ppem;      /* horizontal pixels per EM               */
	FT_UShort  y_ppem;      /* vertical pixels per EM                 */

	FT_Fixed   x_scale;     /* scaling values used to convert font    */
	FT_Fixed   y_scale;     /* units to 26.6 fractional pixels        */

	FT_Pos     ascender;    /* ascender in 26.6 frac. pixels          */
	FT_Pos     descender;   /* descender in 26.6 frac. pixels         */
	FT_Pos     height;      /* text height in 26.6 frac. pixels       */
	FT_Pos     max_advance; /* max horizontal advance, in 26.6 pixels */
} FT_Size_Metrics;

typedef struct  FT_SizeRec_
{
	FT_Face           face;      /* parent face object              */
	FT_Generic        generic;   /* generic pointer for client uses */
	FT_Size_Metrics   metrics;   /* size metrics                    */
	FT_Size_Internal  internal;
} FT_SizeRec;

typedef struct  FT_FaceRec_
{
	FT_Long           num_faces;
	FT_Long           face_index;

	FT_Long           face_flags;
	FT_Long           style_flags;

	FT_Long           num_glyphs;

	FT_String*        family_name;
	FT_String*        style_name;

	FT_Int            num_fixed_sizes;
	FT_Bitmap_Size*   available_sizes;

	FT_Int            num_charmaps;
	FT_CharMap*       charmaps;

	FT_Generic        generic;

	/*# The following member variables (down to `underline_thickness') */
	/*# are only relevant to scalable outlines; cf. @FT_Bitmap_Size    */
	/*# for bitmap fonts.                                              */
	FT_BBox           bbox;

	FT_UShort         units_per_EM;
	FT_Short          ascender;
	FT_Short          descender;
	FT_Short          height;

	FT_Short          max_advance_width;
	FT_Short          max_advance_height;

	FT_Short          underline_position;
	FT_Short          underline_thickness;

	FT_GlyphSlot      glyph;
	FT_Size           size;
	FT_CharMap        charmap;

	/* ft2 private
	FT_Driver         driver;
	FT_Memory         memory;
	FT_Stream         stream;

	FT_ListRec        sizes_list;

	FT_Generic        autohint;
	void*             extensions;

	FT_Face_Internal  internal;
	*/
} FT_FaceRec;

typedef struct  FT_GlyphSlotRec_
{
	FT_Library        library;
	FT_Face           face;
	FT_GlyphSlot      next;
	FT_UInt           reserved;       /* retained for binary compatibility */
	FT_Generic        generic;

	FT_Glyph_Metrics  metrics;
	FT_Fixed          linearHoriAdvance;
	FT_Fixed          linearVertAdvance;
	FT_Vector         advance;

	FT_Glyph_Format   format;

	FT_Bitmap         bitmap;
	FT_Int            bitmap_left;
	FT_Int            bitmap_top;

	FT_Outline        outline;

	FT_UInt           num_subglyphs;
	FT_SubGlyph       subglyphs;

	void*             control_data;
	long              control_len;

	FT_Pos            lsb_delta;
	FT_Pos            rsb_delta;

	void*             other;

	FT_Slot_Internal  internal;
} FT_GlyphSlotRec;

#define FT_FACE_FLAG_SCALABLE          ( 1L <<  0 )
#define FT_FACE_FLAG_FIXED_SIZES       ( 1L <<  1 )
#define FT_FACE_FLAG_FIXED_WIDTH       ( 1L <<  2 )
#define FT_FACE_FLAG_SFNT              ( 1L <<  3 )
#define FT_FACE_FLAG_HORIZONTAL        ( 1L <<  4 )
#define FT_FACE_FLAG_VERTICAL          ( 1L <<  5 )
#define FT_FACE_FLAG_KERNING           ( 1L <<  6 )
#define FT_FACE_FLAG_FAST_GLYPHS       ( 1L <<  7 )
#define FT_FACE_FLAG_MULTIPLE_MASTERS  ( 1L <<  8 )
#define FT_FACE_FLAG_GLYPH_NAMES       ( 1L <<  9 )
#define FT_FACE_FLAG_EXTERNAL_STREAM   ( 1L << 10 )
#define FT_FACE_FLAG_HINTER            ( 1L << 11 )
#define FT_FACE_FLAG_CID_KEYED         ( 1L << 12 )
#define FT_FACE_FLAG_TRICKY            ( 1L << 13 )

typedef enum  FT_Kerning_Mode_
{
	FT_KERNING_DEFAULT  = 0,
	FT_KERNING_UNFITTED,
	FT_KERNING_UNSCALED
} FT_Kerning_Mode;

#endif // FT2_DEFS_H_H__
