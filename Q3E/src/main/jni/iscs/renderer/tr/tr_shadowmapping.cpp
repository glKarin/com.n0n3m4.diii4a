
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../tr_local.h"

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
        idRenderMatrix::Multiply(ID_RENDER_MATRIX tr.viewDef->worldSpace.mvp,
                                 ID_RENDER_MATRIX light->inverseBaseLightProject,
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
        idRenderMatrix::Multiply( localProject, inverseLightMatrix, ID_RENDER_MATRIX light->baseLightProject );

        // Invert the light projection so we can deform zero-to-one cubes into
        // the light model and calculate global bounds.
        if( !idRenderMatrix::Inverse( ID_RENDER_MATRIX light->baseLightProject, ID_RENDER_MATRIX light->inverseBaseLightProject ) )
        {
            idLib::Warning( "baseLightProject invert failed" );
        }
        //else (ID_RENDER_MATRIX light->inverseBaseLightProject).Identity();

        // calculate the global light bounds by inverse projecting the zero to one cube with the 'inverseBaseLightProject'
        //idRenderMatrix::ProjectedBounds( light->globalLightBounds, light->inverseBaseLightProject, bounds_zeroOneCube, false );
    }
}

