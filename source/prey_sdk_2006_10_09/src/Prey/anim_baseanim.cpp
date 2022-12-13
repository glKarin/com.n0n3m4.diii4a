
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"
#include "../renderer/Model_local.h"			// Hackish.  Needed to access certain classes.  Imitates game/anim/Blend.cpp
/*
===============
hhMD5Anim::hhMD5Anim
===============
*/
hhMD5Anim::hhMD5Anim(void) {

	start = 0.0f;
	end = 1.0f;
}


/*
==============================
hhMD5Anim::setLimits
  Set the limits of the animation to play.
  start and end should vary from 0 to 1
==============================
*/
void hhMD5Anim::SetLimits(float start, float end) const {

	if (start <= end) {
		this->start = start;
		this->end = end;
	}
	else {
		this->start = end;
		this->end = start;
	}

}


/*
==============================
hhMD5Anim::ConvertTimeToFrame
==============================
*/
void hhMD5Anim::ConvertTimeToFrame( int time, int cycleCount, 
									 frameBlend_t &frame ) const {
	float newTime;
	float timeOffset;


	timeOffset = animLength * start;

	if (time < 0) {
		newTime = timeOffset;
	}
	else {
		newTime = time + timeOffset;
	}
	
	idMD5Anim::ConvertTimeToFrame(newTime, cycleCount, frame);


}


/*
==============================
hhMD5Anim::Length
==============================
*/
int hhMD5Anim::Length(void) const {
	int intLength;
	

	intLength = animLength * (end - start);
	if ( ( intLength == 0 ) && ( end != start ) ) {
		intLength = 1;
	}
	
	
	return( intLength );

}

/*
================
hhMD5Anim::Save
================
*/
void hhMD5Anim::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( start );
	savefile->WriteFloat( end );
}

/*
================
hhMD5Anim::Restore
================
*/
void hhMD5Anim::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( start );
	savefile->ReadFloat( end );
}

