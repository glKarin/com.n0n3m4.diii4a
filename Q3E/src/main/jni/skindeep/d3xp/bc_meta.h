
#include "Entity.h"
#include "Target.h"
#include "ai/AI.h"
// Be careful adding includes here if you don't need them (you can forward declare instead)

const int MAX_PIPESTATUSES = 8;

const int MAX_LOS_NODES = 16;

//const int ALLCLEAR_PLAYER_WALKIETALKIE_TIMEDURATION = 3000; //how long the player 'all clear' vo audio is. 2-4-2025: this is no longer used. Value now changes depending on wav file duration.
const int ALLCLEAR_GAP_DURATION = 200;

const int SENSORY_UPDATETIME = 200; //How often to do sensory update on this interestpoint.

enum
{
	PIPE_HYPERFUEL,			//0
	PIPE_ELECTRICITY,		//1
	PIPE_NAVCOMPUTER,		//2	
};

enum
{
	NAVNODE_0,		//0
	NAVNODE_1a,		//1
	NAVNODE_1b,		//2
	NAVNODE_2a,		//3
	NAVNODE_2b,		//4
	NAVNODE_3a,		//5
	NAVNODE_3b,		//6
	NAVNODE_PIRATEBASE		//7
};

enum
{
	COMBATSTATE_IDLE,
	COMBATSTATE_COMBAT,
	COMBATSTATE_SEARCH
};

enum
{
	METASTATE_ACTIVE, //mission is in progress
	METASTATE_KILLCAM,
	METASTATE_KILLCAM_WAITFORSLOWMO_END,
	METASTATE_POSTGAME //postgame freezeframe
};


enum
{
	ENEMYDESTROYEDSTATECHECK_NONE,
	ENEMYDESTROYEDSTATECHECK_WAITDELAY,
	ENEMYDESTROYEDSTATECHECK_CATS_AWAITING_RESCUE, //game starts in this phase. cats are awaiting rescue.

	ENEMYDESTROYEDSTATECHECK_CATS_ALLRESCUED, //the player has successfully placed ALL CATS in the rescue pod.
	
	ENEMYDESTROYEDSTATECHECK_CATS_AWAITING_RESCUE_DELAY, //this may be deprecated.....

	ENEMYDESTROYEDSTATECHECK_REINFORCEMENTDELAY,
	ENEMYDESTROYEDSTATECHECK_REINFORCEMENTPHASE,
};


enum { INFOS_MAPDISPLAY, INFOS_FTLCOUNTDOWN };

enum { REPRESULT_SUCCESS, REPRESULT_FAIL, REPRESULT_ZERO_REPAIRABLES, REPRESULT_ALREADY_REPAIRBOT };

//typedef enum
//{
//	HLT_GAS_SPARK,
//} highlightType_t;

class idMeta : public idTarget
{
public:
	CLASS_PROTOTYPE(idMeta);

	idMeta();
	void				Spawn();

	void				Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void				Restore( idRestoreGame *savefile );

	virtual void		Think(void);


	idVec3				beaconPosition;

	void				Event_ActivateSpeakers(const char *soundshaderName, const s_channelType soundchannel = SND_CHANNEL_VOICE);
	void				Event_StopSpeakers(const s_channelType soundchannel = SND_CHANNEL_ANY);

	idEntityPtr<idEntity>	GetFTLDrive;

	bool				SetPipeStatus(int pipeIndex, bool value);

	idEntity			*lkpEnt = nullptr; //last-known position
	void				SetLKPPosition(idVec3 pos);
	void				SetLKPReachablePosition(const idVec3 &pos);
	void				SetLKPPositionByEntity(idEntity *enemyEnt);

	void				UpdateMetaLKP(bool hasGainedLOS);
	idVec3				GetLKPReachablePosition();

	void				IncreaseEscalationLevel();
	void				ResetEscalationLevel();

	void				OnEnemyStoredLifeboat();

	void				OnEnemyDestroyed(idEntity *ent);

	idEntityPtr<idEntity>	bossEnt;
	idEntityPtr<idEntity> skyController;

	bool				IsEscalationMaxed();

