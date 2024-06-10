// DeclRenderProg.h
//

#pragma once

static const int PC_ATTRIB_INDEX_VERTEX = 0;
static const int PC_ATTRIB_INDEX_COLOR = 3;
static const int PC_ATTRIB_INDEX_COLOR2 = 4;
static const int PC_ATTRIB_INDEX_ST = 8;
static const int PC_ATTRIB_INDEX_TANGENT = 9;
static const int PC_ATTRIB_INDEX_BINORMAL = 10;
static const int PC_ATTRIB_INDEX_NORMAL = 11;

struct glslUniformLocation_t {
	int		parmIndex;
	GLint	uniformIndex;
	GLint   textureUnit;
};

//
// rvmDeclRenderProg
//
class rvmDeclRenderProg : public idDecl {
public:
	virtual size_t			Size(void) const;
	virtual bool			SetDefaultText(void);
	virtual const char* DefaultDefinition(void) const;
	virtual bool			Parse(const char* text, const int textLength);
	virtual void			FreeData(void);

	void					Bind(void);

	void					BindNull(void);
private:
	void					CreateVertexShader(idStr &bracketText);
	void					CreatePixelShader(idStr& bracketText);

	idStr					ParseRenderParms(idStr& bracketText, const char *programMacro);
private:
	int						LoadGLSLShader(GLenum target, idStr& programGLSL);
	void					LoadGLSLProgram(void);
private:
	idStr					vertexShader;
	int						vertexShaderHandle;

	idStr					pixelShader;
	int						pixelShaderHandle;

	GLuint					program;

	int						tmu;

	static int				currentRenderProgram;

	idList<rvmDeclRenderParam*> renderParams;
	idList<glslUniformLocation_t> uniformLocations;
	idList<int> uniformLocationUpdateId;

	idStr					declText;
	idStr					globalText;
};

// OpenGL is stupid reason #40004000 and too many.
ID_INLINE void R_SetupDrawVertBindings(void) {
	// enable the vertex arrays
	glEnableVertexAttribArrayARB(PC_ATTRIB_INDEX_ST);
	glEnableVertexAttribArrayARB(PC_ATTRIB_INDEX_COLOR);
	glEnableVertexAttribArrayARB(PC_ATTRIB_INDEX_TANGENT);
	glEnableVertexAttribArrayARB(PC_ATTRIB_INDEX_VERTEX);
	glEnableVertexAttribArrayARB(PC_ATTRIB_INDEX_NORMAL);
#if !defined(__ANDROID__) //karin: GLES programming pipeline
	glEnableClientState(GL_COLOR_ARRAY);
#endif

	idDrawVert* ac = nullptr;
#if !defined(__ANDROID__) //karin: GLES programming pipeline
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(idDrawVert), ac->color);
#endif
	glVertexAttribPointerARB(PC_ATTRIB_INDEX_NORMAL, 3, GL_FLOAT, false, sizeof(idDrawVert), ac->normal.ToFloatPtr());
	glVertexAttribPointerARB(PC_ATTRIB_INDEX_TANGENT, 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[0].ToFloatPtr());
	glVertexAttribPointerARB(PC_ATTRIB_INDEX_ST, 2, GL_FLOAT, false, sizeof(idDrawVert), ac->st.ToFloatPtr());
	glVertexAttribPointerARB(PC_ATTRIB_INDEX_VERTEX, 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
	glVertexAttribPointerARB(PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(idDrawVert), (void*)&ac->color);
}

ID_INLINE void R_UnbindDrawVertBindings(void) {
	glDisableVertexAttribArrayARB(PC_ATTRIB_INDEX_COLOR);
	glDisableVertexAttribArrayARB(PC_ATTRIB_INDEX_ST);
	glDisableVertexAttribArrayARB(PC_ATTRIB_INDEX_TANGENT);
	glDisableVertexAttribArrayARB(PC_ATTRIB_INDEX_VERTEX);
	glDisableVertexAttribArrayARB(PC_ATTRIB_INDEX_NORMAL);
#if !defined(__ANDROID__) //karin: GLES programming pipeline
	glDisableClientState(GL_COLOR_ARRAY);
#endif
}
