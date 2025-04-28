#include "../../idlib/precompiled.h"
#pragma hdrstop

#define FONT_SCALE(x, scale) (int)((float)(x) * scale)

ID_INLINE static void ReadBig(idFile *file, int &out)
{
    file->ReadInt(out);
    out = BigLong(out);
}

ID_INLINE static void ReadBig(idFile *file, short &out)
{
    file->ReadShort(out);
    out = BigShort(out);
}

ID_INLINE static void ReadBig(idFile *file, unsigned int &out)
{
    int i;
    file->ReadInt(i);
    i = BigLong(i);
    out = *(unsigned int *)&i;
}

ID_INLINE static void Little(int &out)
{
    out = LittleLong(out);
}

ID_INLINE static void Little(unsigned short &out)
{
    short sh = *(short *)&out;
    sh = LittleShort(sh);
    out = *(unsigned short *)&sh;
}

ID_INLINE static void Little(unsigned int &out)
{
    int i = *(int *)&out;
    i = LittleLong(i);
    out = *(unsigned int *)&i;
}

ID_INLINE static void Little(float &out)
{
    out = LittleFloat(out);
}

ID_INLINE static void Little(signed char &)
{
}

ID_INLINE static void Little(unsigned char &)
{
}

ID_INLINE static void LittleArray(unsigned int *c, int count)
{
    for( int i = 0; i < count; i++ )
    {
        Little( c[i] );
    }
}

struct d3bfg_glyphInfo_t
{
    byte	width;	// width of glyph in pixels
    byte	height;	// height of glyph in pixels
    signed char	top;	// distance in pixels from the base line to the top of the glyph
    signed char	left;	// distance in pixels from the pen to the left edge of the glyph
    byte	xSkip;	// x adjustment after rendering this glyph
    uint16_t	s;		// x offset in image where glyph starts (in pixels)
    uint16_t	t;		// y offset in image where glyph starts (in pixels)
};

struct d3bfg_fontInfo_t
{
    struct oldInfo_t
    {
        float maxWidth;
        float maxHeight;
    } oldInfo[3];

    short		ascender;
    short		descender;

    short		numGlyphs;
    d3bfg_glyphInfo_t* glyphData;

    // This is a sorted array of all characters in the font
    // This maps directly to glyphData, so if charIndex[0] is 42 then glyphData[0] is character 42
    uint32_t* 	charIndex;

    // As an optimization, provide a direct mapping for the ascii character set
    signed char		ascii[128];

    const idMaterial* 	material;

    int pointSize;
};

struct oldGlyphInfo_t
{
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
    int					junk;
    char				materialName[32];
};

extern bool R_GetBimageSize(const char *filename, int &width, int &height);

static void R_Font_FreeD3BFGFont(d3bfg_fontInfo_t &fontInfo)
{
	free(fontInfo.glyphData);
}

static void R_Font_ConvertD3BFGGlyph(const d3bfg_fontInfo_t &fontInfo, const d3bfg_glyphInfo_t *d3bfg_glyph, glyphInfo_t *glyph, int size, int imageWidth, int imageHeight)
{
    float scale = 1.0f / (float)size;
    glyph->height = FONT_SCALE(d3bfg_glyph->height, scale);
    glyph->top = FONT_SCALE(d3bfg_glyph->top, scale);
    glyph->bottom = glyph->top - glyph->height;
    glyph->pitch = FONT_SCALE(d3bfg_glyph->left + d3bfg_glyph->width, scale);
    glyph->xSkip = FONT_SCALE(d3bfg_glyph->xSkip, scale);
    glyph->imageWidth = FONT_SCALE(d3bfg_glyph->width, scale);
    glyph->imageHeight = FONT_SCALE(d3bfg_glyph->height, scale);
    glyph->glyph = fontInfo.material;

    float invMaterialWidth = 1.0f / (float)imageWidth; //fontInfo.material->GetImageWidth();
    float invMaterialHeight = 1.0f / (float)imageHeight; //fontInfo.material->GetImageHeight();
    glyph->s = ( (float)d3bfg_glyph->s - 0.5f ) * invMaterialWidth;
    glyph->t = ( (float)d3bfg_glyph->t - 0.5f ) * invMaterialHeight;
    glyph->s2 = ( (float)d3bfg_glyph->s + (float)d3bfg_glyph->width + 0.5f ) * invMaterialWidth;
    glyph->t2 = ( (float)d3bfg_glyph->t + (float)d3bfg_glyph->height + 0.5f ) * invMaterialHeight;
}

