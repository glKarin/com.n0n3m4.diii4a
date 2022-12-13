
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


/*
============
hhAnimator::hhAnimator
============
*/

hhAnimator::hhAnimator() {

	lastCycleRotate = 0;

}		//. hhAnimator::hhAnimator()


/*
=====================
hhAnimator::CycleAnimRandom
=====================
*/
void hhAnimator::CycleAnim( int channelNum, int anim, int currentTime, int blendTime, const idEventDef *pEvent ) {

	//! NLANOTE - Add ability to skip cycling
	idAnimator::CycleAnim( channelNum, anim, currentTime, blendTime );

	if ( !anim ) { return; }
	

	hhAnim *animPtr = (hhAnim *)GetAnim( anim );
	//HUMANHEAD rww - crash here, can't repro. putting extra logic in for non-gold builds to check.
#if !GOLD
	if (!animPtr) {
		assert(0);
		common->Warning("hhAnimator::CycleAnim anim index %i returned NULL for GetAnim.", anim);
		return;
	}
#endif
//HUMANHEAD END

	// If the anim was an exact match to the requested name, if so, just return
	if ( animPtr->exactMatch ) {
		return;
	}
	
	// Determine if there are multiple anims to play.  If not, then just return.
	int numAnimVariants = GetNumAnimVariants( anim );

	if ( numAnimVariants < 2 ) {
		return;
	}

	// The CycleAnim could have started at a random spot in the anim, take this into account, and add only the time left in the anim
	int lengthLeft;
	lengthLeft = animPtr->Length() - channels[ channelNum ][0].AnimTime( currentTime );
		
	channels[ channelNum ][0].rotateTime = currentTime + lengthLeft;
	channels[ channelNum ][0].rotateEvent = pEvent;
		
	entity->PostEventMS( &EV_CheckCycleRotate, lengthLeft );
	
}		//. BlendCycleRandom( int, idAnim *, float, int, int )


/*
==============
hhAnimator::GetNumAnimVariants
==============
*/
int hhAnimator::GetNumAnimVariants( int anim ) {
	const char *name = GetAnim( anim )->Name();
	int len, numAnims, maxAnims;


	// nla - Copied from idDeclModelDef::GetAnim( const char * )
	len = strlen( name );
	if ( len && idStr::CharIsNumeric( name[ len - 1 ] ) ) {
		// find a specific animation
		return( 1 );
	}

	// find all animations with same name
	numAnims = 0;
	maxAnims = NumAnims();
	
	for( int i = 1; i < maxAnims; i++ ) {
		if ( !strcmp( GetAnim( i )->Name(), name ) ) {
			numAnims++;
		}
	}
	
	return( numAnims );
}		//. hhAnimator::GetNumAnimVariants( int )



/*
=====================
hhAnimator::CheckCycleRotate
  Check the blend animations to see if we need to switch any animations
=====================
*/
void hhAnimator::CheckCycleRotate() {
	hhAnimBlend		*blend;
	int				anim;
	int 			i;	
	const idEventDef	*event;
	

	if ( gameLocal.time <= lastCycleRotate ) {
		return;
	}
	lastCycleRotate = gameLocal.time;

	// Cycle through the channels
	for( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		// Only cycle out the first/main anim of the channel.  The rest ar being phased out and don't matter
		blend = channels[ i ];
		if ( !blend->IsDone( gameLocal.time ) ) {
			if ( blend->Anim() && ( blend->rotateTime > 0 ) && 
				 ( blend->rotateTime <= gameLocal.time ) ) { // HUMANHEAD JRM
			
				anim = GetAnim( blend->Anim()->Name() );
				event = blend->rotateEvent;

				// gameLocal.Printf( "%s Cycling Anim %s\n", entity->name.c_str(), GetAnim( anim )->FullName() );

				// 	blendWeight.Init( currentTime, blendTime, weight, newweight );
				int remainingBlend = ( blend->blendStartTime + blend->blendDuration ) -
					gameLocal.time;
				if ( remainingBlend < 0 ) { remainingBlend = 0; }
				blend->CycleAnim( modelDef, anim, gameLocal.time, remainingBlend );
				entity->BecomeActive( TH_ANIMATE );
				
				blend->rotateTime = gameLocal.time + GetAnim( anim )->Length();
				blend->rotateEvent = event;

				entity->PostEventMS( &EV_CheckCycleRotate, GetAnim( anim )->Length() );

				// Process any change events
				if ( event ) {
					entity->ProcessEvent( event );
				}
			}
		}
	}
}		//. CheckCycleRotate


