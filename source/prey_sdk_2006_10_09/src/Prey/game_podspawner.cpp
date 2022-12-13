#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

//==========================================================================
//
//	hhPodSpawner
//
// type of mine spawner that animates and spawns pods
//==========================================================================

const idEventDef EV_DropPod("<DropPod>", NULL);
const idEventDef EV_DoneSpawning("<DoneSpawning>", NULL);

CLASS_DECLARATION(hhMineSpawner, hhPodSpawner)
	EVENT( EV_DropPod,					hhPodSpawner::Event_DropPod)
	EVENT( EV_PlayIdle,					hhPodSpawner::Event_PlayIdle)
	EVENT( EV_DoneSpawning,				hhPodSpawner::Event_DoneSpawning)
END_CLASS


void hhPodSpawner::Spawn(void) {
	fl.takedamage = false;
	GetPhysics()->SetContents( CONTENTS_BODY );

	pod = NULL;
	spawning = false;
	idleAnim = GetAnimator()->GetAnim("idle");
	painAnim = GetAnimator()->GetAnim("pain");
	spawnAnim = GetAnimator()->GetAnim("spawn");

	PostEventMS(&EV_PlayIdle, 0);
}

void hhPodSpawner::Save(idSaveGame *savefile) const {
	savefile->WriteInt( idleAnim );
	savefile->WriteInt( painAnim );
	savefile->WriteInt( spawnAnim );
	savefile->WriteBool( spawning );
	savefile->WriteObject(pod);
}

void hhPodSpawner::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( idleAnim );
	savefile->ReadInt( painAnim );
	savefile->ReadInt( spawnAnim );
	savefile->ReadBool( spawning );
	savefile->ReadObject( reinterpret_cast<idClass *&>(pod) );
}

void hhPodSpawner::SpawnMine() {
	idVec3		offset;

	if (spawning || health < 0 || fl.isDormant) {
		return;
	}

	population++;
	spawning = true;

	offset = idVec3(0, 0, -64);		// Move up to top

	idDict args;
	args.Clear();
	args.Set( "origin", (GetPhysics()->GetOrigin() + offset).ToString() );
	args.Set( "nodrop", "1" );		// Don't put on the floor, wait for release
	args.Set( "deformType", "0" );	// Don't start deforming until released

	// pass along any spawn keys
	const idKeyValue *arg = spawnArgs.MatchPrefix("spawn_");
	while( arg ) {
		args.Set( arg->GetKey().Right( arg->GetKey().Length() - 6 ), arg->GetValue() );
		arg = spawnArgs.MatchPrefix( "spawn_", arg );
	}

	// spawn a pod
	pod = static_cast<hhPod*>(gameLocal.SpawnObject(spawnArgs.GetString("def_pod"), &args));

	// attach to bone
	const char *podBone = "PodPosition";
	pod->MoveToJoint(this, podBone);
	pod->AlignToJoint(this, podBone);
	pod->BindToJoint(this, podBone, true);
	pod->fl.takedamage = false;	// No damage while attached
	pod->SetSpawner(this);

	// Due to the animation, bone isn't in position yet, so wait a little
	// before making it visible
	pod->Hide();
	pod->PostEventMS(&EV_Show, 500);

	// start animation
	if (spawnAnim) {
		GetAnimator()->ClearAllAnims(gameLocal.time, 0);
		GetAnimator()->PlayAnim(ANIMCHANNEL_ALL, spawnAnim, gameLocal.time, 500);
		int opentime = GetAnimator()->GetAnim( spawnAnim )->Length();
		PostEventMS( &EV_PlayIdle, opentime );
		
		//TODO: Move this to a frame command (event)
		PostEventMS( &EV_DropPod, 3700 );

		PostEventMS( &EV_DoneSpawning, opentime );

		StartSound( "snd_spawn", SND_CHANNEL_ANY );
	}
}

void hhPodSpawner::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	// Don't actually take damage, but give feedback

	// Play pain
	if (painAnim) {
		GetAnimator()->ClearAllAnims(gameLocal.time, 0);
		GetAnimator()->PlayAnim(ANIMCHANNEL_ALL, painAnim, gameLocal.time, 500);
		int opentime = GetAnimator()->GetAnim( painAnim )->Length();
		PostEventMS( &EV_PlayIdle, opentime );
		StartSound( "snd_pain", SND_CHANNEL_ANY );
	}
}

void hhPodSpawner::Event_PlayIdle() {
	if (idleAnim) {
		GetAnimator()->ClearAllAnims(gameLocal.time, 0);
		GetAnimator()->CycleAnim(ANIMCHANNEL_ALL, idleAnim, gameLocal.time, 0);
	}
}

void hhPodSpawner::Event_DropPod() {
	if (pod) {
		pod->Unbind();
		pod->fl.takedamage = true;
		pod->Release();
	}
}

void hhPodSpawner::Event_DoneSpawning() {
	spawning = false;
	CheckPopulation();
}

