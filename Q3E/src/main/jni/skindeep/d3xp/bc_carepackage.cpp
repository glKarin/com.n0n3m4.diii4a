#include "framework/DeclEntityDef.h"
#include "Entity.h"
#include "Player.h"

#include "Fx.h"
#include "bc_carepackage.h"

const int LANDING_DISTANCE_FROM_POD = 384;
const int CHECKDIST = 32;

const int LIGHT_RADIUS = 128;

#define MINIPACKAGE_STATICTIME 1000 //how long it floats around
#define MINIPACKAGE_DEPLOYDELAY 1000 //how long before it deploys

#define WIREWIDTH 1

CLASS_DECLARATION(idAnimatedEntity, idCarepackage)
END_CLASS

idCarepackage::idCarepackage(void)
{
	memset(&boatlight, 0, sizeof(boatlight));
	boatlightHandle = -1;

	spacenudgeNode.SetOwner(this);
	spacenudgeNode.AddToEnd(gameLocal.spacenudgeEntities);

	for (int i = 0; i < 2; i++)
	{
		wireOrigin[i] = NULL;
		wireTarget[i] = NULL;
	}
}

idCarepackage::~idCarepackage(void)
{
	StopSound(SND_CHANNEL_ANY, false);

	if (boatlightHandle != -1)
		gameRenderWorld->FreeLightDef(boatlightHandle);

	spacenudgeNode.Remove();
}

void idCarepackage::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt( state ); //  carepackage_state_t state
	savefile->WriteInt( stateTimer ); //  int stateTimer

	savefile->WriteObject( idleSmoke ); //  idFuncEmitter * idleSmoke

	savefile->WriteRenderLight( boatlight ); //  renderLight_t boatlight
	savefile->WriteInt( boatlightHandle ); //  int boatlightHandle


	savefile->WriteObject( packageMini ); //  idEntity * packageMini

	SaveFileWriteArray(wireOrigin, 2, WriteObject); // idBeam* wireOrigin[2];
	SaveFileWriteArray(wireTarget, 2, WriteObject); // idBeam* wireTarget[2];
}

void idCarepackage::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt( (int&)state ); //  carepackage_state_t state
	savefile->ReadInt( stateTimer ); //  int stateTimer

	savefile->ReadObject( CastClassPtrRef(idleSmoke) ); //  idFuncEmitter * idleSmoke

	savefile->ReadRenderLight( boatlight ); //  renderLight_t boatlight
	savefile->ReadInt( boatlightHandle ); //  int boatlightHandle
	if ( boatlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( boatlightHandle, &boatlight );
	}

	savefile->ReadObject( packageMini ); //  idEntity * packageMini

	SaveFileReadArrayCast(wireOrigin, ReadObject, idClass*&); // idBeam* wireOrigin[2];
	SaveFileReadArrayCast(wireTarget, ReadObject, idClass*&); // idBeam* wireOrigin[2];
}

void idCarepackage::Spawn( void )
{
	//The carepackage hides at spawn.
	Hide();

	#define SPAWN_DISTANCE_FROM_POD 14

	//Spawn the physics object on the side of the pod.
	idVec3 cubbyPos = spawnArgs.GetVector("cubbypos");
	idVec3 podPos = spawnArgs.GetVector("podpos");
	idVec3 perpendicularDir = cubbyPos - podPos;
	perpendicularDir.Normalize();
	idVec3 spawnPos = cubbyPos + perpendicularDir * SPAWN_DISTANCE_FROM_POD;
	idMat3 podRot = spawnArgs.GetMatrix("podforward");

	idDict args;
	args.Set("classname", spawnArgs.GetString("def_minipackage"));
	args.SetVector("origin", spawnPos);
	args.SetMatrix("rotation", podRot);
	args.Set("skin", this->spawnArgs.GetString("skin", "")); // SW: components inherit the care package's skin
	//args.SetBool("solid", false);
	gameLocal.SpawnEntityDef(args, &packageMini);
	if (packageMini)
	{
		gameLocal.DoParticle(packageMini->spawnArgs.GetString("model_spawnparticle"), spawnPos);
		packageMini->fl.takedamage = false;
		//packageMini->GetPhysics()->SetContents(0);
		//packageMini->GetPhysics()->PutToRest();

		//give it a gentle push.
		idVec3 awayDir = spawnPos - podPos;
		awayDir.Normalize();

		idVec3 forwardDir = podRot.ToAngles().ToForward();
		idVec3 pushDir = (awayDir * 32) + (forwardDir * -64);

		packageMini->GetPhysics()->SetLinearVelocity(pushDir);
		packageMini->GetPhysics()->SetAngularVelocity(idVec3(gameLocal.random.RandomInt(-16, 16), gameLocal.random.RandomInt(-16, 16), 0));
	}

	stateTimer = gameLocal.time + MINIPACKAGE_STATICTIME;
	state = CPK_MINIPACKAGE_STATIC;
	BecomeActive(TH_THINK);
}


