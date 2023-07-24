// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Player.h"
#include "PlayerView.h"
#include "rules/GameRules.h"
#include "vehicles/Transport.h"
#include "vehicles/VehicleView.h"
#include "guis/UserInterfaceLocal.h"
#include "Weapon.h"
#include "demos/DemoManager.h"
#include "script/Script_Helper.h"
#include "script/Script_ScriptObject.h"
#include "guis/GuiSurface.h"
#include "misc/WorldToScreen.h"
#include "roles/WayPointManager.h"
#include "Atmosphere.h"
#include "client/ClientEffect.h"
#include "ContentMask.h"

const int IMPULSE_DELAY = 150;

/*
==============
idPlayerView::idPlayerView
==============
*/
idPlayerView::idPlayerView( void ) {
	memset( &currentView, 0, sizeof( currentView ) );
	kickFinishTime = 0;
	kickAngles.Zero();
	swayAngles.Zero();
	lastDamageTime = 0.0f;
	fadeTime = 0;
	fadeRate = 0.0;
	fadeFromColor.Zero();
	fadeToColor.Zero();
	fadeColor.Zero();
	shakeAng.Zero();
	shakeAxes.Identity();
	lastSpectateUpdateTime = 0;

	ClearEffects();
	ClearRepeaterView();
}

/*
==============
idPlayerView::~idPlayerView
==============
*/
idPlayerView::~idPlayerView( void ) {
}

/*
==============
idPlayerView::ClearEffects
==============
*/
void idPlayerView::ClearEffects() {
	lastDamageTime = 0;

	kickFinishTime = 0;

	fadeTime = 0;

	underWaterEffect.StopDetach();
}

/*
==============
idPlayerView::DamageImpulse

LocalKickDir is the direction of force in the player's coordinate system,
which will determine the head kick direction
==============
*/
void idPlayerView::DamageImpulse( idVec3 localKickDir, const sdDeclDamage* damageDecl ) {
	//
	// double vision effect
	//
	if ( lastDamageTime > 0.0f && SEC2MS( lastDamageTime ) + IMPULSE_DELAY > gameLocal.time ) {
		// keep shotgun from obliterating the view
		return;
	}

	//
	// head angle kick
	//
	float kickTime = damageDecl->GetKickTime();
	if ( kickTime ) {
		kickFinishTime = gameLocal.time + static_cast< int >( g_kickTime.GetFloat() * kickTime );

		// forward / back kick will pitch view
		kickAngles[0] = localKickDir[0];

		// side kick will yaw view
		kickAngles[1] = localKickDir[1]*0.5f;

		// up / down kick will pitch view
		kickAngles[0] += localKickDir[2];

		// roll will come from  side
		kickAngles[2] = localKickDir[1];

		float kickAmplitude = damageDecl->GetKickAmplitude();
		if ( kickAmplitude ) {
			kickAngles *= kickAmplitude;
		}
	}

	//
	// save lastDamageTime for tunnel vision accentuation
	//
	lastDamageTime = MS2SEC( gameLocal.time );

}

/*
==================
idPlayerView::WeaponFireFeedback

Called when a weapon fires, generates head twitches, etc
==================
*/
void idPlayerView::WeaponFireFeedback( const weaponFeedback_t& feedback ) {
	// don't shorten a damage kick in progress
	if ( feedback.recoilTime != 0 && kickFinishTime < gameLocal.time ) {
		kickAngles = feedback.recoilAngles;
		int	finish = gameLocal.time + static_cast< int >( g_kickTime.GetFloat() * feedback.recoilTime );
		kickFinishTime = finish;
	}	

}

/*
===================
idPlayerView::SetActiveProxyView
===================
*/
void idPlayerView::SetActiveProxyView( idEntity* other, idPlayer* player ) {
	if ( activeViewProxy.IsValid() ) {
		activeViewProxy->GetUsableInterface()->StopActiveViewProxy();
		activeViewProxy = NULL;

		gameLocal.localPlayerProperties.OnActiveViewProxyChanged( NULL );
	}

	if ( other != NULL ) {
		other->GetUsableInterface()->BecomeActiveViewProxy( player );
		activeViewProxy = other;

		gameLocal.localPlayerProperties.OnActiveViewProxyChanged( other );
	}
}

/*
===================
idPlayerView::UpdateProxyView
===================
*/
void idPlayerView::UpdateProxyView( idPlayer* player, bool force ) {
	idEntity* proxy = player != NULL ? player->GetProxyEntity() : NULL;

	if ( activeViewProxy != proxy || force ) {
		SetActiveProxyView( proxy, player );
	}

	if ( activeViewProxy.IsValid() ) {
		activeViewProxy->GetUsableInterface()->UpdateProxyView( player );
	}
}

