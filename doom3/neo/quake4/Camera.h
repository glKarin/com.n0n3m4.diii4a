
#ifndef __GAME_CAMERA_H__
#define __GAME_CAMERA_H__

/*
===============================================================================

Camera providing an alternative view of the level.

===============================================================================
*/

class idCamera : public idEntity {
public:
	ABSTRACT_PROTOTYPE( idCamera );

	void					Spawn( void );
	virtual void			GetViewParms( renderView_t *view ) = 0;
	virtual renderView_t *	GetRenderView();
	virtual void			Stop( void ){} ;
};

/*
===============================================================================

idCameraView

===============================================================================
*/

extern const idEventDef EV_SetFOV;
extern const idEventDef EV_Camera_Start;
extern const idEventDef EV_Camera_Stop;

class idCameraView : public idCamera {
public:
	CLASS_PROTOTYPE( idCameraView );
 							idCameraView();
 							
	// save games
	void					Save( idSaveGame *savefile ) const;				// archives object for save game file
	void					Restore( idRestoreGame *savefile );				// unarchives object from save game file

	void					Spawn( );
	virtual void			GetViewParms( renderView_t *view );
	virtual void			Stop( void );

protected:
	void					Event_Activate( idEntity *activator );
	void					Event_SetAttachments();
	void					SetAttachment( idEntity **e, const char *p );
// RAVEN BEGIN
// bdube: changed fov to interpolated value
	idInterpolate<float>	fov;
// RAVEN END
	idEntity				*attachedTo;
	idEntity				*attachedView;

// RAVEN BEGIN
// bdube: added setfov event
	void					Event_SetFOV		( float fov );
	void					Event_BlendFOV		( float beginFOV, float endFOV, float blendTime );
	void					Event_GetFOV		( void );
// RAVEN END
};



/*
===============================================================================

A camera which follows a path defined by an animation.

===============================================================================
*/

// RAVEN BEGIN
// rjohnson: camera is now contained in a def for frame commands

/*
==============================================================================================

	rvCameraAnimation

==============================================================================================
*/
class idDeclCameraDef;

typedef struct {
	idCQuat				q;
	idVec3				t;
	float				fov;
} cameraFrame_t;

class rvCameraAnimation {
private:
	idList<int>					cameraCuts;
	idList<cameraFrame_t>		camera;
	idList<frameLookup_t>		frameLookup;
	idList<frameCommand_t>		frameCommands;
	int							frameRate;
	idStr						name;
	idStr						realname;

public:
								rvCameraAnimation();
								rvCameraAnimation( const idDeclCameraDef *cameraDef, const rvCameraAnimation *anim );
								~rvCameraAnimation();

	void						SetAnim( const idDeclCameraDef *cameraDef, const char *sourcename, const char *animname, idStr filename );
	const char					*Name( void ) const;
	const char					*FullName( void ) const;
	int							NumFrames( void ) const;
	const cameraFrame_t *		GetAnim( int index ) const;
	int							NumCuts( void ) const;
	const int					GetCut( int index ) const;
	const int					GetFrameRate( void ) const;

	const char					*AddFrameCommand( const class idDeclCameraDef *cameraDef, const idList<int>& frames, idLexer &src, const idDict *def );
	void						CallFrameCommands( idEntity *ent, int from, int to ) const;
	void						CallFrameCommandSound ( const frameCommand_t& command, idEntity* ent, const s_channelType channel ) const;
};

ID_INLINE const cameraFrame_t *rvCameraAnimation::GetAnim( int index ) const {
	return &camera[ index ];
}

ID_INLINE const int rvCameraAnimation::GetCut( int index ) const {
	return cameraCuts[ index ];
}

ID_INLINE const int rvCameraAnimation::GetFrameRate( void ) const {
	return frameRate;
}

/*
==============================================================================================

	idDeclCameraDef

==============================================================================================
*/
// RAVEN BEGIN
// jsinger: added to support serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
class idDeclCameraDef : public idDecl, public Serializable<'IDCD'> {
public:
								idDeclCameraDef( SerialInputStream &stream );
								
