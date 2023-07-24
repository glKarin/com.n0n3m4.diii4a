// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __COMPILEDSCRIPT_BASE_H__
#define __COMPILEDSCRIPT_BASE_H__

class idEventDef;

const int COMPILED_SCRIPT_INTERFACE_VERSION = 2;

class sdCSTypeInfo {
public:
	class sdCSTypeInfoList {
	public:
		sdCSTypeInfoList( void ) : list( NULL ) { ; }

		void Append( sdCSTypeInfo* item ) {
			item->SetNext( list );
			list = item;
		}

		int Num( void ) {
			int count = 0;
			sdCSTypeInfo* p = list;
			while ( p != NULL ) {
				count++;
				p = p->GetNext();
			}
			return count;
		}

		sdCSTypeInfo* FindRoot( void ) {
			sdCSTypeInfo* p = list;
			while ( p != NULL ) {
				if ( strlen( p->GetSuperName() ) == 0 ) {
					return p;
				}
				p = p->GetNext();
			}
			return NULL;
		}

		sdCSTypeInfo* FindChild( sdCSTypeInfo* root, sdCSTypeInfo* child ) {
			if ( child == NULL ) {
				child = list;
			} else {
				child = child->GetNext();
			}
			while ( child != NULL ) {
				if ( *root == child->GetSuperName() ) {					
					return child;
				}
				child = child->GetNext();
			}
			return NULL;
		}

	private:
		sdCSTypeInfo* list;
	};

	typedef sdCSTypeInfo* Ptr;
	typedef sdCSTypeInfoList typeList_t;

	sdCSTypeInfo( const char* name_, const char* superName_ ) : typeBegin( 0 ), typeEnd( 0 ), name( name_ ), superName( superName_ ), next( NULL ) {
		GetTypeList().Append( this );
	}
	~sdCSTypeInfo() {}

	bool			operator==( const char* rhs ) const {
		return strcmp( rhs, name ) == 0;
	}

	bool			IsType( const sdCSTypeInfo& rhs ) const {
		return typeBegin >= rhs.typeBegin && typeEnd <= rhs.typeEnd;
	}

	const char*		GetName() const { return name; }
	const char*		GetSuperName() const { return superName; }

	void			SetNext( sdCSTypeInfo* _next ) { next = _next; }
	sdCSTypeInfo*	GetNext( void ) { return next; }

	static void 	Init() {
		if ( Initialised( false, false ) ) {
			return;
		}

		sdCSTypeInfo* root = GetTypeList().FindRoot();
		if ( root != NULL ) {
			int id = 0;
			NumberTypes( root, id );
		}

		Initialised( true, true );
	}
	static void 	Shutdown() {
		Initialised( true, false );
	}

	static int		NumTypes() { return GetTypeList().Num(); }

private:
	const char*		name;
	const char*		superName;
	int				typeBegin;
	int				typeEnd;
	sdCSTypeInfo*	next;

private:
	static void 		NumberTypes( Ptr root, int& id ) {
		root->typeBegin = id++;

		sdCSTypeInfo* child = NULL;
		while ( child = GetTypeList().FindChild( root, child ) ) {
			NumberTypes( child, id );
		}

		root->typeEnd = id - 1;
	}

	static bool Initialised( bool set, bool value ) {
		static bool initialized = false;
		if ( set ) {
			initialized = value;
			return value;
		}
		return initialized;
	}

	static typeList_t&	GetTypeList() {
		static typeList_t types;
		return types;
	}
};

#define SD_CS_DECLARE_TYPE_INFO																												\
	static sdCSTypeInfo Type;																										\
	virtual bool IsType( const sdCSTypeInfo& rhs ) const	{ return Type.IsType( rhs ); }												\
	virtual const sdCSTypeInfo& GetType() const			{ return Type; }															\

#define SD_CS_DECLARE_CLASS( ClassName )																										\
public:																																		\
	SD_CS_DECLARE_TYPE_INFO

