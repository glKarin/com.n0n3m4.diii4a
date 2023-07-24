// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_PLAYERVIEW_H__
#define __GAME_PLAYERVIEW_H__

/*
===============================================================================

  Player view.

===============================================================================
*/

class idMaterial;
class sdDeclDamage;
class sdUserInterfaceLocal;
class sdWorldToScreenConverter;

#include "../renderer/RenderWorld.h"
#include "client/ClientEntity.h"
#include "effects/Effects.h"
#include "CrosshairInfo.h"

struct weaponFeedback_t;

#define	MAX_SCREEN_BLOBS	8

class idPlayerView {
public:
	struct repeaterViewInfo_t {
		idVec3								origin;
		idVec3								velocity;
		idAngles							deltaViewAngles;
		idAngles							viewAngles;
		bool								foundSpawn;
	};

						idPlayerView( void );
						~idPlayerView( void );

	void				ClearEffects();
	void				DamageImpulse( idVec3 localKickDir, const sdDeclDamage* damage );

	void				Init( void );

	void				WeaponFireFeedback( const weaponFeedback_t& feedback );

	idAngles			AngleOffset( const idPlayer* player ) const;

	const idMat3&		ShakeAxis( void ) const;

	void				CalculateShake( idPlayer* player );
	void				UpdateProxyView( idPlayer* player, bool force );
	void				SetActiveProxyView( idEntity* other, idPlayer* player );

	void						CalculateRepeaterView( renderView_t& view );
	void						UpdateRepeaterView( void );
	void						SetRepeaterUserCmd( const usercmd_t& usercmd );
	void						ClearRepeaterView( void );
	const usercmd_t&			GetRepeaterUserCmd( void ) const { return repeaterUserCmd; }
	const repeaterViewInfo_t	GetRepeaterViewInfo( void ) const { return repeaterViewInfo; }
	const sdCrosshairInfo&		CalcRepeaterCrosshairInfo( void );
	void						SetRepeaterViewPosition( const idVec3& origin, const idAngles& angles );

	bool				RenderPlayerView( idPlayer* player );
	bool				RenderPlayerView2D( idPlayer* player );

	void				Fade( const idVec4& color, int time );

	void				Flash( const idVec4& color, int time );

	const renderView_t&	GetCurrentView( void ) const { return currentView; }
	renderView_t&		GetCurrentView( void ) { return currentView; }

	void				SetShakeParms( float time, float scale ) { shakeFinishTime = SEC2MS( time ); shakeScale = scale; }
	void				SetTunnelParms( float time, float scale ) { lastDamageTime = time; tunnelScale = 1.0f / scale; }

	void				DrawScoreBoard( idPlayer *player );

	void				SetupCockpit( const idDict& info, idEntity* vehicle );
	void				ClearCockpit( void );
	bool				CockpitIsValid( void ) const { return cockpit.IsValid(); }
	rvClientEntityPtr< sdClientAnimated > &GetCockpit( void ) { return cockpit; }

	idEntity*			GetActiveViewProxy( void ) const { return activeViewProxy; }

	bool				SkipWorldRender( void );

	void				UpdateOcclusionQueries( const renderView_t* view );

	void				UpdateSpectateView( idPlayer* other );

	void				SetupEffect( void );

private:
	void				SingleView( idPlayer* viewPlayer, const renderView_t *view );
	void				SingleView2D( idPlayer* viewPlayer );
	void				ScreenFade();

	void				BuildViewFrustum( const renderView_t* view, idFrustum& viewFrustum );

	void				DrawWorldGuis( const renderView_t* view, const idFrustum& viewFrustum );

	int					kickFinishTime;		// view kick will be stopped at this time
	idAngles			kickAngles;

	idAngles			swayAngles;

	float				lastDamageTime;		// accentuate the tunnel effect for a while

	int					shakeFinishTime;
	float				shakeScale;
	float				tunnelScale;

	idVec4				fadeColor;			// fade color
	idVec4				fadeToColor;		// color to fade to
	idVec4				fadeFromColor;		// color to fade from
	float				fadeRate;			// fade rate
	int					fadeTime;			// fade time

	idAngles			shakeAng;			// from the sound sources
	idMat3				shakeAxes;

	renderView_t		currentView;

	idEntityPtr< idPlayer >					lastSpectatePlayer;
	int										lastSpectateUpdateTime;
	idVec3									lastSpectateOrigin;
	idQuat									lastSpectateAxis;

	usercmd_t								repeaterUserCmd;
	repeaterViewInfo_t						repeaterViewInfo;			
	sdCrosshairInfo							repeaterCrosshairInfo;

	idEntityPtr< idEntity >					activeViewProxy;	
	rvClientEntityPtr< sdClientAnimated >	cockpit;

	sdEffect								underWaterEffect;
	bool									underWaterEffectRunning;
};

#endif /* !__GAME_PLAYERVIEW_H__ */
