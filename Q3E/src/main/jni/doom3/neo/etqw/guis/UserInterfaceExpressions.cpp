// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UserInterfaceExpressions.h"
#include "UserInterfaceLocal.h"

UI_CONST_EXPRESSION_HASH_IMPL( String )
UI_CONST_EXPRESSION_HASH_IMPL( Float )

const char sdUITransitionFloat_Identifier[] 			= "sdUITransitionFloat";
const char sdUITransitionVec2_Identifier[]				= "sdUITransitionVec2";
const char sdUITransitionVec3_Identifier[]				= "sdUITransitionVec3";
const char sdUITransitionVec4_Identifier[]				= "sdUITransitionVec4";

const char sdFunctionExpression_Identifier[]			= "sdFunctionExpression";
const char sdFloatParmExpression_Identifier[]			= "sdFloatParmExpression";
const char sdStringParmExpression_Identifier[]			= "sdStringParmExpression";

const char sdPropertyExpressionSingle_Identifier[]		= "sdPropertyExpressionSingle";
const char sdPropertyExpressionField_Identifier[]		= "sdPropertyExpressionField";

const char sdUIEvaluatorTypeInt_Identifier[]			= "sdUIEvaluatorTypeInt";
const char sdUIEvaluatorTypeFloat_Identifier[]			= "sdUIEvaluatorTypeFloat";
const char sdUIEvaluatorTypeVec2_Identifier[]			= "sdUIEvaluatorTypeVec2";
const char sdUIEvaluatorTypeVec3_Identifier[]			= "sdUIEvaluatorTypeVec2";
const char sdUIEvaluatorTypeVec4_Identifier[]			= "sdUIEvaluatorTypeVec2";
const char sdUIEvaluatorTypeString_Identifier[] 		= "sdUIEvaluatorTypeString";
const char sdUIEvaluatorTypeWString_Identifier[] 		= "sdUIEvaluatorTypeWString";

const char sdUIEvaluatorInt_Identifier[]				= "sdUIEvaluatorInt";
const char sdUIEvaluatorFloat_Identifier[]				= "sdUIEvaluatorFloat";
const char sdUIEvaluatorVec2_Identifier[]				= "sdUIEvaluatorVec2";
const char sdUIEvaluatorVec3_Identifier[]				= "sdUIEvaluatorVec2";
const char sdUIEvaluatorVec4_Identifier[]				= "sdUIEvaluatorVec2";
const char sdUIEvaluatorString_Identifier[] 			= "sdUIEvaluatorString";
const char sdUIEvaluatorWString_Identifier[] 			= "sdUIEvaluatorWString";

const char sdSingleParmExpressionInt_Identifier[]		= "sdSingleParmExpressionInt";
const char sdSingleParmExpressionFloat_Identifier[]		= "sdSingleParmExpressionFloat";
const char sdSingleParmExpressionString_Identifier[]	= "sdSingleParmExpressionString";
const char sdSingleParmExpressionWString_Identifier[]	= "sdSingleParmExpressionWString";
const char sdSingleParmExpressionVec2_Identifier[] 		= "sdSingleParmExpressionVec2";
const char sdSingleParmExpressionVec3_Identifier[] 		= "sdSingleParmExpressionVec3";
const char sdSingleParmExpressionVec4_Identifier[] 		= "sdSingleParmExpressionVec4";

const char sdConstParmExpressionInt_Identifier[]		= "sdConstParmExpressionInt";
const char sdConstParmExpressionFloat_Identifier[]		= "sdConstParmExpressionFloat";
const char sdConstParmExpressionString_Identifier[] 	= "sdConstParmExpressionString";
const char sdConstParmExpressionVec2_Identifier[]		= "sdConstParmExpressionVec2";
const char sdConstParmExpressionVec3_Identifier[]		= "sdConstParmExpressionVec3";
const char sdConstParmExpressionVec4_Identifier[]		= "sdConstParmExpressionVec4";

/*
===============================================================================

	sdUIExpression

===============================================================================
*/

idStaticList< sdUIExpression*, sdUIExpression::MAX_ACTIVE_ONCHANGED_HANDLERS > sdUIExpression::activeOnChangedHandlers;

/*
================
sdUIExpression::IncActiveOnChangeHandlers
================
*/
void sdUIExpression::IncActiveOnChangeHandlers( sdUIExpression* expression ) {
	sdUIExpression** newExpressionEntry = activeOnChangedHandlers.Alloc();
	if ( newExpressionEntry == NULL ) {
		for ( int i = 0; i < activeOnChangedHandlers.Num(); i++ ) {
			activeOnChangedHandlers[ i ]->OnOnChangedOverflow();
		}
		activeOnChangedHandlers.Clear();
		common->FatalError( "sdUIExpression; OnChanged Handler Overflow" );
	}

	*newExpressionEntry = expression;
}

/*
================
sdUIExpression::DecActiveOnChangeHandlers
================
*/
void sdUIExpression::DecActiveOnChangeHandlers( void ) {
	if ( activeOnChangedHandlers.Num() == 0 ) {
		gameLocal.Error( "sdUIExpression; OnChanged Handler Underflow" );
	}
	activeOnChangedHandlers.SetNum( activeOnChangedHandlers.Num() - 1 );
}

/*
================
sdUIExpression::AllocTransition
================
*/
sdUIExpression* sdUIExpression::AllocTransition( sdProperties::ePropertyType type, sdUserInterfaceScope* scope, idLexer* src ) {
	idStr temp;
	src->ParseBracedSection( temp, -1, true, '(', ')' );

	idLexer parser( temp, temp.Length(), "AllocFunctionExpression" );
	parser.ExpectTokenString( "(" );

	idStr value;
	idStrList terms;
	sdUIScriptEvent::ReadExpression( &parser, value, terms, "(", ")", true );

	switch ( type ) {
		case sdProperties::PT_FLOAT:
			return new sdUITransitionFloat( scope, terms );
		case sdProperties::PT_VEC2:
			return new sdUITransitionVec2( scope, terms );
		case sdProperties::PT_VEC3:
			return new sdUITransitionVec3( scope, terms );
		case sdProperties::PT_VEC4:
			return new sdUITransitionVec4( scope, terms );
		default:
			gameLocal.Error( "Transitions Only Valid For Floating Point Based Properties" );
			return NULL;
	}
}

/*
================
sdUIExpression::AllocFunctionExpression
================
*/
sdUIExpression* sdUIExpression::AllocFunctionExpression( const char* functionName, sdUIFunctionInstance* function, sdUserInterfaceScope* scope, idLexer* src ) {
	idStr temp;
	src->ParseBracedSection( temp, -1, true, '(', ')' );

	idLexer parser( temp, temp.Length(), functionName );
	parser.ExpectTokenString( "(" );

	idStr value;
	idStrList terms;
	sdUIScriptEvent::ReadExpression( &parser, value, terms, "(", ")", true );

	return new sdFunctionExpression( functionName, scope, function, terms );
}



/*
============
IsNumeric
similar to idStr::IsNumeric, but skips whitespace after a - sign
============
*/
static bool IsNumeric( const char *s ) {
	int		i;
	bool	dot;

	if ( *s == '-' ) {
		s++;
	}

	while( s && *s == ' ' ) {
		s++;
	}

	dot = false;
	for ( i = 0; s[i]; i++ ) {
		if ( !isdigit( s[i] ) ) {
			if ( ( s[ i ] == '.' ) && !dot ) {
				dot = true;
				continue;
			}
			return false;
		}
	}

	return true;
}

