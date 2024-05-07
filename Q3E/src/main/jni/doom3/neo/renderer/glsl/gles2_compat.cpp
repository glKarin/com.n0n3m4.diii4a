#define glDepthRange(a, b) qglDepthRangef(a, b)

#define GL_COLOR_ARRAY				0x8076
#define GL_TEXTURE_COORD_ARRAY			0x8078

#define GL_POLYGON GL_TRIANGLE_FAN // GL_LINE_LOOP
#define GL_QUADS GL_TRIANGLE_FAN
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

#define CLIENT_STATE_VERTEX 1
#define CLIENT_STATE_TEXCOORD (1 << 1)
#define CLIENT_STATE_COLOR (1 << 2)

static GLenum gl_RenderType;
static GLfloat gl_TexCoord[2];
static GLfloat gl_Color[4] = {0, 0, 0, 1};
static idList<idDrawVert> gl_VertexList;
static idList<glIndex_t> gl_IndexList;
static GLenum gl_MatrixMode = GL_MODELVIEW;
static GLuint gl_ClientState = CLIENT_STATE_VERTEX | 0 | CLIENT_STATE_COLOR;
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
#undef _SETBS
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

// must call glPushMatrix first
static void glOrtho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearZ, GLfloat farZ)
{
	if(gl_MatrixMode == GL_PROJECTION)
	{
		if(gl_ProjectionMatrixStack.Empty())
		{
			Sys_Printf("[GLCompat]: must call glPushMatrix() first for ortho matrix!\n");
			return;
		}
		GLfloat m[16];
		memcpy(m, gl_ProjectionMatrix, sizeof(GLfloat) * 16);
		esOrtho((ESMatrix *)m, left, right, bottom, top, nearZ, farZ);
		gl_ProjectionMatrixStack.Set(m);
	}
	else
	{
		if(gl_ModelviewMatrixStack.Empty())
		{
			Sys_Printf("[GLCompat]: must call glPushMatrix() first for ortho matrix!\n");
			return;
		}
		GLfloat m[16];
		memcpy(m, gl_ModelviewMatrix, sizeof(GLfloat) * 16);
		esOrtho((ESMatrix *)m, left, right, bottom, top, nearZ, farZ);
		gl_ModelviewMatrixStack.Set(m);
	}
}

static void glrbFillVertex(idDrawVert &drawVert)
{
	drawVert.color[0] = (byte)(gl_Color[0] * 255.0f);
	drawVert.color[1] = (byte)(gl_Color[1] * 255.0f);
	drawVert.color[2] = (byte)(gl_Color[2] * 255.0f);
	drawVert.color[3] = (byte)(gl_Color[3] * 255.0f);
	drawVert.st.Set(gl_TexCoord[0], gl_TexCoord[1]);
}

static void glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	idDrawVert drawVert;
	drawVert.xyz.Set(x, y, z);
	glrbFillVertex(drawVert);
	gl_VertexList.Append(drawVert);
}

static void glVertex2f(GLfloat x, GLfloat y)
{
	glVertex3f(x, y, 0.0f);
}

static void glVertex3fv(const GLfloat v[3])
{
	glVertex3f(v[0], v[1], v[2]);
}

static void glTexCoord2f(GLfloat s, GLfloat t)
{
	gl_TexCoord[0] = s;
	gl_TexCoord[1] = t;
}

static void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	gl_Color[0] = r;
	gl_Color[1] = g;
	gl_Color[2] = b;
	gl_Color[3] = a;
}

static void glColor3f(GLfloat r, GLfloat g, GLfloat b)
{
	glColor4f(r, g, b, 1.0f);
}

static void glColor3fv(const GLfloat v[3])
{
	glColor3f(v[0], v[1], v[2]);
}

static void glColor4fv(const GLfloat v[4])
{
	glColor4f(v[0], v[1], v[2], v[3]);
}

static void glColor4ubv(const GLubyte v[4])
{
	glColor4f((float)v[0] / 255.0f, (float)v[1] / 255.0f, (float)v[2] / 255.0f, (float)v[3] / 255.0f);
}

static void glArrayElement(glIndex_t index)
{
	gl_IndexList.Append(index);
}

static void glDisableClientState(GLenum e)
{
	// `default` glsl shader must attr_Color is all [255, 255, 255, 255]
	if(e == GL_TEXTURE_COORD_ARRAY)
		gl_ClientState &= ~CLIENT_STATE_TEXCOORD;
	else if(e == GL_COLOR_ARRAY)
		gl_ClientState &= ~CLIENT_STATE_COLOR;
}

