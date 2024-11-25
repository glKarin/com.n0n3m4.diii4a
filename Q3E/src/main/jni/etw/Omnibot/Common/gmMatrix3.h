#ifndef __GMMATRIX3_H__
#define __GMMATRIX3_H__

#include "gmBind.h"

// Handy typedef if we want to easily change which Wm3Vector template we want to use.

typedef Matrix3f gmMat3Type;

class gmMatrix3 : public gmBind< gmMat3Type, gmMatrix3 >
{
public:
	GMBIND_DECLARE_FUNCTIONS( );
	GMBIND_DECLARE_PROPERTIES( );
	GMBIND_DECLARE_OPERATORS( );

	// Functions
	static int gmfTransformVector(gmThread *a_thread);
	static int gmfInverseTransformVector(gmThread *a_thread);
	static int gmfInverse(gmThread *a_thread);

	// Operators
	static bool opAdd( gmThread *a_thread, gmVariable *a_operands );
	static bool opSub( gmThread *a_thread, gmVariable *a_operands );
	static bool opMul( gmThread *a_thread, gmVariable *a_operands );
	static bool opDiv( gmThread *a_thread, gmVariable *a_operands );
	static bool opIsEqual( gmThread *a_thread, gmVariable *a_operands );
	static bool opIsNotEqual( gmThread *a_thread, gmVariable *a_operands );

	// Properties
	static bool gmfGetZero( gmMat3Type *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool gmfGetIdentity( gmMat3Type *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool gmfGetX( gmMat3Type *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool gmfGetY( gmMat3Type *a_native, gmThread *a_thread, gmVariable *a_operands );
	static bool gmfGetZ( gmMat3Type *a_native, gmThread *a_thread, gmVariable *a_operands );

	// Utility
	static gmMat3Type *Constructor(gmThread *a_thread);
	static void Destructor(gmMat3Type *_native);
	static void AsString(gmUserObject *a_object, char *a_buffer, int a_bufferLen);
	static void DebugInfo(gmUserObject *a_object, gmMachine *a_machine, gmChildInfoCallback a_infoCallback);
};

#endif