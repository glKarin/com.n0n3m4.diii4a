#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "Trigger.h"

#include "bc_radiocheckin.h"
#include "bc_idletask.h"
#include "bc_meta.h"
#include "bc_walkietalkie.h"

const int RADAR_UPDATETIME = 100; //how often to update radar display.
const int RADAR_MAXLOCMARKERS = 10; //max amount of markers on radar. This is set via the loc* entries in the walkie talkie gui.
const int RADAR_MAXRANGE = 500;

const int RADAR_VERTICALRADIUS_ABOVE = 96;
const int RADAR_VERTICALRADIUS_BELOW = 127;

const float RADAR_DRAWTHRESHOLD = .95f; //position is drawn -1.0 - 1.0 on the radar display. This is the threshold for drawing the dot or not.

const int BATTERYMAX = 3;

const float DEADBATTERYGLOWCOLOR = .6f;

CLASS_DECLARATION(idMoveableItem, idWalkietalkie)
END_CLASS

idWalkietalkie::idWalkietalkie(void)
{
	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;

	walkietalkieState = 0;
	timer = 0;

	lureAvailable = false;

	visualTimer = 0;
	radarUpdateTimer = 0;
	unitsDrawOnRadar = 0;

	buttonDisplayState = 0;

	flagModel = nullptr;

	batteryAmount = 0;

}

idWalkietalkie::~idWalkietalkie(void)
{
	if (headlightHandle != -1)
		gameRenderWorld->FreeLightDef(headlightHandle);

	if (flagModel)
	{
		delete flagModel;
	}
}


void idWalkietalkie::Spawn(void)
{
	//Spawn flag.
	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	idVec3 flagPos = GetPhysics()->GetOrigin() + (forward * 1.5f) + (right * 1.7f) + (up * 6);

	idDict args;
	args.Clear();
	args.SetVector("origin", flagPos);
	args.SetMatrix("rotation", GetPhysics()->GetAxis());
	args.SetBool("hide", true);
	args.Set("model", spawnArgs.GetString("model_flag"));
	args.Set("bind", this->GetName());
	flagModel = (idAnimatedEntity*)gameLocal.SpawnEntityType(idAnimatedEntity::Type, &args);

	walkietalkieState = WTK_IDLE;
	timer = 0;	
	lureAvailable = true;
	visualTimer = 0;
	//showItemLine = true;

	UpdateButtonVisualStates();

	this->GetRenderEntity()->gui[0] = uiManager->FindGui(spawnArgs.GetString("gui"), true, true); //Create a UNIQUE gui so that it doesn't auto sync with other guis.

	//Radar
	radarUpdateTimer = 0;
	unitsDrawOnRadar = 0;

	buttonDisplayState = WB_UNITIALIZED;

	// SW 24th Feb 2025: walkie-talkie can spawn with less than maximum battery,
	// if it is a dupe of another walkie-talkie
	batteryAmount = spawnArgs.GetInt("initialbattery", "3");

	Event_SetGuiInt("batterycount", batteryAmount);

	//gameRenderWorld->DebugArrowSimple(flagPos);
}

void idWalkietalkie::Save(idSaveGame *savefile) const
{
	savefile->WriteRenderLight( headlight ); //  renderLight_t headlight
	savefile->WriteInt( headlightHandle ); //  int headlightHandle

	savefile->WriteInt( walkietalkieState ); //  int walkietalkieState
	savefile->WriteInt( timer ); //  int timer

	savefile->WriteBool( lureAvailable ); //  bool lureAvailable

	savefile->WriteInt( visualTimer ); //  int visualTimer
	savefile->WriteInt( radarUpdateTimer ); //  int radarUpdateTimer
	savefile->WriteInt( unitsDrawOnRadar ); //  int unitsDrawOnRadar

	savefile->WriteInt( buttonDisplayState ); //  int buttonDisplayState

	savefile->WriteObject( flagModel ); //  idAnimatedEntity* flagModel

	savefile->WriteInt( batteryAmount ); //  int batteryAmount
}

void idWalkietalkie::Restore(idRestoreGame *savefile)
{
	savefile->ReadRenderLight( headlight ); //  renderLight_t headlight
	savefile->ReadInt( headlightHandle ); //  int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadInt( walkietalkieState ); //  int walkietalkieState
	savefile->ReadInt( timer ); //  int timer

	savefile->ReadBool( lureAvailable ); //  bool lureAvailable

	savefile->ReadInt( visualTimer ); //  int visualTimer
	savefile->ReadInt( radarUpdateTimer ); //  int radarUpdateTimer
	savefile->ReadInt( unitsDrawOnRadar ); //  int unitsDrawOnRadar

	savefile->ReadInt( buttonDisplayState ); //  int buttonDisplayState

	savefile->ReadObject( CastClassPtrRef( flagModel ) ); //  idAnimatedEntity* flagModel

	savefile->ReadInt( batteryAmount ); //  int batteryAmount
}