/*
===================
idPlayerView::CalculateShake
===================
*/
void idPlayerView::CalculateShake( idPlayer* other ) {
	idVec3	origin, matrix;

	float shakeVolume = gameSoundWorld->CurrentShakeAmplitudeForPosition( gameLocal.time, other->firstPersonViewOrigin );

	//
	// shakeVolume should somehow be molded into an angle here
	// it should be thought of as being in the range 0.0 -> 1.0, although
	// since CurrentShakeAmplitudeForPosition() returns all the shake sounds
	// the player can hear, it can go over 1.0 too.
	//

	if( shakeVolume ) {
		shakeAng[0] = gameLocal.random.CRandomFloat() * shakeVolume;
		shakeAng[1] = gameLocal.random.CRandomFloat() * shakeVolume;
		shakeAng[2] = gameLocal.random.CRandomFloat() * shakeVolume;
		shakeAxes = shakeAng.ToMat3();
	} else {
		shakeAng.Zero();
		shakeAxes.Identity();
	}
}

/*
===================
idPlayerView::ShakeAxis
===================
*/
const idMat3& idPlayerView::ShakeAxis( void ) const {
	return shakeAxes;
}

/*
===================
idPlayerView::AngleOffset
===================
*/
idAngles idPlayerView::AngleOffset( const idPlayer* player ) const {
	idAngles angles;

	angles.Zero();

	if ( gameLocal.time < kickFinishTime ) {
		int offset = kickFinishTime - gameLocal.time;

		angles = kickAngles * static_cast< float >( Square( offset ) ) * g_kickAmplitude.GetFloat();

		for ( int i = 0 ; i < 3 ; i++ ) {
			if ( angles[i] > 70.0f ) {
				angles[i] = 70.0f;
			} else if ( angles[i] < -70.0f ) {
				angles[i] = -70.0f;
			}
		}
	}

	if ( player ) {
		const idWeapon* weapon = player->GetWeapon();
		if ( weapon ) {
			weapon->SwayAngles( angles );
		}
	}

	return angles;
}

/*
==================
idPlayerView::SkipWorldRender
==================
*/
bool idPlayerView::SkipWorldRender( void ) {
	bool skipWorldRender =  false;
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer != NULL ) {
		sdHudModule* module = NULL;		
		for ( module = gameLocal.localPlayerProperties.GetDrawHudModule(); module; module = module->GetDrawNode().Next() ) {
			const sdUserInterfaceLocal* ui = module->GetGui();
			if( ui == NULL ) {
				continue;
			}
			if( ui->TestGUIFlag( sdUserInterfaceLocal::GUI_INHIBIT_GAME_WORLD ) ) {
				skipWorldRender = true;
				break;
			}			
		}
		if ( sdUserInterfaceLocal* scoreboardUI = gameLocal.GetUserInterface( gameLocal.localPlayerProperties.GetScoreBoard() ) ) {
			skipWorldRender |= scoreboardUI->TestGUIFlag( sdUserInterfaceLocal::GUI_INHIBIT_GAME_WORLD );
		}
	}
	return skipWorldRender;
}

/*
==================
idPlayerView::SingleView
==================
*/
void idPlayerView::SingleView( idPlayer* viewPlayer, const renderView_t* view ) {

	// normal rendering
	if ( view == NULL ) {
		return;
	}

	bool runEffect = false;
	idVec3 const &viewOrg = view->vieworg;
	if ( !g_skipLocalizedPrecipitation.GetBool() && gameLocal.GetLocalPlayer() != NULL ) {
		idBounds checkBounds( viewOrg );
		checkBounds.ExpandSelf( 8.0f );
		const idClipModel* waterModel;
		int found = gameLocal.clip.FindWater( CLIP_DEBUG_PARMS checkBounds, &waterModel, 1 );
		int cont = 0;
		if ( found ) {
			cont = gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS viewOrg, NULL, mat3_identity, CONTENTS_WATER, waterModel, waterModel->GetOrigin(), waterModel->GetAxis() );
		}

		runEffect = (cont & CONTENTS_WATER) ? true : false;
	}
	// Update the background effect
	if ( runEffect ) {
		underWaterEffect.GetRenderEffect().origin = viewOrg;
		if ( !underWaterEffectRunning ) {
			SetupEffect();
			underWaterEffect.Start( gameLocal.time );
			underWaterEffectRunning = true;
		} else {
			underWaterEffect.Update();
		}
	} else {
		underWaterEffect.StopDetach();
		underWaterEffectRunning = false;
	}


	if ( viewPlayer != NULL ) {
		sdClientAnimated* sight = viewPlayer->GetSight();
		if ( sight != NULL ) {
			float fovx, fovy;
			gameLocal.CalcFov( viewPlayer->GetSightFOV(), fovx, fovy );
			sight->GetRenderEntity()->weaponDepthHackFOV_x = fovx;
			sight->GetRenderEntity()->weaponDepthHackFOV_y = fovy;

			sight->SetOrigin( view->vieworg );
			sight->SetAxis( view->viewaxis );
			sight->Present();
		}
	}

	if ( !SkipWorldRender() ) {
		idFrustum viewFrustum;

		BuildViewFrustum( view, viewFrustum );

		// emit render commands for in-world guis
		DrawWorldGuis( view, viewFrustum );

		gameRenderWorld->UpdatePortalOccTestView( view->viewID );

		UpdateOcclusionQueries( view );

		// draw the game
		gameRenderWorld->RenderScene( view );
	}

	sdDemoManager::GetInstance().SetRenderedView( *view );
}

