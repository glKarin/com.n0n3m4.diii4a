
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#include "../prey/ai_speech.h"
#include "../prey/ai_reaction.h"

// HUMANHEAD additions to the idEntity class

//HUMANHEAD
const idEventDef EV_SetSkinByName( "setSkin", "s" );
const idEventDef EV_Deactivate( "deactivate" );
const idEventDef EV_GlobalBeginState( "<globalbeginstate>" );
const idEventDef EV_GlobalTicker( "<globalticker>" );
const idEventDef EV_GlobalEndState( "<globalendstate>" );
const idEventDef EV_GlobalDeactivate( "<globaldeactivate>" );
const idEventDef EV_GlobalActivate( "<globalactivate>", "e" );
const idEventDef EV_DeadBeginState( "<deadbeginstate>" );
const idEventDef EV_DeadTicker( "<deadticker>" );
const idEventDef EV_DeadEndState( "<deadendstate>" );
const idEventDef EV_MoveToJoint( "moveToJoint", "es" );
const idEventDef EV_MoveToJointWeighted( "moveToJointWeighted", "esv" );
const idEventDef EV_MoveJointToJoint( "moveJointToJoint", "ses" );
const idEventDef EV_MoveJointToJointOffset( "moveJointToJointOffset", "sesv" );
const idEventDef EV_SpawnDebris("spawnDebris", "s");
const idEventDef EV_DamageEntity( "<damageEntity>", "es" );
const idEventDef EV_DelayDamageEntity( "delayDamageEntity", "esf" );
const idEventDef EV_Dispose( "dispose" );
const idEventDef EV_ResetGravity( "<resetgravity>", NULL );
const idEventDef EV_LoadReactions("<loadReactions>");
const idEventDef EV_SetDeformation("setDeformation", "dff" );
const idEventDef EV_Broadcast_AssignFx( "<assignFx>", "e" );
const idEventDef EV_Broadcast_AppendFxToList( "<appendFxToList>", "e" );

const idEventDef EV_SetFloatKey( "setFloatKey", "sf" );
const idEventDef EV_SetVectorKey( "setVectorKey", "sv" );
const idEventDef EV_SetEntityKey( "setEntityKey", "se" );

const idEventDef EV_PlayerCanSee( "playerCanSee", NULL, 'd' );

const idEventDef EV_SetPortalCollision( "setPortalCollision", "d" );
const idEventDef EV_KillBox( "killbox", NULL );	// pdm
//HUMANHEAD END


/*
================
idEntity::InitCoreStateInfo

HUMANHEAD: aob - state init code
================
*/
void idEntity::InitCoreStateInfo() {
}

/*
================
idEntity::Ticker
	HUMANHEAD pdm
================
*/
void idEntity::Ticker( void ) {
	// Overridable function for conditionally running logic each frame, seperate from
	// whether you want to run physics, animation, rendering, and sound
}

/*
================
idEntity::RestoreGUI

HUMANHEAD: aob
================
*/
void idEntity::RestoreGUI( const char* guiKey, idUserInterface** gui ) {
	if( guiKey && guiKey[0] && gui ) {
		AddRenderGui( spawnArgs.GetString(guiKey), gui, &spawnArgs );
	}
}

/*
================
idEntity::RestoreGUIs

HUMANHEAD: aob
================
*/
void idEntity::RestoreGUIs() {
	RestoreGUI( "gui", &renderEntity.gui[0] );
	for( int ix = 1; ix < MAX_RENDERENTITY_GUI; ix++ ) {
		RestoreGUI( va("gui%d", ix+1), &renderEntity.gui[ix] );
	}
}

/*
================
idEntity::SetSkinByName
	HUMANHEAD: aob
================
*/
void idEntity::SetSkinByName( const char *skinname ) {
	SetSkin( (!skinname || !skinname[0]) ? NULL : declManager->FindSkin(skinname) );
}

//=========================================================================
//
// idEntity::Portalled
//
// HUMANHEAD
// This entity was just portalled 
//=========================================================================

void idEntity::Portalled(idEntity *portal) { 
	// Scan for any shuttles that are tractoring me and tell it to let go in that case
	for( idEntity *ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->IsType( hhShuttle::Type ) ) {
			hhShuttle *shuttle = static_cast<hhShuttle*>(ent);
			if (shuttle->GetTractorTarget() == this) {
				shuttle->ReleaseTractorTarget();
			}
		}
	}

	return;
}

/*
=====================
idEntity::CheckPortal

Checks if a portal can teleport this entity

HUMANHEAD: cjr
=====================
*/
bool idEntity::CheckPortal( const idEntity *other, int contentMask ) {
	return false;
}

bool idEntity::CheckPortal( const idClipModel *mdl, int contentMask ) {
	return false;
}

void idEntity::CollideWithPortal( const idEntity *other ) { // HUMANHEAD CJR PCF 04/26/06
}

void idEntity::CollideWithPortal( const idClipModel *mdl ) { // HUMANHEAD CJR PCF 04/26/06
}

/*
================
idEntity:MoveToJoint

HUMANHEAD nla
================
*/
void idEntity::MoveToJoint( idEntity *master, const char *bonename ) {
	idVec3 weight(1, 1, 1);

	MoveToJointWeighted( master, bonename, weight);
}

/*
================
idEntity::MoveToJointWeighted

  master - The owner of the bone to move to
  bonename - The name of the bone to move to
  weight - Vector giving the amount of influence the bone will have over the new location.
			(1, 1, 0) would move the player 100% in X and Y, but none in Z.
			(.5, 0, 1) would move the player 1/2 way to the bone in x, none in y, and 100% in z


HUMANHEAD nla
================
*/
void idEntity::MoveToJointWeighted( idEntity *master, const char *bonename, idVec3 &weight ) {
	jointHandle_t		jointnum;
	idVec3				boneOrigin;
	idMat3				boneAxis;
	idVec3				newOrigin;


	if ( master == this ) {
		gameLocal.Error( "Tried to move an object to itself." );
		return;
	}
	
	// Make sure what we are moving to has joints!
	if ( !master->GetAnimator() ) {
		gameLocal.Error( "Tried to move %s (%s) to a joint on non animated entity %s (%s)\n",
						name.c_str(), spawnArgs.GetString( "spawnclass" ),
						master->name.c_str(), master->spawnArgs.GetString( "spawnclass" ) );
	}
	
	jointnum = master->GetAnimator()->GetJointHandle( bonename );
	if ( jointnum == INVALID_JOINT ) {
		gameLocal.Error( "Bone  '%s' not found.", bonename );
		return;
	}

	master->GetAnimator()->GetJointTransform( jointnum, gameLocal.time, boneOrigin, boneAxis );
	
	//gameLocal.Printf("Moving origin from (%s) %s to ", boneOrigin.ToString(), GetPhysics()->GetOrigin().ToString() );

//  boneOrigin is already rotated by the master axis 
	newOrigin = boneOrigin * master->GetAxis() + master->GetOrigin();
	idVec3 myOrigin = GetOrigin();
	newOrigin.x = newOrigin.x * weight.x + myOrigin.x * ( 1 - weight.x );
	newOrigin.y = newOrigin.y * weight.y + myOrigin.y * ( 1 - weight.y );
	newOrigin.z = newOrigin.z * weight.z + myOrigin.z * ( 1 - weight.z );
	
	SetOrigin( newOrigin );
	
	//gameLocal.Printf("%s\n", myOrigin.ToString() );
}

