// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACEEXPRESSIONS_H__
#define __GAME_GUIS_USERINTERFACEEXPRESSIONS_H__

#include "UserInterfaceTypes.h"

class sdFloatParmExpression;


class sdUIExpression {
protected:
	virtual								~sdUIExpression( void ) { ; }

public:
	virtual void						Free( void ) = 0;
	virtual void						InputChanged( void ) = 0;
	virtual void						SetProperty( sdProperties::sdProperty* output, int index, int key, sdUserInterfaceScope* outputScope ) { assert( false ); }
	virtual void						Attach( sdUIExpression* output ) = 0;
	virtual void						Detach( void ) = 0;
	virtual sdProperties::ePropertyType GetType( void ) const = 0;
	virtual bool						UpdateValue( void ) { return false; } // for transitions

	virtual void						Evaluate( void ) { assert( false ); }

	virtual int							GetIntValue( void ) { assert( false ); return 0; }
	virtual float						GetFloatValue( void ) { assert( false ); return 0.f; }
	virtual const idStr	&				GetStringValue( idStr &temp ) { static idStr empty = ""; assert( false ); return empty; }
	virtual const idWStr &				GetWStringValue( idWStr &temp ) { static idWStr empty = L""; assert( false ); return empty; }
	virtual const idVec2 &				GetVec2Value( idVec2 &temp ) { assert( false ); return vec2_zero; }
	virtual const idVec3 &				GetVec3Value( idVec3 &temp ) { assert( false ); return vec3_zero; }
	virtual const idVec4 &				GetVec4Value( idVec4 &temp ) { assert( false ); return vec4_zero; }

	virtual void						OnSnapshotHitch( int delta ) {}
	virtual void						OnOnChangedOverflow( void ) = 0;

	static sdUIExpression*				AllocIntExpression( sdUserInterfaceScope* scope, const char* text );
	static sdUIExpression*				AllocFloatExpression( sdUserInterfaceScope* scope, const char* text );
	static sdUIExpression*				AllocStringExpression( sdUserInterfaceScope* scope, const char* text );
	static sdUIExpression*				AllocWStringExpression( sdUserInterfaceScope* scope, const char* text );
	static sdUIExpression*				AllocSingleParmExpression( sdProperties::ePropertyType type, sdUserInterfaceScope* scope, const char* text );
	static sdUIExpression*				AllocFunctionExpression( const char* functionName, sdUIFunctionInstance* function, sdUserInterfaceScope* scope, idLexer* src );
	static sdUIExpression*				AllocTransition( sdProperties::ePropertyType type, sdUserInterfaceScope* scope, idLexer* src );

	static void							IncActiveOnChangeHandlers( sdUIExpression* expression );
	static void							DecActiveOnChangeHandlers( void );

private:
	static const int					MAX_ACTIVE_ONCHANGED_HANDLERS = 256;
	static idStaticList< sdUIExpression*, MAX_ACTIVE_ONCHANGED_HANDLERS > activeOnChangedHandlers;
};

template< typename T, sdProperties::ePropertyType TYPE, int COUNT >
class sdUITransition : public sdUIExpression {
public:
										sdUITransition( sdUserInterfaceScope* scope, idStrList& list );
	virtual								~sdUITransition( void );

	virtual void						OnOnChangedOverflow( void ) { ; }

	virtual void						InputChanged( void ) { assert( false ); }
	virtual bool						UpdateValue( void );
	virtual void						Detach( void );
	virtual sdProperties::ePropertyType GetType( void ) const { return TYPE; }
	virtual void						OnSnapshotHitch( int delta );

protected:
	const idDeclTable*					table;
	T									start;
	T									end;
	T									value;
	idVec2								accelTimes;
	
	idInterpolateAccelDecelLinear< T >*	interpolator;


	sdUIExpression*						startExpressions[ COUNT ];
	sdUIExpression*						endExpressions[ COUNT ];

	int									startTime;
	sdUIExpression*						duration;
	sdUIExpression*						outputExpression;

	float								_cachedDuration;

	sdUserInterfaceLocal*				ui;

	static idBlockAlloc< idInterpolateAccelDecelLinear< T >, 32 > interpolatorAllocator;
};

template< typename T, sdProperties::ePropertyType TYPE, int COUNT > idBlockAlloc< idInterpolateAccelDecelLinear< T >, 32 > sdUITransition< T, TYPE, COUNT >::interpolatorAllocator;

extern const char sdUITransitionFloat_Identifier[];
class sdUITransitionFloat :
	public sdUITransition< float, sdProperties::PT_FLOAT, 1 >,
	public sdPoolAllocator< sdUITransitionFloat, sdUITransitionFloat_Identifier, 1024 > {
public:
	typedef sdPoolAllocator< sdUITransitionFloat, sdUITransitionFloat_Identifier, 1024 > allocator_t;
#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )
#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif

										sdUITransitionFloat( sdUserInterfaceScope* scope, idStrList& list ) : sdUITransition< float, sdProperties::PT_FLOAT, 1 >( scope, list ) { }
	virtual								~sdUITransitionFloat( void ) {}
	virtual void						Attach( sdUIExpression* output );
	virtual float						GetFloatValue( void ) { return value; }
	virtual void						Free( void ) { delete this; }
private:	
};

extern const char sdUITransitionVec2_Identifier[];
class sdUITransitionVec2 :
	public sdUITransition< idVec2, sdProperties::PT_VEC2, 2 >,
	public sdPoolAllocator< sdUITransitionVec2, sdUITransitionVec2_Identifier, 1024 > {
public:
	typedef sdPoolAllocator< sdUITransitionVec2, sdUITransitionVec2_Identifier, 1024 > allocator_t;
#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )
#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif

										sdUITransitionVec2( sdUserInterfaceScope* scope, idStrList& list ) : sdUITransition< idVec2, sdProperties::PT_VEC2, 2 >( scope, list ) { }
	virtual								~sdUITransitionVec2( void ) {}
	virtual void						Attach( sdUIExpression* output );
	virtual const idVec2 &				GetVec2Value( idVec2 &temp ) { return value; }
	virtual void						Free( void ) { delete this; }
private:
};

