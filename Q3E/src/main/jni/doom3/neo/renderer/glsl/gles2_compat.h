#ifndef _GLES2_COMPAT_H
#define _GLES2_COMPAT_H

#define GLES_COMPAT 1

#ifndef GL_VERTEX_ARRAY
#define GL_VERTEX_ARRAY				0x8074
#endif
#ifndef GL_COLOR_ARRAY
#define GL_COLOR_ARRAY				0x8076
#endif
#ifndef GL_TEXTURE_COORD_ARRAY
#define GL_TEXTURE_COORD_ARRAY			0x8078
#endif

/* prim */
#ifdef GL_POLYGON
#undef GL_POLYGON
#endif
#define GL_POLYGON 0x0009 // GL_TRIANGLE_FAN // GL_LINE_LOOP
#ifdef GL_QUADS
#undef GL_QUADS
#endif
#define GL_QUADS 0x0007 // GL_TRIANGLE_FAN
#ifdef GL_QUAD_STRIP
#undef GL_QUAD_STRIP
#endif
#define GL_QUAD_STRIP 0x0008 // GL_TRIANGLE_STRIP

/* Matrix Mode */
#ifndef GL_MODELVIEW
#define GL_MODELVIEW				0x1700
#endif
#ifndef GL_PROJECTION
#define GL_PROJECTION				0x1701
#endif
#ifndef GL_PROJECTION_MATRIX
#define GL_PROJECTION_MATRIX			0x0BA7
#endif

/* state */
#ifndef GL_ALL_ATTRIB_BITS
#define GL_ALL_ATTRIB_BITS			0x000FFFFF
#endif
#ifndef GL_CURRENT_COLOR
#define GL_CURRENT_COLOR			0x0B00
#endif

/* polygon mode */
#ifndef GL_LINE
#define GL_LINE				0x1B01
#endif
#ifndef GL_FILL
#define GL_FILL				0x1B02
#endif

/* texture */
#ifndef GL_TEXTURE_1D
#define GL_TEXTURE_1D				0x0DE0
#endif


void glDisableClientState(GLenum e);
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glLoadMatrixf(const GLfloat matrix[16]);
void glBegin(GLenum t);
void glEnd(void);
void glVertex3f(GLfloat x, GLfloat y, GLfloat z);

void glVertex3fv(const GLfloat v[3]);
void glTexCoord2f(GLfloat s, GLfloat t);
void glTexCoord2fv(const GLfloat st[2]);
void glEnableClientState(GLenum e);
void glColor3f(GLfloat r, GLfloat g, GLfloat b);
void glVertex2f(GLfloat x, GLfloat y);
void glMatrixMode(GLenum mode);
void glLoadIdentity(void);
void glOrtho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearZ, GLfloat farZ);
void glPushMatrix(void);
void glPopMatrix(void);
void glColor3fv(const GLfloat v[3]);
void glColor4fv(const GLfloat v[4]);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
void glPopAttrib(void);
void glPushAttrib(GLint mask);
void glPolygonMode(GLenum face, GLenum mode);

void glesGetFloatv(GLenum pname, GLfloat *data);
void glesEnable(GLenum pname);
void glesDisable(GLenum pname);
void glesReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void * data, GLsizei align = 1);

void glesShutdown(void);

typedef void GLUquadricObj;
void gluSphere(GLUquadricObj *, float r, int lats, int longs);

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
void glArrayElement(GLint index);

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
