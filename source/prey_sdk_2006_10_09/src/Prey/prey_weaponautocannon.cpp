#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/***********************************************************************

  hhWeaponAutoCannon
	
***********************************************************************/
const idEventDef EV_Weapon_AdjustHeat( "adjustHeat", "f" );
const idEventDef EV_Weapon_GetHeatLevel( "getHeatLevel", "", 'f' );

const idEventDef EV_Weapon_SpawnRearGasFX( "spawnRearGasFX" );
const idEventDef EV_SpawnSparkLocal( "<spawnSparkLocal>", "s" );

const idEventDef EV_Broadcast_AssignLeftRearFx( "<assignLeftRearFx>", "e" );
const idEventDef EV_Broadcast_AssignRightRearFx( "<assignRightRearFx>", "e" );

const idEventDef EV_Weapon_OverHeatNetEvent( "overHeatNetEvent" ); //rww

CLASS_DECLARATION( hhWeapon, hhWeaponAutoCannon )
	EVENT( EV_Weapon_AdjustHeat,				hhWeaponAutoCannon::Event_AdjustHeat )
	EVENT( EV_Weapon_GetHeatLevel,				hhWeaponAutoCannon::Event_GetHeatLevel )
	EVENT( EV_Weapon_SpawnRearGasFX,			hhWeaponAutoCannon::Event_SpawnRearGasFX )
	EVENT( EV_SpawnSparkLocal,					hhWeaponAutoCannon::Event_SpawnSparkLocal )
	EVENT( EV_Broadcast_AssignLeftRearFx,		hhWeaponAutoCannon::Event_AssignLeftRearFx )
	EVENT( EV_Broadcast_AssignRightRearFx,		hhWeaponAutoCannon::Event_AssignRightRearFx )
	EVENT( EV_Weapon_OverHeatNetEvent,			hhWeaponAutoCannon::Event_OverHeatNetEvent )
END_CLASS

/*
================
hhWeaponAutoCannon::Spawn
================
*/
void hhWeaponAutoCannon::Spawn() {
	BecomeActive( TH_TICKER );

	beamSystem.Clear();
	rearGasFxL.Clear();
	rearGasFxR.Clear();
}

/*
================
hhWeaponAutoCannon::~hhWeaponAutoCannon
================
*/
hhWeaponAutoCannon::~hhWeaponAutoCannon() {
	SAFE_REMOVE( beamSystem );
	SAFE_REMOVE( rearGasFxL );
	SAFE_REMOVE( rearGasFxR );
}

/*
================
hhWeaponAutoCannon::ParseDef
================
*/
void hhWeaponAutoCannon::ParseDef( const char *objectname ) {
	hhWeapon::ParseDef( objectname );

	SetHeatLevel( 0.0f );

	InitBoneInfo();

	if (owner.IsValid() && owner.GetEntity() && owner.GetEntity() == gameLocal.GetLocalPlayer()) { //rww - let's make the beam locally, since it is a client entity. no need for the server to do anything.
		ProcessEvent( &EV_SpawnSparkLocal, dict->GetString("beam_spark") );
	}
	//BroadcastBeam( dict->GetString("beam_spark"), EV_SpawnSparkLocal ); 
}

/*
================
hhWeaponAutoCannon::UpdateGUI
================
*/
void hhWeaponAutoCannon::UpdateGUI() {
	if ( GetRenderEntity()->gui[ 0 ] && state != idStr(WP_HOLSTERED) ) {
		GetRenderEntity()->gui[ 0 ]->SetStateFloat( "temperature", GetHeatLevel() );
	}
}

/*
================
hhWeaponAutoCannon::Ticker
================
*/
void hhWeaponAutoCannon::Ticker() {
	idVec3 boneOriginL, boneOriginR;
	idMat3 boneAxisL, boneAxisR;

	if( beamSystem.IsValid() ) {
		GetJointWorldTransform( sparkBoneL, boneOriginL, boneAxisL );
		GetJointWorldTransform( sparkBoneR, boneOriginR, boneAxisR );

		if( (boneOriginL - boneOriginR).Length() > sparkGapSize ) {
			if( !beamSystem->IsHidden() ) {
				beamSystem->Hide();
				SetShaderParm( SHADERPARM_MODE, 0.0f );
			}
		} else if ( owner->CanShowWeaponViewmodel() ) {
			if( beamSystem->IsHidden() ) {
				beamSystem->Show();
				SetShaderParm( SHADERPARM_MODE, 1.0f );
			}
		}

		beamSystem->SetOrigin( boneOriginL );
		beamSystem->SetTargetLocation( boneOriginR );
	}
}