/*
================
idEntity::MoveJointToJoint

HUMANHEAD nla
================
*/
void idEntity::MoveJointToJoint( const char *ourBone, idEntity *master, const char *masterBone ) {
	
	MoveJointToJointOffset( ourBone, master, masterBone, vec3_origin );
}

/*
================
idEntity::MoveJointToJointOffset

  Offset ourBone location in global space.

HUMANHEAD nla
================
*/
void idEntity::MoveJointToJointOffset( const char *ourBone, idEntity *master, const char *masterBone, idVec3 &offset ) {
	jointHandle_t		masterJointnum;
	jointHandle_t		ourJointnum;
	idVec3				masterBoneOrigin;
	idMat3				masterBoneAxis;
	idVec3				ourBoneOrigin;
	idMat3				ourBoneAxis;

	if ( master == this ) {
		gameLocal.Error( "Tried to move an object to itself." );
		return;
	}

	if ( !master->GetAnimator() ) {
		gameLocal.Error( "Tried to move %s (%s) to a joint on non animated entity %s (%s)\n",
						name.c_str(), spawnArgs.GetString( "spawnclass" ),
						master->name.c_str(), master->spawnArgs.GetString( "spawnclass" ) );
	}

	if ( !GetAnimator() ) {
		gameLocal.Error( "Tried to move via a joint from a non animated entity %s (%s) to\n",
						name.c_str(), spawnArgs.GetString( "spawnclass" ) );
	}

	masterJointnum = master->GetAnimator()->GetJointHandle( masterBone );
	if ( masterJointnum == INVALID_JOINT ) {
		gameLocal.Error( "Bone '%s'  not found.", masterBone );
		return;
	}

	master->GetAnimator()->GetJointTransform( masterJointnum, gameLocal.time, masterBoneOrigin, masterBoneAxis );
	
	ourJointnum = GetAnimator()->GetJointHandle( ourBone );
	if ( ourJointnum == INVALID_JOINT ) {
		gameLocal.Error( "Bone '%s' not  found.", ourBone );
		return;
	}

	GetAnimator()->GetJointTransform( ourJointnum, gameLocal.time, ourBoneOrigin, ourBoneAxis );

	//HUMANHEAD: aob - removed GetPhysics calls
	SetOrigin( masterBoneOrigin * master->GetAxis() + master->GetOrigin() - ourBoneOrigin * GetAxis() - offset  );
	//HUMANHEAD END
	
}

/*
================
idEntity::AlignToJoint

HUMANHEAD nla
================
*/
void idEntity::AlignToJoint( idEntity *master, const char *bonename ) {
	jointHandle_t		jointnum;
	idVec3				boneOrigin;
	idMat3				boneAxis;

	if ( master == this ) {
		gameLocal.Error( "Tried to align an object to itself." );
		return;
	}
	
	jointnum = master->GetAnimator()->GetJointHandle( bonename );
	if ( jointnum == INVALID_JOINT ) {
		gameLocal.Error( "Bone  '%s'  not found.", bonename );
		return;
	}

	master->GetAnimator()->GetJointTransform( jointnum, gameLocal.time, boneOrigin, boneAxis );

	/*
	gameLocal.Printf( " Before %.2f %.2f %.2f  %.2f %.2f %.2f  %.2f %.2f %.2f\n",
				GetPhysics()->GetAxis()[0].x, GetPhysics()->GetAxis()[0].y, GetPhysics()->GetAxis()[0].z,
				GetPhysics()->GetAxis()[1].x, GetPhysics()->GetAxis()[1].y, GetPhysics()->GetAxis()[1].z,
				GetPhysics()->GetAxis()[2].x, GetPhysics()->GetAxis()[2].y, GetPhysics()->GetAxis()[2].z );
	*/

	//HUMANHEAD: aob - removed GetPhysics call
	SetAxis( boneAxis * master->GetAxis() );
	//HUMANHEAD END

	/*
	gameLocal.Printf( " After %.2f %.2f %.2f  %.2f %.2f %.2f  %.2f %.2f %.2f\n",
				GetPhysics()->GetAxis()[0].x, GetPhysics()->GetAxis()[0].y, GetPhysics()->GetAxis()[0].z,
				GetPhysics()->GetAxis()[1].x, GetPhysics()->GetAxis()[1].y, GetPhysics()->GetAxis()[1].z,
				GetPhysics()->GetAxis()[2].x, GetPhysics()->GetAxis()[2].y, GetPhysics()->GetAxis()[2].z );
	*/
}

/*
================
idEntity::AlignJointToJoint

HUMANHEAD nla
================
*/
void idEntity::AlignJointToJoint( const char *ourBone, idEntity *master, const char *masterBone ) {
	jointHandle_t		masterJointnum;
	jointHandle_t		ourJointnum;
	idVec3				masterBoneOrigin;
	idMat3				masterBoneAxis;
	idVec3				ourBoneOrigin;
	idMat3				ourBoneAxis;

	if ( master == this ) {
		gameLocal.Error( "Tried to align an object to itself." );
		return;
	}
	
	masterJointnum = master->GetAnimator()->GetJointHandle( masterBone );
	if ( masterJointnum == INVALID_JOINT ) {
		gameLocal.Error( "Bone '%s'  not  found.", masterBone );
		return;
	}

	master->GetAnimator()->GetJointTransform( masterJointnum, gameLocal.time, masterBoneOrigin, masterBoneAxis );
	
	ourJointnum = GetAnimator()->GetJointHandle( ourBone );
	if ( ourJointnum == INVALID_JOINT ) {
		gameLocal.Error( "Bone  '%s'  not  found.", ourBone );
		return;
	}

	GetAnimator()->GetJointTransform( ourJointnum, gameLocal.time, ourBoneOrigin, ourBoneAxis );

	/*
	gameLocal.Printf( " Before B %.2f %.2f %.2f  %.2f %.2f %.2f  %.2f %.2f %.2f\n",
				GetPhysics()->GetAxis()[0].x, GetPhysics()->GetAxis()[0].y, GetPhysics()->GetAxis()[0].z,
				GetPhysics()->GetAxis()[1].x, GetPhysics()->GetAxis()[1].y, GetPhysics()->GetAxis()[1].z,
				GetPhysics()->GetAxis()[2].x, GetPhysics()->GetAxis()[2].y, GetPhysics()->GetAxis()[2].z );
	*/
	// Bad logic!  Fix after fixing the  an anim items
	//HUMANHEAD: aob - removed GetPhysics calls
	SetAxis( ( ( ( masterBoneAxis * master->GetAxis() ).ToAngles() - ( ourBoneAxis * GetAxis() ).ToAngles() ) + GetAxis().ToAngles() ).ToMat3() );
	//HUMANHEAD END

	/*
	gameLocal.Printf( " After B %.2f %.2f %.2f  %.2f %.2f %.2f  %.2f %.2f %.2f\n",
				GetPhysics()->GetAxis()[0].x, GetPhysics()->GetAxis()[0].y, GetPhysics()->GetAxis()[0].z,
				GetPhysics()->GetAxis()[1].x, GetPhysics()->GetAxis()[1].y, GetPhysics()->GetAxis()[1].z,
				GetPhysics()->GetAxis()[2].x, GetPhysics()->GetAxis()[2].y, GetPhysics()->GetAxis()[2].z );
	*/
	// SetAxis( ( ( ( ourBoneAxis * GetPhysics()->GetAxis() ).ToAngles() - ( masterBoneAxis * master->GetPhysics()->GetAxis() ).ToAngles() ) + GetPhysics()->GetAxis().ToAngles() ).ToMat3() );
	
}

