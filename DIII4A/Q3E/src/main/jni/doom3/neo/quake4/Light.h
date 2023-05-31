#ifndef __GAME_LIGHT_H__
#define __GAME_LIGHT_H__

/*
===============================================================================

  Generic light.

===============================================================================
*/

extern const idEventDef EV_Light_GetLightParm;
extern const idEventDef EV_Light_SetLightParm;
extern const idEventDef EV_Light_SetLightParms;

class idLight : public idEntity {
public:
	CLASS_PROTOTYPE( idLight );

					idLight();
					~idLight();

	void			Spawn( void );

	void			Save( idSaveGame *savefile ) const;					// archives object for save game file
	void			Restore( idRestoreGame *savefile );					// unarchives object from save game file

	virtual void	UpdateChangeableSpawnArgs( const idDict *source );
	virtual void	Think( void );
	virtual void	FreeLightDef( void );
	virtual bool	GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );
	void			Present( void );

	void			SaveState( idDict *args );
	virtual void	SetColor( float red, float green, float blue );
	virtual void	SetColor( const idVec4 &color );
	virtual void	GetColor( idVec3 &out ) const;
	virtual void	GetColor( idVec4 &out ) const;
	const idVec3 &	GetBaseColor( void ) const { return baseColor; }
	void			SetShader( const char *shadername );
	void			SetLightParm( int parmnum, float value );
	void			SetLightParms( float parm0, float parm1, float parm2, float parm3 );
	void			SetRadiusXYZ( float x, float y, float z );
	void			SetRadius( float radius );
	void			On( void );
	void			Off( void );
	void			Fade( const idVec4 &to, float fadeTime );
	void			FadeOut( float time );
	void			FadeIn( float time );
	void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	void			BecomeBroken( idEntity *activator );
	qhandle_t		GetLightDefHandle( void ) const { return lightDefHandle; }
	void			SetLightParent( idEntity *lparent ) { lightParent = lparent; }
	void			SetLightLevel( void );

// RAVEN BEGIN
// jshepard: other entities (speakers) need access to the refSound of a light object
	void			SetRefSound( int rSound ) { refSound.referenceSoundHandle = rSound;}
// ddynerman: sometimes the game needs to know if this light is ambient
	bool			IsAmbient( void ) { return renderLight.shader->IsAmbientLight(); }
// RAVEN END
	virtual void	ShowEditingDialog( void );

	enum {
		EVENT_BECOMEBROKEN = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

	virtual void	ClientPredictionThink( void );
	virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void	ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual bool	ClientReceiveEvent( int event, int time, const idBitMsg &msg );

private:
	renderLight_t	renderLight;				// light presented to the renderer
	idVec3			localLightOrigin;			// light origin relative to the physics origin
	idMat3			localLightAxis;				// light axis relative to physics axis
	qhandle_t		lightDefHandle;				// handle to renderer light def
	idStr			brokenModel;
	int				levels;
	int				currentLevel;
	idVec3			baseColor;
	bool			breakOnTrigger;
	int				count;
	int				triggercount;
	idEntity *		lightParent;
	idVec4			fadeFrom;
	idVec4			fadeTo;
	int				fadeStart;
	int				fadeEnd;
// RAVEN BEGIN
// bdube: light gui
	idEntityPtr<idEntity>	lightGUI;
// abahr:
	float			wait;
	float			random;	
// RAVEN END

private:
	bool			soundWasPlaying;

	void			PresentLightDefChange( void );
	void			PresentModelDefChange( void );

// RAVEN BEGIN
// jscott: added events for light level
private:
	void			Event_SetCurrentLightLevel ( int in );
	void			Event_SetMaxLightLevel ( int in );
	void			Event_IsOn( void );
	void			Event_Break( idEntity *activator, float turnOff );
	void			Event_DoneBlinking( void );
	void			Event_DoneBlinkingOff( void );
	void			Event_EarthQuake( float requiresLOS );
	void			Event_Timer( void );
// RAVEN END

private:
	void			Event_SetShader( const char *shadername );
	void			Event_GetLightParm( int parmnum );
	void			Event_SetLightParm( int parmnum, float value );
	void			Event_SetLightParms( float parm0, float parm1, float parm2, float parm3 );
	void			Event_SetRadiusXYZ( float x, float y, float z );
	void			Event_SetRadius( float radius );
	void			Event_Hide( void );
	void			Event_Show( void );
	void			Event_On( void );
	void			Event_Off( void );
	void			Event_ToggleOnOff( idEntity *activator );
	void			Event_SetSoundHandles( void );
	void			Event_FadeOut( float time );
	void			Event_FadeIn( float time );
// RAVEN BEGIN
// bdube: set light gui
	void			Event_SetLightGUI( const char* gui );
// RAVEN END	
};

// RAVEN BEGIN
// bdube: externed events
extern const idEventDef EV_Light_SetCurrentLightLevel;
extern const idEventDef EV_Light_SetMaxLightLevel;
extern const idEventDef EV_Light_SetRadius;
// RAVEN END

#endif /* !__GAME_LIGHT_H__ */
