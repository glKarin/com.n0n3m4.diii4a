//----------------------------------------------------------------
// VehicleController.cpp
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "Vehicle.h"

/*
================
rvVehiclePosition::rvVehiclePosition
================
*/
rvVehiclePosition::rvVehiclePosition ( void ) {	
	memset ( &mInputCmd, 0, sizeof(mInputCmd) );
	mInputAngles.Zero ( );

	memset ( &fl, 0, sizeof(fl) );

	mCurrentWeapon  = -1;
	mSoundPart		= -1;
	
	mDriver			= NULL;
	mParent			= NULL;
	
	mEyeOrigin.Zero();
	mEyeAxis.Identity();

	mEyeOffset.Zero();
	mEyeJoint = INVALID_JOINT;
	
	mDriverOffset.Zero ( );
	mDriverJoint = INVALID_JOINT;
}

/*
================
rvVehiclePosition::~rvVehiclePosition
================
*/
rvVehiclePosition::~rvVehiclePosition ( void ) {
	mParts.DeleteContents ( true );
	mWeapons.DeleteContents ( true );
}

void InitIntArray( const idVec3& vec, int array[] ) {
	int ix;
	for( ix = 0; ix < vec.GetDimension(); ++ix ) {
		array[ix] = (int)vec[ix];
	}
}

/*
================
rvVehiclePosition::Init
================
*/
void rvVehiclePosition::Init ( rvVehicle* parent, const idDict& args ) {
	idVec3 garbage;

	mParent    = parent;

	mEyeJoint  = mParent->GetAnimator()->GetJointHandle ( args.GetString ( "eyeJoint" ) );
	// abahr & twhitaker: removed this due to other changes.
	//if( !mParent->GetAnimator()->GetJointTransform(mEyeJoint, gameLocal.GetTime(), garbage, mEyeJointTransform) ) {
		mEyeJointTransform.Identity();
	//}
	//mEyeJointTransform.TransposeSelf();
	mEyeOffset = args.GetVector ( "eyeOffset", "0 0 0" );
	mDeltaEyeAxisScale = args.GetAngles( "deltaEyeAxisScale", "1 1 0" );
	mDeltaEyeAxisScale.Clamp( ang_zero, idAngles(1.0f, 1.0f, 1.0f) );
	InitIntArray( args.GetVector("eyeJointAxisMap", "0 1 2"), mEyeJointAxisMap );
	InitIntArray( args.GetVector("eyeJointDirMap", "1 1 1"), mEyeJointDirMap );

	mAxisOffset = args.GetAngles( "angles_offset" ).ToMat3();

	mDriverJoint = mParent->GetAnimator()->GetJointHandle ( args.GetString ( "driverJoint" ) );
	// abahr & twhitaker: removed this due to other changes.
	//if( !mParent->GetAnimator()->GetJointTransform(mDriverJoint, gameLocal.GetTime(), garbage, mDriverJointTransform) ) {
		mDriverJointTransform.Identity();
	//}
	//mDriverJointTransform.TransposeSelf(	);
	mDriverOffset		= args.GetVector ( "driverOffset", "0 0 0" );
	mDeltaDriverAxisScale = args.GetAngles( "deltaDriverAxisScale", "1 1 0" );
	mDeltaDriverAxisScale.Clamp( ang_zero, idAngles(1.0f, 1.0f, 1.0f) );
	InitIntArray( args.GetVector("driverJointAxisMap", "0 1 2"), mDriverJointAxisMap );
	InitIntArray( args.GetVector("driverJointDirMap", "1 1 1"), mDriverJointDirMap );

	mExitPosOffset		= args.GetVector( "exitPosOffset" );
	mExitAxisOffset		= args.GetAngles( "exitAxisOffset" ).ToMat3();

	mDriverAnim			= args.GetString ( "driverAnim", "driver" );

	fl.driverVisible	= args.GetBool ( "driverVisible", "0" );
	fl.engine			= args.GetBool ( "engine", "0" );
	fl.depthHack		= args.GetBool ( "depthHack", "1" );
	fl.bindDriver		= args.GetBool ( "bindDriver", "1" );

  	args.GetString ( "internalSurface", "", mInternalSurface );
	
	SetParts ( args );

	mSoundMaxSpeed = args.GetFloat ( "maxsoundspeed", "0" );

	// Looping sound when occupied?
	if ( *args.GetString ( "snd_loop", "" ) ) {
		mSoundPart = AddPart ( rvVehicleSound::GetClassType(), args );
		static_cast<rvVehicleSound*>(mParts[mSoundPart])->SetAutoActivate ( false );
	}

	SelectWeapon ( 0 );
	
	UpdateInternalView ( true );
}