/*
==================
idEntity::SpawnFXLocal
==================
*/
//HUMANHEAD rww - added forceClient
hhEntityFx* idEntity::SpawnFxLocal( const char *fxName, const idVec3 &origin, const idMat3& axis, const hhFxInfo* const fxInfo, bool forceClient ) {
	idDict				fxArgs;
	hhEntityFx *		fx = NULL;

	if ( g_skipFX.GetBool() ) {
		return NULL;
	}

	if( !fxName || !fxName[0] ) {
		return NULL;
	}

	// Spawn an fx 
	fxArgs.Set( "fx", fxName );
	fxArgs.SetBool( "start", fxInfo ? fxInfo->StartIsSet() : true );
	fxArgs.SetVector( "origin", origin );
	fxArgs.SetMatrix( "rotation", axis );
	//HUMANHEAD: aob
	if( fxInfo ) {
		fxArgs.SetBool( "removeWhenDone", fxInfo->RemoveWhenDone() );
		fxArgs.SetBool( "onlyVisibleInSpirit", fxInfo->OnlyVisibleInSpirit() ); // CJR
		fxArgs.SetBool( "onlyInvisibleInSpirit", fxInfo->OnlyInvisibleInSpirit() ); // tmj
		fxArgs.SetBool( "toggle", fxInfo->Toggle() );
		fxArgs.SetBool( "triggered", fxInfo->Triggered() ); // mdl
	}		
	//HUMANHEAD END

	//HUMANHEAD rww - use forceClient
	if (forceClient) {
		//this can happen on the "server" in the case of listen servers as well
		fx = (hhEntityFx *)gameLocal.SpawnClientObject( "func_fx", &fxArgs );
	}
	else {
		assert(!gameLocal.isClient);
		fx = (hhEntityFx *)gameLocal.SpawnObject( "func_fx", &fxArgs );
	}
	if( fxInfo ) {
		fx->SetFxInfo( *fxInfo );
	}

	if( fxInfo && fxInfo->EntityIsSet() ) {
		fx->fl.noRemoveWhenUnbound = fxInfo->NoRemoveWhenUnbound();
		if( fxInfo->BindBoneIsSet() ) {
			fx->MoveToJoint( fxInfo->GetEntity(), fxInfo->GetBindBone() );
			fx->BindToJoint( fxInfo->GetEntity(), fxInfo->GetBindBone(), true );
		} else if( fx && fx->Joint() && *fx->Joint() ) {
			fx->MoveToJoint( fxInfo->GetEntity(), fx->Joint() );
			fx->BindToJoint( fxInfo->GetEntity(), fx->Joint(), true );
		} else {
			fx->Bind( fxInfo->GetEntity(), true );
		}
	}

	fx->Show();
	
	return fx;
}

/*
==================
idEntity::SpawnFXPrefixLocal
==================
*/
void idEntity::SpawnFXPrefixLocal( const idDict* dict, const char* fxKeyPrefix, const idVec3& origin, const idMat3& axis, const hhFxInfo* const fxInfo, const idEventDef* eventDef ) {
	hhEntityFx* fx = NULL;

	for( const idKeyValue* kv = dict->MatchPrefix(fxKeyPrefix); kv; kv = dict->MatchPrefix(fxKeyPrefix, kv) ) {
		if( !kv->GetValue().Length() ) {
			continue;
		}

		fx = SpawnFxLocal( kv->GetValue().c_str(), origin, axis, fxInfo, true );
		if( eventDef ) {
			ProcessEvent( eventDef, fx );
		}
	}
}

/*
================
idEntity::GetMasterDefaultPosition

HUMANHEAD: aob - helper function to be overridden
================
*/
void idEntity::GetMasterDefaultPosition( idVec3 &masterOrigin, idMat3 &masterAxis ) const {
	//HUMANHEAD: aob - if modelDefHandle is valid then we know that renderEntity values are valid
	masterOrigin = (bindMaster->modelDefHandle == -1) ? bindMaster->GetOrigin() : bindMaster->GetRenderEntity()->origin;
	masterAxis = (bindMaster->modelDefHandle == -1) ? bindMaster->GetAxis() : bindMaster->GetRenderEntity()->axis;
	//HUMANHEAD END
}		

/*
================
idEntity::Event_SpawnDebris

//HUMANHEAD: jrm
================
*/
void idEntity::Event_SpawnDebris(const char *debrisKey) {
	if (GetPhysics()) {
		idMat3 rot = GetPhysics()->GetAxis();
		idVec3 fw  = rot[0];
		idVec3 orig	= GetPhysics()->GetOrigin();
		idVec3 vel  = rot[2];
		
		idActor *act = NULL;
		if(IsType(idActor::Type)) {
			act = (idActor*)this;
		}
		if(act) {
			orig = act->GetEyePosition();

			if (GetPhysics()->IsType(idPhysics_Monster::Type)) {
				idPhysics_Monster *mp = (idPhysics_Monster*)GetPhysics();
				idVec3 delta = mp->GetDelta();
				vel = mp->GetDelta() * USERCMD_HZ;
			}			
		}
		
		if (debrisKey) {
			const char *debrisName = spawnArgs.GetString(debrisKey, NULL);
			hhUtils::SpawnDebrisMass(debrisName, orig, &fw, &vel);
		}
	}
}

/*
================
idEntity::DrawDebug

HUMANHEAD pdm
================
*/
void idEntity::DrawDebug(int page) {
	idStr text;
	idPhysics *phys = GetPhysics();

	if (phys) {
		hhUtils::DebugAxis(phys->GetOrigin(), phys->GetAxis(), 30, 0);

		idVec3 vel = phys->GetLinearVelocity();
		gameRenderWorld->DebugArrow(colorGreen, phys->GetOrigin(), phys->GetOrigin() + vel, 10);

		sprintf(text, "Mass=%.2f", phys->GetMass());
		idMat3 axis = gameLocal.GetLocalPlayer()->viewAngles.ToMat3();
		gameRenderWorld->DrawText(text, GetOrigin(), 0.4f, colorRed, axis);
	}
}

