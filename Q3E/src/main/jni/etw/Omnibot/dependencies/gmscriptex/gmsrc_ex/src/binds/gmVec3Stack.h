#ifndef __VEC3_STACK_H__
#define __VEC3_STACK_H__

#if(GM_USE_VECTOR3_STACK)

#include "mathlib/vector.h"

struct gmVec3Data
{
	float x, y, z;

	bool operator==(const gmVec3Data &a) const
	{
		return a.x == x && a.y == y && a.z == z;
	};
	bool operator!=(const gmVec3Data &a) const
	{
		return a.x != x || a.y != y || a.z != z;
	};
};

inline gmVec3Data ConvertVec3(const Vec3 &v)
{
	gmVec3Data vr = {v.x,v.y,v.z};
	return vr;
}

inline Vec3 ConvertVec3(const gmVec3Data &v)
{
	return Vec3(v.x,v.y,v.z);
}
extern gmVec3Data ZERO_VEC3;

class gmMachine;
class gmThread;
struct gmVariable;
void BindVector3Stack(gmMachine *a_machine);

#endif

#endif