/*
================
rvVehiclePosition::SetParts
================
*/
void rvVehiclePosition::SetParts ( const idDict& args ) {
	const idKeyValue* kv;
	
	// Spawn all parts
	kv = args.MatchPrefix( "def_part", NULL );
	while ( kv ) {
		const idDict*	dict;
		idTypeInfo*		typeInfo;
		
		// Skip empty strings
		if ( !kv->GetValue().Length() ) {
			kv = args.MatchPrefix( "def_part", kv );
			continue;
		}
		
		// Get the dictionary for the part
		dict = gameLocal.FindEntityDefDict ( kv->GetValue() );
		if ( !dict ) {
			gameLocal.Error ( "Invalid vehicle part definition '%'", kv->GetValue().c_str() );
		}
		
		// Determine the part type
		typeInfo = idClass::GetClass ( dict->GetString ( "spawnclass" ) );
		if ( !typeInfo || !typeInfo->IsType ( rvVehiclePart::GetClassType() ) ) {
			gameLocal.Error ( "Class '%s' is not a vehicle part", dict->GetString ( "spawnclass" ) );
		}

		// Add the new part
		AddPart ( *typeInfo, *dict );
		
		kv = args.MatchPrefix( "def_part", kv );
	}
}

/*
================
rvVehiclePosition::AddPart
================
*/
int rvVehiclePosition::AddPart ( const idTypeInfo &classdef, const idDict& args ) {
	rvVehiclePart*	part;
	int				soundChannel;
	
	// Get a sound channel
	soundChannel = SND_CHANNEL_CUSTOM;
	soundChannel += mParts.Num();
	
	// Allocate the new part	
	part = static_cast<rvVehiclePart*>(classdef.CreateInstance ( ));
	part->Init ( this, args, (s_channelType)soundChannel );					
	part->CallSpawn ( );

	// Weapons go into their own list since only one can be active
	// and any given point in time.
	if ( part->IsType ( rvVehicleWeapon::GetClassType() ) ) {
		return mWeapons.Append ( part );
	}

	return mParts.Append ( part );	
}

/*
================
rvVehiclePosition::SetDriver
================
*/
bool rvVehiclePosition::SetDriver ( idActor* driver ) {
	if ( mDriver == driver ) {
		return false;
	}

	fl.inputValid = false;	

	if ( driver ) {
		mDriver = driver;

		// Keep the driver visible if the position is exposed
		if ( fl.driverVisible ) {
			mDriver->Show ( );
		} else {
			mDriver->Hide ( );
		
			// Dont let the player take damage when inside the if not visible
			mDriver->fl.takedamage = false;
		}		

		if ( mSoundPart != -1 ) {
			static_cast<rvVehicleSound*>(mParts[mSoundPart])->Play ( );
		}
		
		// Bind the driver to a joint or to the vehicles origin?
		if( fl.bindDriver ) {
			if ( INVALID_JOINT != mDriverJoint ) {
				mDriver->GetPhysics()->SetAxis ( mParent->GetAxis( ) );// Not sure if this is needed
				mDriver->BindToJoint ( mParent, mDriverJoint, true );
				mDriver->GetPhysics()->SetOrigin ( vec3_origin );
			} else {
				mDriver->Bind ( mParent, true );
				mDriver->GetPhysics()->SetOrigin( mDriverOffset );
			}
		} else {// If not bound put the vehicle first in the active list
			mParent->activeNode.Remove();
			mParent->activeNode.AddToFront( gameLocal.activeEntities );
		}
		
		// Play a certain animation on the driver
		if ( mDriverAnim.Length() ) {
			mDriver->GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, mDriver->GetAnimator()->GetAnim( mDriverAnim ), gameLocal.time, 0 );
		}
	} else {
		if ( !fl.driverVisible ) {
			mDriver->Show ( );

			// Take damage again
			mDriver->fl.takedamage = true;
		}		

		// Driver is no longer bound to the vehicle
		mDriver->Unbind();
		
		mDriver = driver;

		if ( mSoundPart != -1 ) {
			static_cast<rvVehicleSound*>(mParts[mSoundPart])->Stop ( );
		}
		
		// Clear out the input commands so the guns dont keep firing and what not when
		// the player gets out
		memset ( &mInputCmd, 0, sizeof(mInputCmd) );
		mInputAngles.Zero ( );
	}
	
	return true;
}

