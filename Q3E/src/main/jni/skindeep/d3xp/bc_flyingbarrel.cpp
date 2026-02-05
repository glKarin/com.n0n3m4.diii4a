#include "framework/DeclEntityDef.h"

#include "Trigger.h"
#include "Fx.h"
#include "Light.h"

#include "bc_flyingbarrel.h"

const int FIRESPEW_MAXTIME = 1500; //7000
const int SPIGOT_MAXDISTANCE = 12;
const int DETONATION_DELAY = 400;

CLASS_DECLARATION(idMoveableItem, idFlyingBarrel )
	EVENT(EV_Explode, idFlyingBarrel::Event_PreDeath)
END_CLASS


idFlyingBarrel::idFlyingBarrel()
{
	jetNode = nullptr;
	barrelLight = nullptr;
}

idFlyingBarrel::~idFlyingBarrel()
{

}

void idFlyingBarrel::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt( state ); //  explode_state_t state

	savefile->WriteInt( fireSpewIntervalTimer ); //  int fireSpewIntervalTimer
	savefile->WriteBool( isSpewingFire ); //  bool isSpewingFire
	savefile->WriteInt( fireSpewTimer ); //  int fireSpewTimer
	savefile->WriteVec3( fireSpewDirection ); //  idVec3 fireSpewDirection

	savefile->WriteObject( jetNode ); //  idEntity * jetNode

	savefile->WriteObject( barrelLight ); //  idLight * barrelLight
}

void idFlyingBarrel::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt( (int&)state ); //  explode_state_t state

	savefile->ReadInt( fireSpewIntervalTimer ); //  int fireSpewIntervalTimer
	savefile->ReadBool( isSpewingFire ); //  bool isSpewingFire
	savefile->ReadInt( fireSpewTimer ); //  int fireSpewTimer
	savefile->ReadVec3( fireSpewDirection ); //  idVec3 fireSpewDirection

	savefile->ReadObject( jetNode ); //  idEntity * jetNode

	savefile->ReadObject( CastClassPtrRef(barrelLight) ); //  idLight * barrelLight
}

void idFlyingBarrel::Spawn( void )
{
	//Precache the clipmodels.
	gameLocal.PrecacheDef(this, "def_valve");
	gameLocal.PrecacheDef(this, "projectile_shrapnel");

	//Precache the shrapnel model.
	gameLocal.FindEntityDef("projectile_shrapnel", false);


	isSpewingFire = false;
	fl.takedamage = true;
	fireSpewTimer = 0;
	fireSpewIntervalTimer = 0;
	fireSpewDirection = vec3_zero;
	jetNode = NULL;

	state = NORMAL;
	
}


void idFlyingBarrel::Event_PreDeath()
{
	if (state != PREDEATH)
		return;

	state = DEATH;
	Killed(NULL, NULL, 0, vec3_zero, 0);
}

void idFlyingBarrel::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	BecomeInactive(TH_THINK);

	if (state == NORMAL)
	{
		const idDeclEntityDef *broachDef;
		idDebris *debris;
		idEntity *broachValve;
		idVec3 upDir;

		state = PREDEATH;
		PostEventMS(&EV_Explode, DETONATION_DELAY);

		//Make the little red valve eject off.
		broachDef = gameLocal.FindEntityDef(spawnArgs.GetString("def_valve"), false);
		gameLocal.SpawnEntityDef(broachDef->dict, &broachValve, false);
		if (!broachValve || !broachValve->IsType(idDebris::Type))
		{
			return;
		}

		this->GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &upDir);
		debris = static_cast<idDebris *>(broachValve);
		debris->Create(NULL, this->GetPhysics()->GetOrigin() + (upDir * 58), mat3_identity);
		debris->Launch();
		debris->GetPhysics()->SetLinearVelocity(upDir * 256);
		debris->GetPhysics()->SetAngularVelocity(idVec3(gameLocal.random.CRandomFloat() * 128, gameLocal.random.CRandomFloat() * 128, gameLocal.random.CRandomFloat() * 128));

		//Set to skin with invisible red valve.
		SetSkin(declManager->FindSkin("skins/models/objects/gascylinder/novalve"));			

		return;
	}


	//gameRenderWorld->DebugArrow(colorRed, GetPhysics()->GetOrigin() + idVec3(0, 0, 128), GetPhysics()->GetOrigin(), 8, 10000);
	Explode(attacker);
}

