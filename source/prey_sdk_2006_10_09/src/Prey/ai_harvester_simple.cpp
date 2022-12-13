#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef MA_AntiProjectileAttack("<antiProjectileAttack>", "e");
const idEventDef MA_StartPreDeath("<startPreDeath>" );
const idEventDef MA_HandlePassageway("<handlePassageway>", NULL);
const idEventDef MA_EnterPassageway("<enterPassageway>", "e",NULL);
const idEventDef MA_ExitPassageway("<exitPassageway>", "e",NULL);
const idEventDef MA_UseThisPassageway("useThisPassageway", "ed",NULL);
const idEventDef MA_GibOnDeath("gibOnDeath", "d");

CLASS_DECLARATION( hhMonsterAI, hhHarvesterSimple )
	EVENT( MA_OnProjectileLaunch,		hhHarvesterSimple::Event_OnProjectileLaunch )
	EVENT( MA_AntiProjectileAttack,		hhHarvesterSimple::Event_AntiProjectileAttack )
	EVENT( MA_StartPreDeath,			hhHarvesterSimple::Event_StartPreDeath )
	EVENT( MA_HandlePassageway,			hhHarvesterSimple::Event_HandlePassageway )
	EVENT( MA_EnterPassageway,			hhHarvesterSimple::Event_EnterPassageway )
	EVENT( MA_ExitPassageway,			hhHarvesterSimple::Event_ExitPassageway )	
	EVENT( MA_UseThisPassageway,		hhHarvesterSimple::Event_UseThisPassageway )	
	EVENT( EV_Broadcast_AppendFxToList,	hhHarvesterSimple::Event_AppendFxToList )
	EVENT( MA_GibOnDeath,				hhHarvesterSimple::Event_GibOnDeath )
END_CLASS

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
//
// Spawn()
//
void hhHarvesterSimple::Spawn(void) {
	allowPreDeath = true;
	lastAntiProjectileAttack = 0;
	lastPassagewayTime = 0;
	passageCount = spawnArgs.GetInt("passage_count", "0"); // Used for passing to and from torso
	bSmokes = spawnArgs.GetBool("smokes", "0");
	bGibOnDeath = false;
}

//
// Event_OnProjectileLaunch()
//
void hhHarvesterSimple::Event_OnProjectileLaunch(hhProjectile *proj) {
	
	// Can't launch again yet
	float min = spawnArgs.GetFloat( "dda_delay_min" );
	float max = spawnArgs.GetFloat( "dda_delay_max" );
	float delay = min + (max - min) * (1.0f - gameLocal.GetDDAValue());

	if(gameLocal.GetTime() - lastAntiProjectileAttack < delay)
		return;	
	
	// The person who launched this projectile wasn't someone to worry about
	if(proj->GetOwner() && !(ReactionTo(proj->GetOwner()) & (ATTACK_ON_SIGHT | ATTACK_ON_DAMAGE)))
		return;
	
	// TODO: more intelligent checks for if we should launch the anti-projectile chaff
	idVec3 fw		= viewAxis[0];
	idVec3 projFw	= proj->GetAxis()[0]; 
	if(proj->GetOwner())
		projFw = proj->GetOwner()->GetAxis()[0];
	float dot = fw * projFw;
	if(dot > -.7f)
		return;

	ProcessEvent(&MA_AntiProjectileAttack, proj);
}


