#ifndef __LEAF_H__
#define __LEAF_H__

class idEntity_Leaf : idMoveable {
public:
	CLASS_PROTOTYPE( idEntity_Leaf );
	void	Spawn();
	void	Think();

private:
	float liveTime;
	float moveSpeed;
	float spread;
	float windPower;
	idVec3 windDir;
	idVec3 gravity;
	idVec3 origin;
	idVec3 curDir;
	idVec3 angles;
	idVec3 curVel;
	idAngles ang;
	float spreadX;
	float spreadY;
	float dir;
	float dieTime;
	float nextAngles;
};

#endif // __LEAF_H__
