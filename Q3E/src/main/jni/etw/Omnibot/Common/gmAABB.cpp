#include "PrecompCommon.h"
#include "gmAABB.h"

#define CHECK_THIS_AABB() \
	AABB *pNative = gmAABB::GetThisObject( a_thread ); \
	if(!pNative) { return GM_EXCEPTION; }

// script: AABB
//		Script Bindings for <AABB>

GMBIND_INIT_TYPE( gmAABB, "AABB" );

GMBIND_FUNCTION_MAP_BEGIN( gmAABB )
	GMBIND_FUNCTION( "CenterPoint", gmfCenterPoint )
	GMBIND_FUNCTION( "Expand", gmfExpand )
	GMBIND_FUNCTION( "Scale", gmfScale )
	GMBIND_FUNCTION( "IsZero", gmfIsZero )
	GMBIND_FUNCTION( "Set", gmfSet )
	GMBIND_FUNCTION( "SetCenter", gmfSetCenter )
	GMBIND_FUNCTION( "Intersects", gmfIntersects )
	GMBIND_FUNCTION( "Contains", gmfContains )
	GMBIND_FUNCTION( "FindIntersection", gmfFindIntersection )
	GMBIND_FUNCTION( "GetAxisLength", gmfGetAxisLength )
	GMBIND_FUNCTION( "Render", gmfRenderAABB )
GMBIND_FUNCTION_MAP_END()

GMBIND_PROPERTY_MAP_BEGIN( gmAABB )
	// var: Mins
	//		<Vector3> - The Mins of this AABB
	GMBIND_PROPERTY( "Mins", getMins, NULL/*setMins*/ )
	// var: Maxs
	//		<Vector3> - The Maxs of this AABB
	GMBIND_PROPERTY( "Maxs", getMaxs, NULL/*setMaxs*/ )
GMBIND_PROPERTY_MAP_END();

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

AABB *gmAABB::Constructor(gmThread *a_thread)
{
	AABB *pNewAABB = new AABB;
	memset(pNewAABB, 0, sizeof(AABB));

	if(a_thread)
	{
		if(a_thread->GetNumParams() == 2)
		{
			if(a_thread->ParamType(0) == GM_VEC3 && 
				a_thread->ParamType(1) == GM_VEC3)
			{
				a_thread->Param(0).GetVector(pNewAABB->m_Mins[0], pNewAABB->m_Mins[1], pNewAABB->m_Mins[2]);
				a_thread->Param(1).GetVector(pNewAABB->m_Maxs[0], pNewAABB->m_Maxs[1], pNewAABB->m_Maxs[2]);
			}
		}
		else if(a_thread->GetNumParams() == 6)
		{
			bool bGood = true;
			float flts[6] = {};
			for(int i = 0; i < 6; ++i)
			{
				if(!a_thread->Param(i).GetFloatSafe(flts[i]))
				{
					bGood = false;
					break;
				}
			}

			if(bGood)
			{
				pNewAABB->m_Mins[0] = flts[0];
				pNewAABB->m_Mins[1] = flts[1];
				pNewAABB->m_Mins[2] = flts[2];
				pNewAABB->m_Maxs[0] = flts[3];
				pNewAABB->m_Maxs[1] = flts[4];
				pNewAABB->m_Maxs[2] = flts[5];
			}
		}
	}
	return pNewAABB;
}

void gmAABB::Destructor(AABB *_native)
{
	delete _native;
}

void gmAABB::AsString(gmUserObject *a_object, char *a_buffer, int a_bufferLen)
{
	AABB *aabb = gmAABB::GetNative(a_object);
	_gmsnprintf(a_buffer, a_bufferLen, "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f", 
		aabb->m_Mins[0],aabb->m_Mins[1],aabb->m_Mins[2],
		aabb->m_Maxs[0],aabb->m_Maxs[1],aabb->m_Maxs[2]);
}

//////////////////////////////////////////////////////////////////////////

