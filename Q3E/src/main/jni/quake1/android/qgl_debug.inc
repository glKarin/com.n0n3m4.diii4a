#if 1
#define QGLPRINT(x) x
#else
#define QGLPRINT(x)
#endif

static void R_ClearGLErrors(void) {
	while(__glGetError() != GL_NO_ERROR);
}

void qglActiveTexture(GLenum texture) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glActiveTexture(" "0X%04X" ")" , texture));
	(void) __glActiveTexture(texture);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglAttachShader(GLuint program, GLuint shader) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glAttachShader(" "%u, %u" ")" , program, shader));
	(void) __glAttachShader(program, shader);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglBindAttribLocation(GLuint program, GLuint index, const GLchar *name) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glBindAttribLocation(" "%u, %u, %s" ")" , program, index, name));
	(void) __glBindAttribLocation(program, index, name);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglBindBuffer(GLenum target, GLuint buffer) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glBindBuffer(" "0X%04X, %u" ")" , target, buffer));
	(void) __glBindBuffer(target, buffer);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglBindFramebuffer(GLenum target, GLuint framebuffer) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glBindFramebuffer(" "0X%04X, %u" ")" , target, framebuffer));
	(void) __glBindFramebuffer(target, framebuffer);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglBindRenderbuffer(GLenum target, GLuint renderbuffer) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glBindRenderbuffer(" "0X%04X, %u" ")" , target, renderbuffer));
	(void) __glBindRenderbuffer(target, renderbuffer);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglBindTexture(GLenum target, GLuint texture) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glBindTexture(" "0X%04X, %u" ")" , target, texture));
	(void) __glBindTexture(target, texture);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glBlendColor(" "%f, %f, %f, %f" ")" , red, green, blue, alpha));
	(void) __glBlendColor(red, green, blue, alpha);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglBlendEquation(GLenum mode) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glBlendEquation(" "0X%04X" ")" , mode));
	(void) __glBlendEquation(mode);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glBlendEquationSeparate(" "0X%04X, 0X%04X" ")" , modeRGB, modeAlpha));
	(void) __glBlendEquationSeparate(modeRGB, modeAlpha);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglBlendFunc(GLenum sfactor, GLenum dfactor) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glBlendFunc(" "0X%04X, 0X%04X" ")" , sfactor, dfactor));
	(void) __glBlendFunc(sfactor, dfactor);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glBlendFuncSeparate(" "0X%04X, 0X%04X, 0X%04X, 0X%04X" ")" , sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha));
	(void) __glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glBufferData(" "0X%04X, %ld, %p, 0X%04X" ")" , target, size, data, usage));
	(void) __glBufferData(target, size, data, usage);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glBufferSubData(" "0X%04X, %lu, %ld, %p" ")" , target, offset, size, data));
	(void) __glBufferSubData(target, offset, size, data);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

GLenum qglCheckFramebufferStatus(GLenum target) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glCheckFramebufferStatus(" "0X%04X" ")" , target));
	GLenum _ret = __glCheckFramebufferStatus(target);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" -> 0X%04X (GLenum)\n", _ret));
	}
	return _ret;
}

void qglClear(GLbitfield mask) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glClear(" "0x%x" ")" , mask));
	(void) __glClear(mask);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glClearColor(" "%f, %f, %f, %f" ")" , red, green, blue, alpha));
	(void) __glClearColor(red, green, blue, alpha);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglClearDepthf(GLfloat d) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glClearDepthf(" "%f" ")" , d));
	(void) __glClearDepthf(d);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglClearStencil(GLint s) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glClearStencil(" "%d" ")" , s));
	(void) __glClearStencil(s);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glColorMask(" "%d, %d, %d, %d" ")" , red, green, blue, alpha));
	(void) __glColorMask(red, green, blue, alpha);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglCompileShader(GLuint shader) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glCompileShader(" "%u" ")" , shader));
	(void) __glCompileShader(shader);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glCompressedTexImage2D(" "0X%04X, %d, 0X%04X, %u, %u, %d, %u, %p" ")" , target, level, internalformat, width, height, border, imageSize, data));
	(void) __glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glCompressedTexSubImage2D(" "0X%04X, %d, %d, %d, %u, %u, 0X%04X, %u, %p" ")" , target, level, xoffset, yoffset, width, height, format, imageSize, data));
	(void) __glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glCopyTexImage2D(" "0X%04X, %d, 0X%04X, %d, %d, %u, %u, %d" ")" , target, level, internalformat, x, y, width, height, border));
	(void) __glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glCopyTexSubImage2D(" "0X%04X, %d, %d, %d, %d, %d, %u, %u" ")" , target, level, xoffset, yoffset, x, y, width, height));
	(void) __glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

