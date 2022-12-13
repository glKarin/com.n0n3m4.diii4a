
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_SetGravityVector("setgravity", "v");
const idEventDef EV_SetGravityFactor("setgravityfactor", "f");
const idEventDef EV_DeactivateZone("<deactivatezone>", NULL);

//-----------------------------------------------------------------------
//
// hhZone
//
// NOTE: You do not get a EntityLeaving() callback for entities that are
// removed.  Could possibly use hhSafeEntitys to tell when they are removed
// but what's the point of calling EntityLeaving() with an invalid pointer.
//-----------------------------------------------------------------------

ABSTRACT_DECLARATION(hhTrigger, hhZone)
	EVENT( EV_DeactivateZone,	hhZone::Event_TurnOff )
	EVENT( EV_Enable,			hhZone::Event_Enable )
	EVENT( EV_Disable,			hhZone::Event_Disable )
	EVENT( EV_Touch,			hhZone::Event_Touch )
END_CLASS

//NOTE: If this works, this entity can cease inheriting from trigger, just need to take the
// tracemodel creation logic, isSimpleBox variable, make our own enable/disable functions
// touch goes away, triggeraction goes away, much simpler interface
#define	ZONES_ALWAYS_ACTIVE		1	// testing: want to be able to use dormancy to turn off --pdm

void hhZone::Spawn(void) {
	slop = 0.0f;				// Extra slop for bounds check
#if !ZONES_ALWAYS_ACTIVE
	fl.neverDormant = true;
#endif

#if ZONES_ALWAYS_ACTIVE
	fl.neverDormant = false;
	BecomeActive(TH_THINK);
	bActive = true;
	bEnabled = true;
#endif
}

void hhZone::Save(idSaveGame *savefile) const {
	savefile->WriteInt(zoneList.Num());				// idList<int>
	for (int i=0; i<zoneList.Num(); i++) {
		savefile->WriteInt(zoneList[i]);
	}

	savefile->WriteFloat(slop);
}

void hhZone::Restore( idRestoreGame *savefile ) {
	int num;

	zoneList.Clear();								// idList<int>
	savefile->ReadInt(num);
	zoneList.SetNum(num);
	for (int i=0; i<num; i++) {
		savefile->ReadInt(zoneList[i]);
	}

	savefile->ReadFloat(slop);
}

bool hhZone::ValidEntity(idEntity *ent) {
	return (ent && ent!=this &&
		ent->GetPhysics() &&
		!ent->GetPhysics()->IsType(idPhysics_Static::Type) &&
		ent->GetPhysics()->GetContents() != 0);
}

void hhZone::Empty() {
}

bool hhZone::ContainsEntityOfType(const idTypeInfo &t) {
	idEntity		*touch[ MAX_GENTITIES ];
	idBounds clipBounds;

	clipBounds.FromTransformedBounds( GetPhysics()->GetBounds(), GetOrigin(), GetAxis() );
	int num = gameLocal.clip.EntitiesTouchingBounds( clipBounds.Expand(slop), MASK_SHOT_BOUNDINGBOX, touch, MAX_GENTITIES );
	for (int i=0; i<num; i++) {
		if (touch[i] && touch[i]->IsType(t)) {
			gameLocal.Printf("Contains a %s\n", t.classname);
			return true;
		}
	}
	gameLocal.Printf("Doesn't contain a %s\n", t.classname);
	return false;
}

bool PointerInList(idEntity *target, idEntity **list, int num) {
	for (int j=0; j < num; j++ ) {
		if (list[j] == target) {
			return true;
		}
	}
	return false;
}

void hhZone::ResetZoneList() {
	// Call Leaving for anything previously entered
	idEntity		*previouslyInZone;
	for (int i=0; i < zoneList.Num(); i++ ) {
		previouslyInZone = gameLocal.entities[zoneList[i]];

		if (previouslyInZone) {
			EntityLeaving(previouslyInZone);
		}
	}
	zoneList.Clear();
}

void hhZone::TriggerAction(idEntity *activator) {
	CancelEvents(&EV_DeactivateZone);
	// Turn on until all encroachers are gone
	BecomeActive(TH_THINK);
}

void hhZone::ApplyToEncroachers() {
	idEntity		*touch[ MAX_GENTITIES ];
	idEntity		*previouslyInZone;
	idEntity		*encroacher;
	int				i, num;

	idBounds clipBounds;
	clipBounds.FromTransformedBounds( GetPhysics()->GetBounds(), GetOrigin(), GetAxis() );

	// Find all encroachers
	if (isSimpleBox) {
		num = gameLocal.clip.EntitiesTouchingBounds( clipBounds.Expand(slop), MASK_SHOT_BOUNDINGBOX | CONTENTS_PROJECTILE | CONTENTS_TRIGGER, touch, MAX_GENTITIES ); // CONTENTS_TRIGGER for walkthrough movables
	}
	else {
		num = hhUtils::EntitiesTouchingClipmodel( GetPhysics()->GetClipModel(), touch, MAX_GENTITIES, MASK_SHOT_BOUNDINGBOX | CONTENTS_TRIGGER );
	}

	// for anything previously applied, but no longer encroaching, call EntityLeaving()
	for (i=0; i < zoneList.Num(); i++ ) {
		previouslyInZone = gameLocal.entities[zoneList[i]];

		if (previouslyInZone) {
			if (!ValidEntity(previouslyInZone) || !PointerInList(previouslyInZone, touch, num)) {
				// We've applied before, but it's no longer encroaching
				EntityLeaving(previouslyInZone);

				// NOTE: Rather than removing and dealing with the list shifting, we reconstruct the list
				// from the touch list later
			}
		}
	}

	// Check touch list for any newly entered encroachers
	for (i = 0; i < num; i++ ) {
		encroacher = touch[i];
		if (ValidEntity(encroacher)) {
			if (zoneList.FindIndex(encroacher->entityNumber) == -1) {
				EntityEntered(encroacher);
			}
		}
	}

	// Call all encroachers and rebuild list
	zoneList.Clear();	//fixme: could make a version of clear() that doesn't deallocate the memory
	for (i = 0; i < num; i++ ) {
		encroacher = touch[i];
		if (ValidEntity(encroacher)) {
			zoneList.Append(encroacher->entityNumber);
			EntityEncroaching(encroacher);
		}
	}

	// Deactivate if no encroachers left
	if (!zoneList.Num()) {
		Empty();
#if !ZONES_ALWAYS_ACTIVE
		PostEventMS(&EV_DeactivateZone, 0);
#endif
	}
}