/*
================
sdUIExpression::AllocSingleParmExpression
================
*/
sdUIExpression* sdUIExpression::AllocSingleParmExpression( sdProperties::ePropertyType type, sdUserInterfaceScope* scope, const char* text ) {
	idLexer parser( text, idStr::Length( text ), "AllocSingleParmExpression" );

	sdUserInterfaceScope* altScope = gameLocal.GetUserInterfaceScope( *scope, &parser );

	idToken token;
	parser.ReadToken( &token );

	sdProperties::sdProperty* rhs = altScope->GetProperty( token );
	if ( rhs ) {
		if ( rhs->GetValueType() != type ) {
			gameLocal.Error( "Invalid Property Type in '%s'", text );
		}

		idToken token;
		if ( parser.ReadToken( &token ) ) {
			gameLocal.Error( "Invalid Parameters '%s' After Property", token.c_str() );
		}

		switch( type ) {
			case sdProperties::PT_INT:
				return new sdSingleParmExpressionInt( rhs );
			case sdProperties::PT_FLOAT:				
				return new sdSingleParmExpressionFloat( rhs );
			case sdProperties::PT_VEC2:
				return new sdSingleParmExpressionVec2( rhs );
			case sdProperties::PT_VEC3:
				return new sdSingleParmExpressionVec3( rhs );
			case sdProperties::PT_VEC4:
				return new sdSingleParmExpressionVec4( rhs );
			case sdProperties::PT_STRING:
				return new sdSingleParmExpressionString( rhs );
			case sdProperties::PT_WSTRING:
				return new sdSingleParmExpressionWString( rhs );
		}
		assert( false );
		return NULL;
	}

	sdUIFunctionInstance* function = altScope->GetFunction( token );
	if ( function ) {
		if ( function->GetFunctionInfo()->GetReturnType() != type ) {
			gameLocal.Error( "Invalid Expression '%s'", text );
		}

		return sdUIExpression::AllocFunctionExpression( token, function, scope, &parser );
	}

	sdUIEvaluatorTypeBase* evaluator = altScope->GetEvaluator( token );
	if ( evaluator ) {
		if ( evaluator->GetReturnType() != type ) {
			gameLocal.Error( "Invalid Expression '%s'", text );
		}

		switch ( type ) {
			case sdProperties::PT_INT:
				return new sdUIEvaluatorInt( evaluator, scope, &parser );
			case sdProperties::PT_FLOAT:
				return new sdUIEvaluatorFloat( evaluator, scope, &parser );
			case sdProperties::PT_STRING:
				return new sdUIEvaluatorString( evaluator, scope, &parser );
			case sdProperties::PT_WSTRING:
				return new sdUIEvaluatorWString( evaluator, scope, &parser );
			case sdProperties::PT_VEC2:
				return new sdUIEvaluatorVec2( evaluator, scope, &parser );
			case sdProperties::PT_VEC3:
				return new sdUIEvaluatorVec3( evaluator, scope, &parser );
			case sdProperties::PT_VEC4:
				return new sdUIEvaluatorVec4( evaluator, scope, &parser );
			default:
				gameLocal.Error( "Invalid Type on Evaluator" );
				return NULL;
		}
	}

	idStr temp = text;
	temp.StripLeadingOnce( "\"" );
	temp.StripTrailingOnce( "\"" );

	idStr value;
	idStrList sublist;
	idLexer parser2( temp, temp.Length(), "" );
	sdUIScriptEvent::ReadExpression( &parser2, value, sublist, NULL, NULL );

	int count = sdProperties::CountForPropertyType( type );

	if ( sublist.Num() != count ) {
		gameLocal.Error( "Invalid Number of Parms '%s' (expected %i but found %i)", text, count, sublist.Num() );
	}

	switch( type ) {
		case sdProperties::PT_INT:
			if( !IsNumeric( sublist[ 0 ].c_str() ) ) {
				gameLocal.Error( "'%s' is not numeric", text );
			}
			return sdConstParmExpressionInt::Alloc( atoi( sublist[ 0 ] ) );
		case sdProperties::PT_FLOAT:				
			if( !IsNumeric( sublist[ 0 ].c_str() ) ) {
				gameLocal.Error( "'%s' is not numeric", text );
			}
			return sdConstParmExpressionFloat::Alloc( atof( sublist[ 0 ] ) );
		case sdProperties::PT_VEC2:
			if( !IsNumeric( sublist[ 0 ].c_str() ) || !IsNumeric( sublist[ 1 ].c_str() ) ) {
				gameLocal.Error( "'%s' is not numeric", text );
			}
			return sdConstParmExpressionVec2::Alloc( idVec2( atof( sublist[ 0 ] ), atof( sublist[ 1 ] ) ) );
		case sdProperties::PT_VEC3:
			if( !IsNumeric( sublist[ 0 ].c_str() ) || !IsNumeric( sublist[ 1 ].c_str() ) || !IsNumeric( sublist[ 2 ].c_str() ) ) {
				gameLocal.Error( "'%s' is not numeric", text );
			}
			return sdConstParmExpressionVec3::Alloc( idVec3( atof( sublist[ 0 ] ), atof( sublist[ 1 ] ), atof( sublist[ 2 ] ) ) );
		case sdProperties::PT_VEC4:
			if( !IsNumeric( sublist[ 0 ].c_str() ) || !IsNumeric( sublist[ 1 ].c_str() ) || !IsNumeric( sublist[ 2 ].c_str() ) || !IsNumeric( sublist[ 3 ].c_str() ) ) {
				gameLocal.Error( "'%s' is not numeric", text );
			}
			return sdConstParmExpressionVec4::Alloc( idVec4( atof( sublist[ 0 ] ), atof( sublist[ 1 ] ), atof( sublist[ 2 ] ), atof( sublist[ 3 ] ) ) );
		case sdProperties::PT_STRING:
			return sdConstParmExpressionString::Alloc( sublist[ 0 ] );
		default:
			gameLocal.Error( "Invalid Type on Constant" );
			return NULL;
	}

	return NULL;
}

/*
================
sdUIExpression::AllocIntExpression
================
*/
sdUIExpression* sdUIExpression::AllocIntExpression( sdUserInterfaceScope* scope, const char* text ) {
	idLexer src( text, idStr::Length( text ), "AllocIntExpression" );
	idToken token;

	sdUserInterfaceScope* altScope = gameLocal.GetUserInterfaceScope( *scope, &src );

	src.ExpectAnyToken( &token );

	idStr propertyName;
	sdProperties::sdProperty* property = altScope->GetProperty( token );
	if ( property != NULL ) {
		if ( property->GetValueType() != sdProperties::PT_INT ) {
			src.Error( "Cannot use '%s' in an Int expression", sdProperties::sdProperty::TypeToString( property->GetValueType() ) );
		}
		return new sdSingleParmExpressionInt( property );
	}

	sdUIFunctionInstance* function = altScope->GetFunction( token );
	if ( function != NULL ) {
		if ( function->GetFunctionInfo()->GetReturnType() != sdProperties::PT_INT ) {
			int type = function->GetFunctionInfo()->GetReturnType();
			delete function;
			src.Error( "Return Type '%s' was not an Int", sdProperties::sdProperty::TypeToString( type ) );
		}
		return sdUIExpression::AllocFunctionExpression( token, function, scope, &src );
	}

	sdUIEvaluatorTypeBase* evaluator = altScope->GetEvaluator( token );
	if ( evaluator != NULL ) {
		if ( evaluator->GetReturnType() != sdProperties::PT_INT ) {
			src.Error( "Return Type '%s' was not an Int", sdProperties::sdProperty::TypeToString( evaluator->GetReturnType() ) );
		}
		return new sdUIEvaluatorInt( evaluator, scope, &src );
	}

	idStr message = token;
	while ( src.ReadToken( &token ) ) {
		if ( token.linesCrossed ) {
			src.UnreadToken( &token );
			break;
		}
		message += token;
	}
	src.Error( "Invalid Term '%s' in '%s'", message.c_str(), scope->GetUI()->GetName() );
	return NULL;
}

/*
================
sdUIExpression::AllocFloatExpression
================
*/
sdUIExpression* sdUIExpression::AllocFloatExpression( sdUserInterfaceScope* scope, const char* text ) {

	sdUIExpression* expression = NULL;
	try {
		idLexer parser( text, idStr::Length( text ), "AllocFloatExpression" );
		sdFloatParmExpression* floatExpression = new sdFloatParmExpression( scope, &parser );
		if ( floatExpression->IsConstant() ) {
			float value = floatExpression->GetFloatValue();
//			gameLocal.Printf( "%s = %f\n", text, value );
			expression = sdConstParmExpressionFloat::Alloc( value );
			delete floatExpression;
		} else {
			expression = floatExpression;
		}

	} catch( idException& ){
		gameLocal.Printf( "Failed to parse expression ^7'%s'^0\n", text );
		delete expression;
		throw;
	}

	return expression;
}


/*
================
sdUIExpression::AllocStringExpression
================
*/
sdUIExpression* sdUIExpression::AllocStringExpression( sdUserInterfaceScope* scope, const char* text ) {
	idLexer parser( text, idStr::Length( text ), "AllocStringExpression" );
	sdUIExpression* expression = NULL;

	try {
		sdStringParmExpression* stringExpression = new sdStringParmExpression( scope, &parser );
		if ( stringExpression->IsConstant() ) {
			idStr temp;
			idStr const &value = stringExpression->GetStringValue( temp );
//			gameLocal.Printf( "%s = %s\n", text, value.c_str() );
			expression = sdConstParmExpressionString::Alloc( value );
			delete stringExpression;
		} else {
			expression = stringExpression;
		}
	} catch( idException& ){
		gameLocal.Printf( "Failed to parse expression ^7'%s'^0\n", text );
		delete expression;
		expression = NULL;
		throw;
	}

	return expression;
}

/*
================
sdUIExpression::AllocWStringExpression
================
*/
sdUIExpression* sdUIExpression::AllocWStringExpression( sdUserInterfaceScope* scope, const char* text ) {
	idLexer src( text, idStr::Length( text ), "AllocWStringExpression" );
	idToken token;

	sdUserInterfaceScope* altScope = gameLocal.GetUserInterfaceScope( *scope, &src );

	src.ExpectAnyToken( &token );

	idStr propertyName;
	sdProperties::sdProperty* property = altScope->GetProperty( token );
	if ( property != NULL ) {
		if ( property->GetValueType() != sdProperties::PT_WSTRING ) {
			src.Error( "Only Wide String Terms May be Used" );
		}
		return new sdSingleParmExpressionWString( property );
	}

	sdUIFunctionInstance* function = altScope->GetFunction( token );
	if ( function != NULL ) {
		if ( function->GetFunctionInfo()->GetReturnType() != sdProperties::PT_WSTRING ) {
			int type = function->GetFunctionInfo()->GetReturnType();
			delete function;
			src.Error( "Return Type '%s' was not a WString", sdProperties::sdProperty::TypeToString( type ) );
		}
		return sdUIExpression::AllocFunctionExpression( token, function, scope, &src );
	}

	sdUIEvaluatorTypeBase* evaluator = altScope->GetEvaluator( token );
	if ( evaluator != NULL ) {
		if ( evaluator->GetReturnType() != sdProperties::PT_WSTRING ) {
			src.Error( "Return Type Was Not a Wide String" );
		}
		return new sdUIEvaluatorWString( evaluator, scope, &src );
	}

	idStr message = token;
	while ( src.ReadToken( &token ) ) {
		if ( token.linesCrossed ) {
			src.UnreadToken( &token );
			break;
		}
		message += token;
	}
	src.Error( "Invalid Term '%s' in '%s'", message.c_str(), scope->GetUI()->GetName() );
	return NULL;
}


