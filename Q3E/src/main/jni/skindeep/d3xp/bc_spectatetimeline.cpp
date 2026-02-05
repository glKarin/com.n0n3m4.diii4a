
#include "Entity.h"
#include "Actor.h"
#include "Player.h"


#include "bc_spectatenode.h"
#include "bc_spectatetimeline.h"



CLASS_DECLARATION(idEntity, idSpectateTimeline)
END_CLASS

//This handles the spawning of spectate timeline.

idSpectateTimeline::idSpectateTimeline()
{
	rolloverDebounceIdx = -1;
}

idSpectateTimeline::~idSpectateTimeline()
{
}

void idSpectateTimeline::Spawn(void)
{
	if (gameLocal.eventLogList.Num() <= 0)
		return; //no events. nothing to do here.

	for (int i = 0; i < gameLocal.eventLogList.Num(); i++)
	{
		if (gameLocal.eventLogList[i].eventType == EL_DEATH || gameLocal.eventLogList[i].eventType == EL_DESTROYED)
		{
			//Spawn an event node.

			idEntity* newNode = NULL;
			idDict args;
			args.Set("classname", gameLocal.eventLogList[i].eventType == EL_DEATH ? "env_spectatenode" : "env_spectatenode_destroy");
			args.SetVector("origin", gameLocal.eventLogList[i].position);
			args.Set("text", gameLocal.eventLogList[i].name);
			args.SetInt("time", gameLocal.eventLogList[i].timestamp);
			gameLocal.SpawnEntityDef(args, &newNode);
			if (newNode)
			{
				newNode->Think(); //we force it to do one think cycle in order to make the entity appear; since the game is paused during spectatormode.
			}
		}
	}
}

void idSpectateTimeline::Save(idSaveGame* savefile) const
{
	savefile->WriteInt( rolloverDebounceIdx ); // int rolloverDebounceIdx
}
void idSpectateTimeline::Restore(idRestoreGame* savefile)
{
	savefile->ReadInt( rolloverDebounceIdx ); // int rolloverDebounceIdx
}

void idSpectateTimeline::Think(void)
{
	//Determine which, if any, spectatenode the player is looking at.

	const idVec2 MIN_SCREEN(270.0f, 190.0f); //Determines how centered the node has to be to draw the label.
	const idVec2 MAX_SCREEN = idVec2(640.0f, 480.0f) - MIN_SCREEN;
	const idVec2 CENTER_SCREEN(320.0f, 240.0f);

	idEntity* spectatenodeEnt = nullptr;
	float bestDist = 10000.0f;
	for (idEntity* ent = gameLocal.spectatenodeEntities.Next(); ent != NULL; ent = ent->spectatenodeNode.Next())
	{
		if (!ent)
			continue;

		//iterate through all spectate nodes.
		if (ent->IsHidden())
			continue;

		idVec3 labelWorldPos = ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 17);

		idVec3 dirToEnt = labelWorldPos - gameLocal.GetLocalPlayer()->firstPersonViewOrigin;
		float facingResult = DotProduct(dirToEnt, gameLocal.GetLocalPlayer()->viewAngles.ToForward());
		if (facingResult < 0)
		{
			continue;
		}

		

		idVec2 screenPos = gameLocal.GetLocalPlayer()->GetWorldToScreen(labelWorldPos);
		if (screenPos.x >= MIN_SCREEN.x && screenPos.y >= MIN_SCREEN.y &&
			screenPos.x <= MAX_SCREEN.x && screenPos.y <= MAX_SCREEN.y)
		{
			screenPos -= CENTER_SCREEN;
			float dist = screenPos.Length();
			if (dist < bestDist)
			{
				//check los
				if (!hasLOS(ent))
					continue;

				spectatenodeEnt = ent;
			}
		}
	}

	if (spectatenodeEnt != nullptr)
	{
		if (spectatenodeEnt->IsType(idSpectateNode::Type))
		{
			static_cast<idSpectateNode*>(spectatenodeEnt)->Draw();
		}

		if (spectatenodeEnt->entityNumber != rolloverDebounceIdx)
		{
			rolloverDebounceIdx = spectatenodeEnt->entityNumber;
			gameLocal.GetLocalPlayer()->StartSound("snd_showenemyhealth", SND_CHANNEL_ANY);
		}
	}
	else
	{
		rolloverDebounceIdx = -1;
	}
}

bool idSpectateTimeline::hasLOS(idEntity* nodeEnt)
{
	//if (!gameLocal.InPlayerConnectedArea(nodeEnt))
	//	return false;

	//We do a trace to the origin AND the node icon so that we get the most chances possible to prevent false positives
	trace_t tr;
	gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->firstPersonViewOrigin, nodeEnt->GetPhysics()->GetOrigin(), MASK_SOLID, NULL);
	if (tr.fraction >= 1)
		return true;

	gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->firstPersonViewOrigin, nodeEnt->GetPhysics()->GetOrigin() + idVec3(0,0,17), MASK_SOLID, NULL);
	if (tr.fraction >= 1)
		return true;

	return false;
}

void idSpectateTimeline::ToggleAllNodes(bool enable)
{
	for (idEntity* ent = gameLocal.spectatenodeEntities.Next(); ent != NULL; ent = ent->spectatenodeNode.Next())
	{
		if (!ent)
			continue;

		if (enable)
			ent->Show();
		else
			ent->Hide();

		ent->UpdateVisuals();
		ent->Think();
	}
}
