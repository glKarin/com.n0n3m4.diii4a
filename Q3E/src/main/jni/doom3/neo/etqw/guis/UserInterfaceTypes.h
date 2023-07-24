// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACETYPES_H__
#define __GAME_GUIS_USERINTERFACETYPES_H__

#include "../../game/decls/DeclGUI.h"
#include "../../decllib/declTable.h"
#include "../../decllib/declTypeHolder.h"

#define SD_UI_PROPERTY_TAG( InfoText )

#define	SD_UI_PUSH_CLASS_TAG( Class )
#define	SD_UI_POP_CLASS_TAG
#define SD_UI_PUSH_GROUP_TAG( Group )
#define SD_UI_POP_GROUP_TAG

#define SD_UI_EVENT_TAG( EventName, Input, Description )			EventName

#define SD_UI_EVENT_PARM_TAG( EventName, Input, Description )		EventName
#define SD_UI_EVENT_RETURN_PARM( Type, Description )
#define SD_UI_EVENT_PARM( Type, ParmName, Description )
#define SD_UI_END_EVENT_TAG

#define SD_UI_FUNC_TAG( FuncName, Description )
#define SD_UI_FUNC_RETURN_PARM( Type, Description )
#define SD_UI_FUNC_PARM( Type, ParmName, Description )
#define SD_UI_END_FUNC_TAG

#define SD_UI_ENUM_TAG( Identifier, Description )

#define SD_UI_CLASS_INFO_TAG( Text )
#define SD_UI_CLASS_EXAMPLE_TAG( Text )

#define UI_ADD_STR_CALLBACK( Property, Class, Func ) Property.AddOnChangeHandler( sdFunctions::sdBindMem0< void, const idStr&, const idStr&, sdFunctions::sdEmptyType >( &Class::Func, this ) );
#define UI_ADD_WSTR_CALLBACK( Property, Class, Func ) Property.AddOnChangeHandler( sdFunctions::sdBindMem0< void, const idWStr&, const idWStr&, sdFunctions::sdEmptyType >( &Class::Func, this ) );
#define UI_ADD_INT_CALLBACK( Property, Class, Func ) Property.AddOnChangeHandler( sdFunctions::sdBindMem0< void, const int&, const int&, sdFunctions::sdEmptyType >( &Class::Func, this ) );
#define UI_ADD_VEC2_CALLBACK( Property, Class, Func ) Property.AddOnChangeHandler( sdFunctions::sdBindMem0< void, const idVec2&, const idVec2&, sdFunctions::sdEmptyType >( &Class::Func, this ) );
#define UI_ADD_VEC3_CALLBACK( Property, Class, Func ) Property.AddOnChangeHandler( sdFunctions::sdBindMem0< void, const idVec3&, const idVec3&, sdFunctions::sdEmptyType >( &Class::Func, this ) );
#define UI_ADD_VEC4_CALLBACK( Property, Class, Func ) Property.AddOnChangeHandler( sdFunctions::sdBindMem0< void, const idVec4&, const idVec4&, sdFunctions::sdEmptyType >( &Class::Func, this ) );
#define UI_ADD_FLOAT_CALLBACK( Property, Class, Func ) Property.AddOnChangeHandler( sdFunctions::sdBindMem0< void, const float, const float, sdFunctions::sdEmptyType >( &Class::Func, this ) );


