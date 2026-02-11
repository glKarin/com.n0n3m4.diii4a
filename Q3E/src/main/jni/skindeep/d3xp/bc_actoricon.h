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




typedef enum {
	ACTORICON_SEARCHING,
	ACTORICON_ALERTED,
	ACTORICON_STUNNED,
	ACTORICON_NONE //Make sure this is last!
} actorIconType_t;

//Make sure this is the SAME ORDER as the enum above.
//This refers to the KEY name in the monster's def file. This enum is NOT the material name.
static const char * iconKeys[ACTORICON_NONE] = {
	"mtr_icon_searching",
	"mtr_icon_alerted",
	"mtr_icon_stunned"
};


class idActorIcon {
public:

public:
	idActorIcon();
	~idActorIcon();

	void	Draw( idActor *actor, jointHandle_t joint, int _aiState);
	void	Draw( idActor *actor, const idVec3 &origin, actorIconType_t _icontype);

public:
	actorIconType_t		iconType;
	renderEntity_t		renderEnt;
	qhandle_t			iconHandle;

	int					timer;

	enum				{ ICONSTATE_NONE= 0, ICONSTATE_SPAWNING, ICONSTATE_IDLE, ICONSTATE_DESPAWNING, ICONSTATE_DONE };
	int					iconState;

public:
	void	FreeIcon( void );
	
	bool	CreateIcon(idActor* actor, actorIconType_t type, const idVec3 &origin, const idMat3 &axis );
	void	UpdateIcon(idActor* actor, const idVec3 &origin, const idMat3 &axis );

private:
	bool	CreateIcon(idActor* actor, actorIconType_t type, const char *mtr, const idVec3 &origin, const idMat3 &axis);

};

