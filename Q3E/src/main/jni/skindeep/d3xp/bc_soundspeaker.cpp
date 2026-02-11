#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
//#include "Light.h"
//#include "Player.h"
#include "Fx.h"
//#include "Moveable.h"
//#include "trigger.h"

#include "bc_soundspeaker.h"

const int DEATHTIME = 1500;

CLASS_DECLARATION(idEntity, idSoundspeaker)

END_CLASS


idSoundspeaker::idSoundspeaker(void)
{
}

idSoundspeaker::~idSoundspeaker(void)
{
	physicsObj.PostEventMS(&EV_Remove, 0);
	soundwaves->PostEventMS(&EV_Remove, 0);
	soundwaves = nullptr;
}

void idSoundspeaker::Spawn(void)
{
	idDict args;
	idVec3 forwardDir;
	idAngles smokeAng = GetPhysics()->GetAxis().ToAngles();
	smokeAng.pitch += 130;

	args.Clear();
	args.Set("model", spawnArgs.GetString("model_musicparticle"));
	//args.Set("start_off", "1");
	soundwaves = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, NULL, NULL);
	soundwaves->SetOrigin(GetPhysics()->GetOrigin() + (forwardDir * 8));
	soundwaves->SetAngles(smokeAng);
	soundwaves->Bind(this, true);
	//soundwaves->SetActive(true);

	fl.takedamage = true;
	BecomeActive(TH_THINK);
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);

	StartSound("snd_ambient", SND_CHANNEL_AMBIENT, 0, false, NULL);	
}

void idSoundspeaker::Save(idSaveGame *savefile) const
{
	savefile->WriteStaticObject( idSoundspeaker::physicsObj ); // idPhysics_RigidBody physicsObj
	bool restorePhysics = &physicsObj == GetPhysics();
	savefile->WriteBool( restorePhysics );

	savefile->WriteObject( soundwaves ); // idFuncEmitter * soundwaves
	savefile->WriteInt( deathtimer ); // int deathtimer
}

void idSoundspeaker::Restore(idRestoreGame *savefile)
{
	savefile->ReadStaticObject( physicsObj ); // idPhysics_RigidBody physicsObj
	bool restorePhys;
	savefile->ReadBool( restorePhys );
	if (restorePhys)
	{
		RestorePhysics( &physicsObj );
	}

	savefile->ReadObject( CastClassPtrRef(soundwaves) ); // idFuncEmitter * soundwaves
	savefile->ReadInt( deathtimer ); // int deathtimer
}

void idSoundspeaker::Think(void)
{
	//if (health > 0)
	//{
	//	//Play particle effect if a sound is playing.
	//	bool isPlayingSound = false;
	//	if (this->refSound.referenceSound != NULL)
	//	{
	//		if (this->refSound.referenceSound->CurrentlyPlaying())
	//		{
	//			isPlayingSound = true;
	//		}
	//	}
	//
	//	if (isPlayingSound)
	//	{
	//		if (!soundwaves->GetParticleActive())
	//			soundwaves->SetActive(true);
	//	}
	//	else
	//	{
	//		if (soundwaves->GetParticleActive())
	//			soundwaves->SetActive(false);
	//	}
	//}

	if (!fl.takedamage && gameLocal.time > deathtimer)
	{
		//Make it explode.
		Hide();
		StopSound(SND_CHANNEL_ANY, false);
		idEntityFx::StartFx("fx/explosion_gascylinder", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
		gameLocal.SpawnInterestPoint(this, GetPhysics()->GetOrigin(), spawnArgs.GetString("def_interestpoint"));
		PostEventMS(&EV_Remove, 0);
	}


	RunPhysics();
	Present();
}



void idSoundspeaker::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	idTraceModel			trm;

	if (!fl.takedamage)
		return;

	idEntityFx::StartFx("fx/explosion_spearbot", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
	
	soundwaves->SetModel(spawnArgs.GetString("model_brokenparticle"));

	StopSound(SND_CHANNEL_AMBIENT, false);
	fl.takedamage = false;

	StartSound("snd_break", SND_CHANNEL_BODY, 0, false, NULL);
	
	deathtimer = gameLocal.time + DEATHTIME;

	if (!collisionModelManager->TrmFromModel(spawnArgs.GetString("clipmodel"), trm))
	{
		gameLocal.Error("soundspeaker '%s': cannot load collision model %s", name.c_str(), spawnArgs.GetString("clipmodel"));
		return;
	}

	idVec3 forwardDir;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, NULL, NULL);

	//Turn it into a physics object and let it drop to ground.
	physicsObj.SetSelf(this);
	physicsObj.SetClipModel(new idClipModel(trm), 0.02f);
	physicsObj.SetOrigin(GetPhysics()->GetOrigin());
	physicsObj.SetAxis(GetPhysics()->GetAxis());
	physicsObj.SetBouncyness(0.2f);
	physicsObj.SetFriction(0.6f, 0.6f, 0.2f);
	physicsObj.SetGravity(gameLocal.GetGravity());
	physicsObj.SetContents(CONTENTS_RENDERMODEL);
	physicsObj.SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL );
	SetPhysics(&physicsObj);


}

