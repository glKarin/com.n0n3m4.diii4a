// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../../idlib/PropertiesImpl.h"
#include "UserInterfaceScript.h"
#include "UserInterfaceTypes.h"
#include "UserInterfaceExpressions.h"
#include "UserInterfaceLocal.h"

/*
===============================================================================

	sdUIScriptEvent

===============================================================================
*/
/*
================
sdUIScriptEvent::sdUIScriptEvent
================
*/
sdUIScriptEvent::sdUIScriptEvent( void ) : ops(1) {
}

/*
================
sdUIScriptEvent::ReadQuotes
================
*/
const char* sdUIScriptEvent::ReadSection( char breakon, const char* p ) {
	const char* s;
	for ( s = p; *s; s++ ) {
		if ( *s == breakon ) {
			break;
		}
		if ( *s == '(' ) {
			s = ReadSection( ')', s + 1 );
		} else if ( *s == '"' ) {
			s = ReadSection( '"', s + 1 );
		}
	}
	return s;
}

/*
================
sdUIScriptEvent::ReadExpression
================
*/
void sdUIScriptEvent::ReadExpression( idLexer* src, idStr& value, idStrList& list, const char* start, const char* terminator, bool allowEmpty, int initialDepth ) {
	int depth = initialDepth;

	sdStringBuilder_Heap builder;

	idToken token;
	while ( true ) {
		if ( !src->ReadToken( &token ) ) {
			if ( terminator ) {
				src->Error( "Unexpected End of File" );
			} else {
				break;
			}
		}

		if ( start && !token.Icmp( start ) && token.type != TT_STRING ) {
			depth++;
			builder += token.c_str();
			builder += " ";
			continue;
		} else if ( terminator && !token.Icmp( terminator ) && token.type != TT_STRING ) {
			depth--;
			if ( depth == 0 ) {
				if ( !allowEmpty && !builder.Length() ) {
					src->Error( "Unexpected '%s'", terminator );
				}
				break;
			}
			builder += token.c_str();
			builder += " ";
			continue;
		} else {
			if ( token.type == TT_STRING ) {
				builder += "\"" + token + "\"";
			} else {
				builder += token.c_str();
			}
			builder += " ";
		}
	}
	builder.ToString( value );

	if ( builder.Length() ) {
		const char* start = value;
		const char* end = start;
		do {
			end = ReadSection( ',', start );
			idStr temp( start, 0, end - start );
			temp.StripLeading( ' ' );
			temp.StripTrailing( ' ' );
			list.Append( temp );
			start = end + 1;
		} while ( *end );
	}
}

/*
================
sdUIScriptEvent::EmitOpCode
================
*/
#if 0
int opcount[7] = {0,0,0,0,0,0,0};
int possibleDataSize = 0;
#endif
int sdUIScriptEvent::EmitOpCode( opCode_t code, int parm1, int parm2, int parm3, sdUserInterfaceScope* assigmentScope, const char* opName ) {

	eventOp_t& op = ops.Alloc();

	if ( parm1 < 0 || parm1 > USHRT_MAX ) {
		gameLocal.Error( "sdUIScriptEvent::EmitOpCode Parm1 Out of Range" );
	}

	if ( parm2 < 0 || parm2 > USHRT_MAX ) {
		gameLocal.Error( "sdUIScriptEvent::EmitOpCode Parm2 Out of Range" );
	}

	if ( parm3 < 0 || parm3 > USHRT_MAX ) {
		gameLocal.Error( "sdUIScriptEvent::EmitOpCode Parm3 Out of Range" );
	}

	op.op				= ( unsigned short )code;
	op.parm1			= ( unsigned short )parm1;
	op.parm2			= ( unsigned short )parm2;
	op.parm3			= ( unsigned short )parm3;

	op.assigmentScope	= assigmentScope;
	//op.opName			= opName;
#if 0
	opcount[op.op]++;
	possibleDataSize++;
	switch (op.op) {
		case EO_NOOP:
			break;
		case EO_ASSIGN_PROPERTY_EXPRESSION:
			possibleDataSize += 4 * 4;
			break;
		case EO_FUNCTION:
			possibleDataSize += 4 * 1;
			break;
		case EO_CALL_EVENT:
			possibleDataSize += 4 * 1;
			break;
		case EO_IF:
			possibleDataSize += 4 * 3;
			break;
		case EO_BREAK:
			break;
		case EO_RETURN:
			break;
	}
#endif
	return ops.Num() - 1;
}

