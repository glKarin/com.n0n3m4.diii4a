/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __DECLMANAGER_H__
#define __DECLMANAGER_H__

#define DECL_PROGRAM_GENERATED_DIRECTORY "_program_generated"

#if defined(_RAVEN) || defined(_SPLASHDAMAGE) //karin: BSE
class rvDeclEffect;
#endif
#ifdef _RAVEN // quake4 new decl
class rvDeclPlayback;
class rvDeclLipSync;
class rvDeclMatType;
#endif

/*
===============================================================================

	Declaration Manager

	All "small text" data types, like materials, sound shaders, fx files,
	entity defs, etc. are managed uniformly, allowing reloading, purging,
	listing, printing, etc. All "large text" data types that never have more
	than one declaration in a given file, like maps, models, AAS files, etc.
	are not handled here.

	A decl will never, ever go away once it is created. The manager is
	garranteed to always return the same decl pointer for a decl type/name
	combination. The index of a decl in the per type list also stays the
	same throughout the lifetime of the engine. Although the pointer to
	a decl always stays the same, one should never maintain pointers to
	data inside decls. The data stored in a decl is not garranteed to stay
	the same for more than one engine frame.

	The decl indexes of explicitely defined decls are garrenteed to be
	consistent based on the parsed decl files. However, the indexes of
	implicit decls may be different based on the order in which levels
	are loaded.

	The decl namespaces are separate for each type. Comments for decls go
	above the text definition to keep them associated with the proper decl.

	During decl parsing, errors should never be issued, only warnings
	followed by a call to MakeDefault().

===============================================================================
*/

typedef enum {
	DECL_TABLE				= 0,
	DECL_MATERIAL,
	DECL_SKIN,
	DECL_SOUND,
	DECL_ENTITYDEF,
#ifdef _SPLASHDAMAGE //karin: sync with declIdentifierType_t
	DECL_EFFECT,
	DECL_AF,
	DECL_ATMOSPHERE,
	DECL_AMBIENTCUBEMAP,
	DECL_STUFFTYPE,
	DECL_SURFACETYPE,
	DECL_SURFACETYPEMAP,
	DECL_RENDERPROGRAM,
	DECL_RENDERBINDING,
	DECL_TEMPLATE,
	DECL_IMPOSTER,
	DECL_IMPOSTERGENERATOR,
	DECL_LOCSTR,
	DECL_DECAL,
	DECL_MODELEXPORT,
	DECL_FONT,
#endif
	DECL_MODELDEF,
	DECL_FX,
	DECL_PARTICLE,
#if !defined(_SPLASHDAMAGE) //karin: sync with declIdentifierType_t
	DECL_AF,
#endif
	DECL_PDA,
	DECL_VIDEO,
	DECL_AUDIO,
	DECL_EMAIL,
#if !defined(_SPLASHDAMAGE) //karin: sync with declIdentifierType_t
	DECL_MODELEXPORT,
#endif
	DECL_MAPDEF,

	// new decl types can be added here

#ifdef _RAVEN // quake4 new decl
	// RAVEN BEGIN
// jscott: added new decls
    DECL_MATERIALTYPE,
    DECL_LIPSYNC,
    DECL_PLAYBACK,
    DECL_EFFECT,
// rjohnson: camera is now contained in a def for frame commands
    DECL_CAMERADEF,
// jscott: don't use these
//      DECL_FX,
//      DECL_PARTICLE,
// RAVEN END
//

	// new decl types can be added here
	DECL_PLAYER_MODEL,
#endif
#ifdef _HUMANHEAD
    DECL_BEAM, // HUMANHEAD CJR
#endif
#ifdef _SPLASHDAMAGE //karin: increment num decl types, but I think it should be dynamic-length
	DECL_MAX_TYPES			= 96
#else
	DECL_MAX_TYPES			= 32
#endif
}
#ifdef _SPLASHDAMAGE
;
typedef qhandle_t declType_t;
#else
declType_t;
#endif

#ifdef _SPLASHDAMAGE
extern const char* declTableIdentifier;
extern const char* declMaterialIdentifier;
extern const char* declSkinIdentifier;
extern const char* declSoundShaderIdentifier;
extern const char* declEntityDefIdentifier;
extern const char* declEffectsIdentifier;
extern const char* declAFIdentifier;
extern const char* declAtmosphereIdentifier;
extern const char* declAmbientCubeMapIdentifier;
extern const char* declStuffTypeIdentifier;
extern const char* declDecalIdentifier;
extern const char* declSurfaceTypeIdentifier;
extern const char* declSurfaceTypeMapIdentifier;
extern const char* declRenderProgramIdentifier;
extern const char* declRenderBindingIdentifier;
extern const char* declTemplateIdentifier;
extern const char* declImposterIdentifier;
extern const char* declImposterGeneratorIdentifier;
extern const char* declLocStrIdentifier;
extern const char* declModelExportIdentifier;