/*
================
hhWeaponAutoCannon::InitBoneInfo
================
*/
void hhWeaponAutoCannon::InitBoneInfo() {
	assert( dict );

	GetJointHandle( dict->GetString("joint_sparkL"), sparkBoneL );
	GetJointHandle( dict->GetString("joint_sparkR"), sparkBoneR );
}

/*
================
hhWeaponAutoCannon::SetHeatLevel
================
*/
void hhWeaponAutoCannon::SetHeatLevel( const float heatLevel ) {
	this->heatLevel = heatLevel;

	SetShaderParm( SHADERPARM_MISC, heatLevel );
}

/*
================
hhWeaponAutoCannon::AdjustHeat
================
*/
void hhWeaponAutoCannon::AdjustHeat( const float amount ) {
	SetHeatLevel( hhMath::ClampFloat(0.0f, 1.0f, GetHeatLevel() + amount) );

//#if _DEBUG
//    gameLocal.Printf("HeatLevel: %.2f\n", GetHeatLevel());
//#endif
}

/*
================
hhWeaponAutoCannon::PresentWeapon
================
*/
void hhWeaponAutoCannon::PresentWeapon( bool showViewModel ) {
	if( IsHidden() || !owner->CanShowWeaponViewmodel() || pm_thirdPerson.GetBool() ) {
		if( beamSystem.IsValid() ) {
			beamSystem->Activate( false );
		}
	} else {
		if( beamSystem.IsValid() ) {
			beamSystem->Activate( true );
		}
	}

	hhWeapon::PresentWeapon( showViewModel );
}

void hhWeaponAutoCannon::Show() {
	if ( beamSystem.IsValid() )
		beamSystem->Show();
	hhWeapon::Show();
}

void hhWeaponAutoCannon::Hide() {
	if ( beamSystem.IsValid() )
		beamSystem->Hide();
	hhWeapon::Hide();
}

void hhWeaponAutoCannon::WriteToSnapshot( idBitMsgDelta &msg ) const {
	hhWeapon::WriteToSnapshot( msg );	

	msg.WriteFloat( GetHeatLevel() );
}

void hhWeaponAutoCannon::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	hhWeapon::ReadFromSnapshot( msg );	

	SetHeatLevel( msg.ReadFloat() );
}

/*
================
hhWeaponAutoCannon::Event_SpawnSparkLocal
================
*/
void hhWeaponAutoCannon::Event_SpawnSparkLocal( const char* defName ) {
	assert(dict); //rww - this could happen if the server sent a beam event before we initialized the weap on the client
	sparkGapSize = dict->GetFloat( "sparkGapSize" );
	
	SAFE_REMOVE( beamSystem );
	beamSystem = hhBeamSystem::SpawnBeam( GetOrigin(), defName, mat3_identity, true );
	if( !beamSystem.IsValid() ) {
		return;
	}

	//rww - this particular beam not a network entity
	beamSystem->fl.networkSync = false;
	beamSystem->fl.clientEvents = true;

	beamSystem->fl.neverDormant = true;
	beamSystem->GetRenderEntity()->weaponDepthHack = true;
	beamSystem->GetRenderEntity()->allowSurfaceInViewID = owner->entityNumber + 1;
}