void hhZone::Think() {
	if (thinkFlags & TH_THINK) {
		ApplyToEncroachers();
	}
}

void hhZone::Event_TurnOff() {
	BecomeInactive(TH_THINK);
	bActive = false;
}

void hhZone::Event_Enable( void ) {
	hhTrigger::Event_Enable();
	TriggerAction(this);
}

void hhZone::Event_Disable( void ) {
	BecomeInactive(TH_THINK);
	ResetZoneList();
	hhTrigger::Event_Disable();
}

void hhZone::Event_Touch( idEntity *other, trace_t *trace ) {
	CancelEvents(&EV_DeactivateZone);
	// Turn on until all encroachers are gone
	BecomeActive(TH_THINK);

	bActive = true;
}


//-----------------------------------------------------------------------
//
// hhTriggerZone
//
// Zone used for precise trigger/untrigger mechanic.  Fires trigger once
// upon a valid entity entering, and again when a valid entity leaves. Also,
// optionally calls a function for each entity in the volume each tick.
//-----------------------------------------------------------------------

CLASS_DECLARATION(hhZone, hhTriggerZone)
END_CLASS

void hhTriggerZone::Spawn() {
	funcRefInfo.ParseFunctionKeyValue( spawnArgs.GetString("inCallRef") );
}

void hhTriggerZone::Save(idSaveGame *savefile) const {
	savefile->WriteStaticObject( funcRefInfo );
}

void hhTriggerZone::Restore( idRestoreGame *savefile ) {
	savefile->ReadStaticObject( funcRefInfo );
}

bool hhTriggerZone::ValidEntity(idEntity *ent) {
	return (hhZone::ValidEntity(ent) && !IsType(hhProjectile::Type));
}

void hhTriggerZone::EntityEntered(idEntity *ent) {
	ActivateTargets(ent);
}

void hhTriggerZone::EntityLeaving(idEntity *ent) {
	ActivateTargets(ent);
}

void hhTriggerZone::EntityEncroaching( idEntity *ent ) {
	if (funcRefInfo.GetFunction() != NULL) {
		funcRefInfo.SetParm_Entity( ent, 0 );
		funcRefInfo.Verify();
		funcRefInfo.CallFunction( spawnArgs );
	}
}

//-----------------------------------------------------------------------
//
// hhGravityZoneBase
//
//-----------------------------------------------------------------------

ABSTRACT_DECLARATION(hhZone, hhGravityZoneBase)
END_CLASS

void hhGravityZoneBase::Spawn(void) {
	bReorient = spawnArgs.GetBool("reorient");
	bShowVector = spawnArgs.GetBool("showVector");
	bKillsMonsters = spawnArgs.GetBool("killmonsters");

	//rww - avoid dictionary lookup post-spawn
	gravityOriginOffset = vec3_origin;
	if (spawnArgs.GetVector("override_origin", gravityOriginOffset.ToString(), gravityOriginOffset)) {
		gravityOriginOffset -= GetOrigin();
	}

	//rww - sync over network
	fl.networkSync = true;
}

void hhGravityZoneBase::Save(idSaveGame *savefile) const {
	savefile->WriteBool(bReorient);
	savefile->WriteBool(bKillsMonsters);
	savefile->WriteBool(bShowVector);
	savefile->WriteVec3(gravityOriginOffset);
}

void hhGravityZoneBase::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool(bReorient);
	savefile->ReadBool(bKillsMonsters);
	savefile->ReadBool(bShowVector);
	savefile->ReadVec3(gravityOriginOffset);
}

//rww - network code
void hhGravityZoneBase::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits(bReorient, 1);
	msg.WriteFloat(slop);
}

void hhGravityZoneBase::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	bReorient = !!msg.ReadBits(1);
	slop = msg.ReadFloat();
}

void hhGravityZoneBase::ClientPredictionThink( void ) {
	Think();
}
//rww - end network code

const idVec3 hhGravityZoneBase::GetGravityOrigin() const {
	return GetOrigin()+gravityOriginOffset;
}

bool hhGravityZoneBase::ValidEntity(idEntity *ent) {
	if (!ent) {
		return false;
	}
	if (ent->fl.ignoreGravityZones) {
		return false;
	}

	if (ent->IsType(hhProjectile::Type) && ent->GetPhysics()->GetGravity() == vec3_origin) {
		return false;	// Projectiles with zero gravity
	}
	if (ent->IsType(hhPlayer::Type)) {
		hhPlayer *pl = static_cast<hhPlayer*>(ent);
		if (pl->noclip) {
			return false; // Noclipping players
		}
		else if (pl->spectating) {
			return false; //spectating players
		}
		else if (gameLocal.isMultiplayer && ent->health <= 0) {
			return false; //dead mp players (only the prox ragdoll needs gravity)
		}
	}
	if (ent->IsType(hhVehicle::Type)) {
		if (static_cast<hhVehicle*>(ent)->IsNoClipping()) {
			return false;	// Noclipping vehicles
		}
		if (ent->IsType(hhShuttle::Type) && static_cast<hhShuttle*>(ent)->IsConsole()) {
			return false;	// unpiloted shuttles
		}
	}
	if (ent->IsType(hhPortal::Type) ) {
		return true;	// Portals are always valid entities in zones
	}

	if (!hhZone::ValidEntity( ent )) {
		return false;
	}

	return true;
}

