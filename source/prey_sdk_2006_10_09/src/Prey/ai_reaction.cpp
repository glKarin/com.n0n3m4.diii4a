#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


///////////////////////////////////////////
//
// hhReactionDesc
//
///////////////////////////////////////////


//
// CausedToStr()
//
idStr hhReactionDesc::CauseToStr(Cause c) {	
	switch(c) {
	case Cause_Damage:
		return idStr("Damage");
	case Cause_Touch:
		return idStr("Touch");
	case Cause_Use:
		return idStr("Use");
	case Cause_None:
		return idStr("None");
	case Cause_PlayAnim:
		return idStr("PlayAnim");
	case Cause_Telepathic_Trigger:
		return idStr("TelepathicTrigger");
	case Cause_Telepathic_Throw:
		return idStr("TelepathicThrow");
	}

	return idStr("!!UNDEFINED!!");
}

//
// EffectToStr()
//
idStr hhReactionDesc::EffectToStr(Effect e) {
	switch(e) {
	case Effect_Damage:
		return idStr("Damage");
	case Effect_DamageEnemy:
		return idStr("DamageEnemy");
	case Effect_VehicleDock:
		return idStr("VehicleDock");
	case Effect_Vehicle:
		return idStr("Vehicle");
	case Effect_Heal:
		return idStr("Heal");
	case Effect_HaveFun:
		return idStr("HaveFun");
	case Effect_ProvideCover:
		return idStr("ProvideCover");
	case Effect_BroadcastTargetMsgs:
		return idStr("BroadcastTargetMsgs");
	case Effect_Climb:
		return idStr("Climb");
	case Effect_Passageway:
		return idStr("Passageway");
	case Effect_CallBackup:
		return idStr("CallBackup");
	}

	return idStr("!!UNDEFINED!!");
}

//
// StrToCause()
//
hhReactionDesc::Cause hhReactionDesc::StrToCause(const char* str) {
	for(int i=0;i<Cause_Total;i++) {
		if(!CauseToStr((Cause)i).Icmp(str)) {
			return (Cause)i;
		}
	}

	return Cause_Invalid;
}

//
// StrToEffect()
//
hhReactionDesc::Effect hhReactionDesc::StrToEffect(const char* str) {
	for(int i=0;i<Effect_Total;i++)
	{
		if(!EffectToStr((Effect)i).Icmp(str))
			return (Effect)i;
	}
	
	return Effect_Invalid;
}

