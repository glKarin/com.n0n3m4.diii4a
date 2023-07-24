// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop


#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UserInterfaceManager.h"
#include "UIWindow.h"
#include "UICrosshairInfo.h"
#include "UserInterfaceLocal.h"
#include "../CrosshairInfo.h"
#include "../Player.h"
#include "../misc/WorldToScreen.h"
#include "../script/Script_Helper.h"
#include "../roles/WayPointManager.h"
#include "../ContentMask.h"

#include "../../sys/sys_local.h"

idCVar g_damageIndicatorFadeTime( "g_damageIndicatorFadeTime", "2.0", CVAR_FLOAT | CVAR_GAME | CVAR_NOCHEAT, "number of seconds that a damage indicator stays visible" );
idCVar g_damageIndicatorWidth( "g_damageIndicatorWidth", "256", CVAR_FLOAT | CVAR_GAME | CVAR_NOCHEAT, "width of the damage indicators" );
idCVar g_damageIndicatorHeight( "g_damageIndicatorHeight", "128", CVAR_FLOAT | CVAR_GAME | CVAR_NOCHEAT, "height of the damage indicators" );
idCVar g_damageIndicatorColor( "g_damageIndicatorColor", "1 0 0", CVAR_GAME | CVAR_NOCHEAT, "color of the damage indicators" );
idCVar g_damageIndicatorAlphaScale( "g_damageIndicatorAlphaScale", "0.3", CVAR_FLOAT | CVAR_GAME | CVAR_NOCHEAT, "alpha of the damage indicators" );
idCVar g_repairIndicatorColor( "g_repairIndicatorColor", "0 1 0", CVAR_GAME | CVAR_NOCHEAT, "color of the repair indicators" );
idCVar g_repairIndicatorAlphaScale( "g_repairIndicatorAlphaScale", "0.3", CVAR_FLOAT | CVAR_GAME | CVAR_NOCHEAT, "alpha of the repair indicators" );
idCVar g_waypointAlphaScale( "g_waypointAlphaScale",	"0.7",	CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE,			"alpha to apply to world-based objective icons" );
idCVar g_showWayPoints( "g_showWayPoints",	"1",	CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE | CVAR_PROFILE, "show or hide world-based objective icons" );
idCVar g_waypointSizeMin( "g_waypointSizeMin", "16", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "min world-view icon size" );
idCVar g_waypointSizeMax( "g_waypointSizeMax", "32", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "max world-view icon size" );
idCVar g_waypointDistanceMin( "g_waypointDistanceMin", "512", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "max distance at which to show min icon size" );
idCVar g_waypointDistanceMax( "g_waypointDistanceMax", "3084", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "min distance at which to show max icon size" );

using namespace sdProperties;

const char sdUITemplateFunctionInstance_IdentifierCrosshairInfo[]	= "sdUICrosshairInfoFunction";
idHashMap< sdUICrosshairInfo::CrosshairInfoTemplateFunction* >	sdUICrosshairInfo::crosshairInfoFunctions;

const int sdUICrosshairInfo::lineTextScale = 12;

static const float HIGHLIGHT_TIME = 2000.0f;


/*
================
sdUICrosshairInfo::sdUICrosshairInfo
================
*/
sdUICrosshairInfo::sdUICrosshairInfo( void ) {

	crosshairParts.SetNum( CP_MAX );
	scriptState.GetProperties().RegisterProperty( "barMaterial",			barMaterial );
	scriptState.GetProperties().RegisterProperty( "barFillMaterial",		barFillMaterial );
	scriptState.GetProperties().RegisterProperty( "barLineMaterial",		barLineMaterial );
	scriptState.GetProperties().RegisterProperty( "barLineColor",			barLineColor );
	
	scriptState.GetProperties().RegisterProperty( "bracketBaseColor",		bracketBaseColor );	
	scriptState.GetProperties().RegisterProperty( "bracketLeftMaterial",	bracketLeftMaterial );	
	scriptState.GetProperties().RegisterProperty( "bracketRightMaterial",	bracketRightMaterial );	
	scriptState.GetProperties().RegisterProperty( "damageMaterial",			damageMaterial );
	scriptState.GetProperties().RegisterProperty( "crosshairInfoFlags",		crosshairInfoFlags );
	
	barLineColor = colorWhite;
	bracketBaseColor = colorWhite;

	UI_ADD_STR_CALLBACK( barMaterial,			sdUICrosshairInfo, OnBarMaterialChanged );
	UI_ADD_STR_CALLBACK( barFillMaterial,		sdUICrosshairInfo, OnBarFillMaterialChanged );
	UI_ADD_STR_CALLBACK( barLineMaterial,		sdUICrosshairInfo, OnBarLineMaterialChanged );
	UI_ADD_STR_CALLBACK( bracketLeftMaterial,	sdUICrosshairInfo, OnLeftBracketMaterialChanged );
	UI_ADD_STR_CALLBACK( bracketRightMaterial,	sdUICrosshairInfo, OnRightBracketMaterialChanged );

	UI_ADD_STR_CALLBACK( damageMaterial,		sdUICrosshairInfo, OnDamageMaterialChanged );

	waypointTextFadeTime = 0;
	bracketFadeTime = 0;	
	dockedIcons.SetNum( 6 );
}

/*
================
sdUICrosshairInfo::~sdUICrosshairInfo
================
*/
sdUICrosshairInfo::~sdUICrosshairInfo( void ) {
	DisconnectGlobalCallbacks();
}

/*
============
sdUICrosshairInfo::GetRepeaterCrosshairInfo
============
*/
const sdCrosshairInfo& sdUICrosshairInfo::GetRepeaterCrosshairInfo( void ) {
	return gameLocal.playerView.CalcRepeaterCrosshairInfo();
}

