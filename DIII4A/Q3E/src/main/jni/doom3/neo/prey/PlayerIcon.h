// Copyright (C) 2004 Id Software, Inc.
//

#ifndef	__PLAYERICON_H__
#define	__PLAYERICON_H__

typedef enum {
	ICON_LAG,
	ICON_CHAT,
	ICON_SAMETEAM, //HUMANHEAD rww
	ICON_NONE
} playerIconType_t;

class idPlayerIcon {
public:
	
public:
	idPlayerIcon();
	~idPlayerIcon();

	virtual void	Draw( idActor *player, jointHandle_t joint ); //HUMANHEAD rww - made virtual, general actor support
	virtual void	Draw( idActor *player, const idVec3 &origin ); //HUMANHEAD rww - made virtual, general actor support

public:
	playerIconType_t	iconType;
	renderEntity_t		renderEnt;
	qhandle_t			iconHandle;

public:
	void	FreeIcon( void );
	bool	CreateIcon( idActor* player, playerIconType_t type, const char *mtr, const idVec3 &origin, const idMat3 &axis ); //HUMANHEAD rww - general actor support
	bool	CreateIcon( idActor* player, playerIconType_t type, const idVec3 &origin, const idMat3 &axis ); //HUMANHEAD rww - general actor support
	void	UpdateIcon( idActor* player, const idVec3 &origin, const idMat3 &axis ); //HUMANHEAD rww - general actor support

};

//HUMANHEAD rww
class hhPlayerTeamIcon : public idPlayerIcon {
public:
	virtual void	Draw( idActor *player, jointHandle_t joint );
	virtual void	Draw( idActor *player, const idVec3 &origin );
};
//HUMANHEAD END

#endif	/* !_PLAYERICON_H_ */

