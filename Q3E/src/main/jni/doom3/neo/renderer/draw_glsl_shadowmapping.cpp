/**
 * Shadow mapping in DIII4A from RBDOOM3-BFG
 * maybe has some wrong.
 *
 * OpenGLES2.0
 * FrameBuffer: color buffer, depth buffer(24)
 * glClearColor: 1.0 as max depth
 * glClear: color buffer | depth buffer
 * shadow depth value: color::RED component. So because of color buffer float precision limit, worse than depth texture with OpenGLES3.0,
 * point light: 2D cubemap color texture, 2D depth texture. shadow map depth stage: clip MVP(-1 - 1) -> fragment shader (z / w + 1.0) * 0.5; interaction stage: window space MVP(0 - 1) x 6.
 * parallel light: 2D color texture, 2D depth texture. NO CASCADE like RBDOOM3-BFG. shadow map depth stage: clip MVP(-1 - 1) -> fragment shader (z / w + 1.0) * 0.5; interaction stage: window space MVP(0 - 1).
 * spot light: 2D color texture, 2D depth texture. shadow map depth stage: window space MVP(0 - 1) -> fragment shader (z / w); interaction stage: window space MVP(0 - 1).
 *
 * OpenGLES3.0
 * FrameBuffer: depth buffer(24)
 * glClear: depth buffer
 * shadow depth value: depth texture, and setup compare mode, using texture2DShadow to sample
 * point light: 2D array depth texture, 6 layers: +X, -X, +Y, -Y, +Z, -Z, sequence same as cubemap. shadow map depth stage: clip MVP(-1 - 1); interaction stage: window space MVP(0 - 1) x 6.
 * parallel light: 2D array depth texture. because NO CASCADE, so only using 0 layer. shadow map depth stage: clip MVP(-1 - 1); interaction stage: window space MVP(0 - 1).
 * spot light: 2D array depth texture, only using 0 layer. shadow map depth stage: window space MVP(0 - 1); interaction stage: window space MVP(0 - 1).
 *
 * PCF:
 * 9 vec2/vec3 constants
 * 5 LODs: 2048, 1024, 512, 512, 256
 * 5 sample base factor: 1 / 2048, 1 / 1024, 1 / 512, 1 / 512, 1 / 256
 *
 * BIAS:
 * 0 - 0.001. I am not sure.
 *
 * Point light:
 * Most light type at scene in Most levels.
 * In OpenGLES2.0, cvar harm_r_shadowMapPointLight
 * 0. glsl macro _HARM_POINT_LIGHT_Z_AS_DEPTH: using window space z value as depth value[(gl_Position.z / gl_Position.w + 1.0) * 0.5](slowest and bad, but render same as OpenGLES3.0)
 * 1. glsl macro _HARM_POINT_LIGHT_FRUSTUM_FAR: using light position to vertex position distance divide frustum far value as depth value[(VertexPositionInLightSpace - LightGlobalPosition) / LightRadiusLengthAsFrustumFar](fastest and best, but depend frustum far)
 * 2. glsl macro _HARM_POINT_LIGHT_Z_AS_DEPTH: calculate z transform as depth value(faster and well)
 *
 * Parallel light:
 * Few light type. In out of room big scene, the lights most are point-light, but in lotaa/lotab/lotad of Prey are parallel light.
 *
 * Spot light:
 * Few light type. Most in ceiling lamp, e.g. in DOOM3, vent lights with rotating fan on ceiling; in Quake4 lights on ceiling(building_b start position).
 *
 * CVar:
 * r_useShadowMapping: bool, 0, enable shadow mapping
 * r_shadowMapFrustumFOV: float, 90, point light view FOV
 * harm_r_shadowMapLod: int, -1, force using shadow map LOD(0 - 4), -1 is auto
 * harm_r_shadowMapAlpha: float, 0.5, shadow alpha(0 - 1)
 * harm_r_shadowMapSampleFactor: float, -1.0, 0 multiple, 0 is disable, < 0 is auto, > 0 multi with fixed value
 * harm_r_shadowMapPointLight: int, 1, point light render method in OpenGLES2.0, 0 = using window space z value as depth value[(gl_Position.z / gl_Position.w + 1.0) * 0.5], 1 = using light position to vertex position distance divide frustum far value as depth value[(VertexPositionInLightSpace - LightGlobalPosition) / LightRadiusLengthAsFrustumFar], 2 = calculate z transform as depth value
 *
 */

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "tr_local.h"

#define POINT_LIGHT_FRUSTUM_FAR 7996.0f

enum pointLightRenderMethod_e
{
    POINT_LIGHT_RENDER_METHOD_Z_AS_DEPTH = 0,
    POINT_LIGHT_RENDER_METHOD_USING_FRUSTUM_FAR = 1,
    POINT_LIGHT_RENDER_METHOD_EMULATE_Z = 2,
};

static bool r_shadowMapping = false;
static int r_shadowMapPointLight = 0;

// see Framebuffer.h::shadowMapResolutions
static float SampleFactors[MAX_SHADOWMAP_RESOLUTIONS] = {1.0f / 2048.0, 1.0f / 1024.0, 1.0f / 512.0, 1.0f / 512.0, 1.0f / 256.0};

static idCVar SaveColorBuffer("SaveColorBuffer", "0", CVAR_BOOL, "");
static idCVar harm_r_shadowMapLightType("harm_r_shadowMapLightType", "0", CVAR_INTEGER|CVAR_RENDERER, "[Harmattan]: debug light type mask. 1: parallel, 2: point, 4: spot");
static idCVar harm_r_shadowMapDebug("harm_r_shadowMapDebug", "0", CVAR_INTEGER|CVAR_RENDERER, "[Harmattan]: debug shadow map frame buffer.");
char RB_ShadowMapPass_T = 'G'; // G - global shadow, L - local shadow

ID_INLINE void RB_SetMVP( const idRenderMatrix& mvp )
{
	GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mvp.m);
}

