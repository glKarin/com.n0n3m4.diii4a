/*
===========================================================================

Icarus Starship Command Simulator GPL Source Code
Copyright (C) 2017 Steven Eric Boyette.

This file is part of the Icarus Starship Command Simulator GPL Source Code (?Icarus Starship Command Simulator GPL Source Code?).

Icarus Starship Command Simulator GPL Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Icarus Starship Command Simulator GPL Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Icarus Starship Command Simulator GPL Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef __SBSHIP_H__
#define __SBSHIP_H__

class sbTransporterPad;
class sbCaptainChair;

// boyette spaceship command begin
#include "sbConsole.h"
#include "sbModule.h"
// boyette spaceship command begin

const int MAX_DIALOGUE_BRANCHES = 128;  // - This is for the dialogue branch bitset.
const int MAX_SHIP_DOOR_ENTITIES = 20;  // - This is for the ship doors loops.
const int MAX_HULL_REPAIR_AMOUNT_PER_SHIP_REPAIR_CYCLE = 10; // was 100  // - This is for ship repair mode - amount repaired per a cycle.
const int TIME_BETWEEN_SHIP_REPAIR_CYCLES = 500; // was 1000  // - 1000 milliseconds (which is 1 second)

const int MAX_SHIP_LOG_ITEMS = 32;  // for the ship log laptop list
const int MAX_SHIP_ENCYCLOPEDIA_ITEMS = 128;  // for the ship encyclopedia laptop list

const int MAX_RESERVE_CREW_ON_SHIPS = 90;

const int MAX_MAX_HULLSTRENGTH = 3000;
const int MAX_MAX_SHIELDSTRENGTH = 3000;

const int MAX_MAX_POWER_RESERVE = 46;

const int ALWAYS_NEUTRAL_TEAM = 15001;

// SHIP AI EVENTS BEGIN

// ship ai verbs/goal_actions: // BOYETTE NOTE TODO: put the abbreviated versions of these in the ai_ship.script and see if it conflicts with other global variables in other script files.
enum {
	SHIP_AI_IDLE,
	SHIP_AI_PROTECT,
	SHIP_AI_FLEE_FROM,
	SHIP_AI_DEFEND_AGAINST,
	SHIP_AI_ATTACK,
	SHIP_AI_SEEK_AND_DESTROY,
	SHIP_AI_FOLLOW,
	SHIP_AI_MOVE_TO,
	SHIP_AI_BOARD,
	SHIP_AI_RETRIEVE_CREW_FROM,
	SHIP_AI_HAVE_CREW_REPAIR,
	SHIP_AI_HAVE_CREW_SABOTAGE,
	SHIP_AI_MAKE_DERELICT,
	SHIP_AI_HAIL,
	SHIP_AI_MAX_GOAL_ACTIONS
};

struct ai_ship_goal {
	int					goal_action; // verb // e.g. SHIP_AI_ATTACK
	idEntity*			goal_entity; // noun // e.g. playership

	// constructors:
	ai_ship_goal(){
		goal_action = SHIP_AI_IDLE; goal_entity = NULL;
	}
    ai_ship_goal(int Verb, idEntity* Noun) { 
        goal_action = Verb; goal_entity = Noun; 
    }
};
// SHIP AI EVENTS END

enum {
	MEDICALCREWID, // ->MedicalOfficer
	ENGINESCREWID, // ->EnginesOfficer
	WEAPONSCREWID, // ->WeaponsOfficer
	TORPEDOSCREWID, // ->TorpedosOfficer
	SHIELDSCREWID, // ->ShieldsOfficer
	SENSORSCREWID, // ->SensorsOfficer
	ENVIRONMENTCREWID, // ->EnvironmentOfficer
	COMPUTERCREWID, // ->crew[COMPUTERCREWID]
	SECURITYCREWID, // ->SecurityOfficer
	CAPTAINCREWID, // ->crew[CAPTAINCREWID]
	MAX_CREW_ON_SHIPS
};


static const idStr role_description[MAX_CREW_ON_SHIPS] = { 
	"medical", 
	"engines", 
	"weapons", 
	"torpedos", 
	"shields", 
	"sensors", 
	"environment", 
	"computer", 
	"security", 
	"captain" 
};

enum {
	MEDICALMODULEID, // ->MedicalConsole->ControlledModule 
	ENGINESMODULEID, // ->EnginesConsole->ControlledModule
	WEAPONSMODULEID, // ->WeaponsConsole->ControlledModule
	TORPEDOSMODULEID, // ->TorpedosConsole->ControlledModule
	SHIELDSMODULEID, // ->ShieldsConsole->ControlledModule
	SENSORSMODULEID, // ->SensorsConsole->ControlledModule
	ENVIRONMENTMODULEID, // ->EnvironmentConsole->ControlledModule
	COMPUTERMODULEID, // ->ComputerConsole->ControlledModule
	SECURITYMODULEID, // ->SecurityConsole->ControlledModule
	MAX_MODULES_ON_SHIPS
};

static const idStr module_description[MAX_MODULES_ON_SHIPS] = { 
	"medical", 
	"engines", 
	"weapons", 
	"torpedos", 
	"shields", 
	"sensors", 
	"environment", 
	"computer", 
	"security"
};

static const idStr module_description_upper[MAX_MODULES_ON_SHIPS] = { 
	"Medical", 
	"Engines", 
	"Weapons", 
	"Torpedos", 
	"Shields", 
	"Sensors", 
	"Environment", 
	"Computer", 
	"Security"
};

enum {
	MEDICALROOMID, // ->MedicalRoomNode
	ENGINESROOMID, // ->EnginesRoomNode
	WEAPONSROOMID, // ->WeaponsRoomNode
	TORPEDOSROOMID, // ->TorpedosRoomNode
	SHIELDSROOMID, // ->ShieldsRoomNode
	SENSORSROOMID, // ->SensorsRoomNode
	ENVIRONMENTROOMID, // ->EnvironmentRoomNode
	COMPUTERROOMID, // ->ComputerRoomNode
	SECURITYROOMID, // ->SecurityRoomNode
	MISCONEROOMID,
	MISCTWOROOMID,
	MISCTHREEROOMID,
	MISCFOURROOMID,
	MAX_ROOMS_ON_SHIPS
};

static const idStr room_description[MAX_ROOMS_ON_SHIPS] = { 
	"medical", 
	"engines", 
	"weapons", 
	"torpedos", 
	"shields", 
	"sensors", 
	"environment", 
	"computer", 
	"security",
	"misc_one",
	"misc_two",
	"misc_three",
	"misc_four"
};


/*
static const char *crew_roles[ MAX_CREW_ON_SHIPS ] = {
	"medical", 
	"engines", 
	"weapons", 
	"torpedos", 
	"shields", 
	"sensors", 
	"environment", 
	"computer", 
	"security", 
	"captain" 
};
*/

