#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

const idEventDef EV_OpenGate( "<openGate>" );
const idEventDef EV_CloseGate( "<closeGate>" );

//=======================================================
//
//	rvTramGate
//
//=======================================================
CLASS_DECLARATION( idAnimatedEntity, rvTramGate )
	EVENT( EV_Touch,			rvTramGate::Event_Touch )
	EVENT( EV_Activate,			rvTramGate::Event_Activate )
	EVENT( EV_OpenGate,			rvTramGate::Event_OpenGate )
	EVENT( EV_CloseGate,		rvTramGate::Event_CloseGate )
	EVENT( EV_Door_Lock,		rvTramGate::Event_Lock )
	EVENT( EV_Door_IsOpen,		rvTramGate::Event_IsOpen )
	EVENT( EV_Door_IsLocked,	rvTramGate::Event_IsLocked )
END_CLASS

/*
================
rvTramGate::Spawn
================
*/
void rvTramGate::Spawn() {
	SpawnDoors();

	AdjustFrameRate();
}

/*
================
rvTramGate::~rvTramGate
================
*/
rvTramGate::~rvTramGate() {
	doorList.RemoveContents( true );
}

/*
================
rvTramGate::SpawnDoors
================
*/
void rvTramGate::SpawnDoors() {
	idDict	args;
	idVec3	dir = spawnArgs.GetAngles("doorsAxisOffset").ToMat3() * GetPhysics()->GetAxis()[1];

	args.Set( "team", GetName() );
	args.SetMatrix( "rotation", dir.ToMat3() );
	args.SetVector( "origin", GetPhysics()->GetOrigin() );

	int len = strlen("door_");
	for( const idKeyValue* kv = spawnArgs.MatchPrefix("door"); kv; kv = spawnArgs.MatchPrefix("door", kv) ) {
		args.Set( kv->GetKey().Right(kv->GetKey().Length() - len), kv->GetValue() );
	}

	args.SetFloat( "movedir", (-dir).ToYaw() );
	idDoor* door = gameLocal.SpawnSafeEntityDef<idDoor>( spawnArgs.GetString("def_door1"), &args );
	if( door ) {
		doorList.Alloc() = door;
		door->SetDoorFrameController( this );
	}

	args.SetFloat( "movedir", dir.ToYaw() );
	door = gameLocal.SpawnSafeEntityDef<idDoor>( spawnArgs.GetString("def_door2"), &args );
	if( door ) {
		doorList.Alloc() = door;
		door->SetDoorFrameController( this );
	}

	//assert( GetDoorMaster() == door->GetMoveMaster() );
}

/*
================
rvTramGate::AdjustFrameRate
================
*/
void rvTramGate::AdjustFrameRate() {
	GetAnimator()->SetPlaybackRate( "open", spawnArgs.GetFloat("openFrameRateScale") );
	GetAnimator()->SetPlaybackRate( "close", spawnArgs.GetFloat("closeFrameRateScale") );
}

/*
================
rvTramGate::OpenGate
================
*/
void rvTramGate::OpenGate() {
	PlayAnim( ANIMCHANNEL_ALL, "open" );
}

/*
================
rvTramGate::CloseGate
================
*/
void rvTramGate::CloseGate() {
	PlayAnim( ANIMCHANNEL_ALL, "close" );
}

/*
================
rvTramGate::Save
================
*/
void rvTramGate::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( doorList.Num() );
	for( int i = 0; i < doorList.Num(); i++ ) {
		doorList[i].Save( savefile );
	}
}

/*
================
rvTramGate::Restore
================
*/
void rvTramGate::Restore( idRestoreGame *savefile ) {
	int num = 0;
	idEntityPtr<idDoor> temp;
	savefile->ReadInt( num );
	for( int i = 0; i < num; i++ ) {
		temp.Restore( savefile );
		doorList.Append( temp );
	}
}

/*
================
rvTramGate::PlayAnim
================
*/
int rvTramGate::PlayAnim( int channel, const char* animName, int blendFrames ) {
	int animHandle = GetAnimator()->GetAnim( animName );

	if( !animHandle ) {
		ClearAllAnims( blendFrames );
		return 0;
	}

	GetAnimator()->PlayAnim( channel, animHandle, gameLocal.GetTime(), FRAME2MS(blendFrames) );
	return GetAnimator()->CurrentAnim(channel)->PlayLength();
}

