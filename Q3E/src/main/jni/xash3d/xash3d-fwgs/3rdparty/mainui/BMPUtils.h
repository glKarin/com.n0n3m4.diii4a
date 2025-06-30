/*
BMPUtils.h -- single-header BMP library
Copyright (C) 2018 a1batross

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#pragma once

#define BI_FILE_HEADER_SIZE 14
#define BI_SIZE	40 // size of bitmap info header.
typedef unsigned short       word;

#pragma pack( push, 1 )
struct bmp_t
{
	char id[2];		// bmfh.bfType
	uint fileSize;		// bmfh.bfSize
	uint reserved0;	// bmfh.bfReserved1 + bmfh.bfReserved2
	uint bitmapDataOffset;	// bmfh.bfOffBits
	uint bitmapHeaderSize;	// bmih.biSize
	uint width;		// bmih.biWidth
	int	 height;		// bmih.biHeight
	word planes;		// bmih.biPlanes
	word bitsPerPixel;	// bmih.biBitCount
	uint compression;	// bmih.biCompression
	uint bitmapDataSize;	// bmih.biSizeImage
	uint hRes;		// bmih.biXPelsPerMeter
	uint vRes;		// bmih.biYPelsPerMeter
	uint colors;		// bmih.biClrUsed
	uint importantColors;	// bmih.biClrImportant
};

struct rgbquad_t
{
	byte b;
	byte g;
	byte r;
	byte reserved;
};
#pragma pack( pop )

class CBMP
{
public:
	static CBMP* LoadFile( const char *filename ); // implemented in library!

	CBMP( uint w, uint h )
	{
		bmp_t bhdr;

		const size_t cbPalBytes = 0;
		const uint pixel_size = 4; // RGBA

		bhdr.id[0] = 'B';
		bhdr.id[1] = 'M';
		bhdr.width = ( w + 3 ) & ~3;
		bhdr.height = h;
		bhdr.bitmapHeaderSize = BI_SIZE; // must be 40!
		bhdr.bitmapDataOffset = sizeof( bmp_t ) + cbPalBytes;
		bhdr.bitmapDataSize = bhdr.width * bhdr.height * pixel_size;
		bhdr.fileSize = bhdr.bitmapDataOffset + bhdr.bitmapDataSize;

		// constant
		bhdr.reserved0 = 0;
		bhdr.planes = 1;
		bhdr.bitsPerPixel = pixel_size * 8;
		bhdr.compression = 0;
		bhdr.hRes = bhdr.vRes = 0;
		bhdr.colors = ( pixel_size == 1 ) ? 256 : 0;
		bhdr.importantColors = 0;

		fileAllocated = false;
		data = new byte[bhdr.fileSize];
		memcpy( data, &bhdr, sizeof( bhdr ));
		memset( data + bhdr.bitmapDataOffset, 0, bhdr.bitmapDataSize );
	}

	CBMP( const bmp_t *header, uint img_sz )
	{
		data = new byte[ header->bitmapDataOffset + img_sz ];
		fileAllocated = false;

		// copy the header and palette
		memcpy( data, header, header->bitmapDataOffset );

		// fixup
		bmp_t *hdr = GetBitmapHdr();
		hdr->bitmapDataSize = img_sz;
		hdr->fileSize = hdr->bitmapDataOffset + hdr->bitmapDataSize;
	}

	~CBMP()
	{
		if( data )
		{
			if( fileAllocated )
				EngFuncs::COM_FreeFile( data );
			else
				delete []data;
		}
	}

	void Increase(uint w, uint h)
	{
		bmp_t *hdr = GetBitmapHdr();
		bmp_t bhdr;

		const int pixel_size = 4; // create 32 bit image everytime

		memcpy( &bhdr, hdr, sizeof( bhdr ));

		bhdr.width  = (w + 3) & ~3;
		bhdr.height = h;
		bhdr.bitmapDataSize = bhdr.width * bhdr.height * pixel_size;
		bhdr.fileSize = bhdr.bitmapDataOffset + bhdr.bitmapDataSize;

		// I think that we need to only increase BMP size
		assert( bhdr.width >= hdr->width );
		assert( bhdr.height >= hdr->height );

		byte *newData = new byte[bhdr.fileSize];
		memcpy( newData, &bhdr, sizeof( bhdr ));
		memset( newData + bhdr.bitmapDataOffset, 0, bhdr.bitmapDataSize );

		// now copy texture
		byte *src = GetTextureData();
		byte *dst = newData + bhdr.bitmapDataOffset;

		// to keep texcoords still valid, we copy old texture through the end
		for( int y = 0; y < hdr->height; y++ )
		{
			byte *ydst = &dst[(y + (bhdr.height - hdr->height))* bhdr.width * 4];
			byte *ysrc = &src[y * hdr->width * 4];

			memcpy( ydst, ysrc, 4 * hdr->width );
		}

		delete []data;
		data = newData;
	}

	void RemapLogo( int stripes, const byte *rgb )
	{
		// palette is always right after header
		rgbquad_t *palette = GetPaletteData();

		// no palette
		if( GetBitmapHdr()->bitsPerPixel > 8 )
			return;

		int max_palette_slots = (int)(256 / (float)stripes) * stripes;

		for( int i = 0; i < max_palette_slots; i += stripes )
		{
			double t = (double)i / max_palette_slots;

			t = Q_min( t, 1.0 );

			for( int j = 0; j < stripes; j++ )
			{
				int x = i + j;

				palette[x].r = (byte)(rgb[j * 3 + 0] * t);
				palette[x].g = (byte)(rgb[j * 3 + 1] * t);
				palette[x].b = (byte)(rgb[j * 3 + 2] * t);
			}
		}

		if( stripes == 1 )
			return;

		const bmp_t *hdr = GetBitmapHdr();
		double lines_per_stripe = hdr->height / (double)stripes;
		byte *data = GetTextureData();

		for( int i = 0; i < hdr->height; i++ )
		{
			int stripe = (int)(( hdr->height - i - 1 ) / lines_per_stripe );

			for( int j = 0; j < hdr->width; j++ )
			{
				byte c = data[i * hdr->width + j];
				if( c == 0 )
					continue;

				// remap to the palette
				int idx = ( c / 256.0f ) * max_palette_slots; // remap to limited palette
				idx = (int)((double)( idx ) / stripes) * stripes; // now remap per palette
				idx += stripe; // add stripe index
				data[i * hdr->width + j] = Q_min( idx, max_palette_slots ); //
			}
		}
	}

	inline byte *GetBitmap()
	{
		return data;
	}

	inline bmp_t *GetBitmapHdr()
	{
		return (bmp_t*)data;
	}

	inline byte *GetTextureData()
	{
		return data + GetBitmapHdr()->bitmapDataOffset;
	}

	inline uint GetTextureDataSize()
	{
		return GetBitmapHdr()->bitmapDataSize;
	}

	inline rgbquad_t *GetPaletteData()
	{
		// palette is always right after header
		return (rgbquad_t*)(data + BI_FILE_HEADER_SIZE + GetBitmapHdr()->bitmapHeaderSize);
	}

	inline size_t GetPaletteSize()
	{
		bmp_t *hdr = GetBitmapHdr();

		if( hdr->bitsPerPixel > 8 )
			return 0;

		if( hdr->colors == 0 )
		{
			// silently fixup hdr WTF?
			hdr->colors = 256;
			return ( 1 << hdr->bitsPerPixel ) * sizeof( rgbquad_t );
		}

		return hdr->colors * sizeof( rgbquad_t );
	}

private:
	CBMP( bmp_t *data ) :
		fileAllocated( true ), data( (byte*)data )
	{
	}

	bool fileAllocated;
	byte *data;
};