/*
================
sdUIScriptEvent::Run
================
*/
bool sdUIScriptEvent::Run( sdUIScript* script, sdUserInterfaceScope* scope, int offset, int num ) {	
	if( num < 0 ) {
		num = ops.Num();
	}

	bool retVal = true;
	for ( int i = offset; i < offset + num; i++ ) {
		switch ( ops[ i ].op ) {
			case EO_NOOP:
				break;
			case EO_ASSIGN_PROPERTY_EXPRESSION:
				ops[ i ].assigmentScope->SetPropertyExpression( ops[ i ].parm1, ops[ i ].parm2, scope->GetExpression( ops[ i ].parm3 ) );
				break;
			case EO_FUNCTION:
				scope->RunFunction( ops[ i ].parm3 );
				break;
			case EO_CALL_EVENT:
				script->RunEventHandle( ops[ i ].parm1, scope );
				break;
			case EO_IF: {
					sdUIExpression* condition = scope->GetExpression( ops[ i ].parm1 );
					if( condition->GetFloatValue( ) && ops[ i ].parm2 ) {
						retVal = Run( script, scope, i + 1, ops[ i ].parm2 );
					} else if( ops[ i ].parm3 ) {
						retVal = Run( script, scope, i + 1 + ops[ i ].parm2, ops[ i ].parm3 );						
					}
					if( retVal ) {
						i += ops[ i ].parm2 + ops[ i ].parm3;
					} else {
						i = offset + num;	//terminate
					}
				}
				break;
			case EO_BREAK:
				assert( !"EO_BREAK" );
				break;
			case EO_RETURN:
				i = offset + num;
				retVal = false;
				break;
		}
	}
	return retVal;
}

/*
================
sdUIScriptEvent::GetPropertyField
================
*/
int sdUIScriptEvent::GetPropertyField( sdProperties::ePropertyType type, idLexer* src ) {
	int numPropertyFields = sdProperties::CountForPropertyType( type );
	int propertyField = -1;

	idToken token;
	if ( src->ReadToken( &token ) == 0 ) {
		return propertyField;
	}
	if ( !token.Icmp( "." ) ) {
		if ( numPropertyFields <= 1 ) {
			gameLocal.Error( "Field Access Only Valid on Vectors" );
		}
		src->ReadToken( &token );
		if ( token.Length() == 1 ) {
			switch( idStr::ToLower( token[ 0 ] ) ) {
				case 'x':
				case 'r':
					propertyField = 0;
					break;

				case 'y':
				case 'g':
					propertyField = 1;
					break;

				case 'z':
				case 'b':
				case 'w':
					propertyField = 2;
					break;

				case 'h':
				case 'a':
					propertyField = 3;
					break;
			}
		} else {
			if( !idStr::Icmp( "red", token ) || !idStr::Icmp( "left", token ) ) {
				propertyField = 0;
			} else if( !idStr::Icmp( "green", token ) || !idStr::Icmp( "top", token ) ) {
				propertyField = 1;
			} else if( !idStr::Icmp( "blue", token ) || !idStr::Icmp( "width", token ) ) {
				propertyField = 2;
			} else if( !idStr::Icmp( "alpha", token ) || !idStr::Icmp( "height", token ) ) {
				propertyField = 3;
			} 
		}
		if ( propertyField == -1 || propertyField >= numPropertyFields ) {
			gameLocal.Error( "Invalid Field '%s' on Vector", token.c_str() );
		}
	} else {
		src->UnreadToken( &token );
	}

	return propertyField;
}