// BOYETTE GAMEPLAY BALANCING BEGIN
static const float module_base_charge_per_cycle[MAX_MODULES_ON_SHIPS] = { 
	4.0f, // medical
	0.2f, // engines
	1.0f, // weapons
	1.0f, // torpedos
	5.0f, // shields
	1.0f, // sensors
	50.0f, // environment
	1.0f, // computer
	4.0f  // security
};
// BOYETTE GAMEPLAY BALANCING END

class sbSpaceEntity : public idEntity {

public:
			CLASS_PROTOTYPE( sbSpaceEntity );

protected:

};

class sbShip : public sbSpaceEntity {

public:
			CLASS_PROTOTYPE( sbShip ); //the necessary idClass prototypes

						sbShip();
						~sbShip( void );

	sbShip*				TargetEntityInSpace;			// boyette mod
	//sbModule*			CurrentTargetModule;		// boyette mod  // this is not used currently 03 18 13

	idPortalSky*		MySkyPortalEnt;			// boyette mod
	bool				alway_snap_to_my_sky_portal_entity;

	// this can probably be deleted now
	idEntity*			TestHideShipEntity;

	// used for the ship diagram on the CaptainMenu
	idEntity*			ShipDiagramDisplayNode; // This will be used to calculate the position of entities on the ship diagrams.
	idDict				spawnArgs_adjusted_ShipDiagramDisplayNode;

	// Ship Crew
	idAI*	crew[MAX_CREW_ON_SHIPS];
	idDict	spawnArgs_adjusted_crew[MAX_CREW_ON_SHIPS];

	std::vector<idDict>	reserve_Crew;
	int					max_reserve_crew;

	// AI's on board this ship - includes crew[MAX_CREW_ON_SHIPS]
    std::vector<idAI*>	AIsOnBoard; // boyette mod
	std::vector<idDict>	spawnArgs_adjusted_AIsOnBoard;

	// room nodes used as move targets - should usually be between the console and the module
	idEntity*			room_node[MAX_ROOMS_ON_SHIPS];
	idDict				spawnArgs_adjusted_room_node[MAX_ROOMS_ON_SHIPS];

	// Ship Consoles (each one has the abilty to be assigned a module in its def).
	sbConsole*			consoles[MAX_MODULES_ON_SHIPS];
	idDict				spawnArgs_adjusted_consoles[MAX_MODULES_ON_SHIPS];
	idDict				spawnArgs_adjusted_consoles_ControlledModule[MAX_MODULES_ON_SHIPS];
	/*
	sbConsole*			MedicalConsole;
	sbConsole*			EnginesConsole;
	sbConsole*			WeaponsConsole;
	sbConsole*			TorpedosConsole;
	sbConsole*			ShieldsConsole;
	sbConsole*			SensorsConsole;
	sbConsole*			EnvironmentConsole;
	sbConsole*			ComputerConsole;
	sbConsole*			SecurityConsole;
	*/

	// the captain sits in this (or the player if the player is the captain) - it is also used to take over other ships
	sbCaptainChair*		CaptainChair;
	idDict				spawnArgs_adjusted_CaptainChair;

	// the ready room chair - has various text and visual things to expand the story of the game
	sbCaptainChair*		ReadyRoomCaptainChair;
	idDict				spawnArgs_adjusted_ReadyRoomCaptainChair;

	// the shields of the ship in the skybox - it flashes when the ship takes damage when it's shields are up
	idEntity*			ShieldEntity;
	idDict				spawnArgs_adjusted_ShieldEntity;

	// used for transporter pads and it's visual effects
	idEntity*			TransporterBounds;
	idDict				spawnArgs_adjusted_TransporterBounds;
	sbTransporterPad*	TransporterPad;
	idDict				spawnArgs_adjusted_TransporterPad;
	idEntity*			TransporterParticleEntitySpawnMarker;
	idDict				spawnArgs_adjusted_TransporterParticleEntitySpawnMarker;

	// the ship's viewscreen
	idEntity*			ViewScreenEntity;
	idDict				spawnArgs_adjusted_ViewScreenEntity;

	// Doors on board the ship
	idList< idEntityPtr<idEntity> >	shipdoors;		// These doors are controlled/effected by the security module.
	std::vector<idDict>				spawnArgs_adjusted_shipdoors;
	std::vector<idDict>				spawnArgs_adjusted_shipdoors_partners;

