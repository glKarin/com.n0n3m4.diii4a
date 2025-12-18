#include "gles2_compat.h"

#define glDepthRange(a, b) qglDepthRangef(a, b)

#define countof(x) (sizeof(x) / sizeof(x[0]))
#define SIZEOF_MATRIX (sizeof(GLfloat) * 16)
#define MATCPY(d, s) { if((d) != (s)) memcpy((d), (s), SIZEOF_MATRIX); }

#define BACKEND_MODELVIEW_MATRIX (backEnd.viewDef->worldSpace.modelViewMatrix)
#define BACKEND_PROJECTION_MATRIX (backEnd.viewDef->projectionMatrix)

#define CLIENT_STATE_VERTEX 1
#define CLIENT_STATE_TEXCOORD (1 << 1)
#define CLIENT_STATE_COLOR (1 << 2)

#define GLRB_API // static
#define GLRB_PRIV static

#define COLOR_F2B(x) (GLubyte)((x) * 255.0f)
#define COLOR_B2F(x) (float)(x) / 255.0f

#define TEXTURE_STATE_1D 1
#define TEXTURE_STATE_2D (1 << 1)
#define TEXTURE_STATE_CUBE (1 << 2)
#define TEXTURE_STATE_3D (1 << 3)

//#pragma pack(push, 1)
// Draw vertex
struct GLvert
{
    GLfloat			xyz[3];
    GLfloat			st[2];
    //GLfloat			normal[3];
    GLubyte			color[4];
};
//#pragma pack(pop)
// strip size of glXxxxxxPointer/glVertexAttribPointer
#define GLVERT_STRIP sizeof(GLvert) // 0

static GLenum gl_RenderType; // glBegin() param
static GLfloat gl_TexCoord[2]; // glTexCoord2f() param
static GLboolean gl_TexCoordCalled = GL_FALSE; // if called glTexCoord in glBegin/glEnd
//static GLfloat gl_Normal[3]; // glNormal3f() param
static GLfloat gl_Color[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // glColor4f() param
static idList<GLvert> gl_VertexList; // glVertex3f() param, will fill texcoord/color automatic
static idList<glIndex_t> gl_IndexList; // glArrayElement() param
static GLenum gl_MatrixMode = GL_MODELVIEW; // glMatrixModel() param
static GLuint gl_ClientState = CLIENT_STATE_VERTEX | 0 | CLIENT_STATE_COLOR; // glEnableClientState)/glDisableClientState() param
static const float	GL_IDENTITY_MATRIX[16] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
};
static GLuint gl_drawPixelsImage = 0; // glDrawPixels texture handle
static GLuint gl_UseTexture = TEXTURE_STATE_2D;
static GLenum gl_PolygonMode = GL_FILL;

extern int MakePowerOfTwo(int num);

// OpenGL pipeline state
struct GLstate
{
	GLboolean TEXTURE_2D;
	GLboolean DEPTH_TEST;
	GLboolean CULL_FACE;
	GLboolean SCISSOR_TEST;
	GLboolean DEPTH_WRITEMASK;
	GLboolean BLEND;
    GLboolean STENCIL_TEST;
    GLint SCISSOR_BOX[4];
    GLfloat COLOR_CLEAR_VALUE[4];
};

template <int SIZE>
struct GLstate_stack
{
public:
	GLstate_stack()
			: num(0)
	{}

	GLstate * Push(const GLstate *state = NULL) {
		if(num > SIZE)
		{
			Sys_Printf("[GLCompat]: GLstate_stack::Push over\n");
			return NULL;
		}

		GLstate *ret = &stack[num++];
		if(state)
			memcpy(ret, state, sizeof(*state));
		return ret;
	}

	GLstate * Pop(void) {
		if(num <= 0)
		{
			Sys_Printf("[GLCompat]: GLstate_stack::Pop under\n");
			return NULL;
		}
		GLstate *ret = &stack[--num];
		return ret;
	}

	int Num(void) const {
		return num;
	}

/*	bool Full(void) const {
		return num >= SIZE;
	}

	bool Empty(void) const {
		return num <= 0;
	}*/

	GLstate stack[SIZE];
	int num;
};

template <int SIZE>
struct GLmatrix_stack
{
public:
	GLmatrix_stack(const char *n = "")
	: num(0), name(n), counter(0)
	{}

	void Push(const GLfloat m[16]) {
        counter++;
        //Sys_Printf("[GLCompat]: Push(%s)::(%d >= %d)\n", name.c_str(), counter, num);
		if(num >= SIZE)
		{
			Sys_Printf("[GLCompat]: GLmatrix_stack(%s)::Push over(%d >= %d)\n", name.c_str(), counter, num);
			return;
		}

		const GLfloat *mi;
		if(num > 0)
		{
			mi = Top();
		}
		else
		{
			mi = m;
		}
        GLfloat *t = stack[num++];
		MATCPY(t, mi);
	}

	void Pop(void) {
        counter--;
        //Sys_Printf("[GLCompat]: Pop(%s)::(%d >= %d)\n", name.c_str(), counter, num);
		if(num <= 0)
		{
			Sys_Printf("[GLCompat]: GLmatrix_stack(%s)::Pop under(%d <= %d)\n", name.c_str(), counter, num);
			return;
		}
		num--;
	}

	GLfloat * Top(void) {
		if(num > 0)
			return stack[num - 1];
		else
		{
			Sys_Printf("[GLCompat]: GLmatrix_stack(%s)::Top under\n", name.c_str());
			return NULL;
		}
	}

	void Set(const GLfloat m[16]) {
		if(num > 0)
		{
			GLfloat *t = Top();
			MATCPY(t, m);
		}
		else
			Sys_Printf("[GLCompat]: GLmatrix_stack(%s)::Set under\n", name.c_str());
	}

