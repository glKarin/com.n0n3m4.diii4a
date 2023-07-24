
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
#include "../../framework/Licensee.h"

bool idAnimManager::forceExport = false;

idCVar anim_reduced( "anim_reduced", "1", CVAR_BOOL|CVAR_ARCHIVE, "" );
idCVar r_writeAnimB( "r_writeAnimB", "0", CVAR_BOOL, "Write out binary versions of animations." );
idCVar r_loadAnimB( "r_loadAnimB", "1", CVAR_BOOL, "Attempt loading of binary version of animations." );


/*
===============================================================================

  idAnimBlendNetworkInfo_Minimal

===============================================================================
*/

/*
==================
idAnimBlendNetworkInfo_Minimal::MakeDefault
==================
*/
void idAnimBlend::idAnimBlendNetworkInfo_Minimal::MakeDefault( void ) {
	startTime		= 0;
	endTime			= 0;
	blendStartTime	= 0;
	blendDuration	= 0;
	blendStartValue	= 0.f;
	blendEndValue	= 0.f;
	animNum			= -1;
}

/*
==================
idAnimBlendNetworkInfo_Minimal::operator=
==================
*/
void idAnimBlend::idAnimBlendNetworkInfo_Minimal::operator=( const idAnimBlend& anim ) {
	startTime		= anim.starttime;
	endTime			= anim.endtime;
	blendStartTime	= anim.blendStartTime;
	blendDuration	= anim.blendDuration;
	blendStartValue	= anim.blendStartValue;
	blendEndValue	= anim.blendEndValue;
	animNum			= anim.animNum;
}

/*
==================
idAnimBlendNetworkInfo_Minimal::Write
==================
*/
void idAnimBlend::idAnimBlendNetworkInfo_Minimal::Write( idAnimBlend& anim ) const {
	anim.starttime			= startTime;
	anim.endtime			= endTime;
	anim.blendStartTime		= blendStartTime;
	anim.blendDuration		= blendDuration;
	anim.blendStartValue	= blendStartValue;
	anim.blendEndValue		= blendEndValue;
	anim.animNum			= animNum;
}

/*
==================
idAnimBlendNetworkInfo_Minimal::operator==
==================
*/
bool idAnimBlend::idAnimBlendNetworkInfo_Minimal::operator==( const idAnimBlendNetworkInfo_Minimal& rhs ) const {
	return	startTime == rhs.startTime &&
			endTime == rhs.endTime &&
			blendStartTime == rhs.blendStartTime &&
			blendDuration == rhs.blendDuration &&
			blendStartValue == rhs.blendStartValue &&
			blendEndValue == rhs.blendEndValue &&
			animNum == rhs.animNum;
}

/*
==================
idAnimBlendNetworkInfo_Minimal::operator==
==================
*/
bool idAnimBlend::idAnimBlendNetworkInfo_Minimal::operator==( const idAnimBlend& rhs ) const {
	return	startTime == rhs.starttime &&
			endTime == rhs.endtime &&
			blendStartTime == rhs.blendStartTime &&
			blendDuration == rhs.blendDuration &&
			blendStartValue == rhs.blendStartValue &&
			blendEndValue == rhs.blendEndValue &&
			animNum == rhs.animNum;
}

/*
==================
idAnimBlend::idAnimBlendNetworkInfo_Minimal::Read
==================
*/
void idAnimBlend::idAnimBlendNetworkInfo_Minimal::Read( const idAnimBlendNetworkInfo_Minimal& base, const idBitMsg& msg ) {
	startTime		= msg.ReadDeltaLong( base.startTime );
	endTime			= msg.ReadDeltaLong( base.endTime );
	blendStartTime	= msg.ReadDeltaLong( base.blendStartTime );
	blendDuration	= msg.ReadDeltaLong( base.blendDuration );
	blendStartValue	= msg.ReadDeltaFloat( base.blendStartValue );
	blendEndValue	= msg.ReadDeltaFloat( base.blendEndValue );
	animNum			= msg.ReadDeltaShort( base.animNum );
}

/*
==================
idAnimBlend::idAnimBlendNetworkInfo_Minimal::Write
==================
*/
void idAnimBlend::idAnimBlendNetworkInfo_Minimal::Write( const idAnimBlendNetworkInfo_Minimal& base, idBitMsg& msg ) const {
	msg.WriteDeltaLong( base.startTime, startTime );
	msg.WriteDeltaLong( base.endTime, endTime );
	msg.WriteDeltaLong( base.blendStartTime, blendStartTime );
	msg.WriteDeltaLong( base.blendDuration, blendDuration );
	msg.WriteDeltaFloat( base.blendStartValue,blendStartValue  );
	msg.WriteDeltaFloat( base.blendEndValue, blendEndValue );
	msg.WriteDeltaShort( base.animNum, animNum );
}

/*
==================
idAnimBlend::idAnimBlendNetworkInfo_Minimal::Read
==================
*/
void idAnimBlend::idAnimBlendNetworkInfo_Minimal::Read( idFile* file ) {
}

/*
==================
idAnimBlend::idAnimBlendNetworkInfo_Minimal::Write
==================
*/
void idAnimBlend::idAnimBlendNetworkInfo_Minimal::Write( idFile* file ) const {
}

/***********************************************************************

	idMD5Anim

***********************************************************************/

/*
====================
idMD5Anim::idMD5Anim
====================
*/
idMD5Anim::idMD5Anim() {
	ref_count	= 0;
	numFrames	= 0;
	numJoints	= 0;
	frameRate	= 24;
	animLength	= 0;
	reduced		= false;
	totaldelta.Zero();
}

/*
====================
idMD5Anim::idMD5Anim
====================
*/
idMD5Anim::~idMD5Anim() {
	Free();
}

/*
====================
idMD5Anim::Free
====================
*/
void idMD5Anim::Free( void ) {
	numFrames	= 0;
	numJoints	= 0;
	frameRate	= 24;
	animLength	= 0;
	reduced		= false;
	name		= "";

	totaldelta.Zero();

	jointInfo.Clear();
	bounds.Clear();
	componentFrames.Clear();
}

/*
====================
idMD5Anim::NumFrames
====================
*/
int	idMD5Anim::NumFrames( void ) const {
	return numFrames;
}

/*
====================
idMD5Anim::NumJoints
====================
*/
int	idMD5Anim::NumJoints( void ) const {
	return numJoints;
}

/*
====================
idMD5Anim::Length
====================
*/
int idMD5Anim::Length( void ) const {
	return animLength;
}

/*
=====================
idMD5Anim::TotalMovementDelta
=====================
*/
const idVec3 &idMD5Anim::TotalMovementDelta( void ) const {
	return totaldelta;
}

/*
=====================
idMD5Anim::TotalMovementDelta
=====================
*/
const char *idMD5Anim::Name( void ) const {
	return name;
}

/*
====================
idMD5Anim::Reload
====================
*/
bool idMD5Anim::Reload( void ) {
	idStr filename;

	filename = name;
	Free();

	return LoadAnim( filename );
}

/*
====================
idMD5Anim::Allocated
====================
*/
size_t idMD5Anim::Allocated( void ) const {
	size_t size = bounds.Allocated() + jointInfo.Allocated() + baseFrame.Allocated() + componentFrames.Allocated() + name.Allocated();
	return size;
}

/*
====================
idMD5Anim::LoadAnim
====================
*/

ID_INLINE short AssertShortRange( int value ) {
	assert( value >= -( 1 << ( sizeof( short ) * 8 - 1 ) ) );
	assert( value < ( 1 << ( sizeof( short ) * 8 - 1 ) ) );
	return (short) value;
}