extern const char sdUITransitionVec3_Identifier[];
class sdUITransitionVec3 :
	public sdUITransition< idVec3, sdProperties::PT_VEC3, 3 >,
	public sdPoolAllocator< sdUITransitionVec3, sdUITransitionVec3_Identifier, 1024 > {
public:
	typedef sdPoolAllocator< sdUITransitionVec3, sdUITransitionVec3_Identifier, 1024 > allocator_t;
#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )
#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif

										sdUITransitionVec3( sdUserInterfaceScope* scope, idStrList& list ) : sdUITransition< idVec3, sdProperties::PT_VEC3, 3 >( scope, list ) { }
	virtual								~sdUITransitionVec3( void ) {}
	virtual void						Attach( sdUIExpression* output );
	virtual const idVec3 &				GetVec3Value( idVec3 &temp ) { return value; }
	virtual void						Free( void ) { delete this; }
private:
};

extern const char sdUITransitionVec4_Identifier[];
class sdUITransitionVec4 :
	public sdUITransition< idVec4, sdProperties::PT_VEC4, 4 >,
	public sdPoolAllocator< sdUITransitionVec4, sdUITransitionVec4_Identifier, 1024 > {
public:
	typedef sdPoolAllocator< sdUITransitionVec4, sdUITransitionVec4_Identifier, 1024 > allocator_t;
#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )
#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif

										sdUITransitionVec4( sdUserInterfaceScope* scope, idStrList& list ) : sdUITransition< idVec4, sdProperties::PT_VEC4, 4 >( scope, list ) { }
	virtual								~sdUITransitionVec4( void ) {}
	virtual void						Attach( sdUIExpression* output );
	virtual const idVec4 &				GetVec4Value( idVec4 &temp ) { return value; }
	virtual void						Free( void ) { delete this; }
private:
};

extern const char sdFunctionExpression_Identifier[];
class sdFunctionExpression :
	public sdUIExpression,
	public sdPoolAllocator< sdFunctionExpression, sdFunctionExpression_Identifier, 1024 > {
public:
	typedef sdPoolAllocator< sdFunctionExpression, sdFunctionExpression_Identifier, 1024 > allocator_t;
#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )
#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif


										sdFunctionExpression( const char* functionName, sdUserInterfaceScope* scope, sdUIFunctionInstance* _function, idStrList& list );
	virtual								~sdFunctionExpression( void );

	virtual void						InputChanged( void ) { assert( false ); }
	virtual void						Attach( sdUIExpression* output ) { }
	virtual void						Detach( void ) {}
	virtual sdProperties::ePropertyType	GetType( void ) const { return function->GetFunctionInfo()->GetReturnType(); }

	virtual void						OnOnChangedOverflow( void ) { ; }

	virtual int							GetIntValue( void );
	virtual float						GetFloatValue( void );
	virtual const idStr	&				GetStringValue( idStr &temp );
	virtual const idWStr &				GetWStringValue( idWStr &temp );
	virtual const idVec2 &				GetVec2Value( idVec2 &temp );
	virtual const idVec3 &				GetVec3Value( idVec3 &temp );
	virtual const idVec4 &				GetVec4Value( idVec4 &temp );

	virtual void						Evaluate( void );
	virtual void						Free( void ) { delete this; }

private:

	void								AllocStack( void );
	void								FreeStack( void );
	void								EvaluateInternal( bool maintainStack );

	sdUIFunctionInstance*				function;
	sdUIFunctionStack					*_stack;
	idList< sdUIExpression* >			expressions;
};

extern const char sdFloatParmExpression_Identifier[];
class sdFloatParmExpression :
	public sdUIExpression,
	public sdPoolAllocator< sdFloatParmExpression, sdFloatParmExpression_Identifier, 1024 > {
public:
	typedef sdPoolAllocator< sdFloatParmExpression, sdFloatParmExpression_Identifier, 1024 > allocator_t;
#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )
#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif
										sdFloatParmExpression( sdUserInterfaceScope* _scope, idLexer* src );
	virtual								~sdFloatParmExpression( void );

	virtual void						Attach( sdUIExpression* output );
	virtual void						Detach( void );
	virtual sdProperties::ePropertyType GetType( void ) const { return sdProperties::PT_FLOAT; }
	virtual float						GetFloatValue( void ) { EvaluateRegisters(); return registers[ valueRegister ].value; }

	virtual void						InputChanged( void );
	virtual void						Free( void ) { delete this; }

	void SetValue( int index, const float newValue ) {
		registers[ index ].value = newValue;
		Update();
	}

	bool IsConstant( void ) const {
		return symbols.Num() == 0;
	}

private:

	typedef enum {
		WOP_TYPE_ADD,
		WOP_TYPE_SUBTRACT,
		WOP_TYPE_MULTIPLY,
		WOP_TYPE_DIVIDE,
		WOP_TYPE_TABLE,
		WOP_TYPE_GT,
		WOP_TYPE_GE,
		WOP_TYPE_LT,
		WOP_TYPE_LE,
		WOP_TYPE_EQ,
		WOP_TYPE_NE,
		WOP_TYPE_BIT_AND,
		WOP_TYPE_BIT_XOR,
		WOP_TYPE_BIT_OR,
		WOP_TYPE_AND,
		WOP_TYPE_OR,
		WOP_TYPE_MOD,
		WOP_TYPE_NOT,
		WOP_TYPE_BIT_COMP,
	} wexpOpType_t;

	typedef struct {
		unsigned char opType;	
		unsigned char a;
		unsigned char b;
		unsigned char c;
	} wexpOp_t;

	int								ExpressionConstant( float f );
	int								ExpressionTemporary( void );
	wexpOp_t&						ExpressionOp( void );
	int								ExpressionExpression( sdUIExpression* expression );
	int								EmitOp( int a, int b, wexpOpType_t opType, wexpOp_t** opp = NULL );
	int								ParseEmitOp( idLexer *src, int a, wexpOpType_t opType, int priority, const char* delimiter, sdUserInterfaceScope* scope, wexpOp_t** opp = NULL );
	void							Parse( idLexer* src, sdUserInterfaceScope* scope );
	int								ParseTerm( idLexer *src, const char* delimiter, sdUserInterfaceScope* scope );
	int								ParseExpression( idLexer *src, const char* delimiter, sdUserInterfaceScope* scope );
	int								ParseExpressionPriority( idLexer *src, int priority, const char* delimiter, sdUserInterfaceScope* scope );
	void							EvaluateRegisters( void );
	void							Update( void );
	virtual void					OnOnChangedOverflow( void ) { outputExpression->OnOnChangedOverflow(); }

	typedef struct expressionRegister_s {
		float						value;
	} expressionRegister_t;

	typedef struct expressionSymbol_s {
		sdUIExpression*				expression;
		short						position;
		short						immediate;
	} expressionSymbol_t;

	idList< expressionSymbol_t >	symbols;
	idList< expressionRegister_t >	registers;
	idList< wexpOp_t >				ops;

	int								valueRegister;
	sdUIExpression*					outputExpression;
	bool							immediate;
	int								inImmediate;
};