/*
================
hhWeaponAutoCannon::Event_SpawnRearGasFX
================
*/
void hhWeaponAutoCannon::Event_SpawnRearGasFX() {
	if( !owner->CanShowWeaponViewmodel() || pm_thirdPerson.GetBool() )
		return;

	hhFxInfo fxInfo;

	float fxHeatThreshold = hhMath::ClampFloat( 0.0f, 1.0f, dict->GetFloat("heatThreshold") );
	if( fxHeatThreshold >= GetHeatLevel() ) {
		return;
	}

	//Checking if fx's are done yet.  Only want to get in when fx's are done
	if( !rearGasFxL.IsValid() && !rearGasFxR.IsValid() ) {
		fxInfo.UseWeaponDepthHack( true );
		fxInfo.RemoveWhenDone( true );

		BroadcastFxInfoAlongBone( dict->RandomPrefix("fx_rearGas", gameLocal.random), dict->GetString("joint_rearGasFxL"), &fxInfo, &EV_Broadcast_AssignLeftRearFx, false );
		BroadcastFxInfoAlongBone( dict->RandomPrefix("fx_rearGas", gameLocal.random), dict->GetString("joint_rearGasFxR"), &fxInfo, &EV_Broadcast_AssignRightRearFx, false );

		if( GetHeatLevel() >= 1.0f )
			StartSound( "snd_overheat", SND_CHANNEL_BODY, 0, false, NULL );
		else
			StartSound( "snd_steam_vent", SND_CHANNEL_BODY, 0, false, NULL );
	}
}

/*
================
hhWeaponAutoCannon::Event_AssignLeftRearFx
================
*/
void hhWeaponAutoCannon::Event_AssignLeftRearFx( hhEntityFx* fx ) {
	rearGasFxL = fx;
}

/*
================
hhWeaponAutoCannon::Event_AssignRightRearFx
================
*/
void hhWeaponAutoCannon::Event_AssignRightRearFx( hhEntityFx* fx ) {
	rearGasFxR = fx;
}

/*
================
hhWeaponAutoCannon::Event_AdjustHeat
================
*/
void hhWeaponAutoCannon::Event_AdjustHeat( const float fAmount ) {
	if (gameLocal.isClient) { //rww - don't adjust on client
		return;
	}
	AdjustHeat( fAmount );
}

/*
================
hhWeaponAutoCannon::Event_GetHeatLevel
================
*/
void hhWeaponAutoCannon::Event_GetHeatLevel() {
	idThread::ReturnFloat( GetHeatLevel() );
}

/*
================
hhWeaponAutoCannon::Event_OverHeatNetEvent
================
*/
void hhWeaponAutoCannon::Event_OverHeatNetEvent() { //rww
	if (gameLocal.isClient) {
		return;
	}

	idBitMsg	msg;
	byte		msgBuf[MAX_EVENT_PARAM_SIZE];

	msg.Init( msgBuf, sizeof( msgBuf ) );
	ServerSendEvent( EVENT_OVERHEAT, &msg, false, -1 );
}

/*
================
hhWeaponAutoCannon::ClientReceiveEvent
================
*/
bool hhWeaponAutoCannon::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) { //rww
	switch (event) {
		case EVENT_OVERHEAT: {
			SetState("OverHeated", 10);
			return true;
		}
		default: {
			return hhWeapon::ClientReceiveEvent( event, time, msg );
		}
	}
}


/*
================
hhWeaponAutoCannon::Save
================
*/
void hhWeaponAutoCannon::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( heatLevel );
	savefile->WriteFloat( sparkGapSize );

	beamSystem.Save( savefile );

	savefile->WriteInt( sparkBoneL.view );
	savefile->WriteInt( sparkBoneL.world );

	savefile->WriteInt( sparkBoneR.view );
	savefile->WriteInt( sparkBoneR.world );

	rearGasFxL.Save( savefile );
	rearGasFxR.Save( savefile );
}

/*
================
hhWeaponAutoCannon::Restore
================
*/
void hhWeaponAutoCannon::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( heatLevel );
	savefile->ReadFloat( sparkGapSize );

	beamSystem.Restore( savefile );

	savefile->ReadInt( reinterpret_cast<int &> ( sparkBoneL.view ) );
	savefile->ReadInt( reinterpret_cast<int &> ( sparkBoneL.world ) );

	savefile->ReadInt( reinterpret_cast<int &> ( sparkBoneR.view ) );
	savefile->ReadInt( reinterpret_cast<int &> ( sparkBoneR.world ) );

	rearGasFxL.Restore( savefile );
	rearGasFxR.Restore( savefile );
}