bool hhGravityZoneBase::TouchingOtherZones(idEntity *ent, bool traceCheck, idVec3 &otherInfluence) { //rww
	if (!ent->GetPhysics()) {
		return false;
	}

	bool hitAny = false;

	otherInfluence.Zero();

	idBounds clipBounds;

	idEntity		*touch[ MAX_GENTITIES ];
	clipBounds.FromTransformedBounds( ent->GetPhysics()->GetBounds(), ent->GetOrigin(), ent->GetAxis() );
	int num = gameLocal.clip.EntitiesTouchingBounds( clipBounds, GetPhysics()->GetContents(), touch, MAX_GENTITIES );
	for (int i = 0; i < num; i++) {
		if (touch[i] && touch[i]->entityNumber != entityNumber && touch[i]->IsType(hhGravityZoneBase::Type)) {
			//touching the object, isn't me, and seems to be another gravity zone
			bool touchValid = false;

			if (traceCheck) { //let's perform a trace from the ent's origin to see which zone is hit first. (this is not an ideal solution, but it works)
				trace_t tr;
				const int checkContents = GetPhysics()->GetContents();
				const idVec3 &start = ent->GetOrigin();
				const float testLength = 512.0f;
				idVec3 end;

				//first trace against the other
				end = (touch[i]->GetPhysics()->GetBounds().GetCenter()-start).Normalize()*testLength;
				gameLocal.clip.TracePoint(tr, start, end, checkContents, ent);
				if (tr.c.entityNum == touch[i]->entityNumber) { //if the trace actually hit the other one
					float otherFrac = tr.fraction;

					//now trace against me
					end = (GetPhysics()->GetBounds().GetCenter()-start).Normalize()*testLength;
					gameLocal.clip.TracePoint(tr, start, GetPhysics()->GetBounds().GetCenter(), checkContents, ent);
					if (tr.c.entityNum != entityNumber || tr.fraction >= otherFrac) { //if the impact was further away (or same, don't want fighting), i lose.
						touchValid = true;
					}
				}
			}
			else {
				touchValid = true;
			}

			if (touchValid) {
				//accumulate force from other zones
				hhGravityZoneBase *zone = static_cast<hhGravityZoneBase *>(touch[i]);
				if (zone->isSimpleBox || ent->GetPhysics()->ClipContents(zone->GetPhysics()->GetClipModel())) { //if not simple box perform a clip check
					idVec3 grav = zone->GetCurrentGravity(ent->GetOrigin());
					hitAny = true;

					grav.Normalize();
					otherInfluence += grav;

					otherInfluence.Normalize();
				}
			}
		}
	}

	return hitAny;
}

void hhGravityZoneBase::EntityEntered( idEntity *ent ) {
	if( ent->RespondsTo(EV_ShouldRemainAlignedToAxial) ) {
		ent->ProcessEvent( &EV_ShouldRemainAlignedToAxial, (int)false );
	}
	if( ent->RespondsTo(EV_OrientToGravity) ) {
		ent->ProcessEvent( &EV_OrientToGravity, (int)bReorient );
	}
}

void hhGravityZoneBase::EntityLeaving( idEntity *ent ) {
	if( ent->RespondsTo(EV_ShouldRemainAlignedToAxial) ) {
		ent->ProcessEvent( &EV_ShouldRemainAlignedToAxial, (int)true );
	}

	// Instead of reseting gravity here, post a message to do it, so if we are transitioning
	// to another gravity zone or wallwalk, there won't be any discontinuities
	if (gameLocal.isClient && !ent->fl.clientEvents && !ent->fl.clientEntity && ent->IsType(hhProjectile::Type)) {
		ent->fl.clientEvents = true; //hackery to let normal projectiles reset their gravity for prediction
		ent->PostEventMS( &EV_ResetGravity, 200 );
		ent->fl.clientEvents = false;
	}
	else {
		ent->PostEventMS( &EV_ResetGravity, 200 );
	}
}

void hhGravityZoneBase::EntityEncroaching( idEntity *ent ) {
	// Cancel any pending gravity resets from other zones
	ent->CancelEvents( &EV_ResetGravity );

	idVec3 curGravity = GetCurrentGravity( ent->GetOrigin() );
	idVec3 otherGravity;
	if (TouchingOtherZones(ent, false, otherGravity)) { //factor in gravity for all other zones being touched to avoid back-and-forth behaviour
		float l = curGravity.Normalize();
		curGravity += otherGravity;
		curGravity *= l*0.5f;
	}
	if (ent->GetPhysics()->IsAtRest() && ent->GetGravity() != curGravity) {
		ent->SetGravity( curGravity );
		ent->GetPhysics()->Activate();
	}
	else {
		ent->SetGravity( curGravity );
	}
	
	if (ent->IsType( hhMonsterAI::Type )) {
		if (bKillsMonsters && ent->health > 0 &&
			!static_cast<hhMonsterAI*>(ent)->OverrideKilledByGravityZones() &&
			!ent->IsType(hhCrawler::Type) &&
			(idMath::Fabs(curGravity.x) > 0.01f || idMath::Fabs(curGravity.y) > 0.01f || curGravity.z >= 0.0f) &&
			static_cast<hhMonsterAI*>(ent)->IsActive() ) {

			const char *monsterDamageType = spawnArgs.GetString("def_monsterdamage");
			ent->Damage(this, NULL, vec3_origin, monsterDamageType, 1.0f, 0);
		}
	}
}


