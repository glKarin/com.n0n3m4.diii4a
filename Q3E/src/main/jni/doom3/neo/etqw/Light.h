// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_LIGHT_H__
#define __GAME_LIGHT_H__

/*
===============================================================================

  Generic light.

===============================================================================
*/

class idLight : public idEntity {
public:
	CLASS_PROTOTYPE( idLight );

						idLight();
						~idLight();

	void				Spawn( void );

	virtual void		UpdateChangeableSpawnArgs( const idDict *source );
	virtual void		Think( void );
	virtual void		FreeLightDef( void );
	virtual bool		GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );
	void				Present( void );

	virtual void		SetColor( float red, float green, float blue );
	virtual void		SetColor( const idVec4 &color );
	virtual void		GetColor( idVec3 &out ) const;
	virtual void		GetColor( idVec4 &out ) const;
	const idVec3 &		GetBaseColor( void ) const { return baseColor; }
	void				On( void );
	void				Off( void );
	void				Fade( const idVec4 &to, float fadeTime );
	void				FadeOut( float time );
	void				FadeIn( float time );
	qhandle_t			GetLightDefHandle( void ) const { return lightDefHandle; }
	void				SetLightParent( idEntity *lparent ) { lightParent = lparent; }
	void				SetLightLevel( void );

	static void			PushMapLight( qhandle_t handle );
	static void			OnNewMapLoad( void );
	static void			OnMapClear( void );
	static void			FreeMapLights( void );

	virtual void		PostMapSpawn( void );

	virtual bool		IsDynamicLight( void ) { return spawnArgs.GetBool( "dynamic" ); }

protected:
	virtual void		PresentLightDefChange( void );
	void				SetLightAreas();

protected:
	renderLight_t		renderLight;				// light presented to the renderer
	idVec3				localLightOrigin;			// light origin relative to the physics origin
	idMat3				localLightAxis;				// light axis relative to physics axis
	qhandle_t			lightDefHandle;				// handle to renderer light def
	int					levels;
	int					currentLevel;
	idVec3				baseColor;
	int					count;
	int					triggercount;
	idEntity *			lightParent;
	idVec4				fadeFrom;
	idVec4				fadeTo;
	int					fadeStart;
	int					fadeEnd;
	bool				soundWasPlaying;
	bool				interior;

	static idList< qhandle_t > s_lightHandles;

private:
	void				PresentModelDefChange( void );

	void				Event_Hide( void );
	void				Event_Show( void );
	void				Event_On( void );
	void				Event_Off( void );
	void				Event_ToggleOnOff( idEntity *activator );
	void				Event_SetAtmosphere( float value );
};

#endif /* !__GAME_LIGHT_H__ */
