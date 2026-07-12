#ifndef __CRC32_H__
#define __CRC32_H__

/*
===============================================================================

	Calculates a checksum for a block of data
	using the CRC-32.

===============================================================================
*/

void CRC32_InitChecksum(unsigned int &crcvalue);
void CRC32_UpdateChecksum(unsigned int &crcvalue, const void *data, int length);
void CRC32_FinishChecksum(unsigned int &crcvalue);
unsigned int CRC32_BlockChecksum(const void *data, int length);
#ifdef _SPLASHDAMAGE
void CRC32_UpdateChecksum( unsigned /* 64long */int &crcvalue, const byte data );
#endif

#endif /* !__CRC32_H__ */
