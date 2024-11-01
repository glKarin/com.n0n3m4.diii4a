#include "gmConfig.h"

#if(GM_USE_VECTOR3_STACK)

#include "gmVec3Stack.h"
#include "gmThread.h"
#include "gmHelpers.h"

#include "mathlib/matrix.h"

gmVec3Data ZERO_VEC3 = {};

//////////////////////////////////////////////////////////////////////////
// Operators
static int GM_CDECL gmVector3OpAdd( gmThread * a_thread, gmVariable * a_operands )
{
	if ( a_operands[ 0 ].IsVector() && a_operands[ 1 ].IsVector() )
	{
		a_operands[ 0 ].SetVector(
			a_operands[ 0 ].m_value.m_vec3.x + a_operands[ 1 ].m_value.m_vec3.x,
			a_operands[ 0 ].m_value.m_vec3.y + a_operands[ 1 ].m_value.m_vec3.y,
			a_operands[ 0 ].m_value.m_vec3.z + a_operands[ 1 ].m_value.m_vec3.z );
		return GM_OK;
	}

	a_operands[ 0 ].Nullify();
	return GM_EXCEPTION;
}
static int GM_CDECL gmVector3OpSub( gmThread * a_thread, gmVariable * a_operands )
{
	if ( a_operands[ 0 ].IsVector() && a_operands[ 1 ].IsVector() )
	{
		a_operands[ 0 ].SetVector(
			a_operands[ 0 ].m_value.m_vec3.x - a_operands[ 1 ].m_value.m_vec3.x,
			a_operands[ 0 ].m_value.m_vec3.y - a_operands[ 1 ].m_value.m_vec3.y,
			a_operands[ 0 ].m_value.m_vec3.z - a_operands[ 1 ].m_value.m_vec3.z );
		return GM_OK;
	}

	a_operands[ 0 ].Nullify();
	return GM_EXCEPTION;
}
static int GM_CDECL gmVector3OpNEG( gmThread * a_thread, gmVariable * a_operands )
{
	if ( a_operands[ 0 ].IsVector() )
	{
		a_operands[ 0 ].SetVector(
			-a_operands[ 0 ].m_value.m_vec3.x,
			-a_operands[ 0 ].m_value.m_vec3.y,
			-a_operands[ 0 ].m_value.m_vec3.z );
		return GM_OK;
	}

	a_operands[ 0 ].Nullify();
	return GM_EXCEPTION;
}
static int GM_CDECL gmVector3OpMul( gmThread * a_thread, gmVariable * a_operands )
{
	float fScalar = 0.f;
	if ( a_operands[ 0 ].IsVector() && a_operands[ 1 ].GetFloatSafe( fScalar ) )
	{
		a_operands[ 0 ].SetVector(
			a_operands[ 0 ].m_value.m_vec3.x * fScalar,
			a_operands[ 0 ].m_value.m_vec3.y * fScalar,
			a_operands[ 0 ].m_value.m_vec3.z * fScalar );
		return GM_OK;
	}
	else if ( a_operands[ 1 ].IsVector() && a_operands[ 0 ].GetFloatSafe( fScalar ) )
	{
		a_operands[ 0 ].SetVector(
			a_operands[ 1 ].m_value.m_vec3.x * fScalar,
			a_operands[ 1 ].m_value.m_vec3.y * fScalar,
			a_operands[ 1 ].m_value.m_vec3.z * fScalar );
		return GM_OK;
	}
	else if ( a_operands[ 1 ].IsVector() && a_operands[ 0 ].IsVector() )
	{
		a_operands[ 0 ].SetVector(
			a_operands[ 1 ].m_value.m_vec3.x * a_operands[ 0 ].m_value.m_vec3.x,
			a_operands[ 1 ].m_value.m_vec3.y * a_operands[ 0 ].m_value.m_vec3.y,
			a_operands[ 1 ].m_value.m_vec3.z * a_operands[ 0 ].m_value.m_vec3.z );
		return GM_OK;
	}

	a_operands[ 0 ].Nullify();
	return GM_EXCEPTION;
}
static int GM_CDECL gmVector3OpDiv( gmThread * a_thread, gmVariable * a_operands )
{
#if GMMACHINE_GMCHECKDIVBYZERO
	if(a_operands[0].IsVector() && a_operands[1].IsNumber())
	{
		const float fScalar = a_operands[1].GetFloatSafe();
		if(fScalar != 0.f)
		{
			a_operands[0].SetVector(
				a_operands[0].m_value.m_vec3.x / fScalar, 
				a_operands[0].m_value.m_vec3.y / fScalar, 
				a_operands[0].m_value.m_vec3.z / fScalar);
		}
		else
		{
			a_thread->GetMachine()->GetLog().LogEntry("divide by zero.");
			a_operands[0].Nullify(); // NOTE: Should probably return +/- INF, not null
			return GM_EXCEPTION;
		}
	}
#else // GMMACHINE_GMCHECKDIVBYZERO
	float fScalar = 0.f;
	if ( a_operands[ 0 ].IsVector() && a_operands[ 1 ].GetFloatSafe( fScalar ) )
	{
		a_operands[ 0 ].SetVector(
			a_operands[ 0 ].m_value.m_vec3.x / fScalar,
			a_operands[ 0 ].m_value.m_vec3.y / fScalar,
			a_operands[ 0 ].m_value.m_vec3.z / fScalar );
		return GM_OK;
	}
#endif // GMMACHINE_GMCHECKDIVBYZERO
	a_operands[ 0 ].Nullify();
	return GM_EXCEPTION;
}
static int GM_CDECL gmVector3OpEQ( gmThread * a_thread, gmVariable * a_operands )
{
	if ( a_operands[ 0 ].IsVector() )
	{
		if ( a_operands[ 1 ].IsVector() )
		{
			a_operands[ 0 ].SetInt(
				( a_operands[ 0 ].m_value.m_vec3.x == a_operands[ 1 ].m_value.m_vec3.x &&
				a_operands[ 0 ].m_value.m_vec3.y == a_operands[ 1 ].m_value.m_vec3.y &&
				a_operands[ 0 ].m_value.m_vec3.z == a_operands[ 1 ].m_value.m_vec3.z ) ? 1 : 0 );
			return GM_OK;
		}
		else if ( a_operands[ 1 ].IsNull() )
		{
			a_operands[ 0 ].SetInt( 0 );
			return GM_OK;
		}
	}

	a_operands[ 0 ].Nullify();
	return GM_EXCEPTION;
}
static int GM_CDECL gmVector3OpNEQ( gmThread * a_thread, gmVariable * a_operands )
{
	if ( a_operands[ 0 ].IsVector() )
	{
		if ( a_operands[ 1 ].IsVector() )
		{
			a_operands[ 0 ].SetInt(
				( a_operands[ 0 ].m_value.m_vec3.x == a_operands[ 1 ].m_value.m_vec3.x &&
				a_operands[ 0 ].m_value.m_vec3.y == a_operands[ 1 ].m_value.m_vec3.y &&
				a_operands[ 0 ].m_value.m_vec3.z == a_operands[ 1 ].m_value.m_vec3.z ) ? 0 : 1 );
			return GM_OK;
		}
		else if ( a_operands[ 1 ].IsNull() )
		{
			a_operands[ 0 ].SetInt( 1 );
			return GM_OK;
		}
	}

	a_operands[ 0 ].Nullify();
	return GM_EXCEPTION;
}

