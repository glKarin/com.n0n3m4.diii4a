#define glDepthRange(a, b) qglDepthRangef(a, b)

#define GL_COLOR_ARRAY				0x8076
#define GL_TEXTURE_COORD_ARRAY			0x8078

#define GL_POLYGON GL_LINE_LOOP // GL_TRIANGLE_FAN
// #define GL_POLYGON				0x0009
#define GL_ALL_ATTRIB_BITS			0xFFFFFFFF

/* Matrix Mode */
#define GL_MODELVIEW				0x1700
#define GL_PROJECTION				0x1701


#define countof(x) (sizeof(x) / sizeof(x[0]))
#define SIZEOF_MATRIX (sizeof(GLfloat) * 16)
#define MATCPY(d, s) { if(d != s) memcpy((d), (s), SIZEOF_MATRIX); }

#define BACKEND_MODELVIEW_MATRIX (backEnd.viewDef->worldSpace.modelViewMatrix)
#define BACKEND_PROJECTION_MATRIX (backEnd.viewDef->projectionMatrix)

static GLenum gl_RenderType;
static GLfloat gl_Color[4] = {0, 0, 0, 1};
static idList<idVec3> gl_VertexList;
static GLenum gl_MatrixMode = GL_MODELVIEW;
static GLuint gl_ClientState = 1 | 0 | 4;
static const float	GL_IDENTITY_MATRIX[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
};

struct GLstate
{
	GLboolean TEXTURE_2D;
	GLboolean DEPTH_TEST;
	GLboolean CULL_FACE;
	GLboolean SCISSOR_TEST;
};

template <int SIZE>
struct GLstate_stack
{
public:
	GLstate_stack()
			: num(0)
	{}

	GLstate * Push(const GLstate *state = NULL) {
		if(num >= SIZE)
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
			Sys_Printf("[GLCompat]: GLstate_stack::Push under\n");
			return NULL;
		}
		GLstate *ret = &stack[num--];
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
	GLmatrix_stack()
	: num(0)
	{}

	void Push(const GLfloat m[16]) {
		if(num >= SIZE)
		{
			Sys_Printf("[GLCompat]: GLmatrix_stack::Push over\n");
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
		if(num <= 0)
		{
			Sys_Printf("[GLCompat]: GLmatrix_stack::Push under\n");
			return;
		}
		num--;
	}

	GLfloat * Top(void) {
		if(num > 0)
			return stack[num - 1];
		else
		{
			Sys_Printf("[GLCompat]: GLmatrix_stack::Top under\n");
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
			Sys_Printf("[GLCompat]: GLmatrix_stack::Set under\n");
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

	GLfloat stack[16][SIZE];
	int num;
};
static GLmatrix_stack<2> gl_ProjectionMatrixStack;
static GLmatrix_stack<8> gl_ModelviewMatrixStack; // 64

#define gl_ModelviewMatrix (gl_ModelviewMatrixStack.Top(BACKEND_MODELVIEW_MATRIX))
#define gl_ProjectionMatrix (gl_ProjectionMatrixStack.Top(BACKEND_PROJECTION_MATRIX))

static GLstate_stack<2> gl_StateStack; // 16

static void glPushAttrib(GLint mask)
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
#undef _GETBS
}

static void glPopAttrib(void)
{
	GLstate *state = gl_StateStack.Pop();
	if(!state)
		return;
#define _SETBS(x) { if(state->x) qglEnable(GL_##x); else qglDisable(GL_##x); }
	_SETBS(TEXTURE_2D);
	_SETBS(DEPTH_TEST);
	_SETBS(CULL_FACE);
	_SETBS(SCISSOR_TEST);
#undef _SETBS;
}

static void glPushMatrix(void)
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

static void glPopMatrix(void)
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

static void glMatrixMode(GLenum mode)
{
	gl_MatrixMode = mode;
}

// must call glPushMatrix first
static void glLoadIdentity(void)
{
	if(gl_MatrixMode == GL_PROJECTION)
	{
		if(gl_ProjectionMatrixStack.Empty())
		{
			Sys_Printf("[GLCompat]: must call glPushMatrix() first for identity projection matrix!\n");
			return;
		}
		gl_ProjectionMatrixStack.Identity();
	}
	else
	{
		if(gl_ModelviewMatrixStack.Empty())
		{
			Sys_Printf("[GLCompat]: must call glPushMatrix() first for identity modelview matrix!\n");
			return;
		}
		gl_ModelviewMatrixStack.Identity();
	}
}

static void glVertex3f(const GLfloat x, GLfloat y, GLfloat z)
{
	gl_VertexList.Append(idVec3(x, y, z));
}

static void glVertex3fv(const GLfloat *v)
{
	gl_VertexList.Append(idVec3(v[0], v[1], v[2]));
}

static void glColor3f(GLfloat r, GLfloat g, GLfloat b)
{
	gl_Color[0] = r;
	gl_Color[1] = g;
	gl_Color[2] = b;
	gl_Color[3] = 1;
}

static void glColor3fv(const GLfloat *v)
{
	gl_Color[0] = v[0];
	gl_Color[1] = v[1];
	gl_Color[2] = v[2];
	gl_Color[3] = 1;
}

static void glColor4fv(const GLfloat *v)
{
	gl_Color[0] = v[0];
	gl_Color[1] = v[1];
	gl_Color[2] = v[2];
	gl_Color[3] = v[3];
}

static void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	gl_Color[0] = r;
	gl_Color[1] = g;
	gl_Color[2] = b;
	gl_Color[3] = a;
}

static void glColor4ubv(const GLubyte *v)
{
	gl_Color[0] = (float)v[0] / 255.0f;
	gl_Color[1] = (float)v[1] / 255.0f;
	gl_Color[2] = (float)v[2] / 255.0f;
	gl_Color[3] = (float)v[3] / 255.0f;
}

static void qglDisableClientState(int i)
{
	//gl_ClientState &= ~(1 << i); // `default` glsl shader must attr_Color is all [255, 255, 255, 255]
}

// must call glPushMatrix first
static void glLoadMatrixf(const GLfloat matrix[16])
{
	if(gl_MatrixMode == GL_PROJECTION)
	{
		if(gl_ProjectionMatrixStack.Empty())
		{
			Sys_Printf("[GLCompat]: must call glPushMatrix() first for load projection matrix!\n");
			return;
		}
		gl_ProjectionMatrixStack.Set(matrix);
	}
	else
	{
		if(gl_ModelviewMatrixStack.Empty())
		{
			Sys_Printf("[GLCompat]: must call glPushMatrix() first for load modelview matrix!\n");
			return;
		}
		gl_ModelviewMatrixStack.Set(matrix);
	}
}

// must call glrbStartRender first
static void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), size, type, false, stride, pointer);
}

