#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

/*
================
rvScriptFuncUtility::rvScriptFuncUtility
================
*/
rvScriptFuncUtility::rvScriptFuncUtility() {
	func = NULL;
	parms.Clear();
}

/*
================
rvScriptFuncUtility::rvScriptFuncUtility
================
*/
rvScriptFuncUtility::rvScriptFuncUtility( const rvScriptFuncUtility* sfu ) {
	Assign( sfu );
}

/*
================
rvScriptFuncUtility::rvScriptFuncUtility
================
*/
rvScriptFuncUtility::rvScriptFuncUtility( const rvScriptFuncUtility& sfu ) {
	Assign( &sfu );
}

/*
================
rvScriptFuncUtility::rvScriptFuncUtility
================
*/
rvScriptFuncUtility::rvScriptFuncUtility( const char* source ) {
	Init( source );
}

/*
================
rvScriptFuncUtility::rvScriptFuncUtility
================
*/
rvScriptFuncUtility::rvScriptFuncUtility( const idCmdArgs& args ) {
	Init( args );
}

/*
================
rvScriptFuncUtility::Init
================
*/
sfuReturnType rvScriptFuncUtility::Init( const char* source ) {
	Clear();

	if( !source || !source[0] ) {
		return SFU_NOFUNC;
	}

	idStr::Split( source, parms, ' ' );
	return Init();
}

/*
================
rvScriptFuncUtility::Init
================
*/
sfuReturnType rvScriptFuncUtility::Init( const idCmdArgs& args ) {
	Clear();

	//Start at index 1 so we ignore 'call'
	for( int ix = 1; ix < args.Argc(); ++ix ) {
		InsertString( args.Argv(ix), ix );
	}

	return Init();
}

/*
================
rvScriptFuncUtility::Init
================
*/
sfuReturnType rvScriptFuncUtility::Init() {
	assert( parms.Num() );

	func = FindFunction( GetParm(0) );
	if( func ) {
		RemoveIndex( 0 );// remove Function name
		returnKey.Clear();
	} else if( parms.Num() >= 2 ) {
		returnKey = GetParm( 0 );
		RemoveIndex( 0 );// remove key name
		func = FindFunction( GetParm(0) );
		RemoveIndex( 0 );// remove Function name
	} else {
		gameLocal.Warning( "Unable to find function %s in rvScriptFuncUtility::Init\n", parms[0].c_str() );
	}

	return func != NULL ? SFU_OK : SFU_ERROR;
}

/*
================
rvScriptFuncUtility::Clear
================
*/
void rvScriptFuncUtility::Clear() {
	func = NULL;
	parms.Clear();
}

/*
================
rvScriptFuncUtility::Save
================
*/
void rvScriptFuncUtility::Save( idSaveGame *savefile ) const {
	bool validFunc = GetFunc() != NULL;
	savefile->WriteBool( validFunc );
	if( !validFunc ) {
		return;
	}
	
	savefile->WriteString( GetReturnKey() );

	savefile->WriteString( GetFunc()->Name() );

	savefile->WriteInt( NumParms() );
	for( int ix = 0; ix < NumParms(); ++ix ) {
		savefile->WriteString( func->Name() );
	}
}

/*
================
rvScriptFuncUtility::Restore
================
*/
void rvScriptFuncUtility::Restore( idRestoreGame *savefile ) {
	idStr value;
	idStr element;
	int numParms = 0;
	bool validFunc = false;

	savefile->ReadBool( validFunc );
	if( !validFunc ) {
		return;
	}

	savefile->ReadString( element );
	if( element.Length() ) {
		value += element;
		value += ' ';
	}

	savefile->ReadString( element );
	if( element.Length() ) {
		value += element;
		value += ' ';
	}

	savefile->ReadInt( numParms );
	for( int ix = 0; ix < numParms; ++ix ) {
		savefile->ReadString( element );
		value += element;
		value += ' ';
	}

	value.StripTrailing( ' ' );
	sfuReturnType status = Init(value.c_str());
	if ( status != SFU_OK ) {
		assert( 0 );
	}
}

/*
================
rvScriptFuncUtility::NumParms
================
*/
int	rvScriptFuncUtility::NumParms() const {
	return (func && func->type) ? func->type->NumParameters() : 0;
}

/*
================
rvScriptFuncUtility::ReturnsAVal
================
*/
bool rvScriptFuncUtility::ReturnsAVal() const {
	return GetReturnType() != &type_void;
}

/*
================
rvScriptFuncUtility::GetParm
================
*/
idTypeDef* rvScriptFuncUtility::GetParmType( int index ) const {
	return (func && func->type) ? func->type->GetParmType( index ) : NULL;
}

/*
================
rvScriptFuncUtility::GetReturnType
================
*/
idTypeDef* rvScriptFuncUtility::GetReturnType() const {
	return (func && func->type) ? func->type->ReturnType() : &type_void;
}

/*
================
rvScriptFuncUtility::SetFunction
================
*/
void rvScriptFuncUtility::SetFunction( const function_t* func ) {
	this->func = func;
}

/*
================
rvScriptFuncUtility::SetParms
================
*/
void rvScriptFuncUtility::SetParms( const idList<idStr>& parms ) {
	this->parms = parms;
}

/*
================
rvScriptFuncUtility::GetParm
================
*/
const char* rvScriptFuncUtility::GetParm( int index ) const {
	return parms[ index ].c_str();
}

