#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "WorldSpawn.h"
#include "framework/DeclEntityDef.h"

#include "bc_tnt.h"

const int DETONATIONTIME = 10000; //how long timer is.
const int TRIGGER_TIME = 700; //how long the buzzer rings.


const int EXPLOSION_MAXRANGE = 2048;

const int PACKUPTIME = 300;

const int EXPLOSIONARRAY_INTERVAL = 300;

CLASS_DECLARATION(idMoveableItem, idTNT)

END_CLASS


void idTNT::Spawn(void)
{
	idDict args;

	args.Clear();
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.Set("model", "model_tnt");
	args.Set("start_anim", "closed");
	animatedEnt = (idAnimated*)gameLocal.SpawnEntityType(idAnimated::Type, &args);	
	animatedEnt->SetAxis(GetPhysics()->GetAxis());
	animatedEnt->Bind(this, true);

	state = TNT_MOVEABLE;
	lastThrowTime = 0;

	loudtick0 = false;
	loudtick1 = false;
	loudtick2 = false;

	for (int i = 0; i < SPHEREPOINTCOUNT; i++)
	{
		damageRayPositions[i] = vec3_zero;
		damageRayAngles[i] = vec3_zero;
	}

	explosionIndex = 0;
	explosionArrayTimer = 0;

	if (spawnArgs.GetBool("deploy", "0"))
	{
		Deploy();
	}

	
}

void idTNT::Save(idSaveGame* savefile) const
{
	savefile->WriteInt( state ); // int state

	savefile->WriteObject( animatedEnt ); // idAnimated* animatedEnt
	savefile->WriteJoint( clockJoint ); //  saveJoint_t clockJoint
	savefile->WriteJoint( originJoint ); // saveJoint_t originJoint;

	savefile->WriteInt( detonationTimer ); // int detonationTimer


	savefile->WriteBool( loudtick0 ); // bool loudtick0
	savefile->WriteBool( loudtick1 ); // bool loudtick1
	savefile->WriteBool( loudtick2 ); // bool loudtick2

	SaveFileWriteArray( damageRayPositions, SPHEREPOINTCOUNT, WriteVec3 ); // idVec3 damageRayPositions[SPHEREPOINTCOUNT]
	SaveFileWriteArray( damageRayAngles, SPHEREPOINTCOUNT, WriteVec3 ); // idVec3 damageRayAngles[SPHEREPOINTCOUNT]
	savefile->WriteInt( explosionIndex ); // int explosionIndex
	savefile->WriteInt( explosionArrayTimer ); // int explosionArrayTimer

	savefile->WriteInt( lastThrowTime ); // int lastThrowTime
}

void idTNT::Restore(idRestoreGame* savefile)
{
	savefile->ReadInt( state ); // int state

	savefile->ReadObject( CastClassPtrRef(animatedEnt) ); // idAnimated* animatedEnt
	savefile->ReadJoint( clockJoint ); //  saveJoint_t clockJoint
	savefile->ReadJoint( originJoint ); // saveJoint_t originJoint;

	savefile->ReadInt( detonationTimer ); // int detonationTimer

	savefile->ReadBool( loudtick0 ); // bool loudtick0
	savefile->ReadBool( loudtick1 ); // bool loudtick1
	savefile->ReadBool( loudtick2 ); // bool loudtick2

	SaveFileReadArray( damageRayPositions, ReadVec3 ); // idVec3 damageRayPositions[SPHEREPOINTCOUNT]
	SaveFileReadArray( damageRayAngles, ReadVec3 ); // idVec3 damageRayAngles[SPHEREPOINTCOUNT]
	savefile->ReadInt( explosionIndex ); // int explosionIndex
	savefile->ReadInt( explosionArrayTimer ); // int explosionArrayTimer

	savefile->ReadInt( lastThrowTime ); // int lastThrowTime
}