//-----------------------------------------------------------------------
//
// hhGravityZone
//
//-----------------------------------------------------------------------

CLASS_DECLARATION(hhGravityZoneBase, hhGravityZone)
	EVENT( EV_SetGravityVector,	hhGravityZone::Event_SetNewGravity )
END_CLASS

void hhGravityZone::Spawn(void) {
	zeroGravOnChange = spawnArgs.GetBool("zeroGravOnChange");
	idVec3 startGravity( spawnArgs.GetVector("gravity") );
	interpolationTime = SEC2MS(spawnArgs.GetFloat("interpTime"));
	gravityInterpolator.Init( gameLocal.time, 0, startGravity, startGravity );

	if (startGravity != gameLocal.GetGravity()) {
		if (!gameLocal.isMultiplayer) { //don't play sound in mp
			StartSound("snd_gravity_loop_on", SND_CHANNEL_MISC1, 0, true);
		}
	}
}

void hhGravityZone::Save(idSaveGame *savefile) const {
	savefile->WriteFloat( gravityInterpolator.GetStartTime() );	// idInterpolate<idVec3>
	savefile->WriteFloat( gravityInterpolator.GetDuration() );
	savefile->WriteVec3( gravityInterpolator.GetStartValue() );
	savefile->WriteVec3( gravityInterpolator.GetEndValue() );

	savefile->WriteInt(interpolationTime);
	savefile->WriteBool(zeroGravOnChange);
}

void hhGravityZone::Restore( idRestoreGame *savefile ) {
	float set;
	idVec3 vec;

	savefile->ReadFloat( set );			// idInterpolate<idVec3>
	gravityInterpolator.SetStartTime( set );
	savefile->ReadFloat( set );
	gravityInterpolator.SetDuration( set );
	savefile->ReadVec3( vec );
	gravityInterpolator.SetStartValue( vec );
	savefile->ReadVec3( vec );
	gravityInterpolator.SetEndValue( vec );

	savefile->ReadInt(interpolationTime);
	savefile->ReadBool(zeroGravOnChange);
}

void hhGravityZone::Think() {
	hhGravityZoneBase::Think();
	if (thinkFlags & TH_THINK) {
		if (bShowVector) {
			gameRenderWorld->DebugArrow(colorGreen, renderEntity.origin, renderEntity.origin + GetCurrentGravity(vec3_origin), 10);
		}
	}
}

const idVec3 hhGravityZone::GetDestinationGravity() const {
	return gravityInterpolator.GetEndValue();
}

const idVec3 hhGravityZone::GetCurrentGravity(const idVec3 &location) const {
	return gravityInterpolator.GetCurrentValue( gameLocal.time );
}

void hhGravityZone::SetGravityOnZone( idVec3 &newGravity ) {
	idVec3	startGrav;

	if (!gameLocal.isMultiplayer) { //don't play sound in mp
		if ( newGravity.Compare(gameLocal.GetGravity(), VECTOR_EPSILON) ) {
			StartSound("snd_gravity_off", SND_CHANNEL_ANY);
			StopSound(SND_CHANNEL_MISC1, true);
			StartSound("snd_gravity_loop_off", SND_CHANNEL_MISC1, 0, true);
		}
		else {
			StartSound("snd_gravity_on", SND_CHANNEL_ANY);
			StopSound(SND_CHANNEL_MISC1, true);
			StartSound("snd_gravity_loop_on", SND_CHANNEL_MISC1, 0, true);
		}
	}

	if ( zeroGravOnChange ) {	// nla
		startGrav = vec3_origin;
	}
	else {
		startGrav = GetCurrentGravity(vec3_origin);
	}
	// Interpolate to new gravity
	gravityInterpolator.Init( gameLocal.time, interpolationTime, startGrav, newGravity );
}

//rww - network code
void hhGravityZone::WriteToSnapshot( idBitMsgDelta &msg ) const {
	hhGravityZoneBase::WriteToSnapshot(msg);

	msg.WriteFloat(gravityInterpolator.GetStartTime());
	msg.WriteFloat(gravityInterpolator.GetDuration());
	idVec3 vecStart = gravityInterpolator.GetStartValue();
	msg.WriteFloat(vecStart.x);
	msg.WriteFloat(vecStart.y);
	msg.WriteFloat(vecStart.z);
	idVec3 vecEnd = gravityInterpolator.GetEndValue();
	msg.WriteDeltaFloat(vecStart.x, vecEnd.x);
	msg.WriteDeltaFloat(vecStart.y, vecEnd.y);
	msg.WriteDeltaFloat(vecStart.z, vecEnd.z);
}

void hhGravityZone::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	hhGravityZoneBase::ReadFromSnapshot(msg);
	gravityInterpolator.SetStartTime(msg.ReadFloat());
	gravityInterpolator.SetDuration(msg.ReadFloat());
	idVec3 vecStart;
	vecStart.x = msg.ReadFloat();
	vecStart.y = msg.ReadFloat();
	vecStart.z = msg.ReadFloat();
    gravityInterpolator.SetStartValue(vecStart);
	idVec3 vecEnd;
	vecEnd.x = msg.ReadDeltaFloat(vecStart.x);
	vecEnd.y = msg.ReadDeltaFloat(vecStart.y);
	vecEnd.z = msg.ReadDeltaFloat(vecStart.z);
	gravityInterpolator.SetEndValue(vecEnd);
}

void hhGravityZone::ClientPredictionThink( void ) {
	hhGravityZoneBase::ClientPredictionThink();
}
//rww - end network code