	// Light models on board the ship - each one should have at least one idLight entity as a target(but it doesn't need one).
	idList< idEntityPtr<idEntity> >	shiplights;		// when this red alert is called these entities will start flashing red.
	std::vector<idDict>				spawnArgs_adjusted_shiplights;
	std::vector<idDict>				spawnArgs_adjusted_shiplights_targets;

	// BOYETTE GAMEPLAY BALANCING BEGIN
	virtual void		SetInitialShipAttributes();
	virtual void		UpdateShipAttributes();
	virtual void		PrintShipAttributes();

	int					module_max_powers[MAX_MODULES_ON_SHIPS];
	// BOYETTE GAMEPLAY BALANCING END


	virtual void		BeginTransporterSequence();
	virtual void		InitiateTransporter();
	virtual void		BeginRetrievalTransportSequence();
	virtual void		InitiateRetrievalTransport();

	const idDict *					TransporterParticleEntityDef;
	idEntityPtr<idFuncEmitter>		TransporterParticleEntityFX;
	virtual void					TriggerShipTransporterPadFX();

	float				min_shields_percent_for_blocking_foreign_transporters;

	int					verified_warp_stargrid_postion_x;
	int					verified_warp_stargrid_postion_y;
	virtual bool		AttemptWarp(int stargriddestx,int stargriddesty);
	virtual void		EngageWarp(int stargriddestx,int stargriddesty);
	virtual void		GivePlayerAppropriatePDAEmailsAndVideosForStargridPosition(int stargriddestx,int stargriddesty);
	virtual void		ClaimUnnoccupiedSkyPortalEntity();
	virtual void		ClaimUnnoccupiedPlayerSkyPortalEntity();
	virtual void		ClaimSpecifiedSkyPortalEntity( idPortalSky* skyportal_to_claim );

	virtual void		DoStuffAfterAllMapEntitiesHaveSpawned();
	virtual void		DoStuffAfterPlayerHasSpawned();
	void				Event_UpdateBeamVisibility( void );
	void				Event_UpdateShieldEntityVisibility( void );
	void				Event_CheckTorpedoStatus( void );
	void				Event_TestScriptFunction( void );
	void				Event_SetMinimumModulePowers( void );
	void				Event_HandleBeginningShipDormancy( void );
	void				Event_SetTargetEntityInSpace( void );
	void				Event_EngageWarp( void );

	void				Event_InitiateTransporter( void );
	void				Event_InitiateRetrievalTransport( void );

	void				Event_InitiateOffPadRetrievalTransport( idEntity* entity_to_transport = NULL );
	void				Event_InitiateOffPadRetrievalTransportToReserveCrew( idEntity* entity_to_transport = NULL );

	void				UpdatePlayerShipQuickWarpStatus();

	void				Event_DisplayStoryWindow( void );
	bool				story_window_satisfied;
	void				Event_StartSynchdRedAlertFX( void );

	void				Event_UpdateViewscreenCamera( void );
	void				ScheduleUpdateViewscreenCamera( int ms_from_now = 0 );  // boyette 05 21 2016

	bool				should_warp_in_when_first_encountered;
	void				SetupWarpInVisualEffects( int ms_from_now = 0 );
	void				Event_DoWarpInVisualEffects();
	void				Event_FinishWarpInVisualEffects();

	virtual void		Spawn( void );
	virtual void		Save( idSaveGame *savefile ) const;
	virtual void		Restore( idRestoreGame *savefile );
	virtual void		Think( void );

	// Functions run on ::Think
	virtual void		VerifyCrewMemberPointers();
	virtual void		UpdateModuleCharges();
	virtual void		CheckForModuleActions();


	virtual bool		ValidEntity(idEntity *ent);
	virtual void		EntityEncroaching(idEntity *ent);

	int					GetCaptainTestNumber(); // boyette mod
	virtual void		RecieveVolley();		// boyette mod

	virtual void		GiveCrewMoveCommand(idEntity* MoveToEntity, sbShip* ShipToBeAboard );	// boyette mod
	virtual void		SetSelectedCrewMember(idAI* SelectedAI);		// boyette mod
	idAI*				SelectedCrewMember;								// boyette mod
    std::vector<idAI*>	SelectedCrewMembers;							// boyette mod
	virtual void		AddCrewMemberToSelection(idAI* SelectedAI);		// boyette mod
	virtual void		ClearCrewMemberSelection();						// boyette mod
	virtual void		ClearCrewMemberMultiSelection();				// boyette mod

	bool				IsThisAIACrewmember( idAI* ai_to_check );

	virtual void		SetSelectedModule(sbModule* SelectedsbModule);		// boyette mod
	sbModule*			SelectedModule;										// boyette mod
	virtual void		ClearModuleSelection();								// boyette mod

	int					ReturnOnBoardEntityDiagramPositionX(idEntity *ent);		// boyette mod
	int					ReturnOnBoardEntityDiagramPositionY(idEntity *ent);		// boyette mod
	int					ReturnOnBoardEntityDiagramPositionZ(idEntity *ent);		// boyette mod - Not used yet - maybe one day.

	int					maximum_power_reserve;
	int					current_power_reserve;
	virtual void		IncreaseModulePower(sbModule *module);
	virtual void		DecreaseModulePower(sbModule *module);

	virtual void		ReturnToReserveExcessPowerFromDamagedModules();

	// phaser
	idBeam*				beam;
	idBeam*				beamTarget;
	bool				weapons_shot_missed;

	// torpedo
	idProjectile*		CreateProjectile( const idVec3 &pos, const idVec3 &dir );
	const idDict *		projectileDef;
	idEntityPtr<idProjectile> projectile; // for the ship's torpedo
	bool				torpedo_shot_missed;

