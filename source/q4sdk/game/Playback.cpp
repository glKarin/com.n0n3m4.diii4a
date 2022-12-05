#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "ai/AI.h"

#define RECORD_STATE_TRACE_LEN		2048.0f

class rvGamePlayback
{
public:
							rvGamePlayback( void );
							~rvGamePlayback( void );

			void			RecordData( const usercmd_t &cmd, idEntity *source );
private:
			int				mStartTime;
			byte			mOldFlags;
			idStr			mName;
			idClipModel		*mClipModel;
			rvDeclPlayback	*mPlayback;
};

idCVar g_recordPlayback( "g_recordPlayback", "0", CVAR_GAME | CVAR_INTEGER | CVAR_NOCHEAT, "record the player movement in a playback" );
idCVar g_playPlayback( "g_playPlayback", "0", CVAR_GAME | CVAR_INTEGER | CVAR_NOCHEAT, "plays the current playback in a camera path" );

rvGamePlayback		*gamePlayback = NULL;
rvCameraPlayback	*playbackCamera = NULL;

rvGamePlayback::rvGamePlayback( void )
{
	idStr			newName;
	const idVec3 trace_mins( -1.0f, -1.0f, -1.0f );
	const idVec3 trace_maxs( 1.0f, 1.0f, 1.0f );
	const idBounds trace_bounds( trace_mins, trace_maxs );
	idTraceModel	traceModel( trace_bounds );
		
	mStartTime = gameLocal.time;
	mOldFlags = 0;
	mClipModel = new idClipModel( traceModel );

	if( !g_currentPlayback.GetInteger() )
	{
		newName = declManager->GetNewName( DECL_PLAYBACK, "playbacks/untitled" );
		mPlayback = ( rvDeclPlayback * )declManager->CreateNewDecl( DECL_PLAYBACK, newName, newName + ".playback" );
		mPlayback->ReplaceSourceFileText();
		mPlayback->Invalidate();

		g_currentPlayback.SetInteger( mPlayback->Index() );
	}
	else
	{
		mPlayback = ( rvDeclPlayback * )declManager->PlaybackByIndex( g_currentPlayback.GetInteger() );
	}

	declManager->StartPlaybackRecord( mPlayback );
	common->Printf( "Starting playback record to %s type %d\n", mPlayback->GetName(), g_recordPlayback.GetInteger() );
}

rvGamePlayback::~rvGamePlayback( void )
{
	declManager->FinishPlayback( mPlayback );
	delete mClipModel;

	common->Printf( "Stopping playback play/record\n" );
}

void rvGamePlayback::RecordData( const usercmd_t &cmd, idEntity *source )
{
	idPlayer			*player;
	trace_t				trace;
	idMat3				axis;
	idVec3				start, end;
	rvDeclPlaybackData	info;

	info.Init();

	switch( g_recordPlayback.GetInteger() )
	{
	case 1:
		info.SetPosition( source->GetPhysics()->GetOrigin() );
		info.SetAngles( source->GetPhysics()->GetAxis().ToAngles() );
		break;

	case 2:
		gameLocal.GetPlayerView( start, axis );
		
		end = start + axis[0] * RECORD_STATE_TRACE_LEN;

		gameLocal.Translation( gameLocal.GetLocalPlayer(), trace, start, end, mClipModel, mat3_identity, CONTENTS_SOLID | CONTENTS_RENDERMODEL, source );

		if( trace.fraction != 1.0f ) 
		{
			info.SetPosition( trace.endpos );
			info.SetAngles( trace.c.normal.ToAngles() );
		}
		break;

	case 3:
		assert( source->IsType( idPlayer::GetClassType() ) );
		if( source->IsType( idPlayer::GetClassType() ) )
		{
			player = static_cast<idPlayer *>( source );
			info.SetPosition( player->GetEyePosition() ); 
			info.SetAngles( source->GetPhysics()->GetAxis().ToAngles() );
		}
		break;
	}

	// Record buttons
	info.SetButtons( cmd.buttons );

	// Record impulses
	if( ( cmd.flags & UCF_IMPULSE_SEQUENCE ) != ( mOldFlags & UCF_IMPULSE_SEQUENCE ) )
	{
		info.SetImpulse( cmd.impulse );
	}
	else
	{
		info.SetImpulse( 0 );
	}
	mOldFlags = cmd.flags;

	declManager->SetPlaybackData( mPlayback, gameLocal.time - mStartTime, -1, &info );
}

// ================================================================================================

