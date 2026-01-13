#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

// 1	normal
// 2	scared
// 3	surprised
// 4	panicked
// 5	angry
// 6	suspicious with rt eyelid raised
// 7	suspicious with lft eyelid raised
// 8	curious
// 9	tired
// 10	happy

idCVar fas_debug( "fas_debug", "0", CVAR_INTEGER, "debug info for facial animation system" );
idCVar fas_threshhold0( "fas_threshhold0", "60", CVAR_INTEGER, "intensity required to use frame set 0" );
idCVar fas_threshhold1( "fas_threshhold1", "30", CVAR_INTEGER, "intensity required to use frame set 1" );
idCVar fas_blendBias( "fas_blendBias", "1.5", 0, "multiplier to the per phoneme blend time" );
idCVar fas_intensityBias( "fas_intensityBias", "0", CVAR_INTEGER, "bias applied to the intensity of the phoneme when trying to extract the viseme" );
idCVar fas_timeOffset( "fas_timeOffset", "50", CVAR_INTEGER, "ms offset to the viseme frame" );

idStr					phonemeFile;
idHashTable<rvViseme>	*visemeTable100;
idHashTable<rvViseme>	*visemeTable66;
idHashTable<rvViseme>	*visemeTable33;

/*
================
rvViseme::Init
================
*/
void rvViseme::Init( idStr &phon, int f, int bt )
{
	phoneme = phon;
	frame = f;
	blendTime = bt;
}

/*
================
FAS_LoadPhonemes

Load in the the file that cross references phonemes with visemes.
================
*/
bool FAS_LoadPhonemes( const char *visemes )
{
	idStr		visemeFile;
	rvViseme	viseme;
	idLexer		lexer;
	idToken		token;
	idStr		phoneme;
	int			frame, blendTime, intensity;

	phonemeFile = visemes;
	visemeTable100->Clear();
	visemeTable66->Clear();
	visemeTable33->Clear();

	common->Printf( "Loading viseme file: %s\n", visemes );

	visemeFile = "lipsync/";
	visemeFile += visemes;
	visemeFile += ".viseme";

	lexer.SetFlags( DECL_LEXER_FLAGS );
	lexer.LoadFile( visemeFile );

	if( !lexer.ExpectTokenString( "visemes" ) )
	{
		return( false );
	}

	if( !lexer.ExpectTokenString( "{" ) )
	{
		return( false );
	}

	while( true )
	{
		if( !lexer.ReadToken( &token ) )
		{
			return( false );
		}

		if( token == "}" )
		{
			break;
		}

		phoneme = token;
		lexer.ExpectTokenString( "," );
		frame = lexer.ParseInt();
		lexer.ExpectTokenString( "," );
		blendTime = lexer.ParseInt();
		lexer.ExpectTokenString( "," );
		intensity = lexer.ParseInt();

		viseme.Init( phoneme, frame, blendTime );

		if( intensity > fas_threshhold0.GetInteger() )
		{
			visemeTable100->Set( phoneme, viseme );
		}
		else if( intensity > fas_threshhold1.GetInteger() )
		{
			visemeTable66->Set( phoneme, viseme );
		}
		else
		{
			visemeTable33->Set( phoneme, viseme );
		}
	}

	return( true );
}

/*
================
rvLipSyncData
================
*/
rvLipSyncData::rvLipSyncData( const rvDeclLipSync *ls, int time ) 
{
	const char *lsd = ls->GetLipSyncData();

	mLexer.SetFlags( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWNUMBERNAMES );
	mLexer.LoadMemory( lsd, idStr::Length( lsd ), ls->GetName() ); 
	mFlags = 0;
	mFrame = 0;
	mBlendTime = 0;
	mEmotion = "idle";
	mNextTokenTime = time;
}

void rvLipSyncData::SetFrame( int frame )
{
	mLastFrame = mFrame;
	mFrame = frame;
	mVisemeStartTime = mNextTokenTime;
}

float rvLipSyncData::GetFrontLerp( void )
{
	float lerp = 1.0f - idMath::ClampFloat( 0.0f, 1.0f, ( float )( gameLocal.GetTime() + fas_timeOffset.GetInteger() - mVisemeStartTime ) / ( float )mBlendTime );
	return( lerp );
}

/*
================
FAS_StartVisemeExtraction

Use a lexer to extract the phoneme data. This is a pretty slow but safe way.
There shouldn't be much in the way of multiple lip syncs going on, and if there are, it should be in a non performance critical cinematic.
================
*/
rvLipSyncData *FAS_StartVisemeExtraction( const rvDeclLipSync *ls, int time )
{
	rvLipSyncData	*lsd;

	lsd = new rvLipSyncData( ls, time );
		
	return( lsd );
}