#define UI_ADD_STR_VALIDATOR( Property, Class, Func ) Property.AddValidator( sdFunctions::sdBindMem0< bool, const idStr&, sdFunctions::sdEmptyType, sdFunctions::sdEmptyType >( &Class::Func, this ) );
#define UI_ADD_WSTR_VALIDATOR( Property, Class, Func ) Property.AddValidator( sdFunctions::sdBindMem0< bool, const idWStr&, sdFunctions::sdEmptyType, sdFunctions::sdEmptyType >( &Class::Func, this ) );
#define UI_ADD_INT_VALIDATOR( Property, Class, Func ) Property.AddValidator( sdFunctions::sdBindMem0< bool, const int, sdFunctions::sdEmptyType, sdFunctions::sdEmptyType >( &Class::Func, this ) );
#define UI_ADD_VEC2_VALIDATOR( Property, Class, Func ) Property.AddValidator( sdFunctions::sdBindMem0< bool, const idVec2&, sdFunctions::sdEmptyType, sdFunctions::sdEmptyType >( &Class::Func, this ) );
#define UI_ADD_VEC3_VALIDATOR( Property, Class, Func ) Property.AddValidator( sdFunctions::sdBindMem0< bool, const idVec3&, sdFunctions::sdEmptyType, sdFunctions::sdEmptyType >( &Class::Func, this ) );
#define UI_ADD_VEC4_VALIDATOR( Property, Class, Func ) Property.AddValidator( sdFunctions::sdBindMem0< bool, const idVec4&, sdFunctions::sdEmptyType, sdFunctions::sdEmptyType >( &Class::Func, this ) );
#define UI_ADD_FLOAT_VALIDATOR( Property, Class, Func ) Property.AddValidator( sdFunctions::sdBindMem0< bool, const float, sdFunctions::sdEmptyType, sdFunctions::sdEmptyType >( &Class::Func, this ) );

typedef sdHandle< unsigned int, 0 > guiHandle_t;

typedef void (*uiRenderCallback_t)( class sdUserInterfaceLocal*, float, float, float, float );
typedef enum uiRenderCallbackType_t { UIRC_PRE, UIRC_POST, UIRC_MAX };

typedef bool (*uiInputHandler_t)( class sdUIWindow*, const sdSysEvent* );
typedef void (*uiListEnumerationCallback_t)( class sdUIList* );
typedef void (*uiIconEnumerationCallback_t)( class sdUIIconNotification* );
typedef void (*uiRadialMenuEnumerationCallback_t)( class sdUIRadialMenu* );

typedef sdHashMapGeneric< idStr, idListGranularityOne< int >, sdHashCompareStrIcmp > uiTimelineHash_t;

/*
============
sdUIEventInfo
============
*/
using namespace sdUtility;
typedef sdHandle< int, -1 > sdUIEventHandle;
class sdUIEventInfo {
public:
	sdUIEventInfo( void ) : parameter( -1 ) { ; }
	sdUIEventInfo( sdUIEventHandle h, int parm ) : eventType( h ), parameter( parm ) { ; }
	
	bool operator==( const sdUIEventInfo& rhs ) const {
		return ( ( eventType == rhs.eventType ) && ( parameter == rhs.parameter ) );
	}
	bool operator!=( const sdUIEventInfo& rhs ) const {
		return !( *this == rhs );
	}

public:
	sdUIEventHandle				eventType;		// event class ( mouse down, key event, etc )
	int							parameter;
};


/*
============
sdUIEventTable
============
*/
class sdUIEventTable {
public:
							sdUIEventTable();

	void					SetNumEvents( int num ) { events.SetNum( num ); }

	bool					Empty() const { return events.Empty(); }

	sdUIEventHandle			GetEvent( const sdUIEventInfo& info ) const;
	void					AddEvent( const sdUIEventInfo& info, sdUIEventHandle scriptHandle );

	void					Clear();

	int						Num( const sdUIEventHandle type  ) const { return events[ type ].events.Num(); }

private:
	struct eventList_t {
		idList< sdUIEventHandle >	events;
	};
	idList< eventList_t >			events;
};

/*
============
sdUserInterfaceScope
============
*/
class sdDeclGUIWindow;
class sdFloatParmExpression;
class sdUIExpression;
class sdUIEvaluatorTypeBase;
class sdUserInterfaceScope;

