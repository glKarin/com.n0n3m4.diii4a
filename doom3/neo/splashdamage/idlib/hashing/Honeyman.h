// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __HONEYMAN_H__
#define __HONEYMAN_H__

/*
===============================================================================

	Calculates a checksum for a block of data
	using the simplified version of the pathalias hashing
	function by Steve Belovin and Peter Honeyman.

===============================================================================
*/

void Honeyman_InitChecksum( unsigned /* 64long */int &crcvalue );
void Honeyman_UpdateChecksum( unsigned /* 64long */int &crcvalue, const byte data );
void Honeyman_UpdateChecksum( unsigned /* 64long */int &crcvalue, const void *data, int length );
void Honeyman_FinishChecksum( unsigned /* 64long */int &crcvalue );
unsigned /* 64long */int Honeyman_BlockChecksum( const void *data, int length );

#endif /* !__HONEYMAN_H__ */
