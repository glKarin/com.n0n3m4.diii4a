#include "idlib/precompiled.h"

#include "renderer/tr_local.h"

#include "RenderProgram.h"
#include "RenderProgramManager.h"

idCVar harm_r_skipAreaAmbient("harm_r_skipAreaAmbient", "0", CVAR_BOOL | CVAR_RENDERER, "skip areas ambient rendering before draw interactions");

static idCVar harm_r_builtinAreaAmbient("harm_r_builtinAreaAmbient", "0", CVAR_BOOL | CVAR_RENDERER, "using built-in global illumination for area ambient rendering");

idCVar harm_r_areaAmbientScale("harm_r_areaAmbientScale", "1.0", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "area ambient scale");
idCVar harm_r_areaAmbientAlpha("harm_r_areaAmbientAlpha", "1.0", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "area ambient alpha");

extern void RB_CreateSingleDrawAreaAmbient_external(const drawSurf_t *drawSurf, void (*DrawInteraction)(const drawInteraction_t *));
extern void RB_CreateSingleDrawAreaAmbient_builtin(const drawSurf_t *drawSurf, void (*DrawInteraction)(const drawInteraction_t *));

static const sdRenderProgram *ambientBasicShader;

static void RB_DrawAreaAmbient_builtin(const drawInteraction_t *din)
{
    GL_Uniform1f(offsetof(shaderProgram_t, specularExponent), din->surf->space->areaAmbient->GetBrightness() * harm_r_areaAmbientScale.GetFloat());

    // load all the vertex program parameters
    GL_Uniform4fv(offsetof(shaderProgram_t, localViewOrigin), din->localViewOrigin.ToFloatPtr());
    GL_Uniform4fv(offsetof(shaderProgram_t, bumpMatrixS), din->bumpMatrix[0].ToFloatPtr());
    GL_Uniform4fv(offsetof(shaderProgram_t, bumpMatrixT), din->bumpMatrix[1].ToFloatPtr());
    GL_Uniform4fv(offsetof(shaderProgram_t, diffuseMatrixS), din->diffuseMatrix[0].ToFloatPtr());
    GL_Uniform4fv(offsetof(shaderProgram_t, diffuseMatrixT), din->diffuseMatrix[1].ToFloatPtr());
    GL_Uniform4fv(offsetof(shaderProgram_t, specularMatrixS), din->specularMatrix[0].ToFloatPtr());
    GL_Uniform4fv(offsetof(shaderProgram_t, specularMatrixT), din->specularMatrix[1].ToFloatPtr());

    switch (din->vertexColor) {
        case SVC_MODULATE:
            GL_Uniform1fv(offsetof(shaderProgram_t, colorModulate), oneModulate);
            GL_Uniform1fv(offsetof(shaderProgram_t, colorAdd), zero);
            break;
        case SVC_INVERSE_MODULATE:
            GL_Uniform1fv(offsetof(shaderProgram_t, colorModulate), negOneModulate);
            GL_Uniform1fv(offsetof(shaderProgram_t, colorAdd), one);
            break;
        case SVC_IGNORE:
        default:
            GL_Uniform1fv(offsetof(shaderProgram_t, colorModulate), zero);
            GL_Uniform1fv(offsetof(shaderProgram_t, colorAdd), one);
            break;
    }

    // set the constant colors
    GL_Uniform4fv(offsetof(shaderProgram_t, diffuseColor), din->diffuseColor.ToFloatPtr());
    GL_Uniform4fv(offsetof(shaderProgram_t, specularColor), din->specularColor.ToFloatPtr());

    // set the textures

    // texture 0 will be the per-surface bump map
    GL_SelectTextureNoClient(0);
    din->bumpImage->Bind();

    // texture 1 is the per-surface diffuse map
    GL_SelectTextureNoClient(1);
    din->diffuseImage->Bind();

    // texture 2 is the per-surface specular map
    GL_SelectTextureNoClient(2);
    din->specularImage->Bind();

    GL_SelectTextureNoClient(0); //k2023
#ifdef INTERACTION_ALPHA_TEST
	GL_Uniform1f(offsetof(shaderProgram_t, alphaTest), din->alphaTest);
#endif

    // draw it
    RB_DrawElementsWithCounters(din->surf->geo);
}