	void Mult(const GLfloat m[16]) {
		if(num > 0)
		{
			GLfloat *t = Top();
			GLfloat b[16];
			memcpy(b, t, sizeof(b));
			myGlMultMatrix(m, b, t);
		}
		else
			Sys_Printf("[GLCompat]: GLmatrix_stack(%s)::Mult under\n", name.c_str());
	}

	void Identity(void) {
		Set(GL_IDENTITY_MATRIX);
	}

	bool Empty(void) const {
		return num <= 0;
	}

	int Num(void) const {
		return num;
	}

	const GLfloat * Top(const GLfloat *m) const {
		if(num > 0)
			return stack[num - 1];
		else
			return m;
	}

	void Clear(void) {
		num = 0;
	}

	GLfloat stack[16][SIZE];
	int num;
    idStr name;
    int counter;
};
// Projection matrix stack for glPushMatrix()/glPopMatrix()/glXxxxMatrix
static GLmatrix_stack</*2*/8> gl_ProjectionMatrixStack("Projection");
// Modelview matrix stack for glPushMatrix()/glPopMatrix()/glXxxxMatrix
static GLmatrix_stack<8> gl_ModelviewMatrixStack("Modelview"); // 64

#define gl_ProjectionMatrix (gl_ProjectionMatrixStack.Top(BACKEND_PROJECTION_MATRIX))
#define gl_ModelviewMatrix (gl_ModelviewMatrixStack.Top(BACKEND_MODELVIEW_MATRIX))

// Pipeline state stack for glPushAttrib()/glPopAttrib()
static GLstate_stack<2> gl_StateStack; // 16

GLRB_API void glPushAttrib(GLint mask)
{
	(void)mask;

	GLstate *state = gl_StateStack.Push();
	if(!state)
		return;
#define _GETBS(x) state->x = qglIsEnabled(GL_##x);
	_GETBS(TEXTURE_2D);
	_GETBS(DEPTH_TEST);
	_GETBS(CULL_FACE);
	_GETBS(SCISSOR_TEST);
	_GETBS(BLEND);
    _GETBS(STENCIL_TEST);
#undef _GETBS
#define _GETBS(x) qglGetBooleanv(GL_##x, &state->x);
	_GETBS(DEPTH_WRITEMASK);
#undef _GETBS
#define _GETBS(x) qglGetIntegerv(GL_##x, &state->x[0]);
    _GETBS(SCISSOR_BOX);
#undef _GETBS
#define _GETBS(x) qglGetFloatv(GL_##x, &state->x[0]);
    _GETBS(COLOR_CLEAR_VALUE);
#undef _GETBS
}

GLRB_API void glPopAttrib(void)
{
	GLstate *state = gl_StateStack.Pop();
	if(!state)
		return;
#define _SETBS(x) { if(state->x) qglEnable(GL_##x); else qglDisable(GL_##x); }
	_SETBS(TEXTURE_2D);
	_SETBS(DEPTH_TEST);
	_SETBS(CULL_FACE);
	_SETBS(SCISSOR_TEST);
	_SETBS(BLEND);
    _SETBS(STENCIL_TEST);
#undef _SETBS
	qglDepthMask(state->DEPTH_WRITEMASK);
    qglScissor(state->SCISSOR_BOX[0], state->SCISSOR_BOX[1], state->SCISSOR_BOX[2], state->SCISSOR_BOX[3]);
    qglClearColor(state->COLOR_CLEAR_VALUE[0], state->COLOR_CLEAR_VALUE[1], state->COLOR_CLEAR_VALUE[2], state->COLOR_CLEAR_VALUE[3]);
}

GLRB_API void glPushMatrix(void)
{
	if(gl_MatrixMode == GL_PROJECTION)
	{
		gl_ProjectionMatrixStack.Push(BACKEND_PROJECTION_MATRIX);
	}
	else
	{
		gl_ModelviewMatrixStack.Push(BACKEND_MODELVIEW_MATRIX);
	}
}

GLRB_API void glPopMatrix(void)
{
	if(gl_MatrixMode == GL_PROJECTION)
	{
		gl_ProjectionMatrixStack.Pop();
	}
	else
	{
		gl_ModelviewMatrixStack.Pop();
	}
}

GLRB_API void glMatrixMode(GLenum mode)
{
	gl_MatrixMode = mode;
}

GLRB_API void glLoadIdentity(void)
{
	if(gl_MatrixMode == GL_PROJECTION)
	{
		if(gl_ProjectionMatrixStack.Empty())
			gl_ProjectionMatrixStack.Push(GL_IDENTITY_MATRIX);
		else
			gl_ProjectionMatrixStack.Identity();
	}
	else
	{
		if(gl_ModelviewMatrixStack.Empty())
			gl_ModelviewMatrixStack.Push(GL_IDENTITY_MATRIX);
		else
			gl_ModelviewMatrixStack.Identity();
	}
}

#define GLCP_MULT_MATRIX(x) \
if(gl_MatrixMode == GL_PROJECTION) \
{ \
    if(gl_ProjectionMatrixStack.Empty()) \
    { \
        gl_ProjectionMatrixStack.Push(BACKEND_PROJECTION_MATRIX); \
    } \
    GLfloat m[16]; \
    memcpy(m, gl_ProjectionMatrix, sizeof(GLfloat) * 16); \
    x; \
    gl_ProjectionMatrixStack.Set(m); \
} \
else \
{ \
    if(gl_ModelviewMatrixStack.Empty()) \
    { \
        gl_ModelviewMatrixStack.Push(BACKEND_MODELVIEW_MATRIX); \
    } \
    GLfloat m[16]; \
    memcpy(m, gl_ModelviewMatrix, sizeof(GLfloat) * 16); \
    x; \
    gl_ModelviewMatrixStack.Set(m); \
}

