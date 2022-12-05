
#ifndef __DECLPLAYBACK_H__
#define __DECLPLAYBACK_H__

// WARNING: These must stay mirrored in both Q4Monster.state
#define PBFL_GET_POSITION			BIT( 0 )	// Stored in playback file
#define PBFL_GET_ANGLES				BIT( 1 )
#define PBFL_GET_BUTTONS			BIT( 2 )

#define PBFL_GET_VELOCITY			BIT( 4 )	// Derived from data
#define PBFL_GET_ACCELERATION		BIT( 5 )
#define PBFL_GET_ANGLES_FROM_VEL	BIT( 6 )

#define PBFL_AT_DEST				BIT( 7 )
#define PBFL_RELATIVE_POSITION		BIT( 8 )

#define PBFL_ED_MODIFIED			BIT( 29 )
#define PBFL_ED_NEW					BIT( 30 )
#define PBFL_ED_CHECKEDIN			BIT( 31 )

#define PBFL_ED_MASK				( PBFL_ED_MODIFIED | PBFL_ED_NEW | PBFL_ED_CHECKEDIN )

#define PBCB_NONE					0
#define PBCB_BUTTON_DOWN			1
#define PBCB_BUTTON_UP				2
#define PBCB_IMPULSE				3

typedef void ( *pbCallback_t )( int type, float time, const void *data );

class rvDeclPlaybackData
{
public:
	void					Init( void ) { entity = NULL; Callback = NULL; position.Zero(); velocity.Zero(); acceleration.Zero(); angles.Zero(); changed = 0; button = 0; impulse = 0; }
	
	void					SetPosition( const idVec3 &pos ) { position = pos; }
	void					SetVelocity( const idVec3 &vel ) { velocity = vel; }
	void					SetAcceleration( const idVec3 &accel ) { acceleration = accel; }
	void					SetAngles( const idAngles &ang ) { angles = ang; }
	void					SetChanged( const byte chg ) { changed = chg; }
	void					SetButtons( const byte btn ) { button = btn; }
	void					SetImpulse( const byte imp ) { impulse = imp; }

	const idVec3			&GetPosition( void ) const { return( position ); }
	const idVec3			&GetVelocity( void ) const { return( velocity ); }
	const idVec3			&GetAcceleration( void ) const { return( acceleration ); }
	const idAngles			&GetAngles( void ) const { return( angles ); }
	byte					GetChanged( void ) const { return( changed ); }
	byte					GetButtons( void ) const { return( button ); }
	byte					GetImpulse( void ) const { return( impulse ); }

	class idEntity			*GetEntity( void ) const { return( entity ); }

	void					SetCallback( class idEntity *ent, pbCallback_t cb ) { entity = ent; Callback = cb; }
	void					CallCallback( int type, float time ) { if( Callback ) { Callback( type, time, this ); } }
private:
	class idEntity			*entity;
	pbCallback_t			Callback;

	idVec3					position;
	idVec3					velocity;
	idVec3					acceleration;
	idAngles				angles;
	byte					changed;
	byte					button;
	byte					impulse;
};

class rvButtonState
{
public:
	void					Init( float t, byte b = 0, byte i = 0 ) { time = t; state = b; impulse = i; }

	float					time;
	byte					state;
	byte					impulse;
};

#ifdef RV_BINARYDECLS
class rvDeclPlayback : public idDecl, public Serializable<'RDP '>
{
public:
// jsinger: allow exporting of this decl type in a preparsed form
	virtual void			Write( SerialOutputStream &stream ) const;
	virtual void			AddReferences() const;
							rvDeclPlayback( SerialInputStream &stream );
#else
class rvDeclPlayback : public idDecl
{
#endif
public:
							rvDeclPlayback( void );
							~rvDeclPlayback( void );

			void			SetFlag( bool on, int flag ) { on ? flags |= flag : flags &= ~flag; }