static int GM_CDECL gmVector3OpPOS( gmThread * a_thread, gmVariable * a_operands )
{
	return GM_EXCEPTION;
}

static int GM_CDECL gmVector3GetDot( gmThread * a_thread, gmVariable * a_operands )
{
	gmStringObject *pStr = a_operands[ 1 ].GetStringObjectSafe();
	if ( pStr )
	{
		if ( !_gmstricmp( pStr->GetString(), "x" ) )
		{
			a_operands[ 0 ].SetFloat( a_operands[ 0 ].m_value.m_vec3.x );
			return GM_OK;
		}
		else if ( !_gmstricmp( pStr->GetString(), "y" ) )
		{
			a_operands[ 0 ].SetFloat( a_operands[ 0 ].m_value.m_vec3.y );
			return GM_OK;
		}
		else if ( !_gmstricmp( pStr->GetString(), "z" ) )
		{
			a_operands[ 0 ].SetFloat( a_operands[ 0 ].m_value.m_vec3.z );
			return GM_OK;
		}
		else if ( !_gmstricmp( pStr->GetString(), "UNIT_X" ) )
		{
			a_operands[ 0 ].SetVector( 1.f, 0.f, 0.f );
			return GM_OK;
		}
		else if ( !_gmstricmp( pStr->GetString(), "UNIT_Y" ) )
		{
			a_operands[ 0 ].SetVector( 0.f, 1.f, 0.f );
			return GM_OK;
		}
		else if ( !_gmstricmp( pStr->GetString(), "UNIT_X" ) )
		{
			a_operands[ 0 ].SetVector( 0.f, 0.f, 1.f );
			return GM_OK;
		}
		else if ( !_gmstricmp( pStr->GetString(), "ZERO" ) )
		{
			a_operands[ 0 ].SetVector( 0.f, 0.f, 0.f );
			return GM_OK;
		}
	}
	a_operands[ 0 ].Nullify();
	return GM_EXCEPTION;
}
//static void GM_CDECL gmVector3SetDot(gmThread * a_thread, gmVariable * a_operands)
//{
//	gmStringObject *pStr = a_operands[2].GetStringObjectSafe();
//	if(pStr)
//	{
//		if(!_gmstricmp(pStr->GetString(), "x"))
//			a_operands[1].GetFloatSafe(a_operands[0].m_value.m_vec3.x);
//		else if(!_gmstricmp(pStr->GetString(), "y"))
//			a_operands[1].GetFloatSafe(a_operands[0].m_value.m_vec3.y);
//		else if(!_gmstricmp(pStr->GetString(), "z"))
//			a_operands[1].GetFloatSafe(a_operands[0].m_value.m_vec3.z);
//		else
//			a_operands[0].Nullify();
//	}
//	else
//		a_operands[0].Nullify();
//}
static int GM_CDECL gmVector3GetInd( gmThread * a_thread, gmVariable * a_operands )
{
	int iIndex = 0;
	if ( a_operands[ 1 ].IsInt() && a_operands[ 1 ].GetIntSafe( iIndex, 0 ) && iIndex >= 0 && iIndex < 3 )
	{
		switch ( iIndex )
		{
			case 0:
				a_operands[ 0 ].SetFloat( a_operands[ 0 ].m_value.m_vec3.x );
				return GM_OK;
			case 1:
				a_operands[ 0 ].SetFloat( a_operands[ 0 ].m_value.m_vec3.y );
				return GM_OK;
			case 2:
				a_operands[ 0 ].SetFloat( a_operands[ 0 ].m_value.m_vec3.z );
				return GM_OK;
			default:
				a_thread->GetMachine()->GetLog().LogEntry( "index out of range %d", iIndex );
				return GM_EXCEPTION;
		}
	}

	a_operands[ 0 ].Nullify();
	return GM_EXCEPTION;
}
//static void GM_CDECL gmVector3SetInd(gmThread * a_thread, gmVariable * a_operands)
//{
//	float fVal = 0.f;
//	int iIndex = a_operands[1].GetInt();
//	if(a_operands[2].GetFloatSafe(fVal) && iIndex >=0 && iIndex < 3)
//	{
//		switch(iIndex)
//		{
//		case 0:
//			a_operands[0].m_value.m_vec3.x = fVal;
//			break;
//		case 1:
//			a_operands[0].m_value.m_vec3.y = fVal;
//			break;
//		case 2:
//			a_operands[0].m_value.m_vec3.z = fVal;
//			break;
//		default:
//			a_operands[0].Nullify();
//			return;
//		}
//	}
//	else
//		a_operands[0].Nullify();
//}
static int GM_CDECL gmVector3OpNOT( gmThread * a_thread, gmVariable * a_operands )
{
	if ( a_operands->m_type == GM_NULL )
		a_operands[ 0 ].SetInt( 1 );
	else
		a_operands[ 0 ].SetInt( 0 );
	a_operands->m_type = GM_INT;
	return GM_OK;
}
static int GM_CDECL gmVector3OpBOOL( gmThread * a_thread, gmVariable * a_operands )
{
	a_operands[ 0 ].SetInt( 1 );
	return GM_OK;
}
//////////////////////////////////////////////////////////////////////////

