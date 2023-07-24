// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __DECL_PARSE_HELPER__
#define __DECL_PARSE_HELPER__

/*
==============
sdDeclParseHelper

A helper class to ensure that binary data generation is handled uniformly across decl types
Should be added to the Parse() member of an idDecl-derived class

To ensure that tokens are generated consistently it should be created after the LEXFL_  parsing flags are set on the input idParser

The destructor handles setting dependencies and storing any binary generated data
==============
*/


/*
============
sdDeclParseHelper
============
*/
class sdDeclParseHelper {
public:
							sdDeclParseHelper( idDecl* decl_, const char* text, int textLength, idParser& src_ );
							~sdDeclParseHelper();

	void					GetBinaryBuffer( const byte*& buffer, int& length ) const;	
	const idFile_Memory*	GetOutputFile() const { return binaryOutput; }
	void					Finish( void );
		
private:
	idParser&				src;				// parser that we're reading from/settings up	
	idFile_Memory*			binaryOutput;		// tokenized parser output to be stored on the decl	

	idDecl*					decl;				// source/target decl
	idDeclTypeInterface*	type;				// type of the decl, used for decl type-specific behavior
	
	byte*					declBuffer;			// binary buffer retrieved from the decl
	int						declBufferLength;	// binary buffer length retrieved from the decl
};


/*
============
sdDeclParseHelper::sdDeclParseHelper
============
*/
ID_INLINE sdDeclParseHelper::sdDeclParseHelper( idDecl* decl_, const char* text, int textLength, idParser& src_ ) :
	binaryOutput( NULL ),
	decl( decl_ ),
	src( src_ ),
	declBuffer( NULL ),
	declBufferLength( 0 ) {

	type = declManager->GetDeclType( decl->GetType() );

	idTokenCache* cache = NULL;
	if( !type->UsePrivateTokens() ) {
		cache = &declManager->GetGlobalTokenCache();
	}

	if( decl->HasBinaryBuffer() && decl->GetState() != DS_DEFAULTED ) {
		decl->GetBinarySource( declBuffer, declBufferLength );
		src.LoadMemoryBinary( declBuffer, declBufferLength, va( "%s: %s", decl->GetFileName(), decl->GetName() ), cache );
	} else {
		// store expanded text (except for templates) so that it's written in its entirety to the binary decl file
		if( cvarSystem->GetCVarBool( "com_writeBinaryDecls" ) && !type->WriteBinary() && !type->AlwaysGenerateBinary() && decl->GetState() != DS_DEFAULTED ) {
			src.LoadMemory( text, textLength, va( "%s: %s", decl->GetFileName(), decl->GetName() ), decl->GetLineNum() );
			if( decl->GetFileLevelIncludeDependencies() != NULL ) {
				src.AddIncludes( *( decl->GetFileLevelIncludeDependencies() ) );
			}

			sdStringBuilder_Heap builder;

			idToken token;
			while( src.ReadToken( &token )) {
				if( token.type == TT_STRING ) {
					builder += "\"";
				}
				builder += token;
				if( token.type == TT_STRING ) {
					builder += "\"";
				}
				builder += " ";
			}
			decl->SetText( builder.c_str() );
			src.FreeSource( true );
		}		
		src.LoadMemory( text, textLength, va( "%s: %s", decl->GetFileName(), decl->GetName() ), decl->GetLineNum() );
	}

	// no need to setup file dependencies in binary mode
	if( decl->GetFileLevelIncludeDependencies() != NULL && !decl->HasBinaryBuffer() ) {
		src.AddIncludes( *( decl->GetFileLevelIncludeDependencies() ) );
	}

	if( type->WriteBinary() ) {
		if( ( cvarSystem->GetCVarBool( "com_writeBinaryDecls" ) || type->AlwaysGenerateBinary() ) && 
			( !decl->HasBinaryBuffer() || ( type->AlwaysGenerateBinary() && decl->GetState() == DS_DEFAULTED ) ) ) {
			binaryOutput = fileSystem->OpenMemoryFile( decl->GetName() );
			binaryOutput->SetGranularity( 256 );

			src.WriteBinary( binaryOutput, cache );
			src.ResetBinaryParsing();
		}
	}
}

/*
============
sdDeclParseHelper::~sdDeclParseHelper
============
*/
ID_INLINE sdDeclParseHelper::~sdDeclParseHelper() {
	Finish();
}

/*
============
sdDeclParseHelper::GetBinaryBuffer
============
*/
ID_INLINE void sdDeclParseHelper::GetBinaryBuffer( const byte*& buffer, int& length ) const {
	if( declBuffer == NULL || declBufferLength == 0 ) {
		if( binaryOutput != NULL ) {
			buffer = reinterpret_cast< const byte* >( binaryOutput->GetDataPtr() );
			length = binaryOutput->Length();
			return;
		}
	}

	buffer = declBuffer;
	length = declBufferLength;
}

/*
============
sdDeclParseHelper::Finish
============
*/
ID_INLINE void sdDeclParseHelper::Finish( void ) {
	// no dependencies in binary mode, except for types that always generate
	if( decl != NULL && ( binaryOutput == NULL || type->AlwaysGenerateBinary() ) ) {
		declManager->AddDependencies( decl, src );				
	}

	if( decl != NULL && binaryOutput != NULL && decl->GetState() != DS_DEFAULTED ) {
		decl->SetBinarySource( reinterpret_cast< const byte* >( binaryOutput->GetDataPtr() ), binaryOutput->Length() );
	}

	if( decl != NULL && declBuffer != NULL ) {
		decl->FreeSourceBuffer( declBuffer );
		declBuffer = NULL;
	}

	if( binaryOutput != NULL ) {
		fileSystem->CloseFile( binaryOutput );
		binaryOutput = NULL;
	}

	decl = NULL;
}


#endif // !__DECL_PARSE_HELPER__
