
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ERRORS_H
#include FT_SYSTEM_H
#include FT_IMAGE_H
#include FT_OUTLINE_H
#include FT_GLYPH_H

#define _FLOOR(x)  ((x) & -64)
#define _CEIL(x)   (((x)+63) & -64)
#define _TRUNC(x)  ((x) >> 6)

typedef uint32_t fontChar_t;

typedef struct {
    int					height;			// number of scan lines
    int					top;			// top of glyph in buffer
    int					bottom;			// bottom of glyph in buffer
    int					pitch;			// width for copying
    int					xSkip;			// x adjustment
    int					imageWidth;		// width of actual image
    int					imageHeight;	// height of actual image
    float				s;				// x offset in image where glyph starts
    float				t;				// y offset in image where glyph starts
    float				s2;
    float				t2;
    int 	            glyph;			// shader with the glyph 4 bytes
    char				shaderName[32];
} glyphInfoExport_t;

typedef struct {
    glyphInfoExport_t	glyphs[GLYPHS_PER_FONT];
    float				glyphScale;
    char				name[64];
} fontInfoExport_t;

typedef struct {
    fontInfoExport_t    base;

    unsigned int		magic;
    unsigned int		version;
    int                 numFiles;
    int					width;
    int					height;
    int                 numIndexes;
    int					*indexes;
    int                 numGlyphs;
    glyphInfoExport_t	*glyphsTable;
} wfontInfo_t;

#ifdef _RAVEN //k: quake4 font
#pragma pack( push, 1 )
typedef struct {
    float				imageWidth;		// width of actual image
    float				imageHeight;	// height of actual image
    float				xSkip;			// x adjustment
    float				pitch;			// width for copying
    float				top;			// top of glyph in buffer
    float				s;				// x offset in image where glyph starts
    float				t;				// y offset in image where glyph starts
    float				s2;
    float				t2;
} q4_glyphInfoExport_t;

typedef struct { // sizeof == 9236, non-align
    q4_glyphInfoExport_t	glyphs[GLYPHS_PER_FONT];
    float                   pointSize;
    float                   maxWidth;
    float                   maxHeight;
    float                   placeholder1; // unknown
    float                   placeholder2; // unknown
} q4_fontInfoExport_t;
#pragma pack( pop )

static void R_Font_ConvertToQ4Glyph(const glyphInfoExport_t &d3, q4_glyphInfoExport_t &q4)
{
    q4.imageWidth = (float)d3.imageWidth;
    q4.imageHeight = (float)d3.imageHeight;
    q4.xSkip = (float)d3.xSkip;
    q4.pitch = (float)d3.pitch;
    q4.top = (float)d3.top;
    q4.s = (float)d3.s;
    q4.t = (float)d3.t;
    q4.s2 = (float)d3.s2;
    q4.t2 = (float)d3.t2;
}

static void R_Font_ConvertToQ4Info(const fontInfoExport_t &d3, q4_fontInfoExport_t &q4)
{
    int i;
    int mw = 0;
    int mh = 0;

    for(i = 0; i < GLYPHS_PER_FONT; i++)
        R_Font_ConvertToQ4Glyph(d3.glyphs[i], q4.glyphs[i]);
    q4.pointSize = 0.0f;
    for (i = GLYPH_START; i < GLYPH_END; i++) {
        if (mh < d3.glyphs[i].height) {
            mh = d3.glyphs[i].height;
        }

        if (mw < d3.glyphs[i].xSkip) {
            mw = d3.glyphs[i].xSkip;
        }
    }
    q4.maxWidth = (float)mw;
    q4.maxHeight = (float)mh;
    q4.placeholder1 = 0.0f;
    q4.placeholder2 = 0.0f;
}
#endif

typedef struct ftGlobalVars_s
{
    FT_Library ftLibrary;
    void *faceData;
    int len;
} ftGlobalVars_t;

static void R_Font_InitExporter(ftGlobalVars_t *exporter)
{
    exporter->ftLibrary = NULL;
    exporter->faceData = NULL;
    exporter->len = 0;
}

static void R_Font_GetGlyphInfo(FT_GlyphSlot glyph, int *left, int *right, int *width, int *top, int *bottom, int *height, int *pitch)
{

    *left  = _FLOOR(glyph->metrics.horiBearingX);
    *right = _CEIL(glyph->metrics.horiBearingX + glyph->metrics.width);
    *width = _TRUNC(*right - *left);

    *top    = _CEIL(glyph->metrics.horiBearingY);
    *bottom = _FLOOR(glyph->metrics.horiBearingY - glyph->metrics.height);
    *height = _TRUNC(*top - *bottom);
    *pitch  = (true ? (*width+3) & -4 : (*width+7) >> 3);
}