/*
==================
idPlayerView::SingleView2D
==================
*/
void idPlayerView::SingleView2D( idPlayer* viewPlayer ) {
	sdPostProcess* postProcess = gameLocal.localPlayerProperties.GetPostProcess();
	postProcess->DrawPost();		

	if ( sdDemoManager::GetInstance().g_showDemoHud.GetBool() && sdDemoManager::GetInstance().InPlayBack() ) {
		sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( sdDemoManager::GetInstance().GetHudHandle() );

		if ( ui != NULL ) {
			ui->Draw();
		}
	}	

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer != NULL || gameLocal.serverIsRepeater ) {
		bool drawHud;
		if ( gameLocal.serverIsRepeater ) {
			drawHud = true;
		} else {
			drawHud = !localPlayer->InhibitHud();
		}

		if ( viewPlayer != NULL ) {
			idEntity* proxy = viewPlayer->GetProxyEntity();
			if ( proxy != NULL ) {
				drawHud &= !proxy->GetUsableInterface()->GetHideHud( viewPlayer );
			}
		}
		
		sdHudModule* module = NULL;		
		for ( module = gameLocal.localPlayerProperties.GetDrawHudModule(); module; module = module->GetDrawNode().Next() ) {
			if( module->ManualDraw() ) {
				continue;
			}
			if( drawHud || !module->AllowInhibit() ) {
				module->Draw();
			}			
		}
			// jrad - the scoreboard should top everything
		if ( drawHud ) {
			DrawScoreBoard( localPlayer );
		}		
	}

	// test a single material drawn over everything
	if ( g_testPostProcess.GetString()[0] ) {
		const idMaterial* mtr = declHolder.declMaterialType.LocalFind( g_testPostProcess.GetString(), false );
		if ( mtr == NULL ) {
			gameLocal.Printf( "Material not found.\n" );
			g_testPostProcess.SetString( "" );
		} else {
			deviceContext->SetColor( 1.0f, 1.0f, 1.0f, 1.0f );
			deviceContext->DrawRect( 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f, mtr );
		}
	}

	if ( !g_skipViewEffects.GetBool() ) {
		ScreenFade();
	}
}

/*
=================
idPlayerView::Flash

flashes the player view with the given color
=================
*/
void idPlayerView::Flash( const idVec4& color, int time ) {
	Fade( idVec4( 0.0f, 0.0f, 0.0f, 0.0f ), time );
	fadeFromColor = color;
}

/*
=================
idPlayerView::Fade

used for level transition fades
assumes: color.w is 0 or 1
=================
*/
void idPlayerView::Fade( const idVec4& color, int time ) {

	if ( !fadeTime ) {
		fadeFromColor.Set( 0.0f, 0.0f, 0.0f, 1.0f - color[ 3 ] );
	} else {
		fadeFromColor = fadeColor;
	}
	fadeToColor = color;

	if ( time <= 0 ) {
		fadeRate = 0;
		time = 0;
		fadeColor = fadeToColor;
	} else {
		fadeRate = 1.0f / ( float )time;
	}

	if ( gameLocal.realClientTime == 0 && time == 0 ) {
		fadeTime = 1;
	} else {
		fadeTime = gameLocal.realClientTime + time;
	}
}

/*
=================
idPlayerView::ScreenFade
=================
*/
void idPlayerView::ScreenFade() {
	int		msec;
	float	t;

	if ( !fadeTime ) {
		return;
	}

	msec = fadeTime - gameLocal.realClientTime;

	if ( msec <= 0 ) {
		fadeColor = fadeToColor;
		if ( fadeColor[ 3 ] == 0.0f ) {
			fadeTime = 0;
		}
	} else {
		t = ( float )msec * fadeRate;
		fadeColor = fadeFromColor * t + fadeToColor * ( 1.0f - t );
	}

	if ( fadeColor[ 3 ] != 0.0f ) {
		deviceContext->SetColor( fadeColor[ 0 ], fadeColor[ 1 ], fadeColor[ 2 ], fadeColor[ 3 ] );
		float aspectRatio = deviceContext->GetAspectRatioCorrection();
		deviceContext->DrawRect( 0, 0, SCREEN_WIDTH * 1.0f / aspectRatio, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f, declHolder.declMaterialType.LocalFind( "_white" ) );
	}
}