//
// Event_AntiProjectileAttack()
//
void hhHarvesterSimple::Event_AntiProjectileAttack(hhProjectile *proj) {

	if(!spawnArgs.GetBool("block_projectiles", "1")) {
		return;
	}

	const idDict *projectileDef = gameLocal.FindEntityDefDict( spawnArgs.GetString("def_chaff") );
	if ( !projectileDef ) {
		gameLocal.Error( "Unknown def_chaff:  %s\n", spawnArgs.GetString("def_chaff") );
	}
	hhProjectile* projectile = NULL;
	
	//bjk: new parms
	int numProj = spawnArgs.GetInt("shieldNumProj", "10");
	float spread = DEG2RAD( spawnArgs.GetFloat("shieldSpread", "10") );
	float yaw = DEG2RAD( spawnArgs.GetFloat("shieldYawSpread", "10") );

	//StartSound("snd_fire_chaff", SND_CHANNEL_ANY, 0, true, NULL);
	torsoAnim.PlayAnim( GetAnimator()->GetAnim( "antiprojectile_attack" ) );

	idVec3 dir = proj->GetOrigin() - GetOrigin();
	dir.Normalize();
	dir = dir+idVec3(0.f, 0.f, -.3f);	// bjk: bias towards ground
	dir.Normalize();
	idMat3 muzzleAxis = dir.ToMat3();
	// todo: read this in from a bone
	idVec3 launchPos = GetOrigin() + idVec3(0.0f, 0.0f, 64.0f);

	for(int i=0; i<numProj; i++) {
		projectile = hhProjectile::SpawnProjectile( projectileDef );
		HH_ASSERT( projectile );

		//bjk: uses random now
		float ang = spread * hhMath::Sqrt( gameLocal.random.RandomFloat() );
		float spin = hhMath::TWO_PI * gameLocal.random.RandomFloat();
		dir = muzzleAxis[ 0 ] + muzzleAxis[ 2 ] * ( hhMath::Sin(ang) * hhMath::Sin(spin) ) - muzzleAxis[ 1 ] * ( hhMath::Sin(ang+yaw) * hhMath::Cos(spin) );
		dir.Normalize();

		projectile->Create(this, launchPos, dir);
		projectile->Launch(launchPos, dir, idVec3(0.0f, 0.0f, 0.0f));
	}

	lastAntiProjectileAttack = gameLocal.GetTime();
}

bool hhHarvesterSimple::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	int preDeathThresh = spawnArgs.GetInt("predeath_thresh", "-1");	
	if(allowPreDeath && !bGibOnDeath && preDeathThresh > 0 && health < preDeathThresh) {
		allowPreDeath = false;	// We've tried, cannot try anymore
		float preDeathFreq = spawnArgs.GetFloat("predeath_freq", "1");
		if(gameLocal.random.RandomFloat() < preDeathFreq) {
            ProcessEvent(&MA_StartPreDeath);
		}
	}

	if ( bSmokes ) {
		if ( !fxSmoke[0].IsValid() && health <= AI_PASSAGEWAY_HEALTH ) {
			SpawnSmoke();
		} else if (fxSmoke[0].IsValid() && health > AI_PASSAGEWAY_HEALTH)  {
			ClearSmoke();
		}
	}
	
	return( idAI::Pain(inflictor, attacker, damage, dir, location) );
}

void hhHarvesterSimple::Event_StartPreDeath(void) {
	int i;

	// No torso? Just kill myself .....
	if(!spawnArgs.GetString("def_torso")) {
		ProcessEvent(&AI_Kill);
		return;
	}    	
	
	// Spawn death fx, if any
	const char *fx = spawnArgs.GetString("fx_death");
	if (fx && fx[0]) {
		hhFxInfo fxInfo;

		fxInfo.RemoveWhenDone(true);
		BroadcastFxInfo(fx, GetOrigin(), GetAxis(), &fxInfo);
	}

	// do blood splats
	float size = spawnArgs.GetFloat("decal_torso_pop_size","96");
	
	// copy my target keys to the keys that will spawn the torso
	idDict dict;
	const idDict *torsoDict = gameLocal.FindEntityDefDict(spawnArgs.GetString("def_torso"));
	if ( !torsoDict ) {
		gameLocal.Error("Unknown def_torso:  %s\n", spawnArgs.GetString("def_torso"));
	}
	dict.Copy(*torsoDict);
	for(i=0;i<targets.Num();i++)
	{
		if(!targets[i].GetEntity()) {
			continue;
		}

		idStr key("target");
		if(i > 0) {
			sprintf(key, "target%i", i);
		}

		dict.Set((const char*)key, (const char*)targets[i].GetEntity()->name);
	}	

	// Passageway variables
	dict.SetInt("passage_health", AI_PASSAGEWAY_HEALTH);
	dict.SetInt("passage_count", passageCount);
	dict.SetVector("origin", GetOrigin());
	dict.SetMatrix("rotation", GetAxis());
	
	// remove my target keys (so they don't get fired)
	targets.Clear();

	// Spawn the torso
	idEntity *e;
	hhMonsterAI *aiTorso;
	if(!gameLocal.SpawnEntityDef(dict, &e))	
		gameLocal.Error("Failed to spawn ai torso.");
	HH_ASSERT(e && e->IsType(hhMonsterAI::Type));
	aiTorso = static_cast<hhMonsterAI*>(e);
	HH_ASSERT(aiTorso != NULL);

	// Throw gibs	
	idStr debrisDef = spawnArgs.GetString("def_gibDebrisSpawnerPreDeath");
	if(debrisDef.Length() > 0) {
		hhUtils::SpawnDebrisMass(debrisDef.c_str(), this, 1.0f);
	}	

	health = 0;

	// remove myself
	Hide();
	PostEventMS(&EV_Remove, 3000);
}

