// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __BITFIELD_H__
#define __BITFIELD_H__

template< int MAX_BITS >
class sdBitField {
private:
	static const int	INTBITS		= sizeof( int ) * 8;
	static const int	INTBITS_X	= INTBITS - 1;
	static const int	SIZE		= ( MAX_BITS / INTBITS ) + 1;

public:
						sdBitField( void ) { Clear(); }

	void				operator= ( const sdBitField& other ) { memcpy( bits, other.bits, sizeof( bits ) ); }
	int					operator[] ( int bit ) const { return Get( bit ); }
	bool				operator!= ( const sdBitField& other ) const { return memcmp( bits, other.bits, sizeof( bits ) ) != 0; }
	bool				operator== ( const sdBitField& other ) const { return !(operator!=(other)); }	
	int					Get( int bit ) const { return ( ( bits[ bit / INTBITS ] >> ( bit & INTBITS_X ) ) & 1 ); }
	void				Set( int bit ) { bits[ bit / INTBITS ] |= 1 << ( bit & INTBITS_X ); }
	void				Clear( int bit ) { bits[ bit / INTBITS ] &= ~( 1 << ( bit & INTBITS_X ) ); }

	void				Clear( void ) { memset( bits, 0, sizeof( bits ) ); }
	void				SetAll( void ) { memset( bits, 1, sizeof( bits ) ); }

	int*				GetDirect( void ) { return bits; }
	const int*			GetDirect( void ) const { return bits; }
	int					Size( void ) const { return SIZE; }

private:
	int					bits[ SIZE ];
};

class sdBitField_Dynamic {
private:
	static const int	INTBITS		= sizeof( int ) * 8;
	static const int	INTBITS_X	= INTBITS - 1;

public:
						sdBitField_Dynamic( void ) { bits = NULL; size = 0; }
						~sdBitField_Dynamic( void ) { Shutdown(); }

	static int			SizeForBits( int bits ) { return ( bits / INTBITS ) + 1; }

	void				Shutdown( void ) { delete[] bits; bits = NULL; size = 0; }
	void				Init( int MAX_BITS ) { SetSize( SizeForBits( MAX_BITS ) ); }

	int					operator[] ( int bit ) const { return Get( bit ); }
	int					Get( int bit ) const { return ( ( bits[ bit / INTBITS ] >> ( bit & INTBITS_X ) ) & 1 ); }
	void				Set( int bit ) { bits[ bit / INTBITS ] |= 1 << ( bit & INTBITS_X ); }
	void				Clear( int bit ) { bits[ bit / INTBITS ] &= ~( 1 << ( bit & INTBITS_X ) ); }

	void				Clear( void ) { if( bits ) { memset( bits, 0, size * sizeof( int ) ); } }
	void				SetAll( void ) { memset( bits, 1, size * sizeof( int ) ); }

	int					GetSize( void ) const { return size; }
	void				SetSize( int size ) { if ( this->size == size ) { return; } Shutdown(); this->size = size; if ( size > 0 ) { bits = new int[ size ]; } }
	int&				GetDirect( int index ) { return bits[ index ]; }
	const int&			GetDirect( int index ) const { return bits[ index ]; }

private:
	int*				bits;
	int					size;
};

class sdBitField_Stack {
private:
	static const int	INTBITS		= sizeof( int ) * 8;
	static const int	INTBITS_X	= INTBITS - 1;

public:
						sdBitField_Stack( void ) { bits = NULL; }
						~sdBitField_Stack( void ) { ; }

	void				Init( int* bits, int size ) { this->size = size; assert( size > 0 ); this->bits = bits; }
	static int			GetSizeForMaxBits( int MAX_BITS ) { return ( MAX_BITS / INTBITS ) + 1; }

	int					operator[] ( int bit ) const { return Get( bit ); }
	int					Get( int bit ) const { return ( ( bits[ bit / INTBITS ] >> ( bit & INTBITS_X ) ) & 1 ); }
	void				Set( int bit ) { bits[ bit / INTBITS ] |= 1 << ( bit & INTBITS_X ); }
	void				Set( int bit, int to ) { bits[ bit / INTBITS ] ^= ( Get( bit ) ^ to ) << ( bit & INTBITS_X ); }
	void				Clear( int bit ) { bits[ bit / INTBITS ] &= ~( 1 << ( bit & INTBITS_X ) ); }

	void				Clear( void ) { if( bits ) { memset( bits, 0, size * sizeof( int ) ); } }
	void				SetAll( void ) { memset( bits, 1, size * sizeof( int ) ); }

	int					GetSize( void ) const { return size; }
	int&				GetDirect( int index ) { return bits[ index ]; }
	const int&			GetDirect( int index ) const { return bits[ index ]; }

private:
	int*				bits;
	int					size;
};

#endif /* !__BITFIELD_H__ */
