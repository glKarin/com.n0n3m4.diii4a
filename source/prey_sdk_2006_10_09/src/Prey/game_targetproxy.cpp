// Game_targetproxy.cpp
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

//==========================================================================
//
//	hhModelProxy
//
//	Keeps model/anims in sync with another entity
//==========================================================================
CLASS_DECLARATION(hhAnimatedEntity, hhModelProxy)
END_CLASS

void hhModelProxy::Spawn() {
	renderEntity.noSelfShadow = true;
	renderEntity.noShadow = true;

	GetPhysics()->SetContents(0);

	BecomeActive(TH_THINK|TH_TICKER);

	// Required so that models move in place.
	GetAnimator()->RemoveOriginOffset( true );

	original = NULL;
}

void hhModelProxy::Save(idSaveGame *savefile) const {
	original.Save(savefile);
	owner.Save(savefile);
}

void hhModelProxy::Restore( idRestoreGame *savefile ) {
	original.Restore(savefile);
	owner.Restore(savefile);
}

void hhModelProxy::WriteToSnapshot( idBitMsgDelta &msg ) const {
	GetPhysics()->WriteToSnapshot(msg);
	WriteBindToSnapshot(msg);

	msg.WriteBits(original.GetSpawnId(), 32);
	msg.WriteBits(owner.GetSpawnId(), 32);

	msg.WriteBits(renderEntity.allowSurfaceInViewID, GENTITYNUM_BITS);
}

void hhModelProxy::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	GetPhysics()->ReadFromSnapshot(msg);
	ReadBindFromSnapshot(msg);

	original.SetSpawnId(msg.ReadBits(32));
	owner.SetSpawnId(msg.ReadBits(32));

	renderEntity.allowSurfaceInViewID = msg.ReadBits(GENTITYNUM_BITS);
}

void hhModelProxy::ClientPredictionThink( void ) {
	Think();
}


/*
===============
hhModelProxy::SetOriginAndAxis
Note: This was ripped from idEntity::UpdateModelTransform (Was ripped from idEntity)
===============
*/
void hhModelProxy::SetOriginAndAxis( idEntity *entity ) {
	idVec3 origin;
	idMat3 axis;
	

	if ( original->GetPhysicsToVisualTransform( origin, axis ) ) {
		axis = axis * original->GetPhysics()->GetAxis();
		origin = original->GetPhysics()->GetOrigin() + origin * axis;
	}
	else {
		axis = original->GetPhysics()->GetAxis();
		origin = original->GetPhysics()->GetOrigin();
	}

	SetOrigin( origin );
	SetAxis( axis );
}


void hhModelProxy::SetOriginal(idEntity *other) {
	original = other;
	if (original.IsValid() && original.GetEntity()) {
		SetOriginAndAxis( original.GetEntity() );
		Bind(original.GetEntity(), true);
	}
}

idEntity *hhModelProxy::GetOriginal() {
	if (!original.IsValid()) {
		return NULL;
	}
	return original.GetEntity();
}

void hhModelProxy::SetOwner(hhPlayer *other) {
	owner = other;
	renderEntity.allowSurfaceInViewID = owner->entityNumber+1;
}

hhPlayer *hhModelProxy::GetOwner() {
	if (!owner.IsValid()) {
		return NULL;
	}
	return owner.GetEntity();
}

void hhModelProxy::Ticker() {
	if (!owner.IsValid() || !original.IsValid()) { //rww - added validation for "original"
		PostEventMS(&EV_Remove, 0);
		return;
	}
	UpdateVisualState();
}

void hhModelProxy::UpdateVisualState() {

	// Update visual state to state of original
	if (original.IsValid() && original.GetEntity() && !fl.hidden) {
		if (GetRenderEntity()->hModel != original->GetRenderEntity()->hModel) {
			const char *modelname;

			if (gameLocal.isMultiplayer && (original->IsType(hhPlayer::Type) || original->IsType(hhSpiritProxy::Type))) {
				//rww - players and spirit proxies in mp have variable model names, but keep the original "model" field
				//as a fallback.
				modelname = original->spawnArgs.GetString( "playerModel" );
				if (!modelname || !modelname[0]) { //empty string, fallback to "model"
					modelname = original->spawnArgs.GetString( "model" );
				}
			}
			else {
				modelname = original->spawnArgs.GetString( "model" );
			}

			SetModel(modelname);
		}

		// Run our physics to keep our binding to the original up to date.
		GetPhysics()->Evaluate( gameLocal.time - gameLocal.previousTime, gameLocal.time );

		if ( original->GetAnimator() ) {
			GetAnimator()->CopyAnimations( *( original->GetAnimator() ) );
			GetAnimator()->CopyPoses( *( original->GetAnimator() ) );
		}
		
		UpdateVisuals();
	}
}

