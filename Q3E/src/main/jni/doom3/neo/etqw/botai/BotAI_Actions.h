// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __BOTACTIONS_H__
#define __BOTACTIONS_H__

#include "Bot_Common.h"

enum botActionType_t {
	BASE_ACTION = 1,
	BBOX_ACTION = 2,
	VEHICLE_PATH_ACTION = 3,
	TARGET_ACTION = 4,
};

#define ACTION_BBOX_EXPAND_BIG		64.0f
#define ACTION_BBOX_EXPAND_SMALL	12.0f

#define MAX_LINKACTIONS 8
#define MAX_ACTIONS	512					//mal: max number of actions in the game

enum botActionGoals_t {
	ACTION_NULL = -1,
	ACTION_ROAM,
	ACTION_CAMP,
	ACTION_DENY_SPAWNPOINT,		// deny the enemy from having a FDA here.
	ACTION_MAJOR_OBJ_BUILD,
	ACTION_MINOR_OBJ_BUILD,
	ACTION_PREVENT_BUILD,		// the opposite of ACTION_MAJOR_OBJ_BUILD/ACTION_MINOR_OBJ_BUILD
	ACTION_HE_CHARGE,
	ACTION_DEFUSE,				// the opposite of ACTION_DEFUSE
	ACTION_HACK,
	ACTION_PREVENT_HACK,		// the opposite of ACTION_HACK
	ACTION_STEAL,
	ACTION_PREVENT_STEAL,		// the opposite of ACTION_STEAL
	ACTION_SNIPE,
	ACTION_DELIVER,
	ACTION_LANDMINE,
	ACTION_FIRESUPPORT,			// a place for FOps to unleash firesupport
	ACTION_AIRCAN_HINT,
	ACTION_SMOKE_HINT,			// have coverts throw smoke at the obj, so their teammates can do it in peace.
	ACTION_NADE_HINT,
	ACTION_VEHICLE_CAMP,				// vehicle specific camp
	ACTION_VEHICLE_ROAM,				// vehicle specific roam
	ACTION_SUPPLY_HINT,
	ACTION_MCP_OUTPOST,
	ACTION_SHIELD_HINT,			// a good place to drop a shield.
	ACTION_TELEPORTER_HINT,	
	ACTION_FORWARD_SPAWN,		// forward deployment area
	ACTION_PREVENT_DELIVER,
	ACTION_DROP_DEPLOYABLE,		// team can drop a deployable here.
	ACTION_MCP_START,			// this is where the MCP begins its path.
	ACTION_DEFENSE_CAMP,		// HIGH priority camp location.
	ACTION_DEFENSE_MINE,
	ACTION_VEHICLE_GRAB,
	ACTION_THIRDEYE_HINT,
	ACTION_MG_NEST,
	ACTION_MG_NEST_BUILD,
	ACTION_FLYER_HIVE_LAUNCH,
	ACTION_FLYER_HIVE_TARGET,
	ACTION_FLYER_HIVE_HINT,
	ACTION_DROP_PRIORITY_DEPLOYABLE,
	ACTION_MCP_ROUTE_MARKER,
	ACTION_MAX_ACTIONS
};

enum leanTypes_t {
	NULL_LEAN = -1,
	LEAN_BODY_RIGHT,
	LEAN_BODY_LEFT
};

enum botActionStates_t {
	ACTION_STATE_NULL = -1,		//mal: action is dead/deactivated/etc
	ACTION_STATE_NORMAL,		//mal: nothing to see here, move along.... the default state of an action.
	ACTION_STATE_START_BUILD,
	ACTION_STATE_PLANTED,
	ACTION_STATE_START_HACK,
	ACTION_STATE_OBJ_STOLEN,
	ACTION_STATE_DEFUSED,
	ACTION_STATE_BUILD_FIZZLED,		//mal_TODO: add something for minor builds? so that the bots know they're done?
	ACTION_STATE_HACK_FIZZLED,
	ACTION_STATE_OBJ_RETURNED,
	ACTION_STATE_OBJ_DROPPED,
	ACTION_STATE_FINISH_BUILD,
	ACTION_STATE_OBJ_BORN,			//mal: at map start, when the obj is first born
	ACTION_STATE_OBJ_DELIVERED,
	ACTION_STATE_GUN_READY,
	ACTION_STATE_GUN_DESTROYED
};

