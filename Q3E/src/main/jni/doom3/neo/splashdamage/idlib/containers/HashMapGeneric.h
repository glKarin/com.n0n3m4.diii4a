// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __HASHMAP_GENERIC_H__
#define __HASHMAP_GENERIC_H__

/*
============
Hash policies
============
*/

/*
============
sdHashGeneratorDefault
============
*/
struct sdHashGeneratorDefault {
	template< class type >
	static int Hash( const idHashIndex& hasher, const type& value ) {
		return hasher.GenerateKey( value );
	}
};

/*
============
sdHashGeneratorNumeric
============
*/
struct sdHashGeneratorNumeric {
	template< class type >
	static int Hash( const idHashIndex& hasher, const type& value ) {
		return hasher.GenerateKey( value, 0 );
	}
};

/*
============
sdHashGeneratorIHash
case-insensitive string hash
============
*/
struct sdHashGeneratorIHash {
	template< class type >
	static int Hash( const idHashIndex& hasher, const type& value ) {
		return hasher.GenerateKey( value, false );
	}
};


/*
============
Key comparison policies
============
*/

/*
============
sdHashCompareDefault
============
*/
struct sdHashCompareDefault {
	template< class type1, class type2 >
	static bool Compare( const type1& lhs, const type2& rhs ) {
		return lhs == rhs;
	}
};


/*
============
sdHashCompareStrCmp
============
*/
struct sdHashCompareStrCmp {
	template< class type1, class type2 >
	static bool Compare( const type1& lhs, const type2& rhs ) {
		return idStr::Cmp( lhs, rhs ) == 0;
	}
};


/*
============
sdHashCompareStrIcmp
============
*/
struct sdHashCompareStrIcmp {
	template< class type1, class type2 >
	static bool Compare( const type1& lhs, const type2& rhs ) {
		return idStr::Icmp( lhs, rhs ) == 0;
	}
};

/*
============
sdHashMapGeneric
allow for generic mapping between arbitrary data types
============
*/
template<	class key,
			class type, 
			class hashCompare = sdHashCompareDefault,
			class hashGenerator = sdHashGeneratorDefault,
			class hashIndexType = idHashIndex
		>
class sdHashMapGeneric {
public:
	typedef sdPair< key, type >			Pair;
	typedef const Pair					ConstPair;

	typedef Pair*						Iterator;
	typedef const Pair* 				ConstIterator;

	typedef type						Type;
	typedef const type					ConstType;
	typedef const key					ConstKey;

	// < location of insertion, true if the item is new, false if the item already existed >
	typedef sdPair< Iterator, bool >	InsertResult;
	
	typedef	hashGenerator				HashGenerator;
	typedef hashCompare					HashCompare;

										sdHashMapGeneric( const sdHashMapGeneric& rhs );
	explicit							sdHashMapGeneric( int granularity = 16 );
	sdHashMapGeneric&					operator=( const sdHashMapGeneric& rhs );
	
	void								SetGranularity( int newGranularity );
	void								InitHash( const int newHashSize, const int newIndexSize );

	size_t								Size( void ) const;
			
										// if the key-value mapping already exists, the value is merely reassigned
										// the bool parameter of InsertResult reports the results: it is true if
										// the item needed to be added, false if it already existed
	InsertResult						Set( const key& key_, const type& value );

										// this is a convenience accessor equivalent to Set
										// this is slightly more expensive than using Set directly in the case of new item insertions
	type&								operator[]( const key& key_ );

	Iterator							FindIndex( int index );
	ConstIterator						FindIndex( int index ) const;
	
	void								Clear();

	bool								Remove( const key& key_ );
		            					
										// Information
	int									Num() const;
	bool								Empty() const;
		            					
										// Iteration
	Iterator							Begin();
	Iterator							End();
		            					
	ConstIterator						Begin() const;
	ConstIterator						End() const;

	void								Remove( Iterator iter );

	int									Count( const key& key_ ) const;

	void								DeleteKeys();
	void								DeleteValues();

	void								Swap( sdHashMapGeneric& rhs );