void hhGravityZone::Event_SetNewGravity( idVec3 &newGravity ) {
	SetGravityOnZone( newGravity );
}


//-----------------------------------------------------------------------
//
// hhGravityZoneInward
//
//-----------------------------------------------------------------------

CLASS_DECLARATION(hhGravityZoneBase, hhGravityZoneInward)
	EVENT( EV_SetGravityFactor,	hhGravityZoneInward::Event_SetNewGravityFactor )
END_CLASS

void hhGravityZoneInward::Spawn(void) {
	float startFactor = spawnArgs.GetFloat("factor", "50000");
	monsterGravityFactor = spawnArgs.GetFloat("monsterGravFactor", "1");
	interpolationTime = SEC2MS(spawnArgs.GetFloat("interpTime"));
	factorInterpolator.Init( gameLocal.time, 0, startFactor, startFactor );
}

void hhGravityZoneInward::Save(idSaveGame *savefile) const {
	savefile->WriteFloat( factorInterpolator.GetStartTime() );	// idInterpolate<float>
	savefile->WriteFloat( factorInterpolator.GetDuration() );
	savefile->WriteFloat( factorInterpolator.GetStartValue() );
	savefile->WriteFloat( factorInterpolator.GetEndValue() );

	savefile->WriteInt(interpolationTime);
	savefile->WriteFloat(monsterGravityFactor);
}

void hhGravityZoneInward::Restore( idRestoreGame *savefile ) {
	float set;

	savefile->ReadFloat( set );			// idInterpolate<float>
	factorInterpolator.SetStartTime( set );
	savefile->ReadFloat( set );
	factorInterpolator.SetDuration( set );
	savefile->ReadFloat( set );
	factorInterpolator.SetStartValue(set);
	savefile->ReadFloat( set );
	factorInterpolator.SetEndValue( set );

	savefile->ReadInt(interpolationTime);
	savefile->ReadFloat(monsterGravityFactor);
}

void hhGravityZoneInward::EntityEntered(idEntity *ent) {
	hhGravityZoneBase::EntityEntered(ent);
	if ( ent && ent->IsType( hhMonsterAI::Type ) ) {
		static_cast<hhMonsterAI*>(ent)->GravClipModelAxis( true );
	}
	// Disallow slope checking, it makes us stutter when walking on convex surfaces

	// aob - commented this because it allows the player to walk up vertical walls while in inward gravity zone
	//Didn't see any studdering when thia was commented out.  Do we still need it?
	//if (ent->IsType( hhPlayer::Type ) && ent->GetPhysics()->IsType(hhPhysics_Player::Type) ) {
	//	static_cast<hhPhysics_Player*>(ent->GetPhysics())->SetSlopeCheck(false);
	//}
	//now done constantly while in a gravity zone
	/*
	if (ent->IsType( hhPlayer::Type ) && ent->GetPhysics()->IsType(hhPhysics_Player::Type) ) {
		static_cast<hhPhysics_Player*>(ent->GetPhysics())->SetInwardGravity(1);
	}
	*/
}

void hhGravityZoneInward::EntityLeaving(idEntity *ent) {
	hhGravityZoneBase::EntityLeaving(ent);
	if ( ent && ent->IsType( hhMonsterAI::Type ) ) {
		static_cast<hhMonsterAI*>(ent)->GravClipModelAxis( false );
	}
	// Re-enable slope checking

	// aob - commented this because it allows the player to walk up vertical walls while in inward gravity zone
	//Didn't see any studdering when thia was commented out.  Do we still need it?
	//if (ent->IsType( hhPlayer::Type ) && ent->GetPhysics()->IsType(hhPhysics_Player::Type) ) {
	//	static_cast<hhPhysics_Player*>(ent->GetPhysics())->SetSlopeCheck(true);
	//}
	if (ent->IsType( hhPlayer::Type ) && ent->GetPhysics()->IsType(hhPhysics_Player::Type) ) {
		static_cast<hhPhysics_Player*>(ent->GetPhysics())->SetInwardGravity(0);
	}
}

// This is actually called each tick if for entities inside
void hhGravityZoneInward::EntityEncroaching( idEntity *ent ) {

	// Cancel any pending gravity resets from other zones
	ent->CancelEvents( &EV_ResetGravity );

	idVec3 curGravity = GetCurrentGravity( ent->GetOrigin() );
	idVec3 otherGravity;
	if (TouchingOtherZones(ent, false, otherGravity)) { //factor in gravity for all other zones being touched to avoid back-and-forth behaviour
		float l = curGravity.Normalize();
		curGravity += otherGravity;
		curGravity *= l*0.5f;
	}
	if (ent->GetPhysics()->IsAtRest() && ent->GetGravity() != curGravity) {
		ent->SetGravity( curGravity );
		ent->GetPhysics()->Activate();
	}
	else {
		ent->SetGravity( curGravity );
	}
	if (ent->IsType( idAI::Type )) {
		if (bKillsMonsters && ent->health > 0 &&
			!ent->IsType(hhCrawler::Type) &&
			(curGravity.x != 0.0f || curGravity.y != 0.0f || curGravity.z >= 0.0f) &&
			!static_cast<hhMonsterAI*>(ent)->OverrideKilledByGravityZones() &&
			static_cast<idAI*>(ent)->IsActive() ) {
			const char *monsterDamageType = spawnArgs.GetString("def_monsterdamage");
			ent->Damage(this, NULL, vec3_origin, monsterDamageType, 1.0f, 0);
		}
	}
	//rww
	else if (ent->IsType( hhPlayer::Type ) && ent->GetPhysics()->IsType(hhPhysics_Player::Type) ) {
		static_cast<hhPhysics_Player*>(ent->GetPhysics())->SetInwardGravity(1);
	}


	if( ent->IsType(idAI::Type) && ent->health > 0 ) {
		ent->GetPhysics()->SetGravity( ent->GetPhysics()->GetGravity() * monsterGravityFactor );
		ent->GetPhysics()->Activate();
	}

	if( bShowVector ) {
		hhUtils::DebugCross( colorBlue, GetOrigin(), 100, 10 );
		idVec3 newGrav = GetCurrentGravity( ent->GetOrigin() );
		gameRenderWorld->DebugArrow( colorGreen, ent->GetRenderEntity()->origin, ent->GetRenderEntity()->origin + newGrav, 10 );
	}
}