static FT_Bitmap *R_Font_RenderGlyph(FT_Library ftLibrary, FT_GlyphSlot glyph, glyphInfoExport_t *glyphOut, int width256)
{
    FT_Bitmap  *bit2;
    int left, right, width, top, bottom, height, pitch, size;

    R_Font_GetGlyphInfo(glyph, &left, &right, &width, &top, &bottom, &height, &pitch);

    if (glyph->format == ft_glyph_format_outline) {
        size   = pitch*height;

        bit2 = (FT_Bitmap *)Mem_Alloc(sizeof(FT_Bitmap));

        bit2->width      = width;
        bit2->rows       = height;
        bit2->pitch      = pitch;
        bit2->pixel_mode = ft_pixel_mode_grays;
        //bit2->pixel_mode = ft_pixel_mode_mono;
        bit2->buffer     = (unsigned char*)Mem_Alloc(pitch*height);
        bit2->num_grays = 256;


        memset(bit2->buffer, 0, size);

        FT_Outline_Translate(&glyph->outline, -left, -bottom);

        FT_Outline_Get_Bitmap(ftLibrary, &glyph->outline, bit2);

        glyphOut->height = height;
        glyphOut->pitch = pitch;
        glyphOut->top = (glyph->metrics.horiBearingY >> 6) + 1;
        glyphOut->bottom = bottom;
        return bit2;
    } else {
        //if (glyph->format != ft_glyph_format_bitmap)
        {
            FT_Error err = FT_Render_Glyph(glyph, FT_RENDER_MODE_MONO);
            if (err != 0)
                return NULL;
        }

        size   = glyph->bitmap.pitch*glyph->bitmap.rows;

        bit2 = (FT_Bitmap *)Mem_Alloc(sizeof(FT_Bitmap));

        bit2->width      = width;
        bit2->rows       = height;
        bit2->pitch      = glyph->bitmap.pitch;
        bit2->pixel_mode = glyph->bitmap.pixel_mode;
        //bit2->pixel_mode = ft_pixel_mode_mono;
        bit2->buffer     = (unsigned char*)Mem_Alloc(size);
        bit2->num_grays = glyph->bitmap.num_grays;
        memcpy(bit2->buffer, glyph->bitmap.buffer, size);

        glyphOut->height = height;
        glyphOut->pitch = pitch;
        glyphOut->top = (glyph->metrics.horiBearingY >> 6) + 1;
        glyphOut->bottom = bottom;

        return bit2;
    }

    return NULL;
}