void hhHarvesterSimple::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	if ( attacker == this ) {
		return;
	}
	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( !AI_DEAD ) {
		if ( damageDef && damageDef->GetBool( "ice" ) && spawnArgs.GetBool( "can_freeze", "0" ) ) {
			spawnArgs.Set( "bAlwaysGib", "0" );
			allowPreDeath = false;
		} else {
			allowPreDeath = true;
		}
		if ( damageDef && damageDef->GetBool( "no_special_death" ) ) {
			allowPreDeath = false;
		}
	}
	hhMonsterAI::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
}

int hhHarvesterSimple::EvaluateReaction( const hhReaction *react ) {
	// Let the script limit the distance of Climb reactions we consider
	if ( react->desc->effect == hhReactionDesc::Effect_Climb && AI_CLIMB_RANGE >= 0.0f && ( react->causeEntity->GetOrigin() - GetOrigin()).LengthSqr() > AI_CLIMB_RANGE ) {
		return 0;
	}

	// If we're supposed to use a specific passageway, only accept it and reject the others
	if ( nextPassageway.IsValid() && react->desc->effect == hhReactionDesc::Effect_Passageway ) {
		if (react->causeEntity.GetEntity() == nextPassageway.GetEntity()) {
			return 100;
		} else {
			return 0;
		}
	}

	int rank = hhMonsterAI::EvaluateReaction( react );
	if ( react->desc->effect == hhReactionDesc::Effect_Passageway &&
		 react->causeEntity.GetEntity() == lastPassageway.GetEntity() ) {
		if ( lastPassagewayTime + 10000 > gameLocal.time ) {
			if (rank > 10) {
				// Discourage entering the same passageway we just left
				return 10;
			}
		} else {
			// Ignore previous passageway for 10 seconds
			return 0;
		}
	}
	return rank;
}

