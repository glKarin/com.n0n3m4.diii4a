#include "Player.h"
#include "WorldSpawn.h"
#include "bc_meta.h"
#include "bc_sabotagepoint.h"
#include "bc_ftl.h"
#include "framework/DeclEntityDef.h"
#include "bc_damagejet.h"
#include "Fx.h"

//This is for the secondary sabotage entity.

const int TIME_TO_ADD_TO_FTLCOUNTDOWN = 30000;

CLASS_DECLARATION(idAnimatedEntity, idSabotagePoint)
	EVENT(EV_Activate,	idSabotagePoint::DoFrob)
END_CLASS


void idSabotagePoint::Spawn( void )
{
	//make frobbable, shootable, and no player collision.
	GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
	GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );

	this->noGrab = true;
	this->isFrobbable = true;

	state = IDLE;

	StartSound("snd_ambient", SND_CHANNEL_AMBIENT);
}

void idSabotagePoint::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(state); // int state
}

void idSabotagePoint::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(state);  // int state
}



bool idSabotagePoint::DoFrob(int index, idEntity * frobber)
{
	if (state != IDLE)
	{
		//If already pressed, then do nothing.
		return false;
	}

	StopSound(SND_CHANNEL_AMBIENT);
	state = ARMED;

	Event_PlayAnim(spawnArgs.GetString("anim_press", "press"), 1);
	StartSound( "snd_press", SND_CHANNEL_BODY, 0, false, NULL );

	idMeta *meta = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity());
	if (meta)
	{
		idFTL *ftl = static_cast<idFTL *>(meta->GetFTLDrive.GetEntity());
		if (ftl)
		{
			int timeToAdd = SEC2MS(gameLocal.world->spawnArgs.GetInt("ftl_sabotage_increase"));
			ftl->AddToCountdown(timeToAdd);

			//Debug, deleteme
			idStr text = idStr::Format("+%d ADDED TO FTL COUNTDOWN. TOTAL TIME = %d",  (int)(timeToAdd / 1000.0f), (int)(ftl->GetTotalCountdowntime() / 1000.0f));
			idAngles ang = GetPhysics()->GetAxis().ToAngles();
			ang.yaw += 180;
			gameRenderWorld->DrawText(text.c_str(), GetPhysics()->GetOrigin() + idVec3(0,0,6), .06f, colorWhite, ang.ToMat3(), 1, 15000);
		}
	}

	SetColor(colorRed);
	
	return true;
}

void idSabotagePoint::Think( void )
{
	idAnimatedEntity::Think();
	idAnimatedEntity::Present();
}

bool idSabotagePoint::IsAvailable()
{
	return (state == IDLE);
}

CLASS_DECLARATION(idSabotagePoint, idSabotagePoint_SparkHazard)
END_CLASS

void idSabotagePoint_SparkHazard::Spawn(void)
{
	// Parsing spawnargs
	triggerRadius = spawnArgs.GetInt("trap_trigger_radius", "64");
	sparkJetDef = gameLocal.FindEntityDef(spawnArgs.GetString("trap_jetdef"), false);
	if (!sparkJetDef)
	{
		gameLocal.Error("'%s' cannot find trap_jetdef.", this->GetName());
		return;
	}
	// End spawnargs

	lastRadiusCheck = gameLocal.time;

	idSabotagePoint::Spawn();
}

void idSabotagePoint_SparkHazard::Save( idSaveGame* savefile ) const
{
	savefile->WriteInt( triggerRadius ); //  int triggerRadius
	savefile->WriteEntityDef( sparkJetDef ); // const  idDeclEntityDef*  sparkJetDef

	savefile->WriteInt( lastRadiusCheck ); //  int lastRadiusCheck
	savefile->WriteObject( armedFx ); //  idEntityFx*  armedFx
}

void idSabotagePoint_SparkHazard::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt( triggerRadius ); //  int triggerRadius
	savefile->ReadEntityDef( sparkJetDef ); // const idDeclEntityDef*  sparkJetDef

	savefile->ReadInt( lastRadiusCheck ); //  int lastRadiusCheck
	savefile->ReadObject( CastClassPtrRef( armedFx ) ); //  idEntityFx*  armedFx
}


bool idSabotagePoint_SparkHazard::DoFrob(int index, idEntity* frobber)
{
	bool result = idSabotagePoint::DoFrob(index, frobber);
	if (result) 
	{
		// We're now armed. Set up our interestpoint and some effects to communicate what's happening
		gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin() + this->GetPhysics()->GetAxis().ToAngles().ToForward() * 16, "interest_sabotagepoint_sparks");

		idDict args;
		args.Clear();
		args.SetVector("origin", this->GetPhysics()->GetOrigin());
		args.Set("fx", spawnArgs.GetString("trap_armedfx", "fx/sparks01"));
		args.SetFloat("restart", 3.0f);
		args.SetBool("triggered", false);
		args.SetBool("start", true);

		armedFx = (idEntityFx*)gameLocal.SpawnEntityType(idEntityFx::Type, &args);
	}
	return result;
}

void idSabotagePoint_SparkHazard::Think(void)
{
	if (state == ARMED)
	{
		// Avoid overthinking by spacing out our thinks.
		if (gameLocal.time - lastRadiusCheck > RADIUS_CHECK_TIME)
		{
			lastRadiusCheck = gameLocal.time;

			// Go through nearby entities and try to find an enemy
			int entityCount;
			idEntity* entityList[MAX_GENTITIES];
			entityCount = gameLocal.EntitiesWithinRadius(this->GetPhysics()->GetOrigin(), (float)triggerRadius, entityList, MAX_GENTITIES);

			for (int i = 0; i < entityCount; i++)
			{
				idEntity* ent = entityList[i];

				if (!ent || ent->IsHidden() || !ent->fl.takedamage || !ent->IsType(idAI::Type))
					continue;

				// Check that the AI is in front of the sabotage point
				idVec3 entityCenterMass = ent->GetPhysics()->GetAbsBounds().GetCenter();
				idVec3 dirToEntity = entityCenterMass - this->GetPhysics()->GetOrigin();
				dirToEntity.Normalize();
				float facingResult = DotProduct(dirToEntity, this->GetPhysics()->GetAxis().ToAngles().ToForward());

				if (facingResult > 0)
				{
					// Okay, our bad guy is in front of us. Spawn our damage jet.
					idDict args;

					args.Clear();
					args.SetVector("origin", this->GetPhysics()->GetOrigin());
					args.SetFloat("delay", sparkJetDef->dict.GetFloat("delay", ".3"));
					args.SetFloat("range", sparkJetDef->dict.GetFloat("range", "96"));
					args.Set("def_damage", sparkJetDef->dict.GetString("def_damage"));
					args.SetFloat("lifetime", sparkJetDef->dict.GetFloat("lifetime", "4"));
					args.Set("model_jet", sparkJetDef->dict.GetString("model_jet"));
					args.Set("snd_jet", sparkJetDef->dict.GetString("snd_jet"));
					args.Set("mtr_light", sparkJetDef->dict.GetString("mtr_light"));
					args.SetVector("lightcolor", sparkJetDef->dict.GetVector("lightcolor"));
					args.SetVector("dir", dirToEntity);
					args.SetInt("ownerIndex", this->entityNumber);

					gameLocal.SpawnEntityType(idDamageJet::Type, &args);

					// Our trap has been sprung and should exit the armed state
					state = EXHAUSTED;
					if (armedFx)
					{
						armedFx->PostEventMS(&EV_Remove, 0);
						armedFx = nullptr;
					}
				}
			}
		}
	}

	idSabotagePoint::Think();
}
