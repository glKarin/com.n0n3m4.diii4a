#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "idlib/LangDict.h"

#include "bc_trigger_deodorant.h"
#include "bc_frobcube.h"
#include "bc_sink.h"

const int FLUSHTIME = 1800;

const int FROB_DEBOUNCETIME = 300;

const int FROB_ENEMYFROBINDEX = 7;

const int INTEREST_LIFETIME = 1000; //make sure this matches the 'lifetime' value in the sink's interestpoint.

const int SOAP_COOLDOWNTIME = 1200;

const idEventDef EV_setSinkFaucet("setSinkFaucet", "d");
const idEventDef EV_getSinkFaucet("getSinkFaucet", NULL, 'd');

CLASS_DECLARATION(idStaticEntity, idSink)
	EVENT(EV_setSinkFaucet, idSink::Event_SetSinkActive)
	EVENT(EV_getSinkFaucet, idSink::Event_GetSinkActive)
END_CLASS

idSink::idSink(void)
{
	soapCooldownTimer = 0;
}

idSink::~idSink(void)
{
	StopSound(SND_CHANNEL_ANY, 0);

	if (faucetEmitter.IsValid())
		faucetEmitter.GetEntity()->PostEventMS(&EV_Remove, 0);
	if (frobbutton1.IsValid())
		frobbutton1.GetEntity()->PostEventMS(&EV_Remove, 0);
	if (frobbutton2.IsValid())
		frobbutton2.GetEntity()->PostEventMS(&EV_Remove, 0);
	if (button1Anim.IsValid())
		button1Anim.GetEntity()->PostEventMS(&EV_Remove, 0);
	if (button2Anim.IsValid())
		button2Anim.GetEntity()->PostEventMS(&EV_Remove, 0);
}

void idSink::Spawn(void)
{
	idDict args;
	idVec3 forwardDir, rightDir;

	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);
	isFrobbable = false;	


	//Frobcube for button1.
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, NULL);
	args.Clear();
	args.Set("model", "models/objects/sink/kneebutton2_cm.ase");
	args.SetVector("cursoroffset", idVec3(2, 0, -3.5f));
	args.SetInt("health", 1);
	args.Set("displayname", common->GetLanguageDict()->GetString("#str_def_gameplay_sinkbutton"));
	frobbutton1 = gameLocal.SpawnEntityType(idFrobcube::Type, &args);
	frobbutton1.GetEntity()->SetOrigin(GetPhysics()->GetOrigin() + idVec3(0, 0, 7) + forwardDir * 16);
	frobbutton1.GetEntity()->SetAngles(this->GetPhysics()->GetAxis().ToAngles());
	frobbutton1.GetEntity()->GetPhysics()->GetClipModel()->SetOwner(this);
	frobbutton1.GetEntity()->Bind(this, true);
	static_cast<idFrobcube*>(frobbutton1.GetEntity())->SetIndex(1);

	//Button1 animated.
	args.Clear();
	args.SetVector("origin", frobbutton1.GetEntity()->GetPhysics()->GetOrigin());
	args.Set("model", "env_kneebutton");
	button1Anim = (idAnimated *)gameLocal.SpawnEntityType(idAnimated::Type, &args);
	button1Anim.GetEntity()->SetAngles(this->GetPhysics()->GetAxis().ToAngles());
	button1Anim.GetEntity()->Bind(this, true);

	button1Timer = 0;

	//Frobcube for button2.
	args.Clear();
	args.Set("model", "models/objects/sink/kneebutton2_cm.ase");
	args.SetVector("cursoroffset", idVec3(2, 0, -1.5f));
	args.SetInt("health", 1);
	args.Set("displayname", common->GetLanguageDict()->GetString("#str_def_gameplay_sinkbutton"));
	frobbutton2 = gameLocal.SpawnEntityType(idFrobcube::Type, &args);
	frobbutton2.GetEntity()->SetOrigin(GetPhysics()->GetOrigin() + idVec3(0, 0, 7) + forwardDir * 16 + rightDir * -26);
	frobbutton2.GetEntity()->SetAngles(this->GetPhysics()->GetAxis().ToAngles());
	frobbutton2.GetEntity()->GetPhysics()->GetClipModel()->SetOwner(this);
	frobbutton2.GetEntity()->Bind(this, true);
	static_cast<idFrobcube*>(frobbutton2.GetEntity())->SetIndex(2);

	//Button2 animated.
	args.Clear();
	args.SetVector("origin", frobbutton2.GetEntity()->GetPhysics()->GetOrigin());
	args.Set("model", "env_kneebutton2");
	button2Anim = (idAnimated *)gameLocal.SpawnEntityType(idAnimated::Type, &args);
	button2Anim.GetEntity()->SetAngles(this->GetPhysics()->GetAxis().ToAngles());
	button2Anim.GetEntity()->Bind(this, true);

	BecomeActive(TH_THINK);
	fl.takedamage = true;
	sinkIsOn = false;
	interestTimer = 0;

	//Spawn the faucet particle.
	idAngles particleAngle = GetPhysics()->GetAxis().ToAngles();
	particleAngle.pitch += 90;
	idVec3 faucetEmitterPos = GetFaucetPos();
	args.Clear();
	args.Set("model", spawnArgs.GetString("model_water", "water_faucet_loop.prt"));
	args.SetVector("origin", faucetEmitterPos);
	args.SetMatrix("rotation", particleAngle.ToMat3());
	args.SetBool("start_off", true);
	faucetEmitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	faucetEmitter.GetEntity()->Bind(this, true);
}