/*
============
sdUICrosshairInfo::DrawCrosshairInfo
============
*/
void sdUICrosshairInfo::DrawCrosshairInfo( idPlayer* player ) {
	const float width = SCREEN_WIDTH * 1.0f / deviceContext->GetAspectRatioCorrection();

	const sdCrosshairInfo& info = gameLocal.serverIsRepeater ? GetRepeaterCrosshairInfo() : player->GetCrosshairInfoDirect();	

	if( info.IsValid() && ( TestCrosshairInfoFlag( CF_CROSSHAIR ) || g_radialMenuStyle.GetInteger() == 1 ) ) {
		float w, h;
		idVec4 color;

		idVec2 point( ( width * 0.5f ), ( SCREEN_HEIGHT * 0.5f ) + 30.0f );

		for( int i = 0; i < info.GetNumLines(); i++ ) {
			const chInfoLine_t& line = info.GetLine( i );

			switch( line.type ) {
				case CI_TEXT: {
					sdBounds2D rect( 0.0f, point.y, width, lineTextHeight );

					color = line.foreColor;
					color[ 3 ] *= info.GetAlpha();
					sdUIWindow::DrawText( line.text.c_str(), color, lineTextScale, rect, cachedFontHandle, DTF_CENTER | DTF_BOTTOM | DTF_SINGLELINE | DTF_DROPSHADOW );

					point.y += lineTextHeight + 4.0f;
					break;
							  }
				case CI_BAR: {
					w = line.xy.x;
					h = line.xy.y;					

					color = borderColor;//line.backColor;
					color[ 3 ] *= info.GetAlpha();
					idVec4 rect( point.x - ( w * 0.5f ), point.y, w, h );

					DrawThreeHorizontalParts( rect, color, vec2_one, crosshairParts[ CP_BAR_FILL_LEFT ], crosshairParts[ CP_BAR_FILL_CENTER ], crosshairParts[ CP_BAR_FILL_RIGHT ] );

					color = line.foreColor;
					color[ 3 ] *= info.GetAlpha();

					sdBounds2D clipRect( rect.x, rect.y, rect.z * line.frac, rect.w );

					deviceContext->PushClipRect( clipRect );
					DrawHorizontalProgress( rect, color, vec2_one, crosshairParts[ CP_BAR_LEFT ], crosshairParts[ CP_BAR_CENTER ], crosshairParts[ CP_BAR_RIGHT ] );
					deviceContext->PopClipRect();

					color = barLineColor;
					color[ 3 ] *= info.GetAlpha();
					DrawThreeHorizontalParts( rect, color, vec2_one, crosshairParts[ CP_BAR_LINE_LEFT ], crosshairParts[ CP_BAR_LINE_CENTER ], crosshairParts[ CP_BAR_LINE_RIGHT ] );

					point.y += h + 4.0f;
					break;
							 }
				case CI_IMAGE: {

					color = line.foreColor;
					color[ 3 ] *= info.GetAlpha();

					deviceContext->DrawMaterial( point.x - ( line.xy.x * 0.5f ), point.y, line.xy.x, line.xy.y, line.material, color );

					point.y += line.xy.y + 4.0f;
					break;
				}
			}
		}
	}
}

/*
============
sdUICrosshairInfo::DrawDamageIndicators
============
*/
void sdUICrosshairInfo::DrawDamageIndicators( idPlayer* player ) {
	const float width = SCREEN_WIDTH * 1.0f / deviceContext->GetAspectRatioCorrection();

	static idWinding2D drawWinding;

	float w = g_damageIndicatorWidth.GetFloat();
	float h = g_damageIndicatorHeight.GetFloat();
	float x = -( w * 0.5f );
	float y = -( h * 0.5f ) - g_damageIndicatorHeight.GetFloat();

	float viewYaw = player->renderView.viewaxis.ToAngles().yaw;
	
	float fadeTime = SEC2MS( g_damageIndicatorFadeTime.GetFloat() );

	idMat2 matrix;

	for( int i = 0; i < player->damageEvents.Num(); i++ ) {
		const idPlayer::damageEvent_t& event = player->damageEvents[ i ];
		if( event.hitTime <= 0 ) {
			continue;
		}

		idVec4 color;
		if ( event.hitDamage > 0 ) {
			color.ToVec3() = sdTypeFromString< idVec3 >( g_damageIndicatorColor.GetString() );
		} else {
			color.ToVec3() = sdTypeFromString< idVec3 >( g_repairIndicatorColor.GetString() );
		}

		color.w = 1.0f - static_cast< float > ( gameLocal.time - event.hitTime ) / fadeTime;
		if ( event.hitDamage > 0 ) {
			color.w *= g_damageIndicatorAlphaScale.GetFloat();
		} else {
			color.w *= g_repairIndicatorAlphaScale.GetFloat();
		}
		if( color.w <= 0.0f || color.w > 1.0f ) {
			continue;
		}

		uiDrawPart_t* part = NULL;
		if( event.hitDamage <= 20.0f ) {
			part = &crosshairParts[ CP_SMALL_DAMAGE ];
		} else if( event.hitDamage <= 40.0f ) {
			part = &crosshairParts[ CP_MEDIUM_DAMAGE ];
		} else {
			part = &crosshairParts[ CP_LARGE_DAMAGE ];
		}
		
		drawWinding.Clear();

		drawWinding.AddPoint( x, y, part->mi.st0.x, part->mi.st0.y );
		drawWinding.AddPoint( x + w, y, part->mi.st1.x, part->mi.st0.y );
		drawWinding.AddPoint( x + w, y + h, part->mi.st1.x, part->mi.st1.y );
		drawWinding.AddPoint( x, y + h, part->mi.st0.x, part->mi.st1.y );

		float angle = event.hitAngle;
		if ( event.updateDirection ) {
			angle -= viewYaw;
		}
		matrix.Rotation( DEG2RAD( angle ) );

		for( int i = 0; i < drawWinding.GetNumPoints(); i++ ) {
			idVec2& pt = drawWinding[ i ];
			pt *= matrix;
			pt.x += width * 0.5f;
			pt.y += SCREEN_HEIGHT * 0.5f;
		}

		deviceContext->DrawWindingMaterial( drawWinding, part->mi.material, color );
	}
}

