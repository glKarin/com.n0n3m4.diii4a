// rg_etc1
#ifdef USE_RG_ETC1
#include "etc1/etc_rg_etc1.cpp"
#else
#include "etc1/etc_android.cpp"
#endif

#include "EtcLib/Etc/Etc.cpp"
#include "EtcLib/Etc/EtcFilter.cpp"
#include "EtcLib/Etc/EtcImage.cpp"
#include "EtcLib/Etc/EtcMath.cpp"

#include "EtcLib/EtcCodec/EtcBlock4x4.cpp"
#include "EtcLib/EtcCodec/EtcBlock4x4Encoding.cpp"
#include "EtcLib/EtcCodec/EtcBlock4x4Encoding_ETC1.cpp"
#include "EtcLib/EtcCodec/EtcBlock4x4Encoding_R11.cpp"
#include "EtcLib/EtcCodec/EtcBlock4x4Encoding_RG11.cpp"
#include "EtcLib/EtcCodec/EtcBlock4x4Encoding_RGB8.cpp"
#include "EtcLib/EtcCodec/EtcBlock4x4Encoding_RGB8A1.cpp"
#include "EtcLib/EtcCodec/EtcBlock4x4Encoding_RGBA8.cpp"
#include "EtcLib/EtcCodec/EtcDifferentialTrys.cpp"
#include "EtcLib/EtcCodec/EtcIndividualTrys.cpp"
#include "EtcLib/EtcCodec/EtcSortedBlockList.cpp"

#if 1
#if !defined(_MSC_VER)
//#define IC_PRINT(fmt, args...) { printf(fmt, ##args); }
#define IC_PRINT(fmt, args...)
#define IC_ERROR(fmt, args...) { printf(fmt, ##args); }
#else
//#define IC_PRINT(fmt, ...) { printf(fmt, __VA_ARGS__); }
#define IC_PRINT(fmt, ...)
#define IC_ERROR(fmt, ...) { printf(fmt, __VA_ARGS__); }
#endif
#else
#define IC_PRINT(x)
#define IC_ERROR(x)
#endif

#ifndef GL_ETC1_RGB8_OES
#define GL_ETC1_RGB8_OES                  0x8D64
#endif
#ifndef GL_COMPRESSED_RGBA8_ETC2_EAC
#define GL_COMPRESSED_RGBA8_ETC2_EAC      0x9278
#endif

enum {
    IC_ETC1 = 1,
    IC_RGBA4444 = 2,
    IC_ETC2_RGBA8 = 3,
};

#define IMAGE_COMPRESSION_MAGIC ((unsigned int)('d' << 24 | '3' << 16 | 'e' << 8 | 's'))
#define IMAGE_COMPRESSION_VERSION ((unsigned int)(0x00010001))

// for ETC2 compression texture encode
#include "EtcLib/Etc/Etc.h"

// for ETC1 compression texture encode
#include "etc1/etc1_android.h"

static idCVar harm_image_useCompression( "harm_image_useCompression", "0", CVAR_INTEGER | CVAR_INIT, "Use ETC1/2 compression or RGBA4444 texture for low memory(e.g. 32bits device), it will using lower memory but loading slower\n  0 = RGBA8;\n  1 = ETC1 compression(no alpha);\n  2 = ETC2 compression;\n  3 = RGBA4444" );
static idCVar harm_image_useCompressionCache( "harm_image_useCompressionCache", "0", CVAR_BOOL | CVAR_INIT, "Cache ETC1/2 compression or RGBA4444 texture to filesystem" );
#define image_useETCCompression (harm_image_useCompression.GetInteger() == 1)
#define image_useETC2Compression (harm_image_useCompression.GetInteger() == 2)
#define image_useRGBA4444 (harm_image_useCompression.GetInteger() == 3)
#define image_useCompression (harm_image_useCompression.GetInteger() >= 1 && harm_image_useCompression.GetInteger() <= 3)

ID_INLINE static const char * R_Image_CheckFileNameNotGenerated(const char *cachefname)
{
    return(cachefname && cachefname[0] && cachefname[0] != '_') ? cachefname : NULL;
}

#define IC_CACHE_FILE_NAME(img) ((img)->generatorFunction ? NULL : R_Image_CheckFileNameNotGenerated((img)->imgName.c_str()))
#define IC_CAN_COMPRESSION(internalFormat, dataFormat, dataType) (/*internalFormat == GL_RGBA && */dataFormat == GL_RGBA && dataType == GL_UNSIGNED_BYTE)