struct actionTarget_t {
	bool inuse;
	int radius;
	idVec3 origin;
};

class idBotActions {
public:

	friend class idBotAI;
	friend class idBotThreadData; //mal_FIXME: when get map loading finished, take this out!

							idBotActions();
	virtual					~idBotActions() {}

	//mal_TODO: finish these functions for the map loading stuff.....

	void					SetActive( bool isActive ) { active = isActive; }
	void					SetActionState( const botActionStates_t &state ) { actionState = state; }
	void					SetOrigin( const idVec3 &actionOrigin ) { origin = actionOrigin; }
	void					SetRadius( float actionRadius ) { radius = actionRadius; }
	void					SetActionTimeInSeconds( int time ) { actionTimeInSeconds = time; }
	void					SetAreaNum( int actionAreaNum ) { areaNum = actionAreaNum; }
	void					SetHumanObj( botActionGoals_t obj ) { humanObj = obj; }
	void					SetStroggObj( botActionGoals_t obj ) { stroggObj = obj; }
	void					SetActionGroupNum( int actionGroupNum ) { groupID = actionGroupNum; }
	void					SetActionPriority( bool isPriority ) { priority = isPriority; }
	bool					ActionIsPriority() { return priority; }
	void					SetActionPosture( botMoveFlags_t &actionPosture ) { posture = actionPosture; }
	bool					ActionIsValid() const { return ( baseActionType == BASE_ACTION && ( humanObj != ACTION_NULL || stroggObj != ACTION_NULL ) ); }
	bool					ActionIsActiveForever() const { return activeForever; }
	int						GetActionGroup() const { return groupID; }
	bool					ActionIsActive() const { return active; }
	const botActionGoals_t	GetStroggObj() const { return stroggObj; }
	const botActionGoals_t	GetHumanObj() const { return humanObj; }
	const botActionStates_t	GetActionState() const { return actionState; }
	const botActionGoals_t  GetObjForTeam( playerTeamTypes_t team ) const { return ( team == GDF ) ? humanObj : stroggObj; }
	const botActionGoals_t  GetBaseObjForTeam( playerTeamTypes_t team ) const { return ( team == GDF ) ? baseHumanObj : baseStroggObj; }
	const idBox&			GetBox() const { return actionBBox; }
	float					GetRadius() const { return radius; }
	bool					EntityIsInsideActionBBox( int entNum, const dangerTypes_t entityType, bool expandBox = true ) const;
	bool					EntityOriginIsInsideActionBBox( const idVec3& entOrg );
	void					SetActionDisabled( int actionNum );
	bool					ArmedChargesInsideActionBBox( int ignoreEnt ) const;
	bool					ArmedMinesInsideActionBBox() const;
	int						GetValidClasses() const { return validClasses; }
	bool					CheckTeamMemberIsInsideAction( int ignoreClientNum, const playerTeamTypes_t team, const playerClassTypes_t classType, bool needsBombCharge = false ) const;
	int						GetActionVehicleFlags( const playerTeamTypes_t botTeam ) const;
	void					SetActionVehicleFlags( int vehicleFlags ) { actionVehicleFlags = vehicleFlags; }
	const botChatTypes_t	GetVOChat() const { return VOChatFlag; }
	playerTeamTypes_t		GetTeamOwner() const { return teamOwner; }
	void					SetActionTeamOwner( const playerTeamTypes_t newTeamOwner ) { teamOwner = newTeamOwner; }
	void					SetActionObjState( bool isHome ) { hasObj = isHome; }
	bool					GetActionObjState() const { return hasObj; }
	int						GetActionVehicleType() const { return actionVehicleFlags; }
	const playerWeaponTypes_t GetActionWeapType() const { return weapType; }

