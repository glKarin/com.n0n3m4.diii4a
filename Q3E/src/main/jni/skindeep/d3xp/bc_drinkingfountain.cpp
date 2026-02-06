#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"


#include "bc_drinkingfountain.h"

#define SINK_WATERTIME 2000 //how long drinking fountain is on

#define SLURP_DOTPRODUCT .95f

#define SLURP_DISTANCE 50

const idVec3 IDLECOLOR = idVec3(0, 1, 0);
const idVec3 FROBCOLOR = idVec3(1, .9f, 0);

CLASS_DECLARATION(idStaticEntity, idDrinkingFountain)
END_CLASS

idDrinkingFountain::idDrinkingFountain(void)
{
	slurpTimer = 0;
	isSlurping = false;
	sinkIsOn = false;
	sinkTimer = 0;

	//BC 3-28-2025: possible fix with tutorial generator room reset crash.
	faucetEmitter = NULL;
	drinkEmitter = NULL;
}

idDrinkingFountain::~idDrinkingFountain(void)
{
	if (faucetEmitter != NULL)
		delete faucetEmitter;

	if (drinkEmitter != nullptr)
		delete drinkEmitter;
}

void idDrinkingFountain::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
	isFrobbable = true;

	BecomeInactive(TH_THINK);
	fl.takedamage = true;	

	//Spawn the faucet particle.
	idAngles particleAngle = GetPhysics()->GetAxis().ToAngles();
	particleAngle.pitch += 90 - 30;
	particleAngle.yaw += 100;
	idVec3 faucetEmitterPos = GetFaucetPos();
	idDict args;
	args.Clear();
	args.Set("model", spawnArgs.GetString("model_water", spawnArgs.GetString("model_waterparticle")));
	args.SetVector("origin", faucetEmitterPos);
	args.SetMatrix("rotation", particleAngle.ToMat3());
	args.SetBool("start_off", true);
	faucetEmitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));



	args.Clear();
	args.Set("model", spawnArgs.GetString("model_drinkparticle"));
	args.SetVector("origin", GetDrinkPos());
	args.SetBool("start_off", true);
	drinkEmitter = static_cast<idFuncEmitter*>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));



	SetColor(IDLECOLOR);
}

idVec3 idDrinkingFountain::GetFaucetPos()
{
	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	return GetPhysics()->GetOrigin() + (forward * 7.5f) + (right * 7.5f) + (up * -4.5f);
}

void idDrinkingFountain::Save(idSaveGame *savefile) const
{
	savefile->WriteObject( faucetEmitter ); //  idFuncEmitter			* faucetEmitter

	savefile->WriteBool( sinkIsOn ); //  bool sinkIsOn
	savefile->WriteInt( sinkTimer ); //  int sinkTimer

	savefile->WriteInt( slurpTimer ); //  int slurpTimer
	savefile->WriteBool( isSlurping ); //  bool isSlurping

	savefile->WriteObject( drinkEmitter ); //  idFuncEmitter*			 drinkEmitter
}

void idDrinkingFountain::Restore(idRestoreGame *savefile)
{
	savefile->ReadObject( CastClassPtrRef(faucetEmitter) ); //  idFuncEmitter			* faucetEmitter

	savefile->ReadBool( sinkIsOn ); //  bool sinkIsOn
	savefile->ReadInt( sinkTimer ); //  int sinkTimer

	savefile->ReadInt( slurpTimer ); //  int slurpTimer
	savefile->ReadBool( isSlurping ); //  bool isSlurping

	savefile->ReadObject( CastClassPtrRef(drinkEmitter) ); //  idFuncEmitter*			 drinkEmitter
}

idVec3 idDrinkingFountain::GetDrinkPos()
{
	idVec3 forward, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
	return GetPhysics()->GetOrigin() + (forward * 6) + (up * -4);
}