ID_INLINE void RB_ShadowMapping_polygonOffset(void)
{
    switch( r_shadowMapOccluderFacing.GetInteger() )
    {
        case 0:
            GL_Cull( CT_FRONT_SIDED );
            GL_PolygonOffset( true, r_shadowMapPolygonFactor.GetFloat(), r_shadowMapPolygonOffset.GetFloat() );
            break;

        case 1:
            GL_Cull( CT_BACK_SIDED );
            GL_PolygonOffset( true, -r_shadowMapPolygonFactor.GetFloat(), -r_shadowMapPolygonOffset.GetFloat() );
            break;

        default:
            GL_Cull( CT_TWO_SIDED );
            GL_PolygonOffset( true, r_shadowMapPolygonFactor.GetFloat(), r_shadowMapPolygonOffset.GetFloat() );
            break;
    }
}

ID_INLINE bool RB_ShadowMapping_filterLightType(void)
{
#define LIGHT_TYPE_MASK_PARALLEL 1
#define LIGHT_TYPE_MASK_POINT 2
#define LIGHT_TYPE_MASK_SPOT 4
    const viewLight_t* vLight = backEnd.vLight;
    int lt = harm_r_shadowMapLightType.GetInteger();
    if(lt > 0)
    {
        if(vLight->parallel)
        {
            if(!(lt & LIGHT_TYPE_MASK_PARALLEL))
            {
                return true;
            }
        }
        else if(vLight->pointLight)
        {
            if(!(lt & LIGHT_TYPE_MASK_POINT))
            {
                return true;
            }
        }
        else
        {
            if(!(lt & LIGHT_TYPE_MASK_SPOT))
            {
                return true;
            }
        }
    }
    return false;
}

ID_INLINE float RB_GetPointLightFrustumFar(const viewLight_t* vLight)
{
    float far;
    if(harm_r_shadowMapFrustumFar.GetFloat() == 0)
        far = vLight->lightRadius.Length();
    else if(harm_r_shadowMapFrustumFar.GetFloat() < 0)
        far = vLight->lightRadius.Length() * -harm_r_shadowMapFrustumFar.GetFloat();
    else
    {
        if(harm_r_shadowMapFrustumFar.GetFloat() <= harm_r_shadowMapFrustumNear.GetFloat())
            far = vLight->lightRadius.Length() * 2;
        else
            far = harm_r_shadowMapFrustumFar.GetFloat();
    }
    if(far <= 4)
        far = POINT_LIGHT_FRUSTUM_FAR;
    return far;
}

ID_INLINE shaderProgram_t * RB_SelectShadowMapShader(const viewLight_t* vLight, int side)
{
    shaderProgram_t *shadowShader;
    if( vLight->parallel && side >= 0 )
        shadowShader = &depthShader_parallelLight;
    else if( vLight->pointLight && side >= 0 )
    {
#ifdef GL_ES_VERSION_3_0
        if(USING_GLES3)
            shadowShader = &depthShader_pointLight;
        else
#endif
        {
            switch (r_shadowMapPointLight) {
                case POINT_LIGHT_RENDER_METHOD_Z_AS_DEPTH:
                    shadowShader = &depthShader_pointLight;
                    break;
                case POINT_LIGHT_RENDER_METHOD_USING_FRUSTUM_FAR:
                    shadowShader = &depthShader_pointLight_far;
                    break;
                case POINT_LIGHT_RENDER_METHOD_EMULATE_Z:
                    shadowShader = &depthShader_pointLight_z;
                    break;
                default:
                    shadowShader = &depthShader_pointLight;
                    break;
            }
        }
    }
    else
        shadowShader = &depthShader_spotLight;
    return shadowShader;
}

ID_INLINE shaderProgram_t * RB_SelectShadowMappingInteractionShader(const viewLight_t* vLight)
{
    shaderProgram_t *shadowInteractionShader;
    if(r_usePhong)
    {
        if( vLight->parallel )
        {
            shadowInteractionShader = &interactionShadowMappingShader_parallelLight;
        }
        else if( vLight->pointLight )
        {
#ifdef GL_ES_VERSION_3_0
            if(USING_GLES3)
                shadowInteractionShader = &interactionShadowMappingShader_pointLight;
            else
#endif
            {
                switch (r_shadowMapPointLight) {
                    case POINT_LIGHT_RENDER_METHOD_Z_AS_DEPTH:
                        shadowInteractionShader = &interactionShadowMappingShader_pointLight;
                        break;
                    case POINT_LIGHT_RENDER_METHOD_USING_FRUSTUM_FAR:
                        shadowInteractionShader = &interactionShadowMappingShader_pointLight_far;
                        break;
                    case POINT_LIGHT_RENDER_METHOD_EMULATE_Z:
                        shadowInteractionShader = &interactionShadowMappingShader_pointLight_z;
                        break;
                    default:
                        shadowInteractionShader = &interactionShadowMappingShader_pointLight;
                        break;
                }
            }
        }
        else
        {
            shadowInteractionShader = &interactionShadowMappingShader_spotLight;
        }
    }
    else
    {
        if( vLight->parallel )
        {
            shadowInteractionShader = &interactionShadowMappingBlinnPhongShader_parallelLight;
        }
        else if( vLight->pointLight )
        {
#ifdef GL_ES_VERSION_3_0
            if(USING_GLES3)
                shadowInteractionShader = &interactionShadowMappingBlinnPhongShader_pointLight;
            else
#endif
            {
                switch (r_shadowMapPointLight) {
                    case POINT_LIGHT_RENDER_METHOD_Z_AS_DEPTH:
                        shadowInteractionShader = &interactionShadowMappingBlinnPhongShader_pointLight;
                        break;
                    case POINT_LIGHT_RENDER_METHOD_USING_FRUSTUM_FAR:
                        shadowInteractionShader = &interactionShadowMappingBlinnPhongShader_pointLight_far;
                        break;
                    case POINT_LIGHT_RENDER_METHOD_EMULATE_Z:
                        shadowInteractionShader = &interactionShadowMappingBlinnPhongShader_pointLight_z;
                        break;
                    default:
                        shadowInteractionShader = &interactionShadowMappingBlinnPhongShader_pointLight;
                        break;
                }
            }
        }
        else
        {
            shadowInteractionShader = &interactionShadowMappingBlinnPhongShader_spotLight;
        }
    }
    return shadowInteractionShader;
}