/*
===============================================================================

	sdUITransition

===============================================================================
*/

/*
================
sdUITransition::sdUITransition
================
*/
template< typename T, sdProperties::ePropertyType TYPE, int COUNT >
sdUITransition< T, TYPE, COUNT >::sdUITransition( sdUserInterfaceScope* scope, idStrList& list ) : outputExpression( NULL ), table( NULL ), interpolator( NULL ) {
	if ( list.Num() < 3 || list.Num() > 4 ) {
		gameLocal.Error( "Transition Takes 3 or 4 Parameters" );
	}

	int j;
	for ( j = 0; j < COUNT; j++ ) {
		startExpressions[ j ] = NULL;
		endExpressions[ j ] = NULL;
	}

	idStr temp;
	for ( j = 0; j < 2; j++ ) {
		sdUIExpression** valuePtr = ( j == 0 ? startExpressions : endExpressions );

		temp = list[ j ];
		temp.StripLeadingOnce( "\"" );
		temp.StripTrailingOnce( "\"" );

		idStr value;
		idStrList sublist;
		idLexer parser( temp, temp.Length(), "" );
		sdUIScriptEvent::ReadExpression( &parser, value, sublist, NULL, NULL );

		if ( sublist.Num() == COUNT ) {
			int i;
			for ( i = 0; i < COUNT; i++ ) {
				valuePtr[ i ] = sdUIExpression::AllocFloatExpression( scope, sublist[ i ] );
				assert( valuePtr[ 0 ] != NULL );
			}
		} else if ( sublist.Num() == 1 ) {
			valuePtr[ 0 ] = sdUIExpression::AllocSingleParmExpression( TYPE, scope, sublist[ 0 ] );
			assert( valuePtr[ 0 ] != NULL );
		} else {
			gameLocal.Error( "Invalid Size of Parameter to Transition" );
		}
	}

	if( list.Num() >= 4 ) {
		temp = list[ 3 ];
		temp.StripLeading( "\"" );
		temp.StripTrailing( "\"" );

		if( !temp.IsEmpty() ) {
			// parse an acceleration
			bool isTable = idStr::Icmpn( temp, "table://", 8 ) == 0;
			if( !isTable ) {
				idLexer parser( temp, temp.Length(), "Transition Acceleration" );
				
				accelTimes.x = parser.ParseFloat();
				parser.ExpectTokenString( "," );
				accelTimes.y = parser.ParseFloat();				
			} else if( temp.Length() > 8 ) {
				table = gameLocal.declTableType[ temp.c_str() + 8 ];
				if( !table ) {
					gameLocal.Warning( "sdUITransition: could not find table '%s'", list[ 3 ].c_str() );
				}
			}
		}
	} else {
		accelTimes.Set( 0.0f, 0.0f );
	}

	ui = scope->GetUI();
	
	assert( startExpressions[ 0 ] != NULL );
	assert( endExpressions[ 0 ] != NULL );

	duration = sdUIExpression::AllocFloatExpression( scope, list[ 2 ] );
}

/*
================
sdUITransition::~sdUITransition
================
*/
template< typename T, sdProperties::ePropertyType TYPE, int COUNT >
sdUITransition< T, TYPE, COUNT >::~sdUITransition( void ) {
	for ( int i = 0; i < COUNT; i++ ) {
		if ( startExpressions[ i ] != NULL ) {
			startExpressions[ i ]->Free();
		}
		if ( endExpressions[ i ] != NULL ) {
			endExpressions[ i ]->Free();
		}
	}

	duration->Free();

	if( interpolator ) {
		interpolatorAllocator.Free( interpolator );
		interpolator = NULL;
	}
}

/*
============
sdUITransition::OnSnapshotHitch
============
*/
template< typename T, sdProperties::ePropertyType TYPE, int COUNT >
void sdUITransition< T, TYPE, COUNT >::OnSnapshotHitch( int delta ) {
	startTime-= delta;
}

/*
================
sdUITransitionFloat::UpdateValue
================
*/
template< typename T, sdProperties::ePropertyType TYPE, int COUNT >
bool sdUITransition< T, TYPE, COUNT >::UpdateValue( void ) {
	
	int now = ui->GetCurrentTime();	
	if( now < startTime ) {
		now = startTime;
	}

	if( interpolator ) {
		bool isDone = interpolator->IsDone( now );
		if( isDone ) {
			value = interpolator->GetEndValue();
		} else {
			value = interpolator->GetCurrentValue( now );
		}
		
		outputExpression->InputChanged();
		return !isDone;
	}

	// normal behavior	
	if ( now > startTime + _cachedDuration ) {
		if ( table ) {
			value = Lerp( start, end, table->TableLookup( 1.0f ) );
		} else {
			value = end;
		}
		outputExpression->InputChanged();
		return false;
	}

	float frac = ( now - startTime ) / ( float )( _cachedDuration );
	if ( table ) {
		frac = table->TableLookup( frac );
	}
	value = Lerp( start, end, frac );
	
	outputExpression->InputChanged();
	return true;
}

/*
================
sdUITransitionFloat::Detach
================
*/
template< typename T, sdProperties::ePropertyType TYPE, int COUNT >
void sdUITransition< T, TYPE, COUNT >::Detach( void ) {
	ui->GetState().RemoveTransition( this );
}

/*
================
sdUITransitionFloat::Attach
================
*/
void sdUITransitionFloat::Attach( sdUIExpression* _output ) {
	outputExpression = _output;

	startTime = ui->GetCurrentTime();
	ui->GetState().AddTransition( this );

	start = startExpressions[ 0 ]->GetFloatValue();
	end = endExpressions[ 0 ]->GetFloatValue();

	value = start;

	_cachedDuration = duration->GetFloatValue();

	if( accelTimes.x > idMath::FLT_EPSILON || accelTimes.y > idMath::FLT_EPSILON ) {
		if ( interpolator == NULL ) {
			interpolator = interpolatorAllocator.Alloc();
		}
		interpolator->Init( startTime, accelTimes.x * _cachedDuration, accelTimes.y * _cachedDuration, _cachedDuration, start, end );
	}
}

/*
================
sdUITransitionVec2::Attach
================
*/
void sdUITransitionVec2::Attach( sdUIExpression* _output ) {
	outputExpression = _output;

	startTime = ui->GetCurrentTime();
	ui->GetState().AddTransition( this );

	if ( startExpressions[ 0 ]->GetType() == sdProperties::PT_VEC2 ) {
		idVec2 temp;
		start = startExpressions[ 0 ]->GetVec2Value( temp );
	} else {
		start[ 0 ] = startExpressions[ 0 ]->GetFloatValue();
		start[ 1 ] = startExpressions[ 1 ]->GetFloatValue();
	}

	if ( endExpressions[ 0 ]->GetType() == sdProperties::PT_VEC2 ) {
		idVec2 temp;
		end = endExpressions[ 0 ]->GetVec2Value( temp );
	} else {
		end[ 0 ] = endExpressions[ 0 ]->GetFloatValue();
		end[ 1 ] = endExpressions[ 1 ]->GetFloatValue();
	}

	value = start;

	_cachedDuration = duration->GetFloatValue();

	if( accelTimes.x > idMath::FLT_EPSILON || accelTimes.y > idMath::FLT_EPSILON ) {
		if ( interpolator == NULL ) {
			interpolator = interpolatorAllocator.Alloc();
		}
		interpolator->Init( startTime, accelTimes.x * _cachedDuration, accelTimes.y * _cachedDuration, _cachedDuration, start, end );
	}
}

/*
================
sdUITransitionVec3::Attach
================
*/
void sdUITransitionVec3::Attach( sdUIExpression* _output ) {
	outputExpression = _output;

	startTime = ui->GetCurrentTime();
	ui->GetState().AddTransition( this );

	if ( startExpressions[ 0 ]->GetType() == sdProperties::PT_VEC3 ) {
		idVec3 temp;
		start = startExpressions[ 0 ]->GetVec3Value( temp );
	} else {
		start[ 0 ] = startExpressions[ 0 ]->GetFloatValue();
		start[ 1 ] = startExpressions[ 1 ]->GetFloatValue();
		start[ 2 ] = startExpressions[ 2 ]->GetFloatValue();
	}

	if ( endExpressions[ 0 ]->GetType() == sdProperties::PT_VEC3 ) {
		idVec3 temp;
		end = endExpressions[ 0 ]->GetVec3Value( temp );
	} else {
		end[ 0 ] = endExpressions[ 0 ]->GetFloatValue();
		end[ 1 ] = endExpressions[ 1 ]->GetFloatValue();
		end[ 2 ] = endExpressions[ 2 ]->GetFloatValue();
	}

	value = start;

	_cachedDuration = duration->GetFloatValue();

	if( accelTimes.x > idMath::FLT_EPSILON || accelTimes.y > idMath::FLT_EPSILON ) {
		if ( interpolator == NULL ) {
			interpolator = interpolatorAllocator.Alloc();
		}
		interpolator->Init( startTime, accelTimes.x * _cachedDuration, accelTimes.y * _cachedDuration, _cachedDuration, start, end );
	}
}

