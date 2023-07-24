// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_ROLES_ROLEMANAGER_H__
#define __GAME_ROLES_ROLEMANAGER_H__

class idEntity;
class sdDeclRequirement;

class sdAbilityProvider : protected idList< qhandle_t > {
public:
	void							Add( const char* abilityName );
	bool							HasAbility( qhandle_t handle ) const { return FindIndex( handle ) != -1; }
	void							Clear( void ) { idList< qhandle_t >::Clear(); }
};

class sdRequirementCondition {
public:
	typedef enum rexpOpType_e {
		ROP_TYPE_EQ,
		ROP_TYPE_NE,
		ROP_TYPE_AND,
		ROP_TYPE_OR,
	} rexpOpType_t;	
	
	typedef struct rexpOp_s {
		rexpOpType_t opType;	
		int	a, b, c;
	} rexpOp_t;

	typedef struct expressionRegister_s {
		bool						value;
		bool						temporary;
	} expressionRegister_t;

	typedef struct expressionRequirement_s {
		int							index;
		const sdDeclRequirement*	requirement;
	} expressionRequirement_t;

									sdRequirementCondition( const char* requirement );
	rexpOp_t&						ExpressionOp( void );
	int								ParseExpressionPriority( idLexer *src, int priority );
	int								ExpressionTemporary( void );
	int								ExpressionConstant( bool v );
	int								ExpressionRequirement( const sdDeclRequirement* requirement );
	int								ParseExpression( idLexer *src );
	int								ParseTerm( idLexer *src );
	int								EmitOp( int a, int b, rexpOpType_t opType, rexpOp_t** opp );
	int								ParseEmitOp( idLexer *src, int a, rexpOpType_t opType, int priority, rexpOp_t** opp = NULL );

	bool							Evaluate( idEntity* main, idEntity* other );

private:
	idList< expressionRequirement_t >	requirements;
	idList< rexpOp_t >					ops;
	idList< expressionRegister_t >		registers;
	int									valueRegister;
};

class sdRequirementContainer : protected idList< sdRequirementCondition* > {
public:
									~sdRequirementContainer( void );

	void							Load( const idDict& requirementList, const char* mode );
	void							Load( const char* requirement );
	bool							HasRequirements( void ) const { return Num() > 0; }
	bool							Check( idEntity* main, idEntity* other = NULL ) const;
	void							Clear( void ) { idList< sdRequirementCondition* >::DeleteContents( true ); }
};

class sdRequirementManagerLocal {
public:
									sdRequirementManagerLocal( void );
									~sdRequirementManagerLocal( void );

	qhandle_t						RegisterAbility( const char* name );

	void							Init( void );
	void							Shutdown( void );	

private:
	idHashMap< qhandle_t >			registeredAbilities;
};

typedef sdSingleton< sdRequirementManagerLocal > sdRequirementManager;

#endif // __GAME_ROLES_ROLEMANAGER_H__