static void RB_DrawAreaAmbient_external(const drawInteraction_t *din)
{
    // load all the vertex program parameters

    const sdDeclAmbientCubeMap *areaAmbient = din->surf->space->areaAmbient;
    ambientBasicShader->BindVector("ambientBrightness", areaAmbient->GetBrightness());
    ambientBasicShader->BindVector("ambientScale", harm_r_areaAmbientScale.GetFloat(), harm_r_areaAmbientAlpha.GetFloat());
    ambientBasicShader->BindVector("diffuseMatrix_s", din->diffuseMatrix[0]);
    ambientBasicShader->BindVector("diffuseMatrix_t", din->diffuseMatrix[1]);
    ambientBasicShader->BindVector("bumpMatrix_s", din->bumpMatrix[0]);
    ambientBasicShader->BindVector("bumpMatrix_t", din->bumpMatrix[1]);
    ambientBasicShader->BindVector("viewOriginWorld", backEnd.viewDef->renderView.vieworg);
    ambientBasicShader->BindVector("alphaThresh", din->alphaTest);
    ambientBasicShader->BindVector("viewOrigin", din->localViewOrigin); // parallax

    idMat4 modelMatrix;
    memcpy(&modelMatrix, din->surf->space->modelMatrix, sizeof(modelMatrix));
    modelMatrix.TransposeSelf();
    ambientBasicShader->BindVector("transposedModelMatrix_x", modelMatrix[0]);
    ambientBasicShader->BindVector("transposedModelMatrix_y", modelMatrix[1]);
    ambientBasicShader->BindVector("transposedModelMatrix_z", modelMatrix[2]);

    switch (din->vertexColor) {
        case SVC_MODULATE:
#ifdef NORMALIZE_BYTE_COLOR
            ambientBasicShader->BindVector("colorModulate", one[0]);
#else
            ambientBasicShader->BindVector("colorModulate", oneModulate[0]);
#endif
            ambientBasicShader->BindVector("colorAdd", zero[0]);
            break;
        case SVC_INVERSE_MODULATE:
#ifdef NORMALIZE_BYTE_COLOR
            ambientBasicShader->BindVector("colorModulate", negOne[0]);
#else
            ambientBasicShader->BindVector("colorModulate", negOneModulate[0]);
#endif
            ambientBasicShader->BindVector("colorAdd", one[0]);
            break;
        case SVC_IGNORE:
        default:
            ambientBasicShader->BindVector("colorModulate", zero[0]);
            ambientBasicShader->BindVector("colorAdd", one[0]);
            break;
    }

    // set the constant colors
    ambientBasicShader->BindVector("diffuseColor", din->diffuseColor);
    ambientBasicShader->BindVector("specularColor", din->specularColor);

    // set the textures

    // texture 0 will be the per-surface bump map
    ambientBasicShader->BindImage("bumpMap", din->bumpImage);

    // texture 1 is the per-surface diffuse map
    ambientBasicShader->BindImage("diffuseMap", din->diffuseImage);

    // texture 2 is the per-surface specular map
    ambientBasicShader->BindImage("specularMap", din->specularImage);

    // texture 3 is the cube map
    idImage *ambientCubeMap = NULL;
    ambientCubeMap = din->surf->space->areaAmbient->GetAmbientCubeMap();
    if(!ambientCubeMap)
        ambientCubeMap = globalImages->blackCubeMapImage;
    ambientBasicShader->BindImage("ambientCubeMap", ambientCubeMap);

    // texture 4 is the specular cube map
    idImage *specularCubeMap = NULL;
    specularCubeMap = din->surf->space->areaAmbient->GetSpecularCubeMap();
    if (!specularCubeMap)
        specularCubeMap = globalImages->blackCubeMapImage;
    ambientBasicShader->BindImage("specularCubeMap", specularCubeMap);

    GL_SelectTextureNoClient(0); //k2023

    // draw it
    RB_DrawElementsWithCounters(din->surf->geo);
}

static void RB_CreateDrawAreaAmbient_external(const drawSurf_t *surf)
{
    if (!surf || !surf->geo->ambientCache) {
        return;
    }
    if(!surf->space->areaAmbient)
        return;
    if (surf->material->TestMaterialFlag(MF_NOAMBIENT)) {
        return;
    }

    // set the vertex pointers
    idDrawVert	*ac = (idDrawVert *)vertexCache.Position(surf->geo->ambientCache);

    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Normal), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->normal.ToFloatPtr());
    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Bitangent), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[1].ToFloatPtr());
    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Tangent), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[0].ToFloatPtr());
    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert), ac->st.ToFloatPtr());

    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
#ifdef NORMALIZE_BYTE_COLOR
    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Color), 4, GL_UNSIGNED_BYTE, true, sizeof(idDrawVert), ac->color);
#else
    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Color), 4, GL_UNSIGNED_BYTE, false, sizeof(idDrawVert), ac->color);
#endif

    // this may cause RB_GLSL_DrawInteraction to be exacuted multiple
    // times with different colors and images if the surface or light have multiple layers
    RB_CreateSingleDrawAreaAmbient_external(surf, RB_DrawAreaAmbient_external);

    // disable features
    ambientBasicShader->BindImage("bumpMap", NULL);

    ambientBasicShader->BindImage("diffuseMap", NULL);

    ambientBasicShader->BindImage("specularMap", NULL);

    ambientBasicShader->BindImage("ambientCubeMap", NULL);

    ambientBasicShader->BindImage("specularCubeMap", NULL);

    backEnd.glState.currenttmu = -1;
    GL_SelectTexture(0);
}

