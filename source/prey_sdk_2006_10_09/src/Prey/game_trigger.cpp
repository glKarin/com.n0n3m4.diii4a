#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/**********************************************************************

hhFuncParmAccessor

**********************************************************************/

CLASS_DECLARATION( idClass, hhFuncParmAccessor )
END_CLASS

/*
================
hhFuncParmAccessor::hhFuncParmAccessor
================
*/
hhFuncParmAccessor::hhFuncParmAccessor() { 
	function = NULL;
}

/*
================
hhFuncParmAccessor::hhFuncParmAccessor
================
*/
hhFuncParmAccessor::hhFuncParmAccessor( const hhFuncParmAccessor* accessor ) {
	assert( accessor );

	SetInfo( accessor->GetReturnKey(), accessor->GetFunction(), accessor->GetParms() );
}

/*
================
hhFuncParmAccessor::hhFuncParmAccessor
================
*/
hhFuncParmAccessor::hhFuncParmAccessor( const hhFuncParmAccessor& accessor ) {
	SetInfo( accessor.GetReturnKey(), accessor.GetFunction(), accessor.GetParms() );
}

/*
================
hhFuncParmAccessor::SetInfo
================
*/
void hhFuncParmAccessor::SetInfo( const char* returnKey, const function_t* func, const idList<idStr>& parms ) {
	SetFunction( func );
	SetParms( parms );
	SetReturnKey( returnKey );

	Verify();
}

void hhFuncParmAccessor::InsertParm_String( const char *text, int index ) {
	InsertParm( text, index );
}

void hhFuncParmAccessor::InsertParm_Float( float value, int index ) {
	InsertParm( va("%.2f", value), index );
}

void hhFuncParmAccessor::InsertParm_Int( int value, int index ) {
	InsertParm( va("%d", value), index );
}

void hhFuncParmAccessor::InsertParm_Vector( const idVec3 &vec, int index ) {
	InsertParm( va("'%.2f %.2f %.2f'", vec[0], vec[1], vec[2]), index );
}

void hhFuncParmAccessor::InsertParm_Entity( const idEntity *ent, int index ) {
	if ( ent ) {
		InsertParm( ent->GetName(), index );
	}
}

void hhFuncParmAccessor::SetParm_Entity( const idEntity *ent, int index ) {
	if ( ent ) {
		parms.AssureSize(index+1);
		parms[index] = ent->GetName();
	}
}

void hhFuncParmAccessor::SetParm_String( const char *str, int index ) {
	if ( str ) {
		parms.AssureSize(index+1);
		parms[index] = str;
	}
}

void hhFuncParmAccessor::InsertParm( const char *text, int index ) {
	parms.Insert( text, index );
}

/*
================
hhFuncParmAccessor::ParseFunctionKeyValue
================
*/
void hhFuncParmAccessor::ParseFunctionKeyValue( const char* value ) {
	if( !value || !value[0] ) {
		return;
	}

	hhUtils::SplitString( idStr(value), GetParms(), ' ' );
	ParseFunctionKeyValue( GetParms() );
}

/*
================
hhFuncParmAccessor::ParseFunctionKeyValue
================
*/
void hhFuncParmAccessor::ParseFunctionKeyValue( idList<idStr>& valueList ) {
	SetFunction( FindFunction(valueList[0]) );
	if( GetFunction() ) {
		valueList.RemoveIndex( 0 );//Remove function name
	} else {
		gameLocal.Warning("hhFuncParmAccessor: Function %s did not exist", (const char*)valueList[0]);
		SetReturnKey( valueList[0] );
		valueList.RemoveIndex( 0 );//Remove return key
		SetFunction( FindFunction(valueList[0]) );
		valueList.RemoveIndex( 0 );//Remove function name
	}
}

/*
================
hhFuncParmAccessor::FindFunction
================
*/
function_t* hhFuncParmAccessor::FindFunction( const char* funcname ) {
	function_t* func = NULL;

	if( funcname && *funcname ) {
		func = gameLocal.program.FindFunction( funcname );	
	}

	return func;
}

/*
================
hhFuncParmAccessor::CallFunction
================
*/
void hhFuncParmAccessor::CallFunction( idDict& returnDict ) {
	if( !GetFunction() ) {
		return;
	}

	hhThread* scriptThread = new hhThread;
	if( !scriptThread ) {
		return;
	}
	
	scriptThread->ClearStack();
	if( !scriptThread->ParseAndPushArgsOntoStack(GetParms(), GetFunction()) ) {
		SAFE_DELETE_PTR( scriptThread ); //This is needed because we won't be removed by Execute
		return;
	}
	
	scriptThread->CallFunction( GetFunction(), false );

	scriptThread->Execute();

	idTypeDef* returnType = GetReturnType();
	if( !GetReturnKey() || !GetReturnKey()[0] ) {
		return;
	}
	
	if( returnType == &type_void ) {
		gameLocal.Warning( "Return key assigned for function (%s) that returns void\n", GetFunctionName() );
		return;
	}

	returnDict.Set( GetReturnKey(), returnType->GetReturnValueAsString(gameLocal.program) );
}

