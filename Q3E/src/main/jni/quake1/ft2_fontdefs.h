#ifndef FT2_PRIVATE_H__
#define FT2_PRIVATE_H__

// anything should work, but I recommend multiples of 8
// since the texture size should be a power of 2
#define FONT_CHARS_PER_LINE 16
#define FONT_CHAR_LINES 16
#define FONT_CHARS_PER_MAP (FONT_CHARS_PER_LINE * FONT_CHAR_LINES)

// map.start value for incremental maps to hold a place
#define INCMAP_START 0x110000

typedef struct glyph_slot_s
{
	qbool image;
	// we keep the quad coords here only currently
	// if you need other info, make Font_LoadMapForIndex fill it into this slot
	float txmin; // texture coordinate in [0,1]
	float txmax;
	float tymin;
	float tymax;
	float vxmin;
	float vxmax;
	float vymin;
	float vymax;
	float advance_x;
	float advance_y;
} glyph_slot_t;

struct ft2_font_map_s
{
	Uchar  start;
	float  size;

	// the actual size used in the freetype code
	// by convention, the requested size is the height of the font's bounding box.
	float  intSize;
	int    glyphSize;

	ft2_font_map_t *next;
	cachepic_t     *pic;
	qbool           static_tex;
	glyph_slot_t    glyphs[FONT_CHARS_PER_MAP];
	Uchar           glyphchars[FONT_CHARS_PER_MAP];

	// saves us the trouble of calculating these over and over again
	double          sfx, sfy;

	// note: float width_of[256] was moved to `struct dp_font_s` as width_of_ft2

	// these may only present in a startmap
	// contains the kerning information for the first 256 characters
	// for the other characters, we will lookup the kerning information
	ft2_kerning_t  *kerning;
	// for accessing incremental maps for bigblock glyphs
	font_incmap_t  *incmap;
};

struct font_incmap_s
{
	// associated fontmap; startmap of incmaps
	struct ft2_font_map_s *fontmap;
	int charcount;
	int newmap_start;

	// two rounds of merge will take place, keep those data until then
	unsigned char *data_tier1[FONT_CHARS_PER_LINE];
	unsigned char *data_tier2[FONT_CHAR_LINES];

	// count of merged maps
	int tier1_merged, tier2_merged;
};

struct ft2_attachment_s
{
	const unsigned char *data;
	fs_offset_t          size;
};

//qbool Font_LoadMapForIndex(ft2_font_t *font, Uchar _ch, ft2_font_map_t **outmap);
qbool Font_LoadMapForIndex(ft2_font_t *font, int map_index, Uchar _ch, ft2_font_map_t **outmap);

void font_start(void);
void font_shutdown(void);
void font_newmap(void);

#endif // FT2_PRIVATE_H__