bool idMD5Anim::LoadAnim( const char *filename ) {
	int		version;
	idLexer	parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT );
	idToken	token;
	int		i, j;
	int		num;
	bool	offsetwarning = false;

	int skipFrames = 2;

	if ( !parser.LoadFile( filename ) ) {
		return false;
	}

	Free();

	name = filename;

	parser.ExpectTokenString( MD5_VERSION_STRING );
	version = parser.ParseInt();
	if ( version != MD5_VERSION ) {
		// ARNOUT: FIXME: BACKWARDS COMPATIBILITY
		if ( version != 10 ) {
			parser.Error( "Invalid version %d.  Should be version %d", version, MD5_VERSION );
		}
	}

	// skip the commandline
	parser.ExpectTokenString( "commandline" );
	parser.ReadToken( &token );

	// parse num frames
	parser.ExpectTokenString( "numFrames" );
	numFrames = parser.ParseInt();
	if ( numFrames <= 0 ) {
		parser.Error( "Invalid number of frames: %d", numFrames );
	}

	// parse num joints
	parser.ExpectTokenString( "numJoints" );
	numJoints = parser.ParseInt();
	if ( numJoints <= 0 ) {
		parser.Error( "Invalid number of joints: %d", numJoints );
	}

	// parse frame rate
	parser.ExpectTokenString( "frameRate" );
	frameRate = parser.ParseInt();
	if ( frameRate < 0 ) {
		parser.Error( "Invalid frame rate: %d", frameRate );
	}

	// parse number of animated components
	parser.ExpectTokenString( "numAnimatedComponents" );
	numAnimatedComponents = parser.ParseInt();
	if ( ( numAnimatedComponents < 0 ) || ( numAnimatedComponents > numJoints * 6 ) ) {
		parser.Error( "Invalid number of animated components: %d", numAnimatedComponents );
	}

	// parse the hierarchy
	jointInfo.SetGranularity( 1 );
	jointInfo.SetNum( numJoints );
	parser.ExpectTokenString( "hierarchy" );
	parser.ExpectTokenString( "{" );
	for( i = 0; i < numJoints; i++ ) {
		parser.ReadToken( &token );
		jointInfo[ i ].nameIndex = AssertShortRange( animationLib.JointIndex( token ) );
		
		// parse parent num
		jointInfo[ i ].parentNum = AssertShortRange( parser.ParseInt() );
		if ( jointInfo[ i ].parentNum >= i ) {
			parser.Error( "Invalid parent num: %d", jointInfo[ i ].parentNum );
		}

		if ( ( i != 0 ) && ( jointInfo[ i ].parentNum < 0 ) ) {
			parser.Error( "Animations may have only one root joint" );
		}

		// parse anim bits
		jointInfo[ i ].animBits = AssertShortRange( parser.ParseInt() );
		if ( jointInfo[ i ].animBits & ~63 ) {
			parser.Error( "Invalid anim bits: %d", jointInfo[ i ].animBits );
		}

		// parse first component
		jointInfo[ i ].firstComponent = AssertShortRange( parser.ParseInt() );
		if ( ( numAnimatedComponents > 0 ) && ( ( jointInfo[ i ].firstComponent < 0 ) || ( jointInfo[ i ].firstComponent >= numAnimatedComponents ) ) ) {
			parser.Error( "Invalid first component: %d", jointInfo[ i ].firstComponent );
		}
	}

	parser.ExpectTokenString( "}" );

	// parse bounds
	parser.ExpectTokenString( "bounds" );
	parser.ExpectTokenString( "{" );
	bounds.SetGranularity( 1 );
	bounds.SetNum( numFrames );
	for( i = 0; i < numFrames; i++ ) {
		idBounds b;
		parser.Parse1DMatrix( 3, b[ 0 ].ToFloatPtr() );
		parser.Parse1DMatrix( 3, b[ 1 ].ToFloatPtr() );
		bounds[i].SetBounds( b );
	}
	parser.ExpectTokenString( "}" );

	// parse base frame
	baseFrame.SetGranularity( 1 );
	baseFrame.SetNum( numJoints );
	parser.ExpectTokenString( "baseframe" );
	parser.ExpectTokenString( "{" );
	for( i = 0; i < numJoints; i++ ) {
		idVec3 t;
		idCQuat q;

		parser.Parse1DMatrix( 3, t.ToFloatPtr() );
		parser.Parse1DMatrix( 3, q.ToFloatPtr() );

		t.FixDenormals();
		q.FixDenormals();

		if ( !offsetwarning ) {
			if ( fabsf( t.x ) >= idCompressedJointQuat::MAX_BONE_LENGTH ||
				fabsf( t.y ) >= idCompressedJointQuat::MAX_BONE_LENGTH ||
				fabsf( t.z ) >= idCompressedJointQuat::MAX_BONE_LENGTH ) {
					int jointNum = jointInfo[ i ].nameIndex;
					gameLocal.Warning( "WARNING: bone offset of '%s' joint '%s' greater than %i", 
						filename, 
						animationLib.JointName( jointNum ),
						idCompressedJointQuat::MAX_BONE_LENGTH );

				offsetwarning = true;
			}
		}

		baseFrame[ i ].t[0] = idCompressedJointQuat::OffsetToShort( t.x );
		baseFrame[ i ].t[1] = idCompressedJointQuat::OffsetToShort( t.y );
		baseFrame[ i ].t[2] = idCompressedJointQuat::OffsetToShort( t.z );
		baseFrame[ i ].q[0] = idCompressedJointQuat::QuatToShort( q.x );
		baseFrame[ i ].q[1] = idCompressedJointQuat::QuatToShort( q.y );
		baseFrame[ i ].q[2] = idCompressedJointQuat::QuatToShort( q.z );
	}
	parser.ExpectTokenString( "}" );

	// parse frames
	componentFrames.SetGranularity( 1 );
	componentFrames.SetNum( numAnimatedComponents * numFrames );

	short *componentPtr = componentFrames.Begin();
	for( i = 0; i < numFrames; i++ ) {
		parser.ExpectTokenString( "frame" );
		num = parser.ParseInt();
		if ( num != i ) {
			parser.Error( "Expected frame number %d", i );
		}
		parser.ExpectTokenString( "{" );

		for ( j = 0; j < numJoints; j++ ) {
			int animBits = jointInfo[j].animBits;
			if ( animBits & ANIM_TX ) {
				float x = parser.ParseFloat();
				*componentPtr++ = idCompressedJointQuat::OffsetToShort( x );
			}
			if ( animBits & ANIM_TY ) {
				float y = parser.ParseFloat();
				*componentPtr++ = idCompressedJointQuat::OffsetToShort( y );
			}
			if ( animBits & ANIM_TZ ) {
				float z = parser.ParseFloat();
				*componentPtr++ = idCompressedJointQuat::OffsetToShort( z );
			}
			if ( animBits & ANIM_QX ) {
				float x = parser.ParseFloat();
				*componentPtr++ = idCompressedJointQuat::QuatToShort( x );
			}
			if ( animBits & ANIM_QY ) {
				float y = parser.ParseFloat();
				*componentPtr++ = idCompressedJointQuat::QuatToShort( y );
			}
			if ( animBits & ANIM_QZ ) {
				float z = parser.ParseFloat();
				*componentPtr++ = idCompressedJointQuat::QuatToShort( z );
			}
		}

		parser.ExpectTokenString( "}" );
	}

	// get total move delta
	if ( !numAnimatedComponents ) {
		totaldelta.Zero();
	} else {
		componentPtr = &componentFrames[ jointInfo[ 0 ].firstComponent ];
		if ( jointInfo[ 0 ].animBits & ANIM_TX ) {
			for( i = 0; i < numFrames; i++ ) {
				componentPtr[ numAnimatedComponents * i ] -= baseFrame[ 0 ].t[0];
			}
			totaldelta.x = idCompressedJointQuat::ShortToOffset( componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ] );
			componentPtr++;
		} else {
			totaldelta.x = 0.0f;
		}
		if ( jointInfo[ 0 ].animBits & ANIM_TY ) {
			for( i = 0; i < numFrames; i++ ) {
				componentPtr[ numAnimatedComponents * i ] -= baseFrame[ 0 ].t[1];
			}
			totaldelta.y = idCompressedJointQuat::ShortToOffset( componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ] );
			componentPtr++;
		} else {
			totaldelta.y = 0.0f;
		}
		if ( jointInfo[ 0 ].animBits & ANIM_TZ ) {
			for( i = 0; i < numFrames; i++ ) {
				componentPtr[ numAnimatedComponents * i ] -= baseFrame[ 0 ].t[2];
			}
			totaldelta.z = idCompressedJointQuat::ShortToOffset( componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ] );
		} else {
			totaldelta.z = 0.0f;
		}
	}
	baseFrame[ 0 ].ClearOffset();

	// we don't count last frame because it would cause a 1 frame pause at the end
	animLength = ( ( numFrames - 1 ) * 1000 + frameRate - 1 ) / frameRate;

	if ( numFrames > 4 && numAnimatedComponents && anim_reduced.GetBool() && !r_writeAnimB.GetBool() ) {
		Resample();
	}

	// done
	return true;
}

