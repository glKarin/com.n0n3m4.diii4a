
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../tr_local.h"

#define idVec3_INIT_WITH_ONE_NUM(x) idVec3((x), (x), (x))

idBounds bounds_zeroOneCube( idVec3_INIT_WITH_ONE_NUM( 0.0f ), idVec3_INIT_WITH_ONE_NUM( 1.0f ) );
idBounds bounds_unitCube( idVec3_INIT_WITH_ONE_NUM( -1.0f ), idVec3_INIT_WITH_ONE_NUM( 1.0f ) );

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
========================
R_CalcShadowMappingLOD

Computes the light shadow map LOD.
========================
*/
void R_SetupShadowMappingLOD(const idRenderLightLocal *light, viewLight_t *vLight)
{
    int shadowLod = -1;
    if(r_useShadowMapping.GetBool()) {
        // Calculate the matrix that projects the zero-to-one cube to exactly cover the
        // light frustum in clip space.
        idRenderMatrix invProjectMVPMatrix;
        idRenderMatrix::Multiply(*(idRenderMatrix *) tr.viewDef->worldSpace.mvp,
                                 *(idRenderMatrix *) light->inverseBaseLightProject,
                                 invProjectMVPMatrix);

        // Calculate the projected bounds, either not clipped at all, near clipped, or fully clipped.
        idBounds projected;
        if (harm_r_useLightScissors.GetInteger() == 1) {
            idRenderMatrix::ProjectedBounds(projected, invProjectMVPMatrix, bounds_zeroOneCube);
        } else if (harm_r_useLightScissors.GetInteger() == 2) {
            idRenderMatrix::ProjectedNearClippedBounds(projected, invProjectMVPMatrix,
                                                       bounds_zeroOneCube);
        } else {
            idRenderMatrix::ProjectedFullyClippedBounds(projected, invProjectMVPMatrix,
                                                        bounds_zeroOneCube);
        }

        if (projected[0][2] < projected[1][2]) {
            /*float screenWidth =
                    (float) tr.viewDef->viewport.x2 - (float) tr.viewDef->viewport.x1;
            float screenHeight =
                    (float) tr.viewDef->viewport.y2 - (float) tr.viewDef->viewport.y1;
            idScreenRect lightScissorRect;
            lightScissorRect.x1 = idMath::Ftoi(projected[0][0] * screenWidth);
            lightScissorRect.x2 = idMath::Ftoi(projected[1][0] * screenWidth);
            lightScissorRect.y1 = idMath::Ftoi(projected[0][1] * screenHeight);
            lightScissorRect.y2 = idMath::Ftoi(projected[1][1] * screenHeight);
            lightScissorRect.Expand();

            vLight->scissorRect.Intersect(lightScissorRect);
            vLight->scissorRect.zmin = projected[0][2];
            vLight->scissorRect.zmax = projected[1][2];*/

            const bool lightCastsShadows = /*light->parms.forceShadows || */(
                    !light->parms.noShadows && light->lightShader->LightCastsShadows());
            // RB: calculate shadow LOD similar to Q3A .md3 LOD code

            if (/*r_useShadowMapping.GetBool() && */lightCastsShadows) {
                if (harm_r_shadowMapLod.GetInteger() >= 0 && harm_r_shadowMapLod.GetInteger() < MAX_SHADOWMAP_RESOLUTIONS)
                    shadowLod = harm_r_shadowMapLod.GetInteger();
                else
                {
                    float flod, lodscale;
                    float projectedRadius;
                    int lod = 0;
                    int numLods;

                    shadowLod = 0;
                    numLods = MAX_SHADOWMAP_RESOLUTIONS;

                    // compute projected bounding sphere
                    // and use that as a criteria for selecting LOD
                    idVec3 center = projected.GetCenter();
                    projectedRadius = projected.GetRadius(center);
                    if (projectedRadius > 1.0f) {
                        projectedRadius = 1.0f;
                    }

                    if (projectedRadius != 0) {
                        lodscale = r_shadowMapLodScale.GetFloat();

                        if (lodscale > 20)
                            lodscale = 20;

                        flod = 1.0f - projectedRadius * lodscale;
                    } else {
                        // object intersects near view plane, e.g. view weapon
                        flod = 0;
                    }

                    flod *= ( numLods + 1 );

                    if (flod < 0) {
                        flod = 0;
                    }

                    lod = idMath::Ftoi(flod);
                    lod += r_shadowMapLodBias.GetInteger();

                    if( lod < 0 )
                    {
                        lod = 0;
                    }

                    if (lod >= numLods) {
                        //lod = numLods - 1;
                        // don't draw any shadow
                        lod = -1;
                    }

                    // 2048^2 ultra quality is only for cascaded shadow mapping with sun lights
                    if (lod == 0 && !light->parms.parallel) {
                        if(!harm_r_shadowMapNonParallelLightUltra.GetBool()) //k non parallel light allow using ultra quality shadow map texture
                        lod = 1;
                    }

                    shadowLod = lod;
                    // RB end
                }
            }
        }
        // the light was culled to the view frustum
    }

    vLight->shadowLOD = shadowLod;
}