static void R_Font_ConvertD3BFGFont(const d3bfg_fontInfo_t &fontInfo, fontInfoEx_t &font, int imageWidth, int imageHeight)
{
    int i, m, f;
    glyphInfo_t *glyph;
    d3bfg_glyphInfo_t *d3bfg_glyph;
    int charcode;
    const int dpi = 72;

    memset(&font, 0, sizeof(font));

    font.maxWidthSmall = int((float)fontInfo.oldInfo[0].maxWidth / 4.0f);
    font.maxHeightSmall = int((float)fontInfo.oldInfo[0].maxHeight / 4.0f);
    font.maxWidthMedium = int((float)fontInfo.oldInfo[1].maxWidth / 2.0f);
    font.maxHeightMedium = int((float)fontInfo.oldInfo[1].maxHeight / 2.0f);
    font.maxWidthLarge = fontInfo.oldInfo[2].maxWidth;
    font.maxHeightLarge = fontInfo.oldInfo[2].maxHeight;

	const int scales[] = { 4, 2, 1 };
	fontInfo_t *infos[] = { &font.fontInfoSmall, &font.fontInfoMedium, &font.fontInfoLarge };
	for(f = 0; f < 3; f++)
	{
		fontInfo_t *info = infos[f];
		int scale = scales[f];

		info->glyphScale = (72.0f / (float)dpi) * (float)scale;
		for(i = GLYPH_START; i < GLYPH_END; i++)
		{
#if 0
			if(i < 128)
			{
				m = fontInfo.ascii[i];
				if(m < 0 || m >= fontInfo.numGlyphs)
					continue;
				d3bfg_glyph = &fontInfo.glyphData[m];

				glyph = &info->glyphs[i];
				R_Font_ConvertD3BFGGlyph(fontInfo, d3bfg_glyph, glyph, scale, imageWidth, imageHeight);

				printf("FFF %d: %c %d %d %d | %d %d | %d %d | %f %f %f %f\n",i,i, glyph->height,
						glyph->top, glyph->bottom, glyph->pitch, glyph->xSkip,
						glyph->imageWidth, glyph->imageHeight,
						glyph->s, glyph->t,glyph->s2, glyph->t2
					  );
			}
#else
			for( m = 0; m < fontInfo.numGlyphs; m++ )
			{
				charcode = fontInfo.charIndex[m];
				if( charcode == i )
				{
					d3bfg_glyph = &fontInfo.glyphData[m];

					glyph = &info->glyphs[i];
					R_Font_ConvertD3BFGGlyph(fontInfo, d3bfg_glyph, glyph, scale, imageWidth, imageHeight);

					/*
					printf("FFF %d: %c %d %d %d | %d %d | %d %d | %f %f %f %f\n",i,i, glyph->height,
							glyph->top, glyph->bottom, glyph->pitch, glyph->xSkip,
							glyph->imageWidth, glyph->imageHeight,
							glyph->s, glyph->t,glyph->s2, glyph->t2
						  );
						  */
					break;
				}
			}
#endif
		}

		int maxIndex = 0;
		for( m = 0; m < fontInfo.numGlyphs; m++ )
		{
			charcode = fontInfo.charIndex[m];
			if(charcode > maxIndex)
				maxIndex = charcode;
		}
		info->numIndexes = maxIndex + 1;
		info->indexes = (int *)malloc(info->numIndexes * sizeof(info->indexes[0]));
		for( m = 0; m < info->numIndexes; m++ )
		{
			info->indexes[m] = -1;
		}
		info->numGlyphs = fontInfo.numGlyphs;
		info->glyphsTable = (glyphInfo_t *)malloc(info->numGlyphs * sizeof(info->glyphsTable[0]));
		for( m = 0; m < fontInfo.numGlyphs; m++ )
		{
			charcode = fontInfo.charIndex[m];
			info->indexes[charcode] = m;
			d3bfg_glyph = &fontInfo.glyphData[m];
			glyph = &info->glyphsTable[m];
			R_Font_ConvertD3BFGGlyph(fontInfo, d3bfg_glyph, glyph, scale, imageWidth, imageHeight);
		}
		//printf("EEEEEEEEE %f %f %f\n",font.fontInfoSmall.glyphScale, font.fontInfoMedium.glyphScale, font.fontInfoLarge.glyphScale);
	}

}

