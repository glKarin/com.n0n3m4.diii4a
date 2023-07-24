// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __DECLLIB_DECLTYPE_H__
#define __DECLLIB_DECLTYPE_H__

/*
====================================================================================

sdDeclInfo

====================================================================================
*/

/*
=================
sdDeclInfo::sdDeclInfo
=================
*/
ID_INLINE sdDeclInfo::sdDeclInfo( const char* typeName, int flags, pfnCacheFromDict cacheFromDict, pfnOnReload onReload ) : 
	_cacheFromDictFunction( cacheFromDict ),
	_onReload( onReload ),
	_flags( flags )
{
	idStr::Copynz( _typeName, typeName, sizeof( _typeName ) );
}

/*
====================================================================================

idDeclType

====================================================================================
*/

/*
=================
idDeclType::idDeclType
=================
*/
ID_INLINE idDeclType::idDeclType( void ) {
	declTypeHandle = -1;
}

/*
=================
idDeclType::OnRegister
=================
*/
ID_INLINE void idDeclType::OnRegister( qhandle_t handle ) {
	declTypeHandle = handle;
}

/*
=================
idDeclType::Create
=================
*/
ID_INLINE idDecl* idDeclType::Create( const char *name, const char *fileName ) const {
	return declManager->CreateNewDecl( declTypeHandle, name, fileName );
}

/*
=================
idDeclType::FindByIndex
=================
*/
ID_INLINE const idDecl* idDeclType::FindByIndex( int index, bool forceParse ) const {
	return declManager->DeclByIndex( declTypeHandle, index, forceParse );
}

/*
=================
idDeclType::Find
=================
*/
ID_INLINE const idDecl* idDeclType::Find( const char* name, bool makeDefault ) const {
	return declManager->FindType( declTypeHandle, name, makeDefault );
}

/*
=================
idDeclType::Num
=================
*/
ID_INLINE int idDeclType::Num( void ) const {
	return declManager->GetNumDecls( declTypeHandle );
}


/*
====================================================================================

sdDeclWrapper

====================================================================================
*/

/*
=================
sdDeclWrapper::sdDeclWrapper
=================
*/
ID_INLINE sdDeclWrapper::sdDeclWrapper( void ) {
	declTypeHandle = -1;
}

/*
=================
sdDeclWrapper::Init
=================
*/
ID_INLINE void sdDeclWrapper::Init( const char* identifier ) {
	declTypeHandle = declManager->GetDeclTypeHandle( identifier );
}

/*
=================
sdDeclWrapper::FindByIndex
=================
*/
ID_INLINE const idDecl* sdDeclWrapper::FindByIndex( int index, bool forceParse ) const {
	return declManager->DeclByIndex( declTypeHandle, index, forceParse );
}

/*
=================
sdDeclWrapper::Find
=================
*/
ID_INLINE const idDecl* sdDeclWrapper::Find( const char* name, bool makeDefault ) const {
	return declManager->FindType( declTypeHandle, name, makeDefault );
}

/*
=================
sdDeclWrapper::Num
=================
*/
ID_INLINE int sdDeclWrapper::Num( void ) const {
	return declManager->GetNumDecls( declTypeHandle );
}

/*
=================
sdDeclWrapper::CreateNewDecl
=================
*/
ID_INLINE const idDecl* sdDeclWrapper::CreateNewDecl( const char* name, const char* file ) const {
	return declManager->CreateNewDecl( declTypeHandle, name, file );
}

#endif // __DECLLIB_DECLTYPE_H__
