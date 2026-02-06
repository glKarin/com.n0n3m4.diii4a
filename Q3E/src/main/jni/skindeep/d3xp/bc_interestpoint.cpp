#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "ai/AI.h"
#include "bc_gunner.h"
#include "bc_interestpoint.h"
#include "bc_meta.h"

CLASS_DECLARATION(idEntity, idInterestPoint)
END_CLASS



void idInterestPoint::Spawn(void)
{
	const char *typeName;
	
	priority = spawnArgs.GetInt("priority");
	noticeRadius = spawnArgs.GetInt("noticeRadius", "0");
	duplicateRadius = spawnArgs.GetInt("duplicateRadius", "0");
	sensoryTimer = gameLocal.time + SENSORY_UPDATETIME;
	isClaimed = false;
	claimant = NULL;
	forceCombat = spawnArgs.GetBool("forcecombat", "0");
	breaksConfinedStealth = spawnArgs.GetBool("breaks_confinedstealth", "0");
	cleanupWhenUnobserved = false;
	arrivalDistance = spawnArgs.GetInt("arrivaldistance", "80");
	creationTime = gameLocal.time;

	typeName = spawnArgs.GetString("type");
	if (!idStr::Icmp(typeName, "noise"))
	{
		int lifetime;		
		lifetime = spawnArgs.GetInt("lifetime");
		expirationTime = (lifetime > 0) ? gameLocal.time + lifetime : 0;

		interesttype = IPTYPE_NOISE;
	}
	else if (!idStr::Icmp(typeName, "visual"))
	{
		int lifetime;		
		lifetime = spawnArgs.GetInt("lifetime", "0");
		expirationTime = (lifetime > 0) ? gameLocal.time + lifetime : 0;

		interesttype = IPTYPE_VISUAL;		
	}
	else
	{
		gameLocal.Error("InterestPoint '%s' with invalid type: '%s'", GetName(), typeName);
		return;
	}

	onlyLocalPVS = spawnArgs.GetBool("onlyLocalPVS", "0");

	GetPhysics()->SetContents(0);
	GetPhysics()->SetClipMask(0);

	ownerDisplayName = "";	

	BecomeActive(TH_THINK);
}

idInterestPoint::idInterestPoint(void)
{
	interestNode.SetOwner(this);
	interestNode.AddToEnd(gameLocal.interestEntities);
}

idInterestPoint::~idInterestPoint(void)
{
	interestNode.Remove();
}

void idInterestPoint::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( priority ); // int priority

	savefile->WriteInt( noticeRadius ); // int noticeRadius
	savefile->WriteInt( duplicateRadius ); // int duplicateRadius

	savefile->WriteInt( interesttype ); // int interesttype
	savefile->WriteInt( sensoryTimer ); // int sensoryTimer
	savefile->WriteBool( isClaimed ); // bool isClaimed
	claimant.Save( savefile ); // idEntityPtr<idAI> claimant

	// idList<idEntityPtr<idAI>> observers
	int numObservers = observers.Num();
	savefile->WriteInt(numObservers);
	for (int i = 0; i < numObservers; i++) {
		observers[i].Save(savefile);
	}

	savefile->WriteBool( cleanupWhenUnobserved ); // bool cleanupWhenUnobserved
	savefile->WriteBool( forceCombat ); // bool forceCombat
	savefile->WriteBool( onlyLocalPVS ); // bool onlyLocalPVS
	savefile->WriteObject( interestOwner ); // idEntityPtr<idEntity> interestOwner


	savefile->WriteBool( breaksConfinedStealth ); // bool breaksConfinedStealth


	savefile->WriteInt( arrivalDistance ); // int arrivalDistance


	savefile->WriteInt( expirationTime ); // int expirationTime

	savefile->WriteString( ownerDisplayName ); // idString ownerDisplayName

	savefile->WriteInt( creationTime ); // int creationTime
}

