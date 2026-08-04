/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop

#include "renderer/resources/Image.h"
#include "tests/testing.h"


bool IsImageFormatCompressed( int internalFormat ) {
	if (
		internalFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
		internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
		internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
		internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ||
		internalFormat == GL_COMPRESSED_RG_RGTC2
	) {
		return true;
	}
	return false;
}

int SizeOfCompressedImage( int width, int height, int internalFormat ) {
	assert( IsImageFormatCompressed( internalFormat ) );
	int numBlocks = ( (width + 3) / 4 ) * ( (height + 3) / 4 );
	int bytesPerBlock = 16;
	if (internalFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT || internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
		bytesPerBlock = 8;
	return numBlocks * bytesPerBlock;
}

void CompressImage( int internalFormat, byte *compressedPtr, const byte *srcPtr, int width, int height, int stride ) {
	// replace zero stride with default stride
	if ( stride == 0 )
		stride = 4 * width;

	if ( internalFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT )
		SIMDProcessor->CompressDXT1FromRGBA8( srcPtr, width, height, stride, compressedPtr );
	else if ( internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT )
		SIMDProcessor->CompressDXT3FromRGBA8( srcPtr, width, height, stride, compressedPtr );
	else if ( internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT )
		SIMDProcessor->CompressDXT5FromRGBA8( srcPtr, width, height, stride, compressedPtr );
	else if ( internalFormat == GL_COMPRESSED_RG_RGTC2 )
		SIMDProcessor->CompressRGTCFromRGBA8( srcPtr, width, height, stride, compressedPtr );
	else {
		// note: no need to support DXT1 with transparency: it should never be used automatically
		assert( false );
		memset( compressedPtr, 0, SizeOfCompressedImage( width, height, internalFormat ) );
	}
}

void DecompressImage( int internalFormat, const byte *compressedPtr, byte *dstPtr, int width, int height, int stride ) {
	// replace zero stride with default stride
	if ( stride == 0 )
		stride = 4 * width;

	if ( internalFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT )
		SIMDProcessor->DecompressRGBA8FromDXT1( compressedPtr, width, height, dstPtr, stride, false );
	else if ( internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT )
		SIMDProcessor->DecompressRGBA8FromDXT1( compressedPtr, width, height, dstPtr, stride, true );
	else if ( internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT )
		SIMDProcessor->DecompressRGBA8FromDXT3( compressedPtr, width, height, dstPtr, stride );
	else if ( internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT )
		SIMDProcessor->DecompressRGBA8FromDXT5( compressedPtr, width, height, dstPtr, stride );
	else if ( internalFormat == GL_COMPRESSED_RG_RGTC2 )
		SIMDProcessor->DecompressRGBA8FromRGTC( compressedPtr, width, height, dstPtr, stride );
	else {
		assert( 0 );
		memset( dstPtr, 0, width * height * 4 );
	}
}

//========================================================================================================================

static void TestDecompressOnImage(int W, int H, const idList<byte>& inputUnc, GLenum compressedFormat, bool checkPerfo = false) {
	qglGetError();

	idList<byte> openglUnc;
	openglUnc.SetNum(W * H * 4);
	idList<byte> openglComp;
	openglComp.SetNum(SizeOfCompressedImage(W, H, compressedFormat));

	qglBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	qglBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	CHECK(qglGetError() == 0);

	GLuint tex;
	qglGenTextures(1, &tex);
	CHECK(qglGetError() == 0);
	qglBindTexture(GL_TEXTURE_2D, tex);
	qglTexImage2D(GL_TEXTURE_2D, 0, compressedFormat, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, inputUnc.Ptr());
	CHECK(qglGetError() == 0);
	qglGetCompressedTexImage(GL_TEXTURE_2D, 0, openglComp.Ptr());
	CHECK(qglGetError() == 0);
	qglGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, openglUnc.Ptr());
	CHECK(qglGetError() == 0);
	qglBindTexture(GL_TEXTURE_2D, 0);
	qglDeleteTextures(1, &tex);
	CHECK(qglGetError() == 0);

	idList<byte> softUnc;
	softUnc.SetNum(W * H * 4);
	const int TRIES = checkPerfo ? 5 : 1;
	for (int ntry = 0; ntry < TRIES; ntry++) {
		double startClock = Sys_GetClockTicks();
		DecompressImage(compressedFormat, openglComp.Ptr(), softUnc.Ptr(), W, H);
		double endClock = Sys_GetClockTicks();
		if (checkPerfo && ntry == TRIES-1) {
			MESSAGE(va("Decompressed (%d x %d) image from format %x in %0.3lf ms",
				W, H, compressedFormat, 1e+3 * (endClock - startClock) / Sys_ClockTicksPerSecond()
			));
		}
	}

	idList<int> wrongBytes;
	for (int i = 0; i < W * H * 4; i++) {
		int delta = idMath::Abs(0 + openglUnc[i] - softUnc[i]);
		//note: it is very hard to make image perfectly match due to various quantization/rounding errors
		//I'm not even sure OpenGL defines DXT -> RGBA8 conversion exactly
		if (delta > 1)
			wrongBytes.AddGrow(i);
	}

#ifdef _DEBUG
#define IMSAVE(varname)	idImageWriter().Source(varname.Ptr(), W, H).Dest(fileSystem->OpenFileWrite("temp_" #varname ".tga")).WriteTGA();
	if (wrongBytes.Num()) {
		IMSAVE(inputUnc);
		IMSAVE(openglUnc);
		IMSAVE(softUnc);
	}
#endif

	CHECK(wrongBytes.Num() == 0);
}