static void glBegin(GLenum t)
{
	gl_RenderType = t;
}

static void glrbStartRender(void)
{
	GL_UseProgram(&defaultShader);
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));

	GL_Uniform4fv(offsetof(shaderProgram_t, glColor), gl_Color);
	const GLfloat zero[4] = {0, 0, 0, 0};
	GL_Uniform4fv(offsetof(shaderProgram_t, colorAdd), zero);
	const GLfloat one[4] = {1, 1, 1, 1};
	GL_Uniform4fv(offsetof(shaderProgram_t, colorModulate), one);

	GLfloat	gl_MVPMatrix[16];
	myGlMultMatrix(gl_ModelviewMatrix, gl_ProjectionMatrix, gl_MVPMatrix);

	GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), gl_MVPMatrix);

	//Sys_Printf("Current shader program: %p\n", backEnd.glState.currentProgram);
}

static void glrbEndRender(void)
{
	//Sys_Printf("glrbEndRender shader program: %p\n", backEnd.glState.currentProgram);
	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
	GL_UseProgram(NULL);
}

// draw func
static void glEnd()
{
	int num = gl_VertexList.Num();
	if(gl_RenderType && num)
	{
		glrbStartRender();

		qglBindBuffer(GL_ARRAY_BUFFER, 0);
		GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));

		globalImages->whiteImage->Bind();

		GLfloat *vertex = (GLfloat *)malloc(sizeof(GLfloat) * num * 3);
		GLubyte *color = (GLubyte *)malloc(sizeof(GLubyte) * num * 4);
		memset(color, 0xFF, sizeof(GLubyte) * num * 4);
		for(int i = 0; i < num; i++)
		{
			const idVec3 &v3 = gl_VertexList[i];
			vertex[i * 3] = v3[0];
			vertex[i * 3 + 1] = v3[1];
			vertex[i * 3 + 2] = v3[2];
		}
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, 0, vertex);
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Color), 4, GL_UNSIGNED_BYTE, false, 0, color);
		qglDrawArrays(gl_RenderType, 0, num);
		free(vertex);
		free(color);

		GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));
		glrbEndRender();
	}

	gl_VertexList.Clear();
	gl_RenderType = 0;
}

static void glrbDebugRenderCompat(void)
{
	Sys_Printf("----- glrbDebugRenderCompat -----\n");
	Sys_Printf("Projection matrix stack: %d\n", gl_ProjectionMatrixStack.Num());
	Sys_Printf("Modelview matrix stack: %d\n", gl_ModelviewMatrixStack.Num());
	Sys_Printf("Attrib stack: %d\n", gl_StateStack.Num());
	Sys_Printf("MatrixMode: 0x%X\n", gl_MatrixMode);
	Sys_Printf("Color: %.6f, %.6f, %.6f, %.6f\n", gl_Color[0], gl_Color[1], gl_Color[2], gl_Color[3]);
	Sys_Printf("\n");
}

#define DEBUG_RENDER_COMPAT
// #define DEBUG_RENDER_COMPAT glrbDebugRenderCompat();