// function: CenterPoint
//		This function gets the centerpoint of this AABB
//
// Parameters:
//
//		None
//
// Returns:
//		<Vector3> - The center point for this AABB
int gmAABB::gmfCenterPoint(gmThread *a_thread)
{
	CHECK_THIS_AABB();
	Vector3f vCenter(Vector3f::ZERO);
	pNative->CenterPoint(vCenter);
	a_thread->PushVector(vCenter.x, vCenter.y, vCenter.z);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Expand
//		Expands the box by some value.
//
// Parameters:
//
//		Vector3 - Point to expand box to contain.
//
// Returns:
//		none
int gmAABB::gmfExpand(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	CHECK_THIS_AABB();

	if(a_thread->ParamType(0) == GM_VEC3)
	{
		GM_CHECK_VECTOR_PARAM(v,0);
		float vf[3] = { v.x,v.y,v.z };
		pNative->Expand(vf);
	}
	else if(a_thread->ParamType(0) == gmAABB::GetType())
	{
		GM_CHECK_GMBIND_PARAM(AABB*, gmAABB, aabb, 0);
		pNative->Expand(*aabb);
	}
	else
	{
		GM_EXCEPTION_MSG("expecting param 0 as vector 3 or user type %s", gmAABB::GetTypeName());
		return GM_EXCEPTION;
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Scale
//		Scales the box by some amount.
//
// Parameters:
//
//		float - Amount to scale by.
//
// Returns:
//		none
int gmAABB::gmfScale(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	CHECK_THIS_AABB();

	GM_CHECK_FLOAT_OR_INT_PARAM(sc, 0);
	pNative->Scale(sc);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: IsZero
//		Checks if the AABB is a zero box.
//
// Parameters:
//
//		none
//
// Returns:
//		int - true if zero, false if not
int gmAABB::gmfIsZero(gmThread *a_thread)
{
	CHECK_THIS_AABB();
	a_thread->PushInt(pNative->IsZero() ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Set
//		Sets the parameters for the box.
//
// Parameters:
//
//		<Vector3> - Mins
//		<Vector3> - Maxs
//
// Returns:
//		none
int gmAABB::gmfSet(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	CHECK_THIS_AABB();
	GM_CHECK_VECTOR_PARAM(v1, 0);
	GM_CHECK_VECTOR_PARAM(v2, 1);
	pNative->Set(Vector3f(v1.x,v1.y,v1.z), Vector3f(v2.x,v2.y,v2.z));
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: SetCenter
//		Sets the centerpoint for the box.
//
// Parameters:
//
//		<Vector3> - New centerpoint
//
// Returns:
//		none
int gmAABB::gmfSetCenter(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_VECTOR_PARAM(v, 0);
	CHECK_THIS_AABB();
	pNative->SetCenter(Vector3f(v.x,v.y,v.z));
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Intersects
//		Check is a box intersects another box.
//
// Parameters:
//
//		<AABB> - Box to check intersection with
//
// Returns:
//		int - true if intersection, false if not
int gmAABB::gmfIntersects(gmThread *a_thread)
{
	CHECK_THIS_AABB();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_GMBIND_PARAM(AABB*, gmAABB, aabb, 0);	
	a_thread->PushInt(pNative->Intersects(*aabb) ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Contains
//		Check is a point lies within the box.
//
// Parameters:
//
//		<Vector3> - Point to check with
//
// Returns:
//		int - true if contain, false if not
int gmAABB::gmfContains(gmThread *a_thread)
{
	CHECK_THIS_AABB();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_VECTOR_PARAM(v, 0);	
	a_thread->PushInt(pNative->Contains(Vector3f(v.x,v.y,v.z)) ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: FindIntersection
//		Find the intersection box of 2 boxes.
//
// Parameters:
//
//		<AABB> - Box to check intersection with
//
// Returns:
//		<AABB> - Box that represents overlapped region
//		- OR - 
//		null if the boxes don't intersect
int gmAABB::gmfFindIntersection(gmThread *a_thread)
{
	CHECK_THIS_AABB();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_GMBIND_PARAM(AABB*, gmAABB, aabb, 0);
	
	AABB ov;
	if(pNative->FindIntersection(*aabb, ov))
		gmAABB::PushObject(a_thread, ov);
	else
		a_thread->PushNull();	
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetAxisLength
//		Get the axis length of the box.
//
// Parameters:
//
//		string - "x","y", or "z"
//
// Returns:
//		float - Length of requested axis
int gmAABB::gmfGetAxisLength(gmThread *a_thread)
{
	CHECK_THIS_AABB();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(ax, 0);
	
	int iAxis = -1;

	if(!_gmstricmp(ax, "x"))
		iAxis = 0;
	else if(!_gmstricmp(ax, "y"))
		iAxis = 1;
	else if(!_gmstricmp(ax, "z"))
		iAxis = 2;
	else
	{
		GM_EXCEPTION_MSG("Expected \"x\",\"y\", or \"z\"");
		return GM_EXCEPTION;
	}

	a_thread->PushFloat(pNative->GetAxisLength(iAxis));
	return GM_OK;
}

int gmAABB::gmfRenderAABB(gmThread *a_thread)
{
	CHECK_THIS_AABB();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_FLOAT_OR_INT_PARAM(duration, 0);
	GM_INT_PARAM(col,1,COLOR::MAGENTA.rgba());
	
	Utils::OutlineAABB(*pNative, obColor(col), duration);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
// Property Accessors/Modifiers

bool gmAABB::getMins( AABB *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_thread->PushVector(a_native->m_Mins[0], a_native->m_Mins[1], a_native->m_Mins[2]);
	return true;
}

bool gmAABB::setMins( AABB *a_native, gmThread *a_thread, gmVariable *a_operands )
{	
	GM_ASSERT(a_operands[1].m_type == GM_VEC3);
	if(a_operands[1].m_type == GM_VEC3)
	{
		a_operands[1].GetVector(a_native->m_Mins);
		return true;
	}	
	return false;
}

bool gmAABB::getMaxs( AABB *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_thread->PushVector(a_native->m_Maxs[0], a_native->m_Maxs[1], a_native->m_Maxs[2]);
	return true;
}

bool gmAABB::setMaxs( AABB *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	GM_ASSERT(a_operands[1].m_type == GM_VEC3);
	if(a_operands[1].m_type == GM_VEC3)
	{
		a_operands[1].GetVector(a_native->m_Maxs);
		return true;
	}
	return false;	
}

//////////////////////////////////////////////////////////////////////////

int Bounds_Constructor(gmThread *a_thread)
{
	BoundingBox *pNewAABB = new BoundingBox;
	memset(pNewAABB, 0, sizeof(BoundingBox));

	if(a_thread)
	{
		if(a_thread->GetNumParams() == 2)
		{
			if(a_thread->ParamType(0) == GM_VEC3 && 
				a_thread->ParamType(1) == GM_VEC3)
			{
				a_thread->Param(0).GetVector(pNewAABB->mMins[0], pNewAABB->mMins[1], pNewAABB->mMins[2]);
				a_thread->Param(1).GetVector(pNewAABB->mMaxs[0], pNewAABB->mMaxs[1], pNewAABB->mMaxs[2]);
			}
		}
		else if(a_thread->GetNumParams() == 6)
		{
			bool bGood = true;
			float flts[6] = {};
			for(int i = 0; i < 6; ++i)
			{
				if(!a_thread->Param(i).GetFloatSafe(flts[i]))
				{
					bGood = false;
					break;
				}
			}

			if(bGood)
			{
				pNewAABB->mMins[0] = flts[0];
				pNewAABB->mMins[1] = flts[1];
				pNewAABB->mMins[2] = flts[2];
				pNewAABB->mMaxs[0] = flts[3];
				pNewAABB->mMaxs[1] = flts[4];
				pNewAABB->mMaxs[2] = flts[5];
			}
		}
		else
		{
			pNewAABB->Set(Vec3(0,0,0));
		}
	}
	gmBind2::Class<BoundingBox>::PushObject(a_thread, pNewAABB);
	return GM_OK;
}

void Bounds_AsString(BoundingBox *a_var, char * a_buffer, int a_bufferSize)
{
	_gmsnprintf(a_buffer, a_bufferSize, 
		"Bounds(%.3f,%.3f,%.3f,%.3f,%.3f,%.3f)",
		a_var->mMins.x,a_var->mMins.y,a_var->mMins.z,
		a_var->mMaxs.x,a_var->mMaxs.y,a_var->mMaxs.z);
}

void BindAABB(gmMachine *a_machine)
{
	gmBind2::Class<BoundingBox>("Bounds",a_machine,false)
		.constructor(Bounds_Constructor,"AABB")
		.asString(Bounds_AsString)
		.func(&BoundingBox::Set,"Set")
		.func(&BoundingBox::SetMinMax,"SetMinMax")
		.func(&BoundingBox::IsZero,"IsZero")
		.func(&BoundingBox::CenterPoint,"CenterPoint")
		.func(&BoundingBox::CenterTop,"CenterTop")
		.func(&BoundingBox::CenterBottom,"CenterBottom")
		.func(&BoundingBox::MoveCenter,"MoveCenter")
		.func(&BoundingBox::Expand,"Expand")
		.func(&BoundingBox::Intersects,"Intersects")
		.func(&BoundingBox::Contains,"Contains")
		.func(&BoundingBox::FindIntersection,"FindIntersection")
		.func(&BoundingBox::GetLengthX,"GetLengthX")
		.func(&BoundingBox::GetLengthY,"GetLengthY")
		.func(&BoundingBox::GetLengthZ,"GetLengthZ")
		.func(&BoundingBox::GetArea,"GetArea")
		.func(&BoundingBox::DistanceFromBottom,"DistanceFromBottom")
		.func(&BoundingBox::DistanceFromTop,"DistanceFromTop")
		.func(&BoundingBox::Scale,"Scale")
		.func(&BoundingBox::ScaleCopy,"ScaleCopy")
		.func(&BoundingBox::ExpandX,"ExpandX")
		.func(&BoundingBox::ExpandY,"ExpandY")
		.func(&BoundingBox::ExpandZ,"ExpandZ")
		.func(&BoundingBox::TranslateCopy,"TranslateCopy")
		.var(&BoundingBox::mMins,"Mins")
		.var(&BoundingBox::mMaxs,"Maxs")
		;
}