static void TestCompressOnImage(int W, int H, const idList<byte>& inputUnc, GLenum compressedFormat, float maxAvgError, bool checkPerfo = false) {
	idList<byte> outputComp;
	outputComp.SetNum(SizeOfCompressedImage(W, H, compressedFormat));
	idList<byte> checkUnc;
	checkUnc.SetNum(W * H * 4);

	const int TRIES = checkPerfo ? 5 : 1;
	for (int ntry = 0; ntry < TRIES; ntry++) {
		double startClock = Sys_GetClockTicks();
		CompressImage(compressedFormat, outputComp.Ptr(), inputUnc.Ptr(), W, H);
		double endClock = Sys_GetClockTicks();
		if (checkPerfo && ntry == TRIES-1) {
			MESSAGE(va("Compressed (%d x %d) image to format %x in %0.3lf ms",
				W, H, compressedFormat, 1e+3 * (endClock - startClock) / Sys_ClockTicksPerSecond()
			));
		}
	}

	{
		//SSE-optimized and generic implementations produce exactly the same result
		//it greatly simplifies troubleshooting and debugging, since generic code is more understandable
		idSIMDProcessor *genericProcessor = idSIMD::CreateProcessor("generic");
		idList<byte> outputGeneric;
		outputGeneric.SetNum(outputComp.Num());

		std::swap(SIMDProcessor, genericProcessor);
		CompressImage(compressedFormat, outputGeneric.Ptr(), inputUnc.Ptr(), W, H);
		std::swap(SIMDProcessor, genericProcessor);

		delete genericProcessor;
		int cmp = memcmp(outputGeneric.Ptr(), outputComp.Ptr(), outputGeneric.Num());
		CHECK(cmp == 0);
	}

	//we rely on this function being correct!
	DecompressImage(compressedFormat, outputComp.Ptr(), checkUnc.Ptr(), W, H);

	double sqError = 0;
	for (int i = 0; i < W * H * 4; i++) {
		if (compressedFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT && (i % 4) >= 3)
			continue;	// DXT1: no alpha
		if (compressedFormat == GL_COMPRESSED_RG_RGTC2 && (i % 4) >= 2)
			continue;	// RGTC: no blue and alpha
		int delta = idMath::Abs(0 + checkUnc[i] - inputUnc[i]);
		sqError += delta * delta;
	}
	sqError /= (W * H);
	float avgError = idMath::Sqrt(sqError);

#ifdef _DEBUG
#define IMSAVE(varname)	idImageWriter().Source(varname.Ptr(), W, H).Dest(fileSystem->OpenFileWrite("temp_" #varname ".tga")).WriteTGA();
	if (avgError > maxAvgError) {
		IMSAVE(inputUnc);
		IMSAVE(checkUnc);
	}
#endif

	//compression inevitably results in precision loss
	//we can't do exact comparison, so we do a very basic quality test instead
	CHECK(avgError <= maxAvgError);
}