#define CheckPassageFreqInitial 6000
void hhHarvesterSimple::Event_EnterPassageway(hhAIPassageway *pn) {
	CancelEvents( &MA_OnProjectileLaunch ); // Clear any anti-projectile attacks
	CancelEvents( &MA_AntiProjectileAttack ); // Clear any anti-projectile attacks
	if (nextPassageway.IsValid()) {
		if (nextPassageway.GetEntity() != pn) {
			gameLocal.Warning("hhAIPassageway entered was not the specified one!\n");
		}
		nextPassageway.Clear();
	}

	if(currPassageway != NULL) {
		gameLocal.Error("%s tried to ENTER a passageway but already had a passageway!", (const char*)name);
	}

	HH_ASSERT(currPassageway == NULL);
	Hide();	// Should already be hidden but just to make sure

	// Only consider passageCount if damaged (ignores scripted passage sequences) or if it's the torso
	if (health < spawnHealth || !idStr::Icmp(spawnArgs.GetString("classname"), "monster_harvester_torso") ) {
		passageCount++;
		if (passageCount > 1) {
			//TODO make this affected by the DDA system
			AI_PASSAGEWAY_HEALTH = AI_PASSAGEWAY_HEALTH * 0.75f; // Reduce passageway seeking health level to 3/4 of it's original value
		}
	}

	GetAnimator()->ClearAllAnims(gameLocal.GetTime(), 0);
	Event_AnimState(ANIMCHANNEL_LEGS, "Legs_Idle", (int)0);			
	Event_AnimState(ANIMCHANNEL_TORSO, "Torso_Idle", (int)0);
	torsoAnim.UpdateState();
	legsAnim.UpdateState();

	StartSound("snd_inside_passage", SND_CHANNEL_VOICE, SSF_LOOPING, true, NULL);	

	currPassageway = pn;
	if(GetPhysics())
		GetPhysics()->SetContents(0);

	// Does this passage way give us health?
	int h = 0;
	pn->spawnArgs.GetInt("give_health", "0", h);
	health += h;
	health = idMath::ClampInt(0, spawnArgs.GetInt("health"), health);	

	
	// Should we transform to another monster when entering?
	idStr transTo;
	if(spawnArgs.GetString("passage_transform_to", "", transTo)) {			
		const idDict *def = gameLocal.FindEntityDefDict( transTo );
		if(!def) {
			gameLocal.Error("Unknown def : %s", (const char*)transTo);			
		}

		// copy my target keys to the keys of the transformed-to entity
		idDict dict;		
		dict.Copy(*def);
		for(int i=0;i<targets.Num();i++)
		{
			if(!targets[i].GetEntity())
				continue;

			idStr key("target");
			if(i > 0)
				sprintf(key, "target%i", i);

			dict.Set((const char*)key, (const char*)targets[i].GetEntity()->name);
		}

		// Passageway variables
		dict.SetInt("passage_health", AI_PASSAGEWAY_HEALTH);
		dict.SetInt("passage_count", passageCount);

		idEntity *e = NULL;
		gameLocal.SpawnEntityDef(dict, &e);
		HH_ASSERT( e->IsType( hhHarvesterSimple::Type ) );
		hhHarvesterSimple *ai = static_cast<hhHarvesterSimple*>(e);
		ai->SetOrigin(GetOrigin());
		ai->SetAxis(GetAxis());
		ai->Event_EnterPassageway(pn);
		targets.Clear();
		PostEventSec(&EV_Remove, 0.1f);
		return;
	}
	
	
	PostEventMS(&MA_HandlePassageway, CheckPassageFreqInitial);
}

void hhHarvesterSimple::Event_ExitPassageway(hhAIPassageway *pn) {
	if(currPassageway == NULL) {
		gameLocal.Error("%s tried to EXIT a passageway but did NOT have a curr passageway!", (const char*)name);
	}

	HH_ASSERT(currPassageway != NULL);	

	// Store this passageway so we don't jump right back into it.
	lastPassageway = idEntityPtr<idEntity> (pn);
	lastPassagewayTime = gameLocal.time;

	if(GetPhysics())
		GetPhysics()->SetContents(CONTENTS_BODY);	
	currPassageway = NULL;	
	AI_ACTIVATED = true; // Sometimes this is NOT true?!?! 
	StopSound(SND_CHANNEL_VOICE, true);	

	// Teleport to new pos
	idAngles angs = pn->GetAxis().ToAngles();
	idVec3 pos = pn->GetExitPos();	
	Teleport( pos, angs, pn );
	Show();
	
	current_yaw = angs.yaw;
	ideal_yaw	= angs.yaw;
	turnVel		= 0.0f;	
	HH_ASSERT(FacingIdeal());

	// Exit anim to play?	
	idStr exitAnim = pn->spawnArgs.GetString("exit_anim");
	if(exitAnim.Length() && GetAnimator()->HasAnim(exitAnim)) {
		GetAnimator()->ClearAllAnims(gameLocal.GetTime(), 0);
		torsoAnim.UpdateState();
		legsAnim.UpdateState();
		torsoAnim.PlayAnim( GetAnimator()->GetAnim( exitAnim ) );
		legsAnim.PlayAnim( GetAnimator()->GetAnim( exitAnim ) );
	}
}

