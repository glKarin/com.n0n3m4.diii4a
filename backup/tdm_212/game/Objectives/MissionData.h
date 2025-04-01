/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

// Copyright (C) 2006 Chris Sarantos <csarantos@gmail.com>

// TODO: Right now, AI_FindBody stats does not track the stat of team of body found
// AND team of AI finding body.  It only tracks team of AI finding body.
// Whether an AI reacts to a body should probably be determined by the AI, for example,
// average guards wouldn't react to a dead rat, but an elite might.
// If that's the case, we don't have to track both variables in the stats, since
// AI will only call FindBody if they react to that type of body.

#ifndef MISSIONDATA_H
#define MISSIONDATA_H

#include "precompiled.h"

#include "Objective.h"
#include "ObjectiveComponent.h"

#include "../DarkModGlobals.h"
#include "MissionStatistics.h"

#include "EMissionResult.h"

/**
 * greebo: Mission event type enumeration. This lists all possible event types
 * which can be passed to MissionData::HandleMissionEvent. 
 *
 * See further documentation at method declaration.
 *
 * Important: when adding or changing this enum, keep the #defines in
 * tdm_defs.script in sync!
 */
enum EMissionEventType
{
	EVENT_NOTHING = 0,
	EVENT_READABLE_OPENED = 1,
	EVENT_READABLE_CLOSED = 2,
	EVENT_READABLE_PAGE_REACHED = 3,
	EVENT_INVALID,
};

// TODO: move to Game_local.h?
struct SObjEntParms
{
	idStr	name;
	idStr	group; // inventory group for items, e.g., loot group "gems"
	idStr	classname;
	idStr	spawnclass;

	idVec3	origin;

	// AI data:
	int		team;
	int		type;
	int		innocence;

	/**
	* Numerical value, filled by callbacks in some cases for things that are kept
	* track of externally (for example, number of inventory items, overall loot, etc)
	**/
	int value; // should default to 1
	int valueSuperGroup; // Just used to pass overall loot for now

	bool bIsAI;
	bool bWhileAirborne; // a must-have :)

	SObjEntParms()
	{ 
		Clear();
	}

	~SObjEntParms()
	{ 
		Clear();
	}

	/**
	* Initialize the struct to default values
	**/
	void Clear( void )
	{
		name.Clear();
		group.Clear();
		classname.Clear();
		spawnclass.Clear();

		origin = vec3_zero;
		team = -1;
		type = -1;
		innocence = -1;

		value = 1;
		valueSuperGroup = 1;
		bIsAI = false;
		bWhileAirborne = false;
	}
};

/**
* CMissionData handles the tasks of maintaining stats and objective completion status
* in response to ingame events.
*
* Also handles the task of parsing objectives written by FM author
**/

class CMissionData 
{
public:
	friend class CObjective;

	CMissionData();
	virtual ~CMissionData();

	void Clear();

	void Save( idSaveGame *savefile ) const;
	void Restore( idRestoreGame *savefile );

	// Returns the number of objectives
	int GetNumObjectives() const { return m_Objectives.Num(); };

	/**
	* Update objectives if they need it
	* Called each frame by idPlayer::Think, does nothing if no objectives need updating
	**/
	void UpdateObjectives( void );

	/**
	* Set the completion state of an objective.  Called both externally and internally.
	* NOTE: Uses the "internal" index number, so subtract the index by 1 if calling it with "user" index
	* The fireEvents bool can be used to suppress sounds and GUI messages on objective
	* state changes - this is necessary to be able to use this function during Main Menu display
	* where no local player is spawed yet.
	**/
	void SetCompletionState(int objIndex, int state, bool fireEvents = true);

	/**
	* Get completion state.  Uses "internal" index (starts at 0)
	**/
	int GetCompletionState( int ObjIndex );

	/**
	* Get component state.  Uses "internal" index (starts at 0)
	**/
	bool GetComponentState( int ObjIndex, int CompIndex );

	/**
	* SteveL #3741. Eliminate SetComponentState_Ext to align SetComponentState with all 
	* other objective-related functions.
	* 
	* Sets a given component state.  
	* Externally called version: Checks and reports index validity
	* Uses "user" index (starts at 1 instead of 0)
	* Calls internal SetComponentState.
	*
	* void SetComponentState_Ext( int ObjIndex, int CompIndex, bool bState );
	**/

	/**
	* Sets a given component state.
	*
	* SteveL #3741: Moved SetComponentState from protected area to align with 
	* other objective functions, and to allow elimination of SetComponentState_Ext.
	**/
	void SetComponentState( int ObjIndex, int CompIndex, bool bState );

	/**
	* Set component state when indexed by a pointer to a component
	**/
	void SetComponentState( CObjectiveComponent *pComp, bool bState );