GLRB_API void glOrtho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearZ, GLfloat farZ)
{
    GLCP_MULT_MATRIX(esOrtho((ESMatrix *)m, left, right, bottom, top, nearZ, farZ))
}

GLRB_API void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    GLCP_MULT_MATRIX(esRotate((ESMatrix *)m, angle, x, y, z))
}

GLRB_API void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    GLCP_MULT_MATRIX(esTranslate((ESMatrix *)m, x, y, z))
}

GLRB_API void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

GLRB_API void glPolygonMode(GLenum face, GLenum mode)
{
    (void)face;
    gl_PolygonMode = mode;
};

GLRB_PRIV void glesFillVertex(GLvert &drawVert)
{
	drawVert.color[0] = COLOR_F2B(gl_Color[0]);
	drawVert.color[1] = COLOR_F2B(gl_Color[1]);
	drawVert.color[2] = COLOR_F2B(gl_Color[2]);
	drawVert.color[3] = COLOR_F2B(gl_Color[3]);
	drawVert.st[0] = gl_TexCoord[0];
    drawVert.st[1] = gl_TexCoord[1];
}

GLRB_API void glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	GLvert drawVert;
	drawVert.xyz[0] = x;
    drawVert.xyz[1] = y;
    drawVert.xyz[2] = z;
	glesFillVertex(drawVert);
	gl_VertexList.Append(drawVert);
}

GLRB_API void glNormal3f(GLfloat x, GLfloat y, GLfloat z)
{
#if 0
    gl_Normal[0] = x;
    gl_Normal[1] = y;
    gl_Normal[2] = z;
#endif
}

GLRB_API void glVertex2f(GLfloat x, GLfloat y)
{
	glVertex3f(x, y, 0.0f);
}

GLRB_API void glVertex3fv(const GLfloat v[3])
{
	glVertex3f(v[0], v[1], v[2]);
}

GLRB_API void glTexCoord2f(GLfloat s, GLfloat t)
{
	gl_TexCoord[0] = s;
	gl_TexCoord[1] = t;
    gl_TexCoordCalled = GL_TRUE;
}

GLRB_API void glTexCoord2fv(const GLfloat st[2])
{
    glTexCoord2f(st[0], st[1]);
}

GLRB_API void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	gl_Color[0] = r;
	gl_Color[1] = g;
	gl_Color[2] = b;
	gl_Color[3] = a;
}

GLRB_API void glColor3f(GLfloat r, GLfloat g, GLfloat b)
{
	glColor4f(r, g, b, 1.0f);
}

GLRB_API void glColor3fv(const GLfloat v[3])
{
	glColor3f(v[0], v[1], v[2]);
}

GLRB_API void glColor4fv(const GLfloat v[4])
{
	glColor4f(v[0], v[1], v[2], v[3]);
}

ID_INLINE GLRB_API void glColor4ubv(const GLubyte v[4])
{
	glColor4f(COLOR_B2F(v[0]), COLOR_B2F(v[1]), COLOR_B2F(v[2]), COLOR_B2F(v[3]));
}

ID_INLINE GLRB_API void glArrayElement(GLint index)
{
	gl_IndexList.Append(index);
}

GLRB_API void glDisableClientState(GLenum e)
{
	// `default` glsl shader must attr_Color is all [255, 255, 255, 255]
	if(e == GL_TEXTURE_COORD_ARRAY)
		gl_ClientState &= ~CLIENT_STATE_TEXCOORD;
	else if(e == GL_COLOR_ARRAY)
		gl_ClientState &= ~CLIENT_STATE_COLOR;
}

GLRB_API void glEnableClientState(GLenum e)
{
	// `default` glsl shader must attr_Color is all [255, 255, 255, 255]
	if(e == GL_TEXTURE_COORD_ARRAY)
		gl_ClientState |= CLIENT_STATE_TEXCOORD;
	else if(e == GL_COLOR_ARRAY)
		gl_ClientState |= CLIENT_STATE_COLOR;
}

GLRB_PRIV GLboolean glesClientStateIsEnabled(GLenum e)
{
	// `default` glsl shader must attr_Color is all [255, 255, 255, 255]
	if(e == GL_TEXTURE_COORD_ARRAY)
		return gl_ClientState & CLIENT_STATE_TEXCOORD ? GL_TRUE : GL_FALSE;
	else if(e == GL_COLOR_ARRAY)
		return gl_ClientState & CLIENT_STATE_COLOR ? GL_TRUE : GL_FALSE;
	else
		return GL_TRUE;
}

GLRB_PRIV GLboolean glesIsVertexPointerBinding(GLenum type)
{
    if(!backEnd.glState.currentProgram)
    {
        Sys_Printf("GLSL shader program is not binding current!\n");
        return GL_FALSE;
    }
    GLint usingPointer;
    GLint parm;
    if(type == GL_TEXTURE_COORD_ARRAY)
        parm = SHADER_PARM_HANDLE(SHADER_PARM_ADDR(attr_TexCoord));
    else if(type == GL_COLOR_ARRAY)
        parm = SHADER_PARM_HANDLE(SHADER_PARM_ADDR(attr_Color));
    else
        parm = SHADER_PARM_HANDLE(SHADER_PARM_ADDR(attr_Vertex));
    if(parm < 0)
        return GL_FALSE;
    qglGetVertexAttribiv(parm, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &usingPointer);
    return usingPointer ? GL_TRUE : GL_FALSE;
}

GLRB_API void glLoadMatrixf(const GLfloat matrix[16])
{
	if(gl_MatrixMode == GL_PROJECTION)
	{
		if(gl_ProjectionMatrixStack.Empty())
			gl_ProjectionMatrixStack.Push(matrix);
		else
			gl_ProjectionMatrixStack.Set(matrix);
	}
	else
	{
		if(gl_ModelviewMatrixStack.Empty())
			gl_ModelviewMatrixStack.Push(matrix);
		else
			gl_ModelviewMatrixStack.Set(matrix);
	}
}