typedef enum declIdentifierType_e {
    DECLTYPE_TABLE,
    DECLTYPE_MATERIAL,
    DECLTYPE_SKIN,
    DECLTYPE_SOUND,
    DECLTYPE_ENTITYDEF,
    DECLTYPE_EFFECT,
    DECLTYPE_AF,
    DECLTYPE_ATMOSPHERE,
    DECLTYPE_AMBIENTCUBEMAP,
    DECLTYPE_STUFFTYPE,
    DECLTYPE_SURFACETYPE,
    DECLTYPE_SURFACETYPEMAP,
    DECLTYPE_RENDERPROGRAM,
    DECLTYPE_RENDERBINDING,
    DECLTYPE_TEMPLATE,
    DECLTYPE_IMPOSTER,
    DECLTYPE_IMPOSTERGENERATOR,
    DECLTYPE_LOCSTR,
    DECLTYPE_DECAL,
    DECLTYPE_MODELEXPORT,
    DECLTYPE_FONT,
} declIdentifierType_t;

extern const char* declIdentifierList[];
#endif

typedef enum {
	DS_UNPARSED,
	DS_DEFAULTED,			// set if a parse failed due to an error, or the lack of any source
	DS_PARSED
} declState_t;

#ifdef _SPLASHDAMAGE
const int DECL_LEXER_FLAGS	=	LEXFL_NOSTRINGCONCAT				// multiple strings seperated by whitespaces are not concatenated
								| LEXFL_NOSTRINGESCAPECHARS			// no escape characters inside strings
//								| LEXFL_NODOLLARPRECOMPILE			// don't use the $ sign for precompilation
								| LEXFL_ALLOWPATHNAMES				// allow path seperators in names
								| LEXFL_ALLOWMULTICHARLITERALS		// allow multi character literals
								| LEXFL_ALLOWBACKSLASHSTRINGCONCAT	// allow multiple strings seperated by '\' to be concatenated
//								| LEXFL_NOFATALERRORS				// just set a flag instead of fatal erroring
								| LEXFL_ALLOWRAWSTRINGBLOCKS		// allow raw text blocks embraced with <% %>
								;
#else
const int DECL_LEXER_FLAGS	=	LEXFL_NOSTRINGCONCAT |				// multiple strings seperated by whitespaces are not concatenated
                                        LEXFL_NOSTRINGESCAPECHARS |			// no escape characters inside strings
                                        LEXFL_ALLOWPATHNAMES |				// allow path seperators in names
                                        LEXFL_ALLOWMULTICHARLITERALS |		// allow multi character literals
                                        LEXFL_ALLOWBACKSLASHSTRINGCONCAT |	// allow multiple strings seperated by '\' to be concatenated
                                        LEXFL_NOFATALERRORS;				// just set a flag instead of fatal erroring
#endif


class idDeclBase
{
	public:
		virtual 				~idDeclBase() {};
		virtual const char 	*GetName(void) const = 0;
		virtual declType_t		GetType(void) const = 0;
		virtual declState_t		GetState(void) const = 0;
		virtual bool			IsImplicit(void) const = 0;
		virtual bool			IsValid(void) const = 0;
		virtual void			Invalidate(void) = 0;
		virtual void			Reload(void) = 0;
		virtual void			EnsureNotPurged(void) = 0;
		virtual int				Index(void) const = 0;
		virtual int				GetLineNum(void) const = 0;
		virtual const char 	*GetFileName(void) const = 0;
		virtual void			GetText(char *text) const = 0;
		virtual int				GetTextLength(void) const = 0;
#ifdef _SPLASHDAMAGE
    	virtual void			SetBinarySource( const byte* source, int length ) = 0;
	    virtual void			GetBinarySource( byte*& source, int& length ) const = 0;
    	virtual void			FreeSourceBuffer( byte* buffer ) const = 0;
	    virtual bool			HasBinaryBuffer() const = 0;
#endif
		virtual void			SetText(const char *text) = 0;
		virtual bool			ReplaceSourceFileText(void) = 0;
		virtual bool			SourceFileChanged(void) const = 0;
		virtual void			MakeDefault(void) = 0;
		virtual bool			EverReferenced(void) const = 0;
		virtual bool			SetDefaultText(void) = 0;
		virtual const char 	*DefaultDefinition(void) const = 0;
#ifdef _RAVEN
		virtual bool			Parse(const char *text, const int textLength, bool noCaching = false) = 0;
#else
		virtual bool			Parse(const char *text, const int textLength) = 0;
#endif
		virtual void			FreeData(void) = 0;
		virtual size_t			Size(void) const = 0;
		virtual void			List(void) const = 0;
		virtual void			Print(void) const = 0;
#ifdef _SPLASHDAMAGE
		virtual const idStrList&	GetIncludeDependencies() const = 0;
    	virtual const idStrList*	GetFileLevelIncludeDependencies() const = 0;
#endif
};

