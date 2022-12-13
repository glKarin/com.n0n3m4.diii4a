#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_CheckCycleRotate( "<checkCycleRotate>" );
const idEventDef EV_CheckThaw( "<checkThaw>" );
const idEventDef EV_SpawnFxAlongBone( "spawnFXAlongBone", "d" );
const idEventDef EV_Silent( "<silent>", NULL );

CLASS_DECLARATION( idAnimatedEntity, hhAnimatedEntity )
	EVENT( EV_CheckCycleRotate,			hhAnimatedEntity::Event_CheckAnimatorCycleRotate )
	EVENT( EV_CheckThaw,				hhAnimatedEntity::Event_CheckAnimatorThaw )	
	EVENT( EV_SpawnFxAlongBone,			hhAnimatedEntity::Event_SpawnFXAlongBone )
	EVENT( EV_Thread_SetSilenceCallback,hhAnimatedEntity::Event_SetSilenceCallback )
	EVENT( EV_Silent,					hhAnimatedEntity::Event_Silent )
END_CLASS

/*
==============
hhAnimatedEntity::Spawn
==============
*/
void hhAnimatedEntity::Spawn( void ) {
	hasFlapInfo = false;

	// nla - Added to allow for 1 frame anims to play/proper bone initialization
	if ( GetAnimator() ) {
		int anim = GetAnimator()->GetAnim("init");
		if ( anim ) {
			GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, 0);
			GetAnimator()->ForceUpdate();
			UpdateModel();	
		}
	}
}

void hhAnimatedEntity::Save(idSaveGame *savefile) const {
	savefile->WriteInt( waitingThread );
	savefile->WriteInt( silentTimeOffset );
	savefile->WriteInt( nextSilentTime );
	savefile->WriteFloat( lastAmplitude );
}

void hhAnimatedEntity::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( waitingThread );
	savefile->ReadInt( silentTimeOffset );
	savefile->ReadInt( nextSilentTime );
	savefile->ReadFloat( lastAmplitude );
	hasFlapInfo = false;
}

/*
==============
hhAnimatedEntity::hhAnimatedEntity
==============
*/
hhAnimatedEntity::hhAnimatedEntity() {
	// WaitForSilence support
	waitingThread = 0;
	silentTimeOffset = 0;
	nextSilentTime = 0;
	lastAmplitude = 0.0f;
	hasFlapInfo = false;
}

/*
==============
hhAnimatedEntity::~hhAnimatedEntity
==============
*/
hhAnimatedEntity::~hhAnimatedEntity() {
	Event_Silent();
}

/*
==============
hhAnimatedEntity::FillDebugVars
==============
*/
void hhAnimatedEntity::FillDebugVars(idDict *args, int page) {
	switch(page) {
	case 1:
		args->SetInt("anims", GetAnimator()->NumAnims());
		break;
	}
	idAnimatedEntity::FillDebugVars(args, page);
}

/*
==============
hhAnimatedEntity::Think
==============
*/
void hhAnimatedEntity::Think() {
	idAnimatedEntity::Think();

	//HUMANHEAD: aob - moved from idActor
	UpdateWounds();
	//HUMANHEAD END
}

/*
=====================
hhAnimatedEntity::GetAnimator
=====================
*/
hhAnimator *hhAnimatedEntity::GetAnimator( void ) {
	return &animator;
}

/*
=====================
hhAnimatedEntity::GetAnimator
=====================
*/
const hhAnimator *hhAnimatedEntity::GetAnimator( void ) const {
	return &animator;
}

/*
=====================
hhAnimatedEntity::GetJointWorldTransform
=====================
*/
bool hhAnimatedEntity::GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis ) {
	return idAnimatedEntity::GetJointWorldTransform( jointHandle, currentTime, offset, axis );
}

/*
=====================
hhAnimatedEntity::GetJointWorldTransform
=====================
*/
bool hhAnimatedEntity::GetJointWorldTransform( const char* jointName, idVec3 &offset, idMat3 &axis ) {
	return GetJointWorldTransform( GetAnimator()->GetJointHandle(jointName), gameLocal.GetTime(), offset, axis );
}