/*
================
FAS_EndVisemeExtraction

Delete the workspace. FAS could use a void pointer if it wanted
================
*/
void FAS_EndVisemeExtraction( rvLipSyncData *lsd )
{
	delete lsd;
}

/*
================
FAS_ExtractViseme

Extract the correct viseme frame from the lexer
================
*/
void FAS_ExtractViseme( rvLipSyncData *lsd, int time )
{
	idToken			token;
	rvViseme		*viseme;
	idStr			phoneme, duration;
	int				index, intensity;

	// Make sure not to return any garbage
	lsd->ClearFlags();

	// Grab all the visemes, phrases and emotions until we are current
	while( lsd->Ready( time ) )
	{
		if( !lsd->ReadToken( &token ) )
		{
			lsd->SetFlags( FAS_ENDED );
			return;
		}

		if( token == "<" )
		{
			// Extract phrase
			if( !lsd->ReadToken( &token ) )
			{
				common->Printf( "Failed to parse phrase from phoneme string\n" );
				lsd->SetFlags( FAS_ENDED );
				return;
			}

			lsd->SetLastPhrase( token );
			lsd->ExpectTokenString( ">" );

			lsd->SetFlags( FAS_NEW_PHRASE ); 
		}
		else if( token == "{" )
		{
			// Extract emotion
			if( !lsd->ReadToken( &token ) )
			{
				common->Printf( "Failed to parse emotion from phoneme string\n" );
				lsd->SetFlags( FAS_ENDED );
				return;
			}

			lsd->SetEmotion( token );
			lsd->ExpectTokenString( "}" );

			lsd->SetFlags( FAS_NEW_EMOTION );
		}
		else
		{
			// Extract phoneme data
			index = 0;
			phoneme = idStr( token[index] );
			if( isupper( phoneme[0] ) )
			{
				index++;
				phoneme += token[index];
			}
	
			// Extract duration
			index++;

			duration = idStr( token[index] );
			index++;
			if( isdigit( token[index] ) )
			{
				duration += token[index];
				index++;
			}

			// Extract intensity
			intensity = ( token[index] - 'a' ) * 4;
			intensity += fas_intensityBias.GetInteger();

			// Extract the viseme data for the selected viseme
			viseme = NULL;

			if( intensity > fas_threshhold0.GetInteger() )
			{
				visemeTable100->Get( phoneme, &viseme );
			}
			if( intensity > fas_threshhold1.GetInteger() )
			{
				visemeTable66->Get( phoneme, &viseme );
			}
			else
			{
				visemeTable33->Get( phoneme, &viseme );
			}

			if( !viseme )
			{
				common->Printf( "FAS: Failed to find phoneme %s intensity %d", phoneme.c_str(), intensity );
				lsd->SetFlags( FAS_ENDED );
				return;
			}

			lsd->SetFrame( viseme->GetFrame() );
			lsd->SetNextTokenTime( atol( duration ) * 10 );
			lsd->SetBlendTime( int( viseme->GetBlendTime() * fas_blendBias.GetFloat() ) );
			lsd->SetFlags( FAS_NEW_VISEME );
		}
	}
}

/*
================
FAS_Reload_f
================
*/
void FAS_Reload_f( const idCmdArgs &args )
{
	// mekberg: disable non pre-cached warnings
	fileSystem->SetIsFileLoadingAllowed( true );

	FAS_LoadPhonemes( phonemeFile.c_str() );

	fileSystem->SetIsFileLoadingAllowed( false );
}

/*
================
FAS_Init
================
*/
bool FAS_Init( const char *visemes )
{
	// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG( tag, MA_ANIM );

	cmdSystem->AddCommand( "reloadFAS", FAS_Reload_f, CMD_FL_SYSTEM, "reloads the viseme data" );
	return( FAS_LoadPhonemes( visemes ) );
}

/*
================
FAS_Shutdown
================
*/
void FAS_Shutdown( void )
{
	phonemeFile.Clear();
	if ( visemeTable100 ) {
		visemeTable100->Clear();
		visemeTable66->Clear();
		visemeTable33->Clear();
	}

	cmdSystem->RemoveCommand( "reloadFAS" );
}