static idList<byte> GenImageConstant(int W, int H, int R, int G, int B, int A) {
	idList<byte> res;
	res.SetNum(W * H * 4);
	for (int i = 0; i < W*H; i++) {
		res[4 * i + 0] = R;
		res[4 * i + 1] = G;
		res[4 * i + 2] = B;
		res[4 * i + 3] = A;
	}
	return res;
}

static idList<byte> GenImageGradient(int W, int H) {
	idList<byte> res;
	res.SetNum(W * H * 4);
	int pos = 0;
	for (int i = 0; i < H; i++)
		for (int j = 0; j < W; j++) {
			res[pos++] = i * 255 / (H - 1);
			res[pos++] = (W - 1 - j) * 255 / (W - 1);
			res[pos++] = (i + j) * 255 / (H + W - 2);
			res[pos++] = (H - 1 - i + j) * 255 / (H + W - 2);
		}
	return res;
}

static idList<byte> GenImageRandom(int W, int H, idRandom &rnd) {
	idList<byte> res;
	res.SetNum(W * H * 4);
	for (int i = 0; i < res.Num(); i++)
		res[i] = rnd.RandomInt(256);
	return res;
}

static idList<byte> GenImageCopy(int W, int H, const byte *pic) {
	idList<byte> res;
	res.SetNum(W * H * 4);
	memcpy(res.Ptr(), pic, W * H * 4);
	return res;
}

static void TestDecompressDxt(bool checkPerfo) {
	idRandom rnd;
	static const GLenum FORMATS[] = {
		GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
		GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
		GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
		GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
		GL_COMPRESSED_RG_RGTC2,
		0
	};
	for (int f = 0; FORMATS[f]; f++) {
		GLenum format = FORMATS[f];
		if (checkPerfo) {
			//check performance
			TestDecompressOnImage(1024, 1024, GenImageRandom(1024, 1024, rnd), format, true);
		}
		else {
			//check correctness
			TestDecompressOnImage(16, 16, GenImageConstant(16, 16, 255, 255, 255, 255), format);
			TestDecompressOnImage(16, 16, GenImageConstant(16, 16, 100, 128, 127, 197), format);
			TestDecompressOnImage(16, 16, GenImageConstant(16, 16, 100, 128, 127, 197), format);
			TestDecompressOnImage(16, 16, GenImageGradient(16, 16), format);
			TestDecompressOnImage(512, 512, GenImageGradient(512, 512), format);
			TestDecompressOnImage(23, 17, GenImageGradient(23, 17), format);
			TestDecompressOnImage(21, 18, GenImageGradient(21, 18), format);
			TestDecompressOnImage(22, 18, GenImageGradient(22, 18), format);
			TestDecompressOnImage(16, 16, GenImageRandom(16, 16, rnd), format);
			TestDecompressOnImage(231, 177, GenImageRandom(231, 177, rnd), format);
		}
	}
}