//
// Event_HandlePassageway()
//
#define MaxExits 32
#define CheckPassageFreq 3000
void hhHarvesterSimple::Event_HandlePassageway(void) {

	if(currPassageway == NULL) {
		gameLocal.Error("%s tried to handle a passageway but did NOT have a passageway!", (const char*)name);
	}

	HH_ASSERT(currPassageway != NULL);
	hhAIPassageway *exitNode = NULL;

	// CATCH 'went inside hole - never came out' bug
	if(currPassageway.GetEntity()->lastEntered == this && gameLocal.GetTime() - currPassageway.GetEntity()->timeLastEntered > 30000) {
		gameLocal.Warning("%s has been stuck in a passageway for >30 secs, killing it off.", (const char*)name);
		fl.takedamage = true;
		// Use hhMonsterAI::Damage because our Damage returns if we're the attacker
		hhMonsterAI::Damage(this, this, vec3_origin, "damage_gib", 1.0f, INVALID_JOINT);
		return;
	}

	// No other way out - gotta go out the way we came in
	if(currPassageway->targets.Num() <= 0) {
		exitNode = currPassageway.GetEntity();
	}
	// We can go out another way
	else {	
		hhAIPassageway *possibleExits[MaxExits];
		hhAIPassageway *tmp;
		int count = 0;

		bool isRandom;
		if (gameLocal.GetTime() - currPassageway.GetEntity()->timeLastEntered > SEC2MS(10)) {
			// Try to prevent us from being stuck after 10 seconds
			isRandom = true;
		} else {
			// Choose possibleExits[] randomly if we have no enemy or based on chance.  Otherwise choose based on line-of-sight to the player
			isRandom = !enemy.IsValid() ||  gameLocal.random.RandomInt(100) > 50;
		}
		trace_t tr;
		idVec3 toPos;
		if (!isRandom) {
			toPos = enemy->GetOrigin();
		}

		// We've tried once, now we can use our entrance as an exit
		if(gameLocal.GetTime() - currPassageway.GetEntity()->timeLastEntered > CheckPassageFreqInitial + 1000) {
			count = 1;
			possibleExits[0] = currPassageway.GetEntity(); // We can always go out the same way we came in
		}
		for(int i=0;i<currPassageway->targets.Num();i++) {
			if(currPassageway->targets[i] != NULL && currPassageway->targets[i].GetEntity()->IsType(hhAIPassageway::Type)) {
				if (isRandom) {
					possibleExits[count++] = static_cast<hhAIPassageway*>(currPassageway->targets[i].GetEntity());
				} else {
					tmp = static_cast<hhAIPassageway*>(currPassageway->targets[i].GetEntity());
					// Choose based on visibility to our enemy
					gameLocal.clip.TracePoint( tr, tmp->GetOrigin(), toPos, MASK_SHOT_BOUNDINGBOX, tmp );
					if ( tr.fraction >= 1.0f || ( gameLocal.GetTraceEntity( tr ) == enemy.GetEntity() ) ) {
						lastVisibleEnemyPos = toPos; // Can now see enemy
						possibleExits[count++] = tmp;
					}
				}
			}

			// We've got enough
			if(count >= MaxExits)
				break;
		}

		if(count == 0) {
			exitNode = currPassageway.GetEntity();
		} else if ( enemy.IsValid() && gameLocal.random.RandomInt(100) > 70 ) {
			// Find the one closest to the last known enemy pos
			if (!exitNode) {
				float bestDistance = -1.0f, distSq;
				for (int i = 0; i < count; i++) {
					distSq = ( possibleExits[i]->GetOrigin() - lastVisibleEnemyPos ).LengthSqr();
					if ( bestDistance == -1.0f || distSq < bestDistance ) {
						exitNode = possibleExits[i];
						bestDistance = distSq;
					}
				}
			}
		} else {
            // Pick a random one
			int randExit = gameLocal.random.RandomInt(count); // TODO: Randomness isn't very random! Fix!
			HH_ASSERT(randExit >= 0 && randExit < count);
			exitNode = possibleExits[randExit];
		}
	}
	
	if(!exitNode)
		exitNode = currPassageway.GetEntity();

	// The one we picked is disabled, so go out where we came in
	// TODO: make this time based, try a few more times before going back out
	if(!exitNode->IsPassagewayEnabled()) {
		exitNode = currPassageway.GetEntity();		
	}

	// Special case:  The passageway is never meant to be exited from, use it's specified exit point instead
	const char *altPassage = exitNode->spawnArgs.GetString("force_passageway", "");
	if (altPassage && altPassage[0]) {
		idEntity *node = gameLocal.FindEntity(altPassage);
		if (node && node->IsType(hhAIPassageway::Type)) {
			exitNode = reinterpret_cast<hhAIPassageway *> (node);
		}
	}

	// Make sure no one is blocking where we want to go out
	idBounds myBnds = GetPhysics()->GetBounds();
	myBnds.Expand(12.0f);
	myBnds.TranslateSelf(exitNode->GetExitPos());
	idEntity *ents[MAX_GENTITIES];	
	int count = gameLocal.clip.EntitiesTouchingBounds(myBnds, -1, ents, MAX_GENTITIES);	
	for(int i=0;i<count;i++) {
		// We'll have to try again later when the coast is clear
		if(ents[i] && ents[i] != this && !ents[i]->IsHidden() && (ents[i]->IsType(idActor::Type) || ents[i]->IsType(hhMoveable::Type))) {
			// Chunk moveables
			if ( ents[i]->IsType(idMoveable::Type) ) {
				ents[i]->Damage( this, this, vec3_origin, "damage_gib", 1.0f, INVALID_JOINT );
				ents[i]->SquishedByDoor(this);
				continue;
			}
			gameLocal.Warning("Passageway exit blocked.\n");
			PostEventMS(&MA_HandlePassageway, CheckPassageFreq);
			if(ai_debugBrain.GetInteger() != 0) {
				gameRenderWorld->DebugBounds(colorYellow, myBnds, vec3_origin, 2000);
			}
			return;
		}
	}

	// Else we can just exit now - its safe
	exitNode->ProcessEvent(&EV_Activate, this);
}