void idCarepackage::EquipPrimarySlot()
{
	idList<idStr> primaryCandidates;
	const idKeyValue *kv;
	kv = this->spawnArgs.MatchPrefix("def_primary", NULL);
	while (kv)
	{
		idStr candidate = kv->GetValue();
		if (candidate.Length() > 0)
		{
			primaryCandidates.Append(candidate);
		}

		kv = this->spawnArgs.MatchPrefix("def_primary", kv); //Iterate to next entry.
	}

	int primaryIdx = gameLocal.random.RandomInt(primaryCandidates.Num());

	//spawn the gun.
	idVec3 forward, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);

	idVec3 primaryPos = GetPhysics()->GetOrigin() + (forward * 6);

	idAngles primaryAngle = GetPhysics()->GetAxis().ToAngles();
	//primaryAngle.pitch += 90;
	//primaryAngle.yaw += 90;

	idEntity *primaryEnt;
	idDict args;
	args.Clear();
	args.SetVector("origin", primaryPos);
	args.SetMatrix("rotation", primaryAngle.ToMat3());
	args.Set("classname", primaryCandidates[primaryIdx].c_str());
	args.Set("bind", this->GetName());
	args.Set("bindtojoint", "body");
	gameLocal.SpawnEntityDef(args, &primaryEnt);
	

	//spawn the associated ammo.
	#define AMMOCOUNT 2
	idStr ammoDef = spawnArgs.GetString(idStr::Format("def_ammo%d", primaryIdx + 1));
	if (ammoDef.Length() > 0)
	{
		for (int i = 0; i < AMMOCOUNT; i++)
		{
			idVec3 ammoPos = GetPhysics()->GetOrigin() + (forward * -8) + (up * 12);
			ammoPos += up * (i * -6); //for each ammo item, move it down a slot.

			idAngles ammoAngle = GetPhysics()->GetAxis().ToAngles();

			idEntity *ammoEnt;
			idDict ammoArgs;
			ammoArgs.Clear();
			ammoArgs.SetVector("origin", ammoPos);
			ammoArgs.SetMatrix("rotation", ammoAngle.ToMat3());
			ammoArgs.Set("classname", ammoDef.c_str());
			ammoArgs.Set("bind", this->GetName());
			ammoArgs.Set("bindtojoint", "body");
			gameLocal.SpawnEntityDef(ammoArgs, &ammoEnt);
		}
	}
}

void idCarepackage::EquipSecondarySlot()
{
	idList<idStr> secondaryCandidates;
	const idKeyValue *kv;
	kv = this->spawnArgs.MatchPrefix("def_secondary", NULL);
	while (kv)
	{
		idStr candidate = kv->GetValue();
		if (candidate.Length() > 0)
		{
			secondaryCandidates.Append(candidate);
		}

		kv = this->spawnArgs.MatchPrefix("def_secondary", kv); //Iterate to next entry.
	}

	//spawn items.
	idVec3 forward, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);

	idVec3 basePos = GetPhysics()->GetOrigin() + (forward * -8) + (up * -4);
	idAngles itemAngle = GetPhysics()->GetAxis().ToAngles();

	//itemAngle.yaw += 180;
	
	#define TERTIARYCOUNT 2
	for (int i = 0; i < TERTIARYCOUNT; i++)
	{
		int secondaryIdx = gameLocal.random.RandomInt(secondaryCandidates.Num());

		idVec3 itemPos = basePos + (up * (i * -12));

		idEntity *ent;
		idDict args;
		args.Clear();
		args.SetVector("origin", itemPos);
		args.SetMatrix("rotation", itemAngle.ToMat3());
		args.Set("classname", secondaryCandidates[secondaryIdx].c_str());
		args.Set("bind", this->GetName());
		args.Set("bindtojoint", "body");
		gameLocal.SpawnEntityDef(args, &ent);
	}
}