	idEntityPtr<idProjectile> projectilearray[9]; // I'm not sure what this is used for - might want to get rid of this now - need to put this on the module - and need to replace the nine with a MAX_MODULE_PROJECTILES constant. // BOYETTE NOTE TODO IMPORTANT: I think we can delete this from sbShip - it is handled directly on sbModule now.

	const idDict *					damageFXDef;
	idEntityPtr<idFuncEmitter>			damageFX;
	//idEntityFx*					damageFXptr;

	bool				set_as_playership_on_player_spawn;

	idStr				ShipStargridIcon;
	idStr				ShipImageVisual;

	idStr				ShipStargridArtifactIcon;

	bool				track_on_stargrid; // we don't use this on sbShip because we assume all non-stationionary sbShips will be tracked. It is useful on sbStationarySpaceEntity because currently only some stationary entities (like spacestations and phenomena) will be tracked. (planets and moons will not normally be tracked).

	int					shieldStrength;
	int					max_shieldStrength;
	int					hullStrength;
	int					max_hullStrength;
	virtual void		DealShipToShipDamageWithWeapons();
	virtual void		DealShipToShipDamageWithTorpedos();
	int					weapons_damage_modifier; // we'll get this from the spawn args - other than this, it is the same for every ship - dependent on module efficiency which increases charge speed. Some races' ships will have naturally more powerful weapons(better against shields).
	int					torpedos_damage_modifier; // we'll get this from the spawn args - other than this, it is the same for every ship - dependent on module efficiency which increases charge speed. Some races' ships will have naturally more powerful torpedos(better against hull/structure).

	virtual void		FlashShieldDamageFX(int duration);
	int					shields_repair_per_cycle;

	void				CheckWeaponsTargetQueue();
	void				CheckTorpedosTargetQueue();

	int					WeaponsTargetQueue[MAX_MODULES_ON_SHIPS];
	int					TorpedosTargetQueue[MAX_MODULES_ON_SHIPS];

	// maybe do just two swap functions instead.
	void				MoveUpModuleInWeaponsTargetQueue(int ModuleID);
	void				MoveDownModuleInWeaponsTargetQueue(int ModuleID);
	void				MoveUpModuleInTorpedosTargetQueue(int ModuleID);
	void				MoveDownModuleInTorpedosTargetQueue(int ModuleID);

	// need a swap template as well which take element numbers as arguments (and not ModuleID values like the ones above).
	void				SwapModueleIDInWeaponsTargetQueue(int ModuleID,int swap_to_pos);
	void				SwapModueleIDInTorpedosTargetQueue(int ModuleID,int swap_to_pos);

	// autofire boolean.
	bool	weapons_autofire_on;
	bool	torpedos_autofire_on;

	bool	ship_is_firing_weapons;
	bool	ship_is_firing_torpedo;

	void	SetTargetEntityInSpace(sbShip* SpaceEntityToTarget);
	sbShip*	TempTargetEntityInSpace;

	// ship dialogue system:
	std::bitset<MAX_DIALOGUE_BRANCHES>		hailDialogueBranchTracker; // so the first bit will spawn as set to true - all others will be false. Because we always want the dialogue to start with the first branch. When the dialoge is finished, set the last bit to true. That way hailing will just produce a "no reponse" or something like that screen.
	std::string								hailDialogueBranchTrackerString;
	void	OpenHailWithLocalPlayer();
	bool	currently_in_hail;

	idStr	hail_dialogue_gui_file;
	int		friendlinessWithPlayer;
	bool	has_forgiven_player;
	bool	is_ignoring_player;

	// not sure if this function is necessary for the module power priority queue.
	void	AutoManageModulePowerlevels();
	// Modules power priority queue
	int		ModulesPowerQueue[MAX_MODULES_ON_SHIPS];
	// Modules power auto-manage boolean.
	bool	modules_power_automanage_on;
	// Move module up or down in priority queue.
	void	MoveUpModuleInModulesPowerQueue(int ModuleID);
	void	MoveDownModuleInModulesPowerQueue(int ModuleID);
	// swap positions in the module priority power queue.
	void	SwapModueleIDInModulesPowerQueue(int ModuleID,int swap_to_pos);
	// desired automanage power levels
	int					current_automanage_power_reserve;
	// increase/decrease target levels for automanage module power
	virtual void		IncreaseAutomanageTargetModulePower(sbModule *module);
	virtual void		DecreaseAutomanageTargetModulePower(sbModule *module);

	// returns all automanage power to the automanage power reserves
	virtual void		ResetAutomanagePowerReserves();

	// module charges counter/timer reference to update moduel charges.
	int		update_module_charges_timer;

	// discovery system for the stargrid map
	bool	discovered_by_player;

	// show the artifact locations on the stargrid map
	bool	has_artifact_aboard;

	// oxygen system
	int					current_oxygen_level;
	int					evaluate_current_oxygen_timer;
	virtual void		EvaluateCurrentOxygenLevelAndOxygenActions();
	virtual void		LowOxygenDamageToAIOnBoard(float damage_scale);
	bool				infinite_oxygen;	// used for planets - if set to true it ignores the oxygen calculations so oxygen is always full.

	// Battlestations
	virtual void		SendCrewToBattlestations();

	// Clear Weapons and Torpedos Module Targets - and set autofire to false
	virtual void		CeaseFiringWeaponsAndTorpedos();
	bool				player_ceased_firing_on_this_ship; // BOYETTE NOTE: this is a thing to track for when the player ceases fire by pressing a button on the captain gui - so we don't start shooting at them again.

