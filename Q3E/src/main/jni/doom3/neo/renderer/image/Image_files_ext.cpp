// basic defines and includes
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_ONLY_JPEG // at least for now, only use it for JPEG
#define STBI_NO_STDIO  // images are passed as buffers
#define STBI_ONLY_PNG

#include "../../externlibs/stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../externlibs/stb/stb_image_write.h"

// Load functions
void LoadJPG_stb(const char *filename, unsigned char **pic, int *width, int *height, ID_TIME_T *timestamp)
{
    byte	*fbuffer;
    int	len;

    if (pic) {
        *pic = NULL;		// until proven otherwise
    }

    {
        idFile *f;

        f = fileSystem->OpenFileRead(filename);

        if (!f) {
            return;
        }

        len = f->Length();

        if (timestamp) {
            *timestamp = f->Timestamp();
        }

        if (!pic) {
            fileSystem->CloseFile(f);
            return;	// just getting timestamp
        }

        fbuffer = (byte *)Mem_ClearedAlloc(len);
        f->Read(fbuffer, len);
        fileSystem->CloseFile(f);
    }

    int w=0, h=0, comp=0;
    byte* decodedImageData = stbi_load_from_memory( fbuffer, len, &w, &h, &comp, STBI_rgb_alpha );

    Mem_Free( fbuffer );

    if ( decodedImageData == NULL ) {
        common->Warning( "stb_image was unable to load JPG %s : %s\n",
                         filename, stbi_failure_reason());
        return;
    }

    // *pic must be allocated with R_StaticAlloc(), but stb_image allocates with malloc()
    // (and as there is no R_StaticRealloc(), #define STBI_MALLOC etc won't help)
    // so the decoded data must be copied once
    int size = w*h*4;
    *pic = (byte *)R_StaticAlloc( size );
    memcpy( *pic, decodedImageData, size );
    *width = w;
    *height = h;
    // now that decodedImageData has been copied into *pic, it's not needed anymore
    stbi_image_free( decodedImageData );
}

void LoadPNG(const char *filename, byte **pic, int *width, int *height, ID_TIME_T *timestamp)
{

    byte	*fbuffer;
    int	len;

    if (pic) {
        *pic = NULL;		// until proven otherwise
    }

    {
        idFile *f;

        f = fileSystem->OpenFileRead(filename);

        if (!f) {
            return;
        }

        len = f->Length();

        if (timestamp) {
            *timestamp = f->Timestamp();
        }

        if (!pic) {
            fileSystem->CloseFile(f);
            return;	// just getting timestamp
        }

        fbuffer = (byte *)Mem_ClearedAlloc(len);
        f->Read(fbuffer, len);
        fileSystem->CloseFile(f);
    }

    int w=0, h=0, comp=0;
    byte* decodedImageData = stbi_load_from_memory( fbuffer, len, &w, &h, &comp, STBI_rgb_alpha );

    Mem_Free( fbuffer );

    if ( decodedImageData == NULL ) {
        common->Warning( "stb_image was unable to load PNG %s : %s\n",
                         filename, stbi_failure_reason());
        return;
    }

    // *pic must be allocated with R_StaticAlloc(), but stb_image allocates with malloc()
    // (and as there is no R_StaticRealloc(), #define STBI_MALLOC etc won't help)
    // so the decoded data must be copied once
    int size = w*h*4;
    *pic = (byte *)R_StaticAlloc( size );
    memcpy( *pic, decodedImageData, size );
    *width = w;
    *height = h;
    // now that decodedImageData has been copied into *pic, it's not needed anymore
    stbi_image_free( decodedImageData );
}

