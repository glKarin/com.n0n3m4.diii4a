#include "PrecompCommon.h"
#include "gmMatrix3.h"
#include "gmHelpers.h"

// script: Matrix3
//		Script Bindings for <Matrix3>

GMBIND_INIT_TYPE( gmMatrix3, "Matrix3" );

GMBIND_FUNCTION_MAP_BEGIN( gmMatrix3 )
	// function: TransformVector
	//		Transforms a passed in vector by this matrix.
	GMBIND_FUNCTION( "TransformVector", gmfTransformVector )
	// function: InverseTransformVector
	//		Transforms a passed in vector by the inverse of this matrix.
	GMBIND_FUNCTION( "InverseTransformVector", gmfInverseTransformVector )
	// function: Inverse
	//		Inverts this matrix.
	GMBIND_FUNCTION( "Inverse", gmfInverse )
GMBIND_FUNCTION_MAP_END()

GMBIND_PROPERTY_MAP_BEGIN( gmMatrix3 )
	GMBIND_PROPERTY("ZERO", gmfGetZero, NULL )
	GMBIND_PROPERTY("IDENTITY", gmfGetIdentity, NULL )
	GMBIND_PROPERTY("X", gmfGetX, NULL )
	GMBIND_PROPERTY("Y", gmfGetY, NULL )
	GMBIND_PROPERTY("Z", gmfGetZ, NULL )
GMBIND_PROPERTY_MAP_END();

GMBIND_OPERATOR_MAP_BEGIN( gmMatrix3 )	
	GMBIND_OPERATOR_MUL	( opMul )
	GMBIND_OPERATOR_EQ	( opIsEqual )
	GMBIND_OPERATOR_NEQ	( opIsNotEqual )
GMBIND_OPERATOR_MAP_END();

//////////////////////////////////////////////////////////////////////////
// Functions

// script: Script Types

// class: Matrix3

// function: Matrix3
//		Construct an uninitialized <Matrix3>

// function: Matrix3
//		Construct a <Matrix3> using x,y,z axis components.
//
// Parameters:
//
//		x - The x component of the <Vector3>
//		y - The y component of the <Vector3>
//		z - The z component of the <Vector3>
//
// Returns:
//		The newly constructed <Vector3>
gmMat3Type *gmMatrix3::Constructor(gmThread *a_thread) 
{
	gmMat3Type *pNewMat = NULL;
	
	if(a_thread)
	{
		if(a_thread->GetNumParams() == 3)
		{
			float fX, fY, fZ;
			Vector3f vAxis[3] = { Vector3f::ZERO, Vector3f::ZERO, Vector3f::ZERO };
			if(a_thread->ParamType(0) == GM_VEC3 &&
				a_thread->ParamType(1) == GM_VEC3 &&
				a_thread->ParamType(2) == GM_VEC3)
			{
				vAxis[0] = Vector3f(
					a_thread->Param(0).m_value.m_vec3.x,
					a_thread->Param(0).m_value.m_vec3.y,
					a_thread->Param(0).m_value.m_vec3.z);
				vAxis[1] = Vector3f(
					a_thread->Param(1).m_value.m_vec3.x,
					a_thread->Param(1).m_value.m_vec3.y,
					a_thread->Param(1).m_value.m_vec3.z);
				vAxis[2] = Vector3f(
					a_thread->Param(2).m_value.m_vec3.x,
					a_thread->Param(2).m_value.m_vec3.y,
					a_thread->Param(2).m_value.m_vec3.z);
				pNewMat = new gmMat3Type(gmMat3Type::IDENTITY);
				*pNewMat = gmMat3Type(vAxis[0], vAxis[1], vAxis[2], true);
			}
			else if(a_thread->Param(0).GetFloatSafe(fX) &&
				a_thread->Param(1).GetFloatSafe(fY) &&
				a_thread->Param(2).GetFloatSafe(fZ))
			{
				pNewMat = new gmMat3Type(gmMat3Type::IDENTITY);
				pNewMat->FromEulerAnglesXYZ(fX, fY, fZ);
			}
		}
		else if(a_thread->GetNumParams() == 2)
		{
			float fAngle;
			Vector3f vAxis = Vector3f::ZERO;
			if(a_thread->ParamType(0) == GM_VEC3 &&
				a_thread->Param(1).GetFloatSafe(fAngle))
			{
				vAxis = Vector3f(
					a_thread->Param(0).m_value.m_vec3.x,
					a_thread->Param(0).m_value.m_vec3.y,
					a_thread->Param(0).m_value.m_vec3.z);
				pNewMat = new gmMat3Type(gmMat3Type::IDENTITY);
				*pNewMat = gmMat3Type(vAxis, Mathf::DegToRad(fAngle));
			}
		}
		else if(a_thread->GetNumParams() == 1)
		{
			if(a_thread->ParamType(0) == gmMatrix3::GetType())
			{
				pNewMat = new gmMat3Type(gmMat3Type::IDENTITY);
				*pNewMat = *gmMatrix3::GetNative(
					reinterpret_cast<gmUserObject*>(a_thread->Param(0).m_value.m_ref));
			}
		}
		else if(a_thread->GetNumParams() == 0)
		{
			pNewMat = new gmMat3Type(gmMat3Type::IDENTITY);
		}
	}
	else
	{
		pNewMat = new gmMat3Type(gmMat3Type::IDENTITY);
	}
	return pNewMat; 
}

