#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


//==========================================================================
//
//	hhBindController
//
//	These can be bound to anything.  They can be used to attach a player, monster, almost any entity
//	and take control of their movement through space.  Players are optionally allowed to look around
//	while bound.  This class was necessary to abstract all the physics differences away from the
//	objects doing the binding.
//==========================================================================

const idEventDef EV_BindAttach("bindattach", "ed");
const idEventDef EV_BindDetach("binddrop", NULL);
const idEventDef EV_BindAttachBody("bindattachbody", "edd");

CLASS_DECLARATION(idEntity, hhBindController)
	EVENT( EV_BindAttach,		hhBindController::Event_Attach )
	EVENT( EV_BindDetach,		hhBindController::Event_Detach )
	EVENT( EV_BindAttachBody,	hhBindController::Event_AttachBody )
END_CLASS


void hhBindController::Spawn() {
	boundRider = NULL;
	hand = NULL;
	yawLimit = 180.0f;	// no limit
	float tension = spawnArgs.GetFloat("tension", "0.01");
	const char *anim = spawnArgs.GetString("boundanim");
	const char *hand = spawnArgs.GetString("def_hand");
	float yaw = spawnArgs.GetFloat("yawlimit", "180");
	float pitch = spawnArgs.GetFloat("pitchlimit", "75");
	bOrientPlayer = spawnArgs.GetBool("orientplayer");

	bLooseBind = false;

	SetRiderParameters(anim, hand, yaw, pitch);
	SetTension(tension);
	hangID = 0;
}

hhBindController::~hhBindController() {
	Detach();
}

void hhBindController::Save(idSaveGame *savefile) const {
	boundRider.Save(savefile);
	savefile->WriteObject( hand );
	savefile->WriteBool( bLooseBind );
	savefile->WriteBool( bOrientPlayer );
	savefile->WriteStaticObject( force );
	savefile->WriteString( animationName );
	savefile->WriteString( handName );
	savefile->WriteFloat( yawLimit );
	savefile->WriteFloat(pitchLimit);
	savefile->WriteInt( hangID );
}

void hhBindController::Restore( idRestoreGame *savefile ) {
	boundRider.Restore(savefile);
	savefile->ReadObject( reinterpret_cast<idClass *&>(hand) );
	savefile->ReadBool( bLooseBind );
	savefile->ReadBool( bOrientPlayer );
	savefile->ReadStaticObject( force );
	savefile->ReadString( animationName );
	savefile->ReadString( handName );
	savefile->ReadFloat( yawLimit );
	savefile->ReadFloat(pitchLimit);
	savefile->ReadInt( hangID );
}

idEntity *hhBindController::GetRider() const {
	// Since the entity could be deleted at any time, we get the rider from the force (if used)
	if (bLooseBind) {
		//TODO: Make this force use a safe entityptr too
		return force.GetEntity();
	}
	return boundRider.GetEntity();
}

void hhBindController::SetTension(float tension) {
	force.SetRestoreTime((1.0f - tension)*5.0f);
	force.SetRestoreFactor(tension * g_springConstant.GetFloat());
}

void hhBindController::SetSlack(float slack) {
	force.SetRestoreSlack(slack);
}

void hhBindController::Think() {
	idEntity::Think();	//TODO: This needed?

	if (bLooseBind && GetRider()) {
		idVec3 loc = GetOrigin();
		force.SetTarget(loc);

		//TODO: Cull hangID out of this class if not needed.


		// FIXME: Why is this needed?  (Took out --pdm, test crane/rail before removing)
/*
		idEntity *rider = GetRider();
		if (rider->IsType(idAFEntity_Base::Type)) {		// Update the offset depending on if actor is ragdoll
			idAFEntity_Base *af = static_cast<idAFEntity_Base *>(rider);
			if (af->IsActiveAF()) {
				force.SetEntity(rider, hangID, vec3_origin);
			}
			else {	// non-ragdolled rider
				if (rider->IsType(idActor::Type)) {
					force.SetEntity(rider, 0, af->EyeOffset());
				}
				else {
					force.SetEntity(rider, 0, rider->GetOrigin());
				}
			}
		}
*/
		force.Evaluate(gameLocal.time);
	}
	else if (GetRider() && GetRider()->IsType(hhPlayer::Type) && OrientPlayer()) {
		// This to insure that oriented players don't get out of sync
		hhPlayer *player = static_cast<hhPlayer *>(GetRider());
		idAngles angles = player->GetUntransformedViewAngles();
		player->SetOrientation(GetOrigin(), GetAxis(), angles.ToMat3()[0], angles);
	}
}

// Create our invisible hand to get rid of the weapon
void hhBindController::CreateHand(hhPlayer *player) {
	if (gameLocal.isClient) { //rww
		return;
	}

	if (handName.Length()) {
		ReleaseHand();

		hand = hhHand::AddHand( player, handName.c_str() );
	}
}