/*
====================
idMD5Anim::Resample
====================
*/
void idMD5Anim::Resample( void ) {

	if ( reduced ) {
		return;
	}

	int idealFrames = numFrames/2;
	idList<short> resampledFrames;
	resampledFrames.SetGranularity( 1 );
	resampledFrames.SetNum( numAnimatedComponents * idealFrames );

	idCompressedJointQuat *compressedJoints = (idCompressedJointQuat *)_alloca16( numJoints * sizeof( compressedJoints[0] ) );
	idCompressedJointQuat *compressedBlendJoints = (idCompressedJointQuat *)_alloca16( numJoints * sizeof( compressedBlendJoints[0] ) );
	idJointQuat *joints = (idJointQuat *)_alloca16( numJoints * sizeof( joints[0] ) );
	idJointQuat *blendJoints = (idJointQuat *)_alloca16( numJoints * sizeof( blendJoints[0] ) );
	int *baseIndex = (int*)_alloca16( numJoints * sizeof( baseIndex[0] ) );
	for (int i=0; i<numJoints; i++) {
		baseIndex[i] = i;
	}

	for (int i=0; i<idealFrames; i++) {
		float srcf = (i*(numFrames-1)) / (idealFrames-1);
		int srci = (int)idMath::Floor( srcf );
		float blend = srcf - srci;
		if ( i != srci ) {
			bounds[i] = bounds[srci];
		}
		{
			short *destPtr = &resampledFrames[ i * numAnimatedComponents ];
			short *srcPtr = &componentFrames[ srci * numAnimatedComponents ];
			short *nextSrcPtr;
			if ( (srci+1) < numFrames ) {
				nextSrcPtr = &componentFrames[ (srci+1) * numAnimatedComponents ];
			} else {
				nextSrcPtr = srcPtr;
			}
			int numBaseIndex = 0;
			for ( int j = 0; j < numJoints; j++ ) {
				const jointAnimInfo_t *	infoPtr = &jointInfo[j];
				int animBits = infoPtr->animBits;
				if ( animBits == 0 ) {
					continue;
				}
				baseIndex[numBaseIndex] = numBaseIndex; 
				idCompressedJointQuat *jointPtr = &compressedJoints[numBaseIndex];
				idCompressedJointQuat *blendPtr = &compressedBlendJoints[numBaseIndex];
				const short *jointframe1 = srcPtr + infoPtr->firstComponent;
				const short *jointframe2 = nextSrcPtr + infoPtr->firstComponent;

				*jointPtr = baseFrame[j];

				switch( animBits & (ANIM_TX|ANIM_TY|ANIM_TZ) ) {
					case 0:
						blendPtr->t[0] = jointPtr->t[0];
						blendPtr->t[1] = jointPtr->t[1];
						blendPtr->t[2] = jointPtr->t[2];
						break;
					case ANIM_TX:
						jointPtr->t[0] = jointframe1[0];
						blendPtr->t[0] = jointframe2[0];
						blendPtr->t[1] = jointPtr->t[1];
						blendPtr->t[2] = jointPtr->t[2];
						jointframe1++;
						jointframe2++;
						break;
					case ANIM_TY:
						jointPtr->t[1] = jointframe1[0];
						blendPtr->t[1] = jointframe2[0];
						blendPtr->t[0] = jointPtr->t[0];
						blendPtr->t[2] = jointPtr->t[2];
						jointframe1++;
						jointframe2++;
						break;
					case ANIM_TZ:
						jointPtr->t[2] = jointframe1[0];
						blendPtr->t[2] = jointframe2[0];
						blendPtr->t[0] = jointPtr->t[0];
						blendPtr->t[1] = jointPtr->t[1];
						jointframe1++;
						jointframe2++;
						break;
					case ANIM_TX|ANIM_TY:
						jointPtr->t[0] = jointframe1[0];
						jointPtr->t[1] = jointframe1[1];
						blendPtr->t[0] = jointframe2[0];
						blendPtr->t[1] = jointframe2[1];
						blendPtr->t[2] = jointPtr->t[2];
						jointframe1 += 2;
						jointframe2 += 2;
						break;
					case ANIM_TX|ANIM_TZ:
						jointPtr->t[0] = jointframe1[0];
						jointPtr->t[2] = jointframe1[1];
						blendPtr->t[0] = jointframe2[0];
						blendPtr->t[2] = jointframe2[1];
						blendPtr->t[1] = jointPtr->t[1];
						jointframe1 += 2;
						jointframe2 += 2;
						break;
					case ANIM_TY|ANIM_TZ:
						jointPtr->t[1] = jointframe1[0];
						jointPtr->t[2] = jointframe1[1];
						blendPtr->t[1] = jointframe2[0];
						blendPtr->t[2] = jointframe2[1];
						blendPtr->t[0] = jointPtr->t[0];
						jointframe1 += 2;
						jointframe2 += 2;
						break;
					case ANIM_TX|ANIM_TY|ANIM_TZ:
						jointPtr->t[0] = jointframe1[0];
						jointPtr->t[1] = jointframe1[1];
						jointPtr->t[2] = jointframe1[2];
						blendPtr->t[0] = jointframe2[0];
						blendPtr->t[1] = jointframe2[1];
						blendPtr->t[2] = jointframe2[2];
						jointframe1 += 3;
						jointframe2 += 3;
						break;
				}

				switch( animBits & (ANIM_QX|ANIM_QY|ANIM_QZ) ) {
					case 0:
						blendPtr->q[0] = jointPtr->q[0];
						blendPtr->q[1] = jointPtr->q[1];
						blendPtr->q[2] = jointPtr->q[2];
						break;
					case ANIM_QX:
						jointPtr->q[0] = jointframe1[0];
						blendPtr->q[0] = jointframe2[0];
						blendPtr->q[1] = jointPtr->q[1];
						blendPtr->q[2] = jointPtr->q[2];
						break;
					case ANIM_QY:
						jointPtr->q[1] = jointframe1[0];
						blendPtr->q[1] = jointframe2[0];
						blendPtr->q[0] = jointPtr->q[0];
						blendPtr->q[2] = jointPtr->q[2];
						break;
					case ANIM_QZ:
						jointPtr->q[2] = jointframe1[0];
						blendPtr->q[2] = jointframe2[0];
						blendPtr->q[0] = jointPtr->q[0];
						blendPtr->q[1] = jointPtr->q[1];
						break;
					case ANIM_QX|ANIM_QY:
						jointPtr->q[0] = jointframe1[0];
						jointPtr->q[1] = jointframe1[1];
						blendPtr->q[0] = jointframe2[0];
						blendPtr->q[1] = jointframe2[1];
						blendPtr->q[2] = jointPtr->q[2];
						break;
					case ANIM_QX|ANIM_QZ:
						jointPtr->q[0] = jointframe1[0];
						jointPtr->q[2] = jointframe1[1];
						blendPtr->q[0] = jointframe2[0];
						blendPtr->q[2] = jointframe2[1];
						blendPtr->q[1] = jointPtr->q[1];
						break;
					case ANIM_QY|ANIM_QZ:
						jointPtr->q[1] = jointframe1[0];
						jointPtr->q[2] = jointframe1[1];
						blendPtr->q[1] = jointframe2[0];
						blendPtr->q[2] = jointframe2[1];
						blendPtr->q[0] = jointPtr->q[0];
						break;
					case ANIM_QX|ANIM_QY|ANIM_QZ:
						jointPtr->q[0] = jointframe1[0];
						jointPtr->q[1] = jointframe1[1];
						jointPtr->q[2] = jointframe1[2];
						blendPtr->q[0] = jointframe2[0];
						blendPtr->q[1] = jointframe2[1];
						blendPtr->q[2] = jointframe2[2];
						break;
				}
				numBaseIndex++;
			}
			blendJoints = (idJointQuat *)_alloca16( baseFrame.Num() * sizeof( blendJoints[ 0 ] ) );

			SIMDProcessor->DecompressJoints( joints, compressedJoints, baseIndex, numBaseIndex );
			SIMDProcessor->DecompressJoints( blendJoints, compressedBlendJoints, baseIndex, numBaseIndex );

			SIMDProcessor->BlendJoints( joints, blendJoints, 1.f-blend, baseIndex, numBaseIndex );
			numBaseIndex = 0;
			for ( int j = 0; j < numJoints; j++ ) {
				const jointAnimInfo_t *	infoPtr = &jointInfo[j];
				int animBits = infoPtr->animBits;
				if ( animBits == 0 ) {
					continue;
				}

				idJointQuat const &curjoint = joints[numBaseIndex];
				idCQuat cq = curjoint.q.ToCQuat();
				idCompressedJointQuat cj;
				cj.t[0] = idCompressedJointQuat::OffsetToShort( curjoint.t.x );
				cj.t[1] = idCompressedJointQuat::OffsetToShort( curjoint.t.y );
				cj.t[2] = idCompressedJointQuat::OffsetToShort( curjoint.t.z );
				cj.q[0] = idCompressedJointQuat::QuatToShort( cq.x );
				cj.q[1] = idCompressedJointQuat::QuatToShort( cq.y );
				cj.q[2] = idCompressedJointQuat::QuatToShort( cq.z );

				short *output = &destPtr[ infoPtr->firstComponent ];
				if ( animBits & (ANIM_TX) ) {
					*output++ = cj.t[0];
				}
				if ( animBits & (ANIM_TY) ) {
					*output++ = cj.t[1];
				}
				if ( animBits & (ANIM_TZ) ) {
					*output++ = cj.t[2];
				}
				if ( animBits & (ANIM_QX) ) {
					*output++ = cj.q[0];
				}
				if ( animBits & (ANIM_QY) ) {
					*output++ = cj.q[1];
				}
				if ( animBits & (ANIM_QZ) ) {
					*output++ = cj.q[2];
				}

				numBaseIndex++;
			}
		}
	}
	int nb = numFrames;
	int fr = frameRate;
	frameRate = (frameRate * idealFrames) / numFrames;//(((numFrames - 1) * 1000) + animLength - 1) / (animLength);
	numFrames = idealFrames;
	animLength = ( ( numFrames - 1 ) * 1000 + frameRate - 1 ) / frameRate;

	bounds.SetGranularity( 1 );
	bounds.SetNum( numFrames );
	componentFrames = resampledFrames;

	reduced = true;
}