GLuint qglCreateProgram(void) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glCreateProgram("  ")" ));
	GLuint _ret = __glCreateProgram();
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" -> %u (GLuint)\n", _ret));
	}
	return _ret;
}

GLuint qglCreateShader(GLenum type) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glCreateShader(" "0X%04X" ")" , type));
	GLuint _ret = __glCreateShader(type);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" -> %u (GLuint)\n", _ret));
	}
	return _ret;
}

void qglCullFace(GLenum mode) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glCullFace(" "0X%04X" ")" , mode));
	(void) __glCullFace(mode);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglDeleteBuffers(GLsizei n, const GLuint *buffers) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glDeleteBuffers(" "%u, %p" ")" , n, buffers));
	(void) __glDeleteBuffers(n, buffers);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglDeleteFramebuffers(GLsizei n, const GLuint *framebuffers) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glDeleteFramebuffers(" "%u, %p" ")" , n, framebuffers));
	(void) __glDeleteFramebuffers(n, framebuffers);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglDeleteProgram(GLuint program) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glDeleteProgram(" "%u" ")" , program));
	(void) __glDeleteProgram(program);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glDeleteRenderbuffers(" "%u, %p" ")" , n, renderbuffers));
	(void) __glDeleteRenderbuffers(n, renderbuffers);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglDeleteShader(GLuint shader) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glDeleteShader(" "%u" ")" , shader));
	(void) __glDeleteShader(shader);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglDeleteTextures(GLsizei n, const GLuint *textures) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glDeleteTextures(" "%u, %p" ")" , n, textures));
	(void) __glDeleteTextures(n, textures);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglDepthFunc(GLenum func) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glDepthFunc(" "0X%04X" ")" , func));
	(void) __glDepthFunc(func);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglDepthMask(GLboolean flag) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glDepthMask(" "%d" ")" , flag));
	(void) __glDepthMask(flag);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglDepthRangef(GLfloat n, GLfloat f) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glDepthRangef(" "%f, %f" ")" , n, f));
	(void) __glDepthRangef(n, f);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglDetachShader(GLuint program, GLuint shader) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glDetachShader(" "%u, %u" ")" , program, shader));
	(void) __glDetachShader(program, shader);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglDisable(GLenum cap) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glDisable(" "0X%04X" ")" , cap));
	(void) __glDisable(cap);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglDisableVertexAttribArray(GLuint index) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glDisableVertexAttribArray(" "%u" ")" , index));
	(void) __glDisableVertexAttribArray(index);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglDrawArrays(GLenum mode, GLint first, GLsizei count) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glDrawArrays(" "0X%04X, %d, %u" ")" , mode, first, count));
	(void) __glDrawArrays(mode, first, count);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glDrawElements(" "0X%04X, %u, 0X%04X, %p" ")" , mode, count, type, indices));
	(void) __glDrawElements(mode, count, type, indices);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglEnable(GLenum cap) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glEnable(" "0X%04X" ")" , cap));
	(void) __glEnable(cap);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglEnableVertexAttribArray(GLuint index) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glEnableVertexAttribArray(" "%u" ")" , index));
	(void) __glEnableVertexAttribArray(index);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglFinish(void) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glFinish("  ")" ));
	(void) __glFinish();
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglFlush(void) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glFlush("  ")" ));
	(void) __glFlush();
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glFramebufferRenderbuffer(" "0X%04X, 0X%04X, 0X%04X, %u" ")" , target, attachment, renderbuffertarget, renderbuffer));
	(void) __glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glFramebufferTexture2D(" "0X%04X, 0X%04X, 0X%04X, %u, %d" ")" , target, attachment, textarget, texture, level));
	(void) __glFramebufferTexture2D(target, attachment, textarget, texture, level);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglFrontFace(GLenum mode) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glFrontFace(" "0X%04X" ")" , mode));
	(void) __glFrontFace(mode);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGenBuffers(GLsizei n, GLuint *buffers) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGenBuffers(" "%u, %p" ")" , n, buffers));
	(void) __glGenBuffers(n, buffers);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGenerateMipmap(GLenum target) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGenerateMipmap(" "0X%04X" ")" , target));
	(void) __glGenerateMipmap(target);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGenFramebuffers(GLsizei n, GLuint *framebuffers) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGenFramebuffers(" "%u, %p" ")" , n, framebuffers));
	(void) __glGenFramebuffers(n, framebuffers);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGenRenderbuffers(GLsizei n, GLuint *renderbuffers) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGenRenderbuffers(" "%u, %p" ")" , n, renderbuffers));
	(void) __glGenRenderbuffers(n, renderbuffers);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGenTextures(GLsizei n, GLuint *textures) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGenTextures(" "%u, %p" ")" , n, textures));
	(void) __glGenTextures(n, textures);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetActiveAttrib(" "%u, %u, %u, %p, %p, %p, %s" ")" , program, index, bufSize, length, size, type, name));
	(void) __glGetActiveAttrib(program, index, bufSize, length, size, type, name);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetActiveUniform(" "%u, %u, %u, %p, %p, %p, %s" ")" , program, index, bufSize, length, size, type, name));
	(void) __glGetActiveUniform(program, index, bufSize, length, size, type, name);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetAttachedShaders(" "%u, %u, %p, %p" ")" , program, maxCount, count, shaders));
	(void) __glGetAttachedShaders(program, maxCount, count, shaders);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