void idTNT::Think(void)
{
	if (state == TNT_MOVEABLE)
	{
		idMoveableItem::Think();
		return;
	}
	else if (state == TNT_ARMED)
	{
		idMat3		bodyAxis;
		idVec3		offset;
		idRotation	secondRotation;
		float		timerLerp;

		timerLerp = 1.0f - ((detonationTimer - gameLocal.time) / (float)DETONATIONTIME);

		if (timerLerp >= 1.0f)
		{
			TriggerBuzzer();
			return;
		}

		//Rotate the clock hand.
		animatedEnt->GetAnimator()->GetJointTransform(originJoint, gameLocal.time, offset, bodyAxis);
		secondRotation.SetVec(bodyAxis[0]);
		secondRotation.SetAngle(timerLerp * 360.0f);
		animatedEnt->GetAnimator()->SetJointAxis(clockJoint, JOINTMOD_WORLD, secondRotation.ToMat3());

		if (!isFrobbable)
		{
			if (detonationTimer - gameLocal.time < DETONATIONTIME - 1000)
			{
				isFrobbable = true;
			}
		}

		int remainingTime = detonationTimer - gameLocal.time;

		if (remainingTime <= 3000 && !loudtick2)
		{
			loudtick2 = true;
			StartSound("snd_tickloud", SND_CHANNEL_VOICE, 0, false, NULL);
		}
		else if (remainingTime <= 2000 && !loudtick1)
		{
			loudtick1 = true;
			StartSound("snd_tickloud", SND_CHANNEL_VOICE, 0, false, NULL);
		}
		else if (remainingTime <= 1000 && !loudtick0)
		{
			loudtick0 = true;
			StartSound("snd_tickloud", SND_CHANNEL_VOICE, 0, false, NULL);
		}
	}
	else if (state == TNT_TRIGGERED)
	{
		if (gameLocal.time >= detonationTimer)
		{
			//EXPLODE.
			Explode();
		}
	}
	else if (state == TNT_EXPLODING)
	{
		if (gameLocal.time > explosionArrayTimer)
		{
			float decalSize;

			explosionArrayTimer = gameLocal.time + EXPLOSIONARRAY_INTERVAL;

			idVec3 explosionPosition = damageRayPositions[explosionIndex];
			idVec3 explosionAngle = vec3_zero;

			if (explosionPosition == vec3_zero)
			{
				for (int i = explosionIndex; i < SPHEREPOINTCOUNT; i++)
				{
					//Damage position is empty. Skip it.
					if (damageRayPositions[explosionIndex] == vec3_zero)
					{
						explosionIndex++;
						continue;
					}

					//Found a valid damage position.
					explosionPosition = damageRayPositions[explosionIndex];
					explosionAngle = damageRayAngles[explosionIndex];
					break;
				}
			}

			explosionIndex++;
			if (explosionIndex >= SPHEREPOINTCOUNT)
			{
				state = TNT_EXPLODED;
				return;
			}


			//common->Printf("exploding. index %d    time %d\n", explosionIndex, gameLocal.time);
			//gameRenderWorld->DebugArrow(colorGreen, GetPhysics()->GetOrigin(), explosionPosition, 4, 30000);

			gameLocal.RadiusDamage(explosionPosition, this, this, this, this, "damage_explosion_tnt");
			idEntityFx::StartFx("fx/explosion_tnt", explosionPosition, explosionAngle.ToMat3());

			//Decal.
			decalSize = 100 + gameLocal.random.RandomInt(50);

			gameLocal.ProjectDecal(explosionPosition, -explosionAngle, 8.0f, true, decalSize, (gameLocal.random.RandomInt(2) % 2 == 0) ? "textures/decals/scorch1024_filled" : "textures/decals/scorch1024_faded");
			//gameRenderWorld->DrawTextA(idStr::Format("%f", trRay.fraction), trRay.endpos, .3f, colorWhite, -trRay.c.normal.ToMat3(), 1, 30000, true);
		}


	}
	else if (state == TNT_DISARMING)
	{
		if (gameLocal.time >= detonationTimer)
		{
			state = TNT_PACKUPDONE;
			this->Hide();
			animatedEnt->Hide();
			PostEventMS(&EV_Remove, 100);

			idEntityFx::StartFx("fx/smoke_ring04", &GetPhysics()->GetOrigin(), &mat3_default, NULL, false);

			//Give the TNT back to the player.
			//gameLocal.GetLocalPlayer()->Give("weapon", "weapon_tnt");
			//gameLocal.GetLocalPlayer()->SetAmmoDelta("ammo_tnt", 1);
			gameLocal.GetLocalPlayer()->GiveItem("weapon_tnt");
		}
	}
}

