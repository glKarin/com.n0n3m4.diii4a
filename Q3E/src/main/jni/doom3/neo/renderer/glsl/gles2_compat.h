#ifndef _GLES2_COMPAT_H
#define _GLES2_COMPAT_H

#define GL_COLOR_ARRAY				0x8076
#define GL_TEXTURE_COORD_ARRAY			0x8078

#define GL_POLYGON GL_TRIANGLE_FAN // GL_LINE_LOOP
#define GL_QUADS GL_TRIANGLE_FAN
// #define GL_POLYGON				0x0009
#define GL_ALL_ATTRIB_BITS			0xFFFFFFFF

/* Matrix Mode */
#define GL_MODELVIEW				0x1700
#define GL_PROJECTION				0x1701


void glDisableClientState(GLenum e);
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glLoadMatrixf(const GLfloat matrix[16]);
void glBegin(GLenum t);
void glEnd(void);
void glVertex3f(GLfloat x, GLfloat y, GLfloat z);


#endif
