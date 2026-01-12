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
#ifndef GL_MODELVIEW_MATRIX
#define GL_MODELVIEW_MATRIX               0x0BA6
#endif
#ifndef GL_MATRIX_MODE
#define GL_MATRIX_MODE				0x0BA0
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

typedef void GLUquadricObj;

#define QGLCOMPATPROC(name, rettype, args) rettype name args;
#include "qgl_proc_compat.h"

void glesGetFloatv(GLenum pname, GLfloat *data);
void glesGetIntegerv(GLenum pname, GLint *data);
void glesEnable(GLenum pname);
void glesDisable(GLenum pname);
void glesReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void * data, GLsizei align = 1);
void glrbShutdown(void);
//GLenum glesPushIdentityMatrix(void);
//void glesPopIdentityMatrix(GLenum m);

#endif