#include "../../externlibs/soil/soil_dds_image.h"
void LoadDDS(const char *filename, byte **pic, int *width, int *height, ID_TIME_T *timestamp)
{

    byte	*fbuffer;
    int	len;

    if (pic) {
        *pic = NULL;		// until proven otherwise
    }

    {
        idFile *f;

        f = fileSystem->OpenFileRead(filename);

        if (!f) {
            return;
        }

        len = f->Length();

        if (timestamp) {
            *timestamp = f->Timestamp();
        }

        if (!pic) {
            fileSystem->CloseFile(f);
            return;	// just getting timestamp
        }

        fbuffer = (byte *)Mem_ClearedAlloc(len);
        f->Read(fbuffer, len);
        fileSystem->CloseFile(f);
    }

    int w=0, h=0, comp=0;
    byte* decodedImageData = stbi_dds_load_from_memory( fbuffer, len, &w, &h, &comp, STBI_rgb_alpha );

    Mem_Free( fbuffer );

    if ( decodedImageData == NULL ) {
        common->Warning( "stb_image was unable to load PNG %s : %s\n",
                         filename, stbi_failure_reason());
        return;
    }

    // *pic must be allocated with R_StaticAlloc(), but stb_image allocates with malloc()
    // (and as there is no R_StaticRealloc(), #define STBI_MALLOC etc won't help)
    // so the decoded data must be copied once
    int size = w*h*4;
    *pic = (byte *)R_StaticAlloc( size );
    memcpy( *pic, decodedImageData, size );
    *width = w;
    *height = h;
    // now that decodedImageData has been copied into *pic, it's not needed anymore
    stbi_image_free( decodedImageData );
}

// Utility
static void R_ImageFlipVertical(byte *data, int width, int height, int comp)
{
    int		i;
    byte		*line1, *line2;
    byte *swapBuffer;
    int lineSize = width * comp;

    swapBuffer = (byte *)malloc(width * comp);
    for (i = 0; i < height / 2; ++i) {
        line1 = &data[i * lineSize];
        line2 = &data[(height - i - 1) * lineSize];
        memcpy(swapBuffer, line1, lineSize);
        memcpy(line1, line2, lineSize);
        memcpy(line2, swapBuffer, lineSize);
    }
    free(swapBuffer);
}

#ifdef _HARM_IMAGE_RGBA_TO_RGB
static void R_ImageRGBA8888ToRGB888(const byte *data, int width, int height, byte *to)
{
	int		i, j;
	const byte		*temp;
	byte *target;

	for (i = 0 ; i < width ; i++) {
		for (j = 0 ; j < height ; j++) {
			temp = data + j * 4 * width + i * 4;
			target = to + j * 3 * width + i * 3;
			memcpy(target, temp, 3);
		}
	}
}
#endif

// Save functions
static void R_StbWriteImageFile(void *context, void *data, int size)
{
    idFile *f = (idFile *)context;
    f->Write(data, size);
}

void R_WritePNG(const char *filename, const byte *data, int width, int height, int comp, bool flipVertical, int quality, const char *basePath)
{
    stbi_write_png_compression_level = idMath::ClampInt(0, 9, quality);
    byte *ndata = NULL;
    if(flipVertical)
    {
        ndata = (byte *) malloc(width * height * comp);
        memcpy(ndata, data, width * height * comp);
        R_ImageFlipVertical(ndata, width, height, comp);
        data = ndata;
    }
    if(!basePath)
        basePath = "fs_savepath";
    idFile *f = fileSystem->OpenFileWrite(filename, basePath);
    if(f)
    {
        int r = stbi_write_png_to_func(R_StbWriteImageFile, (void *)f, width, height, comp, data, width * comp);
        free(ndata);
        if(!r)
            common->Warning("R_WritePNG fail: %d", r);
        fileSystem->CloseFile(f);
    }
}

void R_WriteJPG(const char *filename, const byte *data, int width, int height, int comp, bool flipVertical, int compression, const char *basePath)
{
    byte *ndata = NULL;
    if(flipVertical)
    {
        ndata = (byte *) malloc(width * height * comp);
        memcpy(ndata, data, width * height * comp);
        R_ImageFlipVertical(ndata, width, height, comp);
        data = ndata;
    }
    if(!basePath)
        basePath = "fs_savepath";
    idFile *f = fileSystem->OpenFileWrite(filename, basePath);
    if(f)
    {
        int r = stbi_write_jpg_to_func(R_StbWriteImageFile, (void *)f, width, height, comp, data, idMath::ClampInt(1, 100, compression));
        free(ndata);
        if(!r)
            common->Warning("R_WriteJPG fail: %d", r);
        fileSystem->CloseFile(f);
    }
}

