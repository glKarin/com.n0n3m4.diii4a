idCVar harm_r_globalIllumination("harm_r_globalIllumination", "0", CVAR_BOOL | CVAR_RENDERER | CVAR_ARCHIVE, "Render global illumination before draw interactions");
idCVar harm_r_globalIlluminationBrightness("harm_r_globalIlluminationBrightness", "0.5", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "Global illumination brightness");

extern void RB_CreateSingleDrawGlobalIllumination(const drawSurf_t *drawSurf, void (*DrawInteraction)(const drawInteraction_t *));

static void RB_GLSL_DrawGlobalIllumination(const drawInteraction_t *din)
{
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

    // draw it
    RB_DrawElementsWithCounters(din->surf->geo);
}

static void RB_GLSL_CreateDrawGlobalIllumination(const drawSurf_t *surf)
{
    if (!surf || !surf->geo->ambientCache) {
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
    RB_CreateSingleDrawGlobalIllumination(surf, RB_GLSL_DrawGlobalIllumination);

    // disable features
    GL_SelectTextureNoClient(2);
    globalImages->BindNull();

    GL_SelectTextureNoClient(1);
    globalImages->BindNull();

    backEnd.glState.currenttmu = -1;
    GL_SelectTexture(0);
}

void RB_DrawGlobalIlluminations( drawSurf_t **drawSurfs, int numDrawSurfs )
{
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

    RB_LogComment("---------- RB_DrawGlobalIlluminations ----------\n");

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
    //GL_State( GLS_DEFAULT );

//#define BLEND_NORMALS 1

    // RB: even use additive blending to blend the normals
    //GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );

    GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );

    //GL_Color( colorWhite );

#if 0
    idVec4 diffuseColor;
    idVec4 specularColor;
    idVec4 ambientColor;

    if( viewDef->renderView.rdflags & RDF_IRRADIANCE )
    {
        // RB: don't let artist run into a trap when baking multibounce lightgrids

        // use default value of r_lightScale 3
        const float lightScale = 3;
        const idVec4 lightColor = colorWhite * lightScale;

        // apply the world-global overbright and the 2x factor for specular
        diffuseColor = lightColor;
        specularColor = lightColor;// * 2.0f;

        // loose 5% with every bounce like in DDGI
        const float energyConservation = 0.95f;

        //ambientColor.Set( energyConservation, energyConservation, energyConservation, 1.0f );
        float a = r_forceAmbient.GetFloat();

        ambientColor.Set( a, a, a, 1 );
    }
    else
    {
        const float lightScale = r_lightScale.GetFloat();
        const idVec4 lightColor = colorWhite * lightScale;

        // apply the world-global overbright and tune down specular a bit so we have less fresnel overglow
        diffuseColor = lightColor;
        specularColor = lightColor;// * 0.5f;

        float ambientBoost = 1.0f;
        if( !r_usePBR.GetBool() )
        {
            ambientBoost += r_useSSAO.GetBool() ? 0.2f : 0.0f;
            ambientBoost *= r_useHDR.GetBool() ? 1.1f : 1.0f;
        }

        ambientColor.x = r_forceAmbient.GetFloat() * ambientBoost;
        ambientColor.y = r_forceAmbient.GetFloat() * ambientBoost;
        ambientColor.z = r_forceAmbient.GetFloat() * ambientBoost;
        ambientColor.w = 1;
    }

    renderProgManager.SetRenderParm( RENDERPARM_AMBIENT_COLOR, ambientColor.ToFloatPtr() );
#endif

    GL_Uniform1f(offsetof(shaderProgram_t, specularExponent), harm_r_globalIlluminationBrightness.GetFloat());

    for( int i = 0; i < numDrawSurfs; i++ )
    {
        RB_GLSL_CreateDrawGlobalIllumination(drawSurfs[i]);
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
    //SetFragmentParm( RENDERPARM_ALPHA_TEST, vec4_zero.ToFloatPtr() );

    //k GL_SelectTexture( 0 );
}