void R_SaveColorBuffer(const char *name)
{
	Framebuffer *fb = backEnd.glState.currentFramebuffer;
	int width;
	int height;
	if(fb)
	{
		width = fb->width;
		height = fb->height;
	}
	else
	{
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
	}

	byte *data = (byte *)calloc(width * height * 4, 1);
/*    float *ddata = (float *)calloc(width * height, sizeof(float));
    qglReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, ddata);

    for (int i = 0 ; i < width * height ; i++) {
        data[i*4] =
        data[i*4+1] =
        data[i*4+2] = 255 * ddata[i];
        data[i*4+3] = 1;
	}*/

	//GL_CheckErrors("glReadPixels111");
	qglReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	//GL_CheckErrors("glReadPixels");

	R_WriteTGA(name, data, width, height, false);
	free(data);
	//free(ddata);
}

#if 1
static void RB_TestColorBuffer(const viewLight_t* vLight, int side)
{
    if(SaveColorBuffer.GetBool())
    {
        const char *lightTypeName;
        if( vLight->parallel )
            lightTypeName = "parallel";
        else if( vLight->pointLight )
            lightTypeName = "point";
        else
            lightTypeName = "spot";

        const char *sideName;
        switch (side) {
            case 0:
                sideName = "+X";
                break;
            case 1:
                sideName = "-X";
                break;
            case 2:
                sideName = "+Y";
                break;
            case 3:
                sideName = "-Y";
                break;
            case 4:
                sideName = "+Z";
                break;
            case 5:
                sideName = "-Z";
                break;
            default:
                sideName = "";
                break;
        }

        char fn[256];
        sprintf(fn, "sm/shadow_map_%p_SHADOWS(%c)_TYPE(%s)_SIDE(%d %s)_LOD(%d).tga", vLight, RB_ShadowMapPass_T, lightTypeName, side, sideName, vLight->shadowLOD);
        R_SaveColorBuffer(fn);
    }
}
#else
#define RB_TestColorBuffer(vLight, side)
#endif

static void R_PrintMatrix(int i, const float* arr)
{
    printf("%d ------>\n", i);
    for (int i = 0; i < 16; i++)
    {
        printf("%f   ", arr[i]);
        if (i % 4 == 3)
            printf("   |\n");
    }
    printf("\n");
}

/*
================
RB_DrawElementsWithCounters
================
*/
#define SM_SIL_EDGE 1
#define SM_REAR_CAP 2
#define SM_FRONT_CAP 4
static void RB_DrawShadowElementsWithCounters_shadowMapping(const srfTriangles_t *tri/*, int type = 0*/)
{
    if (!backEnd.glState.currentProgram) {
        common->Printf("RB_DrawShadowElementsWithCounters_shadowMapping: no current program object\n");
        __builtin_trap();
        return;
    }

    GLint start;
    GLsizei numIndexes;
	if(tri->numIndexes > tri->numShadowIndexesNoFrontCaps) // has front caps
	{
		start = tri->numShadowIndexesNoFrontCaps;
		numIndexes = tri->numIndexes - tri->numShadowIndexesNoFrontCaps;
	}
	else if(tri->numShadowIndexesNoFrontCaps > tri->numShadowIndexesNoCaps) // rear cap
	{
		start = tri->numShadowIndexesNoCaps;
		numIndexes = tri->numShadowIndexesNoFrontCaps - tri->numShadowIndexesNoCaps;
	}
	else // all
	{
        start = 0;
        numIndexes = tri->numIndexes;
	}
	/*
    if(type == SM_REAR_CAP | SM_SIL_EDGE)
    {
        start = 0;
        numIndexes = tri->numShadowIndexesNoFrontCaps;
    }
    else if(type == SM_FRONT_CAP)
    {
        if(tri->numIndexes == tri->numShadowIndexesNoCaps)
            return;
        if(tri->numIndexes > tri->numShadowIndexesNoFrontCaps) // has front cap
        {
            start = tri->numShadowIndexesNoFrontCaps;
            numIndexes = tri->numIndexes - tri->numShadowIndexesNoFrontCaps;
        }
        else // otherelse using rear cap, but need set w = 1.0 in vertex shader
        {
            start = tri->numShadowIndexesNoCaps;
            numIndexes = tri->numShadowIndexesNoFrontCaps - tri->numShadowIndexesNoCaps;
        }
    }
    else if(type == SM_SIL_EDGE)
    {
        start = 0;
        numIndexes = tri->numShadowIndexesNoCaps;
    }
    else if(type == SM_REAR_CAP)
    {
        start = tri->numShadowIndexesNoCaps;
        numIndexes = tri->numShadowIndexesNoFrontCaps - tri->numShadowIndexesNoCaps;
    }
    else
    {
        start = 0;
        numIndexes = tri->numIndexes;
    }
	*/

    //common->Printf("RB_DrawShadowElementsWithCounters_shadowMapping(%d %d %d)\n", tri->numShadowIndexesNoCaps, tri->numShadowIndexesNoFrontCaps, tri->numIndexes);

    backEnd.pc.c_drawElements++;
    backEnd.pc.c_drawIndexes += numIndexes;
    backEnd.pc.c_drawVertexes += tri->numVerts;

    if (tri->ambientSurface != NULL) {
        if (tri->indexes == tri->ambientSurface->indexes) {
            backEnd.pc.c_drawRefIndexes += numIndexes;
        }

        if (tri->verts == tri->ambientSurface->verts) {
            backEnd.pc.c_drawRefVertexes += tri->numVerts;
        }
    }

    if (tri->indexCache) {
        qglDrawElements(GL_TRIANGLES,
                       r_singleTriangle.GetBool() ? 3 : numIndexes,
                       GL_INDEX_TYPE,
                       (int *)vertexCache.Position(tri->indexCache) + start);
        backEnd.pc.c_vboIndexes += numIndexes;
    } else {
        vertexCache.UnbindIndex();

        qglDrawElements(GL_TRIANGLES,
                       r_singleTriangle.GetBool() ? 3 : numIndexes,
                       GL_INDEX_TYPE,
                       tri->indexes + start);
    }
}