#define LinkScriptVariable( name )	name.LinkTo(scriptObject, #name)
void hhHarvesterSimple::LinkScriptVariables() {
	hhMonsterAI::LinkScriptVariables();
	LinkScriptVariable(AI_CLIMB_RANGE);
	LinkScriptVariable(AI_PASSAGEWAY_HEALTH);
}

void hhHarvesterSimple::Event_UseThisPassageway(hhAIPassageway *pn, bool force) {
	nextPassageway.Assign(pn);

	if (force) { // Snap to position
		Event_FindReaction("passageway");
		hhReaction *reaction = targetReaction.GetReaction();
		if (!reaction) {
			gameLocal.Warning("useThisPassageway could not find a valid reaction!\n");
		} else {
			idVec3 tp = GetTouchPos( reaction->causeEntity.GetEntity(), reaction->desc );
			SetOrigin(tp);
		}
	}
}

/*
=====================
hhHarvesterSimple::Save
=====================
*/
void hhHarvesterSimple::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( allowPreDeath );
	savefile->WriteInt( lastAntiProjectileAttack );
	savefile->WriteInt( passageCount );
	savefile->WriteBool( bGibOnDeath );
	savefile->WriteInt( lastPassagewayTime );

	lastPassageway.Save( savefile );
	nextPassageway.Save( savefile );
}

/*
=====================
hhHarvesterSimple::Restore
=====================
*/
void hhHarvesterSimple::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( allowPreDeath );
	savefile->ReadInt( lastAntiProjectileAttack );
	savefile->ReadInt( passageCount );
	savefile->ReadBool( bGibOnDeath );
	savefile->ReadInt( lastPassagewayTime );

	lastPassageway.Restore( savefile );
	nextPassageway.Restore( savefile );

	bSmokes = spawnArgs.GetBool("smokes", "0");
}

void hhHarvesterSimple::SpawnSmoke(void) {
	const char *bones[MAX_HARVESTER_LEGS];
	bones[0] = spawnArgs.GetString( "bone_smoke_front_left" );
	bones[1] = spawnArgs.GetString( "bone_smoke_front_right" );
	bones[2] = spawnArgs.GetString( "bone_smoke_rear_left" );
	bones[3] = spawnArgs.GetString( "bone_smoke_rear_right" );	

	const char *psystem = spawnArgs.GetString( "fx_smoke" );
	if ( psystem && *psystem ) {
		idVec3 bonePos;
		idMat3 boneAxis;

		for(int i = 0; i < MAX_HARVESTER_LEGS; i++) {		
			if( fxSmoke[i] != NULL ) {
				gameLocal.Warning("Harvester already had smoke particles for leg %i.\n", i);
				continue;
			}

			hhFxInfo fxInfo;

			this->GetJointWorldTransform( bones[i], bonePos, boneAxis );
			fxInfo.SetEntity( this );
			fxInfo.SetBindBone( bones[i] );
			fxInfo.Toggle();
			fxInfo.RemoveWhenDone( false );

			BroadcastFxInfo( psystem, bonePos, boneAxis, &fxInfo, &EV_Broadcast_AppendFxToList );				
		}
	}
}