static int GM_CDECL gmVec3Create( gmThread *a_thread )
{
	switch ( a_thread->GetNumParams() )
	{
		case 0:
			a_thread->PushVector( Vec3( 0.f, 0.f, 0.f ) );
			break;
		case 1:
		{
			float num = gmGetFloatOrIntParamAsFloat( a_thread, 0 );
			a_thread->PushVector( Vec3( num, num, num ) );
			break;
		}
		case 2:
			a_thread->PushVector( Vec3(
				gmGetFloatOrIntParamAsFloat( a_thread, 0 ),
				gmGetFloatOrIntParamAsFloat( a_thread, 1 ),
				0.f ) );
			break;
		case 3:
			a_thread->PushVector( Vec3( gmGetFloatOrIntParamAsFloat( a_thread, 0 ), gmGetFloatOrIntParamAsFloat( a_thread, 1 ), gmGetFloatOrIntParamAsFloat( a_thread, 2 ) ) );
			break;
		default:
			//LOG_ERROR << "Invalid number of paramters to math.Vector() - expecting 0, 1, 2, or 3" << nl;
			break;
	}
	return GM_OK;
}
//////////////////////////////////////////////////////////////////////////
// Functions
static int GM_CDECL gmVec3Normalize( gmThread *a_thread )
{
	Vec3 v = ConvertVec3( a_thread->ThisVector() );
	v.Normalize();
	a_thread->PushVector( v );
	return GM_OK;
}
static int GM_CDECL gmVec3Length( gmThread *a_thread )
{
	Vec3 v = ConvertVec3( a_thread->ThisVector() );
	a_thread->PushFloat( v.Length() );
	return GM_OK;
}
static int GM_CDECL gmVec3Length2( gmThread *a_thread )
{
	Vec3 v = ConvertVec3( a_thread->ThisVector() );
	a_thread->PushFloat( v.Length2() );
	return GM_OK;
}
static int GM_CDECL gmVec3Concave( gmThread *a_thread )
{
	Vec3 v = ConvertVec3( a_thread->ThisVector() );
	GM_CHECK_VECTOR_PARAM( v0, 0 );
	GM_CHECK_VECTOR_PARAM( v1, 1 );
	a_thread->PushInt( v.Concave( v0, v1 ) ? 1 : 0 );
	return GM_OK;
}
static int GM_CDECL gmVec3CrossProduct( gmThread *a_thread )
{
	Vec3 v = ConvertVec3( a_thread->ThisVector() );
	GM_CHECK_VECTOR_PARAM( v0, 0 );
	Vec3 vOut;
	a_thread->PushVector( v.Cross( v0 ) );
	return GM_OK;
}
static int GM_CDECL gmVec3DotProduct( gmThread *a_thread )
{
	Vec3 v = ConvertVec3( a_thread->ThisVector() );
	GM_CHECK_VECTOR_PARAM( v0, 0 );
	a_thread->PushFloat( v.Dot( v0 ) );
	return GM_OK;
}
static int GM_CDECL gmVec3Distance( gmThread *a_thread )
{
	Vec3 v = ConvertVec3( a_thread->ThisVector() );
	GM_CHECK_VECTOR_PARAM( v0, 0 );
	a_thread->PushFloat( v.Distance( v0 ) );
	return GM_OK;
}
static int GM_CDECL gmVec3Distance2( gmThread *a_thread )
{
	Vec3 v = ConvertVec3( a_thread->ThisVector() );
	GM_CHECK_VECTOR_PARAM( v0, 0 );
	a_thread->PushFloat( v.Distance2( v0 ) );
	return GM_OK;
}
static int GM_CDECL gmVec3DistanceXY( gmThread *a_thread )
{
	Vec3 v = ConvertVec3( a_thread->ThisVector() );
	GM_CHECK_VECTOR_PARAM( v0, 0 );
	a_thread->PushFloat( v.DistanceXY( v0 ) );
	return GM_OK;
}
static int GM_CDECL gmVec3Reflection( gmThread *a_thread )
{
	Vec3 v = ConvertVec3( a_thread->ThisVector() );
	GM_CHECK_VECTOR_PARAM( v0, 0 );
	Vec3 vOut;
	vOut.Reflection( v, v0 );
	a_thread->PushVector( vOut );
	return GM_OK;
}