/*
=====================
hhAnimatedEntity::GetJointWorldTransform
=====================
*/
bool hhAnimatedEntity::GetJointWorldTransform( jointHandle_t jointHandle, idVec3 &offset, idMat3 &axis ) {
	return GetJointWorldTransform( jointHandle, gameLocal.GetTime(), offset, axis );
}

/*
==============
hhAnimatedEntity::UpdateWounds
==============
*/
void hhAnimatedEntity::UpdateWounds( void ) {
}

/*
==================
idEntity::SpawnFXAlongBonePrefixLocal
==================
*/
void hhAnimatedEntity::SpawnFxAlongBonePrefixLocal( const idDict* dict, const char* fxKeyPrefix, const char* bonePrefix, const hhFxInfo* const fxInfo, const idEventDef* eventDef ) {
	idVec3 bonePos;
	idMat3 boneAxis;

	for( const idKeyValue* kv = dict->MatchPrefix(bonePrefix); kv; kv = dict->MatchPrefix(bonePrefix, kv) ) {
		if( !kv->GetValue().Length() ) {
			continue;
		}
		
		GetJointWorldTransform( kv->GetValue().c_str(), bonePos, boneAxis );
		SpawnFXPrefixLocal( dict, fxKeyPrefix, bonePos, boneAxis, fxInfo, eventDef );
	}
}

/*
=================
hhAnimatedEntity::SpawnFXAlongBone
=================
*/
void hhAnimatedEntity::BroadcastFxInfoAlongBonePrefix( const idDict* args, const char* fxKey, const char* bonePrefix, const hhFxInfo* const fxInfo, const idEventDef* eventDef, bool broadcast ) {
	if( !fxKey || !fxKey[0] || !bonePrefix || !bonePrefix[0] ) {
		return;
	}

	idVec3 bonePos;
	idMat3 boneAxis;
	hhFxInfo localFxInfo( fxInfo );

	localFxInfo.SetEntity( this );
	localFxInfo.RemoveWhenDone( true );

	const idKeyValue* keyValue = args->MatchPrefix( bonePrefix );
	while( keyValue && keyValue->GetValue().Length() ) {
		
		if( GetJointWorldTransform( keyValue->GetValue().c_str(), bonePos, boneAxis ) ) {
			//AOB: HACK - crashed in idBitMsg::DirToBits because we didn't appear to be normalized
			localFxInfo.SetNormal( boneAxis[0].ToNormal() );
			localFxInfo.SetBindBone( keyValue->GetValue().c_str() );

			BroadcastFxInfoPrefixed( fxKey, bonePos, boneAxis, &localFxInfo, eventDef, broadcast );
		}

		keyValue = args->MatchPrefix( bonePrefix, keyValue );
	}
}

/*
=================
hhAnimatedEntity::SpawnFXAlongBone
mdl
=================
*/
void hhAnimatedEntity::BroadcastFxInfoAlongBonePrefixUnique( const idDict* args, const char* fxKey, const char* bonePrefix, const hhFxInfo* const fxInfo, const idEventDef* eventDef, bool broadcast ) {
	if( !fxKey || !fxKey[0] || !bonePrefix || !bonePrefix[0] ) {
		return;
	}

	idVec3 bonePos;
	idMat3 boneAxis;
	hhFxInfo localFxInfo( fxInfo );

	localFxInfo.SetEntity( this );
	localFxInfo.RemoveWhenDone( true );

	const idKeyValue* keyValue = args->MatchPrefix( bonePrefix );
	idStr postfix;
	int prefixLen = strlen( bonePrefix );
	while( keyValue && keyValue->GetValue().Length() ) {
		postfix = keyValue->GetKey().Right( keyValue->GetKey().Length() - prefixLen );
		
		if( GetJointWorldTransform( keyValue->GetValue().c_str(), bonePos, boneAxis ) ) {
			//AOB: HACK - crashed in idBitMsg::DirToBits because we didn't appear to be normalized
			localFxInfo.SetNormal( boneAxis[0].ToNormal() );
			localFxInfo.SetBindBone( keyValue->GetValue().c_str() );

			// Changed this because it was causing every fx listed to be bound to every bone listed. -mdl
			BroadcastFxInfo( spawnArgs.GetString( fxKey + postfix ), bonePos, boneAxis, &localFxInfo, eventDef, broadcast ); 
			//BroadcastFxInfoPrefixed( fxKey, bonePos, boneAxis, &localFxInfo, eventDef );
		}

		keyValue = args->MatchPrefix( bonePrefix, keyValue );
	}
}