/*
========================
R_SetupShadowMappingProjectionMatrix

Setup the light base projection matrix and inverse base projection matrix.
========================
*/
void R_SetupShadowMappingProjectionMatrix(idRenderLightLocal *light)
{
    // if(r_useShadowMapping.GetBool()) //k: always calc when a light spawned
    {
        idRenderMatrix localProject;
        float zScale = 1.0f;
        if( light->parms.parallel )
        {
            zScale = R_ComputeParallelLightProjectionMatrix( light, localProject );
        }
        else if( light->parms.pointLight )
        {
            zScale = R_ComputePointLightProjectionMatrix( light, localProject );
        }
        else
        {
            zScale = R_ComputeSpotLightProjectionMatrix( light, localProject );
        }

        // Rotate and translate the light projection by the light matrix.
        // 99% of lights remain axis aligned in world space.
        idRenderMatrix lightMatrix;
        idRenderMatrix::CreateFromOriginAxis( light->parms.origin, light->parms.axis, lightMatrix );

        idRenderMatrix inverseLightMatrix;
        if( !idRenderMatrix::Inverse( lightMatrix, inverseLightMatrix ) )
        {
            idLib::Warning( "lightMatrix invert failed" );
        }
        //else inverseLightMatrix.Identity();

        // 'baseLightProject' goes from global space -> light local space -> light projective space
        idRenderMatrix::Multiply( localProject, inverseLightMatrix, *(idRenderMatrix *)light->baseLightProject );

        // Invert the light projection so we can deform zero-to-one cubes into
        // the light model and calculate global bounds.
        if( !idRenderMatrix::Inverse( *(idRenderMatrix *)light->baseLightProject, *(idRenderMatrix *)light->inverseBaseLightProject ) )
        {
            idLib::Warning( "baseLightProject invert failed" );
        }
        //else (*(idRenderMatrix *)light->inverseBaseLightProject).Identity();

        // calculate the global light bounds by inverse projecting the zero to one cube with the 'inverseBaseLightProject'
        //idRenderMatrix::ProjectedBounds( light->globalLightBounds, light->inverseBaseLightProject, bounds_zeroOneCube, false );
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
    if(r_useShadowMapping.GetBool())
    {
        // setup render matrices for faster culling
        idRenderMatrix		projectionRenderMatrix;	// tech5 version of projectionMatrix
        idRenderMatrix::Transpose( *( idRenderMatrix* )tr.viewDef->projectionMatrix, /*tr.viewDef->*/projectionRenderMatrix );
        idRenderMatrix viewRenderMatrix;
        idRenderMatrix::Transpose( *( idRenderMatrix* )tr.viewDef->worldSpace.modelViewMatrix, viewRenderMatrix );
        idRenderMatrix::Multiply( /*tr.viewDef->*/projectionRenderMatrix, viewRenderMatrix, *( idRenderMatrix* )tr.viewDef->worldSpace.mvp );
    }
}
