#ifndef _LIPSYNC_H_INC_
#define _LIPSYNC_H_INC_

// Possible TBDs
// Auto head swapping from simple to lip sync head
// Return time of sound from the speak call for the script to wait
// Set emotion directly without encoding it into the string

class rvViseme
{
public:
				rvViseme( void ) {}
				~rvViseme( void ) { phoneme.Clear(); }

	void		Init( idStr &phon, int f, int bt );

	int			GetFrame( void ) const { return( frame ); }
	int			GetBlendTime( void ) const { return( blendTime ); }

private:
	idStr		phoneme;
	int			frame;
	int			blendTime;
};

#define FAS_NEW_VISEME		BIT( 0 )
#define FAS_NEW_PHRASE		BIT( 1 )
#define FAS_NEW_EMOTION		BIT( 2 )
#define FAS_ENDED			BIT( 3 )

class rvLipSyncData
{
public:
					rvLipSyncData(  const rvDeclLipSync *ls, int time );
					~rvLipSyncData( void ) {}

	bool			Ready( int time ) { return( time >= mNextTokenTime ); }

	int				ReadToken( idToken *token ) { return( mLexer.ReadToken( token ) ); }
	int				ExpectTokenString( const char *str ) { return( mLexer.ExpectTokenString( str ) ); }
	void			SetNextTokenTime( int time ) { mNextTokenTime += time; }
	
	void			ClearFlags( void ) { mFlags = 0; }
	void			SetFlags( int flags ) { mFlags |= flags; }
	bool			HasNewPhoneme( void ) const { return( !!( mFlags & FAS_NEW_VISEME ) ); }
	bool			HasNewPhrase( void ) const { return( !!( mFlags & FAS_NEW_PHRASE ) ); }
	bool			HasNewEmotion( void ) const { return( !!( mFlags & FAS_NEW_EMOTION ) ); }
	bool			HasEnded( void ) const { return( !!( mFlags & FAS_ENDED ) ); }

	void			SetFrame( int frame );
	int				GetFrame( void ) const { return( mFrame ); }
	int				GetLastFrame( void ) { return( mLastFrame ); }

	void			SetBlendTime( int bt ) { mBlendTime = bt; }
	int				GetBlendTime( void ) const { return( mBlendTime ); }

	float			GetFrontLerp( void );

	void			SetEmotion( idStr &str ) { mEmotion = str; }
	const idStr		&GetEmotion( void ) const { return( mEmotion ); }

	void			SetLastPhrase( idStr &str ) { mLastPhrase = str; }
	const idStr		&GetLastPhrase( void ) const { return( mLastPhrase ); }

private:
	int				mNextTokenTime;
	int				mFlags;
	int				mFrame;
	int				mLastFrame;
	int				mVisemeStartTime;
	int				mBlendTime;
	idStr			mEmotion;
	idStr			mLastPhrase;
	idLexer			mLexer;
};

bool FAS_Init( const char *visemes );
void FAS_Shutdown( void );

void FAS_ExtractViseme( class rvLipSyncData *lsd, int time );
class rvLipSyncData *FAS_StartVisemeExtraction( const rvDeclLipSync *ls, int time );
void FAS_EndVisemeExtraction( class rvLipSyncData *lsd );

extern idCVar fas_debug;
extern idCVar fas_timeOffset;

#endif // _LIPSYNC_H_INC_