	// Computer module buff modifier calculations
	virtual void		UpdateCurrentComputerModuleBuffModifiers();

	// utility function to recalculate all module efficiencies
	virtual void		RecalculateAllModuleEfficiencies();

	// ship lights functions and variables
	virtual void		SynchronizeRedAlertSpecialFX();
	virtual void		StopRedAlertSpecialFX();
	bool				red_alert;
	virtual void		GoToRedAlert();
	virtual void		CancelRedAlert();
	virtual void		TurnShipLightsOff();
	virtual void		DimRandomShipLightsOff();
	virtual void		DimShipLightsOff();
	virtual void		TurnShipLightsOn();

	// ship door functions
	virtual void		UpdateShipDoorHealths();
	int					old_security_module_efficiency;

	// update guis on consoles and modules;
	virtual void		UpdateGuisOnConsolesAndModules();

	std::vector<int>	neutral_teams;		// These are teams that this ship (and should make sure other ships on the same team) team has a non-hostility agreement with.
	std::vector<int>	SplitStringToInts(const std::string &s, char delim);
	void				EndNeutralityWithTeam(int team_variable);
	void				EndNeutralityWithShip(sbShip* ship_to_not_be_neutral);
	void				StartNeutralityWithTeam(int team_variable);
	void				StartNeutralityWithShip(sbShip* ship_to_be_neutral);
	bool				HasNeutralityWithTeam(int team_variable);
	bool				HasNeutralityWithShip(sbShip* ship_to_test);
	bool				HasNeutralityWithAI(idAI* ai_to_test);
	void				BecomeASpacePirateShip(); // all entities related to this ship (crewmembers, modules, consoles, etc) should be set to a team number that no other entity has. neutral_teams should be cleared. if this is the playership - friendlinessWithPlayer should be set to zero for all other ships. if this is the playership it should also change the team for the gameLocal.GetLocalPlayer() entity.
	void				RedeemFromBeingASpacePirateShip( sbShip* redeemer_ship ); // all entities related to this ship (crewmembers, modules, consoles, etc) should be set to the team number of the ship that has redeemed you. You will be given its neutral_teams as well.  if this is the playership - friendlinessWithPlayer should be set to default (spawnArgs) for all other ships. if this is the playership it should also change the team for the gameLocal.GetLocalPlayer() entity.

	void				SyncUpOurNeutralityWithThisTeam( int new_neutral_team );
	void				SyncUpOurBrokenNeutralityWithThisTeam( int new_enemy_team );

	bool				CanBeTakenCommandOfByPlayer();
	void				UpdateGuisOnCaptainChair();
	void				UpdateViewScreenEntityVisuals();

	void				UpdateGuisOnTransporterPad();
	void				UpdateGuisOnTransporterPadEveryFrame();

	void				HandleModuleAndConsoleGUITimeCommandsEvenIfGUIsNotActive();

	bool				is_derelict;
	void				BecomeDerelict();
	void				BecomeNonDerelict();
	bool				never_derelict;

	bool				do_red_alert_when_derelict;

	void				ChangeTeam(int new_team);

	void				MaybeSpawnAWreck();

	void				BeginShipDestructionSequence(); // BOYETTE NOTE TODO: need to add countdown voice and warnings
	void				Event_ConcludeShipDestructionSequence( void );
	bool				ship_destruction_sequence_initiated;

	void				InitiateSelfDestructSequence(); // will have a self destruct button on captain gui. - might be fun to have to enter a command code that the player can customize. i.e. - boyette delta delta epsilon alpha
	void				CancelSelfDestructSequence(); // will have a self destruct button on captain gui. - might be fun to have to enter a command code that the player can customize. i.e. - boyette delta delta epsilon alpha
	void				EvaluateSelfDestructSequenceTimer(); // will have a self destruct button on captain gui. - might be fun to have to enter a command code that the player can customize. i.e. - boyette delta delta epsilon alpha
	bool				ship_self_destruct_sequence_initiated;
	int					ship_self_destruct_sequence_timer;
	idStr				original_name;

	int					current_materials_reserves; // used for ship repair and trading with far off species who don't use interstellar credits.
	int					current_currency_reserves; // interstellar credits.

	void				GiveThisShipCurrency( int amount_to_give = 0 );
	void				GiveThisShipMaterials( int amount_to_give = 0 );

	bool				in_repair_mode;
	void				InitiateShipRepairMode();
	void				CancelShipRepairMode();
	void				Event_EvaluateShipRepairModeCycle( void );

	void				RepairShipWithMaterials(int amount_to_repair);

	void				ReduceAllModuleChargesToZero();

	bool				CheckForHostileEntitiesAtCurrentStarGridPosition();
	bool				CheckForHostileEntitiesOnBoard();

	bool				CanHireCrew();
	bool				CanHireReserveCrew();
	int					HireCrew(const char* crew_to_hire_entity_def_name); // returns the CREWID int of the position that was filled.
	bool				InviteAIToJoinCrew( idAI* ai_to_invite = NULL, bool force_transport = false, bool attempt_transport = false, bool allow_to_join_reserves = false, int transport_delay = 2500 );

	bool				ship_deconstruction_sequence_initiated;
	bool				can_be_deconstructed;
	beamTarget_t		ShipBeam;
	bool				ship_beam_active;
	void				CreateBeamToEnt( idEntity* ent );
	void				UpdateBeamToEnt();
	void				RemoveBeamToEnt();

	void				HandleTransporterEventsOnPlayerGuis();

	sbShip*				ship_that_just_fired_at_us;

	bool				always_neutral; // BOYETTE NOTE TODO: this variable isn't used for anything I believe. We can probably delete it.