/*
================
idPlayerView::DrawScoreBoard
================
*/
void idPlayerView::DrawScoreBoard( idPlayer* player ) {
	sdUserInterfaceLocal* scoreboardUI = gameLocal.GetUserInterface( gameLocal.localPlayerProperties.GetScoreBoard() );
	if ( scoreboardUI == NULL ) {
		return;
	}

	bool skipWorldRender = SkipWorldRender();

	bool showScores;
	if ( sdDemoManager::GetInstance().InPlayBack() ) {
		showScores = sdDemoManager::GetInstance().GetDemoCommand().clientButtons.showScores;
	} else if ( player != NULL ) {
		showScores = player->usercmd.clientButtons.showScores;
	} else if ( gameLocal.serverIsRepeater ) {
		showScores = gameLocal.playerView.GetRepeaterUserCmd().clientButtons.showScores;
	} else {
		showScores = false;
	}

	if ( ( showScores && !skipWorldRender ) || gameLocal.localPlayerProperties.ShouldShowEndGame() ) {
		if( !scoreboardUI->IsActive() ) {
			scoreboardUI->Activate();
			if( gameLocal.localPlayerProperties.ShouldShowEndGame() ) {
				sdChatMenu* menu = gameLocal.localPlayerProperties.GetChatMenu();
				if( menu->Enabled() ) {
					menu->Enable( false );
				}
				sdLimboMenu* limboMenu = gameLocal.localPlayerProperties.GetLimboMenu();
				if ( limboMenu->Enabled() ) {
					limboMenu->Enable( false );
				}
			}
		}		
		scoreboardUI->Draw();

		if( gameLocal.localPlayerProperties.ShouldShowEndGame() ) {
			gameLocal.OnEndGameScoreboardActive();
		}
	} else if( scoreboardUI->IsActive() ){
		scoreboardUI->Deactivate();
	}
}

/*
===================
idPlayerView::ClearRepeaterView
===================
*/
void idPlayerView::ClearRepeaterView( void ) {
	repeaterViewInfo.deltaViewAngles.Zero();
	repeaterViewInfo.viewAngles.Zero();
	repeaterViewInfo.origin.Zero();
	repeaterViewInfo.velocity.Zero();
	repeaterViewInfo.foundSpawn = false;
	repeaterCrosshairInfo.Invalidate();
}

/*
===================
idPlayerView::CalcRepeaterCrosshairInfo
===================
*/
const sdCrosshairInfo& idPlayerView::CalcRepeaterCrosshairInfo( void ) {
	idPlayer* viewer = gameLocal.GetActiveViewer();

	float range = 8192;
	float radius = 8.f;

	idEntity* proxy = NULL;
	if ( viewer != NULL ) {
		proxy = viewer->GetProxyEntity();
	}
	if ( proxy != NULL ) {
		proxy->DisableClip( false );
	}

	idVec3 vieworg	= repeaterViewInfo.origin;
	idMat3 viewaxes	= repeaterViewInfo.viewAngles.ToMat3();

	trace_t trace;
	memset( &trace, 0, sizeof( trace ) );
	idVec3 end = vieworg + ( viewaxes[ 0 ] * range );

	bool retVal = gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, vieworg, end, MASK_SHOT_RENDERMODEL | CONTENTS_SHADOWCOLLISION | CONTENTS_SLIDEMOVER | CONTENTS_BODY | CONTENTS_PROJECTILE | CONTENTS_CROSSHAIRSOLID, viewer, TM_CROSSHAIR_INFO, radius );

	// set up approximate distance
	if ( trace.c.material != NULL && trace.c.material->GetSurfaceFlags() & SURF_NOIMPACT ) {
		repeaterCrosshairInfo.SetDistance( range );
	} else {
		repeaterCrosshairInfo.SetDistance( range * trace.fraction );
	}

	if ( retVal ) {
		if ( trace.c.entityNum != ENTITYNUM_NONE && trace.c.entityNum != ENTITYNUM_WORLD ) {

			idEntity* traceEnt = gameLocal.entities[ trace.c.entityNum ];

			if ( repeaterCrosshairInfo.GetEntity() != traceEnt || !repeaterCrosshairInfo.IsValid() ) {
				repeaterCrosshairInfo.SetEntity( traceEnt );
				repeaterCrosshairInfo.SetStartTime( gameLocal.time );
			}

			if ( traceEnt != NULL ) {
				if ( traceEnt->UpdateCrosshairInfo( viewer, repeaterCrosshairInfo ) ) {
					repeaterCrosshairInfo.Validate();
				}
			}
		}
	}

	if ( proxy != NULL ) {
		proxy->EnableClip();
	}

	repeaterCrosshairInfo.SetTrace( trace );
	return repeaterCrosshairInfo;
}