void idInterestPoint::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( priority ); // int priority

	savefile->ReadInt( noticeRadius ); // int noticeRadius
	savefile->ReadInt( duplicateRadius ); // int duplicateRadius

	savefile->ReadInt( interesttype ); // int interesttype
	savefile->ReadInt( sensoryTimer ); // int sensoryTimer
	savefile->ReadBool( isClaimed ); // bool isClaimed
	claimant.Restore( savefile ); // idEntityPtr<idAI> claimant

	if (savefile->GetSaveVersion() < SAVEGAME_VERSION_0004) {
		idList<idAI*> observersTemp;
		SaveFileReadListCast(observersTemp, ReadObject, idClass*&); // idList<idAI*> observers
		for (int i = 0; i < observersTemp.Num(); i++) {
			idEntityPtr<idAI> observer;
			observer = observersTemp[i];
			observers.Append(observer);
		}
	} else {
		int numObservers = 0;
		savefile->ReadInt(numObservers);
		observers.SetNum(numObservers);
		for (int i = 0; i < numObservers; i++) {
			observers[i].Restore(savefile);
		}
	}

	savefile->ReadBool( cleanupWhenUnobserved ); // bool cleanupWhenUnobserved
	savefile->ReadBool( forceCombat ); // bool forceCombat
	savefile->ReadBool( onlyLocalPVS ); // bool onlyLocalPVS
	savefile->ReadObject( interestOwner ); // idEntityPtr<idEntity> interestOwner


	savefile->ReadBool( breaksConfinedStealth ); // bool breaksConfinedStealth


	savefile->ReadInt( arrivalDistance ); // int arrivalDistance


	savefile->ReadInt( expirationTime ); // int expirationTime

	savefile->ReadString( ownerDisplayName ); // idString ownerDisplayName

	savefile->ReadInt( creationTime ); // int creationTime
}

void idInterestPoint::Think()
{
	if (isClaimed)
	{
		Present();
	}

	if (ai_showInterestPoints.GetInteger() == 1)
	{
		//debug.
		idStr intName;
		int arrowLength = 48;
		idVec4 debugcolor = (isClaimed ? colorGreen : colorRed);

		intName = this->GetName();
		if (intName.Find("idinterestpoint", false) == 0)
		{
			intName = intName.Right(intName.Length() - 16);
		}

		gameRenderWorld->DebugArrow(debugcolor, this->GetPhysics()->GetOrigin() + idVec3(0, 0, arrowLength), this->GetPhysics()->GetOrigin(), 4, 100);
		gameRenderWorld->DrawText(idStr::Format("%s", intName.c_str()), this->GetPhysics()->GetOrigin() + idVec3(0, 0, arrowLength + 16), .1f, debugcolor, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 100);
		gameRenderWorld->DrawText(idStr::Format("priority: %d", this->priority), this->GetPhysics()->GetOrigin() + idVec3(0, 0, arrowLength + 12), .1f, debugcolor, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 100);

		idStr ownername;
		if (this->GetPhysics()->GetClipModel()->GetOwner() == NULL)
			ownername = "none";
		else
			ownername = this->GetPhysics()->GetClipModel()->GetOwner()->GetName();
		gameRenderWorld->DrawText(idStr::Format("owner: %s", ownername.c_str()), this->GetPhysics()->GetOrigin() + idVec3(0, 0, arrowLength + 8), .1f, debugcolor, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 100);


		if (this->claimant.IsValid())
			gameRenderWorld->DebugLine(colorWhite, this->GetPhysics()->GetOrigin(), this->claimant.GetEntity()->GetPhysics()->GetOrigin(), 4);
		for (int i = 0; i < observers.Num(); i++)
		{
			if (observers[i].IsValid())
			{
				gameRenderWorld->DebugLine(debugcolor * 0.25, this->GetPhysics()->GetOrigin(), observers[i].GetEntity()->GetPhysics()->GetOrigin(), 4);
			}
		}
	}
}

void idInterestPoint::SetClaimed(bool value, idAI* _claimant)
{
	this->isClaimed = value;
	if (value)
	{
		this->claimant = _claimant;
	}
	else
	{
		this->claimant = NULL;
	}
}

int idInterestPoint::GetExpirationTime(void)
{
	return expirationTime;
}

void idInterestPoint::AddObserver(idAI* observer)
{
	idEntityPtr<idAI> ptr;
	ptr = observer;
	observers.Append(ptr);
}

void idInterestPoint::RemoveObserver(idAI* observer)
{
	idEntityPtr<idAI> ptr;
	ptr = observer;
	observers.Remove(ptr);
}

bool idInterestPoint::HasObserver(idAI* observer)
{
	idEntityPtr<idAI> ptr;
	ptr = observer;
	return observers.Find(ptr) != NULL;
}

void idInterestPoint::ClearObservers(void)
{
	observers.Clear();
}

void idInterestPoint::SetOwnerDisplayName(idStr _name)
{
	ownerDisplayName = _name;
}

idStr idInterestPoint::GetOwnerDisplayName()
{
	return ownerDisplayName;
}

idStr idInterestPoint::GetHUDName()
{
	if ( displayName.Length() <= 0 )
	{
		//If interestpoint def has no displayname, then fall back to see if the interestpoint's owner has a displayname.
		return GetOwnerDisplayName();
	}

	return displayName;
}

int idInterestPoint::GetCreationTime()
{
	return creationTime;
}