#if 0
extern const char sdIntParmExpression_Identifier[];
class sdIntParmExpression :
	public sdUIExpression,
	public sdPoolAllocator< sdIntParmExpression, sdIntParmExpression_Identifier, 1024 > {
public:
	typedef sdPoolAllocator< sdIntParmExpression, sdIntParmExpression_Identifier, 1024 > allocator_t;
#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )
#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif
										sdIntParmExpression( sdUserInterfaceScope* _scope, idLexer* src );
	virtual								~sdIntParmExpression( void );

	virtual void						Attach( sdUIExpression* output );
	virtual void						Detach( void );
	virtual sdProperties::ePropertyType GetType( void ) const { return sdProperties::PT_INT; }
	virtual int							GetIntValue( void ) { EvaluateRegisters(); return registers[ valueRegister ].value; }

	virtual void						InputChanged( void );

	void SetValue( int index, const int newValue ) {
		assert( registers[ index ].temporary );
		registers[ index ].value = newValue;
		Update();
	}

private:

	typedef enum {
		WOP_TYPE_ADD,
		WOP_TYPE_SUBTRACT,
		WOP_TYPE_MULTIPLY,
		WOP_TYPE_DIVIDE,
		WOP_TYPE_TABLE,
		WOP_TYPE_GT,
		WOP_TYPE_GE,
		WOP_TYPE_LT,
		WOP_TYPE_LE,
		WOP_TYPE_EQ,
		WOP_TYPE_NE,
		WOP_TYPE_BIT_AND,
		WOP_TYPE_BIT_XOR,
		WOP_TYPE_BIT_OR,
		WOP_TYPE_AND,
		WOP_TYPE_OR,
		WOP_TYPE_MOD,
	} wexpOpType_t;

	typedef struct {
		wexpOpType_t opType;	
		int	a, b, c, d;
	} wexpOp_t;

	int								ExpressionConstant( float f );
	int								ExpressionTemporary( void );
	wexpOp_t&						ExpressionOp( void );
	int								ExpressionExpression( sdUIExpression* expression );
	int								EmitOp( int a, int b, wexpOpType_t opType, wexpOp_t** opp = NULL );
	int								ParseEmitOp( idLexer *src, int a, wexpOpType_t opType, int priority, const char* delimiter, sdUserInterfaceScope* scope, wexpOp_t** opp = NULL );
	void							Parse( idLexer* src, sdUserInterfaceScope* scope );
	int								ParseTerm( idLexer *src, const char* delimiter, sdUserInterfaceScope* scope );
	int								ParseExpression( idLexer *src, const char* delimiter, sdUserInterfaceScope* scope );
	int								ParseExpressionPriority( idLexer *src, int priority, const char* delimiter, sdUserInterfaceScope* scope );
	void							EvaluateRegisters( void );
	void							Update( void );

	typedef struct expressionRegister_s {
		int							value;
		bool						temporary;
	} expressionRegister_t;

	typedef struct expressionSymbol_s {
		sdUIExpression*				expression;
		int							position;
		bool						immediate;
	} expressionSymbol_t;

	idList< expressionSymbol_t >	symbols;
	idList< expressionRegister_t >	registers;
	idList< wexpOp_t >				ops;

	int								valueRegister;
	sdUIExpression*					outputExpression;
	bool							immediate;
	int								inImmediate;
};
#endif

extern const char sdStringParmExpression_Identifier[];
class sdStringParmExpression :
	public sdUIExpression,
	public sdPoolAllocator< sdStringParmExpression, sdStringParmExpression_Identifier, 1024 > {
public:
	typedef sdPoolAllocator< sdStringParmExpression, sdStringParmExpression_Identifier, 1024 > allocator_t;
#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )
#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif

										sdStringParmExpression( sdUserInterfaceScope* _scope, idLexer* src );
	virtual								~sdStringParmExpression( void );

	virtual void						Attach( sdUIExpression* output );
	virtual void						Detach( void );
	virtual sdProperties::ePropertyType GetType( void ) const { return sdProperties::PT_STRING; }
	virtual const idStr &				GetStringValue( idStr &temp ) { EvaluateRegisters(); return registers[ valueRegister ].value; }

	virtual void						InputChanged( void );
	virtual void						Free( void ) { delete this; }
	virtual void						OnOnChangedOverflow( void ) { outputExpression->OnOnChangedOverflow(); }

	bool IsConstant( void ) const {
		return symbols.Num() == 0;
	}

	void SetValue( int index, const char* newValue ) {
		assert( registers[ index ].temporary );
		registers[ index ].value = newValue;
		Update();
	}

private:

	typedef enum {
		WOP_TYPE_ADD,
		WOP_TYPE_EQ,
		WOP_TYPE_NE,
	} wexpOpType_t;

	typedef struct {
		unsigned char opType;	
		unsigned char a;
		unsigned char b;
		unsigned char c;
	} wexpOp_t;

	int								ExpressionConstant( const char* str );
	int								ExpressionTemporary( void );
	wexpOp_t&						ExpressionOp( void );
	int								ExpressionExpression( sdUIExpression* expression );
	int								EmitOp( int a, int b, wexpOpType_t opType, wexpOp_t** opp = NULL );
	int								ParseEmitOp( idLexer *src, int a, wexpOpType_t opType, int priority, const char* delimiter, sdUserInterfaceScope* scope, wexpOp_t** opp = NULL );
	void							Parse( idLexer* src, sdUserInterfaceScope* scope );
	int								ParseTerm( idLexer *src, const char* delimiter, sdUserInterfaceScope* scope );
	int								ParseExpression( idLexer *src, const char* delimiter, sdUserInterfaceScope* scope );
	int								ParseExpressionPriority( idLexer *src, int priority, const char* delimiter, sdUserInterfaceScope* scope );
	void							EvaluateRegisters( void );
	void							Update( void );

	typedef struct expressionRegister_s {
		idStr						value;
		bool						temporary;
	} expressionRegister_t;

	typedef struct expressionSymbol_s {
		sdUIExpression*				expression;
		short						position;
		short						immediate;
	} expressionSymbol_t;

	idList< expressionSymbol_t >	symbols;
	idList< expressionRegister_t >	registers;
	idList< wexpOp_t >				ops;

	int								valueRegister;
	sdUIExpression*					outputExpression;
	bool							immediate;
	int								inImmediate;
};