/*
============
sdUICrosshairInfo::DrawLocal
============
*/
void sdUICrosshairInfo::DrawLocal() {
	if( g_showCrosshairInfo.GetInteger() != 1 ) {
		return;
	}

	idPlayer* player = gameLocal.GetLocalViewPlayer();
	if ( player == NULL ) {
		if ( gameLocal.serverIsRepeater ) {
			DrawCrosshairInfo( NULL );
		}
		return;
	}

	idEntity* entity = DrawContextEntity( player );
	DrawWayPoints( entity );
	DrawEntityIcons();
	DrawPlayerIcons2D();
	ClearInactiveDockIcons();
	DrawDockedIcons();
	DrawCrosshairInfo( player );
	DrawDamageIndicators( player );
}

/*
============
sdUICrosshairInfo::ApplyLayout
============
*/
void sdUICrosshairInfo::ApplyLayout() {
	if( windowState.recalculateLayout ) {
		BeginLayout();
		lineTextHeight = deviceContext->GetFontHeight( cachedFontHandle, lineTextScale );
		EndLayout();
	}
	sdUIObject::ApplyLayout();
}

/*
============
sdUICrosshairInfo::FindFunction
============
*/
const sdUICrosshairInfo::CrosshairInfoTemplateFunction* sdUICrosshairInfo::FindFunction( const char* name ) {
	sdUICrosshairInfo::CrosshairInfoTemplateFunction** ptr;
	return crosshairInfoFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUICrosshairInfo::GetFunction
============
*/
sdUIFunctionInstance* sdUICrosshairInfo::GetFunction( const char* name ) {
	const CrosshairInfoTemplateFunction* function = sdUICrosshairInfo::FindFunction( name );
	if ( !function ) {		
		return sdUIWindow::GetFunction( name );
	}
	return new sdUITemplateFunctionInstance< sdUICrosshairInfo, sdUITemplateFunctionInstance_IdentifierCrosshairInfo >( this, function );
}

/*
============
sdUICrosshairInfo::RunNamedMethod
============
*/
bool sdUICrosshairInfo::RunNamedMethod( const char* name, sdUIFunctionStack& stack ) {
	const CrosshairInfoTemplateFunction* func = sdUICrosshairInfo::FindFunction( name );
	if ( !func ) {
		return sdUIWindow::RunNamedMethod( name, stack );
	}

	CALL_MEMBER_FN_PTR( this, func->GetFunction() )( stack );
	return true;
}

/*
============
sdUICrosshairInfo::InitFunctions
============
*/
void sdUICrosshairInfo::InitFunctions() {
	sdDeclGUI::AddDefine( va( "CF_CROSSHAIR %i", CF_CROSSHAIR ) );
	sdDeclGUI::AddDefine( va( "CF_PLAYER_ICONS %i", CF_PLAYER_ICONS ) );
	sdDeclGUI::AddDefine( va( "CF_TASKS %i", CF_TASKS ) );
	sdDeclGUI::AddDefine( va( "CF_OBJ_MIS %i", CF_OBJ_MIS ) );
}

/*
============
sdUICrosshairInfo::OnBarMaterialChanged
============
*/
void sdUICrosshairInfo::OnBarMaterialChanged( const idStr& oldValue, const idStr& newValue ) {
	GetUI()->SetupPart( crosshairParts[ CP_BAR_LEFT ], "b", newValue );
	GetUI()->SetupPart( crosshairParts[ CP_BAR_CENTER ], "c", newValue );
	GetUI()->SetupPart( crosshairParts[ CP_BAR_RIGHT ], "e", newValue );
}

/*
============
sdUICrosshairInfo::OnBarFillMaterialChanged
============
*/
void sdUICrosshairInfo::OnBarFillMaterialChanged( const idStr& oldValue, const idStr& newValue ) {
	GetUI()->SetupPart( crosshairParts[ CP_BAR_FILL_LEFT ], "l", newValue );
	GetUI()->SetupPart( crosshairParts[ CP_BAR_FILL_CENTER ], "c", newValue );
	GetUI()->SetupPart( crosshairParts[ CP_BAR_FILL_RIGHT ], "r", newValue );
}

/*
============
sdUICrosshairInfo::OnBarLineMaterialChanged
============
*/
void sdUICrosshairInfo::OnBarLineMaterialChanged( const idStr& oldValue, const idStr& newValue ) {
	GetUI()->SetupPart( crosshairParts[ CP_BAR_LINE_LEFT ], "l", newValue );
	GetUI()->SetupPart( crosshairParts[ CP_BAR_LINE_CENTER ], "c", newValue );
	GetUI()->SetupPart( crosshairParts[ CP_BAR_LINE_RIGHT ], "r", newValue );
}

/*
============
sdUICrosshairInfo::EndLevelLoad
============
*/
void sdUICrosshairInfo::EndLevelLoad() {
	sdUIWindow::EndLevelLoad();
	sdUserInterfaceLocal::LookupPartSizes( crosshairParts.Begin(), crosshairParts.Num() );
}

/*
============
sdUICrosshairInfo::OnDamageMaterialChanged
============
*/
void sdUICrosshairInfo::OnDamageMaterialChanged( const idStr& oldValue, const idStr& newValue ) {
	GetUI()->SetupPart( crosshairParts[ CP_SMALL_DAMAGE ], "s", newValue );
	GetUI()->SetupPart( crosshairParts[ CP_MEDIUM_DAMAGE ], "m", newValue );
	GetUI()->SetupPart( crosshairParts[ CP_LARGE_DAMAGE ], "l", newValue );
}

/*
============
sdUICrosshairInfo::OnLeftBracketMaterialChanged
============
*/
void sdUICrosshairInfo::OnLeftBracketMaterialChanged( const idStr& oldValue, const idStr& newValue ) {
	GetUI()->SetupPart( crosshairParts[ CP_L_BRACKET_TOP ], "t", newValue );
	GetUI()->SetupPart( crosshairParts[ CP_L_BRACKET_CENTER ], "c", newValue );
	GetUI()->SetupPart( crosshairParts[ CP_L_BRACKET_BOTTOM ], "b", newValue );
}

/*
============
sdUICrosshairInfo::OnRightBracketMaterialChanged
============
*/
void sdUICrosshairInfo::OnRightBracketMaterialChanged( const idStr& oldValue, const idStr& newValue ) {
	GetUI()->SetupPart( crosshairParts[ CP_R_BRACKET_TOP ], "t", newValue );
	GetUI()->SetupPart( crosshairParts[ CP_R_BRACKET_CENTER ], "c", newValue );
	GetUI()->SetupPart( crosshairParts[ CP_R_BRACKET_BOTTOM ], "b", newValue );
}



/*
===============
sdUICrosshairInfo::DrawEntityIcons
==============
*/
void sdUICrosshairInfo::DrawEntityIcons( void ) {	
	idPlayer* viewPlayer = gameLocal.GetLocalViewPlayer();
	if ( viewPlayer == NULL ) {
		return;
	}

	// TODO: Add crosshair info flag like for player icons?
	const float width = SCREEN_WIDTH * 1.0f / deviceContext->GetAspectRatioCorrection();

	sdWorldToScreenConverter converter( gameLocal.playerView.GetCurrentView() );

	// find out how many icons we need to draw
	int numIcons = 0;
	for ( idLinkList< idEntity >* node = gameLocal.GetIconEntities(); node; node = node->NextNode() ) {
		idEntity* ent = node->Owner();
		assert( ent->GetDisplayIconInterface() );

		if ( ent->GetDisplayIconInterface()->HasIcon( viewPlayer, converter ) ) {
			numIcons++;
		}
	}

	if ( numIcons == 0 ) {
		return;
	}

	// allocate temporary storage for the icons & fill them in
	sdEntityDisplayIconInfo* icons = ( sdEntityDisplayIconInfo* )_alloca16( numIcons * sizeof( sdEntityDisplayIconInfo ) );
	int iconUpto = 0;
	for ( idLinkList< idEntity >* node = gameLocal.GetIconEntities(); node; node = node->NextNode() ) {
		idEntity* ent = node->Owner();
		assert( ent->GetDisplayIconInterface() );
	
		if ( ent->GetDisplayIconInterface()->GetEntityDisplayIconInfo( viewPlayer, converter, icons[ iconUpto ] ) ) {
			iconUpto++;
		}
	}

	// sort them by distance
	sdEntityDisplayIconInfo** sortedIcons = ( sdEntityDisplayIconInfo** )_alloca16( numIcons * sizeof( sdEntityDisplayIconInfo* ) );
	for ( int i = 0; i < numIcons; i++ ) {
		sortedIcons[ i ] = &icons[ i ];
	}

	sdQuickSort( sortedIcons, sortedIcons + numIcons, sdEntityDisplayIconInfo::SortByDistance );

	// now we can draw
	for ( int i = 0; i < numIcons; i++ ) {
		sdEntityDisplayIconInfo* icon = sortedIcons[ i ];

		bool draw = true;

		const float MARGIN_OFFSET = ( width * 0.0125f );
		const float LEFT_MARGIN = MARGIN_OFFSET;
		const float RIGHT_MARGIN = width - MARGIN_OFFSET;

		if ( icon->origin.x + icon->size.x <= LEFT_MARGIN || icon->origin.x - icon->size.x >= RIGHT_MARGIN ) {
			draw = false;
		}

		if ( draw ) {
			float height = icon->origin.y;

			idVec2 screenPos( icon->origin.x - ( icon->size.x * 0.5f ), height - icon->size.y );

			deviceContext->DrawMaterial( screenPos.x, screenPos.y, icon->size.x, icon->size.y, icon->material, icon->color );
		}
	}
}

/*
===============
sdUICrosshairInfo::DrawPlayerIcons2D
==============
*/
void sdUICrosshairInfo::DrawPlayerIcons2D( void ) {	
	idPlayer* viewPlayer = gameLocal.GetLocalViewPlayer();
	if ( viewPlayer == NULL ) {
		return;
	}

	if ( viewPlayer->IsBeingBriefed() ) {
		return;
	}

	if ( !TestCrosshairInfoFlag( CF_PLAYER_ICONS ) ) {
		return;
	}

	const float width = SCREEN_WIDTH * 1.0f / deviceContext->GetAspectRatioCorrection();

	sdWorldToScreenConverter converter( gameLocal.playerView.GetCurrentView() );

	// fill the icons list up
	sdPlayerDisplayIconList playerIcons;
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* other = gameLocal.GetClient( i );
		if ( other == NULL || other->IsInLimbo() ) {
			continue;
		}

		if ( !pm_thirdPerson.GetBool() && viewPlayer == other ) {
			continue;
		}

		sdPlayerDisplayIcon icon;
		if ( other->GetPlayerIcon( viewPlayer, icon, converter ) ) {
			playerIcons.Append( icon );
		}
	}

	// sort the list by distance
	sdPlayerDisplayIconPtrList sortedPlayerIcons;
	sortedPlayerIcons.SetNum( playerIcons.Num() );
	for ( int i = 0; i < playerIcons.Num(); i++ ) {
		sortedPlayerIcons[ i ] = &playerIcons[ i ];
	}

	sortedPlayerIcons.Sort( sdPlayerDisplayIcon::SortByDistance );

	for ( int i = 0; i < sortedPlayerIcons.Num(); i++ ) {
		sdPlayerDisplayIcon* icon = sortedPlayerIcons[ i ];

		bool draw = true;

		const float HALF_ICON_WIDTH = icon->icon.size.x * 0.5f;
		const float MARGIN_OFFSET = ( width * 0.0125f );
		const float LEFT_MARGIN = MARGIN_OFFSET;
		const float RIGHT_MARGIN = width - MARGIN_OFFSET;

		sdBounds2D iconBounds;
		iconBounds.Clear();
		iconBounds.AddPoint( icon->icon.size * 0.5f );
		iconBounds.AddPoint( icon->arrowIcon.size * 0.5f );

		if ( icon->offScreenIcon.material != NULL ) {
			if ( icon->origin.x - iconBounds.GetMaxs().x <= LEFT_MARGIN ) {
				draw = false;

				idVec2 startOrg( LEFT_MARGIN, icon->origin.y );
				idVec2 endOrg( startOrg.x, SCREEN_HEIGHT * 0.5f );

				MakeDockIcon( icon->player, true, startOrg, endOrg, icon->offScreenIcon, icon->icon );
			} else if ( ( icon->origin.x + iconBounds.GetMaxs().x ) >= RIGHT_MARGIN ) {
				draw = false;

				idVec2 startOrg( RIGHT_MARGIN, icon->origin.y );
				idVec2 endOrg( startOrg.x, SCREEN_HEIGHT * 0.5f );

				MakeDockIcon( icon->player, false, startOrg, endOrg, icon->offScreenIcon, icon->icon );
			}
		} else {
			if ( icon->origin.x <= LEFT_MARGIN || ( icon->origin.x + icon->icon.size.x ) >= RIGHT_MARGIN ) {
				draw = false;
			}
		}

		if ( draw ) {
			float height = icon->origin.y;
			if ( icon->arrowIcon.material != NULL ) {
				idVec2 screenPos( icon->origin.x - ( icon->arrowIcon.size.x * 0.5f ), height - icon->arrowIcon.size.y );

				deviceContext->DrawMaterial( screenPos.x, screenPos.y, icon->arrowIcon.size.x, icon->arrowIcon.size.y, icon->arrowIcon.material, icon->arrowIcon.color );
				height -= icon->arrowIcon.size.y;
			}

			idVec2 screenPos( icon->origin.x - ( icon->icon.size.x * 0.5f ), height - icon->icon.size.y );

			deviceContext->DrawMaterial( screenPos.x, screenPos.y, icon->icon.size.x, icon->icon.size.y, icon->icon.material, icon->icon.color );
		}
	}
}