#ifdef _RAVEN // quake4 guide
// RAVEN BEGIN
// jscott: for guides
#define MAX_GUIDE_PARMS				20
#define	MAX_GUIDE_SHADER_SIZE		20480

class rvDeclGuide
{
private:
    idStr		mName;
    idStr		mParms[MAX_GUIDE_PARMS];
    idStr		mDefinition;
    int			mNumParms;

public:
    rvDeclGuide( idStr &name );
    ~rvDeclGuide( void );

    const char	*GetName( void ) const
    {
        return( mName.c_str() );
    }
    int			GetNumParms( void ) const
    {
        return( mNumParms );
    }
    const char	*GetParm( int index ) const
    {
        assert( index < mNumParms );
        return( mParms[index].c_str() );
    }

    void		SetParm( int index, const char *value );
    void		RemoveOuterBracing( void );
    void		Parse( idLexer *src );
    bool		Evaluate( idLexer *src, idStr &definition );
};
// RAVEN END
#endif

class idDecl
{
	public:
		// The constructor should initialize variables such that
		// an immediate call to FreeData() does no harm.
		idDecl(void) {
			base = NULL;
		}
		virtual 				~idDecl(void) {};

		// Returns the name of the decl.
		const char 			*GetName(void) const {
			return base->GetName();
		}

		// Returns the decl type.
		declType_t				GetType(void) const {
			return base->GetType();
		}

		// Returns the decl state which is usefull for finding out if a decl defaulted.
		declState_t				GetState(void) const {
			return base->GetState();
		}

		// Returns true if the decl was defaulted or the text was created with a call to SetDefaultText.
		bool					IsImplicit(void) const {
			return base->IsImplicit();
		}

		// The only way non-manager code can have an invalid decl is if the *ByIndex()
		// call was used with forceParse = false to walk the lists to look at names
		// without touching the media.
		bool					IsValid(void) const {
			return base->IsValid();
		}

		// Sets state back to unparsed.
		// Used by decl editors to undo any changes to the decl.
		void					Invalidate(void) {
			base->Invalidate();
		}

		// if a pointer might possible be stale from a previous level,
		// call this to have it re-parsed
		void					EnsureNotPurged(void) {
			base->EnsureNotPurged();
		}

		// Returns the index in the per-type list.
		int						Index(void) const {
			return base->Index();
		}

		// Returns the line number the decl starts.
		int						GetLineNum(void) const {
			return base->GetLineNum();
		}

		// Returns the name of the file in which the decl is defined.
		const char 			*GetFileName(void) const {
			return base->GetFileName();
		}

		// Returns the decl text.
		void					GetText(char *text) const {
			base->GetText(text);
		}

		// Returns the length of the decl text.
		int						GetTextLength(void) const {
			return base->GetTextLength();
		}

		// Sets new decl text.
		void					SetText(const char *text) {
			base->SetText(text);
		}

#ifdef _SPLASHDAMAGE
		// Sets new binary source data.
		void					SetBinarySource( const byte* source, int length )  { base->SetBinarySource( source, length ); }

		// Retrieve binary source data. FreeSourceBuffer must be called when the client is finished with source.
		void					GetBinarySource( byte*& source, int& length ) const { base->GetBinarySource( source, length ); }

		// Frees the buffer allocated by a previous call to GetBinarySource.
		void					FreeSourceBuffer( byte* source ) const { base->FreeSourceBuffer( source ); }

		bool					HasBinaryBuffer() const { return base->HasBinaryBuffer(); }
#endif

		// Saves out new text for the decl.
		// Used by decl editors to replace the decl text in the source file.
		bool					ReplaceSourceFileText(void) {
			return base->ReplaceSourceFileText();
		}

		// Returns true if the source file changed since it was loaded and parsed.
		bool					SourceFileChanged(void) const {
			return base->SourceFileChanged();
		}

		// Frees data and makes the decl a default.
		void					MakeDefault(void) {
			base->MakeDefault();
		}

		// Returns true if the decl was ever referenced.
		bool					EverReferenced(void) const {
			return base->EverReferenced();
		}

	public:
		// Sets textSource to a default text if necessary.
		// This may be overridden to provide a default definition based on the
		// decl name. For instance materials may default to an implicit definition
		// using a texture with the same name as the decl.
		virtual bool			SetDefaultText(void) {
			return base->SetDefaultText();
		}

		// Each declaration type must have a default string that it is guaranteed
		// to parse acceptably. When a decl is not explicitly found, is purged, or
		// has an error while parsing, MakeDefault() will do a FreeData(), then a
		// Parse() with DefaultDefinition(). The defaultDefintion should start with
		// an open brace and end with a close brace.
		virtual const char 	*DefaultDefinition(void) const {
			return base->DefaultDefinition();
		}