/*
================
idEntity::FillDebugVars

HUMANHEAD pdm
================
*/
void idEntity::FillDebugVars(idDict *args, int page) {
	idStr text;
	switch(page) {
	case 1:
		args->Set("class", GetClassname());
		args->Set("name", name.c_str());
		args->SetInt("modelHandle", modelDefHandle);
		args->Set("bindMaster", bindMaster ? bindMaster->name.c_str() : "none");
//		args->Set("skin", renderEntity.customSkin ? renderEntity.customSkin->name : "none");
		args->SetInt("health", health);
		if (GetPhysics()) {
			args->Set("physics", GetPhysics()->GetType()->classname);
			if (GetPhysics()->GetNumClipModels() > 1) {
				args->SetInt("Bodies", GetPhysics()->GetNumClipModels());
				for (int ix=0; ix<GetPhysics()->GetNumClipModels(); ix++) {
					text = collisionModelManager->ContentsName(GetPhysics()->GetContents(ix));
					args->Set(va("Body %d contents", ix), text);

					text = collisionModelManager->ContentsName(GetPhysics()->GetClipMask(ix));
					args->Set(va("Body %d clipmask", ix), text);
				}
			}
			else {
				text = collisionModelManager->ContentsName(GetPhysics()->GetContents());
				args->Set("contents", text);

				text = collisionModelManager->ContentsName(GetPhysics()->GetClipMask());
				args->Set("clipmask", text);
			}
		}
		else {
			args->Set("physics", "none");
		}
		args->SetBool("dormant", fl.isDormant);

		text.Empty();
		if (fl.noRemoveWhenUnbound)	text += "noRemoveWhenUnbound ";
		if (fl.refreshReactions)	text += "refreshReactions ";
		if (fl.touchTriggers)		text += "touchTriggers ";
		if (fl.notarget)			text += "notarget ";
		if (fl.noknockback)			text += "noknockback ";
		if (fl.takedamage)			text += "takedamage ";
		if (fl.hidden)				text += "hidden ";
		if (fl.bindOrientated)		text += "bindOriented ";
		if (fl.solidForTeam)		text += "solidForTeam ";
		if (fl.forcePhysicsUpdate)	text += "forcePhysicsUpdate ";
		if (fl.selected)			text += "selected ";
		if (fl.neverDormant)		text += "neverDormant ";
		if (fl.isDormant)			text += "isDormant ";
		if (fl.hasAwakened)			text += "hasAwakened ";
		args->Set("flags", text.c_str());
		break;
	case 2:		// Physics page
		if (GetPhysics()) {
			args->Set("physics", GetPhysics()->GetType()->classname);
			text = collisionModelManager->ContentsName(GetPhysics()->GetContents());
			args->Set("contents", text);

			text = collisionModelManager->ContentsName(GetPhysics()->GetClipMask());
			args->Set("clipmask", text);

			text = GetPhysics()->GetGravity().ToString(2);
			args->Set("gravity", text);

			text = GetPhysics()->GetOrigin().ToString(2);
			args->Set("origin", text);
			text = GetPhysics()->GetAxis().ToString(2);
			args->Set("axis", text);

			args->SetFloat("mass", GetPhysics()->GetMass());

			args->SetFloat("speed", GetPhysics()->GetLinearVelocity().Length() );
		}
		else {
			args->Set("physics", "none");
		}
		
	case 3:
		break;
	}
}

/*
=====================
idEntity::SetHealth

//HUMANHEAD: aob
=====================
*/
void idEntity::SetHealth( const int newHealth ) {

	// JRM
	if(newHealth > health)
		lastHealTime = gameLocal.GetTime();

	health = (newHealth < GetMaxHealth()) ? newHealth : GetMaxHealth();
}

/*
=====================
idEntity::GetHealthLevel

//HUMANHEAD: aob
=====================
*/
float idEntity::GetHealthLevel() {
	return ( (float)GetHealth() / (float)GetMaxHealth() );
}

/*
=====================
idEntity::GetWoundLevel

//HUMANHEAD: aob
=====================
*/
float idEntity::GetWoundLevel() {
	return 1.0f - GetHealthLevel();
}

/*
=====================
idEntity::SwapBindInfo

	HUMANHEAD pdm: Swaps the bindings of two entities, used for spiritwalking while bound
=====================
*/
bool idEntity::SwapBindInfo(idEntity *ent2) {
	idEntity *ent1 = this;
	if (!ent1->IsBound() && !ent2->IsBound()) {
		return false;
	}

	idEntity *master1 = ent1->bindMaster;
	idEntity *master2 = ent2->bindMaster;
	jointHandle_t joint1 = ent1->bindJoint;
	jointHandle_t joint2 = ent2->bindJoint;
	int body1 = ent1->bindBody;
	int body2 = ent2->bindBody;
	bool oriented1 = ent1->fl.bindOrientated;
	bool oriented2 = ent2->fl.bindOrientated;
	const char *bone1;
	const char *bone2;

	ent1->Unbind();
	ent2->Unbind();

	// Bind ent2 to what ent1 was bound to
	if (master1 != NULL) {
		if (joint1 != INVALID_JOINT) {
			bone1 = master1->GetAnimator()->GetJointName(joint1);
			ent2->BindToJoint(master1, bone1, oriented1);
		}
		else if (body1 >= 0) {
			ent2->BindToBody(master1, body1, oriented1);
		}
		else {
			ent2->Bind(master1, oriented1);
		}
	}

	// Bind ent1 to what ent2 was bound to
	if (master2 != NULL) {
		if (joint2 != INVALID_JOINT) {
			bone2 = master2->GetAnimator()->GetJointName(joint2);
			ent1->BindToJoint(master2, bone2, oriented2);
		}
		else if (body2 >= 0) {
			ent1->BindToBody(master2, body2, oriented2);
		}
		else {
			ent1->Bind(master2, oriented2);
		}
	}
	return true;
}

/*
=====================
idEntity::AllowCollision

	HUMANHEAD: pdm/aob
=====================
*/
bool idEntity::AllowCollision( const trace_t &collision ) {
	return true;
}

/*
=====================
idEntity::GetAimPosition

	HUMANHEAD: pdm
=====================
*/
idVec3 idEntity::GetAimPosition() const {
	idVec3 offset = spawnArgs.GetVector( "offset_aim" );
	return GetOrigin() + offset * GetAxis();
}

/*
=====================
idEntity::PlayImpactSound

Colliding objects should call their own versions of this( aka movables and projectiles )
=====================
*/
int idEntity::PlayImpactSound( const idDict* dict, const idVec3 &origin, surfTypes_t type ) {
	const char *snd = gameLocal.MatterTypeToMatterKey( "snd_impact", type );
	if( !snd || !snd[0] || !dict ) {
		return -1;
	}
		
	int length = 0;
	StartSoundShader( declManager->FindSound(dict->GetString(snd), false), SND_CHANNEL_BODY, 0, false, &length );
	return length;
}

/*
==============
idEntity::AddLocalMatterWound

HUMANHEAD: aob
==============
*/
void idEntity::AddLocalMatterWound( jointHandle_t jointNum, const idVec3 &localOrigin, const idVec3 &localNormal, const idVec3 &localDir, int damageDefIndex, const idMaterial *collisionMaterial ) {
	PROFILE_SCOPE("AddLocalMatterWound", PROFMASK_COMBAT);
	const idDeclEntityDef *def = static_cast<const idDeclEntityDef *>( declManager->DeclByIndex( DECL_ENTITYDEF, damageDefIndex ) );
	if ( def == NULL ) {
		return;
	}

	surfTypes_t matterType = gameLocal.GetMatterType( this, collisionMaterial, "idEntity::AddLocalMatterWound" );

	GetWoundManager()->AddWounds( def, matterType, jointNum, localOrigin, localNormal, localDir );

	//Not sure we need to do anything special with the head.  It should have its own collision
	//TODO: Grab head projection code from below and put into ApplyImpactMark()
}

