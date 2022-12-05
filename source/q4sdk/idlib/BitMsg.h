
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

	void			Init( byte *data, int length );
	void			Init( const byte *data, int length );
	byte *			GetData( void );						// get data for writing
	const byte *	GetData( void ) const;					// get data for reading
	const byte *	GetReadData( void ) const;
	int				GetMaxSize( void ) const;				// get the maximum message size
	void			SetAllowOverflow( bool set );			// generate error if not set and message is overflowed
	bool			IsOverflowed( void ) const;				// returns true if the message was overflowed

	int				GetSize( void ) const;					// size of the message in bytes
	void			SetSize( int size );					// set the message size
	int				GetWriteBit( void ) const;				// get current write bit
	void			SetWriteBit( int bit );					// set current write bit
	int				GetNumBitsWritten( void ) const;		// returns number of bits written
	int				GetRemainingSpace( void ) const;		// space left in bytes for writing
	int				GetRemainingWriteBits( void ) const;	// space left in bits for writing
	void			SaveWriteState( int &s, int &b ) const;	// save the write state
	void			RestoreWriteState( int s, int b );		// restore the write state

	int				GetReadCount( void ) const;				// bytes read so far
	void			SetReadCount( int bytes );				// set the number of bytes and bits read
	int				GetReadBit( void ) const;				// get current read bit
	void			SetReadBit( int bit );					// set current read bit
	int				GetNumBitsRead( void ) const;			// returns number of bits read
	int				GetRemainingData( void ) const;			// number of bytes left to read
	int				GetRemainingReadBits( void ) const;		// number of bits left to read
	void			SaveReadState( int &c, int &b ) const;	// save the read state
	void			RestoreReadState( int c, int b ) const;		// restore the read state

	void			BeginWriting( void );					// begin writing
	void			WriteByteAlign( void );					// write up to the next byte boundary
	void			WriteBits( int value, int numBits );	// write the specified number of bits
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
	void			WriteString( const char *s, int maxLength = -1 );
	void			WriteData( const void *data, int length );
	void			WriteNetadr( const netadr_t adr );

	void			WriteDeltaChar( int oldValue, int newValue );
	void			WriteDeltaByte( int oldValue, int newValue );
	void			WriteDeltaShort( int oldValue, int newValue );
	void			WriteDeltaLong( int oldValue, int newValue );
	void			WriteDeltaFloat( float oldValue, float newValue );
	void			WriteDeltaFloat( float oldValue, float newValue, int exponentBits, int mantissaBits );
	void			WriteDeltaByteCounter( int oldValue, int newValue );
	void			WriteDeltaShortCounter( int oldValue, int newValue );
	void			WriteDeltaLongCounter( int oldValue, int newValue );
	bool			WriteDeltaDict( const idDict &dict, const idDict *base );

	void			BeginReading( void ) const;				// begin reading.
	void			ReadByteAlign( void ) const;			// read up to the next byte boundary
	int				ReadBits( int numBits ) const;			// read the specified number of bits
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
	int				ReadString( char *buffer, int bufferSize ) const;
	int				ReadData( void *data, int length ) const;
	void			ReadNetadr( netadr_t *adr ) const;

	int				ReadDeltaChar( int oldValue ) const;
	int				ReadDeltaByte( int oldValue ) const;
	int				ReadDeltaShort( int oldValue ) const;
	int				ReadDeltaLong( int oldValue ) const;
	float			ReadDeltaFloat( float oldValue ) const;
	float			ReadDeltaFloat( float oldValue, int exponentBits, int mantissaBits ) const;
	int				ReadDeltaByteCounter( int oldValue ) const;
	int				ReadDeltaShortCounter( int oldValue ) const;
	int				ReadDeltaLongCounter( int oldValue ) const;
	bool			ReadDeltaDict( idDict &dict, const idDict *base ) const;

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
	bool			CheckOverflow( int numBits );
	byte *			GetByteSpace( int length );
	void			WriteDelta( int oldValue, int newValue, int numBits );
	int				ReadDelta( int oldValue, int numBits ) const;
};


ID_INLINE void idBitMsg::Init( byte *data, int length ) {
	writeData = data;
	readData = data;
	maxSize = length;
}

ID_INLINE void idBitMsg::Init( const byte *data, int length ) {
	writeData = NULL;
	readData = data;
	maxSize = length;
}

ID_INLINE byte *idBitMsg::GetData( void ) {
	return writeData;
}

ID_INLINE const byte *idBitMsg::GetData( void ) const {
	return readData;
}

