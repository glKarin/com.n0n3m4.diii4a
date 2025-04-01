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

#ifndef __COMPRESSOR_H__
#define __COMPRESSOR_H__

/*
===============================================================================

	idCompressor is a layer ontop of idFile which provides lossless data
	compression. The compressor can be used as a regular file and multiple
	compressors can be stacked ontop of each other.

===============================================================================
*/

class idCompressor : public idFile {
public:
							// compressor allocation
	static idCompressor *	AllocNoCompression( void );
	static idCompressor *	AllocBitStream( void );
	static idCompressor *	AllocRunLength( void );
	static idCompressor *	AllocRunLength_ZeroBased( void );
	static idCompressor *	AllocHuffman( void );
	static idCompressor *	AllocArithmetic( void );
	static idCompressor *	AllocLZSS( void );
	static idCompressor *	AllocLZSS_WordAligned( void );
	static idCompressor *	AllocLZW( void );

							// initialization
	virtual void			Init( idFile *f, bool compress, int wordLength ) = 0;
	virtual void			FinishCompress( void ) = 0;
	virtual float			GetCompressionRatio( void ) const = 0;

							// common idFile interface
	virtual const char *	GetName( void ) override = 0;
	virtual const char *	GetFullPath( void ) override = 0;
	virtual int				Read( void *outData, int outLength ) override = 0;
	virtual int				Write( const void *inData, int inLength ) override = 0;
	virtual int				Length( void ) override = 0;
	virtual ID_TIME_T		Timestamp( void ) override = 0;
	virtual int				Tell( void ) override = 0;
	virtual void			ForceFlush( void ) override = 0;
	virtual void			Flush( void ) override = 0;
	virtual int				Seek( long offset, fsOrigin_t origin ) override = 0;
};

#endif /* !__COMPRESSOR_H__ */