const idVec3 hhGravityZoneInward::GetCurrentGravity( const idVec3 &location ) const {
	idVec3 grav;
	idVec3 origin = GetGravityOrigin();
	float factor = factorInterpolator.GetCurrentValue( gameLocal.GetTime() );
	idVec3 inward = origin - location;
	inward.Normalize();
	grav = inward * DEFAULT_GRAVITY * factor;
	return grav;
}

void hhGravityZoneInward::Event_SetNewGravityFactor( float newFactor ) {
	// Interpolate to new gravity factor
	float curFactor = factorInterpolator.GetCurrentValue( gameLocal.GetTime() );
	factorInterpolator.Init( gameLocal.GetTime(), interpolationTime, curFactor, newFactor );
}

//-----------------------------------------------------------------------
//
// hhAIWallwalkZone
//
//-----------------------------------------------------------------------

CLASS_DECLARATION(hhGravityZone, hhAIWallwalkZone)
END_CLASS

bool hhAIWallwalkZone::ValidEntity(idEntity *ent) {
	// allow AI that isnt dead
	return ent->IsType(idAI::Type) && ent->health > 0;
}

void hhAIWallwalkZone::EntityEncroaching( idEntity *ent ) {
	// Cancel any pending gravity resets from other zones
	ent->CancelEvents( &EV_ResetGravity );

	trace_t TraceInfo;
	gameLocal.clip.TracePoint(TraceInfo, ent->GetOrigin(), ent->GetOrigin() + (idVec3(0,0,-300)*ent->GetRenderEntity()->axis), ent->GetPhysics()->GetClipMask(), ent);
	if( TraceInfo.fraction < 1.0f ) { // && ent->health > 0 ) {
		ent->SetGravity( -TraceInfo.c.normal );
		ent->GetPhysics()->Activate();
	}
}

//-----------------------------------------------------------------------
//
// hhGravityZoneSinkhole
//
//-----------------------------------------------------------------------

CLASS_DECLARATION(hhGravityZoneInward, hhGravityZoneSinkhole)
	EVENT( EV_SetGravityFactor,	hhGravityZoneSinkhole::Event_SetNewGravityFactor )
END_CLASS

void hhGravityZoneSinkhole::Spawn(void) {
	bReorient = false;
	maxMagnitude = spawnArgs.GetFloat("maxMagnitude", "10000");
	minMagnitude = spawnArgs.GetFloat("minMagnitude", "0");
}

void hhGravityZoneSinkhole::Save(idSaveGame *savefile) const {
	savefile->WriteFloat(maxMagnitude);
	savefile->WriteFloat(minMagnitude);
}

void hhGravityZoneSinkhole::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat(maxMagnitude);
	savefile->ReadFloat(minMagnitude);
}

// Still have this in case we want to do something mass based
const idVec3 hhGravityZoneSinkhole::GetCurrentGravityEntity(const idEntity *ent) const {
	idVec3 grav = vec3_origin;
	if (ent) {
		// precalc mass product / G as a constant and expose that as the fudge factor
		idVec3 origin = GetGravityOrigin();
		float factor = factorInterpolator.GetCurrentValue( gameLocal.time );
		idVec3 inward = origin - ent->GetOrigin();
		float distance = inward.Normalize();
		float distanceSquared = distance*distance;

		// Some different gravitational fields
	//	float gravMag = (mass * ent->GetPhysics()->GetMass() * GRAVITATIONAL_CONSTANT) / distanceSquared;
	//	float gravMag = factor*factor / distanceSquared;		// Inverse squared distance
	//	float gravMag = factor / sqrt(distance);				// Inverse sqrt distance
	//	float gravMag = factor * sqrt(distance);				// sqrt distance
		float gravMag = factor*factor / 2 + 0.2f * distanceSquared;		// Inverse squared distance
		//gameLocal.Printf("factor=%.0f  gravity magnitude=%.2f\n", factor, gravMag);

		gravMag = hhMath::ClampFloat(minMagnitude, maxMagnitude, gravMag);	// This will cut off extremely large forces
		grav = inward * gravMag;
	}
	return grav;
}

const idVec3 hhGravityZoneSinkhole::GetCurrentGravity(const idVec3 &location) const {
	idVec3 grav;

	// precalc mass product / G as a constant and expose that as the fudge factor
	idVec3 origin = GetGravityOrigin();
	float factor = factorInterpolator.GetCurrentValue( gameLocal.time );
	idVec3 inward = origin - location;
	float distance = inward.Normalize();
	float distanceSquared = distance*distance;

	// Some different gravitational fields
	float gravMag = factor*factor / 2 + 0.2f * distanceSquared;			// Inverse squared distance
	gravMag = hhMath::ClampFloat(minMagnitude, maxMagnitude, gravMag);	// This will cut off extremely large forces
	grav = inward * gravMag;
	return grav;
}

void hhGravityZoneSinkhole::Event_SetNewGravityFactor( float newFactor ) {
	// Interpolate to new gravity factor
	float curFactor = factorInterpolator.GetCurrentValue(gameLocal.time);
	factorInterpolator.Init( gameLocal.time, interpolationTime, curFactor, newFactor );
}