/*
================
sdUIScriptEvent::ParsePropertyExpression
================
*/
void sdUIScriptEvent::ParsePropertyExpression( idLexer* src, sdProperties::sdProperty* property, const char* propertyName, sdUserInterfaceScope* scope, sdUserInterfaceScope* propertyScope ) {
	
	idStr value;
	try {
		int propertyIndex = propertyScope->IndexForProperty( property );
		if ( propertyIndex == -1 ) {
			src->Error( "Tried to Modify a Read-Only Property '%s'", propertyName );
		}

		int numPropertyFields = sdProperties::CountForPropertyType( property->GetValueType() );
		int propertyField = -1;

		idToken token;

		propertyField = GetPropertyField( property->GetValueType(), src );

		src->ExpectTokenString( "=" );

		if ( !src->ReadToken( &token ) ) {
			gameLocal.Error( "Right Hand Side of Expression Missing" );
		}

		if ( !token.Icmp( "transition" ) ) {
			sdUIExpression* exp;
			
			if ( propertyField == -1 ) {
				exp = new sdPropertyExpressionSingle( propertyName, sdUIExpression::AllocTransition( property->GetValueType(), scope, src ) );
			} else {
				exp = new sdPropertyExpressionField( propertyName, sdUIExpression::AllocTransition( sdProperties::PT_FLOAT, scope, src ), propertyField );
			}

			src->ExpectTokenString( ";" );

			EmitOpCode( EO_ASSIGN_PROPERTY_EXPRESSION, propertyIndex, propertyField == -1 ? 0 : propertyField, scope->AddExpression( exp ), propertyScope, va( "%s transiton", propertyName ) );

			return;

		} else {
			src->UnreadToken( &token );
		}
		
		idStrList list;
		ReadExpression( src, value, list );

		if ( property->GetValueType() == sdProperties::PT_STRING ) {

			if ( list.Num() != 1 ) {
				gameLocal.Error( "Invalid Expression '%s' for Property", value.c_str() );
			}

			EmitOpCode( EO_ASSIGN_PROPERTY_EXPRESSION, propertyIndex, 0, scope->AddExpression( new sdPropertyExpressionSingle( propertyName, sdUIExpression::AllocStringExpression( scope, list[ 0 ] ) ) ), propertyScope, va( "%s = %s", propertyName, list[ 0 ].c_str() ) );

			return;

		} else if ( property->GetValueType() == sdProperties::PT_FLOAT ) {

			if ( list.Num() != 1 ) {
				gameLocal.Error( "Invalid Expression '%s' for Property", value.c_str() );
			}

			EmitOpCode( EO_ASSIGN_PROPERTY_EXPRESSION, propertyIndex, 0, scope->AddExpression( new sdPropertyExpressionSingle( propertyName, sdUIExpression::AllocFloatExpression( scope, list[ 0 ] ) ) ), propertyScope );

			return;

		} else {

			if ( list.Num() == 1 ) {
				if ( propertyField == -1 ) {
					EmitOpCode( EO_ASSIGN_PROPERTY_EXPRESSION, propertyIndex, 0, scope->AddExpression( new sdPropertyExpressionSingle( propertyName, sdUIExpression::AllocSingleParmExpression( property->GetValueType(), scope, list[ 0 ] ) ) ), propertyScope );
				} else {
					EmitOpCode( EO_ASSIGN_PROPERTY_EXPRESSION, propertyIndex, propertyField, scope->AddExpression( new sdPropertyExpressionField( propertyName, sdUIExpression::AllocFloatExpression( scope, list[ 0 ] ), propertyField ) ), propertyScope );
				}

				return;

			} else if ( list.Num() == numPropertyFields ) {

				int j;
				for ( j = 0; j < numPropertyFields; j++ ) {
					EmitOpCode( EO_ASSIGN_PROPERTY_EXPRESSION, propertyIndex, j, scope->AddExpression( new sdPropertyExpressionField( propertyName, sdUIExpression::AllocFloatExpression( scope, list[ j ] ), j ) ), propertyScope );
				}

				return;

			} else {
				gameLocal.Error( "Invalid Expression '%s' for Property", value.c_str() );
			}
		}
	} catch( idException& ) {
		gameLocal.Printf( "Failed to parse expression ^7'%s' = '%s'^0\n", propertyName, value.c_str() );
		throw;
	}
}