void R_WriteBMP(const char *filename, const byte *data, int width, int height, int comp, bool flipVertical, const char *basePath)
{
    byte *ndata = NULL;
    if(flipVertical)
    {
        ndata = (byte *) malloc(width * height * comp);
        memcpy(ndata, data, width * height * comp);
        R_ImageFlipVertical(ndata, width, height, comp);
        data = ndata;
    }
    if(!basePath)
        basePath = "fs_savepath";
    idFile *f = fileSystem->OpenFileWrite(filename, basePath);
    if(f)
    {
        int r = stbi_write_bmp_to_func(R_StbWriteImageFile, (void *)f, width, height, comp, data);
        free(ndata);
        if(!r)
            common->Warning("R_WriteBMP fail: %d", r);
        fileSystem->CloseFile(f);
    }
}

#include "../../externlibs/soil/image_dxt.h"
void R_WriteDDS(const char *filename, const byte *data, int width, int height, int comp, bool flipVertical, const char *basePath)
{
    byte *ndata = NULL;
    if(flipVertical)
    {
        ndata = (byte *) malloc(width * height * comp);
        memcpy(ndata, data, width * height * comp);
        R_ImageFlipVertical(ndata, width, height, comp);
        data = ndata;
    }
    if(!basePath)
        basePath = "fs_savepath";
    idFile *f = fileSystem->OpenFileWrite(filename, basePath);
    if(f)
    {
        int r = soil_save_image_as_func(R_StbWriteImageFile, (void *)f, width, height, comp, data);
        free(ndata);
        if(!r)
            common->Warning("R_WriteDDS fail: %d", r);
        fileSystem->CloseFile(f);
    }
}

void R_WriteScreenshotImage(const char *filename, const byte *data, int width, int height, int comp, bool flipVertical, const char *basePath)
{
    idStr fn(filename);
    switch(r_screenshotFormat.GetInteger())
    {
        case 1: {// bmp
            fn.SetFileExtension("bmp");
            R_WriteBMP(fn.c_str(), data, width, height, comp, flipVertical,  basePath);
        }
            break;
        case 2: {// png
            fn.SetFileExtension("png");
            R_WritePNG(fn.c_str(), data, width, height, comp, flipVertical, idMath::ClampInt(0, 9, r_screenshotPngCompression.GetInteger()), basePath);
        }
            break;
        case 3: {// jpg
            fn.SetFileExtension("jpg");
            R_WriteJPG(fn.c_str(), data, width, height, comp, flipVertical, idMath::ClampInt(1, 100, r_screenshotJpgQuality.GetInteger()), basePath);
        }
            break;
        case 4: {// dds
            fn.SetFileExtension("dds");
            R_WriteDDS(fn.c_str(), data, width, height, comp, flipVertical, basePath);
        }
            break;
        case 0: // tga
        default:
            fn.SetFileExtension("tga");
            R_WriteTGA(fn.c_str(), data, width, height, flipVertical); //TODO: 4 comp
            break;
    }
}

