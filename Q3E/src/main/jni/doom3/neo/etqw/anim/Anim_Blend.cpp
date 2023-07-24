// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Anim.h"
#include "Anim_FrameCommands.h"
#include "../Entity.h"
//#include "../ai/AI.h"
#include "../Player.h"
#include "../../framework/DeclParseHelper.h"

static const char* channelNames[ ANIM_NumAnimChannels ] = {
	"all", "torso", "legs", "head"
};

/***********************************************************************

	idAnim

***********************************************************************/

/*
=====================
idAnim::idAnim
=====================
*/
idAnim::idAnim( void ) {
	baseModelDef = NULL;

	refCount = 1;

	numAnims = 0;
	memset( anims, 0, sizeof( anims ) );
	memset( &flags, 0, sizeof( flags ) );
}

/*
=====================
idAnim::~idAnim
=====================
*/
idAnim::~idAnim() {
	int i;

	for( i = 0; i < numAnims; i++ ) {
		anims[ i ]->DecreaseRefs();
	}

	ClearFrameCommands();
}

/*
=====================
idAnim::SetAnim
=====================
*/
void idAnim::SetAnim( const idDeclModelDef *modelDef, const char *sourcename, const char *animname, int num, const idMD5Anim *md5anims[ ANIM_MaxSyncedAnims ] ) {
	baseModelDef = modelDef;

	for( int i = 0; i < numAnims; i++ ) {
		anims[ i ]->DecreaseRefs();
		anims[ i ] = NULL;
	}

	assert( ( num > 0 ) && ( num <= ANIM_MaxSyncedAnims ) );

	numAnims	= num;
	realname	= sourcename;
	name		= animname;

	for( int i = 0; i < num; i++ ) {
		anims[ i ] = md5anims[ i ];
		anims[ i ]->IncreaseRefs();
	}

	memset( &flags, 0, sizeof( flags ) );

	ClearFrameCommands();
}

/*
=====================
idAnim::ClearFrameCommands
=====================
*/
void idAnim::ClearFrameCommands( void ) {
	frameLookup.Clear();
	for ( int i = 0; i < frameCommands.Num(); i++ ) {
		frameCommands[ i ]->DecRef();
	}
	frameCommands.Clear();

	for ( int i = 0; i < startCommands.Num(); i++ ) {
		startCommands[ i ]->DecRef();
	}
	startCommands.Clear();

	for ( int i = 0; i < finishCommands.Num(); i++ ) {
		finishCommands[ i ]->DecRef();
	}
	finishCommands.Clear();

	for ( int i = 0; i < beginCommands.Num(); i++ ) {
		beginCommands[ i ]->DecRef();
	}
	beginCommands.Clear();

	for ( int i = 0; i < endCommands.Num(); i++ ) {
		endCommands[ i ]->DecRef();
	}
	endCommands.Clear();
}


/*
=====================
idAnim::Name
=====================
*/
const char* idAnim::Name( void ) const {
	return name;
}

/*
=====================
idAnim::FullName
=====================
*/
const char *idAnim::FullName( void ) const {
	return realname;
}

/*
=====================
idAnim::MD5Anim

index 0 will never be NULL.  Any anim >= NumAnims will return NULL.
=====================
*/
const idMD5Anim* idAnim::MD5Anim( int num ) const {
	if ( anims[ 0 ] == NULL ) { 
		return NULL;
	}
	return anims[ num ];
}

/*
=====================
idAnim::Length
=====================
*/
int idAnim::Length( void ) const {
	if ( !anims[ 0 ] ) {
		return 0;
	}

	return anims[ 0 ]->Length();
}

/*
=====================
idAnim::NumFrames
=====================
*/
int	idAnim::NumFrames( void ) const { 
	if ( !anims[ 0 ] ) {
		return 0;
	}
	
	return anims[ 0 ]->NumFrames();
}

/*
=====================
idAnim::NumAnims
=====================
*/
int	idAnim::NumAnims( void ) const { 
	return numAnims;
}

/*
=====================
idAnim::TotalMovementDelta
=====================
*/
const idVec3 &idAnim::TotalMovementDelta( void ) const {
	if ( !anims[ 0 ] ) {
		return vec3_zero;
	}
	
	return anims[ 0 ]->TotalMovementDelta();
}

/*
=====================
idAnim::GetOrigin
=====================
*/
bool idAnim::GetOrigin( idVec3 &offset, int animNum, int currentTime, int cyclecount ) const {
	if ( !anims[ animNum ] ) {
		offset.Zero();
		return false;
	}

	anims[ animNum ]->GetOrigin( offset, currentTime, cyclecount );
	return true;
}

/*
=====================
idAnim::GetOriginRotation
=====================
*/
bool idAnim::GetOriginRotation( idQuat &rotation, int animNum, int currentTime, int cyclecount ) const {
	if ( !anims[ animNum ] ) {
		rotation.Set( 0.0f, 0.0f, 0.0f, 1.0f );
		return false;
	}

	anims[ animNum ]->GetOriginRotation( rotation, currentTime, cyclecount );
	return true;
}

/*
=====================
idAnim::GetBounds
=====================
*/
ID_INLINE bool idAnim::GetBounds( idBounds &bounds, int animNum, int currentTime, int cyclecount ) const {
	if ( !anims[ animNum ] ) {
		return false;
	}

	anims[ animNum ]->GetBounds( bounds, currentTime, cyclecount );
	return true;
}


/*
=====================
idAnim::AddFrameCommand

Returns true if no error.
=====================
*/
sdAnimFrameCommand* idAnim::ReadFrameCommand( idParser& src ) {
	idToken token;
	if( !src.ReadTokenOnLine( &token ) ) {
		src.Error( "idAnim::AddFrameCommand Unexpected end of line" );
		return false;
	}

	sdAnimFrameCommand* fc = sdAnimFrameCommand::Alloc( token );	
	if ( !fc ) {
		src.Error( "idAnim::AddFrameCommand Unknown command '%s'", token.c_str() );
		return NULL;
	}

	if ( !fc->Init( src ) ) {
		delete fc;
		return NULL;
	}

	return fc;
}

/*
=====================
idAnim::AddFrameCommand

Returns true if no error.
=====================
*/
bool idAnim::AddFrameCommand( int framenum, sdAnimFrameCommand* fc ) {
	// make sure we're within bounds
	if ( framenum > 1 && anims[ 0 ]->IsReduced() ) {
		framenum /= 2;
	}
	if ( ( framenum < 1 ) || ( framenum > anims[ 0 ]->NumFrames() ) ) {
		gameLocal.Error( "idAnim::AddFrameCommand Frame %d out of range for anim '%s' '%s' %d", framenum, realname.c_str(), anims[0]->Name(), anims[0]->NumFrames() );
		return false;
	}

	fc->IncRef();

	// frame numbers are 1 based in .def files, but 0 based internally
	framenum--;

	// check if we've initialized the frame lookup table
	if ( !frameLookup.Num() ) {
		// we haven't, so allocate the table and initialize it
		frameLookup.SetGranularity( 1 );
		frameLookup.SetNum( anims[ 0 ]->NumFrames() );
		for( int i = 0; i < frameLookup.Num(); i++ ) {
			frameLookup[ i ].num = 0;
			frameLookup[ i ].firstCommand = 0;
		}
	}

	// allocate space for a new command
	frameCommands.Alloc();

	// calculate the index of the new command
	int index = frameLookup[ framenum ].firstCommand + frameLookup[ framenum ].num;

	// move all commands from our index onward up one to give us space for our new command
	for( int i = frameCommands.Num() - 1; i > index; i-- ) {
		frameCommands[ i ] = frameCommands[ i - 1 ];
	}

	// fix the indices of any later frames to account for the inserted command
	for( int i = framenum + 1; i < frameLookup.Num(); i++ ) {
		frameLookup[ i ].firstCommand++;
	}

	// store the new command 
	frameCommands[ index ] = fc;

	// increase the number of commands on this frame
	frameLookup[ framenum ].num++;

	// return with no error
	return true;
}

/*
=====================
idAnim::AddFrameCommand

Returns true if no error.
=====================
*/
bool idAnim::AddAbsoluteFrameCommand( absoluteFrameCommandType_t framenum, sdAnimFrameCommand* fc ) {
	idToken				token;

	idList< sdAnimFrameCommand* >* list;
	switch( framenum ) {
		case FC_START:
			list = &startCommands;
			break;
		case FC_FINISH:
			list = &finishCommands;
			break;
		case FC_BEGIN:
			list = &beginCommands;
			break;
		case FC_END:
			list = &endCommands;
			break;
		default:
			gameLocal.Error( "Unknown absolute frame command '%i' for anim '%s'", ( int )framenum, realname.c_str() );
			return false;
	}

	fc->IncRef();

	// store the new command 
	list->Alloc() = fc;
	return true;
}

/*
=====================
idAnim::CallFrameCommands
=====================
*/
void idAnim::CallFrameCommands( idClass* ent, int from, int to ) const {
	int index;
	int end;
	int frame;
	int numframes;

	numframes = anims[ 0 ]->NumFrames();
	if( numframes == 0 || frameLookup.Num() == 0 ) {
		return;
	}

	frame = from;
	while ( frame != to ) {
		frame++;

		if( frame >= frameLookup.Num() ) {
			break;
		}

		if ( frame >= numframes ) {
			frame = 0;
		}

		index = frameLookup[ frame ].firstCommand;
		end = index + frameLookup[ frame ].num;
		while ( index < end ) {
			const sdAnimFrameCommand* command = frameCommands[ index++ ];

			if ( g_debugFrameCommands.GetBool() ) {
				if( !idStr::Length( g_debugFrameCommandsFilter.GetString()) || idStr::FindText( command->GetTypeName(), g_debugFrameCommandsFilter.GetString(), false ) != idStr::INVALID_POSITION )  {
					gameLocal.Printf( "Anim '%s' frame command: Frame: %d, '%s'\n", realname.c_str(), frame + 1, command->GetTypeName() );	
				}
			}

			command->Run( ent );
		}
	}
}

/*
=====================
idAnim::CallAbosoluteFrameCommands
=====================
*/
void idAnim::CallAbsoluteFrameCommands( idClass *ent, absoluteFrameCommandType_t frame ) const {
	const char* frameName = NULL;
	const idList< sdAnimFrameCommand* >* list;
	switch( frame ) {
		case FC_START:
			list = &startCommands;
			frameName = "start";
			break;
		case FC_FINISH:
			list = &finishCommands;
			frameName = "finish";
			break;
		case FC_BEGIN:
			list = &beginCommands;
			frameName = "begin";
			break;
		case FC_END:
			list = &endCommands;
			frameName = "end";
			break;
		default:
			gameLocal.Error( "Unknown absolute frame command '%i' for anim '%s'", (int)frame, realname.c_str() );
	}

	for( int index = 0; index < list->Num(); index++ ) {
		const sdAnimFrameCommand* command = (*list)[ index ];

		if ( g_debugFrameCommands.GetBool() ) {
			if( !idStr::Length( g_debugFrameCommandsFilter.GetString()) || idStr::FindText( command->GetTypeName(), g_debugFrameCommandsFilter.GetString(), false ) != idStr::INVALID_POSITION )  {
				gameLocal.Printf( "Anim '%s' frame command: Frame: '%s', '%s'\n", realname.c_str(), frameName, command->GetTypeName() );	
			}
		}
		command->Run( ent );
	}
}

/*
=====================
idAnim::HasFrameCommands
=====================
*/
bool idAnim::HasFrameCommands( void ) const {
	if ( !frameCommands.Num() && !startCommands.Num() && !finishCommands.Num() ) {
		return false;
	}
	return true;
}

/*
=====================
idAnim::SetAnimFlags
=====================
*/
void idAnim::SetAnimFlags( const animFlags_t &animflags ) {
	flags = animflags;
}

/*
=====================
idAnim::GetAnimFlags
=====================
*/
const animFlags_t &idAnim::GetAnimFlags( void ) const {
	return flags;
}

/***********************************************************************

	idAnimBlend

***********************************************************************/

/*
=====================
idAnimBlend::idAnimBlend
=====================
*/
idAnimBlend::idAnimBlend( void ) {
	Reset( NULL );
}


/*
=====================
idAnimBlend::Reset
=====================
*/
void idAnimBlend::Reset( const idDeclModelDef *_modelDef ) {
	modelDef			= _modelDef;
	cycle				= 1;
	starttime			= 0;
	endtime				= 0;
	timeOffset			= 0;
	rate				= 1.0f;
	frame				= 0;
	allowMove			= true;
	allowFrameCommands	= true;
	animNum				= 0;

	memset( animWeights, 0, sizeof( animWeights ) );

	blendStartValue 	= 0.0f;
	blendEndValue		= 0.0f;
    blendStartTime		= 0;
	blendDuration		= 0;
}

/*
=====================
idAnimBlend::FullName
=====================
*/
const char *idAnimBlend::AnimFullName( void ) const {
	const idAnim *anim = Anim();
	if ( !anim ) {
		return "";
	}

	return anim->FullName();
}

/*
=====================
idAnimBlend::AnimName
=====================
*/
const char *idAnimBlend::AnimName( void ) const {
	const idAnim *anim = Anim();
	if ( !anim ) {
		return "";
	}

	return anim->Name();
}

/*
=====================
idAnimBlend::NumFrames
=====================
*/
int idAnimBlend::NumFrames( void ) const {
	const idAnim *anim = Anim();
	if ( !anim ) {
		return 0;
	}

	return anim->NumFrames();
}

/*
=====================
idAnimBlend::Length
=====================
*/
int	idAnimBlend::Length( void ) const {
	const idAnim *anim = Anim();
	if ( !anim ) {
		return 0;
	}

	return anim->Length();
}

/*
=====================
idAnimBlend::GetWeight
=====================
*/
float idAnimBlend::GetWeight( int currentTime ) const {
	int	timeDelta = currentTime - blendStartTime;

	if ( timeDelta <= 0 ) {
		return blendStartValue;
	} else if ( timeDelta >= blendDuration ) {
		return blendEndValue;
	}

	return Lerp( blendStartValue, blendEndValue, timeDelta / ( float )blendDuration );
}

/*
=====================
idAnimBlend::GetFinalWeight
=====================
*/
float idAnimBlend::GetFinalWeight( void ) const {
	return blendEndValue;
}

/*
=====================
idAnimBlend::SetWeight
=====================
*/
void idAnimBlend::SetWeight( float newweight, int currentTime, int blendTime ) {
	blendStartValue = GetWeight( currentTime );
	blendEndValue = newweight;
    blendStartTime = currentTime - 1;
	blendDuration = blendTime;

	if ( !newweight ) {
		endtime = currentTime + blendTime;
	}
}

/*
=====================
idAnimBlend::NumSyncedAnims
=====================
*/
int idAnimBlend::NumSyncedAnims( void ) const {
	const idAnim *anim = Anim();
	if ( !anim ) {
		return 0;
	}

	return anim->NumAnims();
}

/*
=====================
idAnimBlend::SetSyncedAnimWeight
=====================
*/
bool idAnimBlend::SetSyncedAnimWeight( int num, float weight ) {
	const idAnim *anim = Anim();
	if ( !anim ) {
		return false;
	}

	if ( ( num < 0 ) || ( num > anim->NumAnims() ) ) {
		return false;
	}

	animWeights[ num ] = weight;
	return true;
}

