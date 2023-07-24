// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __BITMSG_H__
#define __BITMSG_H__

/*
===============================================================================

  idBitMsg

  Handles byte ordering and avoids alignment errors.
  Allows concurrent writing and reading.
  The data set with Init is never freed.

===============================================================================
*/

class idBitMsg {
public:
					idBitMsg();
					~idBitMsg() {}

	void			InitWrite( byte *data, int length );		// both read & write
	void			InitRead( const byte *data, int length );	// read only

	byte *			GetData( void );						// get data for writing
	const byte *	GetData( void ) const;					// get data for reading
	int				GetMaxSize( void ) const;				// get the maximum message size
	void			SetAllowOverflow( bool set );			// generate error if not set and message is overflowed
	bool			IsOverflowed( void ) const;				// returns true if the message was overflowed

	int				GetSize( void ) const;					// size of the message in bytes
	void			SetSize( int size );					// set the message size
	int				GetWriteBit( void ) const;				// get current write bit
	void			SetWriteBit( int bit );					// set current write bit
	int				GetNumBitsWritten( void ) const;		// returns number of bits written
	void			SaveWriteState( int &s, int &b ) const;	// save the write state
	void			RestoreWriteState( int s, int b );		// restore the write state

	int				GetReadCount( void ) const;				// bytes read so far
	void			SetReadCount( int bytes ) const;		// set the number of bytes and bits read
	int				GetReadBit( void ) const;				// get current read bit
	void			SetReadBit( int bit );					// set current read bit
	int				GetNumBitsRead( void ) const;			// returns number of bits read
	void			SaveReadState( int &c, int &b ) const;	// save the read state
	void			RestoreReadState( int c, int b );		// restore the read state

	void			BeginWriting( void );					// begin writing
	int				GetRemainingSpace( void ) const;		// space left in bytes
	void			WriteByteAlign( void );					// write up to the next byte boundary
	void			WriteBits( int value, int numBits );	// write the specified number of bits
	void			WriteBool( bool c );
	void			WriteChar( int c );
	void			WriteByte( int c );
	void			WriteShort( int c );
	void			WriteUShort( int c );
	void			WriteLong( int c );
	void			WriteFloat( float f );
	void			WriteFloat( float f, int exponentBits, int mantissaBits );
	void			WriteCQuat( const idCQuat& quat );
	void			WriteVector( const idVec3& vec );
	void			WriteAngle8( float f );
	void			WriteAngle16( float f );
	void			WriteDir( const idVec3 &dir, int numBits );
	void			WriteString( const char *s, int maxLength = -1, bool make7Bit = true );
	void			WriteString( const wchar_t *s, int maxLength = -1 );
	void			WriteData( const void *data, int length );
	void			WriteNetadr( const netadr_t adr );

	void			WriteDeltaChar( int oldValue, int newValue );
	void			WriteDeltaByte( int oldValue, int newValue );
	void			WriteDeltaShort( int oldValue, int newValue );
	void			WriteDeltaUShort( int oldValue, int newValue );
	void			WriteDeltaLong( int oldValue, int newValue );
	void			WriteDeltaCQuat( const idCQuat& oldValue, const idCQuat& newValue );
	void			WriteDeltaVector( const idVec3& oldValue, const idVec3& newValue );
	void			WriteDeltaVector( const idVec3& oldValue, const idVec3& newValue, int exponentBits, int mantissaBits );
	void			WriteDeltaFloat( float oldValue, float newValue );
	void			WriteDeltaFloat( float oldValue, float newValue, int exponentBits, int mantissaBits );
	void			WriteDeltaByteCounter( int oldValue, int newValue );
	void			WriteDeltaShortCounter( int oldValue, int newValue );
	void			WriteDeltaLongCounter( int oldValue, int newValue );
	bool			WriteDeltaDict( const idDict &dict, const idDict *base );
	void			WriteDeltaData( const void* oldValue, const void* newValue, int size );
	void			WriteDelta( int oldValue, int newValue, int numBits );

	void			EnsureByteBoundary( void );

