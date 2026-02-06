//#include "script/Script_Thread.h"
#include "framework/DeclEntityDef.h"

#include "WorldSpawn.h"
#include "Actor.h"
#include "Mover.h"
#include "Player.h"
#include "bc_meta.h"
#include "bc_radiocheckin.h"


#define CHECKIN_GAPTIME 80 //delay between alpha, bravo, charlie, etc checkins. We need a gap or else it all sounds all jumbled up, as the sound length value isn't always accurate.

#define COOLDOWN_TIME 60000 //cooldown time between radio checkin sequences.

CLASS_DECLARATION(idEntity, idRadioCheckin)
END_CLASS

idRadioCheckin::idRadioCheckin()
{
}

idRadioCheckin::~idRadioCheckin()
{
}


void idRadioCheckin::Spawn()
{
	state = RDC_NONE;
	timer = 0;
	for (int i = 0; i < RADIOCHECK_MAX; i++)
	{
		aliveKnowledge[i] = true;
	}
	for (int i = 0; i < RADIOCHECK_MAX; i++)
	{
		unitEntityNumbers[i] = -1;
	}
	unitMaxCount = 0;
	checkinIndex = UNIT_ALPHA;
	isPlayingStatic = false;
	cooldownTimer = 0;

	BecomeActive(TH_THINK);
}

void idRadioCheckin::Save(idSaveGame* savefile) const
{
	savefile->WriteInt( state ); // int state

	savefile->WriteInt( timer ); // int timer
	SaveFileWriteArray(aliveKnowledge, RADIOCHECK_MAX, WriteBool); // bool aliveKnowledge[RADIOCHECK_MAX]
	savefile->WriteInt( unitMaxCount ); // int unitMaxCount

	SaveFileWriteArray(unitEntityNumbers, RADIOCHECK_MAX, WriteInt); // int unitEntityNumbers[RADIOCHECK_MAX];
	savefile->WriteInt( checkinIndex ); // int checkinIndex

	savefile->WriteBool( isPlayingStatic ); // bool isPlayingStatic

	savefile->WriteInt( cooldownTimer ); // int cooldownTimer
}
void idRadioCheckin::Restore(idRestoreGame* savefile)
{
	savefile->ReadInt( state ); // int state

	savefile->ReadInt( timer ); // int timer
	SaveFileReadArray(aliveKnowledge, ReadBool); // bool aliveKnowledge[RADIOCHECK_MAX]
	savefile->ReadInt( unitMaxCount ); // int unitMaxCount

	SaveFileReadArray(unitEntityNumbers, ReadInt); // int unitEntityNumbers[RADIOCHECK_MAX];
	savefile->ReadInt( checkinIndex ); // int checkinIndex

	savefile->ReadBool( isPlayingStatic ); // bool isPlayingStatic

	savefile->ReadInt( cooldownTimer ); // int cooldownTimer
}

//Tell this system how many bad guys total in level. This is called via idMeta.
void idRadioCheckin::InitializeEnemyCount()
{
	int totalEnemies = 0;
	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity)
			continue;

		if (!entity->IsType(idActor::Type) || entity->team != TEAM_ENEMY || entity->IsHidden()) //note: ignore enemies in the reinforcements queue (they're marked isHidden)
			continue;

		// SM: Don't allow writing out of bounds to prevent memory corruption
		if ( totalEnemies < RADIOCHECK_MAX )
		{
			unitEntityNumbers[totalEnemies] = entity->entityNumber;
		}
		totalEnemies++;
	}

	if (totalEnemies > RADIOCHECK_MAX)
	{
		gameLocal.Warning("idRadioCheckin: enemy count (%d) exceeds radio checkin maximum value (%d).", totalEnemies, RADIOCHECK_MAX);
	}

	unitMaxCount = totalEnemies;
}

