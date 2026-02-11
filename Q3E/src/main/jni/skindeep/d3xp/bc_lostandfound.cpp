//#include "sys/platform.h"
//#include "Entity.h"

#include "framework/DeclEntityDef.h"
#include "Fx.h"

#include "Item.h"
#include "bc_lostandfound.h"
#include "Player.h"
#include "framework/Common.h"
#include "idlib/LangDict.h"

const int VENDING_DISPENSETIME = 1750;
const idVec3 COLOR_IDLE = idVec3(0, 1, 0);
const idVec3 COLOR_BUSY = idVec3(1, .7f, 0);
const idVec3 BUTTONOFFSET = idVec3(17, 3, 58.5f);

const int IDX_RETRIEVEBUTTON = 700;
const int UPDATE_CACHE_TIME = 1000;

CLASS_DECLARATION(idStaticEntity, idLostAndFound)
END_CLASS

idLostAndFound::idLostAndFound(void)
{
	locbox = NULL;

	proximityAnnouncer.sensor = this;
	proximityAnnouncer.checkHealth = false;
	proximityAnnouncer.coolDownPeriod = 10000;

	repairNode.SetOwner(this);
	repairNode.AddToEnd(gameLocal.repairEntities);

	itemList = uiManager->AllocListGUI();
}

idLostAndFound::~idLostAndFound(void)
{
	repairNode.Remove();

	uiManager->FreeListGUI(itemList);
}

void idLostAndFound::Spawn(void)
{
	idDict args;		

	StartSound("snd_ambient", SND_CHANNEL_BODY, 0, false, NULL);	 //idle hum.
	fl.takedamage = true;
	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);		
	
	vendState = VENDSTATE_IDLE;
	SetColor(COLOR_IDLE);

	needsRepair = false;
	repairrequestTimestamp = 0;

	UpdateVisuals();

	//item the player wants to retrieve
	itemDef = nullptr;

	//the list that the items will populate
	itemList->Config(renderEntity.gui[0], "itemList");
	itemList->Clear();
	itemList->SetSelection(0);

	proximityAnnouncer.Start();

	//3-25-2025: locbox
	idVec3 forward, up;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward,NULL, &up);
	#define LOCBOXWIDTH 30
	#define LOCBOXHEIGHT 5
	#define LOCBOXDEPTH 3
	args.Clear();
	args.Set("text", common->GetLanguageDict()->GetString("#str_gui_info_station_100075"));
	args.SetVector("origin", GetPhysics()->GetOrigin() + (up * 90) + (forward * 16));
	args.SetBool("playerlook_trigger", true);
	args.SetVector("mins", idVec3(-LOCBOXDEPTH, -LOCBOXWIDTH, -LOCBOXHEIGHT));
	args.SetVector("maxs", idVec3(LOCBOXDEPTH, LOCBOXWIDTH, LOCBOXHEIGHT));
	locbox = static_cast<idTrigger_Multi*>(gameLocal.SpawnEntityType(idTrigger_Multi::Type, &args));
	locbox->SetAxis(GetPhysics()->GetAxis());


	updateCacheTimer = 0;
	BecomeActive( TH_THINK );
}



void idLostAndFound::Save(idSaveGame *savefile) const
{
	SaveFileWriteArray( cachedEntityDefNums,cachedEntityDefNums.Num(), WriteInt ); // idList<int> cachedEntityDefNums

	savefile->WriteInt( vendState ); // int vendState
	savefile->WriteInt( stateTimer ); // int stateTimer
	savefile->WriteInt( updateCacheTimer ); // int updateCacheTimer

	itemList->Save( savefile ); // idListGUI * itemList

	savefile->WriteEntityDef( itemDef ); // const idDeclEntityDef	* itemDef

	proximityAnnouncer.Save( savefile ); // idProximityAnnouncer proximityAnnouncer

	savefile->WriteObject( locbox ); //  idEntity* locbox
}

void idLostAndFound::Restore(idRestoreGame *savefile)
{
	SaveFileReadList( cachedEntityDefNums, ReadInt ); // idList<int> cachedEntityDefNums

	savefile->ReadInt( vendState ); // int vendState
	savefile->ReadInt( stateTimer ); // int stateTimer
	savefile->ReadInt( updateCacheTimer ); // int updateCacheTimer

	itemList->Restore( savefile ); // idListGUI * itemList

	savefile->ReadEntityDef( itemDef ); // const idDeclEntityDef	* itemDef

	proximityAnnouncer.Restore( savefile ); // idProximityAnnouncer proximityAnnouncer

	savefile->ReadObject( locbox ); //  idEntity* locbox
}

