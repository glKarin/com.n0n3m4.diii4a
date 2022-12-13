
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

//
// hhAISpawnCase
//

const idEventDef EV_CreateAI( "<createAI>", NULL);
const idEventDef EV_AutoTrigger( "<autotrigger>", NULL);

CLASS_DECLARATION( hhAnimated, hhAISpawnCase )
	EVENT( EV_Activate,	   	hhAISpawnCase::Event_Trigger		)
	EVENT( EV_CreateAI,		hhAISpawnCase::Event_CreateAI		)
	EVENT( EV_AutoTrigger,	hhAISpawnCase::Event_AutoTrigger	)
END_CLASS

//
// Spawn()
//
void hhAISpawnCase::Spawn( void ) {

	triggerToggle			= FALSE;
	aiSpawnCount			= 0;	
	waitingForAutoTrigger	= FALSE;
	triggerQueue			= 0;

	CreateEntSpawnArgs();

	GetPhysics()->SetContents(CONTENTS_SHOOTABLE|CONTENTS_PLAYERCLIP|CONTENTS_MOVEABLECLIP|CONTENTS_IKCLIP|CONTENTS_SHOOTABLEBYARROW);

	// Next frame, create the AI to attach to this case - Wait a frame so that we can copy targets
	PostEventMS(&EV_CreateAI, 0);
}

//
// Event_CreateAI()
//
void hhAISpawnCase::Event_CreateAI( void ) {

	// Spawn the monster
	const char *monsterName = spawnArgs.GetString("def_monster", NULL);
	int i;
	if ( aiSpawnCount < spawnArgs.GetInt("max_monsters", "1") && monsterName && monsterName[0]) {
		idDict args = entSpawnArgs;

		idVec3 offset = spawnArgs.GetVector("monster_offset", "0 0 0");
		offset *= GetPhysics()->GetAxis();
		
		args.SetVector( "origin", GetPhysics()->GetOrigin() + offset);
		args.SetBool("encased", TRUE);		
		args.Set("encased_idle",	spawnArgs.GetString("anim_encased_monster_idle", "encased_idle"));
		args.Set("encased_exit",	spawnArgs.GetString("anim_encased_monster_exit", "encased_exit"));
		args.Set( "encased_exit_offset",	spawnArgs.GetString( "encased_exit_offset", "0 0 0" ) );
		args.SetMatrix("rotation", GetAxis());
		encasedAI = static_cast<idAI*>(gameLocal.SpawnObject(monsterName, &args));
		if(!encasedAI.GetEntity()) {
			//gameLocal.Warning("No monster specified for case");
			return;
		}		
		
		// Copy the targets that our case has to our newly spawned monster
		for( i = 0; i < targets.Num(); i++ ) {
			encasedAI->targets.AddUnique(targets[i]);			
		}
		if ( spawnArgs.GetBool( "rotated" ) ) {
			encasedAI->SetAxis( GetAxis() );
			encasedAI->GetPhysics()->SetAxis( GetAxis() );
			encasedAI->GetRenderEntity()->axis = GetAxis();
			encasedAI->viewAxis = GetAxis();
			encasedAI->Bind(this, true);
		} else {
			encasedAI->viewAxis = GetAxis();
			encasedAI->Bind(this, FALSE);
		}
		aiSpawnCount++;

		if(spawnArgs.GetBool("monster_hide", "0")) {
			encasedAI->ProcessEvent(&EV_Hide);
		}
	}
}