/*
================
hhFuncParmAccessor::Verify
================
*/
void hhFuncParmAccessor::Verify() {
	if( !GetFunction() ) {
		return;
	}

	int numParms = GetFunction()->def->TypeDef()->NumParameters();
	const char* parm = NULL;
	idTypeDef* parmType = NULL;

	if( numParms != GetParms().Num() ) {
		gameLocal.Warning( "Wrong # of parms passed for function %s", GetFunctionName() );
		return;
	}

	if( GetReturnType() == &type_void && GetReturnKey() && GetReturnKey()[0] ) {
		gameLocal.Warning( "Used Return key %s for function %s that returns void", GetReturnKey(), GetFunctionName() );
		return;
	}

	for( int ix = 0; ix < numParms; ++ix ) {
		parmType = GetParmType( ix );
		parm = GetParm( ix );
		if( !parmType->VerifyData(parm) ) {
			gameLocal.Warning( "%s is not Type %s", parm, parmType->Name() );
			return;
		}
	}
}

/*
================
hhFuncParmAccessor::GetFunctionName
================
*/
const char*	hhFuncParmAccessor::GetFunctionName() const {
	return GetFunction()->Name();
}

/*
================
hhFuncParmAccessor::GetFunction
================
*/
const function_t* hhFuncParmAccessor::GetFunction() const {
	return function;
}

/*
================
hhFuncParmAccessor::GetReturnKey
================
*/
const char*	hhFuncParmAccessor::GetReturnKey() const {
	return returnKey.c_str();
}

/*
================
hhFuncParmAccessor::GetReturnType
================
*/
idTypeDef* hhFuncParmAccessor::GetReturnType() const {
	return GetFunction()->def->TypeDef()->ReturnType();
}

/*
================
hhFuncParmAccessor::GetParm
================
*/
const char*	hhFuncParmAccessor::GetParm( int index ) const {
	return parms[ index ];
}

/*
================
hhFuncParmAccessor::GetParmType
================
*/
idTypeDef* hhFuncParmAccessor::GetParmType( int index ) const {
	return GetFunction()->def->TypeDef()->GetParmType( index );
}

/*
================
hhFuncParmAccessor::GetParms
================
*/
const idList<idStr>& hhFuncParmAccessor::GetParms() const {
	return parms;
}

/*
================
hhFuncParmAccessor::GetParms
================
*/
idList<idStr>& hhFuncParmAccessor::GetParms() {
	return parms;
}

/*
================
hhFuncParmAccessor::SetFunction
================
*/
void hhFuncParmAccessor::SetFunction( const function_t* func ) {
	function = func;
}

/*
================
hhFuncParmAccessor::SetParms
================
*/
void hhFuncParmAccessor::SetParms( const idList<idStr>& parms ) {
	this->parms = parms;
}

/*
================
hhFuncParmAccessor::SetReturnKey
================
*/
void hhFuncParmAccessor::SetReturnKey( const char* key ) {
	returnKey = key;
}

/*
================
hhFuncParmAccessor::Save
================
*/
void hhFuncParmAccessor::Save( idSaveGame *savefile ) const {
	int num = parms.Num();
	savefile->WriteInt( num );
	for( int i = 0; i < num; i++ ) {
		savefile->WriteString( parms[i] );
	}

	savefile->WriteString( returnKey );
	savefile->WriteString( function ? function->Name() : "" );
}

/*
================
hhFuncParmAccessor::Restore
================
*/
void hhFuncParmAccessor::Restore( idRestoreGame *savefile ) {
	int num;
	savefile->ReadInt( num );
	idStr tmp;
	parms.Clear();
	for( int i = 0; i < num; i++ ) {
		savefile->ReadString( tmp );
		parms.Append( tmp );
	}

	savefile->ReadString( returnKey );
	savefile->ReadString( tmp );
	if( tmp.Length() > 0 ) {
		function = FindFunction( tmp );
		HH_ASSERT( function );
	} else {
		function = NULL;
	}
}

/**********************************************************************

hhTrigger

**********************************************************************/

const idEventDef EV_Trigger("<triggerAction>", "e");
const idEventDef EV_Retrigger("<retriggerAction>", "e");
const idEventDef EV_UnTrigger("<untriggerAction>", NULL);
const idEventDef EV_PollForUntouch("<pollforuntouch>", NULL);

CLASS_DECLARATION( idEntity, hhTrigger )
	EVENT( EV_Enable,			hhTrigger::Event_Enable )
	EVENT( EV_Disable,			hhTrigger::Event_Disable )
	EVENT( EV_Activate,			hhTrigger::Event_Activate)
	EVENT( EV_Deactivate,		hhTrigger::Event_Deactivate)
	EVENT( EV_Touch,			hhTrigger::Event_Touch)
	EVENT( EV_Trigger,			hhTrigger::Event_TriggerAction )
	EVENT( EV_Retrigger,		hhTrigger::Event_Retrigger )
	EVENT( EV_UnTrigger,		hhTrigger::Event_UnTriggerAction )
	EVENT( EV_PollForUntouch,	hhTrigger::Event_PollForUntouch )
	EVENT( EV_PostSpawn,		hhTrigger::Event_PostSpawn )
END_CLASS

