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

#ifndef __GAME_EDIT_H__
#define __GAME_EDIT_H__


/*
===============================================================================

	Ingame cursor.

===============================================================================
*/

class idCursor3D : public idEntity {
public:
	CLASS_PROTOTYPE( idCursor3D );

							idCursor3D( void );
	virtual					~idCursor3D( void ) override;

	void					Spawn( void );
	virtual void			Present( void ) override;
	virtual void			Think( void ) override;

	idForce_Drag			drag;
	idVec3					draggedPosition;
};


/*
===============================================================================

	Allows entities to be dragged through the world with physics.

===============================================================================
*/

class idDragEntity {
public:
							idDragEntity( void );
							~idDragEntity( void );

	void					Clear();
	void					Update( idPlayer *player );
	void					SetSelected( idEntity *ent );
	idEntity *				GetSelected( void ) const { return selected.GetEntity(); }
	void					DeleteSelected( void );
	void					BindSelected( void );
	void					UnbindSelected( void );

private:
	idEntityPtr<idEntity>	dragEnt;			// entity being dragged
	jointHandle_t			joint;				// joint being dragged
	int						id;					// id of body being dragged
	idVec3					localEntityPoint;	// dragged point in entity space
	idVec3					localPlayerPoint;	// dragged point in player space
	idStr					bodyName;			// name of the body being dragged
	idCursor3D *			cursor;				// cursor entity
	idEntityPtr<idEntity>	selected;			// last dragged entity

	void					StopDrag( void );
};


/*
===============================================================================

	Handles ingame entity editing.

===============================================================================
*/
typedef struct selectedTypeInfo_s {
	idTypeInfo *typeInfo;
	idStr		textKey;
} selectedTypeInfo_t;

class idEditEntities {
public:
							idEditEntities( void );
	bool					SelectEntity( const idVec3 &origin, const idVec3 &dir, const idEntity *skip );
	void					AddSelectedEntity( idEntity *ent );
	void					RemoveSelectedEntity( idEntity *ent );
	void					ClearSelectedEntities( void );
	void					DisplayEntities( void );
	bool					EntityIsSelectable( idEntity *ent, idVec4 *color = NULL, idStr *text = NULL );
private:
	int						nextSelectTime;
	idList<selectedTypeInfo_t> selectableEntityClasses;
	idList<idEntity *>		selectedEntities;
};

#endif /* !__GAME_EDIT_H__ */