GLRB_API void glMultMatrixf(const GLfloat matrix[16])
{
	if(gl_MatrixMode == GL_PROJECTION)
	{
		if(gl_ProjectionMatrixStack.Empty())
			gl_ProjectionMatrixStack.Push(matrix);
		else
			gl_ProjectionMatrixStack.Mult(matrix);
	}
	else
	{
		if(gl_ModelviewMatrixStack.Empty())
			gl_ModelviewMatrixStack.Push(matrix);
		else
			gl_ModelviewMatrixStack.Mult(matrix);
	}
}

// must call glesStartRender first
ID_INLINE GLRB_API void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), size, type, false, stride, pointer);
}

GLRB_API void glesGetFloatv(GLenum pname, GLfloat *data)
{
    switch (pname) {
        case GL_CURRENT_COLOR:
            memcpy(data, gl_Color, sizeof(GLfloat) * 4);
            break;
        case GL_PROJECTION_MATRIX:
            memcpy(data, gl_ProjectionMatrix, sizeof(GLfloat) * 16);
            break;
        default:
            qglGetFloatv(pname, data);
            break;
    }
}

GLRB_API void glesEnable(GLenum pname)
{
    switch (pname) {
        case GL_TEXTURE_1D:
            gl_UseTexture |= TEXTURE_STATE_1D;
            break;
        case GL_TEXTURE_2D:
            gl_UseTexture |= TEXTURE_STATE_2D;
            break;
        case GL_TEXTURE_CUBE_MAP:
            gl_UseTexture |= TEXTURE_STATE_CUBE;
            break;
        case GL_TEXTURE_3D:
            gl_UseTexture |= TEXTURE_STATE_3D;
            break;
        default:
            qglEnable(pname);
            break;
    }
}

GLRB_API void glesDisable(GLenum pname)
{
    switch (pname) {
        case GL_TEXTURE_1D:
            gl_UseTexture &= ~TEXTURE_STATE_1D;
            break;
        case GL_TEXTURE_2D:
            gl_UseTexture &= ~TEXTURE_STATE_2D;
            break;
        case GL_TEXTURE_CUBE_MAP:
            gl_UseTexture &= ~TEXTURE_STATE_CUBE;
            break;
        case GL_TEXTURE_3D:
            gl_UseTexture &= ~TEXTURE_STATE_3D;
            break;
        default:
            qglDisable(pname);
            break;
    }
}

GLRB_API void glBegin(GLenum t)
{
	gl_RenderType = t;
}

GLRB_API void glesDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
	GLboolean usingColor = glesIsVertexPointerBinding(GL_COLOR_ARRAY);
    GLboolean usingTexCoord = glesIsVertexPointerBinding(GL_TEXTURE_COORD_ARRAY);

	GL_SelectTexture(0);
	if(!usingTexCoord || gl_UseTexture == 0)
		globalImages->whiteImage->Bind();

	if(usingColor) // use color array
	{
		GL_Uniform1fv(SHADER_PARM_ADDR(colorAdd), zero);
		GL_Uniform1fv(SHADER_PARM_ADDR(colorModulate), oneModulate);
		GL_Uniform4f(SHADER_PARM_ADDR(glColor), 1.0f, 1.0f, 1.0f, 1.0f);
	}
	else // not use color array
	{
		GL_Uniform1fv(SHADER_PARM_ADDR(colorAdd), one);
		GL_Uniform1fv(SHADER_PARM_ADDR(colorModulate), zero);
		GL_Uniform4fv(SHADER_PARM_ADDR(glColor), gl_Color);
	}
	qglDrawElements(mode, count, type, indices);
}

GLRB_PRIV void glesStartRender(void)
{
	GL_UseProgram(&defaultShader);
	GL_EnableVertexAttribArray(SHADER_PARM_ADDR(attr_Vertex));

	GL_Uniform4fv(SHADER_PARM_ADDR(glColor), gl_Color);
	GL_Uniform1fv(SHADER_PARM_ADDR(colorAdd), zero);
	GL_Uniform1fv(SHADER_PARM_ADDR(colorModulate), oneModulate);

	GLfloat	gl_MVPMatrix[16];
	myGlMultMatrix(gl_ModelviewMatrix, gl_ProjectionMatrix, gl_MVPMatrix);

	GL_UniformMatrix4fv(SHADER_PARM_ADDR(modelViewProjectionMatrix), gl_MVPMatrix);
	float textureMatrix[16];
	esMatrixLoadIdentity((ESMatrix *)textureMatrix);
    GL_UniformMatrix4fv(SHADER_PARM_ADDR(textureMatrix), textureMatrix);

	//Sys_Printf("Current shader program: %p\n", backEnd.glState.currentProgram);
}

GLRB_PRIV void glesEndRender(void)
{
	//Sys_Printf("glesEndRender shader program: %p\n", backEnd.glState.currentProgram);
	GL_DisableVertexAttribArray(SHADER_PARM_ADDR(attr_Vertex));
	GL_UseProgram(NULL);
	//gl_UseTexture = GL_FALSE;
}