/*
================
rvVehiclePosition::GetAxis
================
*/
idMat3 rvVehiclePosition::GetAxis() const {
	return mAxisOffset * GetParent()->GetPhysics()->GetAxis();
}

/*
================
rvVehiclePosition::GetOrigin
================
*/
idVec3 rvVehiclePosition::GetOrigin( const idVec3& offset ) const {
	return GetParent()->GetPhysics()->GetOrigin() + (mDriverOffset + offset) * GetAxis();
}

/*
================
rvVehiclePosition::ActivateParts
================
*/
void rvVehiclePosition::ActivateParts ( bool active ) {
	int i;
	
	// Activate or deactive the parts based on whether there is a driver	
	for ( i = mParts.Num() - 1; i >= 0; i -- ) {
		mParts[i]->Activate ( active );
	}

	for ( i = mWeapons.Num() - 1; i >= 0; i -- ) {
		mWeapons[i]->Activate ( active );
	}
}

/*
================
rvVehiclePosition::EjectDriver
================
*/
bool rvVehiclePosition::EjectDriver ( bool force ) {
	if ( !IsOccupied ( ) ) {
		return false;
	}

	// Physically eject the actor from the position
	if ( !force && mParent->IsLocked ( ) ) {
		mParent->IssueLockedWarning ( );
		return false;
	}
	
	// Remove the driver and if successful disable all parts for
	// this position
	if ( SetDriver ( NULL ) ) {
		ActivateParts ( false );
		return true;
	}

	return false;
}

/*
================
rvVehiclePosition::SetInput
================
*/
void rvVehiclePosition::SetInput ( const usercmd_t& cmd, const idAngles& newAngles ) {
	if ( gameDebug.IsHudActive ( DBGHUD_VEHICLE ) ) {
		if ( mDriver == gameLocal.GetLocalPlayer ( ) ) {
			gameDebug.SetFocusEntity ( mParent );
		}
	}

	if ( fl.inputValid || !mDriver ) {
		mOldInputCmd = mInputCmd;
		mOldInputAngles = mInputAngles;
	} else {
		mOldInputCmd = cmd;
		mInputCmd = cmd;
		mInputAngles = newAngles;
		
		// We have valid input now and there is a driver so activate all of the parts
		if ( !fl.inputValid && mDriver ) {
			ActivateParts ( true );
		}
	}
	
	fl.inputValid = true;
	mInputCmd = cmd;
	mInputAngles = newAngles;
}

/*
================
rvVehiclePosition::GetInput
================
*/
void rvVehiclePosition::GetInput( usercmd_t& cmd, idAngles& newAngles ) const {
	cmd = mInputCmd;
	newAngles = mInputAngles;
}