		// The manager will have already parsed past the type, name and opening brace.
		// All necessary media will be touched before return.
		// The manager will have called FreeData() before issuing a Parse().
		// The subclass can call MakeDefault() internally at any point if
		// there are parse errors.
#ifdef _RAVEN
		virtual bool			Parse(const char *text, const int textLength , bool noCaching = false) {
			return base->Parse(text, textLength, noCaching/*false*/);
		}
#else
		virtual bool			Parse(const char *text, const int textLength) {
			return base->Parse(text, textLength);
		}
#endif

		// Frees any pointers held by the subclass. This may be called before
		// any Parse(), so the constructor must have set sane values. The decl will be
		// invalid after issuing this call, but it will always be immediately followed
		// by a Parse()
		virtual void			FreeData(void) {
			base->FreeData();
		}

		// Returns the size of the decl in memory.
		virtual size_t			Size(void) const {
			return base->Size();
		}

		// If this isn't overridden, it will just print the decl name.
		// The manager will have printed 7 characters on the line already,
		// containing the reference state and index number.
		virtual void			List(void) const {
			base->List();
		}

		// The print function will already have dumped the text source
		// and common data, subclasses can override this to dump more
		// explicit data.
		virtual void			Print(void) const {
			base->Print();
		}
#ifdef _SPLASHDAMAGE
		virtual const idStrList&	GetIncludeDependencies() const { return base->GetIncludeDependencies(); }
		virtual const idStrList*	GetFileLevelIncludeDependencies() const { return base->GetFileLevelIncludeDependencies(); }
#endif

	public:
		idDeclBase 			*base;
};


template< class type >
ID_INLINE idDecl *idDeclAllocator(void)
{
	return new type;
}


#ifdef _SPLASHDAMAGE
/*
============
idDeclLocal
============
*/
class idDeclLocal;

class idDeclTypeInterface
{
public:
    typedef void ( *pfnOnPostParse )( idDecl* decl );

    virtual						~idDeclTypeInterface( void ) { }

    virtual bool				SkipChecksum( void ) const = 0;
    virtual bool				AllowTemplateEvaluation( void ) const = 0;
    virtual bool				SkipParsing( void ) const = 0;
    virtual bool				NotPrecached( void ) const = 0;
    virtual bool				AlwaysGenerateBinary( void ) const = 0;
    virtual bool				UsePrivateTokens( void ) const = 0;
    virtual bool				WriteBinary( void ) const = 0;
    virtual bool				NeverStoreBinary( void ) const = 0;

    virtual void				OnRegister( qhandle_t handle ) = 0;
    virtual qhandle_t			GetHandle( void ) const = 0;

    virtual idDecl*				Alloc( void ) = 0;
    virtual void				OnReload( idDecl* decl ) const = 0;
    virtual idDecl*				Create( const char *name, const char *fileName ) const = 0;
    virtual const idDecl*		FindByIndex( int index, bool forceParse = true ) const = 0;
    virtual const idDecl*		Find( const char* name, bool makeDefault = true ) const = 0;
    virtual int					Num( void ) const = 0;
    virtual const char*			GetName( void ) const = 0;
    virtual void				CacheFromDict( const idDict& dict ) const = 0;
    virtual bool				CanCacheFromDict() const = 0;
    virtual void				PostParse( idDecl* decl ) const = 0;

    virtual void				RegisterPostParse( pfnOnPostParse postParse ) const = 0;
	virtual void				UnregisterPostParse( pfnOnPostParse postParse ) const = 0;

#ifdef _SPLASHDAMAGE
	public:
		idStr					typeName;
		declType_t				type;
		idDecl *(*allocator)(void);
		idDeclTypeInterface *ref; // from game
#endif
};


/*
============
idDeclType
============
*/
class idDeclType : public idDeclTypeInterface
{
public:
    idDeclType( void );
    virtual						~idDeclType( void ) {}

    virtual void				OnRegister( qhandle_t handle );
    virtual qhandle_t			GetHandle( void ) const
    {
        return declTypeHandle;
    }

    virtual idDecl*				Create( const char *name, const char *fileName ) const;
    virtual const idDecl*		FindByIndex( int index, bool forceParse = true ) const;
    virtual const idDecl*		Find( const char* name, bool makeDefault = true ) const;
    virtual int					Num( void ) const;

#ifdef _SPLASHDAMAGE //karin: for compat
	virtual bool				SkipChecksum( void ) const;
	virtual bool				AllowTemplateEvaluation( void ) const;
	virtual bool				SkipParsing( void ) const;
	virtual bool				NotPrecached( void ) const;
	virtual bool				AlwaysGenerateBinary( void ) const;
	virtual bool				UsePrivateTokens( void ) const;
	virtual bool				WriteBinary( void ) const;
	virtual bool				NeverStoreBinary( void ) const;