void hhHarvesterSimple::ClearSmoke(void) {
	for(int i=0; i < MAX_HARVESTER_LEGS; i++) {
		if(fxSmoke[i].IsValid()) {
			fxSmoke[i]->Nozzle(false);
		}			
	}

	for(int i = 0; i < MAX_HARVESTER_LEGS; i++) {
		SAFE_REMOVE(fxSmoke[MAX_HARVESTER_LEGS]);
	}
}

void hhHarvesterSimple::Event_AppendFxToList(hhEntityFx *fx) {
	for(int i = 0; i < MAX_HARVESTER_LEGS; ++i) {
		if(fxSmoke[i] != NULL) {
			continue;
		}

		fxSmoke[i] = fx;
		return;
	}
}

void hhHarvesterSimple::Event_GibOnDeath(const idList<idStr>* parmList) {
	bGibOnDeath = (atoi((*parmList)[0].c_str()) != 0);
}

void hhHarvesterSimple::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location) {
	ClearSmoke();
	hhMonsterAI::Killed(inflictor, attacker, damage, dir, location);

	if (bGibOnDeath) {
		Gib(vec3_origin, "damage_gib");
	}
}

void hhHarvesterSimple::Event_Activate(idEntity *activator) {
	if (fl.hidden) {
		// If we're hidden, we're probably birthing out of passageway.  Figure out which one.
		hhAIPassageway *nearest = NULL;
		float dist, nearDist = idMath::INFINITY;
		hhAIPassageway *passage = reinterpret_cast<hhAIPassageway *> (gameLocal.FindEntityOfType( hhAIPassageway::Type, NULL ));
		while (passage) {
			dist = (passage->GetOrigin() - GetOrigin()).Length();
			if (dist < nearDist) {
				nearDist = dist;
				nearest = passage;
			}
			passage = reinterpret_cast<hhAIPassageway *> (gameLocal.FindEntityOfType( hhAIPassageway::Type, passage ));
		}

		// If we found one near us and it's near enough, mark it as our last passageway
		if (nearest && nearDist < 250.0f) {
			lastPassageway = nearest;
			lastPassagewayTime = gameLocal.time;
		}
	}
	hhMonsterAI::Event_Activate(activator);
}