/*
================
idAFAttachment::EndLipSyncing
================
*/
void idAFAttachment::EndLipSyncing( void )
{
	frameBlend_t frameBlend = { 0, 0, 0, 1.0f, 0.0f };
	animator.SetFrame( ANIMCHANNEL_TORSO, lipSyncAnim, frameBlend );

	animator.CycleAnim( ANIMCHANNEL_HEAD, animator.GetAnim( "emotion_idle" ), gameLocal.time, 200 );
	animator.CycleAnim( ANIMCHANNEL_EYELIDS, animator.GetAnim( "emotion_idle" ), gameLocal.time, 200 );

	FAS_EndVisemeExtraction( lipSyncData );
	lipSyncData = NULL;
}

/*
================
idAFAttachment::StartLipSyncing
================
*/
int idAFAttachment::StartLipSyncing( const char *speechDecl ) 
{
	int		length;

	length = 0;

	// Clean up any spurious data
	EndLipSyncing();

	// Start a new lipsync if there is one
	if( speechDecl[0] ) 
	{
		const rvDeclLipSync	*lipSync;
		int					emotion;
		idStr				anim;

        lipSync = declManager->FindLipSync( speechDecl );
		lipSyncData = FAS_StartVisemeExtraction( lipSync, gameLocal.GetTime() );

		// Output debug info
		if( lipSync->GetDescription() && fas_debug.GetInteger() ) 
		{
			gameLocal.Printf( "Name: %s\n", speechDecl );
			gameLocal.Printf( "Sub: %s\n", lipSync->GetDescription().c_str() );
			gameLocal.Printf( "Lip: %s\n", lipSync->GetLipSyncData() );
		}

		// Start the associated sound
		refSound.diversity = 0.0f; 
		renderEntity.referenceSoundHandle = refSound.referenceSoundHandle;
		StartSoundShader( declManager->FindSound( lipSync->GetName() ), SND_CHANNEL_VOICE, refSound.parms.soundShaderFlags | SSF_IS_VO, false, &length );

		// Start the default emotion
		anim = "emotion_";
		anim += lipSyncData->GetEmotion();
		emotion = animator.GetAnim( anim );
		animator.CycleAnim( ANIMCHANNEL_HEAD, emotion, gameLocal.time, 200 );
		animator.CycleAnim( ANIMCHANNEL_EYELIDS, emotion, gameLocal.time, 200 );
	}

	return( length );
}

/*
================
idAFAttachment::HandleLipSync
================
*/
void idAFAttachment::HandleLipSync( void )
{
	idStr	anim;
	int		emotion;

	if( !lipSyncData )
	{
		return;
	}

	FAS_ExtractViseme( lipSyncData, gameLocal.GetTime() + fas_timeOffset.GetInteger() );
	if( lipSyncData->HasEnded() ) 
	{
		EndLipSyncing();
		return;
	}

	// If frame non zero - blend to it as a new viseme
	if( lipSyncData->GetFrame() || lipSyncData->GetLastFrame() )
	{
		frameBlend_t	frameBlend;

		frameBlend.cycleCount = 0;
		frameBlend.frame1 = idMath::ClampInt( 0, 120, lipSyncData->GetLastFrame() - 1 );
		frameBlend.frame2 = idMath::ClampInt( 0, 120, lipSyncData->GetFrame() - 1 );
		frameBlend.frontlerp = lipSyncData->GetFrontLerp();
		frameBlend.backlerp = 1.0f - frameBlend.frontlerp;

		animator.SetFrame( ANIMCHANNEL_TORSO, lipSyncAnim, frameBlend );

		if( fas_debug.GetInteger() > 1 )
		{
			common->Printf( "Blending: %d (%2f) -> %d (%2f)\n", frameBlend.frame1, frameBlend.frontlerp, frameBlend.frame2, frameBlend.backlerp );
		}
	}

	// If an embedded emotion command, play it.
	if( lipSyncData->HasNewEmotion() )
	{
		anim = "emotion_";
		anim += lipSyncData->GetEmotion();
		emotion = animator.GetAnim( anim );
		animator.CycleAnim( ANIMCHANNEL_HEAD, emotion, gameLocal.time, 200 );
		animator.CycleAnim( ANIMCHANNEL_EYELIDS, emotion, gameLocal.time, 200 );

		if( fas_debug.GetInteger() )
		{
			common->Printf( "Emotion: %s\n", lipSyncData->GetEmotion().c_str() );
		}
	}

	// If a new phrase, display in debug mode
	if( fas_debug.GetInteger() && lipSyncData->HasNewPhrase() )
	{
		common->Printf( "Phrase: %s(%i)\n", lipSyncData->GetLastPhrase().c_str(), lipSyncData->GetFrame() );
	}
}

// end