/*
============
sdUICrosshairInfo::DrawWayPoints
============
*/
void sdUICrosshairInfo::DrawWayPoints( idEntity* exclude ) {
	
	if ( !g_showWayPoints.GetBool() && !sdWayPointManager::GetInstance().GetShowWayPoints() ) {
		return;
	}

	if ( !TestCrosshairInfoFlag( CF_TASKS | CF_OBJ_MIS ) ) {
		return;
	}

	idVec4 color = colorWhite;

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer == NULL || localPlayer->IsSpectating() ) {
		return;
	}

	idPlayer* viewPlayer = gameLocal.GetLocalViewPlayer();
	if ( viewPlayer == NULL ) {
		return;
	}

	if ( viewPlayer->IsBeingBriefed() ) {
		return;
	}

	const float screenWidth = SCREEN_WIDTH * 1.0f / deviceContext->GetAspectRatioCorrection();

	const float MARGIN_SIZE = ( screenWidth * 0.0125f );
	const float LEFT_MARGIN = MARGIN_SIZE;
	const float RIGHT_MARGIN = screenWidth - MARGIN_SIZE;

	idVec3 org = viewPlayer->renderView.vieworg;
	idMat3 axes = viewPlayer->renderView.viewaxis;

	sdWorldToScreenConverter converter( gameLocal.playerView.GetCurrentView() );

	sdWayPoint* highlightedWaypoint = NULL;
	idVec2 bestDistanceFromCenter( idMath::INFINITY, idMath::INFINITY );
	idVec2 bestDrawCenter;
	float bestIconWidth;
	float bestWidth;
	float bestDistSqr;
	float bestLeft;

	for ( sdWayPoint* wayPoint = sdWayPointManager::GetInstance().GetFirstActiveWayPoint(); wayPoint != NULL; wayPoint = wayPoint->GetActiveNode().Next() ) {
		if ( !wayPoint->IsValid() ) {
			continue;
		}

		idBounds bounds		= wayPoint->GetBounds();
		idVec3	size		= bounds.GetSize();
		idVec3	center		= bounds.GetCenter();
		
		float smallestAxis = idMath::INFINITY;
		float largestAxis = -idMath::INFINITY;
		for( int i = 0; i < 3; i++ ) {
			if( size[ i ] > largestAxis ) {
				largestAxis = size[ i ];
			}
			if( size[ i ] < smallestAxis ) {
				smallestAxis = size[ i ];
			}
		}
		//bounds.GetMins().Set( -smallestAxis, -smallestAxis, -smallestAxis );
		//bounds.GetMaxs().Set( smallestAxis, smallestAxis, smallestAxis );

		//bounds.TranslateSelf( center - bounds.GetCenter() );

		idMat3	axes		= wayPoint->GetOrientation();
		idVec3	origin		= wayPoint->GetPosition();
		float	distSquare	= ( org - origin ).LengthSqr();
		float	fadeAlpha	= distSquare / Square( largestAxis * 2.0f ) - 0.25f;

		float playerDist = ( viewPlayer->GetPhysics()->GetOrigin() - wayPoint->GetPosition() ).LengthFast() - g_waypointDistanceMin.GetFloat();
		float waypointDist = g_waypointDistanceMax.GetFloat() - g_waypointDistanceMin.GetFloat();
		float rangePct = waypointDist != 0.0f ? idMath::ClampFloat( 0.0f, 1.0f, playerDist / waypointDist ) : 0.0f;
		float iconWidth = g_waypointSizeMin.GetFloat() + ( g_waypointSizeMax.GetFloat() - g_waypointSizeMin.GetFloat() ) * ( 1.0f - rangePct );

		if ( fadeAlpha <= 0.0f ) {
			continue;
		}
		fadeAlpha = idMath::ClampFloat( 0.0f, 1.0f, fadeAlpha );

		sdBounds2D screenBounds;
		converter.Transform( bounds, axes, origin, screenBounds );

		float left			= screenBounds.GetLeft();
		float right			= screenBounds.GetRight();
		float top			= screenBounds.GetTop();
		float bottom		= screenBounds.GetBottom();
		float width			= screenBounds.GetWidth();
		float height		= screenBounds.GetHeight();

		float visiblePct = idMath::ClampFloat( 0.0f, 100.0f, 100.0f * ( ( width * height ) / ( screenWidth * SCREEN_HEIGHT ) ) );

		// update the visibility of the waypoint
		idVec3 temp;
		bool isVisible = true;
		if ( wayPoint->ShouldCheckLineOfSight() ){
			isVisible = wayPoint->GetOwner()->IsVisibleOcclusionTest();

			// ao: implicitly know it's a task if there's LOS check
			if ( !TestCrosshairInfoFlag( CF_TASKS ) ) {
				isVisible = false;
			}
		} else {
			if ( !wayPoint->IsVisible() ) {
				isVisible = false;
			}

			// ao: implicitly know it's a mission/objective if there's no LOS check
			if ( !TestCrosshairInfoFlag( CF_OBJ_MIS ) ) {
				isVisible = false;
			} 
		}

		if ( !isVisible ) {
			visiblePct = 0.0f;
		}

		wayPoint->SetVisible( isVisible );

		// update highlighting
		if ( visiblePct > 0.05f && visiblePct < 40.f ) {
			if ( !wayPoint->IsHighlightActive() ) {
				wayPoint->MakeHighlightActive();
			}
		} else {
			if ( wayPoint->IsHighlightActive() ) {
				wayPoint->MakeHighlightInActive();
			}
		}

		idVec4 color = colorWhite;
		if ( wayPoint->GetFlags() & sdWayPoint::WF_TEAMCOLOR ) {
			idEntity* owner = wayPoint->GetOwner();
			if ( owner != NULL ) {
				teamAllegiance_t allegiance = owner->GetEntityAllegiance( viewPlayer );
				if ( allegiance != TA_NEUTRAL ) {
					color = idEntity::GetColorForAllegiance( allegiance );
				}
			}
		}

		float factor		= wayPoint->GetActiveFraction() * fadeAlpha;

		color[ 3 ] *= factor;

		idVec4 highlightColor = color;
		highlightColor[ 3 ] *= wayPoint->GetHighLightActiveFraction() * fadeAlpha;

		float scaleFactor = ( 1.0f - ( visiblePct / 0.33f ) );
		converter.Transform( bounds, axes, origin, screenBounds );

		idVec2 drawCenter = screenBounds.GetCenter();

		float resize = 0.0f;
		if ( gameLocal.ToGuiTime( gameLocal.time ) < wayPoint->GetFlashEndTime() ) {
			resize = 2.0f;
		}

		idVec2 drawSize( iconWidth + resize, iconWidth + resize );

		idVec2 drawHalfSize = drawSize * 0.5f;
		idVec2 drawOrigin = drawCenter - ( drawSize * 0.5f );
		drawOrigin.y = idMath::ClampFloat( MARGIN_SIZE, SCREEN_HEIGHT - MARGIN_SIZE - drawSize.y, drawOrigin.y );

		idVec2 distanceFromCenter(  idMath::Fabs( ( screenWidth * 0.5f ) - drawCenter.x ) / ( screenWidth * 0.5f ),
									idMath::Fabs( ( SCREEN_HEIGHT * 0.5f ) - drawCenter.y ) / ( SCREEN_HEIGHT * 0.5f ) );

		idVec4 localColor = colorWhite;
		
		if ( gameLocal.ToGuiTime( gameLocal.time ) < wayPoint->GetFlashEndTime() ) {
			localColor.w = idMath::Cos( gameLocal.ToGuiTime( gameLocal.time ) * 0.02f ) * 0.5f + 0.5f;
		}

		localColor.w = localColor.w * fadeAlpha * g_waypointAlphaScale.GetFloat();

		iconInfo_t icon;
		icon.material = wayPoint->GetOffScreenMaterial();
		icon.color = idVec4( color.x, color.y, color.z, idMath::ClampFloat( 0.35f, 1.0f, color.w * scaleFactor ) );
		icon.size = drawHalfSize;

		if ( drawOrigin.x <= LEFT_MARGIN ) {
			if ( localColor.w >= 0.01f ) {
				idVec2 startOrg( LEFT_MARGIN, drawOrigin.y );
				idVec2 endOrg( startOrg.x, SCREEN_HEIGHT * 0.5f );

				if ( isVisible ) {
					MakeDockIcon( wayPoint, true, endOrg, endOrg, icon, iconInfo_t() );
				}
			}
		} else if ( ( drawOrigin.x + drawSize.x ) >= RIGHT_MARGIN ) {
			if ( localColor.w >= 0.01f ) {
				idVec2 startOrg( RIGHT_MARGIN, drawOrigin.y );
				idVec2 endOrg( startOrg.x, SCREEN_HEIGHT * 0.5f );

				if ( isVisible ) {
					MakeDockIcon( wayPoint, false, endOrg, endOrg, icon, iconInfo_t() );
				}
			}
		} else {
			if ( isVisible ) {
				if ( wayPoint->GetMaterial() != NULL ) {
					float time = wayPoint->Selected();
					if( time >= 0.0f ) {
						float value = 1.0f - idMath::ClampFloat( 0.0f, 1.0f, static_cast< float >( gameLocal.time - time ) / HIGHLIGHT_TIME );
						deviceContext->SetRegister( 4, value );
					} else {
						deviceContext->SetRegister( 4, 0.0f );
					}
					deviceContext->DrawMaterial( drawOrigin.x, drawOrigin.y, drawSize.x, drawSize.y, wayPoint->GetMaterial(), localColor );
				}
			}
		}
		if( isVisible ) {
			sdDockedScreenIcon* icon = FindDockedIcon( wayPoint );
			if( icon != NULL ) {
				icon->highlightedTime = wayPoint->Selected();
			}
		}
		
		if( isVisible && distanceFromCenter.x < 0.1f && distanceFromCenter.y < 0.1f && distanceFromCenter.x < bestDistanceFromCenter.x && distanceFromCenter.y < bestDistanceFromCenter.y ) {
			waypointTextFadeTime = gameLocal.time;
			bestDistanceFromCenter = distanceFromCenter;
			highlightedWaypoint = wayPoint;
			bestDrawCenter = drawCenter;
			bestIconWidth = iconWidth;
			bestWidth = width;
			bestDistSqr = distSquare;
			bestLeft = left;
		}			

		if ( wayPoint->Bracketed() ) {
			if( exclude != NULL && wayPoint->GetOwner() == exclude ) {
				if( idMath::ClampFloat( 0.0f, 1.0f, idMath::Fabs( bracketFadeTime - gameLocal.time ) / ( float )sdWayPoint::ACTIVATE_TIME ) <= idMath::FLT_EPSILON ) {
					continue;
				}
			}
			idVec3 bracketOrigin = wayPoint->GetBracketPosition();

			converter.Transform( bounds, axes, bracketOrigin, screenBounds );

			DrawBrackets( screenBounds, highlightColor );
		}
	}

	static const int TEXT_FADE_TIME = SEC2MS( 1 );

	const sdCrosshairInfo& info = localPlayer->GetCrosshairInfoDirect();	
	if( !info.IsValid() && highlightedWaypoint != NULL && ( gameLocal.time - waypointTextFadeTime ) <= TEXT_FADE_TIME ) {
		static const int TEXT_SCALE = 12;
		idVec4 colorLocal = colorWhite;
		colorLocal[ 3 ] *= 1.0f - ( static_cast< float >( gameLocal.time - waypointTextFadeTime ) / TEXT_FADE_TIME );

		idWStrList args;
		args.SetNum( 2 );
		args[ 0 ] = va( L"%i", ( int )InchesToMetres( idMath::Sqrt( bestDistSqr ) ) );
		args[ 1 ] = common->LocalizeText( "game/meters" );

		idWStr text = common->LocalizeText( "game/range", args );

		sdBounds2D rect( bestLeft, bestDrawCenter.y + bestIconWidth, bestWidth, lineTextHeight );

		sdUIWindow::DrawText( text.c_str(), colorLocal, TEXT_SCALE, rect, cachedFontHandle, DTF_CENTER | DTF_BOTTOM | DTF_SINGLELINE | DTF_DROPSHADOW );

		rect.TranslateSelf( 0.0f, lineTextHeight );

		sdUIWindow::DrawText( highlightedWaypoint->GetText(), colorLocal, TEXT_SCALE, rect, cachedFontHandle, DTF_CENTER | DTF_BOTTOM | DTF_SINGLELINE | DTF_DROPSHADOW );
	}
}