/*
============
sdPropertyBinder
============
*/
class sdPropertyBinder {
public:
	int									IndexForProperty( sdProperties::sdProperty* property );
	void								SetPropertyExpression( int propertyKey, int propertyIndex, sdUIExpression* expression, sdUserInterfaceScope* scope );
	void								Clear( void );
	void								ClearPropertyExpression( int propertyKey, int propertyIndex );
	sdProperties::sdProperty*			PropertyForKey( int propertyKey ) { return indexedProperties[ propertyKey ].first; }

private:
	typedef sdPair< sdProperties::sdProperty*, int > boundProperty_t;
	idList< boundProperty_t >			indexedProperties;
	idList< sdUIExpression* >			propertyExpressions;
};

extern const char sdUIFunctionStack_Identifier[];

/*
============
sdUIFunctionStack
============
*/
class sdUIFunctionStack {
public:
	typedef idBlockAlloc< sdUIFunctionStack, 32 > allocator_t;

	static allocator_t	allocator;

	void								Push( float value )	{ floatStack.Append( value ); }
	void								Push( const idVec2& value ) { floatStack.Append( value[ 1 ] ); floatStack.Append( value[ 0 ] ); }
	void								Push( const idVec3& value ) { floatStack.Append( value[ 2 ] ); floatStack.Append( value[ 1 ] ); floatStack.Append( value[ 0 ] ); }
	void								Push( const idVec4& value ) { floatStack.Append( value[ 3 ] ); floatStack.Append( value[ 2 ] ); floatStack.Append( value[ 1 ] ); floatStack.Append( value[ 0 ] ); }
	void								Push( const char* value )	{ stringStack.Append( value ); }
	void								Push( const wchar_t* value ){ wstringStack.Append( value ); }
	void								Push( int value )			{ float temp = static_cast< float >( value ); Push( temp ); }
	void								Push( bool value )			{ float temp = value ? 1.0f : 0.0f; Push( temp ); }

	void								Pop( float& value );
	void								Pop( idVec2& value );
	void								Pop( idVec3& value );
	void								Pop( idVec4& value );
	void								Pop( idStr& value );
	void								Pop( idWStr& value );
	void								Pop( int& value );
	void								Pop( bool& value );

	void								Top( float& value ) const;
	void								Top( idVec2& value ) const;
	void								Top( idVec3& value ) const;
	void								Top( idVec4& value ) const;
	void								Top( idStr& value ) const;
	void								Top( idWStr& value ) const;
	void								Top( int& value ) const;
	void								Top( bool& value ) const;

	void								Clear( void ) { floatStack.SetNum( 0 ); stringStack.SetNum( 0 ); wstringStack.SetNum( 0 ); }

	bool								Empty( void ) const { return floatStack.Empty() && stringStack.Empty() && wstringStack.Empty(); }

	void								SetID( const char* id ) { identifier = id; }
	const char*							GetID() const { return identifier.c_str(); }

private:
	idStaticList< float, 32 >			floatStack;
	idStaticList< idStr, 6 >			stringStack;
	idStaticList< idWStr, 6 >			wstringStack;
	idStr								identifier;
};


/*
============
sdUIFunction
============
*/
class sdUIFunction {
public:
	virtual int							GetNumParms( void ) const = 0;
	virtual sdProperties::ePropertyType	GetParm( int index ) const = 0;
	virtual sdProperties::ePropertyType	GetReturnType( void ) const = 0;
};


/*
============
sdUIFunctionInstance
============
*/
class sdUIFunctionInstance {
public:
	virtual								~sdUIFunctionInstance( void ) { ; }

	virtual void						Run( sdUIFunctionStack& stack ) = 0;
	virtual const sdUIFunction*			GetFunctionInfo( void ) const = 0;
	virtual const char*					GetName( void ) const = 0;
};


/*
============
sdUserInterfaceScope
============
*/
class sdUserInterfaceScope {
public:
												sdUserInterfaceScope() {}
	virtual										~sdUserInterfaceScope() {}