/*
====================
idMD5Anim::IncreaseRefs
====================
*/
void idMD5Anim::IncreaseRefs( void ) const {
	ref_count++;
}

/*
====================
idMD5Anim::DecreaseRefs
====================
*/
void idMD5Anim::DecreaseRefs( void ) const {
	ref_count--;
}

/*
====================
idMD5Anim::NumRefs
====================
*/
int idMD5Anim::NumRefs( void ) const {
	return ref_count;
}

/*
====================
idMD5Anim::GetFrameBlend
====================
*/
void idMD5Anim::GetFrameBlend( int framenum, frameBlend_t &frame ) const {
	frame.cycleCount	= 0;
	frame.backlerp		= 0.0f;
	frame.frontlerp		= 1.0f;

	// frame 1 is first frame
	framenum--;
	if ( framenum < 0 ) {
		framenum = 0;
	} else if ( framenum >= numFrames ) {
		framenum = numFrames - 1;
	}

	frame.frame1 = framenum;
	frame.frame2 = framenum;
}

/*
====================
idMD5Anim::ConvertTimeToFrame
====================
*/
void idMD5Anim::ConvertTimeToFrame( int time, int cyclecount, frameBlend_t &frame ) const {
	int frameTime;
	int frameNum;

	if ( numFrames <= 1 ) {
		frame.frame1		= 0;
		frame.frame2		= 0;
		frame.backlerp		= 0.0f;
		frame.frontlerp		= 1.0f;
		frame.cycleCount	= 0;
		return;
	}

	if ( time <= 0 ) {
		frame.frame1		= 0;
		frame.frame2		= 1;
		frame.backlerp		= 0.0f;
		frame.frontlerp		= 1.0f;
		frame.cycleCount	= 0;
		return;
	}
	
	frameTime			= time * frameRate;
	frameNum			= frameTime / 1000;
	frame.cycleCount	= frameNum / ( numFrames - 1 );

	if ( ( cyclecount > 0 ) && ( frame.cycleCount >= cyclecount ) ) {
		frame.cycleCount	= cyclecount - 1;
		frame.frame1		= numFrames - 1;
		frame.frame2		= frame.frame1;
		frame.backlerp		= 0.0f;
		frame.frontlerp		= 1.0f;
		return;
	}
	
	frame.frame1 = frameNum % ( numFrames - 1 );
	frame.frame2 = frame.frame1 + 1;
	if ( frame.frame2 >= numFrames ) {
		frame.frame2 = 0;
	}

	frame.backlerp	= ( frameTime % 1000 ) * 0.001f;
	frame.frontlerp	= 1.0f - frame.backlerp;
}

/*
====================
idMD5Anim::GetOrigin
====================
*/
void idMD5Anim::GetOrigin( idVec3 &offset, int time, int cyclecount ) const {
	frameBlend_t frame;

	offset[0] = idCompressedJointQuat::ShortToOffset( baseFrame[ 0 ].t[0] );
	offset[1] = idCompressedJointQuat::ShortToOffset( baseFrame[ 0 ].t[1] );
	offset[2] = idCompressedJointQuat::ShortToOffset( baseFrame[ 0 ].t[2] );

	if ( !( jointInfo[ 0 ].animBits & ( ANIM_TX | ANIM_TY | ANIM_TZ ) ) ) {
		// just use the baseframe
		return;
	}

	ConvertTimeToFrame( time, cyclecount, frame );

	const short *componentPtr1 = &componentFrames[ numAnimatedComponents * frame.frame1 + jointInfo[ 0 ].firstComponent ];
	const short *componentPtr2 = &componentFrames[ numAnimatedComponents * frame.frame2 + jointInfo[ 0 ].firstComponent ];

	if ( jointInfo[ 0 ].animBits & ANIM_TX ) {
		offset.x = idCompressedJointQuat::ShortToOffset( *componentPtr1 ) * frame.frontlerp + idCompressedJointQuat::ShortToOffset( *componentPtr2 ) * frame.backlerp;
		componentPtr1++;
		componentPtr2++;
	}

	if ( jointInfo[ 0 ].animBits & ANIM_TY ) {
		offset.y = idCompressedJointQuat::ShortToOffset( *componentPtr1 ) * frame.frontlerp + idCompressedJointQuat::ShortToOffset( *componentPtr2 ) * frame.backlerp;
		componentPtr1++;
		componentPtr2++;
	}

	if ( jointInfo[ 0 ].animBits & ANIM_TZ ) {
		offset.z = idCompressedJointQuat::ShortToOffset( *componentPtr1 ) * frame.frontlerp + idCompressedJointQuat::ShortToOffset( *componentPtr2 ) * frame.backlerp;
	}

	if ( frame.cycleCount ) {
		offset += totaldelta * ( float )frame.cycleCount;
	}
}