	void			BeginReading( void ) const;				// begin reading.
	int				GetRemainingData( void ) const;			// number of bytes left to read
	void			ReadByteAlign( void ) const;			// read up to the next byte boundary
	int				ReadBits( int numBits ) const;			// read the specified number of bits
	bool			ReadBool( void ) const;
	int				ReadChar( void ) const;
	int				ReadByte( void ) const;
	int				ReadShort( void ) const;
	int				ReadUShort( void ) const;
	int				ReadLong( void ) const;
	float			ReadFloat( void ) const;
	float			ReadFloat( int exponentBits, int mantissaBits ) const;
	idCQuat			ReadCQuat( void ) const;
	idVec3			ReadVector( void ) const;
	float			ReadAngle8( void ) const;
	float			ReadAngle16( void ) const;
	idVec3			ReadDir( int numBits ) const;
	int				ReadString( char *buffer, int bufferSize ) const;
	int				ReadString( wchar_t *buffer, int bufferSize ) const;
	int				ReadData( void *data, int length ) const;
	void			ReadNetadr( netadr_t *adr ) const;

	int				ReadDeltaChar( int oldValue ) const;
	int				ReadDeltaByte( int oldValue ) const;
	int				ReadDeltaShort( int oldValue ) const;
	int				ReadDeltaUShort( int oldValue ) const;
	int				ReadDeltaLong( int oldValue ) const;

	idCQuat			ReadDeltaCQuat( const idCQuat& oldValue ) const;

	idVec3			ReadDeltaVector( const idVec3& oldValue ) const;
	idVec3			ReadDeltaVector( const idVec3& oldValue, int exponentBits, int mantissaBits ) const;

	float			ReadDeltaFloat( float oldValue ) const;
	float			ReadDeltaFloat( float oldValue, int exponentBits, int mantissaBits ) const;
	int				ReadDeltaByteCounter( int oldValue ) const;
	int				ReadDeltaShortCounter( int oldValue ) const;
	int				ReadDeltaLongCounter( int oldValue ) const;
	bool			ReadDeltaDict( idDict &dict, const idDict *base ) const;
	void			ReadDeltaData( const void* oldValue, void* newValue, int size ) const;
	int				ReadDelta( int oldValue, int numBits ) const;

	static int		DirToBits( const idVec3 &dir, int numBits );
	static idVec3	BitsToDir( int bits, int numBits );

private:
	byte *			writeData;			// pointer to data for writing
	const byte *	readData;			// pointer to data for reading
	int				maxSize;			// maximum size of message in bytes
	int				curSize;			// current size of message in bytes
	int				writeBit;			// number of bits written to the last written byte
	mutable int		readCount;			// number of bytes read so far
	mutable int		readBit;			// number of bits read from the last read byte
	bool			allowOverflow;		// if false, generate an error when the message is overflowed
	bool			overflowed;			// set to true if the buffer size failed (with allowOverflow set)

private:
	bool			CheckOverflow( int length );
	byte *			GetByteSpace( int length );
};


ID_INLINE void idBitMsg::InitWrite( byte *data, int length ) {
	writeData = data;
	readData = data;
	maxSize = length;
}

ID_INLINE void idBitMsg::InitRead( const byte *data, int length ) {
	writeData = NULL;
	readData = data;
	maxSize = length;
}

ID_INLINE byte *idBitMsg::GetData( void ) {
	assert( writeData != NULL );
	return writeData;
}

ID_INLINE const byte *idBitMsg::GetData( void ) const {
	assert( readData != NULL );
	return readData;
}

ID_INLINE int idBitMsg::GetMaxSize( void ) const {
	return maxSize;
}

ID_INLINE void idBitMsg::SetAllowOverflow( bool set ) {
	allowOverflow = set;
}

ID_INLINE bool idBitMsg::IsOverflowed( void ) const {
	return overflowed;
}

ID_INLINE int idBitMsg::GetSize( void ) const {
	return curSize;
}

ID_INLINE void idBitMsg::SetSize( int size ) {
	if ( size > maxSize ) {
		curSize = maxSize;
	} else {
		curSize = size;
	}
}

ID_INLINE int idBitMsg::GetWriteBit( void ) const {
	return writeBit;
}

ID_INLINE void idBitMsg::SetWriteBit( int bit ) {
	writeBit = bit & 7;
	if ( writeBit ) {
		writeData[curSize - 1] &= ( 1 << writeBit ) - 1;
	}
}