// GL_TRIANGLE_STRIP: odd: n-1, n-2, n | even: n-2, n-1, n
// GL_QUAD_STRIP: 2n-1、2n、2n+2, 2n+1
GLRB_PRIV int glesNormalizeVertexes(void)
{
    if(gl_RenderType == GL_QUADS)
    {
        if(gl_VertexList.Num() < 4)
            return 0; // must >= 4

		// if == 4, using triangle fan
        if(gl_VertexList.Num() < 8)
        {
			// if < 8, remove invalid vertexes
			if(gl_VertexList.Num() > 4)
				gl_VertexList.SetNum(4);
			if(gl_PolygonMode == GL_LINE)
				gl_RenderType = GL_LINE_LOOP;
			else
				gl_RenderType = GL_TRIANGLE_FAN;
            return 1;
        }

		int numQuads = gl_VertexList.Num() / 4;
		if(gl_PolygonMode == GL_LINE)
		{
			gl_RenderType = GL_LINES;
			gl_IndexList.Resize(numQuads * 4 * 2);
			for(int i = 0; i < numQuads; i++)
			{
				glIndex_t *target = &gl_IndexList[i * 8];
				glIndex_t base = i * 4;
				int v1 = base; 
				int v2 = base + 1;
				int v3 = base + 2;
				int v4 = base + 3;
				target[0] = v1;
				target[1] = v2;
				target[2] = v2;
				target[3] = v3;
				target[4] = v3;
				target[5] = v4;
				target[6] = v4;
				target[7] = v1;
			}
		}
		else
		{
			// if more than 2 quads, using triangles
			gl_RenderType = GL_TRIANGLES;
			gl_IndexList.Resize(numQuads * 3 * 2);
			for(int i = 0; i < numQuads; i++)
			{
				glIndex_t *target = &gl_IndexList[i * 6];
				glIndex_t base = i * 4;
				int v1 = base; 
				int v2 = base + 1;
				int v3 = base + 2;
				int v4 = base + 3;
				target[0] = v1;
				target[1] = v2;
				target[2] = v3;
				target[3] = v1;
				target[4] = v3;
				target[5] = v4;
			}
		}
        return 1;
    }
    else if(gl_RenderType == GL_QUAD_STRIP) // 2n-1、2n、2n+2, 2n+1
    {
        if(gl_VertexList.Num() < 4)
            return 0; // must >= 4

		// if == 4, using triangle strip
        if(gl_VertexList.Num() < 6)
        {
			// if < 6, remove invalid vertexes
			if(gl_VertexList.Num() > 4)
				gl_VertexList.SetNum(4);
			if(gl_PolygonMode == GL_LINE)
			{
				gl_RenderType = GL_LINE_LOOP;
				gl_IndexList.SetNum(4);
				gl_IndexList[0] = 0;
				gl_IndexList[1] = 1;
				gl_IndexList[2] = 3;
				gl_IndexList[3] = 2;
			}
			else
				gl_RenderType = GL_TRIANGLE_STRIP;
            return 1;
        }

		int numQuads = gl_VertexList.Num() / 2 - 1;
		if(gl_PolygonMode == GL_LINE)
		{
			gl_RenderType = GL_LINES;
			gl_IndexList.Resize(numQuads * 4 * 2);
			for(int i = 1, j = 0; i < numQuads; i++, j += 8)
			{
				// 1: 1 2 4 3; 2: 3 4 6 5; 3: 5 6 8 7
				glIndex_t *target = &gl_IndexList[j];
				int v1 = 2 * i - 1;
				int v2 = 2 * i;
				int v3 = 2 * i + 2;
				int v4 = 2 * i + 1;
				target[0] = v1;
				target[1] = v2;
				target[2] = v2;
				target[3] = v4;
				target[4] = v4;
				target[5] = v3;
				target[6] = v3;
				target[7] = v1;
			}
		}
		else
		{
			// if more than 2 quads, using triangles
			gl_RenderType = GL_TRIANGLES;
			gl_IndexList.Resize(numQuads * 3 * 2);
			for(int i = 1, j = 0; i < numQuads; i++, j += 6)
			{
				// 1: 1 2 4 3; 2: 3 4 6 5; 3: 5 6 8 7
				glIndex_t *target = &gl_IndexList[j];
				int v1 = 2 * i - 1;
				int v2 = 2 * i;
				int v3 = 2 * i + 2;
				int v4 = 2 * i + 1;
				target[0] = v1;
				target[1] = v2;
				target[2] = v4;
				target[3] = v1;
				target[4] = v4;
				target[5] = v3;
			}
		}
        return 1;
    }
    else if(gl_RenderType == GL_POLYGON) // using GL_TRIANGLE_FAN
    {
		if(gl_PolygonMode == GL_LINE)
		{
			switch (gl_VertexList.Num()) {
				case 1:
					gl_RenderType = GL_POINTS;
					break;
				case 2:
					gl_RenderType = GL_LINES;
					break;
				case 3:
				default:
					gl_RenderType = GL_LINE_LOOP;
					break;
			}
		}
		else
		{
			switch (gl_VertexList.Num()) {
				case 1:
					gl_RenderType = GL_POINTS;
					break;
				case 2:
					gl_RenderType = GL_LINES;
					break;
				case 3:
					gl_RenderType = GL_TRIANGLES;
					break;
				default:
					gl_RenderType = GL_TRIANGLE_FAN;
					break;
			}
		}
        return 1;
    }
    else if(gl_RenderType == GL_TRIANGLES)
    {
		if(gl_VertexList.Num() < 3)
			return 0; // must >= 3

		if(gl_PolygonMode == GL_LINE)
		{
			gl_RenderType = GL_LINES;
			int numTris = gl_VertexList.Num() / 3;
			gl_IndexList.Resize(numTris * 3 * 2);
			for(int i = 0; i < numTris; i++)
			{
				glIndex_t *target = &gl_IndexList[i * 6];
				int base = i * 3;
				int v1 = base;
				int v2 = base + 1;
				int v3 = base + 2;
				target[0] = v1;
				target[1] = v2;
				target[2] = v2;
				target[3] = v3;
				target[4] = v3;
				target[5] = v1;
			}
		}
        return 1;
    }
    else if(gl_RenderType == GL_TRIANGLE_STRIP)
    {
        if(gl_VertexList.Num() < 3)
            return 0; // must >= 3
		if(gl_PolygonMode == GL_LINE)
		{
			gl_RenderType = GL_LINES;
			int numTris = gl_VertexList.Num() - 2;
			gl_IndexList.Resize(numTris * 3 * 2);
			for(int i = 2, j = 0; i < gl_VertexList.Num(); i++, j += 6)
			{
				glIndex_t *target = &gl_IndexList[j];
				int v1, v2, v3;
				if(i % 2)
				{
					v1 = i - 2;
					v2 = i - 1;
					v3 = i;
				}
				else
				{
					v1 = i - 1;
					v2 = i - 2;
					v3 = i;
				}
				target[0] = v1;
				target[1] = v2;
				target[2] = v2;
				target[3] = v3;
				target[4] = v3;
				target[5] = v1;
			}
		}
		return 1;
    }
    else if(gl_RenderType == GL_TRIANGLE_FAN)
    {
        if(gl_VertexList.Num() < 3)
            return 0; // must >= 3
		if(gl_PolygonMode == GL_LINE)
			gl_RenderType = GL_LINE_LOOP;
        return 1;
    }
    else
        return 1;
}

