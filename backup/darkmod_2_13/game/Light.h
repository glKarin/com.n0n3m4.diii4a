/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __GAME_LIGHT_H__
#define __GAME_LIGHT_H__

/*
===============================================================================

  Generic light.

===============================================================================
*/
class CLightMaterial;

extern const idEventDef EV_Light_GetLightParm;
extern const idEventDef EV_Light_SetLightParm;
extern const idEventDef EV_Light_SetLightParms;

class idLight : public idEntity {
public:
	CLASS_PROTOTYPE( idLight );

	idLight();
	virtual ~idLight() override;

	void			Spawn( void );

	void			Save( idSaveGame *savefile ) const;					// archives object for save game file
	void			Restore( idRestoreGame *savefile );					// unarchives object from save game file

	virtual void	UpdateChangeableSpawnArgs( const idDict *source ) override;
	virtual void	Think( void ) override;
	virtual void	FreeLightDef( void ) override;
	virtual bool	GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) override;
	virtual void	Present( void ) override;
	virtual void	Hide( void ) override;
	virtual void	Show( void ) override;

	void			SaveState( idDict *args );
	virtual void	SetColor( const idVec3 &color ) override;
	virtual void	SetColor( const idVec4 &color ) override;
	virtual void	GetColor( idVec3 &out ) const override;
	virtual void	GetColor( idVec4 &out ) const override;

	/**
	* Tels: idLight::GetLightOrigin returns the origin of the light in the world. This
	* is different from the physics origin, since the light can be offset.
	*/
	const idVec3 &	GetLightOrigin( void ) const { return renderLight.origin; }

	const idVec3 &	GetBaseColor( void ) const { return baseColor; }
	void			SetShader( const char *shadername );
	void			SetLightParm( int parmnum, float value );
	void			SetLightParms( float parm0, float parm1, float parm2, float parm3 );
	void			SetRadiusXYZ( const float x, const float y, const float z );
	void			SetRadius( const float radius );
	void			On( void );
	void			Off( const bool stopSound = true );
	void			Fade( const idVec4 &to, float fadeTime );
	void			FadeOut( float time );
	void			FadeIn( float time );
	void			FadeTo( idVec3 color, float time );
	void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) override;
	void			BecomeBroken( idEntity *activator );
	qhandle_t		GetLightDefHandle( void ) const { return lightDefHandle; }
	void			SetLightParent( idEntity *lparent ) { lightParent = lparent; }
	void			SetLightLevel( void );

	/**
	 * greebo: Returns the current lightlevel (currentlevel).
	 */
	int				GetLightLevel() const;

	/**
	 * Tels: return the current light radius.
	 */
	void			GetRadius( idVec3 &out ) const;
	idVec3			GetRadius( void ) const { return idVec3(renderLight.lightRadius[0], renderLight.lightRadius[1], renderLight.lightRadius[2] ); }

	virtual void	ShowEditingDialog( void ) override;

	enum {
		EVENT_BECOMEBROKEN = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

	virtual void	ClientPredictionThink( void ) override;
	virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void	ReadFromSnapshot( const idBitMsgDelta &msg ) override;
	virtual bool	ClientReceiveEvent( int event, int time, const idBitMsg &msg ) override;

	/**	Returns a bounding box surrounding the light.
	 */
	idBounds		GetBounds();
	/**	Called to update m_renderTrigger after the render light is modified.
	 *	Only updates the render trigger if a thread is waiting for it.
	 */
	void			PresentRenderTrigger();

	/**
	 * This will return a grayscale value dependent on the value from the light.
	 * X and Y are the coordinates returned by calculating the position from the 
	 * player related to the light. The Z coordinate can be ignored. The distance
	 * is required when the light has no textures to calculate a falloff.
	 */
	float			GetDistanceColor(float distance, float x, float y);

	/**
	 * GetTextureIndex calculates the index into the texture based on the x/y coordinates
	 * given and returns the index.
	 */
	int				GetTextureIndex(float x, float y, int TextureWidth, int TextureHeight, int BytesPerPixel);

	/**
	 * Returns true if the light is a parallel light.
	 */
	inline bool		IsParallel(void) { return renderLight.parallel; };
	inline bool		IsPointlight(void) { return renderLight.pointLight; };
	bool			CastsShadow(void);
	bool			IsAmbient(void) const;		// Returns true if this is an ambient light. - J.C.Denton
	bool			IsFog(void) const;			// Returns true if this is a fog light. - tels
	bool			IsBlend(void) const;		// Returns true if this is a blend light. - tels
	bool			IsSeenByAI(void) const;		// Whether the light affects lightgem and visbility of other objects to AI #4128

	/**
	 * GetLightCone returns the lightdata.
	 * If the light is a pointlight it will return an ellipsoid defining the light.
	 * In case of a projected light, the returned data is a cone.
	 * If the light is a projected light and uses the additional vectors for
	 * cut off cones, it will return true.
	 */
	bool GetLightCone(idVec3 &Origin, idVec3 &Axis, idVec3 &Center);
	bool GetLightCone(idVec3 &Origin, idVec3 &Target, idVec3 &Right, idVec3 &Up, idVec3 &Start, idVec3 &End);

	/**
	 * grayman #2603 - add a switch to the switch list
	 */

	void AddSwitch(idEntity* newSwitch);

	/**
	 * grayman #2603 - If there are switches, returns the closest one to the calling user
	 */

	idEntity* GetSwitch(idAI* user);

	/**
	 * grayman #2603 - Change the flag that says if this light is being relit.
	 */

	void SetBeingRelit(bool relighting);

	/**
	 * grayman #2603 - Is an AI in the process or relighting this light?
	 */

	bool IsBeingRelit();

	/**
	 * grayman #2603 - Set the chance that this light can be barked about negatively
	 */

	void SetChanceNegativeBark(float newChance);

	/**
	 * grayman #2603 - can an AI make a negative bark about this light (found off, or won't relight)
	 */

	bool NegativeBark(idAI* ai);

	/**
	 * grayman #2603 - Get when the light was turned off
	 */

	int GetWhenTurnedOff();

	/**
	 * grayman #2603 - Get when can this light be relit
	 */

	int GetRelightAfter();

	/**
	 * grayman #2603 - Set when can this light be relit
	 */

	void SetRelightAfter();

	/*
	 * grayman #2603 - Get when an AI can next emit a "light's out" or "won't relight" bark
	 */

	int GetNextTimeLightOutBark();

	/*
	 * grayman #2603 - Set when an AI can next emit a "light's out" or "won't relight" bark
	 */

	void SetNextTimeLightOutBark(int newNextTimeLightOutBark);

	/*
	 * grayman #2603 - Is a flame vertical?
	 */

	bool IsVertical(float degreesFromVertical);

	/*
	 * grayman #2603 - flame is out and smoking?
	 */

	bool IsSmoking();

	/*
	 * grayman #2905 - was the light out at spawn time?
	 */

	bool GetStartedOff();

	renderLight_t*	GetRenderLight( void ) { return &renderLight; }

private:
	renderLight_t	renderLight;				// light presented to the renderer
	idVec3			localLightOrigin;			// light origin relative to the physics origin
	idMat3			localLightAxis;				// light axis relative to physics axis
	qhandle_t		lightDefHandle;				// handle to renderer light def
	int				levels;
	int				currentLevel;
	idVec3			baseColor;
	bool			breakOnTrigger;
	int				count;
	int				triggercount;
	idEntity *		lightParent;
	idList< idEntityPtr<idEntity> > switchList;	// grayman #2603 - list of my switches
	float			chanceNegativeBark;			// grayman #2603 - chance of negative barks ("light's out" and "won't relight")
	int				whenTurnedOff;				// grayman #2603 - when this light was turned off
	int				nextTimeLightOutBark;		// grayman #2603 - the next time an AI can bark about a light that's out, or a decision not to relight it
	int				relightAfter;				// grayman #2603 - when a light can be relit
	float			nextTimeVerticalCheck;		// grayman #2603 - the next time to check if a lit flame is vertical
	bool			smoking;					// grayman #2603 - the flame model has changed to a smoke partical model; considered "out"
	int				whenToDouse;				// grayman #2603 - when a non-vertical flame can be doused
	bool			startedOff;					// grayman #2905 - was the light off at spawn time?

	idVec4			fadeFrom;
	idVec4			fadeTo;
	int				fadeStart;
	int				fadeEnd;
	bool			soundWasPlaying;

	/**
	 * grayman #2603 - Per-AI info for counting negative barks. This allows
	 * lights to keep track of the number of negative barks it elicits from
	 * passing AI. AI are only allowed 2 negative barks per light. The list
	 * is cleared when the light is relit, allowing AI to once again make
	 * negative barks.
	 */
	struct AIBarks
	{
		// The number of negative barks for this AI
		int count;
		
		// The AI who made them
		idEntityPtr<idEntity> ai;
	};

	idList<AIBarks> aiBarks; // grayman #2603


private:
	void			PresentLightDefChange( void );
	void			PresentModelDefChange( void );

	void			Event_SetShader( const char *shadername );
	void			Event_GetShader( void );	// Added in #3765
	void			Event_GetLightParm( int parmnum );
	void			Event_SetLightParm( int parmnum, float value );
	void			Event_SetLightParms( float parm0, float parm1, float parm2, float parm3 );
	void			Event_SetRadiusXYZ( float x, float y, float z );
	void			Event_GetRadius( void ) const;
	void			Event_SetRadius( float radius );
	void			Event_Hide( void );
	void			Event_Show( void );
	void			Event_On( void );
	void			Event_Off( void );
	void			Event_ToggleOnOff( idEntity *activator );
	void			Event_SetSoundHandles( void );
	void			Event_FadeOut( float time );
	void			Event_FadeIn( float time );
	/**
	* Tels: Allows the color of the light to fade over to a new value over a time period.
	*/
	void			Event_FadeToLight( idVec3 &color, float time );
	/**
	* Allows script to get and set the light origin separate from model origin.
	* Used to achieve moving lights with a stationary model
	**/
	void			Event_GetLightOrigin( void );
	void			Event_SetLightOrigin( idVec3 &pos );

	/**
	* Allows us to get the light level, and tell if the light is on or off.
	* "On" light levels are > 0.0
	*/
	void			Event_GetLightLevel();


	/*
	*	Add the light to the appropriate LAS area
	*/
	void			Event_AddToLAS();

	/**	Returns 1 if the light is in PVS.
	 *	Doesn't take into account vis-area optimizations for shadowcasting lights.
	 */
	void			Event_InPVS();

	/**
	 * grayman #2603
	 */

	void			Event_Smoking(int state);

	/**
	 * grayman #2905
	 */

	void			Event_SetStartedOff();

	/**
	* SteveL #3752: Allow blend lights to be toggled by remembering the material for
	* replacement later. Do not re-use m_MaterialName because (1) it might be changed so
	* that it updates when SetShader() is called, which would break it for this purpose,
	* and (2) m_MaterialName is not restored from save games.
	*/
	const idMaterial *m_BlendlightTexture;

public:

	/**
	 * Each light also gets the maxlightradius, which determines which value
	 * is the maximum radius for that particular light,
	 */
	float			m_MaxLightRadius;

	bool			beingRelit;					// grayman #2603 - true if being relit

	/*!
	* Darkmod LAS
	* The area the light is in, assigned by The Dark Mod Lighting Awareness System (LAS)
	*/
	int LASAreaIndex;
};

#endif /* !__GAME_LIGHT_H__ */
