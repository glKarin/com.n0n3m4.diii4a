#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "idlib/LangDict.h"

#include "bc_trigger_deodorant.h"
#include "bc_handsanitizer.h"

#define SINK_WATERTIME 1000 //frob cooldown time

const idVec3 IDLECOLOR = idVec3(0, 1, 0);
const idVec3 FROBCOLOR = idVec3(1, .9f, 0);

CLASS_DECLARATION(idStaticEntity, idHandSanitizer)
END_CLASS

idHandSanitizer::idHandSanitizer(void)
{
	faucetEmitter = nullptr;

	sinkIsOn = false;
	sinkTimer = 0;

	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;
}

idHandSanitizer::~idHandSanitizer(void)
{
	if (faucetEmitter != NULL) {
		faucetEmitter->PostEventMS(&EV_Remove, 0);
		faucetEmitter = nullptr;
	}

	if (headlightHandle != -1)
		gameRenderWorld->FreeLightDef(headlightHandle);
}

void idHandSanitizer::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
	isFrobbable = true;

	BecomeInactive(TH_THINK);
	fl.takedamage = true;	

	//Spawn the faucet particle.
	idAngles particleAngle = GetPhysics()->GetAxis().ToAngles();
	particleAngle.pitch += 90 - 30;
	particleAngle.yaw += 120;
	idVec3 faucetEmitterPos = GetFaucetPos();
	idDict args;
	args.Clear();
	args.Set("model", spawnArgs.GetString("model_water", spawnArgs.GetString("model_waterparticle")));
	args.SetVector("origin", faucetEmitterPos);
	args.SetMatrix("rotation", particleAngle.ToMat3());
	args.SetBool("start_off", true);
	faucetEmitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));



	idVec3 forward;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
	idVec3 lightPos = GetPhysics()->GetOrigin() + forward * 12;
	headlight.shader = declManager->FindMaterial("lights/defaultPointLight", false);
	headlight.pointLight = true;
	headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = 16;
	headlight.shaderParms[0] = .4f;
	headlight.shaderParms[1] = .1f;
	headlight.shaderParms[2] = .1f;
	headlight.shaderParms[3] = 1.0f;
	headlight.noShadows = true;
	headlight.isAmbient = false;
	headlight.axis = mat3_identity;
	headlightHandle = gameRenderWorld->AddLightDef(&headlight);
	headlight.origin = lightPos;
	gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);


	SetColor(.2f, .2f, .2f);
}

idVec3 idHandSanitizer::GetFaucetPos()
{
	idVec3 forward, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
	return GetPhysics()->GetOrigin() + (forward * 7) + (up * -10);
}

void idHandSanitizer::Save(idSaveGame *savefile) const
{
	savefile->WriteObject( faucetEmitter ); // idFuncEmitter * faucetEmitter

	savefile->WriteBool( sinkIsOn ); // bool sinkIsOn
	savefile->WriteInt( sinkTimer ); // int sinkTimer

	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle
}

void idHandSanitizer::Restore(idRestoreGame *savefile)
{
	savefile->ReadObject( CastClassPtrRef(faucetEmitter) ); // idFuncEmitter * faucetEmitter

	savefile->ReadBool( sinkIsOn ); // bool sinkIsOn
	savefile->ReadInt( sinkTimer ); // int sinkTimer

	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}
}

void idHandSanitizer::Think(void)
{
	idStaticEntity::Think();

	if (sinkIsOn && gameLocal.time > sinkTimer)
	{
		sinkIsOn = false;
		isFrobbable = true;
		BecomeInactive(TH_THINK);		
	}	
}

void idHandSanitizer::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	if (!fl.takedamage)
		return;

	gameLocal.AddEventlogDeath(this, 0, inflictor, attacker, "", EL_DESTROYED);

	fl.takedamage = false;
	StopSound(SND_CHANNEL_ANY, false);

	faucetEmitter->SetActive(false);
	sinkIsOn = false;
	SetColor(0, 0, 0); //dead light
	isFrobbable = true;
	
	//Spew water.
	idEntityFx::StartFx(spawnArgs.GetString("fx_waterdamage"), GetFaucetPos() + idVec3(0,0,-4), mat3_identity);	

	SetModel(spawnArgs.GetString("model_broken"));
	StartSound("snd_break", SND_CHANNEL_ANY, 0, false, NULL);

	headlight.shaderParms[0] = 0;
	headlight.shaderParms[1] = 0;
	headlight.shaderParms[2] = 0;
	gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
}

bool idHandSanitizer::DoFrob(int index, idEntity * frobber)
{
	if (health <= 0)
	{
		StartSound("snd_button", SND_CHANNEL_BODY);
		return true;
	}

	idAngles particleAng;
	particleAng = GetPhysics()->GetAxis().ToAngles();
	particleAng.pitch += 90;
	idEntityFx::StartFx(spawnArgs.GetString("fx_soap"), GetFaucetPos(), particleAng.ToMat3());

	faucetEmitter->SetActive(true);
	StartSound("snd_button", SND_CHANNEL_BODY);
	sinkIsOn = true;
	isFrobbable = false;
	BecomeActive(TH_THINK);
	sinkTimer = gameLocal.time + SINK_WATERTIME;
	
	idVec3 forward, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
	gameLocal.DoParticle(spawnArgs.GetString("model_frobparticle"), GetPhysics()->GetOrigin() + (forward * 16) + (up * -11.3f));

	if (frobber != NULL)
	{
		if (frobber->entityNumber == gameLocal.GetLocalPlayer()->entityNumber)
		{
			gameLocal.GetLocalPlayer()->SetSmelly(false);
		}
	}

	CreateCloud();

	return true;
}

void idHandSanitizer::CreateCloud()
{
	#define RANDRADIUS 32
	idVec3 forwardDir, rightDir, upDir;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, &upDir);

	idVec3 cloudPos = GetFaucetPos() + (forwardDir * gameLocal.random.RandomInt(RANDRADIUS)) + (rightDir * gameLocal.random.RandomInt(-RANDRADIUS, RANDRADIUS)) + (upDir * gameLocal.random.RandomInt(RANDRADIUS));

	idDict args;
	int radius = spawnArgs.GetInt("spewRadius", "32");
	args.SetVector("origin", cloudPos);
	args.SetVector("mins", idVec3(-radius, -radius, -radius));
	args.SetVector("maxs", idVec3(radius, radius, radius));
	args.Set("spewParticle", spawnArgs.GetString("model_spewParticle", "deodorantburst02.prt"));
	args.SetInt("spewLifetime", spawnArgs.GetInt("spewLifetime", "20000"));	
	args.Set("classname", "trigger_cloud_deodorant");
	gameLocal.SpawnEntityDef(args);

	gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_cloud_created"), displayName.c_str()), GetPhysics()->GetOrigin());
}