/*
================
hhTrigger::Activate

the trigger was just activated by activator
================
*/
void hhTrigger::Activate( idEntity *activator ) {
	int triggerDelay = 0;

	unTriggerActivator = activator;

	// if not enabled, return
	if( !bEnabled ) {
		return;
	}

	bActive = true;

	if ( nextTriggerTime > gameLocal.time ) {
		// can't retrigger until the wait is over
		return;
	}

	// see if this trigger requires an item
	if ( !gameLocal.RequirementMet( activator, requires, removeItem ) ) {
		return;
	}

	if ( delay > 0 ) {
		// don't allow it to trigger again until our delay has passed
		triggerDelay = hhMath::hhMax( 0, SEC2MS(delay + randomDelay * gameLocal.random.CRandomFloat()) );
		nextTriggerTime = gameLocal.time + triggerDelay + 1;
		PostEventMS( &EV_Trigger, triggerDelay, activator );
	} else {
		//Changed so we check to see if the activator is still with in the trigger volume
		ProcessEvent( &EV_Trigger, activator );
	}
}

/*
================
hhTrigger::Spawn
================
*/
void hhTrigger::Spawn(void) {
	
	unTriggerActivator=NULL;
	bActive=false;

	spawnArgs.GetFloat( "wait", "0.5", wait );
	spawnArgs.GetFloat( "random", "0", random );
	spawnArgs.GetFloat( "delay", "0", delay );
	spawnArgs.GetFloat( "randomeDelay", "0", randomDelay );
	spawnArgs.GetString( "requires", "", requires );
	spawnArgs.GetInt( "removeItem", "0", removeItem );
	triggerBehavior = (triggerBehavior_t)spawnArgs.GetInt( "triggerBehavior" );
	spawnArgs.GetBool( "noTouch", "0", noTouch );
	spawnArgs.GetFloat( "uncalldelay", "0.2", untouchGranularity );
	spawnArgs.GetFloat( "refire", "-1", refire );
	spawnArgs.GetBool( "enabled", "1", initiallyEnabled );
	spawnArgs.GetBool( "untrigger", "0", bUntrigger );
	spawnArgs.GetBool( "always_trigger", "0", alwaysTrigger );
	spawnArgs.GetBool( "isSimpleBox", "1", isSimpleBox );
	spawnArgs.GetBool( "noVehicles", "0", bNoVehicles );

	GetTriggerClasses( spawnArgs );

	nextTriggerTime = 0;

	PostEventMS( (initiallyEnabled) ? &EV_Enable : &EV_Disable, 0);

	PostEventMS( &EV_PostSpawn, 0 );
}

/*
================
hhTrigger::Save
================
*/
void hhTrigger::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( bActive );
	savefile->WriteBool( bEnabled );
	savefile->WriteFloat( untouchGranularity );
	savefile->WriteFloat( wait );
	savefile->WriteFloat( random );
	savefile->WriteFloat( delay );
	savefile->WriteFloat( randomDelay );
	savefile->WriteFloat( refire );
	savefile->WriteInt( nextTriggerTime );
	savefile->WriteBool( alwaysTrigger );
	savefile->WriteBool( isSimpleBox );
	savefile->WriteBool( bNoVehicles );
	savefile->WriteStaticObject( funcInfo );
	savefile->WriteStaticObject( unfuncInfo );
	savefile->WriteStaticObject( funcRefInfo );
	savefile->WriteStaticObject( unfuncRefInfo );
	savefile->WriteStaticObject( funcRefActivatorInfo );
	savefile->WriteStaticObject( unfuncRefActivatorInfo );
	savefile->WriteString( requires );
	savefile->WriteInt( removeItem );
	savefile->WriteBool( noTouch );
	savefile->WriteBool( initiallyEnabled );
	savefile->WriteBool( bUntrigger );

	unTriggerActivator.Save( savefile );

	savefile->WriteStringList( TriggerClasses );
	savefile->WriteInt( triggerBehavior );
}

/*
================
hhTrigger::Restore
================
*/
void hhTrigger::Restore( idRestoreGame *savefile ) {	
	savefile->ReadBool( bActive );
	savefile->ReadBool( bEnabled );
	savefile->ReadFloat( untouchGranularity );
	savefile->ReadFloat( wait );
	savefile->ReadFloat( random );
	savefile->ReadFloat( delay );
	savefile->ReadFloat( randomDelay );
	savefile->ReadFloat( refire );
	savefile->ReadInt( nextTriggerTime );
	savefile->ReadBool( alwaysTrigger );
	savefile->ReadBool( isSimpleBox );
	savefile->ReadBool( bNoVehicles );
	savefile->ReadStaticObject( funcInfo );
	savefile->ReadStaticObject( unfuncInfo );
	savefile->ReadStaticObject( funcRefInfo );
	savefile->ReadStaticObject( unfuncRefInfo );
	savefile->ReadStaticObject( funcRefActivatorInfo );
	savefile->ReadStaticObject( unfuncRefActivatorInfo );
	savefile->ReadString( requires );
	savefile->ReadInt( removeItem );
	savefile->ReadBool( noTouch );
	savefile->ReadBool( initiallyEnabled );
	savefile->ReadBool( bUntrigger );

	unTriggerActivator.Restore( savefile );

	savefile->ReadStringList( TriggerClasses );
	savefile->ReadInt( reinterpret_cast<int &> ( triggerBehavior ) );
}

