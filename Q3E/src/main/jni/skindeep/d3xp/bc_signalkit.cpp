#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
//#include "Light.h"
#include "Player.h"
//#include "Fx.h"
#include "framework/DeclEntityDef.h"
#include "bc_ftl.h"
#include "bc_glasspiece.h"

#include "bc_signalkit.h"


//TODO: Make the white UI line on item invisible.

const int MAX_GLASSPIECES = 3;


CLASS_DECLARATION(idStaticEntity, idSignalkit)
END_CLASS

idSignalkit::idSignalkit(void)
{
}

idSignalkit::~idSignalkit(void)
{
}

void idSignalkit::Spawn(void)
{
	state = SK_ALIVE;

	fl.takedamage = true;
	
	
	idVec3 forward;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
	idVec3 spawnPos = this->GetPhysics()->GetOrigin() + (forward * 5.0f); //spawn it in center.

	idAngles spawnAng = this->GetPhysics()->GetAxis().ToAngles();
	spawnAng.yaw += 90;

	//Spawn the item inside.
	idEntity *itemEnt;
	idDict args;
	args.SetVector("origin", spawnPos);
	args.SetFloat("angle", spawnAng.yaw);
	args.Set("classname", spawnArgs.GetString("spawnitem", "moveable_item_signallamp"));
	gameLocal.SpawnEntityDef(args, &itemEnt);


	isFrobbable = true;

	if (itemEnt)
	{		
		//itemEnt->GetPhysics()->SetOrigin(spawnPos);
		itemEnt->isFrobbable = false;

		if (itemEnt->IsType(idMoveableItem::Type))
		{
			static_cast<idMoveableItem *>(itemEnt)->SetItemlineActive(false);
			itemEnt->Bind(this, true);
		}

		myItem = itemEnt;
	}

	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	BecomeInactive(TH_THINK);
}



void idSignalkit::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteObject( myItem ); // idEntityPtr<idEntity> myItem
	savefile->WriteStaticObject( idSignalkit::physicsObj ); // idPhysics_RigidBody physicsObj
	bool restorePhysics = &physicsObj == GetPhysics();
	savefile->WriteBool( restorePhysics );
}

void idSignalkit::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadObject( myItem ); // idEntityPtr<idEntity> myItem
	savefile->ReadStaticObject( physicsObj ); // idPhysics_RigidBody physicsObj
	bool restorePhys;
	savefile->ReadBool( restorePhys );
	if (restorePhys)
	{
		RestorePhysics( &physicsObj );
	}
}

void idSignalkit::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	if (state == SK_BROKEN)
		return;

	state = SK_BROKEN;
	isFrobbable = false;
	SetColor(1, 0, 0);
	UpdateVisuals();
	health = 0;
    
	if (myItem.IsValid())
	{
		myItem.GetEntity()->isFrobbable = true;

		if (myItem.GetEntity()->IsType(idMoveableItem::Type))
		{
			static_cast<idMoveableItem *>(myItem.GetEntity())->SetItemlineActive(true);
			myItem.GetEntity()->Unbind();


			static_cast<idMoveableItem *>(myItem.GetEntity())->SetNextSoundTime(gameLocal.time + 1000);
		}
	}


	//Sound.
	StartSound("snd_shatter", SND_CHANNEL_ANY, 0, false, NULL);

	//Particle.
	idVec3 particlePos;
	idVec3 forward, right, up;
	idAngles particleAng;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	particleAng = this->GetPhysics()->GetAxis().ToAngles();
	particleAng.pitch += 80;
	particlePos = this->GetPhysics()->GetOrigin() + (forward * 16) + (up * -9);
	gameLocal.DoParticle(spawnArgs.GetString("smoke_shatter"), particlePos, particleAng.ToForward());

	//Glass pieces.
	
	const idDeclEntityDef *glassDef;
	glassDef = gameLocal.FindEntityDef("debris_glass", false);

	for (int i = 0; i < MAX_GLASSPIECES; i++)
	{
		idEntity *glassEnt;
		if (gameLocal.SpawnEntityDef(glassDef->dict, &glassEnt, false))
		{
			idVec3 glassPos = particlePos + (up * (-10 - (i*2))) + (right * (i*6));
			glassEnt->SetOrigin(glassPos); //TODO: add spawn position variation?

			if (glassEnt && glassEnt->IsType(idGlassPiece::Type))
			{
				idGlassPiece *glassDebris = static_cast<idGlassPiece *>(glassEnt);
				glassDebris->Create(glassPos, mat3_identity);
				glassDebris->GetPhysics()->SetAngularVelocity(idVec3(gameLocal.random.CRandomFloat() * 8, gameLocal.random.CRandomFloat() * 8, gameLocal.random.CRandomFloat() * 8));
				glassDebris->GetPhysics()->SetLinearVelocity(particleAng.ToForward() * (2 + gameLocal.random.RandomInt(4)));
				//glassDebris->GetPhysics()->SetGravity(vec3_zero);


				//gameRenderWorld->DebugSphere(colorGreen, idSphere(glassDebris->GetPhysics()->GetOrigin(), 1), 5000);
			}
		}
	}
	


	//Swap to collision that does not have glass.
	const char *brokenModel = spawnArgs.GetString("model_broken");

	if (brokenModel[0] != '\0')
	{
		SetModel(brokenModel);
		GetPhysics()->SetClipModel(new idClipModel(brokenModel), 1.0f);
	}
	else
	{
		gameLocal.Error("idSignalkit '%s' has no model_broken.", this->GetName());
	}
}


bool idSignalkit::DoFrob(int index, idEntity * frobber)
{
	//if (state == SK_BROKEN || frobber != gameLocal.GetLocalPlayer())
	if (state == SK_BROKEN)
		return false;

	Damage(frobber, frobber, vec3_zero, "damage_generic", 1.0f, 0, 0);
	return true;
}