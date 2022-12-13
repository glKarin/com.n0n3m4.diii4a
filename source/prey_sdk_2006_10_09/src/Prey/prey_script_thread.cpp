#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( idThread, hhThread )
END_CLASS

hhThread::hhThread() : idThread() {}
hhThread::hhThread( idEntity *self, const function_t *func ) : idThread( self, func ) {}
hhThread::hhThread( const function_t *func ) : idThread( func ) {}
hhThread::hhThread( idInterpreter *source, const function_t *func, int args ) : idThread( source, func, args ) {}

/*
================
hhThread::PushParm
================
*/
void hhThread::PushParm( int value ) {
	interpreter.Push( value );
}

/*
================
hhThread::PushString
================
*/
void hhThread::PushString( const char *text ) {
	PushParm( (int)new idStr(text) );
}

/*
================
hhThread::PushFloat
================
*/
void hhThread::PushFloat( float value ) {
	PushParm( *(int*)&value );
}

/*
================
hhThread::PushInt
================
*/
void hhThread::PushInt( int value ) {
	PushParm( value );
}

/*
================
hhThread::PushVector
================
*/
void hhThread::PushVector( const idVec3 &vec ) {
	float val = 0.0f;
	for( int ix = 0; ix < vec.GetDimension(); ++ix ) {
		val = vec[ ix ];
		PushParm( *(int*)&val );
	}
}

/*
================
hhThread::PushEntity
================
*/
void hhThread::PushEntity( const idEntity *ent ) {
	HH_ASSERT( ent );

	PushParm( ent->entityNumber + 1 );
}

/*
================
hhThread::ClearStack
================
*/
void hhThread::ClearStack() {
	interpreter.Reset();
}

/*
================
hhThread::ParseAndPushArgsOntoStack
================
*/
bool hhThread::ParseAndPushArgsOntoStack( const idCmdArgs &args, const function_t* function ) {
	idList<idStr>	parmList;

	hhUtils::SplitString( args, parmList );

	return ParseAndPushArgsOntoStack( parmList, function );
}

/*
================
hhThread::ParseAndPushArgsOntoStack
================
*/
bool hhThread::ParseAndPushArgsOntoStack( const idList<idStr>& args, const function_t* function ) {
	int numParms = function->def->TypeDef()->NumParameters();
	idTypeDef* parmType = NULL;
	const char* parm = NULL;

	for( int ix = 0; ix < numParms; ++ix ) {
		parmType = function->def->TypeDef()->GetParmType( ix );
		parm = args[ ix ].c_str();

		parmType->PushOntoStack( parm, this );
	}

	return true;
}