#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif

#define UI_SINGLEPARM_EXPRESSION( TYPENAME, TYPE, VALUENAME, VALUETYPE )																																							\
	extern const char sdSingleParmExpression##TYPENAME##_Identifier[];\
class sdSingleParmExpression##TYPENAME :																																															\
	public sdUIExpression,																																																			\
	public sdPoolAllocator< sdSingleParmExpression##TYPENAME, sdSingleParmExpression##TYPENAME##_Identifier, 1024 > {																																								\
public:																																																								\
	typedef sdPoolAllocator< sdSingleParmExpression##TYPENAME, sdSingleParmExpression##TYPENAME##_Identifier, 1024 > allocator_t;																									\
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )																							\
										sdSingleParmExpression##TYPENAME( sdProperties::sdProperty* input ) { assert( !callbackHandle.IsValid() ); inputProperty = input; outputExpression = NULL; }										\
	virtual								~sdSingleParmExpression##TYPENAME( void ) { Detach(); }																										\
	virtual void						Attach( sdUIExpression* output ) { assert( !callbackHandle.IsValid() ); outputExpression = output; callbackHandle = inputProperty->value.VALUENAME->AddOnChangeHandler( sdFunctions::sdBindMem0< void, const TYPE&, const TYPE&, sdFunctions::sdEmptyType >( &sdSingleParmExpression##TYPENAME::OnChanged, this ) ); }				\
	virtual void						Detach( void ) { if( callbackHandle.IsValid() ) { inputProperty->value.VALUENAME->RemoveOnChangeHandler( callbackHandle ); }}																		\
	virtual sdProperties::ePropertyType GetType( void ) const { return VALUETYPE; }																													\
	void								OnChanged( const TYPE& oldValue, const TYPE& newValue ) { IncActiveOnChangeHandlers( this ); assert( outputExpression ); outputExpression->InputChanged(); DecActiveOnChangeHandlers(); }					\
	virtual TYPE						Get##TYPENAME##Value( void ) { assert( inputProperty->GetValueType() == VALUETYPE ); return inputProperty->value.VALUENAME->GetValue(); }					\
	virtual void						InputChanged( void ) { assert( false ); }																													\
	virtual void						Free( void ) { delete this; }																																\
	virtual void						OnOnChangedOverflow( void ) { outputExpression->OnOnChangedOverflow(); }																					\
																																																	\
private:																																															\
	sdProperties::sdProperty*			inputProperty;																																				\
	sdUIExpression*						outputExpression;																																			\
	sdProperties::CallbackHandle		callbackHandle;																																				\
};

#define UI_SINGLEPARM_EXPRESSION_REF( TYPENAME, TYPE, VALUENAME, VALUETYPE )																																							\
	extern const char sdSingleParmExpression##TYPENAME##_Identifier[];\
class sdSingleParmExpression##TYPENAME :																																															\
	public sdUIExpression,																																																			\
	public sdPoolAllocator< sdSingleParmExpression##TYPENAME, sdSingleParmExpression##TYPENAME##_Identifier, 1024 > {																																								\
public:																																																								\
	typedef sdPoolAllocator< sdSingleParmExpression##TYPENAME, sdSingleParmExpression##TYPENAME##_Identifier, 1024 > allocator_t;																									\
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )																							\
										sdSingleParmExpression##TYPENAME( sdProperties::sdProperty* input ) { assert( !callbackHandle.IsValid() ); inputProperty = input; outputExpression = NULL; }										\
	virtual								~sdSingleParmExpression##TYPENAME( void ) { Detach(); }																										\
	virtual void						Attach( sdUIExpression* output ) { assert( !callbackHandle.IsValid() ); outputExpression = output; callbackHandle = inputProperty->value.VALUENAME->AddOnChangeHandler( sdFunctions::sdBindMem0< void, const TYPE&, const TYPE&, sdFunctions::sdEmptyType >( &sdSingleParmExpression##TYPENAME::OnChanged, this ) ); }				\
	virtual void						Detach( void ) { if( callbackHandle.IsValid() ) { inputProperty->value.VALUENAME->RemoveOnChangeHandler( callbackHandle ); }}																		\
	virtual sdProperties::ePropertyType GetType( void ) const { return VALUETYPE; }																													\
	void								OnChanged( const TYPE& oldValue, const TYPE& newValue ) { IncActiveOnChangeHandlers( this ); assert( outputExpression ); outputExpression->InputChanged(); DecActiveOnChangeHandlers(); }									\
	virtual const TYPE	&				Get##TYPENAME##Value( TYPE& temp ) { assert( inputProperty->GetValueType() == VALUETYPE ); return inputProperty->value.VALUENAME->GetValue(); }				\
	virtual void						InputChanged( void ) { assert( false ); }																													\
	virtual void						Free( void ) { delete this; }																																\
	virtual void						OnOnChangedOverflow( void ) { outputExpression->OnOnChangedOverflow(); }																					\
																																																	\
private:																																															\
	sdProperties::sdProperty*			inputProperty;																																				\
	sdUIExpression*						outputExpression;																																			\
	sdProperties::CallbackHandle		callbackHandle;																																				\
};

UI_SINGLEPARM_EXPRESSION( Int,		int,	intValue,		sdProperties::PT_INT	)
UI_SINGLEPARM_EXPRESSION( Float,	float,	floatValue,		sdProperties::PT_FLOAT	)
UI_SINGLEPARM_EXPRESSION_REF( Vec2,		idVec2, vec2Value,		sdProperties::PT_VEC2	)
UI_SINGLEPARM_EXPRESSION_REF( Vec3,		idVec3, vec3Value,		sdProperties::PT_VEC3	)
UI_SINGLEPARM_EXPRESSION_REF( Vec4,		idVec4, vec4Value,		sdProperties::PT_VEC4	)
UI_SINGLEPARM_EXPRESSION_REF( String,	idStr,	stringValue,	sdProperties::PT_STRING	)
UI_SINGLEPARM_EXPRESSION_REF( WString,	idWStr,	wstringValue,	sdProperties::PT_WSTRING)

#define UI_SINGLEPARM_EXPRESSION_FIELD( TYPENAME, TYPE, VALUENAME, VALUETYPE )																																							\
	extern const char sdSingleParmExpression##TYPENAME##_Identifier[];																																									\
class sdSingleParmExpressionField##TYPENAME :																																															\
	public sdUIExpression,																																																				\
	public sdPoolAllocator< sdSingleParmExpressionField##TYPENAME, sdSingleParmExpression##TYPENAME##_Identifier, 1024 > {																												\
public:																																																									\
	typedef sdPoolAllocator< sdSingleParmExpressionField##TYPENAME, sdSingleParmExpression##TYPENAME##_Identifier, 1024 > allocator_t;																									\
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )																																														\
										sdSingleParmExpressionField##TYPENAME( sdProperties::sdProperty* input, int index ) { assert( !callbackHandle.IsValid() ); inputIndex = index; inputProperty = input; outputExpression = NULL; }\
	virtual								~sdSingleParmExpressionField##TYPENAME( void ) { Detach(); }																																	\
	virtual void						Attach( sdUIExpression* output ) { assert( !callbackHandle.IsValid() ); outputExpression = output; callbackHandle = inputProperty->value.VALUENAME->AddOnChangeHandler( sdFunctions::sdBindMem0< void, const TYPE&, const TYPE&, sdFunctions::sdEmptyType >( &sdSingleParmExpressionField##TYPENAME::OnChanged, this )); }								\
	virtual void						Detach( void ) { if( callbackHandle.IsValid() ) { inputProperty->value.VALUENAME->RemoveOnChangeHandler( callbackHandle ); }}																	\
	virtual sdProperties::ePropertyType GetType( void ) const { return VALUETYPE; }																																						\
	void								OnChanged( const TYPE& oldValue, const TYPE& newValue ) { IncActiveOnChangeHandlers( this ); assert( outputExpression ); outputExpression->InputChanged(); DecActiveOnChangeHandlers(); }		\
	virtual float						GetFloatValue( void ) { assert( inputProperty->GetValueType() == VALUETYPE ); return inputProperty->value.VALUENAME->GetValue()[ inputIndex ]; }			\
	virtual void						InputChanged( void ) { assert( false ); }																													\
	virtual void						Free( void ) { delete this; }																																\
	virtual void						OnOnChangedOverflow( void ) { outputExpression->OnOnChangedOverflow(); }																					\
private:																																															\
	sdProperties::sdProperty*			inputProperty;																																				\
	int									inputIndex;																																					\
	sdUIExpression*						outputExpression;																																			\
	sdProperties::CallbackHandle		callbackHandle;																																				\
};

UI_SINGLEPARM_EXPRESSION_FIELD( Vec2,		idVec2, vec2Value,		sdProperties::PT_VEC2	)
UI_SINGLEPARM_EXPRESSION_FIELD( Vec3,		idVec3, vec3Value,		sdProperties::PT_VEC3	)
UI_SINGLEPARM_EXPRESSION_FIELD( Vec4,		idVec4, vec4Value,		sdProperties::PT_VEC4	)

#define UI_CONST_EXPRESSION( TYPENAME, TYPE, VALUENAME, VALUETYPE )																\
	extern const char sdConstParmExpression##TYPENAME##_Identifier[];															\
class sdConstParmExpression##TYPENAME : public sdUIExpression, public sdPoolAllocator< sdConstParmExpression##TYPENAME, sdConstParmExpression##TYPENAME##_Identifier, 1024 > {			\
public:																															\
	typedef sdPoolAllocator< sdConstParmExpression##TYPENAME, sdConstParmExpression##TYPENAME##_Identifier, 1024 > allocator_t;	\
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )																				\
																																\
private:																														\
										sdConstParmExpression##TYPENAME( const TYPE& inputValue ) { value = inputValue; }		\
	virtual								~sdConstParmExpression##TYPENAME( void ) { }											\
																																\
public:																															\
	static sdConstParmExpression##TYPENAME*	Alloc( const TYPE& inputValue ) {													\
		return new sdConstParmExpression##TYPENAME( inputValue );																\
	}																															\
																																\
	virtual void OnOnChangedOverflow( void ) {																					\
	}																															\
	virtual void						Free( void ) { delete this; }															\
																																\
	virtual void						Attach( sdUIExpression* output ) { }													\
	virtual void						Detach( void ) { }																		\
	virtual sdProperties::ePropertyType GetType( void ) const { return VALUETYPE; }												\
	virtual TYPE						Get##TYPENAME##Value( void ) { return value; }											\
	virtual void						InputChanged( void ) { assert( false ); }												\
																																\
private:																														\
	TYPE								value;																					\
};

#define UI_CONST_EXPRESSION_REF( TYPENAME, TYPE, VALUENAME, VALUETYPE )																\
	extern const char sdConstParmExpression##TYPENAME##_Identifier[];															\
class sdConstParmExpression##TYPENAME : public sdUIExpression, public sdPoolAllocator< sdConstParmExpression##TYPENAME, sdConstParmExpression##TYPENAME##_Identifier, 1024 > {			\
public:																															\
	typedef sdPoolAllocator< sdConstParmExpression##TYPENAME, sdConstParmExpression##TYPENAME##_Identifier, 1024 > allocator_t;	\
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )																				\
																																\