void idFlyingBarrel::Explode(idEntity *attacker)
{
	const char *splashDmgDef;
	idStr fxBreak;

	StopSound(SND_CHANNEL_ANY, false);

	Hide();
	physicsObj.SetContents(0);

	fxBreak = spawnArgs.GetString("fx_explode");
	if (fxBreak.Length())
	{
		idVec3 fxPos = GetPhysics()->GetAbsBounds().GetCenter(); //Center the explosion at gas cylinder middle, not origin.
		idMat3 fxMat = mat3_identity;
		idEntityFx::StartFx(fxBreak, &fxPos, &fxMat, NULL, false);
	}

	splashDmgDef = spawnArgs.GetString("def_splash_damage", "damage_explosion");
	if (splashDmgDef && *splashDmgDef)
	{
		gameLocal.RadiusDamage(GetPhysics()->GetAbsBounds().GetCenter(), this, attacker, this, this, splashDmgDef);
		gameLocal.ThrowShrapnel(GetPhysics()->GetAbsBounds().GetCenter(), "projectile_shrapnel", this);
	}

	physicsObj.PutToRest();
	CancelEvents(&EV_Explode);

	gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin(), "interest_explosion");

	PostEventMS(&EV_Remove, 0);
}

//This is called when the gas cylinder is damaged.
void idFlyingBarrel::AddDamageEffect(const trace_t &collision, const idVec3 &velocity, const char *damageDefName)
{
	idFuncEmitter *splashEnt;
	idDict splashArgs;
	idAngles sprayAngle;
	idDict args;
	idVec3 upDir;
	idVec3 spigotPos;
	float distToSpigot;

	if (health <= 0 || !fl.takedamage)
		return;

	BecomeActive(TH_THINK);

	//Check where the gas cylinder was hit.
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &upDir);
	spigotPos = this->GetPhysics()->GetOrigin() + (upDir * 58);

	distToSpigot = (collision.endpos - spigotPos).Length();

	if (distToSpigot <= SPIGOT_MAXDISTANCE)
	{
		//Was damaged in the spigot area.
		isSpewingFire = false;
		this->Damage(NULL, NULL, vec3_zero, "damage_suicide", 1.0f, 0);
		return;
	}

	if (isSpewingFire)
	{
		//Is already spewing fire and was hit again.

		//TODO: add a sound cue that it was hit a second time.

		fireSpewTimer -= (FIRESPEW_MAXTIME  * .5f);
	}
	else
	{
		//Spawn light.
		idDict lightArgs;

		lightArgs.Clear();
		lightArgs.SetVector("origin", GetPhysics()->GetOrigin() + idVec3(0, 0, 32));
		lightArgs.Set("texture", "lights/flames");
		lightArgs.SetInt("noshadows", 1);
		lightArgs.SetInt("ambient", 1);
		lightArgs.Set("_color", ".4 .2 0 1");
		lightArgs.SetFloat("light", 96);
		barrelLight = (idLight *)gameLocal.SpawnEntityType(idLight::Type, &lightArgs);
		barrelLight->SetOrigin(this->GetPhysics()->GetOrigin() + (upDir * 32));
		barrelLight->Bind(this, false);

		//How long to spew fire.
		fireSpewTimer = gameLocal.time + FIRESPEW_MAXTIME;
	}

	isSpewingFire = true;

	StartSound("snd_firespray", SND_CHANNEL_ANY, 0, false, NULL);

	//gameRenderWorld->DebugLine(colorRed, collision.endpos, collision.endpos + (collision.c.normal * 16), 10000);

	//TODO: Add lightsource to fire.

	//Spray fire particles.
	splashArgs.Set("model", "flame_jet_gascylinder.prt");
	splashArgs.Set("start_off", "1");
	splashEnt = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &splashArgs));

	sprayAngle = (collision.c.normal).ToAngles();
	sprayAngle.pitch += 90 + (gameLocal.random.CRandomFloat() * 20);

	splashEnt->GetPhysics()->SetOrigin(collision.endpos);
	splashEnt->GetPhysics()->SetAxis(sprayAngle.ToMat3());
	splashEnt->PostEventMS(&EV_Activate, 0, this);
	splashEnt->PostEventMS(&EV_Remove, 9000);
	splashEnt->Bind(this, true);


	//Decal.
	gameLocal.ProjectDecal(collision.c.point, -collision.c.normal, 8.0f, true, 20.0f, "textures/decals/bullethole");


	fireSpewDirection = collision.c.normal;
	

	//Spawn the jetNode
	args.Clear();
	args.SetVector("origin", collision.c.point);	
	jetNode = (idTarget *)gameLocal.SpawnEntityType(idTarget::Type, &args);
	sprayAngle.pitch -= 90;
	jetNode->SetAxis(sprayAngle.ToMat3());
	jetNode->Bind(this, true);

	args.Clear();
	args.SetVector("origin", collision.c.point);
	args.SetVector("mins", idVec3(-16, -16, -16));
	args.SetVector("maxs", idVec3(16, 16, 16));
	args.SetFloat("delay", .3f);
	args.Set("def_damage", spawnArgs.GetString("def_damage", "damage_fire"));
	args.SetVector("damage_origin", collision.c.point);
	idTrigger_Hurt* hurtTrigger = (idTrigger_Hurt *)gameLocal.SpawnEntityType(idTrigger_Hurt::Type, &args);
	hurtTrigger->SetAxis(sprayAngle.ToMat3());
	hurtTrigger->PostEventMS(&EV_Remove, FIRESPEW_MAXTIME);
	hurtTrigger->Bind(this, false);
    hurtTrigger->GetPhysics()->GetClipModel()->SetOwner(this);

	this->ApplyImpulse(NULL, 0, collision.c.point, sprayAngle.ToForward() * -512000);
}