	idVec3				fx_color_theme;

	bool				was_sensor_scanned;

	bool				shields_raised;
	int					shieldStrength_copy;
	void				LowerShields();
	void				RaiseShields();

	bool				ship_lights_on;

	idVec3				ReturnSuitableTorpedoLaunchPoint();
	bool				CheckTorpedoLaunchPoint(idVec3 start,idVec3 end);
	idVec3				suitable_torpedo_launchpoint_offset;

	idVec3				ReturnSuitableWeaponsOrTorpedoTargetPointForMiss();
	bool				CheckWeaponsOrTorpedoTargetPointForMiss(idVec3 start,idVec3 end);

	std::vector<sbShip*>	ships_at_my_stargrid_position;		// These are teams that this ship (and should make sure other ships on the same team) team has a non-hostility agreement with.
	idStr					stargridstartpos_random_team;		// This is used to group certain space entities that we want to start at the same random stargrid position. For example, a couple ships that will be involved in a dispute. Default is "". Team must be set to something other than "" for this to work. Overrides stargridstartpos_try_to_be_alone  and stargridstartpos_avoid_entities_of_same_class spawnargs
	bool					stargridstartpos_try_to_be_alone;	// If stargridstartpos_random(or stargridstartposx_random or stargridstartposy_random) is set to 1 and this is also set to 1 then this ship will try to be at its own stargrid position when the map spawns. Default is 0.
	bool					stargridstartpos_avoid_entities_of_same_class;		// If stargridstartpos_random(or stargridstartposx_random or stargridstartposy_random) is set to 1 and this is also set to 1 then this ship will try to avoid other entities of the same type as itself at stargrid positions when the map spawns. Default is 0.
	void					LeavingStargridPosition(int leaving_pos_x,int leaving_pos_y); // all this does now is update ships_at_my_stargrid_position for all the ships at the stargrid position we are leaving
	void					ArrivingAtStargridPosition(int arriving_pos_x,int arriving_pos_y); // all this does now is update ships_at_my_stargrid_position for all the ships at the stargrid position we are arriving to

	void				SetToNullAllPointersToThisEntity();

	// SHIP AI EVENTS BEGIN
	virtual bool		ShouldConstructScriptObjectAtSpawn( void ) const;

	void				Event_IsPlayerShip( void );
	void				Event_IsAtPlayerSGPosition( void );
	void				Event_IsAtPlayerShipSGPosition( void );
	void				Event_SpaceEntityIsAtMySGPosition( idEntity* entity_to_check );
	void				Event_IsDerelict( void );
	void				Event_IsDerelictShip( idEntity* ship_to_check );
	void				Event_IsFriendlyShip( idEntity* ent );
	void				Event_IsHostileShip( idEntity* ent );
	bool				IsHostileShip( idEntity* ent );

	void				Event_SetMainGoal( int verb, idEntity* noun );

	void				Event_AddMiniGoal( int verb, idEntity* noun );
	void				Event_RemoveMiniGoal( int verb, idEntity* noun );
	void				Event_RemoveMiniGoalAction( int verb );
	void				Event_ClearMiniGoals();
	void				Event_NextMiniGoal();
	void				Event_PreviousMiniGoal();
	void				Event_PrioritizeMiniGoal( int verb, idEntity* noun );

	void				Event_ReturnCurrentGoalAction();
	void				Event_ReturnCurrentGoalEntity();
	void				Event_ReturnHostileTargetingMyGoalEntity();

	void				Event_SetTargetShipInSpace( idEntity* target );
	void				Event_ClearTargetShipInSpace();
	void				Event_ReturnTargetShipInSpace();
	void				Event_GetATargetShipInSpace();

	void				Event_ReturnBestFriendlyToProtect();
	void				Event_WeAreAProtector();

	void				Event_ShipAIAggressiveness();

	void				Event_CrewAllAboard();
	void				Event_CrewIsAboard( idEntity* ship_to_check );
	void				Event_SpareCrewIsNotOnTransporterPad();
	void				Event_OrderSpareCrewToMoveToTransporterPad();
	void				Event_OrderSpareCrewToReturnToBattlestations();
	void				Event_InitiateShipTransporter();
	void				Event_ShipIsSuitableForBoarding( idEntity* ship_to_check );

	void				Event_ShipShieldsAreLowEnoughForBoarding( idEntity* ship_to_check );

	void				Event_IsCrewRetreivableFrom( idEntity* ship_to_check );
	void				Event_ShipWithOurCrewAboardIt();
	void				Event_CrewOnBoardShipIsNotOnTransporterPad( idEntity* ship_to_check );
	void				Event_OrderCrewOnBoardShipToMoveToTransporterPad( idEntity* ship_to_check );
	void				Event_InitiateShipRetrievalTransporter();

	void				Event_ActivateAutoModeForCrewAboardShip( idEntity* ship_to_check );
	void				Event_DeactivateAutoModeForCrewAboardShip( idEntity* ship_to_check );

	void				Event_PrioritizeWeaponsAndTorpedosModulesInMyAutoPowerQueue();
	void				Event_PrioritizeEnginesInMyAutoPowerQueue();
	void				Event_PrioritizeEnginesShieldsOxygenWeaponsAndTorpedosModulesInMyAutoPowerQueue();
	void				Event_TurnAutoFireOn();
	void				Event_CeaseFire();

	void				Event_CanAttemptWarp();
	void				Event_AttemptWarpTowardsEntityAvoidingHostilesIfPossible( idEntity* ent );
	void				Event_AttemptWarpAwayFromEntityAvoidingHostilesIfPossible( idEntity* ent );