/*
===============
idEntity::Event_DamageEntity

	HUMANHEAD: nla
===============
*/
void idEntity::Event_DamageEntity( idEntity *target, const char *damageDefName ) {
	assert( target );
	assert( damageDefName );

	target->Damage( this, this, idVec3( 0, 0, 0 ), damageDefName, 1.0f, INVALID_JOINT );
}

/*
===============
idEntity::Event_DelayDamageEntity

	HUMANHEAD: jrm: for scripts to call
===============
*/
void idEntity::Event_DelayDamageEntity( idEntity *target, const char *damageDefName, float secs ) {	
	const idDeclEntityDef *damageDecl = gameLocal.FindEntityDef(damageDefName, false);
	if (damageDecl == NULL) {
		gameLocal.Warning("delayDamageEntity: invalid damage type: %s\n", damageDefName);
		return;
	}

	PostEventSec(&EV_DamageEntity, secs, target, damageDefName);
}


//TODO: Move this dispose logic to idActor? (monsters and player)
/*
==============
idEntity::Event_Dispose
HUMANHEAD nla
==============
*/
void idEntity::Event_Dispose() {
	if( fl.disposed ) {
		return;
	}

	fl.disposed = true;
	fl.takedamage = false;
	GetPhysics()->SetContents( 0 );
	GetPhysics()->SetClipMask( 0 );
	GetPhysics()->UnlinkClip();
	//GetPhysics()->PutToRest();

	RemoveBinds();

	idVec3 origin = GetOrigin();	// This might be slightly up in the air because it's the origin of the ragdoll
	trace_t trace;
	memset(&trace, 0, sizeof(trace_t));
	if (gameLocal.clip.TracePoint( trace, origin, origin+gameLocal.GetGravityNormal()*50, CONTENTS_SOLID, NULL )) {
		origin = trace.endpos - gameLocal.GetGravityNormal() * 5;	// 5 units up from ground
	}

	hhFxInfo		fxInfo;
	fxInfo.RemoveWhenDone( true );
	BroadcastFxInfoPrefixed( "fx_deatheffect", origin, GetAxis(), &fxInfo );

	// Start the transition out , remove when transition is done.
	renderEntity.noShadow = true;
	if( modelDefHandle > 0 && !spawnArgs.GetBool("keepDecals") ) {
		gameRenderWorld->RemoveDecals( modelDefHandle );
	}
	SetShaderParm(SHADERPARM_TIME_OF_DEATH, MS2SEC(gameLocal.time));
//	SetDeformation(DEFORMTYPE_DEATHEFFECT, gameLocal.time, 5000);	// starttime, duration
	PostEventMS( &EV_Remove, 5000 );
}


/*
==============
idEntity::StartDisposeCountdown
HUMANHEAD nla
==============
*/
void idEntity::StartDisposeCountdown() {
	float delay = spawnArgs.GetFloat( "dispose_delay", "2" );
	CancelEvents( &EV_Dispose );
	PostEventSec( &EV_Dispose, delay );

}


/*
==============================
idEntity::GetChannelForAnim
  Given a group name, return the corresponding channel
  If no group matches, channel 0 is returned
	HUMANHEAD pdm: exposed this channel utility function for all entities
==============================
*/
int idEntity::GetChannelForAnim( const char *anim ) const {
	const char *channelName;
	int channel;

	channelName = GetChannelNameForAnim( anim );
	if ( !channelName || !channelName[0] ) {
		return( 0 );
	}

	channel = ChannelName2Num( channelName );
	if ( channel < 0 ) {
		return( 0 );
	}
	
	return( channel );
}


/*
===========================
idEntity::ChannelName2Num
  Given a channel name, return the channel to play on
  Returns -1 if none found, else the proper channel num
HUMANHEAD nla
===========================
*/
int idEntity::ChannelName2Num( const char *channelName ) const {

	if ( GetAnimator() && GetAnimator()->ModelDef() ) {
		return( GetAnimator()->ModelDef()->channelDict.GetInt( channelName, "-1" ) );
	}

	return( -1 );
}


/*
===========================
idEntity::GetChannelNameForAnim
  Given a channel name, return the channel to play on
  Returns NULL if none found, else the proper channel num
HUMANHEAD nla
===========================
*/
const char * idEntity::GetChannelNameForAnim( const char *animName ) const {

	if ( GetAnimator() && GetAnimator()->ModelDef() ) {
		return( GetAnimator()->ModelDef()->channelDict.GetString( va( "channel4anim_%s", animName ), NULL ) );
	}

	return( NULL );
}


/*
===============
idEntity::SetSoundMinDistance

HUMANHEAD: aob
===============
*/
void idEntity::SetSoundMinDistance( float minDistance, const s_channelType channel ) {
	if( !refSound.referenceSound ) {
		return;
	}

	hhSoundShaderParmsModifier modifierParms;
	
	modifierParms.SetMinDistance( minDistance );
	refSound.referenceSound->ModifySound( NULL, channel, modifierParms );
}

/*
===============
idEntity::SetSoundMaxDistance

HUMANHEAD: aob
===============
*/
void idEntity::SetSoundMaxDistance( float maxDistance, const s_channelType channel ) {
	if( !refSound.referenceSound ) {
		return;
	}

	hhSoundShaderParmsModifier modifierParms;
	
	modifierParms.SetMaxDistance( maxDistance );
	refSound.referenceSound->ModifySound( NULL, channel, modifierParms );
}

/*
===============
idEntity::HH_SetSoundVolume

HUMANHEAD: aob: takes a linear [0..1] volume
===============
*/
void idEntity::HH_SetSoundVolume( float volume, const s_channelType channel ) {
	if( !refSound.referenceSound ) {
		return;
	}
	// Convert from linear volume to dB
	float dB = hhMath::Scale2dB(volume);

	hhSoundShaderParmsModifier modifierParms;
	
	modifierParms.SetVolume( dB );
	refSound.referenceSound->ModifySound( NULL, channel, modifierParms );
}

float idEntity::GetVolume( const s_channelType channel ) {
	if( !refSound.referenceSound ) {
		return 0.0f;
	}

	soundShaderParms_t *ssp = refSound.referenceSound->GetSoundParms(NULL, channel);
	return hhMath::dB2Scale(ssp->volume);
}

/*
===============
idEntity::SetSoundShakes

HUMANHEAD: aob
===============
*/
void idEntity::SetSoundShakes( float shakes, const s_channelType channel ) {
	if( !refSound.referenceSound ) {
		return;
	}

	hhSoundShaderParmsModifier modifierParms;
	
	modifierParms.SetShakes( shakes );
	refSound.referenceSound->ModifySound( NULL, channel, modifierParms );
}