	// These various Find version are templates so that in certain cases (mainly string mappings)
	// we can avoid constructing a temporary of type key
	template< class OtherKey >
	Iterator							Find( const OtherKey& key_ ) {
		int hashKey = HashGenerator::Hash( hash, key_ );
		for( int i = hash.GetFirst( hashKey ); i != idHashIndex::NULL_INDEX; i = hash.GetNext( i ) ) {
			if( HashCompare::Compare( pairs[ i ].first, key_ )) {
				return &pairs[ i ];
			}
		}

		return End();
	}
	template< class OtherKey >
	ConstIterator							Find( const OtherKey& key_ ) const {
		int hashKey = HashGenerator::Hash( hash, key_ );
		for( int i = hash.GetFirst( hashKey ); i != idHashIndex::NULL_INDEX; i = hash.GetNext( i ) ) {
			if( HashCompare::Compare( pairs[ i ].first, key_ )) {
				return &pairs[ i ];
			}
		}

		return End();
	}

private:
	InsertResult						SetNewItem( const key& key_, const type& value );

private:
	typedef idList< Pair > PairList;
	
	hashIndexType 	hash;
	PairList		pairs;
};

#define HASHMAP_TEMPLATE_HEADER template< class key, class type, class hashCompare, class hashGenerator, class hashIndexType >
#define HASHMAP_TEMPLATE_TAG sdHashMapGeneric< key, type, hashCompare, hashGenerator, hashIndexType >

/*
============
HASHMAP_TEMPLATE_TAG::sdHashMapGeneric
============
*/

HASHMAP_TEMPLATE_HEADER 
ID_INLINE 
HASHMAP_TEMPLATE_TAG::sdHashMapGeneric( int granularity ) {
	SetGranularity( granularity );
}