/*
=====================
idAnimBlend::SetFrame
=====================
*/
void idAnimBlend::SetFrame( idClass* ent, const idDeclModelDef *modelDef, int _animNum, float _frame, int currentTime, int blendTime ) {
	Reset( modelDef );
	if ( !modelDef ) {
		return;
	}
	
	const idAnim *_anim = modelDef->GetAnim( _animNum );
	if ( _anim == NULL ) {
		return;
	}
	_anim->CallAbsoluteFrameCommands( ent, idAnim::FC_BEGIN );

	const idMD5Anim *md5anim = _anim->MD5Anim( 0 );
	if ( modelDef->Joints().Num() != md5anim->NumJoints() ) {
		gameLocal.Warning( "Model '%s' has different # of joints than anim '%s'", modelDef->GetModelName(), md5anim->Name() );
		return;
	}
	
	animNum				= _animNum;
	starttime			= currentTime;
	endtime				= -1;
	cycle				= -1;
	animWeights[ 0 ]	= 1.0f;
	frame				= _frame;

	// a frame of 0 means it's not a single frame blend, so we set it to frame + 1
	if ( frame <= 0 ) {
		frame = 1;
	} else if ( frame > _anim->NumFrames() ) {
		frame = _anim->NumFrames();
	}

	// set up blend
	blendEndValue		= 1.0f;
	blendStartTime		= currentTime - 1;
	blendDuration		= blendTime;
	blendStartValue		= 0.0f;
}

/*
=====================
idAnimBlend::CycleAnim
=====================
*/
void idAnimBlend::CycleAnim( idClass* ent, const idDeclModelDef *modelDef, int _animNum, int currentTime, int blendTime ) {
	Reset( modelDef );
	if ( !modelDef ) {
		return;
	}
	
	const idAnim *_anim = modelDef->GetAnim( _animNum );
	if ( _anim == NULL ) {
		return;
	}
	_anim->CallAbsoluteFrameCommands( ent, idAnim::FC_BEGIN );

	const idMD5Anim *md5anim = _anim->MD5Anim( 0 );
	if ( modelDef->Joints().Num() != md5anim->NumJoints() ) {
		gameLocal.Warning( "Model '%s' has different # of joints than anim '%s'", modelDef->GetModelName(), md5anim->Name() );
		return;
	}

	animNum				= _animNum;
	animWeights[ 0 ]	= 1.0f;
	endtime				= -1;
	cycle				= -1;
	if ( _anim->GetAnimFlags().random_cycle_start ) {
		// start the animation at a random time so that characters don't walk in sync
		starttime = currentTime - static_cast< int >( gameLocal.random.RandomFloat() * _anim->Length() );
	} else {
		starttime = currentTime;
	}

	// set up blend
	blendEndValue		= 1.0f;
	blendStartTime		= currentTime - 1;
	blendDuration		= blendTime;
	blendStartValue		= 0.0f;
}

/*
=====================
idAnimBlend::PlayAnim
=====================
*/
void idAnimBlend::PlayAnim( idClass* ent, const idDeclModelDef *modelDef, int _animNum, int currentTime, int blendTime ) {
	Reset( modelDef );
	if ( !modelDef ) {
		return;
	}
	
	const idAnim *_anim = modelDef->GetAnim( _animNum );
	if ( _anim == NULL ) {
		return;
	}
	_anim->CallAbsoluteFrameCommands( ent, idAnim::FC_BEGIN );

	const idMD5Anim *md5anim = _anim->MD5Anim( 0 );
	if ( modelDef->Joints().Num() != md5anim->NumJoints() ) {
		gameLocal.Warning( "Model '%s' has different # of joints than anim '%s'", modelDef->GetModelName(), md5anim->Name() );
		return;
	}

	animNum				= _animNum;
	starttime			= currentTime;
	endtime				= starttime + _anim->Length();
	cycle				= 1;
	animWeights[ 0 ]	= 1.0f;

	// set up blend
	blendEndValue		= 1.0f;
	blendStartTime		= currentTime - 1;
	blendDuration		= blendTime;
	blendStartValue		= 0.0f;
}

/*
=====================
idAnimBlend::Clear
=====================
*/
void idAnimBlend::Clear( idClass* ent, int currentTime, int clearTime ) {
	const idAnim* anim = Anim();
	if ( anim != NULL ) {
		anim->CallAbsoluteFrameCommands( ent, idAnim::FC_END );
	}

	if ( !clearTime ) {
		Reset( modelDef );
	} else {
		SetWeight( 0.0f, currentTime, clearTime );
	}
}

/*
=====================
idAnimBlend::IsDone
=====================
*/
bool idAnimBlend::IsDone( int currentTime ) const {
	if ( frame || ( endtime > 0 && ( currentTime >= endtime ) ) ) {
		return true;
	}

	if ( ( blendEndValue <= 0.0f ) && ( currentTime >= ( blendStartTime + blendDuration ) ) ) {
		return true;
	}

	return false;
}

/*
=====================
idAnimBlend::FrameHasChanged
=====================
*/
bool idAnimBlend::FrameHasChanged( int currentTime ) const {
	// if we don't have an anim, no change
	if ( !animNum ) {
		return false;
	}

	// if anim is done playing, no change
	if ( ( endtime > 0 ) && ( currentTime > endtime ) ) {
		return false;
	}

	// if our blend weight changes, we need to update
	if ( ( currentTime < ( blendStartTime + blendDuration ) && ( blendStartValue != blendEndValue ) ) ) {
		return true;
	}

	// if we're a single frame anim and this isn't the frame we started on, we don't need to update
	if ( ( frame || ( NumFrames() == 1 ) ) && ( currentTime != starttime ) ) {
		return false;
	}

	return true;
}

/*
=====================
idAnimBlend::GetCycleCount
=====================
*/
int idAnimBlend::GetCycleCount( void ) const {
	return cycle;
}

/*
=====================
idAnimBlend::SetCycleCount
=====================
*/
void idAnimBlend::SetCycleCount( int count ) {
	const idAnim *anim = Anim();

	if ( !anim ) {
		cycle = -1;
		endtime = 0;
	} else {
		cycle = count;
		if ( cycle < 0 ) {
			cycle = -1;
			endtime	= -1;
		} else if ( cycle == 0 ) {
			cycle = 1;

			// most of the time we're running at the original frame rate, so avoid the int-to-float-to-int conversion
			if ( rate == 1.0f ) {
				endtime	= starttime - timeOffset + anim->Length();
			} else if ( rate != 0.0f ) {
				endtime	= starttime - timeOffset + static_cast< int >( anim->Length() / rate );
			} else {
				endtime = -1;
			}
		} else {
			// most of the time we're running at the original frame rate, so avoid the int-to-float-to-int conversion
			if ( rate == 1.0f ) {
				endtime	= starttime - timeOffset + anim->Length() * cycle;
			} else if ( rate != 0.0f ) {
				endtime	= starttime - timeOffset + static_cast< int >( ( anim->Length() * cycle ) / rate );
			} else {
				endtime = -1;
			}
		}
	}
}

/*
=====================
idAnimBlend::SetPlaybackRate
=====================
*/
void idAnimBlend::SetPlaybackRate( int currentTime, float newRate ) {
	int animTime;

	if ( rate == newRate ) {
		return;
	}

	animTime = AnimTime( currentTime );
	if ( newRate == 1.0f ) {
		timeOffset = animTime - ( currentTime - starttime );
	} else {
		timeOffset = animTime - static_cast< int >( ( currentTime - starttime ) * newRate );
	}

	rate = newRate;

	// update the anim endtime
	SetCycleCount( cycle );
}

/*
=====================
idAnimBlend::GetPlaybackRate
=====================
*/
float idAnimBlend::GetPlaybackRate( void ) const {
	return rate;
}

/*
=====================
idAnimBlend::SetStartTime
=====================
*/
void idAnimBlend::SetStartTime( int _startTime ) {
	starttime = _startTime;

	// update the anim endtime
	SetCycleCount( cycle );
}

/*
=====================
idAnimBlend::GetStartTime
=====================
*/
int idAnimBlend::GetStartTime( void ) const {
	if ( !animNum ) {
		return 0;
	}

	return starttime;
}

/*
=====================
idAnimBlend::GetEndTime
=====================
*/
int idAnimBlend::GetEndTime( void ) const {
	if ( !animNum ) {
		return 0;
	}

	return endtime;
}

/*
=====================
idAnimBlend::PlayLength
=====================
*/
int idAnimBlend::PlayLength( void ) const {
	if ( !animNum ) {
		return 0;
	}

	if ( endtime < 0 ) {
		return -1;
	}

	return endtime - starttime + timeOffset;
}

/*
=====================
idAnimBlend::AllowMovement
=====================
*/
void idAnimBlend::AllowMovement( bool allow ) {
	allowMove = allow;
}

/*
=====================
idAnimBlend::AllowFrameCommands
=====================
*/
void idAnimBlend::AllowFrameCommands( bool allow ) {
	allowFrameCommands = allow;
}


/*
=====================
idAnimBlend::Anim
=====================
*/
const idAnim *idAnimBlend::Anim( void ) const {
	if ( !modelDef ) {
		return NULL;
	}

	const idAnim *anim = modelDef->GetAnim( animNum );
	return anim;
}

/*
=====================
idAnimBlend::AnimNum
=====================
*/
int idAnimBlend::AnimNum( void ) const {
	return animNum;
}

/*
=====================
idAnimBlend::AnimTime
=====================
*/
int idAnimBlend::AnimTime( int currentTime ) const {
	int time;
	int length;
	const idAnim *anim = Anim();

	if ( anim == NULL ) {
		return 0;
	}

	if ( !idMath::FloatIsZero( frame ) ) {
		const idMD5Anim* md5anim = anim->MD5Anim( 0 );
		return ( ( frame - 1 ) * 1000.f ) / md5anim->GetFrameRate();
	}

	// most of the time we're running at the original frame rate, so avoid the int-to-float-to-int conversion
	if ( rate == 1.0f ) {
		time = currentTime - starttime + timeOffset;
	} else {
		time = static_cast<int>( ( currentTime - starttime ) * rate ) + timeOffset;
	}

	// given enough time, we can easily wrap time around in our frame calculations, so
	// keep cycling animations' time within the length of the anim.
	length = anim->Length();
	if ( ( cycle < 0 ) && ( length > 0 ) ) {
		time %= length;

		// time will wrap after 24 days (oh no!), resulting in negative results for the %.
		// adding the length gives us the proper result.
		if ( time < 0 ) {
			time += length;
		}
	}

	return time;
}

/*
=====================
idAnimBlend::GetFrameNumber
=====================
*/
int idAnimBlend::GetFrameNumber( int currentTime ) const {
	const idMD5Anim	*md5anim;
	frameBlend_t	frameinfo;
	int				animTime;

	const idAnim *anim = Anim();
	if ( !anim ) {
		return 1;
	}

	if ( frame ) {
		return frame;
	}

	md5anim = anim->MD5Anim( 0 );
	animTime = AnimTime( currentTime );
	md5anim->ConvertTimeToFrame( animTime, cycle, frameinfo );

	return frameinfo.frame1 + 1;
}

/*
=====================
idAnimBlend::CallFrameCommands
=====================
*/
void idAnimBlend::CallFrameCommands( idClass *ent, int fromtime, int totime ) const {
	const idMD5Anim	*md5anim;
	frameBlend_t	frame1;
	frameBlend_t	frame2;
	int				fromFrameTime;
	int				toFrameTime;

	if ( !allowFrameCommands || !ent || frame || ( ( endtime > 0 ) && ( fromtime > endtime ) ) ) {
		return;
	}

	const idAnim *anim = Anim();
	if ( !anim || !anim->HasFrameCommands() ) {
		return;
	}

	if ( totime <= starttime ) {
		// don't play until next frame or we'll play commands twice.
		// this happens on the player sometimes.
		anim->CallAbsoluteFrameCommands( ent, idAnim::FC_START );
		return;
	}

	if ( IsDone( totime )) {
		anim->CallAbsoluteFrameCommands( ent, idAnim::FC_FINISH );
	}

	fromFrameTime	= AnimTime( fromtime );
	toFrameTime		= AnimTime( totime );
	if ( toFrameTime < fromFrameTime ) {
		toFrameTime += anim->Length();
	}

	md5anim = anim->MD5Anim( 0 );
	md5anim->ConvertTimeToFrame( fromFrameTime, cycle, frame1 );
	md5anim->ConvertTimeToFrame( toFrameTime, cycle, frame2 );

	if ( fromFrameTime <= 0 ) {
		// make sure first frame is called
		anim->CallFrameCommands( ent, -1, frame2.frame1 );
	} else {
		anim->CallFrameCommands( ent, frame1.frame1, frame2.frame1 );
	}
}

/*
=====================
idAnimBlend::BlendAnim
=====================
*/
bool idAnimBlend::BlendAnim( int currentTime, animChannel_t channel, int numJoints, idJointQuat *blendFrame, float &blendWeight, bool removeOriginOffset, bool overrideBlend, bool printInfo ) const {
	int				i;
	float			lerp;
	float			mixWeight;
	const idMD5Anim	*md5anim;
	idJointQuat		*ptr;
	frameBlend_t	frametime;
	idJointQuat		*jointFrame;
	idJointQuat		*mixFrame;
	int				numAnims;
	int				time;

	const idAnim *anim = Anim();
	if ( !anim ) {
		return false;
	}

	float weight = GetWeight( currentTime );
	if ( blendWeight > 0.0f ) {
		if ( ( endtime >= 0 ) && ( currentTime >= endtime ) ) {
			return false;
		}
		if ( !weight ) {
			return false;
		}
		if ( overrideBlend ) {
			blendWeight = 1.0f - weight;
		}
	}

	if ( ( channel == ANIMCHANNEL_ALL ) && !blendWeight ) {
		// we don't need a temporary buffer, so just store it directly in the blend frame
		jointFrame = blendFrame;
	} else {
		// allocate a temporary buffer to copy the joints from
		jointFrame = ( idJointQuat * )_alloca16( numJoints * sizeof( *jointFrame ) );
	}

	time = AnimTime( currentTime );

	numAnims = anim->NumAnims();
	if ( numAnims == 1 ) {
		md5anim = anim->MD5Anim( 0 );
		md5anim->ConvertTimeToFrame( time, cycle, frametime );
		md5anim->GetInterpolatedFrame( frametime, jointFrame, modelDef->GetChannelJoints( channel ), modelDef->NumJointsOnChannel( channel ) );
	} else {
		//
		// need to mix the multipoint anim together first
		//
		// allocate a temporary buffer to copy the joints to
		mixFrame = ( idJointQuat * )_alloca16( numJoints * sizeof( *jointFrame ) );

		anim->MD5Anim( 0 )->ConvertTimeToFrame( time, cycle, frametime );

		ptr = jointFrame;
		mixWeight = 0.0f;
		for( i = 0; i < numAnims; i++ ) {
			if ( animWeights[ i ] > 0.0f ) {
				mixWeight += animWeights[ i ];
				lerp = animWeights[ i ] / mixWeight;
				md5anim = anim->MD5Anim( i );
				md5anim->GetInterpolatedFrame( frametime, ptr, modelDef->GetChannelJoints( channel ), modelDef->NumJointsOnChannel( channel ) );

				// only blend after the first anim is mixed in
				if ( ptr != jointFrame ) {
					SIMDProcessor->BlendJoints( jointFrame, ptr, lerp, modelDef->GetChannelJoints( channel ), modelDef->NumJointsOnChannel( channel ) );
				}

				ptr = mixFrame;
			}
		}

		if ( !mixWeight ) {
			return false;
		}
	}

	if ( removeOriginOffset ) {
		if ( allowMove ) {
#ifdef VELOCITY_MOVE
			jointFrame[ 0 ].t.x = 0.0f;
#else
			jointFrame[ 0 ].t.Zero();
#endif
		}

		if ( anim->GetAnimFlags().anim_turn ) {
			jointFrame[ 0 ].q.Set( -0.70710677f, 0.0f, 0.0f, 0.70710677f );
		}
	}

	if ( !blendWeight ) {
		blendWeight = weight;
		if ( channel != ANIMCHANNEL_ALL ) {
			const int *index = modelDef->GetChannelJoints( channel );
			const int num = modelDef->NumJointsOnChannel( channel );
			for( i = 0; i < num; i++ ) {
				int j = index[i];
				blendFrame[j].t = jointFrame[j].t;
				blendFrame[j].q = jointFrame[j].q;
			}
		}
    } else {
		blendWeight += weight;
		lerp = weight / blendWeight;
		SIMDProcessor->BlendJoints( blendFrame, jointFrame, lerp, modelDef->GetChannelJoints( channel ), modelDef->NumJointsOnChannel( channel ) );
	}

	if ( printInfo ) {
		if ( frame != 0.0f ) {
			gameLocal.Printf( "  %s: '%s', %.3f, %.2f%%\n", channelNames[ channel ], anim->FullName(), frame, weight * 100.0f );
		} else {
			gameLocal.Printf( "  %s: '%s', %.3f, %.2f%%\n", channelNames[ channel ], anim->FullName(), ( float )frametime.frame1 + frametime.backlerp, weight * 100.0f );
		}
	}

	return true;
}

