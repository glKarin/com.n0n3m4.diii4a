#include "sys/platform.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"

#include "framework/DeclEntityDef.h"

#include "bc_meta.h"
#include "bc_upgradecargo.h"




CLASS_DECLARATION(idStaticEntity, idUpgradecargo)
END_CLASS

idUpgradecargo::idUpgradecargo(void)
{
	fl.takedamage = false;

	gameLocal.upgradecargoEntities.Append(this);
}

idUpgradecargo::~idUpgradecargo(void)
{
	
}

void idUpgradecargo::Spawn(void)
{

	upgradeDef = NULL;
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
	state = UC_LOCKED;
	
	//this->GetRenderEntity()->gui[0] = uiManager->FindGui(spawnArgs.GetString("gui"), true, true); //Create a UNIQUE gui so that it doesn't auto sync with other guis.

	//this->Event_SetGuiParm("gui_status", "LOCKED");

	idDict args;
	args.Clear();
	args.SetInt("solid", 1);
	args.Set("model", spawnArgs.GetString("lockmodel"));
	lockplateEnt = (idStaticEntity *)gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
	lockplateEnt->SetOrigin(this->GetPhysics()->GetOrigin());
	lockplateEnt->SetAxis(this->GetPhysics()->GetAxis());

	if (lockplateEnt == NULL)
	{
		gameLocal.Error("failed to spawn lockmodel for idUpgradecargo\n");
	}
}

void idUpgradecargo::Save(idSaveGame *savefile) const
{
	savefile->WriteEntityDef( upgradeDef ); // const idDeclEntityDef * upgradeDef
	savefile->WriteInt( state ); // int state
	savefile->WriteObject( lockplateEnt ); // idEntity * lockplateEnt
}

void idUpgradecargo::Restore(idRestoreGame *savefile)
{
	savefile->ReadEntityDef( upgradeDef ); // const idDeclEntityDef * upgradeDef
	savefile->ReadInt( state ); // int state
	savefile->ReadObject( lockplateEnt ); // idEntity * lockplateEnt
}

void idUpgradecargo::Think(void)
{
	idEntity::Think();
}


void idUpgradecargo::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{	
	//take damage. meow!!!
}

//interacted with the GUI.
void idUpgradecargo::DoGenericImpulse(int index)
{
	if (index == 0)
	{
		//pressed the unlock button.
		
	}
}

bool idUpgradecargo::IsLocked()
{
	return (state == UC_LOCKED);
}

bool idUpgradecargo::IsAvailable()
{
	return (state == UC_AVAILABLE);
}

//This is what is called when the upgradecargo is made available to interact with. This is called ONCE.
void idUpgradecargo::SetAvailable()
{
	state = UC_AVAILABLE;
	lockplateEnt->PostEventMS(&EV_Remove, 0);
	lockplateEnt = nullptr;



	//BC disabling this for now, until we find a way to make the occupied baddie be more of a threat
	return;


	//Find a suitable space to spawn the interestpoint.
// 	idVec3 forward, forwardPos;
// 	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
// 	trace_t downTr;
// 	forwardPos = this->GetPhysics()->GetOrigin() + forward * 32;
// 	gameLocal.clip.TracePoint(downTr, forwardPos, forwardPos + idVec3(0, 0, -72), MASK_SOLID, NULL);
// 	
// 	if (downTr.fraction >= 1.0f)
// 		return; //Wasn't able to find a piece of floor..... this should never happen.
// 
// 	//spawn the idletask.
// 	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SpawnIdleTask(this, downTr.endpos, "idletask_upgradecargo");




	//idVec3 forward, up;
	//this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
	//gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin() + (forward * 8) + (up * 12), "interest_upgradecargo");
}

void idUpgradecargo::SetDormant()
{
	state = UC_DORMANT;
	Event_GuiNamedEvent(1, "onClaim");
}

bool idUpgradecargo::IsDormant()
{
	return (state == UC_DORMANT);
}

bool idUpgradecargo::SetInfo(const char *upgradeName)
{
	upgradeDef = gameLocal.FindEntityDef(upgradeName, false);

	if (!upgradeDef)
	{
		return false;
	}

	idDict upgradeDict = upgradeDef->dict;
	const char *displayname = upgradeDict.GetString("displayname", "");

	if (displayname[0] == '\0') //check if string is empty.
	{
		gameLocal.Error("upgrade def '%s' has no displayname.", upgradeName);
	}

	this->Event_SetGuiParm("gui_parm0", displayname);
	
	return true;
}

