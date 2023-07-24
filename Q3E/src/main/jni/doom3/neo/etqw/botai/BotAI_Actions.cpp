// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h" 
#include "BotThreadData.h"
#include "BotAI_Main.h"

idBotActions::idBotActions() {
    actionBBox.Clear();
	posture = WALK;
	actionState = ACTION_STATE_NORMAL;
	radius = 70.0f;
	groupID = 0;
	routeID = -1; //dont use a route by default.
	activeForever = false;
	baseHumanObj = humanObj = ACTION_NULL;
	baseStroggObj = stroggObj = ACTION_NULL;
	active = false;
	actionTimeInSeconds = 0;
	areaNum = 0;
	areaNumVehicle = 0;
	disguiseSafe = true;
	noHack = false;
	name = "";
	targetAction = "";
	requiresVehicleType = false;
	origin.Zero();
	blindFire = false;
	priority = true;
	teamOwner = NOTEAM;
	weapType = NULL_WEAP;
	baseActionType = BASE_ACTION;
	actionVehicleFlags = -1; //mal: by default, no vehicle used.
	spawnControllerEntityNum = -1;
	actionActivateTime = 0;
	memset( actionTargets, 0, sizeof( actionTargets ) );

//mal_TODO: add more variables here to clear as create them.
}

/*
================
idBotActions::EntityOriginIsInsideActionBBox
================
*/
bool idBotActions::EntityOriginIsInsideActionBBox( const idVec3& entOrg ) {
	idBox actionBounds = actionBBox;
	actionBounds.ExpandSelf( ACTION_BBOX_EXPAND_BIG ); //mal: be a bit forgiving when doing this bounds check.

	if ( actionBounds.ContainsPoint( entOrg ) ) {
		return true;
	}

	return false;
}

/*
================
idBotActions::EntityIsInsideActionBBox

Is the entity inside the bbox of the action?
================
*/
bool idBotActions::EntityIsInsideActionBBox( int entNum, const dangerTypes_t entityType, bool expandBox ) const {
	int i, j;
	idBox entBounds;
	idBox actionBounds = actionBBox;

	if ( expandBox ) {
		actionBounds.ExpandSelf( ACTION_BBOX_EXPAND_BIG ); //mal: be a bit forgiving when doing this bounds check.
	}

    if ( entityType == PLANTED_CHARGE ) {
        for( i = 0; i < MAX_CLIENT_CHARGES; i++ ) {
			const plantedChargeInfo_t& charge = botThreadData.GetBotWorldState()->chargeInfo[ i ];

			if ( charge.entNum != entNum ) {
				continue;
			}

			if ( actionBounds.ContainsPoint( charge.origin ) ) {
				return true;
			}
		}
	} else if ( entityType == PLANTED_LANDMINE ) {
		for( i = 0; i < MAX_CLIENTS; i++ ) {
			for( j = 0; j < MAX_MINES; j++ ) {
			
				if ( botThreadData.GetGameWorldState()->clientInfo[ i ].weapInfo.landMines[ j ].entNum != entNum ) {
					continue;
				}

				if ( actionBounds.ContainsPoint( botThreadData.GetBotWorldState()->clientInfo[ i ].weapInfo.landMines[ j ].origin ) ) {
					return true;
				}
			}
		}
	} else if ( entityType == INGAME_PLAYER ) {
		entBounds = idBox( botThreadData.GetBotWorldState()->clientInfo[ entNum ].localBounds, botThreadData.GetBotWorldState()->clientInfo[ entNum ].origin, botThreadData.GetBotWorldState()->clientInfo[ entNum ].bodyAxis );
		entBounds.ExpandSelf( 16.0f ); //mal: make it a bit flexible.
		if ( actionBounds.IntersectsBox( entBounds ) ) {
			return true;
		}
	} //mal_TODO: keep adding more entity types here!

	return false;
}

