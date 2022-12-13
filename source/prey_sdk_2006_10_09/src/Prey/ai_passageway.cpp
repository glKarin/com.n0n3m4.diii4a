//
// ai_passageway.cpp
//
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_EnablePassageway("enable", NULL);
const idEventDef EV_DisablePassageway("disable", NULL);

// hhAIPassageway
CLASS_DECLARATION(hhAnimated, hhAIPassageway)
	EVENT(EV_AnimDone,			hhAIPassageway::Event_AnimDone)
	EVENT(EV_Activate,	   		hhAIPassageway::Event_Trigger)
	EVENT(EV_PostSpawn,			hhAIPassageway::PostSpawn)
	EVENT(EV_EnablePassageway,	hhAIPassageway::Event_EnablePassageway)
	EVENT(EV_DisablePassageway,	hhAIPassageway::Event_DisablePassageway)
END_CLASS

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build

//
// Spawn()
//
void hhAIPassageway::Spawn() {
	timeLastEntered	= 0;
	lastEntered		= NULL;
	enabled			= TRUE;
	hasEntered		= FALSE;

	GetPhysics()->SetContents(CONTENTS_SOLID);

	PostEventMS( &EV_PostSpawn, 0 );
}

//
// PostSpawn()
//
void hhAIPassageway::PostSpawn() {
	SetEnablePassageway(spawnArgs.GetBool("enabled", "1"));
}

//
// SetEnablePassageway()
//
void hhAIPassageway::SetEnablePassageway(bool tf) {
	
	// Ignore if we're already in that state
	if(enabled == tf)
		return;
	
	fl.refreshReactions = tf;
	enabled				= tf;
}

//
// Event_AnimDone()
//
void hhAIPassageway::Event_AnimDone( int animIndex ) {
	//When we are done we want to pass ourselves as the activator
	activator = this;

	hhAnimated::Event_AnimDone( animIndex );
}

//
// Event_Trigger()
//
void hhAIPassageway::Event_Trigger( idEntity *activator ) {

	if(!activator)
		return;

	// Ignore being activated by another passage node - they link to eachother,
	// so they must be ignored otherwise it will continuously trigger
	if( activator->IsType(hhAIPassageway::Type) ) {
		return;
	}

	// Play our anim
	hhAnimated::Event_Activate( activator );
	HH_ASSERT(anim != NULL);

	if( !activator->IsType(hhMonsterAI::Type) ) {
		return;
	}

	hhMonsterAI *ai = static_cast<hhMonsterAI*>(activator);

	// Can't enter if dead
	if(ai->health <= 0)
		return;

	if(!ai->IsInsidePassageway())
	{
		//HUMANHEAD PCF mdl 04/29/06 - Added lastEntered check
		if(hasEntered && lastEntered.GetEntity() == ai) {
			ai->ProcessEvent(&MA_EnterPassageway, this);
			hasEntered = FALSE;
		} else {
			lastEntered		= ai;
			timeLastEntered	= gameLocal.GetTime();
			hasEntered = TRUE;
		}	
		
	}
	else
	{
		ai->PostEventMS(&MA_ExitPassageway, 32, this);
		hasEntered = FALSE;
	}
}

//
// GetExitPos()
//
idVec3 hhAIPassageway::GetExitPos(void) {
	idVec3 pos = GetOrigin();
	idVec3 exitOffset;
	spawnArgs.GetVector("exit_offset", "0 0 0", exitOffset);
	exitOffset *= GetAxis();
	pos += exitOffset;
	pos.z += 0.2f;
	return pos;
}

void hhAIPassageway::Save(idSaveGame *savefile) const {
	savefile->WriteInt( timeLastEntered - gameLocal.time );
	lastEntered.Save( savefile );
	savefile->WriteBool( hasEntered );
	savefile->WriteBool( enabled );
}

void hhAIPassageway::Restore(idRestoreGame *savefile) {
	savefile->ReadInt( timeLastEntered );
	timeLastEntered += gameLocal.time;

	lastEntered.Restore( savefile );
	savefile->ReadBool( hasEntered );
	savefile->ReadBool( enabled );	
}

#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build