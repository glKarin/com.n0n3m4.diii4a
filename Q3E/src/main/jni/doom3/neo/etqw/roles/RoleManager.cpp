// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "RoleManager.h"
#include "../structures/TeamManager.h"

/*
===============================================================================

	sdRequirementCondition

===============================================================================
*/

/*
================
sdRequirementCondition::sdRequirementCondition
================
*/
sdRequirementCondition::sdRequirementCondition( const char* requirement ) {
	idLexer parser( requirement, idStr::Length( requirement ), "sdRequirementCondition::sdRequirementCondition" );
	valueRegister = ParseExpression( &parser );
}

/*
================
sdRequirementCondition::ExpressionOp
================
*/
sdRequirementCondition::rexpOp_t& sdRequirementCondition::ExpressionOp( void ) {
	rexpOp_t& wop = ops.Alloc();
	memset( &wop, 0, sizeof( wop ) );
	return wop;
}

/*
================
sdRequirementCondition::ExpressionTemporary
================
*/
int sdRequirementCondition::ExpressionTemporary( void ) {
	int i = registers.Num();
	expressionRegister_t& r = registers.Alloc();
	r.temporary				= true;
	r.value					= false;
	return i;
}

/*
================
sdRequirementCondition::EmitOp
================
*/
int sdRequirementCondition::EmitOp( int a, int b, rexpOpType_t opType, rexpOp_t** opp ) {
	rexpOp_t& op = ExpressionOp();
	op.opType = opType;
	op.a = a;
	op.b = b;
	op.c = ExpressionTemporary();

	if ( opp ) {
		*opp = &op;
	}
	return op.c;
}

/*
================
sdRequirementCondition::ParseEmitOp
================
*/
int sdRequirementCondition::ParseEmitOp( idLexer *src, int a, rexpOpType_t opType, int priority, rexpOp_t** opp ) {
	int b = ParseExpressionPriority( src, priority );
	return EmitOp( a, b, opType, opp );  
}

/*
================
sdRequirementCondition::ParseExpression
================
*/
int sdRequirementCondition::ParseExpression( idLexer *src ) {
	const int TOP_PRIORITY = 2;
	return ParseExpressionPriority( src, TOP_PRIORITY );
}

/*
================
sdRequirementCondition::ExpressionConstant
================
*/
int sdRequirementCondition::ExpressionConstant( bool v ) {
	int		i;

	for ( i = 0; i < registers.Num(); i++ ) {
		if ( !registers[ i ].temporary && registers[ i ].value == v ) {
			return i;
		}
	}
	
	i = registers.Num();
	expressionRegister_t& r = registers.Alloc();
	r.temporary = false;
	r.value		= v;
	return i;
}

/*
================
sdRequirementCondition::ParseTerm
=================
*/
int sdRequirementCondition::ParseTerm( idLexer *src ) {
	idToken token;
	int	a;

	src->ReadToken( &token );

	if ( token == "(" ) {
		a = ParseExpression( src );
		src->ExpectTokenString( ")" );
		return a;
	}
	
	if ( !token.Icmp( "true" ) ) {
		return ExpressionConstant( true );
	}
	
	if ( !token.Icmp( "false" ) ) {
		return ExpressionConstant( false );
	}

	const sdDeclRequirement* requirement = gameLocal.declRequirementType[ token ];
	if ( requirement ) {
		return ExpressionRequirement( requirement );
	}

	src->Error( "sdRequirementCondition::ParseTerm Invalid Term '%s'", token.c_str() );
	return -1;
}

/*
================
sdRequirementCondition::ExpressionRequirement
================
*/
int sdRequirementCondition::ExpressionRequirement( const sdDeclRequirement* requirement ) {
	int i = ExpressionTemporary();
	expressionRequirement_t& er = requirements.Alloc();
	er.requirement		= requirement;
	er.index			= i;
	return i;
}

/*
=================
sdRequirementCondition::ParseExpressionPriority
=================
*/
int sdRequirementCondition::ParseExpressionPriority( idLexer *src, int priority ) {
	idToken token;
	int		a = -1;

	if ( priority == 0 ) {
		return ParseTerm( src );
	}

	a = ParseExpressionPriority( src, priority - 1 );

	if ( !src->ReadToken( &token ) ) {
		return a;
	}

	switch( priority ) {
		case 1: {
			if ( token == "==" ) {
				return ParseEmitOp( src, a, ROP_TYPE_EQ, priority );
			}
			if ( token == "!=" ) {
				return ParseEmitOp( src, a, ROP_TYPE_NE, priority );
			}
			src->UnreadToken( &token );
			return a;
		}
		case 2: {
			if ( token == "&&" ) {
				return ParseEmitOp( src, a, ROP_TYPE_AND, priority );
			}
			if ( token == "||" ) {
				return ParseEmitOp( src, a, ROP_TYPE_OR, priority );
			}
			src->UnreadToken( &token );
			return a;	// FIXME: this is not correct, but I cba fixing this parser atm
		}
	}

	// assume that anything else terminates the expression
	// not too robust error checking...

	src->Error( "sdRequirementCondition::ParseExpressionPriority Unknown Token '%s'", token.c_str() );

	return a;
}