void idCarepackage::EquipTertiarySlot()
{
	idList<idStr> tertiaryCandidates;
	const idKeyValue *kv;
	kv = this->spawnArgs.MatchPrefix("def_tertiary", NULL);
	while (kv)
	{
		idStr candidate = kv->GetValue();
		if (candidate.Length() > 0)
		{
			tertiaryCandidates.Append(candidate);
		}

		kv = this->spawnArgs.MatchPrefix("def_tertiary", kv); //Iterate to next entry.
	}

	//spawn items.
	idVec3 right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, &right, &up);

	idVec3 basePos = GetPhysics()->GetOrigin() + (right * 12) + (up * 8);
	idAngles itemAngle = GetPhysics()->GetAxis().ToAngles();

	//itemAngle.yaw += 180;

	#define TERTIARYCOUNT 2
	for (int i = 0; i < TERTIARYCOUNT; i++)
	{
		int tertiaryIdx = gameLocal.random.RandomInt(tertiaryCandidates.Num());

		idVec3 itemPos = basePos + (up * (i * -18));

		idEntity *ent;
		idDict args;
		args.Clear();
		args.SetVector("origin", itemPos);
		args.SetMatrix("rotation", itemAngle.ToMat3());
		args.Set("classname", tertiaryCandidates[tertiaryIdx].c_str());
		args.Set("bind", this->GetName());
		args.Set("bindtojoint", "body");
		gameLocal.SpawnEntityDef(args, &ent);
	}
}




idVec3 idCarepackage::FindClearSpot(idVec3 _pos, idMat3 _rot)
{
	//Find clear spot to spawn the package.
	#define RANDOMARRAYSIZE 27
	idVec3 randomArray[] =
	{
		idVec3(0,0,0),
		idVec3(0,0,CHECKDIST),
		idVec3(0,0,-CHECKDIST),
		
		idVec3(CHECKDIST, 0, 0),
		idVec3(-CHECKDIST, 0, 0),
		idVec3(0,CHECKDIST,0),
		idVec3(0,-CHECKDIST,0),

		idVec3(CHECKDIST, CHECKDIST, 0),
		idVec3(-CHECKDIST, CHECKDIST, 0),
		idVec3(CHECKDIST,-CHECKDIST,0),
		idVec3(-CHECKDIST,-CHECKDIST,0),

		idVec3(CHECKDIST,0,CHECKDIST),
		idVec3(-CHECKDIST,0,CHECKDIST),
		idVec3(0,CHECKDIST, CHECKDIST),
		idVec3(0,-CHECKDIST, CHECKDIST),
		
		idVec3(CHECKDIST,0,-CHECKDIST),
		idVec3(-CHECKDIST,0,-CHECKDIST),
		idVec3(0,CHECKDIST,-CHECKDIST),
		idVec3(0,-CHECKDIST,-CHECKDIST),

		idVec3(CHECKDIST,-CHECKDIST,CHECKDIST),
		idVec3(-CHECKDIST,-CHECKDIST,CHECKDIST),
		idVec3(CHECKDIST,CHECKDIST,CHECKDIST),
		idVec3(-CHECKDIST,CHECKDIST,CHECKDIST),

		idVec3(CHECKDIST,-CHECKDIST,-CHECKDIST),
		idVec3(-CHECKDIST,-CHECKDIST,-CHECKDIST),
		idVec3(CHECKDIST,CHECKDIST,-CHECKDIST),
		idVec3(-CHECKDIST,CHECKDIST,-CHECKDIST),

	};
	
	//Do clearance checks.	
	idBounds packageBound = idBounds(idVec3(-8, -8, -25), idVec3(8, 8, 25));
	packageBound.Rotate(_rot);
	for (int i = 0; i < RANDOMARRAYSIZE; i++)
	{
		idVec3 candidatePos = _pos + randomArray[i];	
		//ENTITYNUM_WORLD	
		//do a solidity check.
		int penetrationContents = gameLocal.clip.Contents(candidatePos, NULL, mat3_identity, CONTENTS_SOLID, NULL);
		if (penetrationContents & MASK_SOLID)
		{
			continue; //If it starts in solid, then exit.
		}
		
		trace_t boundTr;
		gameLocal.clip.TraceBounds(boundTr, candidatePos, candidatePos, packageBound, MASK_SOLID, NULL);
		if (boundTr.fraction < 1)
			continue;
	
		//We now have a reasonable position to land.
		return candidatePos;
	}

	//Failed. Just go with the original spot.
	return _pos;
}