void hhBindController::ReleaseHand() {
	if (hand) {
		hand->RemoveHand();
		hand = NULL;
	}
}

void hhBindController::SetRiderParameters(const char *animname, const char *handname, float yaw, float pitch) {
	animationName = animname;
	handName = handname;
	yawLimit = yaw;
	pitchLimit = pitch;
}

bool hhBindController::ValidRider(idEntity *ent) const {
	return (ent != NULL);
}

void hhBindController::Attach(idEntity *ent, bool loose, int bodyID, idVec3 &point) {
	hhPlayer *player = ent->IsType(hhPlayer::Type) ? static_cast<hhPlayer *>(ent) : NULL;

	if (GetRider()==NULL && ValidRider(ent)) {

		bLooseBind = loose;

		if (bLooseBind) {
			idVec3 loc = GetOrigin();
			force.SetEntity(ent, bodyID, point);
			force.SetTarget(loc);
			if (ent->spawnArgs.GetBool("stable_tractor", "0")) {
				force.SetAxisEntity(this);
			} else {
				force.SetAxisEntity(NULL);
			}
			BecomeActive(TH_THINK);
		}
		else {
			boundRider = ent;
			boundRider->SetOrigin(GetOrigin());

			if (player && bOrientPlayer) {
				idAngles angles = player->GetUntransformedViewAngles();
				player->SetOrientation(GetOrigin(), GetAxis(), angles.ToMat3()[0], angles);
			}
			else {
				boundRider->SetAxis(GetAxis());
			}
			boundRider->Bind(this, false);
		}

		// Apply player parameters
		if (player && !gameLocal.isMultiplayer) { //rww - in mp, don't want to restrict players when bound
			player->maxRelativeYaw = yawLimit;
			player->maxRelativePitch = pitchLimit;
			player->bClampYaw = yawLimit < 180.0f;
			CreateHand(player);
//			player->hhweapon->Hide();
		}
		// Play animation on actors (but not players in mp -rww)
		if (ent->IsType(idActor::Type) && !ent->fl.tooHeavyForTractor && (!gameLocal.isMultiplayer || !ent->IsType(hhPlayer::Type))) {
			idActor *actor = static_cast<idActor *>(ent);
			actor->AI_BOUND = true;
			if ( actor->GetHealth() > 0 ) {
				actor->ProcessEvent(&AI_SetAnimPrefix, animationName.c_str());
				actor->SetAnimState( ANIMCHANNEL_LEGS, "Legs_Bound", 4 );
				actor->SetAnimState( ANIMCHANNEL_TORSO, "Torso_Bound", 4 );
			}
			actor->BecameBound(this);
		}

		hangID = 0;
		if (ent->IsType(idAFEntity_Base::Type)) {
			hangID = static_cast<idAFEntity_Base*>(ent)->spawnArgs.GetInt("hangID");
		}
	}
}

void hhBindController::Detach() {
	idEntity *rider = GetRider();
	hhPlayer *player = rider && rider->IsType(hhPlayer::Type) ? static_cast<hhPlayer *>(rider) : NULL;
	if (rider) {
		if (player) {
			player->bClampYaw = false;

			if (gameLocal.isMultiplayer) { //do not fiddle with the player's anims in MP when he is grabbed, let him act normally
				// Release player animation
				player->AI_BOUND = false;
				player->SetAnimState( ANIMCHANNEL_TORSO, "Torso_Idle", 4 );
				player->SetAnimState( ANIMCHANNEL_LEGS, "Legs_Idle", 4 );
			}

			ReleaseHand();
		}
		if (rider->IsType(idActor::Type)) {
			idActor *actor = static_cast<idActor *>(rider);
			actor->AI_BOUND = false;
			actor->BecameUnbound(this);
		}

		if (bLooseBind) {
			force.SetEntity(NULL);
		}
		else {
			rider->Unbind();
			rider->SetAxis(mat3_identity);
			if (player && bOrientPlayer) {
				// Set everything back to normal at current view angles
				idAngles angles = player->GetUntransformedViewAngles();
				player->SetOrientation(GetOrigin(), mat3_identity, angles.ToMat3()[0], angles);
			}
			boundRider = NULL;
		}
	}
}

idVec3	hhBindController::GetRiderBindPoint() const {
	idEntity* rider = GetRider();

	if( !rider ) {
		return vec3_origin;
	}
		
	if( rider->IsType(idActor::Type) ) {
		idActor *actor = static_cast<idActor*>( rider );
		if( actor->IsActiveAF() ) {
			return actor->GetOrigin( GetHangID() );
		} else {
			return actor->GetEyePosition();
		}
	}

	return rider->GetOrigin();
}

void hhBindController::Event_Attach(idEntity *rider, bool loose) {
	Attach(rider, loose);
}

void hhBindController::Event_AttachBody(idEntity *rider, bool loose, int bodyID) {
	Attach(rider, loose, bodyID);
}

void hhBindController::Event_Detach() {
	Detach();
}