//
// Set()
//
void hhReactionDesc::Set(const idDict &keys) {
	
	cause				= StrToCause(keys.GetString("cause"));
	effect				= StrToEffect(keys.GetString("effect"));

	flags				= 0;
	effectRadius		= keys.GetFloat("effect_radius","-1.0f");
	effectMinRadius		= keys.GetFloat("effect_min_radius","0.0f");

	if(keys.GetBool("snap_to_point")) {
		flags |= flag_SnapToPoint;
	}
	if(keys.GetBool("effect_all_players")) {
		flags |= flag_EffectAllPlayers;
	}
	if(keys.GetBool("effect_all_monsters")) {
		flags |= flag_EffectAllMonsters;
	}
	if(keys.GetBool("req_novehicle")) {
		flags |= flagReq_NoVehicle;
	}
	if(keys.GetBool("exclusive")) {
		flags |= flag_Exclusive;
	}
	if(keys.GetBool("effect_listener")) {
		flags |= flag_EffectListener;
	}
	
	if(keys.GetBool("anim_face_cause_dir")) {
		flags |= flag_AnimFaceCauseDir;
	}
	if(keys.GetBool("anim_trigger_cause")) {
		flags |= flag_AnimTriggerCause;
	}
		

	idStr keyString("");
	if(keys.GetString("req_key", "", keyString)) {
		if(keyString.Length() > 1) {
			flags |= flagReq_KeyValue;
			int firstSpace = keyString.Find(' ');
			if( firstSpace >= 0 ) {				
				key		= keyString.Left( firstSpace);		
				keyVal	= keyString.Right( keyString.Length() - firstSpace -1);
			}		
		}
		else
			gameLocal.Error("Invalide key for 'reaction_req_ley' value: %s", (const char*)keyString);
	}
//MDC begin
	keyString.Empty();
	if( keys.GetString( "finish_key", "", keyString)) {
		if( keyString.Length() > 1 ) {
			int firstSpace = keyString.Find(' ');
			if( firstSpace >= 0 ) {
				finish_key = keyString.Left( firstSpace );
				finish_val = keyString.Right( keyString.Length() - firstSpace - 1 );
			}
		}
	}
//MDC end

	idStr animString("");
	if(keys.GetString("req_anim", "", animString)) {
		flags |= flagReq_Anim;
		anim = animString;
	}

	if(keys.GetBool("req_rangeattack")) {
		flags |= flagReq_RangeAttack;
	}
	if(keys.GetBool("req_meleeattack")) {
		flags |= flagReq_MeleeAttack;
	}
	if(keys.GetBool("req_can_see")) {
		flags |= flagReq_CanSee;
	}

	if(keys.GetBool("req_telepathic") || cause == hhReactionDesc::Cause_Telepathic_Throw || cause == hhReactionDesc::Cause_Telepathic_Trigger) {
		flags |= flagReq_Telepathic;
	}

	
	// Effect volumes
	effectVolumes.Clear();
	const idKeyValue *kv = keys.MatchPrefix("effect_volume", NULL);
	while(kv) {
		idStr effectVolName = kv->GetValue();
		if(effectVolName.Length() > 0) {
			idEntity *e = gameLocal.FindEntity(effectVolName.c_str());
			if(!e) {
				gameLocal.Error("Failed to find effect_volume named %s", effectVolName.c_str());
			}
			if(!e->IsType(hhReactionVolume::Type)) {
				gameLocal.Error("effect_volume named %s was of incorrect spawn type. Must be hhReactionVolume", effectVolName.c_str());
			}

			effectVolumes.AddUnique(static_cast<hhReactionVolume*>(e));
		}

		// Move to next volume
		kv = keys.MatchPrefix("effect_volume", kv);
	}	
	
	listenerRadius		= keys.GetFloat("listener_radius", "-1.0f");
	listenerMinRadius	= keys.GetFloat("listener_min_radius", "-1.0f");	

	// Store listener volumes
	listenerVolumes.Clear();
	kv = keys.MatchPrefix("listener_volume", NULL);
	while(kv) {
		idStr listenVolName = kv->GetValue();
		if(listenVolName.Length() > 0) {
			idEntity *e = gameLocal.FindEntity(listenVolName.c_str());
			if(!e) {
				gameLocal.Error("Failed to find listener_volume named %s", listenVolName.c_str());
			}
			if(!e->IsType(hhReactionVolume::Type)) {
				gameLocal.Error("listener_volume named %s was of incorrect spawn type. Must be hhReactionVolume", listenVolName.c_str());
			}

			listenerVolumes.AddUnique(static_cast<hhReactionVolume*>(e));
		}

		// Move to next volume
		kv = keys.MatchPrefix("listener_volume", kv);
	}

	// Store touchdir
	touchDir = keys.GetString("touchdir","");

	// Extract out touch offsets
	kv = keys.MatchPrefix("touchoffset_", NULL);
	touchOffsets.SetGranularity(1);
	touchOffsets.Clear();
	idStr tmpStr;
	idStr keyName;
	int usIndex;
	while(kv) {
		tmpStr = kv->GetKey();
		usIndex = tmpStr.FindChar("touchoffset_", '_');
		keyName = tmpStr.Mid(usIndex+1, strlen(kv->GetKey())-usIndex-1);
		touchOffsets.Set(keyName, kv->GetValue());
		
		// Move to the next
		kv = keys.MatchPrefix("touchoffset_", kv);
	}

	// Extract out touch bounds
	kv = keys.MatchPrefix("safebound_", NULL );
	while(kv) {
		tmpStr = kv->GetKey();
		usIndex = tmpStr.FindChar("safebound_", '_');
		keyName = tmpStr.Mid(usIndex+1, strlen(kv->GetKey())-usIndex-1);
		touchOffsets.Set(keyName, kv->GetValue());
		kv = keys.MatchPrefix("safebound_", kv);
	}
}