/*
====================
hhAnimator::CheckThaw
HUMANHEAD nla
====================
*/
void hhAnimator::CheckThaw( ) {
	hhAnimBlend		*blend;
	int 			i, j;	


	// Cycle through the channels
	blend = channels[0];
	for( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
			if ( blend->Anim() ) {
				blend->ThawIfTime( gameLocal.time );
			}
		}
	}

}		//. CheckThaw( )

/*
=====================
hhAnimator::CheckTween
  Check if the current animation should be tweened to from the previous anim
  Inputs: channel - The channel to check the current anim with
  Outputs: The time in ms for the tween.  0 if non given.
HUMANHEAD nla
=====================
*/
int hhAnimator::CheckTween( int channelNum ) {
	idStr 	toAnimName;
	idStr 	fromAnimName 	= "";
	int 	timeMS 			= 0;
	int		tempInt;


	// Ensure we have a valid channel number
	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "idAnimator::CheckTween : channel out of range" );
	}


	// If no valid anim playing, then nothing to tween. :)
	if ( channels[ channelNum ][ 0 ].Anim() == NULL ) {
		return( 0 );
	}
	else {		// Set the to anim name
		toAnimName = channels[ channelNum ][ 0 ].AnimName();
	}

	// If a valid anim was playing, get its name
	if ( channels[ channelNum ][ 1 ].Anim() != NULL ) {
		fromAnimName = channels[ channelNum ][ 1 ].AnimName();
	}

	// We aren't really tweening, head out. = )
	if ( toAnimName == fromAnimName ) {
		// gameLocal.Printf("Earlying out!");
		return( 0 );
	}

	// See if a default anim tween time is given
	if ( entity->spawnArgs.GetInt( va( "tween2%s", 
									   	(const char *) toAnimName ),
								   "0", tempInt ) ) {
		timeMS = tempInt;
		// JRM - removed
		//gameLocal.Printf( "Got tween2%s of %d\n", (const char *) toAnimName,
		//									timeMS );						   
	}
	
	// See if a anim to anim tween time is given
	if ( fromAnimName.Length() && 
		 entity->spawnArgs.GetInt( va( "tween%s2%s", 
										(const char *) fromAnimName,
	 	 								(const char *) toAnimName ),
								   "0", tempInt ) ) {
		//! Check that they aren't the same anims

		timeMS = tempInt;
		// JRM - removed
		//gameLocal.Printf( "Got tween%s2%s of %d\n", (const char *) fromAnimName,
		//									  (const char *) toAnimName,
		//									  timeMS );   
	}
	
	// If a valid tween time is valid, use it
	if ( timeMS > 0 ) {
	  	// Freeze the current anim
		channels[ channelNum ][ 0 ].Freeze( gameLocal.time, GetEntity(), timeMS );
	
	  	// Setup the new blend times
		// NLAMERGE 5 - This really used anymore?
		/*
	  	channels[ channelNum ][ 0 ].blendWeight.SetDuration( timeMS );
	  	channels[ channelNum ][ 1 ].blendWeight.SetDuration( timeMS );
		*/
	}
	
	return( timeMS );
}		//. CheckTween( int )


/*
=====================
hhAnimator::IsAnimPlaying
HUMANHEAD nla
=====================
*/
bool hhAnimator::IsAnimPlaying( int channelNum, const idAnim *anim ) {
	idAnimBlend *	playingAnim;


	playingAnim = FindAnim( channelNum, anim );
	if ( playingAnim && ! playingAnim->IsDone( gameLocal.time ) ) {
		return( true );
	}
	
	return( false );
}		//. IsAnimPlaying( idAnim * )

