
/*
====================
GL_UseProgram
====================
*/
void GL_UseProgram(shaderProgram_t *program)
{
    if (backEnd.glState.currentProgram == program) {
        return;
    }

#ifdef _HARM_SHADER_NAME
    RB_LogComment("Last shader program: %s, %p\n", backEnd.glState.currentProgram ? backEnd.glState.currentProgram->name : "NULL", backEnd.glState.currentProgram);
	RB_LogComment("Current shader program: %s, %p\n", program ? program->name : "NULL", program);
#endif
    qglUseProgram(program ? program->program : 0);
    backEnd.glState.currentProgram = program;

    HARM_CHECK_SHADER_ERROR();
}

/*
====================
GL_Uniform1fv
====================
*/
void GL_Uniform1fv(GLint location, const GLfloat *value)
{
    HARM_CHECK_SHADER("GL_Uniform1fv");

    qglUniform1fv(*(GLint *)((char *)backEnd.glState.currentProgram + location), 1, value);

    HARM_CHECK_SHADER_ERROR();
}

/*
====================
GL_Uniform4fv
====================
*/
void GL_Uniform4fv(GLint location, const GLfloat *value)
{
    HARM_CHECK_SHADER("GL_Uniform4fv");

    qglUniform4fv(*(GLint *)((char *)backEnd.glState.currentProgram + location), 1, value);

    HARM_CHECK_SHADER_ERROR();
}

/*
====================
GL_Uniform4f
====================
*/
void GL_Uniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    HARM_CHECK_SHADER("GL_Uniform4f");

    qglUniform4f(*(GLint *)((char *)backEnd.glState.currentProgram + location), x, y, z, w);

    HARM_CHECK_SHADER_ERROR();
}

/*
====================
GL_Uniform4f
====================
*/
void GL_Uniform1i(GLint location, GLint w)
{
    HARM_CHECK_SHADER("GL_Uniform1i");

    qglUniform1i(*(GLint *)((char *)backEnd.glState.currentProgram + location), w);

    HARM_CHECK_SHADER_ERROR();
}

/*
====================
GL_Uniform3fv
====================
*/
void GL_Uniform3fv(GLint location, const GLfloat *value)
{
    HARM_CHECK_SHADER("GL_Uniform3fv");

    qglUniform3fv(*(GLint *)((char *)backEnd.glState.currentProgram + location), 1, value);

    HARM_CHECK_SHADER_ERROR();
}

/*
====================
GL_UniformMatrix4fv
====================
*/
void GL_UniformMatrix4fv(GLint location, const GLfloat *value)
{
    HARM_CHECK_SHADER("GL_UniformMatrix4fv");

    qglUniformMatrix4fv(*(GLint *)((char *)backEnd.glState.currentProgram + location), 1, GL_FALSE, value);

    HARM_CHECK_SHADER_ERROR();
}

/*
====================
GL_Uniform1f
====================
*/
void GL_Uniform1f(GLint location, GLfloat value)
{
    HARM_CHECK_SHADER("GL_Uniform1f");

    qglUniform1f(*(GLint *)((char *)backEnd.glState.currentProgram + location), value);

    HARM_CHECK_SHADER_ERROR();
}

/*
====================
GL_UniformMatrix4fv
====================
*/
void GL_UniformMatrix4fv(GLint location, const GLsizei n, const GLfloat *value)
{
    HARM_CHECK_SHADER("GL_UniformMatrix4fv");

    qglUniformMatrix4fv(*(GLint *)((char *)backEnd.glState.currentProgram + location), n, GL_FALSE, value);

    HARM_CHECK_SHADER_ERROR();
}

/*
====================
GL_EnableVertexAttribArray
====================
*/
void GL_EnableVertexAttribArray(GLuint index)
{
    HARM_CHECK_SHADER("GL_EnableVertexAttribArray");

    HARM_CHECK_SHADER_ATTR("GL_EnableVertexAttribArray", index);

    //RB_LogComment("qglEnableVertexAttribArray( %i );\n", index);
    qglEnableVertexAttribArray(*(GLint *)((char *)backEnd.glState.currentProgram + index));

    HARM_CHECK_SHADER_ERROR();
}

/*
====================
GL_DisableVertexAttribArray
====================
*/
void GL_DisableVertexAttribArray(GLuint index)
{
    HARM_CHECK_SHADER("GL_DisableVertexAttribArray");

    HARM_CHECK_SHADER_ATTR("GL_DisableVertexAttribArray", index);

    qglDisableVertexAttribArray(*(GLint *)((char *)backEnd.glState.currentProgram + index));

    HARM_CHECK_SHADER_ERROR();
}