#define SD_CS_IMPLEMENT_TYPE_INFO( ClassName, SuperClassName )																					\
	sdCSTypeInfo ClassName::Type( #ClassName, #SuperClassName );

#define SD_CS_IMPLEMENT_CLASS( ClassName, SuperClassName )																						\
	SD_CS_IMPLEMENT_TYPE_INFO( ClassName, SuperClassName )


class sdCompiledScript_ClassBase {
public:
	SD_CS_DECLARE_CLASS( sdCompiledScript_ClassBase );

										sdCompiledScript_ClassBase( void ) { __s_handle = 0; }
	virtual								~sdCompiledScript_ClassBase( void ) { ; }

	int									__S_GetHandle( void ) const { return __s_handle; }
	void								__S_SetHandle( int handle ) { __s_handle = handle; }

	template< typename T >
	T* Cast( void ) {
		if ( this == NULL ) {
			return NULL;
		}
		if ( IsType( T::Type ) ) {
			return reinterpret_cast< T* >( this );
		}
		return NULL;
	}

private:
	int									__s_handle;
};

typedef sdCompiledScript_ClassBase* ( *classAllocator_t )( void );

typedef void ( sdCompiledScript_ClassBase::*classFunctionCallback_t )( void );
typedef void ( *classFunctionWrapper_t )( sdCompiledScript_ClassBase* obj, classFunctionCallback_t callback, const byte* data );

typedef void ( *functionCallback_t )( void );
typedef void ( *functionWrapper_t )( functionCallback_t callback, const byte* data );

typedef byte* ( sdCompiledScript_ClassBase::*variableLookup_t )( void );

struct sdClassFunctionInfo {
	const char*				name;
	classFunctionWrapper_t	wrapper;
	classFunctionCallback_t	function;
	int						numParms;
	int						parmSizeTotal;
};

enum variableType_t {
	V_FLOAT,
	V_BOOLEAN,
	V_VECTOR,
	V_OBJECT,
	V_STRING,
	V_WSTRING,
	V_NONE,
};

struct sdClassVariableInfo {
	const char*				name;
	variableType_t			type;
	variableLookup_t		variable;
};

struct sdFunctionInfo {
	const char*				name;
	functionWrapper_t		wrapper;
	functionCallback_t		function;

	sdFunctionInfo*			next;
};

struct sdClassInfo {
	const char*				name;
	sdClassFunctionInfo*	functionInfo;
	sdClassVariableInfo*	variableInfo;
	sdClassInfo*			superClass;
	classAllocator_t		allocator;

	sdClassInfo*			next;
};

class sdProcedureCall {
public:
	virtual void		Go( void ) = 0;
	virtual ~sdProcedureCall() { ; }
};

class idScriptObject;
class idEntity;

class sdCompilerInterface {
public:
	virtual const idEventDef*			FindEvent( const char* name ) = 0;
	virtual int							AllocThread( sdProcedureCall* call ) = 0;
	virtual int							AllocThread( sdCompiledScript_ClassBase* object, const char* name, sdProcedureCall* call ) = 0;
	virtual int							AllocGuiThread( sdProcedureCall* call ) = 0;
	virtual int							AllocGuiThread( sdCompiledScript_ClassBase* object, const char* name, sdProcedureCall* call ) = 0;
	virtual sdCompiledScript_ClassBase*	AllocObject( const char* name ) = 0;
	virtual void						FreeObject( sdCompiledScript_ClassBase* object ) = 0;
	virtual sdCompiledScript_ClassBase*	GetObject( int handle ) = 0;

	virtual void						SysCall( const idEventDef* event, const UINT_PTR* data ) = 0;
	virtual void						EventCall( const idEventDef* event, sdCompiledScript_ClassBase* obj, const UINT_PTR* data ) = 0;

	virtual idScriptObject*				GetScriptObject( int handle ) = 0;
	virtual idEntity*					GetEntity( int handle ) = 0;

	virtual void						Wait( float time ) = 0;
	virtual void						WaitFrame( void ) = 0;

	virtual float						GetReturnedFloat( void ) = 0;
	virtual bool						GetReturnedBoolean( void ) = 0;
	virtual sdCompiledScript_ClassBase*	GetReturnedObject( void ) = 0;
	virtual const char*					GetReturnedString( void ) = 0;
	virtual float*						GetReturnedVector( void ) = 0;
	virtual int							GetReturnedInteger( void ) = 0;
	virtual const wchar_t*				GetReturnedWString( void ) = 0;

	virtual void						ReturnWString( const wchar_t* value ) = 0;
	virtual void						ReturnString( const char* value ) = 0;
	virtual void						ReturnVector( float* value ) = 0;
	virtual void						ReturnFloat( float value ) = 0;
	virtual void						ReturnBoolean( bool value ) = 0;
	virtual void						ReturnObject( sdCompiledScript_ClassBase* obj ) = 0;
};

class sdScriptInterface {
public:
	virtual sdClassInfo*			GetClassInfo( void ) = 0;
	virtual sdFunctionInfo*			GetFunctionInfo( void ) = 0;

	virtual int						GetClassFunctionInfoSize( void ) = 0;
	virtual int						GetFunctionInfoSize( void ) = 0;

	virtual int						GetVersion( void ) = 0;
};

typedef sdScriptInterface* ( *scriptInitFunc_t )( sdCompilerInterface* iface );

extern sdCompilerInterface* compilerInterface;

#endif // __COMPILEDSCRIPT_BASE_H__