/*
=====================
idAnimBlend::BlendOrigin
=====================
*/
void idAnimBlend::BlendOrigin( int currentTime, idVec3 &blendPos, float &blendWeight, bool removeOriginOffset ) const {
	float	lerp;
	idVec3	animpos;
	idVec3	pos;
	int		time;
	int		num;
	int		i;

	if ( frame || ( ( endtime > 0 ) && ( currentTime > endtime ) ) ) {
		return;
	}

	const idAnim *anim = Anim();
	if ( !anim ) {
		return;
	}

	if ( allowMove && removeOriginOffset ) {
		return;
	}

	float weight = GetWeight( currentTime );
	if ( !weight ) {
		return;
	}

	time = AnimTime( currentTime );

	pos.Zero();
	num = anim->NumAnims();
	for( i = 0; i < num; i++ ) {
		anim->GetOrigin( animpos, i, time, cycle );
		pos += animpos * animWeights[ i ];
	}

	if ( !blendWeight ) {
		blendPos = pos;
		blendWeight = weight;
	} else {
		lerp = weight / ( blendWeight + weight );
		blendPos += lerp * ( pos - blendPos );
		blendWeight += weight;
	}
}

/*
=====================
idAnimBlend::BlendDelta
=====================
*/
void idAnimBlend::BlendDelta( int fromtime, int totime, idVec3 &blendDelta, float &blendWeight ) const {
	idVec3	pos1;
	idVec3	pos2;
	idVec3	animpos;
	idVec3	delta;
	int		time1;
	int		time2;
	float	lerp;
	int		num;
	int		i;
	
	if ( frame || !allowMove || ( ( endtime > 0 ) && ( fromtime > endtime ) ) ) {
		return;
	}

	const idAnim *anim = Anim();
	if ( !anim ) {
		return;
	}

	float weight = GetWeight( totime );
	if ( !weight ) {
		return;
	}

	time1 = AnimTime( fromtime );
	time2 = AnimTime( totime );
	if ( time2 < time1 ) {
		time2 += anim->Length();
	}

	num = anim->NumAnims();

	pos1.Zero();
	pos2.Zero();
	for( i = 0; i < num; i++ ) {
		anim->GetOrigin( animpos, i, time1, cycle );
		pos1 += animpos * animWeights[ i ];

		anim->GetOrigin( animpos, i, time2, cycle );
		pos2 += animpos * animWeights[ i ];
	}

	delta = pos2 - pos1;
	if ( !blendWeight ) {
		blendDelta = delta;
		blendWeight = weight;
	} else {
		lerp = weight / ( blendWeight + weight );
		blendDelta += lerp * ( delta - blendDelta );
		blendWeight += weight;
	}
}

/*
=====================
idAnimBlend::BlendDeltaRotation
=====================
*/
void idAnimBlend::BlendDeltaRotation( int fromtime, int totime, idQuat &blendDelta, float &blendWeight ) const {
	idQuat	q1;
	idQuat	q2;
	idQuat	q3;
	int		time1;
	int		time2;
	float	lerp;
	float	mixWeight;
	int		num;
	int		i;
	
	if ( frame || !allowMove || ( ( endtime > 0 ) && ( fromtime > endtime ) ) ) {
		return;
	}

	const idAnim *anim = Anim();
	if ( !anim || !anim->GetAnimFlags().anim_turn ) {
		return;
	}

	float weight = GetWeight( totime );
	if ( !weight ) {
		return;
	}

	time1 = AnimTime( fromtime );
	time2 = AnimTime( totime );
	if ( time2 < time1 ) {
		time2 += anim->Length();
	}

	q1.Set( 0.0f, 0.0f, 0.0f, 1.0f );
	q2.Set( 0.0f, 0.0f, 0.0f, 1.0f );

	mixWeight = 0.0f;
	num = anim->NumAnims();
	for( i = 0; i < num; i++ ) {
		if ( animWeights[ i ] > 0.0f ) {
			mixWeight += animWeights[ i ];
			if ( animWeights[ i ] == mixWeight ) {
				anim->GetOriginRotation( q1, i, time1, cycle );
				anim->GetOriginRotation( q2, i, time2, cycle );
			} else {
				lerp = animWeights[ i ] / mixWeight;
				anim->GetOriginRotation( q3, i, time1, cycle );
				q1.Slerp( q1, q3, lerp );

				anim->GetOriginRotation( q3, i, time2, cycle );
				q2.Slerp( q1, q3, lerp );
			}
		}
	}

	q3 = q1.Inverse() * q2;
	if ( !blendWeight ) {
		blendDelta = q3;
		blendWeight = weight;
	} else {
		lerp = weight / ( blendWeight + weight );
		blendDelta.Slerp( blendDelta, q3, lerp );
		blendWeight += weight;
	}
}

/*
=====================
idAnimBlend::AddBounds
=====================
*/
bool idAnimBlend::AddBounds( int currentTime, idBounds &bounds, bool removeOriginOffset, bool ignoreLastFrame ) const {
	int			i;
	int			num;
	idBounds	b;
	int			time;
	idVec3		pos;
	bool		addorigin;

	if ( !ignoreLastFrame && ( endtime > 0 ) && ( currentTime > endtime ) ) {
		return false;
	}

	const idAnim *anim = Anim();
	if ( !anim ) {
		return false;
	}

	float weight = GetWeight( currentTime );
	if ( !weight ) {
		return false;
	}

	time = AnimTime( currentTime );
	num = anim->NumAnims();
	
	addorigin = !allowMove || !removeOriginOffset;
	for( i = 0; i < num; i++ ) {
		if ( anim->GetBounds( b, i, time, cycle ) ) {
			if ( addorigin ) {
				anim->GetOrigin( pos, i, time, cycle );
				b.TranslateSelf( pos );
			}
			bounds.AddBounds( b );
		}
	}

	return true;
}

/***********************************************************************

	idDeclModelDef

***********************************************************************/

/*
=====================
idDeclModelDef::idDeclModelDef
=====================
*/
idDeclModelDef::idDeclModelDef() : anims(1), animLookup(1) {
	modelHandle	= NULL;
	skin		= NULL;
	offset.Zero();
	for ( int i = 0; i < ANIM_NumAnimChannels; i++ ) {
		channelJoints[i].Clear();
	}
}

/*
=====================
idDeclModelDef::~idDeclModelDef
=====================
*/
idDeclModelDef::~idDeclModelDef() {
	FreeData();
}

/*
=================
idDeclModelDef::Size
=================
*/
size_t idDeclModelDef::Size( void ) const {
	size_t size = sizeof( idDeclModelDef );
	size += joints.Size();
	size += jointParents.Size();
	for ( int i = 0; i < ANIM_NumAnimChannels; i++ ) {
		size += channelJoints[ i ].Size();
	}
	size += anims.Size();
	size += animLookup.Size();
	return size;
}

/*
=====================
idDeclModelDef::CopyDecl
=====================
*/
void idDeclModelDef::CopyDecl( const idDeclModelDef *decl ) {
	int i;

	FreeData();

	offset = decl->offset;
	modelHandle = decl->modelHandle;
	skin = decl->skin;

	anims = decl->anims;

	AnimTable::Iterator iter = anims.Begin();
	for( iter; iter != anims.End(); ++iter ) {
		iter->second->IncRef();
	}

/*	AnimTable::Iterator iter = anims.Begin();

	int count = 0;
	for( iter; iter != anims.End(); ++iter ) {
		iter->second = new idAnim( this, iter->second );
		count++;
	}
	gameLocal.Printf( "^2Copied %d anims from '%s' to '%s'\n", count, decl->GetName(), GetName() );*/

	joints.SetNum( decl->joints.Num() );
	memcpy( joints.Begin(), decl->joints.Begin(), decl->joints.Num() * sizeof( joints[0] ) );
	jointParents.SetNum( decl->jointParents.Num() );
	memcpy( jointParents.Begin(), decl->jointParents.Begin(), decl->jointParents.Num() * sizeof( jointParents[0] ) );
	for ( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		channelJoints[i] = decl->channelJoints[i];
	}
}

/*
=====================
idDeclModelDef::FreeData
=====================
*/
void idDeclModelDef::FreeData( void ) {
	AnimTable::Iterator iter = anims.Begin();
	for( iter; iter != anims.End(); ++iter ) {
		iter->second->DecRef();
	}
	anims.Clear();

	animLookup.Clear();

	joints.Clear();
	jointParents.Clear();
	modelHandle	= NULL;
	skin = NULL;
	offset.Zero();
	for ( int i = 0; i < ANIM_NumAnimChannels; i++ ) {
		channelJoints[i].Clear();
	}
}

/*
================
idDeclModelDef::DefaultDefinition
================
*/
const char *idDeclModelDef::DefaultDefinition( void ) const {
	return "{ }";
}

/*
====================
idDeclModelDef::FindJoint
====================
*/
const jointInfo_t *idDeclModelDef::FindJoint( const char *name ) const {
	int					i;
	const idMD5Joint	*joint;

	if ( !modelHandle ) {
		return NULL;
	}
	
	joint = modelHandle->GetJoints();
	for( i = 0; i < joints.Num(); i++, joint++ ) {
		if ( !joint->name.Icmp( name ) ) {
			return &joints[ i ];
		}
	}

	return NULL;
}

/*
=====================
idDeclModelDef::GetJointList
=====================
*/
void idDeclModelDef::GetJointList( const char *jointnames, idList< jointHandle_t >& jointList ) const {
	const char*			pos;
	idStr				jointname;
	const jointInfo_t*	joint;
	const jointInfo_t*	child;
	int					i;
	int					num;
	bool				getChildren;
	bool				subtract;

	if ( !modelHandle ) {
		return;
	}

	jointList.Clear();

	num = modelHandle->NumJoints();

	// scan through list of joints and add each to the joint list
	pos = jointnames;
	while( *pos ) {
		// skip over whitespace
		while( ( *pos != 0 ) && isspace( *pos ) ) {
			pos++;
		}

		if ( !*pos ) {
			// no more names
			break;
		}

		// copy joint name
		jointname = "";

		if ( *pos == '-' ) {
			subtract = true;
			pos++;
		} else {
			subtract = false;
		}

		if ( *pos == '*' ) {
			getChildren = true;
			pos++;
		} else {
			getChildren = false;
		}

		while( ( *pos != 0 ) && !isspace( *pos ) ) {
			jointname += *pos;
			pos++;
		}

		joint = FindJoint( jointname );
		if ( !joint ) {
			gameLocal.Warning( "Unknown joint '%s' in '%s' for model '%s'", jointname.c_str(), jointnames, GetName() );
			continue;
		}

		if ( !subtract ) {
			jointList.AddUnique( joint->num );
		} else {
			jointList.Remove( joint->num );
		}

		if ( getChildren ) {
			// include all joint's children
			child = joint + 1;
			for( i = joint->num + 1; i < num; i++, child++ ) {
				// all children of the joint should follow it in the list.
				// once we reach a joint without a parent or with a parent
				// who is earlier in the list than the specified joint, then
				// we've gone through all it's children.
				if ( child->parentNum < joint->num ) {
					break;
				}

				if ( !subtract ) {
					jointList.AddUnique( child->num );
				} else {
					jointList.Remove( child->num );
				}
			}
		}
	}
}

/*
=====================
idDeclModelDef::Touch
=====================
*/
void idDeclModelDef::Touch( void ) const {
	if ( modelHandle ) {
		renderModelManager->FindModel( modelHandle->Name() );
	}
}

/*
=====================
idDeclModelDef::GetDefaultSkin
=====================
*/
const idDeclSkin *idDeclModelDef::GetDefaultSkin( void ) const {
	return skin;
}

/*
=====================
idDeclModelDef::GetDefaultPose
=====================
*/
const idJointQuat *idDeclModelDef::GetDefaultPose( void ) const {
	return modelHandle->GetDefaultPose();
}

/*
=====================
idDeclModelDef::SetupJoints
=====================
*/
void idDeclModelDef::SetupJoints( int *numJoints, idJointMat **jointList, idBounds &frameBounds, bool removeOriginOffset ) const {
	int					num;
	const idJointQuat	*pose;
	idJointMat			*list;

	if ( !modelHandle || modelHandle->IsDefaultModel() ) {
		Mem_FreeAligned( (*jointList) );
		(*jointList) = NULL;
		frameBounds.Clear();
		return;
	}

	// get the number of joints
	num = modelHandle->NumJoints();

	if ( !num ) {
		gameLocal.Error( "model '%s' has no joints", modelHandle->Name() );
	}

	// set up initial pose for model (with no pose, model is just a jumbled mess)
	list = (idJointMat *) Mem_AllocAligned( num * sizeof( list[0] ), ALIGN_16 );
	pose = GetDefaultPose();

	// convert the joint quaternions to joint matrices
	SIMDProcessor->ConvertJointQuatsToJointMats( list, pose, joints.Num() );

	// check if we offset the model by the origin joint
	if ( removeOriginOffset ) {
#ifdef VELOCITY_MOVE
		list[ 0 ].SetTranslation( idVec3( offset.x, offset.y + pose[0].t.y, offset.z + pose[0].t.z ) );
#else
		list[ 0 ].SetTranslation( offset );
#endif
	} else {
		list[ 0 ].SetTranslation( pose[0].t + offset );
	}

	// transform the joint hierarchy
	SIMDProcessor->TransformJoints( list, jointParents.Begin(), 1, joints.Num() - 1 );

	*numJoints = num;
	*jointList = list;

	// get the bounds of the default pose
	frameBounds = modelHandle->Bounds( NULL );
}

void G_BuidAnimList_r( animList_t& list, const idList< idStrList >& animGroups, int depth, const char* buildString, const char* animName ) {
	if ( depth == animGroups.Num() ) {
		animListEntry_t& entry = list.Alloc();
		entry.name = va( "%s%s", buildString, animName );
		entry.anim = NULL;
		return;
	}

	for ( int i = 0; i < animGroups[ depth ].Num(); i++ ) {
		idStr temp = va( "%s%s_", buildString, animGroups[ depth ][ i ].c_str() );
		G_BuidAnimList_r( list, animGroups, depth + 1, temp, animName );
	}
}