/*
================
hhTrigger::CallFunctions
================
*/
void hhTrigger::CallFunctions( idEntity *activator ) {
	funcInfo.CallFunction( spawnArgs );
	funcRefInfo.CallFunction( spawnArgs );

	funcRefActivatorInfo.InsertParm_Entity( activator, 1 );//Needs to be second parm.  After self
	funcRefActivatorInfo.CallFunction( spawnArgs );
}

/*
================
hhTrigger::UncallFunctions
================
*/
void hhTrigger::UncallFunctions( idEntity *activator ) {
	unfuncInfo.CallFunction( spawnArgs );
	unfuncRefInfo.CallFunction( spawnArgs );

	unfuncRefActivatorInfo.InsertParm_Entity( activator, 1 );//Needs to be second parm.  After self
	unfuncRefActivatorInfo.CallFunction( spawnArgs );
}

/*
================
hhTrigger::TriggerAction
================
*/
void hhTrigger::TriggerAction( idEntity *activator ) {

	// Activate all targets
	ActivateTargets( activator );

	CallFunctions( activator );

	if ( wait >= 0 ) {
		nextTriggerTime = gameLocal.time + SEC2MS( wait + random * gameLocal.random.CRandomFloat() );
	} else {
		nextTriggerTime = gameLocal.time + 1;
		PostEventMS( &EV_Disable, 0 );
	}

	// Handle refire
	if (refire > 0) {
		PostEventSec( &EV_Retrigger, refire + random * gameLocal.random.CRandomFloat(), activator);
	}
}

/*
================
hhTrigger::UnTriggerAction
================
*/
void hhTrigger::UnTriggerAction() {
	if( bUntrigger ) {
		ActivateTargets( unTriggerActivator.GetEntity() );
	}

	UncallFunctions( unTriggerActivator.GetEntity() );
}

/*
===============
hhTrigger::IsEncroaching
===============
*/
bool hhTrigger::IsEncroaching( const idEntity* entity ) {

	// nla - Simplified checks to bounds when possible.
	// Error was due to contents of an idActivator not hitting anything here.  (it has contents 0)
	// But since the collide logic has been done elsewhere, we can simplify this to just a is this in this check.
	if (isSimpleBox) {
		// num = gameLocal.clip.EntitiesTouchingBounds( GetPhysics()->GetAbsBounds(), entity->GetPhysics()->GetContents(), touch, MAX_GENTITIES );
		if ( GetPhysics()->GetAbsBounds().IntersectsBounds( entity->GetPhysics()->GetAbsBounds() ) ) {
			return( true );
		}
	}
	else {
		//num = hhUtils::EntitiesTouchingClipmodel( GetPhysics()->GetClipModel(), touch, MAX_GENTITIES, entity->GetPhysics()->GetContents() );
		if ( entity->GetPhysics()->ClipContents( GetPhysics()->GetClipModel() ) ) {
			return( true );
		}
	}

	return false;
}

/*
===============
hhTrigger::IsEncroached
===============
*/
bool hhTrigger::IsEncroached() {
	idEntity		*touch[ MAX_GENTITIES ];
	int				num;

	if (isSimpleBox) {
		num = gameLocal.clip.EntitiesTouchingBounds( GetPhysics()->GetAbsBounds(), MASK_SHOT_BOUNDINGBOX, touch, MAX_GENTITIES );
	}
	else {
		num = hhUtils::EntitiesTouchingClipmodel( GetPhysics()->GetClipModel(), touch, MAX_GENTITIES );
	}

	for (int i = 0; i < num; i++ ) {
		if( touch[ i ] == this ) {
			continue;
		}

		if( CheckUnTriggerClass(touch[ i ]) ) {
			return true;
		}
	}
	return false;
}

/*
===============
hhTrigger::SetTriggerClasses
===============
*/
void hhTrigger::SetTriggerClasses( idList<idStr>& list ) {
	TriggerClasses.Clear();
	
	TriggerClasses.SetNum(list.Num());

	for(int iIndex = 0; iIndex < list.Num(); ++iIndex) {
		TriggerClasses[iIndex] = list[iIndex];
	}
}

/*
===============
hhTrigger::CheckTriggerClass
===============
*/
bool hhTrigger::CheckTriggerClass( idEntity* activator ) {
	if( !activator ) {
		return false;
	}

	if(!TriggerClasses.Num() || triggerBehavior == TB_ANY) {
		return true;
	}


	for(int iIndex = 0; iIndex < TriggerClasses.Num(); ++iIndex) {
		//Look for exact match then try for prefix match
		if( !idStr(activator->spawnArgs.GetString("classname")).Icmp(TriggerClasses[iIndex].c_str()) ||
			!idStr(activator->spawnArgs.GetString("classname")).IcmpPrefix(TriggerClasses[iIndex].c_str()) ) {

			if( activator->IsType(hhPlayer::Type) ) {//Player needs client
				hhPlayer* player = static_cast<hhPlayer*>(activator);
				if ( player->noclip || !player->ShouldTouchTrigger(this) ) {
					return false;
				}
				if ( bNoVehicles && player->InVehicle() ) {
					return false;
				}
			}
			if( activator->IsType(idActor::Type) ) {	// Dead monsters shouldn't keep refires going
				if (activator->health <= 0) {
					return false;
				}
			}
			return true;
		}
	}

	return false;
}

