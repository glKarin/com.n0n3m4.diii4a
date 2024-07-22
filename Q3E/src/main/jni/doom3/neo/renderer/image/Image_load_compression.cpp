//Code from raspberrypi q3

#include "../etc/etc.h"

#if 1
#if !defined(_MSC_VER)
//#define IC_PRINT(fmt, args...) { printf(fmt, ##args); }
#define IC_PRINT(x)
#define IC_ERROR(fmt, args...) { printf(fmt, ##args); }
#else
//#define IC_PRINT(fmt, ...) { printf(fmt, __VA_ARGS__); }
#define IC_PRINT(x)
#define IC_ERROR(fmt, ...) { printf(fmt, __VA_ARGS__); }
#endif
#else
#define IC_PRINT(x)
#define IC_ERROR(x)
#endif

enum {
    IC_ETC1 = 1,
    IC_RGBA4444 = 2,
#ifdef _OPENGLES3
    IC_ETC2_RGBA8 = 3,
#endif
};

#define IMAGE_COMPRESSION_MAGIC ((unsigned int)('d' << 24 | '3' << 16 | 'e' << 8 | 's'))
#define IMAGE_COMPRESSION_VERSION ((unsigned int)(0x00010001))

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

static void R_AllocCompression(imageCompression_t *image, int width, int height, int type, int mipmap = 0, int length = 0, byte *data = NULL)
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

static void R_AllocCompression(imageCompression_t *image, int width, int height, int type, int expended_width, int expended_height, int mipmap = 0, int length = 0, byte *data = NULL)
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