/*
================
hhReactionDesc::Save
================
*/
void hhReactionDesc::Save(idSaveGame *savefile) const {
	savefile->WriteString(name);
	savefile->WriteInt(effect);
	savefile->WriteInt(cause);
	savefile->WriteInt(flags);
	savefile->WriteFloat(effectRadius);
	savefile->WriteFloat(effectMinRadius);

	int num = effectVolumes.Num();
	savefile->WriteInt(num);
	for(int i = 0; i < num; i++) {
		savefile->WriteObject( effectVolumes[i] );
	}

	savefile->WriteString(key);
	savefile->WriteString(keyVal);
	savefile->WriteString(anim);
	savefile->WriteFloat(listenerRadius);
	savefile->WriteFloat(listenerMinRadius);

	num = listenerVolumes.Num();
	savefile->WriteInt(num);
	for(int i = 0; i < num; i++) {
		savefile->WriteObject(listenerVolumes[i]);
	}

	savefile->WriteDict(&touchOffsets);
	savefile->WriteString(touchDir);
	savefile->WriteString(finish_key);
	savefile->WriteString(finish_val);
}

/*
================
hhReactionDesc::Restore
================
*/
void hhReactionDesc::Restore( idRestoreGame *savefile ) {
	savefile->ReadString(name);
	savefile->ReadInt(reinterpret_cast<int &> (effect));
	savefile->ReadInt(reinterpret_cast<int &> (cause));
	savefile->ReadInt(reinterpret_cast<int &> (flags));
	savefile->ReadFloat(effectRadius);
	savefile->ReadFloat(effectMinRadius);

	int num;
	savefile->ReadInt(num);
	effectVolumes.SetNum(num);
	for(int i = 0; i < num; i++) {
		savefile->ReadObject(reinterpret_cast<idClass *&> (effectVolumes[i]));
	}

	savefile->ReadString(key);
	savefile->ReadString(keyVal);
	savefile->ReadString(anim);
	savefile->ReadFloat(listenerRadius);
	savefile->ReadFloat(listenerMinRadius);

	savefile->ReadInt(num);
	listenerVolumes.SetNum(num);
	for(int i = 0; i < num; i++) {
		savefile->ReadObject(reinterpret_cast<idClass *&> (listenerVolumes[i]));
	}

	savefile->ReadDict(&touchOffsets);
	savefile->ReadString(touchDir);
	savefile->ReadString(finish_key);
	savefile->ReadString(finish_val);
}

///////////////////////////////////////////
//
// hhReactionVolume
//
///////////////////////////////////////////
CLASS_DECLARATION(idEntity, hhReactionVolume)	
END_CLASS

//
// Spawn()
//
void hhReactionVolume::Spawn() {

	flags = 0;

	if(spawnArgs.GetBool("players", "1")) {
		flags |= flagPlayers;
	}

	if(spawnArgs.GetBool("monsters", "1")) {
		flags |= flagMonsters;
	}

	if(flags == 0) {
		gameLocal.Error("Reaction volume %s is not targetting players or monsters - Why even have this volume?", (const char*)name);
	}

}

