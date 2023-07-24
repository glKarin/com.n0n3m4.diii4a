// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_STRUCTURES_COMMANDMAPPOST_H__
#define __GAME_STRUCTURES_COMMANDMAPPOST_H__

#include "../Entity.h"

class sdCommandMapPost {
public:
	static void					WorldToMapTransform( const idVec2& inverseSize, idVec2& coords, const idVec2& cameraPosition, const idVec2& screenExtents );
	static void					ClampMapCamera( const idVec2& org, const idVec2& size, idVec2& cameraPosition, const sdBounds2D& bounds );
	static void					DrawLocalPlayerCommandMapInfo( sdUserInterfaceLocal* ui, float x, float y, float w, float h );	
	static bool					HandleLocalPlayerCommandMapInput( sdUIWindow* window, const sdSysEvent* event );
	static void					DrawLocalPlayerCommandMapInfo( sdUserInterfaceLocal* ui, float x, float y, float w, float h, const idVec2& offset );
	static void					DrawLocalPlayerCommandMapInfo_Icons( sdUserInterfaceLocal* ui, float x, float y, float w, float h );

private:
	static const sdPlayZone*	GetPlayZone( sdUserInterfaceLocal* ui, const idVec3& worldPos );
};

#endif // __GAME_STRUCTURES_COMMANDMAPPOST_H__