/*
=====================
idDeclModelDef::ParseAbsoluteFrameCommand
=====================
*/
bool idDeclModelDef::ParseAbsoluteFrameCommand( idAnim::absoluteFrameCommandType_t cmd, idParser& src, animList_t& animList ) {
	sdAnimFrameCommand* _cmd = idAnim::ReadFrameCommand( src );
	if ( _cmd == NULL ) {
		src.Error( "idDeclModelDef::ParseAnim Failed to Parse Frame Command" );
	}

	for ( int i = 0; i < animList.Num(); i++ ) {
		if ( !animList[ i ].anim->AddAbsoluteFrameCommand( cmd, _cmd ) ) {
			_cmd->DecRef();
			return false;
		}
	}

	_cmd->DecRef();

	return true;
}

/*
=====================
idDeclModelDef::ParseAnim
=====================
*/
bool idDeclModelDef::ParseAnim( idParser &src, const idList< idStrList >& animGroups ) {
	const idMD5Anim*	md5anims[ ANIM_MaxSyncedAnims ];
	const idMD5Anim*	md5anim;
	idToken				token;
	int					numAnims;

	numAnims = 0;
	memset( md5anims, 0, sizeof( md5anims ) );

	idToken realName;
	if ( !src.ReadToken( &realName ) ) {
		src.Warning( "idDeclModelDef::ParseAnim Unexpected end of file" );
		return false;
	}

	animList_t animList;
	G_BuidAnimList_r( animList, animGroups, 0, "", realName );
	for ( int i = 0; i < animList.Num(); i++ ) {
		AnimTable::Iterator iter = anims.Find( animList[ i ].name );

		if ( iter != anims.End() ) {
			if ( iter->second->IsBaseModelDef( this ) ) {
				src.Error( "Duplicate anim '%s'", animList[ i ].name.c_str() );
				return false;
			}
			iter->second->DecRef();
			animList[ i ].anim = new idAnim();
		} else {
			// create the alias associated with this animation
			animList[ i ].anim = new idAnim();
			anims.Set( animList[ i ].name, animList[ i ].anim );
		}

		animList[ i ].alias = animList[ i ].name;

		// random anims end with a number.  find the numeric suffix of the animation.
		int len = animList[ i ].alias.Length();
		int j;
		for ( j = len - 1; j > 0; j-- ) {
			if ( !isdigit( animList[ i ].alias[ j ] ) ) {
				break;
			}
		}

		// check for zero length name, or a purely numeric name
		if ( j <= 0 ) {
			src.Error( "Invalid animation name '%s'", animList[ i ].alias.c_str() );
			return false;
		}

		// remove the numeric suffix
		animList[ i ].alias.CapLength( j + 1 );
	}

	// parse the anims from the string
	do {
		if( !src.ReadToken( &token ) ) {
			src.Warning( "Unexpected end of file" );
			return false;
		}

		// lookup the animation
		md5anim = animationLib.GetAnim( token );
		if ( !md5anim ) {
			src.Warning( "Couldn't load anim '%s'", token.c_str() );
			return false;
		}

		md5anim->CheckModelHierarchy( modelHandle );

		if ( numAnims > 0 ) {
			// make sure it's the same length as the other anims
			if ( md5anim->Length() != md5anims[ 0 ]->Length() ) {
				src.Warning( "Anim '%s' does not match length of anim '%s'", md5anim->Name(), md5anims[ 0 ]->Name() );
				return false;
			}
		}

		if ( numAnims >= ANIM_MaxSyncedAnims ) {
			src.Warning( "Exceeded max synced anims (%d)", ANIM_MaxSyncedAnims );
			return false;
		}

		// add it to our list
		md5anims[ numAnims ] = md5anim;
		numAnims++;
	} while ( src.CheckTokenString( "," ) );

	if ( !numAnims ) {
		src.Warning( "No animation specified" );
		return false;
	}

	for ( int i = 0; i < animList.Num(); i++ ) {
		animList[ i ].anim->SetAnim( this, animList[ i ].name, animList[ i ].alias, numAnims, md5anims );
	}

	animFlags_t flags;
	memset( &flags, 0, sizeof( flags ) );

	// parse any frame commands or animflags
	if ( src.CheckTokenString( "{" ) ) {
		while( 1 ) {
			if( !src.ReadToken( &token ) ) {
				src.Warning( "Unexpected end of file" );
				return false;
			}
			if ( token == "}" ) {
				break;
			}else if ( token == "prevent_idle_override" ) {
				flags.prevent_idle_override = true;
			} else if ( token == "random_cycle_start" ) {
				flags.random_cycle_start = true;
			} else if ( token == "ai_no_turn" ) {
				flags.ai_no_turn = true;
			} else if ( token == "ai_fixed_forward" ) {
				flags.ai_fixed_forward = true;
			} else if ( token == "no_pitch" ) {
				flags.no_pitch = true;
			} else if ( token == "anim_turn" ) {
				flags.anim_turn = true;
			} else if ( token == "frame" ) {
				// create a frame command
				int			framenum;

				// make sure we don't have any line breaks while reading the frame command so the error line # will be correct
				if ( !src.ReadTokenOnLine( &token ) ) {
					src.Warning( "Missing frame # after 'frame'" );
					return false;
				}
				if ( token.type == TT_PUNCTUATION && token == "-" ) {
					src.Warning( "Invalid frame # after 'frame'" );
					return false;
				} else if( token.Icmp( "start" ) == 0 ) {
					if ( !ParseAbsoluteFrameCommand( idAnim::FC_START, src, animList ) ) {
						return false;
					}
				} else if( token.Icmp( "finish" ) == 0 ) {
					if ( !ParseAbsoluteFrameCommand( idAnim::FC_FINISH, src, animList ) ) {
						return false;
					}
				} else if( token.Icmp( "begin" ) == 0 ) {
					if ( !ParseAbsoluteFrameCommand( idAnim::FC_BEGIN, src, animList ) ) {
						return false;
					}
				} else if( token.Icmp( "end" ) == 0 ) {
					if ( !ParseAbsoluteFrameCommand( idAnim::FC_END, src, animList ) ) {
						return false;
					}
				} else if ( token.type != TT_NUMBER || token.subtype == TT_FLOAT ) {
					src.Error( "expected positive integer value, 'start' or 'finish', found '%s'", token.c_str() );
				} else {
					// get the frame number
					framenum = token.GetIntValue();

					sdAnimFrameCommand* cmd = idAnim::ReadFrameCommand( src );
					if ( !cmd ) {
						src.Error( "idDeclModelDef::ParseAnim Failed to Parse Frame Command" );
					} else {
						for ( int i = 0; i < animList.Num(); i++ ) {
							// put the command on the specified frame of the animation
							if ( !animList[ i ].anim->AddFrameCommand( framenum, cmd ) ) {
								cmd->DecRef();
								return false;
							}
						}
						cmd->DecRef();
					}
				}
			} else {
				src.Warning( "Unknown command '%s'", token.c_str() );
				return false;
			}
		}
	}

	for ( int i = 0; i < animList.Num(); i++ ) {
		// set the flags
		animList[ i ].anim->SetAnimFlags( flags );
	}
	return true;
}

size_t c_modelDefMemory = 0;

/*
================
idDeclModelDef::Parse
================
*/
bool idDeclModelDef::Parse( const char *text, const int textLength ) {
	int					i;
	int					num;
	idStr				filename;
	idStr				extension;
	const idMD5Joint	*md5joint;
	const idMD5Joint	*md5joints;
	idParser				src;
	idToken				token;
	idToken				token2;
	idStr				jointnames;
	animChannel_t		channel;
	jointHandle_t		jointnum;
	idList<jointHandle_t> jointList;

	src.SetFlags( DECL_LEXER_FLAGS );
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );

	src.SkipUntilString( "{", &token );

	idList< idStrList > animGroups;

	static int counter = 0;
	bool updatePacifier = !networkSystem->IsDedicated();

	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		}

		if( counter & 15 && updatePacifier ) {
			common->PacifierUpdate();
		}
		counter++;

		if ( token == "inherit" ) {
			if( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				return false;
			}
			
			const idDeclModelDef* copy = gameLocal.declModelDefType.LocalFind( token2, false );
			if ( !copy ) {
				gameLocal.Warning( "Unknown model definition '%s'", token2.c_str() );
			} else if ( copy->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "inherited model definition '%s' defaulted", token2.c_str() );
				return false;
			} else {
				CopyDecl( copy );
			}
		} else if ( token == "skin" ) {
			if( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				return false;
			}
			skin = declHolder.declSkinType.LocalFind( token2 );
			if ( !skin ) {
				src.Warning( "Skin '%s' not found", token2.c_str() );
				return false;
			}
		} else if ( token == "mesh" ) {
			if( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				return false;
			}
			filename = token2;
			filename.ExtractFileExtension( extension );
			if ( extension != MD5_MESH_EXT ) {
				src.Warning( "Invalid model for MD5 mesh" );
				return false;
			}
			modelHandle = renderModelManager->FindModel( filename );
			if ( !modelHandle ) {
				src.Warning( "Model '%s' not found", filename.c_str() );
				return false;
			}

			if ( modelHandle->IsDefaultModel() ) {
				src.Warning( "Model '%s' defaulted", filename.c_str() );
				return false;
			}

			// get the number of joints
			num = modelHandle->NumJoints();
			if ( !num ) {
				src.Warning( "Model '%s' has no joints", filename.c_str() );
			}

			// set up the joint hierarchy
			joints.SetGranularity( 1 );
			joints.SetNum( num );
			jointParents.SetNum( num );
			channelJoints[0].SetNum( num );
			md5joints = modelHandle->GetJoints();
			md5joint = md5joints;
			for( i = 0; i < num; i++, md5joint++ ) {
				joints[i].channel = ANIMCHANNEL_ALL;
				joints[i].num = static_cast<jointHandle_t>( i );
				if ( md5joint->parent ) {
					joints[i].parentNum = static_cast<jointHandle_t>( md5joint->parent - md5joints );
				} else {
					joints[i].parentNum = INVALID_JOINT;
				}
				jointParents[i] = joints[i].parentNum;
				channelJoints[0][i] = i;
			}
		} else if ( token == "remove" ) {
			// removes any anims whose name matches
			if( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				return false;
			}
			num = 0;
			AnimTable::Iterator iter = anims.Begin();
			for( iter; iter != anims.End(); ++iter ) {
				if ( ( token2 == iter->second->Name() ) || ( token2 == iter->second->FullName() ) ) {
					if ( iter->second->IsBaseModelDef( this ) ) {
						src.Warning( "Anim '%s' was not inherited.  Anim should be removed from the model def.", token2.c_str() );
						return false;
					}

					delete iter->second;
					anims.Remove( iter );

					--iter;
					num++;
					continue;
				}
			}
			if ( !num ) {
				src.Warning( "Couldn't find anim '%s' to remove", token2.c_str() );
				return false;
			}
		} else if ( !token.Icmp( "animGroup" ) ) {
			if ( !modelHandle ) {
				src.Warning( "Must specify mesh before defining anims" );
				return false;
			}

			idStrList& localList = animGroups.Alloc();

			while ( true ) {
				src.ReadToken( &token );

				if ( token == "{" ) {
					break;
				}

				localList.Alloc() = token;
			}

			if ( localList.Num() == 0 ) {
				src.Error( "Anim group with no names" );
			}

			while ( true ) {
				if ( !src.ReadToken( &token ) ) {
					src.Warning( "Unexpected end of file in animGroup" );
					return false;
				}

				if ( !token.Icmp( "animGroup" ) ) {
					idStrList& localList = animGroups.Alloc();

					while ( true ) {
						src.ReadToken( &token );
						if ( token == "{" ) {
							break;
						}

						localList.Alloc() = token;
					}

					if ( localList.Num() == 0 ) {
						src.Error( "Anim group with no names" );
					}
					continue;
				}

				if ( !token.Cmp( "}" ) ) {
					animGroups.SetNum( animGroups.Num() - 1 );
					if ( animGroups.Num() == 0 ) {
						break;
					}
					continue;
				}

				if ( !token.Icmp( "anim" ) ) {
					if ( !ParseAnim( src, animGroups ) ) {
						return false;
					}
					continue;
				}
			}
		} else if ( token == "anim" ) {
			if ( !modelHandle ) {
				src.Warning( "Must specify mesh before defining anims" );
				return false;
			}
			if ( !ParseAnim( src, animGroups ) ) {
				return false;
			}
		} else if ( token == "offset" ) {
			if ( !src.Parse1DMatrix( 3, offset.ToFloatPtr() ) ) {
				src.Warning( "Expected vector following 'offset'" );
				return false;
			}
		} else if ( token == "channel" ) {
			if ( !modelHandle ) {
				src.Warning( "Must specify mesh before defining channels" );
				return false;
			}

			// set the channel for a group of joints
			if( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				return false;
			}
			if ( !src.CheckTokenString( "(" ) ) {
				src.Warning( "Expected { after '%s'", token2.c_str() );
				return false;
			}

			for( i = ANIMCHANNEL_ALL + 1; i < ANIM_NumAnimChannels; i++ ) {
				if ( !idStr::Icmp( channelNames[ i ], token2 ) ) {
					break;
				}
			}

			if ( i >= ANIM_NumAnimChannels ) {
				src.Warning( "Unknown channel '%s'", token2.c_str() );
				return false;
			}

			channel = ( animChannel_t )i;
			jointnames = "";

			while( !src.CheckTokenString( ")" ) ) {
				if( !src.ReadToken( &token2 ) ) {
					src.Warning( "Unexpected end of file" );
					return false;
				}
				jointnames += token2;
				if ( ( token2 != "*" ) && ( token2 != "-" ) ) {
					jointnames += " ";
				}
			}

			GetJointList( jointnames, jointList );

			channelJoints[ channel ].SetNum( jointList.Num() );
			for( num = i = 0; i < jointList.Num(); i++ ) {
				jointnum = jointList[ i ];
				if ( joints[ jointnum ].channel != ANIMCHANNEL_ALL ) {
					src.Warning( "Joint '%s' assigned to multiple channels", modelHandle->GetJointName( jointnum ) );
					continue;
				}
				joints[ jointnum ].channel = channel;
				channelJoints[ channel ][ num++ ] = jointnum;
			}
			channelJoints[ channel ].SetNum( num );
		} else {
			src.Warning( "unknown token '%s'", token.c_str() );
			return false;
		}
	}

	numChannels = 0;
	for ( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		if ( NumJointsOnChannel( ( animChannel_t )i ) > 0 ) {
			numChannels = i + 1;
		}
	}

	int uniqueCount = 0;
	idHashIndex uniqueCountHash;

	for ( int i = 0; i < anims.Num(); i++ ) {
		const char* name = anims.FindIndex( i )->second->Name();

		int key = uniqueCountHash.GenerateKey( name );

		int index;
		for ( index = uniqueCountHash.GetFirst( key ); index != idHashIndex::NULL_INDEX; index = uniqueCountHash.GetNext( index ) ) {
			if ( idStr::Cmp( anims.FindIndex( index )->second->Name(), name ) == 0 ) {
				break;
			}
		}

		if ( index != idHashIndex::NULL_INDEX ) {
			continue;
		}

		uniqueCountHash.Add( key, i );
		uniqueCount++;
	}

	if ( uniqueCount > 0 ) {
		animLookup.SetGranularity( uniqueCount );
	}

	int checkCount = 0;

	for ( int i = 0; i < anims.Num(); i++ ) {
		const char* name = anims.FindIndex( i )->second->Name();

		AnimNumTable::Iterator lookupIter = animLookup.Find( name );
		if ( lookupIter != animLookup.End() ) {
			continue;
		}

		checkCount++;

		AnimNumTable::InsertResult result = animLookup.Set( name, idListGranularityOne< int >() );

		int count = 0;
		for ( int j = i; j < anims.Num(); j++ ) {
			if ( idStr::Icmp( anims.FindIndex( j )->second->Name(), name ) != 0 ) {
				continue;
			}

			count++;
		}

		result.first->second.SetNum( count );
		int k = 0;
		for ( int j = i; j < anims.Num(); j++ ) {
			if ( idStr::Icmp( anims.FindIndex( j )->second->Name(), name ) != 0 ) {
				continue;
			}

			result.first->second[ k++ ] = j;
			if ( k == count ) {
				break;
			}
		}
	}

	assert( checkCount == uniqueCount );

	c_modelDefMemory += Size();

	return true;
}

