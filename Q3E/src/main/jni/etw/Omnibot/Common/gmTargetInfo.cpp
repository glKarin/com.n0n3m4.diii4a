#include "PrecompCommon.h"
#include "gmTargetInfo.h"

// script: TargetInfo
//		Script bindings for <TargetInfo>

GMBIND_INIT_TYPE( gmTargetInfo, "TargetInfo" );

GMBIND_FUNCTION_MAP_BEGIN( gmTargetInfo )
	GMBIND_FUNCTION( "IsA", gmIsA )
GMBIND_FUNCTION_MAP_END()

// property: Distance
//		The distance this target is from the bot.
// property: Position
//		The <Vector3> position of the target.
// property: Velocity
//		The <Vector3> velocity of the target.

GMBIND_PROPERTY_MAP_BEGIN( gmTargetInfo )
	GMBIND_PROPERTY( "Distance", getDistanceTo, NULL )
	GMBIND_PROPERTY( "Position", getPosition, NULL )
	GMBIND_PROPERTY( "Facing", getFacing, NULL )
	GMBIND_PROPERTY( "Velocity", getVelocity, NULL )
	GMBIND_PROPERTY( "Class", getClass, NULL )
GMBIND_PROPERTY_MAP_END();

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

TargetInfo *gmTargetInfo::Constructor(gmThread *a_thread)
{
	return NULL;
}

void gmTargetInfo::Destructor(TargetInfo *_native)
{
}

//////////////////////////////////////////////////////////////////////////
// Functions

int gmTargetInfo::gmIsA(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(checkclass, 0);
	TargetInfo *pNative = gmTargetInfo::GetThisObject( a_thread );
	OBASSERT(pNative, "Invalid Object");
	if ( pNative == NULL ) {
		return GM_EXCEPTION;
	}
	a_thread->PushInt(pNative->IsA(checkclass) ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
// Property Accessors/Modifiers

bool gmTargetInfo::getDistanceTo( TargetInfo *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetFloat(a_native->m_DistanceTo);
	return true;
}

bool gmTargetInfo::getPosition( TargetInfo *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetVector(
		a_native->m_LastPosition.x, 
		a_native->m_LastPosition.y, 
		a_native->m_LastPosition.z);
	return true;
}

bool gmTargetInfo::getFacing( TargetInfo *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetVector(
		a_native->m_LastFacing.x, 
		a_native->m_LastFacing.y, 
		a_native->m_LastFacing.z);
	return true;
}

bool gmTargetInfo::getVelocity( TargetInfo *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetVector(
		a_native->m_LastVelocity.x, 
		a_native->m_LastVelocity.y, 
		a_native->m_LastVelocity.z);
	return true;
}

bool gmTargetInfo::getClass( TargetInfo *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetInt(a_native->m_EntityClass);
	return true;
}

//////////////////////////////////////////////////////////////////////////
// Operators