//-----------------------------------------------------------------------
//
// hhVelocityZone
//
//-----------------------------------------------------------------------

const idEventDef EV_SetVelocityVector("setvelocity", "v");

CLASS_DECLARATION(hhZone, hhVelocityZone)
	EVENT( EV_SetVelocityVector,	hhVelocityZone::Event_SetNewVelocity )
END_CLASS

void hhVelocityZone::Spawn(void) {
	bReorient = spawnArgs.GetBool("reorient");
	interpolationTime = SEC2MS(spawnArgs.GetFloat("interpTime"));
	idVec3 startVelocity = spawnArgs.GetVector("velocity");
	//slop = 25.0f;	// we use a slightly larger bounds to catch things that are rotated by bReorient
	bShowVector = spawnArgs.GetBool("showVector");
	bKillsMonsters = spawnArgs.GetBool("killmonsters");

	velocityInterpolator.Init(gameLocal.time, 0, startVelocity, startVelocity);
}

void hhVelocityZone::Save(idSaveGame *savefile) const {
	savefile->WriteFloat( velocityInterpolator.GetStartTime() );	// idInterpolate<idVec3>
	savefile->WriteFloat( velocityInterpolator.GetDuration() );
	savefile->WriteVec3( velocityInterpolator.GetStartValue() );
	savefile->WriteVec3( velocityInterpolator.GetEndValue() );

	savefile->WriteBool(bKillsMonsters);
	savefile->WriteBool(bReorient);
	savefile->WriteBool(bShowVector);
	savefile->WriteInt(interpolationTime);
}

void hhVelocityZone::Restore( idRestoreGame *savefile ) {
	float set;
	idVec3 vec;

	savefile->ReadFloat( set );					// idInterpolate<idVec3>
	velocityInterpolator.SetStartTime( set );
	savefile->ReadFloat( set );
	velocityInterpolator.SetDuration( set );
	savefile->ReadVec3( vec );
	velocityInterpolator.SetStartValue( vec );
	savefile->ReadVec3( vec );
	velocityInterpolator.SetEndValue( vec );

	savefile->ReadBool(bKillsMonsters);
	savefile->ReadBool(bReorient);
	savefile->ReadBool(bShowVector);
	savefile->ReadInt(interpolationTime);
}

void hhVelocityZone::EntityLeaving(idEntity *ent) {
	ent->GetPhysics()->SetLinearVelocity(idVec3(0, 0, 0));
	if( ent->RespondsTo(EV_OrientToGravity) ) {
		ent->ProcessEvent( &EV_OrientToGravity, (int)bReorient );
	}
}

void hhVelocityZone::EntityEncroaching(idEntity *ent) {
	idVec3 baseVelocity = velocityInterpolator.GetCurrentValue(gameLocal.time);
	idVec3 baseVelocityDirection = baseVelocity;
	baseVelocityDirection.Normalize();

	idVec3 curVelocity;
	curVelocity = ent->GetPhysics()->GetLinearVelocity();
	curVelocity.ProjectOntoPlane(baseVelocityDirection);

	ent->GetPhysics()->SetLinearVelocity( curVelocity + baseVelocity );
	if( ent->RespondsTo(EV_OrientToGravity) ) {
		ent->ProcessEvent( &EV_OrientToGravity, (int)bReorient );
	}
	else if (ent->IsType( idAI::Type )) {
		if (bKillsMonsters && ent->health > 0 &&
			!static_cast<idAI*>(ent)->IsFlying()) {
			const char *monsterDamageType = spawnArgs.GetString("def_monsterdamage");
			ent->Damage(this, NULL, vec3_origin, monsterDamageType, 1.0f, 0);
		}
	}
}

void hhVelocityZone::Think() {
	hhZone::Think();
	if (thinkFlags & TH_THINK) {
		if (bShowVector) {
			idVec3 baseVelocity = velocityInterpolator.GetCurrentValue(gameLocal.time);
			gameRenderWorld->DebugArrow(colorGreen, renderEntity.origin, renderEntity.origin+baseVelocity, 10);
		}
	}
}

void hhVelocityZone::Event_SetNewVelocity( idVec3 &newVelocity ) {
	// Interpolate to new velocity
	idVec3 currentVelocity = velocityInterpolator.GetCurrentValue(gameLocal.time);
	velocityInterpolator.Init(gameLocal.time, interpolationTime, currentVelocity, newVelocity);
}


//-----------------------------------------------------------------------
//
// hhShuttleRecharge
//
//-----------------------------------------------------------------------

CLASS_DECLARATION(hhZone, hhShuttleRecharge)
END_CLASS

void hhShuttleRecharge::Spawn(void) {
	amountHealth = spawnArgs.GetInt("amounthealth");
	amountPower = spawnArgs.GetInt("amountpower");
}

void hhShuttleRecharge::Save(idSaveGame *savefile) const {
	savefile->WriteInt(amountHealth);
	savefile->WriteInt(amountPower);
}

void hhShuttleRecharge::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt(amountHealth);
	savefile->ReadInt(amountPower);
}

bool hhShuttleRecharge::ValidEntity(idEntity *ent) {
	return ent && ent->IsType(hhShuttle::Type);
}

void hhShuttleRecharge::EntityEntered(idEntity *ent) {
	//static_cast<hhShuttle *>(ent)->SetRecharging(true);
}

void hhShuttleRecharge::EntityLeaving(idEntity *ent) {
	//static_cast<hhShuttle *>(ent)->SetRecharging(false);
}