private:																														\
										sdConstParmExpression##TYPENAME( const TYPE& inputValue ) { value = inputValue; }		\
	virtual								~sdConstParmExpression##TYPENAME( void ) { }											\
																																\
public:																															\
	static sdConstParmExpression##TYPENAME*	Alloc( const TYPE& inputValue ) {													\
		return new sdConstParmExpression##TYPENAME( inputValue );																\
	}																															\
																																\
	virtual void OnOnChangedOverflow( void ) {																					\
	}																															\
	virtual void						Free( void ) { delete this; }															\
																																\
	virtual void						Attach( sdUIExpression* output ) { }													\
	virtual void						Detach( void ) { }																		\
	virtual sdProperties::ePropertyType GetType( void ) const { return VALUETYPE; }												\
	virtual const TYPE &				Get##TYPENAME##Value( TYPE &temp ) { return value; }									\
	virtual void						InputChanged( void ) { assert( false ); }												\
																																\
private:																														\
	TYPE								value;																					\
};

#define UI_CONST_EXPRESSION_HASH( TYPENAME, TYPE, VALUENAME, VALUETYPE )																\
	extern const char sdConstParmExpression##TYPENAME##_Identifier[];															\
class sdConstParmExpression##TYPENAME : public sdUIExpression, public sdPoolAllocator< sdConstParmExpression##TYPENAME, sdConstParmExpression##TYPENAME##_Identifier, 1024 > {			\
public:																															\
	typedef sdPoolAllocator< sdConstParmExpression##TYPENAME, sdConstParmExpression##TYPENAME##_Identifier, 1024 > allocator_t;	\
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )																				\
																																\
