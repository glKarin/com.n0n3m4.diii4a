
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
							~idCursor3D( void );

	void					Spawn( void );
	void					Present( void );
	void					Think( void );

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
// RAVEN BEGIN
// bdube: added
	idEntity*				FindTraceEntity( idVec3 start, idVec3 end, const idEntity *skip );
// RAVEN END
private:
	int						nextSelectTime;
	idList<selectedTypeInfo_t> selectableEntityClasses;
	idList<idEntity *>		selectedEntities;
};

#endif /* !__GAME_EDIT_H__ */