	//for searchnode LOS array.
	bool				GenerateNodeLOS( idVec3 pointToObserve);	
	idVec3				nodePositions[MAX_LOS_NODES];
	int					nodePosArraySize;
	idVec3				lastNodeSearchPos;


	int					combatMetastate;

	void				AlertAIFriends(idEntity *caller);

	int					GetSearchTimer();
	void				SomeoneStartedSearching();

	idEntity *			SpawnInterestPoint(idEntity *ownerEnt, idVec3 position, const char *interestDef);
	void				InterestPointInvestigated(idEntity* ent);
	void				InterestPointDistracted(idEntity* ent, idAI* distractedAI);
	void				InterestPointRepaired(idEntity* ent);
	void				ClearInterestPoints(void);

	void				UpdateSighted();

	void				SetLKPVisible(bool visible);

	bool				StartVentPurge(idEntity *interestpoint);
	bool				IsPurging();

	//for the signal lamp. This keeps track of whether signal lamp stuff is currently happening or not. Refer to this to deterine what gui to render on the signal lamp weapon gui.
	idEntityPtr<idEntity> signallampEntity;

	//Upgrade Cargo	
	void				DoUpgradeCargoUnlocks();


	idEntity *			SpawnIdleTask(idEntity *ownerEnt, idVec3 position, const char *idletaskName);

    int                 GetTotalEnemyCount();

	//int					GetMetaState();
	int					GetCombatState(); //idle, search, combat

	idHashTable<idStrList> itemdropsTable;


	void				SetLifeboatsLeave();

	void				GotoCombatIdleState();

	int					GetEnemiesRemaining(int includeSkullsavers);

	int					totalSecuritycamerasAtGameStart;

	//Mapbounds.
	idVec2				GetMapguiNormalizedPosition(idVec3 entityPosition);
	idVec2				GetMapguiNormalizedPositionViaEnt(idEntity *ent);

	void				SetPlayerExitedCryopod(bool value);	
	bool				GetPlayerExitedCryopod();
	int					GetCryoexitLocationEntnum();
	int					GetCryoExitTime();

	void				StartAllClearSequence(int delayTime, int voiceprint = VOICEPRINT_A);
	bool				StartPlayerWalkietalkieSequence(int *_voLength);
	int					dispatchVoiceprint; //The last voiceprint used. This is so we can re-use the same voiceprint after a purge is done to say "purge is complete, good job fellow pirates"

	void				SetWalkietalkieLure(idEntity *ent);

	
	void				SetAllInfostationsMode(int mode);
	void				InfostationsLockdisplayUpdate();

	void				LimitActiveGlassPieces();
	void				DestroyNearbyGlassPieces(idVec3 position);

	class idRadioCheckin *radioCheckinEnt = nullptr;
	class idHighlighter* highlighterEnt = nullptr;

	void				GotoCombatSearchState();
	int					GetCombatStartTime();

	void				StartFTLRescueSequence();

	void				FTL_DoShutdown();

	bool				FTL_IsActive();

	void				FTL_JumpEnded();
    void                FTL_JumpStarted();
	void                FTL_CountdownStarted();

	enum				{FTLSIGN_OFF, FTLSIGN_COUNTDOWN, FTLSIGN_ACTIVE};
	void				SetAll_FTL_Signage(int ftlsign_value);

	void				OnEnemySeeHostile();

	void				StartPostGame();

	void				SetAllClearInterrupt();

	void				SetWorldBaffled(bool value);
	bool				GetWorldBaffled();

	void				StartReinforcementsSequence();
	bool				SpawnPirateShip();
	enum				{REINF_NONE, REINF_SPAWNDELAY, REINF_PIRATES_ENROUTE, REINF_PIRATES_ALL_SPAWNED};
	int					reinforcementPhase;
	int					reinforcementPhaseTimer;
	void				SetReinforcementPiratesAllSpawned();
	bool				GetReinforcementsActive();

	void				UpdateCagecageObjectiveText();

	void				SetCombatDurationTime(int value);