/*
================
rvVehiclePosition::UpdateHUD
================
*/
void rvVehiclePosition::UpdateHUD ( idUserInterface* gui ) {

	// HACK: twhitaker: this is ugly but since the GEV and the Walker now have a unified HUD, it is a necessary evil
	int guiWeaponID;

	if ( !stricmp( mParent->spawnArgs.GetString( "classname" ), "vehicle_walker" ) ) {
		guiWeaponID = mCurrentWeapon + 2;
	} else {
		guiWeaponID = mCurrentWeapon;
	}

	gui->SetStateInt ( "vehicle_weapon", guiWeaponID );
	// HACK: twhitaker end

	gui->SetStateInt ( "vehicle_weaponcount", mWeapons.Num() );
	if ( mCurrentWeapon >= 0 && mCurrentWeapon < mWeapons.Num() ) {
		rvVehicleWeapon* weapon;
		weapon = static_cast<rvVehicleWeapon*>(mWeapons[mCurrentWeapon]);
		gui->SetStateFloat ( "vehicle_weaponcharge", weapon->GetCurrentCharge() );
		gui->SetStateInt ( "vehicle_weaponammo", weapon->GetCurrentAmmo() );
	}	

	// Calculate the rotation of the view in relation to the vehicle itself			
	gui->SetStateFloat ( "vehicle_rotate", -idMath::AngleDelta ( mEyeAxis.ToAngles()[YAW], mParent->GetAxis ( ).ToAngles()[YAW] ) );

	if( GetParent() ) {
		GetParent()->UpdateHUD( GetDriver(), gui );
	}
}

/*
================
rvVehiclePosition::UpdateCursorGUI
================
*/
void rvVehiclePosition::UpdateCursorGUI ( idUserInterface* gui ) {
	if ( mCurrentWeapon < 0 || mCurrentWeapon >= mWeapons.Num() ) {
		return;
	}
	
	rvVehicleWeapon* weapon;
	weapon = static_cast<rvVehicleWeapon*>(mWeapons[mCurrentWeapon]);
	assert ( weapon );
	weapon->UpdateCursorGUI ( gui );
}

/*
================
rvVehiclePosition::UpdateInternalView
================
*/
void rvVehiclePosition::UpdateInternalView ( bool force ) {
	bool internal = false;

	// If the local player is driving and not in third person then use internal
	internal = ( mDriver == gameLocal.GetLocalPlayer() && !pm_thirdPerson.GetInteger() );

	// force the update?
	if ( !force && internal == fl.internalView ) {
		return;
	}
	
	fl.internalView = internal;

	if ( fl.depthHack ) {
		mParent->GetRenderEntity()->weaponDepthHackInViewID = (IsOccupied() && fl.internalView) ? GetDriver()->entityNumber + 1 : 0;
	}
	
	// Show and hide the internal surface
	if ( mInternalSurface.Length() ) {
		mParent->ProcessEvent ( fl.internalView ? &EV_ShowSurface : &EV_HideSurface, mInternalSurface.c_str() );
	}
}

/*
================
rvVehiclePosition::RunPrePhysics
================
*/
void rvVehiclePosition::RunPrePhysics ( void ) {
	int i;

	UpdateInternalView ( );	

	if ( mParent->IsStalled() ) {
		if ( !fl.stalled ) {
			// we just stalled
			fl.stalled = true;
			ActivateParts( false );
		}
		return;
	} else if ( IsOccupied() && !mParent->IsStalled() && fl.stalled ) {
		// we just restarted
		fl.stalled = false;
		ActivateParts( true );
	}

	// Give the parts a chance to set up for physics
	for ( i =  mParts.Num() - 1; i >= 0; i -- ) {
		assert ( mParts[i] );
		mParts[i]->RunPrePhysics ( );
	}

	if ( mCurrentWeapon >= 0 ) {
		mWeapons[mCurrentWeapon]->RunPrePhysics ( );
	}
	
	// Run physics for each part
	for ( i =  mParts.Num() - 1; i >= 0; i -- ) {
		assert ( mParts[i] );

		mParts[i]->RunPhysics ( );
	}

	// Attenuate the attached sound if speed based
	if ( mSoundPart != -1 && mSoundMaxSpeed > 0.0f ) {
		float	f;
		float	speed;
		idVec3	vel;
		
		// Only interested in forward or backward velocity	
		vel   = mParent->GetPhysics()->GetLinearVelocity ( ) * mParent->GetPhysics()->GetAxis ( );
		speed = idMath::ClampFloat ( 0.0f, mSoundMaxSpeed, vel.Normalize ( ) );		
		f     = speed / mSoundMaxSpeed;
		
		static_cast<rvVehicleSound*>(mParts[mSoundPart])->Attenuate ( f, f );
	}
	
	if ( mCurrentWeapon >= 0 ) {
		mWeapons[mCurrentWeapon]->RunPhysics ( );
	}
}

