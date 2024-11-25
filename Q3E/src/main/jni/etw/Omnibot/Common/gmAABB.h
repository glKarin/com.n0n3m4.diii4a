#ifndef __GM_AABB_H__
#define __GM_AABB_H__

#include "gmBind.h"

class gmAABB : public gmBind<AABB, gmAABB>
{
public:
	GMBIND_DECLARE_FUNCTIONS( );
	GMBIND_DECLARE_PROPERTIES( );

	// Functions
	static int gmfCenterPoint(gmThread *a_thread);
	static int gmfExpand(gmThread *a_thread);
	static int gmfScale(gmThread *a_thread);
	static int gmfIsZero(gmThread *a_thread);
	static int gmfSet(gmThread *a_thread);
	static int gmfSetCenter(gmThread *a_thread);
	static int gmfIntersects(gmThread *a_thread);
	static int gmfContains(gmThread *a_thread);
	static int gmfFindIntersection(gmThread *a_thread);
	static int gmfGetAxisLength(gmThread *a_thread);
	static int gmfRenderAABB(gmThread *a_thread);

	// Property Accessors
	static bool getMins( AABB *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool setMins( AABB *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool getMaxs( AABB *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool setMaxs( AABB *a_native, gmThread *a_thread, gmVariable *a_operands );

	static AABB *Constructor(gmThread *a_thread);
	static void Destructor(AABB *_native);

	static void AsString(gmUserObject *a_object, char *a_buffer, int a_bufferLen);
};


void BindAABB(gmMachine *a_machine);

#endif