static bool R_Font_LoadOldGlyphData( const char* filename, oldGlyphInfo_t glyphInfo[GLYPHS_PER_FONT] )
{
    idFile* fd = fileSystem->OpenFileRead( filename );
    if( fd == NULL )
    {
        return false;
    }
    fd->Read( glyphInfo, GLYPHS_PER_FONT * sizeof( oldGlyphInfo_t ) );
    for( int i = 0; i < GLYPHS_PER_FONT; i++ )
    {
        Little( glyphInfo[i].height );
        Little( glyphInfo[i].top );
        Little( glyphInfo[i].bottom );
        Little( glyphInfo[i].pitch );
        Little( glyphInfo[i].xSkip );
        Little( glyphInfo[i].imageWidth );
        Little( glyphInfo[i].imageHeight );
        Little( glyphInfo[i].s );
        Little( glyphInfo[i].t );
        Little( glyphInfo[i].s2 );
        Little( glyphInfo[i].t2 );
        assert( glyphInfo[i].imageWidth == glyphInfo[i].pitch );
        assert( glyphInfo[i].imageHeight == glyphInfo[i].height );
        assert( glyphInfo[i].imageWidth == ( glyphInfo[i].s2 - glyphInfo[i].s ) * 256 );
        assert( glyphInfo[i].imageHeight == ( glyphInfo[i].t2 - glyphInfo[i].t ) * 256 );
        assert( glyphInfo[i].junk == 0 );
    }
    fileSystem->CloseFile(fd);
    return true;
}

