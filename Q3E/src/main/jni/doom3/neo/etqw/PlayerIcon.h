// Copyright (C) 2007 Id Software, Inc.
//

#ifndef	__PLAYERICON_H__
#define	__PLAYERICON_H__

#include "misc/Door.h"

class idPlayer;
class sdWorldToScreenConverter;

enum playerIconType_t {
	ICON_REVIVE,
	ICON_GOD,
	ICON_NONE
};

struct playerIconData_t {
	const idMaterial*		material;
	int						priority;
	int						timeout;
};

struct iconInfo_t {
	iconInfo_t() : size( 0.f, 0.f ), material( NULL ), color( 0.f, 0.f, 0.f, 0.f ) {
	}

	idVec2 size;
	const idMaterial* material;
	idVec4 color;
};

class sdPlayerDisplayIcon {
public:
	typedef sdPlayerDisplayIcon* sdPlayerDisplayIconPtr;
	static int SortByDistance( const sdPlayerDisplayIconPtr* a, const sdPlayerDisplayIconPtr* b ) {
		return ( ( *b )->distance - ( *a )->distance ) > 0.0f ? 1 : -1;
	}

	iconInfo_t			icon;
	iconInfo_t			arrowIcon;
	iconInfo_t			offScreenIcon;

	idVec2				origin;
	float				distance;

	idPlayer*			player;

	const static int	MAX_PLAYER_ICONS = 64;
};

class sdPlayerDisplayIconList : public idStaticList< sdPlayerDisplayIcon, sdPlayerDisplayIcon::MAX_PLAYER_ICONS > {
public:
};

class sdPlayerDisplayIconPtrList : public idStaticList< sdPlayerDisplayIcon*, sdPlayerDisplayIcon::MAX_PLAYER_ICONS > {
public:
};

class idPlayerIcon {
public:
								idPlayerIcon( void );
								~idPlayerIcon( void );

	void						Init( const idDict& dict );

	bool						Draw( sdPlayerDisplayIconList& list, idPlayer *player, jointHandle_t joint, float offset, const sdWorldToScreenConverter& converter, float distance, bool visible );
	static void					DrawTeamIdentifier( idPlayer* player, const sdDeclPlayerClass* cls, jointHandle_t joint, float offset, const sdWorldToScreenConverter& converter, float distance, bool visible );
	static void					GetPosition( idPlayer *player, jointHandle_t joint, float offset, idVec3& origin );

	const idMaterial*			GetActiveIcon( void );

	void						FreeIcon( qhandle_t handle );
	qhandle_t					CreateIcon( const idMaterial* material, int priority, int timeout );

private:
	void						FindActiveIcon( void );
	void						UpdateIcons( void );

private:
	idList< playerIconData_t >	icons;
	qhandle_t					activeIcon;
};

#endif	/* !_PLAYERICON_H_ */