void gmMatrix3::Destructor(gmMat3Type *_native)
{
	delete _native;
}

void gmMatrix3::AsString(gmUserObject *a_object, char *a_buffer, int a_bufferLen)
{
	gmMat3Type *pNative = gmMatrix3::GetNative(a_object);
	OBASSERT(pNative, "Invalid Object");
	if ( pNative == NULL ) {
		_gmsnprintf( a_buffer, a_bufferLen, "not a matrix3" );
		return;
	}

	float fX = 0.f, fY = 0.f, fZ = 0.f;
	pNative->ToEulerAnglesXYZ(fX, fY, fZ);
	//Note '#' always display decimal place, 'g' display exponent if > precision or 4
	_gmsnprintf(a_buffer, a_bufferLen, "(%#.8g, %#.8g, %#.8g)", fX, fY, fZ);
}

void gmMatrix3::DebugInfo(gmUserObject *a_object, gmMachine *a_machine, gmChildInfoCallback a_infoCallback)
{	
	const int iBufferSize = 256;
	char buffer[iBufferSize];

	gmVariable var(a_object);
	a_infoCallback("Value", var.AsString(a_machine, buffer, iBufferSize), a_machine->GetTypeName(GM_STRING), 0);

	gmBind<Matrix3f, gmMatrix3>::DebugInfo(a_object, a_machine, a_infoCallback);
}

//////////////////////////////////////////////////////////////////////////

int gmMatrix3::gmfTransformVector(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	gmMat3Type* native = gmMatrix3::GetThisObject( a_thread );
	GM_CHECK_VECTOR_PARAM(v,0);

	Vector3f vt(v.x,v.y,v.z);
	vt = *native * vt;
	a_thread->PushVector(vt.x, vt.y, vt.z);
	return GM_OK; 
}

//////////////////////////////////////////////////////////////////////////

int gmMatrix3::gmfInverseTransformVector(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	gmMat3Type* native = gmMatrix3::GetThisObject( a_thread );
	GM_CHECK_VECTOR_PARAM(v,0);

	Matrix3f mInv = native->Inverse();
	
	Vector3f vt(v.x,v.y,v.z);
	vt = mInv * vt;
	a_thread->PushVector(vt.x, vt.y, vt.z);
	return GM_OK; 
}

//////////////////////////////////////////////////////////////////////////

int gmMatrix3::gmfInverse(gmThread *a_thread)
{
	gmMat3Type* native = gmMatrix3::GetThisObject( a_thread );
	Matrix3f mInv = native->Inverse();
	gmMatrix3::PushObject(a_thread, mInv);
	return GM_OK; 
}

//////////////////////////////////////////////////////////////////////////

