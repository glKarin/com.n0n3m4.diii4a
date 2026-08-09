/*
===========================================================================

DarklightNG Source Code
Copyright (C) 2026 - Justin Marshall(aka IceColdDuke).

This file is part of the DarklightNG GPL source code.
This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

DarklightNG is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

DarklightNG is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

===========================================================================
*/

#ifndef __MEGATEXTURE_CODEC_H__
#define __MEGATEXTURE_CODEC_H__

class idBareDCTHuffmanTable {
public:
	idBareDCTHuffmanTable();
	void Init( const byte *bits, const byte *values );

	int minCode[17];
	int maxCode[18];
	int valueOffset[17];
	byte symbols[256];
};

class idBareDctBase {
public:
	idBareDctBase();
	virtual ~idBareDctBase();

	void SetQuality( int luminanceQuality, int chrominanceQuality, int alphaQuality );
	void SetQuality_Generic( int luminanceQuality, int chrominanceQuality, int alphaQuality );
	void SetQuality_MMX( int luminanceQuality, int chrominanceQuality, int alphaQuality );
	void SetQuality_SSE2( int luminanceQuality, int chrominanceQuality, int alphaQuality );
	void SetQuality_Xenon( int luminanceQuality, int chrominanceQuality, int alphaQuality );

protected:
	void InitHuffmanTable();
	void InitQuantTable();
	static int QuantizationScaleFromQuality( int quality );
	static void ScaleQuantTable( unsigned short *result, const byte *standard, int scale );

	int luminanceQuality;
	int chrominanceQuality;
	int alphaQuality;
	unsigned short quantTableY[64];
	unsigned short quantTableCoCg[64];
	unsigned short quantTableA[64];
	idBareDCTHuffmanTable huffTableYDC;
	idBareDCTHuffmanTable huffTableYAC;
	idBareDCTHuffmanTable huffTableCoCgDC;
	idBareDCTHuffmanTable huffTableCoCgAC;
	idBareDCTHuffmanTable huffTableADC;
	idBareDCTHuffmanTable huffTableAAC;
};

class idBareDctDecoder : public idBareDctBase {
public:
	idBareDctDecoder();
	virtual ~idBareDctDecoder();

	bool DecompressImageMono( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressImageMono_Generic( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressImageRGB( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressImageRGB_Generic( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressImageRGB_MMX( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressImageRGB_SSE2( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressImageRGB_Xenon( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressImageRGBA( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressImageRGBA_Generic( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressImageRGBA_MMX( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressImageRGBA_SSE2( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressImageRGBA_Xenon( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressImageYCoCg( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressImageYCoCg_Generic( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressImageYCoCg_MMX( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressImageYCoCg_SSE2( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressImageYCoCg_Xenon( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressLuminanceEnhancement( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressLuminanceEnhancement_Generic( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressLuminanceEnhancement_MMX( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressLuminanceEnhancement_SSE2( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );
	bool DecompressLuminanceEnhancement_Xenon( const byte *inBuf, byte *outBuf, int width, int height, int inputBytes );

private:
	bool DecompressColorImage( const byte *inBuf, byte *outBuf, int width, int height,
							int inputBytes, bool alpha, bool storeYCoCg );
	bool BeginImage( const byte *inBuf, int width, int height, int inputBytes );
	bool FillBitBuffer( int count );
	int GetBits( int count );
	int DecodeSymbol( const idBareDCTHuffmanTable &table );
	int ValueFromCategory( int value, int category ) const;
	bool HuffmanDecode( short *coefficients, const idBareDCTHuffmanTable &dcTable,
						const idBareDCTHuffmanTable &acTable, int &lastDC );
	void InverseDCT( const short *coefficients, const unsigned short *quant, short *output ) const;
	bool DecompressMacroBlock( short blocks[10][64], bool alpha );
	void StoreRGBMacroBlock( short blocks[10][64], byte *output, int outputWidth,
							int originX, int originY, int width, int height, bool alpha, bool storeYCoCg );

	const byte *data;
	int dataBytes;
	int dataOffset;
	unsigned int getBuff;
	int getBits;
	bool bitError;
	int dcY;
	int dcCo;
	int dcCg;
	int dcA;
};

// Encoder for the headerless DCT stream consumed by idBareDctDecoder.  The
// three quality bytes are deliberately not written here; MegaTexture records
// store those immediately before the compressed payload.
class idBareDctEncoder : public idBareDctBase {
public:
	idBareDctEncoder();

	bool CompressImageRGB( const byte *inBuf, byte *outBuf, int width, int height,
						int outputCapacity, int &outputBytes );
	bool CompressImageRGBA( const byte *inBuf, byte *outBuf, int width, int height,
						 int outputCapacity, int &outputBytes );

private:
	struct huffmanCode_t {
		unsigned short code;
		byte bits;
	};

	void BuildCodeTable( huffmanCode_t table[256], const byte *bits, const byte *values );
	bool CompressColorImage( const byte *inBuf, byte *outBuf, int width, int height,
						 int outputCapacity, int &outputBytes, bool alpha );
	void ForwardDCT( const short *input, const unsigned short *quant, short *coefficients ) const;
	bool EncodeBlock( const short *input, const unsigned short *quant,
					  const huffmanCode_t dcTable[256], const huffmanCode_t acTable[256], int &lastDC );
	bool PutBits( unsigned int value, int count );
	bool FlushBits();
	static int Category( int value );

	huffmanCode_t codeYDC[256];
	huffmanCode_t codeYAC[256];
	huffmanCode_t codeCoCgDC[256];
	huffmanCode_t codeCoCgAC[256];
	byte *output;
	int capacity;
	int offset;
	unsigned int pendingByte;
	int pendingBits;
	bool overflow;
	int dcY;
	int dcCo;
	int dcCg;
	int dcA;
};

class idDxtEncoder {
public:
	idDxtEncoder();
	bool CompressImageDXT1Fast_Generic( const byte *inBuf, byte *outBuf, int width, int height, int &outputBytes );
	bool CompressImageDXT5Fast_Generic( const byte *inBuf, byte *outBuf, int width, int height, int &outputBytes );
	bool CompressImageDXT1Fast_MMX( const byte *inBuf, byte *outBuf, int width, int height, int &outputBytes );
	bool CompressImageDXT5Fast_MMX( const byte *inBuf, byte *outBuf, int width, int height, int &outputBytes );
	bool CompressImageDXT1Fast_SSE2( const byte *inBuf, byte *outBuf, int width, int height, int &outputBytes );
	bool CompressImageDXT5Fast_SSE2( const byte *inBuf, byte *outBuf, int width, int height, int &outputBytes );
	bool CompressImageDXT1Fast_Xenon( const byte *inBuf, byte *outBuf, int width, int height, int &outputBytes );
	bool CompressImageDXT5Fast_Xenon( const byte *inBuf, byte *outBuf, int width, int height, int &outputBytes );

private:
	void CompressColorBlock( const byte *block, byte *out, bool allowTransparent ) const;
	void CompressAlphaBlock( const byte *block, byte *out ) const;
};

class idMipMap {
public:
	static void CreateMips( byte *data, int numLevels );
	static void CreateMips_MMX( byte *data, int numLevels );
	static void CreateMips_SSE2( byte *data, int numLevels );
};

void MegaTextureUpscale2xBicubic( const byte *source, int width, int height, int stride, byte *destination );
void MegaTextureConvertYCoCgToRGB( byte *image, int width, int height );
int MegaTextureMipChainBytes();

#endif