void hhShuttleRecharge::EntityEncroaching(idEntity *ent) {
	if (ent->IsType(hhVehicle::Type)) {
		hhVehicle *vehicle = static_cast<hhVehicle *>(ent);

		//HUMANHEAD bjk PCF (4-27-06) - shuttle recharge was slow
		if(USERCMD_HZ == 30) {
			vehicle->GiveHealth(2*amountHealth);
			vehicle->GivePower(2*amountPower);
		} else {
			vehicle->GiveHealth(amountHealth);
			vehicle->GivePower(amountPower);
		}
	}
}


//-----------------------------------------------------------------------
//
// hhDockingZone
//
//-----------------------------------------------------------------------

CLASS_DECLARATION(hhZone, hhDockingZone)
END_CLASS

void hhDockingZone::Spawn(void) {
	dock = NULL;
	triggerBehavior = TB_PLAYER_MONSTERS_FRIENDLIES;	// Allow all actors to trigger it

	fl.networkSync = true;
}

void hhDockingZone::Save(idSaveGame *savefile) const {
	dock.Save(savefile);
}

void hhDockingZone::Restore( idRestoreGame *savefile ) {
	dock.Restore(savefile);
}

void hhDockingZone::RegisterDock(hhDock *d) {
	dock = d;
}

bool hhDockingZone::ValidEntity(idEntity *ent) {
	if (ent) {
		if (dock.IsValid() && dock->ValidEntity(ent)) {
			return true;
		}
		if (ent->IsType(idActor::Type)) {	//FIXME: Is this causing the shuttleCount to go wrong
			return hhShuttle::ValidPilot(static_cast<idActor*>(ent));
		}
	}
	return false;
}

void hhDockingZone::EntityEncroaching(idEntity *ent) {
	if (dock.IsValid()) {
		dock->EntityEncroaching(ent);
	}
}

void hhDockingZone::EntityEntered(idEntity *ent) {
	if (dock.IsValid()) {
		dock->EntityEntered(ent);
	}
}

void hhDockingZone::EntityLeaving(idEntity *ent) {
	if (dock.IsValid()) {
		dock->EntityLeaving(ent);
	}
}

void hhDockingZone::WriteToSnapshot( idBitMsgDelta &msg ) const {
	GetPhysics()->WriteToSnapshot(msg);
	msg.WriteBits(dock.GetSpawnId(), 32);
}

void hhDockingZone::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	GetPhysics()->ReadFromSnapshot(msg);
	dock.SetSpawnId(msg.ReadBits(32));
}

void hhDockingZone::ClientPredictionThink( void ) {
	if (!gameLocal.isNewFrame) {
		return;
	}
	Think();
}

//-----------------------------------------------------------------------
//
// hhShuttleDisconnect
//
//-----------------------------------------------------------------------

CLASS_DECLARATION(hhZone, hhShuttleDisconnect)
END_CLASS

void hhShuttleDisconnect::Spawn(void) {
}

bool hhShuttleDisconnect::ValidEntity(idEntity *ent) {
	return ent && ent->IsType(hhShuttle::Type);
}

void hhShuttleDisconnect::EntityEntered(idEntity *ent) {
	static_cast<hhShuttle*>(ent)->AllowTractor(false);
}

void hhShuttleDisconnect::EntityEncroaching(idEntity *ent) {
}

void hhShuttleDisconnect::EntityLeaving(idEntity *ent) {
	static_cast<hhShuttle*>(ent)->AllowTractor(true);
}


//-----------------------------------------------------------------------
//
// hhShuttleSlingshot
//
//-----------------------------------------------------------------------

CLASS_DECLARATION(hhZone, hhShuttleSlingshot)
END_CLASS

void hhShuttleSlingshot::Spawn(void) {
}

bool hhShuttleSlingshot::ValidEntity(idEntity *ent) {
	return ent && ent->IsType(hhShuttle::Type);
}

void hhShuttleSlingshot::EntityEntered(idEntity *ent) {
}

void hhShuttleSlingshot::EntityEncroaching(idEntity *ent) {
	float factor = spawnArgs.GetFloat("BoostFactor");

	hhShuttle *shuttle = static_cast<hhShuttle*>(ent);
	shuttle->ApplyBoost( 255.0f * factor);

	// CJR: Alter the player's view when zooming through a slingshot zone
	shuttle->GetPilot()->PostEventMS( &EV_SetOverlayMaterial, 0, spawnArgs.GetString( "mtr_speedView" ), -1, false );

}

void hhShuttleSlingshot::EntityLeaving(idEntity *ent) {
	// CJR: Reset the player's view after zooming through a slingshot zone
	hhShuttle *shuttle = static_cast<hhShuttle*>(ent);
	shuttle->GetPilot()->PostEventMS( &EV_SetOverlayMaterial, 0, "", -1, false );
}



//-----------------------------------------------------------------------
//
// hhRemovalVolume
//
//-----------------------------------------------------------------------

CLASS_DECLARATION(hhZone, hhRemovalVolume)
END_CLASS

void hhRemovalVolume::Spawn(void) {
}

bool hhRemovalVolume::ValidEntity(idEntity *ent) {
	return ent && (
		ent->IsType(idMoveable::Type) ||
		ent->IsType(idItem::Type) ||
		ent->IsType(hhAFEntity::Type) ||
		ent->IsType(hhAFEntity_WithAttachedHead::Type) ||
		(ent->IsType(hhMonsterAI::Type) && ent->health<=0 && !ent->fl.isTractored) );
}

void hhRemovalVolume::EntityEntered(idEntity *ent) {
	ent->PostEventMS(&EV_Remove, 0);
}

void hhRemovalVolume::EntityEncroaching(idEntity *ent) {
}

void hhRemovalVolume::EntityLeaving(idEntity *ent) {
}