/*
====================
idMD5Anim::GetOriginRotation
====================
*/
void idMD5Anim::GetOriginRotation( idQuat &rotation, int time, int cyclecount ) const {
	frameBlend_t	frame;
	int				animBits;
	
	animBits = jointInfo[ 0 ].animBits;
	if ( !( animBits & ( ANIM_QX | ANIM_QY | ANIM_QZ ) ) ) {
		// just use the baseframe		
		rotation[0] = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[0] );
		rotation[1] = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[1] );
		rotation[2] = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[2] );
		rotation.w = rotation.CalcW();
		return;
	}

	ConvertTimeToFrame( time, cyclecount, frame );

	const short *jointframe1 = &componentFrames[ numAnimatedComponents * frame.frame1 + jointInfo[ 0 ].firstComponent ];
	const short *jointframe2 = &componentFrames[ numAnimatedComponents * frame.frame2 + jointInfo[ 0 ].firstComponent ];

	if ( animBits & ANIM_TX ) {
		jointframe1++;
		jointframe2++;
	}

	if ( animBits & ANIM_TY ) {
		jointframe1++;
		jointframe2++;
	}

	if ( animBits & ANIM_TZ ) {
		jointframe1++;
		jointframe2++;
	}

	idQuat q1;
	idQuat q2;

	switch( animBits & (ANIM_QX|ANIM_QY|ANIM_QZ) ) {
		case ANIM_QX:
			q1.x = idCompressedJointQuat::ShortToQuat( jointframe1[0] );
			q2.x = idCompressedJointQuat::ShortToQuat( jointframe2[0] );
			q1.y = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[1] );
			q2.y = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[1] );
			q1.z = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[2] );
			q2.z = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[2] );
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QY:
			q1.y = idCompressedJointQuat::ShortToQuat( jointframe1[0] );
			q2.y = idCompressedJointQuat::ShortToQuat( jointframe2[0] );
			q1.x = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[0] );
			q2.x = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[0] );
			q1.z = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[2] );
			q2.z = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[2] );
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QZ:
			q1.z = idCompressedJointQuat::ShortToQuat( jointframe1[0] );
			q2.z = idCompressedJointQuat::ShortToQuat( jointframe2[0] );
			q1.x = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[0] );
			q2.x = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[0] );
			q1.y = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[1] );
			q2.y = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[1] );
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QX|ANIM_QY:
			q1.x = idCompressedJointQuat::ShortToQuat( jointframe1[0] );
			q1.y = idCompressedJointQuat::ShortToQuat( jointframe1[1] );
			q2.x = idCompressedJointQuat::ShortToQuat( jointframe2[0] );
			q2.y = idCompressedJointQuat::ShortToQuat( jointframe2[1] );
			q1.z = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[2] );
			q2.z = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[2] );
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QX|ANIM_QZ:
			q1.x = idCompressedJointQuat::ShortToQuat( jointframe1[0] );
			q1.z = idCompressedJointQuat::ShortToQuat( jointframe1[1] );
			q2.x = idCompressedJointQuat::ShortToQuat( jointframe2[0] );
			q2.z = idCompressedJointQuat::ShortToQuat( jointframe2[1] );
			q1.y = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[1] );
			q2.y = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[1] );
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QY|ANIM_QZ:
			q1.y = idCompressedJointQuat::ShortToQuat( jointframe1[0] );
			q1.z = idCompressedJointQuat::ShortToQuat( jointframe1[1] );
			q2.y = idCompressedJointQuat::ShortToQuat( jointframe2[0] );
			q2.z = idCompressedJointQuat::ShortToQuat( jointframe2[1] );
			q1.x = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[0] );
			q2.x = idCompressedJointQuat::ShortToQuat( baseFrame[ 0 ].q[0] );
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QX|ANIM_QY|ANIM_QZ:
			q1.x = idCompressedJointQuat::ShortToQuat( jointframe1[0] );
			q1.y = idCompressedJointQuat::ShortToQuat( jointframe1[1] );
			q1.z = idCompressedJointQuat::ShortToQuat( jointframe1[2] );
			q2.x = idCompressedJointQuat::ShortToQuat( jointframe2[0] );
			q2.y = idCompressedJointQuat::ShortToQuat( jointframe2[1] );
			q2.z = idCompressedJointQuat::ShortToQuat( jointframe2[2] );
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
	}

	rotation.Slerp( q1, q2, frame.backlerp );
}

/*
====================
idMD5Anim::GetBounds
====================
*/
void idMD5Anim::GetBounds( idBounds &bnds, int time, int cyclecount ) const {
	frameBlend_t	frame;
	idVec3			offset;

	ConvertTimeToFrame( time, cyclecount, frame );

	bnds = bounds[ frame.frame1 ].ToBounds();
	bnds.AddBounds( bounds[ frame.frame2 ].ToBounds() );

	// origin position
	offset[0] = idCompressedJointQuat::ShortToOffset( baseFrame[ 0 ].t[0] );
	offset[1] = idCompressedJointQuat::ShortToOffset( baseFrame[ 0 ].t[1] );
	offset[2] = idCompressedJointQuat::ShortToOffset( baseFrame[ 0 ].t[2] );

	if ( jointInfo[ 0 ].animBits & ( ANIM_TX | ANIM_TY | ANIM_TZ ) ) {
		const short *componentPtr1 = &componentFrames[ numAnimatedComponents * frame.frame1 + jointInfo[ 0 ].firstComponent ];
		const short *componentPtr2 = &componentFrames[ numAnimatedComponents * frame.frame2 + jointInfo[ 0 ].firstComponent ];

		if ( jointInfo[ 0 ].animBits & ANIM_TX ) {
			offset.x = idCompressedJointQuat::ShortToOffset( *componentPtr1 ) * frame.frontlerp + idCompressedJointQuat::ShortToOffset( *componentPtr2 ) * frame.backlerp;
			componentPtr1++;
			componentPtr2++;
		}

		if ( jointInfo[ 0 ].animBits & ANIM_TY ) {
			offset.y = idCompressedJointQuat::ShortToOffset( *componentPtr1 ) * frame.frontlerp + idCompressedJointQuat::ShortToOffset( *componentPtr2 ) * frame.backlerp;
			componentPtr1++;
			componentPtr2++;
		}

		if ( jointInfo[ 0 ].animBits & ANIM_TZ ) {
			offset.z = idCompressedJointQuat::ShortToOffset( *componentPtr1 ) * frame.frontlerp + idCompressedJointQuat::ShortToOffset( *componentPtr2 ) * frame.backlerp;
		}
	}

	bnds[ 0 ] -= offset;
	bnds[ 1 ] -= offset;
}

