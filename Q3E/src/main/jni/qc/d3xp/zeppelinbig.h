
#ifndef __GAME_ZEPPELINBIG_H__
#define __GAME_ZEPPELINBIG_H__

class idZeppelinBig : public idAnimatedEntity
{
public:
							CLASS_PROTOTYPE( idZeppelinBig );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Think( void );
	void					Spawn( void );

private:

	enum					{ OFF, ON };
	int						state;

	int						nextAttackTime;
	int						nextVolleyTime;
	int						volleyCount;
	int						volleyDelay;
	int						maxVolley;
	int						attackDelay;
	float					initialDelay;

	void					SpawnBeam(idBeam *beamStart, idBeam *beamEnd, const char *attachJoint);
	void					AimBeam(const char *jointName, idBeam *beamEnd, idVec3 aimPos);
	void					Event_zeppelinactivate(int value);

	idBeam*					beam1Start;
	idBeam*					beam1End;

	idBeam*					beam2Start;	
	idBeam*					beam2End;

	idLight *				light;
};


#endif