// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __PAIR_H__
#define __PAIR_H__

template< class T, class U >
class sdPair {
public:
	sdPair() : first( T() ), second( U () ) { }

	template< class V, class W >
	sdPair( const sdPair< V , W >& rhs ) : first( rhs.first ), second( rhs.second ) { }

	template< class V, class W >
	sdPair( const V& f, const W& s ) : first( f ), second( s ) { }

	const bool operator==( const sdPair& rhs ) const {
		return ( rhs.first == first ) && ( rhs.second == second );
	}

	const bool operator!=( const sdPair& rhs ) const {
		return !(*this == rhs);
	}

	template< class V, class W >
	const bool operator==( const sdPair< V , W >& rhs ) const {
		return ( rhs.first == first ) && ( rhs.second == second );
	}

	T first;
	U second;
};

#endif /* ! __PAIR_H__ */