ID_INLINE static bool R_Image_CheckExistsAndNotGenerated(const char *cachefname)
{
	return(NULL != R_Image_CheckFileNameNotGenerated(cachefname)
    // && fileSystem->ReadFile(cachefname, NULL, NULL) > 0
    );
}

ID_INLINE static unsigned int etc1_data_size(unsigned int width, unsigned int height) {
    return (((width + 3) & ~3) * ((height + 3) & ~3)) >> 1;
}

ID_INLINE static unsigned int rgba4444_data_size(unsigned int width, unsigned int height) {
    return width * height * 2;
}

ID_INLINE static unsigned short CalcExtendedDimension(unsigned short a_ushOriginalDimension)
{
    return (unsigned short)((a_ushOriginalDimension + 3) & ~3);
}

ID_INLINE static unsigned int etc2_data_size(unsigned int width, unsigned int height) {
    // return etc2_data_size(width) * etc2_data_size(height);
    return (CalcExtendedDimension(width) >> 2) * (CalcExtendedDimension(height) >> 2) * Etc::Block4x4EncodingBits::GetBytesPerBlock(Etc::Block4x4EncodingBits::Format::RGBA8);
}

static void * rgba4444_convert_tex_image(unsigned int width, unsigned int height, const void *pixels)
{
    unsigned char const *cpixels = (unsigned char const *) pixels;
    unsigned short *rgba4444data = (unsigned short *)malloc(2 * width * height);
    rgba4444data = (unsigned short *) ((unsigned char *) rgba4444data);
    int i;
    for (i = 0; i < width * height; i++) {
        unsigned char r, g, b, a;
        r = cpixels[4 * i] >> 4;
        g = cpixels[4 * i + 1] >> 4;
        b = cpixels[4 * i + 2] >> 4;
        a = cpixels[4 * i + 3] >> 4;
        rgba4444data[i] = r << 12 | g << 8 | b << 4 | a;
    }
    return rgba4444data;
}

static void * etc1_compress_tex_image(unsigned int width, unsigned int height, const void *pixels)
{
    unsigned char const *cpixels = (unsigned char const *) pixels;
    unsigned char *etc1data;
    unsigned int size = etc1_data_size(width, height);
    etc1data = (unsigned char *)malloc(size);
    etc1_encode_image(cpixels, width, height, 4, width * 4, etc1data);
    return etc1data;
}

extern void Sys_CPUCount( int& logicalNum, int& coreNum, int& packageNum );
static void * etc2_compress_tex_image(unsigned int width, unsigned int height, const void *pixels, unsigned int *size, unsigned int *rwidth, unsigned int *rheight)
{
    unsigned char *paucEncodingBits;
    unsigned int uiEncodingBitsBytes;
    unsigned int uiExtendedWidth;
    unsigned int uiExtendedHeight;
    int iEncodingTime_ms;

    static bool getCPUInfo = false;
    static int numPhysicalCpuCores;
    static int numLogicalCpuCores;
    static int numCpuPackages;

    if(!getCPUInfo)
    {
        Sys_CPUCount( numLogicalCpuCores, numPhysicalCpuCores, numCpuPackages );
        getCPUInfo = true;
    }

    Etc::Encode((float *)pixels,
                width, height,
                Etc::Image::Format::RGBA8,
                Etc::ErrorMetric::RGBA,
                0, // fEffort,
                numPhysicalCpuCores * numCpuPackages,
                numLogicalCpuCores * numCpuPackages,
                &paucEncodingBits, &uiEncodingBitsBytes,
                &uiExtendedWidth, &uiExtendedHeight,
                &iEncodingTime_ms);

    *size = uiEncodingBitsBytes;
    *rwidth = uiExtendedWidth;
    *rheight = uiExtendedHeight;
    return paucEncodingBits;
}

typedef struct imageCompressionHeader_s
{
    uint32_t magic;
    uint32_t version;
    uint16_t type;
    uint32_t width;
    uint32_t height;
    uint8_t mipmap;
    uint8_t expended;
    uint32_t expended_width;
    uint32_t expended_height;
    uint32_t length;
} imageCompressionHeader_t;

typedef struct imageCompression_s
{
    imageCompressionHeader_t header;
    const byte *data;
} imageCompression_t;

