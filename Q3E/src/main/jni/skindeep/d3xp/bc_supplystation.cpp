#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "WorldSpawn.h"
#include "Player.h"
#include "bc_meta.h"
#include "bc_ftl.h"

#include "bc_lifeboat.h"
#include "bc_supplystation.h"

//impulse numberS in the supply station GUI file.
#define INDEX_PURCHASE0		0
#define INDEX_PURCHASE1		1
#define INDEX_TAKEOFF		9

CLASS_DECLARATION(idStaticEntity, idSupplyStation)
END_CLASS

idSupplyStation::idSupplyStation(void)
{
}

idSupplyStation::~idSupplyStation(void)
{
}

void idSupplyStation::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);
	fl.takedamage = true;


	const char *ownerName = spawnArgs.GetString("podowner");
	idEntity *ownerEnt = (gameLocal.FindEntity(ownerName));
	if (ownerEnt != NULL)
	{
		podOwner = ownerEnt;
	}
	else
	{
		common->Warning("SupplyStation '%s' has invalid pod owner ('%s').", GetName(), ownerName);
	}


	BecomeActive(TH_THINK);
}

void idSupplyStation::Save(idSaveGame *savefile) const
{
	savefile->WriteObject( podOwner ); // idEntityPtr<idEntity> podOwner
}

void idSupplyStation::Restore(idRestoreGame *savefile)
{
	savefile->ReadObject( podOwner ); // idEntityPtr<idEntity> podOwner
}

void idSupplyStation::Think(void)
{	
	//if not in PVS, then don't do anything...
	if (!gameLocal.InPlayerConnectedArea(this))
		return;

	//turn the monitor to always face player.
	idVec3 directionToMonitor = idVec3(gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().x, gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().y, GetPhysics()->GetOrigin().z) - GetPhysics()->GetOrigin();
	idAngles newMonitorAngle = directionToMonitor.ToAngles();
	SetAxis(newMonitorAngle.ToMat3());

    Event_SetGuiInt("coins", gameLocal.GetLocalPlayer()->GetCoinCount());

	idStaticEntity::Think();
}

void idSupplyStation::PurchaseItem(const char *defName, int cost)
{
    int coinsPlayerHas = gameLocal.GetLocalPlayer()->GetCoinCount();

    if (cost > coinsPlayerHas)
    {
        StartSound("snd_error", SND_CHANNEL_ANY);
        Event_GuiNamedEvent(1, "onInsufficientCoin");
        return;
    }

    if (podOwner.IsValid())
    {
        if (podOwner.GetEntity()->IsType(idLifeboat::Type))
        {
            static_cast<idLifeboat *>(podOwner.GetEntity())->SpawnGoodie(defName);

            //Deduct coins.
            gameLocal.GetLocalPlayer()->SetCoinDelta(-cost);
        }
    }
}

//When player hits gui buttons.
void idSupplyStation::DoGenericImpulse(int index)
{
	if (index == INDEX_PURCHASE0)
	{
		//Purchase item in slot #0
        PurchaseItem("moveable_aloeplant", 1);
	}
    else if (index == INDEX_PURCHASE1)
    {
        //Purchase item in slot #1
        PurchaseItem("moveable_bloodbag", 5);
    }
	else if (index == INDEX_TAKEOFF)
	{
		//Take off.
		if (podOwner.IsValid())
		{
			if (podOwner.GetEntity()->IsType(idLifeboat::Type))
			{
				static_cast<idLifeboat *>(podOwner.GetEntity())->StartTakeoff();

				gameLocal.DoParticle("smoke_ring17.prt", GetPhysics()->GetOrigin());

				//Disappear!
				this->Hide();
			}
		}
	}
}



void idSupplyStation::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
}


