#ifndef _GLES2_COMPAT_H
#define _GLES2_COMPAT_H

#ifndef GL_COLOR_ARRAY
#define GL_COLOR_ARRAY				0x8076
#endif
#ifndef GL_TEXTURE_COORD_ARRAY
#define GL_TEXTURE_COORD_ARRAY			0x8078
#endif

#ifdef GL_POLYGON
#undef GL_POLYGON
#endif
#define GL_POLYGON GL_TRIANGLE_FAN // GL_LINE_LOOP
#ifdef GL_QUADS
#undef GL_QUADS
#endif
#define GL_QUADS GL_TRIANGLE_FAN
// #define GL_POLYGON				0x0009
#ifndef GL_ALL_ATTRIB_BITS
#define GL_ALL_ATTRIB_BITS			0xFFFFFFFF
#endif

/* Matrix Mode */
#ifndef GL_MODELVIEW
#define GL_MODELVIEW				0x1700
#endif
#ifndef GL_PROJECTION
#define GL_PROJECTION				0x1701
#endif


void glDisableClientState(GLenum e);
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glLoadMatrixf(const GLfloat matrix[16]);
void glBegin(GLenum t);
void glEnd(void);
void glVertex3f(GLfloat x, GLfloat y, GLfloat z);

void glrbShutdown(void);

#if 0
void glVertex2f(GLfloat x, GLfloat y);
void glVertex3fv(const GLfloat v[3]);
void glTexCoord2f(GLfloat s, GLfloat t);
void glTexCoord2fv(const GLfloat st[2]);
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glColor3f(GLfloat r, GLfloat g, GLfloat b);
void glColor3fv(const GLfloat v[3]);
void glColor4fv(const GLfloat v[4]);
void glColor4ubv(const GLubyte v[4]);
void glArrayElement(glIndex_t index);

void glPushAttrib(GLint mask);
void glPopAttrib(void);

void glPushMatrix(void);
void glPopMatrix(void);
void glMatrixMode(GLenum mode);

void glLoadIdentity(void);
void glMultMatrixf(const GLfloat matrix[16]);
void glOrtho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearZ, GLfloat farZ);

void glEnableClientState(GLenum e);
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

void glRasterPos2f(GLfloat x, GLfloat y);
void glDrawPixels(GLint width, GLint height, GLenum format, GLenum dataType, const void *data);
#endif


#endif