	virtual sdProperties::sdProperty*			GetProperty( const char* name, sdProperties::ePropertyType type ) = 0;
	virtual sdProperties::sdProperty*			GetProperty( const char* name ) = 0;
	virtual sdUIFunctionInstance*				GetFunction( const char* name ) = 0;
	virtual sdUIEvaluatorTypeBase*				GetEvaluator( const char* name ) = 0;
	virtual sdProperties::sdPropertyHandler&	GetProperties() = 0;

	virtual bool								IsReadOnly() const = 0;

	virtual int									IndexForProperty( sdProperties::sdProperty*	property ) = 0;
	virtual void								SetPropertyExpression( int propertyKey, int propertyIndex, sdUIExpression* expression ) = 0;
	virtual void								ClearPropertyExpression( int propertyKey, int propertyIndex ) = 0;
	virtual void								RunFunction( int expressionIndex ) = 0;

	virtual const char*							FindPropertyNameByKey( int propertyKey, sdUserInterfaceScope*& scope ) = 0;
	virtual const char*							FindPropertyName( sdProperties::sdProperty* property, sdUserInterfaceScope*& scope ) = 0;

	virtual sdUIEventHandle						GetEvent( const sdUIEventInfo& info ) const = 0;
	virtual void								AddEvent( const sdUIEventInfo& info, sdUIEventHandle scriptHandle ) = 0;

	virtual int									AddExpression( sdUIExpression* expression ) = 0;
	virtual sdUIExpression*						GetExpression( int index ) = 0;

	virtual sdUserInterfaceLocal*				GetUI() = 0;

	virtual const char*							GetName() const = 0;

	virtual sdUserInterfaceScope*				GetSubScope( const char* name ) = 0;
	virtual int									NumSubScopes() const = 0;
	virtual const char*							GetSubScopeNameForIndex( int index ) = 0;
	virtual sdUserInterfaceScope*				GetSubScopeForIndex( int index ) = 0;

	virtual const char*							GetScopeClassName() const = 0;

private:
												sdUserInterfaceScope( const sdUserInterfaceScope& );
	sdUserInterfaceScope&						operator=( const sdUserInterfaceScope& );
};


/*
============
sdUIPropertyHolder
Only provides read-only properties
============
*/
class sdUIPropertyHolder : public sdUserInterfaceScope {
public:
	virtual								~sdUIPropertyHolder() {}

	virtual sdUIFunctionInstance*		GetFunction( const char* name ) { assert( false ); return NULL; }

	virtual bool						IsReadOnly() const { return true; }
	virtual int							NumSubScopes() const { return 0; }
	virtual const char*					GetSubScopeNameForIndex( int index ) { assert( 0 ); return ""; }
	virtual sdUserInterfaceScope*		GetSubScopeForIndex( int index ) { assert( 0 ); return NULL; }

	virtual int							IndexForProperty( sdProperties::sdProperty*	property ) { assert( false ); return -1; }
	virtual void						SetPropertyExpression( int propertyKey, int propertyIndex, sdUIExpression* expression ) { assert( false ); }
	virtual void						ClearPropertyExpression( int propertyKey, int propertyIndex ) { assert( false ); }
	virtual void						RunFunction( int expressionIndex ) { assert( false ); }

	virtual const char*					FindPropertyNameByKey( int propertyKey, sdUserInterfaceScope*& scope ) { return NULL; }

	virtual sdUIEventHandle				GetEvent( const sdUIEventInfo& info ) const { return sdUIEventHandle(); }
	virtual void						AddEvent( const sdUIEventInfo& info, sdUIEventHandle scriptHandle ) { assert( false ); }

	virtual int							AddExpression( sdUIExpression* expression ) { assert( false ); return -1; }
	virtual sdUIExpression*				GetExpression( int index ) { assert( false ); return NULL; }
	virtual sdUIEvaluatorTypeBase*		GetEvaluator( const char* name ) { assert( false ); return NULL; }

	virtual sdUserInterfaceLocal*		GetUI() { assert( false ); return NULL; }