void idGameEdit::DrawPlaybackDebugInfo( void )
{
	int						duration, time;
	rvDeclPlaybackData		pbd, pbdOld;
	const rvDeclPlayback	*pb;

	pb = declManager->PlaybackByIndex( g_currentPlayback.GetInteger(), true );
	if( pb )
	{
		duration = SEC2MS( pb->GetDuration() );
		pbd.Init();
		pbdOld.Init();

		declManager->GetPlaybackData( pb, -1, 0, 0, &pbdOld );
		for( time = gameLocal.GetMSec(); time < duration; time += gameLocal.GetMSec() * g_showPlayback.GetInteger() )
		{
			declManager->GetPlaybackData( pb, -1, time, time, &pbd );
			gameRenderWorld->DebugArrow( colorGreen, pbdOld.GetPosition(), pbd.GetPosition(), 2 );
			pbdOld = pbd;
		}

		gameRenderWorld->DebugBounds( colorRed, pb->GetBounds(), pb->GetOrigin() );
	}
}

void idGameEdit::RecordPlayback( const usercmd_t &cmd, idEntity *source )
{
	// Not recording - so instantly exit
	if( !g_recordPlayback.GetInteger() && !gamePlayback ) 
	{
		return;
	}

	if( !gamePlayback )
	{
		gamePlayback = new rvGamePlayback();
	}

	if( g_recordPlayback.GetInteger() )
	{
		gamePlayback->RecordData( cmd, source );
	}
	else
	{
		delete gamePlayback;
		gamePlayback = NULL;
	}
}

// ================================================================================================

bool idGameEdit::PlayPlayback( void )
{
	// Not playing - so instantly exit
	if( !g_playPlayback.GetInteger() && !playbackCamera ) 
	{
		return( false );
	}

	if( !playbackCamera )
	{
		playbackCamera = static_cast<rvCameraPlayback *>( gameLocal.SpawnEntityType( rvCameraPlayback::GetClassType() ) );
		SetCamera( playbackCamera );

		common->Printf( "Starting playback play\n" );
	}

	if( g_currentPlayback.IsModified() )
	{
		// Spawn is a misnomer - it should be init with new data
		playbackCamera->Spawn();
		g_currentPlayback.ClearModified();
	}

	if( !g_playPlayback.GetInteger() )
	{
		playbackCamera->PostEventMS( &EV_Remove, 0 );
		playbackCamera = NULL;
		SetCamera( NULL );
	}

	return( true );
}

// ================================================================================================

void idGameEdit::ShutdownPlaybacks( void )
{
	g_recordPlayback.SetInteger( 0 );
	g_playPlayback.SetInteger( 0 );

	if( gamePlayback )
	{
		delete gamePlayback;
		gamePlayback = NULL;
	}

	if( playbackCamera )
	{
		playbackCamera->PostEventMS( &EV_Remove, 0 );
		playbackCamera = NULL;
		SetCamera( NULL );
	}
}

// ================================================================================================

/*
============
rvPlaybackDriver::Start

Start a new playback automatically blending with the old playback (if any) over numFrames
============
*/
bool rvPlaybackDriver::Start( const char *playback, idEntity *owner, int flags, int numFrames )
{
	idVec3	startPos;

	if( !idStr::Length( playback ) )
	{
		mPlaybackDecl = NULL;
		mOldPlaybackDecl = NULL;
		return( true );
	}

	const rvDeclPlayback *pb = declManager->FindPlayback( playback );

	if( g_showPlayback.GetInteger() )
	{
		common->Printf( "Starting playback: %s\n", pb->GetName() );
	}

	mOldPlaybackDecl = mPlaybackDecl;
	mOldFlags = mFlags;
	mOldStartTime = mStartTime;
	mOldOffset = mOffset;

	mPlaybackDecl = pb;
	mFlags = flags;
	mStartTime = gameLocal.time;
	mOffset.Zero();

	if( flags & PBFL_RELATIVE_POSITION )
	{
		mOffset = owner->GetPhysics()->GetOrigin() - pb->GetOrigin();
	}

	mTransitionTime = numFrames * gameLocal.GetMSec();
	return( true );
}

/*
============
PlaybackCallback

Called whenever a button up or down, or an impulse event is found while getting the playback data
============
*/
void PlaybackCallback( int type, float time, const void *data )
{
	const rvDeclPlaybackData *pbd = ( const rvDeclPlaybackData * )data;
	idEntity *ent = pbd->GetEntity();

	ent->PostEventSec( &EV_PlaybackCallback, time, type, pbd->GetChanged(), pbd->GetImpulse() );
}