// Convert image
bool R_ConvertImage(const char *filename, const char *toFormat, idStr &ret, int comp = 4, int compression = -1, bool flipVertical = false, bool makePowerOf2 = false, const char *basePath = NULL, int *rwidth = NULL, int *rheight = NULL)
{
    idStr fileNameStr(filename);
    idStr srcExt;
    int width = 0, height = 0;
    byte *pic = NULL;
    ID_TIME_T timestamp;
    bool res;

    if(rwidth)
        *rwidth = -1;
    if(rheight)
        *rheight = -1;

    // PreCheck
    fileNameStr.ExtractFileExtension(srcExt);
    if(srcExt.IsEmpty())
    {
        ret = idStr("Can not get source image extension: ") + filename;
        return false;
    }
    if(!idStr::Icmp(srcExt, toFormat))
    {
        ret = idStr("Source image extension same as target: ") + srcExt;
        return false;
    }

    // Load
    if(!idStr::Icmp(srcExt, "tga"))
    {
        LoadTGA(filename, &pic, &width, &height, &timestamp);
    }
    else if(!idStr::Icmp(srcExt, "jpg") || !idStr::Icmp(srcExt, "jpeg") )
    {
        LoadJPG(filename, &pic, &width, &height, &timestamp);
    }
    else if (!idStr::Icmp(srcExt, "pcx"))
    {
        LoadPCX32(filename, &pic, &width, &height, &timestamp);
    }
    else if(!idStr::Icmp(srcExt, "png"))
    {
        LoadPNG(filename, &pic, &width, &height, &timestamp);
    }
    else if(!idStr::Icmp(srcExt, "dds"))
    {
        LoadDDS(filename, &pic, &width, &height, &timestamp);
    }
    else if(!idStr::Icmp(srcExt, "bmp"))
    {
        LoadBMP(filename, &pic, &width, &height, &timestamp);
    }
    else
    {
        ret = idStr("Unsupport source image format: ") + srcExt;
        return false;
    }

    // Check load
    if ((width < 1) || (height < 1)) {
        if (pic) {
            R_StaticFree(pic);
            ret = idStr("Unable get source image size");
            return false;
        }
    }

    if ((!pic) || (timestamp == -1)) {
        if (pic) {
            R_StaticFree(pic);
        }
        ret = idStr("Unable get source image data");
        return false;
    }

    //
    // convert to exact power of 2 sizes
    //
    if (pic && makePowerOf2) {
        int		w, h;
        int		scaled_width, scaled_height;
        byte	*resampledBuffer;

        w = width;
        h = height;

        for (scaled_width = 1 ; scaled_width < w ; scaled_width<<=1)
            ;

        for (scaled_height = 1 ; scaled_height < h ; scaled_height<<=1)
            ;

        if (scaled_width != w || scaled_height != h) {
            if (globalImages->image_roundDown.GetBool() && scaled_width > w) {
                scaled_width >>= 1;
            }

            if (globalImages->image_roundDown.GetBool() && scaled_height > h) {
                scaled_height >>= 1;
            }

            resampledBuffer = R_ResampleTexture(pic, w, h, scaled_width, scaled_height);
            R_StaticFree(pic);
            pic = resampledBuffer;
            width = scaled_width;
            height = scaled_height;
        }
    }

    // Save
    idStr targetPath;
    if(basePath && basePath[0])
    {
        targetPath = basePath;
        targetPath.AppendPath(filename);
    }
    else
    {
        targetPath = filename;
    }
    targetPath.SetFileExtension(toFormat);
    res = false;

    if(!idStr::Icmp(toFormat, "tga"))
    {
        R_WriteTGA(targetPath.c_str(), pic, width, height, flipVertical);
        res = true;
    }
    else if(!idStr::Icmp(toFormat, "jpg") || !idStr::Icmp(toFormat, "jpeg") )
    {
        if(compression < 0 || compression > 100)
            compression = 100;
        R_WriteJPG(targetPath.c_str(), pic, width, height, comp, flipVertical, compression);
        res = true;
    }
    else if(!idStr::Icmp(toFormat, "png"))
    {
        if(compression < 0 || compression > 9)
            compression = 9;
        R_WritePNG(targetPath.c_str(), pic, width, height, comp, flipVertical, compression);
        res = true;
    }
    else if(!idStr::Icmp(toFormat, "dds"))
    {
        R_WriteDDS(targetPath.c_str(), pic, width, height, comp, flipVertical);
        res = true;
    }
    else if(!idStr::Icmp(toFormat, "bmp"))
    {
        R_WriteBMP(targetPath.c_str(), pic, width, height, comp, flipVertical);
        res = true;
    }
    else
    {
        ret = idStr("Unsupport target image format: ") + toFormat;
    }

    // Free
    if (pic)
        R_StaticFree(pic);
    if(res)
    {
        ret = targetPath;
        if(rwidth)
            *rwidth = width;
        if(rheight)
            *rheight = height;
    }

    return true;
}

// Command functions
/*
 * convertImage <src_image> <dest_format> [
 * 	--directory=<save_directory>
 * 	-d <save_directory>
 *
 * 	--flipVertical
 * 	-f
 *
 * 	--compression=<jpg_compression | png_quality>
 * 	-q <jpg_compression | png_quality>
 *
 * 	--makePowerOf2
 * 	-p
 *
 * 	--component=<component>
 * 	-c <component>
 * ]
 */
