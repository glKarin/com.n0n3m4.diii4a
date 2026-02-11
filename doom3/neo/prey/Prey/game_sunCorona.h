
#ifndef __GAME_SUNCORONA_H__
#define __GAME_SUNCORONA_H__

//-----------------------------------------------------------------------------

class hhSunCorona : public idEntity {
public:
	CLASS_PROTOTYPE(hhSunCorona);

				~hhSunCorona();	//fixme: does this need to be virtual ?
	void		Spawn(void);
	void		Save( idSaveGame *savefile ) const;
	void		Restore( idRestoreGame *savefile );
	void		Draw( hhPlayer *player );

protected:
	const idMaterial	*corona;
	float				scale;
	idVec3				sunVector;
	float				sunDistance;
};

#endif /* __GAME_SUNCORONA_H__ */