static bool R_LoadCompression(byte *data, int length, imageCompression_t *image)
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
    if(header.version > IMAGE_COMPRESSION_VERSION)
	{
        IC_ERROR("Invalid idTech4A++ compression texture version: %u > %u.\n", header.version, IMAGE_COMPRESSION_VERSION);
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

static bool R_CacheCompression(const imageCompression_t *image, const char *cachefname)
{
    idFile *file = fileSystem->OpenFileWrite(cachefname);
    if(!file)
        return false;
    file->Write(&image->header, sizeof(image->header));
    file->Write(image->data, image->header.length);
    fileSystem->CloseFile(file);
    return true;
}

ID_INLINE static idStr R_GenerateCompressionFileName(const char *name, int width, int height, const char *ext /* etc, rgba4444, etc2 */, int mipmap = 0)
{
    idStr str(name);
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

ID_INLINE static unsigned int etc1_data_size(unsigned int width, unsigned int height) {
    return (((width + 3) & ~3) * ((height + 3) & ~3)) >> 1;
}

ID_INLINE static unsigned int rgba4444_data_size(unsigned int width, unsigned int height) {
    return width * height * 2;
}

static int isopaque(GLint width, GLint height, const GLvoid *pixels) {
    unsigned char const *cpixels = (unsigned char const *) pixels;
    int i;
    for (i = 0; i < width * height; i++) {
        if (cpixels[i * 4 + 3] != 0xff)
            return 0;
    }
    return 1;
}

#ifdef _OPENGLES3
ID_INLINE static unsigned short CalcExtendedDimension(unsigned short a_ushOriginalDimension)
{
    return (unsigned short)((a_ushOriginalDimension + 3) & ~3);
}

ID_INLINE static unsigned int etc2_data_size(unsigned int width, unsigned int height) {
#ifdef USE_RG_ETC2
    return etc2_data_size_rgba(width, height);
#else
    return (CalcExtendedDimension(width) >> 2) * (CalcExtendedDimension(height) >> 2) * 16/*Block4x4EncodingBits::GetBytesPerBlock(Format::RGBA8)*/;
#endif
}

ID_INLINE static bool R_IsKeepSize(int w)
{
    return w == CalcExtendedDimension(w);
}

ID_INLINE static bool R_IsETC2Supported()
{
    return USING_GLES3;
}

ID_INLINE static bool R_IsETC2Enabled()
{
    return R_IsETC2Supported() && r_useETC2.GetBool();
}

extern void Sys_CPUCount( int& logicalNum, int& coreNum, int& packageNum );
static void etc2_compress_tex_image(const char *cachefname, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    unsigned char *paucEncodingBits;
    unsigned int uiEncodingBitsBytes;
    unsigned int uiExtendedWidth;
    unsigned int uiExtendedHeight;

    static int numPhysicalCpuCores = 1;
    static int numLogicalCpuCores = 1;
    static int numCpuPackages = 1;
    static bool getCPUInfo = false;

    if(!getCPUInfo)
    {
        Sys_CPUCount( numLogicalCpuCores, numPhysicalCpuCores, numCpuPackages );
        getCPUInfo = true;
    }

#ifdef USE_RG_ETC2
    int rsize;
    paucEncodingBits = etc2_encode_image_rgba((const unsigned char*)pixels, width, height, &rsize);
    uiEncodingBitsBytes = rsize;
    uiExtendedWidth = width;
    uiExtendedHeight = height;
#else
    EncodeC((unsigned char *)pixels,
                width, height,
            4, // static_cast<int>(Etc::Image::Format::RGBA8),
                0, // Etc::ErrorMetric::RGBA,
                0, // fEffort,
                numPhysicalCpuCores,
                numLogicalCpuCores,
                &paucEncodingBits, &uiEncodingBitsBytes,
                &uiExtendedWidth, &uiExtendedHeight);
#endif

    qglCompressedTexImage2D(
            target,
            level,
            GL_COMPRESSED_RGBA8_ETC2_EAC,
            width,
            height,
            0,
            uiEncodingBitsBytes,
            paucEncodingBits);
    if (cachefname != 0) {
        imageCompression_t image;
        R_AllocCompression(&image, width, height, IC_ETC2_RGBA8, uiExtendedWidth, uiExtendedHeight, level, uiEncodingBitsBytes, (byte *)paucEncodingBits);
        IC_PRINT("Caching idTech4A++ compression texture ETC2: %dx%d %s.\n", image.header.width, image.header.height, cachefname);
        R_CacheCompression(&image, cachefname);
    }
    free(paucEncodingBits);
}
#endif

void rgba4444_convert_tex_image(const char *cachefname, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    int size = rgba4444_data_size(width, height);
    unsigned char const *cpixels = (unsigned char const *) pixels;
    unsigned short *rgba4444data = (unsigned short *) malloc(size);
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
    qglTexImage2D(target, level, format, width, height, border, format, GL_UNSIGNED_SHORT_4_4_4_4, rgba4444data);
    rgba4444data = (unsigned short *) ((unsigned char *) rgba4444data);
    if (cachefname != 0) {
        imageCompression_t image;
        R_AllocCompression(&image, width, height, IC_RGBA4444, level, size, (byte *)rgba4444data);
		IC_PRINT("Caching idTech4A++ compression texture RGBA4444: %dx%d %s.\n", image.header.width, image.header.height, cachefname);
        R_CacheCompression(&image, cachefname);
    }
    free(rgba4444data);
}

void etc1_compress_tex_image(const char *cachefname, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    unsigned char const *cpixels = (unsigned char const *) pixels;
    unsigned char *etc1data;
    unsigned int size = etc1_data_size(width, height);
    etc1data = (unsigned char *)malloc(size);
#ifdef USE_RG_ETC1
    rg_etc1::etc1_encode_image(cpixels, width, height, 4, width*4, etc1data);
#else
    etc1_encode_image(cpixels, width, height, 4, width * 4, etc1data);
#endif
    qglCompressedTexImage2D(
            target,
            level,
            GL_ETC1_RGB8_OES,
            width,
            height,
            0,
            size,
            etc1data);
    if (cachefname != 0) {
        imageCompression_t image;
        R_AllocCompression(&image, width, height, IC_ETC1, level, size, (byte *)etc1data);
		IC_PRINT("Caching idTech4A++ compression texture ETC1: %dx%d %s.\n", image.header.width, image.header.height, cachefname);
        R_CacheCompression(&image, cachefname);
    }
    free(etc1data);
}

ID_INLINE static int etcavail(const char *cachefname) {
    return (
           r_useETC1Cache.GetBool())
           && (r_useETC1.GetBool())
           && (cachefname != 0)
           && (cachefname[0] != 0)
           && (fileSystem->ReadFile(cachefname, 0, 0) != -1
           );
}

int uploadetc(const char *cachefname, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type) {
    char *tmp;
    int failed = 0;
    int sz = fileSystem->ReadFile(cachefname, (void **)&tmp, 0);
    if(sz)
    {
        imageCompression_t image;
        if(R_LoadCompression((byte *)tmp, sz, &image))
        {
            if(image.header.width == width && image.header.height == height
                // && image.header.mipmap == level
            ) {
                if (image.header.type == IC_ETC1) {
                    IC_PRINT("Loading idTech4A++ compression texture ETC1: %dx%d %s.\n",
                             image.header.width, image.header.height, cachefname);
                    qglCompressedTexImage2D(target, level, GL_ETC1_RGB8_OES, width, height, 0,
                                            image.header.length, image.data);
                } else if (image.header.type == IC_RGBA4444) {
#ifdef _OPENGLES3
                    if(R_IsETC2Enabled())
                    {
                        IC_ERROR("Unmatch idTech4A++ compression texture format: RGBA4444 != ETC2_RGBA (%s).\n", cachefname);
                        failed = 1;
                    }
                    else
                    {
#endif
                        IC_PRINT("Loading idTech4A++ compression texture RGBA4444: %dx%d %s.\n",
                                 image.header.width, image.header.height, cachefname);
                        qglTexImage2D(target, level, format, width, height, border, format,
                                      GL_UNSIGNED_SHORT_4_4_4_4, image.data);

#ifdef _OPENGLES3
                    }
#endif
#ifdef _OPENGLES3
                } else if (image.header.type == IC_ETC2_RGBA8) {
                    if(!R_IsETC2Enabled())
                    {
                        IC_ERROR("Unmatch idTech4A++ compression texture format: ETC2_RGBA != RGBA4444 (%s).\n", cachefname);
                        failed = 1;
                    }
                    else
                    {
                        IC_PRINT("Loading idTech4A++ compression texture ETC2: %dx%d %s.\n",
                                 image.header.width, image.header.height, cachefname);
                        qglCompressedTexImage2D(target, level, GL_COMPRESSED_RGBA8_ETC2_EAC, width,
                                                height, 0, image.header.length, image.data);
                    }
#endif
                } else {
                    IC_ERROR("Unsupport idTech4A++ compression texture format: %d (%s).\n", image.header.type, cachefname);
                    failed = 1;
                }
            }
            else
            {
                IC_ERROR("idTech4A++ compression texture size changed: %u != %d || %u != %d (%s).\n", image.header.width, width, image.header.height, height, cachefname);
                failed = 1;
            }
        }
        else
        {
            failed = 1;
        }
    }
    else
	{
        IC_ERROR("idTech4A++ compression texture missing: %s.\n", cachefname);
        failed = 1;
	}
    fileSystem->FreeFile(tmp);
    return failed;
}

void myglTexImage2D(const char *cachefname, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    static int opaque = 0;
#ifdef _OPENGLES3
    static int useETC2 = 0;
#endif
    if (r_useETC1.GetBool() && format == GL_RGBA && type == GL_UNSIGNED_BYTE) {

        if (level == 0)
        {
            opaque = isopaque(width, height, pixels);
#ifdef _OPENGLES3
            useETC2 = !opaque && R_IsETC2Enabled()/* && target == GL_TEXTURE_2D && R_IsKeepSize(width) && R_IsKeepSize(height)*/;
#endif
        }

        if (!r_useETC1Cache.GetBool())
            cachefname = 0;

        if (opaque)
            etc1_compress_tex_image(cachefname, target, level, format, width, height, border, format, type, pixels);
        else
        {
#ifdef _OPENGLES3
            if(useETC2)
                etc2_compress_tex_image(cachefname, target, level, format, width, height, border, format, type, pixels);
            else
#endif
                rgba4444_convert_tex_image(cachefname, target, level, format, width, height, border, format, type, pixels);
        }
    } else
        qglTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

//end