GLRB_PRIV void glesImmediateModeRender(void)
{
    if(!gl_RenderType)
        return;

    const int num = gl_VertexList.Num();
    const int numIndex = gl_IndexList.Num();

    if(num > 0) // call glVertex[n]*
    {
        //gl_IndexList.Clear();
        if(glesNormalizeVertexes())
        {
            const GLboolean usingTexCoord = gl_TexCoordCalled && (/*glesClientStateIsEnabled(GL_TEXTURE_COORD_ARRAY) || */gl_UseTexture != 0);

            glesStartRender();

            qglBindBuffer(GL_ARRAY_BUFFER, 0);
            GL_EnableVertexAttribArray(SHADER_PARM_ADDR(attr_Color));
            if(usingTexCoord)
                GL_EnableVertexAttribArray(SHADER_PARM_ADDR(attr_TexCoord));

            GL_SelectTexture(0);
            if(!usingTexCoord)
                globalImages->whiteImage->Bind();

            const GLfloat *vertex = (GLfloat *)&gl_VertexList[0].xyz[0];
            const GLubyte *color = (GLubyte *)&gl_VertexList[0].color[0];
            const GLfloat *texcoord = NULL;
            if(usingTexCoord)
                texcoord = (GLfloat *)&gl_VertexList[0].st[0];

            GL_VertexAttribPointer(SHADER_PARM_ADDR(attr_Vertex), 3, GL_FLOAT, false, GLVERT_STRIP, vertex);
            GL_VertexAttribPointer(SHADER_PARM_ADDR(attr_Color), 4, GL_UNSIGNED_BYTE, false, GLVERT_STRIP, color);
            if(usingTexCoord)
                GL_VertexAttribPointer(SHADER_PARM_ADDR(attr_TexCoord), 2, GL_FLOAT, false, GLVERT_STRIP, texcoord);

            GL_Uniform1fv(SHADER_PARM_ADDR(colorAdd), zero);
            GL_Uniform1fv(SHADER_PARM_ADDR(colorModulate), oneModulate);
            GL_Uniform4f(SHADER_PARM_ADDR(glColor), 1.0f, 1.0f, 1.0f, 1.0f);

            if(gl_IndexList.Num())
            {
                qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
                qglDrawElements(gl_RenderType, gl_IndexList.Num(), GL_INDEX_TYPE, &gl_IndexList[0]);
            }
            else
                qglDrawArrays(gl_RenderType, 0, num);

            GL_DisableVertexAttribArray(SHADER_PARM_ADDR(attr_Color));
            if(usingTexCoord)
                GL_DisableVertexAttribArray(SHADER_PARM_ADDR(attr_TexCoord));

            glesEndRender();
        }
    }
    else if(numIndex > 0) // call glArrayElement
    {
        const GLboolean usingTexCoord = glesClientStateIsEnabled(GL_TEXTURE_COORD_ARRAY) && gl_UseTexture != 0 && glesIsVertexPointerBinding(GL_TEXTURE_COORD_ARRAY);
        GLboolean usingColor = glesClientStateIsEnabled(GL_COLOR_ARRAY) && glesIsVertexPointerBinding(GL_COLOR_ARRAY);

        qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        GL_SelectTexture(0);
        if(!usingTexCoord)
            globalImages->whiteImage->Bind();

        if(usingColor) // use color array
        {
            GL_Uniform1fv(SHADER_PARM_ADDR(colorAdd), zero);
            GL_Uniform1fv(SHADER_PARM_ADDR(colorModulate), oneModulate);
            GL_Uniform4f(SHADER_PARM_ADDR(glColor), 1.0f, 1.0f, 1.0f, 1.0f);
        }
        else // not use color array
        {
            GL_Uniform1fv(SHADER_PARM_ADDR(colorAdd), one);
            GL_Uniform1fv(SHADER_PARM_ADDR(colorModulate), zero);
            GL_Uniform4fv(SHADER_PARM_ADDR(glColor), gl_Color);
        }

        qglDrawElements(gl_RenderType, numIndex, GL_INDEX_TYPE, &gl_IndexList[0]);
    }
}

// Immediate mode draw func
GLRB_API void glEnd(void)
{
    glesImmediateModeRender();

	gl_VertexList.Clear();
	gl_IndexList.Clear();
	gl_RenderType = 0;
    gl_TexCoordCalled = GL_FALSE;
}