bool gmMatrix3::opMul( gmThread *a_thread, gmVariable *a_operands )
{
	// Check types
	if(a_operands[0].m_type == gmMatrix3::GetType() && (a_operands[1].m_type != gmMatrix3::GetType()))
	{
		// Get operands
		gmMat3Type *vecObjA = gmMatrix3::GetNative(reinterpret_cast<gmUserObject*>(a_operands[0].m_value.m_ref));
		
		float fScalar = 0.0f;
		if(!gmGetFloatOrIntParamAsFloat(a_operands[1], fScalar))
		{
			return false;
		}

		gmMat3Type vVec = (*vecObjA) * fScalar;	
		gmMatrix3::SetObject(a_thread->GetMachine(), a_operands[0], vVec);
	}
	else if((a_operands[0].m_type != gmMatrix3::GetType()) && a_operands[1].m_type == gmMatrix3::GetType())
	{
		// Get operands
		gmMat3Type *vecObjB = gmMatrix3::GetNative(reinterpret_cast<gmUserObject*>(a_operands[1].m_value.m_ref));

		float fScalar = 0.0f;
		if(!gmGetFloatOrIntParamAsFloat(a_operands[0], fScalar))
		{
			return false;
		}
		
		gmMat3Type vVec = fScalar * (*vecObjB);	
		gmMatrix3::SetObject(a_thread->GetMachine(), a_operands[0], vVec);
	}
	else
	{
		a_operands[0].Nullify();
		return false;
	}
	return true;
}

bool gmMatrix3::opIsEqual( gmThread *a_thread, gmVariable *a_operands )
{
	// Check types
	if(a_operands[0].m_type == gmMatrix3::GetType() && a_operands[1].m_type == gmMatrix3::GetType())
	{
		// Get operands
		gmMat3Type *vecObjA = gmMatrix3::GetNative(reinterpret_cast<gmUserObject*>(a_operands[0].m_value.m_ref));
		gmMat3Type *vecObjB = gmMatrix3::GetNative(reinterpret_cast<gmUserObject*>(a_operands[1].m_value.m_ref));
		a_operands[0].SetInt((*vecObjA) == (*vecObjB) ? 1 : 0);
	}
	else
	{
		a_operands[0].Nullify();
		return false;
	}
	return true;
}

bool gmMatrix3::opIsNotEqual( gmThread *a_thread, gmVariable *a_operands )
{
	// Check types
	if(a_operands[0].m_type == gmMatrix3::GetType() && a_operands[1].m_type == gmMatrix3::GetType())
	{
		// Get operands
		gmMat3Type *vecObjA = gmMatrix3::GetNative(reinterpret_cast<gmUserObject*>(a_operands[0].m_value.m_ref));
		gmMat3Type *vecObjB = gmMatrix3::GetNative(reinterpret_cast<gmUserObject*>(a_operands[1].m_value.m_ref));
		a_operands[0].SetInt((*vecObjA) != (*vecObjB) ? 1 : 0);
	}
	else
	{
		a_operands[0].Nullify();
		return false;
	}
	return true;
}

//bool gmMatrix3::opGetIndex( gmMat3Type *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//
//	return true;
//}
//
//bool gmMatrix3::opSetIndex( gmMat3Type *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//
//	return true;
//}

bool gmMatrix3::gmfGetZero( gmMat3Type *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	gmMatrix3::SetObject( a_thread->GetMachine(), a_operands[0], gmMat3Type::ZERO );
	return true;
}

bool gmMatrix3::gmfGetIdentity( gmMat3Type *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	gmMatrix3::SetObject( a_thread->GetMachine(), a_operands[0], gmMat3Type::IDENTITY );
	return true;
}

bool gmMatrix3::gmfGetX( gmMat3Type *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetVector(
		a_native->GetColumn(0).x, 
		a_native->GetColumn(0).y, 
		a_native->GetColumn(0).z);
	return true;
}

bool gmMatrix3::gmfGetY( gmMat3Type *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetVector(
		a_native->GetColumn(1).x, 
		a_native->GetColumn(1).y, 
		a_native->GetColumn(1).z);
	return true;
}

bool gmMatrix3::gmfGetZ( gmMat3Type *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetVector(
		a_native->GetColumn(2).x, 
		a_native->GetColumn(2).y, 
		a_native->GetColumn(2).z);
	return true;
}
