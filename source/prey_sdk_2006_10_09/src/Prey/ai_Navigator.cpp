
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"



/*
=================
hhNavigator::hhNavigator()
=================
*/
hhNavigator::hhNavigator(void) {


	hhSelf = NULL;

//	followStateFunction = FollowAllyStay;

}		//. hhNavigator::hhNavigator(void)


/*
========================
hhNavigator::Spawn(void)
========================
*/

// Required for CLASS_DECLARATION
void hhNavigator::Spawn(void) {


}		//. hhNavigator::hhNavigator(void)


/*
=======================================
hhNavigator::SetOwner(idAI *owner)
=======================================
*/
void hhNavigator::SetOwner(idAI *owner) {
	
	idNavigator::SetOwner(owner);

	if (owner->IsType(hhAI::Type)) {
		hhSelf = static_cast<hhAI *>(owner);
	}
	
}		//. hhNavigator::SetOwner(idAI *)

/* JRM- Removed because brain stuff handles this logic now
//
//===============================
//hhNavigator::SetAlly(idActor *)
//===============================
//
void hhNavigator::SetAlly(idActor *ally) {

	if (ally != NULL) {
		FollowAlly(ally);
	}

}		//. hhNavigator::SetAlly(idActor *)


//
//=============================
//hhNavigator::FollowAlly(ally)
//=============================
//
void hhNavigator::FollowAlly(idActor *ally) {

	// If we aren't on an hhAI, we can't follow, so leave
	if (hhSelf == NULL) {
		return;
	}
	


	(this->*followStateFunction)(ally);



}		//. hhNavigator::FollowAlly(idActor *ally)
*/





/* HUMANHEAD JRM - REMOVED
//
//===================================
//hhNavigator::FollowAllyStay(void)
//===================================
//
void hhNavigator::FollowAllyStay(idActor *ally) {
	static bool first = true;
	

	if (first) {
		if (ai_debug->integer > 0) {
			gameLocal.Printf("in FollowAllyStay\n");
		}
		first = false;
	}
	
	if (hhSelf->AI_ALLY_FAR ||
		!hhSelf->AI_ALLY_VISIBLE) {
		followStateFunction = FollowAllyFollow;
		first = true;
	}
	else if (hhSelf->AI_ALLY_TOUCHED) {
		followStateFunction = FollowAllyLead;
		first = true;
	}
	else {		// Just stay still
		moveType	= MOVE_NONE;
	}


}		//. hhNavigator::FollowAllyStay(void)


//
//===================================
//hhNavigator::FollowAllyFollow(void)
//===================================
//
void hhNavigator::FollowAllyFollow(idActor *ally) {
	static bool first = true;


	hhSelf->AI_FOLLOW_ALLY = true;

	if (first) {
		if (ai_debug->integer > 0) {
			gameLocal.Printf("in FollowAllyFollow\n");
		}
		first = false;
	}

	if (hhSelf->AI_ALLY_NEAR) {
		StopMove();

		followStateFunction = FollowAllyStay;
		hhSelf->AI_FOLLOW_ALLY = false;
		first = true;
	}
	// In case the min_dist is too close
	else if (hhSelf->AI_ALLY_TOUCHED) {
		followStateFunction = FollowAllyLead;
		hhSelf->AI_FOLLOW_ALLY = false;
		first = true;
	}
	else {
		moveDest	= ally->GetFloorPos();
		goal		= ally;
		moveType	= MOVE_TO_ALLY;

		if (aas) {
			toAreaNum = aas->PointReachableAreaNum( moveDest );
			toAreaEnemy = false;
		}
	}	//. Follow the player

}		//. hhNavigator::FollowAllyFollow(void)


//
//===================================
//hhNavigator::FollowAllyLead(void)
//===================================
//
void hhNavigator::FollowAllyLead(idActor *ally) {
	static bool first = true;
	static int nextUpdateTime;
	idVec3 leadPosition;

	
	hhSelf->AI_LEAD_ALLY = true;

	if (first) {
		if (ai_debug->integer > 0) {
			gameLocal.Printf("in FollowAllyLead\n");
		}
		first = false;
	}

	// Reset this variable, as it was used to get here.
	if (hhSelf->AI_ALLY_TOUCHED) {
		hhSelf->AI_ALLY_TOUCHED = false;
		nextUpdateTime = 0;
	}

	if (gameLocal.time * 1000.0f >= nextUpdateTime) {
	//if (nextUpdateTime == 0) {

		leadPosition = FindNewLeadPosition(ally);
		
		if (leadPosition != vec3_origin) {
			moveDest = leadPosition;
			moveType = MOVE_TO_ALLY;
			goal = NULL;

			if (aas) {
				toAreaNum = aas->PointReachableAreaNum(moveDest);
				toAreaEnemy = false;
			}

			nextUpdateTime = gameLocal.time * 1000.0f + 
				hhSelf->follow_lead_update_time;
		}
		
	}	//. Time to update position

	if (!hhSelf->AI_ALLY_NEAR) {
		followStateFunction = FollowAllyStay;
		hhSelf->AI_LEAD_ALLY = false;
		first = true;
	}


}		//. hhNavigator::FollowAllyLead(void)


//
//================================
//hhNavigator::FindNewLeadPosition
//================================
//
idVec3 hhNavigator::FindNewLeadPosition(idActor *ally) {
	idVec3 direction;
	aasTrace_t trace;

	
	direction = hhSelf->GetFloorPos() - ally->GetFloorPos();
	direction.Normalize();
	direction *= hhSelf->follow_min_dist;

	if (aas) {
		idVec3		bestPos;
		
		bestPos = aas->FindNearestPoint(ally->GetFloorPos(),
										hhSelf->GetFloorPos(),
										hhSelf->follow_min_dist);
		if (bestPos != vec3_origin) {
			return(bestPos);
		}

		// Didn't find a good point
		
		// Just move as far as we can in the direction nudged
		aas->Trace(trace, hhSelf->GetPhysics()->GetOrigin(), 
				   hhSelf->GetPhysics()->GetOrigin() + direction);
		
		//? How does this work on slopes?
		//! Do we want to use the floor position?
		return(ally->GetFloorPos() + direction * trace.fraction);
	}

	return(ally->GetFloorPos() + direction);



}		//. hhNavigator::FindNewLeadPosition(idActor *)
*/