	ai_ship_goal				main_goal;
	std::vector<ai_ship_goal>	mini_goals;
	int							current_goal_iterator;

	bool				we_are_a_protector;

	bool				ai_always_targets_random_module;

	int					ship_ai_aggressiveness;

	int					max_spare_crew_size;
	int					max_modules_to_take_spare_crew_from;

	bool				ignore_boarding_problems;
	float				min_hullstrength_percent_required_for_boarding;
	int					min_environment_module_efficiency_required_for_boarding;
	float				min_oxygen_percent_required_for_boarding;

	bool				boarders_should_target_player;
	bool				boarders_should_target_random_module;
	int					boarders_should_target_module_id;

	bool				in_no_action_hail_mode;
	bool				should_hail_the_playership;
	// boyette today begin
	float				wait_to_hail_order_num;
	// boyette today end
	bool				should_go_into_no_action_hail_mode_on_hail;
	void				Event_ShouldHailThePlayerShip();
	void				Event_ReturnWaitToHailOrderNum();
	void				Event_PlayerIsInStarGridStoryWindowOrHailOrItIsNotOurTurnToHail();
	void				Event_AttemptToHailThePlayerShip();
	void				Event_InNoActionHailMode();
	void				Event_PutAllShipsAtTheSameSGPosIntoNoActionHailMode();
	void				Event_ExitAllShipsAtTheSameSGPosFromNoActionHailMode();

	void				Event_HasFledFromShip( idEntity* ship_to_check );
	void				Event_HasEscapedFromUs( idEntity* ship_to_check );

	void				Event_GoToRedAlert();
	void				Event_CancelRedAlert();
	void				Event_RaiseShields();
	void				Event_LowerShields();
	void				Event_BattleStations();
	bool				battlestations;
	bool				is_attempting_warp;

	int					successful_flee_distance;

	void				Event_ShouldAlwaysMoveToPrioritySpaceEntityToTarget();
	void				Event_ReturnPrioritySpaceEntityToTarget();
	idEntity*			priority_space_entity_to_target;
	bool				prioritize_playership_as_space_entity_to_target;

	void				Event_ShouldAlwaysMoveToSpaceEntityToProtect();
	void				Event_ShouldAlwaysMoveToPrioritySpaceEntityToProtect();
	void				Event_ReturnPrioritySpaceEntityToProtect();
	idEntity*			priority_space_entity_to_protect;
	bool				prioritize_playership_as_space_entity_to_protect;

	void				HandleEmergencyOxygenSituation();
	void				Event_OptimizeModuleQueuesForIdleness();
	void				Event_OptimizeModuleQueuesForFleeing();
	void				Event_OptimizeModuleQueuesForDefending();
	void				Event_OptimizeModuleQueuesForAttacking();
	void				Event_OptimizeModuleQueuesForSeekingAndDestroying();
	void				Event_OptimizeModuleQueuesForMoving();
	void				Event_OptimizeModuleQueuesForBoarding();
	void				Event_OptimizeModuleQueuesForRetrievingCrew();

	void				Event_ReturnExtraShipAIWaitTimeForLowSensorsModuleEfficiency();

	// ship ai become dormant conditions
	bool				ship_begin_dormant;
	bool				ship_is_never_dormant;
	bool				ship_tries_to_be_dormant_when_not_at_player_shiponboard_sg_pos;
	bool				ship_tries_to_be_dormant_when_not_at_active_ship_sg_pos;
	bool				ship_is_dormant_until_awoken_by_player_shiponboard;
	bool				ship_is_dormant_until_awoken_by_an_active_ship;

	bool				ship_modules_must_be_repaired_to_go_dormant;

	bool				try_to_be_dormant;

	// SHIP DESPAWNING/RESPAWNING SYSTEM BEGIN
	virtual void		DespawnShipInteriorEntities();
	virtual void		RespawnShipInteriorEntities();
	idEntity*			SpawnShipInteriorEntityFromAdjustedidDict( const idDict &dict_to_spawn );
	// SHIP DESPAWNING/RESPAWNING SYSTEM END

	void				Event_IsDormantShip();
	void				Event_ShouldBecomeDormantShip();
	void				Event_BecomeDormantShip();
	void				Event_BecomeNonDormantShip();


	void				Event_ActivateShipAutoPower();
	void				Event_AttemptWarpTowardsShipPlayerIsOnBoard();

	void				Event_DetermineDefensiveActionsForSpareCrewOnBoard();
	void				Event_AllModulesAreFullyRepaired();
	void				Event_ActivateAutoModeForAllCrewAboardShip();
	void				Event_DeactivateAutoModeForAllCrewAboardShip();
	void				Event_SendCrewToStations();

	void				Event_DoPhenomenonActions();

	void				Event_ShipShouldFlee();
	float				flee_hullstrength_percentage;

	// hail conditions begin
	bool				hail_conditionals_met;
	float				hail_conditional_hull_below_this_percentage;
	bool				hail_conditional_no_hostiles_at_my_stargrid_position;
	bool				hail_conditional_hostiles_at_my_stargrid_position;
	bool				hail_conditional_no_friendlies_at_my_stargrid_position;
	bool				hail_conditional_friendlies_at_my_stargrid_position;
	bool				hail_conditional_captain_officer_killed;
	bool				hail_conditional_player_is_aboard_playership;
	bool				hail_conditional_not_at_player_shiponboard_position;
	bool				hail_conditional_is_playership_target;
	// hail conditions end
	// SHIP AI EVENTS END

	bool				CheckDormant();
	bool				DoDormantTests();