void idLostAndFound::Think(void)
{	
	if (vendState == VENDSTATE_DELAY)
	{
		if (gameLocal.time >= stateTimer)
		{
			idEntity *spawnedItem;			
			idVec3 candidateSpawnPos;
			bool successfulSpawn = false;
	
			if (itemDef == nullptr)
			{
				vendState = VENDSTATE_IDLE;
				return;
			}

			gameLocal.SpawnEntityDef(itemDef->dict, &spawnedItem, false);
			if (spawnedItem)
			{
				//Find a suitable place to spawn the item.
				candidateSpawnPos = FindValidSpawnPosition(spawnedItem->GetPhysics()->GetBounds());
	
				if (candidateSpawnPos != vec3_zero)
				{
					//all clear.
					spawnedItem->SetOrigin(candidateSpawnPos);
					gameLocal.DoParticle("smoke_ring13.prt", candidateSpawnPos);
					successfulSpawn = true;

					idVec3 forward, right;
					this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right,NULL);
					spawnedItem->GetPhysics()->SetLinearVelocity((forward * 24) + (right * (gameLocal.random.CRandomFloat() * 48)));
	
					if (spawnedItem->IsType(idItem::Type))
					{
						static_cast<idItem *>(spawnedItem)->SetJustDropped(true);
					}
				}
				else
				{
					spawnedItem->PostEventMS(&EV_Remove, 0);
				}
			}
	
			if (!successfulSpawn)
			{
				StartSound("snd_fail", SND_CHANNEL_ANY, 0, false, NULL);
			}
	
			SetColor(COLOR_IDLE);
			vendState = VENDSTATE_IDLE;
		}
	}
	else
	{
		bool hasCriticalItem = false;

		for (int i = 0; i < cachedEntityDefNums.Num(); i++)
		{
			int defNum = cachedEntityDefNums[i];
			const idDeclEntityDef* def = static_cast<const idDeclEntityDef*>(declManager->DeclByIndex(DECL_ENTITYDEF, defNum));
			
			bool isCriticalItem = def->dict.GetBool("criticalitem", "0");
			if (isCriticalItem)
			{
				hasCriticalItem = true;
				break;
			}
		}

		if( hasCriticalItem && health > 0) //BC 3-3-2025: don't do announcement if I'm dead
		{
			proximityAnnouncer.Update();
		}
	}

	if ( gameLocal.time > updateCacheTimer )
	{
		UpdateLostEntityCache();
	}

	idStaticEntity::Think();
}

//BC 4-10-2025: lost and found now attempts various candidate spawn positions.
idVec3 idLostAndFound::FindValidSpawnPosition(idBounds itemBounds)
{
	//this is an array of offset positions for the object spawn point.
	//Values are: forward, right, up
	const int ARRAYSIZE = 9;
	idVec3 offsetArray[] =
	{
		idVec3(0, 0, 0),	//no offset.
		idVec3(8, 0, 0),	//a little bit forward
		idVec3(16, 0, 0),	//a lot of bit forward
		idVec3(16, -16, 0), //forward, and to the left
		idVec3(16, 16, 0),	//forward, and to the right
		idVec3(32, 0, 0),	//desperation time
		idVec3(32, -32, 0), //desperation time
		idVec3(32, 32, 0),	//desperation time
		idVec3(64, 0, 0)	//desperation time
	};

	idVec3 forward, right, up;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	for (int i = 0; i < ARRAYSIZE; i++)
	{
		idVec3 spawnPosition = GetDispenserPosition() + (forward * offsetArray[i].x) + (right * offsetArray[i].y) + (up * offsetArray[i].z);

		//Check if this point starts inside geometry
		//We do this because we can get a false positive if the entirety of the bounds check is inside the void.
		int penetrationContents = gameLocal.clip.Contents(spawnPosition, NULL, mat3_identity, CONTENTS_SOLID, NULL);
		if (penetrationContents & MASK_SOLID)
		{
			continue; //If it starts in solid, then exit.
		}

		trace_t boundTr;
		gameLocal.clip.TraceBounds(boundTr, spawnPosition, spawnPosition, itemBounds, MASK_SOLID, NULL);

		if (boundTr.fraction >= 1.0f)
		{
			return spawnPosition; //There is clearance, so spot is valid. Return valid position.
		}
	}

	return vec3_zero; //Failed to find a valid spot.
}

idVec3 idLostAndFound::GetDispenserPosition()
{
	idVec3 forward, right, up;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	return this->GetPhysics()->GetOrigin() + (forward * 24) + (up * 43) + (right * 14);
}