/*
===============
idEntity::SetSoundShaderFlags

HUMANHEAD: aob
===============
*/
void idEntity::SetSoundShaderFlags( int flags, const s_channelType channel ) {
	if( !refSound.referenceSound ) {
		return;
	}

	hhSoundShaderParmsModifier modifierParms;
	
	modifierParms.SetSoundShaderFlags( flags );
	refSound.referenceSound->ModifySound( NULL, channel, modifierParms );
}

/*
===============
idEntity::FadeSoundShader

HUMANHEAD: aob
===============
*/
void idEntity::FadeSoundShader( int to, int over, const s_channelType channel ) {
	if( !refSound.referenceSound ) {
		return;
	}

	refSound.referenceSound->FadeSound( channel, to, over );
}

/*
===============
idEntity::Event_ResetGravity

HUMANHEAD: pdm: Posted when entity is leaving a gravity zone
===============
*/
void idEntity::Event_ResetGravity() {
	idVec3 localGravityDir = hhUtils::GetLocalGravity( GetOrigin(), GetPhysics()->GetBounds(), gameLocal.GetGravity() );

	float gravityMagnitude = spawnArgs.GetFloat( "gravity", va("%.2f", localGravityDir.Normalize()) );
	GetPhysics()->SetGravity( gravityMagnitude * localGravityDir );
	GetPhysics()->Activate();
}

// 
// Event_LoadReactions()
// HUMANHEAD JRM
void idEntity::Event_LoadReactions() {
	LoadReactions();
}

//
// LoadReactions()
//
// HUMANHEAD JRM
//
#define MaxNumReactions 16
void idEntity::LoadReactions() {
	
	assert(reactions.Num() <= 0); // Can't have any loaded still!
	
	// Loop through each one in ORDER so the array list is ordered properly (so we can reference later by index if we want)
	idStr keyName;
	idStr reactPrefix;
	for(int i=0;i<MaxNumReactions;i++) {
		if(i > 0) {
			keyName = idStr("def_reaction") + idStr(i);
			reactPrefix = idStr("reaction") + idStr(i) + idStr("_");
		} else {
			keyName = idStr("def_reaction");
			reactPrefix = idStr("reaction_");
		}

		// Hit the end of our list
		const idKeyValue *kv = spawnArgs.FindKey((const char*)keyName);
		if(!kv)
			break;

		// Store unique keys if the user specified them specifically
		// ie. "reaction1_cause" to override from the default
		idDict uniqueKeys;
		uniqueKeys.Clear();
		const idKeyValue *reactKeyVal = spawnArgs.MatchPrefix(reactPrefix.c_str(), NULL);
		while(reactKeyVal) {
			uniqueKeys.Set(reactKeyVal->GetKey().c_str(), reactKeyVal->GetValue().c_str());
            reactKeyVal = spawnArgs.MatchPrefix(reactPrefix.c_str(), reactKeyVal);
		}

		const char *name = (const char*)kv->GetValue();
		if (name && name[0]) {
			const hhReactionDesc *rd = gameLocal.GetReactionHandler()->LoadReactionDesc(name, uniqueKeys);		
			assert(rd != NULL);
			hhReaction *newReaction		= new hhReaction();
			newReaction->desc			= rd;
			newReaction->causeEntity	= this;			// By default, all messages sent by us mean that the cause is US
			newReaction->effectEntity.Resize(8, 8);		// Try to avoid lots of mem allocations
			reactions.Append(newReaction);
		}

	}

	fl.refreshReactions = FALSE;
	if(reactions.Num() > 0) {
		fl.refreshReactions = spawnArgs.GetBool("reaction_enabled", "1");
	}
}

/*
==============================
idEntity::PickRandomTarget

HUMANHEAD: aob
==============================
*/
idEntity* idEntity::PickRandomTarget() {
	RemoveNullTargets();
	if( !targets.Num() ) {
		return NULL;
	}

	return targets[ gameLocal.random.RandomInt(targets.Num()) ].GetEntity();
}

//----------------------------------------------
//
// MSCallback
//
//----------------------------------------------
bool idEntity::MSCallback( renderEntity_s *renderEntity, const renderView_t *renderView, int ms) {
	const idEntity *ent = gameLocal.entities[ renderEntity->entityNum ];
	if ( !ent ) {
		gameLocal.Error( "idEntity::MSCallback: callback with NULL game entity" );
	}

	bool animChanged = false;
	bool deformChanged = false;

	// If this entity is animated, issue that callback
	if (ent->animCallback) {
		animChanged = ent->animCallback(renderEntity, renderView);
	}

	if ( renderView && renderView->time >= ent->nextCallbackTime ) {
		ent->nextCallbackTime = renderView->time + ms;
		deformChanged = true;
	}
	return (animChanged || deformChanged);
}

bool idEntity::TenHertzCallback( renderEntity_s *renderEntity, const renderView_t *renderView ) {
	return idEntity::MSCallback(renderEntity, renderView, 100);
}
bool idEntity::TwentyHertzCallback( renderEntity_s *renderEntity, const renderView_t *renderView ) {
	return idEntity::MSCallback(renderEntity, renderView, 50);
}
bool idEntity::ThirtyHertzCallback( renderEntity_s *renderEntity, const renderView_t *renderView ) {
	return idEntity::MSCallback(renderEntity, renderView, 33);
}
bool idEntity::SixtyHertzCallback( renderEntity_s *renderEntity, const renderView_t *renderView ) {
	return idEntity::MSCallback(renderEntity, renderView, 16);
}

/*
==============================
idEntity::SetDeformCallback

HUMANHEAD: pdm: set appropriate callback for the current deform.
==============================
*/
void idEntity::SetDeformCallback() {
	int deformType = renderEntity.shaderParms[SHADERPARM_ANY_DEFORM];

	if (deformType == DEFORMTYPE_NONE) {
		return;
	}

	// Store animation callback if it exists, so we can piggy back it onto ours
	if (renderEntity.callback == idEntity::ModelCallback) {
		animCallback = renderEntity.callback;
	}
	else {
		animCallback = NULL;
	}

	// All deform types that run on their own timer need a callback entry in here
	switch(deformType) {
		case DEFORMTYPE_PLANTSWAYX:
		case DEFORMTYPE_PLANTSWAYY:
		case DEFORMTYPE_RIPPLE:
		case DEFORMTYPE_RIPPLECENTER:
		case DEFORMTYPE_TURBULENT:
		case DEFORMTYPE_RAYS:
		case DEFORMTYPE_ALPHAGLOW:
			renderEntity.callback = idEntity::TwentyHertzCallback;
			nextCallbackTime = gameLocal.time;
			break;
		case DEFORMTYPE_VIBRATE:
			renderEntity.callback = idEntity::ThirtyHertzCallback;
			nextCallbackTime = gameLocal.time;
			break;
		case DEFORMTYPE_PORTAL:
		case DEFORMTYPE_DEATHEFFECT:	// Callback for these already used for animation
		case DEFORMTYPE_POD:
		case DEFORMTYPE_WINDBLAST:
			renderEntity.callback = idEntity::SixtyHertzCallback;
			nextCallbackTime = gameLocal.time;
			break;
		default:
			break;
	}
}