/*
================
hhAnimatedEntity::SpawnFxAlongBone
================
*/
void hhAnimatedEntity::BroadcastFxInfoAlongBone( const char* fxName, const char* boneName, const hhFxInfo* const fxInfo, const idEventDef* eventDef, bool broadcast ) {
	//mdc: pass through to our newer version of the function (this is mainly done for backwards compatibility)
	BroadcastFxInfoAlongBone( false, fxName, boneName, fxInfo, eventDef, broadcast );
}

void hhAnimatedEntity::BroadcastFxInfoAlongBone( bool bNoRemoveWhenUnbound, const char* fxName, const char* boneName, const hhFxInfo* const fxInfo, const idEventDef* eventDef, bool broadcast ) {
	if( !fxName || !fxName[0] || !boneName || !boneName[0] ) {
		return;
	}

	idVec3 bonePos;
	idMat3 boneAxis;
	hhFxInfo localFxInfo( fxInfo );

	GetJointWorldTransform( boneName, bonePos, boneAxis );
	localFxInfo.SetNormal( boneAxis[0] );
	localFxInfo.SetEntity( this );
	localFxInfo.NoRemoveWhenUnbound( bNoRemoveWhenUnbound );	//mdc: added
	localFxInfo.SetBindBone( boneName );

	BroadcastFxInfo( fxName, bonePos, boneAxis, &localFxInfo, eventDef, broadcast );
}

/*
================
hhAnimatedEntity::Event_SpawnFXAlongBone
================
*/
void hhAnimatedEntity::Event_SpawnFXAlongBone( idList<idStr>* fxParms ) {
	if ( !fxParms ) {
		return;
	}

	bool bNoRemoveWhenUnbound = false;	//default to false (we normally want to delete fx when the bound-entity is deleted)
	HH_ASSERT( fxParms->Num() >= 2 && fxParms->Num() <= 3 );	//allow 2-3 parameters
	if( fxParms->Num() == 3 ) {
		//make sure the 3rd paramater is our special noRemoveWhenUnbound flag
		if( !idStr::Icmp((*fxParms)[2].c_str(), "noremovewhenunbound") ) {
			bNoRemoveWhenUnbound = true;
		}
		else {
			gameLocal.Warning( "unrecognized 3rd paramater on frame command for SpawnFXAlongBone on entity '%s'", this->GetName() );
		}
	}
	BroadcastFxInfoAlongBone(bNoRemoveWhenUnbound, spawnArgs.GetString((*fxParms)[0].c_str()), (*fxParms)[1].c_str() );
}