/*
================
sdUITransitionVec4::Attach
================
*/
void sdUITransitionVec4::Attach( sdUIExpression* _output ) {
	outputExpression = _output;

	startTime = ui->GetCurrentTime();
	ui->GetState().AddTransition( this );

	if ( startExpressions[ 0 ]->GetType() == sdProperties::PT_VEC4 ) {
		idVec4 temp;
		start = startExpressions[ 0 ]->GetVec4Value( temp );
	} else {
		start[ 0 ] = startExpressions[ 0 ]->GetFloatValue();
		start[ 1 ] = startExpressions[ 1 ]->GetFloatValue();
		start[ 2 ] = startExpressions[ 2 ]->GetFloatValue();
		start[ 3 ] = startExpressions[ 3 ]->GetFloatValue();
	}

	if ( endExpressions[ 0 ]->GetType() == sdProperties::PT_VEC4 ) {
		idVec4 temp;
		end = endExpressions[ 0 ]->GetVec4Value( temp );
	} else {
		end[ 0 ] = endExpressions[ 0 ]->GetFloatValue();
		end[ 1 ] = endExpressions[ 1 ]->GetFloatValue();
		end[ 2 ] = endExpressions[ 2 ]->GetFloatValue();
		end[ 3 ] = endExpressions[ 3 ]->GetFloatValue();
	}

	value = start;

	_cachedDuration = duration->GetFloatValue();

	if( accelTimes.x > idMath::FLT_EPSILON || accelTimes.y > idMath::FLT_EPSILON ) {
		if ( interpolator == NULL ) {
			interpolator = interpolatorAllocator.Alloc();
		}
		interpolator->Init( startTime, accelTimes.x * _cachedDuration, accelTimes.y * _cachedDuration, _cachedDuration, start, end );
	}
}


/*
===============================================================================

	sdFunctionExpression

===============================================================================
*/



/*
============
sdFunctionExpression::AllocStack
============
*/
void sdFunctionExpression::AllocStack( void ) {
	if ( _stack == NULL ) {
		_stack = sdUIFunctionStack::allocator.Alloc();
		_stack->Clear();
	}
}


/*
============
sdFunctionExpression::FreeStack
============
*/
void sdFunctionExpression::FreeStack( void ) {
	if ( _stack != NULL && _stack->Empty() ) {
		sdUIFunctionStack::allocator.Free( _stack );
		_stack = NULL;
	}
}

/*
================
sdFunctionExpression::sdFunctionExpression
================
*/
sdFunctionExpression::sdFunctionExpression( const char* functionName, sdUserInterfaceScope* scope, sdUIFunctionInstance* _function, idStrList& list ) : function( NULL ), _stack(NULL), expressions(1) {
	const sdUIFunction* functionInfo = _function->GetFunctionInfo();

	if ( list.Num() != functionInfo->GetNumParms() ) {
		gameLocal.Error( "Invalid Number of Parms for Function '%s' ( expected %i but received %i )", functionName, functionInfo->GetNumParms(), list.Num() );
	}

	function = _function;

	int i;
	for ( i = 0; i < functionInfo->GetNumParms(); i++ ) {
		sdProperties::ePropertyType type = functionInfo->GetParm( i );

		switch ( type ) {
			case sdProperties::PT_STRING:
				expressions.Append( sdUIExpression::AllocStringExpression( scope, list[ i ] ) );
				break;
			case sdProperties::PT_WSTRING:
				expressions.Append( sdUIExpression::AllocWStringExpression( scope, list[ i ] ) );
				break;
			case sdProperties::PT_INT:
				expressions.Append( sdUIExpression::AllocIntExpression( scope, list[ i ] ) );
				break;
			case sdProperties::PT_FLOAT:
			case sdProperties::PT_VEC2:
			case sdProperties::PT_VEC3:
			case sdProperties::PT_VEC4: {

				idStr temp = list[ i ];
				temp.StripLeadingOnce( "\"" );
				temp.StripTrailingOnce( "\"" );

				int count = sdProperties::CountForPropertyType( type );

				idStr value;
				idStrList sublist;
				idLexer parser( temp, temp.Length(), "" );
				sdUIScriptEvent::ReadExpression( &parser, value, sublist, NULL, NULL );

				if ( sublist.Num() == count ) {
					int j;
					for ( j = 0; j < count; j++ ) {
						expressions.Append( sdUIExpression::AllocFloatExpression( scope, sublist[ j ] ) );
					}
				} else if ( sublist.Num() == 1 ) {
					expressions.Append( sdUIExpression::AllocSingleParmExpression( type, scope, sublist[ 0 ] ) );
				} else {
					gameLocal.Error( "Invalid Expression '%s' for Function Parm", value.c_str() );
				}
				break;
			}
			default:
				assert( false );
		}
	}
}

/*
================
sdFunctionExpression::~sdFunctionExpression
================
*/
sdFunctionExpression::~sdFunctionExpression( void ) {
	delete function;
	if ( _stack ) {
		_stack->Clear();
	}
	FreeStack();

	for ( int i = 0; i < expressions.Num(); i++ ) {
		expressions[ i ]->Free();
	}
	expressions.Clear();
}

/*
============
sdFunctionExpression::EvaluateInternal
============
*/
void sdFunctionExpression::EvaluateInternal( bool maintainStack ) {
	AllocStack();
	_stack->Clear();
	_stack->SetID( function->GetName() );

	int i;
	for ( i = expressions.Num() - 1; i >= 0; i-- ) {
		sdUIExpression* expression = expressions[ i ];
		switch ( expression->GetType() ) {
			case sdProperties::PT_STRING:
				{
					idStr temp;
					_stack->Push( expression->GetStringValue( temp ) );
				}
				break;
			case sdProperties::PT_WSTRING:
				{
					idWStr temp;
					_stack->Push( expression->GetWStringValue( temp ).c_str() );
				}
				break;
			case sdProperties::PT_INT:
				_stack->Push( expression->GetIntValue() );
				break;
			case sdProperties::PT_FLOAT:
				_stack->Push( expression->GetFloatValue() );
				break;
			case sdProperties::PT_VEC2:
				{
					idVec2 temp;
					_stack->Push( expression->GetVec2Value( temp ) );
				}
				break;
			case sdProperties::PT_VEC3:
				{
					idVec3 temp;
					_stack->Push( expression->GetVec3Value( temp ) );
				}
				break;
			case sdProperties::PT_VEC4:
				{
					idVec4 temp;
					_stack->Push( expression->GetVec4Value( temp ) );
				}
				break;
		}
	}

	function->Run( *_stack );
	if( !maintainStack ) {
		FreeStack();
	}
}

/*
================
sdFunctionExpression::Evaluate
================
*/
void sdFunctionExpression::Evaluate( void ) {
	EvaluateInternal( false );
}

/*
================
sdFunctionExpression::GetIntValue
================
*/
int sdFunctionExpression::GetIntValue( void ) {
	assert( function->GetFunctionInfo()->GetReturnType() == sdProperties::PT_INT );

	EvaluateInternal( true );

	int temp;
	_stack->Pop( temp );
	FreeStack();
	return temp;
}

/*
================
sdFunctionExpression::GetFloatValue
================
*/
float sdFunctionExpression::GetFloatValue( void ) {
	assert( function->GetFunctionInfo()->GetReturnType() == sdProperties::PT_FLOAT );

	EvaluateInternal( true );

	float temp;
	_stack->Pop( temp );
	FreeStack();
	return temp;
}

/*
================
sdFunctionExpression::GetStringValue
================
*/
const idStr & sdFunctionExpression::GetStringValue( idStr &temp ) {
	assert( function->GetFunctionInfo()->GetReturnType() == sdProperties::PT_STRING );

	EvaluateInternal( true );

	_stack->Pop( temp );
	FreeStack();
	return temp;
}

/*
================
sdFunctionExpression::GetWStringValue
================
*/
const idWStr & sdFunctionExpression::GetWStringValue( idWStr &temp ) {
	assert( function->GetFunctionInfo()->GetReturnType() == sdProperties::PT_WSTRING );

	EvaluateInternal( true );

	_stack->Pop( temp );
	FreeStack();
	return temp;
}

/*
================
sdFunctionExpression::GetVec2Value
================
*/
const idVec2 & sdFunctionExpression::GetVec2Value( idVec2 &temp ) {
	assert( function->GetFunctionInfo()->GetReturnType() == sdProperties::PT_VEC2 );

	EvaluateInternal( true );

	_stack->Pop( temp );
	FreeStack();
	return temp;
}

/*
================
sdFunctionExpression::GetVec3Value
================
*/
const idVec3 & sdFunctionExpression::GetVec3Value( idVec3 &temp ) {
	assert( function->GetFunctionInfo()->GetReturnType() == sdProperties::PT_VEC3 );

	EvaluateInternal( true );

	_stack->Pop( temp );
	FreeStack();
	return temp;
}

/*
================
sdFunctionExpression::GetVec4Value
================
*/
const idVec4 & sdFunctionExpression::GetVec4Value( idVec4 &temp ) {
	assert( function->GetFunctionInfo()->GetReturnType() == sdProperties::PT_VEC4 );

	EvaluateInternal( true );

	_stack->Pop( temp );
	FreeStack();
	return temp;
}

/*
===============================================================================

	sdFloatParmExpression

===============================================================================
*/

/*
================
sdFloatParmExpression::sdFloatParmExpression
================
*/
sdFloatParmExpression::sdFloatParmExpression( sdUserInterfaceScope* _scope, idLexer* src ) : symbols(1), registers(1), ops(1), outputExpression( NULL ), immediate( false ) {
	Parse( src, _scope );

	if ( registers.Num() == 0 ) {
		src->Error( "Invalid Float Expression" );
	}
}