/*
same as D3DXMatrixOrthoOffCenterRH

http://msdn.microsoft.com/en-us/library/bb205348(VS.85).aspx
*/
static void MatrixOrthogonalProjectionRH( float m[16], float left, float right, float bottom, float top, float zNear, float zFar )
{
    m[0] = 2 / ( right - left );
    m[4] = 0;
    m[8] = 0;
    m[12] = ( left + right ) / ( left - right );
    m[1] = 0;
    m[5] = 2 / ( top - bottom );
    m[9] = 0;
    m[13] = ( top + bottom ) / ( bottom - top );
    m[2] = 0;
    m[6] = 0;
    m[10] = 1 / ( zNear - zFar );
    m[14] = zNear / ( zNear - zFar );
    m[3] = 0;
    m[7] = 0;
    m[11] = 0;
    m[15] = 1;
}

// RB: moved this up because we need to call this several times for shadow mapping
static void RB_ResetViewportAndScissorToDefaultCamera( const viewDef_t* viewDef )
{
    // set the window clipping
    qglViewport(tr.viewportOffset[0] + backEnd.viewDef->viewport.x1,
               tr.viewportOffset[1] + backEnd.viewDef->viewport.y1,
               backEnd.viewDef->viewport.x2 + 1 - backEnd.viewDef->viewport.x1,
               backEnd.viewDef->viewport.y2 + 1 - backEnd.viewDef->viewport.y1);

    // the scissor may be smaller than the viewport for subviews
    qglScissor(tr.viewportOffset[0] + backEnd.viewDef->viewport.x1 + backEnd.viewDef->scissor.x1,
              tr.viewportOffset[1] + backEnd.viewDef->viewport.y1 + backEnd.viewDef->scissor.y1,
              backEnd.viewDef->scissor.x2 + 1 - backEnd.viewDef->scissor.x1,
              backEnd.viewDef->scissor.y2 + 1 - backEnd.viewDef->scissor.y1);
    backEnd.currentScissor = backEnd.viewDef->scissor;
}
// RB end

static void RB_CalcParallelLightMatrix(const viewLight_t* vLight, int side, idRenderMatrix &lightViewRenderMatrix, idRenderMatrix &lightProjectionRenderMatrix)
{
    assert( side >= 0 && side < 6 );
    assert( side == 0 ); // not cascade

    // original light direction is from surface to light origin
    idVec3 lightDir = -vLight->lightCenter;
    if( lightDir.Normalize() == 0.0f )
    {
        lightDir[2] = -1.0f;
    }

    idMat3 rotation = lightDir.ToMat3();
    //idAngles angles = lightDir.ToAngles();
    //idMat3 rotation = angles.ToMat3();

#if 1
    idRenderMatrix::CreateViewMatrix( backEnd.viewDef->renderView.vieworg, rotation, lightViewRenderMatrix );
#else
    const idVec3 viewDir = backEnd.viewDef->renderView.viewaxis[0];
        const idVec3 viewPos = backEnd.viewDef->renderView.vieworg;

        float lightViewMatrix[16];
		MatrixLookAtRH( lightViewMatrix, viewPos, lightDir, viewDir );
		idRenderMatrix::Transpose( *( idRenderMatrix* )lightViewMatrix, lightViewRenderMatrix );
#endif

    idBounds lightBounds;
    lightBounds.Clear();

    ALIGNTYPE16 frustumCorners_t corners;
    idRenderMatrix::GetFrustumCorners( corners, *(idRenderMatrix *)vLight->inverseBaseLightProject, bounds_zeroOneCube );

    idVec4 point, transf;
    for( int j = 0; j < 8; j++ )
    {
        point[0] = corners.x[j];
        point[1] = corners.y[j];
        point[2] = corners.z[j];
        point[3] = 1;

        lightViewRenderMatrix.TransformPoint( point, transf );
        transf[0] /= transf[3];
        transf[1] /= transf[3];
        transf[2] /= transf[3];

        lightBounds.AddPoint( transf.ToVec3() );
    }

    float lightProjectionMatrix[16];
    MatrixOrthogonalProjectionRH( lightProjectionMatrix, lightBounds[0][0], lightBounds[1][0], lightBounds[0][1], lightBounds[1][1], -lightBounds[1][2], -lightBounds[0][2] );
    idRenderMatrix::Transpose( *( idRenderMatrix* )lightProjectionMatrix, lightProjectionRenderMatrix );


    // 	'frustumMVP' goes from global space -> camera local space -> camera projective space
    // invert the MVP projection so we can deform zero-to-one cubes into the frustum pyramid shape and calculate global bounds

#if 0
    idRenderMatrix splitFrustumInverse;
        if( !idRenderMatrix::Inverse( backEnd.viewDef->frustumMVPs[FRUSTUM_CASCADE1 + side], splitFrustumInverse ) )
        {
            idLib::Warning( "splitFrustumMVP invert failed" );
        }

        // splitFrustumCorners in global space
        ALIGNTYPE16 frustumCorners_t splitFrustumCorners;
        idRenderMatrix::GetFrustumCorners( splitFrustumCorners, splitFrustumInverse, bounds_unitCube );


        idRenderMatrix lightViewProjectionRenderMatrix;
        idRenderMatrix::Multiply( lightProjectionRenderMatrix, lightViewRenderMatrix, lightViewProjectionRenderMatrix );

        // find the bounding box of the current split in the light's clip space
        idBounds cropBounds;
        cropBounds.Clear();
        for( int j = 0; j < 8; j++ )
        {
            point[0] = splitFrustumCorners.x[j];
            point[1] = splitFrustumCorners.y[j];
            point[2] = splitFrustumCorners.z[j];
            point[3] = 1;

            lightViewRenderMatrix.TransformPoint( point, transf );
            transf[0] /= transf[3];
            transf[1] /= transf[3];
            transf[2] /= transf[3];

            cropBounds.AddPoint( transf.ToVec3() );
        }

        // don't let the frustum AABB be bigger than the light AABB
        if( cropBounds[0][0] < lightBounds[0][0] )
        {
            cropBounds[0][0] = lightBounds[0][0];
        }

        if( cropBounds[0][1] < lightBounds[0][1] )
        {
            cropBounds[0][1] = lightBounds[0][1];
        }

        if( cropBounds[1][0] > lightBounds[1][0] )
        {
            cropBounds[1][0] = lightBounds[1][0];
        }

        if( cropBounds[1][1] > lightBounds[1][1] )
        {
            cropBounds[1][1] = lightBounds[1][1];
        }

        cropBounds[0][2] = lightBounds[0][2];
        cropBounds[1][2] = lightBounds[1][2];

        //float cropMatrix[16];
        //MatrixCrop(cropMatrix, cropBounds[0], cropBounds[1]);

        //idRenderMatrix cropRenderMatrix;
        //idRenderMatrix::Transpose( *( idRenderMatrix* )cropMatrix, cropRenderMatrix );

        //idRenderMatrix tmp = lightProjectionRenderMatrix;
        //idRenderMatrix::Multiply( cropRenderMatrix, tmp, lightProjectionRenderMatrix );

        MatrixOrthogonalProjectionRH( lightProjectionMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], -cropBounds[1][2], -cropBounds[0][2] );
        idRenderMatrix::Transpose( *( idRenderMatrix* )lightProjectionMatrix, lightProjectionRenderMatrix );

        backEnd.shadowV[side] = lightViewRenderMatrix;
        backEnd.shadowP[side] = lightProjectionRenderMatrix;