// HUMANHEAD pdm: Jaw flapping support
// This provides generalized support for any animated entity to translate voice channel sound to bone movements
void hhAnimatedEntity::JawFlap(hhAnimator *theAnimator) {
	PROFILE_SCOPE("Jaw Flap", PROFMASK_NORMAL);
	float amplitude = 0.0f;

	if (fl.noJawFlap || !g_jawflap.GetBool()) {
		return;
	}

	// Get amplitude from head or body
	idEntity *headEntity = NULL;
	if (IsType(idActor::Type)) {
		headEntity = static_cast<idActor*>(this)->GetHead();
		if (headEntity && headEntity->GetSoundEmitter()) {
			amplitude = headEntity->GetSoundEmitter()->CurrentVoiceAmplitude( SND_CHANNEL_VOICE );
		}
	}

	if( amplitude == 0.0f && GetSoundEmitter() ) {
		amplitude = GetSoundEmitter()->CurrentVoiceAmplitude( SND_CHANNEL_VOICE );
	}

	if (amplitude != 0.0f || lastAmplitude != 0.0f) {
		//HUMANHEAD rww - only get the joint handles and info for jaw flapping once
		//we have to do this here, since the head is not valid at hhAnimatedEntity::Spawn
		if (!hasFlapInfo) {
			jawFlapList.Clear();
			const char *prefix = "jawbone_";
			const idKeyValue *kv = spawnArgs.MatchPrefix(prefix);
			while( kv && kv->GetValue().Length() ) {
				jointHandle_t bone = theAnimator->GetJointHandle( kv->GetValue() );
				if (bone != INVALID_JOINT) {
					jawFlapInfo_t flapInfo;

					idStr xformName = kv->GetKey();
					xformName.Strip(prefix);

					flapInfo.bone = bone;
					flapInfo.rMagnitude = spawnArgs.GetVector( va("jawflapR_%s", xformName.c_str()) );
					flapInfo.tMagnitude = spawnArgs.GetVector( va("jawflapT_%s", xformName.c_str()) );
					flapInfo.rMinThreshold = spawnArgs.GetFloat( va("jawflapRMin_%s", xformName.c_str()) );
					flapInfo.tMinThreshold = spawnArgs.GetFloat( va("jawflapTMin_%s", xformName.c_str()) );
					jawFlapList.Append(flapInfo);
				}
				kv = spawnArgs.MatchPrefix(prefix, kv);
			}
			hasFlapInfo = true;
		}

		lastAmplitude = amplitude;
		//HUMANHEAD rww - changed to use a list and not do constant runtime joint lookups
		for (int i = 0; i < jawFlapList.Num(); i++) {
			jawFlapInfo_t &flapInfo = jawFlapList[i];

			// Handle Rotation
			idAngles angles = ang_zero;
			if (amplitude > flapInfo.rMinThreshold) {
				float factor = amplitude - flapInfo.rMinThreshold;
				angles = idAngles(factor*flapInfo.rMagnitude[0], factor*flapInfo.rMagnitude[1], factor*flapInfo.rMagnitude[2]);
			}
			theAnimator->SetJointAxis( flapInfo.bone, JOINTMOD_WORLD, angles.ToMat3() );

			// Handle translation
			idVec3 translate = vec3_origin;
			if (amplitude > flapInfo.tMinThreshold) {
				float factor = amplitude - flapInfo.tMinThreshold;
				translate = idVec3(factor*flapInfo.tMagnitude[0], factor*flapInfo.tMagnitude[1], factor*flapInfo.tMagnitude[2]);
			}
			theAnimator->SetJointPos( flapInfo.bone, JOINTMOD_WORLD, translate );
		}
	}
}

// WaitForSilence support: Tracks the next time the voice channel will be silent, and if any threads are doing a waitForSilence() block, calls them back.
// If a waitForSilence() call is issued while no sound is playing on the voice channel, it will wait until there is something and then wait for silence.
bool hhAnimatedEntity::StartSoundShader( const idSoundShader *shader, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length ) {
	int localLength = 0;
	bool retVal = idAnimatedEntity::StartSoundShader(shader, channel, soundShaderFlags, broadcast, &localLength);

	// need to track 'nextSilentTime' locally also, so if a tiny sound is played after a longer sound, it doesn't think silence will follow short sound
	if (channel == SND_CHANNEL_VOICE && (localLength > 0) && (gameLocal.time + localLength > nextSilentTime)) {
		nextSilentTime = gameLocal.time + localLength;

		// If there's already a thread waiting for silence and this sound pushes silence out further, repost an event for it
		if (waitingThread) {
			CancelEvents(&EV_Silent);
			int time = localLength+silentTimeOffset;
			time = idMath::ClampInt(1, time, time);	// Keep positive and wait at least 1ms
			PostEventMS(&EV_Silent, time);
		}
	}
	if (length) {
		*length = localLength;
	}
	return retVal;
}

void hhAnimatedEntity::Event_Silent() {
	if (waitingThread) {
		idThread::ObjectMoveDone( waitingThread, this );
		waitingThread = 0;
	}
}

void hhAnimatedEntity::Event_SetSilenceCallback(float plusOrMinusSeconds) {

	if (!waitingThread) {
		waitingThread = idThread::CurrentThreadNum();
		silentTimeOffset = SEC2MS(plusOrMinusSeconds);

		if (nextSilentTime > gameLocal.time) {
			// If sound already playing
			int time = (nextSilentTime - gameLocal.time) + silentTimeOffset;
			time = idMath::ClampInt(1, time, time);	// Keep positive and wait at least 1ms
			CancelEvents(&EV_Silent);
			PostEventMS(&EV_Silent, time);
		}
		else {
			// No sound playing, wait for sound to play first
		}
		idThread::ReturnInt( true );
	}
	else {
		idThread::ReturnInt( false );
	}
}