/*
============
rvPlaybackDriver::UpdateFrame

Blend two playbacks together 
============
*/
bool rvPlaybackDriver::UpdateFrame( idEntity *ent, rvDeclPlaybackData &out )
{
	rvDeclPlaybackData	pbd, oldPbd;
	float				blend, invBlend;
	idStr				ret;
	bool				expired, oldExpired;
	
	// Get the current playback position
	pbd.Init();
	pbd.SetCallback( ent, PlaybackCallback );
	expired = declManager->GetPlaybackData( mPlaybackDecl, mFlags, gameLocal.time - mStartTime, mLastTime - mStartTime, &pbd );
	pbd.SetPosition( pbd.GetPosition() + mOffset );

	// Get the playback data we are merging from
	oldPbd.Init();
	oldExpired = declManager->GetPlaybackData( mOldPlaybackDecl, mOldFlags, gameLocal.time - mOldStartTime, gameLocal.time - mOldStartTime, &oldPbd );
	oldPbd.SetPosition( oldPbd.GetPosition() + mOldOffset );

	mLastTime = gameLocal.time;

	if( g_showPlayback.GetInteger() && mPlaybackDecl )
	{
		common->Printf( "Running playback: %s at %.1f\n", mPlaybackDecl->GetName(), MS2SEC( gameLocal.time - mStartTime ) );
	}

	// Fully merged - so delete the old one
	if( gameLocal.time > mStartTime + mTransitionTime )
	{
		oldExpired = true;
	}

	// Interpolate the result
	if( expired && oldExpired )
	{
		out.Init();
		mPlaybackDecl = NULL;
		mOldPlaybackDecl = NULL;
	}
	else if( !expired && oldExpired )
	{
		out = pbd;
		mOldPlaybackDecl = NULL;
	}
	else if( expired && !oldExpired )
	{
		out = oldPbd;
		mPlaybackDecl = NULL;
	}
	else
	{
		// Linear zero to one
		blend = idMath::ClampFloat( 0.0f, 1.0f, ( gameLocal.time - mStartTime ) / ( float )mTransitionTime );
		
		// Sinusoidal
		blend = idMath::Sin( blend * idMath::HALF_PI );
		invBlend = 1.0f - blend;

		out.SetPosition( blend * pbd.GetPosition() + invBlend * oldPbd.GetPosition() );
		out.SetAngles( blend * pbd.GetAngles() + invBlend * oldPbd.GetAngles() );
		out.SetButtons( pbd.GetButtons() );
		out.SetImpulse( pbd.GetImpulse() );
	}

	return( expired );
}

// cnicholson: Begin  Added save/restore functionality
/*
============
rvPlaybackDriver::Save

Save all member vars for save/load games
============
*/
void rvPlaybackDriver::Save( idSaveGame *savefile ) const {

	savefile->WriteInt( mLastTime );		// cnicholson: Added unsaved var
	savefile->WriteInt( mTransitionTime );	// cnicholson: Added unsaved var

	savefile->WriteInt( mStartTime );		// cnicholson: Added unsaved var
	savefile->WriteInt( mFlags );			// cnicholson: Added unsaved var
	// TOSAVE: const rvDeclPlayback	*mPlaybackDecl;
	savefile->WriteVec3( mOffset );			// cnicholson: Added unsaved var

	savefile->WriteInt( mOldStartTime );	// cnicholson: Added unsaved var
	savefile->WriteInt( mOldFlags );		// cnicholson: Added unsaved var
	// TOSAVE: const rvDeclPlayback	*mOldPlaybackDecl;
	savefile->WriteVec3( mOldOffset );		// cnicholson: Added unsaved var
}

/*
============
rvPlaybackDriver::Restore

Restore all member vars for save/load games
============
*/
void rvPlaybackDriver::Restore( idRestoreGame *savefile ) {

	savefile->ReadInt( mLastTime );			// cnicholson: Added unrestored var
	savefile->ReadInt( mTransitionTime );	// cnicholson: Added unrestored var

	savefile->ReadInt( mStartTime );		// cnicholson: Added unrestored var
	savefile->ReadInt( mFlags );			// cnicholson: Added unrestored var
	// TOSAVE: const rvDeclPlayback	*mPlaybackDecl;
	savefile->ReadVec3( mOffset );			// cnicholson: Added unrestored var

	savefile->ReadInt( mOldStartTime );		// cnicholson: Added unrestored var
	savefile->ReadInt( mOldFlags );			// cnicholson: Added unrestored var
	// TOSAVE: const rvDeclPlayback	*mOldPlaybackDecl;
	savefile->ReadVec3( mOldOffset );		// cnicholson: Added unrestored var
}
// cnicholson: End  Added save/restore functionality

// end