/*
================
sdFloatParmExpression::ExpressionConstant
================
*/
int sdFloatParmExpression::ExpressionConstant( float f ) {
	int		i;

	for ( i = 0; i < registers.Num(); i++ ) {
		if ( registers[ i ].value == f ) {
			return i;
		}
	}
	
	i = registers.Num();
	expressionRegister_t& r = registers.Alloc();
	r.value = f;
	return i;
}

/*
================
sdFloatParmExpression::ExpressionExpression
================
*/
int sdFloatParmExpression::ExpressionExpression( sdUIExpression* expression ) {
	int i = registers.Num();
	expressionRegister_t& r = registers.Alloc();
	r.value = idMath::FLT_EPSILON;
	expressionSymbol_t& symbol = symbols.Alloc();
	symbol.expression	= expression;
	symbol.position		= i;
	symbol.immediate	= inImmediate != 0;
	return i;
}

/*
================
sdFloatParmExpression::ExpressionTemporary
================
*/
int sdFloatParmExpression::ExpressionTemporary( void ) {
	int i = registers.Num();
	expressionRegister_t& r = registers.Alloc();
	r.value = idMath::FLT_EPSILON;
	return i;
}

/*
================
sdFloatParmExpression::ExpressionOp
================
*/
sdFloatParmExpression::wexpOp_t& sdFloatParmExpression::ExpressionOp( void ) {
	wexpOp_t& wop = ops.Alloc();
	memset( &wop, 0, sizeof( wop ) );
	return wop;
}

/*
================
sdFloatParmExpression::EmitOp
================
*/
int sdFloatParmExpression::EmitOp( int a, int b, wexpOpType_t opType, wexpOp_t** opp ) {
	wexpOp_t& op = ExpressionOp();
	int c = ExpressionTemporary();

	if ( opType < 0 || opType > UCHAR_MAX ) {
		gameLocal.Error( "sdFloatParmExpression::EmitOp Opcode Out of Range" );
	}

	if ( a < 0 || a > UCHAR_MAX ) {
		gameLocal.Error( "sdFloatParmExpression::EmitOp Parm a Out of Range" );
	}

	if ( b < 0 || b > UCHAR_MAX ) {
		gameLocal.Error( "sdFloatParmExpression::EmitOp Parm b Out of Range" );
	}

	if ( c < 0 || c > UCHAR_MAX ) {
		gameLocal.Error( "sdFloatParmExpression::EmitOp Parm c Out of Range" );
	}

	op.opType	= ( unsigned char )opType;
	op.a		= ( unsigned char )a;
	op.b		= ( unsigned char )b;
	op.c		= ( unsigned char )c;

	if ( opp ) {
		*opp = &op;
	}
	return op.c;
}

/*
================
sdFloatParmExpression::ParseEmitOp
================
*/
int sdFloatParmExpression::ParseEmitOp( idLexer *src, int a, wexpOpType_t opType, int priority, const char* delimiter, sdUserInterfaceScope* scope, wexpOp_t** opp ) {
	int b = ParseExpressionPriority( src, priority, delimiter, scope );
	return EmitOp( a, b, opType, opp );  
}

/*
================
sdFloatParmExpression::ParseTerm

Returns a register index
=================
*/
int sdFloatParmExpression::ParseTerm( idLexer *src, const char* delimiter, sdUserInterfaceScope* scope ) {
	idToken token;
	int		a, b;

	src->ReadToken( &token );

	if ( token == "(" ) {
		a = ParseExpression( src, ")", scope );
		src->ExpectTokenString( ")" );
		return a;
	} else if ( token == "-" ) {
		src->ReadToken( &token );
		if ( token.type == TT_NUMBER || token == "." ) {
			return ExpressionConstant( -token.GetFloatValue() );
		}
		src->Warning( "Bad negative number '%s'", token.c_str() );
		return 0;
	} else if ( token.type == TT_NUMBER || token == "." ) {
		return ExpressionConstant( token.GetFloatValue() );
	}

	// see if it is a table name
	const idDeclTable *table = gameLocal.declTableType[ token.c_str() ];
	if ( table ) {
		a = table->Index();
		// parse a table expression
		src->ExpectTokenString( "[" );
		b = ParseExpression( src, "]", scope );
		src->ExpectTokenString( "]" );
		return EmitOp( a, b, WOP_TYPE_TABLE );
	}

	if ( !token.Icmp( "immediate" ) ) {
		inImmediate++;

		src->ExpectTokenString( "(" );
		a = ParseExpression( src, ")", scope );
		src->ExpectTokenString( ")" );

		inImmediate--;

		return a;
	}

	if( !token.Cmp( "!" ) ) {
		src->ReadToken( &token );
		if( !token.Cmp( "(" ) ) {
			src->UnreadToken( &token );
			src->ExpectTokenString( "(" );
			a = ParseExpression( src, ")", scope );
			src->ExpectTokenString( ")" );
		} else {
			src->UnreadToken( &token );
			a = ParseExpression( src, NULL, scope );
		}

		return EmitOp( a, 0, WOP_TYPE_NOT );
	}

	if( !token.Cmp( "~" ) ) {
		src->ReadToken( &token );
		if( !token.Cmp( "(" ) ) {
			src->UnreadToken( &token );
			src->ExpectTokenString( "(" );
			a = ParseExpression( src, ")", scope );
			src->ExpectTokenString( ")" );
		} else {
			src->UnreadToken( &token );
			a = ParseExpression( src, NULL, scope );
		}

		return EmitOp( a, 0, WOP_TYPE_BIT_COMP );
	}



	src->UnreadToken( &token );

	sdUserInterfaceScope* altScope = gameLocal.GetUserInterfaceScope( *scope, src );

	src->ReadToken( &token );

	idStr propertyName;
	sdProperties::sdProperty* property = altScope->GetProperty( token );
	if ( property ) {
		int propertyIndex = sdUIScriptEvent::GetPropertyField( property->GetValueType(), src );
		if ( propertyIndex == -1 ) {
			if ( property->GetValueType() != sdProperties::PT_FLOAT ) {
				src->Error( "Only Floating Point Terms May be Used" );
			}
			return ExpressionExpression( new sdSingleParmExpressionFloat( property ) );
		} else {
			switch( property->GetValueType() ) {
				case sdProperties::PT_VEC2:
					return ExpressionExpression( new sdSingleParmExpressionFieldVec2( property, propertyIndex ) );
				case sdProperties::PT_VEC3:
					return ExpressionExpression( new sdSingleParmExpressionFieldVec3( property, propertyIndex ) );
				case sdProperties::PT_VEC4:
					return ExpressionExpression( new sdSingleParmExpressionFieldVec4( property, propertyIndex ) );
			}
			src->Error( "Only Floating Point Terms May be Used" );
		}
	}

	sdUIFunctionInstance* function = altScope->GetFunction( token );
	if ( function ) {
		if ( function->GetFunctionInfo()->GetReturnType() != sdProperties::PT_FLOAT ) {
			int type = function->GetFunctionInfo()->GetReturnType();
			delete function;
			src->Error( "Return Type '%s' was not a Float", sdProperties::sdProperty::TypeToString( type ) );
		}
		immediate = true;

		sdUIExpression* expression = sdUIExpression::AllocFunctionExpression( token, function, scope, src );
		return ExpressionExpression( expression );
	}

	sdUIEvaluatorTypeBase* evaluator = altScope->GetEvaluator( token );
	if ( evaluator ) {
		if ( evaluator->GetReturnType() != sdProperties::PT_FLOAT ) {
			src->Error( "Return Type Was Not a Float" );
		}
		return ExpressionExpression( new sdUIEvaluatorFloat( evaluator, scope, src ) );
	}

	idStr message = token;
	while( src->ReadToken( &token ) ) {
		if ( token.linesCrossed ) {
			src->UnreadToken( &token );
			break;
		}
		message += token;
	}
	src->Error( "Invalid Term '%s' in '%s'", message.c_str(), scope->GetUI()->GetName() );
	return -1;
}