#endif
}

static void RB_CalcPointLightMatrix(const viewLight_t* vLight, int side, idRenderMatrix &lightViewRenderMatrix, idRenderMatrix &lightProjectionRenderMatrix)
{
    assert( side >= 0 && side < 6 );

    // FIXME OPTIMIZE no memset

    float	viewMatrix[16];

    idVec3	origin = vLight->globalLightOrigin;

    // side of a point light
    memset( viewMatrix, 0, sizeof( viewMatrix ) );
    switch( side )
    {
        case 0:
            viewMatrix[0] = 1;
            viewMatrix[9] = 1;
            viewMatrix[6] = -1;
            break;
        case 1:
            viewMatrix[0] = -1;
            viewMatrix[9] = -1;
            viewMatrix[6] = -1;
            break;
        case 2:
            viewMatrix[4] = 1;
            viewMatrix[1] = -1;
            viewMatrix[10] = 1;
            break;
        case 3:
            viewMatrix[4] = -1;
            viewMatrix[1] = -1;
            viewMatrix[10] = -1;
            break;
        case 4:
            viewMatrix[8] = 1;
            viewMatrix[1] = -1;
            viewMatrix[6] = -1;
            break;
        case 5:
            viewMatrix[8] = -1;
            viewMatrix[1] = 1;
            viewMatrix[6] = -1;
            break;
    }

    viewMatrix[12] = -origin[0] * viewMatrix[0] + -origin[1] * viewMatrix[4] + -origin[2] * viewMatrix[8];
    viewMatrix[13] = -origin[0] * viewMatrix[1] + -origin[1] * viewMatrix[5] + -origin[2] * viewMatrix[9];
    viewMatrix[14] = -origin[0] * viewMatrix[2] + -origin[1] * viewMatrix[6] + -origin[2] * viewMatrix[10];

    viewMatrix[3] = 0;
    viewMatrix[7] = 0;
    viewMatrix[11] = 0;
    viewMatrix[15] = 1;

    // from world space to light origin, looking down the X axis
    float	unflippedLightViewMatrix[16];

    // from world space to OpenGL view space, looking down the negative Z axis
    float	lightViewMatrix[16];

    static float	s_flipMatrix[16] =
            {
                    // convert from our coordinate system (looking down X)
                    // to OpenGL's coordinate system (looking down -Z)
                    0, 0, -1, 0,
                    -1, 0, 0, 0,
                    0, 1, 0, 0,
                    0, 0, 0, 1
            };

    memcpy( unflippedLightViewMatrix, viewMatrix, sizeof( unflippedLightViewMatrix ) );
    R_MatrixMultiply( viewMatrix, s_flipMatrix, lightViewMatrix );

    idRenderMatrix::Transpose( *( idRenderMatrix* )lightViewMatrix, lightViewRenderMatrix );




    // set up 90 degree projection matrix
    const float zNear = 4;
    const float	fov = r_shadowMapFrustumFOV.GetFloat();

    float ymax = zNear * tan( fov * idMath::PI / 360.0f );
    float ymin = -ymax;

    float xmax = zNear * tan( fov * idMath::PI / 360.0f );
    float xmin = -xmax;

    const float width = xmax - xmin;
    const float height = ymax - ymin;

    // from OpenGL view space to OpenGL NDC ( -1 : 1 in XYZ )
    float lightProjectionMatrix[16];

    lightProjectionMatrix[0 * 4 + 0] = 2.0f * zNear / width;
    lightProjectionMatrix[1 * 4 + 0] = 0.0f;
    lightProjectionMatrix[2 * 4 + 0] = ( xmax + xmin ) / width;	// normally 0
    lightProjectionMatrix[3 * 4 + 0] = 0.0f;

    lightProjectionMatrix[0 * 4 + 1] = 0.0f;
    lightProjectionMatrix[1 * 4 + 1] = 2.0f * zNear / height;
    lightProjectionMatrix[2 * 4 + 1] = ( ymax + ymin ) / height;	// normally 0
    lightProjectionMatrix[3 * 4 + 1] = 0.0f;

    // this is the far-plane-at-infinity formulation, and
    // crunches the Z range slightly so w=0 vertexes do not
    // rasterize right at the wraparound point
    lightProjectionMatrix[0 * 4 + 2] = 0.0f;
    lightProjectionMatrix[1 * 4 + 2] = 0.0f;
    lightProjectionMatrix[2 * 4 + 2] = -0.999f; // adjust value to prevent imprecision issues
    lightProjectionMatrix[3 * 4 + 2] = -2.0f * zNear;

    lightProjectionMatrix[0 * 4 + 3] = 0.0f;
    lightProjectionMatrix[1 * 4 + 3] = 0.0f;
    lightProjectionMatrix[2 * 4 + 3] = -1.0f;
    lightProjectionMatrix[3 * 4 + 3] = 0.0f;

    idRenderMatrix::Transpose( *( idRenderMatrix* )lightProjectionMatrix, lightProjectionRenderMatrix );
}

