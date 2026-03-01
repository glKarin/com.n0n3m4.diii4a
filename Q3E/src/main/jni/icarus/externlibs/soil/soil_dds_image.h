#ifndef _SOIL_DDS_IMAGE_H
#define _SOIL_DDS_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STBI_NO_STDIO
#define STBI_NO_STDIO
#endif

#define __forceinline ID_INLINE

#define epuc(x,y)  ((unsigned char *) (e(x,y)?(void *)0:(void *)0))

static int e(char *str)
{
    static char *failure_reason;
    failure_reason = str;
    return 0;
}

#ifdef STBI_NO_FAILURE_STRINGS
#define e(x,y)  0
#elif defined(STBI_FAILURE_USERMSG)
#define e(x,y)  e(y)
#else
#define e(x,y)  e(x)
#endif

// implementation:
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef   signed short  int16;
typedef unsigned int   uint32;
typedef   signed int    int32;
typedef unsigned int   uint;

typedef unsigned char stbi_uc;

typedef struct
{
    uint32 img_x, img_y;
    int img_n, img_out_n;

#ifndef STBI_NO_STDIO
    FILE  *img_file;
#endif
    uint8 *img_buffer, *img_buffer_end;
} stbi;

__forceinline static int get8(stbi *s)
{
#ifndef STBI_NO_STDIO
    if (s->img_file) {
      int c = fgetc(s->img_file);
      return c == EOF ? 0 : c;
   }
#endif
    if (s->img_buffer < s->img_buffer_end)
        return *s->img_buffer++;
    return 0;
}

static int get16le(stbi *s)
{
    int z = get8(s);
    return z + (get8(s) << 8);
}

static uint32 get32le(stbi *s)
{
    uint32 z = get16le(s);
    return z + (get16le(s) << 16);
}

static void start_mem(stbi *s, uint8 const *buffer, int len)
{
#ifndef STBI_NO_STDIO
    s->img_file = NULL;
#endif
    s->img_buffer = (uint8 *) buffer;
    s->img_buffer_end = (uint8 *) buffer+len;
}

static void skip(stbi *s, int n)
{
#ifndef STBI_NO_STDIO
    if (s->img_file)
      fseek(s->img_file, n, SEEK_CUR);
   else
#endif
    s->img_buffer += n;
}

static void getn(stbi *s, stbi_uc *buffer, int n)
{
#ifndef STBI_NO_STDIO
    if (s->img_file) {
      fread(buffer, 1, n, s->img_file);
      return;
   }
#endif
    memcpy(buffer, s->img_buffer, n);
    s->img_buffer += n;
}

static uint8 compute_y(int r, int g, int b)
{
    return (uint8) (((r*77) + (g*150) +  (29*b)) >> 8);
}

static unsigned char *convert_format(unsigned char *data, int img_n, int req_comp, uint x, uint y)
{
    int i,j;
    unsigned char *good;

    if (req_comp == img_n) return data;
    assert(req_comp >= 1 && req_comp <= 4);

    good = (unsigned char *) malloc(req_comp * x * y);
    if (good == NULL) {
        free(data);
        return epuc("outofmem", "Out of memory");
    }

    for (j=0; j < (int) y; ++j) {
        unsigned char *src  = data + j * x * img_n   ;
        unsigned char *dest = good + j * x * req_comp;

#define COMBO(a,b)  ((a)*8+(b))
#define CASE(a,b)   case COMBO(a,b): for(i=x-1; i >= 0; --i, src += a, dest += b)
        // convert source image with img_n components to one with req_comp components;
        // avoid switch per pixel, so use switch per scanline and massive macros
        switch(COMBO(img_n, req_comp)) {
            CASE(1,2) dest[0]=src[0], dest[1]=255; break;
            CASE(1,3) dest[0]=dest[1]=dest[2]=src[0]; break;
            CASE(1,4) dest[0]=dest[1]=dest[2]=src[0], dest[3]=255; break;
            CASE(2,1) dest[0]=src[0]; break;
            CASE(2,3) dest[0]=dest[1]=dest[2]=src[0]; break;
            CASE(2,4) dest[0]=dest[1]=dest[2]=src[0], dest[3]=src[1]; break;
            CASE(3,4) dest[0]=src[0],dest[1]=src[1],dest[2]=src[2],dest[3]=255; break;
            CASE(3,1) dest[0]=compute_y(src[0],src[1],src[2]); break;
            CASE(3,2) dest[0]=compute_y(src[0],src[1],src[2]), dest[1] = 255; break;
            CASE(4,1) dest[0]=compute_y(src[0],src[1],src[2]); break;
            CASE(4,2) dest[0]=compute_y(src[0],src[1],src[2]), dest[1] = src[3]; break;
            CASE(4,3) dest[0]=src[0],dest[1]=src[1],dest[2]=src[2]; break;
            default: assert(0);
        }
#undef CASE
    }

    free(data);
    return good;
}

#include "stbi_dds_aug.h"
#include "image_dxt.h"
#include "stbi_dds_aug_c.h"

#ifdef __cplusplus
};
#endif

#endif /* _SOIL_DDS_IMAGE_H	*/
