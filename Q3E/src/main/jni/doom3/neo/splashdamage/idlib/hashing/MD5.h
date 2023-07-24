// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __MD5_H__
#define __MD5_H__

/*
===============================================================================

	Calculates a checksum for a block of data
	using the MD5 message-digest algorithm.

===============================================================================
*/

struct md5Context_t {
	unsigned int	state[4];
	unsigned int	bits[2];
	unsigned char	in[64];
};

unsigned long MD5_BlockChecksum( const void *data, int length );

void MD5_StartChecksum( md5Context_t& context );
void MD5_UpdateChecksum( md5Context_t& context, const void *data, int length );
unsigned long MD5_FinishChecksum( md5Context_t& context );
unsigned long MD5_FinishChecksum( md5Context_t& context, unsigned char digest[16] );

#endif /* !__MD5_H__ */