/*
===================
idPlayerView::SetRepeaterUserCmd
===================
*/
void idPlayerView::SetRepeaterUserCmd( const usercmd_t& usercmd ) {
	if ( ( usercmd.buttons.btn.altAttack ) != ( repeaterUserCmd.buttons.btn.altAttack ) ) {
		if ( usercmd.buttons.btn.altAttack ) {
			gameLocal.ChangeLocalSpectateClient( -1 );
		}
	}
	if ( ( usercmd.buttons.btn.attack ) != ( repeaterUserCmd.buttons.btn.attack ) ) {
		if ( usercmd.buttons.btn.attack ) {
			int followIndex = gameLocal.GetRepeaterFollowClientIndex();

			int index = followIndex;
			int count = MAX_ASYNC_CLIENTS;
			while ( count > 0 ) {
				count--;
				index++;
				if ( index >= MAX_ASYNC_CLIENTS ) {
					index = 0;
				}

				if ( gameLocal.GetClient( index ) != NULL ) {
					break;
				}
			}
			if ( count != 0 ) {
				if ( index != followIndex ) {
					gameLocal.ChangeLocalSpectateClient( index );
				}
			}
		}
	}
	repeaterUserCmd = usercmd;
}

/*
===================
idPlayerView::UpdateRepeaterView
===================
*/
void idPlayerView::UpdateRepeaterView( void ) {
	repeaterUserOrigin_t userOrigin;
	userOrigin.followClient = gameLocal.GetRepeaterFollowClientIndex();

	if ( userOrigin.followClient != -1 ) {
		idPlayer* player = gameLocal.GetClient( userOrigin.followClient );
		assert( player != NULL );

		renderView_t* view = player->GetRenderView();

		repeaterViewInfo.origin = view->vieworg;
		repeaterViewInfo.velocity.Zero();
		repeaterViewInfo.viewAngles = view->viewaxis.ToAngles();
	} else {
		// update camera origin
		for( int i = 0; i < 3; i++ ) {
			repeaterViewInfo.viewAngles[ i ] = idMath::AngleNormalize180( SHORT2ANGLE( repeaterUserCmd.angles[ i ] ) + repeaterViewInfo.deltaViewAngles[ i ] );
		}

		repeaterViewInfo.viewAngles.pitch = idMath::ClampFloat( -89.f, 89.f, repeaterViewInfo.viewAngles.pitch );
		repeaterViewInfo.viewAngles.roll = 0.f;

		if ( !repeaterViewInfo.foundSpawn ) {
			if ( gameLocal.SelectInitialSpawnPointForRepeaterClient( repeaterViewInfo.origin, repeaterViewInfo.viewAngles ) ) {
				repeaterViewInfo.velocity.Zero();
				repeaterViewInfo.foundSpawn = true;
			}
		}

		// calculate vectors
		idVec3 gravityNormal = gameLocal.GetGravity();
		gravityNormal.Normalize();

		idVec3 viewForward = repeaterViewInfo.viewAngles.ToForward();
		viewForward.Normalize();
		idVec3 viewRight = gravityNormal.Cross( viewForward );
		viewRight.Normalize();

		// scale user command
		int		max;
		float	scale;

		int forwardmove = repeaterUserCmd.forwardmove;
		int rightmove = repeaterUserCmd.rightmove;
		int upmove = repeaterUserCmd.upmove;

		max = abs( forwardmove );
		if ( abs( rightmove ) > max ) {
			max = abs( rightmove );
		}
		if ( abs( upmove ) > max ) {
			max = abs( upmove );
		}

		if ( !max ) {
			scale = 0.0f;
		} else {
			float total = idMath::Sqrt( (float) forwardmove * forwardmove + rightmove * rightmove + upmove * upmove );
			scale = pm_democamspeed.GetFloat() * max / ( 127.0f * total );
		}

		// move
		float	frametime = MS2SEC( USERCMD_MSEC );

		float	speed, drop, friction, newspeed, stopspeed;
		float	wishspeed;
		idVec3	wishdir;

		// friction
		speed = repeaterViewInfo.velocity.Length();
		if ( speed < 20.f ) {
			repeaterViewInfo.velocity.Zero();
		} else {
			stopspeed = pm_democamspeed.GetFloat() * 0.3f;
			if ( speed < stopspeed ) {
				speed = stopspeed;
			}
			friction = 12.0f;
			drop = speed * friction * frametime;

			// scale the velocity
			newspeed = speed - drop;
			if (newspeed < 0) {
				newspeed = 0;
			}

			repeaterViewInfo.velocity *= newspeed / speed;
		}

		// accelerate
		wishdir = scale * (viewForward * repeaterUserCmd.forwardmove + viewRight * repeaterUserCmd.rightmove);
		wishdir -= scale * gravityNormal * repeaterUserCmd.upmove;
		wishspeed = wishdir.Normalize();
		wishspeed *= scale;

		// q2 style accelerate
		float addspeed, accelspeed, currentspeed;

		currentspeed = repeaterViewInfo.velocity * wishdir;
		addspeed = wishspeed - currentspeed;
		if ( addspeed > 0 ) {
			accelspeed = 10.0f * frametime * wishspeed;
			if ( accelspeed > addspeed ) {
				accelspeed = addspeed;
			}
			
			repeaterViewInfo.velocity += accelspeed * wishdir;
		}

		// move
		idVec3 velocity = repeaterViewInfo.velocity;
		float timeLeft = frametime;

		int count = 10;
		while ( count > 0 ) {
			idVec3 newOrigin = repeaterViewInfo.origin + ( timeLeft * velocity );

			idBounds bounds;
			idPhysics_Player::CalcSpectateBounds( bounds );
			const idClipModel* clipModel = gameLocal.clip.GetTemporaryClipModel( bounds );

			trace_t trace;
			gameLocal.clip.TranslationWorld( trace, repeaterViewInfo.origin, newOrigin, clipModel, mat3_identity, CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BODY ); 
			if ( trace.fraction != 1.f ) {
				repeaterViewInfo.origin = trace.endpos;
				timeLeft -= timeLeft * trace.fraction;
				velocity.ProjectOntoPlane( trace.c.normal, 1.001f );
			} else {
				repeaterViewInfo.origin = newOrigin;
				break;
			}

			count--;
		}
	}

	for( int i = 0; i < 3; i++ ) {
		repeaterViewInfo.deltaViewAngles[ i ] = repeaterViewInfo.viewAngles[ i ] - SHORT2ANGLE( repeaterUserCmd.angles[ i ] );
	}

	userOrigin.origin = repeaterViewInfo.origin;

#ifdef SD_SUPPORT_REPEATER
	networkSystem->SetClientRepeaterUserOrigin( userOrigin );
#endif // SD_SUPPORT_REPEATER
}