static int GM_CDECL gmVec3IsZero( gmThread *a_thread )
{
	Vec3 v = ConvertVec3( a_thread->ThisVector() );
	a_thread->PushInt( Vec3( 0, 0, 0 ) == v ? 1 : 0 );
	return GM_OK;
}
static int GM_CDECL gmVec3Set( gmThread *a_thread )
{
	Vec3 v = ConvertVec3( a_thread->ThisVector() );
	if ( a_thread->Param( 0 ).IsNumber() )
		v.x = a_thread->Param( 0 ).GetFloatSafe();
	if ( a_thread->Param( 1 ).IsNumber() )
		v.y = a_thread->Param( 1 ).GetFloatSafe();
	if ( a_thread->Param( 2 ).IsNumber() )
		v.z = a_thread->Param( 2 ).GetFloatSafe();
	a_thread->PushVector( v );
	return GM_OK;
}
static int GM_CDECL gmVec3GetYaw( gmThread *a_thread )
{
	Vec3 v = ConvertVec3( a_thread->ThisVector() );
	a_thread->PushFloat( v.GetYaw() );
	return GM_OK;
}
static int GM_CDECL gmVec3GetPitch( gmThread *a_thread )
{
	Vec3 v = ConvertVec3( a_thread->ThisVector() );
	a_thread->PushFloat( v.GetPitch() );
	return GM_OK;
}
//////////////////////////////////////////////////////////////////////////
// Math funcs
static int GM_CDECL gmArea( gmThread *a_thread )
{
	GM_CHECK_VECTOR_PARAM( v0, 0 );
	GM_CHECK_VECTOR_PARAM( v1, 1 );
	GM_CHECK_VECTOR_PARAM( v2, 2 );
	a_thread->PushFloat( v0.Area( v1, v2 ) );
	return GM_OK;
}
static int GM_CDECL gmAngleAxis( gmThread *a_thread )
{
	GM_CHECK_FLOAT_OR_INT_PARAM( angl, 0 );
	GM_CHECK_VECTOR_PARAM( axis, 1 );
	Vec3 vOut;
	vOut.AngleAxis( angl*DEG_TO_RAD, axis );
	a_thread->PushVector( vOut );
	return GM_OK;
}
static int GM_CDECL gmComputeNormal( gmThread *a_thread )
{
	GM_CHECK_VECTOR_PARAM( v0, 0 );
	GM_CHECK_VECTOR_PARAM( v1, 1 );
	GM_CHECK_VECTOR_PARAM( v2, 2 );
	Vec3 vOut;
	vOut.ComputeNormal( v0, v1, v2 );
	a_thread->PushVector( vOut );
	return GM_OK;
}
static int GM_CDECL gmCrossProduct( gmThread *a_thread )
{
	GM_CHECK_VECTOR_PARAM( v0, 0 );
	GM_CHECK_VECTOR_PARAM( v1, 1 );
	Vec3 vOut;
	vOut.Cross( v0, v1 );
	a_thread->PushVector( vOut );
	return GM_OK;
}
static int GM_CDECL gmDotProduct( gmThread *a_thread )
{
	GM_CHECK_VECTOR_PARAM( v0, 0 );
	GM_CHECK_VECTOR_PARAM( v1, 1 );
	a_thread->PushFloat( v0.Dot( v1 ) );
	return GM_OK;
}
static int GM_CDECL gmLerp( gmThread *a_thread )
{
	GM_CHECK_VECTOR_PARAM( v0, 0 );
	GM_CHECK_VECTOR_PARAM( v1, 1 );
	GM_CHECK_FLOAT_OR_INT_PARAM( t, 2 );
	Vec3 vOut;
	vOut.Lerp( v0, v1, t );
	a_thread->PushVector( vOut );
	return GM_OK;
}
static int GM_CDECL gmReflection( gmThread *a_thread )
{
	GM_CHECK_VECTOR_PARAM( v0, 0 );
	GM_CHECK_VECTOR_PARAM( v1, 1 );
	Vec3 vOut;
	vOut.Reflection( v0, v1 );
	a_thread->PushVector( vOut );
	return GM_OK;
}
static int GM_CDECL gmNearestPointInLine( gmThread *a_thread )
{
	GM_CHECK_VECTOR_PARAM( pt, 0 );
	GM_CHECK_VECTOR_PARAM( v0, 1 );
	GM_CHECK_VECTOR_PARAM( v1, 2 );
	Vec3 vOut;
	vOut.NearestPointInLine( pt, v0, v1 );
	a_thread->PushVector( vOut );
	return GM_OK;
}
static int GM_CDECL gmNearestPointInLineSegment( gmThread *a_thread )
{
	GM_CHECK_VECTOR_PARAM( pt, 0 );
	GM_CHECK_VECTOR_PARAM( v0, 1 );
	GM_CHECK_VECTOR_PARAM( v1, 2 );
	Vec3 vOut;
	vOut.NearestPointInLineSegment( pt, v0, v1 );
	a_thread->PushVector( vOut );
	return GM_OK;
}
static int GM_CDECL gmNearestPointInPlane( gmThread *a_thread )
{
	if ( a_thread->GetNumParams() == 4 )
	{
		GM_CHECK_VECTOR_PARAM( pt, 0 );
		GM_CHECK_VECTOR_PARAM( v0, 1 );
		GM_CHECK_VECTOR_PARAM( v1, 2 );
		GM_CHECK_VECTOR_PARAM( v2, 3 );

		Vec3 vOut;
		vOut.NearestPointInPlane( pt, v0, v1, v2 );
		a_thread->PushVector( vOut );
		return GM_OK;
	}
	else if ( a_thread->GetNumParams() == 3 )
	{
		GM_CHECK_VECTOR_PARAM( pt, 0 );
		GM_CHECK_VECTOR_PARAM( pp, 1 );
		GM_CHECK_VECTOR_PARAM( pn, 1 );

		Vec3 vOut;
		vOut.NearestPointInPlane( pt, pp, pn );
		a_thread->PushVector( vOut );
		return GM_OK;
	}

	GM_EXCEPTION_MSG( "expected 3 or 4 vector3" );
	return GM_EXCEPTION;
}
static int GM_CDECL gmNearestPointInTriangle( gmThread *a_thread )
{
	GM_CHECK_VECTOR_PARAM( pt, 0 );
	GM_CHECK_VECTOR_PARAM( v0, 1 );
	GM_CHECK_VECTOR_PARAM( v1, 2 );
	GM_CHECK_VECTOR_PARAM( v2, 3 );

	Vec3 vOut;
	vOut.NearestPointInTriangle( pt, v0, v1, v2 );
	a_thread->PushVector( vOut );
	return GM_OK;
}
static int GM_CDECL gmVec3FromSpherical( gmThread *a_thread )
{
	GM_CHECK_FLOAT_OR_INT_PARAM( hdg, 0 );
	GM_CHECK_FLOAT_OR_INT_PARAM( pitch, 1 );
	GM_FLOAT_OR_INT_PARAM( rad, 2, 1.f );
	Vec3 v;
	v.FromSpherical( hdg, pitch, rad );
	a_thread->PushVector( v );
	return GM_OK;
}
//////////////////////////////////////////////////////////////////////////
gmFunctionEntry vec3_lib [] =
{
	{ "Vector3", gmVec3Create },
	{ "Vec3", gmVec3Create },
	{ "Vector2", gmVec3Create },
	{ "Vec2", gmVec3Create },

	{ "Area", gmArea },
	{ "AngleAxis", gmAngleAxis },
	{ "ComputeNormal", gmComputeNormal },
	{ "CrossProduct", gmCrossProduct },
	{ "Cross", gmCrossProduct },
	{ "DotProduct", gmDotProduct },
	{ "Lerp", gmLerp },
	{ "Reflection", gmReflection },
	{ "NearestPointInLine", gmNearestPointInLine },
	{ "NearestPointInLineSegment", gmNearestPointInLineSegment },
	{ "NearestPointInPlane", gmNearestPointInPlane },
	{ "PointInTriangle", gmNearestPointInTriangle },
	{ "Vec3FromSpherical", gmVec3FromSpherical },

};
gmFunctionEntry vec3_methods [] =
{
	{ "Normalize", gmVec3Normalize },
	{ "Length", gmVec3Length },
	{ "Length2", gmVec3Length2 },

	{ "Concave", gmVec3Concave },
	{ "Distance", gmVec3Distance },
	{ "Distance2", gmVec3Distance2 },
	{ "DistanceXY", gmVec3DistanceXY },
	{ "CrossProduct", gmVec3CrossProduct },
	{ "DotProduct", gmVec3DotProduct },
	{ "Reflection", gmVec3Reflection },

	{ "IsZero", gmVec3IsZero },

	{ "Set", gmVec3Set },
	{ "SetXYZ", gmVec3Set },

	{ "GetYaw", gmVec3GetYaw },
	{ "GetPitch", gmVec3GetPitch },
};