/*
====================
GL_VertexAttribPointer
====================
*/
void GL_VertexAttribPointer(GLuint index, GLint size, GLenum type,
                            GLboolean normalized, GLsizei stride,
                            const GLvoid *pointer)
{
    HARM_CHECK_SHADER("GL_VertexAttribPointer");

    HARM_CHECK_SHADER_ATTR("GL_VertexAttribPointer", index);

    // RB_LogComment("qglVertexAttribPointer( %i, %i, %i, %i, %i, %p );\n", index, size, type, normalized, stride, pointer);
    qglVertexAttribPointer(*(GLint *)((char *)backEnd.glState.currentProgram + index),
                           size, type, normalized, stride, pointer);

    HARM_CHECK_SHADER_ERROR();
}

/*
=================
RB_ComputeMVP

Compute the model view matrix, with eventually required projection matrix depth hacks
=================
*/
void RB_ComputeMVP( const drawSurf_t * const surf, float mvp[16] ) {
    // Get the projection matrix
    float localProjectionMatrix[16];
    memcpy(localProjectionMatrix, backEnd.viewDef->projectionMatrix, sizeof(localProjectionMatrix));

    // Quick and dirty hacks on the projection matrix
    if ( surf->space->weaponDepthHack ) {
        localProjectionMatrix[14] = backEnd.viewDef->projectionMatrix[14] * 0.25;
    }
    if ( surf->space->modelDepthHack != 0.0 ) {
        localProjectionMatrix[14] = backEnd.viewDef->projectionMatrix[14] - surf->space->modelDepthHack;
    }

    // precompute the MVP
    myGlMultMatrix(surf->space->modelViewMatrix, localProjectionMatrix, mvp);
}

/*
====================
GL_SelectTextureNoClient
====================
*/
ID_INLINE static void GL_SelectTextureNoClient(int unit)
{
    backEnd.glState.currenttmu = unit;
    qglActiveTexture(GL_TEXTURE0 + unit);
    RB_LogComment("qglActiveTexture( %i )\n", unit);
}

ID_INLINE void			GL_Scissor( int x /* left*/, int y /* bottom */, int w, int h )
{
    qglScissor( x, y, w, h );
}

ID_INLINE void			GL_Viewport( int x /* left */, int y /* bottom */, int w, int h )
{
    qglViewport( x, y, w, h );
}

ID_INLINE void	GL_Scissor( const idScreenRect& rect )
{
    GL_Scissor( rect.x1, rect.y1, rect.x2 - rect.x1 + 1, rect.y2 - rect.y1 + 1 );
}

ID_INLINE void	GL_Viewport( const idScreenRect& rect )
{
    GL_Viewport( rect.x1, rect.y1, rect.x2 - rect.x1 + 1, rect.y2 - rect.y1 + 1 );
}

ID_INLINE void	GL_ViewportAndScissor( int x, int y, int w, int h )
{
    GL_Viewport( x, y, w, h );
    GL_Scissor( x, y, w, h );
}

ID_INLINE void	GL_ViewportAndScissor( const idScreenRect& rect )
{
    GL_Viewport( rect );
    GL_Scissor( rect );
}

/*
================
RB_SetMVP
================
*/
ID_INLINE void RB_SetMVP( const float mvp[16] )
{
    GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mvp);
}

/*
====================
GL_PolygonOffset
====================
*/
ID_INLINE void GL_PolygonOffset( bool enabled, float scale = 0.0f, float bias = 0.0f )
{
    if(enabled)
    {
        qglPolygonOffset( scale, bias );
        qglEnable(GL_POLYGON_OFFSET_FILL);
    }
    else
        qglDisable(GL_POLYGON_OFFSET_FILL);
}

/*
====================
GL_Color
====================
*/
ID_INLINE void GL_Color( float r, float g, float b, float a )
{
    float parm[4];
    parm[0] = idMath::ClampFloat( 0.0f, 1.0f, r );
    parm[1] = idMath::ClampFloat( 0.0f, 1.0f, g );
    parm[2] = idMath::ClampFloat( 0.0f, 1.0f, b );
    parm[3] = idMath::ClampFloat( 0.0f, 1.0f, a );
    GL_Uniform4fv(offsetof(shaderProgram_t, glColor), parm);
}

// RB begin
ID_INLINE void GL_Color( const idVec3& color )
{
    GL_Color( color[0], color[1], color[2], 1.0f );
}

ID_INLINE void GL_Color( const idVec4& color )
{
    GL_Color( color[0], color[1], color[2], color[3] );
}
// RB end

/*
====================
GL_Color
====================
*/
ID_INLINE void GL_Color( float r, float g, float b )
{
    GL_Color( r, g, b, 1.0f );
}

void GL_SelectTextureForce(int unit)
{
    backEnd.glState.currenttmu = -1;
    GL_SelectTexture(unit);
}