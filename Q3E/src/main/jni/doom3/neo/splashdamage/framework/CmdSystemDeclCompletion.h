// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __CMDSYSTEM_DECL_COMPLETION_H__
#define __CMDSYSTEM_DECL_COMPLETION_H__

// TTimo: only on include paths that have both decls and cmdSystem, after cmdSystem declaration

template< declIdentifierType_t INDEX >
void idArgCompletionDecl_f( const idCmdArgs &args, void ( *callback )( const char* ) ) {
	cmdSystem->ArgCompletion_DeclName( args, callback, declIdentifierList[ INDEX ] );
}

#endif