void BindVector3Stack( gmMachine *a_machine )
{
	a_machine->RegisterLibrary( vec3_lib, sizeof( vec3_lib ) / sizeof( vec3_lib[ 0 ] ) );
	a_machine->RegisterTypeLibrary( GM_VEC3, vec3_methods, sizeof( vec3_methods ) / sizeof( vec3_methods[ 0 ] ) );

	a_machine->RegisterTypeOperator( GM_VEC3, O_GETDOT, 0, gmVector3GetDot );
	a_machine->RegisterTypeOperator( GM_VEC3, O_GETIND, 0, gmVector3GetInd );
	//a_machine->RegisterTypeOperator(GM_VEC3, O_SETDOT, 0, gmVector3SetDot);
	//a_machine->RegisterTypeOperator(GM_VEC3, O_SETIND, 0, gmVector3SetInd);
	//a_machine->RegisterTypeOperator(GM_VEC3, O_BIT_AND, 0, gm_vector_op_and);
	a_machine->RegisterTypeOperator( GM_VEC3, O_ADD, 0, gmVector3OpAdd );
	a_machine->RegisterTypeOperator( GM_VEC3, O_SUB, 0, gmVector3OpSub );
	a_machine->RegisterTypeOperator( GM_VEC3, O_MUL, 0, gmVector3OpMul );
	a_machine->RegisterTypeOperator( GM_VEC3, O_DIV, 0, gmVector3OpDiv );
	a_machine->RegisterTypeOperator( GM_VEC3, O_EQ, 0, gmVector3OpEQ );
	a_machine->RegisterTypeOperator( GM_VEC3, O_NEQ, 0, gmVector3OpNEQ );
	a_machine->RegisterTypeOperator( GM_VEC3, O_POS, 0, gmVector3OpPOS );
	a_machine->RegisterTypeOperator( GM_VEC3, O_NOT, 0, gmVector3OpNOT );
	a_machine->RegisterTypeOperator( GM_VEC3, O_NEG, 0, gmVector3OpNEG );

#if GM_BOOL_OP
	a_machine->RegisterTypeOperator( GM_VEC3, O_BOOL, 0, gmVector3OpBOOL );
#endif	
}

#endif