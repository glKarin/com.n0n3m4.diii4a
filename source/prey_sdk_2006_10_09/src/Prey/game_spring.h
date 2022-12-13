
#ifndef __GAME_SPRING_H__
#define __GAME_SPRING_H__

/***********************************************************************

hhSpring

***********************************************************************/
extern const idEventDef EV_HHLinkSpring;
extern const idEventDef EV_HHUnlinkSpring;

class hhSpring : public idEntity {
	CLASS_PROTOTYPE( hhSpring );

public:

	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	virtual void	Think( void );

	// HUMANHEAD pdm
	void			LinkSpring(idEntity *ent1, idEntity *ent2 );
	void			LinkSpringIDs(idEntity *ent1, int bodyID1, idEntity *ent2, int bodyID2);
	void			LinkSpringAll(idEntity *ent1, int bodyID1, idVec3 &offset1, idEntity *ent2, int bodyID2, idVec3 &offset2);
	void			UnLinkSpring();
	void			SpringSettings(float kStretch, float kCompress, float damping, float restLength);
	// HUMANHEAD END

protected:
	void			Event_LinkSpring( void );
	void			Event_UnlinkSpring( void );

protected:
	idStr			name1, name2;
	idEntityPtr<idEntity> physics1, physics2;
	int				id1, id2;
	idVec3			p1, p2;
	idForce_Spring	spring;
};


#endif /* !__GAME_SPRING_H__ */
