
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../tr_local.h"

/*
========================
R_ComputePointLightProjectionMatrix

Computes the light projection matrix for a point light.
========================
*/
float R_ComputePointLightProjectionMatrix( idRenderLightLocal* light, idRenderMatrix& localProject )
{
    assert( light->parms.pointLight );

    // A point light uses a box projection.
    // This projects into the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range.
    localProject.Zero();
    localProject[0][0] = 0.5f / light->parms.lightRadius[0];
    localProject[1][1] = 0.5f / light->parms.lightRadius[1];
    localProject[2][2] = 0.5f / light->parms.lightRadius[2];
    localProject[0][3] = 0.5f;
    localProject[1][3] = 0.5f;
    localProject[2][3] = 0.5f;
    localProject[3][3] = 1.0f;	// identity perspective

    return 1.0f;
}

static const float SPOT_LIGHT_MIN_Z_NEAR	= 8.0f;
static const float SPOT_LIGHT_MIN_Z_FAR		= 16.0f;

/*
========================
R_ComputeSpotLightProjectionMatrix

Computes the light projection matrix for a spot light.
========================
*/
float R_ComputeSpotLightProjectionMatrix( idRenderLightLocal* light, idRenderMatrix& localProject )
{
    const float targetDistSqr = light->parms.target.LengthSqr();
    const float invTargetDist = idMath::InvSqrt( targetDistSqr );
    const float targetDist = invTargetDist * targetDistSqr;

    const idVec3 normalizedTarget = light->parms.target * invTargetDist;
    const idVec3 normalizedRight = light->parms.right * ( 0.5f * targetDist / light->parms.right.LengthSqr() );
    const idVec3 normalizedUp = light->parms.up * ( -0.5f * targetDist / light->parms.up.LengthSqr() );

    localProject[0][0] = normalizedRight[0];
    localProject[0][1] = normalizedRight[1];
    localProject[0][2] = normalizedRight[2];
    localProject[0][3] = 0.0f;

    localProject[1][0] = normalizedUp[0];
    localProject[1][1] = normalizedUp[1];
    localProject[1][2] = normalizedUp[2];
    localProject[1][3] = 0.0f;

    localProject[3][0] = normalizedTarget[0];
    localProject[3][1] = normalizedTarget[1];
    localProject[3][2] = normalizedTarget[2];
    localProject[3][3] = 0.0f;

    // Set the falloff vector.
    // This is similar to the Z calculation for depth buffering, which means that the
    // mapped texture is going to be perspective distorted heavily towards the zero end.
    const float zNear = Max( light->parms.start * normalizedTarget, SPOT_LIGHT_MIN_Z_NEAR );
    const float zFar = Max( light->parms.end * normalizedTarget, SPOT_LIGHT_MIN_Z_FAR );
    const float zScale = ( zNear + zFar ) / zFar;

    localProject[2][0] = normalizedTarget[0] * zScale;
    localProject[2][1] = normalizedTarget[1] * zScale;
    localProject[2][2] = normalizedTarget[2] * zScale;
    localProject[2][3] = - zNear * zScale;

    // now offset to the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range
    idVec4 projectedTarget;
    localProject.TransformPoint( light->parms.target, projectedTarget );

    const float ofs0 = 0.5f - projectedTarget[0] / projectedTarget[3];
    localProject[0][0] += ofs0 * localProject[3][0];
    localProject[0][1] += ofs0 * localProject[3][1];
    localProject[0][2] += ofs0 * localProject[3][2];
    localProject[0][3] += ofs0 * localProject[3][3];

    const float ofs1 = 0.5f - projectedTarget[1] / projectedTarget[3];
    localProject[1][0] += ofs1 * localProject[3][0];
    localProject[1][1] += ofs1 * localProject[3][1];
    localProject[1][2] += ofs1 * localProject[3][2];
    localProject[1][3] += ofs1 * localProject[3][3];

    return 1.0f / ( zNear + zFar );
}

/*
========================
R_ComputeParallelLightProjectionMatrix

Computes the light projection matrix for a parallel light.
========================
*/
float R_ComputeParallelLightProjectionMatrix( idRenderLightLocal* light, idRenderMatrix& localProject )
{
    assert( light->parms.parallel );

    // A parallel light uses a box projection.
    // This projects into the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range.
    localProject.Zero();
    localProject[0][0] = 0.5f / light->parms.lightRadius[0];
    localProject[1][1] = 0.5f / light->parms.lightRadius[1];
    localProject[2][2] = 0.5f / light->parms.lightRadius[2];
    localProject[0][3] = 0.5f;
    localProject[1][3] = 0.5f;
    localProject[2][3] = 0.5f;
    localProject[3][3] = 1.0f;	// identity perspective

    return 1.0f;
}

/*
============================
R_ShadowBounds

Even though the extruded shadows are drawn projected to infinity, their effects are limited
to a fraction of the light's volume.  An extruded box would require 12 faces to specify and
be a lot of trouble, but an axial bounding box is quick and easy to determine.

If the light is completely contained in the view, there is no value in trying to cull the
shadows, as they will all pass.

Pure function.
============================
*/
void R_ShadowBounds( const idBounds& modelBounds, const idBounds& lightBounds, const idVec3& lightOrigin, idBounds& shadowBounds )
{
	for( int i = 0; i < 3; i++ )
	{
		shadowBounds[0][i] = ( modelBounds[0][i] >= lightOrigin[i] ? modelBounds[0][i] : lightBounds[0][i] );
		shadowBounds[1][i] = ( lightOrigin[i] >= modelBounds[1][i] ? modelBounds[1][i] : lightBounds[1][i] );
	}
}