/*
============
sdUICrosshairInfo::DrawDockedIcons
============
*/
void sdUICrosshairInfo::DrawDockedIcons( void ) {
	idVec2 offsets( 0.f, 0.f );

	const float DOCK_SPACING = 32.0f;

	for ( int i = 0; i < dockedIcons.Num(); i++ ) {
		const sdDockedScreenIcon& dock = dockedIcons[ i ];
		if ( !dock.active ) {
			continue;
		}

		if ( dock.arrowIcon.material == NULL ) {
			assert( false );
			continue;
		}

		float percent = static_cast< float >( gameLocal.time - ( dock.startTime + 300 ) ) / sdDockedScreenIcon::DOCK_ANIMATION_MS;
		percent = idMath::ClampFloat( 0.0f, 1.0f, percent );

		idVec2 drawOrigin = Lerp( dock.startOrigin, dock.endOrigin, percent );

		int index = dock.flipped ? 1 : 0;

		drawOrigin.y += offsets[ index ];

		float scale = dock.flipped ? 1.0f : -1.0f;

		float xOffset = 0.f;
		float yOffset = 0.f;

		float time = dock.highlightedTime;
		float value = 1.0f - idMath::ClampFloat( 0.0f, 1.0f, dock.highlightedTime < 0 ? 1.0f : ( static_cast< float >( gameLocal.time - time ) / HIGHLIGHT_TIME ) );
		float drawScale = 1.0f + ( 1.0f * value );
		deviceContext->SetRegister( 4, value );


		if ( dock.icon.material != NULL ) {
			idVec2 screenPos = drawOrigin;
			screenPos.y -= ( DOCK_SPACING * 0.5f );
			if ( !dock.flipped ) {
				screenPos.x -= dock.icon.size.x;
			}
			
			float drawScale = 1.0f;			

			deviceContext->DrawMaterial( screenPos.x, screenPos.y, dock.icon.size.x * drawScale, dock.icon.size.y * drawScale, dock.icon.material, dock.icon.color, 1.f );

			xOffset -= scale * dock.arrowIcon.size.x;

			yOffset = dock.icon.size.y;
		}

		idVec2 screenPos = drawOrigin;
		if ( dock.icon.material != NULL ) {
			screenPos.y -= ( DOCK_SPACING * 0.5f - dock.icon.size.y * 0.25f );
		} else {
			screenPos.y -= ( DOCK_SPACING * 0.5f );
		}
		if ( !dock.flipped ) {
			screenPos.x -= dock.arrowIcon.size.x;
		}
		deviceContext->DrawMaterial( xOffset + screenPos.x, screenPos.y, dock.arrowIcon.size.x * drawScale, dock.arrowIcon.size.y * drawScale, dock.arrowIcon.material, dock.arrowIcon.color, scale );
		if ( dock.arrowIcon.size.y > yOffset ) {
			yOffset = dock.arrowIcon.size.y;
		}

		offsets[ index ] += yOffset * 1.25f;
	}
}