/*
================
rvVehiclePosition::RunPostPhysics
================
*/
void rvVehiclePosition::RunPostPhysics ( void ) {
	int i;
	for ( i = 0; i < mParts.Num(); i ++ ) {
		assert ( mParts[i] );
		mParts[i]->RunPostPhysics ( );
	}
	
	if ( mCurrentWeapon >= 0 ) {
		mWeapons[mCurrentWeapon]->RunPostPhysics ( );
	}

	if ( !IsOccupied ( ) ) {
		return;
	}
	
	if ( fl.inputValid ) {
		if ( mInputCmd.upmove > 0 && !mParent->IsLocked() ) {
			// inform the driver that its time to get out of the vehicle.
			if( !mDriver->EventIsPosted(&AI_ExitVehicle) ) {
				mDriver->PostEventMS( &AI_ExitVehicle, 250, false );// To remove jump when exiting
			}
			
			// If the position isnt occupied anymore then the vehicle exit was successful and there
			// is nothing else to do
			if ( !IsOccupied ( ) ) {
				return;
			}
		} else if ( (mInputCmd.flags & UCF_IMPULSE_SEQUENCE) != (mOldInputFlags & UCF_IMPULSE_SEQUENCE) ) {
			int i;
		
			if ( mInputCmd.impulse >= IMPULSE_0 && mInputCmd.impulse <= IMPULSE_12 ) {
				SelectWeapon( mInputCmd.impulse - IMPULSE_0 );
			}

			if ( mWeapons.Num () ) {				
				switch ( mInputCmd.impulse ) {
					case IMPULSE_14:
						SelectWeapon ( (mCurrentWeapon + mWeapons.Num() + 1) % mWeapons.Num() );
						break;

					case IMPULSE_15:
						SelectWeapon ( (mCurrentWeapon + mWeapons.Num() - 1) % mWeapons.Num() );
						break;
				}
			}
				
			for( i = 0; i < mParts.Num(); i++ ) {
				mParts[ i ]->Impulse ( mInputCmd.impulse );
			}		
							
			mOldInputFlags = mInputCmd.flags;
		}
	}

	// Update the eye origin and axis
	GetEyePosition( mEyeOrigin, mEyeAxis );

	if ( g_debugVehicle.GetInteger() == 2 ) {
		gameRenderWorld->DebugArrow( colorMagenta, mEyeOrigin, mEyeOrigin + mEyeAxis[0] * 30.0f, 3 );
	}		
}

/*
================
rvVehiclePosition::GetEyePosition
================
*/
void rvVehiclePosition::GetEyePosition( idVec3& origin, idMat3& axis ) const {
	GetPosition( mEyeJoint, mEyeOffset, mEyeJointTransform, mDeltaEyeAxisScale, mEyeJointAxisMap, mEyeJointDirMap, origin, axis );
	axis *= GetParent()->GetAxis();
}

/*
================
rvVehiclePosition::GetDriverPosition
================
*/
void rvVehiclePosition::GetDriverPosition( idVec3& origin, idMat3& axis ) const {
	if( fl.bindDriver ) {
		GetPosition( mDriverJoint, mDriverOffset, mDriverJointTransform, mDeltaDriverAxisScale, mDriverJointAxisMap, mDriverJointDirMap, origin, axis );
	} else {
		origin = GetDriver()->GetPhysics()->GetOrigin();
		axis = GetDriver()->viewAxis;
	}
}