ID_INLINE int idBitMsg::GetNumBitsWritten( void ) const {
	return ( ( curSize << 3 ) - ( writeBit ? 8 - writeBit : 0 ) );
}

ID_INLINE void idBitMsg::SaveWriteState( int &s, int &b ) const {
	s = curSize;
	b = writeBit;
}

ID_INLINE void idBitMsg::RestoreWriteState( int s, int b ) {
	curSize = s;
	writeBit = b & 7;
	if ( writeBit ) {
		writeData[curSize - 1] &= ( 1 << writeBit ) - 1;
	}
}

ID_INLINE int idBitMsg::GetReadCount( void ) const {
	return readCount;
}

ID_INLINE void idBitMsg::SetReadCount( int bytes ) const {
	readCount = bytes;
}

ID_INLINE int idBitMsg::GetReadBit( void ) const {
	return readBit;
}

ID_INLINE void idBitMsg::SetReadBit( int bit ) {
	readBit = bit & 7;
}

ID_INLINE int idBitMsg::GetNumBitsRead( void ) const {
	return ( ( readCount << 3 ) - ( readBit ? 8 - readBit : 0 ) );
}

ID_INLINE void idBitMsg::SaveReadState( int &c, int &b ) const {
	c = readCount;
	b = readBit;
}

ID_INLINE void idBitMsg::RestoreReadState( int c, int b ) {
	readCount = c;
	readBit = b & 7;
}

ID_INLINE void idBitMsg::BeginWriting( void ) {
	curSize = 0;
	overflowed = false;
	writeBit = 0;
}

ID_INLINE int idBitMsg::GetRemainingSpace( void ) const {
	return maxSize - curSize;
}

ID_INLINE void idBitMsg::WriteByteAlign( void ) {
	writeBit = 0;
}

ID_INLINE void idBitMsg::WriteChar( int c ) {
	WriteBits( c, -8 );
}

ID_INLINE void idBitMsg::WriteByte( int c ) {
	WriteBits( c, 8 );
}

ID_INLINE void idBitMsg::WriteShort( int c ) {
	WriteBits( c, -16 );
}

ID_INLINE void idBitMsg::WriteUShort( int c ) {
	WriteBits( c, 16 );
}

ID_INLINE void idBitMsg::WriteLong( int c ) {
	WriteBits( c, 32 );
}

ID_INLINE void idBitMsg::WriteBool( bool c ) {
	WriteBits( c ? 1 : 0, 1 );
}

ID_INLINE void idBitMsg::WriteFloat( float f ) {
	WriteBits( *reinterpret_cast<int *>(&f), 32 );
}

ID_INLINE void idBitMsg::WriteFloat( float f, int exponentBits, int mantissaBits ) {
	int bits = idMath::FloatToBits( f, exponentBits, mantissaBits );
	WriteBits( bits, 1 + exponentBits + mantissaBits );
}

ID_INLINE void idBitMsg::WriteAngle8( float f ) {
	WriteByte( ANGLE2BYTE( f ) );
}

ID_INLINE void idBitMsg::WriteAngle16( float f ) {
	WriteShort( ANGLE2SHORT(f) );
}

ID_INLINE void idBitMsg::WriteDir( const idVec3 &dir, int numBits ) {
	WriteBits( DirToBits( dir, numBits ), numBits );
}

ID_INLINE void idBitMsg::WriteDeltaChar( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, -8 );
}

ID_INLINE void idBitMsg::WriteDeltaByte( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, 8 );
}

ID_INLINE void idBitMsg::WriteDeltaShort( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, -16 );
}

ID_INLINE void idBitMsg::WriteDeltaUShort( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, 16 );
}

ID_INLINE void idBitMsg::WriteDeltaLong( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, 32 );
}

ID_INLINE void idBitMsg::WriteDeltaFloat( float oldValue, float newValue ) {
	WriteDelta( *reinterpret_cast<int *>(&oldValue), *reinterpret_cast<int *>(&newValue), 32 );
}

ID_INLINE void idBitMsg::WriteDeltaCQuat( const idCQuat& oldValue, const idCQuat& newValue ) {
	WriteDeltaFloat( oldValue.x, newValue.x );
	WriteDeltaFloat( oldValue.y, newValue.y );
	WriteDeltaFloat( oldValue.z, newValue.z );
}

