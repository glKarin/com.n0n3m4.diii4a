/* Header for FreeType 2 and UTF-8 encoding support for
 * DarkPlaces
 */

#ifndef DP_FREETYPE2_H__
#define DP_FREETYPE2_H__

//#include <sys/types.h>

#include "utf8lib.h"

/* 
 * From http://www.unicode.org/Public/UNIDATA/Blocks.txt
 *
 *   E000..F8FF; Private Use Area
 *   F0000..FFFFF; Supplementary Private Use Area-A
 *
 * We use:
 *   Range E000 - E0FF
 *     Contains the non-FreeType2 version of characters.
 */

typedef struct ft2_font_map_s ft2_font_map_t;
typedef struct font_incmap_s font_incmap_t;
typedef struct incmap_lookup_cache_s incmap_lookup_cache_t;
typedef struct ft2_attachment_s ft2_attachment_t;
#ifdef WIN64
#define ft2_oldstyle_map ((ft2_font_map_t*)-1LL)
#else
#define ft2_oldstyle_map ((ft2_font_map_t*)-1)
#endif

typedef float ft2_kernvec[2];
typedef struct ft2_kerning_s
{
	ft2_kernvec kerning[256][256]; /* kerning[left char][right char] */
} ft2_kerning_t;

typedef struct ft2_font_s
{
	char   name[64];
	qbool  has_kerning;
	// last requested size loaded using Font_SetSize
	float  currentw;
	float  currenth;
	float  ascend;
	float  descend;
	qbool  image_font; // only fallbacks are freetype fonts

	// TODO: clean this up and do not expose everything.

	const unsigned char  *data; // FT2 needs it to stay
	//fs_offset_t     datasize;
	void                 *face;

	// an unordered array of ordered linked lists of glyph maps for a specific size
	ft2_font_map_t       *font_maps[MAX_FONT_SIZES];
	int                   num_sizes;

	// attachments
	size_t                attachmentcount;
	ft2_attachment_t     *attachments;

	ft2_settings_t       *settings;

	// fallback mechanism
	struct ft2_font_s    *next;
} ft2_font_t;

void            Font_CloseLibrary(void);
void            Font_Init(void);
qbool           Font_OpenLibrary(void);
ft2_font_t*     Font_Alloc(void);
void            Font_UnloadFont(ft2_font_t *font);
// IndexForSize suggests to change the width and height if a font size is in a reasonable range
// for example, you render at a size of 12.4, and a font of size 12 has been loaded
// in such a case, *outw and *outh are set to 12, which is often a good alternative size
int             Font_IndexForSize(ft2_font_t *font, float size, float *outw, float *outh);
ft2_font_map_t *Font_MapForIndex(ft2_font_t *font, int index);
qbool           Font_LoadFont(const char *name, dp_font_t *dpfnt);
qbool           Font_GetKerningForSize(ft2_font_t *font, float w, float h, Uchar left, Uchar right, float *outx, float *outy);
qbool           Font_GetKerningForMap(ft2_font_t *font, int map_index, float w, float h, Uchar left, Uchar right, float *outx, float *outy);
float           Font_VirtualToRealSize(float sz);
float           Font_SnapTo(float val, float snapwidth);
// since this is used on a font_map_t, let's name it FontMap_*
ft2_font_map_t *FontMap_FindForChar(ft2_font_map_t *start, Uchar ch);
qbool           Font_GetMapForChar(ft2_font_t *font, int map_index, Uchar ch, ft2_font_map_t **outmap, int *outmapch);
#endif // DP_FREETYPE2_H__