/*
============
sdUICrosshairInfo::FindDockedIcon
============
*/
sdDockedScreenIcon*	sdUICrosshairInfo::FindDockedIcon( void* tag ) {
	for ( int i = 0; i < dockedIcons.Num(); i++ ) {
		if ( !dockedIcons[ i ].active ) {
			continue;
		}

		if ( dockedIcons[ i ].tag == tag ) {
			return &dockedIcons[ i ];
		}
	}
	return NULL;
}

/*
============
sdUICrosshairInfo::FindFreeDockedIcon
============
*/
sdDockedScreenIcon*	sdUICrosshairInfo::FindFreeDockedIcon( void ) {
	for( int i = 0; i < dockedIcons.Num(); i++ ) {
		if ( !dockedIcons[ i ].active ) {
			return &dockedIcons[ i ];
		}
	}
	return NULL;
}

/*
============
sdUICrosshairInfo::MakeDockIcon
============
*/
void sdUICrosshairInfo::MakeDockIcon( void* tag, bool flipped, const idVec2& startOrg, const idVec2& endOrg, const iconInfo_t& arrowInfo, const iconInfo_t& iconInfo ) {
	sdDockedScreenIcon* icon = FindDockedIcon( tag );
	if ( icon == NULL ) {
		icon = FindFreeDockedIcon();
		if ( icon == NULL ) {
			return;
		}
		icon->tag = tag;
		icon->startTime = gameLocal.time;
		icon->active = true;
	}

	icon->icon = iconInfo;
	icon->arrowIcon = arrowInfo;
	icon->endOrigin = endOrg;
	icon->flipped = flipped;
	icon->lastUpdateTime = gameLocal.time;
	icon->startOrigin = startOrg;

	// Can get some very large y values
	if ( icon->startOrigin.y > SCREEN_HEIGHT ) {
		icon->startOrigin.y = SCREEN_HEIGHT + icon->icon.size.y;
	} else if ( icon->startOrigin.y + icon->icon.size.y < 0 ) {
		icon->startOrigin.y = -icon->icon.size.y;
	}
}