/*
=====================
ggAnimator::IsAnimPlaying
HUMANHEAD nla
=====================
*/
bool hhAnimator::IsAnimPlaying( int channelNum, const char* anim ) {
	return IsAnimPlaying( channelNum, GetAnim( GetAnim( anim ) ) );
}


/*
=====================
hhAnimator::IsAnimPlaying
HUMANHEAD nla
=====================
*/
bool hhAnimator::IsAnimPlaying( const idAnim *anim ) {

	for( int i = 0; i < ANIM_NumAnimChannels; i++ ) {
		if ( IsAnimPlaying( i, anim ) ) {
			return( true );
		}
	}

	return( false );
}


/*
=====================
hhAnimator::IsAnimPlaying
HUMANHEAD nla
=====================
*/
bool hhAnimator::IsAnimPlaying( const char* anim ) {
	return IsAnimPlaying( GetAnim( GetAnim( anim ) ) );
}


/*
=====================
hhAnimator::FindAnim
HUMANHEAD nla
=====================
*/
idAnimBlend *hhAnimator::FindAnim( int channelNum, const idAnim *anim ) {
	int i;

	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "hhAnimator::FindAnim : channel out of range" );
	}

	for( i = 0; i < ANIM_MaxAnimsPerChannel; i++ ) {
		if ( channels[ channelNum ][ i ].Anim() == anim ) {
			return &( channels[ channelNum ][ i ] );
		}
	}

	return NULL;	
}


/*
=====================
hhAnimator::GetBlendAnim
HUMANHEAD nla
=====================
*/
const idAnimBlend * hhAnimator::GetBlendAnim( int channelNum, int index ) const { 

	if ( channelNum < 0 || channelNum >= ANIM_NumAnimChannels ) { 
		gameLocal.Error("Channel out of range"); 
	} 

	return ( ( index < 0 ) || ( index >= ANIM_MaxAnimsPerChannel ) ) ? NULL : &(channels[channelNum][ index ]); 
}


/*
=====================
hhAnimator::NumBlendAnims
// HUMANHEAD Copied logic from IsAnimating
HUMANHEAD nla
=====================
*/
int	hhAnimator::NumBlendAnims( int currentTime ) { 
	int					i, j;
	const idAnimBlend	*blend;
	// HUMANHEAD nla
	int				num;

	num = 0;
	// HUMANHEAD END

	if ( !modelDef->ModelHandle() ) {
		return false;
	}

	// if animating with an articulated figure
	if ( AFPoseJoints.Num() && currentTime <= AFPoseTime ) {
		return true;
	}

	blend = channels[ 0 ];
	for( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
			if ( !blend->IsDone( currentTime ) ) {
				// HUMANHEAD nla
				num++;
				// HUMANHEAD END
			}
		}
	}

	return num;
}


/*
====================
hhAnimator::CopyAnimations
HUMANHEAD nla
====================
*/
void hhAnimator::CopyAnimations( hhAnimator &source ) {


	// Loop through each channel
	for ( int chan = 0; chan < ANIM_NumAnimChannels; ++chan ) {				

		// NLA - Simplifed to the double loop in the Nov 2003 "Big Merge" (c)
		// Copy the channel anim info
		for ( int anim = 0; anim < ANIM_MaxAnimsPerChannel;
			  ++anim ) {

			channels[ chan ][ anim ] = source.channels[ chan ][ anim ];

		}
	}
	

}


