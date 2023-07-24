// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __HASHINDEX_IMPL_H__
#define __HASHINDEX_IMPL_H__

#define HASHINDEX_TEMPLATE_HEADER template< class type, int Max, int NullIndex, int DEFAULT_HASH_SIZE, int DEFAULT_HASH_GRANULARITY >
#define HASHINDEX_TEMPLATE_TAG idHashIndexBase< type, Max, NullIndex, DEFAULT_HASH_SIZE, DEFAULT_HASH_GRANULARITY >

/*
================
idHashIndexBase::Write
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE void HASHINDEX_TEMPLATE_TAG::Write( idFile* file ) const {
	file->WriteInt( hashSize );
	if ( hash ) {
		file->WriteBool( true );
		file->Write( hash, sizeof( hash[0] ) * hashSize );
	} else {
		file->WriteBool( false );
	}

	file->WriteInt( indexSize );
	if ( indexChain ) {
		file->WriteBool( true );
		file->Write( indexChain, sizeof( indexChain[0] ) * indexSize );
	} else {
		file->WriteBool( false );
	}

	file->WriteInt( granularity );
	file->WriteInt( hashMask );
}

/*
================
idHashIndexBase::Read
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE void HASHINDEX_TEMPLATE_TAG::Read( idFile* file ) {
	Free();

	bool temp;

	file->ReadInt( hashSize );
	file->ReadBool( temp );
	if ( temp ) {
		hash = new type[ hashSize ];
		file->Read( hash, sizeof( hash[0] ) * hashSize );
	} else {
		hash = NULL;
	}

	file->ReadInt( indexSize );
	file->ReadBool( temp );
	if ( temp ) {
		indexChain = new type[ indexSize ];
		file->Read( indexChain, sizeof( indexChain[0] ) * indexSize );
	} else {
		indexChain = NULL;
	}

	file->ReadInt( granularity );
	file->ReadInt( hashMask );
}

#undef HASHINDEX_TEMPLATE_HEADER
#undef HASHINDEX_TEMPLATE_TAG

#endif // !__HASHINDEX_IMPL_H__