			bool			GetHasPositions( void ) const { return( !!( flags & PBFL_GET_POSITION ) ); }
			bool			GetHasAngles( void ) const { return( !!( flags & PBFL_GET_ANGLES ) ); }
			bool			GetHasButtons( void ) const { return( !!( flags & PBFL_GET_BUTTONS ) ); }
			bool			GetEditorModified( void ) const { return( !!( flags & PBFL_ED_MODIFIED ) ); }
			bool			GetEditorNew( void ) const { return( !!( flags & PBFL_ED_NEW ) ); }
			bool			GetEditorCheckedIn( void ) const { return( !!( flags & PBFL_ED_CHECKEDIN ) ); }

			void			SetHasPositions( bool pos ) { SetFlag( pos, PBFL_GET_POSITION ); }
			void			SetHasAngles( bool ang ) { SetFlag( ang, PBFL_GET_ANGLES ); }
			void			SetHasButtons( bool btn ) { SetFlag( btn, PBFL_GET_BUTTONS ); }
			void			SetEditorModified( bool em ) { SetFlag( em, PBFL_ED_MODIFIED ); }
			void			SetEditorNew( bool en ) { SetFlag( en, PBFL_ED_NEW ); }
			void			SetEditorCheckedIn( bool eci ) { SetFlag( eci, PBFL_ED_CHECKEDIN ); }

			int				GetFlags( void ) const { return( flags ); }
			void			SetFlags( int in ) { flags = in; }

			float			GetFrameRate( void ) const { return( frameRate ); }
			void			SetFrameRate( float in ) { frameRate = in; }

			idVec3			GetOrigin( void ) const { return( origin ); }
			void			SetOrigin( idVec3 &in ) { origin = in; }

			float			GetDuration( void ) const { return( duration ); }
			void			SetDuration( float dur ) { duration = dur; }

			idBounds		GetBounds( void ) const { return( bounds ); }

			void			ParseSample( idLexer *src, idVec3 &pos, idAngles &ang );
			void			WriteData( idFile_Memory &f );
			void			WriteButtons( idFile_Memory &f );
			void			WriteSequence( idFile_Memory &f );

			bool			ParseData( idLexer *src );
			void			ParseButton( idLexer *src, byte &button, rvButtonState &state );
			bool			ParseSequence( idLexer *src );
			bool			ParseButtons( idLexer *src );

			void			Copy( rvDeclPlayback *pb );
			void			SetOrigin( void );
			void			Start( void );
			bool			Finish( float desiredDuration = -1.0f );

			bool			SetCurrentData( float localTime, int control, rvDeclPlaybackData *pbd );
			bool			GetCurrentOffset( float localTime, idVec3 &pos ) const;
			bool			GetCurrentAngles( float localTime, idAngles &ang ) const;
			bool			GetCurrentData( int control, float localTime, float lastTime, rvDeclPlaybackData *pbd ) const;

	virtual const char		*DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength, bool noCaching );
	virtual void			FreeData( void );
	virtual	bool			RebuildTextSource( void );
	virtual size_t			Size( void ) const;

// RAVEN BEGIN
// scork: for detailed error-reporting
	virtual bool			Validate( const char *psText, int iTextLength, idStr &strReportTo ) const;
// RAVEN END


	idCurve_UniformCubicBSpline<idVec3>		&GetPoints( void ) { return( points ); }
	idCurve_UniformCubicBSpline<idAngles>	&GetAngles( void ) { return( angles ); }
	idList<rvButtonState>					&GetButtons( void ) { return( buttons ); }

private:

	int							flags;
	float						frameRate;
	float						duration;
	idVec3						origin;
	idBounds					bounds;

	idCurve_UniformCubicBSpline<idVec3>		points;
	idCurve_UniformCubicBSpline<idAngles>	angles;
	idList<rvButtonState>					buttons;
};

class rvDeclPlaybackEdit
{
public:
	virtual ~rvDeclPlaybackEdit() {}
	virtual bool			Finish( rvDeclPlayback *edit, float desiredDuration ) = 0;
	virtual void			SetOrigin( rvDeclPlayback *edit ) = 0;
	virtual void			SetOrigin( rvDeclPlayback *edit, idVec3 &origin ) = 0;
	virtual void			Copy( rvDeclPlayback *edit, rvDeclPlayback *copy ) = 0;
};

extern rvDeclPlaybackEdit		*declPlaybackEdit;

#endif // __DECLPLAYBACK_H__