/*
====================
hhAnimator::CopyPoses
HUMANHEAD nla
====================
*/
void hhAnimator::CopyPoses( hhAnimator &source ) {
	int i, num;

	// Copy over the joint Mod info
	jointMods.DeleteContents( true );
	num = source.jointMods.Num();
	jointMods.SetNum( num );
	for( i = 0; i < num; i++ ) {
		jointMods[ i ] = new jointMod_t;
		*jointMods[ i ] = *source.jointMods[ i ];
	}
	
	// Copy over the frameBounds
	frameBounds = source.frameBounds;

	// Copy over AF info
	AFPoseBlendWeight = source.AFPoseBlendWeight;

	num = source.AFPoseJoints.Num();
	AFPoseJoints.SetNum( num );
	for ( int i = 0; i < num; i++ ) {
		AFPoseJoints[ i ] = source.AFPoseJoints[ i ];
	}

	num = source.AFPoseJointMods.Num();
	AFPoseJointMods.SetNum( num );
	for ( int i = 0; i < num; i++ ) {
		AFPoseJointMods[ i ] = source.AFPoseJointMods[ i ];
	}

	num = source.AFPoseJointFrame.Num();
	AFPoseJointFrame.SetNum( num );
	for ( int i = 0; i < num; i++ ) {
		AFPoseJointFrame[ i ] = source.AFPoseJointFrame[ i ];
	}

	AFPoseBounds = source.AFPoseBounds;
	AFPoseTime = source.AFPoseTime;
}


/*
====================
hhAnimator::Freeze
HUMANHEAD nla
====================
*/
bool hhAnimator::Freeze( ) {
	hhAnimBlend *blend;


	blend = &( channels[ 0 ][ 0 ] );
	
	return( blend->Freeze( gameLocal.time, GetEntity() ) );

}


/*
====================
hhAnimator::Thaw
HUMANHEAD nla
====================
*/
bool hhAnimator::Thaw( ) {
	hhAnimBlend *blend;


	blend = &( channels[ 0 ][ 0 ] );
	
	return( blend->Thaw( gameLocal.time ) );

}


/*
=====================
hhAnimator::IsAnimating
=====================
*/
bool hhAnimator::IsAnimating( int currentTime ) const {
	int					i, j;
	const hhAnimBlend	*blend;
	// HUMANHEAD nla
	int				numFrozen = 0;
	// HUMANHEAD END


	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	// if animating with an articulated figure
	if ( AFPoseJoints.Num() && currentTime <= AFPoseTime ) {
		return true;
	}

	blend = channels[ 0 ];
	for( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
			if ( !blend->IsDone( currentTime ) ) {
				return true;
			}
			// HUMANHEAD nla
			if ( blend->IsFrozen() ) { numFrozen++; }
			// HUMANHEAD END
		}
	}

	// HUMANHEAD nla
	if ( numFrozen > 1 ) { return( true ); }
	// HUMANHEAD END

	return false;
}

/*
=====================
hhAnimator::FrameHasChanged
=====================
*/
bool hhAnimator::FrameHasChanged( int currentTime ) const {
	int					i, j;
	const hhAnimBlend	*blend;
	// HUMANHEAD nla - Used to allow for tween (2 frozen) anims to work
	int				numFrozen = 0;
	// HUMANHEAD END


	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	// if animating with an articulated figure
	if ( AFPoseJoints.Num() && currentTime <= AFPoseTime ) {
		return true;
	}

	blend = channels[ 0 ];
	for( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
			if ( blend->FrameHasChanged( currentTime ) ) {
				return true;
			}
			// HUMANHEAD nla
			if ( blend->IsFrozen() ) { numFrozen++; }
			// HUMANHEAD END
		}
	}

	// HUMANHEAD nla
	if ( numFrozen > 1 ) { return( true ); }
	// HUMANHEAD END

	if ( forceUpdate && IsAnimating( currentTime ) ) {
		return true;
	}

	return false;
}

//================
//hhAnimator::Save
//================
void hhAnimator::Save( idSaveGame *savefile ) const {
	// Not a subclass of idClass, so this is necessary -mdl
	idAnimator::Save( savefile );
	savefile->WriteInt( lastCycleRotate );
}

//================
//hhAnimator::Restore
//================
void hhAnimator::Restore( idRestoreGame *savefile ) {
	// Not a subclass of idClass, so this is necessary -mdl
	idAnimator::Restore( savefile );
	savefile->ReadInt( lastCycleRotate );
}

