// rg_etc1
#ifdef USE_RG_ETC1
#include "ETC1/etc_rg_etc1.cpp"
#else
#include "ETC1/etc_android.cpp"
#endif

#ifdef _OPENGLES3
#ifdef USE_RG_ETC2
#include "ETC1/etc2_encoder.cpp"

static uint32_t GetPixel(const uint8_t* data, int width, int components, int x, int y)
{
    uint32_t out[4];
    out[0] = data[(y * width + x) * components];
    out[1] = data[(y * width + x) * components + 1];
    out[2] = data[(y * width + x) * components + 2];
    out[3] = components == 4 ? data[(y * width + x) * components + 3] : 255;
    return (out[3] << 24) | (out[2] << 16) | (out[1] << 8) | (out[0] << 0);
}

static uint32_t GetPixelClamped(const uint8_t* data, int width, int height, int components, int x, int y)
{
    if (x >= width)
        x = width - 1;
    if (y >= height)
        y = height - 1;
    return GetPixel(data, width, components, x, y);
}

static void CompressImageTask(const uint8_t* image, int width, int height, int components, rg_etc1::etc1_pack_params params, bool useEtc2, bool alpha, int x, int y, uint8_t* destination)
{
    uint32_t pixelBlock[16];
    // Copy pixel block:
    for(int blockY = 0; blockY < 4; ++blockY)
    {
        for(int blockX = 0; blockX < 4; ++blockX)
        {
            pixelBlock[blockY * 4 + blockX] = GetPixelClamped(image, width, height, components, x + blockX, y + blockY);
        }
    }

    // Compress block:
    if(useEtc2)
    {
        if(alpha)
            etc2_rgba8_block_pack(destination, pixelBlock, params);
        else
            etc2_rgb8_block_pack(destination, pixelBlock, params);
    }
    else
    {
        rg_etc1::etc1_pack_params params2 = params;
        rg_etc1::pack_etc1_block(destination, pixelBlock, params2);
    }
}

static uint8_t * CompressImage(const uint8_t* image, int width, int height, int components, rg_etc1::etc1_pack_params params, bool useEtc2, bool alpha, int *size)
{
    const size_t blockSize = (useEtc2 && alpha ? 16 : 8);
    int length = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
    rg_etc1::init_pack_etc1_block();

    uint8_t *output = (uint8_t *)malloc( length );
    uint8_t* destination = output;
    for(int y = 0; y < height; y += 4)
    {
        for(int x = 0; x < width; x += 4)
        {
            CompressImageTask(image, width, height, components, params, useEtc2, alpha, x, y, destination);
            destination += blockSize;
        }
    }
    if(size)
        *size = length;
    return output;
}

inline static etc1_uint32 etc2_get_encoded_data_size(etc1_uint32 width, etc1_uint32 height, bool useEtc2, bool alpha) {
    const size_t blockSize = (useEtc2 && alpha ? 16 : 8);
    return ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
}

unsigned int etc2_data_size_rgba(int width, int height)
{
    return etc2_get_encoded_data_size(width, height, true, true);
}

unsigned int etc2_data_size_rgb(int width, int height)
{
    return etc2_get_encoded_data_size(width, height, true, false);
}

unsigned char * etc2_encode_image_rgba(const unsigned char* image, int width, int height, int *size)
{
    rg_etc1::etc1_pack_params params;
    return CompressImage(image, width, height, 4, params, true, true, size);
}

unsigned char * etc2_encode_image_rgb(const unsigned char* image, int width, int height, int *size)
{
    rg_etc1::etc1_pack_params params;
    return CompressImage(image, width, height, 3, params, true, false, size);
}

#else
// ETCLib
#if 0
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
#endif
#endif

#endif