static void glEnableClientState(GLenum e)
{
	// `default` glsl shader must attr_Color is all [255, 255, 255, 255]
	if(e == GL_TEXTURE_COORD_ARRAY)
		gl_ClientState |= CLIENT_STATE_TEXCOORD;
	else if(e == GL_COLOR_ARRAY)
		gl_ClientState |= CLIENT_STATE_COLOR;
}

static GLboolean glrbClientStateIsEnabled(GLenum e)
{
	// `default` glsl shader must attr_Color is all [255, 255, 255, 255]
	if(e == GL_TEXTURE_COORD_ARRAY)
		return gl_ClientState & CLIENT_STATE_TEXCOORD ? GL_TRUE : GL_FALSE;
	else if(e == GL_COLOR_ARRAY)
		return gl_ClientState & CLIENT_STATE_COLOR ? GL_TRUE : GL_FALSE;
	else
		return GL_TRUE;
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
	if(gl_RenderType)
	{
		GLboolean usingTexCoord = glrbClientStateIsEnabled(GL_TEXTURE_COORD_ARRAY);
		int num = gl_VertexList.Num();
		int numIndex = gl_IndexList.Num();

		if(num > 0)
		{
			glrbStartRender();

			qglBindBuffer(GL_ARRAY_BUFFER, 0);
			GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));
			if(usingTexCoord)
				GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));

			GL_SelectTexture(0);
			if(!usingTexCoord)
				globalImages->whiteImage->Bind();

			GLfloat *vertex = (GLfloat *)malloc(sizeof(GLfloat) * num * 3);
			GLubyte *color = (GLubyte *)malloc(sizeof(GLubyte) * num * 4);
			//memset(color, 0xFF, sizeof(GLubyte) * num * 4);
			GLfloat *texcoord = NULL;
			if(usingTexCoord)
				texcoord = (GLfloat *)malloc(sizeof(GLfloat) * num * 2);
			for(int i = 0; i < num; i++)
			{
				const idDrawVert &drawVert = gl_VertexList[i];
				vertex[i * 3] = drawVert.xyz[0];
				vertex[i * 3 + 1] = drawVert.xyz[1];
				vertex[i * 3 + 2] = drawVert.xyz[2];
				color[i * 4] = drawVert.color[0];
				color[i * 4 + 1] = drawVert.color[1];
				color[i * 4 + 2] = drawVert.color[2];
				color[i * 4 + 3] = drawVert.color[3];
				if(usingTexCoord)
				{
					texcoord[i * 2] = drawVert.st[0];
					texcoord[i * 2 + 1] = drawVert.st[1];
				}
			}
			GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, 0, vertex);
			GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Color), 4, GL_UNSIGNED_BYTE, false, 0, color);
			if(usingTexCoord)
				GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, 0, texcoord);

			qglDrawArrays(gl_RenderType, 0, num);

			GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));
			if(usingTexCoord)
				GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));

			free(vertex);
			free(color);
			free(texcoord);

			glrbEndRender();
		}
		else if(numIndex > 0)
		{
			GLboolean usingColor = glrbClientStateIsEnabled(GL_COLOR_ARRAY);

			qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			GL_SelectTexture(0);
			if(!usingTexCoord)
				globalImages->whiteImage->Bind();

			glIndex_t *index = (glIndex_t *)malloc(sizeof(glIndex_t) * numIndex);
			for(int i = 0; i < numIndex; i++)
			{
				index[i] = gl_IndexList[i];
			}

			GLubyte *color = NULL;
			if(!usingColor)
			{
				GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));
				color = (GLubyte *)malloc(sizeof(GLubyte) * num * 4);
				GLubyte glColor[] = {
					(GLubyte)(gl_Color[0] * 255),
					(GLubyte)(gl_Color[1] * 255),
					(GLubyte)(gl_Color[2] * 255),
					(GLubyte)(gl_Color[3] * 255),
				};
				for(int i = 0; i < numIndex; i++)
				{
					index[i] = gl_IndexList[i];
					color[i * 4] = glColor[0];
					color[i * 4 + 1] = glColor[1];
					color[i * 4 + 2] = glColor[2];
					color[i * 4 + 3] = glColor[3];
				}
				GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Color), 4, GL_UNSIGNED_BYTE, false, 0, color);
			}

			qglDrawElements(gl_RenderType, numIndex, GL_INDEX_TYPE, index);

			free(index);
			free(color);
			if(!usingColor)
				GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));
		}
	}

	gl_VertexList.Clear();
	gl_IndexList.Clear();
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
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
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
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	end = Sys_Milliseconds();
	common->Printf("%i msec to draw all images\n", end - start);
}