	virtual sdUserInterfaceScope*		GetSubScope( const char* name ) { return NULL; }

	virtual const char*					GetScopeClassName() const { return "propertyHolder"; }
};

/*
============
sdUIWritablePropertyHolder
============
*/
class sdUIWritablePropertyHolder : public sdUIPropertyHolder {
public:
	virtual								~sdUIWritablePropertyHolder() {}

	virtual bool						IsReadOnly() const { return false; }
	virtual int							IndexForProperty( sdProperties::sdProperty*	property ) { return boundProperties.IndexForProperty( property ); }
	virtual void						SetPropertyExpression( int propertyKey, int propertyIndex, sdUIExpression* expression ) { boundProperties.SetPropertyExpression( propertyKey, propertyIndex, expression, this ); }
	virtual void						ClearPropertyExpression( int propertyKey, int propertyIndex ) { boundProperties.ClearPropertyExpression( propertyKey, propertyIndex ); }

	virtual const char*					FindPropertyNameByKey( int propertyKey, sdUserInterfaceScope*& scope ) { return FindPropertyName( boundProperties.PropertyForKey( propertyKey ), scope ); }

private:
	sdPropertyBinder					boundProperties;
};


class sdUserInterfaceLocal;
class sdDeclGUIWindow;

/*
============
sdUITemplateFunction
============
*/
template< typename T >
class sdUITemplateFunction :
	public sdUIFunction {
public:
	typedef void ( T::*windowFunction_t )( sdUIFunctionStack& );

	sdUITemplateFunction( char _returnType, const char* parms, windowFunction_t _function ) : function( _function ) {
		int len = idStr::Length( parms );
		int i;
		for ( i = 0; i < len; i++ ) {
			switch ( parms[ i ] ) {
				case 'i': parmTypes.Append( sdProperties::PT_INT ); break;
				case 'f': parmTypes.Append( sdProperties::PT_FLOAT ); break;
				case '2': parmTypes.Append( sdProperties::PT_VEC2 ); break;
				case '3': parmTypes.Append( sdProperties::PT_VEC3 ); break;
				case '4': parmTypes.Append( sdProperties::PT_VEC4 ); break;
				case 's': parmTypes.Append( sdProperties::PT_STRING ); break;
				case 'w': parmTypes.Append( sdProperties::PT_WSTRING ); break;
				default: gameError( "sdUITemplateFunction::sdUITemplateFunction Invalid Parameter Type '%c'", parms[ i ] ); break;
			}
		}

		switch ( _returnType ) {
			case 'i': returnType = sdProperties::PT_INT; break;
			case 'f': returnType = sdProperties::PT_FLOAT; break;
			case '2': returnType = sdProperties::PT_VEC2; break;
			case '3': returnType = sdProperties::PT_VEC3; break;
			case '4': returnType = sdProperties::PT_VEC4; break;
			case 's': returnType = sdProperties::PT_STRING; break;
			case 'w': returnType = sdProperties::PT_WSTRING; break;
			case 'v': returnType = sdProperties::PT_INVALID; break;
			default: gameError( "sdUITemplateFunction::sdUITemplateFunction Invalid Return Type '%c'", _returnType ); break;
		}
	}

	virtual int									GetNumParms( void ) const { return parmTypes.Num(); }
	virtual sdProperties::ePropertyType			GetParm( int index ) const { return parmTypes[ index ]; }
	virtual sdProperties::ePropertyType			GetReturnType( void ) const { return returnType; }
	windowFunction_t							GetFunction( void ) const { return function; }

private:
	idList< sdProperties::ePropertyType >		parmTypes;
	sdProperties::ePropertyType					returnType;
	windowFunction_t							function;
};