	/**
	* Unlatch an irreversible objective (used by scripting)
	* Uses internal index (starts at 0)
	**/
	void UnlatchObjective( int ObjIndex );

	/**
	* Unlatch an irreversible component (used by scripting)
	* Uses internal indeces(starts at 0)
	**/
	void UnlatchObjectiveComp( int ObjIndex, int CompIndex );

	/**
	* Set whether an objective shows up in the player's objectives screen
	* The objective index is 0-based.
	**/
	void SetObjectiveVisibility(int objIndex, bool visible, bool fireEvents = true);

	/**
	* Get objective visibility.  Uses "internal" index (starts at 0)
	**/
	bool GetObjectiveVisibility( int ObjIndex );

	/**
	* Set whether an objective is mandatory or not.
	* The objective index is 0-based.
	**/
	void SetObjectiveMandatory(int objIndex, bool mandatory);

	/**
	* Set the ongoing flag of this objective.
	* The objective index is 0-based.
	**/
	void SetObjectiveOngoing(int objIndex, bool ongoing);

	/**
	* Replace an objective's list of enabling components with a new one
	* Takes a string list of space-delimited ints and parses it in.
	**/
	void SetEnablingObjectives(int objIndex, const idStr& enablingStr);

	/**
	* Tels #3217: Change the text of an objective.
	**/
	void SetObjectiveText(int objIndex, const char *descr);

	/**
	* Getters for the mission stats.  Takes an objective component event type,
	* index for the category (for example, the index would be the team int if 
	* you are calling GetStatByTeam)
	* 
	* The AlertLevel must be specified if you are getting alert stats, but otherwise
	* is optional.
	**/
	SStat* GetStat( EComponentType CompType, int AlertLevel = 0 );
	int GetStatByTeam( EComponentType CompType, int index, int AlertLevel = 0 );
	int GetStatByType( EComponentType CompType, int index, int AlertLevel = 0 );
	int GetStatByInnocence( EComponentType CompType, int index, int AlertLevel = 0 );

	/**
	* The following stat functions don't need an index var, since there is only one of them tracked
	**/
	int GetStatOverall( EComponentType CompType, int AlertLevel = 0 );
	int GetStatAirborne( EComponentType CompType, int AlertLevel = 0);
	unsigned int GetTotalGamePlayTime();
	int GetDamageDealt();
	int GetDamageReceived();
	int GetHealthReceived();
	int GetPocketsPicked();
	int GetFoundLoot();
	int GetMissionLoot();
	int GetTotalTimePlayerSeen();
	int GetNumberTimesPlayerSeen();
	int GetNumberTimesAISuspicious();
	int GetNumberTimesAISearched();
	float GetSightingScore();
	float GetStealthScore();
	int GetSecretsFound();
	int GetSecretsTotal();

	idStr GetDifficultyName(int level); // grayman #3292

	/**
	* Setters for the mission stats.
	**/
	void IncrementPlayerSeen(); // grayman #2887
	void Add2TimePlayerSeen( int amount ); // grayman #2887

	void SetDifficultyNames(idStr _difficultyNames[]); // grayman #3292

	/**
	* Called when the player takes damage.  Used for damage stats
	**/
	void PlayerDamaged( int DamageAmount );

	/**
	* Called when the player damages AI.  Used for damage stats.
	**/
	void AIDamagedByPlayer( int DamageAmount );

	/**
	 * greebo: Gets called if the player gets healed by something
	 */
	void HealthReceivedByPlayer(int amount);

	void ChangeFoundLoot(LootType lootType, int amount);

	/**
	 * greebo: This adds a given amount of loot to the total amount available in the mission.
	 * The total loot value is interesting at the end of the mission where the player wants to
	 * see how much loot he/she missed.
	 */
	void AddMissionLoot(LootType lootType, int amount);

	// Callback functions:

	/**
	* Called by external callbacks when events occur that could effect objectives.
	* This is the most general call and requires passing in filled-in SObjEntParms objects
	* bBoolArg is a multifunctional bool argument
	* 
	* For AI, bBoolArg represents whether the player is responsible for the action
	* 
	* For items and locations, bBoolArg is true when the ent is entering the location/inventory
	* and false when leaving the location or being dropped from the inventory.
	*
	* This is the most generic version, will be called by the inventory after it puts "value" into the parms
	**/
	void MissionEvent
		( 
		EComponentType CompType, 					 
		SObjEntParms *EntDat1, 				 
		SObjEntParms *EntDat2,
		bool bBoolArg = false
		);

	void MissionEvent
		( 
		EComponentType CompType, 					 
		SObjEntParms *EntDat1, 				 
		bool bBoolArg = false
		)
	{ MissionEvent( CompType, EntDat1, NULL, bBoolArg ); };