idVec3 idSink::GetFaucetPos()
{
	idVec3 forward, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
	return GetPhysics()->GetOrigin() + (forward * -4) + (up * 30);
}

void idSink::Save(idSaveGame *savefile) const
{
	button1Anim.Save( savefile ); // idEntityPtr<idAnimated> button1Anim
	savefile->WriteObject( frobbutton1 ); // idEntityPtr<idEntity> frobbutton1
	savefile->WriteInt( button1Timer ); // int button1Timer

	button2Anim.Save( savefile ); // idEntityPtr<idAnimated> button2Anim
	savefile->WriteObject( frobbutton2 ); // idEntityPtr<idEntity> frobbutton2

	faucetEmitter.Save( savefile ); // idEntityPtr<idFuncEmitter> faucetEmitter
	savefile->WriteBool( sinkIsOn ); // bool sinkIsOn

	savefile->WriteInt( interestTimer ); // int interestTimer

	savefile->WriteInt( soapCooldownTimer ); // int soapCooldownTimer
}

void idSink::Restore(idRestoreGame *savefile)
{
	button1Anim.Restore( savefile ); // idEntityPtr<idAnimated> button1Anim
	savefile->ReadObject( frobbutton1 ); // idEntityPtr<idEntity> frobbutton1
	savefile->ReadInt( button1Timer ); // int button1Timer

	button2Anim.Restore( savefile ); // idEntityPtr<idAnimated> button2Anim
	savefile->ReadObject( frobbutton2 ); // idEntityPtr<idEntity> frobbutton2

	faucetEmitter.Restore( savefile ); // idEntityPtr<idFuncEmitter> faucetEmitter
	savefile->ReadBool( sinkIsOn ); // bool sinkIsOn

	savefile->ReadInt( interestTimer ); // int interestTimer

	savefile->ReadInt( soapCooldownTimer ); // int soapCooldownTimer
}

void idSink::Think(void)
{
	idStaticEntity::Think();

	if (gameLocal.time > button1Timer && frobbutton1.IsValid() && !frobbutton1.GetEntity()->isFrobbable)
	{
		frobbutton1.GetEntity()->isFrobbable = true;
	}

	if (gameLocal.time > interestTimer && sinkIsOn)
	{
		interestTimer = gameLocal.time + INTEREST_LIFETIME;
		gameLocal.SpawnInterestPoint(this, GetFaucetPos(), spawnArgs.GetString("interest_faucet"));
	}
}

void idSink::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	if (!fl.takedamage)
		return;

	idVec3 forwardDir, rightDir;

	fl.takedamage = false;
	StopSound(SND_CHANNEL_ANY, false);

	SetModel(spawnArgs.GetString("model_brokenmodel"));
	SetSkin(declManager->FindSkin(spawnArgs.GetString("brokenskin")));
	StartSound("snd_break", SND_CHANNEL_VOICE, 0, false, NULL);

	if (button1Anim.IsValid())
		button1Anim.GetEntity()->PostEventMS(&EV_Remove, 0);
	
	if (button2Anim.IsValid())
		button2Anim.GetEntity()->PostEventMS(&EV_Remove, 0);
	
	if (frobbutton1.IsValid())
		frobbutton1.GetEntity()->PostEventMS(&EV_Remove, 0);
	
	if (frobbutton2.IsValid())
		frobbutton2.GetEntity()->PostEventMS(&EV_Remove, 0);

	if (faucetEmitter.IsValid())
		faucetEmitter.GetEntity()->SetActive(false);
	
	//Spew water.
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, NULL);
	idEntityFx::StartFx("fx/water_jet", this->GetPhysics()->GetOrigin() + forwardDir * -11 + idVec3(0,0,22), mat3_identity);

	idEntityFx::StartFx("fx/soap_spew", this->GetPhysics()->GetOrigin() + forwardDir * 12 + rightDir * -28 + idVec3(0, 0, 20), mat3_identity);

	idMoveableItem::DropItemsBurst(this, "gib", idVec3(0, 0, 32));

	#define DEODORANTCLOUD_DEATHSPAWNCOUNT 2
	for (int i = 0; i < DEODORANTCLOUD_DEATHSPAWNCOUNT; i++)
	{
		//on death, make it spew out some deodorant clouds.
		CreateCloud();
	}
}