/*
====================
idMD5Anim::GetInterpolatedFrame
====================
*/
void idMD5Anim::GetInterpolatedFrame( frameBlend_t &frame, idJointQuat *joints, const int *index, int numIndexes ) const {
	int						i, numLerpJoints;
	const short *			frame1;
	const short *			frame2;
	const short *			jointframe1;
	const short *			jointframe2;
	const jointAnimInfo_t *	infoPtr;
	int						animBits;
	idJointQuat *			blendJoints;
	idCompressedJointQuat *	compressedJoints;
	idCompressedJointQuat *	compressedBlendJoints;
	idCompressedJointQuat *	jointPtr;
	idCompressedJointQuat *	blendPtr;
	int *					lerpIndex;
	int *					baseIndex;

	// FIXME: have global static ?
	// index with all joints
	baseIndex = (int *)_alloca16( baseFrame.Num() * sizeof( baseIndex[ 0 ] ) );
	for ( i = 0; i < baseFrame.Num(); i++ ) {
		baseIndex[i] = i;
	}

	if ( !numAnimatedComponents ) {
		// just use the base frame
		SIMDProcessor->DecompressJoints( joints, baseFrame.Begin(), baseIndex, baseFrame.Num() );
		return;
	}

	compressedJoints = (idCompressedJointQuat *)_alloca16( baseFrame.Num() * sizeof( compressedJoints[0] ) );
	compressedBlendJoints = (idCompressedJointQuat *)_alloca16( baseFrame.Num() * sizeof( compressedBlendJoints[0] ) );

	SIMDProcessor->Memcpy( compressedJoints, baseFrame.Begin(), baseFrame.Num() * sizeof( compressedJoints[0] ) );

	lerpIndex = (int *)_alloca16( baseFrame.Num() * sizeof( lerpIndex[ 0 ] ) );
	numLerpJoints = 0;

	frame1 = &componentFrames[ frame.frame1 * numAnimatedComponents ];
	frame2 = &componentFrames[ frame.frame2 * numAnimatedComponents ];

	// delta decompression relative to base frame
	for ( i = 0; i < numIndexes; i++ ) {
		int j = index[i];
		infoPtr = &jointInfo[j];

		animBits = infoPtr->animBits;
		if ( animBits == 0 ) {
			continue;
		}

		jointPtr = &compressedJoints[j];
		blendPtr = &compressedBlendJoints[j];

		lerpIndex[numLerpJoints++] = j;

		jointframe1 = frame1 + infoPtr->firstComponent;
		jointframe2 = frame2 + infoPtr->firstComponent;

		switch( animBits & (ANIM_TX|ANIM_TY|ANIM_TZ) ) {
			case 0:
				blendPtr->t[0] = jointPtr->t[0];
				blendPtr->t[1] = jointPtr->t[1];
				blendPtr->t[2] = jointPtr->t[2];
				break;
			case ANIM_TX:
				jointPtr->t[0] = jointframe1[0];
				blendPtr->t[0] = jointframe2[0];
				blendPtr->t[1] = jointPtr->t[1];
				blendPtr->t[2] = jointPtr->t[2];
				jointframe1++;
				jointframe2++;
				break;
			case ANIM_TY:
				jointPtr->t[1] = jointframe1[0];
				blendPtr->t[1] = jointframe2[0];
				blendPtr->t[0] = jointPtr->t[0];
				blendPtr->t[2] = jointPtr->t[2];
				jointframe1++;
				jointframe2++;
				break;
			case ANIM_TZ:
				jointPtr->t[2] = jointframe1[0];
				blendPtr->t[2] = jointframe2[0];
				blendPtr->t[0] = jointPtr->t[0];
				blendPtr->t[1] = jointPtr->t[1];
				jointframe1++;
				jointframe2++;
				break;
			case ANIM_TX|ANIM_TY:
				jointPtr->t[0] = jointframe1[0];
				jointPtr->t[1] = jointframe1[1];
				blendPtr->t[0] = jointframe2[0];
				blendPtr->t[1] = jointframe2[1];
				blendPtr->t[2] = jointPtr->t[2];
				jointframe1 += 2;
				jointframe2 += 2;
				break;
			case ANIM_TX|ANIM_TZ:
				jointPtr->t[0] = jointframe1[0];
				jointPtr->t[2] = jointframe1[1];
				blendPtr->t[0] = jointframe2[0];
				blendPtr->t[2] = jointframe2[1];
				blendPtr->t[1] = jointPtr->t[1];
				jointframe1 += 2;
				jointframe2 += 2;
				break;
			case ANIM_TY|ANIM_TZ:
				jointPtr->t[1] = jointframe1[0];
				jointPtr->t[2] = jointframe1[1];
				blendPtr->t[1] = jointframe2[0];
				blendPtr->t[2] = jointframe2[1];
				blendPtr->t[0] = jointPtr->t[0];
				jointframe1 += 2;
				jointframe2 += 2;
				break;
			case ANIM_TX|ANIM_TY|ANIM_TZ:
				jointPtr->t[0] = jointframe1[0];
				jointPtr->t[1] = jointframe1[1];
				jointPtr->t[2] = jointframe1[2];
				blendPtr->t[0] = jointframe2[0];
				blendPtr->t[1] = jointframe2[1];
				blendPtr->t[2] = jointframe2[2];
				jointframe1 += 3;
				jointframe2 += 3;
				break;
		}

		switch( animBits & (ANIM_QX|ANIM_QY|ANIM_QZ) ) {
			case 0:
				blendPtr->q[0] = jointPtr->q[0];
				blendPtr->q[1] = jointPtr->q[1];
				blendPtr->q[2] = jointPtr->q[2];
				break;
			case ANIM_QX:
				jointPtr->q[0] = jointframe1[0];
				blendPtr->q[0] = jointframe2[0];
				blendPtr->q[1] = jointPtr->q[1];
				blendPtr->q[2] = jointPtr->q[2];
				break;
			case ANIM_QY:
				jointPtr->q[1] = jointframe1[0];
				blendPtr->q[1] = jointframe2[0];
				blendPtr->q[0] = jointPtr->q[0];
				blendPtr->q[2] = jointPtr->q[2];
				break;
			case ANIM_QZ:
				jointPtr->q[2] = jointframe1[0];
				blendPtr->q[2] = jointframe2[0];
				blendPtr->q[0] = jointPtr->q[0];
				blendPtr->q[1] = jointPtr->q[1];
				break;
			case ANIM_QX|ANIM_QY:
				jointPtr->q[0] = jointframe1[0];
				jointPtr->q[1] = jointframe1[1];
				blendPtr->q[0] = jointframe2[0];
				blendPtr->q[1] = jointframe2[1];
				blendPtr->q[2] = jointPtr->q[2];
				break;
			case ANIM_QX|ANIM_QZ:
				jointPtr->q[0] = jointframe1[0];
				jointPtr->q[2] = jointframe1[1];
				blendPtr->q[0] = jointframe2[0];
				blendPtr->q[2] = jointframe2[1];
				blendPtr->q[1] = jointPtr->q[1];
				break;
			case ANIM_QY|ANIM_QZ:
				jointPtr->q[1] = jointframe1[0];
				jointPtr->q[2] = jointframe1[1];
				blendPtr->q[1] = jointframe2[0];
				blendPtr->q[2] = jointframe2[1];
				blendPtr->q[0] = jointPtr->q[0];
				break;
			case ANIM_QX|ANIM_QY|ANIM_QZ:
				jointPtr->q[0] = jointframe1[0];
				jointPtr->q[1] = jointframe1[1];
				jointPtr->q[2] = jointframe1[2];
				blendPtr->q[0] = jointframe2[0];
				blendPtr->q[1] = jointframe2[1];
				blendPtr->q[2] = jointframe2[2];
				break;
		}
	}

	blendJoints = (idJointQuat *)_alloca16( baseFrame.Num() * sizeof( blendJoints[ 0 ] ) );

	SIMDProcessor->DecompressJoints( joints, compressedJoints, baseIndex, baseFrame.Num() );
	SIMDProcessor->DecompressJoints( blendJoints, compressedBlendJoints, lerpIndex, numLerpJoints );

	SIMDProcessor->BlendJoints( joints, blendJoints, frame.backlerp, lerpIndex, numLerpJoints );

	if ( frame.cycleCount ) {
		joints[ 0 ].t += totaldelta * ( float )frame.cycleCount;
	}
}