void idFlyingBarrel::Think( void )
{
	RunPhysics();
	TouchTriggers();

	//gameRenderWorld->DrawTextA(idStr::Format("%d", (int)health), this->GetPhysics()->GetOrigin() + idVec3(0, 0, 32), .15f, idVec4(1, 1, 1, 1), gameLocal.GetLocalPlayer()->viewAngles.ToMat3());

	if (isSpewingFire)
	{
		if (gameLocal.time > fireSpewIntervalTimer)
		{
			idVec3 jetPos, jetDir, force;


			fireSpewIntervalTimer = gameLocal.time + 300;

			jetPos = jetNode->GetPhysics()->GetOrigin();
			jetDir = -jetNode->GetPhysics()->GetAxis().ToAngles().ToForward();

			jetDir.z += 0.5f;
			force = (jetDir * 64) * this->GetPhysics()->GetMass();
			this->ApplyImpulse(NULL, 0, jetPos, force);
		}

		//TODO: Figure out why firespewtimer expiration isn't getting called sometimes.

		if (gameLocal.time >= fireSpewTimer)
		{
			isSpewingFire = false;
			//gameRenderWorld->DebugArrow(colorGreen, GetPhysics()->GetOrigin() + idVec3(16, 16, 128), GetPhysics()->GetOrigin(), 8, 10000);
			this->Damage(NULL, NULL, vec3_zero, "damage_suicide", 1.0f, 0);
		}
	}

	Present();
}


void idFlyingBarrel::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	int damage;
	const idDeclEntityDef *damageDef = gameLocal.FindEntityDef(damageDefName, false);

	if (!fl.takedamage)
		return;

	if (!damageDef)
	{
		common->Warning("%s unable to find damagedef %s\n", this->GetName(), damageDefName);
		return;
	}

	damage = damageDef->dict.GetInt("damage", "0");

	if (damage > 0)
	{
		//Inflict damage.
		health -= damage;
	}

	if (health <= 0)
	{
		fl.takedamage = false;
		Killed(NULL, NULL, 0, vec3_zero, 0);
	}
}

#define COLLISIONSPEED_THRESHOLD 400

bool idFlyingBarrel::Collide(const trace_t &collision, const idVec3 &velocity)
{
	float v = -(velocity * collision.c.normal);

	if (v >= COLLISIONSPEED_THRESHOLD && !this->IsHidden())
	{
		//AddDamageEffect(collision, velocity, "damage_generic");
		Explode(NULL);
	}

	return idMoveableItem::Collide(collision, velocity);
}