GLint qglGetAttribLocation(GLuint program, const GLchar *name) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetAttribLocation(" "%u, %s" ")" , program, name));
	GLint _ret = __glGetAttribLocation(program, name);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" -> %d (GLint)\n", _ret));
	}
	return _ret;
}

void qglGetBooleanv(GLenum pname, GLboolean *data) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetBooleanv(" "0X%04X, %p" ")" , pname, data));
	(void) __glGetBooleanv(pname, data);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetBufferParameteriv(GLenum target, GLenum pname, GLint *params) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetBufferParameteriv(" "0X%04X, 0X%04X, %p" ")" , target, pname, params));
	(void) __glGetBufferParameteriv(target, pname, params);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

GLenum qglGetError(void) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetError("  ")" ));
	GLenum _ret = __glGetError();
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" -> 0X%04X (GLenum)\n", _ret));
	}
	return _ret;
}

void qglGetFloatv(GLenum pname, GLfloat *data) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetFloatv(" "0X%04X, %p" ")" , pname, data));
	(void) __glGetFloatv(pname, data);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint *params) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetFramebufferAttachmentParameteriv(" "0X%04X, 0X%04X, 0X%04X, %p" ")" , target, attachment, pname, params));
	(void) __glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetIntegerv(GLenum pname, GLint *data) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetIntegerv(" "0X%04X, %p" ")" , pname, data));
	(void) __glGetIntegerv(pname, data);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetProgramiv(GLuint program, GLenum pname, GLint *params) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetProgramiv(" "%u, 0X%04X, %p" ")" , program, pname, params));
	(void) __glGetProgramiv(program, pname, params);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetProgramInfoLog(" "%u, %u, %p, %s" ")" , program, bufSize, length, infoLog));
	(void) __glGetProgramInfoLog(program, bufSize, length, infoLog);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetRenderbufferParameteriv(" "0X%04X, 0X%04X, %p" ")" , target, pname, params));
	(void) __glGetRenderbufferParameteriv(target, pname, params);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetShaderiv(GLuint shader, GLenum pname, GLint *params) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetShaderiv(" "%u, 0X%04X, %p" ")" , shader, pname, params));
	(void) __glGetShaderiv(shader, pname, params);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetShaderInfoLog(" "%u, %u, %p, %s" ")" , shader, bufSize, length, infoLog));
	(void) __glGetShaderInfoLog(shader, bufSize, length, infoLog);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetShaderPrecisionFormat(" "0X%04X, 0X%04X, %p, %p" ")" , shadertype, precisiontype, range, precision));
	(void) __glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetShaderSource(" "%u, %u, %p, %s" ")" , shader, bufSize, length, source));
	(void) __glGetShaderSource(shader, bufSize, length, source);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

const GLubyte * qglGetString(GLenum name) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetString(" "0X%04X" ")" , name));
	const GLubyte * _ret = __glGetString(name);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" -> %p (const GLubyte *)\n", _ret));
	}
	return _ret;
}

void qglGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetTexParameterfv(" "0X%04X, 0X%04X, %p" ")" , target, pname, params));
	(void) __glGetTexParameterfv(target, pname, params);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetTexParameteriv(GLenum target, GLenum pname, GLint *params) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetTexParameteriv(" "0X%04X, 0X%04X, %p" ")" , target, pname, params));
	(void) __glGetTexParameteriv(target, pname, params);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetUniformfv(GLuint program, GLint location, GLfloat *params) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetUniformfv(" "%u, %d, %p" ")" , program, location, params));
	(void) __glGetUniformfv(program, location, params);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetUniformiv(GLuint program, GLint location, GLint *params) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetUniformiv(" "%u, %d, %p" ")" , program, location, params));
	(void) __glGetUniformiv(program, location, params);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

GLint qglGetUniformLocation(GLuint program, const GLchar *name) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetUniformLocation(" "%u, %s" ")" , program, name));
	GLint _ret = __glGetUniformLocation(program, name);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" -> %d (GLint)\n", _ret));
	}
	return _ret;
}

void qglGetVertexAttribfv(GLuint index, GLenum pname, GLfloat *params) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetVertexAttribfv(" "%u, 0X%04X, %p" ")" , index, pname, params));
	(void) __glGetVertexAttribfv(index, pname, params);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetVertexAttribiv(GLuint index, GLenum pname, GLint *params) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetVertexAttribiv(" "%u, 0X%04X, %p" ")" , index, pname, params));
	(void) __glGetVertexAttribiv(index, pname, params);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglGetVertexAttribPointerv(GLuint index, GLenum pname, void **pointer) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glGetVertexAttribPointerv(" "%u, 0X%04X, %p" ")" , index, pname, pointer));
	(void) __glGetVertexAttribPointerv(index, pname, pointer);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglHint(GLenum target, GLenum mode) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glHint(" "0X%04X, 0X%04X" ")" , target, mode));
	(void) __glHint(target, mode);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

GLboolean qglIsBuffer(GLuint buffer) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glIsBuffer(" "%u" ")" , buffer));
	GLboolean _ret = __glIsBuffer(buffer);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" -> %d (GLboolean)\n", _ret));
	}
	return _ret;
}

GLboolean qglIsEnabled(GLenum cap) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glIsEnabled(" "0X%04X" ")" , cap));
	GLboolean _ret = __glIsEnabled(cap);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" -> %d (GLboolean)\n", _ret));
	}
	return _ret;
}

GLboolean qglIsFramebuffer(GLuint framebuffer) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glIsFramebuffer(" "%u" ")" , framebuffer));
	GLboolean _ret = __glIsFramebuffer(framebuffer);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" -> %d (GLboolean)\n", _ret));
	}
	return _ret;
}

GLboolean qglIsProgram(GLuint program) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glIsProgram(" "%u" ")" , program));
	GLboolean _ret = __glIsProgram(program);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" -> %d (GLboolean)\n", _ret));
	}
	return _ret;
}

GLboolean qglIsRenderbuffer(GLuint renderbuffer) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glIsRenderbuffer(" "%u" ")" , renderbuffer));
	GLboolean _ret = __glIsRenderbuffer(renderbuffer);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" -> %d (GLboolean)\n", _ret));
	}
	return _ret;
}

GLboolean qglIsShader(GLuint shader) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glIsShader(" "%u" ")" , shader));
	GLboolean _ret = __glIsShader(shader);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" -> %d (GLboolean)\n", _ret));
	}
	return _ret;
}

GLboolean qglIsTexture(GLuint texture) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glIsTexture(" "%u" ")" , texture));
	GLboolean _ret = __glIsTexture(texture);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" -> %d (GLboolean)\n", _ret));
	}
	return _ret;
}