	bool				SpawnSecurityBot(idVec3 _roomPos);
	bool				SpawnSecurityBotInRoom(idEntity *targetEnt);
	int					GetSecurityBotsInRoom(idEntity* targetEnt);

	//For the fusebox lockdowns.
	void				SetEnableTrashchutes(bool value);
	void				SetEnableAirlocks(bool value);
	void				SetEnableVents(bool value);
	void				SetEnableWindows(bool value);
	void				SetEnableDoorBarricades();
	bool				IsFuseboxLocked(int systemIndex);


	void				SetEnableRadioCheckin(int value);
    
	int 				RepairEntitiesInRoom(idVec3 _pos);

	idEntity *			GetSecurityCameraViaIndex(int index);

	
	bool				DoHighlighter(idEntity* ent1, idEntity* ent2);
	bool				GetHightlighterActive();
	void				DoHightlighterThink();
	void				DrawHighlighterBars();
	void				SkipHighlighter();
	void				Event_DoHighlighter(idEntity* e1, idEntity* e2);

	int					GetTotalCatCages();

	void				SetPlayerEnterCatpod();

	void				SetMilestone(int index);

	bool				WasMilestoneTrueAtLevelStart(int milestoneIndex);

	int					GetTotalTimeSpentInSpace();

	void                Event_SetCombatState(int value);

	
	bool				GetPlayerHasEnteredCatpod();
	bool				GetPlayerIsCurrentlyInCatpod();

	bool				GetEnemiesKnowOfNina();

	//BC PUBLIC END


private:
	
	void				UpdateBeacon();	
	float				beaconAngle;

	void				Event_GetBeaconPosition();

	virtual void		Event_PostSpawn(void);

	void				Event_GetLastSwordfishTime();
	void				Event_SetSwordfishTime(float value);
	float				lastSwordfishTime;

	bool				pipeStatuses[MAX_PIPESTATUSES];
	
	
	void				GetLKPPosition();	
	idEntity			*lkpEnt_Eye = nullptr; //last-known position	

	idVec3				lkpReachablePosition;
	void				Event_GetLKPReachable();

	int					escalationLevel;
	void				Event_GetEscalationLevel();


	void				SetSpaceNode(int navnode);
	int					currentNavnode;
	int					nextNavnode;
	
	

	void				PirateBaseSpawnCheck();



	int					searchTimer;
	void				Event_GetSearchTimer();

	void				UpdateInterestPoints();

	int					lastCombatMetastate;



	void				UpdateRepairManager();
	int					GetRepairBotCount();
	bool				SendAvailableRepairBot(idEntity * repairableEnt);
	int					repairmanagerTimer;
	bool				HasRepairEntityBeenClaimed(idEntity * repairableEnt);
	void				ActivateClosestRepairHatch(idEntity * repairableEnt);
	bool				IsBotRepairingSomethingNearby(idEntity * repairableEnt);
	bool				DoesEnemyHaveLOStoRepairable(idEntity *repairableEnt);
	bool				CanAISeeRepairable(idEntity *repairableEnt, idAI *aiEnt);
    int                 repairVO_timer;


	//Vent purge system.
	enum				{VENTPURGESTATE_IDLE, VENTPURGESTATE_DISPATCH, VENTPURGESTATE_ANNOUNCE, VENTPURGESTATE_CHARGEUP, VENTPURGESTATE_PURGING, VENTPURGESTATE_ALLCLEAR_DELAY};
	int					ventpurgeState;
	void				UpdateVentpurge();
	int					ventpurgeTimer;
	void				PlayVentSound(const char *soundname);
	



	//Post-purge all-clear sequence.
	void				UpdateAllClearSequence();	
	int					allclearState;
	int					allclearTimer;
	enum				{ALLCLEAR_DORMANT, ALLCLEAR_DELAY, ALLCLEAR_ANNOUNCING, ALLCLEAR_PLAYERWALKIETALKIE};
	


	//EscalationMeter.

	int					escalationmeterAmount;
	int					escalationmeterTimer;
	void				UpdateEscalationmeter();

	bool				IsFTLActive();


	//Enemy spawn system.
	bool				SpawnEnemies(int cryoLocationEntnum, bool doCryospawnCheck);