ID_INLINE void idBitMsg::WriteDeltaVector( const idVec3& oldValue, const idVec3& newValue ) {
	WriteDeltaFloat( oldValue[ 0 ], newValue[ 0 ] );
	WriteDeltaFloat( oldValue[ 1 ], newValue[ 1 ] );
	WriteDeltaFloat( oldValue[ 2 ], newValue[ 2 ] );
}

ID_INLINE void idBitMsg::WriteDeltaVector( const idVec3& oldValue, const idVec3& newValue, int exponentBits, int mantissaBits ) {
	WriteDeltaFloat( oldValue[ 0 ], newValue[ 0 ], exponentBits, mantissaBits );
	WriteDeltaFloat( oldValue[ 1 ], newValue[ 1 ], exponentBits, mantissaBits );
	WriteDeltaFloat( oldValue[ 2 ], newValue[ 2 ], exponentBits, mantissaBits );
}

ID_INLINE void idBitMsg::WriteDeltaFloat( float oldValue, float newValue, int exponentBits, int mantissaBits ) {
	int oldBits = idMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = idMath::FloatToBits( newValue, exponentBits, mantissaBits );
	WriteDelta( oldBits, newBits, 1 + exponentBits + mantissaBits );
}

ID_INLINE void idBitMsg::WriteCQuat( const idCQuat& quat ) {
	WriteFloat( quat.x );
	WriteFloat( quat.y );
	WriteFloat( quat.z );
}

ID_INLINE void idBitMsg::WriteVector( const idVec3& vec ) {
	WriteFloat( vec[ 0 ] );
	WriteFloat( vec[ 1 ] );
	WriteFloat( vec[ 2 ] );
}

ID_INLINE void idBitMsg::BeginReading( void ) const {
	readCount = 0;
	readBit = 0;
}

ID_INLINE int idBitMsg::GetRemainingData( void ) const {
	return curSize - readCount;
}

ID_INLINE void idBitMsg::ReadByteAlign( void ) const {
	readBit = 0;
}

ID_INLINE bool idBitMsg::ReadBool( void ) const {
	return ( ReadBits( 1 ) == 1 );
}

ID_INLINE int idBitMsg::ReadChar( void ) const {
	return (signed char)ReadBits( -8 );
}

ID_INLINE int idBitMsg::ReadByte( void ) const {
	return (unsigned char)ReadBits( 8 );
}

ID_INLINE int idBitMsg::ReadShort( void ) const {
	return (short)ReadBits( -16 );
}

ID_INLINE int idBitMsg::ReadUShort( void ) const {
	return (unsigned short)ReadBits( 16 );
}

ID_INLINE int idBitMsg::ReadLong( void ) const {
	return ReadBits( 32 );
}

ID_INLINE float idBitMsg::ReadFloat( void ) const {
	float value;
	*reinterpret_cast<int *>(&value) = ReadBits( 32 );
	return value;
}

ID_INLINE float idBitMsg::ReadFloat( int exponentBits, int mantissaBits ) const {
	int bits = ReadBits( 1 + exponentBits + mantissaBits );
	return idMath::BitsToFloat( bits, exponentBits, mantissaBits );
}

ID_INLINE idCQuat idBitMsg::ReadCQuat( void ) const {
	idCQuat temp;
	temp.x = ReadFloat();
	temp.y = ReadFloat();
	temp.z = ReadFloat();
	return temp;
}

ID_INLINE idVec3 idBitMsg::ReadVector( void ) const {
	idVec3 temp;
	temp[ 0 ] = ReadFloat();
	temp[ 1 ] = ReadFloat();
	temp[ 2 ] = ReadFloat();
	return temp;
}

ID_INLINE float idBitMsg::ReadAngle8( void ) const {
	return BYTE2ANGLE( ReadByte() );
}

ID_INLINE float idBitMsg::ReadAngle16( void ) const {
	return SHORT2ANGLE( ReadShort() );
}

ID_INLINE idVec3 idBitMsg::ReadDir( int numBits ) const {
	return BitsToDir( ReadBits( numBits ), numBits );
}

ID_INLINE int idBitMsg::ReadDeltaChar( int oldValue ) const {
	return (signed char)ReadDelta( oldValue, -8 );
}

ID_INLINE int idBitMsg::ReadDeltaByte( int oldValue ) const {
	return (unsigned char)ReadDelta( oldValue, 8 );
}