//Start the radio checkin sequence.
void idRadioCheckin::StartCheckin(idVec3 _pos)
{
	//If we're currently already doing a radio checkin, then exit here.
	if (state != RDC_NONE || !gameLocal.world->spawnArgs.GetBool("meta_radiocheck", "1"))
		return;

	//If we're already in combat state, then exit here.
	if (static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetCombatState() == COMBATSTATE_COMBAT)
		return;

	//If we 'believe' that only one unit is alive, then exit here. It's weird if one guy just does a radio checkin with themself.
	int aliveAmount = 0;
	for (int i = 0; i < min(unitMaxCount, RADIOCHECK_MAX); i++)
	{
		if (aliveKnowledge[i])
		{
			aliveAmount++;
		}
	}
	if (aliveAmount <= 1)
		return;

	//If the cooldown is still happening, then exit here.
	if (cooldownTimer > gameLocal.time)
		return;

	ResetCooldown();

	//Start the sequence.
	state = RDC_STARTCHECK;
	int len;
	StartSound("snd_start", SND_CHANNEL_VOICE, 0, false, &len);
	timer = gameLocal.time + len; //make the state last as long as the sound file length.

	gameLocal.AddEventLog("#str_def_gameplay_checkin_start", _pos);
}

void idRadioCheckin::ResetCooldown()
{
	cooldownTimer = gameLocal.time + COOLDOWN_TIME;
}

void idRadioCheckin::Think(void)
{
	if (state == RDC_STARTCHECK)
	{
		if (gameLocal.time > timer)
		{
			state = RDC_CHECKINS;
			checkinIndex = -1; //reset the checkin index to the first slot.
			timer = 0;
			isPlayingStatic = false;
		}
	}
	else if (state == RDC_CHECKINS)
	{
		//A unit is doing its check in.
		if (gameLocal.time >= timer)
		{
			if (isPlayingStatic)
			{
				//The last reply was static (dead guard). The jig is up!!!!!!!!!!!!!!!!! Kick into combat alert.
				state = RDC_COMBATALERT;

				//if (static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetCombatState() == COMBATSTATE_IDLE)
				{
					//static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GotoCombatSearchState();
					static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->AlertAIFriends(NULL); //activate combat state.
				}

				int len;
				StartSound("snd_fail", SND_CHANNEL_VOICE, 0, false, &len);
				timer = gameLocal.time + len; //make the state last as long as the sound file length.

				//Update our 'knowledge' of who the system knows/believes is alive and dead.
				aliveKnowledge[checkinIndex] = false; //We didn't get a radio signal from this index, so we now believe they are dead.

				gameLocal.AddEventLog("#str_def_gameplay_checkin_fail", vec3_zero);
				isPlayingStatic = false;

				return;
			}

			//start next available check in.
			//Find someone who we 'believe' is alive.
			checkinIndex = GetNextAvailableCheckinIndex(checkinIndex);

			if (checkinIndex < 0)
			{
				//There is no one else to check in to.
				int len;
				StartSound("snd_allclear", SND_CHANNEL_VOICE, 0, false, &len);
				timer = gameLocal.time + len;
				state = RDC_ALLCLEAR;

				gameLocal.AddEventLog("#str_def_gameplay_checkin_done", vec3_zero);

				return;
			}

			//Start the checkin sound.
			//We first need to see if the person is DEAD or ALIVE.
			int voiceprint;
			bool unitIsAlive = IsUnitActuallyAlive(checkinIndex, &voiceprint);

			idStr soundCue;

			if (unitIsAlive)
			{
				//unit is alive.
				isPlayingStatic = false;
				switch (checkinIndex)
				{
					case UNIT_ALPHA:	{ soundCue = "alpha"; break; }		//0
					case UNIT_BRAVO:	{ soundCue = "bravo"; break; }		//1
					case UNIT_CHARLIE:	{ soundCue = "charlie"; break; }	//2
					case UNIT_DELTA:	{ soundCue = "delta"; break; }		//3
					case UNIT_ECHO:		{ soundCue = "echo"; break; }		//4
					case UNIT_FOX:		{ soundCue = "fox"; break; }		//5
					case UNIT_GAMMA:	{ soundCue = "gamma"; break; }		//6
					case UNIT_HAVANA:	{ soundCue = "havana"; break; }		//7
					case UNIT_KILO:		{ soundCue = "kilo"; break; }		//8
					case UNIT_LAMBDA:	{ soundCue = "lambda"; break; }		//9
					case UNIT_ROMEO:	{ soundCue = "romeo"; break; }		//10
					case UNIT_TANGO:	{ soundCue = "tango"; break; }		//11
					default:			{ common->Warning("idRadioCheckin: invalid checkin index '%d'\n", checkinIndex); break; }
				}


				//BC 3-17-2025: added support for heavy monster.
				idStr voiceprintPrefix = "";
				if (voiceprint == 2)
					voiceprintPrefix = "snd_c";
				else if (voiceprint == 1)
					voiceprintPrefix = "snd_b";
				else
					voiceprintPrefix = "snd_a"; //voiceprint 0, or any other number. This is the fallback default.

				soundCue = idStr::Format("%s_%s", voiceprintPrefix.c_str(), soundCue.c_str());

			}
			else
			{
				//unit is dead. Play the static sound.
				soundCue = "snd_static";
				isPlayingStatic = true;
			}

			//Play the sound.
			int len;
			StartSound(soundCue, SND_CHANNEL_VOICE, 0, false, &len);
			timer = gameLocal.time + len + CHECKIN_GAPTIME;

			
			//TODO: make sound waves come out of AI's head.

		}

		//If index exceeds max unit count, then end.
		if (checkinIndex >= unitMaxCount)
		{
			int len;
			StartSound("snd_allclear", SND_CHANNEL_VOICE, 0, false, &len);
			timer = gameLocal.time + len;
			state = RDC_ALLCLEAR;

			gameLocal.AddEventLog("#str_def_gameplay_checkin_done", vec3_zero);

			return;
		}
	}
	else if (state == RDC_COMBATALERT)
	{
		//someone has failed to check in, so we play the "something has gone wrong" radio call.
		if (gameLocal.time > timer)
		{
			state = RDC_NONE;
		}
	}
	else if (state == RDC_ALLCLEAR)
	{
		//all known people who are alive have checked in. Play "all clear, everything is fine" call.
		if (gameLocal.time > timer)
		{
			state = RDC_NONE;

			//Make the world state go back to idle.
			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GotoCombatIdleState();
		}
	}
}