	idAI*	SpawnEntityDefOnShip( const char* spawn_entity_def_name = NULL, idEntity* entity_to_spawn_on = NULL, int team_spawnarg_to_set = 0 );
	void	PhenomenonDealShipToShipDamage( sbShip* PhenomenonTarget = NULL, int phenomenon_module_id = 0, int phenomenon_damage_amount = 100 );
	// phenomenon actions variables begin
	bool	phenomenon_show_damage_or_disable_beam;
	bool	phenomenon_should_do_ship_damage;
	bool	phenomenon_should_damage_modules;
	int			phenomenon_module_damage_amount;
	std::vector<int>	phenomenon_module_ids_to_damage;
	bool	phenomenon_should_damage_random_module;
	bool	phenomenon_should_disable_modules;
	std::vector<int>	phenomenon_module_ids_to_disable;
	bool	phenomenon_should_disable_random_module;
	bool	phenomenon_should_set_oxygen_level;
	int			phenomenon_oxygen_level_to_set;
	bool	phenomenon_should_set_ship_shields_to_zero;
	bool	phenomenon_should_spawn_entity_def_on_playership;
	int			phenomenon_number_of_entity_defs_to_spawn;
	idStr	phenomenon_entity_def_to_spawn;
	bool	phenomenon_should_change_random_playership_crewmember_team;
	bool	phenomenon_should_make_everything_go_slowmo;
	bool	phenomenon_should_toggle_slowmo_on_and_off;
	bool	phenomenon_should_ignore_the_rest_of_the_ship_ai_loop;
	// phenomenon actions variables end

	bool	play_low_oxygen_alert_sound;
	bool	show_low_oxygen_alert_display;

protected:
	int					regenAmount;



};



class sbPlayerShip : public sbShip {

public:
			CLASS_PROTOTYPE( sbPlayerShip );

/*
	virtual void		SetWarpDestination(int x, int y);
	virtual void		SetWarpPosition(int x, int y);
*/
protected:

};


class sbEnemyShip : public sbShip {

public:
			CLASS_PROTOTYPE( sbEnemyShip );

protected:

};


class sbFriendlyShip : public sbShip {

public:
			CLASS_PROTOTYPE( sbFriendlyShip );

protected:

};

//////////////////////////////////STATIONARY SPACE ENTITIES////////////////////////////////////////////////
class sbStationarySpaceEntity : public sbShip {

public:
			CLASS_PROTOTYPE( sbStationarySpaceEntity );
			sbStationarySpaceEntity();
			~sbStationarySpaceEntity();
			virtual void		Spawn( void );
			//virtual void		Save( idSaveGame *savefile ) const;
			//virtual void		Restore( idRestoreGame *savefile );
			virtual void		Think( void );
			virtual bool		AttemptWarp(int stargriddestx,int stargriddesty);
			virtual void		EngageWarp(int stargriddestx,int stargriddesty);
			virtual void		ClaimUnnoccupiedSkyPortalEntity();
			virtual void		ClaimUnnoccupiedPlayerSkyPortalEntity();

protected:

};


class sbPlanetaryBody : public sbStationarySpaceEntity {

public:
			CLASS_PROTOTYPE( sbPlanetaryBody );

protected:

};


class sbPhenomenon : public sbStationarySpaceEntity {

public:
			CLASS_PROTOTYPE( sbPhenomenon );

protected:

};

class sbCaptainChair : public idEntity {

public:
			CLASS_PROTOTYPE( sbCaptainChair ); //the necessary idClass prototypes

						sbCaptainChair();

	virtual void		Spawn( void );
	virtual void		Save( idSaveGame *savefile ) const;
	virtual void		Restore( idRestoreGame *savefile );

	sbShip*				ParentShip;

	void				DoStuffAfterAllMapEntitiesHaveSpawned();

	// gui
	virtual bool		HandleSingleGuiCommand( idEntity *entityGui, idLexer *src );

	virtual	void		SetRenderEntityGuisStrings( const char* varName, const char* value );
	virtual	void		SetRenderEntityGuisBools( const char* varName, bool value );
	virtual	void		SetRenderEntityGuisInts( const char* varName, int value );
	virtual	void		SetRenderEntityGuisFloats( const char* varName, float value );
	virtual	void		HandleNamedEventOnGuis( const char* eventName );

	idAngles			min_view_angles;
	idAngles			max_view_angles;
	void				ReleasePlayerCaptain();

	bool				has_console_display;
	void				PopulateCaptainLaptop();
	int					log_page_number;
	int					encyclopedia_page_number;
	bool				ship_directive_overridden;

	bool				in_confirmation_process_of_player_take_command;

	idEntityPtr<idEntity> SeatedEntity;

protected:

};

class sbTransporterPad : public idEntity {

public:
			CLASS_PROTOTYPE( sbTransporterPad ); //the necessary idClass prototypes

						sbTransporterPad();

	virtual void		Spawn( void );
	virtual void		Save( idSaveGame *savefile ) const;
	virtual void		Restore( idRestoreGame *savefile );

	sbShip*				ParentShip;

	void				DoStuffAfterAllMapEntitiesHaveSpawned();

	// gui
	virtual bool		HandleSingleGuiCommand( idEntity *entityGui, idLexer *src );

	virtual	void		SetRenderEntityGuisStrings( const char* varName, const char* value );
	virtual	void		SetRenderEntityGuisBools( const char* varName, bool value );
	virtual	void		SetRenderEntityGuisInts( const char* varName, int value );
	virtual	void		SetRenderEntityGuisFloats( const char* varName, float value );
	virtual	void		HandleNamedEventOnGuis( const char* eventName );

protected:

};




#endif /* __GAME_SBSHIP_H__ */