/*
================
idBotActions::ArmedChargesInsideActionBBox

Is there any armed charges currently inside this action's bbox?
================
*/
bool idBotActions::ArmedChargesInsideActionBBox( int ignoreEnt ) const {

	idBox actionBox = actionBBox;
	actionBox.ExpandSelf( ACTION_BBOX_EXPAND_BIG );

	for( int i = 0; i < MAX_CLIENT_CHARGES; i++ ) {
		const plantedChargeInfo_t& bombInfo = botThreadData.GetBotWorldState()->chargeInfo[ i ];

		if ( bombInfo.entNum == 0 ) {
			continue;
		}

		if ( bombInfo.entNum == ignoreEnt ) {
			continue;
		}

		if ( bombInfo.state != BOMB_ARMED ) {
			continue;
		}

		idBox bombBox = idBox( bombInfo.bbox );

		if ( actionBox.IntersectsBox( bombBox ) ) {
			return true;
		}
	}

	return false;
}

/*
================
idBotActions::ArmedMinesInsideActionBBox

Is there any armed mines currently inside this action's bbox?
================
*/
bool idBotActions::ArmedMinesInsideActionBBox() const {
	for( int j = 0; j < MAX_CLIENTS; j++ ) {
        for( int i = 0; i < MAX_MINES; i++ ) {
			if ( botThreadData.GetBotWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].entNum == 0 ) {
				continue;
			}

			plantedMineInfo_t mine = botThreadData.GetBotWorldState()->clientInfo[ j ].weapInfo.landMines[ i ];

			if ( mine.state != BOMB_ARMED ) {
				continue;
			}

			idBox mineBox = idBox( mine.bbox );
			idBox actionBox = actionBBox;

			actionBox.ExpandSelf( ACTION_BBOX_EXPAND_BIG );

			if ( botThreadData.AllowDebugData() ) {
				gameRenderWorld->DebugBox( colorGreen, mineBox, 1024 );
				gameRenderWorld->DebugBox( colorRed, actionBox, 1024 );
			}

			if ( actionBox.IntersectsBox( mineBox ) ) {
				return true;
			}
		}
	}

	return false;
}

/*
================
idBotActions::FindBBoxCenterLinePoint

Find a point to look in the actions bbox, staying in the bounds of the box.
================
*/
void idBotActions::FindBBoxCenterLinePoint( idVec3& point ) const {
	int index = Max3Index( actionBBox.GetExtents()[ 0 ], actionBBox.GetExtents()[ 1 ], actionBBox.GetExtents()[ 2 ] );
	float extents = actionBBox.GetExtents()[ index ];

	if ( extents > 64.0f ) {
		extents -= 16.0f;
	} else {
		extents *= 0.80f;
	}

	idVec3 dir = actionBBox.GetAxis()[ index ] * extents;
	idVec3 start = actionBBox.GetCenter() + dir;
	idVec3 end = actionBBox.GetCenter() - dir;

	ProjectPointOntoLine( start, end, point );

/*	
	idBounds bounds = actionBBox;
    idVec3 v = bounds[1] - bounds[0];
	int i = MaxIndex( v[0], v[1] );
	idVec3 start = bounds.GetCenter();
	idVec3 end = bounds.GetCenter();
	start[i] = bounds[0][i] + 16.0f; //mal: move in from the start/end points just a bit, incase the bbox is right at the edge of a wall.
	end[i] = bounds[1][i] - 16.0f;
*/

//mal_DEBUG
	if ( botThreadData.AllowDebugData() ) {
		gameRenderWorld->DebugLine( colorGreen, start, end, 1024 );
	}
}

/*
================
idBotActions::ProjectPointOntoLine
================
*/
void idBotActions::ProjectPointOntoLine( const idVec3 &start, const idVec3 &end, idVec3 &point ) const {
	idVec3 v1 = end - start;
	float l = v1.Normalize();
	idVec3 v2 = point - start;
	float d = ( v1 * v2 );
	if ( d < 0.0f ) {
		point = start;
	} else if ( d > l ) {
		point = end;
	}

	point = start + d * v1;
}