/*
========================
R_SetupFrontEndViewDefMVP

Compute and setup frontend view model-view-projection matrix.
========================
*/
void R_SetupFrontEndViewDefMVP(void)
{
    if(
#if defined(_SHADOW_MAPPING) && defined(_D3BFG_CULLING)
			r_useShadowMapping.GetBool() || harm_r_occlusionCulling.GetBool()
#elif defined(_SHADOW_MAPPING)
			r_useShadowMapping.GetBool()
#elif defined(_D3BFG_CULLING)
			harm_r_occlusionCulling.GetBool()
#else
			false
#endif
	)
    {
        // setup render matrices for faster culling
        idRenderMatrix		projectionRenderMatrix;	// tech5 version of projectionMatrix
        idRenderMatrix::Transpose( ID_TO_RENDER_MATRIX tr.viewDef->projectionMatrix, /*tr.viewDef->*/projectionRenderMatrix );
        idRenderMatrix viewRenderMatrix;
        idRenderMatrix::Transpose( ID_TO_RENDER_MATRIX tr.viewDef->worldSpace.modelViewMatrix, viewRenderMatrix );
        idRenderMatrix::Multiply( /*tr.viewDef->*/projectionRenderMatrix, viewRenderMatrix, ID_RENDER_MATRIX tr.viewDef->worldSpace.mvp );
    }
}

void R_SetupSplitFrustums( viewDef_t* viewDef )
{
    idVec3			planeOrigin;

    const float zNearStart = ( viewDef->renderView.cramZNear ) ? ( r_znear.GetFloat() * 0.25f ) : r_znear.GetFloat();
    float zFarEnd = 10000;

    float zNear = zNearStart;
    float zFar = zFarEnd;

    float lambda = r_shadowMapSplitWeight.GetFloat();
    float ratio = zFarEnd / zNearStart;

    for( int i = 0; i < 6; i++ )
    {
        tr.viewDef->frustumSplitDistances[i] = idMath::INFINITY;
    }

    for( int i = 1; i <= ( r_shadowMapSplits.GetInteger() + 1 ) && i < MAX_FRUSTUMS; i++ )
    {
        float si = (float)i / ( float )( r_shadowMapSplits.GetInteger() + 1 );

        if( i > FRUSTUM_CASCADE1 )
        {
            zNear = zFar - ( zFar * 0.005f );
        }

        zFar = 1.005f * lambda * ( zNearStart * powf( ratio, si ) ) + ( 1 - lambda ) * ( zNearStart + ( zFarEnd - zNearStart ) * si );

        if( i <= r_shadowMapSplits.GetInteger() )
        {
            tr.viewDef->frustumSplitDistances[i - 1] = zFar;
        }

        float projectionMatrix[16];
        R_SetupProjectionMatrix2( tr.viewDef, zNear, zFar, projectionMatrix );

        // setup render matrices for faster culling
        idRenderMatrix projectionRenderMatrix;
        idRenderMatrix::Transpose( ID_TO_RENDER_MATRIX projectionMatrix, projectionRenderMatrix );
        idRenderMatrix viewRenderMatrix;
        idRenderMatrix::Transpose( ID_TO_RENDER_MATRIX tr.viewDef->worldSpace.modelViewMatrix, viewRenderMatrix );
        idRenderMatrix::Multiply( projectionRenderMatrix, viewRenderMatrix, tr.viewDef->frustumMVPs[i] );

        // the planes of the view frustum are needed for portal visibility culling
        idRenderMatrix::GetFrustumPlanes( tr.viewDef->frustums[i], tr.viewDef->frustumMVPs[i], false, true );

        // the DOOM 3 frustum planes point outside the frustum
        for( int j = 0; j < 6; j++ )
        {
            tr.viewDef->frustums[i][j] = - tr.viewDef->frustums[i][j];
        }

        // remove the Z-near to avoid portals from being near clipped
        if( i == FRUSTUM_CASCADE1 )
        {
            tr.viewDef->frustums[i][4][3] -= r_znear.GetFloat();
        }
    }
}

void R_SetupFrontEndFrustums(void)
{
    if(r_useShadowMapping.GetBool())
#ifdef GL_ES_VERSION_3_0
    if(USING_GLES3 && r_shadowMapSplits.GetInteger() > 0)
#endif
    {
        // the planes of the view frustum are needed for portal visibility culling
        idRenderMatrix::GetFrustumPlanes( tr.viewDef->frustums[FRUSTUM_PRIMARY], tr.viewDef->worldSpace.mvp, false, true );

        // the DOOM 3 frustum planes point outside the frustum
        for( int i = 0; i < 6; i++ )
        {
            tr.viewDef->frustums[FRUSTUM_PRIMARY][i] = - tr.viewDef->frustums[FRUSTUM_PRIMARY][i];
        }
        // remove the Z-near to avoid portals from being near clipped
        tr.viewDef->frustums[FRUSTUM_PRIMARY][4][3] -= r_znear.GetFloat();

        R_SetupSplitFrustums( tr.viewDef );
    }
}