/*
===============
hhTrigger::CheckUnTriggerClass

Helper function used for overriding
===============
*/
bool hhTrigger::CheckUnTriggerClass(idEntity* activator) {
	return CheckTriggerClass(activator);
}

/*
===============
hhTrigger::GetTriggerClasses
===============
*/
void hhTrigger::GetTriggerClasses(idDict& Args) {
	const idKeyValue* pKeyValue = NULL;
	int iNumKeyValues = Args.GetNumKeyVals();

	TriggerClasses.Clear();

	if(triggerBehavior == TB_PLAYER_ONLY) {
		TriggerClasses.AddUnique( "player" );

	} else if(triggerBehavior == TB_FRIENDLIES_ONLY) {
		TriggerClasses.AddUnique( "player" );
		TriggerClasses.AddUnique( "character" );

	} else if(triggerBehavior == TB_MONSTERS_ONLY) {
		TriggerClasses.AddUnique( "monster" );

	} else if(triggerBehavior == TB_PLAYER_MONSTERS_FRIENDLIES) {
		TriggerClasses.AddUnique( "player" );
		TriggerClasses.AddUnique( "monster" );
		TriggerClasses.AddUnique( "character" );

	} else if(triggerBehavior == TB_SPECIFIC_ENTITIES){
		for( int iIndex = 0; iIndex < iNumKeyValues; ++iIndex ) {
			pKeyValue = Args.GetKeyVal( iIndex );
			if ( !pKeyValue->GetKey().Cmpn( "trigger_class", 13 ) ) {
				TriggerClasses.AddUnique( pKeyValue->GetValue() );
			}
		}
	}
}

/*
===============
hhTrigger::Enable
===============
*/
void hhTrigger::Enable() {
	GetPhysics()->SetContents( CONTENTS_TRIGGER );
	GetPhysics()->EnableClip();

	bEnabled = true;
}

/*
===============
hhTrigger::Disable
===============
*/
void hhTrigger::Disable() {
	// we may be relinked if we're bound to another object, so clear the contents as well
	GetPhysics()->SetContents( 0 );
	GetPhysics()->DisableClip();

	CancelEvents( &EV_Retrigger );
	bEnabled = false;
}

/*
===============
hhTrigger::Event_PostSpawn
===============
*/
void hhTrigger::Event_PostSpawn() {
	funcInfo.ParseFunctionKeyValue( spawnArgs.GetString("call") );
	funcInfo.Verify();
	unfuncInfo.ParseFunctionKeyValue( spawnArgs.GetString("uncall") );
	unfuncInfo.Verify();

	funcRefInfo.ParseFunctionKeyValue( spawnArgs.GetString("callRef") );
	funcRefInfo.InsertParm_Entity( this, 0 );
	funcRefInfo.Verify();

	unfuncRefInfo.ParseFunctionKeyValue( spawnArgs.GetString("uncallRef") );
	unfuncRefInfo.InsertParm_Entity( this, 0 );
	unfuncRefInfo.Verify();

	funcRefActivatorInfo.ParseFunctionKeyValue( spawnArgs.GetString("callRefActivator") );
	funcRefActivatorInfo.InsertParm_Entity( this, 0 );
	// NOTE: Activator parm inserted at time of function call, therefore don't verify now

	unfuncRefActivatorInfo.ParseFunctionKeyValue( spawnArgs.GetString("uncallRefActivator") );
	unfuncRefActivatorInfo.InsertParm_Entity( this, 0 );
	// NOTE: Activator parm inserted at time of function call, therefore don't verify now
}

/*
===============
hhTrigger::Event_PollForUntouch
===============
*/
void hhTrigger::Event_PollForUntouch() {
	if (!IsEncroached()) {
		CancelEvents(&EV_PollForUntouch);
		PostEventMS(&EV_Deactivate, 0);
	}
	else {
		PostEventSec(&EV_PollForUntouch, untouchGranularity);
	}
}

/*
===============
hhTrigger::Event_Touch
===============
*/
void hhTrigger::Event_Touch( idEntity *other, trace_t *trace ) {

	if( noTouch || !CheckTriggerClass(other) ) {
		return;
	}

	if( !IsActive() ) {
		Activate( other );

		// If this trigger uses any of the unTrigger mechanisms, start polling
		if (bUntrigger || refire || unfuncInfo.GetFunction()  || unfuncRefInfo.GetFunction() || unfuncRefActivatorInfo.GetFunction()) {
			PostEventSec(&EV_PollForUntouch, untouchGranularity);
		}
	}
	// If we have already triggered the first time, but should always trigger
	else if ( alwaysTrigger ) {
		Activate( other );
	}
}

/*
================
hhTrigger::Retrigger
================
*/
void hhTrigger::Event_Retrigger( idEntity *activator ) {
	//AOB - needed to stop infinite retrigger when triggered from gui
	//rww - activator can be null if removed before event is serviced. actually, isn't this a bad idea? maybe it's better to use an index.
	if (!activator || (!noTouch && !IsEncroaching(activator)) ) {
		CancelEvents( &EV_Retrigger );
		PostEventMS( &EV_Deactivate, 0 );
		return;
	}

	// This is seperate event so we can cull them seperately from normal trigger events

	TriggerAction( activator );
}