/*
============
RE_ConstructGlyphInfo
============
*/
static glyphInfoExport_t *R_Font_ConstructGlyphInfo(FT_Library ftLibrary, unsigned char *imageOut, int *xOut, int *yOut, int *maxHeight, FT_Face face, const fontChar_t c, bool calcHeight, int width256)
{
    int i;
    static glyphInfoExport_t glyph;
    unsigned char *src, *dst;
    float scaled_width, scaled_height;
    FT_Bitmap *bitmap = NULL;
	int width255 = width256 - 1;

    memset(&glyph, 0, sizeof(glyphInfoExport_t));

    // make sure everything is here
    if (face != NULL) {
        FT_Load_Glyph(face, FT_Get_Char_Index(face, c), FT_LOAD_DEFAULT);
        bitmap = R_Font_RenderGlyph(ftLibrary, face->glyph, &glyph, width256);

        if (bitmap) {
            glyph.xSkip = (face->glyph->metrics.horiAdvance >> 6) + 1;
        } else {
            return NULL; // &glyph;
        }

        if (glyph.height > *maxHeight) {
            *maxHeight = glyph.height;
        }

        if (calcHeight) {
            Mem_Free(bitmap->buffer);
            Mem_Free(bitmap);
            return &glyph;
        }

        /*
                // need to convert to power of 2 sizes so we do not get
                // any scaling from the gl upload
                for (scaled_width = 1 ; scaled_width < glyph.pitch ; scaled_width<<=1)
                    ;
                for (scaled_height = 1 ; scaled_height < glyph.height ; scaled_height<<=1)
                    ;
        */

        scaled_width = (float)glyph.pitch;
        scaled_height = (float)glyph.height;

        // we need to make sure we fit
        if (*xOut + scaled_width + 1 >= width255) {
            if (*yOut + *maxHeight + 1 >= width255) {
                *yOut = -1;
                *xOut = -1;
                Mem_Free(bitmap->buffer);
                Mem_Free(bitmap);
                return &glyph;
            } else {
                *xOut = 0;
                *yOut += *maxHeight + 1;
            }
        } /*else*/ if (*yOut + *maxHeight + 1 >= width255) {
            *yOut = -1;
            *xOut = -1;
            Mem_Free(bitmap->buffer);
            Mem_Free(bitmap);
            return &glyph;
        }
        //wprintf(L"%lc ", (wchar_t )c);

        src = bitmap->buffer;
        dst = imageOut + (*yOut * width256) + *xOut;

        if (bitmap->pixel_mode == ft_pixel_mode_mono) {
            for (i = 0; i < glyph.height; i++) {
                int j;
                unsigned char *_src = src;
                unsigned char *_dst = dst;
                unsigned char mask = 0x80;
                unsigned char val = *_src;

                for (j = 0; j < glyph.pitch; j++) {

#if 0
                    if (mask == 0x80) {
                        val = *_src++;
                    }

                    if (val & mask) {
                        *_dst = 0xff;
                    }

                    mask >>= 1;

                    if (mask == 0) {
                        mask = 0x80;
                    }
#else
                    *_dst = bitmap->buffer[i*bitmap->pitch + (j>>3)]<<(j%8)&0x80;
#endif

                    _dst++;
                }

                src += glyph.pitch;
                dst += width256;

            }
        } else {
            for (i = 0; i < glyph.height; i++) {
                memcpy(dst, src, glyph.pitch);
                src += glyph.pitch;
                dst += width256;
            }
        }

        // we now have an 8 bit per pixel grey scale bitmap
        // that is width wide and pf->ftSize->metrics.y_ppem tall

        glyph.imageHeight = scaled_height;
        glyph.imageWidth = scaled_width;
        glyph.s = (float)*xOut / width256;
        glyph.t = (float)*yOut / width256;
        glyph.s2 = glyph.s + (float)scaled_width / width256;
        glyph.t2 = glyph.t + (float)scaled_height / width256;

        *xOut += scaled_width + 1;
    }

    Mem_Free(bitmap->buffer);
    Mem_Free(bitmap);

    return &glyph;
}

static bool R_Font_LoadFont(ftGlobalVars_t *exporter, const char *fontName)
{
    ID_TIME_T ftime;
    int len;
    FT_Library ftLibrary;
    void *faceData;

    if (FT_Init_FreeType(&ftLibrary)) {
        common->Printf("Export font: Unable to initialize FreeType.\n");
    }

    len = fileSystem->ReadFile(fontName, &faceData, &ftime);

    if (len <= 0) {
        common->Warning("Export font: Unable to read font file");
        return false;
    }

    common->Printf("Export font: Load font '%s' %d bytes.\n", fontName, len);

    exporter->ftLibrary = ftLibrary;
    exporter->faceData = faceData;
    exporter->len = len;

    return true;
}

static void R_Font_UnloadFont(ftGlobalVars_t *exporter)
{
    if(exporter->faceData)
    {
        fileSystem->FreeFile(exporter->faceData);
        exporter->faceData = NULL;
    }
    exporter->len = 0;
    if(exporter->ftLibrary)
    {
        FT_Done_FreeType(exporter->ftLibrary);
        exporter->ftLibrary = NULL;
    }
}

