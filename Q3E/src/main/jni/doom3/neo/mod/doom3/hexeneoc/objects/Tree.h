#ifndef __TREE_H__
#define __TREE_H__

const int OBJECT_TREE_PAUSE_TIME		= 0; // in ms, pause after each transition // update: this seems to make movement much less life-like, so I Set it to zero.
const int OBJECT_TREE_NUM_JOINTS		= 5;
const int OBJECT_TREE_MAX_SWAY_DIST		= 32; // in units, the maximum distance trees will sway when winds are strongest
const int OBJECT_TREE_MAX_WIND_SPEED	= 10;
const int OBJECT_TREE_JOINT_ANGLE_TRANSITIONS_PER_SEC = 12; // this is about all we're gunna get. the joints don't update quick enough (at least on my system, but I assume it's time-based so it shouldnt matter). regardless, each transition is another calculation.
const int OBJECT_TREE_MAX_RANDOM_JOINT_ANGLE = 2;

class idAnimated_Tree : idAnimated {
public:
	CLASS_PROTOTYPE( idAnimated_Tree );
	void		Spawn();
	void		Think();
	void		RandomizeJoints( void );

private:
	idAngles	normAngle;
	idAngles	curAngle;
	float		maxAngle;
	idAngles	swayAngle;
	float		swayDir;
	float		swayTime;
	float		randSwayRange;
	float		nextSway;
	float		transitions;
	float		windSpeed;
};

#endif // __TREE_H__