	virtual idDecl*				Alloc( void );
	virtual void				OnReload( idDecl* decl ) const;
	virtual const char*			GetName( void ) const;
	virtual void				CacheFromDict( const idDict& dict ) const;
	virtual bool				CanCacheFromDict() const;
	virtual void				PostParse( idDecl* decl ) const;

	virtual void				RegisterPostParse( pfnOnPostParse postParse ) const;
	virtual void				UnregisterPostParse( pfnOnPostParse postParse ) const;
#endif
private:
    qhandle_t					declTypeHandle;
};


/*
============
sdDeclInfo
============
*/
enum declInfoFlags_e {
    DIF_NONE						= 0,
    DIF_ALLOW_TEMPLATES 			= BITT< 0 >::VALUE,		// allow decl template evaluation
    DIF_SKIP_PARSING				= BITT< 1 >::VALUE,		// don't parse text when touched
    DIF_NOT_PRECACHED				= BITT< 2 >::VALUE,		// not explicitly pre-cached; always write during binary builds
    DIF_SKIP_CHECKSUM				= BITT< 3 >::VALUE,		// not checksummed for pure
    DIF_ALWAYS_GENERATE_BINARY		= BITT< 4 >::VALUE,		// always generate binary data, even when com_writeBinaryDecls is disabled
    DIF_USE_PRIVATE_TOKENS			= BITT< 5 >::VALUE,		// don't use the global binary token pool
    DIF_WRITE_BINARY				= BITT< 6 >::VALUE,		// allow writing of binary data when com_writeBinaryDecls is enabled
    DIF_NEVER_STORE_BINARY			= BITT< 7 >::VALUE,		// never store text or binary data when com_writeBinaryDecls is enabled
};

class sdDeclInfo
{
public:
    static const int MAX_DECLINFO_STRING = 256;
    typedef void ( *pfnCacheFromDict )( const idDict& dict );
    typedef void ( *pfnOnReload )( idDecl* );
    typedef void ( *pfnOnPostParse )( idDecl* );

    sdDeclInfo( const char* typeName, int flags = DIF_ALLOW_TEMPLATES, pfnCacheFromDict cacheFromDict = NULL, pfnOnReload onReload = NULL );

    const char*					GetTypeName( void ) const
    {
        return _typeName;
    }
    pfnCacheFromDict			CacheFromDict( void ) const
    {
        return _cacheFromDictFunction;
    }
    pfnOnReload					OnReload( void ) const
    {
        return _onReload;
    }

    bool						SkipChecksum( void ) const
    {
        return ( _flags & DIF_SKIP_CHECKSUM ) != 0;
    }
    bool						AllowTemplateEvaluation( void ) const
    {
        return ( _flags & DIF_ALLOW_TEMPLATES ) != 0;
    }
    bool						SkipParsing( void ) const
    {
        return ( _flags & DIF_SKIP_PARSING ) != 0;
    }
    bool						NotPrecached( void ) const
    {
        return ( _flags & DIF_NOT_PRECACHED ) != 0;
    }
    bool						AlwaysGenerateBinary( void ) const
    {
        return ( _flags & DIF_ALWAYS_GENERATE_BINARY ) != 0;
    }
    bool						UsePrivateTokens( void ) const
    {
        return ( _flags & DIF_USE_PRIVATE_TOKENS ) != 0;
    }
    bool						WriteBinary( void ) const
    {
        return ( _flags & DIF_WRITE_BINARY ) != 0;
    }
    bool						NeverStoreBinary( void ) const
    {
        return ( _flags & DIF_NEVER_STORE_BINARY ) != 0;
    }

    // jrad - postParse callbacks
    void						RegisterPostParse( pfnOnPostParse postParse ) const
    {
        assert( postParse != NULL );
        _onPostParse.AddUnique( postParse );
    }

    void						UnregisterPostParse( pfnOnPostParse postParse ) const
    {
        assert( postParse );
        _onPostParse.Remove( postParse );
    }
    void						PostParse( idDecl* decl ) const
    {
        for( int i = 0; i < _onPostParse.Num(); i++ ) {
            assert( _onPostParse[ i ] != NULL );
            _onPostParse[ i ]( decl );
        }
    }
private:
    int									_flags;
    char								_typeName[ MAX_DECLINFO_STRING ];
    pfnCacheFromDict					_cacheFromDictFunction;
    pfnOnReload							_onReload;
    mutable idList<	pfnOnPostParse >	_onPostParse;
};

/*
============
idDeclTypeTemplate
============
*/
template< typename DECLTYPE, const sdDeclInfo* INFO >
class idDeclTypeTemplate : public idDeclType
{
public:
    typedef DECLTYPE			DeclType;

    virtual						~idDeclTypeTemplate( void ) {}