GLRB_PRIV void glesDebugRenderCompat(void)
{
	Sys_Printf("----- glesDebugRenderCompat -----\n");
	Sys_Printf("Projection matrix stack: %d\n", gl_ProjectionMatrixStack.Num());
	Sys_Printf("Modelview matrix stack: %d\n", gl_ModelviewMatrixStack.Num());
	Sys_Printf("Attrib stack: %d\n", gl_StateStack.Num());
	Sys_Printf("MatrixMode: 0x%X\n", gl_MatrixMode);
	Sys_Printf("Color: %.6f, %.6f, %.6f, %.6f\n", gl_Color[0], gl_Color[1], gl_Color[2], gl_Color[3]);
	Sys_Printf("\n");
}

#define DEBUG_RENDER_COMPAT
// #define DEBUG_RENDER_COMPAT glesDebugRenderCompat();

static void RB_SetGL2D_compat(void)
{
	// set 2D virtual screen size
	qglViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);

	if (r_useScissor.GetBool()) {
		qglScissor(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	}

	// always assume 640x480 virtual coordinates
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 640, 480, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	GL_State(GLS_DEPTHFUNC_ALWAYS |
			 GLS_SRCBLEND_SRC_ALPHA |
			 GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

	GL_Cull(CT_TWO_SIDED);

	qglDisable(GL_DEPTH_TEST);
	qglDisable(GL_STENCIL_TEST);
}

void RB_ShowImages_compat(void)
{
	int		i;
	idImage	*image;
	float	x, y, w, h;
	int		start, end;

	RB_SetGL2D_compat();

	//qglClearColor( 0.2, 0.2, 0.2, 1 );
	//qglClear( GL_COLOR_BUFFER_BIT );

	qglFinish();
//	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
//	glDisableClientState(GL_COLOR_ARRAY);
	glColor4f(1.0, 1.0, 1.0, 1.0);

	start = Sys_Milliseconds();

	for (i = 0 ; i < globalImages->images.Num() ; i++) {
		image = globalImages->images[i];

		if (image->texnum == idImage::TEXTURE_NOT_LOADED && image->partialImage == NULL) {
			continue;
		}

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if (r_showImages.GetInteger() == 2) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		image->Bind();
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2f(x, y);
		glTexCoord2f(1, 0);
		glVertex2f(x + w, y);
		glTexCoord2f(1, 1);
		glVertex2f(x + w, y + h);
		glTexCoord2f(0, 1);
		glVertex2f(x, y + h);
		glEnd();
	}

	qglFinish();
//	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
//	glEnableClientState(GL_COLOR_ARRAY);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	end = Sys_Milliseconds();
	common->Printf("%i msec to draw all images\n", end - start);
}

ID_INLINE GLRB_API void glRasterPos2f(GLfloat x, GLfloat y)
{

}

GLRB_PRIV void glesCreateDrawPixelsImage(GLint width, GLint height, const GLubyte *data, GLfloat *x, GLfloat *y)
{
    if(gl_drawPixelsImage == 0)
    {
        qglGenTextures(1, &gl_drawPixelsImage);
    }

    int scaled_width = MakePowerOfTwo(width);
    int scaled_height = MakePowerOfTwo(height);

    qglBindTexture(GL_TEXTURE_2D, gl_drawPixelsImage);
    qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if(scaled_width == width && scaled_height == height)
    {
        qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
        if(x)
            *x = 1.0f;
        if(y)
            *y = 1.0f;
    }
    else
    {
        GLubyte *d = (GLubyte *)malloc(scaled_width * scaled_height * 4);
        for(int h = 0; h < height; h++)
        {
            for(int w = 0; w < width; w++)
            {
                GLubyte *out = d + (h * scaled_width + w) * 4;
                const GLubyte *in_ = data + (h * width + w) * 4;
                memcpy(out, in_, 4);
            }
        }
        qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, d );
        free(d);
        if(x)
            *x = (float)width / (float)scaled_width;
        if(y)
            *y = (float)height / (float)scaled_height;
    }
	//qglGenerateMipmap(GL_TEXTURE_2D);
}

GLRB_API void glDrawPixels(GLint width, GLint height, GLenum format, GLenum dataType, const void *data)
{
    if(format != GL_RGBA)
	{
		Sys_Printf("glDrawPixels only support format = GL_RGBA\n");
		return;
	}
    if(dataType != GL_UNSIGNED_BYTE)
	{
		Sys_Printf("glDrawPixels only support dataType = GL_UNSIGNED_BYTE\n");
		return;
	}

	GLint texid;
	qglGetIntegerv(GL_TEXTURE_BINDING_2D, &texid);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    qglDisable(GL_DEPTH_TEST);
    qglDepthMask(GL_FALSE);
	qglDisable(GL_BLEND);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);

    float tcw, tch;
    glesCreateDrawPixelsImage(width, height, (const GLubyte *)data, &tcw, &tch);
    GLuint old = gl_UseTexture;
    if((old & TEXTURE_STATE_2D) == 0)
	    gl_UseTexture |= TEXTURE_STATE_2D;

    glBegin(GL_TRIANGLE_STRIP); // CW
	{
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(0.0f, 0.0f);

        glTexCoord2f(0.0f, tch);
        glVertex2f(0.0f, height);

        glTexCoord2f(tcw, 0.0f);
        glVertex2f(width, 0.0f);

		glTexCoord2f(tcw, tch);
		glVertex2f(width, height);
	}
    glEnd();

    if(old != gl_UseTexture)
        gl_UseTexture = old;

    qglBindTexture(GL_TEXTURE_2D, texid);

    glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

void glesShutdown(void)
{
    Sys_Printf("GL compat layer shutdown.\n");
    if(gl_drawPixelsImage != 0)
    {
        if(qglIsTexture(gl_drawPixelsImage))
            qglDeleteTextures(1, &gl_drawPixelsImage);
        gl_drawPixelsImage = 0;
    }
}

void glesReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void * data, GLsizei align)
{
    GLint packAlign = align;
    qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);
    bool changed = packAlign != align;
    if(changed)
        qglPixelStorei(GL_PACK_ALIGNMENT, align);	// otherwise small rows get padded to 32 bits

    qglReadPixels(x, y, width, height, format, type, data);

    // restore
    if(changed)
        qglPixelStorei(GL_PACK_ALIGNMENT, packAlign);	// otherwise small rows get padded to 32 bits
}

void RB_RenderDepthToColor(bool notPack = false)
{
    depthStencilRenderer.Blit(GL_DEPTH_BUFFER_BIT);

    depthStencilRenderer.BeginRender();
    qglClear(GL_COLOR_BUFFER_BIT);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    qglDisable(GL_STENCIL_TEST);
    qglDisable(GL_DEPTH_TEST);
    qglDepthMask(GL_FALSE);
    qglDisable(GL_BLEND);

    GL_UseProgram(&depthToColorShader);

    GL_SelectTextureNoClient(0);
    depthStencilRenderer.BindDepth();

    float tcw, tch;
    if(depthStencilRenderer.Width() == depthStencilRenderer.UploadWidth())
        tcw = 1.0f;
    else
        tcw = (float)depthStencilRenderer.Width() / (float)depthStencilRenderer.UploadWidth();
    if(depthStencilRenderer.Height() == depthStencilRenderer.UploadHeight())
        tch = 1.0f;
    else
        tch = (float)depthStencilRenderer.Height() / (float)depthStencilRenderer.UploadHeight();

    float width = 1.0f;
    float height = 1.0f;
    GLfloat vertexes[] = {
        0.0f, 0.0f,
        0.0f, 0.0f,
        0.0f, tch,
        0.0f, height,
        tcw, 0.0f,
        width, 0.0f,
        tcw, tch,
        width, height,
    };

    qglBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));

    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(GLfloat) * 4, &vertexes[0]);
    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 2, GL_FLOAT, false, sizeof(GLfloat) * 4, &vertexes[2]);

    GLfloat	mvp[16];
    esMatrixLoadIdentity((ESMatrix *)mvp);
    esOrtho((ESMatrix *)mvp, 0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
    esMatrixTranspose((ESMatrix *)mvp);
    GL_UniformMatrix4fv(SHADER_PARM_ADDR(modelViewProjectionMatrix), mvp);

    GL_Uniform1i(SHADER_PARMS_ADDR(u_uniformParm, 0), int(notPack));

    qglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));

    GL_UseProgram(NULL);

    glPopAttrib();

    depthStencilRenderer.End();
}

void RB_RenderStencilToColor(int comp = 0)
{
    depthStencilRenderer.Blit(GL_STENCIL_BUFFER_BIT);

    depthStencilRenderer.BeginRender();
    qglClear(GL_COLOR_BUFFER_BIT);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    qglDisable(GL_STENCIL_TEST);
    qglDisable(GL_DEPTH_TEST);
    qglDepthMask(GL_FALSE);
    qglDisable(GL_BLEND);

    GL_UseProgram(&stencilToColorShader);

    GL_SelectTextureNoClient(0);
    depthStencilRenderer.BindStencil();

    float tcw, tch;
    if(depthStencilRenderer.Width() == depthStencilRenderer.UploadWidth())
        tcw = 1.0f;
    else
        tcw = (float)depthStencilRenderer.Width() / (float)depthStencilRenderer.UploadWidth();
    if(depthStencilRenderer.Height() == depthStencilRenderer.UploadHeight())
        tch = 1.0f;
    else
        tch = (float)depthStencilRenderer.Height() / (float)depthStencilRenderer.UploadHeight();

    float width = 1.0f;
    float height = 1.0f;
    GLfloat vertexes[] = {
            0.0f, 0.0f,
            0.0f, 0.0f,
            0.0f, tch,
            0.0f, height,
            tcw, 0.0f,
            width, 0.0f,
            tcw, tch,
            width, height,
    };

    qglBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));

    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(GLfloat) * 4, &vertexes[0]);
    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 2, GL_FLOAT, false, sizeof(GLfloat) * 4, &vertexes[2]);

    GLfloat	mvp[16];
    esMatrixLoadIdentity((ESMatrix *)mvp);
    esOrtho((ESMatrix *)mvp, 0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
    esMatrixTranspose((ESMatrix *)mvp);
    GL_UniformMatrix4fv(SHADER_PARM_ADDR(modelViewProjectionMatrix), mvp);

    GL_Uniform1i(SHADER_PARMS_ADDR(u_uniformParm, 0), comp);

    qglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));

    GL_UseProgram(NULL);

    glPopAttrib();

    depthStencilRenderer.End();
}

/*
================
GLSphere - DG: from SteelStorm2
================
*/
void gluSphere(GLUquadricObj *, float r, int lats, int longs)
{
    int i, j;
    for(i = 0; i <= lats; i++)
    {
        float lat0 = idMath::PI  * (-0.5 + (float) (i - 1) / lats);
        float z0  = idMath::Sin(lat0);
        float zr0 =  idMath::Cos(lat0);
        float lat1 = idMath::PI * (-0.5 + (float) i / lats);
        float z1 = sin(lat1);
        float zr1 = cos(lat1);

        glBegin(GL_QUAD_STRIP);
        for(j = 0; j <= longs; j++)
        {
            float lng = 2 * idMath::PI * (float) (j - 1) / longs;
            float x = idMath::Cos(lng);
            float y = idMath::Sin(lng);

            glNormal3f(x * zr0, y * zr0, z0);
            glVertex3f(x * zr0, y * zr0, z0);
            glNormal3f(x * zr1, y * zr1, z1);
            glVertex3f(x * zr1, y * zr1, z1);
        }
        glEnd();
    }
}