/*
================
idBotActions::FindRandomPointInBBox

Find a random point in a action's bbox, along the center axis, for the bot to move toward.
================
*/
void idBotActions::FindRandomPointInBBox( idVec3 &point, int ignoreClientNum, const playerTeamTypes_t team ) const {
	int index = Max3Index( actionBBox.GetExtents()[ 0 ], actionBBox.GetExtents()[ 1 ], actionBBox.GetExtents()[ 2 ] );
	float extents = actionBBox.GetExtents()[ index ];

	if ( extents > 64.0f ) {
		extents -= 16.0f;
	} else {
		extents *= 0.80f;
	}

	idVec3 dir = actionBBox.GetAxis()[ index ] * extents;
	idVec3 start = actionBBox.GetCenter() + dir;
	idVec3 end = actionBBox.GetCenter() - dir;

	point = actionBBox.GetCenter();

	point = start + botThreadData.random.RandomFloat() * ( end - start );

	int j = 0;
	int i;

	while( j < 10 ) {
		for( i = 0; i < MAX_CLIENTS; i++ ) {

			if ( i == ignoreClientNum ) { //mal: dont scan the calling client.
				continue;
			}

			const clientInfo_t& player = botThreadData.GetBotWorldState()->clientInfo[ i ];

			if ( player.inGame == false || player.team == NOTEAM || player.team != team || player.health <= 0 ) {
				continue;
			}

			idVec3 vec = player.origin - point;
			vec.z = 0.0f;

			float z = vec.z;

			if ( idMath::Fabs( z ) > 128.0f ) {
				continue;
			}

			if ( vec.LengthSqr() > Square( SAFE_PLAYER_BODY_WIDTH ) ) {
				continue;
			}

			j++;

			point = actionBBox.GetCenter();
			point = start + botThreadData.random.RandomFloat() * ( end - start );
			break;
		}

		if ( i >= 32 ) {
			break;
		}
	}

	if ( botThreadData.AllowDebugData() ) {
		end = point;
		end[ 2 ] += 64;
		gameRenderWorld->DebugLine( colorRed, point, end, 1024 );
	}
}

/*
================
idBotActions::CheckTeamMemberIsInsideAction
================
*/
bool idBotActions::CheckTeamMemberIsInsideAction( int ignoreClientNum, const playerTeamTypes_t team, const playerClassTypes_t classType, bool needsBombCharge ) const {
	bool hasMate = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == ignoreClientNum ) {
			continue;
		}

		const clientInfo_t& player = botThreadData.GetBotWorldState()->clientInfo[ i ];

		if ( player.inGame == false || player.team == NOTEAM || player.team != team || player.health <= 0 || player.classType != classType ) {
			continue;
		}

		idBox playerBox = idBox( player.localBounds, player.origin, player.bodyAxis );
		playerBox.ExpandSelf( 16.0f );

		if ( !actionBBox.IntersectsBox( playerBox ) ) {
			continue;
		}

		if ( needsBombCharge ) {
			if ( player.bombChargeUsed > 0 ) {
				continue;
			}
		}

		hasMate = true;
		break;
	}

	return hasMate;
}

/*
================
idBotActions::GetActionVehicleFlags

only allow the Icarus to be used if the action is inside a valid vehicle AAS area.
================
*/
int idBotActions::GetActionVehicleFlags( const playerTeamTypes_t team ) const {
	if ( actionVehicleFlags == 0 && areaNumVehicle == 0 && team == STROGG ) {
		return ALL_VEHICLES_BUT_ICARUS;
	}

	if ( actionVehicleFlags == 1 && areaNumVehicle == 0 && team == STROGG ) {
		return NO_VEHICLE;
	}

	if ( actionVehicleFlags != -1 && areaNumVehicle == 0 && ( actionVehicleFlags & PERSONAL ) && team == STROGG ) {
		int newFlags = actionVehicleFlags;
		newFlags &= ~PERSONAL;
		return newFlags;
	}

	return actionVehicleFlags;
}