//
// Event_Trigger()
//
void hhAISpawnCase::Event_Trigger( idEntity *activator ) {	
	bool triggeredAI = false;
	bool showedAI = false;

	// We are waiting for an auto-trigger, so don't fire now - but remember that we need to later
	if(waitingForAutoTrigger) {
		triggerQueue++;
		//gameLocal.Printf("\nQUEUED TRIGGERS: %i", triggerQueue);
		return;
	}
	
	//gameLocal.Printf("\n * TRIGGERED *");
	if(encasedAI.GetEntity()) {
		if(encasedAI->IsHidden()) {
			encasedAI->ProcessEvent(&EV_Show);
			showedAI = true;
		}

		//if triggered_spawn is set, spawn the encasedAI on the first trigger, 
		//and but don't activate it until a second trigger
		if ( !showedAI || !spawnArgs.GetBool( "triggered_spawn", "0" ) ) {
			encasedAI->Unbind();
			// removed since monster_offset should take care of this
			//encasedAI->SetOrigin(encasedAI->GetOrigin() + encasedAI->viewAxis[0] * 64);
			encasedAI.GetEntity()->ProcessEvent(&EV_Activate, activator);		
			triggeredAI = true;
		}
	}

	// Start the anim playing
	hhAnimated::Event_Activate(activator);

	if ( spawnArgs.GetInt( "no_anim", "0" ) ) {
		ProcessEvent(&EV_CreateAI);
		//if triggered_spawn is set, dont show the next AI until the next trigger
		if ( encasedAI.IsValid() && triggeredAI && spawnArgs.GetBool( "triggered_spawn", "0" ) ) {
			encasedAI->ProcessEvent(&EV_Hide);
		}
	}
	triggerToggle = !triggerToggle;
}

//
// Event_AutoTrigger()
//
void hhAISpawnCase::Event_AutoTrigger( void ) {
	waitingForAutoTrigger = FALSE;
	ProcessEvent(&EV_Activate, this);
}

//
// Event_AnimDone()
//
void hhAISpawnCase::Event_AnimDone( int animIndex ) {

	// Call back first
	hhAnimated::Event_AnimDone(animIndex);

	const char *n = NULL;
	
	// Door is OPEN, now we queue up the CLOSE anim
	if(triggerToggle) {
		n = spawnArgs.GetString("anim_retrigger");

		// Should we automatically retrigger? (ie. close the door after it was opened?)
		if(spawnArgs.GetFloat("auto_retrigger_delay", "-1") >= 0.0f ) {
			int delay = SEC2MS(spawnArgs.GetFloat("auto_retrigger_delay", "-1"));
			waitingForAutoTrigger = TRUE;
			PostEventMS(&EV_AutoTrigger, delay);
		}
	}
	// Door is CLOSED, now we queue up the OPEN anim
	else {

		// If we have queued triggers saved up - then lets fire one now since we are now closed
		if(triggerQueue > 0) {
			triggerQueue--;
			PostEventMS(&EV_Activate, 0, this);
			//gameLocal.Printf("\nQUEUED TRIGGERS: %i", triggerQueue);
		}
		n = spawnArgs.GetString("anim");
		int maxMonsters = spawnArgs.GetInt("max_monsters", "1");
		if(maxMonsters < 0 || aiSpawnCount < maxMonsters) {
			Event_CreateAI();
		} else {
			//gameLocal.Printf("\nMax monsters reached.");
		}
	}

	anim = GetAnimator()->GetAnim( n );
	HH_ASSERT( anim );
}

/*
================
hhAISpawnCase::Save
================
*/
void hhAISpawnCase::Save( idSaveGame *savefile ) const {
	encasedAI.Save( savefile );
	savefile->WriteBool( triggerToggle );
	savefile->WriteInt( aiSpawnCount );
	savefile->WriteBool( waitingForAutoTrigger );
	savefile->WriteInt( triggerQueue );
}

/*
================
hhAISpawnCase::Restore
================
*/
void hhAISpawnCase::Restore( idRestoreGame *savefile ) {
	encasedAI.Restore( savefile );
	savefile->ReadBool( triggerToggle );
	savefile->ReadInt( aiSpawnCount );
	savefile->ReadBool( waitingForAutoTrigger );
	savefile->ReadInt( triggerQueue );

	CreateEntSpawnArgs();
}

/*
================
hhAISpawnCase::CreateEntSpawnArgs
================
*/
void hhAISpawnCase::CreateEntSpawnArgs( void ) {
	// Create list of spawn args, copied to encasedAI in Event_CreateAI
	entSpawnArgs.Clear();
	idStr tmpStr, realKeyName;
	const idKeyValue *kv = spawnArgs.MatchPrefix("ent_", NULL);
	while(kv) {
		tmpStr = kv->GetKey();
		int usIndex = tmpStr.FindChar("ent_", '_');
		realKeyName = tmpStr.Mid(usIndex+1, strlen( kv->GetKey())-usIndex-1);
		entSpawnArgs.Set(realKeyName, kv->GetValue());
		kv = spawnArgs.MatchPrefix("ent_", kv);
	}
}