/*
===================
idPlayerView::CalculateRepeaterView
===================
*/
void idPlayerView::CalculateRepeaterView( renderView_t& view ) {
	// jrad - ensure that this is running updates
	sdPostProcess* postProcess = gameLocal.localPlayerProperties.GetPostProcess();
	if ( postProcess != NULL && !postProcess->Enabled() ) {
		postProcess->Enable( true );
	}

	view.viewID = 0;
	view.vieworg = repeaterViewInfo.origin;
	view.viewaxis = repeaterViewInfo.viewAngles.ToMat3();
	view.fov_x = g_fov.GetFloat();

	view.time = gameLocal.time;

	view.x = 0;
	view.y = 0;
	view.width = SCREEN_WIDTH;
	view.height = SCREEN_HEIGHT;
}

/*
===================
idPlayerView::SetRepeaterViewPosition
===================
*/
void idPlayerView::SetRepeaterViewPosition( const idVec3& origin, const idAngles& angles ) {
	repeaterViewInfo.origin = origin;

	// set the delta angle
	idAngles delta;
	for( int i = 0; i < 3; i++ ) {
		delta[ i ] = angles[ i ] - SHORT2ANGLE( repeaterUserCmd.angles[ i ] );
	}

	repeaterViewInfo.deltaViewAngles = delta;
}