/*
=====================
idDeclModelDef::HasAnim
=====================
*/
bool idDeclModelDef::HasAnim( const char *name ) const {
	return anims.Find( name ) != anims.End();
}

/*
=====================
idDeclModelDef::NumAnims
=====================
*/
int idDeclModelDef::NumAnims( void ) const {
	return anims.Num() + 1;
}

/*
=====================
idDeclModelDef::GetSpecificAnim

Gets the exact anim for the name, without randomization.
=====================
*/
int idDeclModelDef::GetSpecificAnim( const char *name ) const {
	AnimTable::ConstIterator iter = anims.Find( name );
	if( iter == anims.End() ) {
		return 0;
	}
	return iter - anims.Begin() + 1;
}

/*
=====================
idDeclModelDef::GetAnim
=====================
*/
const idAnim *idDeclModelDef::GetAnim( int index ) const {
	if ( ( index < 1 ) || ( index > anims.Num() ) ) {
		return NULL;
	}
	
	return anims.FindIndex( index - 1 )->second;
}

/*
=====================
idDeclModelDef::GetAnim
=====================
*/
int idDeclModelDef::GetAnim( const char *name ) const {
	int len = idStr::Length( name );
	if ( len && idStr::CharIsNumeric( name[ len - 1 ] ) ) {
		// find a specific animation
		return GetSpecificAnim( name );
	}

	AnimNumTable::ConstIterator lookupIter = animLookup.Find( name );
	if ( lookupIter == animLookup.End()) {
		return 0;
	}

	// get a random anim
	//FIXME: don't access gameLocal here?
	return lookupIter->second[ gameLocal.random.RandomInt( lookupIter->second.Num() ) ] + 1;
}

/*
=====================
idDeclModelDef::GetSkin
=====================
*/
const idDeclSkin *idDeclModelDef::GetSkin( void ) const {
	return skin;
}

/*
=====================
idDeclModelDef::GetModelName
=====================
*/
const char *idDeclModelDef::GetModelName( void ) const {
	if ( modelHandle ) {
		return modelHandle->Name();
	} else {
		return "";
	}
}

/*
=====================
idDeclModelDef::Joints
=====================
*/
const idList<jointInfo_t> &idDeclModelDef::Joints( void ) const {
	return joints;
}

/*
=====================
idDeclModelDef::JointParents
=====================
*/
const int * idDeclModelDef::JointParents( void ) const {
	return jointParents.Begin();
}

/*
=====================
idDeclModelDef::NumJoints
=====================
*/
int idDeclModelDef::NumJoints( void ) const {
	return joints.Num();
}

/*
=====================
idDeclModelDef::GetJoint
=====================
*/
const jointInfo_t *idDeclModelDef::GetJoint( int jointHandle ) const {
	if ( ( jointHandle < 0 ) || ( jointHandle > joints.Num() ) ) {
		gameLocal.Error( "idDeclModelDef::GetJoint : joint handle out of range" );
	}
	return &joints[ jointHandle ];
}

/*
====================
idDeclModelDef::GetJointName
====================
*/
const char *idDeclModelDef::GetJointName( int jointHandle ) const {
	const idMD5Joint *joint;

	if ( !modelHandle ) {
		return NULL;
	}
	
	if ( ( jointHandle < 0 ) || ( jointHandle > joints.Num() ) ) {
		gameLocal.Error( "idDeclModelDef::GetJointName : joint handle out of range" );
	}

	joint = modelHandle->GetJoints();
	return joint[ jointHandle ].name.c_str();
}

/*
=====================
idDeclModelDef::CacheFromDict
=====================
*/
void idDeclModelDef::CacheFromDict( const idDict& dict ) {
	const idKeyValue* kv = NULL;
	while ( kv = dict.MatchPrefix( "model", kv ) ) {
		if ( kv->GetValue().Length() ) {
			declManager->MediaPrint( "Precaching model %s\n", kv->GetValue().c_str() );
			// precache the render model
			renderModelManager->CheckModel( kv->GetValue() );
			// precache animations
			gameLocal.declModelDefType[ kv->GetValue() ];
		}
	}

	while ( kv = dict.MatchPrefix( "cm", kv ) ) { // Gordon: meh, this shouldn't be here, but nowhere else nice to put it right now
		gameLocal.clip.PrecacheModel( kv->GetValue() );
	}
}

/***********************************************************************

	idAnimator

***********************************************************************/

/*
=====================
idAnimator::idAnimator
=====================
*/
idAnimator::idAnimator() {
	int	i, j;

	modelDef				= NULL;
	entity					= NULL;
	numJoints				= 0;
	joints					= NULL;
	lastTransformTime		= -1;
	transformCount			= 0;
	stoppedAnimatingUpdate	= false;
	removeOriginOffset		= false;
	forceUpdate				= false;
	numAFPoseJointFrame		= 0;
	AFPoseJointFrame		= NULL;

	frameBounds.Clear();

	AFPoseJoints.SetGranularity( 1 );
	AFPoseJointMods.SetGranularity( 1 );

	ClearAFPose();

	for( i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++ ) {
			channels[ i ][ j ].Reset( NULL );
		}
	}
}

/*
=====================
idAnimator::~idAnimator
=====================
*/
idAnimator::~idAnimator() {
	FreeData();
}

/*
=====================
idAnimator::Allocated
=====================
*/
size_t idAnimator::Allocated( void ) const {
	size_t	size;

	size = jointMods.Allocated() + numJoints * sizeof( joints[0] ) + jointMods.Num() * sizeof( jointMods[ 0 ] ) + AFPoseJointMods.Allocated() + numAFPoseJointFrame * sizeof( AFPoseJointFrame[0] ) + AFPoseJoints.Allocated();

	return size;
}

/*
=====================
idAnimator::FreeData
=====================
*/
void idAnimator::FreeData( void ) {
	int	i, j;

	if ( entity ) {
		entity->BecomeInactive( TH_ANIMATE );
	}

	for( i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++ ) {
			channels[ i ][ j ].Reset( NULL );
		}
	}

	jointMods.DeleteContents( true );

	Mem_FreeAligned( joints );
	joints = NULL;
	numJoints = 0;

	Mem_FreeAligned( AFPoseJointFrame );
	AFPoseJointFrame = NULL;
	numAFPoseJointFrame = 0;

	modelDef = NULL;

	ForceUpdate();
}

/*
=====================
idAnimator::PushAnims
=====================
*/
void idAnimator::PushAnims( animChannel_t channelNum, int currentTime, int blendTime ) {
	int			i;
	idAnimBlend *channel;

	channel = channels[ channelNum ];
	if ( channel[ 0 ].starttime < gameLocal.time ) {
		if ( !channel[ 0 ].GetWeight( currentTime ) || ( channel[ 0 ].starttime == currentTime ) ) {
			return;
		}
	}

#if 0
	if ( channel[ ANIM_MaxAnimsPerChannel - 1 ].GetWeight( currentTime ) > 0.f ) {
		gameLocal.Warning( "idAnimator::PushAnims Anim Channels Overflowed" );
	}
#endif // 0

	for( i = ANIM_MaxAnimsPerChannel - 1; i > 0; i-- ) {
		channel[ i ] = channel[ i - 1 ];
	}

	channel[ 0 ].Reset( modelDef );
	channel[ 1 ].Clear( entity, currentTime, blendTime );
	ForceUpdate();
}

/*
=====================
idAnimator::SetModel
=====================
*/
idRenderModel *idAnimator::SetModel( const char *modelname ) {
	int i, j;

	FreeData();

	// check if we're just clearing the model
	if ( !modelname || !*modelname ) {
		return NULL;
	}

	modelDef = gameLocal.declModelDefType.LocalFind( modelname, false );
	if ( !modelDef ) {
		return NULL;
	}
	
	idRenderModel *renderModel = modelDef->ModelHandle();
	if ( !renderModel ) {
		modelDef = NULL;
		return NULL;
	}

	// make sure model hasn't been purged
	modelDef->Touch();

	modelDef->SetupJoints( &numJoints, &joints, frameBounds, removeOriginOffset );
	modelDef->ModelHandle()->Reset();

	// set the modelDef on all channels
	for( i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++ ) {
			channels[ i ][ j ].Reset( modelDef );
		}
	}

	return modelDef->ModelHandle();
}

/*
=====================
idAnimator::Size
=====================
*/
size_t idAnimator::Size( void ) const {
	return sizeof( *this ) + Allocated();
}

/*
=====================
idAnimator::SetEntity
=====================
*/
void idAnimator::SetEntity( idClass *ent ) {
	entity = ent;
}

/*
=====================
idAnimator::GetEntity
=====================
*/
idEntity *idAnimator::GetEntity( void ) const {
	return entity->Cast< idEntity >();
}

/*
=====================
idAnimator::RemoveOriginOffset
=====================
*/
void idAnimator::RemoveOriginOffset( bool remove ) {
	removeOriginOffset = remove;
}

/*
=====================
idAnimator::RemoveOrigin
=====================
*/
bool idAnimator::RemoveOrigin( void ) const {
	return removeOriginOffset;
}

/*
=====================
idAnimator::GetJointList
=====================
*/
void idAnimator::GetJointList( const char *jointnames, idList<jointHandle_t> &jointList ) const {
	if ( modelDef ) {
		modelDef->GetJointList( jointnames, jointList );
	}
}

/*
=====================
idAnimator::NumAnims
=====================
*/
int	idAnimator::NumAnims( void ) const {
	if ( !modelDef ) {
		return 0;
	}
	
	return modelDef->NumAnims();
}

/*
=====================
idAnimator::GetAnim
=====================
*/
const idAnim *idAnimator::GetAnim( int index ) const {
	if ( !modelDef ) {
		return NULL;
	}
	
	return modelDef->GetAnim( index );
}

/*
=====================
idAnimator::GetAnim
=====================
*/
int idAnimator::GetAnim( const char *name ) const {
	if ( !modelDef ) {
		return 0;
	}
	
	return modelDef->GetAnim( name );
}

/*
=====================
idAnimator::HasAnim
=====================
*/
bool idAnimator::HasAnim( const char *name ) const {
	if ( !modelDef ) {
		return false;
	}
	
	return modelDef->HasAnim( name );
}

/*
=====================
idAnimator::NumJoints
=====================
*/
int	idAnimator::NumJoints( void ) const {
	return numJoints;
}

/*
=====================
idAnimator::ModelHandle
=====================
*/
idRenderModel *idAnimator::ModelHandle( void ) const {
	if ( !modelDef ) {
		return NULL;
	}
	
	return modelDef->ModelHandle();
}

/*
=====================
idAnimator::ModelDef
=====================
*/
const idDeclModelDef *idAnimator::ModelDef( void ) const {
	return modelDef;
}

/*
=====================
idAnimator::CurrentAnim
=====================
*/
idAnimBlend *idAnimator::CurrentAnim( animChannel_t channelNum ) {
	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "idAnimator::CurrentAnim : channel out of range" );
	}

	return &channels[ channelNum ][ 0 ];
}

/*
=====================
idAnimator::Clear
=====================
*/
void idAnimator::Clear( animChannel_t channelNum, int currentTime, int cleartime ) {
	int			i;
	idAnimBlend	*blend;

	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "idAnimator::Clear : channel out of range" );
	}

	blend = channels[ channelNum ];
	for( i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
		blend->Clear( entity, currentTime, cleartime );
	}
	ForceUpdate();
}

/*
=====================
idAnimator::SetFrame
=====================
*/
void idAnimator::SetFrame( animChannel_t channelNum, int animNum, float frame, int currentTime, int blendTime ) {
	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "idAnimator::SetFrame : channel out of range" );
	}

	if ( !modelDef || !modelDef->GetAnim( animNum ) ) {
		return;
	}
	
	PushAnims( channelNum, currentTime, 0 );
	channels[ channelNum ][ 0 ].SetFrame( entity, modelDef, animNum, frame, currentTime, blendTime );
	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
}

/*
=====================
idAnimator::CycleAnim
=====================
*/
void idAnimator::CycleAnim( animChannel_t channelNum, int animNum, int currentTime, int blendTime ) {
	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "idAnimator::CycleAnim : channel out of range" );
	}

	if ( !modelDef || !modelDef->GetAnim( animNum ) ) {
		return;
	}
	
	PushAnims( channelNum, currentTime, blendTime );
	channels[ channelNum ][ 0 ].CycleAnim( entity, modelDef, animNum, currentTime, blendTime );
	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
}

/*
=====================
idAnimator::PlayAnim
=====================
*/
void idAnimator::PlayAnim( animChannel_t channelNum, int animNum, int currentTime, int blendTime ) {
	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "idAnimator::PlayAnim : channel out of range" );
	}

	if ( !modelDef || !modelDef->GetAnim( animNum ) ) {
		return;
	}
	
	PushAnims( channelNum, currentTime, blendTime );
	channels[ channelNum ][ 0 ].PlayAnim( entity, modelDef, animNum, currentTime, blendTime );
	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
}

/*
=====================
idAnimator::SyncAnimChannels
=====================
*/
void idAnimator::SyncAnimChannels( animChannel_t channelNum, animChannel_t fromChannelNum, int currentTime, int blendTime ) {
	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) || ( fromChannelNum < 0 ) || ( fromChannelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "idAnimator::SyncToChannel : channel out of range" );
	}

	idAnimBlend &fromBlend = channels[ fromChannelNum ][ 0 ];
	idAnimBlend &toBlend = channels[ channelNum ][ 0 ];

	float weight = fromBlend.blendEndValue;
	if ( ( fromBlend.Anim() != toBlend.Anim() ) || ( fromBlend.GetStartTime() != toBlend.GetStartTime() ) || ( fromBlend.GetEndTime() != toBlend.GetEndTime() ) ) {
		PushAnims( channelNum, currentTime, blendTime );
		toBlend = fromBlend;
		toBlend.blendStartValue = 0.0f;
		toBlend.blendEndValue = 0.0f;
	}
    toBlend.SetWeight( weight, currentTime - 1, blendTime );

	// disable framecommands on the current channel so that commands aren't called twice
	toBlend.AllowFrameCommands( false );

	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
}

/*
=====================
idAnimator::SetJointPos
=====================
*/
void idAnimator::SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3 &pos ) {
	int i;
	jointMod_t *jointMod;

	if ( !modelDef || !modelDef->ModelHandle() || ( jointnum < 0 ) || ( jointnum >= numJoints ) ) {
		return;
	}

	jointMod = NULL;
	for( i = 0; i < jointMods.Num(); i++ ) {
		if ( jointMods[ i ]->jointnum == jointnum ) {
			jointMod = jointMods[ i ];
			break;
		} else if ( jointMods[ i ]->jointnum > jointnum ) {
			break;
		}
	}

	if ( !jointMod ) {
		jointMod = new jointMod_t;
		jointMod->jointnum = jointnum;
		jointMod->mat.Identity();
		jointMod->transform_axis = JOINTMOD_NONE;
		jointMods.Insert( jointMod, i );
	}

	jointMod->pos = pos;
	jointMod->transform_pos = transform_type;

	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
	ForceUpdate();
}