/*
================
hhTrigger::Event_UnTriggerAction
================
*/
void hhTrigger::Event_UnTriggerAction() {
	UnTriggerAction();
}

/*
================
hhTrigger::Event_TriggerAction
================
*/
void hhTrigger::Event_TriggerAction( idEntity *activator ) {
	//HUMANHEAD rww - we are running into a null activator situation when the spirit proxy for the player runs into a trigger,
	//and then the player switches back to his body and removes the proxy before the trigger fires (due to a relay/delay/whatever),
	//as well as possibly any other situation where a delayed trigger is activated by an entity that gets removed before firing.
	//it has been decided that in these situations, the world should be used as the activator.
	if (!activator) {
		activator = gameLocal.world;
	}
	//HUMANHEAD END
	// Added noTouch && !IsEncroached to fix the issue with retriggered hurt constantly damaging the player, even when they weren't in it.
	// nla - Added check to allow 'delayed' triggers to function when you weren't in them.
	if (!noTouch && !IsEncroaching(activator) && !delay ) {
		CancelEvents( &EV_Retrigger );
		PostEventMS( &EV_Deactivate, 0 );
		return;
	}

	TriggerAction( activator );
}

/*
================
hhTrigger::Event_Enable
================
*/
void hhTrigger::Event_Enable( void ) {
	Enable();
}

/*
================
hhTrigger::Event_Disable
================
*/
void hhTrigger::Event_Disable( void ) {
	Disable();
}

/*
================
hhTrigger::Event_Activate
================
*/
void hhTrigger::Event_Activate( idEntity *activator ) {
	Activate( activator );
}

/*
================
hhTrigger::Event_Deactivate
================
*/
void hhTrigger::Event_Deactivate() {
	CancelEvents( &EV_Retrigger );
	PostEventMS( &EV_UnTrigger, 0 );
	bActive=false;
}

/***********************************************************************

hhDamageTrigger
	
***********************************************************************/

CLASS_DECLARATION( hhTrigger, hhDamageTrigger )
	EVENT( EV_Enable,			hhDamageTrigger::Event_Enable )
	EVENT( EV_Disable,			hhDamageTrigger::Event_Disable )
END_CLASS


void hhDamageTrigger::Spawn(void) {
	funcRefInfoDamage.ParseFunctionKeyValue( spawnArgs.GetString("damageCallRef") );
	funcRefInfoDamage.InsertParm_Entity( this,  0 );
}

void hhDamageTrigger::Save(idSaveGame *savefile) const {
	savefile->WriteStaticObject( funcRefInfoDamage );
}

void hhDamageTrigger::Restore( idRestoreGame *savefile ) {
	savefile->ReadStaticObject( funcRefInfoDamage );
}

void hhDamageTrigger::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location) {
	// Call script if requested with damageDef, used for spherebrain
	if (funcRefInfoDamage.GetFunction() != NULL) {
		funcRefInfoDamage.InsertParm_Entity( attacker,  1 );
		spawnArgs.Set( "last_damage", damageDefName );
		funcRefInfoDamage.CallFunction( spawnArgs );
	}
	if ( spawnArgs.GetBool( "activate_targets" ) ) {
		ActivateTargets( attacker );
	}
	hhTrigger::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);
}

void hhDamageTrigger::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if (!CheckTriggerClass(attacker)) {
		health += damage;
		return;
	}

	if ( gameLocal.time < nextTriggerTime ) {
		health += damage;
		return;
	}

	Activate( attacker );
}

void hhDamageTrigger::Event_Enable() {
	bEnabled = true;
	fl.takedamage = true;
	GetPhysics()->SetContents( CONTENTS_SHOOTABLE|CONTENTS_SHOOTABLEBYARROW );
}

void hhDamageTrigger::Event_Disable() {
	GetPhysics()->SetContents( 0 );
	fl.takedamage = false;
	bEnabled = false;
}

/***********************************************************************

hhTriggerPain
	
***********************************************************************/

const idEventDef EV_DamageBox("damageBox", "e");

CLASS_DECLARATION( hhTrigger, hhTriggerPain )
	EVENT( EV_Activate,		hhTriggerPain::Event_Activate )
	EVENT( EV_DamageBox,	hhTriggerPain::Event_DamageBox )
END_CLASS

/*
================
hhTriggerPain

	Damages activator
	Can be turned on or off by enable/disable
	Fires events, triggers targets
================
*/
void hhTriggerPain::Spawn(void) {
	spawnArgs.GetBool( "apply_impulse", "0", applyImpulse );
}

void hhTriggerPain::Event_DamageBox( idEntity *activator ) {
	idEntity		*touch[ MAX_GENTITIES ];
	int				num;

	if (isSimpleBox) {
		num = gameLocal.clip.EntitiesTouchingBounds( GetPhysics()->GetAbsBounds(), MASK_SHOT_BOUNDINGBOX, touch, MAX_GENTITIES );
	}
	else {
		num = hhUtils::EntitiesTouchingClipmodel( GetPhysics()->GetClipModel(), touch, MAX_GENTITIES );
	}

	for (int i = 0; i < num; i++ ) {
		if( touch[ i ] == this ) {
			continue;
		}

		if( CheckTriggerClass( touch[i] ) ) {
			touch[i]->Damage(activator, activator, vec3_origin, spawnArgs.GetString("def_damage"), 1.0f, INVALID_JOINT );
		}
	}
}