void idCarepackage::DoTetherWire(idVec3 _pos, int _idx)
{
	#define SPHEREDENSITY 24
	idVec3* pointsOnSphere;
	int i;

	pointsOnSphere = gameLocal.GetPointsOnSphere(SPHEREDENSITY);

	idVec3 bestSpot = vec3_zero;
	float closestDist = 999999999;
	idVec3 hitnormal = vec3_zero;
	for (i = 0; i < SPHEREDENSITY; i++)
	{
		idVec3 wireDir;
		wireDir = pointsOnSphere[i];
		wireDir.Normalize();

		if (wireDir.y < 0)
			continue;

		idVec3 endPos = _pos + wireDir * 1024;

		trace_t tr;
		gameLocal.clip.TracePoint(tr, _pos, endPos, MASK_SOLID, NULL);

		if (tr.fraction >= 1 || tr.c.entityNum != ENTITYNUM_WORLD)
			continue;

		if (tr.c.material)
		{
			if (tr.c.material->GetSurfaceFlags() >= 256)
			{
				//Hit the SKY.
				continue;
			}
		}

		float dist = (tr.endpos - _pos).Length();
		if (dist < closestDist)
		{
			closestDist = dist;
			bestSpot = tr.endpos;
			hitnormal = tr.c.normal;
		}
	}

	if (bestSpot == vec3_zero)
		return; //Failed to find a spot.

	//Spawn decal.
	#define DECALSIZE 16.0f
	gameLocal.ProjectDecal(bestSpot, -hitnormal, 8.0f, true, DECALSIZE, "textures/decals/spearcrack"); //decal fx

	jointHandle_t joint = this->GetAnimator()->GetJointHandle("body");

	//Found a valid spot.
	//Spawn the cable.
	idDict args;
	args.Clear();
	args.SetVector("origin", _pos);
	args.SetFloat("width", WIREWIDTH);
	wireTarget[_idx] = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);	
	wireTarget[_idx]->BecomeActive(TH_PHYSICS);
	
	args.Clear();
	args.Set("target", wireTarget[_idx]->name.c_str());
	args.SetBool("start_off", false);
	args.SetVector("origin", bestSpot);
	args.SetFloat("width", WIREWIDTH);
	args.Set("skin", "skins/beam_harpoon");	
	wireOrigin[_idx] = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
	wireOrigin[_idx]->BindToJoint(this, joint, false);
	wireOrigin[_idx]->BecomeActive(TH_PHYSICS);
	//wireOrigin[_idx]->Bind(harpoonModel, false);
	
	//todo: make the beam end not at the harpoon tip , but at the harpoon end.

	//Spawn the harpoon model.
	idAngles harpoonAngle = (bestSpot - _pos).ToAngles();
	idEntity *harpoonModel;
	args.Clear();
	args.Set("classname", "func_static");
	args.Set("model", "models/objects/pirateship/grapple/grapple12.ase");
	args.SetVector("origin", bestSpot);
	args.SetMatrix("rotation", harpoonAngle.ToMat3());
	gameLocal.SpawnEntityDef(args, &harpoonModel);
	

		
}