void hhHarvesterSimple::AnimMove( void ) {
	idVec3				goalPos;
	idVec3				delta;
	idVec3				goalDelta;
	float				goalDist;
	monsterMoveResult_t	moveResult;
	idVec3				newDest;

	// Turn on physics to prevent flying harvesters
	BecomeActive(TH_PHYSICS);

	idVec3 oldorigin = physicsObj.GetOrigin();
#ifdef HUMANHEAD //jsh wallwalk
	idMat3 oldaxis = GetGravViewAxis();
#else
	idMat3 oldaxis = viewAxis;
#endif

	AI_BLOCKED = false;

	if ( move.moveCommand < NUM_NONMOVING_COMMANDS ){ 
		move.lastMoveOrigin.Zero();
		move.lastMoveTime = gameLocal.time;
	}

	move.obstacle = NULL;
	if ( ( move.moveCommand == MOVE_FACE_ENEMY ) && enemy.GetEntity() ) {
		TurnToward( lastVisibleEnemyPos );
		goalPos = oldorigin;
	} else if ( ( move.moveCommand == MOVE_FACE_ENTITY ) && move.goalEntity.GetEntity() ) {
		TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
		goalPos = oldorigin;
	} else if ( GetMovePos( goalPos ) ) {
		if ( move.moveCommand != MOVE_WANDER ) {
			CheckObstacleAvoidance( goalPos, newDest );
			TurnToward( newDest );
		} else {
			TurnToward( goalPos );
		}
	}
		
	Turn();	

	if ( move.moveCommand == MOVE_SLIDE_TO_POSITION ) {
		if ( gameLocal.time < move.startTime + move.duration ) {
			goalPos = move.moveDest - move.moveDir * MS2SEC( move.startTime + move.duration - gameLocal.time );
			delta = goalPos - oldorigin;
			delta.z = 0.0f;
		} else {
			delta = move.moveDest - oldorigin;
			delta.z = 0.0f;
			StopMove( MOVE_STATUS_DONE );
		}
	} else if ( allowMove ) {
#ifdef HUMANHEAD //jsh wallwalk
		GetMoveDelta( oldaxis, GetGravViewAxis(), delta );
#else
		GetMoveDelta( oldaxis, viewAxis, delta );
#endif
	} else {
		delta.Zero();
	}

	if ( move.moveCommand == MOVE_TO_POSITION ) {
		goalDelta = move.moveDest - oldorigin;
		goalDist = goalDelta.LengthFast();
		if ( goalDist < delta.LengthFast() ) {
			delta = goalDelta;
		}
	}
	
#ifdef HUMANHEAD //shrink functionality
	float scale = renderEntity.shaderParms[SHADERPARM_ANY_DEFORM_PARM1];
	if ( scale > 0.0f && scale < 2.0f ) {
		delta *= scale;
	}
#endif
	physicsObj.SetDelta( delta );
	physicsObj.ForceDeltaMove( disableGravity );

	RunPhysics();

	if ( ai_debugMove.GetBool() ) {
		// HUMANHEAD JRM - so we can see if grav is on or off
		if(disableGravity) {
			gameRenderWorld->DebugLine( colorRed, oldorigin, physicsObj.GetOrigin(), 5000 );
		} else {
			gameRenderWorld->DebugLine( colorCyan, oldorigin, physicsObj.GetOrigin(), 5000 );
		}
	}

	moveResult = physicsObj.GetMoveResult();
	if ( !af_push_moveables && attack.Length() && TestMelee() ) {
		DirectDamage( attack, enemy.GetEntity() );
	} else {
		idEntity *blockEnt = physicsObj.GetSlideMoveEntity();
		if ( blockEnt && blockEnt->IsType( hhHarvesterSimple::Type ) ) {
			StopMove( MOVE_STATUS_BLOCKED_BY_MONSTER );
			return;
		}
		if ( blockEnt && blockEnt->IsType( idMoveable::Type ) && blockEnt->GetPhysics()->IsPushable() ) {
			KickObstacles( viewAxis[ 0 ], kickForce, blockEnt );
		}
	}

	BlockedFailSafe();

	AI_ONGROUND = physicsObj.OnGround();

	idVec3 org = physicsObj.GetOrigin();
	if ( oldorigin != org ) {
		TouchTriggers();
	}

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), org, gameLocal.msec );
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), move.moveDest, gameLocal.msec );
		gameRenderWorld->DebugLine( colorYellow, org + EyeOffset(), org + EyeOffset() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, gameLocal.msec, true );
		DrawRoute();
	}
}

void hhHarvesterSimple::Event_CanBecomeSolid( void ) {
	int			i;
	int			num;
	idEntity *	hit;
	idClipModel *cm;
	idClipModel *clipModels[ MAX_GENTITIES ];

	num = gameLocal.clip.ClipModelsTouchingBounds( physicsObj.GetAbsBounds(), MASK_MONSTERSOLID, clipModels, MAX_GENTITIES );
	for ( i = 0; i < num; i++ ) {
		cm = clipModels[ i ];

		// don't check render entities
		if ( cm->IsRenderModel() ) {
			continue;
		}

		hit = cm->GetEntity();
		if ( hit == this ) {
			continue;
		}

		// Special case, crush any moveables blocking our start point
		if ( hit->IsType( idMoveable::Type ) ) {
			hit->Damage( this, this, vec3_origin, "damage_gib", 1.0f, INVALID_JOINT );
			hit->SquishedByDoor(this);
		}

		if ( !hit->fl.takedamage ) {
			continue;
		}

		if ( physicsObj.ClipContents( cm ) ) {
			idThread::ReturnFloat( false );
			return;
		}
	}

	idThread::ReturnFloat( true );
}

#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build