static void R_Image_WriteCompressionStruct(imageCompression_t *image, int width, int height, int type, int mipmap = 0, int length = 0, byte *data = NULL)
{
    unsigned int magic = IMAGE_COMPRESSION_MAGIC;
    memcpy(&image->header.magic, &magic, 4);
    image->header.version = IMAGE_COMPRESSION_VERSION;
    image->header.type = type;
    image->header.width = width;
    image->header.height = height;
    image->header.mipmap = mipmap;
    image->header.expended = 0;
    image->header.expended_width = width;
    image->header.expended_height = height;
    image->header.length = length;
    image->data = data;
}

static void R_Image_WriteCompressionStruct(imageCompression_t *image, int width, int height, int type, int expended_width, int expended_height, int mipmap = 0, int length = 0, byte *data = NULL)
{
    unsigned int magic = IMAGE_COMPRESSION_MAGIC;
    memcpy(&image->header.magic, &magic, 4);
    image->header.version = IMAGE_COMPRESSION_VERSION;
    image->header.type = type;
    image->header.width = width;
    image->header.height = height;
    image->header.mipmap = mipmap;
    image->header.expended = width != expended_width || height != expended_height;
    image->header.expended_width = expended_width;
    image->header.expended_height = expended_height;
    image->header.length = length;
    image->data = data;
}

static bool R_Image_ReadCompressionStruct(byte *data, int length, imageCompression_t *image)
{
    if(length < sizeof(imageCompressionHeader_t))
    {
        IC_ERROR("Invalid idTech4A++ compression texture header.\n");
        return false;
    }

    imageCompressionHeader_t header;
    memcpy(&header, data, sizeof(header));
    if(header.magic != IMAGE_COMPRESSION_MAGIC)
    {
        IC_ERROR("Invalid idTech4A++ compression texture magic: %u != %u.\n", header.magic, IMAGE_COMPRESSION_MAGIC);
        return false;
    }
    if(header.version != IMAGE_COMPRESSION_VERSION) // only support current version
    {
        IC_ERROR("Invalid idTech4A++ compression texture version: %u != %u.\n", header.version, IMAGE_COMPRESSION_VERSION);
        return false;
    }
    // if(type);
    if(length < header.length + sizeof(header))
    {
        IC_ERROR("Invalid idTech4A++ compression texture length: %u + %zu > %d.\n", header.length, sizeof(header), length);
        return false;
    }

    memcpy(&image->header, &header, sizeof(header));
    image->data = data + sizeof(header);

    return true;
}

static bool R_Image_CacheCompressionStruct(const imageCompression_t *image, const char *cachefname)
{
    idFile *file = fileSystem->OpenFileWrite(cachefname, "fs_basepath");
    if(!file)
        return false;
    file->Write(&image->header, sizeof(image->header));
    file->Write(image->data, image->header.length);
    fileSystem->CloseFile(file);
    return true;
}

/*
================
ImageProgramStringToFileCompressedFileName
================
*/
static void ImageProgramStringToCompressedFileName(const char *imageProg, char *fileName)
{
    const char	*s;
    char	*f;

    strcpy(fileName, "generated/gles/");
    f = fileName + strlen(fileName);

    int depth = 0;

    // convert all illegal characters to underscores
    // this could conceivably produce a duplicated mapping, but we aren't going to worry about it
    for (s = imageProg ; *s ; s++) {
        if (*s == '/' || *s == '\\' || *s == '(') {
            if (depth < 4) {
                *f = '/';
                depth ++;
            } else {
                *f = ' ';
            }

            f++;
        } else if (*s == '<' || *s == '>' || *s == ':' || *s == '|' || *s == '"' || *s == '.') {
            *f = '_';
            f++;
        } else if (*s == ' ' && *(f-1) == '/') {	// ignore a space right after a slash
        } else if (*s == ')' || *s == ',') {		// always ignore these
        } else {
            *f = *s;
            f++;
        }
    }

    *f++ = 0;
    strcat(fileName, ".dds");
}

static idStr R_Image_GenerateCompressionFileName(const char *name, int width, int height, const char *ext /* etc, rgba4444, etc2 */, int mipmap = 0)
{
    char filename[MAX_IMAGE_NAME];
    ImageProgramStringToCompressedFileName(name, filename);

    idStr str(filename);
    str.StripFileExtension();
#if 0
    str.Append(va("_%dx%d", width, height));
#else
    (void)width; (void)height;
#endif
    if(mipmap > 0)
        str.Append(va("_%d%d", mipmap / 10, mipmap % 10));
    str.SetFileExtension(va(".%s", ext));
    return str;
}

