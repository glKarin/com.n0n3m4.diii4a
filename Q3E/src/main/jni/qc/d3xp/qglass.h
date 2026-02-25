
#ifndef __GAME_QGLASS_H__
#define __GAME_QGLASS_H__

class idQGlass : public idEntity
{
public:
							CLASS_PROTOTYPE( idQGlass );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	void					Spawn( void );
	void					Event_glassreset( void );


private:

	enum					{ IDLE, DESTROYED };
	int						state;
	int						count;

	idStr					fxFracture;

	virtual void			AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force );
	
	
};


#endif