	virtual void				Write( SerialOutputStream &stream ) const;
	virtual void				AddReferences() const;
#else
class idDeclCameraDef : public idDecl {
#endif
// RAVEN END
public:
								idDeclCameraDef();
								~idDeclCameraDef();

	virtual size_t				Size( void ) const;
	virtual const char *		DefaultDefinition( void ) const;
	virtual bool				Parse( const char *text, const int textLength/* jmarshall , bool noCaching*/ );
	virtual void				FreeData( void );

// RAVEN BEGIN
// jscott: to prevent a recursive crash
	virtual	bool				RebuildTextSource( void ) { return( false ); }
// scork: for detailed error-reporting
	virtual bool				Validate( const char *psText, int iTextLength, idStr &strReportTo ) const;
// RAVEN END

	void						Touch( void ) const;

	int							NumAnims( void ) const;
	const rvCameraAnimation *	GetAnim( int index ) const;
	int							GetSpecificAnim( const char *name ) const;
	int							GetAnim( const char *name ) const;
	bool						HasAnim( const char *name ) const;
	
private:
	void						CopyDecl( const idDeclCameraDef *decl );
	bool						ParseAnim( idLexer &src, int numDefaultAnims );

private:
	idList<rvCameraAnimation *>	anims;
};

ID_INLINE const rvCameraAnimation *idDeclCameraDef::GetAnim( int index ) const {
	if ( ( index < 1 ) || ( index > anims.Num() ) ) {
		return NULL;
	}
	return anims[ index - 1 ];
}

/*
==============================================================================================

	idCameraAnim

==============================================================================================
*/
class idCameraAnim : public idCamera {
public:
	CLASS_PROTOTYPE( idCameraAnim );

							idCameraAnim();
							~idCameraAnim();

	// save games
	void					Save( idSaveGame *savefile ) const;				// archives object for save game file
	void					Restore( idRestoreGame *savefile );				// unarchives object from save game file

	void					Spawn( void );
	virtual void			GetViewParms( renderView_t *view );

private:
	int						threadNum;
	idVec3					offset;
	int						starttime;
	int						cycle;
	const idDeclCameraDef	*cameraDef;
	int						lastFrame;
	idEntityPtr<idEntity>	activator;

	void					Start( void );
	void					Stop( void );
	void					Think( void );

	void					LoadAnim( void );
	void					Event_Start( void );
	void					Event_Stop( void );
	void					Event_SetCallback( void );
	void					Event_Activate( idEntity *activator );

// RAVEN BEGIN
// mekberg: wait support
	void					Event_IsActive( );

	idList<dword>			imageTable;
	idList<int>				imageCmds;
// RAVEN END
};
// RAVEN END

// RAVEN BEGIN
/*
===============================================================================

rvCameraPortalSky

===============================================================================
*/
// jscott: for portal skies
class rvCameraPortalSky : public idCamera {
public:
	CLASS_PROTOTYPE( rvCameraPortalSky );

							rvCameraPortalSky( void ) {}
							~rvCameraPortalSky( void ) {}

	// save games
	void					Save( idSaveGame *savefile ) const;				// archives object for save game file
	void					Restore( idRestoreGame *savefile );				// unarchives object from save game file

	void					Spawn( void );
	virtual void			GetViewParms( renderView_t *view );
};

/*
===============================================================================

rvCameraPlayback

===============================================================================
*/
class rvCameraPlayback : public idCamera {
public:
	CLASS_PROTOTYPE( rvCameraPlayback );

							rvCameraPlayback( void ) {}
							~rvCameraPlayback( void ) {}

	// save games
	void					Save( idSaveGame *savefile ) const;				// archives object for save game file
	void					Restore( idRestoreGame *savefile );				// unarchives object from save game file

	void					Spawn( void );
	virtual void			GetViewParms( renderView_t *view );

private:
	int						startTime;
	const rvDeclPlayback	*playback;
};
// RAVEN END

#endif /* !__GAME_CAMERA_H__ */