/*
====================
idMD5Anim::GetSingleFrame
====================
*/
void idMD5Anim::GetSingleFrame( int framenum, idJointQuat *joints, const int *index, int numIndexes ) const {
	int						i;
	const short *			frame;
	const short *			jointframe;
	int						animBits;
	idCompressedJointQuat *	compressedJoints;
	idCompressedJointQuat *	jointPtr;
	const jointAnimInfo_t *	infoPtr;
	int	*					baseIndex;

	// FIXME: have global static ?
	// index with all joints
	baseIndex = (int *)_alloca16( baseFrame.Num() * sizeof( baseIndex[ 0 ] ) );
	for ( i = 0; i < baseFrame.Num(); i++ ) {
		baseIndex[i] = i;
	}

	if ( ( framenum == 0 ) || !numAnimatedComponents ) {
		// just use the base frame
		SIMDProcessor->DecompressJoints( joints, baseFrame.Begin(), baseIndex, baseFrame.Num() );
		return;
	}

	compressedJoints = (idCompressedJointQuat *)_alloca16( baseFrame.Num() * sizeof( compressedJoints[0] ) );

	SIMDProcessor->Memcpy( compressedJoints, baseFrame.Begin(), baseFrame.Num() * sizeof( baseFrame[0] ) );

	frame = &componentFrames[ framenum * numAnimatedComponents ];

	// delta decompression relative to base frame
	for ( i = 0; i < numIndexes; i++ ) {
		int j = index[i];
		infoPtr = &jointInfo[j];

		animBits = infoPtr->animBits;
		if ( animBits == 0 ) {
			continue;
		}

		jointPtr = &compressedJoints[j];

		jointframe = frame + infoPtr->firstComponent;

		switch( animBits & (ANIM_TX|ANIM_TY|ANIM_TZ) ) {
			case 0:
				break;
			case ANIM_TX:
				jointPtr->t[0] = jointframe[0];
				jointframe++;
				break;
			case ANIM_TY:
				jointPtr->t[1] = jointframe[0];
				jointframe++;
				break;
			case ANIM_TZ:
				jointPtr->t[2] = jointframe[0];
				jointframe++;
				break;
			case ANIM_TX|ANIM_TY:
				jointPtr->t[0] = jointframe[0];
				jointPtr->t[1] = jointframe[1];
				jointframe += 2;
				break;
			case ANIM_TX|ANIM_TZ:
				jointPtr->t[0] = jointframe[0];
				jointPtr->t[2] = jointframe[1];
				jointframe += 2;
				break;
			case ANIM_TY|ANIM_TZ:
				jointPtr->t[1] = jointframe[0];
				jointPtr->t[2] = jointframe[1];
				jointframe += 2;
				break;
			case ANIM_TX|ANIM_TY|ANIM_TZ:
				jointPtr->t[0] = jointframe[0];
				jointPtr->t[1] = jointframe[1];
				jointPtr->t[2] = jointframe[2];
				jointframe += 3;
				break;
		}

		switch( animBits & (ANIM_QX|ANIM_QY|ANIM_QZ) ) {
			case 0:
				break;
			case ANIM_QX:
				jointPtr->q[0] = jointframe[0];
				break;
			case ANIM_QY:
				jointPtr->q[1] = jointframe[0];
				break;
			case ANIM_QZ:
				jointPtr->q[2] = jointframe[0];
				break;
			case ANIM_QX|ANIM_QY:
				jointPtr->q[0] = jointframe[0];
				jointPtr->q[1] = jointframe[1];
				break;
			case ANIM_QX|ANIM_QZ:
				jointPtr->q[0] = jointframe[0];
				jointPtr->q[2] = jointframe[1];
				break;
			case ANIM_QY|ANIM_QZ:
				jointPtr->q[1] = jointframe[0];
				jointPtr->q[2] = jointframe[1];
				break;
			case ANIM_QX|ANIM_QY|ANIM_QZ:
				jointPtr->q[0] = jointframe[0];
				jointPtr->q[1] = jointframe[1];
				jointPtr->q[2] = jointframe[2];
				break;
		}
	}

	SIMDProcessor->DecompressJoints( joints, compressedJoints, baseIndex, baseFrame.Num() );
}

/*
====================
idMD5Anim::CheckModelHierarchy
====================
*/
void idMD5Anim::CheckModelHierarchy( const idRenderModel *model ) const {
	int	i;
	int	jointNum;
	int	parent;

	if ( jointInfo.Num() != model->NumJoints() ) {
		gameLocal.Error( "Model '%s' has different # of joints than anim '%s'", model->Name(), name.c_str() );
	}

	const idMD5Joint *modelJoints = model->GetJoints();
	for( i = 0; i < jointInfo.Num(); i++ ) {
		jointNum = jointInfo[ i ].nameIndex;
		if ( modelJoints[ i ].name != animationLib.JointName( jointNum ) ) {
			gameLocal.Error( "Model '%s''s joint names don't match anim '%s''s", model->Name(), name.c_str() );
		}
		if ( modelJoints[ i ].parent ) {
			parent = modelJoints[ i ].parent - modelJoints;
		} else {
			parent = -1;
		}
		if ( parent != jointInfo[ i ].parentNum ) {
			gameLocal.Error( "Model '%s' has different joint hierarchy than anim '%s'", model->Name(), name.c_str() );
		}
	}
}

/***********************************************************************

	idAnimManager

***********************************************************************/

/*
====================
idAnimManager::idAnimManager
====================
*/
idAnimManager::idAnimManager() {
}

/*
====================
idAnimManager::~idAnimManager
====================
*/
idAnimManager::~idAnimManager() {
	Shutdown();
}

/*
====================
idAnimManager::Shutdown
====================
*/
void idAnimManager::Shutdown( void ) {
	animations.DeleteContents();
	jointnames.Clear();
	jointnamesHash.Free();
}


/*
==============
idMD5Anim::LoadAnimBinary
==============
*/
bool idMD5Anim::LoadAnimBinary( const char *filename ) {
	int ident, version, num;

	idFile* file = fileSystem->OpenFileRead( filename );
	if ( file == NULL ) {
//		common->Warning( "Couldn't load binary anim, %s", filename );
		return false;
	}

#if defined( SD_BUFFERED_FILE_LOADS )
	file = fileSystem->OpenBufferedFile( file );
#endif

	Free();

	file->ReadInt( ident );
	if ( ident != ANIMB_IDENT ) {
		common->Warning( "idMD5Anim::LoadAnimBinary : unknown fileid on '%s'", filename );
		return false;
	}

	file->ReadInt( version );
	if ( version != ANIMB_VERSION ) {
		common->Warning( "idMD5Anim::LoadAnimBinary : wrong version on '%s' (%i should be %i)", filename, version, ANIMB_VERSION );
		return false;
	}

	file->ReadInt( numFrames );
	file->ReadInt( frameRate );
	file->ReadInt( animLength );
	file->ReadInt( numJoints );
	file->ReadInt( numAnimatedComponents );

	file->ReadInt( num );
	bounds.SetGranularity( 1 );
	bounds.SetNum( num );
	for ( int i=0; i<num; i++ ) {
		short list[6];

		file->ReadShort( list[0] );
		file->ReadShort( list[1] );
		file->ReadShort( list[2] );
		file->ReadShort( list[3] );
		file->ReadShort( list[4] );
		file->ReadShort( list[5] );

		bounds[i].SetBounds( list );
	}

	file->ReadInt( num );
	jointInfo.SetGranularity( 1 );
	jointInfo.SetNum( num );
	idStr temp;
	for ( int i=0; i<num; i++ ) {
		file->ReadString( temp );
		jointInfo[i].nameIndex = animationLib.JointIndex( temp );

		file->ReadShort( jointInfo[i].parentNum );
		file->ReadShort( jointInfo[i].animBits );
		file->ReadShort( jointInfo[i].firstComponent );
	}

	file->ReadInt( num );
	baseFrame.SetGranularity( 1 );
	baseFrame.SetNum( num );
	for ( int i=0; i<num; i++ ) {
		file->ReadShort( baseFrame[i].q[0] );
		file->ReadShort( baseFrame[i].q[1] );
		file->ReadShort( baseFrame[i].q[2] );
		file->ReadShort( baseFrame[i].t[0] );
		file->ReadShort( baseFrame[i].t[1] );
		file->ReadShort( baseFrame[i].t[2] );
	}

	file->ReadInt( num );
	componentFrames.SetGranularity( 1 );
	componentFrames.SetNum( num );
	for ( int i=0; i<num; i++ ) {
		file->ReadShort( componentFrames[i] );
	}

	file->ReadString( name );
	file->ReadVec3( totaldelta );

	fileSystem->CloseFile( file );

	if ( numFrames > 4 && numAnimatedComponents && anim_reduced.GetBool() ) {
		Resample();
	}

	return true;
}

