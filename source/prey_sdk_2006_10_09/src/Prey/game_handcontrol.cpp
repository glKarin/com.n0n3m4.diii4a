#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


CLASS_DECLARATION( hhHand, hhControlHand )
END_CLASS


void hhControlHand::Spawn() {
	bProcessControls = false;
	oldStatus = 0;

	anims[ 0 ][ 0 ] = GetAnimator()->GetAnim("bottom_backward");
	anims[ 0 ][ 1 ] = GetAnimator()->GetAnim("bottom_center");
	anims[ 0 ][ 2 ] = GetAnimator()->GetAnim("bottom_forward");
	anims[ 1 ][ 0 ] = GetAnimator()->GetAnim("center_backward");
	anims[ 1 ][ 1 ] = GetAnimator()->GetAnim("center_center");
	anims[ 1 ][ 2 ] = GetAnimator()->GetAnim("center_forward");
	anims[ 2 ][ 0 ] = GetAnimator()->GetAnim("top_backward");
	anims[ 2 ][ 1 ] = GetAnimator()->GetAnim("top_center");
	anims[ 2 ][ 2 ] = GetAnimator()->GetAnim("top_forward");

	fl.networkSync = true;
}

void hhControlHand::Save(idSaveGame *savefile) const {
	savefile->Write( anims, sizeof(int)*HAND_MATRIX_WIDTH*HAND_MATRIX_HEIGHT );
	savefile->WriteBool( bProcessControls );
	savefile->WriteInt( oldStatus );
}

void hhControlHand::Restore( idRestoreGame *savefile ) {
	savefile->Read( anims, sizeof(int)*HAND_MATRIX_WIDTH*HAND_MATRIX_HEIGHT );
	savefile->ReadBool( bProcessControls );
	savefile->ReadInt( oldStatus );
}

void hhControlHand::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits(bProcessControls, 1);
	msg.WriteBits(oldStatus, 32);

	hhHand::WriteToSnapshot(msg);
}

void hhControlHand::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	bProcessControls = !!msg.ReadBits(1);
	oldStatus = msg.ReadBits(32);

	hhHand::ReadFromSnapshot(msg);
}

void hhControlHand::ClientPredictionThink( void ) {
	RunPhysics();
	
	// HUMANHEAD pdm
	if (thinkFlags & TH_TICKER) {
		Ticker();
	}

	UpdateAnimation();
	UpdateVisuals();
	Present();
}


void hhControlHand::Raise( void ) {
	hhHand::Raise();

	SetShaderParm(4, -MS2SEC(gameLocal.time));		// time
	SetShaderParm(5, 1.0f);							// Dir
}

void hhControlHand::Ready() {
	hhHand::Ready();
	bProcessControls = true;
}

void hhControlHand::PutAway( void ) {
	hhHand::PutAway();

	SetShaderParm(4, -MS2SEC(gameLocal.time));		// time
	SetShaderParm(5, -1.0f);						// Dir
}

void hhControlHand::UpdateControlDirection(idVec3 &dir) {
	int anim;

	int curStatus = dir.DirectionMask();

	if (bProcessControls && oldStatus != curStatus) {
		// Determine which anim group to play. (Up/Down/Normal & Forward/Center/Back)
		int z_index = dir.z < 0 ? 0 : dir.z > 0 ? 2 : 1;
		int x_index = dir.x < 0 ? 0 : dir.x > 0 ? 2 : 1;

		anim = anims[ z_index ][ x_index ];
		GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, 250); 

		// Now determine which left and right to play
		float left = dir.y > 0 ? 1.0f : 0.0f;
		float right = dir.y < 0 ? 1.0f : 0.0f;
			
		GetAnimator()->CurrentAnim( ANIMCHANNEL_ALL )->SetSyncedAnimWeight( 0, left );
		GetAnimator()->CurrentAnim( ANIMCHANNEL_ALL )->SetSyncedAnimWeight( 1, 1.0f );
		GetAnimator()->CurrentAnim( ANIMCHANNEL_ALL )->SetSyncedAnimWeight( 2, right );

		oldStatus = curStatus;
	}
}