ID_INLINE static void RB_CalcSpotLightMatrix(const viewLight_t* vLight, idRenderMatrix &lightViewRenderMatrix, idRenderMatrix &lightProjectionRenderMatrix)
{
    lightViewRenderMatrix.Identity();
    lightProjectionRenderMatrix << vLight->baseLightProject;
}

/*
=====================
RB_ShadowMapPass
=====================
*/
void RB_ShadowMapPass( const drawSurf_t* drawSurfs, int side, bool clear )
{
    const viewLight_t* vLight = backEnd.vLight;

    RB_LogComment( "---------- RB_ShadowMapPass( side = %i ) ----------\n", side );

	if(!harm_r_shadowMapDebug.GetInteger())
	{
    globalFramebuffers.shadowFBO[vLight->shadowLOD]->Bind();
#ifdef GL_ES_VERSION_3_0
	if(USING_GLES3)
	{
#ifdef SHADOW_MAPPING_DEBUG
		globalFramebuffers.shadowFBO[vLight->shadowLOD]->AttachImage2D( globalImages->shadowImage[vLight->shadowLOD]); // for debug, not need render color buffer with OpenGLES3.0
#endif
		if( side < 0 )
			globalFramebuffers.shadowFBO[vLight->shadowLOD]->AttachImageDepthLayer( globalImages->shadowES3Image[vLight->shadowLOD], 0 );
		else
			globalFramebuffers.shadowFBO[vLight->shadowLOD]->AttachImageDepthLayer( globalImages->shadowES3Image[vLight->shadowLOD], side );
	}
	else
#endif
	{
        globalFramebuffers.shadowFBO[vLight->shadowLOD]->AttachImageDepth( globalImages->shadowDepthImage[vLight->shadowLOD]);

        if( vLight->parallel && side >= 0 )
            globalFramebuffers.shadowFBO[vLight->shadowLOD]->AttachImage2D( globalImages->shadowImage[vLight->shadowLOD]);
        else if( vLight->pointLight && side >= 0 )
            globalFramebuffers.shadowFBO[vLight->shadowLOD]->AttachImage2DLayer( globalImages->shadowCubeImage[vLight->shadowLOD], side );
        else
            globalFramebuffers.shadowFBO[vLight->shadowLOD]->AttachImage2D( globalImages->shadowImage[vLight->shadowLOD]);
	}

#if 0
    globalFramebuffers.shadowFBO[vLight->shadowLOD]->Check();
#endif
	}

    GL_ViewportAndScissor( 0, 0, shadowMapResolutions[vLight->shadowLOD], shadowMapResolutions[vLight->shadowLOD] );

	if(clear)
	{
		qglDepthMask(GL_TRUE); // depth buffer lock update yet
#ifdef GL_ES_VERSION_3_0
		if(USING_GLES3)
		{
#ifdef SHADOW_MAPPING_DEBUG
            qglClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			qglClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
#else
            qglClear( GL_DEPTH_BUFFER_BIT );
#endif
		}
		else
#endif
        {
#if 0
            if(vLight->parallel)
                qglClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            else if(vLight->pointLight)
            {
                if(r_shadowMapPointLight != POINT_LIGHT_RENDER_METHOD_EMULATE_Z)
                    qglClearColor(1.0f, 1.0f, 1.0f, 1.0f);
                else
                    qglClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            }
            else
                qglClearColor(1.0f, 1.0f, 1.0f, 1.0f);
#endif
			qglClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		    qglClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
	    }
		qglDepthMask(GL_FALSE);
	}

    if( drawSurfs == NULL
        || RB_ShadowMapping_filterLightType()
    )
	{
        Framebuffer::BindNull();
        RB_ResetViewportAndScissorToDefaultCamera(backEnd.viewDef);
        return;
	}

    // the actual stencil func will be set in the draw code, but we need to make sure it isn't
    // disabled here, and that the value will get reset for the interactions without looking
    // like a no-change-required
    idRenderMatrix lightProjectionRenderMatrix;
    idRenderMatrix lightViewRenderMatrix;

    if( vLight->parallel && side >= 0 )
    {
        RB_CalcParallelLightMatrix(vLight, side, lightViewRenderMatrix, lightProjectionRenderMatrix);

        backEnd.shadowV[0] << lightViewRenderMatrix;
        backEnd.shadowP[0] << lightProjectionRenderMatrix;
    }
    else if( vLight->pointLight && side >= 0 )
    {
        RB_CalcPointLightMatrix(vLight, side, lightViewRenderMatrix, lightProjectionRenderMatrix);

        backEnd.shadowV[side] << lightViewRenderMatrix;
        backEnd.shadowP[side] << lightProjectionRenderMatrix;
    }
    else
    {
        RB_CalcSpotLightMatrix(vLight, lightViewRenderMatrix, lightProjectionRenderMatrix);

        backEnd.shadowV[0] << lightViewRenderMatrix;
        backEnd.shadowP[0] << lightProjectionRenderMatrix;
    }

    // backup current OpenGL state
    uint64 glState = backEnd.glState.glStateBits;
    int faceCulling = backEnd.glState.faceCulling;

    // select shadow map shader
    shaderProgram_t *shadowShader = RB_SelectShadowMapShader(vLight, side);

    // setup OpenGL state
    GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO |
             /*GLS_DEPTHMASK | GLS_ALPHAMASK | GLS_GREENMASK | GLS_BLUEMASK |*/
             // GLS_REDMASK |
                     GLS_DEPTHFUNC_LESS); // TODO: in OpenGLES2.0, only write RED color buffer and depth buffer; in OpenGLES3.0, only write depth buffer
    qglDisable(GL_BLEND);
	qglDepthMask(GL_TRUE); // depth buffer lock update yet
    //qglDepthFunc(GL_LESS);

    // process the chain of shadows with the current rendering state
    backEnd.currentSpace = NULL;
    float mvp[16];
    float modelMatrix[16];
    idVec4 localLight;
    float globalLightOrigin[4] = {
            vLight->globalLightOrigin[0],
            vLight->globalLightOrigin[1],
            vLight->globalLightOrigin[2],
            1.0f,
    };

    for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != NULL; drawSurf = drawSurf->nextOnLight )
    {
        if( drawSurf->geo->numIndexes == 0 )
        {
            continue;	// a job may have created an empty shadow geometry
        }

        if( drawSurf->space != backEnd.currentSpace )
        {
            idRenderMatrix modelRenderMatrix;
            idRenderMatrix::Transpose( *( idRenderMatrix* )drawSurf->space->modelMatrix, modelRenderMatrix );

            idRenderMatrix modelToLightRenderMatrix;
            idRenderMatrix::Multiply( lightViewRenderMatrix, modelRenderMatrix, modelToLightRenderMatrix );

            idRenderMatrix clipMVP;
            idRenderMatrix::Multiply( lightProjectionRenderMatrix, modelToLightRenderMatrix, clipMVP );

#if 1
            clipMVP >> mvp;
#else
            if (vLight->parallel)
            {
                idRenderMatrix MVP;
                idRenderMatrix::Multiply(renderMatrix_clipSpaceToWindowSpace, clipMVP, MVP);
                MVP >> mvp;
            }
            else if (side < 0)
            {
                // from OpenGL view space to OpenGL NDC ( -1 : 1 in XYZ )
                idRenderMatrix MVP;
                idRenderMatrix::Multiply(renderMatrix_windowSpaceToClipSpace, clipMVP, MVP);
                MVP >> mvp;
            }
            else
            {
                clipMVP >> mvp;
            }
#endif

            (*(idRenderMatrix *)modelMatrix) << drawSurf->space->modelMatrix;

            // set the local light position to allow the vertex program to project the shadow volume end cap to infinity
			R_GlobalPointToLocal(drawSurf->space->modelMatrix, vLight->globalLightOrigin, localLight.ToVec3());
			localLight.w = 0.0f;

            backEnd.currentSpace = drawSurf->space;
        }

#if 0 //k TODO: no ambient cache in shadow surf
        bool didDraw = false;

        const idMaterial* shader = drawSurf->material;

        // get the expressions for conditionals / color / texcoords
        const float* regs = drawSurf->shaderRegisters;
        idVec4 color( 0, 0, 0, 1 );

        // set polygon offset if necessary
        if( shader && shader->TestMaterialFlag( MF_POLYGONOFFSET ) )
        {
            GL_PolygonOffset( true, r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
        }
        // else GL_PolygonOffset( false );

        if( shader && shader->Coverage() == MC_PERFORATED )
        {
            idDrawVert *ac = (idDrawVert *)vertexCache.Position(drawSurf->geo->ambientCache);
            // perforated surfaces may have multiple alpha tested stages
            for( int stage = 0; stage < shader->GetNumStages(); stage++ )
            {
                const shaderStage_t* pStage = shader->GetStage( stage );

                if( !pStage->hasAlphaTest ) {
                    continue;
                }

                // check the stage enable condition
                if( regs[ pStage->conditionRegister ] == 0 ) {
                    continue;
                }

                // if we at least tried to draw an alpha tested stage,
                // we won't draw the opaque surface
                didDraw = true;

                // set the alpha modulate
                color[3] = regs[ pStage->color.registers[3] ];

                // skip the entire stage if alpha would be black
                if( color[3] <= 0.0f ) {
                    continue;
                }

                // set privatePolygonOffset if necessary
                if( pStage->privatePolygonOffset )
                {
                    GL_PolygonOffset( true, r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset );
                }

                // renderProgManager.BindShader_TextureVertexColor();
                GL_UseProgram(&depthFillShader);

                idDrawVert *ac = (idDrawVert *)vertexCache.Position(drawSurf->geo->ambientCache);
                GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
                GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert), reinterpret_cast<void *>(&ac->st));

                RB_SetMVP(mvp);
                GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelMatrix), modelMatrix);

                GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
                GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));

                GL_Color( color );
                GL_Uniform1fv(offsetof(shaderProgram_t, alphaTest), &regs[pStage->alphaTestRegister]);

                // bind the texture
                GL_SelectTexture( 0 );
                pStage->texture.image->Bind();

                // set texture matrix and texGens
                RB_PrepareStageTexturing( pStage, drawSurf, ac );

                // must render with less-equal for Z-Cull to work properly
                assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

                // draw it
                RB_DrawElementsWithCounters( drawSurf->geo );

                // clean up
                RB_FinishStageTexturing( pStage, drawSurf, ac );

                // unset privatePolygonOffset if necessary
                if( pStage->privatePolygonOffset )
                {
                    GL_PolygonOffset( true, r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
                }

                GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
                GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
            }
        }

        if( !didDraw )