//TNT Deploys and gets affixed to a surface.
void idTNT::Deploy()
{
	idDict args;

	state = TNT_ARMED;

	idVec3 rawAngleValue = spawnArgs.GetVector("angles");
	idAngles spawnAngle = rawAngleValue.ToAngles();

	this->SetAngles(spawnAngle);

	//BC 2-21-2025: workaround fix for TNT animated entity not orienting correctly.
	animatedEnt->Unbind();
	animatedEnt->SetAngles(spawnAngle);
	animatedEnt->Bind(this, true);
	

	animatedEnt->Event_PlayAnim("unfold", 1);

	clockJoint = animatedEnt->GetAnimator()->GetJointHandle("clockhand");
	originJoint = animatedEnt->GetAnimator()->GetJointHandle("origin");

	detonationTimer = gameLocal.time + DETONATIONTIME;

	StartSound("snd_ticking", SND_CHANNEL_BODY, 0, false, NULL);	

	//Dust kickup when tnt is affixed to surface.
	idEntityFx::StartFx("fx/smoke_ring04", GetPhysics()->GetOrigin(), spawnAngle.ToMat3());
}

void idTNT::TriggerBuzzer(void)
{
	if (state == TNT_TRIGGERED)
		return;

	state = TNT_TRIGGERED;
	detonationTimer = gameLocal.time + TRIGGER_TIME;
	StartSound("snd_tickdone", SND_CHANNEL_BODY, 0, false, NULL);
	fl.takedamage = false;

	gameLocal.DoParticle(spawnArgs.GetString("model_warning"), GetPhysics()->GetOrigin());
}

void idTNT::Explode(void)
{
	int i;
	idVec3* pointsOnSphere;
	idVec3 bombDir;
	idVec3 bombPos;


	if (state == TNT_EXPLODING)
		return;

	state = TNT_EXPLODING;

	gameLocal.SetSuspiciousNoise(this, this->GetPhysics()->GetOrigin(), spawnArgs.GetInt("noise_radius", "2048"), NOISE_COMBATPRIORITY);

	bombDir = animatedEnt->GetPhysics()->GetAxis().ToAngles().ToForward();
	//bombDir.yaw += 90;

	//Local damage around self.
	bombPos = this->GetPhysics()->GetOrigin();
	gameLocal.RadiusDamage(bombPos, this, this, this, this, "damage_explosion_tnt_originpoint");
	idEntityFx::StartFx("fx/explosion_tnt_linger", bombPos, bombDir.ToMat3());

	//Now do a decal at the TNT location itself.
	gameLocal.ProjectDecal(bombPos, -bombDir, 8.0f, true, 300, "textures/decals/scorch1024");

	animatedEnt->Hide();
	this->Hide();
	this->GetPhysics()->SetContents(0);
	PostEventMS(&EV_Remove, 1000);
	fl.takedamage = false;
	StopSound(SND_CHANNEL_BODY, false);

	pointsOnSphere = gameLocal.GetPointsOnSphere(SPHEREPOINTCOUNT);


	for (i = 0; i < SPHEREPOINTCOUNT; i++)
	{
		float vdot;
		trace_t trRay;
		idVec3 raydir = pointsOnSphere[i];
		raydir.NormalizeFast();

		//Skip tracelines that go behind the bomb.
		vdot = DotProduct(bombDir, raydir);

		if (vdot < 0)
			continue;

		//Do traceline check.
		gameLocal.clip.TracePoint(trRay, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + raydir * EXPLOSION_MAXRANGE, MASK_SOLID, NULL);

		if (trRay.fraction >= 1.0f)
		{
			damageRayPositions[i] = vec3_zero;
			damageRayAngles[i] = vec3_zero;
		}
		else
		{
			damageRayPositions[i] = trRay.endpos;
			damageRayAngles[i] = trRay.c.normal;
		}
	}
}