bool idWalkietalkie::DoBatteryLogic()
{
	if (batteryAmount <= 0)
	{
		//No more battery left.
		Event_GuiNamedEvent(1, "battery_error");
		return false;
	}

	batteryAmount--;
	Event_SetGuiInt("batterycount", batteryAmount);

	if (batteryAmount <= 0 && headlightHandle != -1)
	{
		//Make the ambient glow be red.
		headlight.shaderParms[0] = DEADBATTERYGLOWCOLOR;
		headlight.shaderParms[1] = 0;
		headlight.shaderParms[2] = 0;
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
	}

	return true;
}

bool idWalkietalkie::DoFrob(int index, idEntity * frobber)
{
	if (index == CARRYFROB_INDEX && frobber == gameLocal.GetLocalPlayer())
	{
		if (walkietalkieState == WTK_PLAYERSPEAKING)
		{
			//Player is already talking. Frob does nothing here.........
			return true;
		}

		if (walkietalkieState != WTK_IDLE)
			return true;

		//if the radio checkin is in progress, allow player to do the fake radio checkin.
		bool radiocheckinStatic = static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->radioCheckinEnt->GetPlayingStatic();
		if (radiocheckinStatic)
		{
			if (DoBatteryLogic())
			{
				//Great, do the fake radio checkin.
				static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->radioCheckinEnt->DoPlayerFakeCheckin();
				Event_GuiNamedEvent(1, "search_use");
				return true;
			}
			else
			{
				//No battery left :(
				return false;
			}
		}

		//special player frob when player is carrying the item.
		int worldState = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetCombatState();

		//if (worldState == COMBATSTATE_COMBAT)
		//{
		//	StartSound("snd_cancel", SND_CHANNEL_ANY);
		//	Event_GuiNamedEvent(1, "combat_error");
		//	return true;
		//}
		if (worldState == COMBATSTATE_SEARCH || worldState == COMBATSTATE_COMBAT)
		{
			int walkieVoDuration;

			if (unitsDrawOnRadar > 0)
			{
				if (batteryAmount <= 0)
				{
					//No more battery left.
					Event_GuiNamedEvent(1, "battery_error");
					return false;
				}

				//Someone is nearby. Do not do the check in.
				StartSound("snd_cancel", SND_CHANNEL_ANY);
				Event_GuiNamedEvent(1, "nearby_error");
				return true;
			}
			else if (batteryAmount <= 0) //BC 4-16-2025: fix logic with being able to use walkietalkie with zero battery.
			{
				//No more battery left.
				Event_GuiNamedEvent(1, "battery_error");
				return false;
			}
			else if (static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->StartPlayerWalkietalkieSequence(&walkieVoDuration) && DoBatteryLogic())
			{
				//Using walkie talkie during WORLD SEARCH state. End search state.
				walkietalkieState = WTK_PLAYERSPEAKING;
				timer = gameLocal.time + walkieVoDuration + ALLCLEAR_GAP_DURATION;
				Event_GuiNamedEvent(1, "search_use");
			}
			
		}
		else
		{
			//Not in search state.
			StartSound("snd_cancel", SND_CHANNEL_ANY);
			Event_GuiNamedEvent(1, "search_error");
			return true;
		}

		return true;
	}	


	return idMoveableItem::DoFrob(index, frobber);	
}