    virtual idDecl*				Alloc( void )
    {
        return new DECLTYPE;
    }

    DECLTYPE*					LocalCreate( const char *name, const char *fileName ) const
    {
        return static_cast< DECLTYPE* >( Create( name, fileName) );
    }
    const DECLTYPE*				LocalFind( const char* name, bool makeDefault = true ) const
    {
        return static_cast< const DECLTYPE* >( Find( name, makeDefault ) );
    }
    const DECLTYPE*				LocalFindByIndex( int index, bool forceParse = true ) const
    {
        return static_cast< const DECLTYPE* >( FindByIndex( index, forceParse ) );
    }
    const DECLTYPE*				operator[]( const char* name ) const
    {
        return static_cast< const DECLTYPE* >( Find( name, false ) );
    }
    const DECLTYPE*				operator[]( int index ) const
    {
        return static_cast< const DECLTYPE* >( FindByIndex( index, true ) );
    }
    const DECLTYPE*				SafeIndex( int index ) const
    {
        return ( index < 0 || index >= Num() ) ? NULL : static_cast< const DECLTYPE* >( FindByIndex( index, true ) );
    }

    virtual bool				SkipChecksum( void ) const
    {
        return INFO->SkipChecksum();
    }
    virtual bool				SkipParsing( void ) const
    {
        return INFO->SkipParsing();
    }
    virtual bool				NotPrecached( void ) const
    {
        return INFO->NotPrecached();
    }
    virtual bool				AlwaysGenerateBinary( void ) const
    {
        return INFO->AlwaysGenerateBinary();
    }
    virtual bool				UsePrivateTokens( void ) const
    {
        return INFO->UsePrivateTokens();
    }
    virtual bool				WriteBinary( void ) const
    {
        return INFO->WriteBinary();
    }
    virtual bool				NeverStoreBinary( void ) const
    {
        return INFO->NeverStoreBinary();
    }
    virtual const char*			GetName( void ) const
    {
        return INFO->GetTypeName();
    }
    virtual void				CacheFromDict( const idDict& dict ) const
    {
        if ( INFO->CacheFromDict() ) {
            INFO->CacheFromDict()( dict );
        }
    }
    virtual bool				CanCacheFromDict() const
    {
        return INFO->CacheFromDict() != NULL;
    }
    virtual void				OnReload( idDecl* decl ) const
    {
        if ( INFO->OnReload() ) {
            INFO->OnReload()( decl );
        }
    }
    virtual bool				AllowTemplateEvaluation( void ) const
    {
        return INFO->AllowTemplateEvaluation();
    }

    virtual void				PostParse( idDecl* decl ) const
    {
        INFO->PostParse( decl );
    }
    virtual void				RegisterPostParse( pfnOnPostParse postParse ) const
    {
        assert( postParse );
        INFO->RegisterPostParse( postParse );
        for( int i = 0; i < this->Num(); i++ ) {
            idDecl* decl = const_cast< idDecl* >( FindByIndex( i, false ));
            declState_t state = decl->GetState();
            if( state == DS_PARSED || state == DS_DEFAULTED )  {
                postParse( decl );
            }
        }
    }
    virtual void				UnregisterPostParse( pfnOnPostParse postParse ) const
    {
        INFO->UnregisterPostParse( postParse );
    }
};


/*
============
sdDeclWrapper
============
*/
class sdDeclWrapper
{
public:
    sdDeclWrapper( void );

    void						Init( const char* identifier );

    const idDecl*				FindByIndex( int index, bool forceParse = true ) const;
    const idDecl*				Find( const char* name, bool makeDefault = true ) const;
    int							Num( void ) const;
    qhandle_t					GetHandle( void ) const
    {
        return declTypeHandle;
    }
    const idDecl*				CreateNewDecl( const char *name, const char *file ) const;

private:
    qhandle_t					declTypeHandle;
};

/*
============
sdDeclWrapperTemplate
============
*/
template< typename DECLTYPE >
class sdDeclWrapperTemplate : public sdDeclWrapper
{
public:
    const DECLTYPE*				New( const char* name, const char* file ) const
    {
        return reinterpret_cast< const DECLTYPE* >( CreateNewDecl( name, file ) );
    }
    const DECLTYPE*				operator[]( const char* name ) const
    {
        return reinterpret_cast< const DECLTYPE* >( Find( name, false ) );
    }
    const DECLTYPE*				operator[]( int index ) const
    {
        return reinterpret_cast< const DECLTYPE* >( FindByIndex( index, true ) );
    }
    const DECLTYPE*				SafeIndex( int index ) const
    {
        return ( index < 0 || index >= Num() ) ? NULL : reinterpret_cast< const DECLTYPE* >( FindByIndex( index, true ) );
    }
    const DECLTYPE*				LocalFind( const char* name, bool makeDefault = true ) const
    {
        return reinterpret_cast< const DECLTYPE* >( Find( name, makeDefault ) );
    }
    const DECLTYPE*				LocalFindByIndex( int index, bool forceParse = true ) const
    {
        return reinterpret_cast< const DECLTYPE* >( FindByIndex( index, forceParse ) );
    }
};
#endif