ID_INLINE int idBitMsg::ReadDeltaShort( int oldValue ) const {
	return (short)ReadDelta( oldValue, -16 );
}

ID_INLINE int idBitMsg::ReadDeltaUShort( int oldValue ) const {
	return (short)ReadDelta( oldValue, 16 );
}

ID_INLINE int idBitMsg::ReadDeltaLong( int oldValue ) const {
	return ReadDelta( oldValue, 32 );
}

ID_INLINE idCQuat idBitMsg::ReadDeltaCQuat( const idCQuat& oldValue ) const {
	idCQuat value;
	value.x = ReadDeltaFloat( oldValue.x );
	value.y = ReadDeltaFloat( oldValue.y );
	value.z = ReadDeltaFloat( oldValue.z );
	return value;
}

ID_INLINE idVec3 idBitMsg::ReadDeltaVector( const idVec3& oldValue ) const {
	idVec3 value;
	value[ 0 ] = ReadDeltaFloat( oldValue[ 0 ] );
	value[ 1 ] = ReadDeltaFloat( oldValue[ 1 ] );
	value[ 2 ] = ReadDeltaFloat( oldValue[ 2 ] );
	return value;
}

ID_INLINE idVec3 idBitMsg::ReadDeltaVector( const idVec3& oldValue, int exponentBits, int mantissaBits ) const {
	idVec3 value;
	value[ 0 ] = ReadDeltaFloat( oldValue[ 0 ], exponentBits, mantissaBits );
	value[ 1 ] = ReadDeltaFloat( oldValue[ 1 ], exponentBits, mantissaBits );
	value[ 2 ] = ReadDeltaFloat( oldValue[ 2 ], exponentBits, mantissaBits );
	return value;
}

ID_INLINE float idBitMsg::ReadDeltaFloat( float oldValue ) const {
	float value;
	*reinterpret_cast<int *>(&value) = ReadDelta( *reinterpret_cast<int *>(&oldValue), 32 );
	return value;
}

ID_INLINE float idBitMsg::ReadDeltaFloat( float oldValue, int exponentBits, int mantissaBits ) const {
	int oldBits = idMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = ReadDelta( oldBits, 1 + exponentBits + mantissaBits );
	return idMath::BitsToFloat( newBits, exponentBits, mantissaBits );
}


/*
===============================================================================

  idBitMsgDelta

===============================================================================
*/

class idBitMsgDelta {
public:
					idBitMsgDelta();
					~idBitMsgDelta() {}

	void			InitWrite( const idBitMsg *base, idBitMsg *newBase, idBitMsg *delta );			// both read & write
	void			InitRead( const idBitMsg *base, idBitMsg *newBase, const idBitMsg *delta );		// read only

	bool			HasChanged( void ) const;

	void			WriteBits( int value, int numBits );
	void			WriteBool( bool c );
	void			WriteChar( int c );
	void			WriteByte( int c );
	void			WriteShort( int c );
	void			WriteUShort( int c );
	void			WriteLong( int c );
	void			WriteFloat( float f );
	void			WriteFloat( float f, int exponentBits, int mantissaBits );
	void			WriteAngle8( float f );
	void			WriteAngle16( float f );
	void			WriteDir( const idVec3 &dir, int numBits );
	void			WriteVector( const idVec3& vec );
	void			WriteString( const char *s, int maxLength = -1 );
	void			WriteData( const void *data, int length );
	void			WriteDict( const idDict &dict );

	void			WriteDeltaChar( int oldValue, int newValue );
	void			WriteDeltaByte( int oldValue, int newValue );
	void			WriteDeltaShort( int oldValue, int newValue );
	void			WriteDeltaLong( int oldValue, int newValue );
	void			WriteDeltaFloat( float oldValue, float newValue );
	void			WriteDeltaFloat( float oldValue, float newValue, int exponentBits, int mantissaBits );
	void			WriteDeltaByteCounter( int oldValue, int newValue );
	void			WriteDeltaShortCounter( int oldValue, int newValue );
	void			WriteDeltaLongCounter( int oldValue, int newValue );