idVec3 idSink::GetSoapPos()
{
	idVec3 forwardDir, rightDir, upDir;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, &upDir);

	return this->GetPhysics()->GetOrigin() + (forwardDir * 12) + (rightDir * -23) + (upDir * 22);

}

void idSink::Event_SetSinkActive(int value)
{
	if (value >= 1 && !sinkIsOn)
	{
		//Turn ON the sink faucet.
		faucetEmitter.GetEntity()->SetActive(true);
		StartSound("snd_water", SND_CHANNEL_BODY2, 0, false, NULL);
		sinkIsOn = true;
	}
	else if (value <= 0 && sinkIsOn)
	{
		//Turn OFF the sink faucet.
		faucetEmitter.GetEntity()->SetActive(false);
		StopSound(SND_CHANNEL_BODY2, false);

		//Do the dribble.
		StartSound("snd_dribble", SND_CHANNEL_BODY2);
		idVec3 dribblePos = GetFaucetPos();
		gameLocal.DoParticle(spawnArgs.GetString("model_dribble", "water_faucet_dribble.prt"), dribblePos);
		sinkIsOn = false;
	}
}

void idSink::Event_GetSinkActive(void)
{
	idThread::ReturnInt((health > 0 && sinkIsOn) ? 1 : 0);
}

bool idSink::DoFrob(int index, idEntity * frobber)
{
	if (health <= 0)
		return false;

	if (index == 1)
	{
		if (!sinkIsOn)
		{
			Event_SetSinkActive(1);
		}
		else
		{
			Event_SetSinkActive(0);
		}

		button1Anim.GetEntity()->Event_PlayAnim("push", 1);
		frobbutton1.GetEntity()->isFrobbable = false;
		button1Timer = gameLocal.time + FROB_DEBOUNCETIME;
	}
	else if (index == 2)
	{
		//Soap.
		idAngles particleAngle;
		idVec3 forwardDir, rightDir;
		idVec3 particlePos;

		button2Anim.GetEntity()->Event_PlayAnim("push", 1);

		particleAngle = GetPhysics()->GetAxis().ToAngles();
		particleAngle.yaw += -90;
		particleAngle.pitch += 135;

		particlePos = GetSoapPos();

		idEntityFx::StartFx("fx/soap_squirt", particlePos, particleAngle.ToMat3());

		if (gameLocal.time > soapCooldownTimer)
		{
			soapCooldownTimer = gameLocal.time + SOAP_COOLDOWNTIME;
			CreateCloud();
		}
	}
	else if (index == FROB_ENEMYFROBINDEX)
	{
		if (sinkIsOn)
		{
			//The enemy turns off sink.
			DoFrob(1, frobber);
			
			gameLocal.DoParticle("frob_lines.prt", frobbutton1.GetEntity()->GetPhysics()->GetOrigin());
		}
	}

	gameLocal.GetLocalPlayer()->SetSmelly(false);

	if (gameLocal.GetLocalPlayer()->SetOnFire(false)) //Using toilet puts out fire.
	{
		gameLocal.GetLocalPlayer()->SetCenterMessage("#str_def_gameplay_extinguished");
	}

	return true;
}

void idSink::CreateCloud()
{
	#define RANDRADIUS 32
	idVec3 forwardDir, rightDir, upDir;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, &upDir);

	idVec3 cloudPos = GetSoapPos() + (forwardDir * gameLocal.random.RandomInt(RANDRADIUS)) + (rightDir * gameLocal.random.RandomInt(-RANDRADIUS, RANDRADIUS)) + (upDir * gameLocal.random.RandomInt(RANDRADIUS));

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