/*
================
rvScriptFuncUtility::GetFuncName
================
*/
const char* rvScriptFuncUtility::GetFuncName() const {
	if( !func ) {
		return NULL;
	}

	return func->Name();
}

/*
================
rvScriptFuncUtility::InsertInt
================
*/
void rvScriptFuncUtility::InsertInt( int parm, int index ) {
	InsertString( va("%d", parm), index );
}

/*
================
rvScriptFuncUtility::InsertFloat
================
*/
void rvScriptFuncUtility::InsertFloat( float parm, int index ) {
	InsertString( va("%f", parm), index );
}

/*
================
rvScriptFuncUtility::InsertVec3
================
*/
void rvScriptFuncUtility::InsertVec3( const idVec3& parm, int index ) {
	InsertString( parm.ToString(), index );
}

/*
================
rvScriptFuncUtility::InsertEntity
================
*/
void rvScriptFuncUtility::InsertEntity( const idEntity* parm, int index ) {
	assert( parm );

	InsertString( parm->GetName(), index );
}

/*
================
rvScriptFuncUtility::InsertString
================
*/
void rvScriptFuncUtility::InsertString( const char* parm, int index ) {
	assert( parm );

	parms.Insert( parm, index );
}

/*
================
rvScriptFuncUtility::InsertBool
================
*/
void rvScriptFuncUtility::InsertBool( bool parm, int index ) {
	InsertInt( (int)parm, index );
}

/*
================
rvScriptFuncUtility::RemoveIndex
================
*/
void rvScriptFuncUtility::RemoveIndex( int index ) {
	parms.RemoveIndex( index );
}

/*
================
rvScriptFuncUtility::FindFunction
================
*/
const function_t* rvScriptFuncUtility::FindFunction( const char* name ) const {
	idTypeDef* type = gameLocal.program.FindType( name );
	if( type ) {//Find based on type
		return gameLocal.program.FindFunction( name, type );
	}

	//Find based on scope
	return gameLocal.program.FindFunction( name );
}

/*
================
rvScriptFuncUtility::CallFunc
================
*/
void rvScriptFuncUtility::CallFunc( idDict* returnDict ) const {
	idTypeDef* type = NULL;

	if( !Valid() ) {
		return;
	}

	idThread* thread = new idThread();
	if( !thread ) {
		return;
	}

	thread->ClearStack();
	for( int ix = 0; ix < NumParms(); ++ix ) {
		type = GetParmType( ix );
		type->PushOntoStack( thread, GetParm(ix) );
	}

	thread->CallFunction( func, false );

	if( thread->Execute() && returnDict && ReturnsAVal() ) {
		returnDict->Set( GetReturnKey(), GetReturnType()->GetReturnedValAsString(gameLocal.program) );
	}
}

/*
================
rvScriptFuncUtility::Valid
================
*/
bool rvScriptFuncUtility::Valid() const {
//#if _DEBUG
	idTypeDef* type = NULL;

	if( !GetFunc() ) {
		return false;
	}

	if( GetFunc()->eventdef ) {
		gameLocal.Warning( "Function, %s, is an event\n", GetFunc()->Name() );
		return false;
	}

	// FIXME: designers call functions with no parms even though parms are pushed on stack
	//if( NumParms() != parms.Num() ) {
	//	gameLocal.Warning( "Number of parms doesn't equal the num required by function %s\n", func->Name() );
	//	return false;
	//}

	for( int ix = 0; ix < NumParms(); ++ix ) {
		type = GetParmType( ix );
		if( !type->IsValid( GetParm(ix) ) ) {
			gameLocal.Warning( "(Func: %s) Parm '%s' doesn't match expected type '%s'\n", GetFunc()->Name(), GetParm(ix), type->Name() );
			return false;
		}
	}

	if( returnKey.Length() && !ReturnsAVal() ) {
		gameLocal.Warning( "Expecting return val from function %s which doesn't return one\n", func->Name() );
		return false;
	}
//#endif

	return true;
}

/*
================
rvScriptFuncUtility::Assign
================
*/
rvScriptFuncUtility& rvScriptFuncUtility::Assign( const rvScriptFuncUtility* sfu ) {
	assert( sfu );
	func = sfu->func;
	parms = sfu->parms;
	returnKey = sfu->returnKey;

	return *this;
}

/*
================
rvScriptFuncUtility::operator=
================
*/
rvScriptFuncUtility& rvScriptFuncUtility::operator=( const rvScriptFuncUtility* sfu ) {
	return Assign( sfu );
}

/*
================
rvScriptFuncUtility::operator=
================
*/
rvScriptFuncUtility& rvScriptFuncUtility::operator=( const rvScriptFuncUtility& sfu ) {
	return Assign( &sfu );
}

/*
================
rvScriptFuncUtility::IsEqualTo
================
*/
bool rvScriptFuncUtility::IsEqualTo( const rvScriptFuncUtility* sfu ) const {
	assert( sfu );
	return GetFunc() == sfu->GetFunc();
}

/*
================
rvScriptFuncUtility::operator==
================
*/
bool rvScriptFuncUtility::operator==( const rvScriptFuncUtility* sfu ) const {
	return IsEqualTo( sfu );
}

/*
================
rvScriptFuncUtility::operator==
================
*/
bool rvScriptFuncUtility::operator==( const rvScriptFuncUtility& sfu ) const {
	return IsEqualTo( &sfu );
}