static void RB_CreateDrawAreaAmbient_builtin(const drawSurf_t *surf)
{
    if (!surf || !surf->geo->ambientCache) {
        return;
    }
	if(!surf->space->areaAmbient)
		return;
    if (surf->material->TestMaterialFlag(MF_NOAMBIENT)) {
        return;
    }

    // set the vertex pointers
    idDrawVert	*ac = (idDrawVert *)vertexCache.Position(surf->geo->ambientCache);

    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Normal), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->normal.ToFloatPtr());
    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Bitangent), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[1].ToFloatPtr());
    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Tangent), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[0].ToFloatPtr());
    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert), ac->st.ToFloatPtr());

    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Color), 4, GL_UNSIGNED_BYTE, false, sizeof(idDrawVert), ac->color);

    // this may cause RB_GLSL_DrawInteraction to be exacuted multiple
    // times with different colors and images if the surface or light have multiple layers
    RB_CreateSingleDrawAreaAmbient_builtin(surf, RB_DrawAreaAmbient_builtin);

    // disable features
    GL_SelectTextureNoClient(2);
    globalImages->BindNull();

    GL_SelectTextureNoClient(1);
    globalImages->BindNull();

    backEnd.glState.currenttmu = -1;
    GL_SelectTexture(0);
}

static void RB_DrawAreaAmbients_builtin( drawSurf_t **drawSurfs, int numDrawSurfs )
{
    RB_LogComment("---------- RB_DrawAreaAmbients_builtin ----------\n");

    // bind the vertex and fragment shader
    GL_UseProgram(&globalIlluminationShader);

    // enable the vertex arrays
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Tangent));
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Bitangent));
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));	// gl_Vertex
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));	// gl_Color

    // force MVP change on first surface
    backEnd.currentSpace = NULL;

    int glStateBits = backEnd.glState.glStateBits;
    // draw all the subview surfaces, which will already be at the start of the sorted list,
    // with the general purpose path
    //GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );

    GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );

    for( int i = 0; i < numDrawSurfs; i++ )
    {
        RB_CreateDrawAreaAmbient_builtin(drawSurfs[i]);
    }

    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Tangent));
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Bitangent));
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));	// gl_Vertex
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));	// gl_Color

    GL_UseProgram(NULL);

    backEnd.currentSpace = NULL;

    // disable blending
    GL_State( glStateBits );

    //k GL_SelectTexture( 0 );
}

static void RB_DrawAreaAmbients_external( drawSurf_t **drawSurfs, int numDrawSurfs )
{
    ambientBasicShader = renderProgramManager->LoadProgram("ambient/basic");
    if( !ambientBasicShader || !ambientBasicShader->Bind() )
    {
        RB_DrawAreaAmbients_builtin(drawSurfs, numDrawSurfs);
        return;
    }

    RB_LogComment("---------- RB_DrawAreaAmbients_external ----------\n");

    // bind the vertex and fragment shader

    // enable the vertex arrays
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Tangent));
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Bitangent));
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));	// gl_Vertex
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));	// gl_Color

    // force MVP change on first surface
    backEnd.currentSpace = NULL;

    int glStateBits = backEnd.glState.glStateBits;
    // draw all the subview surfaces, which will already be at the start of the sorted list,
    // with the general purpose path
    //GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );

    GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );

    for( int i = 0; i < numDrawSurfs; i++ )
    {
        RB_CreateDrawAreaAmbient_external(drawSurfs[i]);
    }

    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Tangent));
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Bitangent));
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));	// gl_Vertex
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));	// gl_Color

    ambientBasicShader->Unbind();

    backEnd.currentSpace = NULL;

    // disable blending
    GL_State( glStateBits );

    //k GL_SelectTexture( 0 );
}

void RB_DrawAreaAmbient( drawSurf_t **drawSurfs, int numDrawSurfs )
{
    if(harm_r_skipAreaAmbient.GetBool())
        return;

    if( numDrawSurfs == 0 )
    {
        return;
    }

    if( !drawSurfs )
    {
        return;
    }

    // if we are just doing 2D rendering, no need to fill the depth buffer
    if( backEnd.viewDef->viewEntitys == NULL )
    {
        return;
    }

	backEnd.vLight = NULL;
    if (harm_r_builtinAreaAmbient.GetBool())
        RB_DrawAreaAmbients_builtin(drawSurfs, numDrawSurfs);
    else
        RB_DrawAreaAmbients_external(drawSurfs, numDrawSurfs);
}