/*
=====================
idAnimator::SetJointAxis
=====================
*/
void idAnimator::SetJointAxis( jointHandle_t jointnum, jointModTransform_t transform_type, const idMat3 &mat ) {
	int i;
	jointMod_t *jointMod;

	if ( !modelDef || !modelDef->ModelHandle() || ( jointnum < 0 ) || ( jointnum >= numJoints ) ) {
		return;
	}

	jointMod = NULL;
	for( i = 0; i < jointMods.Num(); i++ ) {
		if ( jointMods[ i ]->jointnum == jointnum ) {
			jointMod = jointMods[ i ];
			break;
		} else if ( jointMods[ i ]->jointnum > jointnum ) {
			break;
		}
	}

	if ( !jointMod ) {
		jointMod = new jointMod_t;
		jointMod->jointnum = jointnum;
		jointMod->pos.Zero();
		jointMod->transform_pos = JOINTMOD_NONE;
		jointMods.Insert( jointMod, i );
	}

	jointMod->mat = mat;
	jointMod->transform_axis = transform_type;

	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
	ForceUpdate();
}

/*
=====================
idAnimator::ClearJoint
=====================
*/
void idAnimator::ClearJoint( jointHandle_t jointnum ) {
	int i;

	if ( !modelDef || !modelDef->ModelHandle() || ( jointnum < 0 ) || ( jointnum >= numJoints ) ) {
		return;
	}

	for( i = 0; i < jointMods.Num(); i++ ) {
		if ( jointMods[ i ]->jointnum == jointnum ) {
			delete jointMods[ i ];
			jointMods.RemoveIndex( i );
			ForceUpdate();
			break;
		} else if ( jointMods[ i ]->jointnum > jointnum ) {
			break;
		}
	}
}

/*
=====================
idAnimator::ClearAllJoints
=====================
*/
void idAnimator::ClearAllJoints( void ) {
	if ( jointMods.Num() ) {
		ForceUpdate();
	}
	jointMods.DeleteContents( true );
}

/*
=====================
idAnimator::ClearAllAnims
=====================
*/
void idAnimator::ClearAllAnims( int currentTime, int cleartime ) {
	int	i;

	for( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		Clear( ( animChannel_t )i, currentTime, cleartime );
	}

	ClearAFPose();
	ForceUpdate();
}

/*
====================
idAnimator::GetDelta
====================
*/
void idAnimator::GetDelta( int fromtime, int totime, idVec3 &delta, int maxChannels ) const {
	assert( maxChannels <= ANIM_MaxAnimsPerChannel );

	delta.Zero();

	if ( !modelDef || !modelDef->ModelHandle() || ( fromtime == totime ) ) {
		return;
	}

	float blendWeight = 0.0f;

	const idAnimBlend* blend = channels[ ANIMCHANNEL_ALL ];
	for( int i = 0; i < maxChannels; i++, blend++ ) {
		blend->BlendDelta( fromtime, totime, delta, blendWeight );
	}

	if ( modelDef->Joints()[ 0 ].channel ) {
		blend = channels[ modelDef->Joints()[ 0 ].channel ];
		for( int i = 0; i < maxChannels; i++, blend++ ) {
			blend->BlendDelta( fromtime, totime, delta, blendWeight );
		}
	}
}

/*
====================
idAnimator::GetDeltaRotation
====================
*/
bool idAnimator::GetDeltaRotation( int fromtime, int totime, idMat3 &delta ) const {
	int					i;
	const idAnimBlend	*blend;
	float				blendWeight;
	idQuat				q;

	if ( !modelDef || !modelDef->ModelHandle() || ( fromtime == totime ) ) {
		delta.Identity();
		return false;
	}

	q.Set( 0.0f, 0.0f, 0.0f, 1.0f );
	blendWeight = 0.0f;

	blend = channels[ ANIMCHANNEL_ALL ];
	for( i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
		blend->BlendDeltaRotation( fromtime, totime, q, blendWeight );
	}

	if ( modelDef->Joints()[ 0 ].channel ) { // Gordon: WTF is this about?
		blend = channels[ modelDef->Joints()[ 0 ].channel ];
		for( i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
			blend->BlendDeltaRotation( fromtime, totime, q, blendWeight );
		}
	}

	if ( blendWeight > 0.0f ) {
		delta = q.ToMat3();
		return true;
	} else {
		delta.Identity();
		return false;
	}
}

/*
====================
idAnimator::GetOrigin
====================
*/
void idAnimator::GetOrigin( int currentTime, idVec3 &pos ) const {
	int					i;
	const idAnimBlend	*blend;
	float				blendWeight;

	if ( !modelDef || !modelDef->ModelHandle() ) {
		pos.Zero();
		return;
	}

	pos.Zero();
	blendWeight = 0.0f;

	blend = channels[ ANIMCHANNEL_ALL ];
	for( i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
		blend->BlendOrigin( currentTime, pos, blendWeight, removeOriginOffset );
	}

	if ( modelDef->Joints()[ 0 ].channel ) {
		blend = channels[ modelDef->Joints()[ 0 ].channel ];
		for( i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
			blend->BlendOrigin( currentTime, pos, blendWeight, removeOriginOffset );
		}
	}

	pos += modelDef->GetVisualOffset();
}

/*
====================
idAnimator::GetBounds
====================
*/
bool idAnimator::GetBounds( int currentTime, idBounds &bounds, bool force ) {
	int					i, j;
	const idAnimBlend	*blend;
	int					count;

	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	if ( AFPoseJoints.Num() ) {
		bounds = AFPoseBounds;
		count = 1;
	} else {
		bounds.Clear();
		count = 0;
	}

	blend = channels[ 0 ];
	for( i = ANIMCHANNEL_ALL; i < modelDef->NumChannels(); i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
			if ( blend->AddBounds( currentTime, bounds, removeOriginOffset, force ) ) {
				count++;
			}
		}
	}

	if ( !count ) {
		if ( !frameBounds.IsCleared() ) {
			bounds = frameBounds;
			return true;
		} else {
			bounds.Zero();
			return false;
		}
	}

	bounds.TranslateSelf( modelDef->GetVisualOffset() );

	if ( g_debugBounds.GetBool() ) {
		if ( bounds[1][0] - bounds[0][0] > 2048 || bounds[1][1] - bounds[0][1] > 2048 ) {
			idEntity* ent = entity->Cast< idEntity >();
			if ( ent ) {
				gameLocal.Warning( "big frameBounds on entity '%s' with model '%s': %f,%f", ent->name.c_str(), modelDef->ModelHandle()->Name(), bounds[1][0] - bounds[0][0], bounds[1][1] - bounds[0][1] );
			} else {
				gameLocal.Warning( "big frameBounds on model '%s': %f,%f", modelDef->ModelHandle()->Name(), bounds[1][0] - bounds[0][0], bounds[1][1] - bounds[0][1] );
			}
		}
	}

	frameBounds = bounds;

	return true;
}

/*
=====================
idAnimator::GetMeshBounds
=====================
*/
bool idAnimator::GetMeshBounds( jointHandle_t jointnum, int meshHandle, int currentTime, idBounds& bounds, bool useDefaultAnim ) {

	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	idRenderModel* model = modelDef->ModelHandle();
	if ( meshHandle < 0 || meshHandle >= model->NumMeshes() ) {
		return false;
	}

	// This will update the skel if needed
	idMat3 axis;
	idVec3 offset;
	GetJointTransform( jointnum, currentTime, offset, axis );

	// Now calculate the bounds
	bounds = model->CalcMeshBounds( meshHandle, joints, offset, axis, useDefaultAnim );
	return true;
}

/*
=====================
idAnimator::InitAFPose
=====================
*/
void idAnimator::InitAFPose( void ) {

	if ( !modelDef ) {
		return;
	}

	AFPoseJoints.SetNum( modelDef->Joints().Num(), false );
	AFPoseJoints.SetNum( 0, false );
	AFPoseJointMods.SetNum( modelDef->Joints().Num(), false );

	if ( numAFPoseJointFrame != modelDef->Joints().Num() ) {
		Mem_FreeAligned( AFPoseJointFrame );
		numAFPoseJointFrame = modelDef->Joints().Num();
		AFPoseJointFrame = (idJointQuat *) Mem_AllocAligned( numAFPoseJointFrame * sizeof( AFPoseJointFrame[0] ), ALIGN_16 );
	}
}

/*
=====================
idAnimator::SetAFPoseJointMod
=====================
*/
void idAnimator::SetAFPoseJointMod( const jointHandle_t jointNum, const AFJointModType_t mod, const idMat3 &axis, const idVec3 &origin ) {
	AFPoseJointMods[jointNum].mod = mod;
	AFPoseJointMods[jointNum].axis = axis;
	AFPoseJointMods[jointNum].origin = origin;

	int index = idBinSearch_GreaterEqual<int>( AFPoseJoints.Begin(), AFPoseJoints.Num(), jointNum );
	if ( index >= AFPoseJoints.Num() || jointNum != AFPoseJoints[index] ) {
		AFPoseJoints.Insert( jointNum, index );
	}
}

/*
=====================
idAnimator::FinishAFPose
=====================
*/
void idAnimator::FinishAFPose( int animNum, const idBounds &bounds, const int time ) {
	int					i, j;
	int					numJoints;
	int					parentNum;
	int					jointMod;
	int					jointNum;
	const int *			jointParent;

	if ( !modelDef ) {
		return;
	}
	
	const idAnim *anim = modelDef->GetAnim( animNum );
	if ( !anim ) {
		return;
	}

	numJoints = modelDef->Joints().Num();
	if ( !numJoints ) {
		return;
	}

	idRenderModel		*md5 = modelDef->ModelHandle();
	const idMD5Anim		*md5anim = anim->MD5Anim( 0 );

	if ( numJoints != md5anim->NumJoints() ) {
		gameLocal.Warning( "Model '%s' has different # of joints than anim '%s'", md5->Name(), md5anim->Name() );
		return;
	}

	idJointQuat *jointFrame = ( idJointQuat * )_alloca16( numJoints * sizeof( *jointFrame ) );
	md5anim->GetSingleFrame( 0, jointFrame, modelDef->GetChannelJoints( ANIMCHANNEL_ALL ), modelDef->NumJointsOnChannel( ANIMCHANNEL_ALL ) );

	if ( removeOriginOffset ) {
#ifdef VELOCITY_MOVE
		jointFrame[ 0 ].t.x = 0.0f;
#else
		jointFrame[ 0 ].t.Zero();
#endif
	}

	idJointMat *joints = ( idJointMat * )_alloca16( numJoints * sizeof( *joints ) );

	// convert the joint quaternions to joint matrices
	SIMDProcessor->ConvertJointQuatsToJointMats( joints, jointFrame, numJoints );

	// first joint is always root of entire hierarchy
	if ( AFPoseJoints.Num() && AFPoseJoints[0] == 0 ) {
		switch( AFPoseJointMods[0].mod ) {
			case AF_JOINTMOD_AXIS: {
				joints[0].SetRotation( AFPoseJointMods[0].axis );
				break;
			}
			case AF_JOINTMOD_ORIGIN: {
				joints[0].SetTranslation( AFPoseJointMods[0].origin );
				break;
			}
			case AF_JOINTMOD_BOTH: {
				joints[0].SetRotation( AFPoseJointMods[0].axis );
				joints[0].SetTranslation( AFPoseJointMods[0].origin );
				break;
			}
		}
		j = 1;
	} else {
		j = 0;
	}

	// pointer to joint info
	jointParent = modelDef->JointParents();

	// transform the child joints
	for( i = 1; j < AFPoseJoints.Num(); j++, i++ ) {
		jointMod = AFPoseJoints[j];

		// transform any joints preceding the joint modifier
		SIMDProcessor->TransformJoints( joints, jointParent, i, jointMod - 1 );
		i = jointMod;

		parentNum = jointParent[i];

		switch( AFPoseJointMods[jointMod].mod ) {
			case AF_JOINTMOD_AXIS: {
				joints[i].SetRotation( AFPoseJointMods[jointMod].axis );
				joints[i].SetTranslation( joints[parentNum].ToVec3() + joints[i].ToVec3() * joints[parentNum].ToMat3() );
				break;
			}
			case AF_JOINTMOD_ORIGIN: {
				joints[i].SetRotation( joints[i].ToMat3() * joints[parentNum].ToMat3() );
				joints[i].SetTranslation( AFPoseJointMods[jointMod].origin );
				break;
			}
			case AF_JOINTMOD_BOTH: {
				joints[i].SetRotation( AFPoseJointMods[jointMod].axis );
				joints[i].SetTranslation( AFPoseJointMods[jointMod].origin );
				break;
			}
		}
	}

	// transform the rest of the hierarchy
	SIMDProcessor->TransformJoints( joints, jointParent, i, numJoints - 1 );

	// untransform hierarchy
	SIMDProcessor->UntransformJoints( joints, jointParent, 1, numJoints - 1 );

	// convert joint matrices back to joint quaternions
	SIMDProcessor->ConvertJointMatsToJointQuats( AFPoseJointFrame, joints, numJoints );

	// find all modified joints and their parents
	bool *blendJoints = (bool *) _alloca16( numJoints * sizeof( bool ) );
	memset( blendJoints, 0, numJoints * sizeof( bool ) );

	// mark all modified joints and their parents
	for( i = 0; i < AFPoseJoints.Num(); i++ ) {
		for( jointNum = AFPoseJoints[i]; jointNum != INVALID_JOINT; jointNum = jointParent[jointNum] ) {
			blendJoints[jointNum] = true;
		}
	}

	// lock all parents of modified joints
	AFPoseJoints.SetNum( 0, false );
	for ( i = 0; i < numJoints; i++ ) {
		if ( blendJoints[i] ) {
			AFPoseJoints.Append( i );
		}
	}

	AFPoseBounds = bounds;
	AFPoseTime = time;

	ForceUpdate();
}

/*
=====================
idAnimator::SetAFPoseBlendWeight
=====================
*/
void idAnimator::SetAFPoseBlendWeight( float blendWeight ) {
	AFPoseBlendWeight = blendWeight;
}

/*
=====================
idAnimator::BlendAFPose
=====================
*/
bool idAnimator::BlendAFPose( idJointQuat *blendFrame ) const {

	if ( !AFPoseJoints.Num() ) {
		return false;
	}

	SIMDProcessor->BlendJoints( blendFrame, AFPoseJointFrame, AFPoseBlendWeight, AFPoseJoints.Begin(), AFPoseJoints.Num() );

	return true;
}

/*
=====================
idAnimator::ClearAFPose
=====================
*/
void idAnimator::ClearAFPose( void ) {
	if ( AFPoseJoints.Num() ) {
		ForceUpdate();
	}
	AFPoseBlendWeight = 1.0f;
	AFPoseJoints.SetNum( 0, false );
	AFPoseBounds.Clear();
	AFPoseTime = 0;
}

/*
=====================
idAnimator::ServiceAnims
=====================
*/
void idAnimator::ServiceAnims( int fromtime, int totime ) {
	int			i, j;
	idAnimBlend	*blend;

	if ( !modelDef ) {
		return;
	}

	bool isAnimating = IsAnimating( totime );

	if ( modelDef->ModelHandle() ) {
		blend = channels[ 0 ];
		for( i = 0; i < modelDef->NumChannels(); i++ ) {
			for( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
				blend->CallFrameCommands( entity, fromtime, totime );
			}
		}
	}

	if ( !isAnimating ) {
		stoppedAnimatingUpdate = true;
		if ( entity ) {
			entity->BecomeInactive( TH_ANIMATE );

			// present one more time with stopped animations so the renderer can properly recreate interactions
			entity->BecomeActive( TH_UPDATEVISUALS );
		}
	}
}