void idRadioCheckin::DoPlayerFakeCheckin()
{
	//Player does fake checkin.
	//When a 'dead' guy does their static checkin, allow the player to fake the checkin.
	StopSound(SND_CHANNEL_ANY);

	bool playerIsInjured = gameLocal.GetLocalPlayer()->GetInjured();

	int len;
	len = gameLocal.voManager.SayVO(gameLocal.GetLocalPlayer(), playerIsInjured ? "snd_vo_walkie_hijack_injured" : "snd_allclear", VO_CATEGORY_NARRATIVE);
	timer = gameLocal.time + len + CHECKIN_GAPTIME;
	isPlayingStatic = false;
}

int idRadioCheckin::GetNextAvailableCheckinIndex(int startingIndex)
{
	int maxCounter = 0;
	int startValue = min(startingIndex + 1, unitMaxCount);
	for (int i = startValue; i < unitMaxCount; i++)
	{
		maxCounter++;
		if (maxCounter >= unitMaxCount)
			return -1;

		if (aliveKnowledge[i] == true)
		{
			//to the best of their knowledge, the checkin system 'believes' this unit is alive.
			return i;
		}
	}

	return -1;
}

bool idRadioCheckin::IsUnitActuallyAlive(int unitDesignation, int *voiceprint)
{
	*voiceprint = 0;

	int entityNumber = unitEntityNumbers[unitDesignation];

	if (entityNumber < 0)
	{
		common->Warning("idRadioCheckin: IsUnitActuallyAlive received -1 value.");
		return false;
	}

	idEntity *ent = gameLocal.entities[entityNumber];

	if (!ent)
		return false;

	if (ent == NULL)
		return false;

	if (!ent->IsType(idActor::Type))
		return false;

	//BC 3-17-2025: simplified setup of voiceprint. Now uses spawnarg instead of checking monster classname.
	*voiceprint = ent->spawnArgs.GetInt("voiceprint", "0");

	return (ent->health > 0);
}

void idRadioCheckin::StopCheckin()
{
	if (state == RDC_NONE)
		return;

	state = RDC_NONE;
	StopSound(SND_CHANNEL_ANY);
}

bool idRadioCheckin::GetPlayingStatic()
{
	return isPlayingStatic;
}

bool idRadioCheckin::CheckinActive()
{
	return (state == RDC_STARTCHECK || state == RDC_CHECKINS);
}

void idRadioCheckin::SetEnable(bool value)
{
	//TODO: This isn't hooked up yet. We may not need it since meta_radiocheck worldspawn flag seems to do the same thing.
}