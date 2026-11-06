/*
===========================================================================

openQ4 ETC2 / EAC encoder.

===========================================================================
*/

#ifndef __ETCCODEC_H__
#define __ETCCODEC_H__

/*
================================================================================================

idEtcEncoder

ETC2 and EAC block compression, for the OpenGL ES devices that expose no S3TC
at all and would otherwise carry every texture as uncompressed RGBA8.

Shaped after idDxtEncoder so the two are interchangeable at the call site in
idBinaryImage::Load2DFromMemory: same argument order, same expectation that the
caller has already padded width and height up to a multiple of four.

What this emits today is the ETC1 subset of ETC2 -- individual and differential
modes only. Those are valid ETC2 blocks that every ES 3.0 decoder accepts, and
they carry the whole 4-bits-per-pixel size win, which is the point of the
exercise. ETC2's added T, H and planar modes buy quality on sharp colour
transitions and smooth gradients, not size; they can be added inside these same
entry points later without touching anything that calls them.

================================================================================================
*/
class idEtcEncoder {
public:
	// 4 bpp. Opaque RGB; any alpha in the source is discarded.
	// Writes ( width / 4 ) * ( height / 4 ) * 8 bytes.
	void	CompressImageETC2_RGB8( const byte *inBuf, byte *outBuf, int width, int height ) const;

	// 8 bpp. EAC alpha block followed by the ETC2 colour block, which is the
	// order the format defines. Writes ( width / 4 ) * ( height / 4 ) * 16 bytes.
	void	CompressImageETC2_RGBA8( const byte *inBuf, byte *outBuf, int width, int height ) const;
	// normal maps: X from red, Y from green, Z rebuilt in the shader
	void	CompressImageEAC_RG11( const byte *inBuf, byte *outBuf, int width, int height ) const;
};

#endif /* !__ETCCODEC_H__ */