class idMaterial;
class idDeclSkin;
class idSoundShader;
#ifdef _RAVEN
class idDeclTable;
//k for FindMapDef
class idDeclEntityDef;
#endif
#ifdef _HUMANHEAD
class hhDeclBeam; // HUMANHEAD CJR
#endif

class idDeclManager
{
	public:
		virtual					~idDeclManager(void) {}

		virtual void			Init(void) = 0;
		virtual void			Shutdown(void) = 0;
		virtual void			Reload(bool force) = 0;

		virtual void			BeginLevelLoad() = 0;
		virtual void			EndLevelLoad() = 0;

#ifdef _SPLASHDAMAGE
	    // Returns the system token cache
	    virtual idTokenCache&	GetGlobalTokenCache() = 0;

    	// Registers a new decl type.
    	virtual void			RegisterDeclType( idDeclTypeInterface* type ) = 0;
    	virtual void			UnregisterDeclType( idDeclTypeInterface* type ) = 0;

	    // Registers a new folder with decl files.
	    virtual void			RegisterDeclFolder( const char *folder, const char *extension ) = 0;

	    //Unregister a previously-registered folder
	    virtual void			UnregisterDeclFolder( const char *folder, const char *extension ) = 0;

	    // Called when finished registering decl folders
	    // attempts to find binary decls without a source text file and load them properly
	    virtual void			FinishedRegistering() = 0;
#endif
		// Registers a new decl type.
		virtual void			RegisterDeclType(const char *typeName, declType_t type, idDecl *(*allocator)(void)) = 0;

		// Registers a new folder with decl files.
		virtual void			RegisterDeclFolder(const char *folder, const char *extension, declType_t defaultType) = 0;

		// Returns a checksum for all loaded decl text.
		virtual int				GetChecksum(void) const = 0;

		// Returns the number of decl types.
		virtual int				GetNumDeclTypes(void) const = 0;

		// Returns the type name for a decl type.
		virtual const char 	*GetDeclNameFromType(declType_t type) const = 0;

		// Returns the decl type for a type name.
		virtual declType_t		GetDeclTypeFromName(const char *typeName) const = 0;

		// If makeDefault is true, a default decl of appropriate type will be created
		// if an explicit one isn't found. If makeDefault is false, NULL will be returned
		// if the decl wasn't explcitly defined.
		virtual const idDecl 	*FindType(declType_t type, const char *name, bool makeDefault = true) = 0;

		virtual const idDecl	*FindDeclWithoutParsing(declType_t type, const char *name, bool makeDefault = true) = 0;

		virtual void			ReloadFile(const char *filename, bool force) = 0;

		// Returns the number of decls of the given type.
		virtual int				GetNumDecls(declType_t type) = 0;

		// The complete lists of decls can be walked to populate editor browsers.
		// If forceParse is set false, you can get the decl to check name / filename / etc.
		// without causing it to parse the source and load media.
		virtual const idDecl 	*DeclByIndex(declType_t type, int index, bool forceParse = true) = 0;

		// List and print decls.
		virtual void			ListType(const idCmdArgs &args, declType_t type) = 0;
		virtual void			PrintType(const idCmdArgs &args, declType_t type) = 0;
#ifdef _SPLASHDAMAGE
	    virtual void			ListType( const idCmdArgs &args, const char* typeName ) = 0;
	    virtual void			PrintType( const idCmdArgs &args, const char* typeName ) = 0;
#endif

		// Creates a new default decl of the given type with the given name in
		// the given file used by editors to create a new decls.
		virtual idDecl 		*CreateNewDecl(declType_t type, const char *name, const char *fileName) = 0;

		// BSM - Added for the material editors rename capabilities
		virtual bool			RenameDecl(declType_t type, const char *oldName, const char *newName) = 0;

		// When media files are loaded, a reference line can be printed at a
		// proper indentation if decl_show is set
		virtual void			MediaPrint(const char *fmt, ...) id_attribute((format(printf,2,3))) = 0;

		virtual void			WritePrecacheCommands(idFile *f) = 0;