void qglLineWidth(GLfloat width) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glLineWidth(" "%f" ")" , width));
	(void) __glLineWidth(width);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglLinkProgram(GLuint program) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glLinkProgram(" "%u" ")" , program));
	(void) __glLinkProgram(program);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglPixelStorei(GLenum pname, GLint param) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glPixelStorei(" "0X%04X, %d" ")" , pname, param));
	(void) __glPixelStorei(pname, param);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglPolygonOffset(GLfloat factor, GLfloat units) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glPolygonOffset(" "%f, %f" ")" , factor, units));
	(void) __glPolygonOffset(factor, units);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glReadPixels(" "%d, %d, %u, %u, 0X%04X, 0X%04X, %p" ")" , x, y, width, height, format, type, pixels));
	(void) __glReadPixels(x, y, width, height, format, type, pixels);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglReleaseShaderCompiler(void) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glReleaseShaderCompiler("  ")" ));
	(void) __glReleaseShaderCompiler();
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glRenderbufferStorage(" "0X%04X, 0X%04X, %u, %u" ")" , target, internalformat, width, height));
	(void) __glRenderbufferStorage(target, internalformat, width, height);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglSampleCoverage(GLfloat value, GLboolean invert) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glSampleCoverage(" "%f, %d" ")" , value, invert));
	(void) __glSampleCoverage(value, invert);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glScissor(" "%d, %d, %u, %u" ")" , x, y, width, height));
	(void) __glScissor(x, y, width, height);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglShaderBinary(GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glShaderBinary(" "%u, %p, 0X%04X, %p, %u" ")" , count, shaders, binaryformat, binary, length));
	(void) __glShaderBinary(count, shaders, binaryformat, binary, length);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglShaderSource(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glShaderSource(" "%u, %u, %p, %p" ")" , shader, count, string, length));
	(void) __glShaderSource(shader, count, string, length);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglStencilFunc(GLenum func, GLint ref, GLuint mask) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glStencilFunc(" "0X%04X, %d, %u" ")" , func, ref, mask));
	(void) __glStencilFunc(func, ref, mask);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glStencilFuncSeparate(" "0X%04X, 0X%04X, %d, %u" ")" , face, func, ref, mask));
	(void) __glStencilFuncSeparate(face, func, ref, mask);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglStencilMask(GLuint mask) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glStencilMask(" "%u" ")" , mask));
	(void) __glStencilMask(mask);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglStencilMaskSeparate(GLenum face, GLuint mask) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glStencilMaskSeparate(" "0X%04X, %u" ")" , face, mask));
	(void) __glStencilMaskSeparate(face, mask);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glStencilOp(" "0X%04X, 0X%04X, 0X%04X" ")" , fail, zfail, zpass));
	(void) __glStencilOp(fail, zfail, zpass);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glStencilOpSeparate(" "0X%04X, 0X%04X, 0X%04X, 0X%04X" ")" , face, sfail, dpfail, dppass));
	(void) __glStencilOpSeparate(face, sfail, dpfail, dppass);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glTexImage2D(" "0X%04X, %d, %d, %u, %u, %d, 0X%04X, 0X%04X, %p" ")" , target, level, internalformat, width, height, border, format, type, pixels));
	(void) __glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglTexParameterf(GLenum target, GLenum pname, GLfloat param) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glTexParameterf(" "0X%04X, 0X%04X, %f" ")" , target, pname, param));
	(void) __glTexParameterf(target, pname, param);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glTexParameterfv(" "0X%04X, 0X%04X, %p" ")" , target, pname, params));
	(void) __glTexParameterfv(target, pname, params);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglTexParameteri(GLenum target, GLenum pname, GLint param) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glTexParameteri(" "0X%04X, 0X%04X, %d" ")" , target, pname, param));
	(void) __glTexParameteri(target, pname, param);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglTexParameteriv(GLenum target, GLenum pname, const GLint *params) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glTexParameteriv(" "0X%04X, 0X%04X, %p" ")" , target, pname, params));
	(void) __glTexParameteriv(target, pname, params);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glTexSubImage2D(" "0X%04X, %d, %d, %d, %u, %u, 0X%04X, 0X%04X, %p" ")" , target, level, xoffset, yoffset, width, height, format, type, pixels));
	(void) __glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniform1f(GLint location, GLfloat v0) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniform1f(" "%d, %f" ")" , location, v0));
	(void) __glUniform1f(location, v0);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniform1fv(GLint location, GLsizei count, const GLfloat *value) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniform1fv(" "%d, %u, %p" ")" , location, count, value));
	(void) __glUniform1fv(location, count, value);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniform1i(GLint location, GLint v0) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniform1i(" "%d, %d" ")" , location, v0));
	(void) __glUniform1i(location, v0);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniform1iv(GLint location, GLsizei count, const GLint *value) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniform1iv(" "%d, %u, %p" ")" , location, count, value));
	(void) __glUniform1iv(location, count, value);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniform2f(GLint location, GLfloat v0, GLfloat v1) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniform2f(" "%d, %f, %f" ")" , location, v0, v1));
	(void) __glUniform2f(location, v0, v1);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniform2fv(GLint location, GLsizei count, const GLfloat *value) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniform2fv(" "%d, %u, %p" ")" , location, count, value));
	(void) __glUniform2fv(location, count, value);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniform2i(GLint location, GLint v0, GLint v1) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniform2i(" "%d, %d, %d" ")" , location, v0, v1));
	(void) __glUniform2i(location, v0, v1);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniform2iv(GLint location, GLsizei count, const GLint *value) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniform2iv(" "%d, %u, %p" ")" , location, count, value));
	(void) __glUniform2iv(location, count, value);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniform3f(" "%d, %f, %f, %f" ")" , location, v0, v1, v2));
	(void) __glUniform3f(location, v0, v1, v2);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniform3fv(GLint location, GLsizei count, const GLfloat *value) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniform3fv(" "%d, %u, %p" ")" , location, count, value));
	(void) __glUniform3fv(location, count, value);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniform3i(" "%d, %d, %d, %d" ")" , location, v0, v1, v2));
	(void) __glUniform3i(location, v0, v1, v2);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniform3iv(GLint location, GLsizei count, const GLint *value) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniform3iv(" "%d, %u, %p" ")" , location, count, value));
	(void) __glUniform3iv(location, count, value);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniform4f(" "%d, %f, %f, %f, %f" ")" , location, v0, v1, v2, v3));
	(void) __glUniform4f(location, v0, v1, v2, v3);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniform4fv(GLint location, GLsizei count, const GLfloat *value) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniform4fv(" "%d, %u, %p" ")" , location, count, value));
	(void) __glUniform4fv(location, count, value);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniform4i(" "%d, %d, %d, %d, %d" ")" , location, v0, v1, v2, v3));
	(void) __glUniform4i(location, v0, v1, v2, v3);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniform4iv(GLint location, GLsizei count, const GLint *value) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniform4iv(" "%d, %u, %p" ")" , location, count, value));
	(void) __glUniform4iv(location, count, value);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniformMatrix2fv(" "%d, %u, %d, %p" ")" , location, count, transpose, value));
	(void) __glUniformMatrix2fv(location, count, transpose, value);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniformMatrix3fv(" "%d, %u, %d, %p" ")" , location, count, transpose, value));
	(void) __glUniformMatrix3fv(location, count, transpose, value);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUniformMatrix4fv(" "%d, %u, %d, %p" ")" , location, count, transpose, value));
	(void) __glUniformMatrix4fv(location, count, transpose, value);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglUseProgram(GLuint program) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glUseProgram(" "%u" ")" , program));
	(void) __glUseProgram(program);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglValidateProgram(GLuint program) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glValidateProgram(" "%u" ")" , program));
	(void) __glValidateProgram(program);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglVertexAttrib1f(GLuint index, GLfloat x) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glVertexAttrib1f(" "%u, %f" ")" , index, x));
	(void) __glVertexAttrib1f(index, x);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglVertexAttrib1fv(GLuint index, const GLfloat *v) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glVertexAttrib1fv(" "%u, %p" ")" , index, v));
	(void) __glVertexAttrib1fv(index, v);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglVertexAttrib2f(GLuint index, GLfloat x, GLfloat y) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glVertexAttrib2f(" "%u, %f, %f" ")" , index, x, y));
	(void) __glVertexAttrib2f(index, x, y);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglVertexAttrib2fv(GLuint index, const GLfloat *v) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glVertexAttrib2fv(" "%u, %p" ")" , index, v));
	(void) __glVertexAttrib2fv(index, v);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glVertexAttrib3f(" "%u, %f, %f, %f" ")" , index, x, y, z));
	(void) __glVertexAttrib3f(index, x, y, z);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglVertexAttrib3fv(GLuint index, const GLfloat *v) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glVertexAttrib3fv(" "%u, %p" ")" , index, v));
	(void) __glVertexAttrib3fv(index, v);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glVertexAttrib4f(" "%u, %f, %f, %f, %f" ")" , index, x, y, z, w));
	(void) __glVertexAttrib4f(index, x, y, z, w);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglVertexAttrib4fv(GLuint index, const GLfloat *v) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glVertexAttrib4fv(" "%u, %p" ")" , index, v));
	(void) __glVertexAttrib4fv(index, v);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glVertexAttribPointer(" "%u, %d, 0X%04X, %d, %u, %p" ")" , index, size, type, normalized, stride, pointer));
	(void) __glVertexAttribPointer(index, size, type, normalized, stride, pointer);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}

void qglViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
	R_ClearGLErrors();
	QGLPRINT(printf("GLCALL: glViewport(" "%d, %d, %u, %u" ")" , x, y, width, height));
	(void) __glViewport(x, y, width, height);
	GLenum err = __glGetError();
	if(err != GL_NO_ERROR) {
		printf(" GLERROR -> %X\n", err);
	} else {
		QGLPRINT(printf(" |\n"));
	}
	return;
}
