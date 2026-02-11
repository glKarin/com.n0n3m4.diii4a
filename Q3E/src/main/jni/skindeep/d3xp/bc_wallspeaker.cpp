#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
//#include "Light.h"
//#include "Player.h"
#include "Fx.h"
//#include "Moveable.h"
//#include "trigger.h"

#include "bc_wallspeaker.h"

const int DEATHTIME = 2000;

CLASS_DECLARATION(idEntity, idWallspeaker)

END_CLASS


idWallspeaker::idWallspeaker(void)
{
	//physicsObj = idPhysics_RigidBody();
	soundwaves = {};
	deathtimer = {};
}

idWallspeaker::~idWallspeaker(void)
{
	physicsObj.PostEventMS(&EV_Remove, 0);
	if ( soundwaves ) // check, since might not exist on savegame load failure
	{
		soundwaves->PostEventMS(&EV_Remove, 0);
		soundwaves = nullptr;
	}
}

void idWallspeaker::Spawn(void)
{
	idDict args;
	idVec3 forwardDir;
	idAngles smokeAng = GetPhysics()->GetAxis().ToAngles();
	smokeAng.pitch += 90;

	args.Clear();
	args.Set("model", "sound_waves.prt");
	args.Set("start_off", "1");
	soundwaves = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, NULL, NULL);
	soundwaves->SetOrigin(GetPhysics()->GetOrigin() + (forwardDir * 8));
	soundwaves->SetAngles(smokeAng);
	soundwaves->Bind(this, true);
	

	fl.takedamage = true;
	BecomeActive(TH_THINK);
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);
}

void idWallspeaker::Save(idSaveGame *savefile) const
{
	savefile->WriteStaticObject( idWallspeaker::physicsObj ); //  idPhysics_RigidBody physicsObj
	bool restorePhysics = &physicsObj == GetPhysics();
	savefile->WriteBool( restorePhysics );

	assert( soundwaves );
	savefile->WriteObject(soundwaves); //  idFuncEmitter *soundwaves;
	savefile->WriteInt( deathtimer ); //  int deathtimer
}

void idWallspeaker::Restore(idRestoreGame *savefile)
{
	savefile->ReadStaticObject(physicsObj); //  idPhysics_RigidBody physicsObj
	bool restorePhys;
	savefile->ReadBool( restorePhys );
	if (restorePhys)
	{
		RestorePhysics( &physicsObj );
	}

	savefile->ReadObject(reinterpret_cast<idClass*&>(soundwaves)); //  idFuncEmitter *soundwaves;
	assert( soundwaves );
	savefile->ReadInt( deathtimer ); //  int deathtimer
}

void idWallspeaker::Think(void)
{
	if (health > 0)
	{
		//Play particle effect if a sound is playing.
		bool isPlayingSound = false;
		if (this->refSound.referenceSound != NULL)
		{
			if (this->refSound.referenceSound->CurrentlyPlaying())
			{
				isPlayingSound = true;
			}
		}

		if (isPlayingSound)
		{
			if (!soundwaves->GetParticleActive())
				soundwaves->SetActive(true);
		}
		else
		{
			if (soundwaves->GetParticleActive())
				soundwaves->SetActive(false);
		}
	}

	if (!fl.takedamage && gameLocal.time > deathtimer)
	{
		

		Hide();
		GetPhysics()->SetContents(0);
		StopSound(SND_CHANNEL_ANY, false);
		idEntityFx::StartFx("fx/explosion_gascylinder", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
		gameLocal.SpawnInterestPoint(this, GetPhysics()->GetOrigin(), spawnArgs.GetString("def_interestpoint"));
		PostEventMS(&EV_Remove, 0);

		//Make it explode.
		gameLocal.RadiusDamage(GetPhysics()->GetOrigin(), this, this, this, this, spawnArgs.GetString("def_explosiondamage"));
	}


	RunPhysics();
	Present();
}



void idWallspeaker::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	idTraceModel			trm;

	if (!fl.takedamage)
		return;

	idEntityFx::StartFx("fx/explosion_spearbot", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
	
	if (!soundwaves->GetParticleActive())
		soundwaves->SetActive(true);

	StopSound(SND_CHANNEL_VOICE, false);
	fl.takedamage = false;

	StartSound("snd_break", SND_CHANNEL_BODY, 0, false, NULL);
	
	deathtimer = gameLocal.time + DEATHTIME;

	if (!collisionModelManager->TrmFromModel(spawnArgs.GetString("clipmodel"), trm))
	{
		gameLocal.Error("wallspeaker '%s': cannot load collision model %s", name.c_str(), spawnArgs.GetString("clipmodel"));
		return;
	}

	idEntity *garbleInterestpoint = gameLocal.SpawnInterestPoint(this, GetPhysics()->GetOrigin(), spawnArgs.GetString("def_interest_garble"));
	if (garbleInterestpoint)
	{
		garbleInterestpoint->Bind(this, false);
	}
	

	//Turnt it into a physics object and let it drop to ground.
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

void idWallspeaker::ActivateSpeaker(const char *soundshaderName, const s_channelType soundchannel)
{
	if (health <= 0)
		return;

	//TODO: queue up sounds if one is already playing.
	//if (this->refSound.referenceSound->CurrentlyPlaying())
	//	return; //Queue it up

	StartSound(soundshaderName, soundchannel, 0, false, NULL);
}