void idDrinkingFountain::Think(void)
{
	idStaticEntity::Think();

	if (sinkIsOn && gameLocal.time > slurpTimer)
	{
		slurpTimer = gameLocal.time + 300;

		//check if player is looking at the fountain.
		//idVec3 forward, up;
		//GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
		idVec3 drinkPos = GetDrinkPos();

		idVec3 dirToTarget = drinkPos - gameLocal.GetLocalPlayer()->firstPersonViewOrigin;
		dirToTarget.Normalize();
		float vdot = DotProduct(dirToTarget, gameLocal.GetLocalPlayer()->viewAngles.ToForward());
		
		float distance = (gameLocal.GetLocalPlayer()->firstPersonViewOrigin - drinkPos).Length();

		if (vdot > SLURP_DOTPRODUCT && distance <= SLURP_DISTANCE)
		{
			if (!isSlurping)
			{
				StartSound("snd_slurp", SND_CHANNEL_RADIO);
				isSlurping = true;
				drinkEmitter->SetActive(true);
			}

			if (gameLocal.GetLocalPlayer()->health < gameLocal.GetLocalPlayer()->maxHealth)
			{
				gameLocal.GetLocalPlayer()->GiveHealthGranular(1);
			}


		}
		else
		{
			if (isSlurping)
			{
				StopSound(SND_CHANNEL_RADIO);
				isSlurping = false;
				drinkEmitter->SetActive(false);
			}
		}
	}

	//make particle fly to player mouth.
	if (drinkEmitter != nullptr && isSlurping && sinkIsOn)
	{
		idAngles dirToMouth = ((gameLocal.GetLocalPlayer()->firstPersonViewOrigin + idVec3(0, 0, -4)) - GetDrinkPos()).ToAngles();
		dirToMouth.pitch += 90;
		drinkEmitter->SetAxis(dirToMouth.ToMat3());
	}

	if (!sinkIsOn && isSlurping)
	{
		isSlurping = false;
		StopSound(SND_CHANNEL_RADIO);
		drinkEmitter->SetActive(false);
	}

	if (sinkIsOn && gameLocal.time > sinkTimer)
	{
		//timer done. turn sink off.
		sinkIsOn = false;
		isFrobbable = true;
		BecomeInactive(TH_THINK);
		SetColor(IDLECOLOR);
	}
}

void idDrinkingFountain::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	if (!fl.takedamage)
		return;

	gameLocal.AddEventlogDeath(this, 0, inflictor, attacker, "", EL_DESTROYED);

	fl.takedamage = false;
	StopSound(SND_CHANNEL_ANY, false);

	drinkEmitter->SetActive(false);
	faucetEmitter->SetActive(false);
	isFrobbable = false;
	sinkIsOn = false;
	SetColor(0, 0, 0); //dead light
	
	//Spew water.
	idEntityFx::StartFx(spawnArgs.GetString("fx_waterdamage"), GetFaucetPos() + idVec3(0,0,-4), mat3_identity);

	idMoveableItem::DropItemsBurst(this, "gib", idVec3(0, 0, 32));

	SetModel(spawnArgs.GetString("model_broken"));
	StartSound("snd_break", SND_CHANNEL_ANY, 0, false, NULL);
}

bool idDrinkingFountain::DoFrob(int index, idEntity * frobber)
{
	faucetEmitter->SetActive(true);
	StartSound("snd_button", SND_CHANNEL_BODY);
	StartSound("snd_water", SND_CHANNEL_BODY2);
	sinkIsOn = true;
	isFrobbable = false;
	BecomeActive(TH_THINK);
	sinkTimer = gameLocal.time + SINK_WATERTIME;
	SetColor(FROBCOLOR);
	isSlurping = false;
	slurpTimer = gameLocal.time + 300;


	idVec3 forward, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
	gameLocal.DoParticle(spawnArgs.GetString("model_frobparticle"), GetPhysics()->GetOrigin() + (forward * 16) + (up * -11.3f));

	if (frobber != NULL)
	{
		if (frobber->entityNumber == gameLocal.GetLocalPlayer()->entityNumber)
		{
			gameLocal.GetLocalPlayer()->SetSmelly(false);

			if (gameLocal.GetLocalPlayer()->SetOnFire(false)) //Using toilet puts out fire.
			{
				gameLocal.GetLocalPlayer()->SetCenterMessage("#str_def_gameplay_extinguished");
			}
		}
	}	

	

	return true;
}

