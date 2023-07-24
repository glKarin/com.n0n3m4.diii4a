// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACE_CROSSHAIRINFO_H__
#define __GAME_GUIS_USERINTERFACE_CROSSHAIRINFO_H__

#include "UserInterfaceTypes.h"
#include "../PlayerIcon.h"

extern const char sdUITemplateFunctionInstance_IdentifierCrosshairInfo[];

class sdWayPoint;

class sdDockedScreenIcon {
public:
	sdDockedScreenIcon( void ) : startTime( 0 ), startOrigin( vec2_zero ), endOrigin( vec2_zero ), tag( NULL ), active( false ), flipped( false ), highlightedTime( 0 ) { }
	static const int	DOCK_ANIMATION_MS = 500;

public:
	bool				active;
	int					startTime;
	int					lastUpdateTime;
	idVec2				endOrigin;
	idVec2				startOrigin;
	void*				tag;
	bool				flipped;
	iconInfo_t			icon;
	iconInfo_t			arrowIcon;
	int					highlightedTime;
};

/*
============
sdUICrosshairInfo
============
*/
class sdUICrosshairInfo :
	public sdUIWindow {
public:
	typedef sdUITemplateFunction< sdUICrosshairInfo > CrosshairInfoTemplateFunction;

public:
											sdUICrosshairInfo( void );
	virtual									~sdUICrosshairInfo( void );

	virtual const char*						GetScopeClassName() const { return "sdUICrosshairInfo"; }
	virtual const CrosshairInfoTemplateFunction*	FindFunction( const char* name );
	
	virtual sdUIFunctionInstance*			GetFunction( const char* name );
	static void								InitFunctions();
	static void								ShutdownFunctions( void ) { crosshairInfoFunctions.DeleteContents(); }

	virtual bool							RunNamedMethod( const char* name, sdUIFunctionStack& stack );
	virtual void							EndLevelLoad();

	bool									TestCrosshairInfoFlag( const int flag ) const { return ( idMath::Ftoi( crosshairInfoFlags ) & flag ) != 0; }

protected:
	virtual void							DrawLocal();
	virtual void							ApplyLayout();

	void									DrawEntityIcons( void );
	void									DrawPlayerIcons2D( void );
	void									DrawWayPoints( idEntity* exclude );
	void									DrawDockedIcons( void );
	idEntity*								DrawContextEntity( idPlayer* player );

	sdDockedScreenIcon*						FindDockedIcon( void* tag );
	sdDockedScreenIcon*						FindFreeDockedIcon( void );
	void									MakeDockIcon( void* tag, bool flipped, const idVec2& startOrg, const idVec2& endOrg, const iconInfo_t& arrowInfo, const iconInfo_t& iconInfo );
	void									ClearInactiveDockIcons( void );

	void									DrawBrackets( const sdBounds2D& bounds, const idVec4& color );
	void									DrawCrosshairInfo( idPlayer* player );
	void									DrawDamageIndicators( idPlayer* player );

	const sdCrosshairInfo&					GetRepeaterCrosshairInfo( void );

private:
	void									OnBarMaterialChanged( const idStr& oldValue, const idStr& newValue );
	void									OnBarFillMaterialChanged( const idStr& oldValue, const idStr& newValue );
	void									OnBarLineMaterialChanged( const idStr& oldValue, const idStr& newValue );
	void									OnDamageMaterialChanged( const idStr& oldValue, const idStr& newValue );
	void									OnLeftBracketMaterialChanged( const idStr& oldValue, const idStr& newValue );
	void									OnRightBracketMaterialChanged( const idStr& oldValue, const idStr& newValue );


protected:
	enum crosshairInfo_e {
		CF_CROSSHAIR				= BITT< 0 >::VALUE,
		CF_PLAYER_ICONS				= BITT< 1 >::VALUE,
		CF_TASKS					= BITT< 2 >::VALUE,
		CF_OBJ_MIS					= BITT< 3 >::VALUE
	};

	sdStringProperty						barMaterial;
	sdStringProperty						barFillMaterial;
	sdStringProperty						barLineMaterial;
	sdStringProperty						damageMaterial;
	sdStringProperty						bracketLeftMaterial;
	sdStringProperty						bracketRightMaterial;
	sdVec4Property							barLineColor;
	sdVec4Property							bracketBaseColor;
	sdFloatProperty							crosshairInfoFlags;


private:
	enum eCrosshairPart {	CP_BAR_LEFT, CP_BAR_CENTER, CP_BAR_RIGHT, 
							CP_BAR_FILL_LEFT, CP_BAR_FILL_CENTER, CP_BAR_FILL_RIGHT,
							CP_BAR_LINE_LEFT, CP_BAR_LINE_CENTER, CP_BAR_LINE_RIGHT,
							CP_SMALL_DAMAGE, CP_MEDIUM_DAMAGE, CP_LARGE_DAMAGE,
							CP_L_BRACKET_TOP, CP_L_BRACKET_CENTER, CP_L_BRACKET_BOTTOM,
							CP_R_BRACKET_TOP, CP_R_BRACKET_CENTER, CP_R_BRACKET_BOTTOM,
							CP_MAX
	};

	static const int						lineTextScale;
	int										lineTextHeight;
	int										waypointTextFadeTime;
	int										bracketFadeTime;
	idEntityPtr< idEntity >					lastBracketEntity;

	idStaticList< uiDrawPart_t, CP_MAX > crosshairParts;	
	static idHashMap< CrosshairInfoTemplateFunction* >		crosshairInfoFunctions;
	idStaticList< sdDockedScreenIcon, 6 >					dockedIcons;
};

#endif // __GAME_GUIS_USERINTERFACE_CROSSHAIRINFO_H__