ID_INLINE static idStr R_Image_GenerateCompressionFileName(const char *cachefname, int width, int height, int mipLevel = 0)
{
    idStr ext;
    if(image_useETCCompression)
        ext = "etc1";
    else if(image_useETC2Compression)
        ext = "etc2";
    else if(image_useRGBA4444)
        ext = "rgba4444";
    return R_Image_GenerateCompressionFileName(cachefname, width, height, ext.c_str(), mipLevel);
}

static void R_Image_TexImage2D(GLenum target, GLint level, GLsizei w, GLsizei h)
{
    int compressedSize;

    if(image_useETCCompression)
    {
        compressedSize = etc1_data_size(w, h);
        byte* data = ( byte* )Mem_Alloc( compressedSize, TAG_TEMP );
        TID(glCompressedTexImage2D( target, level, GL_ETC1_RGB8_OES, w, h, 0, compressedSize, data ));
        if( data != NULL )
        {
            Mem_Free( data );
        }
    }
    else if(image_useETC2Compression)
    {
        compressedSize = etc2_data_size(w, h);
        byte* data = ( byte* )Mem_Alloc( compressedSize, TAG_TEMP );
        if( data != NULL )
        {
            memset(data, 0, compressedSize);
            unsigned int etc2_width = CalcExtendedDimension(w);
            unsigned int etc2_height = CalcExtendedDimension(h);
            //void *etc2_data = etc2_compress_tex_image(w, h, data, &etc2_size, &etc2_width, &etc2_height);
            TID(glCompressedTexImage2D( target, level, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2_width, etc2_height, 0, compressedSize, data ));
            //free(etc2_data);
        }
        else
            TID(glCompressedTexImage2D( target, level, GL_COMPRESSED_RGBA8_ETC2_EAC, w, h, 0, compressedSize, data ));
        if( data != NULL )
        {
            Mem_Free( data );
        }
    }
    else if(image_useRGBA4444)
    {
        compressedSize = rgba4444_data_size(w, h);
        byte* data = ( byte* )Mem_Alloc( compressedSize, TAG_TEMP );
        TID(glTexImage2D( target, level, GL_RGBA /*internalFormat*/, w, h, 0, GL_RGBA /*dataFormat*/, GL_UNSIGNED_SHORT_4_4_4_4 /*dataType*/, data ));
        if( data != NULL )
        {
            Mem_Free( data );
        }
    }
    else // RGBA8888
    {
        compressedSize = w * h * 4;
        byte* data = ( byte* )Mem_Alloc( compressedSize, TAG_TEMP );
        TID(glTexImage2D( target, level, GL_RGBA /*internalFormat*/, w, h, 0, GL_RGBA /*dataFormat*/, GL_UNSIGNED_BYTE /*dataType*/, data ));
        if( data != NULL )
        {
            Mem_Free( data );
        }
    }
}

static bool R_Image_TexSubImage2DFromCache(const char *cachefname, GLenum uploadTarget, GLint mipLevel, GLsizei x, GLsizei y, GLsizei width, GLsizei height)
{
    if(!image_useCompression || !harm_image_useCompressionCache.GetBool() || !R_Image_CheckFileNameNotGenerated(cachefname))
        return false;

    idStr cacheName = R_Image_GenerateCompressionFileName(cachefname, width, height, mipLevel);

    bool res = false;
    char *tmp;
    int sz = fileSystem->ReadFile(cacheName.c_str(), (void **)&tmp, 0);
    if(sz > 0)
    {
        imageCompression_t image;
        if(R_Image_ReadCompressionStruct((byte *)tmp, sz, &image))
        {
            if(image.header.width == width && image.header.height == height
                // && image.header.mipmap == level
                    ) {
                if (image.header.type == IC_ETC1) {
                    IC_PRINT("Loading idTech4A++ compression texture ETC1: %dx%d %s.\n", image.header.width, image.header.height, cacheName.c_str());
                    glCompressedTexSubImage2D(uploadTarget, mipLevel, x, y, width, height, GL_ETC1_RGB8_OES, image.header.length, image.data);
                    res = true;
                } else if (image.header.type == IC_RGBA4444) {
                    IC_PRINT("Loading idTech4A++ compression texture RGBA4444: %dx%d %s.\n", image.header.width, image.header.height, cacheName.c_str());
                    glTexSubImage2D(uploadTarget, mipLevel, x, y, width, height, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, image.data);
                    res = true;
                } else if (image.header.type == IC_ETC2_RGBA8) {
                    IC_PRINT("Loading idTech4A++ compression texture ETC2: %dx%d %s.\n", image.header.width, image.header.height, cacheName.c_str());
                    glCompressedTexSubImage2D(uploadTarget, mipLevel, x, y, width, height, GL_COMPRESSED_RGBA8_ETC2_EAC, image.header.length, image.data);
                    res = true;
                } else {
                    IC_ERROR("Unsupport idTech4A++ compression texture format: %d (%s).\n", image.header.type, cacheName.c_str());
                }
            }
            else
            {
                IC_ERROR("idTech4A++ compression texture size changed: %u != %d || %u != %d (%s).\n", image.header.width, width, image.header.height, height, cacheName.c_str());
            }
        }
    }
	if(tmp)
		fileSystem->FreeFile(tmp);
    return res;
}

