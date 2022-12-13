
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


/*
============
hhAnimBlend::hhAnimBlend
============
*/
hhAnimBlend::hhAnimBlend() {

	frozen 		= false;
	freezeStart = -1;
	freezeEnd	= -1;
	freezeCurrent = -1;
	rotateTime 	= -1;
	rotateEvent = NULL;

}


/*
=====================
hhAnimBlend::FrameHasChanged
=====================
*/
bool hhAnimBlend::FrameHasChanged( int currentTime ) const {


	UpdateFreezeTime( currentTime );

	if ( frozen ) {
		return false;
	}	

	return( idAnimBlend::FrameHasChanged( currentTime ) );

}


/*
=====================
hhAnimBlend::AnimTime
=====================
*/
int hhAnimBlend::AnimTime( int currentTime ) const {


	if ( animNum ) {
		UpdateFreezeTime( currentTime );
	}

	return( idAnimBlend::AnimTime( currentTime ) );

}


/*
=====================
hhAnimBlend::GetFrameNumber
=====================
*/
int hhAnimBlend::GetFrameNumber( int currentTime ) const {

	//gameLocal.Printf( "In FRAME NUM\n" );

	UpdateFreezeTime( currentTime );

	return( idAnimBlend::GetFrameNumber( currentTime ) );

}


/*
=====================
hhAnimBlend::CallFrameCommands
=====================
*/
void hhAnimBlend::CallFrameCommands( idEntity *ent, int fromtime, int totime ) const {


	if ( frozen ) { 
		return; 
	}

	idAnimBlend::CallFrameCommands( ent, fromtime, totime );

}

/*
=====================
hhAnimBlend::BlendAnim
=====================
*/
bool hhAnimBlend::BlendAnim( int currentTime, int channel, int numJoints, idJointQuat *blendFrame, float &blendWeight, bool removeOriginOffset, bool overrideBlend, bool printInfo ) const {


	UpdateFreezeTime( currentTime );

	return( idAnimBlend::BlendAnim( currentTime, channel, numJoints, blendFrame, blendWeight, removeOriginOffset, overrideBlend, printInfo ) );

}


/*
=====================
hhAnimBlend::BlendOrigin
=====================
*/
void hhAnimBlend::BlendOrigin( int currentTime, idVec3 &blendPos, float &blendWeight, bool removeOriginOffset ) const {

	//gameLocal.Printf( "BLEND ORIGIN\n" );

	UpdateFreezeTime( currentTime );

	if ( frozen ) {
		return;
	}

	idAnimBlend::BlendOrigin( currentTime, blendPos, blendWeight, removeOriginOffset );

}


/*
=====================
hhAnimBlend::BlendDelta
=====================
*/
void hhAnimBlend::BlendDelta( int fromtime, int totime, idVec3 &blendDelta, float &blendWeight ) const {
	

	if ( frozen ) {
		return;
	}	

	idAnimBlend::BlendDelta( fromtime, totime, blendDelta, blendWeight );

}


/*
=====================
hhAnimBlend::AddBounds
=====================
*/
bool hhAnimBlend::AddBounds( int currentTime, idBounds &bounds, bool removeOriginOffset ) const {


	UpdateFreezeTime( currentTime );

	if ( frozen ) {
		return false;
	}	

	return( idAnimBlend::AddBounds( currentTime, bounds, removeOriginOffset ) );

}


/*
===============
hhAnimBlend::UpdateFreezeTime
===============
*/
void hhAnimBlend::UpdateFreezeTime( int currentTime ) const { 

  if ( !frozen ) { 
  	return; 
  } 
  
  if ( currentTime > freezeCurrent ) { 
  	starttime += currentTime - freezeCurrent; 
  	endtime += currentTime - freezeCurrent; 
	freezeCurrent = currentTime;
  } 

}

extern const idEventDef EV_CheckThaw;

/*
================
hhAnimBlend::Freeze
  Freeze the animation
  Inputs: currentTime - The current time of freeze
  		  freezeEnd - Delta time to end the freeze. Set to -1 if will thaw manually
  Outputs: true if frozen successfully, false otherwise
HUMANHEAD nla
================
*/
bool hhAnimBlend::Freeze( int currentTime, idEntity *owner, int aFreezeEnd ) { 

	if ( frozen ) { 
		return( false ); 
	} 
	
	frozen = true; 
	freezeStart = currentTime; 
	freezeCurrent = currentTime; 
	freezeEnd = currentTime + aFreezeEnd;
	
	// Post the thaw event if time is known
	if ( owner ) {
		owner->PostEventMS( &EV_CheckThaw, aFreezeEnd );
	}
	else {
		gameLocal.Warning( "Freeze called on an animator with no owner!" );
	}
	
	
	return( true );
}


/*
===============
hhAnimBlend::Freeze
HUMANHEAD nla
===============
*/
bool hhAnimBlend::Freeze( int currentTime, idEntity *owner ) { 

	return( Freeze( currentTime, owner, -1 ) );
}


/*
===============
hhAnimBlend::Thaw
HUMANHEAD nla
===============
*/
bool hhAnimBlend::Thaw( int currentTime ) { 

	if ( !frozen ) { 
		return( false ); 
	} 
	
	UpdateFreezeTime( currentTime ); 	
	frozen = false; 
	
	
	return( true );
}


/*
===============
hhAnimBlend::ThawIfTime
HUMANHEAD nla
===============
*/
bool hhAnimBlend::ThawIfTime( int currentTime ) {
	
	if ( !frozen ) {
		return( false );
	}

	if ( freezeEnd < 0 ) {
		return( false );
	}

	if ( currentTime >= freezeEnd ) {
		return( Thaw( currentTime ) );
	}

	return( false );
}