	//Enemy equipment initialization.
	void				EquipEnemyLoadout();

	bool				GetSignallampActive();




	void				InitializeUpgradeCargos();

	void				InitializeItemSpawns();


	void				UpdateIdletasks();
	int					idletaskTimer;

	idEntity			*GetClosestIdleActor(class idIdleTask *idleTask);

    int                 totalEnemies;

	int					metaState;


	int					enemydestroyedCheckTimer;
	int					enemydestroyedCheckState;


	void				SetupWindowseals();


	int					killcamTimer;
	bool				SetupKillcam();
	idEntityPtr<idEntity> killcamTarget;
	idEntity *			killcamCamera = nullptr;
	idVec3				FindKillcamPosition();
	bool				ValidateKillcamPosition(idVec3 position);
	int					lastKilltime;

	

	void				UpdateDynamicSky(void);

	void				ActivateAllTurrets(bool value);
	
	int					combatstateStartTime;

	//Camerasplice setup.
	void				PopulateCameraSplices();

	//Mapbounds.
	void				SetupMapBounds();
	idVec2				mapguiBound[2];

	
	

	int					cryoExitLocationEntnum;
	bool				playerHasExitedCryopod;
	int					cryoExitTimer;

	int					enemiesEliminatedInCurrentPhase;

	idEntityPtr<idEntity> walkietalkieLureTask;
	idBeam*				beamLure = nullptr;
	idBeam*				beamLureTarget = nullptr;
	idEntity*			beamRectangle = nullptr;



	idEntityPtr<idEntity> doorframeInfoEnt;
	bool				ftlGUIsAreOn;
	//int					FTLActiveState;
	void				UpdateDoorframeGUIs();

	void				RespawnSkullsavers();

	void				SetAirlockSignGuiValue(const char *text);
	void				SetAirlockFTLDoorsignGui(const char *text);

	int					glassdestroyTimer;

	idEntity*			GetEmptyAirlock();

    void                EnableSabotageLevers(bool value);

    void                EnableAllElectricalboxes(bool value);
	
	void				UnlockAirlockLockdown();

	void				NameTheEnemies();

	void				UpdateBombStartCheck();

    
	void				Event_GetCombatState(void);

	void				Event_GetAliveEnemies();

	void				Event_StartGlobalStunState(const char* aiDamageDef);

	void				Event_LaunchScriptedProjectile(idEntity* owner, char* damageDef, const idVec3 &spawnPos, const idVec3 &spawnTrajectory);
	void				Event_SetDebrisBurst(const char* defName, idVec3 position, int count, float radius, float speed, idVec3 direction);

	void				ReinforcementEndgameCheck();

	bool				enemyThinksPlayerIsInVentOrAirlock;
	bool				IsEntInVentOrAirlock(idEntity *ent);

	bool				isWorldBaffled;

	int					combatstateDurationTime;

	int					totalCatCagesInLevel;
	int					CalculateTotalCatCages();


	idEntityPtr<idEntity>	catpodInterior;

	void				VerifyLevelIndexes();

	void				GenerateKeypadcodes();
	idStrList			PickRandomCodes(int amount, idStrList keys);
	void				AssignCodesToKeypads(idStrList selectedCodes, idList<int> allSystemIndexes);

	bool				milestoneStartArray[MAXMILESTONES];
	void				InitializeMilestoneStartArray();


	//Handle the stat for tracking how long player is in outer space.
	bool				playerIsInSpace;
	int					playerSpaceTimerStart;
	int					playerSpaceTimerTotal;
	void				UpdatePlayerSpaceTimerStat();


	void				SetExitVR();

	bool				playerHasEnteredCatpod;

	bool				enemiesKnowOfNina;



	int					signalkitDepletionTimer;
	void				DoSignalkitDepletionCheck();

	idEntity*			FindSpaceLocation();
public:
	idEntity*			FindLostAndFoundMachine();
private:

	bool				signalkitDepletionCheckActive;

	int					swordfishTimer;

	//BC 3-24-2025: locbox.
	idEntity* lkpLocbox = nullptr;

	//BC PRIVATE END
};