bool hhReactionVolume::IsValidEntity( idEntity *ent ) {
	assert(GetPhysics() != NULL);
	assert(flags != 0); // have to be looking for something - else whats the point?	
	
	if ( !ent || ent->IsHidden() ) {
		return false;
	}
	if(flags & flagMonsters && !ent->IsType(idAI::Type)) {
		if(flags & flagPlayers && !ent->IsType(hhPlayer::Type)) {
			return false;
		}
	}
	idEntity *	entityList[ MAX_GENTITIES ];
	int numEnts = gameLocal.clip.EntitiesTouchingBounds(GetPhysics()->GetAbsBounds(), CONTENTS_BODY, entityList, MAX_GENTITIES);
	for(int i=0;i<numEnts;i++) {
		if ( entityList[i] == ent ) {
			return true;
		}
	}

	return false;
}

//
// GetValidEntitiesInside()
//
void hhReactionVolume::GetValidEntitiesInside(idList<idEntity*> &outValidEnts) {
	
	assert(GetPhysics() != NULL);
	assert(flags != 0); // have to be looking for something - else whats the point?	
	
	idEntity *	entityList[ MAX_GENTITIES ];
	
	int numEnts = gameLocal.clip.EntitiesTouchingBounds(GetPhysics()->GetAbsBounds(), CONTENTS_BODY, entityList, MAX_GENTITIES);
	for(int i=0;i<numEnts;i++) {
		if(!entityList[i] || entityList[i]->IsHidden())
			continue;
		if(flags & flagPlayers && entityList[i]->IsType(hhPlayer::Type)) {
			outValidEnts.Append(entityList[i]);
			if(ai_debugBrain.GetInteger()) {
				gameRenderWorld->DebugArrow(colorMagenta, GetOrigin(), entityList[i]->GetOrigin(), 10, 1000);
			}
			continue;
		}		
	}

	if(ai_debugBrain.GetInteger()) {
		gameRenderWorld->DebugBounds(colorWhite, GetPhysics()->GetBounds(), GetOrigin(), 1000);
	}
}

/*
================
hhReactionVolume::Save
================
*/
void hhReactionVolume::Save(idSaveGame *savefile) const {
	savefile->WriteInt(flags);
}

/*
================
hhReactionVolume::Restore
================
*/
void hhReactionVolume::Restore(idRestoreGame *savefile) {
	savefile->ReadInt(flags);
}

///////////////////////////////////////////
//
// hhReactionHandler
//
///////////////////////////////////////////


//
// construction
//
hhReactionHandler::hhReactionHandler() {
}
	

//
// destruction
//
hhReactionHandler::~hhReactionHandler() {
	reactionDescs.DeleteContents(TRUE);
}

//
// LoadReactionDesc()
//
const hhReactionDesc* hhReactionHandler::LoadReactionDesc(const char* defName, const idDict *uniqueDescKeys) {

	// Ignore null reactions
	if ( !defName || !defName[0] ) {
		return NULL;
	}

	const hhReactionDesc *ret = NULL;

	// If we do not have unqiue keys, we can just use a cached version
	if(!uniqueDescKeys) {
		ret = FindReactionDesc(defName);
		if(ret) {
			return ret;
		}
	}

	// Get the def dict
	const idDict *reactDefDict = gameLocal.FindEntityDefDict(defName);
	if(!reactDefDict) {
		gameLocal.Error("Failed to find reaction dict named %s", defName);
	}

	// Build the keys that will be passed onto the creation
	idDict finalDict;	
	finalDict.Copy(*reactDefDict);
	StripReactionPrefix(finalDict);
	if(uniqueDescKeys) {
		idDict uniqueCopy(*uniqueDescKeys);
		StripReactionPrefix(uniqueCopy);
		finalDict.Copy(uniqueCopy);
	}

	hhReactionDesc* react = CreateReactionDesc(defName, finalDict);
	return react;
}

//
// createReactionDesc()
//
hhReactionDesc* hhReactionHandler::CreateReactionDesc(const char *desiredName, const idDict &keys) {

	idStr finalName = GetUniqueReactionDescName(desiredName);
	hhReactionDesc *desc = new hhReactionDesc();
	
	desc->name = finalName;	
	desc->Set(keys);

	reactionDescs.Append(desc);
	return desc;	
}