	/**
	* Overloaded MissionEvent with entities instead of parms 
	* (for real ents as opposed to fake inventory items)
	* Used by AI events, locations, etc
	**/
	void MissionEvent
		( 
		EComponentType CompType, 					 
		idEntity *Ent1, 				 
		idEntity *Ent2,
		bool bBoolArg,
		bool bWhileAirborne = false
		);

	void MissionEvent
		( 
		EComponentType CompType, 					 
		idEntity *Ent1, 				 
		bool bBoolArg,
		bool bWhileAirborne = false
		)
	{ MissionEvent( CompType, Ent1, NULL, bBoolArg, bWhileAirborne ); }

	/**
	 * "HandleMissionEvent" is designed to be the script counterpart 
	 * of MissionData::MissionEvent() which isn't usable by D3 scripts. 
	 *
	 * Example:
	 * sys.handleMissionEvent(self, EVENT_READABLE_OPENED, "");
	 *
	 * MissionData::HandleMissionEvent will just digest the arguments 
	 * and pass them on to the actual MissionEvent() method for further processing.
	 */ 
	void HandleMissionEvent(idEntity* objEnt, EMissionEventType eventType, const char* argument);

	/**
	* Fill the SObjEntParms data from an entity.  Does not fill in value and superGroupValue
	**/
	void FillParmsData( idEntity *ent, SObjEntParms *parms );
	
	/**
	* Called by the inventory when a player picks up or drops an item.
	*
	* Entity is the ent being picked up, bPicked up is true if picked up, false if dropped
	* TypeName should be set to the name of the item's type.  E.g., for loot, loot_gems, loot_gold...
	*
	* For loot items, ItemName is automatically set to "loot_gems", "loot_gold", etc.
	* For non-loot items, ItemName is the name of the inventory item (e.g., healthpotion)
	* NOTE: ItemName gets tested when using the "group" specifier.
	*
	* For non-loot items, value should be set to the number of stacked items if non-unique, or 1 if unique
	* For loot items, value should be the current _total_ value of that loot group.
	* For loot items, OverallVal should be set to the overall loot count
	* Ent is the actual entity picked up/dropped.  If NULL, default entity properties will be used.
	**/
	void InventoryCallback( idEntity *ent, idStr ItemName, int value, int OverallVal = 1, bool bPickedUp = true );

	/**
	* Called when AI are alerted
	* The alert value is the alert state, e.g. state 0 = no significant alert, 5 = combat
	**/
	void AlertCallback(idEntity *Alerted, idEntity *Alerter, int AlertVal);

	/**
	* Parse the objective data on an entity and add it to the objectives system
	* Called by CTarget_AddObjectives
	* This may be done during gameplay to add new objectives
	* Returns the index of the LAST objective added, for later addressing
	**/
	int AddObjsFromEnt( idEntity *ent );
	int AddObjsFromDict(const idDict& dict);

	/**
	 * greebo: Parse conditional objective spawnargs allowing one objective
	 * to depend on the objective state in a previous map. The conditions
	 * are automatically evaluated and applied to the existing ones, so make sure
	 * to call this after the objectives are parsed.
	 */
	void ParseObjectiveConditions(const idDict& dict);

    /**
     * Baal: This checks if the entity is referenced by any COMP_LOCATION component of any objective.
     * Called when a stackable item is dropped to set the m_bIsObjective flag that's used by 
     * "info_tdm_objective_location" entities.
    **/
    bool    MatchLocationObjectives( idEntity * );
    
	/**
	 * greebo: Load the objectives directly from the given map file.
	 * This is called by the main menu SDK code.
	 **/
	void LoadDirectlyFromMapFile(idMapFile* mapFile);

	/**
	 * greebo: Loads the named map file. After this call, it's ensured
	 * that the m_mapFile member holds the named map. No action is taken
	 * when the member already held the map with that name to avoid
	 * loading the same data twice.
	 *
	 * Note: the caller must not free the map using "delete".
	 */
	idMapFile* LoadMap(const idStr& mapFileName);

	/**
	 * greebo: This updates the given GUI with the current
	 *         missiondata (objectives state). Called by gameLocal on demand of the main menu.
	 *
	 * @ui: the GUI to be updated.
	 */
	void UpdateGUIState(idUserInterface* ui);

	/**
	 * greebo: Gets called when the main menu is active and a "cmd" string is pending.
	 *         This watches out for any objectives-related commands and interprets them.
	 */
	void HandleMainMenuCommands(const idStr& cmd, idUserInterface* gui);

	// greebo: Call this to trigger an objective update next time the main menu is shown
	void ClearGUIState();

