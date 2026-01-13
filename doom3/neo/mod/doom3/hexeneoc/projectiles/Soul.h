#ifndef __AI_SOUL_H__
#define __AI_SOUL_H__

class idProj_Soul : public idProjectile {
public:
	CLASS_PROTOTYPE( idProj_Soul );
	void	init( void );
	void	Think( void );

private:
	idEntity	*effect;
	idVec3	dir;
	float	dieTime;
	idAngles	ang;
	float		rnd;
};

#endif // __AI_SOUL_H__
