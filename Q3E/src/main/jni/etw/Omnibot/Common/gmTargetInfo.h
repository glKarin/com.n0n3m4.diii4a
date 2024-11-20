#ifndef _GM_TARGETINFO_H_
#define _GM_TARGETINFO_H_

#include "gmBind.h"
#include "TargetInfo.h"

class gmTargetInfo : public gmBind<TargetInfo, gmTargetInfo>
{
public:
	GMBIND_DECLARE_FUNCTIONS( );
	GMBIND_DECLARE_PROPERTIES( );

	static int gmIsA(gmThread *a_thread);

	// Property Accessors
	static bool getDistanceTo( TargetInfo *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool getPosition( TargetInfo *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool getFacing( TargetInfo *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool getVelocity( TargetInfo *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool getClass( TargetInfo *a_native, gmThread *a_thread, gmVariable *a_operands );

	static TargetInfo *Constructor(gmThread *a_thread);
	static void Destructor(TargetInfo *_native);
};

#endif