/*
=================
sdFloatParmExpression::ParseExpressionPriority

Returns a register index
=================
*/
int sdFloatParmExpression::ParseExpressionPriority( idLexer *src, int priority, const char* delimiter, sdUserInterfaceScope* scope ) {
	idToken token;
	int		a;

	if ( priority == 0 ) {
		return ParseTerm( src, delimiter, scope );
	}

	a = ParseExpressionPriority( src, priority - 1, delimiter, scope );

	if ( !src->ReadToken( &token ) ) {
		if ( !delimiter ) {
			// we won't get EOF in a real file, but we can
			// when parsing from generated strings
			return a;
		} else {
			src->Error( "Unexpected End of File" );
		}
	}

	if ( delimiter && !token.Icmp( delimiter ) ) {
		src->UnreadToken( &token );
		return a;
	}

	switch( priority ) {
		case 1: {
			if ( token == "*" ) {
				return ParseEmitOp( src, a, WOP_TYPE_MULTIPLY, priority, delimiter, scope );
			}
			if ( token == "/" ) {
				return ParseEmitOp( src, a, WOP_TYPE_DIVIDE, priority, delimiter, scope );
			}
			if ( token == "%" ) {
				return ParseEmitOp( src, a, WOP_TYPE_MOD, priority, delimiter, scope );
			}
			src->UnreadToken( &token );
			return a;
		}
		case 2: {
			if ( token == "+" ) {
				return ParseEmitOp( src, a, WOP_TYPE_ADD, priority, delimiter, scope );
			}
			if ( token == "-" ) {
				return ParseEmitOp( src, a, WOP_TYPE_SUBTRACT, priority, delimiter, scope );
			}
			src->UnreadToken( &token );
			return a;
		}
		case 3: {
			if ( token == ">" ) {
				return ParseEmitOp( src, a, WOP_TYPE_GT, priority, delimiter, scope );
			}
			if ( token == ">=" ) {
				return ParseEmitOp( src, a, WOP_TYPE_GE, priority, delimiter, scope );
			}
			if ( token == "<" ) {
				return ParseEmitOp( src, a, WOP_TYPE_LT, priority, delimiter, scope );
			}
			if ( token == "<=" ) {
				return ParseEmitOp( src, a, WOP_TYPE_LE, priority, delimiter, scope );
			}
			if ( token == "==" ) {
				return ParseEmitOp( src, a, WOP_TYPE_EQ, priority, delimiter, scope );
			}
			if ( token == "!=" ) {
				return ParseEmitOp( src, a, WOP_TYPE_NE, priority, delimiter, scope );
			}
			src->UnreadToken( &token );
			return a;
		}
		case 4: {
			if ( token == "&" ) {
				return ParseEmitOp( src, a, WOP_TYPE_BIT_AND, priority, delimiter, scope );
			}
			if ( token == "^" ) {
				return ParseEmitOp( src, a, WOP_TYPE_BIT_XOR, priority, delimiter, scope );
			}
			if ( token == "|" ) {
				return ParseEmitOp( src, a, WOP_TYPE_BIT_OR, priority, delimiter, scope );
			}
			src->UnreadToken( &token );
			return a;
		}
		case 5: {
			if ( token == "&&" ) {
				return ParseEmitOp( src, a, WOP_TYPE_AND, priority, delimiter, scope );
			}
			if ( token == "||" ) {
				return ParseEmitOp( src, a, WOP_TYPE_OR, priority, delimiter, scope );
			}
			src->UnreadToken( &token );
		}
	}

	// assume that anything else terminates the expression
	// not too robust error checking...

	src->Error( "Unknown Token '%s'", token.c_str() );

	return a;
}

/*
================
sdFloatParmExpression::ParseExpression

Returns a register index
================
*/
int sdFloatParmExpression::ParseExpression( idLexer *src, const char* delimiter, sdUserInterfaceScope* scope ) {
	const int TOP_PRIORITY = 5;
	return ParseExpressionPriority( src, TOP_PRIORITY, delimiter, scope );
}

/*
===============
sdFloatParmExpression::EvaluateRegisters
===============
*/
void sdFloatParmExpression::EvaluateRegisters( void ) {
	int i;

	for ( i = 0 ; i < symbols.Num() ; i++ ) {
		registers[ symbols[ i ].position ].value = symbols[ i ].expression->GetFloatValue();
	}

	for ( i = 0 ; i < ops.Num() ; i++ ) {
		wexpOp_t& op = ops[ i ];

		switch( op.opType ) {
		case WOP_TYPE_ADD:
			registers[ op.c ].value = registers[ op.a ].value + registers[ op.b ].value;
			break;
		case WOP_TYPE_SUBTRACT:
			registers[ op.c ].value = registers[ op.a ].value - registers[ op.b ].value;
			break;
		case WOP_TYPE_MULTIPLY:
			registers[ op.c ].value = registers[ op.a ].value * registers[ op.b ].value;
			break;
		case WOP_TYPE_DIVIDE:
			registers[ op.c ].value = registers[ op.a ].value / registers[ op.b ].value;
			break;
		case WOP_TYPE_MOD:
			registers[ op.c ].value = registers[ op.a ].value - ( idMath::Floor( registers[ op.a ].value / registers[ op.b ].value ) * registers[ op.b ].value );
			break;
		case WOP_TYPE_TABLE: {
			const idDeclTable* table = declHolder.declTableType.LocalFindByIndex( op.a );
			registers[ op.c ].value = table->TableLookup( registers[ op.b ].value );
			break;
		}
		case WOP_TYPE_NOT:
			registers[ op.c ].value = !idMath::Ftoi( registers[ op.a ].value );
			break;
		case WOP_TYPE_BIT_COMP:
			registers[ op.c ].value = ~idMath::Ftoi( registers[ op.a ].value );
			break;						   
		case WOP_TYPE_GT:
			registers[ op.c ].value = registers[ op.a ].value > registers[ op.b ].value;
			break;
		case WOP_TYPE_GE:
			registers[ op.c ].value = registers[ op.a ].value >= registers[ op.b ].value;
			break;
		case WOP_TYPE_LT:
			registers[ op.c ].value = registers[ op.a ].value < registers[ op.b ].value;
			break;
		case WOP_TYPE_LE:
			registers[ op.c ].value = registers[ op.a ].value <= registers[ op.b ].value;
			break;
		case WOP_TYPE_EQ:
			registers[ op.c ].value = idMath::Fabs( registers[ op.a ].value - registers[ op.b ].value ) < idMath::FLT_EPSILON;
			break;
		case WOP_TYPE_NE:
			registers[ op.c ].value = registers[ op.a ].value != registers[ op.b ].value;
			break;
		case WOP_TYPE_BIT_AND:
			registers[ op.c ].value = idMath::Ftoi( registers[ op.a ].value ) & idMath::Ftoi( registers[ op.b ].value );
			break;
		case WOP_TYPE_BIT_XOR:
			registers[ op.c ].value = idMath::Ftoi( registers[ op.a ].value ) ^ idMath::Ftoi( registers[ op.b ].value );
			break;
		case WOP_TYPE_BIT_OR:
			registers[ op.c ].value = idMath::Ftoi( registers[ op.a ].value ) | idMath::Ftoi( registers[ op.b ].value );
			break;
		case WOP_TYPE_AND:
			registers[ op.c ].value = registers[ op.a ].value && registers[ op.b ].value;
			break;
		case WOP_TYPE_OR:
			registers[ op.c ].value = registers[ op.a ].value || registers[ op.b ].value;
			break;
		default:
			gameLocal.Error( "EvaluateRegisters: bad opcode" );
			break;
		}
	}

}

/*
===============
sdFloatParmExpression::~sdFloatParmExpression
===============
*/
sdFloatParmExpression::~sdFloatParmExpression( void ) {
	for ( int i = 0; i < symbols.Num(); i++ ) {
		symbols[ i ].expression->Free();
	}
	symbols.Clear();
}

/*
===============
sdFloatParmExpression::EvaluateRegisters
===============
*/
void sdFloatParmExpression::Parse( idLexer* src, sdUserInterfaceScope* scope ) {
	inImmediate = 0;

	valueRegister = ParseExpression( src, NULL, scope );
}

/*
===============
sdFloatParmExpression::InputChanged
===============
*/
void sdFloatParmExpression::InputChanged( void ) {
	if ( immediate ) {
		return;
	}

	if ( outputExpression ) {
		outputExpression->InputChanged();
	}
}

/*
===============
sdFloatParmExpression::Update
===============
*/
void sdFloatParmExpression::Update( void ) {
	EvaluateRegisters();

	if ( outputExpression ) {
		outputExpression->InputChanged();
	}
}

/*
===============
sdFloatParmExpression::Attach
===============
*/
void sdFloatParmExpression::Attach( sdUIExpression* output ) {
	if ( !immediate ) {
		int i;
		for ( i = 0; i < symbols.Num(); i++ ) {
			if ( symbols[ i ].immediate ) {
				continue;
			}

			symbols[ i ].expression->Attach( this );
		}
	}
	outputExpression = output;
}

/*
===============
sdFloatParmExpression::Detach
===============
*/
void sdFloatParmExpression::Detach( void ) {
	if ( !immediate ) {
		int i;
		for ( i = 0; i < symbols.Num(); i++ ) {
			if ( symbols[ i ].immediate ) {
				continue;
			}

			symbols[ i ].expression->Detach();
		}
	}
	outputExpression	= NULL;
}









/*
===============================================================================

	sdStringParmExpression

===============================================================================
*/

/*
================
sdStringParmExpression::sdStringParmExpression
================
*/
sdStringParmExpression::sdStringParmExpression( sdUserInterfaceScope* _scope, idLexer* src ) : 
	symbols(1),
	registers(1),
	ops(1),
	outputExpression( NULL ),
	immediate( false ) {
	Parse( src, _scope );
}


/*
================
sdStringParmExpression::ExpressionConstant
================
*/
int sdStringParmExpression::ExpressionConstant( const char* str ) {
	int		i;

	for ( i = 0; i < registers.Num(); i++ ) {
		if ( !registers[ i ].temporary && registers[ i ].value == str ) {
			return i;
		}
	}
	
	i = registers.Num();
	expressionRegister_t& r = registers.Alloc();
	r.temporary = false;
	r.value = str;
	return i;
}

/*
================
sdStringParmExpression::ExpressionExpression
================
*/
int sdStringParmExpression::ExpressionExpression( sdUIExpression* expression ) {
	int i = registers.Num();
	expressionRegister_t& r = registers.Alloc();
	r.temporary = true;
	r.value = "";
	expressionSymbol_t& symbol = symbols.Alloc();
	symbol.expression	= expression;
	symbol.position		= i;
	symbol.immediate	= inImmediate != 0;
	return i;
}