/*
==============================
idEntity::SetDeformationOnRenderEntity

HUMANHEAD pdm
Translates game deformations into the proper format for the renderer.  This is done because
the shaderparms all default to zero, and sometimes we want to be able to specify our parms
in the range of [0..1] for example, therefore we translate the parms into a different range.
This renderentity only version is so the spawnargs can be parsed and passed to it.
==============================
*/
void SetDeformationOnRenderEntity(renderEntity_t *renderEntity, int deformType, float parm1, float parm2) {
	float value1 = parm1;
	float value2 = parm2;

	// Do any shaderparm storage translation here.  We like to expose user friendly ranges to the users,
	// but internally we sometimes store the values differently.
	switch(deformType) {
		case DEFORMTYPE_SCALE:
			value1 = hhMath::hhMax<float>( 0.0f, parm1 );//aob - make sure we don't go below 0.0
			break;
		case DEFORMTYPE_POD:
			value2 = hhMath::hhMax<float>( 0.0f, parm2 );//aob - make sure we don't go below 0.0
			break;
		case DEFORMTYPE_VERTEXCOLOR:
			value1 = parm1 + 1.0f;	// Game code passes in [0..1], we store as [1..2], pass in -1 to stop using
			break;
		default:
			break;
	}

	renderEntity->shaderParms[ SHADERPARM_ANY_DEFORM ] = (float)deformType;
	renderEntity->shaderParms[ SHADERPARM_ANY_DEFORM_PARM1 ] = value1;
	renderEntity->shaderParms[ SHADERPARM_ANY_DEFORM_PARM2 ] = value2;
}

void idEntity::SetDeformation(int deformType, float parm1, float parm2) {
	SetDeformationOnRenderEntity(&renderEntity, deformType, parm1, parm2);
	UpdateVisuals();
}

void idEntity::Event_SetDeformation(int deformType, float parm1, float parm2) {
	SetDeformation(deformType, parm1, parm2);
}

void idEntity::Event_SetPortalCollision( int collide ) { // CJR
	this->fl.noPortal = collide ? true: false; // damn compiler warning
}

bool idEntity::IsScaled() {
	int deformType = (int)renderEntity.shaderParms[ SHADERPARM_ANY_DEFORM ];
	float scale;
	switch(deformType) {
		case DEFORMTYPE_SCALE:
			scale = renderEntity.shaderParms[ SHADERPARM_ANY_DEFORM_PARM1 ];
			return (scale != 0.0f && scale != 1.0f);
			break;
	}
	return false;
}

float idEntity::GetScale() {
	int deformType = (int)renderEntity.shaderParms[ SHADERPARM_ANY_DEFORM ];
	switch(deformType) {
		case DEFORMTYPE_SCALE:
			return renderEntity.shaderParms[ SHADERPARM_ANY_DEFORM_PARM1 ];
			break;
	}
	return 1.0f;
}

// Method to handle named event on all of an entity's guis, and handle any resulting commands
// If your cmds are getting ignored when set in a namedEvent, you aren't call this to issue it
void idEntity::CallNamedEvent(const char *eventName) {
	const char *command = NULL;
	sysEvent_t  ev;

	memset( &ev, 0, sizeof( ev ) );
	ev.evType = SE_NONE;

	for (int ix=0; ix<MAX_RENDERENTITY_GUI; ix++) {
		if (renderEntity.gui[ix]) {
			renderEntity.gui[ix]->HandleNamedEvent(eventName);

			// Now, make sure the pending command gets processed by forcing a HandleEvent call
			command = renderEntity.gui[ix]->HandleEvent( &ev, gameLocal.time );
			HandleGuiCommands( this, command );
		}
	}
}

/*
================
idEntity::BroadcastEventReroute

HUMANHEAD: aob
================
*/
void idEntity::BroadcastEventReroute( int eventId, const idBitMsg *msg, bool saveEvent, int excludeClient ) {
	//If in single player, send event back to entity
	if( !gameLocal.isMultiplayer && !gameLocal.isClient ) {
		ClientReceiveEvent( eventId, gameLocal.GetTime(), *msg );
	} else {
		assert(!gameLocal.isClient);
		ServerSendEvent(eventId, msg, saveEvent, excludeClient);

		//rww - if this is a listen server we want to both broadcast and receieve locally
		if (gameLocal.GetLocalPlayer()) {
			ClientReceiveEvent( eventId, gameLocal.GetTime(), *msg );
		}
	}
}

/*
================
idEntity::BroadcastSpawn

HUMANHEAD: aob
================
*/
void idEntity::BroadcastEventDef( const idEventDef& eventDef ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_EVENT_PARAM_SIZE];

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.BeginWriting();	
	msg.WriteShort( eventDef.GetEventNum() );

	BroadcastEventReroute( EVENT_EVENTDEF, &msg );
}

/*
================
idEntity::BroadcastSpawnDecl

HUMANHEAD: aob
================
*/
void idEntity::BroadcastDecl( declType_t type, const char* defName, const idEventDef& eventDef ) {
	idBitMsg		msg;
	byte			msgBuf[MAX_EVENT_PARAM_SIZE];
	const idDecl*	decl = NULL;

	if( !defName || !defName[0] ) {
		return;
	}

	decl = declManager->FindType( type, defName, false );
	if( !decl ) {
		return;
	}

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.BeginWriting();	
	msg.WriteBits( (int)type, DECL_MAX_TYPES_NUM_BITS );
	msg.WriteShort( decl->Index() );
	msg.WriteBits( eventDef.GetEventNum(), MAX_EVENTS_NUM_BITS );

	BroadcastEventReroute( EVENT_DECL, &msg );
}

/*
================
idEntity::BroadcastSpawnFx

HUMANHEAD: aob
================
*/
void idEntity::BroadcastFx( const char* defName, const idEventDef& eventDef ) {
	const idDecl* decl = declManager->FindType( DECL_FX, defName, false );
	if( decl ) {
		ProcessEvent( &eventDef, decl->GetName() );
	}
}

/*
================
idEntity::BroadcastSpawnEntityDef

HUMANHEAD: aob
================
*/
void idEntity::BroadcastEntityDef( const char* defName, const idEventDef& eventDef ) {
	BroadcastDecl( DECL_ENTITYDEF, defName, eventDef );
}

/*
================
idEntity::BroadcastBeam

HUMANHEAD: aob
================
*/
void idEntity::BroadcastBeam( const char* defName, const idEventDef& eventDef ) {
	BroadcastDecl( DECL_BEAM, defName, eventDef );
}