void R_ConvertImage_f(const idCmdArgs &args)
{
    if(args.Argc() < 3)
    {
        common->Printf("Usage: convertImage <src_image> <dest_format> [\n"
                       "\t-d <save_directory> --directory=<save_directory> \n"
                       "\t-f --flipVertical \n"
                       "\t-q <jpg_compression | png_quality> --compression=<jpg_compression | png_quality> \n"
                       "\t-c <component> --component=<component> \n"
                       "\t-p --makePowerOf2 \n"
                       "]");
        return;
    }

    const char *filename = args.Argv(1);
    const char *toFormat = args.Argv(2);
    bool flipVertical = false;
    bool makePowerOf2 = false;
    int compression = -1;
    int component = 4;
    idStr basePath;

    int width;
    int height;
    idStr ret;

    for(int i = 3; i < args.Argc(); i++)
    {
        idStr arg = args.Argv(i);
        int argType = 0;
        if(!arg.Cmp("-"))
        {
            i++;
            if(i >= args.Argc())
            {
                common->Warning("Missing short argument");
                break;
            }
            arg = args.Argv(i);
            argType = 1;
        }
        else if(!arg.Cmp("--"))
        {
            i++;
            if(i >= args.Argc())
            {
                common->Warning("Missing long argument");
                break;
            }
            arg = args.Argv(i);
            argType = 2;
        }

#define PARSE_LONG_ARG(what) \
		if(argType != 2) \
		{ \
			common->Warning("Required long argument"); \
            continue; \
		} \
		i++; \
		if(i >= args.Argc()) \
		{ \
			common->Warning("Missing long argument `=` token"); \
			break; \
		} \
		arg = args.Argv(i); \
		if(arg != "=") \
		{ \
			common->Warning("Expected long argument `=` token"); \
            continue; \
		} \
		i++; \
		if(i >= args.Argc()) \
		{ \
			common->Warning("Missing long argument `" what "` value"); \
			break; \
		} \
		arg = args.Argv(i);

#define PARSE_SHORT_ARG(what) \
		if(argType != 1) \
		{ \
			common->Warning("Required short argument"); \
            continue; \
		} \
		i++; \
		if(i >= args.Argc()) \
		{ \
			common->Warning("Missing short argument `" what "` value"); \
			break; \
		} \
		arg = args.Argv(i);

        if(arg == "directory")
        {
            PARSE_LONG_ARG("directory");
            basePath = arg;
        }
        else if(arg == "d")
        {
            PARSE_SHORT_ARG("d");
            basePath = arg;
        }
        else if(arg == "flipVertical" || arg == "f")
        {
            flipVertical = true;
        }
        else if(arg == "makePowerOf2" || arg == "p")
        {
            makePowerOf2 = true;
        }
        else if(arg == "compression")
        {
            PARSE_LONG_ARG("compression");
            compression = atoi(arg.c_str());
        }
        else if(arg == "q")
        {
            PARSE_SHORT_ARG("q");
            compression = atoi(arg.c_str());
        }
        else if(arg == "component")
        {
            PARSE_LONG_ARG("component");
            component = atoi(arg.c_str());
        }
        else if(!arg.Cmp("c"))
        {
            PARSE_SHORT_ARG("c");
            component = atoi(arg.c_str());
        }
        else
        {
            common->Warning("Unknown argument: %s", arg.c_str());
        }
    }

#undef PARSE_LONG_ARG
#undef PARSE_SHORT_ARG

    common->Printf("ConvertImage: %s -> %s\n", filename, toFormat);
    common->Printf("\tdirectory: %s\n", basePath.c_str());
    common->Printf("\tflip vertical: %s\n", flipVertical ? "ON" : "OFF");
    common->Printf("\tmake power of 2: %s\n", makePowerOf2 ? "ON" : "OFF");
    common->Printf("\tcompression: %d\n", compression);
    common->Printf("\tcomponent: %d\n", component);

    if(!idStr::Icmp(toFormat, "png"))
    {
        if(compression < 0 || compression > 9)
            compression = 9;
    }
    else if(!idStr::Icmp(toFormat, "jpg") || !idStr::Icmp(toFormat, "jpeg"))
    {
        if(compression < 0 || compression > 100)
            compression = 100;
    }
    if(component != 3 && component != 4)
        component = 4;

    if(R_ConvertImage(filename, toFormat, ret, component, compression, flipVertical, makePowerOf2, basePath, &width, &height))
    {
        common->Printf("Target image save to %s(%d x %d)\n", ret.c_str(), width, height);
    }
    else
    {
        common->Printf("Convert error: %s\n", ret.c_str());
    }
}