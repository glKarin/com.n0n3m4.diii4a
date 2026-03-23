
#ifndef __GAME_IDROTDOOR_H__
#define __GAME_IDROTDOOR_H__

class idRotDoor : public idMover
{
public:
	CLASS_PROTOTYPE( idRotDoor );

	idRotDoor() {m_OpenPos.Zero();}
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );
	bool				IsOpen( void );
	void				Event_Open( void );
	void				Event_Close( void );
	void				Event_isOpen( void );

	bool				shouldOpen;

private:
	void				Event_Activate( idEntity *activator );
	

	idAngles					m_Rotate;
	idVec3						m_ClosedPos;
	idVec3						m_OpenPos;
	idVec3						m_OpenDir;

	idAngles					m_ClosedAngles;
	idAngles					m_OpenAngles;
};

#endif