	int				ReadBits( int numBits ) const;
	bool			ReadBool( void ) const;
	int				ReadChar( void ) const;
	int				ReadByte( void ) const;
	int				ReadShort( void ) const;
	int				ReadUShort( void ) const;
	int				ReadLong( void ) const;
	float			ReadFloat( void ) const;
	float			ReadFloat( int exponentBits, int mantissaBits ) const;
	float			ReadAngle8( void ) const;
	float			ReadAngle16( void ) const;
	idVec3			ReadDir( int numBits ) const;
	idVec3			ReadVector( void ) const;
	void			ReadString( char *buffer, int bufferSize ) const;
	void			ReadData( void *data, int length ) const;
	void			ReadDict( idDict &dict );

	int				ReadDeltaChar( int oldValue ) const;
	int				ReadDeltaByte( int oldValue ) const;
	int				ReadDeltaShort( int oldValue ) const;
	int				ReadDeltaLong( int oldValue ) const;
	float			ReadDeltaFloat( float oldValue ) const;
	float			ReadDeltaFloat( float oldValue, int exponentBits, int mantissaBits ) const;
	int				ReadDeltaByteCounter( int oldValue ) const;
	int				ReadDeltaShortCounter( int oldValue ) const;
	int				ReadDeltaLongCounter( int oldValue ) const;

private:
	const idBitMsg *base;			// base
	idBitMsg *		newBase;		// new base
	idBitMsg *		writeDelta;		// delta from base to new base for writing
	const idBitMsg *readDelta;		// delta from base to new base for reading
	mutable bool	changed;		// true if the new base is different from the base

private:
	void			WriteDelta( int oldValue, int newValue, int numBits );
	int				ReadDelta( int oldValue, int numBits ) const;
};

ID_INLINE idBitMsgDelta::idBitMsgDelta() {
	base = NULL;
	newBase = NULL;
	writeDelta = NULL;
	readDelta = NULL;
	changed = false;
}

ID_INLINE void idBitMsgDelta::InitWrite( const idBitMsg *base, idBitMsg *newBase, idBitMsg *delta ) {
	this->base = base;
	this->newBase = newBase;
	this->writeDelta = delta;
	this->readDelta = delta;
	this->changed = false;
}

ID_INLINE void idBitMsgDelta::InitRead( const idBitMsg *base, idBitMsg *newBase, const idBitMsg *delta ) {
	this->base = base;
	this->newBase = newBase;
	this->writeDelta = NULL;
	this->readDelta = delta;
	this->changed = false;
}

ID_INLINE bool idBitMsgDelta::HasChanged( void ) const {
	return changed;
}

ID_INLINE void idBitMsgDelta::WriteBool( bool c ) {
	WriteBits( c ? 1 : 0, 1 );
}

ID_INLINE void idBitMsgDelta::WriteChar( int c ) {
	WriteBits( c, -8 );
}

ID_INLINE void idBitMsgDelta::WriteByte( int c ) {
	WriteBits( c, 8 );
}

ID_INLINE void idBitMsgDelta::WriteShort( int c ) {
	WriteBits( c, -16 );
}

ID_INLINE void idBitMsgDelta::WriteUShort( int c ) {
	WriteBits( c, 16 );
}

ID_INLINE void idBitMsgDelta::WriteLong( int c ) {
	WriteBits( c, 32 );
}

ID_INLINE void idBitMsgDelta::WriteFloat( float f ) {
	WriteBits( *reinterpret_cast<int *>(&f), 32 );
}

ID_INLINE void idBitMsgDelta::WriteFloat( float f, int exponentBits, int mantissaBits ) {
	int bits = idMath::FloatToBits( f, exponentBits, mantissaBits );
	WriteBits( bits, 1 + exponentBits + mantissaBits );
}

ID_INLINE void idBitMsgDelta::WriteAngle8( float f ) {
	WriteBits( ANGLE2BYTE( f ), 8 );
}

ID_INLINE void idBitMsgDelta::WriteAngle16( float f ) {
	WriteBits( ANGLE2SHORT(f), 16 );
}

ID_INLINE void idBitMsgDelta::WriteDir( const idVec3 &dir, int numBits ) {
	WriteBits( idBitMsg::DirToBits( dir, numBits ), numBits );
}

ID_INLINE void idBitMsgDelta::WriteVector( const idVec3& vec ) {
	WriteFloat( vec[ 0 ] );
	WriteFloat( vec[ 1 ] );
	WriteFloat( vec[ 2 ] );
}