/*
================
rvVehiclePosition::GetPosition
================
*/
void rvVehiclePosition::GetPosition( const jointHandle_t jointHandle, const idVec3& offset, const idMat3& jointTransform, const idAngles& scale, const int axisMap[], const int dirMap[], idVec3& origin, idMat3& axis ) const {
	if( GetParent()->GetAnimator()->GetJointTransform(jointHandle, gameLocal.GetTime(), origin, axis) ) {
		axis *= jointTransform;
		idAngles ang( axis.ToAngles().Remap(axisMap, dirMap).Scale(scale) );
		axis = ang.Normalize360().ToMat3() * mAxisOffset;
		
		origin = GetParent()->GetOrigin() + (origin + offset * axis) * GetParent()->GetAxis();
	} else {
		origin = GetOrigin( mEyeOffset );
		axis   = GetAxis();
	}
}

/*
================
rvVehiclePosition::FireWeapon
================
*/
void rvVehiclePosition::FireWeapon( void ) {
	if ( mCurrentWeapon >= 0 ) {
		static_cast<rvVehicleWeapon*>( mWeapons[mCurrentWeapon] )->Fire();
	}
}

/*
================
rvVehiclePosition::SelectWeapon
================
*/
void rvVehiclePosition::SelectWeapon ( int weapon ) {
	if ( weapon < 0 || weapon >= mWeapons.Num ( ) ) {
		return;
	}

// mekberg: clear effect
	if ( mCurrentWeapon != -1 ) {
		static_cast<rvVehicleWeapon*> ( mWeapons[ mCurrentWeapon ] )->StopTargetEffect( );
	}
	
	mCurrentWeapon = weapon;
}

/*
================
rvVehiclePosition::WriteToSnapshot
================
*/
void rvVehiclePosition::WriteToSnapshot ( idBitMsgDelta &msg ) const {
	msg.WriteBits ( mCurrentWeapon, 3 );
}

/*
================
rvVehiclePosition::ReadFromSnapshot
================
*/
void rvVehiclePosition::ReadFromSnapshot ( const idBitMsgDelta &msg ) {
	mCurrentWeapon = msg.ReadBits ( 3 );
}

/*
================
rvVehiclePosition::Save
================
*/
void rvVehiclePosition::Save ( idSaveGame* savefile ) const {
	int i;

	savefile->WriteUsercmd( mInputCmd );
	savefile->WriteAngles ( mInputAngles );
	savefile->WriteUsercmd ( mOldInputCmd );
	savefile->WriteAngles ( mOldInputAngles );
	savefile->WriteInt ( mOldInputFlags );
	
	savefile->WriteInt ( mCurrentWeapon );
	
	mDriver.Save ( savefile );
	mParent.Save ( savefile );
	
	savefile->WriteInt ( mParts.Num ( ) );
	for ( i = 0; i < mParts.Num(); i ++ ) {
		savefile->WriteString ( mParts[i]->GetClassname() );
		savefile->WriteStaticObject ( *mParts[i] );
	}

	savefile->WriteInt ( mWeapons.Num ( ) );
	for ( i = 0; i < mWeapons.Num(); i ++ ) {
		savefile->WriteString ( mWeapons[i]->GetClassname() );
		savefile->WriteStaticObject ( *mWeapons[i] );
	}
	
	savefile->WriteVec3 ( mEyeOrigin );
	savefile->WriteMat3 ( mEyeAxis );

	savefile->WriteJoint ( mEyeJoint );
	savefile->WriteVec3 ( mEyeOffset );
	savefile->WriteMat3( mEyeJointTransform );
	savefile->WriteAngles( mDeltaEyeAxisScale );
	savefile->Write ( mEyeJointAxisMap, sizeof mEyeJointAxisMap );
	savefile->Write ( mEyeJointDirMap, sizeof mEyeJointDirMap );

	savefile->WriteMat3 ( mAxisOffset );

	savefile->WriteJoint ( mDriverJoint );
	savefile->WriteVec3 ( mDriverOffset );
	savefile->WriteMat3 ( mDriverJointTransform );
	savefile->WriteAngles( mDeltaDriverAxisScale );
	savefile->Write ( mDriverJointAxisMap, sizeof mDriverJointAxisMap );
	savefile->Write ( mDriverJointDirMap, sizeof mDriverJointDirMap );

	savefile->WriteVec3 ( mExitPosOffset );
	savefile->WriteMat3 ( mExitAxisOffset );

	savefile->WriteString ( mDriverAnim );

	savefile->WriteString ( mInternalSurface );

	savefile->Write ( &fl, sizeof ( fl ) );

	savefile->WriteInt ( mSoundPart );
	savefile->WriteFloat ( mSoundMaxSpeed );
}