/*
============
sdUICrosshairInfo::ClearInactiveDockIcons
============
*/
void sdUICrosshairInfo::ClearInactiveDockIcons( void ) {
	for ( int i = 0; i < dockedIcons.Num(); i++ ) {
		if ( dockedIcons[ i ].lastUpdateTime != gameLocal.time ) {
			dockedIcons[ i ].active = false;
		}
	}
}

/*
============
sdUICrosshairInfo::DrawBrackets
============
*/
void sdUICrosshairInfo::DrawBrackets( const sdBounds2D& bounds, const idVec4& color ) {
	const float screenWidth = SCREEN_WIDTH * 1.0f / deviceContext->GetAspectRatioCorrection();

	float left			= bounds.GetLeft();
	float right			= bounds.GetRight();
	float top			= bounds.GetTop();
	float bottom		= bounds.GetBottom();
	float width			= bounds.GetWidth();
	float height		= bounds.GetHeight();

	idVec2 center( left + width * 0.5f, top + height * 0.5f );

	float size = width * 0.25f;

	//
	// left half
	//
	idVec4 rect;
	rect.Set( left - crosshairParts[ CP_L_BRACKET_TOP ].width, top, crosshairParts[ CP_L_BRACKET_TOP ].width, height );

	DrawThreeVerticalParts( rect, color, vec2_one, crosshairParts[ CP_L_BRACKET_TOP ], crosshairParts[ CP_L_BRACKET_CENTER ], crosshairParts[ CP_L_BRACKET_BOTTOM ] );
	
	rect.Set( right, top, crosshairParts[ CP_L_BRACKET_TOP ].width, height );
	DrawThreeVerticalParts( rect, color, vec2_one, crosshairParts[ CP_R_BRACKET_TOP ], crosshairParts[ CP_R_BRACKET_CENTER ], crosshairParts[ CP_R_BRACKET_BOTTOM ] );	
	
}