	/**
	 * greebo: Updates the statistics in the given GUI.
	 *
	 * @gui: The GUI to be updated.
	 * @listDefName: the name of the listDef in the GUI, which should be filled with the values.
	 */
	void UpdateStatisticsGUI(idUserInterface* gui, const idStr& listDefName);

	// Events

	/**
	* The following are called internally when an objective completes or fails
	**/
	void Event_ObjectiveComplete( int ObjIndex );
	void Event_ObjectiveFailed( int ObjIndex );

	// A new objective has been added or a previously hidden one has been shown
	void Event_NewObjective();

	void Event_MissionComplete( void );
	void Event_MissionFailed( void );

	// greebo: This is the general mission end event, regardless whether failed or completed.
	void Event_MissionEnd();

	// Obsttorte: Increment save game counter for end mission screen
	int getTotalSaves();

	// Obsttorte (#5678)
	int getPocketsTotal();

	// Dragofer: set stats for secrets
	void SetSecretsFound( float secrets );
	void SetSecretsTotal( float secrets );

	/**
	* Obsttorte: (#5967) Alter notifications on objective state change
	*/
	bool m_ObjNote;

protected:
	/**
	 * greebo: Tells the missiondata class to remember playerteam. As mission statistics
	 * are populated after the map has been closed, there's no way to know the actual
	 * team of the player, hence this function is used to let the missiondata know.
	 */
	void SetPlayerTeam(int team);

	/**
	* Do the numerical comparison
	* Get the number to compare, either provided or from stats, depending on component type
	* e.g., for AI events, get the number from stats.
	* For item events, the number will be passed in by the callback from the inventory.
	**/
	bool	EvaluateObjective
		(
			CObjectiveComponent *pComp,
			SObjEntParms *EntDat1,
			SObjEntParms *EntDat2,
			bool bBoolArg
		);

	/**
	* Reads the specification type from the objective component,
	* then tests if that specification on EntDat matches that in the component.
	* The index determines which specification of the component to check,
	* since some components have two objectives. ind=0 => first, ind=1 => second
	* A component has a max of two specificaton checks, so ind should never be > 1.
	**/
	bool	MatchSpec( CObjectiveComponent *pComp, SObjEntParms *EntDat, int ind );

	/**
	* Internal function used to check success or failure logic
	* 
	* If bObjComp is true, we are evaluating at the objective level 
	* and ObjNum need not be specified
	*
	* If bObjComp is false, we are evaluating at the component level, 
	* and ObjNum should be specified
	**/
	bool EvalBoolLogic( SBoolParseNode *input, bool bObjComp, int ObjNum = 0 );

	/**
	* Internal function used to parse boolean logic into a conditional matrix
	**/
	bool ParseLogicStr( idStr *input, SBoolParseNode *output );

	/**
	* Parse the success and failure logic strings if they are non-empty
	* Returns true if parsing succeeded
	**/
	bool ParseLogicStrs( void );

protected:
	/**
	* Set to true if any of the objective states have changed and objectives need updating
	**/
	bool m_bObjsNeedUpdate;

	/**
	* List of current objectives
	**/
	idList<CObjective> m_Objectives;

	/**
	* Pointers to objective components that are centrally clocked
	* Components that fall under this domain are:
	* CUSTOM_CLOCKED, DISTANCE, and TIMER
	**/
	idList<CObjectiveComponent *> m_ClockedComponents;

	/**
	* Object holding all mission stats relating to AI, damage to player and AI
	* Loot stats are maintained by the inventory
	**/
	MissionStatistics m_Stats;

	/**
	* Hash indices to store string->enum conversions for objective component type and
	* specification method type.
	* Used for parsing objectives from spawnargs.
	**/
	idHashIndex m_CompTypeHash;
	idHashIndex m_SpecTypeHash;

	/**
	* Success and failure logic strings for overall mission.  Used to reload the parse matrix on save/restore
	**/
	idStr m_SuccessLogicStr;
	idStr m_FailureLogicStr;

	/**
	* Success and failure boolean parsing matrices for overall mission
	**/
	SBoolParseNode m_SuccessLogic;
	SBoolParseNode m_FailureLogic;

	// true if the main menu GUI is up to date
	bool m_MissionDataLoadedIntoGUI; 

	// parsed map for use by Difficulty screen
	idMapFile*	m_mapFile;

	// The team number of the player, needed for the statistics GUI
	int			m_PlayerTeam;

	// stgatilov #6489: this flag is set to true when mission ends in either way
	// it allows us to avoid double-win or double-loss of a mission
	bool		m_hasMissionEnded;
}; // CMissionData
typedef std::shared_ptr<CMissionData> CMissionDataPtr;

#endif // MISSIONDATA_H