ID_INLINE void idBitMsgDelta::WriteDeltaChar( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, -8 );
}

ID_INLINE void idBitMsgDelta::WriteDeltaByte( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, 8 );
}

ID_INLINE void idBitMsgDelta::WriteDeltaShort( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, -16 );
}

ID_INLINE void idBitMsgDelta::WriteDeltaLong( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, 32 );
}

ID_INLINE void idBitMsgDelta::WriteDeltaFloat( float oldValue, float newValue ) {
	WriteDelta( *reinterpret_cast<int *>(&oldValue), *reinterpret_cast<int *>(&newValue), 32 );
}

ID_INLINE void idBitMsgDelta::WriteDeltaFloat( float oldValue, float newValue, int exponentBits, int mantissaBits ) {
	int oldBits = idMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = idMath::FloatToBits( newValue, exponentBits, mantissaBits );
	WriteDelta( oldBits, newBits, 1 + exponentBits + mantissaBits );
}

ID_INLINE bool idBitMsgDelta::ReadBool( void ) const {
	return ( ReadBits( 1 ) == 1 ) ? true : false;
}

ID_INLINE int idBitMsgDelta::ReadChar( void ) const {
	return (signed char)ReadBits( -8 );
}

ID_INLINE int idBitMsgDelta::ReadByte( void ) const {
	return (unsigned char)ReadBits( 8 );
}

ID_INLINE int idBitMsgDelta::ReadShort( void ) const {
	return (short)ReadBits( -16 );
}

ID_INLINE int idBitMsgDelta::ReadUShort( void ) const {
	return (unsigned short)ReadBits( 16 );
}

ID_INLINE int idBitMsgDelta::ReadLong( void ) const {
	return ReadBits( 32 );
}

ID_INLINE float idBitMsgDelta::ReadFloat( void ) const {
	float value;
	*reinterpret_cast<int *>(&value) = ReadBits( 32 );
	return value;
}

ID_INLINE float idBitMsgDelta::ReadFloat( int exponentBits, int mantissaBits ) const {
	int bits = ReadBits( 1 + exponentBits + mantissaBits );
	return idMath::BitsToFloat( bits, exponentBits, mantissaBits );
}

ID_INLINE float idBitMsgDelta::ReadAngle8( void ) const {
	return BYTE2ANGLE( ReadByte() );
}

ID_INLINE float idBitMsgDelta::ReadAngle16( void ) const {
	return SHORT2ANGLE( ReadShort() );
}

ID_INLINE idVec3 idBitMsgDelta::ReadDir( int numBits ) const {
	return idBitMsg::BitsToDir( ReadBits( numBits ), numBits );
}

ID_INLINE idVec3 idBitMsgDelta::ReadVector( void ) const {
	idVec3 vec;
	vec[ 0 ] = ReadFloat();
	vec[ 1 ] = ReadFloat();
	vec[ 2 ] = ReadFloat();
	return vec;
}

ID_INLINE int idBitMsgDelta::ReadDeltaChar( int oldValue ) const {
	return (signed char)ReadDelta( oldValue, -8 );
}

ID_INLINE int idBitMsgDelta::ReadDeltaByte( int oldValue ) const {
	return (unsigned char)ReadDelta( oldValue, 8 );
}

ID_INLINE int idBitMsgDelta::ReadDeltaShort( int oldValue ) const {
	return (short)ReadDelta( oldValue, -16 );
}

ID_INLINE int idBitMsgDelta::ReadDeltaLong( int oldValue ) const {
	return ReadDelta( oldValue, 32 );
}

ID_INLINE float idBitMsgDelta::ReadDeltaFloat( float oldValue ) const {
	float value;
	*reinterpret_cast<int *>(&value) = ReadDelta( *reinterpret_cast<int *>(&oldValue), 32 );
	return value;
}

ID_INLINE float idBitMsgDelta::ReadDeltaFloat( float oldValue, int exponentBits, int mantissaBits ) const {
	int oldBits = idMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = ReadDelta( oldBits, 1 + exponentBits + mantissaBits );
	return idMath::BitsToFloat( newBits, exponentBits, mantissaBits );
}

#endif /* !__BITMSG_H__ */