#endif
        {
            RB_ShadowMapping_polygonOffset();

            GL_UseProgram(shadowShader);

            GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));

            RB_SetMVP(mvp);
            GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelMatrix), modelMatrix);

            GL_Uniform4fv(offsetof(shaderProgram_t, globalLightOrigin), globalLightOrigin);
            GL_Uniform4fv(offsetof(shaderProgram_t, localLightOrigin), localLight.ToFloatPtr());

            GL_Uniform1f(offsetof(shaderProgram_t, u_uniformParm[1]), harm_r_shadowMapFrustumNear.GetFloat());
            GL_Uniform1f(offsetof(shaderProgram_t, u_uniformParm), RB_GetPointLightFrustumFar(vLight));

            GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 4, GL_FLOAT, false, sizeof(shadowCache_t), vertexCache.Position(drawSurf->geo->shadowCache));

#if 0
			float w;
#ifdef GL_ES_VERSION_3_0
            if(USING_GLES3)
                w = 1.0;
            else
#endif
            {
                if(vLight->parallel)
                    w = 1.0;
                else if(vLight->pointLight)
                {
                    if(r_shadowMapPointLight != POINT_LIGHT_RENDER_METHOD_EMULATE_Z)
                        w = 1.0;
                    else
                        w = 0.0;
                }
                else
                    w = 1.0;
            }
			GL_Uniform1f(offsetof(shaderProgram_t, u_uniformParm[2]), w);