void idTNT::Damage(idEntity* inflictor, idEntity* attacker, const idVec3& dir, const char* damageDefName, const float damageScale, const int location, const int materialType)
{
	int damage;
	const idDeclEntityDef* damageDef = gameLocal.FindEntityDef(damageDefName, false);

	if (state == TNT_TRIGGERED || state == TNT_EXPLODING || state == TNT_EXPLODED || health <= 0 || !fl.takedamage)
		return;	

	if (!damageDef)
	{
		common->Warning("%s unable to find damagedef %s\n", this->GetName(), damageDefName);
		return;
	}

	damage = damageDef->dict.GetInt("damage", "0");
	if (damage > 0)
	{
		health -= damage;
	}

	if (health <= 0)
	{
		TriggerBuzzer();
	}
}

bool idTNT::DoFrob(int index, idEntity* frobber)
{
	if (state == TNT_ARMED)
	{
		state = TNT_DISARMING;
		StopSound(SND_CHANNEL_BODY, false);

		fl.takedamage = false;
		animatedEnt->Event_PlayAnim("packup", 1);
		detonationTimer = gameLocal.time + PACKUPTIME;

		gameLocal.GetLocalPlayer()->StartSound("snd_grab", SND_CHANNEL_ANY, 0, false, NULL);
	}
	else
	{
		bool frobbed = idMoveableItem::DoFrob(index, frobber);
		if (frobbed)
		{
			animatedEnt->Hide();
		}
		return frobbed;
	}

	return true;
}




void idTNT::JustThrown()
{
	lastThrowTime = gameLocal.time;
}

bool idTNT::Collide(const trace_t& collision, const idVec3& velocity)
{
	if (state != TNT_MOVEABLE)
		return false;

	//don't deploy tnt right when it spawns. But we do want it to deploy when thrown.
	if (gameLocal.time - spawnTime < 1000 && lastThrowTime <= 0)
		return false;

	//Don't do checks if being held by player
	if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
	{
		if (gameLocal.GetLocalPlayer()->GetCarryable() == this)
		{
			return false;
		}
	}

	//BC 3-6-2025: ignore if I hit a sky brush.
	if (collision.c.material)
	{
		if (collision.c.material->GetSurfaceFlags() >= 256)
			return false; //It's sky. Exit here.
	}


	#define COLLISION_VELOCITY_THRESHOLD 20
	float v;
	v = -(velocity * collision.c.normal);
	if (v > COLLISION_VELOCITY_THRESHOLD && gameLocal.time > 2000) //don't deploy at game start.
	{
		idVec3 deployAngle = collision.c.normal;
		spawnArgs.SetVector("angles", deployAngle);
		SetOrigin(collision.c.point);

		//BC 3-27-2025: hack fix
		//animatedEnt->Unbind();
		//animatedEnt->SetOrigin(GetPhysics()->GetOrigin());
		//animatedEnt->Bind(this, true);

		Deploy();
		gameLocal.BindStickyItemViaTrace(this, collision);
		return false;
	}

	return idMoveableItem::Collide(collision, velocity);
}

//BC 2-21-2025: Fix for tnt not teleporting correctly (i.e. for trashchute)
void idTNT::Teleport(const idVec3& origin, const idAngles& angles, idEntity* destination)
{
	animatedEnt->Unbind();
	idEntity::Teleport(origin, angles, destination);

	animatedEnt->SetOrigin(GetPhysics()->GetOrigin());
	animatedEnt->SetAxis(GetPhysics()->GetAxis());
	animatedEnt->Bind(this, true);
}