/*
=====================
idAnimator::IsCyclingAnim
=====================
*/
bool idAnimator::IsCyclingAnim( animChannel_t channel, int animNum, int currentTime ) const {
	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	for( int j = 0; j < ANIM_MaxAnimsPerChannel; j++ ) {
		const idAnimBlend& blend = channels[ channel ][ j ];
		if ( blend.GetEndTime() > 0 || blend.IsDone( currentTime ) || blend.AnimNum() != animNum ) {
			continue;
		}

		return true;
	}

	return false;
}

/*
=====================
idAnimator::IsPlayingAnimPrimary
=====================
*/
bool idAnimator::IsPlayingAnimPrimary( animChannel_t channel, int animNum, int currentTime ) const {
	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	const idAnimBlend& blend = channels[ channel ][ 0 ];
	if ( blend.IsDone( currentTime ) || blend.AnimNum() != animNum ) {
		return false;
	}

	return true;
}

/*
=====================
idAnimator::IsPlayingAnim
=====================
*/
bool idAnimator::IsPlayingAnim( animChannel_t channel, int animNum, int currentTime ) const {
	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	for( int j = 0; j < ANIM_MaxAnimsPerChannel; j++ ) {
		const idAnimBlend& blend = channels[ channel ][ j ];
		if ( blend.IsDone( currentTime ) || blend.AnimNum() != animNum ) {
			continue;
		}

		return true;
	}

	return false;
}

/*
=====================
idAnimator::IsAnimating
=====================
*/
bool idAnimator::IsAnimating( int currentTime ) const {
	int					i, j;
	const idAnimBlend	*blend;

	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	// if animating with an articulated figure
	if ( AFPoseJoints.Num() && currentTime <= AFPoseTime ) {
		return true;
	}

	blend = channels[ 0 ];
	for( i = 0; i < modelDef->NumChannels(); i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
			if ( !blend->IsDone( currentTime ) ) {
				return true;
			}
		}
	}

	return false;
}

/*
=====================
idAnimator::IsAnimatingOnChannel
=====================
*/
bool idAnimator::IsAnimatingOnChannel( animChannel_t channel, int currentTime ) const {
	int					j;
	const idAnimBlend	*blend;

	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	// if animating with an articulated figure
	if ( AFPoseJoints.Num() && currentTime <= AFPoseTime ) {
		return true;
	}

	blend = channels[ channel ];
	for( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
		if ( !blend->IsDone( currentTime ) ) {
			return true;
		}
	}

	return false;
}

/*
=====================
idAnimator::FrameHasChanged
=====================
*/
bool idAnimator::FrameHasChanged( int currentTime ) const {
	int					i, j;
	const idAnimBlend	*blend;

	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	// if animating with an articulated figure
	if ( AFPoseJoints.Num() && currentTime <= AFPoseTime ) {
		return true;
	}

	blend = channels[ 0 ];
	for( i = 0; i < modelDef->NumChannels(); i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
			if ( blend->FrameHasChanged( currentTime ) ) {
				return true;
			}
		}
	}

	if ( forceUpdate && IsAnimating( currentTime ) ) {
		return true;
	}

	return false;
}

idCVar anim_forceUpdate( "anim_forceUpdate", "0", CVAR_BOOL | CVAR_GAME, "" );

/*
=====================
idAnimator::CreateFrame
=====================
*/
bool idAnimator::CreateFrame( int currentTime, bool force ) {
	if ( !gameLocal.isNewFrame ) {
		return false;
	}

	int					numJoints;
	int					parentNum;
	bool				hasAnim;
	bool				debugInfo;
	float				baseBlend;
	float				blendWeight;
	const idAnimBlend *	blend;
	const int *			jointParent;
	const jointMod_t *	jointMod;
	const idJointQuat *	defaultPose;

	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	idEntity* ent = entity->Cast< idEntity >();

	if ( !force ) {
		if ( lastTransformTime == currentTime ) {
			return false;
		}

		if ( !anim_forceUpdate.GetBool() ) {
			if ( lastTransformTime != -1 && !stoppedAnimatingUpdate && !IsAnimating( currentTime ) ) {
				return false;
			}

			if ( gameLocal.isClient ) {
				if ( ent != NULL && ( ent->aorFlags & AOR_INHIBIT_ANIMATION ) ) {
					if ( ent->AllowAnimationInhibit() ) {
						return false;
					}
				}
			}
		}
	}

	lastTransformTime = currentTime;
	transformCount++;
	stoppedAnimatingUpdate = false;
	debugInfo = false;

	if ( ( ( ent && g_debugAnim.GetInteger() == ent->entityNumber ) || ( g_debugAnim.GetInteger() == -2 ) ) ) {
		debugInfo = true;
		gameLocal.Printf( "---------------\n%d: entity '%s':\n", gameLocal.time, ent->GetName() );
 		gameLocal.Printf( "model '%s':\n", modelDef->GetModelName() );
	}

	// init the joint buffer
	if ( AFPoseJoints.Num() ) {
		// initialize with AF pose anim for the case where there are no other animations and no AF pose joint modifications
		defaultPose = AFPoseJointFrame;
	} else {
		defaultPose = modelDef->GetDefaultPose();
	}

	if ( defaultPose == NULL ) {
		//gameLocal.Warning( "idAnimator::CreateFrame: no defaultPose on '%s'", modelDef->GetName() );
		return false;
	}

	numJoints = modelDef->Joints().Num();
	idJointQuat *jointFrame = ( idJointQuat * )_alloca16( numJoints * sizeof( jointFrame[0] ) );
	SIMDProcessor->Memcpy( jointFrame, defaultPose, numJoints * sizeof( jointFrame[0] ) );

	hasAnim = false;

	// blend the all channel
	baseBlend = 0.0f;
	blend = channels[ ANIMCHANNEL_ALL ];
	int j;
	for ( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
		if ( blend->BlendAnim( currentTime, ANIMCHANNEL_ALL, numJoints, jointFrame, baseBlend, removeOriginOffset, false, debugInfo ) ) {
			hasAnim = true;
			if ( baseBlend >= 1.0f ) {
				break;
			}
		}
	}

	// only blend other channels if there's enough space to blend into
	if ( baseBlend < 1.0f ) {
		for( int i = ANIMCHANNEL_ALL + 1; i < modelDef->NumChannels(); i++ ) {
			blendWeight = baseBlend;
			for( j = 0; j < ANIM_MaxAnimsPerChannel; j++ ) {
				blend = &channels[ i ][ j ];
				if ( blend->BlendAnim( currentTime, ( animChannel_t )i, numJoints, jointFrame, blendWeight, removeOriginOffset, false, debugInfo ) ) {
					hasAnim = true;
					if ( blendWeight >= 1.0f ) {
						// fully blended
						break;
					}
				}
			}

			if ( debugInfo && !AFPoseJoints.Num() && !blendWeight ) {
				gameLocal.Printf( "%d: %s using default pose in model '%s'\n", gameLocal.time, channelNames[ i ], modelDef->GetModelName() );
			}
		}
	}

	// blend the articulated figure pose
	if ( BlendAFPose( jointFrame ) ) {
		hasAnim = true;
	}

	if ( !hasAnim && !jointMods.Num() ) {
		// no animations were updated
		return force || anim_forceUpdate.GetBool();
	}

	// convert the joint quaternions to rotation matrices
	SIMDProcessor->ConvertJointQuatsToJointMats( joints, jointFrame, numJoints );

	// check if we need to modify the origin
	if ( jointMods.Num() && ( jointMods[0]->jointnum == 0 ) ) {
		jointMod = jointMods[0];

		switch( jointMod->transform_axis ) {
			case JOINTMOD_NONE:
				break;

			case JOINTMOD_LOCAL:
				joints[0].SetRotation( jointMod->mat * joints[0].ToMat3() );
				break;
			
			case JOINTMOD_WORLD:
				joints[0].SetRotation( joints[0].ToMat3() * jointMod->mat );
				break;

			case JOINTMOD_LOCAL_OVERRIDE:
			case JOINTMOD_WORLD_OVERRIDE:
				joints[0].SetRotation( jointMod->mat );
				break;
		}

		switch( jointMod->transform_pos ) {
			case JOINTMOD_NONE:
				break;

			case JOINTMOD_LOCAL:
				joints[0].SetTranslation( joints[0].ToVec3() + jointMod->pos );
				break;
			
			case JOINTMOD_LOCAL_OVERRIDE:
			case JOINTMOD_WORLD:
			case JOINTMOD_WORLD_OVERRIDE:
				joints[0].SetTranslation( jointMod->pos );
				break;
		}
		j = 1;
	} else {
		j = 0;
	}

	// add in the model offset
	joints[0].SetTranslation( joints[0].ToVec3() + modelDef->GetVisualOffset() );

	// pointer to joint info
	jointParent = modelDef->JointParents();

	// add in any joint modifications
	int i;
	for( i = 1; j < jointMods.Num(); j++, i++ ) {
		jointMod = jointMods[j];

		// transform any joints preceding the joint modifier
		SIMDProcessor->TransformJoints( joints, jointParent, i, jointMod->jointnum - 1 );
		i = jointMod->jointnum;

		parentNum = jointParent[i];

		// modify the axis
		switch( jointMod->transform_axis ) {
			case JOINTMOD_NONE:
				joints[i].SetRotation( joints[i].ToMat3() * joints[ parentNum ].ToMat3() );
				break;

			case JOINTMOD_LOCAL:
				joints[i].SetRotation( jointMod->mat * ( joints[i].ToMat3() * joints[parentNum].ToMat3() ) );
				break;
			
			case JOINTMOD_LOCAL_OVERRIDE:
				joints[i].SetRotation( jointMod->mat * joints[parentNum].ToMat3() );
				break;

			case JOINTMOD_WORLD:
				joints[i].SetRotation( ( joints[i].ToMat3() * joints[parentNum].ToMat3() ) * jointMod->mat );
				break;

			case JOINTMOD_WORLD_OVERRIDE:
				joints[i].SetRotation( jointMod->mat );
				break;
		}

		// modify the position
		switch( jointMod->transform_pos ) {
			case JOINTMOD_NONE:
				joints[i].SetTranslation( joints[parentNum].ToVec3() + joints[i].ToVec3() * joints[parentNum].ToMat3() );
				break;

			case JOINTMOD_LOCAL:
				joints[i].SetTranslation( joints[parentNum].ToVec3() + ( joints[i].ToVec3() + jointMod->pos ) * joints[parentNum].ToMat3() );
				break;
			
			case JOINTMOD_LOCAL_OVERRIDE:
				joints[i].SetTranslation( joints[parentNum].ToVec3() + jointMod->pos * joints[parentNum].ToMat3() );
				break;

			case JOINTMOD_WORLD:
				joints[i].SetTranslation( joints[parentNum].ToVec3() + joints[i].ToVec3() * joints[parentNum].ToMat3() + jointMod->pos );
				break;

			case JOINTMOD_WORLD_OVERRIDE:
				joints[i].SetTranslation( jointMod->pos );
				break;
		}
	}

	// transform the rest of the hierarchy
	SIMDProcessor->TransformJoints( joints, jointParent, i, numJoints - 1 );

	return true;
}

/*
=====================
idAnimator::ForceUpdate
=====================
*/
void idAnimator::ForceUpdate( void ) {
	lastTransformTime = -1;
	forceUpdate = true;
}

/*
=====================
idAnimator::ClearForceUpdate
=====================
*/
void idAnimator::ClearForceUpdate( void ) {
	forceUpdate = false;
}

/*
=====================
idAnimator::GetJointTransform

=====================
*/
bool idAnimator::GetJointTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset ) {
	if ( ( jointHandle < 0 ) || ( jointHandle >= modelDef->NumJoints() ) ) {
		return false;
	}

	CreateFrame( currentTime, false );

	offset = joints[ jointHandle ].ToVec3();

	return true;
}

/*
=====================
idAnimator::GetJointTransform
=====================
*/
bool idAnimator::GetJointTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis ) {
	if ( !modelDef || ( jointHandle < 0 ) || ( jointHandle >= modelDef->NumJoints() ) ) {
		return false;
	}

	CreateFrame( currentTime, false );

	offset = joints[ jointHandle ].ToVec3();
	axis = joints[ jointHandle ].ToMat3();

	return true;
}

/*
=====================
idAnimator::GetJointTransform
=====================
*/
bool idAnimator::GetJointTransform( jointHandle_t jointHandle, int currentTime, idMat3 &axis ) {
	if ( ( jointHandle < 0 ) || ( jointHandle >= modelDef->NumJoints() ) ) {
		return false;
	}

	CreateFrame( currentTime, false );

	axis = joints[ jointHandle ].ToMat3();

	return true;
}

/*
=====================
idAnimator::GetJointLocalTransform
=====================
*/
bool idAnimator::GetJointLocalTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset ) {
	const idList<jointInfo_t> &modelJoints = modelDef->Joints();

	if ( ( jointHandle < 0 ) || ( jointHandle >= modelJoints.Num() ) ) {
		return false;
	}

	//FIXME: overkill
	CreateFrame( currentTime, false );

	offset = joints[ jointHandle ].ToVec3();
	if ( jointHandle > 0 ) {
		idJointMat m = joints[ modelJoints[ jointHandle ].parentNum ];
		offset = ( offset - m.ToVec3() ) * m.ToMat3();
	}

	return true;
}

/*
=====================
idAnimator::GetJointLocalTransform
=====================
*/
bool idAnimator::GetJointLocalTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis ) {
	if ( !modelDef ) {
		return false;
	}

	const idList<jointInfo_t> &modelJoints = modelDef->Joints();

	if ( ( jointHandle < 0 ) || ( jointHandle >= modelJoints.Num() ) ) {
		return false;
	}

	// FIXME: overkill
	CreateFrame( currentTime, false );

	if ( jointHandle > 0 ) {
		idJointMat m = joints[ jointHandle ];
		m /= joints[ modelJoints[ jointHandle ].parentNum ];
		offset = m.ToVec3();
		axis = m.ToMat3();
	} else {
		offset = joints[ jointHandle ].ToVec3();
		axis = joints[ jointHandle ].ToMat3();
	}

	return true;
}

/*
=====================
idAnimator::GetJointLocalTransform
=====================
*/
bool idAnimator::GetJointLocalTransform( jointHandle_t jointHandle, int currentTime, idMat3 &axis ) {
	if ( !modelDef ) {
		return false;
	}

	const idList<jointInfo_t> &modelJoints = modelDef->Joints();

	if ( ( jointHandle < 0 ) || ( jointHandle >= modelJoints.Num() ) ) {
		return false;
	}

	//FIXME: overkill
	CreateFrame( currentTime, false );

	idJointMat m = joints[ jointHandle ];
	if ( jointHandle > 0 ) {
		m /= joints[ modelJoints[ jointHandle ].parentNum ];
	}
	axis = m.ToMat3();

	return true;
}

/*
=====================
idAnimator::GetJointHandle
=====================
*/
jointHandle_t idAnimator::GetJointHandle( const char *name ) const {
	if ( !modelDef || !modelDef->ModelHandle() ) {
		return INVALID_JOINT;
	}

	return modelDef->ModelHandle()->GetJointHandle( name );
}

/*
=====================
idAnimator::GetJointParent
=====================
*/
jointHandle_t idAnimator::GetJointParent( jointHandle_t jointHandle ) const {
	if ( !modelDef || !modelDef->ModelHandle() || jointHandle <= 0 ) {
		return INVALID_JOINT;
	}	

	return modelDef->Joints()[ jointHandle ].parentNum;
}

/*
=====================
idAnimator::GetJointName
=====================
*/
const char *idAnimator::GetJointName( const jointHandle_t handle ) const {
	if ( !modelDef || !modelDef->ModelHandle() ) {
		return "";
	}

	return modelDef->ModelHandle()->GetJointName( handle );
}