/*
==============
idMD5Anim::WriteAnimBinary
==============
*/
bool idMD5Anim::WriteAnimBinary( const char *filename ) {
	int num;

	idStr str = filename;
	str.StripFileExtension();
	str = str + ".animb";

	idFile* file = fileSystem->OpenFileWrite( str.c_str(), "fs_savepath" );
	if ( file == NULL ) {
		return false;
	}

	file->WriteInt( ANIMB_IDENT );
	file->WriteInt( ANIMB_VERSION );

	file->WriteInt( numFrames );
	file->WriteInt( frameRate );
	file->WriteInt( animLength );
	file->WriteInt( numJoints );
	file->WriteInt( numAnimatedComponents );

	num = bounds.Num();
	file->WriteInt( num );
	for ( int i = 0; i < num; i++ ) {
		const short *list = bounds[i].GetBounds();

		file->WriteShort( list[0] );
		file->WriteShort( list[1] );
		file->WriteShort( list[2] );
		file->WriteShort( list[3] );
		file->WriteShort( list[4] );
		file->WriteShort( list[5] );
	}

	num = jointInfo.Num();
	file->WriteInt( num );
	for ( int i=0; i<num; i++ ) {
		jointAnimInfo_t animInfo = jointInfo[i];

		file->WriteString( animationLib.JointName( animInfo.nameIndex ) );

		file->WriteShort( animInfo.parentNum );
		file->WriteShort( animInfo.animBits );
		file->WriteShort( animInfo.firstComponent );
	}

	num = baseFrame.Num();
	file->WriteInt( num );
	for ( int i=0; i<num; i++ ) {
		idCompressedJointQuat jointQuat = baseFrame[i];

		file->WriteShort( jointQuat.q[0] );
		file->WriteShort( jointQuat.q[1] );
		file->WriteShort( jointQuat.q[2] );
		file->WriteShort( jointQuat.t[0] );
		file->WriteShort( jointQuat.t[1] );
		file->WriteShort( jointQuat.t[2] );
	}

	num = componentFrames.Num();
	file->WriteInt( num );
	for ( int i=0; i<num; i++ ) {
		file->WriteShort( componentFrames[i] );
	}

	file->WriteString( name );
	file->WriteVec3( totaldelta );

	fileSystem->CloseFile( file );

	return true;
}

/*
====================
idAnimManager::GetAnim
====================
*/
idMD5Anim *idAnimManager::GetAnim( const char *name ) {
	idMD5Anim **animptrptr;
	idMD5Anim *anim;
	bool loaded = false;

	// see if it has been asked for before
	animptrptr = NULL;
	if ( animations.Get( name, &animptrptr ) ) {
		anim = *animptrptr;
	} else {
		idStr extension;
		idStr filename = name;

		filename.ExtractFileExtension( extension );
		if ( extension != MD5_ANIM_EXT ) {
			return NULL;
		}

		anim = new idMD5Anim();

		if ( r_loadAnimB.GetBool() ) {
			idStr animbName = va( PREGENERATED_BASEDIR "/animb/%s", name );
			animbName.StripFileExtension();
			animbName = animbName + ".animb";

			loaded = anim->LoadAnimBinary( animbName );
		}
		if ( !loaded ) {
			if ( !anim->LoadAnim( filename ) ) {
				gameLocal.Warning( "Couldn't load anim: '%s'", filename.c_str() );
				delete anim;
				anim = NULL;
			}
		}

        if ( r_writeAnimB.GetBool() && anim ) {
			// Write binary file
			idStr fullPath, relativePath;
			relativePath = va( PREGENERATED_BASEDIR "/animb/%s", name );

			anim->WriteAnimBinary( relativePath );
        }

		animations.Set( filename, anim );
	}

	return anim;
}

/*
================
idAnimManager::ReloadAnims
================
*/
void idAnimManager::ReloadAnims( void ) {
	int			i;
	idMD5Anim	**animptr;

	for ( i = 0; i < animations.Num(); i++ ) {
		animptr = animations.GetIndex( i );
		if ( animptr && *animptr ) {
			( *animptr )->Reload();
		}
	}
}

/*
================
idAnimManager::JointIndex
================
*/
int	idAnimManager::JointIndex( const char *name ) {
	int i, hash;

	hash = jointnamesHash.GenerateKey( name );
	for ( i = jointnamesHash.GetFirst( hash ); i != -1; i = jointnamesHash.GetNext( i ) ) {
		if ( jointnames[i].Cmp( name ) == 0 ) {
			return i;
		}
	}

	i = jointnames.Append( name );
	jointnamesHash.Add( hash, i );
	return i;
}

/*
================
idAnimManager::JointName
================
*/
const char *idAnimManager::JointName( int index ) const {
	return jointnames[ index ];
}

/*
================
idAnimManager::ListAnims
================
*/
void idAnimManager::ListAnims( void ) const {
	int					i;
	idMD5Anim* const*	animptr;
	idMD5Anim*			anim;
	size_t				size;
	size_t				s;
	size_t				namesize;
	int					num;

	num = 0;
	size = 0;
	for ( i = 0; i < animations.Num(); i++ ) {
		animptr = animations.GetIndex( i );
		if ( animptr && *animptr ) {
			anim = *animptr;
			s = anim->Size();
			gameLocal.Printf( "%8d bytes : %2d refs : %s\n", s, anim->NumRefs(), anim->Name() );
			size += s;
			num++;
		}
	}

	namesize = jointnames.Size() + jointnamesHash.Size();
	for( i = 0; i < jointnames.Num(); i++ ) {
		namesize += jointnames[ i ].Size();
	}

	gameLocal.Printf( "\n%d memory used in %d anims\n", size, num );
	gameLocal.Printf( "%d memory used in %d joint names\n", namesize, jointnames.Num() );
}

/*
================
idAnimManager::FlushUnusedAnims
================
*/
void idAnimManager::FlushUnusedAnims( void ) {
	int						i;
	idMD5Anim				**animptr;
	idList<idMD5Anim *>		removeAnims;
	
	for ( i = 0; i < animations.Num(); i++ ) {
		animptr = animations.GetIndex( i );
		if ( animptr && *animptr ) {
			if ( ( *animptr )->NumRefs() <= 0 ) {
				removeAnims.Append( *animptr );
			}
		}
	}

	for( i = 0; i < removeAnims.Num(); i++ ) {
		animations.Remove( removeAnims[ i ]->Name() );
		delete removeAnims[ i ];
	}
}
