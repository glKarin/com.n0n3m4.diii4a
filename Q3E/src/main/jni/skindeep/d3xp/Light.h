/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __GAME_LIGHT_H__
#define __GAME_LIGHT_H__

#include "gamesys/Event.h"
#include "Entity.h"

/*
===============================================================================

  Generic light.

===============================================================================
*/

extern const idEventDef EV_Light_GetLightParm;
extern const idEventDef EV_Light_SetLightParm;
extern const idEventDef EV_Light_SetLightParms;

// SW: Alternative params to be triggered when we switch to the associated sky.
// skyTint_t replaces the light's color, and skyCenter_t replaces the light_center
typedef struct skyTint_s {
	const idMaterial* sky;
	idVec3 color;
} skyTint_t;

typedef struct skyCenter_s {
	const idMaterial* sky;
	idVec3 center;
} skyCenter_t;

class idLight : public idEntity {
public:
	CLASS_PROTOTYPE( idLight );

					idLight();
					~idLight();

	void			Spawn( void );

	void			Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void			Restore( idRestoreGame *savefile );					// unarchives object from save game file

	virtual void	UpdateChangeableSpawnArgs( const idDict *source );
	virtual void	ParseSkySettings(const idList<const idMaterial*> skies);
	virtual void	ApplySkySetting(const idMaterial* sky);
	virtual void	Think( void );
	virtual void	FreeLightDef( void );
	virtual bool	GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );
	void			Present( void );

	void			SaveState( idDict *args );
	virtual void	SetColor( float red, float green, float blue );
	virtual void	SetColor( const idVec3 &color );
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

	virtual void	ShowEditingDialog( void );

	enum {
		EVENT_BECOMEBROKEN = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

	virtual void	ClientPredictionThink( void );
	virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void	ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual bool	ClientReceiveEvent( int event, int time, const idBitMsg &msg );

	//BC public
	void			SetLightTarget(idVec3 newTarget);
	bool			GetNoShadow();
	void			SetNoShadow(bool value);
	bool			Event_IsOn(void);
	void			GetBaseColor(idVec3 &out);
	idVec3			GetRadius();
	bool			Event_IsAmbient(void);
	void			Event_SetAmbient(bool value);
	virtual void	DoRepairTick(int amount);
	idVec4			GetOriginalColor();
	bool			IsFog();
	
	void			SetAffectLightmeter(bool value);
	bool			GetOriginalLightmeter();

	//end public


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
	bool			soundWasPlaying;

	idList<skyTint_t> skyTints;
	idList<skyCenter_t> skyCenters;

private:
	void			PresentLightDefChange( void );
	void			PresentModelDefChange( void );

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
	void			Event_SetSpotlightTarget(const idVec3 &vec);
	void			Event_GetLightHandle(void);

	//BC
	idDict			shardDict;
	idStr			fxBreak;
	void			ToggleTargetedLights(bool value);
	idVec4			originalColor;
	bool			originalAffectLightmeter;
	
};

#endif /* !__GAME_LIGHT_H__ */