/*
============
sdUICrosshairInfo::DrawContextEntity
============
*/
idEntity* sdUICrosshairInfo::DrawContextEntity( idPlayer* player ) {
	return NULL;
	/* jrad - disabled this, as it was confusing
	idEntity* entity = gameLocal.localPlayerProperties.GetContextEntity();

	// see if we've moused over something that can be acted on
	if( entity == NULL ) {
		const sdProgram::sdFunction* callback = player->GetScriptFunction( "ContextUpdateOrder" );
		if ( callback != NULL ) {
			sdScriptHelper h;
			h.Push( "" );
			player->CallNonBlockingScriptEvent( callback, h );

			const char* returnVal = h.GetReturnedString();
			idStrList list;
			idSplitStringIntoList( list, returnVal, "|" );
			bool enabled;
			if ( list.Num() >= 5 ) {
				enabled = sdTypeFromString< bool >( list[ 4 ] );
			} else {
				enabled = true;
			}
			if ( enabled && list.Num() >= 6 ) {
				enabled &= list[ 5 ].Icmp( "Invalid" ) != 0;
			}
			if( enabled ) {
				const sdCrosshairInfo& info = player->GetCrosshairInfoDirect();
				entity = info.GetEntity();
				bracketFadeTime = gameLocal.time;
				lastBracketEntity = entity;
			}
		}
	}

	// try using a cached one
	if( entity == NULL ) {
		entity = lastBracketEntity;
	}

	if( entity != NULL ) {
		idVec3 org = entity->GetPhysics()->GetOrigin();
		idMat3 axes = entity->GetAxis();
		idBounds bounds = entity->GetPhysics()->GetBounds();

		sdWorldToScreenConverter converter( gameLocal.playerView.GetCurrentView() );

		sdBounds2D bracketBounds;
		converter.Transform( bounds, axes, org, bracketBounds );

		idVec4 color = colorWhite;
		color.w = idMath::ClampFloat( 0.0f, 1.0f, idMath::Fabs( bracketFadeTime - gameLocal.time ) / ( float )sdWayPoint::ACTIVATE_TIME );
		color.w = 1.0f - color.w;
		DrawBrackets( bracketBounds, color );
	}

	return entity;
	*/
}