	void					FindBBoxCenterLinePoint( idVec3 &point ) const; //mal: will find a line for the bots to look at. Not the best yet, but it does some good. Needs improvement!
	void					ProjectPointOntoLine( const idVec3 &start, const idVec3 &end, idVec3 &point ) const;
	void					FindRandomPointInBBox( idVec3 &point, int ignoreClientNum, const playerTeamTypes_t team ) const;

	const char*				GetActionName() const { return name.c_str(); }

	int						GetActionSpawnControllerEntNum() const { return spawnControllerEntityNum; }

	int						GetDeployableType() const { return deployableType; }

	const idVec3&			GetActionOrigin() const { return origin; }

	int						GetActionAreaNum() const { return areaNum; }
	int						GetActionVehicleAreaNum() const { return areaNumVehicle; }
	void					SetActionActivateTime( int newTime ) { actionActivateTime = newTime; }

	botActionType_t			GetBaseActionType() const { return baseActionType; }

private:

	bool					active;								// is this action currently active?
	bool					noHack;								
	bool					priority;
	bool					blindFire;
	bool					requiresVehicleType;				// if true - wont do goal unless can find vehicle type.
	bool					activeForever;						// if true, is turned on for the duration of the map.
	bool					disguiseSafe;
	bool					hasObj;								// for carryable objs - if true, obj is home.
	botActionType_t			baseActionType;						// baseAction, vehicleBBox, targetAction or actionBBox?
	int						actionVehicleFlags;					// does this vehicle require the use of a vehicle to do/reach/etc? and if so - what type?
	int						entityNum;							// the entity this action points to. Will be infered by the name of the entity passed.
	int						areaNum;							// the area of AAS that this action is in.
	int						areaNumVehicle;
	int						groupID;							// the group this action belongs to - useful for grouping actions together in the script
	int						validClasses;						// a bitmask of what classes can do this action. 0 = ANYONE, 1 = SOLDIER, 2 = MEDIC, 4 = ENG, 8 = FOPS, 16 = COVERTOPS
	int						actionTimeInSeconds;				// how long the bot should do whatever its supposed to do here. defined in seconds
	int						routeID;
	int						deployableType;						// what kind of deployable belongs here.
	int						spawnControllerEntityNum;			// only used for forward spawns - points to the entity that the action encompansses.
	int						actionActivateTime;
	float					radius;								// this actions range of effect
	
	botChatTypes_t			VOChatFlag;							// what chat the bot should play when selecting this action.
	leanTypes_t				leanDir;
	playerWeaponTypes_t		weapType;
	playerTeamTypes_t		teamOwner;							// which team owns this action.
	actionTarget_t			actionTargets[ MAX_LINKACTIONS ];	// locations on the map for the bots be interested in, while doing this action. EX: an aim point to look at while camped, etc.
	botActionGoals_t		humanObj;							// the human's goal at this action. ACTION_NULL if have none.
	botActionGoals_t		stroggObj;							// the strogg's goal at this action. ACTION_NULL if have none.
	botActionGoals_t		baseHumanObj;						//
	botActionGoals_t		baseStroggObj;						//
	botMoveFlags_t			posture;							// should a bot stand, crouch, or go prone when reaching this action?
	botActionStates_t		actionState;						// a generic action state tracker
	idVec3					origin;								// actions origin
	idStr					name;								// the name of this action.
	idStr					targetAction;						// the action this one points to.
	idBox					actionBBox;							// bounding box that marks where the bot should perform this action. Useful for knowing when bot is "touching" intended action.
	//mal_TODO: add more stuff!!

};

#endif /* !__BOTACTIONS_H__ */
