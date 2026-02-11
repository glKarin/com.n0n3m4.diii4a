// Game_JumpZone.h
//

#ifndef __GAME_JUMPZONE_H__
#define __GAME_JUMPZONE_H__

class hhJumpZone : public hhTrigger {
public:
	CLASS_PROTOTYPE( hhJumpZone );

	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

protected:
	idVec3			CalculateJumpVelocity();
	void			Event_Touch( idEntity *other, trace_t *trace );
	void			Event_Enable( void );
	void			Event_Disable( void );
	void			Event_ResetSlopeCheck(int entNum);

public:
	idVec3			velocity;
	float			pitchDegrees;
};


#endif /* __GAME_JUMPZONE_H__ */