static void TestCompressDxt(bool checkPerfo) {
	idRandom rnd;
	static const GLenum FORMATS[] = {
		GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
		GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
		GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
		GL_COMPRESSED_RG_RGTC2,
		0
	};

	for (int f = 0; FORMATS[f]; f++) {
		GLenum format = FORMATS[f];
		if (checkPerfo) {
			//check performance
			TestCompressOnImage(1024, 1024, GenImageGradient(1024, 1024), format, 10.0f, true);
		}
		else {
			//check correctness
			TestCompressOnImage(16, 16, GenImageConstant(16, 16, 255, 255, 255, 255), format, 0.0f);
			TestCompressOnImage(16, 16, GenImageConstant(16, 16, 0, 0, 0, 0), format, 0.0f);
			{
				float errorCap = 6.0f;	//RGB565 error
				if (format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
					errorCap = 10.0f;	//+ alpha to 4-bit
				if (format == GL_COMPRESSED_RG_RGTC2)
					errorCap = 0.0f;	//key colors saved exactly
				TestCompressOnImage(16, 16, GenImageConstant(16, 16, 100, 128, 127, 197), format, errorCap);
			}

			{
				float errorCap = 40.0f; //high variarion -> pretty bad error
				if (format == GL_COMPRESSED_RG_RGTC2)
					errorCap = 5.0f;	//less quantization
				TestCompressOnImage(16, 16, GenImageGradient(16, 16), format, errorCap);
				TestCompressOnImage(23, 17, GenImageGradient(23, 17), format, errorCap);
				TestCompressOnImage(21, 18, GenImageGradient(21, 18), format, errorCap);
				TestCompressOnImage(22, 18, GenImageGradient(22, 18), format, errorCap);
			}
			{
				float errorCap = 4.0f;
				if (format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
					errorCap = 8.0f;
				if (format == GL_COMPRESSED_RG_RGTC2)
					errorCap = 0.0f;	//encodes 2 exact 8-bit colors per component
				TestCompressOnImage(512, 512, GenImageGradient(512, 512), format, errorCap);
			}

			TestCompressOnImage(16, 16, GenImageRandom(16, 16, rnd), format, 150.0f);
			TestCompressOnImage(231, 177, GenImageRandom(231, 177, rnd), format, 150.0f);

			//test on some images from assets
			byte *pic;
			int w, h;
			{
				//RGB=255 and alpha defines text font
				float errorCap = 0.0f;	//no error when alpha is ignored
				if (format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
					errorCap = 2.5f;
				if (format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
					errorCap = 4.0f;	//worse than DXT5 because alpha is high-frequency

				idImageReader().Source(fileSystem->OpenFileRead("textures/consolefont.tga")).Dest(pic, w, h).LoadExtension();
				TestCompressOnImage(w, h, GenImageCopy(w, h, pic), format, errorCap);
				Mem_Free(pic);
			}
			{
				//sample photo from Kodak benchmark
				float errorCap = 12.0f;
				if (format == GL_COMPRESSED_RG_RGTC2)
					errorCap = 6.0f;

				idImageReader().Source(fileSystem->OpenFileRead("textures/test_images/kodak_sample1.bmp")).Dest(pic, w, h).LoadExtension();
				TestCompressOnImage(w, h, GenImageCopy(w, h, pic), format, errorCap);
				Mem_Free(pic);
			}
			{
				//TDM texture with smooth alpha channel (not just on/off)
				float errorCap = 3.0f;
				if (format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
					errorCap = 5.0f;
				if (format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
					errorCap = 4.0f;
				if (format == GL_COMPRESSED_RG_RGTC2)
					errorCap = 0.3f;

				idImageReader().Source(fileSystem->OpenFileRead("textures/darkmod/sfx/bc_sparx.tga")).Dest(pic, w, h).LoadExtension();
				TestCompressOnImage(w, h, GenImageCopy(w, h, pic), format, errorCap);
				Mem_Free(pic);
			}
			{
				//alpha-tested TDM texture
				float errorCap = 6.0f;
				if (format == GL_COMPRESSED_RG_RGTC2)
					errorCap = 2.0f;

				idImageReader().Source(fileSystem->OpenFileRead("textures/darkmod/decals/vegetation/cattail_blades_atlas.tga")).Dest(pic, w, h).LoadExtension();
				TestCompressOnImage(w, h, GenImageCopy(w, h, pic), format, errorCap);
				Mem_Free(pic);
			}
		}
	}
}

TEST_CASE("DecompressDxt:Correctness") {
	TestDecompressDxt(false);
}

TEST_CASE("DecompressDxt:Performance"
	* doctest::skip()
) {
	TestDecompressDxt(true);
}

TEST_CASE("CompressDxt:Quality") {
	TestCompressDxt(false);
}

TEST_CASE("CompressDxt:Performance"
	* doctest::skip()
) {
	TestCompressDxt(true);
}