/*
================
sdUIScriptEvent::ParseEvent
================
*/
void sdUIScriptEvent::ParseEvent( idLexer* src, const sdUIEventInfo& info, sdUserInterfaceScope* scope ) {
	src->ExpectTokenString( "{" );

	idToken token;

	while ( true ) {
		if ( !src->ReadToken( &token ) ) {
			src->Error( "Unexpected End of Event" );
		} else {

			if( token.Icmp( "breakonparse" ) == 0 ) {
				assert( 0 );
				continue;
			}

			if ( !token.Cmp( ";" ) ) {
				// skip empty expressions
				continue;
			}

			if ( !token.Icmp( "callSuper" ) ) {
				src->ExpectTokenString( "(" );
				src->ExpectTokenString( ")" );
				src->ExpectTokenString( ";" );

				sdUIEventHandle currentInfo = scope->GetEvent( info );
				if ( currentInfo.IsValid() ) {
					EmitOpCode( EO_CALL_EVENT, currentInfo );
				}

				continue;
			}

			if ( !token.Icmp( "breakpoint" ) ) {
				src->ExpectTokenString( ";" );
				EmitOpCode( EO_BREAK );
				continue;
			}

			if ( !token.Icmp( "return" ) ) {
				src->ExpectTokenString( ";" );
				EmitOpCode( EO_RETURN );
				continue;
			}

			if ( !token.Icmp( "if" ) ) {
				idStr expr;
				src->ParseBracedSection( expr, -1, true, '(', ')' );
				expr.StripLeadingOnce( "(" );
				expr.StripTrailingOnce( ")" );
				int expressionIndex = scope->AddExpression( sdUIExpression::AllocFloatExpression( scope, expr ) );
				
				int ifOpcode = EmitOpCode( EO_IF, expressionIndex, 0, 0 );

				int trueBlockOpCount = ops.Num();
				int falseBlockOpCount = 0;
				ParseEvent( src, info, scope );

				trueBlockOpCount = ops.Num() - trueBlockOpCount;

				src->ReadToken( &token );
				if ( !token.Icmp( "else" ) ) {
					falseBlockOpCount = ops.Num();
					ParseEvent( src, info, scope );
					falseBlockOpCount = ops.Num() - falseBlockOpCount;					                 
				} else {
					src->UnreadToken( &token );
				}

				// fix up the opcode with the proper number of statements
				ops[ ifOpcode ].parm2 = trueBlockOpCount;
				ops[ ifOpcode ].parm3 = falseBlockOpCount;

				continue;
			}

			if ( !token.Cmp( "}" ) ) {
				break;
			}
			src->UnreadToken( &token );
		}

		sdUserInterfaceScope* altScope = gameLocal.GetUserInterfaceScope( *scope, src );

		src->ReadToken( &token );

		sdProperties::sdProperty* property = altScope->GetProperty( token );
		if ( property ) {
			ParsePropertyExpression( src, property, token, scope, altScope );
			continue;
		}

		sdUIFunctionInstance* function = altScope->GetFunction( token );
		if ( function ) {
			int expressionIndex = scope->AddExpression( sdUIExpression::AllocFunctionExpression( token, function, scope, src ) );
			src->ExpectTokenString( ";" );
			EmitOpCode( EO_FUNCTION, USHRT_MAX, USHRT_MAX, expressionIndex );
			continue;
		}

		src->Error( "Unexpected Token '%s'", token.c_str() );
	}
}

/*
===============================================================================

	sdUIScript

===============================================================================
*/

/*
================
sdUIScript::sdUIScript
================
*/
sdUIScript::sdUIScript( void ) {
}

/*
================
sdUIScript::~sdUIScript
================
*/
sdUIScript::~sdUIScript( void ) {
	Clear();
}

/*
================
sdUIScript::ParseEvent
================
*/
void sdUIScript::ParseEvent( idLexer* src, const sdUIEventInfo& info, sdUserInterfaceScope* scope ) {
	sdUIScriptEvent* event = new sdUIScriptEvent();
	event->ParseEvent( src, info, scope );

	sdUIEventHandle scriptHandle = events.Append( event );
	scope->AddEvent( info, scriptHandle );
}

/*
================
sdUIScript::RunEventHandle
================
*/
bool sdUIScript::RunEventHandle( sdUIEventHandle handle, sdUserInterfaceScope* scope ) {
	if( !handle.IsValid() ) {
		return false;
	}

	if( handle >= events.Num() ) {
		gameLocal.Warning( "sdUIScript::RunEventHandle: '%s' handle '%i' out of range.", scope->GetUI()->GetName(), (int)handle );
		return false;
	}

	sdUIScriptEvent* event = events[ handle ]; 

	event->Run( this, scope );

	return true;
}

/*
================
sdUIScript::Clear
================
*/
void sdUIScript::Clear( void ) {
	events.DeleteContents( true );
}