/*
============
sdUITemplateFunctionInstance
============
*/
template< typename T, const char allocatorName[] >
class sdUITemplateFunctionInstance :
	public sdUIFunctionInstance,
	public sdPoolAllocator< sdUITemplateFunctionInstance< T, allocatorName >, allocatorName, 4096 > {
public:		

	typedef void ( T::*windowFunction_t )( sdUIFunctionStack& );

												sdUITemplateFunctionInstance( T* _window, const sdUITemplateFunction< T >* _function ) : window( _window ), function( _function ) { ; }
												
												// jrad - provide for implicit conversions that make sense
												template< class U >
												sdUITemplateFunctionInstance( U* _window, const sdUITemplateFunction< U >* _function ) : window( _window ), function( _function ) { ; }

	virtual void								Run( sdUIFunctionStack& stack ) {
													windowFunction_t func = function->GetFunction();
													( window->*func )( stack );
												}

	virtual const sdUIFunction*					GetFunctionInfo( void ) const { return function; }

	virtual const char*							GetName( void ) const { return window->GetName(); }

	T*											window;
	const sdUITemplateFunction< T >*			function;
};

/*
============
sdTransitionEvaluator
used for handling transitioning properties of dynamic items, since they can't have UIExpressions (list items, notify icons, etc)
============
*/
template < class T >
class sdTransitionEvaluator {
private:
	enum eMode { TABLE, LERP, ACCEL_DECEL, CONSTANT };
public:
	sdTransitionEvaluator() :
		startTime( 0 ),
		targetTime( 0 ),
		table( NULL ),
		mode( LERP ),
		initialized( false ) {
	}

	void	InitConstant( const T value ) {
		mode = CONSTANT;
		startTime = 0;
		targetTime = 0;
		startValue = value;
		targetValue = value;
		initialized = true;
	}
	void	InitLerp( const int startTime_, const int targetTime_, const T startValue_, const T targetValue_ ) {
		mode = LERP;
		SetParms( startTime_, targetTime_, startValue_, targetValue_ );
	}

	void	SetParms( const int startTime_, const int targetTime_, const T startValue_, const T targetValue_ ) {
		startTime = startTime_;
		targetTime = targetTime_;
		startValue = startValue_;
		targetValue = targetValue_;
		initialized = true;
	}
	void	InitTableEvaluation( const char* table ) {
		this->table = declHolder.FindTable( table );
		mode = TABLE;
	}
	void	InitAccelDecelEvaluation( const float accelParm, const float decelParm ) {
		float duration = targetTime - startTime;	
		interpolator.Init( startTime, duration * accelParm, duration * decelParm, duration, startValue, targetValue );
		table = NULL;
		mode = ACCEL_DECEL;
	}

	T		Evaluate( const int time ) const {
		if( mode == CONSTANT ) {
			return startValue;
		}
		if( targetTime == startTime ) {
			// avoid divide-by-zero later
			return startValue;
		}
		if( mode == ACCEL_DECEL ) {
			bool isDone = interpolator.IsDone( time );
			if( isDone ) {
				return interpolator.GetEndValue();
			}
			return interpolator.GetCurrentValue( time );
		}

		float percent = static_cast< float> ( ( time - startTime ) / static_cast< float >( targetTime - startTime ) );				
		percent = idMath::ClampFloat( 0.0f, 1.0f, percent );

		if ( percent >= 1.0f ) {
			if ( mode == TABLE && table != NULL ) {
				return Lerp( startValue, targetValue, table->TableLookup( 1.0f ) );
			} else {
				return targetValue;
			}
		}
		if( mode == TABLE && table != NULL ) {
			percent = table->TableLookup( percent );
		}				
		return Lerp( startValue, targetValue, percent );			
	}
	bool	IsDone( const int time ) const  {
		if( targetTime == startTime ) {
			return true;
		}
		if( mode == ACCEL_DECEL ) {
			return interpolator.IsDone( time );
		}

		float percent = static_cast< float> ( ( time - startTime ) / static_cast< float >( targetTime - startTime ) );				
		percent = idMath::ClampFloat( 0.0f, 1.0f, percent );
		return ( idMath::Fabs( 1.0f - percent ) < idMath::FLT_EPSILON );
	}