void idCarepackage::DeployPackage(idVec3 _pos, idMat3 _rot)
{
	idVec3 clearSpot = FindClearSpot(_pos, _rot);

	SetOrigin(clearSpot);
	SetAxis(_rot);
	Show();
	StartSound("snd_idle", SND_CHANNEL_AMBIENT);
	Event_PlayAnim("idle", 1, true);

	StartSound("snd_open", SND_CHANNEL_ANY);
	SpawnDebrisPanels();

	gameLocal.DoParticle(spawnArgs.GetString("model_deploysmoke"), clearSpot);

	boatlight.shader = declManager->FindMaterial(spawnArgs.GetString("mtr_lightshader"), false);
	boatlight.pointLight = true;
	boatlight.lightRadius[0] = LIGHT_RADIUS;
	boatlight.lightRadius[1] = LIGHT_RADIUS;
	boatlight.lightRadius[2] = LIGHT_RADIUS;
	boatlight.shaderParms[0] = 1.0f; // R
	boatlight.shaderParms[1] = 0.8f; // G
	boatlight.shaderParms[2] = 0.0f; // B
	boatlight.shaderParms[3] = 1.0f;
	boatlight.noShadows = true;
	boatlight.isAmbient = true;
	boatlight.axis = mat3_identity;
	boatlightHandle = gameRenderWorld->AddLightDef(&boatlight);

	idVec3 up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &up);
	DoTetherWire(GetPhysics()->GetOrigin() + up * 25, 0);
	DoTetherWire(GetPhysics()->GetOrigin() + up * -25, 1);

	EquipPrimarySlot();
	EquipSecondarySlot();
	EquipTertiarySlot();

	//for some reason, solid carepackage causes frob cursor problems
	GetPhysics()->SetContents(0);
}

void idCarepackage::Think(void)
{
	if (state == CPK_MINIPACKAGE_STATIC)
	{
		if (gameLocal.time > stateTimer)
		{
			//packageMini->GetPhysics()->Activate();			
			state = CPK_MINIPACKAGE_AIRBORNE;
			stateTimer = gameLocal.time + MINIPACKAGE_DEPLOYDELAY;
		}
	}
	else if (state == CPK_MINIPACKAGE_AIRBORNE)
	{
		if (gameLocal.time > stateTimer)
		{
			state = CPK_PACKAGE_DEPLOYED;
			
			idVec3 packagePos = packageMini->GetPhysics()->GetOrigin();
			idMat3 packageRot = packageMini->GetPhysics()->GetAxis();
			packageMini->GetPhysics()->SetContents(0);
			packageMini->Hide();
			packageMini->PostEventMS(&EV_Remove, 0); //delete the mini package.
			packageMini = nullptr;
			
			DeployPackage(packagePos, packageRot);
		}
	}

	//Update light position.
	if (boatlightHandle != -1)
	{
		idVec3 up;
		GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &up);
		boatlight.origin = GetPhysics()->GetOrigin() + up * 48;
		gameRenderWorld->UpdateLightDef(boatlightHandle, &boatlight);
	}

	//idMoveable::Think();
	
	Present();
}

//Detect when touch the ship, or touch the skybox.
bool idCarepackage::Collide(const trace_t &collision, const idVec3 &velocity)
{
	return Collide(collision, velocity);
}

void idCarepackage::SpawnDebrisPanels()
{
	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	
	idVec3 positionArray[] =
	{
		idVec3(10,0,-3),
		idVec3(0,10,-3),
		idVec3(-10,0,-3),
		idVec3(0,-10,-3)
	};

	#define PANELSPEED 48
	for (int i = 0; i < 4; i++)
	{
		idVec3 panelSpawnPos = GetPhysics()->GetOrigin() + (forward * positionArray[i].x) + (right * positionArray[i].y) + (up * positionArray[i].z);
		idMat3 panelRotation = GetPhysics()->GetAxis(); //TODO: make it oriented at correct angles.

		idVec3 force = panelSpawnPos - GetPhysics()->GetOrigin();
		force.Normalize();
		force = force * PANELSPEED;

		idVec3 angular = idVec3(gameLocal.random.RandomInt(-16, 16), gameLocal.random.RandomInt(-32, 32), 0);

		idEntity *debrisEnt;
		idDict args;
		args.Set("classname", spawnArgs.GetString("def_panel"));
		args.SetVector("origin", panelSpawnPos);
		args.SetMatrix("rotation", panelRotation);
		args.SetVector("velocity", force);
		args.SetVector("angular_velocity", angular);
		args.Set("skin", this->spawnArgs.GetString("skin", "")); // SW: components inherit the care package's skin
		gameLocal.SpawnEntityDef(args, &debrisEnt);
		if (debrisEnt)
		{
			if (debrisEnt->IsType(idDebris::Type))
			{
				static_cast<idDebris *>(debrisEnt)->Launch();
			}
		}
	}
		
}