/*
===================
idPlayerView::RenderPlayerView
===================
*/
bool idPlayerView::RenderPlayerView( idPlayer* viewPlayer ) {
	// hack the shake in at the very last moment, so it can't cause any consistency problems
	memset( &currentView, 0, sizeof( currentView ) );

	// allow demo manager to override the view
	if ( sdDemoManager::GetInstance().CalculateRenderView( &currentView ) ) {
		// place the sound origin for the player
		gameSoundWorld->PlaceListener( currentView.vieworg, currentView.viewaxis, -1, gameLocal.time );
		
		// field of view
		gameLocal.CalcFov( currentView.fov_x, currentView.fov_x, currentView.fov_y );
	} else {
		int listenerId = -1;
		if ( viewPlayer == NULL ) {
			if ( gameLocal.serverIsRepeater ) {
				CalculateRepeaterView( currentView );

				// field of view
				gameLocal.CalcFov( currentView.fov_x, currentView.fov_x, currentView.fov_y );
			} else {
				return false;
			}
		} else {
			const renderView_t& view = viewPlayer->renderView;

			currentView = view;

			// readjust the view one frame earlier for regular game draws (again: for base draws, not for extra draws)
			// extra draws do this in their own loop
			if ( gameLocal.com_unlockFPS->GetBool() && !gameLocal.unlock.unlockedDraw && gameLocal.unlock.canUnlockFrames ) {
				if ( g_unlock_updateViewpos.GetBool() && g_unlock_viewStyle.GetInteger() == 1 ) {
					currentView.vieworg = gameLocal.unlock.originlog[ ( gameLocal.framenum + 1 ) & 1 ];
					if ( viewPlayer->weapon != NULL ) {
						viewPlayer->weapon->GetRenderEntity()->origin -= gameLocal.unlock.originlog[ gameLocal.framenum & 1 ] - gameLocal.unlock.originlog[ ( gameLocal.framenum + 1 ) & 1 ];
						viewPlayer->weapon->BecomeActive( TH_UPDATEVISUALS );
						viewPlayer->weapon->Present();

						// if the weapon has a GUI bound, offset it as well
						for ( rvClientEntity* cent = viewPlayer->weapon->clientEntities.Next(); cent != NULL; cent = cent->bindNode.Next() ) {

							sdGuiSurface* guiSurface;
							rvClientEffect* effect;
							if ( ( guiSurface = cent->Cast< sdGuiSurface >() ) != NULL ) {
								idVec3 o;
								guiSurface->GetRenderable().GetOrigin( o );
								o -= gameLocal.unlock.originlog[ gameLocal.framenum & 1 ] - gameLocal.unlock.originlog[ ( gameLocal.framenum + 1 ) & 1 ];
								guiSurface->GetRenderable().SetOrigin( o );
							} else if ( ( effect = cent->Cast< rvClientEffect >() ) != NULL ) {
								effect->ClientUpdateView();
							}
						}
					}
				}
			}

			// shakey shake
			currentView.viewaxis = currentView.viewaxis * ShakeAxis();

			listenerId = viewPlayer->entityNumber + 1;
		}

		gameLocal.UpdateHudStats( viewPlayer );

		// place the sound origin for the player
		gameSoundWorld->PlaceListener( currentView.vieworg, currentView.viewaxis, listenerId, gameLocal.time );
	}

	// Special view mode is enabled
	if ( *g_testViewSkin.GetString() != '\0' ) {
		const idDeclSkin *skin = declHolder.declSkinType.LocalFind( g_testViewSkin.GetString(), false );
		currentView.globalSkin = skin;
		if ( !skin ) {
			gameLocal.Printf( "Skin not found.\n" );
			g_testViewSkin.SetString( "" );
		}
	} else {
		if ( viewPlayer ) {
			currentView.globalSkin = viewPlayer->GetViewSkin();
		} else {
			currentView.globalSkin = NULL;
		}
	}

	currentView.forceClear = g_forceClear.GetBool();

	if ( sdAtmosphere::currentAtmosphere == NULL ) {
		currentView.clearColor.Set( 0.0f, 0.0f, 0.0f, 0.0f );
	} else {
		sdAtmosphere	*atmos = sdAtmosphere::currentAtmosphere;
		idVec3 fogcol = atmos->GetFogColor();
		currentView.clearColor.Set( fogcol[0], fogcol[1], fogcol[2], 0.f );
	}

	SingleView( viewPlayer, &currentView );

	return true;
}

/*
===================
idPlayerView::RenderPlayerView2D
===================
*/
bool idPlayerView::RenderPlayerView2D( idPlayer* viewPlayer ) {
	SingleView2D( viewPlayer );

	return true;
}

/*
===================
idPlayerView::SetupCockpit
===================
*/
void idPlayerView::SetupCockpit( const idDict& info, idEntity* vehicle ) {
	ClearCockpit();

	const char* cockpitDefName = info.GetString( "def_cockpit" );
	const idDeclEntityDef* cockpitDef = gameLocal.declEntityDefType[ cockpitDefName ];
	if ( !cockpitDef ) {
		return;
	}

	const char* scriptObjectName = info.GetString( "scriptobject", "default" );

	cockpit = new sdClientAnimated();
	cockpit->Create( &cockpitDef->dict, gameLocal.program->FindTypeInfo( scriptObjectName ) );

	idScriptObject* scriptObject = cockpit->GetScriptObject();
	if ( scriptObject ) {
		sdScriptHelper h1;
		h1.Push( vehicle->GetScriptObject() );
		scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnCockpitSetup" ), h1 );
	}
}