ID_INLINE const byte *idBitMsg::GetReadData( void ) const {
	return ( readData + readCount );
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
//	return ( ( curSize << 3 ) - ( writeBit ? 8 - writeBit : 0 ) );
	return ( ( curSize << 3 ) - ( ( 8 - writeBit ) & 7 ) );
}

ID_INLINE int idBitMsg::GetRemainingSpace( void ) const {
	return maxSize - curSize;
}

ID_INLINE int idBitMsg::GetRemainingWriteBits( void ) const {
	return ( maxSize << 3 ) - GetNumBitsWritten();
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

ID_INLINE void idBitMsg::SetReadCount( int bytes ) {
	readCount = bytes;
}

ID_INLINE int idBitMsg::GetReadBit( void ) const {
	return readBit;
}

ID_INLINE void idBitMsg::SetReadBit( int bit ) {
	readBit = bit & 7;
}

ID_INLINE int idBitMsg::GetNumBitsRead( void ) const {
//	return ( ( readCount << 3 ) - ( readBit ? 8 - readBit : 0 ) );
	return ( ( readCount << 3 ) - ( ( 8 - readBit ) & 7 ) );
}

ID_INLINE int idBitMsg::GetRemainingData( void ) const {
	return curSize - readCount;
}

ID_INLINE int idBitMsg::GetRemainingReadBits( void ) const {
	return ( curSize << 3 ) - GetNumBitsRead();
}

ID_INLINE void idBitMsg::SaveReadState( int &c, int &b ) const {
	c = readCount;
	b = readBit;
}

ID_INLINE void idBitMsg::RestoreReadState( int c, int b ) const {
	readCount = c;
	readBit = b & 7;
}

ID_INLINE void idBitMsg::BeginWriting( void ) {
	curSize = 0;
	overflowed = false;
	writeBit = 0;
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

ID_INLINE void idBitMsg::WriteDeltaLong( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, 32 );
}

ID_INLINE void idBitMsg::WriteDeltaFloat( float oldValue, float newValue ) {
	WriteDelta( *reinterpret_cast<int *>(&oldValue), *reinterpret_cast<int *>(&newValue), 32 );
}

ID_INLINE void idBitMsg::WriteDeltaFloat( float oldValue, float newValue, int exponentBits, int mantissaBits ) {
	int oldBits = idMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = idMath::FloatToBits( newValue, exponentBits, mantissaBits );
	WriteDelta( oldBits, newBits, 1 + exponentBits + mantissaBits );
}

ID_INLINE void idBitMsg::BeginReading( void ) const {
	readCount = 0;
	readBit = 0;
}

ID_INLINE void idBitMsg::ReadByteAlign( void ) const {
	readBit = 0;
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

ID_INLINE int idBitMsg::ReadDeltaLong( int oldValue ) const {
	return ReadDelta( oldValue, 32 );
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

	void			InitWriting( const idBitMsg *base, idBitMsg *newBase, idBitMsg *delta );
	void			InitReading( const idBitMsg *base, idBitMsg *newBase, const idBitMsg *delta );
	bool			HasChanged( void ) const;

	void			WriteBits( int value, int numBits );
	void			WriteChar( int c );
	void			WriteByte( int c );
	void			WriteShort( int c );
	void			WriteUShort( int c );
	void			WriteLong( int c );
	void			WriteFloat( float f );
	void			WriteFloat( float f, int exponentBits, int mantissaBits );
// RAVEN BEGIN
// abahr:
	void			WriteVec3( const idVec3& v );
	void			WriteDeltaVec3( const idVec3& oldValue, const idVec3& newValue );
	void			WriteVec4( const idVec4& v );
	void			WriteDeltaVec4( const idVec4& oldValue, const idVec4& newValue );
	void			WriteQuat( const idQuat& q );
	void			WriteDeltaQuat( const idQuat& oldValue, const idQuat& newValue );
    void			WriteMat3( const idMat3& m );
	void			WriteDeltaMat3( const idMat3& oldValue, const idMat3& newValue );
// RAVEN END
	void			WriteAngle8( float f );
	void			WriteAngle16( float f );
	void			WriteDir( const idVec3 &dir, int numBits );
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
	int				ReadChar( void ) const;
	int				ReadByte( void ) const;
	int				ReadShort( void ) const;
	int				ReadUShort( void ) const;
	int				ReadLong( void ) const;
	float			ReadFloat( void ) const;
	float			ReadFloat( int exponentBits, int mantissaBits ) const;
// RAVEN BEGIN
// abahr
	idVec3			ReadVec3( void ) const;
	idVec3			ReadDeltaVec3( const idVec3& oldValue ) const;
	idVec4			ReadVec4( void ) const;
	idVec4			ReadDeltaVec4( const idVec4& oldValue ) const;
	idQuat			ReadQuat( void ) const;
	idQuat			ReadDeltaQuat( const idQuat& oldValue ) const;
	idMat3			ReadMat3( void ) const;
	idMat3			ReadDeltaMat3( const idMat3& oldValue ) const;
// RAVEN END
	float			ReadAngle8( void ) const;
	float			ReadAngle16( void ) const;
	idVec3			ReadDir( int numBits ) const;
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

ID_INLINE void idBitMsgDelta::InitWriting( const idBitMsg *base, idBitMsg *newBase, idBitMsg *delta ) {
	this->base = base;
	this->newBase = newBase;
	this->writeDelta = delta;
	this->readDelta = delta;
	this->changed = false;
}

ID_INLINE void idBitMsgDelta::InitReading( const idBitMsg *base, idBitMsg *newBase, const idBitMsg *delta ) {
	this->base = base;
	this->newBase = newBase;
	this->writeDelta = NULL;
	this->readDelta = delta;
	this->changed = false;
}

ID_INLINE bool idBitMsgDelta::HasChanged( void ) const {
	return changed;
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

// RAVEN BEGIN
// abahr
ID_INLINE void idBitMsgDelta::WriteVec3( const idVec3& v ) {
	for( int ix = 0; ix < v.GetDimension(); ++ix ) {
		WriteFloat( v[ix] );
	}
}

ID_INLINE void idBitMsgDelta::WriteDeltaVec3( const idVec3& oldValue, const idVec3& newValue ) {
	for( int ix = 0; ix < oldValue.GetDimension(); ++ix ) {
		WriteDeltaFloat( oldValue[ix], newValue[ix] );
	}
}

ID_INLINE void idBitMsgDelta::WriteVec4( const idVec4& v ) {
	for( int ix = 0; ix < v.GetDimension(); ++ix ) {
		WriteFloat( v[ix] );
	}
}

ID_INLINE void idBitMsgDelta::WriteDeltaVec4( const idVec4& oldValue, const idVec4& newValue ) {
	for( int ix = 0; ix < oldValue.GetDimension(); ++ix ) {
		WriteDeltaFloat( oldValue[ix], newValue[ix] );
	}
}

ID_INLINE void idBitMsgDelta::WriteQuat( const idQuat& q ) {
	for( int ix = 0; ix < q.GetDimension(); ++ix ) {
		WriteFloat( q[ix] );
	}
}

ID_INLINE void idBitMsgDelta::WriteDeltaQuat( const idQuat& oldValue, const idQuat& newValue ) {
	for( int ix = 0; ix < oldValue.GetDimension(); ++ix ) {
		WriteDeltaFloat( oldValue[ix], newValue[ix] );
	}
}

ID_INLINE void idBitMsgDelta::WriteMat3( const idMat3& m ) {
	for( int ix = 0; ix < m.GetVec3Dimension(); ++ix ) {
		WriteVec3( m[ix] );
	}
}

ID_INLINE void idBitMsgDelta::WriteDeltaMat3( const idMat3& oldValue, const idMat3& newValue ) {
	for( int ix = 0; ix < oldValue.GetDimension(); ++ix ) {
		WriteDeltaVec3( oldValue[ix], newValue[ix] );
	}
}
// RAVEN END

ID_INLINE void idBitMsgDelta::WriteAngle8( float f ) {
	WriteBits( ANGLE2BYTE( f ), 8 );
}

ID_INLINE void idBitMsgDelta::WriteAngle16( float f ) {
	WriteBits( ANGLE2SHORT(f), 16 );
}

ID_INLINE void idBitMsgDelta::WriteDir( const idVec3 &dir, int numBits ) {
	WriteBits( idBitMsg::DirToBits( dir, numBits ), numBits );
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

// RAVEN BEGIN
// abahr
ID_INLINE idVec3 idBitMsgDelta::ReadVec3() const {
	idVec3 v;
	for( int ix = 0; ix < v.GetDimension(); ++ix ) {
		v[ix] = ReadFloat();
	}

	return v;
}

ID_INLINE idVec3 idBitMsgDelta::ReadDeltaVec3( const idVec3& oldValue ) const {
	idVec3 value;
	for( int ix = 0; ix < value.GetDimension(); ++ix ) {
		value[ix] = ReadDeltaFloat( oldValue[ix] );
	}
	return value;
}

ID_INLINE idVec4 idBitMsgDelta::ReadVec4( void ) const {
	idVec4 v;
	for( int ix = 0; ix < v.GetDimension(); ++ix ) {
		v[ix] = ReadFloat();
	}

	return v;
}

ID_INLINE idVec4 idBitMsgDelta::ReadDeltaVec4( const idVec4& oldValue ) const {
	idVec4 value;
	for( int ix = 0; ix < value.GetDimension(); ++ix ) {
		value[ix] = ReadDeltaFloat( oldValue[ix] );
	}
	return value;
}

ID_INLINE idQuat idBitMsgDelta::ReadQuat( void ) const {
	idQuat q;
	for( int ix = 0; ix < q.GetDimension(); ++ix ) {
		q[ix] = ReadFloat();
	}

	return q;
}

ID_INLINE idQuat idBitMsgDelta::ReadDeltaQuat( const idQuat& oldValue ) const {
	idQuat value;
	for( int ix = 0; ix < value.GetDimension(); ++ix ) {
		value[ix] = ReadDeltaFloat( oldValue[ix] );
	}
	return value;
}

ID_INLINE idMat3 idBitMsgDelta::ReadMat3() const {
	idMat3 m;
	
	for( int ix = 0; ix < m.GetVec3Dimension(); ++ix ) {
		m[ix] = ReadVec3();
	}

	return m;
}

ID_INLINE idMat3 idBitMsgDelta::ReadDeltaMat3( const idMat3& oldValue ) const {
	idMat3 value;

	for( int ix = 0; ix < value.GetVec3Dimension(); ++ix ) {
		value[ix] = ReadDeltaVec3( oldValue[ix] );
	}

	return value;
}
// RAVEN END

ID_INLINE float idBitMsgDelta::ReadAngle8( void ) const {
	return BYTE2ANGLE( ReadByte() );
}

ID_INLINE float idBitMsgDelta::ReadAngle16( void ) const {
	return SHORT2ANGLE( ReadShort() );
}

ID_INLINE idVec3 idBitMsgDelta::ReadDir( int numBits ) const {
	return idBitMsg::BitsToDir( ReadBits( numBits ), numBits );
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

#define MAX_MSG_QUEUE_SIZE				16384		// must be a power of 2

class idMsgQueue {
public:
					idMsgQueue();

	void			Init( int sequence );

	bool			Add( const byte *data, const int size, bool sequencing );
	// prepend to a message without the need to a memcopy and readjust
	bool			AddConcat( const byte *data1, const int size1, const byte *data2, const int size2, bool sequencing );
	bool			Get( byte *data, int dataSize, int &size, bool sequencing );
	int				GetTotalSize( void ) const;
	int				GetSpaceLeft( void ) const;
	int				GetFirst( void ) const { return first; }
	int				GetLast( void ) const { return last; }
	void			CopyToBuffer( byte *buf ) const;

	void			WriteTo( idBitMsg &msg );
	void			FlushTo( idBitMsg &msg );
	void			ReadFrom( const idBitMsg &msg );

	void			Save( idFile *file ) const;
	void			Restore( idFile *file );

private:
	byte			buffer[MAX_MSG_QUEUE_SIZE];
	int				first;			// sequence number of first message in queue
	int				last;			// sequence number of last message in queue
	int				startIndex;		// index pointing to the first byte of the first message
	int				endIndex;		// index pointing to the first byte after the last message

public:
	void			WriteByte( byte b );
	byte			ReadByte( void );
	void			WriteShort( int s );
	void			WriteUShort( int s );
	int				ReadShort( void );
	int				ReadUShort( void );
	void			WriteLong( int l );
	int				ReadLong( void );
	void			WriteData( const byte *data, const int size );
	void			ReadData( byte *data, const int size );
};

class idBitMsgQueue {
public:
					idBitMsgQueue();

	void			Init( void );

	void			Add( const idBitMsg &msg, const int timestamp );
	bool			Get( idBitMsg &msg, int &timestamp );
	bool			Get( idBitMsg &msg ) { int dummy; return Get( msg, dummy); }
	bool			GetTimestamp( int &timestamp );

private:
	int				nextTimestamp;
	bool			readTimestamp;
	idLinkList<idMsgQueue>	writeList, readList;
};

#endif /* !__BITMSG_H__ */