/*
================
rvVehiclePosition::Restore
================
*/
void rvVehiclePosition::Restore ( idRestoreGame* savefile ) {
	int i;
	int num;

	savefile->ReadUsercmd( mInputCmd );
	savefile->ReadAngles ( mInputAngles );
	savefile->ReadUsercmd ( mOldInputCmd );
	savefile->ReadAngles ( mOldInputAngles );
	savefile->ReadInt ( mOldInputFlags );
	
	savefile->ReadInt ( mCurrentWeapon );
	
	mDriver.Restore ( savefile );
	mParent.Restore ( savefile );

	savefile->ReadInt ( num );
	mParts.Clear ( );
	mParts.SetNum ( num );	
	for ( i = 0; i < num; i ++ ) {
		idStr			spawnclass;
		rvVehiclePart*	part;
		idTypeInfo*		typeInfo;		
		
		savefile->ReadString ( spawnclass );

		// Determine the part type
		typeInfo = idClass::GetClass ( spawnclass );
		if ( !typeInfo || !typeInfo->IsType ( rvVehiclePart::GetClassType() ) )
		{
			gameLocal.Error ( "Class '%s' is not a vehicle part", spawnclass.c_str() );
		}
		
		part = static_cast<rvVehiclePart*>(typeInfo->CreateInstance ( ));
		savefile->ReadStaticObject ( *part );
		mParts[i] = part;
	}

	savefile->ReadInt ( num );
	mWeapons.Clear ( );
	mWeapons.SetNum ( num );	
	for ( i = 0; i < num; i ++ ) {
		idStr			spawnclass;
		rvVehiclePart*	part;
		idTypeInfo*		typeInfo;		
		
		savefile->ReadString ( spawnclass );

		// Determine the part type
		typeInfo = idClass::GetClass ( spawnclass );
		if ( !typeInfo || !typeInfo->IsType ( rvVehiclePart::GetClassType() ) )
		{
			gameLocal.Error ( "Class '%s' is not a vehicle part", spawnclass.c_str() );
		}
		
		part = static_cast<rvVehiclePart*>(typeInfo->CreateInstance ( ));
		savefile->ReadStaticObject ( *part );
		mWeapons[i] = part;
	}
	
	savefile->ReadVec3 ( mEyeOrigin );
	savefile->ReadMat3 ( mEyeAxis );

	savefile->ReadJoint ( mEyeJoint );
	savefile->ReadVec3 ( mEyeOffset );
	savefile->ReadMat3 ( mEyeJointTransform );
	savefile->ReadAngles( mDeltaEyeAxisScale );
	savefile->Read( mEyeJointAxisMap, sizeof mEyeJointAxisMap );
	savefile->Read( mEyeJointDirMap, sizeof mEyeJointDirMap );

	savefile->ReadMat3 ( mAxisOffset );

	savefile->ReadJoint ( mDriverJoint );
	savefile->ReadVec3 ( mDriverOffset );
	savefile->ReadMat3 ( mDriverJointTransform );
	savefile->ReadAngles( mDeltaDriverAxisScale );
	savefile->Read( mDriverJointAxisMap, sizeof mDriverJointAxisMap );
	savefile->Read( mDriverJointDirMap, sizeof mDriverJointDirMap );

	savefile->ReadVec3 ( mExitPosOffset );
	savefile->ReadMat3 ( mExitAxisOffset );

	savefile->ReadString ( mDriverAnim );

	savefile->ReadString ( mInternalSurface );

	savefile->Read ( &fl, sizeof(fl) );
	savefile->ReadInt ( mSoundPart );
	savefile->ReadFloat ( mSoundMaxSpeed );
}
