		// Convenience functions for specific types.
		virtual	const idMaterial 		*FindMaterial(const char *name, bool makeDefault = true) = 0;
		virtual const idDeclSkin 		*FindSkin(const char *name, bool makeDefault = true) = 0;
		virtual const idSoundShader 	*FindSound(const char *name, bool makeDefault = true) = 0;
#ifdef _RAVEN
		virtual const idDeclTable *		FindTable( const char *name, bool makeDefault = true ) = 0;
// RAVEN BEGIN
// jscott: for new Raven decls
		virtual const rvDeclMatType *	FindMaterialType( const char *name, bool makeDefault = true ) = 0;
		virtual	const rvDeclLipSync *	FindLipSync( const char *name, bool makeDefault = true ) = 0;
		virtual	const rvDeclPlayback *	FindPlayback( const char *name, bool makeDefault = true ) = 0;
		virtual	const rvDeclEffect *	FindEffect( const char *name, bool makeDefault = true ) = 0;
// RAVEN END
// RAVEN BEGIN
// jscott: for new Raven decls
		virtual const rvDeclMatType *	MaterialTypeByIndex( int index, bool forceParse = true ) = 0;
		virtual const rvDeclLipSync *	LipSyncByIndex( int index, bool forceParse = true ) = 0;
		virtual	const rvDeclPlayback *	PlaybackByIndex( int index, bool forceParse = true ) = 0;
		virtual const rvDeclEffect *	EffectByIndex( int index, bool forceParse = true ) = 0;

		virtual bool					GetPlaybackData( const rvDeclPlayback *playback, int control, int now, int last, class rvDeclPlaybackData *pbd ) = 0;
		virtual bool					SetPlaybackData(rvDeclPlayback* playback, int now, int control, class rvDeclPlaybackData* pbd) = 0;
		virtual void					StartPlaybackRecord(rvDeclPlayback* playback) = 0;
		virtual bool					FinishPlayback( rvDeclPlayback *playback ) = 0;
// RAVEN END

							// If makeDefault is true, a default decl of appropriate type will be created
							// if an explicit one isn't found. If makeDefault is false, NULL will be returned
							// if the decl wasn't explcitly defined.
		virtual const idDecl *	FindType( declType_t type, const char *name, bool makeDefault, bool noCaching ) = 0;
		//karin: find map def
		virtual const idDeclEntityDef * FindMapDef(const char *mapName, const char *entityFilter = 0) const = 0;
		virtual idDeclEntityDef * FindMapDef(const char *mapName, const char *entityFilter = 0) = 0;
// RAVEN BEGIN
// jscott: for timing
							// Registers a new folder with decl files.
		virtual void			RegisterDeclFolderWrapper( const char *folder, const char *extension, declType_t defaultType, bool unique = false, bool norecurse = false ) = 0;
// RAVEN END
#endif

		virtual const idMaterial 		*MaterialByIndex(int index, bool forceParse = true) = 0;
		virtual const idDeclSkin 		*SkinByIndex(int index, bool forceParse = true) = 0;
		virtual const idSoundShader 	*SoundByIndex(int index, bool forceParse = true) = 0;

#ifdef _HUMANHEAD
        //HUMANHEAD: aob
        virtual const hhDeclBeam *		FindBeam( const char *name, bool makeDefault = true ) = 0; // HUMANHEAD CJR
        virtual const hhDeclBeam *		BeamByIndex( int index, bool forceParse = true ) = 0; // HUMANHEAD CJR

        virtual void					SetInsideLevelLoad(bool b) = 0;
        virtual bool					GetInsideLevelLoad(void) = 0;
#endif

#ifdef _SPLASHDAMAGE
    	virtual int						GetNumMaterials( void ) = 0;
    	virtual void					CacheFromDict( const idDict& dict ) = 0;
		virtual	const rvDeclEffect *	FindEffect( const char *name, bool makeDefault = true ) = 0;
	    virtual idDeclTypeInterface*	GetDeclType( const char* typeName ) const = 0;
	    virtual idDeclTypeInterface*	GetDeclType( qhandle_t typeHandle ) const = 0;
    	virtual qhandle_t				GetDeclTypeHandle( const char* typeName ) const = 0;
    	virtual const char*				GetDeclTypeName( qhandle_t typeHandle ) const = 0;
    	
	    virtual void					AddDependency( const idDecl* decl, const idDecl* dependency ) = 0;
	    virtual void					AddDependency( const idDecl* decl, const char* fileName ) = 0;
    	virtual void					AddDependencies( const idDecl* decl, const idParser& parser ) = 0;
#endif

        virtual const idDecl 	        *AddDeclDef(const char *defname, declType_t type, const idDict &args, bool force = false) = 0;
		virtual bool					EntityDefSet(const char *name, const char *key, const char *value = NULL) = 0;
};

extern idDeclManager 		*declManager;


template< declType_t type >
ID_INLINE void idListDecls_f(const idCmdArgs &args)
{
	declManager->ListType(args, type);
}

template< declType_t type >
ID_INLINE void idPrintDecls_f(const idCmdArgs &args)
{
	declManager->PrintType(args, type);
}


#ifdef _SPLASHDAMAGE
#include "decllib/declType.h"
#endif

#endif /* !__DECLMANAGER_H__ */