/*
=====================
idAnimator::GetChannelForJoint
=====================
*/
animChannel_t idAnimator::GetChannelForJoint( jointHandle_t joint ) const {
	if ( !modelDef ) {
		gameLocal.Error( "idAnimator::GetChannelForJoint: NULL model" );
	}

	if ( ( joint < 0 ) || ( joint >= numJoints ) ) {
		gameLocal.Error( "idAnimator::GetChannelForJoint: invalid joint num (%d)", joint );
	}

	return modelDef->GetJoint( joint )->channel;
}

/*
=====================
idAnimator::GetFirstChild
=====================
*/
jointHandle_t idAnimator::GetFirstChild( const char *name ) const {
	return GetFirstChild( GetJointHandle( name ) );
}

/*
=====================
idAnimator::GetFirstChild
=====================
*/
jointHandle_t idAnimator::GetFirstChild( jointHandle_t jointnum ) const {
	int					i;
	int					num;
	const jointInfo_t	*joint;

	if ( !modelDef ) {
		return INVALID_JOINT;
	}

	num = modelDef->NumJoints();
	if ( !num ) {
		return jointnum;
	}
	joint = modelDef->GetJoint( 0 );
	for( i = 0; i < num; i++, joint++ ) {
		if ( joint->parentNum == jointnum ) {
			return ( jointHandle_t )joint->num;
		}
	}
	return jointnum;
}

/*
=====================
idAnimator::GetJoints
=====================
*/
void idAnimator::GetJoints( int *numJoints, idJointMat **jointsPtr ) {
	*numJoints	= this->numJoints;
	*jointsPtr	= this->joints;
}

/*
=====================
idAnimator::GetAnimFlags
=====================
*/
const animFlags_t idAnimator::GetAnimFlags( int animNum ) const {
	animFlags_t result;

	const idAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->GetAnimFlags();
	}

	memset( &result, 0, sizeof( result ) );
	return result;
}

/*
=====================
idAnimator::NumFrames
=====================
*/
int	idAnimator::NumFrames( int animNum ) const {
	const idAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->NumFrames();
	} else {
		return 0;
	}
}

/*
=====================
idAnimator::NumSyncedAnims
=====================
*/
int	idAnimator::NumSyncedAnims( int animNum ) const {
	const idAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->NumAnims();
	} else {
		return 0;
	}
}

/*
=====================
idAnimator::AnimName
=====================
*/
const char *idAnimator::AnimName( int animNum ) const {
	const idAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->Name();
	} else {
		return "";
	}
}

/*
=====================
idAnimator::AnimFullName
=====================
*/
const char *idAnimator::AnimFullName( int animNum ) const {
	const idAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->FullName();
	} else {
		return "";
	}
}

/*
=====================
idAnimator::AnimLength
=====================
*/
int	idAnimator::AnimLength( int animNum ) const {
	const idAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->Length();
	} else {
		return 0;
	}
}

/*
=====================
idAnimator::TotalMovementDelta
=====================
*/
const idVec3 &idAnimator::TotalMovementDelta( int animNum ) const {
	const idAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->TotalMovementDelta();
	} else {
		return vec3_origin;
	}
}

/*
=====================
idAnimator::WriteAnimStates
=====================
*/
void idAnimator::WriteAnimStates( const animStates_t baseStates, animStates_t states, const idAnimBlend channels[ ANIM_MaxAnimsPerChannel ], idBitMsg& msg ) const {
	for ( int i = 0; i < ANIM_MaxAnimsPerChannel; i++ ) {
		states[ i ] = channels[ i ];

		if ( states[ i ] == baseStates[ i ] ) {
			msg.WriteBool( false );
		} else {
			msg.WriteBool( true );
			states[ i ].Write( baseStates[ i ], msg );
		}
	}
}

/*
=====================
idAnimator::ReadAnimStates
=====================
*/
void idAnimator::ReadAnimStates( const animStates_t baseStates, animStates_t states, const idBitMsg& msg ) const {
	for ( int i = 0; i < ANIM_MaxAnimsPerChannel; i++ ) {
		if ( !msg.ReadBool() ) {
			states[ i ] = baseStates[ i ];
		} else {
			states[ i ].Read( baseStates[ i ], msg );
		}
	}
}

/*
=====================
idAnimator::CheckAnimStates
=====================
*/
bool idAnimator::CheckAnimStates( const animStates_t baseStates, const idAnimBlend channels[ ANIM_MaxAnimsPerChannel ] ) const {
	for ( int i = 0; i < ANIM_MaxAnimsPerChannel; i++ ) {
		if ( !( baseStates[ i ] == channels[ i ] ) ) {
			return true;
		}
	}

	return false;
}

/*
=====================
idAnimator::ApplyAnimStates
=====================
*/
void idAnimator::ApplyAnimStates( const animStates_t states, idAnimBlend channels[ ANIM_MaxAnimsPerChannel ] ) {
	for ( int i = 0; i < ANIM_MaxAnimsPerChannel; i++ ) {
		states[ i ].Write( channels[ i ] );
	}
}

/*
=====================
idAnimator::WriteAnimStates
=====================
*/
void idAnimator::WriteAnimStates( const animStates_t baseStates, animStates_t states, animChannel_t channel, idBitMsg& msg ) const {
	WriteAnimStates( baseStates, states, channels[ channel ], msg );
}

/*
=====================
idAnimator::CheckAnimStates
=====================
*/
bool idAnimator::CheckAnimStates( const animStates_t baseStates, animChannel_t channel ) const {
	return CheckAnimStates( baseStates, channels[ channel ] );
}

/*
=====================
idAnimator::ApplyAnimStates
=====================
*/
void idAnimator::ApplyAnimStates( const animStates_t states, animChannel_t channel ) {
	CheckAnimStates( states, channels[ channel ] );
}

/***********************************************************************

	Util functions

***********************************************************************/

/*
=====================
ANIM_GetModelDefFromEntityDef
=====================
*/
const idDeclModelDef *ANIM_GetModelDefFromEntityDef( const idDict *args ) {
	const idDeclModelDef *modelDef;

	idStr name = args->GetString( "model" );
	modelDef = gameLocal.declModelDefType.LocalFind( name, false );
	if ( modelDef && modelDef->ModelHandle() ) {
		return modelDef;
	}

	return NULL;
}

/*
=====================
idGameEdit::ANIM_GetModelFromEntityDef
=====================
*/
idRenderModel *idGameEdit::ANIM_GetModelFromEntityDef( const idDict *args ) {
	idRenderModel *model;
	const idDeclModelDef *modelDef;

	model = NULL;

	idStr name = args->GetString( "model" );
	modelDef = gameLocal.declModelDefType.LocalFind( name, false );
	if ( modelDef ) {
		model = modelDef->ModelHandle();
	}

	if ( !model ) {
		model = renderModelManager->FindModel( name );
	}

	if ( model && model->IsDefaultModel() ) {
		return NULL;
	}

	return model;
}

/*
=====================
idGameEdit::ANIM_GetModelFromEntityDef
=====================
*/
idRenderModel *idGameEdit::ANIM_GetModelFromEntityDef( const char *classname ) {
	const idDict *args;

	args = gameLocal.FindEntityDefDict( classname, false );
	if ( !args ) {
		return NULL;
	}

	return ANIM_GetModelFromEntityDef( args );
}

/*
=====================
idGameEdit::ANIM_GetModelOffsetFromEntityDef
=====================
*/
const idVec3 &idGameEdit::ANIM_GetModelOffsetFromEntityDef( const idDict *args ) {
	const idDeclModelDef *modelDef = ANIM_GetModelDefFromEntityDef( args );
	if ( modelDef == NULL ) {
		return vec3_origin;
	}

	return modelDef->GetVisualOffset();
}

/*
=====================
idGameEdit::ANIM_GetModelFromName
=====================
*/
idRenderModel *idGameEdit::ANIM_GetModelFromName( const char *modelName ) {
	const idDeclModelDef *modelDef;
	idRenderModel *model;

	model = NULL;
	modelDef = gameLocal.declModelDefType.LocalFind( modelName, false );
	if ( modelDef ) {
		model = modelDef->ModelHandle();
	}
	if ( !model ) {
		model = renderModelManager->FindModel( modelName );
	}
	return model;
}

/*
=====================
idGameEdit::ANIM_GetAnimFromEntityDef
=====================
*/
const idMD5Anim *idGameEdit::ANIM_GetAnimFromEntityDef( const idDict* args ) {
	const idMD5Anim* md5anim = NULL;
	const char*	modelname = args->GetString( "model" );
	const idDeclModelDef* modelDef = gameLocal.declModelDefType.LocalFind( modelname, false );

	if ( modelDef ) {
		const char* animname = args->GetString( "anim" );
		int	animNum = modelDef->GetAnim( animname );
		if ( animNum ) {
			const idAnim *anim = modelDef->GetAnim( animNum );
			if ( anim ) {
				md5anim = anim->MD5Anim( 0 );
			}
		}
	}
	return md5anim;
}

/*
=====================
idGameEdit::ANIM_GetNumAnimsFromEntityDef
=====================
*/
int idGameEdit::ANIM_GetNumAnimsFromEntityDef( const idDict *args ) {
	const char *modelname;
	const idDeclModelDef *modelDef;

	modelname = args->GetString( "model" );
	modelDef = gameLocal.declModelDefType.LocalFind( modelname, false );
	if ( modelDef ) {
		return modelDef->NumAnims();
	}
	return 0;
}

/*
=====================
idGameEdit::ANIM_GetAnimNameFromEntityDef
=====================
*/
const char *idGameEdit::ANIM_GetAnimNameFromEntityDef( const idDict *args, int animNum ) {
	const char *modelname;
	const idDeclModelDef *modelDef;

	modelname = args->GetString( "model" );
	modelDef = gameLocal.declModelDefType.LocalFind( modelname, false );
	if ( modelDef ) {
		const idAnim* anim = modelDef->GetAnim( animNum );
		if ( anim ) {
			return anim->FullName();
		}
	}
	return "";
}

/*
=====================
idGameEdit::ANIM_GetAnim
=====================
*/
const idMD5Anim *idGameEdit::ANIM_GetAnim( const char *fileName ) {
	return animationLib.GetAnim( fileName );
}

/*
=====================
idGameEdit::ANIM_GetLength
=====================
*/
int	idGameEdit::ANIM_GetLength( const idMD5Anim *anim ) {
	if ( !anim ) {
		return 0;
	}
	return anim->Length();
}

/*
=====================
idGameEdit::ANIM_GetNumFrames
=====================
*/
int idGameEdit::ANIM_GetNumFrames( const idMD5Anim *anim ) {
	if ( !anim ) {
		return 0;
	}
	return anim->NumFrames();
}

/*
=====================
idGameEdit::ANIM_CreateAnimFrame
=====================
*/
void idGameEdit::ANIM_CreateAnimFrame( const idRenderModel *model, const idMD5Anim *anim, int numJoints, idJointMat *joints, int time, const idVec3 &offset, bool remove_origin_offset ) {
	int					i;
	frameBlend_t		frame;
	const idMD5Joint	*md5joints;
	int					*index;

	if ( !model || model->IsDefaultModel() || !anim ) {
		return;
	}

	if ( numJoints != model->NumJoints() ) {
		gameLocal.Error( "ANIM_CreateAnimFrame: different # of joints in renderEntity_t than in model (%s)", model->Name() );
	}

	if ( !model->NumJoints() ) {
		// FIXME: Print out a warning?
		return;
	}

	if ( !joints ) {
		gameLocal.Error( "ANIM_CreateAnimFrame: NULL joint frame pointer on model (%s)", model->Name() );
	}

	if ( numJoints != anim->NumJoints() ) {
		gameLocal.Warning( "Model '%s' has different # of joints than anim '%s'", model->Name(), anim->Name() );
		for( i = 0; i < numJoints; i++ ) {
			joints[i].SetRotation( mat3_identity );
			joints[i].SetTranslation( offset );
		}
		return;
	}

	// create index for all joints
	index = ( int * )_alloca16( numJoints * sizeof( int ) );
	for ( i = 0; i < numJoints; i++ ) {
		index[i] = i;
	}

	// create the frame
	anim->ConvertTimeToFrame( time, 1, frame );
	idJointQuat *jointFrame = ( idJointQuat * )_alloca16( numJoints * sizeof( *jointFrame ) );
	anim->GetInterpolatedFrame( frame, jointFrame, index, numJoints );

	// convert joint quaternions to joint matrices
	SIMDProcessor->ConvertJointQuatsToJointMats( joints, jointFrame, numJoints );

	// first joint is always root of entire hierarchy
	if ( remove_origin_offset ) {
		joints[0].SetTranslation( offset );
	} else {
		joints[0].SetTranslation( joints[0].ToVec3() + offset );
	}

	// transform the children
	md5joints = model->GetJoints();
	for( i = 1; i < numJoints; i++ ) {
		joints[i] *= joints[ md5joints[i].parent - md5joints ];
	}
}

/*
=====================
idGameEdit::ANIM_CreateMeshForAnim
=====================
*/
idRenderModel *idGameEdit::ANIM_CreateMeshForAnim( idRenderModel *model, const idDict& args, bool remove_origin_offset ) {
	if ( model == NULL || model->IsDefaultModel() ) {
		return NULL;
	}
			
	const idMD5Anim*		md5anim;
	idStr					filename;
	idStr					extension;
	const idAnim*			anim;
	int						animNum;
	idVec3					offset;
	const idDeclModelDef*	modelDef;
	const char*				animname = args.GetString( "anim", "base" );
	int						frame = args.GetInt( "frame", "1" );

	if( frame < 1 ) {
		frame = 1;
	}

	renderEntity_t			ent;
	memset( &ent, 0, sizeof( ent ) );

	ent.bounds.Clear();
	ent.suppressSurfaceInViewID = 0;
	ent.flags.noHardwareSkinning = true;

	modelDef = ANIM_GetModelDefFromEntityDef( &args );
	if ( modelDef ) {
		animNum = modelDef->GetAnim( animname );
		if ( animNum == 0 ) {
			return NULL;
		}
		anim = modelDef->GetAnim( animNum );
		if ( anim == NULL ) {
			return NULL;
		}
		md5anim = anim->MD5Anim( 0 );
		ent.customSkin = modelDef->GetDefaultSkin();
		offset = modelDef->GetVisualOffset();
	} else {
		filename = animname;
		filename.ExtractFileExtension( extension );
		if ( !extension.Length() ) {
			animname = args.GetString( va( "anim %s", animname ) );
		}

		md5anim = animationLib.GetAnim( animname );
		offset.Zero();
	}

	if ( md5anim == NULL ) {
		return NULL;
	}

	const char* temp = args.GetString( "skin", "" );
	if( temp[ 0 ] ) {
		ent.customSkin = declHolder.declSkinType.LocalFind( temp );
	}

	ent.numJoints = model->NumJoints();
	ent.joints = ( idJointMat * )Mem_AllocAligned( ent.numJoints * sizeof( *ent.joints ), ALIGN_16 );

	ANIM_CreateAnimFrame( model, md5anim, ent.numJoints, ent.joints, ( frame * 1000 ) / md5anim->GetFrameRate(), offset, remove_origin_offset );

	idRenderModel* newmodel = renderSystem->InstantiateDynamicModel( model, &ent );

	Mem_FreeAligned( ent.joints );
	ent.joints = NULL;

	return newmodel;
}


/*
============
idGameEdit::ANIM_GetFrameBounds
============
*/
void idGameEdit::ANIM_GetFrameBounds( const idMD5Anim *anim, idBounds& bounds, int frame, int cyclecount ) {
	if( !anim ) {
		bounds.Clear();
		return;
	}
	anim->GetBounds( bounds, frame, cyclecount );
}
