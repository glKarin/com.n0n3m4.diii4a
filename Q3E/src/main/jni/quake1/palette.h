
#ifndef PALLETE_H
#define PALLETE_H

#define PALETTEFEATURE_STANDARD 1
#define PALETTEFEATURE_REVERSED 2
#define PALETTEFEATURE_PANTS 4
#define PALETTEFEATURE_SHIRT 8
#define PALETTEFEATURE_GLOW 16
#define PALETTEFEATURE_ZERO 32
#define PALETTEFEATURE_TRANSPARENT 128

extern unsigned char palette_rgb[256][3];
extern unsigned char palette_rgb_pantscolormap[16][3];
extern unsigned char palette_rgb_shirtcolormap[16][3];
extern unsigned char palette_rgb_pantsscoreboard[16][3];
extern unsigned char palette_rgb_shirtscoreboard[16][3];

extern unsigned int palette_bgra_complete[256];
extern unsigned int palette_bgra_font[256];
extern unsigned int palette_bgra_alpha[256];
extern unsigned int palette_bgra_nocolormap[256];
extern unsigned int palette_bgra_nocolormapnofullbrights[256];
extern unsigned int palette_bgra_nofullbrights[256];
extern unsigned int palette_bgra_nofullbrights_transparent[256];
extern unsigned int palette_bgra_onlyfullbrights[256];
extern unsigned int palette_bgra_onlyfullbrights_transparent[256];
extern unsigned int palette_bgra_pantsaswhite[256];
extern unsigned int palette_bgra_shirtaswhite[256];
extern unsigned int palette_bgra_transparent[256];
extern unsigned int palette_bgra_embeddedpic[256];
extern unsigned char palette_featureflags[256];

extern unsigned int q2palette_bgra_complete[256];

// used by hardware gamma functions in vid_* files
void BuildGammaTable8(float prescale, float gamma, float scale, float base, float contrastboost, unsigned char *out, int rampsize);
void BuildGammaTable16(float prescale, float gamma, float scale, float base, float contrastboost, unsigned short *out, int rampsize);

void Palette_Init(void);

#endif