/*
================
sdRequirementCondition::Evaluate
================
*/
bool sdRequirementCondition::Evaluate( idEntity* main, idEntity* other ) {

	int i;
	for ( i = 0; i < requirements.Num(); i++ ) {
		registers[ requirements[ i ].index ].value = requirements[ i ].requirement->Check( main, other );
	}

	for ( i = 0 ; i < ops.Num() ; i++ ) {
		rexpOp_t& op = ops[ i ];

		switch( op.opType ) {
			case ROP_TYPE_EQ:
				registers[ op.c ].value = registers[ op.a ].value == registers[ op.b ].value;
				break;
			case ROP_TYPE_NE:
				registers[ op.c ].value = registers[ op.a ].value != registers[ op.b ].value;
				break;
			case ROP_TYPE_AND:
				registers[ op.c ].value = registers[ op.a ].value && registers[ op.b ].value;
				break;
			case ROP_TYPE_OR:
				registers[ op.c ].value = registers[ op.a ].value || registers[ op.b ].value;
				break;
			default:
				gameLocal.Error( "sdFloatParmExpression::EvaluateRegisters: bad opcode" );
				break;
		}
	}

	return registers[ valueRegister ].value;
}

/*
===============================================================================

	sdRequirementContainer

===============================================================================
*/

/*
================
sdRequirementContainer::Load
================
*/
void sdRequirementContainer::Load( const idDict& requirementList, const char* mode ) {
	const idKeyValue* kv = NULL;
	while( kv = requirementList.MatchPrefix( mode, kv ) ) {
		if( kv->GetValue().Length() <= 0 ) {
			continue;
		}
		Load( kv->GetValue().c_str() );
	}
}

/*
================
sdRequirementContainer::Load
================
*/
void sdRequirementContainer::Load( const char* requirement ) {	
	if ( !*requirement ) {
		return;
	}
	Alloc() = new sdRequirementCondition( requirement );
}

/*
================
sdRequirementContainer::~sdRequirementContainer
================
*/
sdRequirementContainer::~sdRequirementContainer( void ) {
	DeleteContents( true );
}

/*
================
sdRequirementContainer::Check
================
*/
bool sdRequirementContainer::Check( idEntity* main, idEntity* other ) const {

	int i;
	for( i = 0; i < Num(); i++ ) {
		if( !( *this )[ i ]->Evaluate( main, other ) ) {
			return false;
		}
	}

	// All requirement checks passed, let 'em have it
	return true;
}

/*
===============================================================================

	sdAbilityProvider

===============================================================================
*/

/*
================
sdAbilityProvider::Add
================
*/
void sdAbilityProvider::Add( const char* abilityName ) {
	Alloc() = sdRequirementManager::GetInstance().RegisterAbility( abilityName );
}

/*
===============================================================================

	sdRequirementManagerLocal

===============================================================================
*/

/*
================
sdRequirementManagerLocal::sdRequirementManagerLocal
================
*/
sdRequirementManagerLocal::sdRequirementManagerLocal( void ) {
}

/*
================
sdRequirementManagerLocal::~sdRequirementManagerLocal
================
*/
sdRequirementManagerLocal::~sdRequirementManagerLocal( void ) {
	Shutdown();
}

/*
================
sdRequirementManagerLocal::Init
================
*/
void sdRequirementManagerLocal::Init( void ) {
}

/*
================
sdRequirementManagerLocal::Shutdown
================
*/
void sdRequirementManagerLocal::Shutdown( void ) {
	registeredAbilities.Clear();
}

/*
================
sdRequirementManagerLocal::RegisterRequirement
================
*/
qhandle_t sdRequirementManagerLocal::RegisterAbility( const char* name ) {
	qhandle_t* handle = NULL;
	if ( registeredAbilities.Get( name, &handle ) ) {
		return *handle;
	}	

	return registeredAbilities.Set( name, ( qhandle_t )registeredAbilities.Num() );
}