static bool R_Font_Export(ftGlobalVars_t *exporter, int pointSize, wfontInfo_t *font, const char *language = "english", const char *fontType = NULL, int width256 = 256)
{
    FT_Face face;
    int j, k, xOut, yOut, lastStart, imageNumber;
    int scaledSize, newSize, maxHeight, left, satLevels;
    unsigned char *out = NULL, *imageBuff;
    glyphInfoExport_t *glyph;
    float max;
    int i, len = exporter->len;
    char name[1024];
    char saveDir[1024];
    FT_Int numCharcode;
    FT_ULong *charcodes;
    FT_UInt maxCharcode;
    FT_ULong charcode;
    FT_UInt gindex;
    void *faceData = exporter->faceData;
    FT_Library ftLibrary = exporter->ftLibrary;
	int width1024 = width256 * 4;

    if (ftLibrary == NULL) {
        common->Warning("Export font: FreeType not initialized.");
        return false;
    }

    FT_UInt dpi = 72;
    float glyphScale;

    // change the scale to be relative to 1 based on 72 dpi ( so dpi of 144 means a scale of .5 )
    glyphScale = 72.0f / (float)dpi;

    // we also need to adjust the scale based on point size relative to 48 points as the ui scaling is based on a 48 point font
    glyphScale *= 48.0f / (float)pointSize;

    // allocate on the stack first in case we fail
    if (FT_New_Memory_Face(exporter->ftLibrary, (FT_Byte *)faceData, len, 0, &face)) {
        common->Warning("Export font: unable to allocate new face.");
        return false;
    }

    // select unicode charmap
    FT_CharMap found = NULL;
    for (i = 0; i < face->num_charmaps; i++) {
        FT_CharMap charmap = face->charmaps[i];
        if (charmap->encoding == FT_ENCODING_UNICODE) {
            found = charmap;
            break;
        }
    }
    if (found) {
        common->Printf("Export font: using unicode charmap.\n");
        FT_Set_Charmap(face, found);
    } else {
        common->Warning("Export font: unicode charmap not found.");
        return false;
    }

    // get max charcode and num charcode and max height
    maxHeight = 0;
    numCharcode = 0;
    maxCharcode = 0;
    gindex = 0;
    charcode = FT_Get_First_Char(face, &gindex);
    while (gindex != 0) {
        if(charcode > maxCharcode)
            maxCharcode = charcode;
        numCharcode++;
        glyph = R_Font_ConstructGlyphInfo(ftLibrary, out, &xOut, &yOut, &maxHeight, face, gindex, true, width256);
        charcode = FT_Get_Next_Char(face, charcode, &gindex);
    }

    if(!numCharcode)
    {
        common->Warning("Export font: get 0 character codes.");
        return false;
    }

    common->Printf("Export font: char count=%u, max charcode=%u, max height=%d\n", numCharcode, maxCharcode, maxHeight);

    charcodes = (FT_ULong *)malloc(numCharcode * sizeof(FT_ULong));
    i = 0;
    gindex = 0;
    charcode = FT_Get_First_Char(face, &gindex);
    while (gindex != 0) {
        charcodes[i] = charcode;
        i++;
        charcode = FT_Get_Next_Char(face, charcode, &gindex);
    }

    // set char size
    if (FT_Set_Char_Size(face, pointSize << 6, pointSize << 6, dpi, dpi)) {
        common->Warning("Export font: Unable to set face char size.");
        free(charcodes);
        return false;
    }

    saveDir[0] = '\0';
    if(fontType && fontType[0])
    {
        idStr::snPrintf(saveDir, sizeof(saveDir), "fonts/%s/%s", language, fontType);
    }
    else
    {
        idStr::snPrintf(saveDir, sizeof(saveDir), "fonts/%s", language);
    }
    common->Printf("Export font: export font file to '%s/%s'\n", saveDir, language);

    // make a 256x256 image buffer, once it is full, register it, clean it and keep going
    // until all glyphs are rendered
    out = (unsigned char *)Mem_Alloc(width1024*width1024);

    memset(out, 0, width1024*width1024);

    xOut = 0;
    yOut = 0;
    i = 0;
    lastStart = i;
    imageNumber = 0;

    memset(font, 0, sizeof(*font));

	maxCharcode += 1;
    font->numIndexes = (int)maxCharcode;
    font->indexes= (int *)malloc(maxCharcode * sizeof(int));
    font->numGlyphs = numCharcode;
    font->glyphsTable = (glyphInfoExport_t *)calloc(numCharcode, sizeof(glyphInfoExport_t));
    glyphInfoExport_t *glyphData = &font->glyphsTable[0];
	int *indexData = &font->indexes[0];
	for(j = 0; j < font->numIndexes; j++)
		indexData[j] = -1;
    int num = 0;

    scaledSize = width256*width256;
    newSize = scaledSize * 4;
    imageBuff = (unsigned char *)Mem_Alloc(newSize);

    i = 0;
    while(i < numCharcode)
    {
        charcode = charcodes[i];
        glyphInfoExport_t *glyphPtr = &glyphData[num];
        glyph = R_Font_ConstructGlyphInfo(ftLibrary, out, &xOut, &yOut, &maxHeight, face, charcode, false, width256);

        if(glyph)
		{
			if (xOut == -1 || yOut == -1 || i == numCharcode - 1)  {
				// ran out of room
				// we need to create an image from the bitmap, set all the handles in the glyphs to this point
				//

				left = 0;
				max = 0;
				satLevels = 255;

                memset(imageBuff, 0, newSize);

				for (k = 0; k < (scaledSize) ; k++) {
					if (max < out[k]) {
						max = out[k];
					}
				}

				if (max > 0) {
					max = 255/max;
				}

				for (k = 0; k < (scaledSize) ; k++) {
					imageBuff[left++] = 255;
					imageBuff[left++] = 255;
					imageBuff[left++] = 255;
					imageBuff[left++] = ((float)out[k] * max);
				}

				imageNumber++;
#ifdef _RAVEN //k: quake4 font texture file
                idStr::snPrintf(name, sizeof(name), "%s_%i_%i.tga", saveDir, imageNumber, pointSize);
#else
                idStr::snPrintf(name, sizeof(name), "%s/fontImage_%i_%i.tga", saveDir, imageNumber, pointSize);
#endif
                common->Printf("Export font: export %d font texture file to '%s'\n", imageNumber, name);
                R_WriteTGA(name, imageBuff, width256, width256);

#ifdef _RAVEN //k: quake4 font texture file
                idStr::snPrintf(name, sizeof(name), "%i_%i.tga", imageNumber, pointSize);
#else
                idStr::snPrintf(name, sizeof(name), "fonts/fontImage_%i_%i.tga", imageNumber, pointSize);
#endif
				for (j = lastStart; j < num; j++) {
					glyphData[j].glyph = j;
                    memset(glyphData[j].shaderName, 0, sizeof(glyphData[j].shaderName));
                    idStr::snPrintf(glyphData[j].shaderName, sizeof(glyphData[j].shaderName), "%s", name);
				}

				lastStart = num;
				memset(out, 0, width1024*width1024);
				xOut = 0;
				yOut = 0;

                if(i == numCharcode - 1)
                    i++;
			}
			else
			{
                i++;
				indexData[charcode] = num;
				memcpy(glyphPtr, glyph, sizeof(glyphInfoExport_t));
                num++;
			}
		}
        else
            i++;
    }

    Mem_Free(imageBuff);

    // keep old glyphs for ASCII
    memset(&font->base.glyphs[0], 0, sizeof(font->base.glyphs));
	for (i = GLYPH_START; i < GLYPH_END; i++) {
		j = indexData[i];
		if(j < 0)
			continue;
		memcpy(&font->base.glyphs[i], &glyphData[j], sizeof(font->base.glyphs[0]));
	}

    common->Printf("Export font: index=%d, read=%d, total=%d, texture=%d.\n", font->numIndexes, num, numCharcode, imageNumber);
	if(num < numCharcode)
	{
		font->numGlyphs = num;
		font->glyphsTable = (glyphInfoExport_t *)malloc(font->numGlyphs * sizeof(glyphInfoExport_t));
		memcpy(&font->glyphsTable[0], &glyphData[0], font->numGlyphs * sizeof(glyphInfoExport_t));
		free(glyphData);
	}

    font->base.glyphScale = glyphScale;

    char filePath[1024] = { 0 };
#ifdef _RAVEN //k: quake4 font data file
    idStr::snPrintf(filePath, sizeof(filePath), "%s_%i.fontdat", saveDir, pointSize);
#else
    idStr::snPrintf(filePath, sizeof(filePath), "%s/fontImage_%i.dat", saveDir, pointSize);
#endif
    common->Printf("Export font: export font data file to '%s'\n", filePath);

    fontInfoExport_t *oldInfo = &font->base;
    idFile *datFile = fileSystem->OpenFileWrite(filePath);

#ifdef _RAVEN //karin: export quake4 fontdat format
    q4_fontInfoExport_t q4Info;
    R_Font_ConvertToQ4Info(font->base, q4Info);
    q4Info.pointSize = (float)pointSize;
    datFile->Write(&q4Info, sizeof(q4_fontInfoExport_t));
#else
    datFile->Write(oldInfo, sizeof(fontInfoExport_t));
#endif
    common->Printf("Export font: base -> bytes=%d, target=%d\n", datFile->Length(), FILESIZE_fontInfo_t);

    font->magic = HARM_NEW_FONT_MAGIC;
    font->version = HARM_NEW_FONT_VERSION;
    font->numFiles = imageNumber;
    font->width = width256;
    font->height = width256;
    datFile->Write(&font->magic, sizeof(font->magic));
    datFile->Write(&font->version, sizeof(font->version));
    datFile->Write(&font->numFiles, sizeof(font->numFiles));
    datFile->Write(&font->width, sizeof(font->width));
    datFile->Write(&font->height, sizeof(font->height));

    datFile->Write(&font->numIndexes, sizeof(font->numIndexes));
    datFile->Write(&font->indexes[0], font->numIndexes * sizeof(font->indexes[0]));
    datFile->Write(&font->numGlyphs, sizeof(font->numGlyphs));
#ifdef _RAVEN //k: quake4 font data file
    q4_glyphInfoExport_t q4Glyph;
    for(i = 0; i < font->numGlyphs; i++)
    {
        R_Font_ConvertToQ4Glyph(font->glyphsTable[i], q4Glyph);
        datFile->Write(&q4Glyph, sizeof(q4_glyphInfoExport_t));
        // write font texture file name
        datFile->Write(&font->glyphsTable[i].shaderName[0], sizeof(font->glyphsTable[i].shaderName));
    }
#else
    datFile->Write(&font->glyphsTable[0], font->numGlyphs * sizeof(font->glyphsTable[0]));
#endif
    common->Printf("Export font: bytes=%d\n", datFile->Length());

    fileSystem->CloseFile(datFile);

    Mem_Free(out);

    free(charcodes);

    return true;
}