/*
================
rvTramGate::CycleAnim
================
*/
void rvTramGate::CycleAnim( int channel, const char* animName, int blendFrames ) {
	int animHandle = GetAnimator()->GetAnim( animName );

	if( !animHandle ) {
		ClearAllAnims( blendFrames );
		return;
	}

	GetAnimator()->CycleAnim( channel, animHandle, gameLocal.GetTime(), FRAME2MS(blendFrames) );
}

/*
================
rvTramGate::Event_Touch
================
*/
void rvTramGate::ClearAllAnims( int blendFrames ) {
	GetAnimator()->ClearAllAnims( gameLocal.GetTime(), FRAME2MS(blendFrames) );
}

/*
================
rvTramGate::Event_Touch
================
*/
void rvTramGate::ClearAnim( int channel, int blendFrames ) {
	GetAnimator()->Clear( channel, gameLocal.GetTime(), FRAME2MS(blendFrames) ); 
}

/*
================
rvTramGate::Event_Touch
================
*/
bool rvTramGate::AnimIsPlaying( int channel, int blendFrames ) {
	return GetAnimator()->CurrentAnim(channel)->GetEndTime() - FRAME2MS(blendFrames) >= gameLocal.GetTime();
}

/*
================
rvTramGate::IsOpen
================
*/
bool rvTramGate::IsOpen() const {
	return (IsDoorMasterValid()) ? GetDoorMaster()->IsOpen() : false;
}

/*
================
rvTramGate::IsClosed
================
*/
bool rvTramGate::IsClosed() const {
	return (IsDoorMasterValid()) ? GetDoorMaster()->IsClosed() : false;
}

/*
================
rvTramGate::GetDoorMaster
================
*/
idDoor* rvTramGate::GetDoorMaster() const {
	return doorList.Num() ? doorList[0] : NULL;
}

/*
================
rvTramGate::IsDoorMasterValid
================
*/
bool rvTramGate::IsDoorMasterValid() const {
	return GetDoorMaster() != NULL;
}

/*
================
rvTramGate::Event_Touch
================
*/
void rvTramGate::Event_Touch( idEntity* other, trace_t* trace ) {
	if( !IsDoorMasterValid() ) {
		return;
	}

	if( IsClosed() ) {
		OpenGate();
		GetDoorMaster()->ProcessEvent( &EV_Touch, this, trace );	
	} else if( IsOpen() ) {
		GetDoorMaster()->ProcessEvent( &EV_Touch, this, trace );	
	}
}

/*
================
rvTramGate::Event_Activate
================
*/
void rvTramGate::Event_Activate( idEntity* activator ) {
	if( !IsDoorMasterValid() ) {
		return;
	}
	
	// FIXME: may need some better logic than this.
	const char* animName = (IsClosed()) ? "open" : (IsOpen()) ? "close" : NULL;
	if( animName ) {
		PlayAnim( ANIMCHANNEL_ALL, animName );
		GetDoorMaster()->ProcessEvent( &EV_Activate, this );
	}
}

/*
================
rvTramGate::Event_OpenGate
================
*/
void rvTramGate::Event_OpenGate() {
	OpenGate();
}

/*
================
rvTramGate::Event_CloseGate
================
*/
void rvTramGate::Event_CloseGate() {
	CloseGate();
}

/*
================
rvTramGate::Event_IsOpen
================
*/
void rvTramGate::Event_IsOpen( void ) {
	idThread::ReturnInt( IsOpen() );
}

/*
================
rvTramGate::Event_IsLocked
================
*/
void rvTramGate::Event_IsLocked( void ) {
	idThread::ReturnInt( (IsDoorMasterValid()) ? GetDoorMaster()->IsLocked() : 0 );
}

/*
================
rvTramGate::Event_Lock
================
*/
void rvTramGate::Event_Lock( int f ) {
	if( IsDoorMasterValid() ) {
		GetDoorMaster()->Lock( f );
	}
}
