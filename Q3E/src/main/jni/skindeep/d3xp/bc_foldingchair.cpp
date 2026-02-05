#include "sys/platform.h"

#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Player.h"

#include "bc_frobcube.h"
#include "bc_foldingchair.h"

const int FROB_OFFTIME = 600; //UNfrobbable for XX seconds after frobbing.


CLASS_DECLARATION(idAnimatedEntity, idFoldingchair)
END_CLASS

idFoldingchair::idFoldingchair(void)
{
}

idFoldingchair::~idFoldingchair(void)
{
}

void idFoldingchair::Spawn(void)
{
	//Remove collision, but still allow frob/damage
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	fl.takedamage = true;
	state = FCH_CLOSED;

	//Spawn frobcube.
	idVec3 jointPos;
	idMat3 jointAxis;
	jointHandle_t jointHandle = GetAnimator()->GetJointHandle("handle");
	GetJointWorldTransform(jointHandle, gameLocal.time, jointPos, jointAxis);	
	idDict args;
	idVec3 upDir;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &upDir);
	args.Clear();
	args.Set("model", "models/objects/frobcube/cube4x4.ase");
	args.Set("displayname", "#str_gameplay_jumpseat");
	args.SetVector("origin", jointPos + (upDir * -2));
	frobCube = gameLocal.SpawnEntityType(idFrobcube::Type, &args);
	frobCube->GetPhysics()->GetClipModel()->SetOwner(this);
	//static_cast<idFrobcube*>(frobCube)->SetIndex(1);
	frobCube->BindToJoint(this, jointHandle, false);


	//The collision box that slides out when the seat opens.
	idEntity *ent;
	args.Clear();
	args.Set("model", spawnArgs.GetString("model_seat"));
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.SetMatrix("rotation", GetPhysics()->GetAxis());
	//args.SetFloat("time", .5f);
	args.Set("classname", "func_mover");
	gameLocal.SpawnEntityDef(args, &ent);
	if (ent)
	{
		seatMover = static_cast<idMover *>(ent);
		seatMover->Event_SetMoveTime(.3f);
		seatMover->Hide();
	}

	idVec3 forward;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
	seatPos_open = GetPhysics()->GetOrigin() + (forward * 26);
	
}


void idFoldingchair::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); //  int state
	savefile->WriteInt( stateTimer ); //  int stateTimer

	savefile->WriteObject( frobCube ); //  idEntity* frobCube

	savefile->WriteObject( seatMover ); //  idMover* seatMover
	savefile->WriteVec3( seatPos_open ); //  idVec3 seatPos_open
}

void idFoldingchair::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); //  int state
	savefile->ReadInt( stateTimer ); //  int stateTimer

	savefile->ReadObject( frobCube ); //  idEntity* frobCube

	savefile->ReadObject( CastClassPtrRef(seatMover) ); //  idMover* seatMover
	savefile->ReadVec3( seatPos_open ); //  idVec3 seatPos_open
}

void idFoldingchair::Think(void)
{
	if (gameLocal.time > stateTimer && !frobCube->isFrobbable)
	{
		frobCube->isFrobbable = true;
		BecomeInactive(TH_THINK);
	}
	
	idAnimatedEntity::Think();
}


bool idFoldingchair::DoFrob(int index, idEntity * frobber)
{
	if (state == FCH_CLOSED)
	{
		//Chair is closed. Open it.
		state = FCH_OPEN;
		Event_PlayAnim("open", 1);

		seatMover->SetOrigin(GetPhysics()->GetOrigin());
		seatMover->Show();
		seatMover->Event_MoveToPos(seatPos_open);

		//BC 2-19-2025: script call for when it goes closed -> opened
		gameLocal.RunMapScriptArgs(spawnArgs.GetString("call_opened"), frobber, this);
	}
	else
	{
		//Chair is open. Close it.
		state = FCH_CLOSED;
		Event_PlayAnim("close", 1);

		// SW 17th Feb 2025: Wake up any moveables sitting on the chair mover
		idEntity* entityList[32];
		idEntity* touchingEntity = NULL;
		int numEntities = gameLocal.EntitiesWithinAbsBoundingbox(seatMover->GetPhysics()->GetAbsBounds().Expand(1), entityList, 32);
		for (int i = 0; i < numEntities; i++)
		{
			touchingEntity = entityList[i];
			// For each entity, if it has valid physics and it's asleep, wake it up
			if (touchingEntity != NULL &&
				touchingEntity != this &&
				touchingEntity->IsType(idMoveableItem::Type) &&
				touchingEntity->GetPhysics() != NULL &&
				touchingEntity->GetPhysics()->IsAtRest())
			{
				touchingEntity->ActivatePhysics(this);
			}
		}

		seatMover->SetOrigin(GetPhysics()->GetOrigin());
		seatMover->Hide();

		//BC 2-19-2025: script call for when it goes open -> closed
		gameLocal.RunMapScriptArgs(spawnArgs.GetString("call_closed"), frobber, this);
	}

	// SW: scripting things being hidden in the seat, etc	
	gameLocal.RunMapScriptArgs(spawnArgs.GetString("call"), frobber, this);	
	//idStr scriptName = spawnArgs.GetString("call", "");
	//if (scriptName.Length() > 0)
	//{
	//	const function_t* scriptFunction;
	//	scriptFunction = gameLocal.program.FindFunction(scriptName);
	//	if (scriptFunction)
	//	{
	//		assert(scriptFunction->parmSize.Num() <= 2);
	//		idThread* thread = new idThread();
	//
	//		// 1 or 0 args always pushes activator (frobber)
	//		thread->PushArg(frobber);
	//		// 2 args pushes self as well
	//		if (scriptFunction->parmSize.Num() == 2)
	//		{
	//			thread->PushArg(this);
	//		}
	//
	//		thread->CallFunction(scriptFunction, false);
	//		thread->DelayedStart(0);
	//	}
	//}


	frobCube->isFrobbable = false;
	stateTimer = gameLocal.time + FROB_OFFTIME;
	BecomeActive(TH_THINK);

	return true;
}