//
// getUniqueReactionDescName()
//
idStr hhReactionHandler::GetUniqueReactionDescName(const char *name) {

	int count = 0;
	idStr finalName(name);
	while(1) {
		if(count > 0) {
			finalName = idStr(name) + idStr("_") + idStr(count);
		}

		// Did we find a name that was not taken?
		if(FindReactionDesc(finalName.c_str()) == NULL) {
			return finalName;
		}
#ifdef _DEBUG
		if(count > 128) {
			assert(FALSE); // How the hell did we get so many copies?
		}
#endif

		count++;
	}
}
	
//
// findReactionDesc()
//
const hhReactionDesc* hhReactionHandler::FindReactionDesc(const char *name) {
	
	for(int i=0;i<reactionDescs.Num();i++) {
	
		if(strcmp(reactionDescs[i]->name.c_str(), name) == 0) {
			return reactionDescs[i];
		}
	}
		
	return NULL;
}

//
// stripReactionPrefix()
//
void hhReactionHandler::StripReactionPrefix(idDict &dict) {
	const idKeyValue *kv = NULL;//dict.MatchPrefix("reaction");

	idDict newDict;
	
	for(int i=0;i<dict.GetNumKeyVals();i++) {
		kv = dict.GetKeyVal(i);
		
		idStr key;
		key = kv->GetKey();

		// Do we have a reaction token to strip out?
		if(idStr::FindText(key.c_str(), "reaction") != -1) {
		
			int endPrefix = idStr::FindChar(key.c_str(), '_');
			if(endPrefix == -1) {
				gameLocal.Error("reactionX_ prefix not found.");
			}
			idStr realKey(key);
			realKey = key.Mid(endPrefix+1, key.Length() - endPrefix - 1);
			key = realKey;
		}		
		
		//dict.Delete(kv->GetKey().c_str());
		newDict.Set(key.c_str(), kv->GetValue());

		kv = dict.MatchPrefix("reaction", kv);
	}
	

	dict.Clear();
	dict.Copy(newDict);
}

void hhReactionHandler::Save(idSaveGame *savefile) const {
	savefile->WriteInt(reactionDescs.Num());
	for (int i = 0; i < reactionDescs.Num(); i++) {
		reactionDescs[i]->Save(savefile);
	}
}

void hhReactionHandler::Restore( idRestoreGame *savefile ) {
	reactionDescs.DeleteContents(true);
	int num;
	savefile->ReadInt(num);
	reactionDescs.SetNum(num);
	for (int i = 0; i < num; i++) {
		reactionDescs[i] = new hhReactionDesc();
		reactionDescs[i]->Restore(savefile);
	}
}

void hhReaction::Save(idSaveGame *savefile) const {
	savefile->WriteBool(active);
	exclusiveOwner.Save(savefile);
	savefile->WriteInt(exclusiveUntil);
	if (desc) {
		savefile->WriteString(desc->name);
	} else {
		savefile->WriteString("");
	}
	causeEntity.Save(savefile);
	int num = effectEntity.Num();
	savefile->WriteInt(num);
	for(int i = 0; i < num; i++) {
		effectEntity[i].ent.Save(savefile);
		savefile->WriteFloat(effectEntity[i].weight);
	}
}
	
void hhReaction::Restore(idRestoreGame *savefile) {
	savefile->ReadBool(active);
	exclusiveOwner.Restore(savefile);
	savefile->ReadInt(exclusiveUntil);
	
	idStr name;
	savefile->ReadString(name);
	desc = gameLocal.GetReactionHandler()->FindReactionDesc(name); 
	causeEntity.Restore(savefile);
	int num;
	savefile->ReadInt(num);
	hhReactionTarget ent;
	for(int i = 0; i < num; i++) {
		ent.ent.Restore(savefile);
		savefile->ReadFloat(ent.weight);
		effectEntity.Append(ent);
	}
}