/*
================
sdStringParmExpression::ExpressionTemporary
================
*/
int sdStringParmExpression::ExpressionTemporary( void ) {
	int i = registers.Num();
	expressionRegister_t& r = registers.Alloc();
	r.temporary = true;
	r.value = "";
	return i;
}

/*
================
sdStringParmExpression::ExpressionOp
================
*/
sdStringParmExpression::wexpOp_t& sdStringParmExpression::ExpressionOp( void ) {
	wexpOp_t& wop = ops.Alloc();
	memset( &wop, 0, sizeof( wop ) );
	return wop;
}

/*
================
sdStringParmExpression::EmitOp
================
*/
int sdStringParmExpression::EmitOp( int a, int b, wexpOpType_t opType, wexpOp_t** opp ) {
	wexpOp_t& op = ExpressionOp();
	int c = ExpressionTemporary();

	if ( opType < 0 || opType > UCHAR_MAX ) {
		gameLocal.Error( "sdStringParmExpression::EmitOp Opcode Out of Range" );
	}

	if ( a < 0 || a > UCHAR_MAX ) {
		gameLocal.Error( "sdStringParmExpression::EmitOp Parm a Out of Range" );
	}

	if ( b < 0 || b > UCHAR_MAX ) {
		gameLocal.Error( "sdStringParmExpression::EmitOp Parm b Out of Range" );
	}

	if ( c < 0 || c > UCHAR_MAX ) {
		gameLocal.Error( "sdStringParmExpression::EmitOp Parm c Out of Range" );
	}

	op.opType	= ( unsigned char )opType;
	op.a		= ( unsigned char )a;
	op.b		= ( unsigned char )b;
	op.c		= ( unsigned char )c;

	if ( opp ) {
		*opp = &op;
	}
	return op.c;
}

/*
================
sdStringParmExpression::ParseEmitOp
================
*/
int sdStringParmExpression::ParseEmitOp( idLexer *src, int a, wexpOpType_t opType, int priority, const char* delimiter, sdUserInterfaceScope* scope, wexpOp_t** opp ) {
	int b = ParseExpressionPriority( src, priority, delimiter, scope );
	return EmitOp( a, b, opType, opp );  
}

/*
================
sdStringParmExpression::ParseTerm

Returns a register index
=================
*/
int sdStringParmExpression::ParseTerm( idLexer *src, const char* delimiter, sdUserInterfaceScope* scope ) {
	idToken token;
	int a;

	src->ReadToken( &token );

	if ( token == "(" && token.type != TT_STRING ) {
		a = ParseExpression( src, ")", scope );
		src->ExpectTokenString( ")" );
		return a;
	} else if( !token.Icmp( "_quote" )) {
		return ExpressionConstant( "\"" );
	} else if( !token.Icmp( "_newline" )) {
		return ExpressionConstant( "\n" );
	} else if ( token.type == TT_STRING ) {
		return ExpressionConstant( token );
	}

	if ( !token.Icmp( "immediate" ) ) {
		inImmediate++;

		src->ExpectTokenString( "(" );

		a = ParseExpression( src, ")", scope );

		src->ExpectTokenString( ")" );

		inImmediate--;

		return a;
	}

	src->UnreadToken( &token );

	sdUserInterfaceScope* altScope = gameLocal.GetUserInterfaceScope( *scope, src );

	src->ReadToken( &token );

	idStr propertyName;
	sdProperties::sdProperty* property = altScope->GetProperty( token );
	if ( property ) {
		if ( property->GetValueType() != sdProperties::PT_STRING ) {
			src->Error( "Cannot assign a '%s' to a string", sdProperties::sdProperty::TypeToString( property->GetValueType() ) );
		}
		return ExpressionExpression( new sdSingleParmExpressionString( property ) );
	}

	sdUIFunctionInstance* function = altScope->GetFunction( token );
	if ( function ) {
		if ( function->GetFunctionInfo()->GetReturnType() != sdProperties::PT_STRING ) {
			int type = function->GetFunctionInfo()->GetReturnType();
			delete function;
			src->Error( "Return Type '%s' was not a String", sdProperties::sdProperty::TypeToString( type ) );
		}
		immediate = true;

		sdUIExpression* expression = sdUIExpression::AllocFunctionExpression( token, function, scope, src );
		return ExpressionExpression( expression );
	}

	sdUIEvaluatorTypeBase* evaluator = altScope->GetEvaluator( token );
	if ( evaluator ) {
		if ( evaluator->GetReturnType() != sdProperties::PT_STRING ) {
			src->Error( "Return Type '%s' was not a String", sdProperties::sdProperty::TypeToString( evaluator->GetReturnType() ) );
		}
		return ExpressionExpression( new sdUIEvaluatorString( evaluator, scope, src ) );
	}

	idStr backup = token;

	idStr message = token;
	while( src->ReadToken( &token ) ) {
		if ( token.linesCrossed ) {
			src->UnreadToken( &token );
			break;
		}
		message += token;
	}
	src->Error( "Invalid Term '%s' in expression '%s' in '%s'", backup.c_str(), message.c_str(), scope->GetUI()->GetName() );
	return -1;
}

/*
=================
sdStringParmExpression::ParseExpressionPriority

Returns a register index
=================
*/
int sdStringParmExpression::ParseExpressionPriority( idLexer *src, int priority, const char* delimiter, sdUserInterfaceScope* scope ) {
	idToken token;
	int		a;

	if ( priority == 0 ) {
		return ParseTerm( src, delimiter, scope );
	}

	a = ParseExpressionPriority( src, priority - 1, delimiter, scope );

	if ( !src->ReadToken( &token ) ) {
		if ( !delimiter ) {
			// we won't get EOF in a real file, but we can
			// when parsing from generated strings
			return a;
		} else {
			src->Error( "Unexpected End of File" );
		}
	}

	if ( delimiter && !token.Icmp( delimiter ) ) {
		src->UnreadToken( &token );
		return a;
	}

	switch( priority ) {
		case 1: {
			src->UnreadToken( &token );
			return a;
		}
		case 2: {
			if ( token == "+" ) {
				return ParseEmitOp( src, a, WOP_TYPE_ADD, priority, delimiter, scope );
			}
			src->UnreadToken( &token );
			return a;
		}
		case 3: {
			src->UnreadToken( &token );
			return a;
		}
		case 4: {
			src->UnreadToken( &token );
		}
	}

	// assume that anything else terminates the expression
	// not too robust error checking...
	if( token == ")" ) {
		src->Error( "Mismatched ')'" );
	}

	src->Error( "Unknown Token '%s'", token.c_str() );

	return a;
}

/*
================
sdStringParmExpression::ParseExpression

Returns a register index
================
*/
int sdStringParmExpression::ParseExpression( idLexer *src, const char* delimiter, sdUserInterfaceScope* scope ) {
	const int TOP_PRIORITY = 4;
	return ParseExpressionPriority( src, TOP_PRIORITY, delimiter, scope );
}

/*
===============
sdStringParmExpression::EvaluateRegisters
===============
*/
void sdStringParmExpression::EvaluateRegisters( void ) {
	int i;

	for ( i = 0 ; i < symbols.Num() ; i++ ) {
		idStr temp;
		registers[ symbols[ i ].position ].value = symbols[ i ].expression->GetStringValue( temp );
	}

	for ( i = 0 ; i < ops.Num() ; i++ ) {
		wexpOp_t& op = ops[ i ];

		switch( op.opType ) {
			case WOP_TYPE_ADD:
				{
					sdStringBuilder_Heap builder;
					builder = registers[ op.a ].value;
					builder += registers[ op.b ].value;
					builder.ToString( registers[ op.c ].value );
					//= builder;//registers[ op.a ].value + registers[ op.b ].value;
				}
				break;
			default:
				common->FatalError( "EvaluateRegisters: bad opcode" );
				break;
		}
	}

}

/*
===============
sdStringParmExpression::~sdStringParmExpression
===============
*/
sdStringParmExpression::~sdStringParmExpression( void ) {
	for ( int i = 0; i < symbols.Num(); i++ ) {
		symbols[ i ].expression->Free();
	}
	symbols.Clear();
}

/*
===============
sdStringParmExpression::EvaluateRegisters
===============
*/
void sdStringParmExpression::Parse( idLexer* src, sdUserInterfaceScope* scope ) {
	inImmediate = 0;

	valueRegister = ParseExpression( src, NULL, scope );
}

/*
===============
sdStringParmExpression::Update
===============
*/
void sdStringParmExpression::Update( void ) {
	EvaluateRegisters();

	if ( outputExpression ) {
		outputExpression->InputChanged();
	}
}

/*
===============
sdStringParmExpression::Attach
===============
*/
void sdStringParmExpression::Attach( sdUIExpression* output ) {
	if ( !immediate ) {
		int i;
		for ( i = 0; i < symbols.Num(); i++ ) {
			if ( symbols[ i ].immediate ) {
				continue;
			}
			symbols[ i ].expression->Attach( this );
		}
	}
	outputExpression = output;
}

/*
===============
sdStringParmExpression::Detach
===============
*/
void sdStringParmExpression::Detach( void ) {
	if ( !immediate ) {
		int i;
		for ( i = 0; i < symbols.Num(); i++ ) {
			if ( symbols[ i ].immediate ) {
				continue;
			}
			symbols[ i ].expression->Detach();
		}
	}
	outputExpression	= NULL;
}

/*
===============
sdStringParmExpression::InputChanged
===============
*/
void sdStringParmExpression::InputChanged( void ) {
	if ( immediate ) {
		return;
	}

	if ( outputExpression ) {
		outputExpression->InputChanged();
	}
}