/*
================
hhTriggerPain::TriggerAction
================
*/
void hhTriggerPain::TriggerAction(idEntity *activator) {
	
	if(activator) {
		// If a player in a vehicle, apply pain to vehicle
		if (activator->IsType(hhPlayer::Type)) {
			hhPlayer *player = static_cast<hhPlayer*>(activator);
			if (player->InVehicle()) {
				activator = player->GetVehicleInterface()->GetVehicle();
			}
		}
		
		idVec3 Velocity = activator->GetPhysics()->GetLinearVelocity();

		activator->Damage(NULL, NULL, -Velocity, spawnArgs.GetString("def_damage"), 1.0f, INVALID_JOINT );

		if ( applyImpulse ) {
//			activator->ApplyImpulse( this, 0, activator->GetPhysics()->GetOrigin(), -Velocity / 10.0 );
			activator->ApplyImpulse( this, 0, activator->GetPhysics()->GetOrigin(), -Velocity * (activator->GetPhysics()->GetMass()*1.5f) );
		}
		
	}

	// Handle default trigger behavior
	hhTrigger::TriggerAction(activator);
}

/*
================
hhTriggerPain::Event_Activate
================
*/
void hhTriggerPain::Event_Activate( idEntity *activator ) {
	//AOB - allow toggling
//	if( IsEnabled() && (IsActive() || !IsEncroached()) ) {
	if( IsEnabled() ) {
		ProcessEvent( &EV_Disable );
		ProcessEvent( &EV_Deactivate );
	} else {
		if( !IsEnabled() ) {
			ProcessEvent( &EV_Enable );
		}
		Activate( activator );
	}
}

/*
================
hhTriggerPain::Save
================
*/
void hhTriggerPain::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( applyImpulse );
}

/*
================
hhTriggerPain::Restore
================
*/
void hhTriggerPain::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( applyImpulse );
}

/***********************************************************************

hhTriggerEnabler
	
***********************************************************************/

CLASS_DECLARATION( hhTrigger, hhTriggerEnabler )
END_CLASS

/*
================
hhTriggerEnabler

	When it is entered, target is enabled
	When it is exited, target is disabled.
================
*/
void hhTriggerEnabler::Spawn(void) {
}

/*
================
hhTriggerEnabler::TriggerAction
================
*/
void hhTriggerEnabler::TriggerAction(idEntity *activator) {

	// Enable all targets
	for (int t=0; t<targets.Num(); t++) {
		idEntity *ent = targets[t].GetEntity();
		if (ent) {
			ent->PostEventMS( &EV_Enable, 0 );
		}
	}

	// Handle default trigger behavior
	hhTrigger::TriggerAction(activator);
}


/*
================
hhTriggerEnabler::UnTriggerAction
================
*/
void hhTriggerEnabler::UnTriggerAction() {

	// Disable all targets
	for (int t=0; t<targets.Num(); t++) {
		idEntity *ent = targets[t].GetEntity();
		if (ent) {
			ent->PostEventMS( &EV_Disable, 0 );
		}
	}

	// Handle default trigger behavior
	hhTrigger::UnTriggerAction();
}


/***********************************************************************

hhTriggerSight

	Activates targets when seen
***********************************************************************/

CLASS_DECLARATION( hhTrigger, hhTriggerSight )
	EVENT( EV_Enable,			hhTriggerSight::Event_Enable )
	EVENT( EV_Disable,			hhTriggerSight::Event_Disable )
END_CLASS


void hhTriggerSight::Spawn(void) {
	BecomeActive(TH_THINK);
}

void hhTriggerSight::Think(void) {

	if (thinkFlags & TH_THINK) {
		hhPlayer *player = NULL;
		for (int i = 0; i < gameLocal.numClients; i++ ) {
			player = static_cast<hhPlayer*>(gameLocal.entities[ i ]);
			if ( !player ) {
				continue;
			}

			// if there is no way we can see this player
			if ( !gameLocal.InPlayerPVS(this) ) {
				continue;
			}

			if (player->CanSee(this, true)) {

				// activate
				PostEventMS(&EV_Activate, 0, player);

				// Automatically disables itself, re-enable for repeat
				PostEventMS(&EV_Disable, 0);
			}
		}
	}

	Present();	// Need this so it get's removed from active list
}

void hhTriggerSight::Event_Enable( void ) {
	bEnabled = true;
	BecomeActive(TH_THINK);
}

void hhTriggerSight::Event_Disable( void ) {
	bEnabled = false;
	BecomeInactive(TH_THINK);
}

/*
================
hhTriggerSight::Save
================
*/
void hhTriggerSight::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( pvsArea );
}

/*
================
hhTriggerSight::Restore
================
*/
void hhTriggerSight::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( pvsArea );
	Spawn();
}

/*
===============================================================================

  hhTrigger_Count
	
===============================================================================
*/

CLASS_DECLARATION( hhTrigger, hhTrigger_Count )
END_CLASS

/*
================
hhTrigger_Count::Save
================
*/
void hhTrigger_Count::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( goal );
	savefile->WriteInt( count );
	savefile->WriteFloat( delay );
}

/*
================
hhTrigger_Count::Restore
================
*/
void hhTrigger_Count::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( goal );
	savefile->ReadInt( count );
	savefile->ReadFloat( delay );
}