private:																														\
										sdConstParmExpression##TYPENAME( const TYPE& inputValue ) { value = inputValue; refCount = 1; }		\
	virtual								~sdConstParmExpression##TYPENAME( void ) { }											\
																																\
public:																															\
	static sdConstParmExpression##TYPENAME*	Alloc( const TYPE& inputValue ) {													\
		int key = hashIndex.GenerateKey( inputValue );																			\
		for ( int index = hashIndex.GetFirst( key ); index != idHashIndex::NULL_INDEX; index = hashIndex.GetNext( index ) ) {	\
			if ( linearList[ index ]->Get##TYPENAME##Value() == inputValue ) {													\
				linearList[ index ]->IncRef();																					\
				return linearList[ index ];																						\
			}																													\
		}																														\
		int index = linearList.FindNull();																						\
		if ( index == -1 ) {																									\
			index = linearList.Num();																							\
			linearList.Alloc() = NULL;																							\
		}																														\
																																\
		linearList[ index ] = new sdConstParmExpression##TYPENAME( inputValue );												\
		hashIndex.Add( key, index );																							\
		return linearList[ index ];																								\
	}																															\
																																\
	virtual void OnOnChangedOverflow( void ) {																					\
	}																															\
	static void	DeAlloc( sdConstParmExpression##TYPENAME* value ) {																\
		int index = linearList.FindIndex( value );																				\
		assert( index != -1 );																									\
		int key = hashIndex.GenerateKey( value->Get##TYPENAME##Value() );														\
		hashIndex.Remove( key, index );																							\
		linearList[ index ] = NULL;																								\
		delete value;																											\
	}																															\
																																\
	void								IncRef( void ) { refCount++; }															\
	void								DecRef( void ) { refCount--; if ( refCount == 0 ) { DeAlloc( this ); } }				\
																																\
	virtual void						Free( void ) { DecRef(); }																\
																																\
	virtual void						Attach( sdUIExpression* output ) { }													\
	virtual void						Detach( void ) { }																		\
	virtual sdProperties::ePropertyType GetType( void ) const { return VALUETYPE; }												\
	virtual TYPE						Get##TYPENAME##Value( void ) { return value; }											\
	virtual void						InputChanged( void ) { assert( false ); }												\
																																\
private:																														\
	TYPE								value;																					\
	int									refCount;																				\
																																\
	static idHashIndex									hashIndex;																\
	static idList< sdConstParmExpression##TYPENAME* >	linearList;																\
};

#define UI_CONST_EXPRESSION_HASH_REF( TYPENAME, TYPE, VALUENAME, VALUETYPE )																\
	extern const char sdConstParmExpression##TYPENAME##_Identifier[];															\
class sdConstParmExpression##TYPENAME : public sdUIExpression, public sdPoolAllocator< sdConstParmExpression##TYPENAME, sdConstParmExpression##TYPENAME##_Identifier, 1024 > {			\
public:																															\
	typedef sdPoolAllocator< sdConstParmExpression##TYPENAME, sdConstParmExpression##TYPENAME##_Identifier, 1024 > allocator_t;	\
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )																				\
																																\
private:																														\
										sdConstParmExpression##TYPENAME( const TYPE& inputValue ) { value = inputValue; refCount = 1; }		\
	virtual								~sdConstParmExpression##TYPENAME( void ) { }											\
																																\
public:																															\
	static sdConstParmExpression##TYPENAME*	Alloc( const TYPE& inputValue ) {													\
		int key = hashIndex.GenerateKey( inputValue );																			\
		for ( int index = hashIndex.GetFirst( key ); index != idHashIndex::NULL_INDEX; index = hashIndex.GetNext( index ) ) {	\
			TYPE temp;																											\
			if ( linearList[ index ]->Get##TYPENAME##Value( temp ) == inputValue ) {											\
				linearList[ index ]->IncRef();																					\
				return linearList[ index ];																						\
			}																													\
		}																														\
		int index = linearList.FindNull();																						\
		if ( index == -1 ) {																									\
			index = linearList.Num();																							\
			linearList.Alloc() = NULL;																							\
		}																														\
																																\
		linearList[ index ] = new sdConstParmExpression##TYPENAME( inputValue );												\
		hashIndex.Add( key, index );																							\
		return linearList[ index ];																								\
	}																															\
																																\
	virtual void OnOnChangedOverflow( void ) {																					\
	}																															\
	static void	DeAlloc( sdConstParmExpression##TYPENAME* value ) {																\
		int index = linearList.FindIndex( value );																				\
		assert( index != -1 );																									\
		TYPE temp;																												\
		int key = hashIndex.GenerateKey( value->Get##TYPENAME##Value( temp ) );													\
		hashIndex.Remove( key, index );																							\
		linearList[ index ] = NULL;																								\
		delete value;																											\
	}																															\
																																\
	void								IncRef( void ) { refCount++; }															\
	void								DecRef( void ) { refCount--; if ( refCount == 0 ) { DeAlloc( this ); } }				\
																																\
	virtual void						Free( void ) { DecRef(); }																\
																																\
	virtual void						Attach( sdUIExpression* output ) { }													\
	virtual void						Detach( void ) { }																		\
	virtual sdProperties::ePropertyType GetType( void ) const { return VALUETYPE; }												\
	virtual const TYPE &				Get##TYPENAME##Value( TYPE &temp ) { return value; }									\
	virtual void						InputChanged( void ) { assert( false ); }												\
																																\
private:																														\
	TYPE								value;																					\
	int									refCount;																				\
																																\
	static idHashIndex									hashIndex;																\
	static idList< sdConstParmExpression##TYPENAME* >	linearList;																\
};