#endif
            RB_DrawShadowElementsWithCounters_shadowMapping( drawSurf->geo/*, SM_REAR_CAP*/ );
            //RB_DrawElementsWithCounters( drawSurf->geo );

            // cleanup the shadow specific rendering state
            GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
        }
    }
    backEnd.currentSpace = NULL;

    // if debug buffer
    RB_TestColorBuffer(vLight, side);

	// restore GL state
    GL_UseProgram(NULL);
    Framebuffer::BindNull();

    //qglDepthFunc(GL_LEQUAL);
    GL_State( glState );
    GL_PolygonOffset( false );
    GL_Cull( faceCulling );
    qglEnable(GL_BLEND);
	qglDepthMask(GL_FALSE); // lock depth buffer

    RB_ResetViewportAndScissorToDefaultCamera(backEnd.viewDef);
}

/*
=============
RB_GLSL_CreateDrawInteractions

=============
*/
void RB_GLSL_CreateDrawInteractions_shadowMapping(const drawSurf_t *surf)
{
    if (!surf) {
        return;
    }

    // bind the vertex and fragment shader
    shaderProgram_t *shadowInteractionShader = RB_SelectShadowMappingInteractionShader(backEnd.vLight);
    GL_UseProgram(shadowInteractionShader);

    GL_Uniform1f(offsetof(shaderProgram_t, u_uniformParm[1]), harm_r_shadowMapFrustumNear.GetFloat());
    GL_Uniform1f(offsetof(shaderProgram_t, u_uniformParm), RB_GetPointLightFrustumFar(backEnd.vLight));

#ifdef SHADOW_MAPPING_DEBUG
    GL_Uniform1f(offsetof(shaderProgram_t, u_uniformParm[4]), harm_r_shadowMapBias.GetFloat());
#endif

            // perform setup here that will be constant for all interactions
    GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE |
             GLS_DEPTHMASK | //k: fix translucent interactions
             backEnd.depthFunc);

    // enable the vertex arrays
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Tangent));
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Bitangent));
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));	// gl_Vertex
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));	// gl_Color

    //GL_Uniform1f(offsetof(shaderProgram_t, bias), harm_r_shadowMapBias.GetFloat());
    GL_Uniform1f(offsetof(shaderProgram_t, u_uniformParm[3]), harm_r_shadowMapAlpha.GetFloat());
    float globalLightOrigin[4] = {
            backEnd.vLight->globalLightOrigin[0],
            backEnd.vLight->globalLightOrigin[1],
            backEnd.vLight->globalLightOrigin[2],
            1.0f,
    };
    GL_Uniform4fv(offsetof(shaderProgram_t, globalLightOrigin), globalLightOrigin);

    float sampleScale = 1.0;
    float sampleFactor = harm_r_shadowMapSampleFactor.GetFloat();
    float lod = sampleFactor != 0 ? SampleFactors[backEnd.vLight->shadowLOD] : 0.0;

#ifdef GL_ES_VERSION_3_0
    if(USING_GLES3)
    {
        if( backEnd.vLight->parallel )
        {
            sampleScale = 1.0;
        }
        else if( backEnd.vLight->pointLight )
        {
            sampleScale = 2.0;
        }
        else
        {
            sampleScale = 2.0;
        }
    }
    else
#endif
    {
    if( backEnd.vLight->parallel )
    {
        sampleScale = 1.0;
    }
    else if( backEnd.vLight->pointLight )
    {
        if(sampleFactor != 0)
            lod = 0.5;
    }
    else
    {
        sampleScale = 5.0;
    }
    }

    if(sampleFactor > 0.0)
        sampleScale *= sampleFactor;

    GL_Uniform1f(offsetof(shaderProgram_t, u_uniformParm[2]), lod * sampleScale);

    // texture 5 is the specular lookup table
    GL_SelectTextureNoClient(5);
    globalImages->specularTableImage->Bind();

    backEnd.currentSpace = NULL; //k2023
    for (; surf ; surf=surf->nextOnLight) {
        // perform setup here that will not change over multiple interaction passes

        // set the modelview matrix for the viewer
        /*float   mat[16];
        myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
        GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mat);*/ //k2023

        // set the vertex pointers
        idDrawVert	*ac = (idDrawVert *)vertexCache.Position(surf->geo->ambientCache);

        GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Normal), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->normal.ToFloatPtr());
        GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Bitangent), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[1].ToFloatPtr());
        GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Tangent), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[0].ToFloatPtr());
        GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert), ac->st.ToFloatPtr());

        GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
        GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Color), 4, GL_UNSIGNED_BYTE, false, sizeof(idDrawVert), ac->color);


        GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelMatrix), surf->space->modelMatrix);

        // this may cause RB_GLSL_DrawInteraction to be exacuted multiple
        // times with different colors and images if the surface or light have multiple layers
        RB_CreateSingleDrawInteractions(surf, RB_GLSL_DrawInteraction);
    }

    backEnd.currentSpace = NULL; //k2023

    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Tangent));
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Bitangent));
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));	// gl_Vertex
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));	// gl_Color

    // disable features
    GL_SelectTextureNoClient(6);
    globalImages->BindNull();

    GL_SelectTextureNoClient(5);
    globalImages->BindNull();

    GL_SelectTextureNoClient(4);
    globalImages->BindNull();

    GL_SelectTextureNoClient(3);
    globalImages->BindNull();

    GL_SelectTextureNoClient(2);
    globalImages->BindNull();

    GL_SelectTextureNoClient(1);
    globalImages->BindNull();

    backEnd.glState.currenttmu = -1;
    GL_SelectTexture(0);

    GL_UseProgram(NULL);
}
