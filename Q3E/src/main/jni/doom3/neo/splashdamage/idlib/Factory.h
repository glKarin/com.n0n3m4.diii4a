// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __FACTORY_H__
#define __FACTORY_H__

#include "../framework/Common_public.h"

//===============================================================
//
//	sdFactory
//
//===============================================================

template<class T> 
class sdFactory {
public:
	typedef T* (*pfnCreate)(void);
	
	~sdFactory() {
		Shutdown();
	}

	void Shutdown() {
		creators.Clear();
	}

	bool CanHandleType( const char* name );
	bool RegisterType( const char* name, pfnCreate createFunc );
	T* CreateType( const char* name );

	template< typename S > static T* Allocator( void ) { return new S; }

private:
	idHashMap< pfnCreate > creators;
};

/*
============
sdFactory::RegisterType
============
*/
template<class T>
ID_INLINE bool sdFactory<T>::RegisterType( const char* name, pfnCreate createFunc ) {
	pfnCreate* findCreator;

	idStr lowerName = name;
	lowerName.ToLower();

	creators.Get( lowerName.c_str(), &findCreator );
	if( !findCreator ) {
		creators.Set( lowerName.c_str(), createFunc );
		return true;
	}
	return false;
}

/*
============
sdFactory::CanHandleType
============
*/
template<class T>
ID_INLINE bool sdFactory<T>::CanHandleType( const char* name ) {
	return creators.Get( name );
}

/*
============
sdFactory::CreateType
============
*/
template<class T>
ID_INLINE T* sdFactory<T>::CreateType( const char* name ) {
	pfnCreate* findCreator;
	
	idStr lowerName = name;
	lowerName.ToLower();

	creators.Get( lowerName.c_str(), &findCreator );
	if( findCreator && *findCreator ) {
		return (**findCreator)();
	}
	
	common->Warning( "Factory::CreateType: Unknown type %s", name );
	return NULL;
}

#endif /* !__FACTORY_H__ */