/*
============
HASHMAP_TEMPLATE_TAG::sdHashMapGeneric
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
HASHMAP_TEMPLATE_TAG::sdHashMapGeneric( const sdHashMapGeneric& rhs ) {
	*this = rhs;
}

/*
============
HASHMAP_TEMPLATE_TAG::sdHashMapGeneric
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
size_t HASHMAP_TEMPLATE_TAG::Size( void ) const {
	return sizeof( *this ) + hash.Size() + pairs.Size();
}

/*
============
HASHMAP_TEMPLATE_TAG::operator
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE
HASHMAP_TEMPLATE_TAG& HASHMAP_TEMPLATE_TAG::operator=( const sdHashMapGeneric& rhs ) {
	if( this != &rhs ) {
		pairs = rhs.pairs;
		hash = rhs.hash;
	}
	return *this;
}

/*
============
HASHMAP_TEMPLATE_TAG::Num
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
int HASHMAP_TEMPLATE_TAG::Num() const {
	return pairs.Num();
}

/*
============
HASHMAP_TEMPLATE_TAG::Empty
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
bool HASHMAP_TEMPLATE_TAG::Empty() const {
	return Num() == 0;
}

/*
============
HASHMAP_TEMPLATE_TAG::Begin
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
typename HASHMAP_TEMPLATE_TAG::Iterator HASHMAP_TEMPLATE_TAG::Begin() {
	return pairs.Begin();	
}

/*
============
HASHMAP_TEMPLATE_TAG::Begin
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
typename HASHMAP_TEMPLATE_TAG::ConstIterator HASHMAP_TEMPLATE_TAG::Begin() const {
	return pairs.Begin();
}

/*
============
HASHMAP_TEMPLATE_TAG::End
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
typename HASHMAP_TEMPLATE_TAG::Iterator HASHMAP_TEMPLATE_TAG::End() {
	return pairs.End();
}

/*
============
HASHMAP_TEMPLATE_TAG::End
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
typename HASHMAP_TEMPLATE_TAG::ConstIterator HASHMAP_TEMPLATE_TAG::End() const {
	return pairs.End();
}

/*
============
HASHMAP_TEMPLATE_TAG::Set
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
typename HASHMAP_TEMPLATE_TAG::InsertResult HASHMAP_TEMPLATE_TAG::Set( const key& key_, const type& value ) {	
	Iterator result = Find( key_ );
	if( result != End() ) {
		result->second = value;
		return InsertResult( result, false );
	}

	return SetNewItem( key_, value );
}

/*
============
HASHMAP_TEMPLATE_TAG::SetNewItem
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
typename HASHMAP_TEMPLATE_TAG::InsertResult HASHMAP_TEMPLATE_TAG::SetNewItem( const key& key_, const type& value ) {	
	assert( Find( key_ ) == End() );
	int hashKey = HashGenerator::Hash( hash, key_ );
	Iterator result = &pairs.Alloc();
	result->first = key_;
	result->second = value;

	hash.Add( hashKey,  pairs.Num() - 1 );
	return InsertResult( result, true );
}

/*
============
HASHMAP_TEMPLATE_TAG::SetGranularity
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
void HASHMAP_TEMPLATE_TAG::SetGranularity( int newGranularity ) {
	pairs.SetGranularity( newGranularity );
	hash.SetGranularity( newGranularity );
}

/*
============
HASHMAP_TEMPLATE_TAG::InitHash
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
void HASHMAP_TEMPLATE_TAG::InitHash( const int newHashSize, const int newIndexSize ) {
	hash.Init( newHashSize, newIndexSize );
}

/*
============
HASHMAP_TEMPLATE_TAG::FindIndex
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
typename HASHMAP_TEMPLATE_TAG::Iterator HASHMAP_TEMPLATE_TAG::FindIndex( int index ) {
	return &pairs[ index ];
}

/*
============
HASHMAP_TEMPLATE_TAG::FindIndex
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
typename HASHMAP_TEMPLATE_TAG::ConstIterator HASHMAP_TEMPLATE_TAG::FindIndex( int index ) const {
	return &pairs[ index ];
}

/*
============
HASHMAP_TEMPLATE_TAG::Clear
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
void HASHMAP_TEMPLATE_TAG::Clear() {
	pairs.Clear();
	hash.Clear();
}

/*
============
HASHMAP_TEMPLATE_TAG::Remove
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
bool HASHMAP_TEMPLATE_TAG::Remove( const key& key_ ) {
	int hashKey = HashGenerator::Hash( hash, key_ );		
	for( int i = hash.GetFirst( hashKey ); i != idHashIndex::NULL_INDEX; i = hash.GetNext( i ) ) {
		if( HashCompare::Compare( pairs[ i ].first, key_ )) {
			hash.RemoveIndex( hashKey, i );
			pairs.RemoveIndex( i );
			return true;
		}
	}
	return false;
}

/*
============
HASHMAP_TEMPLATE_TAG::operator[]
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
type&	HASHMAP_TEMPLATE_TAG::operator[]( const key& key_ ) {
	Iterator iter = Find( key_ );
	if( iter != End() ) {
		return iter->second;
	}
	
	InsertResult result = SetNewItem( key_, type() );
	return result.first->second;
}

/*
============
HASHMAP_TEMPLATE_TAG::Remove
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
void HASHMAP_TEMPLATE_TAG::Remove( Iterator iter ) {
	Remove( iter->first );
}

/*
============
HASHMAP_TEMPLATE_TAG::Count
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
int HASHMAP_TEMPLATE_TAG::Count( const key& key_ ) const {
	int hashKey = HashGenerator::Hash( hash, key_ );

	int count = 0;
	for( int i = hash.GetFirst( hashKey ); i != idHashIndex::NULL_INDEX; i = hash.GetNext( i ) ) {
		count++;
	}
	return count;
}

/*
============
HASHMAP_TEMPLATE_TAG::DeleteKeys
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
void HASHMAP_TEMPLATE_TAG::DeleteKeys() {
	for( int i = 0; i < pairs.Num(); i++ ) {
		delete pairs[ i ].first;
	}
}

/*
============
HASHMAP_TEMPLATE_TAG::DeleteValues
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
void HASHMAP_TEMPLATE_TAG::DeleteValues() {
	for( int i = 0; i < pairs.Num(); i++ ) {
		delete pairs[ i ].second;
	}
}

/*
============
HASHMAP_TEMPLATE_TAG::DeleteValues
============
*/
HASHMAP_TEMPLATE_HEADER
ID_INLINE 
void HASHMAP_TEMPLATE_TAG::Swap( sdHashMapGeneric& rhs ) {
	pairs.Swap( rhs.pairs );
	hash.Swap( rhs.hash );
}

#undef HASHMAP_TEMPLATE_HEADER
#undef HASHMAP_TEMPLATE_TAG

#endif // !__HASHMAP_GENERIC_H__