static void R_Image_WriteTextureCache(const char *cachefname, GLint mipLevel, int x, int y, int width, int height, int size, const void *data)
{
    if(!harm_image_useCompressionCache.GetBool())
        return;
    if(!R_Image_CheckFileNameNotGenerated(cachefname))
        return;

    int type = 0;
    idStr ext;
    if(image_useETCCompression)
    {
        ext = "etc1";
        type = IC_ETC1;
    }
    else if(image_useETC2Compression)
    {
        ext = "etc2";
        type = IC_ETC2_RGBA8;
    }
    else if(image_useRGBA4444)
    {
        ext = "rgba4444";
        type = IC_RGBA4444;
    }
    idStr cacheName = R_Image_GenerateCompressionFileName(cachefname, width, height, ext.c_str(), mipLevel);

    imageCompression_t image;
    R_Image_WriteCompressionStruct(&image, width, height, type, mipLevel, size, (byte *)data);
    IC_PRINT("Caching idTech4A++ compression texture %s: %dx%d %s.\n", ext.c_str(), image.header.width, image.header.height, cacheName.c_str());
    R_Image_CacheCompressionStruct(&image, cacheName.c_str());
}

static void R_Image_TexSubImage2D(const char *cachefname, GLenum uploadTarget, GLint mipLevel, GLsizei x, GLsizei y, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
    if(R_Image_TexSubImage2DFromCache(cachefname, uploadTarget, mipLevel, x, y, width, height))
        return;

    const byte *dpic = (const byte *)pixels;

    if(image_useETCCompression)
    {
        int etc1_size = etc1_data_size(width, height);
        void *etc1_data = etc1_compress_tex_image(width, height, dpic);
        TID(glCompressedTexSubImage2D( uploadTarget, mipLevel, x, y, width, height, GL_ETC1_RGB8_OES, etc1_size, etc1_data ));
        R_Image_WriteTextureCache(cachefname, mipLevel, x, y, width, height, etc1_size, etc1_data);
        free(etc1_data);
    }
    else if(image_useETC2Compression)
    {
        unsigned int etc2_size;
        unsigned int etc2_width;
        unsigned int etc2_height;

        unsigned int floatSize = width * height * sizeof(Etc::ColorFloatRGBA);
        Etc::ColorFloatRGBA* floatData = ( Etc::ColorFloatRGBA* )malloc( floatSize );
        const int Size = width * height;
        for(int i = 0; i < Size; i++)
        {
            floatData[i] = Etc::ColorFloatRGBA::ConvertFromRGBA8(dpic[i * 4], dpic[i * 4 + 1], dpic[i * 4 + 2], dpic[i * 4 + 3]);
        }

        void *etc2_data = etc2_compress_tex_image(width, height, floatData, &etc2_size, &etc2_width, &etc2_height);
        TID(glCompressedTexSubImage2D( uploadTarget, mipLevel, x, y, etc2_width, etc2_height, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2_size, etc2_data ));
        R_Image_WriteTextureCache(cachefname, mipLevel, x, y, width, height, etc2_size, etc2_data);

        free(floatData);
        free(etc2_data);
    }
    else if(image_useRGBA4444)
    {
        void *rgba4444_data = rgba4444_convert_tex_image(width, height, dpic);
        TID(glTexSubImage2D( uploadTarget, mipLevel, x, y, width, height, GL_RGBA /*dataFormat*/, GL_UNSIGNED_SHORT_4_4_4_4, rgba4444_data ));
        R_Image_WriteTextureCache(cachefname, mipLevel, x, y, width, height, rgba4444_data_size(width, height), rgba4444_data);
        free(rgba4444_data);
    }
    else
    {
        TID(glTexSubImage2D( uploadTarget, mipLevel, x, y, width, height, GL_RGBA /*dataFormat*/, GL_UNSIGNED_BYTE /*dataType*/, dpic ));
    }
}