/*
===================
idPlayerView::ClearCockpit
===================
*/
void idPlayerView::ClearCockpit( void ) {
	if ( cockpit.IsValid() ) {
		cockpit->Dispose();
		cockpit = NULL;
	}
}

/*
===================
idPlayerView::BuildViewFrustum
===================
*/
void idPlayerView::BuildViewFrustum( const renderView_t* view, idFrustum& viewFrustum ) {
	float dNear, dFar, dLeft, dUp;

	dNear = 0.0f;
	dFar = MAX_WORLD_SIZE;
	dLeft = dFar * idMath::Tan( DEG2RAD( view->fov_x * 0.5f ) );
	dUp = dFar * idMath::Tan( DEG2RAD( view->fov_y * 0.5f ) );
	viewFrustum.SetOrigin( view->vieworg );
	viewFrustum.SetAxis( view->viewaxis );
	viewFrustum.SetSize( dNear, dFar, dLeft, dUp );
}

/*
===================
idPlayerView::DrawWorldGuis
===================
*/
void idPlayerView::DrawWorldGuis( const renderView_t* view, const idFrustum& viewFrustum ) {
	pvsHandle_t handle = gameLocal.pvs.SetupCurrentPVS( view->vieworg, PVS_ALL_PORTALS_OPEN );

	sdInstanceCollector< sdGuiSurface > guiSurfaces( false );
	for ( int i = 0; i < guiSurfaces.Num(); i++ ) {
		guiSurfaces[i]->DrawCulled( handle, viewFrustum );
	}

	gameLocal.pvs.FreeCurrentPVS( handle );
}

/*
===================
idPlayerView::UpdateOcclusionQueries
===================
*/
void idPlayerView::UpdateOcclusionQueries( const renderView_t* view ) {
	const idList< idEntity* > &ocl = gameLocal.GetOcclusionQueryList();

	for ( int i = 0; i < ocl.Num(); i++ ) {
		if ( ocl[i] == NULL ) {
			continue;
		}

		int handle = ocl[i]->GetOcclusionQueryHandle();
		if ( handle == -1 ) {
			continue;
		}

		const occlusionTest_t& occDef = ocl[i]->UpdateOcclusionInfo( view->viewID );
		gameRenderWorld->UpdateOcclusionTestDef( handle, &occDef );
	}
}

idCVar g_spectateViewLerpScale( "g_spectateViewLerpScale", "0.7", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Controls view smoothing for spectators", 0.2f, 1.f );

/*
===================
idPlayerView::UpdateSpectateView
===================
*/
void idPlayerView::UpdateSpectateView( idPlayer* other ) {
	const float lerpRate = g_spectateViewLerpScale.GetFloat();

	if ( other != NULL ) {
		if ( gameLocal.time > lastSpectateUpdateTime ) {
			lastSpectateUpdateTime = gameLocal.time;
			
			// this is a spectator view
			if ( lastSpectatePlayer != other ) {
				lastSpectatePlayer = other;
				lastSpectateOrigin = other->firstPersonViewOrigin;
				lastSpectateAxis = other->firstPersonViewAxis.ToQuat();
			} else {
				idVec3 diff = other->firstPersonViewOrigin - lastSpectateOrigin;
				if ( diff.LengthSqr() < Square( 1024.f ) ) {
					diff *= lerpRate;
					other->firstPersonViewOrigin = lastSpectateOrigin + diff;
				}
				lastSpectateOrigin = other->firstPersonViewOrigin;
				lastSpectateAxis = other->firstPersonViewAxis.ToQuat();
			}
		} else {
			other->firstPersonViewOrigin = lastSpectateOrigin;
			other->firstPersonViewAxis = lastSpectateAxis.ToMat3();
		}
	} else {
		lastSpectatePlayer = NULL;
	}
}

/*
===================
idPlayerView::Init
===================
*/
void idPlayerView::Init( void ) {
	lastSpectateUpdateTime = 0;
}

void idPlayerView::SetupEffect( void ) {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	
	renderEffect_t &renderEffect = underWaterEffect.GetRenderEffect();
	renderEffect.declEffect = gameLocal.FindEffect( localPlayer->spawnArgs.GetString( "fx_underWater" ), false );
	renderEffect.axis.Identity();
	renderEffect.loop = true;
	renderEffect.shaderParms[SHADERPARM_RED]		= 1.0f;
	renderEffect.shaderParms[SHADERPARM_GREEN]		= 1.0f;
	renderEffect.shaderParms[SHADERPARM_BLUE]		= 1.0f;
	renderEffect.shaderParms[SHADERPARM_ALPHA]		= 1.0f;
	renderEffect.shaderParms[SHADERPARM_BRIGHTNESS]	= 1.0f;

	underWaterEffectRunning = false;
}