void hhModelProxy::ProxyFinished() {
	PostEventMS(&EV_Remove, 0);
}


//==========================================================================
//
//	hhTargetProxy
//
//	Special proxy for spirit bow vision that allows sight through walls
//==========================================================================
const idEventDef EV_FadeOutProxy("<fadeoutproxy>");

CLASS_DECLARATION(hhModelProxy, hhTargetProxy)
	EVENT( EV_FadeOutProxy,			hhTargetProxy::Event_FadeOut )
END_CLASS

void hhTargetProxy::Spawn() {
	renderEntity.weaponDepthHack = true;
	SetShaderParm(5, 0.5f + ((float)(entityNumber % 8)) / 8.0f);	// Unique number [0.5 .. 1.5] for jitter

	owner = NULL;

	//TODO: Need a way to be visible only if owner is local client

	fadeAlpha.Init(gameLocal.time, BOWVISION_FADEIN_DURATION, 0.0f, 1.0f);
	StayAlive();

	fl.networkSync = true;
}

void hhTargetProxy::Save(idSaveGame *savefile) const {
	savefile->WriteFloat( fadeAlpha.GetStartTime() );	// idInterpolate<float>
	savefile->WriteFloat( fadeAlpha.GetDuration() );
	savefile->WriteFloat( fadeAlpha.GetStartValue() );
	savefile->WriteFloat( fadeAlpha.GetEndValue() );
}

void hhTargetProxy::Restore( idRestoreGame *savefile ) {
	float set;

	savefile->ReadFloat( set );			// idInterpolate<float>
	fadeAlpha.SetStartTime( set );
	savefile->ReadFloat( set );
	fadeAlpha.SetDuration( set );
	savefile->ReadFloat( set );
	fadeAlpha.SetStartValue(set);
	savefile->ReadFloat( set );
	fadeAlpha.SetEndValue( set );
}

void hhTargetProxy::WriteToSnapshot( idBitMsgDelta &msg ) const {
	hhModelProxy::WriteToSnapshot(msg);

	msg.WriteFloat(fadeAlpha.GetStartTime());
	msg.WriteFloat(fadeAlpha.GetDuration());
	msg.WriteFloat(fadeAlpha.GetStartValue());
	msg.WriteFloat(fadeAlpha.GetEndValue());
}

void hhTargetProxy::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	hhModelProxy::ReadFromSnapshot(msg);

	fadeAlpha.SetStartTime(msg.ReadFloat());
	fadeAlpha.SetDuration(msg.ReadFloat());
	fadeAlpha.SetStartValue(msg.ReadFloat());
	fadeAlpha.SetEndValue(msg.ReadFloat());
}

void hhTargetProxy::ProxyFinished() {
	CancelEvents(&EV_FadeOutProxy);
	fadeAlpha.Init(gameLocal.time, BOWVISION_FADEOUT_DURATION, 1.0f, 0.0f);
	PostEventMS(&EV_Remove, BOWVISION_FADEOUT_DURATION);
}

void hhTargetProxy::UpdateVisualState() {
	//TODO: If owner not in spirit walk, immediately hide and post a remove
	// This should handle the case of being in altmode and toggling spiritwalk off
	// so that we don't see the targets fade out in non-spiritwalk mode

	//TODO: net play support: snapshot owner->entityNumber.  Hide if owner isn't the localclient
//	if (owner->entityNumber == gameLocal.localClientNum) {
//	}


	if( !original.IsValid() || !original.GetEntity() || original->IsHidden() ) { // mdl:  Don't show hidden entities (keeps actors from showing up in bowvision after gibbing)
		//rww - added validity checks for "original"
		Hide();
		return;
	}

	// Check to see if within players view
	idVec3 toProxy = GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin();
	float score = toProxy * (owner->viewAngles.ToMat3()[0]);
	if (score < 0.2f) {
		Hide();
	}
	else {
		Show();
		SetShaderParm(3, fadeAlpha.GetCurrentValue(gameLocal.time));
		hhModelProxy::UpdateVisualState();
	}
}

void hhTargetProxy::StayAlive() {
	// Stay alive for BOWVISION_UPDATE_FREQUENCY longer (MS) + a little slop
	CancelEvents(&EV_Remove);
	CancelEvents(&EV_FadeOutProxy);
	PostEventMS(&EV_FadeOutProxy, BOWVISION_UPDATE_FREQUENCY * 2);

	// Fade back in if fading out, otherwise stay faded in
	float curvalue = fadeAlpha.GetCurrentValue(gameLocal.time);
	fadeAlpha.Init(gameLocal.time, BOWVISION_FADEIN_DURATION, curvalue, 1.0f);

	if ( owner.IsValid() && owner.GetEntity() ) {
		UpdateVisualState();
	}
}

void hhTargetProxy::Event_FadeOut() {
	ProxyFinished();
}