static bool R_Font_LoadD3BFGFont( const char *name, fontInfoEx_t &font)
{
    idStr fontName = va( "%s/48.dat", name );
    idFile* fd = fileSystem->OpenFileRead( fontName );
    if( fd == NULL )
    {
        return false;
    }

    const int FONT_INFO_VERSION = 42;
    const int FONT_INFO_MAGIC = ( FONT_INFO_VERSION | ( 'i' << 24 ) | ( 'd' << 16 ) | ( 'f' << 8 ) );

    uint32_t version = 0;
    ReadBig( fd, version );
    if( version != FONT_INFO_MAGIC )
    {
        idLib::Warning( "Wrong version in %s", name );
        fileSystem->CloseFile(fd);
        return false;
    }

    d3bfg_fontInfo_t fontInfo;
    memset(&fontInfo, 0, sizeof(fontInfo));

    short pointSize = 0;

    ReadBig( fd, pointSize );
    assert( pointSize == 48 );

    ReadBig( fd, fontInfo.ascender );
    ReadBig( fd, fontInfo.descender );

    ReadBig( fd, fontInfo.numGlyphs );

    fontInfo.pointSize = pointSize;

    //printf("XXX %d %d %d %d\n",pointSize,fontInfo.ascender,fontInfo.descender,fontInfo.numGlyphs);

    fontInfo.glyphData = ( d3bfg_glyphInfo_t* )malloc( sizeof( d3bfg_glyphInfo_t ) * fontInfo.numGlyphs );
    fontInfo.charIndex = ( uint32_t* )malloc( sizeof( uint32_t ) * fontInfo.numGlyphs );

    fd->Read( fontInfo.glyphData, fontInfo.numGlyphs * sizeof( d3bfg_glyphInfo_t ) );

    for( int i = 0; i < fontInfo.numGlyphs; i++ )
    {
        Little( fontInfo.glyphData[i].width );
        Little( fontInfo.glyphData[i].height );
        Little( fontInfo.glyphData[i].top );
        Little( fontInfo.glyphData[i].left );
        Little( fontInfo.glyphData[i].xSkip );
        Little( fontInfo.glyphData[i].s );
        Little( fontInfo.glyphData[i].t );
		/*
        printf("FFF %d %d %d %d %d %d %d %d\n",i,
                fontInfo.glyphData[i].width,
         fontInfo.glyphData[i].height,
         fontInfo.glyphData[i].top,
         fontInfo.glyphData[i].left,
         fontInfo.glyphData[i].xSkip,
         fontInfo.glyphData[i].s,
         fontInfo.glyphData[i].t
               );
			   */
    }

    fd->Read( fontInfo.charIndex, fontInfo.numGlyphs * sizeof( uint32_t ) );
    LittleArray( fontInfo.charIndex, fontInfo.numGlyphs );

    for( int i = 0; i < fontInfo.numGlyphs; i++ )
    //printf("III %d %d\n",i,fontInfo.charIndex[i]);

    memset( fontInfo.ascii, -1, sizeof( fontInfo.ascii ) );
    for( int i = 0; i < fontInfo.numGlyphs; i++ )
    {
        if( fontInfo.charIndex[i] < 128 )
        {
            fontInfo.ascii[fontInfo.charIndex[i]] = i;
        }
        else
        {
            // Since the characters are sorted, as soon as we find a non-ascii character, we can stop
            break;
        }
    }

    idStr fontTextureName = fontName;
    fontTextureName.SetFileExtension( "bimage" );

    fontInfo.material = declManager->FindMaterial( fontTextureName );
    //printf("aa %s\n", fontTextureName.c_str());
    fontInfo.material->SetSort( SS_GUI );

    // Load the old glyph data because we want our new fonts to fit in the old glyph metrics
    int pointSizes[3] = { 12, 24, 48 };
    float scales[3] = { 4.0f, 2.0f, 1.0f };
    for( int i = 0; i < 3; i++ )
    {
        oldGlyphInfo_t oldGlyphInfo[GLYPHS_PER_FONT];
        const char* oldFileName = va( "newfonts/%s/old_%d.dat", name, pointSizes[i] );
        if( R_Font_LoadOldGlyphData( oldFileName, oldGlyphInfo ) )
        {
            int mh = 0;
            int mw = 0;
            for( int g = 0; g < GLYPHS_PER_FONT; g++ )
            {
                if( mh < oldGlyphInfo[g].height )
                {
                    mh = oldGlyphInfo[g].height;
                }
                if( mw < oldGlyphInfo[g].xSkip )
                {
                    mw = oldGlyphInfo[g].xSkip;
                }
            }
            fontInfo.oldInfo[i].maxWidth = scales[i] * mw;
            fontInfo.oldInfo[i].maxHeight = scales[i] * mh;
        }
        else
        {
            int mh = 0;
            int mw = 0;
            for( int g = 0; g < fontInfo.numGlyphs; g++ )
            {
                if( mh < fontInfo.glyphData[g].height )
                {
                    mh = fontInfo.glyphData[g].height;
                }
                if( mw < fontInfo.glyphData[g].xSkip )
                {
                    mw = fontInfo.glyphData[g].xSkip;
                }
            }
            fontInfo.oldInfo[i].maxWidth = mw;
            fontInfo.oldInfo[i].maxHeight = mh;
        }
    }
    fileSystem->CloseFile(fd);

	int imageWidth, imageHeight;
	if(R_GetBimageSize(fontTextureName.c_str(), imageWidth, imageHeight))
	{
		R_Font_ConvertD3BFGFont(fontInfo, font, imageWidth, imageHeight);
		//printf("xxxxyyyzzx %d %d\n", imageWidth, imageHeight);
	}

	R_Font_FreeD3BFGFont(fontInfo);

    return true;
}