/*
==================
idEntity::BroadcastFxInfo

HUMANHEAD: aob
==================
*/
void idEntity::BroadcastFxInfo( const char* fx, const idVec3& origin, const idMat3& axis, const hhFxInfo* const fxInfo, const idEventDef* eventDef, bool broadcast ) {
	/*
	idBitMsg	msg;
	byte		msgBuf[MAX_EVENT_PARAM_SIZE];
	hhFxInfo	localDummyFxInfo;
	const hhFxInfo*	localFxInfo = (fxInfo) ? fxInfo : &localDummyFxInfo;

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.BeginWriting();

	//FIXME: try and find a way to pass the decl index instead of a string
	msg.WriteString( fx );
	msg.WriteVec3( origin );
	msg.WriteMat3( axis );
	localFxInfo->WriteToBitMsg( &msg );
	msg.WriteBits( (eventDef) ? eventDef->GetEventNum() : -1, MAX_EVENTS_NUM_BITS );

	BroadcastEventReroute( EVENT_FX_INFO, &msg );
	*/
	//rww - call to SpawnFxLocal and it will take care of client-server stuff based on where we are calling from.
	if (gameLocal.isClient && broadcast) { //don't spawn broadcast fx on the client.
		return;
	}
	hhEntityFx* efx = SpawnFxLocal( fx, origin, axis, fxInfo, gameLocal.isClient );
	if (efx) {
		efx->fl.networkSync = broadcast;
		if( eventDef ) {
			ProcessEvent( eventDef, efx );
		}
	}
}

/*
==================
idEntity::BroadcastFxInfoPrefixed

HUMANHEAD aob
==================
*/
//FIXME: this could be done in a local event and be much cheaper
void idEntity::BroadcastFxInfoPrefixed( const char *fxPrefix, const idVec3 &origin, const idMat3& axis, const hhFxInfo* const fxInfo, const idEventDef* eventDef, bool broadcast ) {
	if ( g_skipFX.GetBool() ) {
		return;
	}

	const idKeyValue* kv = spawnArgs.MatchPrefix( fxPrefix );
	while( kv ) {
		if( kv->GetValue().Length() ) { 
			BroadcastFxInfo( kv->GetValue().c_str(), origin, axis, fxInfo, eventDef, broadcast );
		}
		kv = spawnArgs.MatchPrefix( fxPrefix, kv );
	}
}

/*
==================
idEntity::BroadcastFxInfoPrefixedRandom

HUMANHEAD aob
==================
*/
//FIXME: this could be done in a local event and be much cheaper
void idEntity::BroadcastFxInfoPrefixedRandom( const char *fxPrefix, const idVec3 &origin, const idMat3& axis, const hhFxInfo* const fxInfo, const idEventDef* eventDef ) {
	if ( g_skipFX.GetBool() ) {
		return;
	}

	const char* fxName = spawnArgs.RandomPrefix( fxPrefix, gameLocal.random );
	if( !fxName || !fxName[0] ) {
		return;
	}

	BroadcastFxInfo( fxName, origin, axis, fxInfo, eventDef ); 
}

/*
==================
idEntity::BroadcastFxPrefixedRandom

HUMANHEAD ckr
==================
*/
void idEntity::BroadcastFxPrefixedRandom( const char *fxPrefix, const idEventDef &eventDef ) {
	if ( g_skipFX.GetBool() ) {
		return;
	}

	const char* fxName = spawnArgs.RandomPrefix( fxPrefix, gameLocal.random );
	if( !fxName || !fxName[0] ) {
		return;
	}

	BroadcastFx( fxName, eventDef ); 
}

/*
================
idEntity::Event_SetFloatKey
================
*/
void idEntity::Event_SetFloatKey( const char *key, float value ) {
	spawnArgs.SetFloat( key, value );
}

/*
================
idEntity::Event_SetVectorKey
================
*/
void idEntity::Event_SetVectorKey( const char *key, const idVec3& value ) {
	spawnArgs.SetVector( key, value );
}

/*
================
idEntity::Event_SetEntityKey
================
*/
void idEntity::Event_SetEntityKey( const char *key, const idEntity* value ) {
	HH_ASSERT( value );

	spawnArgs.Set( key, value->GetName() );
}

//=============================================================================
//
// idEntity::Event_PlayerCanSee
//
//
// HUMANHEAD CJR
//=============================================================================

void idEntity::Event_PlayerCanSee() {
	int			i;
	idEntity	*ent;
	hhPlayer	*player;
	trace_t		traceInfo;
	idEntity	*selfCheck; // Is either the entity or the master

	// Check if this entity is in the player's PVS
	if ( gameLocal.InPlayerPVS( this ) ) {
		for ( i = 0; i < gameLocal.numClients ; i++ ) {
			ent = gameLocal.entities[ i ];

			if ( !ent || !ent->IsType( hhPlayer::Type ) ) {
				continue;
			}

			// Get the player
			player = static_cast<hhPlayer *>( ent );

			// Check if the entity is in the player's FOV, based upon the "fov" key/value
			if ( !player->CheckFOV( this->GetOrigin() ) ) {
				continue;
			}

			if ( this->GetBindMaster() ) {
				selfCheck = this->GetBindMaster();
			} else {
				selfCheck = this;
			}

			// Check if a trace succeeds from the player to the entity
			gameLocal.clip.TracePoint( traceInfo, player->GetEyePosition(), this->GetOrigin(), MASK_SHOT_RENDERMODEL, player );
			if ( traceInfo.fraction >= 1.0f || ( gameLocal.GetTraceEntity( traceInfo ) == selfCheck ) ) {
				idThread::ReturnInt( true );
				return;
			}
		}
	}

	idThread::ReturnInt( false );
}

void idEntity::ActivatePrefixed(const char *prefix, idEntity *activator) {
	const idKeyValue *kv = spawnArgs.MatchPrefix( prefix );
	while( kv ) {
		idEntity *ent = gameLocal.FindEntity( kv->GetValue() );
		if ( ent ) {
			if ( ent && ent->RespondsTo( EV_Activate ) || ent->HasSignal( SIG_TRIGGER ) ) {
				ent->Signal( SIG_TRIGGER );
				ent->ProcessEvent( &EV_Activate, activator );
			} 		
			for ( int j = 0; j < MAX_RENDERENTITY_GUI; j++ ) {
				if ( ent->renderEntity.gui[ j ] ) {
					ent->renderEntity.gui[ j ]->Trigger( gameLocal.time );
				}
			}
		}

		kv = spawnArgs.MatchPrefix( prefix, kv );
	}
}

void idEntity::SquishedByDoor(idEntity *door) {
}

// All the different types of doors do this for theirselves, teammates, and buddies
float idEntity::GetDoorShaderParm(bool locked, bool startup) {
	float parmValue;

	// At startup:
	// shaderparm7: 3=alwayslocked, 2=locked, 1=unlocked, 0=never locked

	// When locking/unlocking:
	// shaderparm7: 3=alwayslocked, 2=locked, 1=unlocked, 0=never locked

	// alwaysLocked overrides all as they want to adjust colors manually
	bool alwaysLocked = spawnArgs.GetBool("alwaysLocked");
//	parmValue = alwaysLocked ? 0.0f : locked ? 2.0f : startup ? 0.0f : 1.0f;
	parmValue = alwaysLocked ? 3.0f : locked ? 2.0f : startup ? 0.0f : 1.0f;
	return parmValue;
}

void idEntity::Event_KillBox( void ) {
	gameLocal.KillBox(this, false);
}

//HUMANHEAD rww
void idEntity::Event_Remove( void ) {
	if (gameLocal.isClient && !fl.clientEntity) { //do not ever event-remove ents which could be in the snapshot on the client.
		Hide();
		return;
	}
	delete this;
}
//HUMANHEAD END