/*
================
hhTrigger_Count::Spawn
================
*/
void hhTrigger_Count::Spawn( void ) {
	spawnArgs.GetInt( "count", "1", goal );
	spawnArgs.GetFloat( "delay", "0", delay );
	count = 0;
}

/*
================
hhTrigger_Count::Activate
================
*/
void hhTrigger_Count::Activate( idEntity *activator ) {
	int triggerDelay = 0;

	unTriggerActivator = activator;

	// if not enabled, return
	if (!bEnabled) {
		return;
	}

	bActive=true;

	if ( nextTriggerTime > gameLocal.time ) {
		// can't retrigger until the wait is over
		return;
	}

	// see if this trigger requires an item
	if ( !gameLocal.RequirementMet( activator, requires, removeItem ) ) {
		return;
	}

	if ( delay >= 0 ) {
		// goal of -1 means trigger has been exhausted
		if (goal >= 0) {
			count++;
			if ( count >= goal ) {
				if (spawnArgs.GetBool("repeat")) {
					count = 0;
				} else {
					goal = -1;
				}

				// don't allow it to trigger again until our delay has passed
				triggerDelay = hhMath::hhMax( 0, SEC2MS(delay + randomDelay * gameLocal.random.CRandomFloat()) );
				nextTriggerTime = gameLocal.time + triggerDelay;
				PostEventMS( &EV_Trigger, triggerDelay, activator );
			}
		}
	}
}

/*
================
hhTrigger_Count::TriggerAction
================
*/
void hhTrigger_Count::TriggerAction( idEntity *activator ) {
	hhTrigger::TriggerAction( activator );

	if (goal == -1) {
		PostEventMS( &EV_Remove, 0 );
	}
}


/*
===============================================================================

  hhTrigger_Event
	
===============================================================================
*/

CLASS_DECLARATION( hhTrigger, hhTrigger_Event )
END_CLASS

/*
================
hhTrigger_Event::Spawn
================
*/
void hhTrigger_Event::Spawn( void ) {
	eventDef = FindEventDef( spawnArgs.GetString("eventDef") );
}

/*
================
hhTrigger_Event::FindEventDef
================
*/
const idEventDef* hhTrigger_Event::FindEventDef( const char* eventDefName ) const {
	function_t *func = hhTrigger_Event::FindFunction( eventDefName );
	if( !func || !func->eventdef ) {
        gameLocal.Error( "Cannot find event %s on trigger %s\n", eventDefName, name.c_str() );
	}

	return func->eventdef;
}

/*
================
hhTrigger_Event::FindFunction
================
*/
function_t* hhTrigger_Event::FindFunction( const char* funcname ) {
	function_t* func = NULL;

	func = gameLocal.program.FindFunction( funcname, gameLocal.program.FindType(funcname) );
	if( !func ) {
		gameLocal.Error( "Cannot find function '%s' in hhTrigger", funcname );
	}

	return func;
}

/*
================
hhTrigger_Event::ActivateTargets
================
*/
void hhTrigger_Event::ActivateTargets( idEntity *activator ) const {
	idEntity	*ent;
	int			i;

	HH_ASSERT( eventDef );
	
	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( !ent ) {
			continue;
		}
		if ( ent->RespondsTo( *eventDef ) ) {
			ent->ProcessEvent( eventDef, activator );
		}
		for (int ix=0; ix<MAX_RENDERENTITY_GUI; ix++) {
			if (renderEntity.gui[ix]) {
				ent->GetRenderEntity()->gui[ix]->Trigger(gameLocal.time);	
			}
		}
	}
}

/***********************************************************************

  hhMineTrigger
	
***********************************************************************/

CLASS_DECLARATION( hhTrigger, hhMineTrigger )
END_CLASS

bool hhMineTrigger::CheckTriggerClass( idEntity* activator ) {
	if( !activator ) {
		return false;
	}

	if(!TriggerClasses.Num() || triggerBehavior == TB_ANY) {
		return true;
	}

	for(int iIndex = 0; iIndex < TriggerClasses.Num(); ++iIndex) {
		//Look for exact match then try for prefix match
		if( !idStr(activator->spawnArgs.GetString("classname")).Icmp(TriggerClasses[iIndex].c_str()) ||
			!idStr(activator->spawnArgs.GetString("classname")).IcmpPrefix(TriggerClasses[iIndex].c_str()) ) {
				
			if( activator->IsType(hhPlayer::Type) ) {//Player needs client
				hhPlayer* player = static_cast<hhPlayer*>(activator);
				if ( player->noclip || !player->ShouldTouchTrigger(this) ) {
					return false;
				}
				if ( bNoVehicles && player->InVehicle() ) {
					return false;
				}
			}
			if( activator->IsType(idActor::Type) ) {	// Dead monsters shouldn't keep refires going
				if (activator->health <= 0) {
					return false;
				}
			}

			return true;
		}
	}
	
	if ( activator->IsType(hhMountedGun::Type) ) {
		return true;
	}

	//HUMANHEAD jsh PCF 4/28/06 hardcode monsters to trigger mines
	if ( !gameLocal.isMultiplayer && activator->IsType( idAI::Type ) && activator->health > 0 ) {
		return true;
	}

	return false;
}