void idLostAndFound::DoRepairTick(int amount)
{
    Event_GuiNamedEvent(1, "onRepaired");
	SetColor(COLOR_IDLE);

	//Repair is done!
	health = maxHealth;
	needsRepair = false;
	vendState = VENDSTATE_IDLE;
	StartSound("snd_ambient", SND_CHANNEL_BODY, 0, false, NULL);	 //idle hum.
	fl.takedamage = true;

	renderEntity.shaderParms[7] = 1; //make sign turn on.
	UpdateVisuals();
}

void idLostAndFound::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	idVec3 forward;
	idAngles particleAng;

	if (!fl.takedamage)
		return;

	gameLocal.AddEventlogDeath(this, 0, inflictor, attacker, "", EL_DESTROYED);

	fl.takedamage = false;
	SetColor(idVec3(0, 0, 0));
	StopSound(SND_CHANNEL_BODY, false);
	StartSound("snd_powerdown", SND_CHANNEL_BODY, 0, false, NULL);

	needsRepair = true;
	repairrequestTimestamp = gameLocal.time;
	
	renderEntity.shaderParms[7] = 0; //make sign turn off.
	UpdateVisuals();

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
	idEntityFx::StartFx("fx/explosion_tnt", this->GetPhysics()->GetOrigin() + (forward * 20) + idVec3(0, 0, 48), mat3_identity, this, false);

	particleAng = this->GetPhysics()->GetAxis().ToAngles();
	particleAng.pitch -= 90;
	gameLocal.DoParticle("vent_purgesmoke.prt", this->GetPhysics()->GetOrigin() + (forward * 18) + idVec3(0, 0, 64), particleAng.ToForward());

    Event_GuiNamedEvent(1, "onDamaged");
}

//When the retrieve button is pressed.
void idLostAndFound::DoGenericImpulse(int index)
{
	if (index == IDX_RETRIEVEBUTTON)
	{
		//Retrieve the item.

		// SW 14th April 2025: Moved this check higher so that player cannot change itemDef by spamming the button mid-vend
		if (vendState != VENDSTATE_IDLE)
		{
			StartSound("snd_fail", SND_CHANNEL_ANY, 0, false, NULL);
			return;
		}
		
		//First get the item definition.
		int selNum = itemList->GetSelection( nullptr, 0 );
		if (selNum == -1)
		{
			StartSound("snd_fail", SND_CHANNEL_ANY, 0, false, NULL);
			return;
		}

		itemDef = static_cast< const idDeclEntityDef* >( declManager->DeclByIndex( DECL_ENTITYDEF, selNum ) );
		//itemDef = gameLocal.FindEntityDef(spawnArgs.GetString("def_itemspawn"), false);

		if (itemDef == nullptr)
		{
			StartSound("snd_fail", SND_CHANNEL_ANY, 0, false, NULL);
			return;
		}

		StartSound("snd_button", SND_CHANNEL_VOICE, 0, false, NULL);

		if (health <= 0)
			return; //machine is busted, don't continue.

		StartSound("snd_dispense", SND_CHANNEL_VOICE2, 0, false, NULL);
		vendState = VENDSTATE_DELAY;
		stateTimer = gameLocal.time + VENDING_DISPENSETIME;

		gameLocal.GetLocalPlayer()->RemoveLostInSpace( selNum );
		UpdateLostEntityCache();

		StartSound("snd_jingle", SND_CHANNEL_ANY, 0, false, NULL);
	}
}

void idLostAndFound::UpdateLostEntityCache()
{
	updateCacheTimer = gameLocal.time + UPDATE_CACHE_TIME;
	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( !player )
		return;

	// Check if it's changed
	bool changed = player->lostInSpaceEntityDefNums.Num() != cachedEntityDefNums.Num();
	if ( !changed )
	{
		// Same size, check to see if each is equal
		for ( int i = 0; i < cachedEntityDefNums.Num(); i++ )
		{
			if ( player->lostInSpaceEntityDefNums[i] != cachedEntityDefNums[i] )
			{
				changed = true;
				break;
			}
		}
	}

	// It's changed, so update the cache and the UI list
	if ( changed )
	{
		cachedEntityDefNums = player->lostInSpaceEntityDefNums;
		itemList->Clear();

		for ( int i = 0; i < cachedEntityDefNums.Num(); i++ )
		{
			int defNum = cachedEntityDefNums[i];
			const idDeclEntityDef* def = static_cast< const idDeclEntityDef* >( declManager->DeclByIndex( DECL_ENTITYDEF, defNum ) );
			const char* displayName = def->dict.GetString( "displayname", "#str_loc_unknown_00104" );
			if ( displayName[0] == '#' )
			{
				displayName = common->GetLanguageDict()->GetString( displayName );
			}
			itemList->Add( defNum, displayName, true );
		}

		itemList->SetSelection( 0 );
	}
}