void idWalkietalkie::Think(void)
{
	//Update renderlight.
	if (!fl.hidden)
	{
		if (headlightHandle != -1)
		{
			//Update light position.
			idVec3 forward;
			GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
			headlight.origin = GetPhysics()->GetOrigin() + (forward * 4);
			gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
		}
		else
		{
			headlight.shader = declManager->FindMaterial("lights/defaultProjectedLight", false);
			headlight.pointLight = true;
			headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = spawnArgs.GetFloat("glow_radius", "24");

			

			if (batteryAmount > 0)
			{
				//Has battery. Yellow ambient glow.				
				headlight.shaderParms[0] = .3f;
				headlight.shaderParms[1] = .3f;
				headlight.shaderParms[2] = 0;
			}
			else
			{
				//No more battery.
				headlight.shaderParms[0] = DEADBATTERYGLOWCOLOR;
				headlight.shaderParms[1] = 0;
				headlight.shaderParms[2] = 0;
			}

			headlight.shaderParms[3] = 1.0f;
			headlight.noShadows = true;
			headlight.isAmbient = false;
			headlight.axis = mat3_identity;
			headlightHandle = gameRenderWorld->AddLightDef(&headlight);
		}

		//Show the key prompt for the alert end button.
		renderEntity.gui[0]->SetStateString("attack_key", gameLocal.GetKeyFromBinding("_attack"));
		
		//Handle the radar.
		UpdateRadar();
	}

	if (walkietalkieState == WTK_PLAYERSPEAKING)
	{
		//if player swaps away from the walkie talkie, then fail the walkie talkie sequence.
		if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
		{
			if (gameLocal.GetLocalPlayer()->GetCarryable() != this)
			{
				InterruptWalkietalkie();
				
			}
		}
		else if (gameLocal.GetLocalPlayer()->GetCarryable() == NULL)
		{
			InterruptWalkietalkie();
		}

		


		if (gameLocal.time >= timer && walkietalkieState == WTK_PLAYERSPEAKING)
		{
			//player is done speaking into walkietalkie.			

			Event_GuiNamedEvent(1, "allclear_success");

			walkietalkieState = WTK_IDLE;

			//walkietalkieState = WTK_EXHAUSTED;
			//
			////Kill the light.
			//if (headlightHandle != -1)
			//{
			//	gameRenderWorld->FreeLightDef(headlightHandle);
			//	headlightHandle = -1;
			//}
		}
	}

	UpdateButtonVisualStates();

	idMoveableItem::Think();
}

void idWalkietalkie::InterruptWalkietalkie()
{
	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetAllClearInterrupt();
	walkietalkieState = WTK_IDLE;
	StartSound("snd_interrupt", SND_CHANNEL_ANY);

	gameLocal.GetLocalPlayer()->SetCenterMessage("#str_def_gameplay_walkie_interrupt");
}

void idWalkietalkie::UpdateRadar()
{
	//We update the radar at set intervals. Not every frame.
	if (gameLocal.time < radarUpdateTimer)
		return;

	radarUpdateTimer = gameLocal.time + RADAR_UPDATETIME;

	int itemsMarked = 0;

	//Iterate over all the bad guys.
	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity->IsActive() || entity->IsHidden() || entity->team != TEAM_ENEMY || entity->health <= 0)
			continue;

		if (!entity->IsType(idAI::Type))
			continue;

		if (!entity->spawnArgs.GetBool("can_talk", "1"))
			continue;

		//distance check.
		float distanceToEnemy = (entity->GetPhysics()->GetOrigin() - idVec3(gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().x, gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().y, entity->GetPhysics()->GetOrigin().z)).Length();
		if (distanceToEnemy > RADAR_MAXRANGE)
			continue;

		int verticalDelta = idMath::Abs(gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().z - entity->GetPhysics()->GetOrigin().z);

		//enemy dot is below me.
		if (gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().z > entity->GetPhysics()->GetOrigin().z && verticalDelta > RADAR_VERTICALRADIUS_BELOW)
			continue;

		//enemy dot is above me.
		if (gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().z < entity->GetPhysics()->GetOrigin().z && verticalDelta > RADAR_VERTICALRADIUS_BELOW)
			continue;
		

		idVec3 dirToEnemy = entity->GetPhysics()->GetOrigin() - idVec3(gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().x, gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().y, entity->GetPhysics()->GetOrigin().z);
		dirToEnemy.Normalize();
		float finalAngle = atan2(dirToEnemy.x, dirToEnemy.y) * -1.0f;
		finalAngle -= DEG2RAD(gameLocal.GetLocalPlayer()->viewAngles.yaw );

		float normalizedDistance = distanceToEnemy / (float)RADAR_MAXRANGE;
		normalizedDistance *= 1.3f; //because the radar is not circle-shaped, we extend this value a bit so that it can reach into the rectangle shape's corners.

		idVec2 drawPosition = vec2_zero;
		drawPosition.x -= cos(finalAngle ) * normalizedDistance;
		drawPosition.y += sin(finalAngle ) * normalizedDistance;

		//because we're detecting the units in a sphere, but the radar display is a rectangle, we do a check here to see if the unit should be drawn or not.
		if (idMath::Fabs(drawPosition.x) > RADAR_DRAWTHRESHOLD || idMath::Fabs(drawPosition.y) > RADAR_DRAWTHRESHOLD)
			continue;
		
		//Mark position on map.
		Event_SetGuiFloat(idStr::Format("loc%d_x", itemsMarked), drawPosition.x);
		Event_SetGuiFloat(idStr::Format("loc%d_y", itemsMarked), drawPosition.y);
		Event_SetGuiInt(idStr::Format("loc%d_vis", itemsMarked), 1); //make the marker visible.
		itemsMarked++;

		if (itemsMarked > RADAR_MAXLOCMARKERS)
		{
			//Uh oh. too many elements for the map gui to handle. Exit here.
			return;
		}
	}

	if (itemsMarked < RADAR_MAXLOCMARKERS)
	{
		for (int i = itemsMarked; i < RADAR_MAXLOCMARKERS; i++)
		{
			Event_SetGuiInt(idStr::Format("loc%d_vis", i), 0); //make the marker hidden.
		}
	}

	Event_SetGuiInt("unitcount", itemsMarked);
	unitsDrawOnRadar = itemsMarked;
}