#define UI_CONST_EXPRESSION_HASH_IMPL( TYPENAME )																				\
	idHashIndex									sdConstParmExpression##TYPENAME::hashIndex;										\
	idList< sdConstParmExpression##TYPENAME* >	sdConstParmExpression##TYPENAME::linearList;

UI_CONST_EXPRESSION( Int,			int,	intValue,		sdProperties::PT_INT	)
UI_CONST_EXPRESSION_HASH( Float,	float,	floatValue,		sdProperties::PT_FLOAT	)
UI_CONST_EXPRESSION_REF( Vec2,			idVec2, vec2Value,		sdProperties::PT_VEC2	)
UI_CONST_EXPRESSION_REF( Vec3,			idVec3, vec3Value,		sdProperties::PT_VEC3	)
UI_CONST_EXPRESSION_REF( Vec4,			idVec4, vec4Value,		sdProperties::PT_VEC4	)
UI_CONST_EXPRESSION_HASH_REF( String,	idStr,	stringValue,	sdProperties::PT_STRING	)

#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif

extern const char sdPropertyExpressionSingle_Identifier[];
class sdPropertyExpressionSingle :
	public sdUIExpression,
	public sdPoolAllocator< sdPropertyExpressionSingle, sdPropertyExpressionSingle_Identifier, 1024 > {
public:
	typedef sdPoolAllocator< sdPropertyExpressionSingle, sdPropertyExpressionSingle_Identifier, 1024 > allocator_t;
#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )
#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif

										sdPropertyExpressionSingle( const char* name, sdUIExpression* input );
	virtual								~sdPropertyExpressionSingle( void );
	virtual void						SetProperty( sdProperties::sdProperty* output, int index, int key, sdUserInterfaceScope* outputScope );
	virtual sdProperties::ePropertyType	GetType( void ) const { return inputExpression->GetType(); }
	virtual void						Attach( sdUIExpression* ) { assert( false ); }
	virtual void						Detach( void );
	virtual void						InputChanged( void );

	virtual void						OnOnChangedOverflow( void );

	virtual void						Update( void );
	virtual void						Free( void ) { delete this; }

protected:
	sdProperties::sdProperty*			outputProperty;
	sdUIExpression*						inputExpression;

	int									scopeIndex;
	int									scopeKey;
	sdUserInterfaceScope*				scope;
};

extern const char sdPropertyExpressionField_Identifier[];
class sdPropertyExpressionField :
	public sdPropertyExpressionSingle,
	public sdPoolAllocator< sdPropertyExpressionField, sdPropertyExpressionField_Identifier, 1024 > {
public:
	typedef sdPoolAllocator< sdPropertyExpressionField, sdPropertyExpressionField_Identifier, 1024 > allocator_t;
#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )
#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif

										sdPropertyExpressionField( const char* name, sdUIExpression* input, int index );
	virtual								~sdPropertyExpressionField() {}

	virtual sdProperties::ePropertyType	GetType( void ) const { return sdProperties::PT_FLOAT; }
	void								Update( void );	
	virtual void						Free( void ) { delete this; }

protected:
	int									outputField;
};

class sdUIEvaluator;

class sdUIEvaluatorTypeBase {
public:
	virtual																~sdUIEvaluatorTypeBase( void ) { ; }

	virtual int															GetIntValue( sdUIEvaluator* ) { assert( false ); return 0; }
	virtual float														GetFloatValue( sdUIEvaluator* ) { assert( false ); return 0.f; }
	virtual idStr														GetStringValue( sdUIEvaluator* ) { assert( false ); return ""; }
	virtual idWStr														GetWStringValue( sdUIEvaluator* ) { assert( false ); return L""; }
	virtual idVec2														GetVec2Value( sdUIEvaluator* ) { assert( false ); return vec2_zero; }
	virtual idVec3														GetVec3Value( sdUIEvaluator* ) { assert( false ); return vec3_zero; }
	virtual idVec4														GetVec4Value( sdUIEvaluator* ) { assert( false ); return vec4_zero; }

	virtual sdProperties::ePropertyType									GetReturnType( void ) const = 0;
	virtual int															GetNumParms( void ) const = 0;
	virtual bool														IsVariadic( void ) const = 0;
	virtual sdProperties::ePropertyType									GetParmType( int index ) const = 0;
	virtual const char*													GetName( void ) const = 0;
};

#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif

#define UI_EVALUATOR_TYPE_DECLARATION( TYPENAME, TYPE, VALUETYPE )																									\
	extern const char sdUIEvaluatorType##TYPENAME##_Identifier[];\
class sdUIEvaluatorType##TYPENAME : public sdUIEvaluatorTypeBase, public sdPoolAllocator< sdUIEvaluatorType##TYPENAME, sdUIEvaluatorType##TYPENAME##_Identifier, 1024 > {										\
public:																																								\
	typedef sdPoolAllocator< sdUIEvaluatorType##TYPENAME, sdUIEvaluatorType##TYPENAME##_Identifier, 1024 > allocator_t;												\
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )																													\
	typedef TYPE ( *evaluationFunc_t )( const sdUIEvaluator* );																										\
																		sdUIEvaluatorType##TYPENAME( const char* parms, const char* name, evaluationFunc_t func );	\
	virtual																~sdUIEvaluatorType##TYPENAME( void ) {}														\
	virtual sdProperties::ePropertyType									GetReturnType( void ) const { return VALUETYPE; }											\
	virtual int															GetNumParms( void ) const { return parms.Num(); }											\
	virtual bool														IsVariadic( void ) const { return isVariadic; }												\
	virtual sdProperties::ePropertyType									GetParmType( int index ) const { return parms[ index ]; }									\
	virtual TYPE														Get##TYPENAME##Value( sdUIEvaluator* evaluator ) { return function( evaluator );	}		\
	virtual const char*													GetName( void ) const { return name; }														\
																																									\