	sdTransitionEvaluator< T >& operator=( const sdTransitionEvaluator< T > &rhs ) {
		startTime = rhs.startTime;
		targetTime = rhs.targetTime;
		startValue = rhs.startValue;
		targetValue = rhs.targetValue;
		table = rhs.table;
		interpolator = rhs.interpolator;
		mode = rhs.mode;
		initialized = rhs.initialized;

		return *this;
	}

	bool IsInitialized() const { return initialized; }

private:
	int									startTime;
	int									targetTime;
	T									startValue;
	T									targetValue;
	const idDeclTable*					table;
	idInterpolateAccelDecelLinear< T >	interpolator;
	eMode								mode;
	bool								initialized;
};

template< class T >
bool sdFloatToContinuousEnum( float value, T minValue, T maxValue, T& out ) {
	int iValue = idMath::Ftoi( value );
	if( iValue <= minValue || iValue >= maxValue ) {
		//gameLocal.Warning( "Invalid enumeration value '%i'.", iValue );
		return false;
	}
	out = static_cast< T >( iValue );
	return true;
}

template< class T >
bool sdIntToContinuousEnum( int iValue, T minValue, T maxValue, T& out ) {
	if( iValue <= minValue || iValue >= maxValue ) {
		//gameLocal.Warning( "Invalid enumeration value '%i'.", iValue );
		return false;
	}
	out = static_cast< T >( iValue );
	return true;
}

template< class T >
class sdUICVarCallback :
	public idCVarCallback {
public:
	sdUICVarCallback( T& object_, idCVar& cvar_, int id_ ) : object( object_ ), cvar( cvar_ ), id( id_ ) {
		cvar.RegisterCallback( this );
	}
	virtual ~sdUICVarCallback() {
		cvar.UnRegisterCallback( this );
	}

	virtual void OnChanged() {
		object.OnCVarChanged( cvar, id );
	}
private:
	T&		object;
	idCVar& cvar;
	int		id;
};

struct uiMaterialInfo_t {
	uiMaterialInfo_t() : material( NULL ), st0( vec2_zero ), st1( vec2_zero ) { Clear(); }

	void Clear() {
		material = NULL;
		flags.flipX = false;
		flags.flipY = false;
		flags.lookupST = false;
		st0.Zero();
		st1.Zero();
	}

	const idMaterial*	material;
	idVec2				st0;
	idVec2				st1;

	struct flags_t {
		bool		flipX : 1;
		bool		flipY : 1;
		bool		lookupST : 1;
	} flags;
};

struct uiDrawPart_t {
	uiDrawPart_t() : width( 0 ), height( 0 ) {}

	uiMaterialInfo_t	mi;
	int					width;
	int					height;
};

// make sure to increase the number of bits for backgroundDrawMode in windowState_t 
enum uiDrawMode_e {
	BDM_SINGLE_MATERIAL,
	BDM_FRAME,
	BDM_TRI_PART_H,
	BDM_TRI_PART_V,
	BDM_FIVE_PART_H,
	BDM_USE_ST,
	BDM_MAX
};

enum uiFramePart_e { FP_TOPLEFT, FP_TOP, FP_TOPRIGHT, FP_LEFT, FP_RIGHT, FP_BOTTOMLEFT, FP_BOTTOM, FP_BOTTOMRIGHT, FP_CENTER, FP_MAX };
typedef idStaticList< uiDrawPart_t, FP_MAX >	uiDrawPartList_t;

struct uiCachedMaterial_t {
	uiDrawMode_e		drawMode;
	uiDrawPartList_t	parts;
	uiMaterialInfo_t	material;

};

typedef sdHashMapGeneric< idStr, uiCachedMaterial_t*, sdHashCompareStrIcmp, sdHashGeneratorIHash >	uiMaterialCache_t;

#endif // __GAME_GUIS_USERINTERFACETYPES_H__
