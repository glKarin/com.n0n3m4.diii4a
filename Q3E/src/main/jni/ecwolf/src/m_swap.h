#ifndef __M_SWAP__
#define __M_SWAP__

#include "wl_def.h"

static inline WORD ReadLittleShort(const BYTE * const ptr)
{
	return WORD(BYTE(*ptr)) |
		(WORD(BYTE(*(ptr+1)))<<8);
}

static inline WORD ReadBigShort(const BYTE * const ptr)
{
	return WORD(BYTE(*(ptr+1))) |
		(WORD(BYTE(*ptr))<<8);
}

static inline DWORD ReadLittle24(const BYTE * const ptr)
{
	return DWORD(BYTE(*ptr)) |
		(DWORD(BYTE(*(ptr+1)))<<8) |
		(DWORD(BYTE(*(ptr+2)))<<16);
}

static inline DWORD ReadBig24(const BYTE * const ptr)
{
	return DWORD(BYTE(*(ptr+2))) |
		(DWORD(BYTE(*(ptr+1)))<<8) |
		(DWORD(BYTE(*ptr))<<16);
}

static inline DWORD ReadLittleLong(const BYTE * const ptr)
{
	return DWORD(BYTE(*ptr)) |
		(DWORD(BYTE(*(ptr+1)))<<8) |
		(DWORD(BYTE(*(ptr+2)))<<16) |
		(DWORD(BYTE(*(ptr+3)))<<24);
}

static inline DWORD ReadBigLong(const BYTE * const ptr)
{
	return (DWORD(BYTE(*ptr))<<24) |
		(DWORD(BYTE(*(ptr+1)))<<16) |
		(DWORD(BYTE(*(ptr+2)))<<8) |
		DWORD(BYTE(*(ptr+3)));
}

static inline void WriteLittleShort(BYTE * const ptr, WORD value)
{
	ptr[0] = value&0xFF;
	ptr[1] = (value>>8)&0xFF;
}

static inline void WriteLittleLong(BYTE * const ptr, DWORD value)
{
	ptr[0] = value&0xFF;
	ptr[1] = (value>>8)&0xFF;
	ptr[2] = (value>>16)&0xFF;
	ptr[3] = (value>>24)&0xFF;
}

static inline void WriteBigLong(BYTE * const ptr, DWORD value)
{
	ptr[0] = (value>>24)&0xFF;
	ptr[1] = (value>>16)&0xFF;
	ptr[2] = (value>>8)&0xFF;
	ptr[3] = value&0xFF;
}

// After the fact Byte Swapping ------------------------------------------------

static inline WORD SwapShort(WORD x)
{
	return ((x&0xFF)<<8) | ((x>>8)&0xFF);
}

static inline DWORD SwapLong(DWORD x)
{
	return ((x&0xFF)<<24) |
		(((x>>8)&0xFF)<<16) |
		(((x>>16)&0xFF)<<8) |
		((x>>24)&0xFF);
}

static inline QWORD SwapLongLong(QWORD x)
{
	return ((x&0xFF)<<56) |
		(((x>>8)&0xFF)<<48) |
		(((x>>16)&0xFF)<<40) |
		(((x>>24)&0xFF)<<32) |
		(((x>>32)&0xFF)<<24) |
		(((x>>40)&0xFF)<<16) |
		(((x>>48)&0xFF)<<8) |
		((x>>56)&0xFF);
}

#ifdef __BIG_ENDIAN__
#define BigShort(x) (x)
#define BigLong(x) (x)
#define BigLongLong(x) (x)
#define LittleShort SwapShort
#define LittleLong SwapLong
#define LittleLongLong SwapLongLong
#define ReadNativeShort ReadBigShort
#define ReadNativeLong ReadBigLong
#else
#define BigShort SwapShort
#define BigLong SwapLong
#define BigLongLong SwapLongLong
#define LittleShort(x) (x)
#define LittleLong(x) (x)
#define LittleLongLong(x) (x)
#define ReadNativeShort ReadLittleShort
#define ReadNativeLong ReadLittleLong
#endif

#endif /* __M_SWAP__ */