private:																																							\
	idList< sdProperties::ePropertyType >								parms;																						\
	evaluationFunc_t													function;																					\
	idStr																name;																						\
	bool																isVariadic;																					\
};

UI_EVALUATOR_TYPE_DECLARATION( Int,			int,	sdProperties::PT_INT	)
UI_EVALUATOR_TYPE_DECLARATION( Float,		float,	sdProperties::PT_FLOAT	)
UI_EVALUATOR_TYPE_DECLARATION( Vec2,		idVec2, sdProperties::PT_VEC2	)
UI_EVALUATOR_TYPE_DECLARATION( Vec3,		idVec3, sdProperties::PT_VEC3	)
UI_EVALUATOR_TYPE_DECLARATION( Vec4,		idVec4, sdProperties::PT_VEC4	)
UI_EVALUATOR_TYPE_DECLARATION( String,		idStr,	sdProperties::PT_STRING	)
UI_EVALUATOR_TYPE_DECLARATION( WString,		idWStr,	sdProperties::PT_WSTRING)

#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif

class sdUIEvaluator : public sdUIExpression {
public:
																		sdUIEvaluator( sdUIEvaluatorTypeBase* type, sdUserInterfaceScope* _scope, idLexer* src );
	virtual																sdUIEvaluator::~sdUIEvaluator( void );

	virtual sdProperties::ePropertyType									GetType( void ) const { return type->GetReturnType(); }
	virtual void														SetProperty( sdProperties::sdProperty* output ) { assert( false ); }
	virtual void														Attach( sdUIExpression* );
	virtual void														Detach( void );
	sdUIExpression*														GetParm( int index ) const { return parms[ index ]; }
	int																	GetNumParms() const { return parms.Num(); }
	bool																IsVariadic() const { return type->IsVariadic(); }
	sdUserInterfaceScope*												GetScope() const { return scope; }

protected:
	sdUIEvaluatorTypeBase*												type;
	sdUIExpression*														outputExpression;
	idList< sdUIExpression* >											parms;
	sdUserInterfaceScope*												scope;
};

#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif

#define UI_EVALUATOR( TYPENAME, TYPE )																																						\
	extern const char sdUIEvaluator##TYPENAME##_Identifier[];																																\
class sdUIEvaluator##TYPENAME : public sdUIEvaluator, public sdPoolAllocator< sdUIEvaluator##TYPENAME, sdUIEvaluator##TYPENAME##_Identifier, 1024 > {										\
public:																																														\
	typedef sdPoolAllocator< sdUIEvaluator##TYPENAME, sdUIEvaluator##TYPENAME##_Identifier, 1024 > allocator_t;																				\
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )																																			\
																		sdUIEvaluator##TYPENAME( sdUIEvaluatorTypeBase* type, sdUserInterfaceScope* _scope, idLexer* src ) : sdUIEvaluator( type, _scope, src ) { }	\
	virtual																~sdUIEvaluator##TYPENAME( void ) {}																					\
																																															\
	virtual TYPE														Get##TYPENAME##Value( void ) { value = type->Get##TYPENAME##Value( this ); return value; }							\
	virtual void														InputChanged( void ) { outputExpression->InputChanged(); }															\
	virtual void														Attach( sdUIExpression* expression ) { sdUIEvaluator::Attach( expression ); }										\
	virtual void														Free( void ) { delete this; }																						\
	virtual void														OnOnChangedOverflow( void ) { outputExpression->OnOnChangedOverflow(); }																					\
protected:																																													\
	TYPE																value;																												\
};

#define UI_EVALUATOR_REF( TYPENAME, TYPE )																																						\
	extern const char sdUIEvaluator##TYPENAME##_Identifier[];																																\
class sdUIEvaluator##TYPENAME : public sdUIEvaluator, public sdPoolAllocator< sdUIEvaluator##TYPENAME, sdUIEvaluator##TYPENAME##_Identifier, 1024 > {										\
public:																																														\
	typedef sdPoolAllocator< sdUIEvaluator##TYPENAME, sdUIEvaluator##TYPENAME##_Identifier, 1024 > allocator_t;																				\
	SD_DISAMBIGUATE_POOL_ALLOCATOR( allocator_t )																																			\
																		sdUIEvaluator##TYPENAME( sdUIEvaluatorTypeBase* type, sdUserInterfaceScope* _scope, idLexer* src ) : sdUIEvaluator( type, _scope, src ) { }	\
	virtual																~sdUIEvaluator##TYPENAME( void ) {}																					\
																																															\
	virtual const TYPE &												Get##TYPENAME##Value( TYPE &temp ) { value = type->Get##TYPENAME##Value( this ); return value; }					\
	virtual void														InputChanged( void ) { outputExpression->InputChanged(); }															\
	virtual void														Attach( sdUIExpression* expression ) { sdUIEvaluator::Attach( expression ); }										\
	virtual void														Free( void ) { delete this; }																						\
	virtual void														OnOnChangedOverflow( void ) { outputExpression->OnOnChangedOverflow(); }																					\
protected:																																													\
	TYPE																value;																												\
};

UI_EVALUATOR( Int,		int		)
UI_EVALUATOR( Float,	float	)
UI_EVALUATOR_REF( Vec2,		idVec2	)
UI_EVALUATOR_REF( Vec3,		idVec3	)
UI_EVALUATOR_REF( Vec4,		idVec4	)
UI_EVALUATOR_REF( String,	idStr	)
UI_EVALUATOR_REF( WString,	idWStr	)

#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif

#endif // __GAME_GUIS_USERINTERFACEEXPRESSIONS_H__