/*
===============================================================================

	sdPropertyExpressionSingle

===============================================================================
*/

/*
================
sdPropertyExpressionSingle::sdPropertyExpressionSingle
================
*/
sdPropertyExpressionSingle::sdPropertyExpressionSingle( const char* name, sdUIExpression* input ) : outputProperty( NULL ) {
	inputExpression = input;
}

/*
================
sdPropertyExpressionSingle::~sdPropertyExpressionSingle
================
*/
sdPropertyExpressionSingle::~sdPropertyExpressionSingle( void ) {
	if ( outputProperty ) {
		scope->ClearPropertyExpression( scopeKey, scopeIndex );
	}

	inputExpression->Free();
}

/*
================
sdPropertyExpressionSingle::OnOnChangedOverflow
================
*/
void sdPropertyExpressionSingle::OnOnChangedOverflow( void ) {
	sdUserInterfaceScope* propertyScope;
	const char* propertyName = scope->FindPropertyNameByKey( scopeKey, propertyScope );
	if ( propertyName != NULL ) {
		gameLocal.Printf( "Property: %s", propertyName );
		if ( propertyScope != NULL ) {
			gameLocal.Printf( " Scope: %s", propertyScope->GetName() );

			sdUserInterfaceLocal* gui = propertyScope->GetUI();
			if ( gui != NULL ) {
				gameLocal.Printf( " GUI: %s", gui->GetDecl()->GetName() );
			}
		}
		gameLocal.Printf( "\n" );
	}
}

/*
================
sdPropertyExpressionSingle::SetProperty
================
*/
void sdPropertyExpressionSingle::SetProperty( sdProperties::sdProperty* output, int index, int key, sdUserInterfaceScope* outputScope ) {
	outputProperty	= output;
	scopeIndex		= index;
	scopeKey		= key;
	scope			= outputScope;

	inputExpression->Attach( this );

	Update();
}

/*
================
sdPropertyExpressionSingle::Detach
================
*/
void sdPropertyExpressionSingle::Detach( void ) {
	inputExpression->Detach();
	outputProperty	= NULL;
	scope			= NULL;
}

/*
================
sdPropertyExpressionSingle::InputChanged
================
*/
void sdPropertyExpressionSingle::InputChanged( void ) {
	Update();
}

/*
================
sdPropertyExpressionSingle::Update
================
*/
void sdPropertyExpressionSingle::Update( void ) {
	switch ( outputProperty->GetValueType() ) {
		case sdProperties::PT_INT:
			*outputProperty->value.intValue = inputExpression->GetIntValue();
			break;
		case sdProperties::PT_FLOAT:
			*outputProperty->value.floatValue = inputExpression->GetFloatValue();
			break;
		case sdProperties::PT_VEC2:
			{
				idVec2 temp;
				*outputProperty->value.vec2Value = inputExpression->GetVec2Value( temp );
			}
			break;
		case sdProperties::PT_VEC3:
			{
				idVec3 temp;
				*outputProperty->value.vec3Value = inputExpression->GetVec3Value( temp );
			}
			break;
		case sdProperties::PT_VEC4:
			{
				idVec4 temp;
				*outputProperty->value.vec4Value = inputExpression->GetVec4Value( temp );
			}
			break;
		case sdProperties::PT_STRING:
			{
				idStr temp;
				*outputProperty->value.stringValue = inputExpression->GetStringValue( temp );
			}
			break;
		case sdProperties::PT_WSTRING:
			{
				idWStr temp;
				*outputProperty->value.wstringValue = inputExpression->GetWStringValue( temp );
			}
			break;
	}
}



/*
===============================================================================

	sdPropertyExpressionField

===============================================================================
*/

/*
================
sdPropertyExpressionField::sdPropertyExpressionField
================
*/
sdPropertyExpressionField::sdPropertyExpressionField( const char* name, sdUIExpression* input, int index ) : sdPropertyExpressionSingle( name, input ) {
	outputField = index;
}

/*
================
sdPropertyExpressionField::Update
================
*/
void sdPropertyExpressionField::Update( void ) {
	switch ( outputProperty->GetValueType() ) {
		case sdProperties::PT_FLOAT:
			*outputProperty->value.floatValue = inputExpression->GetFloatValue();
			break;
		case sdProperties::PT_VEC2:
			outputProperty->value.vec2Value->SetIndex( outputField, inputExpression->GetFloatValue() );
			break;
		case sdProperties::PT_VEC3:
			outputProperty->value.vec3Value->SetIndex( outputField, inputExpression->GetFloatValue() );
			break;
		case sdProperties::PT_VEC4:
			outputProperty->value.vec4Value->SetIndex( outputField, inputExpression->GetFloatValue() );
			break;
	}
}

/*
===============================================================================

	sdUIEvaluatorType

===============================================================================
*/

/*
================
sdUIEvaluatorType::sdUIEvaluatorType
================
*/
#define UI_EVALUATOR_TYPE_DEFINITION( TYPENAME )																			\
sdUIEvaluatorType##TYPENAME::sdUIEvaluatorType##TYPENAME( const char* _parms, const char* _name, evaluationFunc_t func ) {	\
	isVariadic = false;																										\
	int len = idStr::Length( _parms );																						\
	int i;																													\
	for ( i = 0; i < len; i++ ) {																							\
		switch ( _parms[ i ] ) {																							\
			case 'i': parms.Append( sdProperties::PT_INT ); break;															\
			case 'f': parms.Append( sdProperties::PT_FLOAT ); break;														\
			case '2': parms.Append( sdProperties::PT_VEC2 ); break;															\
			case '3': parms.Append( sdProperties::PT_VEC3 ); break;															\
			case '4': parms.Append( sdProperties::PT_VEC4 ); break;															\
			case 's': parms.Append( sdProperties::PT_STRING ); break;														\
			case 'w': parms.Append( sdProperties::PT_WSTRING ); break;														\
			case '#': isVariadic = true; break;																				\
			default: gameLocal.Error( "Invalid Paramter Type '%c'", _parms[ i ] ); break;	\
		}																													\
	}																														\
	function = func;																										\
	name = _name;																											\
}

UI_EVALUATOR_TYPE_DEFINITION( Int )
UI_EVALUATOR_TYPE_DEFINITION( Float )
UI_EVALUATOR_TYPE_DEFINITION( Vec2 )
UI_EVALUATOR_TYPE_DEFINITION( Vec3 )
UI_EVALUATOR_TYPE_DEFINITION( Vec4 )
UI_EVALUATOR_TYPE_DEFINITION( String )
UI_EVALUATOR_TYPE_DEFINITION( WString )

/*
===============================================================================

	sdUIEvaluator

===============================================================================
*/

/*
================
sdUIEvaluator::sdUIEvaluator
================
*/
sdUIEvaluator::sdUIEvaluator( sdUIEvaluatorTypeBase* _type, sdUserInterfaceScope* _scope, idLexer* src ) : type( _type ), scope( _scope ), outputExpression( NULL ) {
	using namespace sdProperties;

	int count = type->GetNumParms();

	idStr temp;
	src->ParseBracedSection( temp, -1, true, '(', ')' );

	idLexer parser( temp, temp.Length(), "sdUIExpression" );
	parser.ExpectTokenString( "(" );

	idStr value;
	idStrList terms;
	sdUIScriptEvent::ReadExpression( &parser, value, terms, "(", ")", true );

	if ( terms.Num() != count && !IsVariadic() ) {
		src->Error( "Invalid Number of Parameters for '%s'", type->GetName() );
		return;
	}

	int i;
	for ( i = 0; i < terms.Num(); i++ ) {
		sdUIExpression* exp = NULL;
		ePropertyType parmType;
		if( i < count ) {
			parmType = type->GetParmType( i );
		} else if( IsVariadic() ) {
			parmType = type->GetReturnType();
		} else {
			break;
		}

		switch ( parmType ) {
			case PT_INT:
				exp = sdUIExpression::AllocIntExpression( _scope, terms[ i ] );
				break;
			case PT_FLOAT:
				exp = sdUIExpression::AllocFloatExpression( _scope, terms[ i ] );
				break;
			case PT_STRING:
				exp = sdUIExpression::AllocStringExpression( _scope, terms[ i ] );
				break;
			case PT_WSTRING:
				exp = sdUIExpression::AllocWStringExpression( _scope, terms[ i ] );
				break;
			case PT_VEC2:
			case PT_VEC3:
			case PT_VEC4:
				exp = sdUIExpression::AllocSingleParmExpression( parmType, _scope, terms[ i ] );
				break;
			default:
				src->Error( "Unknown Parameter Type on '%s' index '%i'", type->GetName(), i );
				break;
		}
		parms.Alloc() = exp;
	}
}

/*
================
sdUIEvaluator::~sdUIEvaluator
================
*/
sdUIEvaluator::~sdUIEvaluator( void ) {
	for ( int i = 0; i < parms.Num(); i++ ) {
		parms[ i ]->Free();
	}
	parms.Clear();
}

/*
================
sdUIEvaluator::Attach
================
*/
void sdUIEvaluator::Attach( sdUIExpression* output ) {
	outputExpression = output;

	int i;
	for ( i = 0; i < parms.Num(); i++ ) {
		parms[ i ]->Attach( this );
	}
}

/*
================
sdUIEvaluator::Detach
================
*/
void sdUIEvaluator::Detach( void ) {
	outputExpression = NULL;

	int i;
	for ( i = 0; i < parms.Num(); i++ ) {
		parms[ i ]->Detach();
	}
}