static void R_ExportFont_f(const idCmdArgs &args)
{
#ifdef _RAVEN //k: quake4 font: require font type
    if(args.Argc() < 3)
    {
        common->Printf("[Usage]: %s <font file: *.ttf, *.ttc> <output font type name> [<language: default using cvar `sys_lang`> <texture width(power of two): default 256, e.g. 256, 512, 1024, 2048, 4096...>]\n", args.Argv(0));
        return;
    }
#else
    if(args.Argc() < 2)
    {
        common->Printf("[Usage]: %s <font file: *.ttf, *.ttc> [<output font folder name: e.g. XXX will export to 'fonts/XXX/', empty to 'fonts/'> <language: default using cvar `sys_lang`> <texture width(power of two): default 256, e.g. 256, 512, 1024, 2048, 4096...]\n", args.Argv(0));
        return;
    }
#endif

    const char *fontPath = args.Argv(1);
    const char *fontType = args.Argc() > 2 ? args.Argv(2) : NULL;
    const char *language = args.Argc() > 3 ? args.Argv(3) : cvarSystem->GetCVarString("sys_lang");
    const char *textureWidth = args.Argc() > 4 ? args.Argv(4) : "256";

    if(fontType && fontType[0] == '\0')
        fontType = NULL;
    if(!language || !language[0])
        language = "english";
    if(!textureWidth || !textureWidth[0])
        textureWidth = "256";

    common->Printf("Export font file(font=%s, type=%s, language=%s, texture width=%s)......\n", fontPath, fontType ? fontType : "", language, textureWidth);

    int width = atoi(textureWidth);
    if(width < 256)
    {
        common->Warning("Export font width must >= 256!");
        return;
    }
    for(int i = 0, w = width; w > 1; i++)
    {
        int d = w % 2;
        if(d != 0)
        {
            common->Warning("Export font width must power of two!");
            return;
        }
        w >>= 2;
    }

    ftGlobalVars_t exporter;
    R_Font_InitExporter(&exporter);
    if(!R_Font_LoadFont(&exporter, fontPath))
        return;

    const int pointSizes[] = { 12, 24, 48, };

    for(int i = 0; i < sizeof(pointSizes) / sizeof(pointSizes[0]); i++)
    {
        common->Printf("Export point size %d......\n", pointSizes[i]);
        wfontInfo_t font;
        R_Font_Export(&exporter, pointSizes[i], &font, language, fontType, width);
    }

    R_Font_UnloadFont(&exporter);

    common->Printf("Export font file done(font=%s, type=%s, language=%s, texture width=%d).\n", fontPath, fontType ? fontType : "", language, width);
}

void R_Font_AddCommand(void)
{
    cmdSystem->AddCommand("exportFont", R_ExportFont_f, CMD_FL_RENDERER, "Convert ttf/ttc font file to DOOM3 wide character font file");
}