/*
bool idWalkietalkie::LureEnemy()
{
	//Find closest enemy to player.
	float closestDistance = 99999;
	idAI *enemyToLure = NULL;
	for (idEntity *ent = gameLocal.aimAssistEntities.Next(); ent != NULL; ent = ent->aimAssistNode.Next())
	{
		if (!ent || ent->fl.hidden || ent->fl.isDormant || ent->health <= 0 || ent->team != TEAM_ENEMY || !ent->IsType(idAI::Type))
		{
			continue;
		}

		float dist = (ent->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).LengthFast();
		if (dist < closestDistance)
		{
			closestDistance = dist;
			enemyToLure = static_cast<idAI *>(ent);
		}
	}

	if (enemyToLure == NULL)
		return false;

	idEntity *taskEnt = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SpawnIdleTask(NULL,
		gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin(), "idletask_lure"); //spawn the idletask.

	if (!taskEnt)
		return false;

	if (!taskEnt->IsType(idIdleTask::Type))
		return false;

	bool pathingSuccess = static_cast<idIdleTask *>(taskEnt)->SetActor(enemyToLure);
	if (!pathingSuccess)
	{
		taskEnt->PostEventMS(&EV_Remove, 0);
		return false;
	}
		
	//Success.
	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetWalkietalkieLure(taskEnt);
	return true;
}
*/


//Handle logic for disabling/enabling buttons depending on world state.
//This just polls the world every xxx milliseconds.
void idWalkietalkie::UpdateButtonVisualStates()
{
	if (visualTimer > gameLocal.time || gameLocal.time < 1000)
		return;

	visualTimer = gameLocal.time + 300;


	bool radiocheckinStatic = static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->radioCheckinEnt->GetPlayingStatic();
	int worldState = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetCombatState();

	if (worldState == COMBATSTATE_SEARCH || worldState == COMBATSTATE_COMBAT || radiocheckinStatic)
	{
		//If world is in search/combat state or if radio checkin is waiting for player to check in...
		if (buttonDisplayState != WB_ENABLED)
		{
			buttonDisplayState = WB_ENABLED;
			Event_GuiNamedEvent(1, "search_enable");

			if (!IsHidden())
				flagModel->Show();

			flagModel->Event_PlayAnim("deploy", 1); //deploy flag model.
		}
	}
	else
	{
		if (buttonDisplayState != WB_GREYED)
		{
			buttonDisplayState = WB_GREYED;
			Event_GuiNamedEvent(1, "search_disable");

			flagModel->Event_PlayAnim("undeploy", 1); //un-deploy flag model.
		}
	}
}




//bool idWalkietalkie::Collide(const trace_t &collision, const idVec3 &velocity)
//{
//}

//void idWalkietalkie::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
//{
//	idMoveableItem::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);	
//}

void idWalkietalkie::Show(void)
{
	idMoveableItem::Show();

	if (buttonDisplayState == WB_ENABLED)
	{
		flagModel->Show();
	}
}

void idWalkietalkie::Hide(void)
{
	idMoveableItem::Hide();

	if (!flagModel->IsHidden())
	{
		flagModel->Hide();
	}

	if (headlightHandle != -1)
	{
		gameRenderWorld->FreeLightDef(headlightHandle);
		headlightHandle = -1;
	}
}

int idWalkietalkie::GetBatteryAmount(void)
{
	return batteryAmount;
}
