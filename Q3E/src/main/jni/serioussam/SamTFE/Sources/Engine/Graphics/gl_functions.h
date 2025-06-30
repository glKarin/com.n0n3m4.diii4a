/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */



// GLFUNCTION(dll, output, name, inputs, params, required)



/*
 * Miscellaneous
 */

DLLFUNCTION( OGL, void , glClearIndex,( GLfloat c ),0,0);

DLLFUNCTION( OGL, void , glClearColor,( GLclampf red,
			  GLclampf green,
			  GLclampf blue,
			  GLclampf alpha ),0,0);

DLLFUNCTION( OGL, void , glClear,( GLbitfield mask ),0,0);

DLLFUNCTION( OGL, void , glIndexMask,( GLuint mask ),0,0);

DLLFUNCTION( OGL, void , glColorMask,( GLboolean red, GLboolean green,
			 GLboolean blue, GLboolean alpha ),0,0);

DLLFUNCTION( OGL, void , glAlphaFunc,( GLenum func, GLclampf ref ),0,0);

DLLFUNCTION( OGL, void , glBlendFunc,( GLenum sfactor, GLenum dfactor ),0,0);

DLLFUNCTION( OGL, void , glLogicOp,( GLenum opcode ),0,0);

DLLFUNCTION( OGL, void , glCullFace,( GLenum mode ),0,0);

DLLFUNCTION( OGL, void , glFrontFace,( GLenum mode ),0,0);

DLLFUNCTION( OGL, void , glPointSize,( GLfloat size ),0,0);

DLLFUNCTION( OGL, void , glLineWidth,( GLfloat width ),0,0);

DLLFUNCTION( OGL, void , glLineStipple,( GLint factor, GLushort pattern ),0,0);

#if !defined(_GLES) //karin: no glPolygonMode on GLES
DLLFUNCTION( OGL, void , glPolygonMode,( GLenum face, GLenum mode ),0,0);
#endif

DLLFUNCTION( OGL, void , glPolygonOffset,( GLfloat factor, GLfloat units ),0,0);

DLLFUNCTION( OGL, void , glPolygonStipple,( const GLubyte *mask ),0,0);

DLLFUNCTION( OGL, void , glGetPolygonStipple,( GLubyte *mask ),0,0);

DLLFUNCTION( OGL, void , glEdgeFlag,( GLboolean flag ),0,0);

DLLFUNCTION( OGL, void , glEdgeFlagv,( const GLboolean *flag ),0,0);

DLLFUNCTION( OGL, void , glScissor,( GLint x, GLint y,
                                   GLsizei width, GLsizei height),0,0);

#ifdef _GLES //karin: glClipPlanef and glGetClipPlanef on GLES
DLLFUNCTION( OGL, void , glClipPlanef,( GLenum plane, const GLfloat *equation ),0,0);
DLLFUNCTION( OGL, void , glGetClipPlanef,( GLenum plane, GLfloat *equation ),0,0);
#else
DLLFUNCTION( OGL, void , glClipPlane,( GLenum plane, const GLdouble *equation ),0,0);

DLLFUNCTION( OGL, void , glGetClipPlane,( GLenum plane, GLdouble *equation ),0,0);
#endif

#if !defined(_GLES) //karin: no glDrawBuffer and glReadBuffer on GLES
DLLFUNCTION( OGL, void , glDrawBuffer,( GLenum mode ),0,0);

DLLFUNCTION( OGL, void , glReadBuffer,( GLenum mode ),0,0);
#endif

DLLFUNCTION( OGL, void , glEnable,( GLenum cap ),0,0);

DLLFUNCTION( OGL, void , glDisable,( GLenum cap ),0,0);

DLLFUNCTION( OGL, GLboolean , glIsEnabled,( GLenum cap ),0,0);


DLLFUNCTION( OGL, void , glEnableClientState,( GLenum cap ),0,0);  /* 1.1 */

DLLFUNCTION( OGL, void , glDisableClientState,( GLenum cap ),0,0);  /* 1.1 */


DLLFUNCTION( OGL, void , glGetBooleanv,( GLenum pname, GLboolean *params ),0,0);

DLLFUNCTION( OGL, void , glGetDoublev,( GLenum pname, GLdouble *params ),0,0);

DLLFUNCTION( OGL, void , glGetFloatv,( GLenum pname, GLfloat *params ),0,0);

DLLFUNCTION( OGL, void , glGetIntegerv,( GLenum pname, GLint *params ),0,0);


DLLFUNCTION( OGL, void , glPushAttrib,( GLbitfield mask ),0,0);

DLLFUNCTION( OGL, void , glPopAttrib,( void ),0,0);


DLLFUNCTION( OGL, void , glPushClientAttrib,( GLbitfield mask ),0,0);  /* 1.1 */

DLLFUNCTION( OGL, void , glPopClientAttrib,( void ),0,0);  /* 1.1 */


DLLFUNCTION( OGL, GLint , glRenderMode,( GLenum mode ),0,0);

DLLFUNCTION( OGL, GLenum , glGetError,( void ),0,1);

DLLFUNCTION( OGL, const GLubyte* , glGetString,( GLenum name ),0,0);

DLLFUNCTION( OGL, void , glFinish,( void ),0,0);

DLLFUNCTION( OGL, void , glFlush,( void ),0,0);

DLLFUNCTION( OGL, void , glHint,( GLenum target, GLenum mode ),0,0);



/*
 * Depth Buffer
 */

#ifdef _GLES //karin: glClearDepthf on GLES
DLLFUNCTION( OGL, void , glClearDepthf,( GLfloat depth ),0,0);
#else
DLLFUNCTION( OGL, void , glClearDepth,( GLclampd depth ),0,0);
#endif

DLLFUNCTION( OGL, void , glDepthFunc,( GLenum func ),0,0);

DLLFUNCTION( OGL, void , glDepthMask,( GLboolean flag ),0,0);

#ifdef _GLES //karin: glDepthRangef on GLES
DLLFUNCTION( OGL, void , glDepthRangef,( GLfloat near_val, GLfloat far_val ),0,0);
#else
DLLFUNCTION( OGL, void , glDepthRange,( GLclampd near_val, GLclampd far_val ),0,0);
#endif


/*
 * Accumulation Buffer
 */

DLLFUNCTION( OGL, void , glClearAccum,( GLfloat red, GLfloat green,
                                      GLfloat blue, GLfloat alpha ),0,0);

DLLFUNCTION( OGL, void , glAccum,( GLenum op, GLfloat value ),0,0);



/*
 * Transformation
 */

DLLFUNCTION( OGL, void , glMatrixMode,( GLenum mode ),0,0);

#ifdef _GLES //karin: glOrthof and glFrustumf on GLES
DLLFUNCTION( OGL, void , glOrthof,( GLfloat left, GLfloat right,
                                 GLfloat bottom, GLfloat top,
                                 GLfloat near_val, GLfloat far_val ),0,0);

DLLFUNCTION( OGL, void , glFrustumf,( GLfloat left, GLfloat right,
                                   GLfloat bottom, GLfloat top,
                                   GLfloat near_val, GLfloat far_val ),0,0);
#else
DLLFUNCTION( OGL, void , glOrtho,( GLdouble left, GLdouble right,
                                 GLdouble bottom, GLdouble top,
                                 GLdouble near_val, GLdouble far_val ),0,0);

DLLFUNCTION( OGL, void , glFrustum,( GLdouble left, GLdouble right,
                                   GLdouble bottom, GLdouble top,
                                   GLdouble near_val, GLdouble far_val ),0,0);
#endif

DLLFUNCTION( OGL, void , glViewport,( GLint x, GLint y,
                                    GLsizei width, GLsizei height ),0,0);

DLLFUNCTION( OGL, void , glPushMatrix,( void ),0,0);

DLLFUNCTION( OGL, void , glPopMatrix,( void ),0,0);

DLLFUNCTION( OGL, void , glLoadIdentity,( void ),0,0);

DLLFUNCTION( OGL, void , glLoadMatrixd,( const GLdouble *m ),0,0);
DLLFUNCTION( OGL, void , glLoadMatrixf,( const GLfloat *m ),0,0);

DLLFUNCTION( OGL, void , glMultMatrixd,( const GLdouble *m ),0,0);
DLLFUNCTION( OGL, void , glMultMatrixf,( const GLfloat *m ),0,0);

DLLFUNCTION( OGL, void , glRotated,( GLdouble angle,
                                   GLdouble x, GLdouble y, GLdouble z ),0,0);
DLLFUNCTION( OGL, void , glRotatef,( GLfloat angle,
                                   GLfloat x, GLfloat y, GLfloat z ),0,0);

DLLFUNCTION( OGL, void , glScaled,( GLdouble x, GLdouble y, GLdouble z ),0,0);
DLLFUNCTION( OGL, void , glScalef,( GLfloat x, GLfloat y, GLfloat z ),0,0);

DLLFUNCTION( OGL, void , glTranslated,( GLdouble x, GLdouble y, GLdouble z ),0,0);
DLLFUNCTION( OGL, void , glTranslatef,( GLfloat x, GLfloat y, GLfloat z ),0,0);



/*
 * Display Lists
 */

DLLFUNCTION( OGL, GLboolean , glIsList,( GLuint list ),0,0);

DLLFUNCTION( OGL, void , glDeleteLists,( GLuint list, GLsizei range ),0,0);

DLLFUNCTION( OGL, GLuint , glGenLists,( GLsizei range ),0,0);

DLLFUNCTION( OGL, void , glNewList,( GLuint list, GLenum mode ),0,0);

DLLFUNCTION( OGL, void , glEndList,( void ),0,0);

DLLFUNCTION( OGL, void , glCallList,( GLuint list ),0,0);

DLLFUNCTION( OGL, void , glCallLists,( GLsizei n, GLenum type,
                                     const GLvoid *lists ),0,0);

DLLFUNCTION( OGL, void , glListBase,( GLuint base ),0,0);



/*
 * Drawing Functions
 */

DLLFUNCTION( OGL, void , glBegin,( GLenum mode ),0,0);

DLLFUNCTION( OGL, void , glEnd,( void ),0,0);


DLLFUNCTION( OGL, void , glVertex2d,( GLdouble x, GLdouble y ),0,0);
DLLFUNCTION( OGL, void , glVertex2f,( GLfloat x, GLfloat y ),0,0);
DLLFUNCTION( OGL, void , glVertex2i,( GLint x, GLint y ),0,0);
DLLFUNCTION( OGL, void , glVertex2s,( GLshort x, GLshort y ),0,0);

DLLFUNCTION( OGL, void , glVertex3d,( GLdouble x, GLdouble y, GLdouble z ),0,0);
DLLFUNCTION( OGL, void , glVertex3f,( GLfloat x, GLfloat y, GLfloat z ),0,0);
DLLFUNCTION( OGL, void , glVertex3i,( GLint x, GLint y, GLint z ),0,0);
DLLFUNCTION( OGL, void , glVertex3s,( GLshort x, GLshort y, GLshort z ),0,0);

DLLFUNCTION( OGL, void , glVertex4d,( GLdouble x, GLdouble y, GLdouble z, GLdouble w ),0,0);
DLLFUNCTION( OGL, void , glVertex4f,( GLfloat x, GLfloat y, GLfloat z, GLfloat w ),0,0);
DLLFUNCTION( OGL, void , glVertex4i,( GLint x, GLint y, GLint z, GLint w ),0,0);
DLLFUNCTION( OGL, void , glVertex4s,( GLshort x, GLshort y, GLshort z, GLshort w ),0,0);

DLLFUNCTION( OGL, void , glVertex2dv,( const GLdouble *v ),0,0);
DLLFUNCTION( OGL, void , glVertex2fv,( const GLfloat *v ),0,0);
DLLFUNCTION( OGL, void , glVertex2iv,( const GLint *v ),0,0);
DLLFUNCTION( OGL, void , glVertex2sv,( const GLshort *v ),0,0);

DLLFUNCTION( OGL, void , glVertex3dv,( const GLdouble *v ),0,0);
DLLFUNCTION( OGL, void , glVertex3fv,( const GLfloat *v ),0,0);
DLLFUNCTION( OGL, void , glVertex3iv,( const GLint *v ),0,0);
DLLFUNCTION( OGL, void , glVertex3sv,( const GLshort *v ),0,0);

DLLFUNCTION( OGL, void , glVertex4dv,( const GLdouble *v ),0,0);
DLLFUNCTION( OGL, void , glVertex4fv,( const GLfloat *v ),0,0);
DLLFUNCTION( OGL, void , glVertex4iv,( const GLint *v ),0,0);
DLLFUNCTION( OGL, void , glVertex4sv,( const GLshort *v ),0,0);


DLLFUNCTION( OGL, void , glNormal3b,( GLbyte nx, GLbyte ny, GLbyte nz ),0,0);
DLLFUNCTION( OGL, void , glNormal3d,( GLdouble nx, GLdouble ny, GLdouble nz ),0,0);
DLLFUNCTION( OGL, void , glNormal3f,( GLfloat nx, GLfloat ny, GLfloat nz ),0,0);
DLLFUNCTION( OGL, void , glNormal3i,( GLint nx, GLint ny, GLint nz ),0,0);
DLLFUNCTION( OGL, void , glNormal3s,( GLshort nx, GLshort ny, GLshort nz ),0,0);

DLLFUNCTION( OGL, void , glNormal3bv,( const GLbyte *v ),0,0);
DLLFUNCTION( OGL, void , glNormal3dv,( const GLdouble *v ),0,0);
DLLFUNCTION( OGL, void , glNormal3fv,( const GLfloat *v ),0,0);
DLLFUNCTION( OGL, void , glNormal3iv,( const GLint *v ),0,0);
DLLFUNCTION( OGL, void , glNormal3sv,( const GLshort *v ),0,0);


DLLFUNCTION( OGL, void , glIndexd,( GLdouble c ),0,0);
DLLFUNCTION( OGL, void , glIndexf,( GLfloat c ),0,0);
DLLFUNCTION( OGL, void , glIndexi,( GLint c ),0,0);
DLLFUNCTION( OGL, void , glIndexs,( GLshort c ),0,0);
DLLFUNCTION( OGL, void , glIndexub,( GLubyte c ),0,0);  /* 1.1 */

DLLFUNCTION( OGL, void , glIndexdv,( const GLdouble *c ),0,0);
DLLFUNCTION( OGL, void , glIndexfv,( const GLfloat *c ),0,0);
DLLFUNCTION( OGL, void , glIndexiv,( const GLint *c ),0,0);
DLLFUNCTION( OGL, void , glIndexsv,( const GLshort *c ),0,0);
DLLFUNCTION( OGL, void , glIndexubv,( const GLubyte *c ),0,0);  /* 1.1 */

DLLFUNCTION( OGL, void , glColor3b,( GLbyte red, GLbyte green, GLbyte blue ),0,0);
DLLFUNCTION( OGL, void , glColor3d,( GLdouble red, GLdouble green, GLdouble blue ),0,0);
DLLFUNCTION( OGL, void , glColor3f,( GLfloat red, GLfloat green, GLfloat blue ),0,0);
DLLFUNCTION( OGL, void , glColor3i,( GLint red, GLint green, GLint blue ),0,0);
DLLFUNCTION( OGL, void , glColor3s,( GLshort red, GLshort green, GLshort blue ),0,0);
DLLFUNCTION( OGL, void , glColor3ub,( GLubyte red, GLubyte green, GLubyte blue ),0,0);
DLLFUNCTION( OGL, void , glColor3ui,( GLuint red, GLuint green, GLuint blue ),0,0);
DLLFUNCTION( OGL, void , glColor3us,( GLushort red, GLushort green, GLushort blue ),0,0);

DLLFUNCTION( OGL, void , glColor4b,( GLbyte red, GLbyte green,
                                   GLbyte blue, GLbyte alpha ),0,0);
DLLFUNCTION( OGL, void , glColor4d,( GLdouble red, GLdouble green,
                                   GLdouble blue, GLdouble alpha ),0,0);
DLLFUNCTION( OGL, void , glColor4f,( GLfloat red, GLfloat green,
                                   GLfloat blue, GLfloat alpha ),0,0);
DLLFUNCTION( OGL, void , glColor4i,( GLint red, GLint green,
                                   GLint blue, GLint alpha ),0,0);
DLLFUNCTION( OGL, void , glColor4s,( GLshort red, GLshort green,
                                   GLshort blue, GLshort alpha ),0,0);
DLLFUNCTION( OGL, void , glColor4ub,( GLubyte red, GLubyte green,
                                    GLubyte blue, GLubyte alpha ),0,0);
DLLFUNCTION( OGL, void , glColor4ui,( GLuint red, GLuint green,
                                    GLuint blue, GLuint alpha ),0,0);
DLLFUNCTION( OGL, void , glColor4us,( GLushort red, GLushort green,
                                    GLushort blue, GLushort alpha ),0,0);


DLLFUNCTION( OGL, void , glColor3bv,( const GLbyte *v ),0,0);
DLLFUNCTION( OGL, void , glColor3dv,( const GLdouble *v ),0,0);
DLLFUNCTION( OGL, void , glColor3fv,( const GLfloat *v ),0,0);
DLLFUNCTION( OGL, void , glColor3iv,( const GLint *v ),0,0);
DLLFUNCTION( OGL, void , glColor3sv,( const GLshort *v ),0,0);
DLLFUNCTION( OGL, void , glColor3ubv,( const GLubyte *v ),0,0);
DLLFUNCTION( OGL, void , glColor3uiv,( const GLuint *v ),0,0);
DLLFUNCTION( OGL, void , glColor3usv,( const GLushort *v ),0,0);

DLLFUNCTION( OGL, void , glColor4bv,( const GLbyte *v ),0,0);
DLLFUNCTION( OGL, void , glColor4dv,( const GLdouble *v ),0,0);
DLLFUNCTION( OGL, void , glColor4fv,( const GLfloat *v ),0,0);
DLLFUNCTION( OGL, void , glColor4iv,( const GLint *v ),0,0);
DLLFUNCTION( OGL, void , glColor4sv,( const GLshort *v ),0,0);
#if !defined(_GLES) //karin: not support on GLES
DLLFUNCTION( OGL, void , glColor4ubv,( const GLubyte *v ),0,0);
#endif
DLLFUNCTION( OGL, void , glColor4uiv,( const GLuint *v ),0,0);
DLLFUNCTION( OGL, void , glColor4usv,( const GLushort *v ),0,0);


DLLFUNCTION( OGL, void , glTexCoord1d,( GLdouble s ),0,0);
DLLFUNCTION( OGL, void , glTexCoord1f,( GLfloat s ),0,0);
DLLFUNCTION( OGL, void , glTexCoord1i,( GLint s ),0,0);
DLLFUNCTION( OGL, void , glTexCoord1s,( GLshort s ),0,0);

DLLFUNCTION( OGL, void , glTexCoord2d,( GLdouble s, GLdouble t ),0,0);
DLLFUNCTION( OGL, void , glTexCoord2f,( GLfloat s, GLfloat t ),0,0);
DLLFUNCTION( OGL, void , glTexCoord2i,( GLint s, GLint t ),0,0);
DLLFUNCTION( OGL, void , glTexCoord2s,( GLshort s, GLshort t ),0,0);

DLLFUNCTION( OGL, void , glTexCoord3d,( GLdouble s, GLdouble t, GLdouble r ),0,0);
DLLFUNCTION( OGL, void , glTexCoord3f,( GLfloat s, GLfloat t, GLfloat r ),0,0);
DLLFUNCTION( OGL, void , glTexCoord3i,( GLint s, GLint t, GLint r ),0,0);
DLLFUNCTION( OGL, void , glTexCoord3s,( GLshort s, GLshort t, GLshort r ),0,0);

DLLFUNCTION( OGL, void , glTexCoord4d,( GLdouble s, GLdouble t, GLdouble r, GLdouble q ),0,0);
DLLFUNCTION( OGL, void , glTexCoord4f,( GLfloat s, GLfloat t, GLfloat r, GLfloat q ),0,0);
DLLFUNCTION( OGL, void , glTexCoord4i,( GLint s, GLint t, GLint r, GLint q ),0,0);
DLLFUNCTION( OGL, void , glTexCoord4s,( GLshort s, GLshort t, GLshort r, GLshort q ),0,0);

DLLFUNCTION( OGL, void , glTexCoord1dv,( const GLdouble *v ),0,0);
DLLFUNCTION( OGL, void , glTexCoord1fv,( const GLfloat *v ),0,0);
DLLFUNCTION( OGL, void , glTexCoord1iv,( const GLint *v ),0,0);
DLLFUNCTION( OGL, void , glTexCoord1sv,( const GLshort *v ),0,0);

DLLFUNCTION( OGL, void , glTexCoord2dv,( const GLdouble *v ),0,0);
DLLFUNCTION( OGL, void , glTexCoord2fv,( const GLfloat *v ),0,0);
DLLFUNCTION( OGL, void , glTexCoord2iv,( const GLint *v ),0,0);
DLLFUNCTION( OGL, void , glTexCoord2sv,( const GLshort *v ),0,0);

DLLFUNCTION( OGL, void , glTexCoord3dv,( const GLdouble *v ),0,0);
DLLFUNCTION( OGL, void , glTexCoord3fv,( const GLfloat *v ),0,0);
DLLFUNCTION( OGL, void , glTexCoord3iv,( const GLint *v ),0,0);
DLLFUNCTION( OGL, void , glTexCoord3sv,( const GLshort *v ),0,0);

DLLFUNCTION( OGL, void , glTexCoord4dv,( const GLdouble *v ),0,0);
DLLFUNCTION( OGL, void , glTexCoord4fv,( const GLfloat *v ),0,0);
DLLFUNCTION( OGL, void , glTexCoord4iv,( const GLint *v ),0,0);
DLLFUNCTION( OGL, void , glTexCoord4sv,( const GLshort *v ),0,0);


DLLFUNCTION( OGL, void , glRasterPos2d,( GLdouble x, GLdouble y ),0,0);
DLLFUNCTION( OGL, void , glRasterPos2f,( GLfloat x, GLfloat y ),0,0);
DLLFUNCTION( OGL, void , glRasterPos2i,( GLint x, GLint y ),0,0);
DLLFUNCTION( OGL, void , glRasterPos2s,( GLshort x, GLshort y ),0,0);

DLLFUNCTION( OGL, void , glRasterPos3d,( GLdouble x, GLdouble y, GLdouble z ),0,0);
DLLFUNCTION( OGL, void , glRasterPos3f,( GLfloat x, GLfloat y, GLfloat z ),0,0);
DLLFUNCTION( OGL, void , glRasterPos3i,( GLint x, GLint y, GLint z ),0,0);
DLLFUNCTION( OGL, void , glRasterPos3s,( GLshort x, GLshort y, GLshort z ),0,0);

DLLFUNCTION( OGL, void , glRasterPos4d,( GLdouble x, GLdouble y, GLdouble z, GLdouble w ),0,0);
DLLFUNCTION( OGL, void , glRasterPos4f,( GLfloat x, GLfloat y, GLfloat z, GLfloat w ),0,0);
DLLFUNCTION( OGL, void , glRasterPos4i,( GLint x, GLint y, GLint z, GLint w ),0,0);
DLLFUNCTION( OGL, void , glRasterPos4s,( GLshort x, GLshort y, GLshort z, GLshort w ),0,0);

DLLFUNCTION( OGL, void , glRasterPos2dv,( const GLdouble *v ),0,0);
DLLFUNCTION( OGL, void , glRasterPos2fv,( const GLfloat *v ),0,0);
DLLFUNCTION( OGL, void , glRasterPos2iv,( const GLint *v ),0,0);
DLLFUNCTION( OGL, void , glRasterPos2sv,( const GLshort *v ),0,0);

DLLFUNCTION( OGL, void , glRasterPos3dv,( const GLdouble *v ),0,0);
DLLFUNCTION( OGL, void , glRasterPos3fv,( const GLfloat *v ),0,0);
DLLFUNCTION( OGL, void , glRasterPos3iv,( const GLint *v ),0,0);
DLLFUNCTION( OGL, void , glRasterPos3sv,( const GLshort *v ),0,0);

DLLFUNCTION( OGL, void , glRasterPos4dv,( const GLdouble *v ),0,0);
DLLFUNCTION( OGL, void , glRasterPos4fv,( const GLfloat *v ),0,0);
DLLFUNCTION( OGL, void , glRasterPos4iv,( const GLint *v ),0,0);
DLLFUNCTION( OGL, void , glRasterPos4sv,( const GLshort *v ),0,0);


DLLFUNCTION( OGL, void , glRectd,( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 ),0,0);
DLLFUNCTION( OGL, void , glRectf,( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 ),0,0);
DLLFUNCTION( OGL, void , glRecti,( GLint x1, GLint y1, GLint x2, GLint y2 ),0,0);
DLLFUNCTION( OGL, void , glRects,( GLshort x1, GLshort y1, GLshort x2, GLshort y2 ),0,0);


DLLFUNCTION( OGL, void , glRectdv,( const GLdouble *v1, const GLdouble *v2 ),0,0);
DLLFUNCTION( OGL, void , glRectfv,( const GLfloat *v1, const GLfloat *v2 ),0,0);
DLLFUNCTION( OGL, void , glRectiv,( const GLint *v1, const GLint *v2 ),0,0);
DLLFUNCTION( OGL, void , glRectsv,( const GLshort *v1, const GLshort *v2 ),0,0);



/*
 * Vertex Arrays  (1.1)
 */

DLLFUNCTION( OGL, void , glVertexPointer,( GLint size, GLenum type,
                                         GLsizei stride, const GLvoid *ptr ),0,0);

DLLFUNCTION( OGL, void , glNormalPointer,( GLenum type, GLsizei stride,
                                         const GLvoid *ptr ),0,0);

DLLFUNCTION( OGL, void , glColorPointer,( GLint size, GLenum type,
                                        GLsizei stride, const GLvoid *ptr ),0,0);

DLLFUNCTION( OGL, void , glIndexPointer,( GLenum type, GLsizei stride,
                                        const GLvoid *ptr ),0,0);

DLLFUNCTION( OGL, void , glTexCoordPointer,( GLint size, GLenum type,
                                           GLsizei stride, const GLvoid *ptr ),0,0);

DLLFUNCTION( OGL, void , glEdgeFlagPointer,( GLsizei stride,
                                           const GLboolean *ptr ),0,0);

DLLFUNCTION( OGL, void , glGetPointerv,( GLenum pname, void **params ),0,0);

DLLFUNCTION( OGL, void , glArrayElement,( GLint i ),0,0);

DLLFUNCTION( OGL, void , glDrawArrays,( GLenum mode, GLint first,
                                      GLsizei count ),0,0);

DLLFUNCTION( OGL, void , glDrawElements,( GLenum mode, GLsizei count,
                                        GLenum type, const GLvoid *indices ),0,0);

DLLFUNCTION( OGL, void , glInterleavedArrays,( GLenum format, GLsizei stride,
                                             const GLvoid *pointer ),0,0);


/*
 * Lighting
 */

DLLFUNCTION( OGL, void , glShadeModel,( GLenum mode ),0,0);

DLLFUNCTION( OGL, void , glLightf,( GLenum light, GLenum pname, GLfloat param ),0,0);
DLLFUNCTION( OGL, void , glLighti,( GLenum light, GLenum pname, GLint param ),0,0);
DLLFUNCTION( OGL, void , glLightfv,( GLenum light, GLenum pname,
                                   const GLfloat *params ),0,0);
DLLFUNCTION( OGL, void , glLightiv,( GLenum light, GLenum pname,
                                   const GLint *params ),0,0);

DLLFUNCTION( OGL, void , glGetLightfv,( GLenum light, GLenum pname,
                                      GLfloat *params ),0,0);
DLLFUNCTION( OGL, void , glGetLightiv,( GLenum light, GLenum pname,
                                      GLint *params ),0,0);

DLLFUNCTION( OGL, void , glLightModelf,( GLenum pname, GLfloat param ),0,0);
DLLFUNCTION( OGL, void , glLightModeli,( GLenum pname, GLint param ),0,0);
DLLFUNCTION( OGL, void , glLightModelfv,( GLenum pname, const GLfloat *params ),0,0);
DLLFUNCTION( OGL, void , glLightModeliv,( GLenum pname, const GLint *params ),0,0);

DLLFUNCTION( OGL, void , glMaterialf,( GLenum face, GLenum pname, GLfloat param ),0,0);
DLLFUNCTION( OGL, void , glMateriali,( GLenum face, GLenum pname, GLint param ),0,0);
DLLFUNCTION( OGL, void , glMaterialfv,( GLenum face, GLenum pname, const GLfloat *params ),0,0);
DLLFUNCTION( OGL, void , glMaterialiv,( GLenum face, GLenum pname, const GLint *params ),0,0);

DLLFUNCTION( OGL, void , glGetMaterialfv,( GLenum face, GLenum pname, GLfloat *params ),0,0);
DLLFUNCTION( OGL, void , glGetMaterialiv,( GLenum face, GLenum pname, GLint *params ),0,0);

DLLFUNCTION( OGL, void , glColorMaterial,( GLenum face, GLenum mode ),0,0);




/*
 * Raster functions
 */

DLLFUNCTION( OGL, void , glPixelZoom,( GLfloat xfactor, GLfloat yfactor ),0,0);

DLLFUNCTION( OGL, void , glPixelStoref,( GLenum pname, GLfloat param ),0,0);
DLLFUNCTION( OGL, void , glPixelStorei,( GLenum pname, GLint param ),0,0);

DLLFUNCTION( OGL, void , glPixelTransferf,( GLenum pname, GLfloat param ),0,0);
DLLFUNCTION( OGL, void , glPixelTransferi,( GLenum pname, GLint param ),0,0);

DLLFUNCTION( OGL, void , glPixelMapfv,( GLenum map, GLint mapsize,
                                      const GLfloat *values ),0,0);
DLLFUNCTION( OGL, void , glPixelMapuiv,( GLenum map, GLint mapsize,
                                       const GLuint *values ),0,0);
DLLFUNCTION( OGL, void , glPixelMapusv,( GLenum map, GLint mapsize,
                                       const GLushort *values ),0,0);

DLLFUNCTION( OGL, void , glGetPixelMapfv,( GLenum map, GLfloat *values ),0,0);
DLLFUNCTION( OGL, void , glGetPixelMapuiv,( GLenum map, GLuint *values ),0,0);
DLLFUNCTION( OGL, void , glGetPixelMapusv,( GLenum map, GLushort *values ),0,0);

DLLFUNCTION( OGL, void , glBitmap,( GLsizei width, GLsizei height,
                                  GLfloat xorig, GLfloat yorig,
                                  GLfloat xmove, GLfloat ymove,
                                  const GLubyte *bitmap ),0,0);

DLLFUNCTION( OGL, void , glReadPixels,( GLint x, GLint y,
                                      GLsizei width, GLsizei height,
                                      GLenum format, GLenum type,
                                      GLvoid *pixels ),0,0);

DLLFUNCTION( OGL, void , glDrawPixels,( GLsizei width, GLsizei height,
                                      GLenum format, GLenum type,
                                      const GLvoid *pixels ),0,0);

DLLFUNCTION( OGL, void , glCopyPixels,( GLint x, GLint y,
                                      GLsizei width, GLsizei height,
                                      GLenum type ),0,0);



/*
 * Stenciling
 */

DLLFUNCTION( OGL, void , glStencilFunc,( GLenum func, GLint ref, GLuint mask ),0,0);

DLLFUNCTION( OGL, void , glStencilMask,( GLuint mask ),0,0);

DLLFUNCTION( OGL, void , glStencilOp,( GLenum fail, GLenum zfail, GLenum zpass ),0,0);

DLLFUNCTION( OGL, void , glClearStencil,( GLint s ),0,0);



/*
 * Texture mapping
 */

DLLFUNCTION( OGL, void , glTexGend,( GLenum coord, GLenum pname, GLdouble param ),0,0);
DLLFUNCTION( OGL, void , glTexGenf,( GLenum coord, GLenum pname, GLfloat param ),0,0);
DLLFUNCTION( OGL, void , glTexGeni,( GLenum coord, GLenum pname, GLint param ),0,0);

DLLFUNCTION( OGL, void , glTexGendv,( GLenum coord, GLenum pname, const GLdouble *params ),0,0);
DLLFUNCTION( OGL, void , glTexGenfv,( GLenum coord, GLenum pname, const GLfloat *params ),0,0);
DLLFUNCTION( OGL, void , glTexGeniv,( GLenum coord, GLenum pname, const GLint *params ),0,0);

DLLFUNCTION( OGL, void , glGetTexGendv,( GLenum coord, GLenum pname, GLdouble *params ),0,0);
DLLFUNCTION( OGL, void , glGetTexGenfv,( GLenum coord, GLenum pname, GLfloat *params ),0,0);
DLLFUNCTION( OGL, void , glGetTexGeniv,( GLenum coord, GLenum pname, GLint *params ),0,0);


DLLFUNCTION( OGL, void , glTexEnvf,( GLenum target, GLenum pname, GLfloat param ),0,0);
DLLFUNCTION( OGL, void , glTexEnvi,( GLenum target, GLenum pname, GLint param ),0,0);

DLLFUNCTION( OGL, void , glTexEnvfv,( GLenum target, GLenum pname, const GLfloat *params ),0,0);
DLLFUNCTION( OGL, void , glTexEnviv,( GLenum target, GLenum pname, const GLint *params ),0,0);

DLLFUNCTION( OGL, void , glGetTexEnvfv,( GLenum target, GLenum pname, GLfloat *params ),0,0);
DLLFUNCTION( OGL, void , glGetTexEnviv,( GLenum target, GLenum pname, GLint *params ),0,0);


DLLFUNCTION( OGL, void , glTexParameterf,( GLenum target, GLenum pname, GLfloat param ),0,0);
DLLFUNCTION( OGL, void , glTexParameteri,( GLenum target, GLenum pname, GLint param ),0,0);

DLLFUNCTION( OGL, void , glTexParameterfv,( GLenum target, GLenum pname,
                                          const GLfloat *params ),0,0);
DLLFUNCTION( OGL, void , glTexParameteriv,( GLenum target, GLenum pname,
                                          const GLint *params ),0,0);

DLLFUNCTION( OGL, void , glGetTexParameterfv,( GLenum target,
                                             GLenum pname, GLfloat *params),0,0);
DLLFUNCTION( OGL, void , glGetTexParameteriv,( GLenum target,
                                             GLenum pname, GLint *params ),0,0);

#if !defined(_GLES) //karin: not support on GLES
DLLFUNCTION( OGL, void , glGetTexLevelParameterfv,( GLenum target, GLint level,
                                                  GLenum pname, GLfloat *params ),0,0);
DLLFUNCTION( OGL, void , glGetTexLevelParameteriv,( GLenum target, GLint level,
                                                  GLenum pname, GLint *params ),0,0);
#endif


DLLFUNCTION( OGL, void , glTexImage1D,( GLenum target, GLint level,
                                      GLint internalFormat,
                                      GLsizei width, GLint border,
                                      GLenum format, GLenum type,
                                      const GLvoid *pixels ),0,0);

DLLFUNCTION( OGL, void , glTexImage2D,( GLenum target, GLint level,
                                      GLint internalFormat,
                                      GLsizei width, GLsizei height,
                                      GLint border, GLenum format, GLenum type,
                                      const GLvoid *pixels ),0,0);

DLLFUNCTION( OGL, void , glGetTexImage,( GLenum target, GLint level,
                                       GLenum format, GLenum type,
                                       GLvoid *pixels ),0,0);



/* 1.1 functions */

DLLFUNCTION( OGL, void , glGenTextures,( GLsizei n, GLuint *textures ),0,0);

DLLFUNCTION( OGL, void , glDeleteTextures,( GLsizei n, const GLuint *textures),0,0);

DLLFUNCTION( OGL, void , glBindTexture,( GLenum target, GLuint texture ),0,0);

DLLFUNCTION( OGL, void , glPrioritizeTextures,( GLsizei n,
                                              const GLuint *textures,
                                              const GLclampf *priorities ),0,0);

DLLFUNCTION( OGL, GLboolean , glAreTexturesResident,( GLsizei n,
                                                    const GLuint *textures,
                                                    GLboolean *residences ),0,0);

DLLFUNCTION( OGL, GLboolean , glIsTexture,( GLuint texture ),0,0);


DLLFUNCTION( OGL, void , glTexSubImage1D,( GLenum target, GLint level,
                                         GLint xoffset,
                                         GLsizei width, GLenum format,
                                         GLenum type, const GLvoid *pixels ),0,0);


DLLFUNCTION( OGL, void , glTexSubImage2D,( GLenum target, GLint level,
                                         GLint xoffset, GLint yoffset,
                                         GLsizei width, GLsizei height,
                                         GLenum format, GLenum type,
                                         const GLvoid *pixels ),0,0);


DLLFUNCTION( OGL, void , glCopyTexImage1D,( GLenum target, GLint level,
                                          GLenum internalformat,
                                          GLint x, GLint y,
                                          GLsizei width, GLint border ),0,0);


DLLFUNCTION( OGL, void , glCopyTexImage2D,( GLenum target, GLint level,
                                          GLenum internalformat,
                                          GLint x, GLint y,
                                          GLsizei width, GLsizei height,
                                          GLint border ),0,0);


DLLFUNCTION( OGL, void , glCopyTexSubImage1D,( GLenum target, GLint level,
                                             GLint xoffset, GLint x, GLint y,
                                             GLsizei width ),0,0);


DLLFUNCTION( OGL, void , glCopyTexSubImage2D,( GLenum target, GLint level,
                                             GLint xoffset, GLint yoffset,
                                             GLint x, GLint y,
                                             GLsizei width, GLsizei height ),0,0);




/*
 * Evaluators
 */

DLLFUNCTION( OGL, void , glMap1d,( GLenum target, GLdouble u1, GLdouble u2,
                                 GLint stride,
                                 GLint order, const GLdouble *points ),0,0);
DLLFUNCTION( OGL, void , glMap1f,( GLenum target, GLfloat u1, GLfloat u2,
                                 GLint stride,
                                 GLint order, const GLfloat *points ),0,0);

DLLFUNCTION( OGL, void , glMap2d,( GLenum target,
		     GLdouble u1, GLdouble u2, GLint ustride, GLint uorder,
		     GLdouble v1, GLdouble v2, GLint vstride, GLint vorder,
		     const GLdouble *points ),0,0);
DLLFUNCTION( OGL, void , glMap2f,( GLenum target,
		     GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
		     GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
		     const GLfloat *points ),0,0);

DLLFUNCTION( OGL, void , glGetMapdv,( GLenum target, GLenum query, GLdouble *v ),0,0);
DLLFUNCTION( OGL, void , glGetMapfv,( GLenum target, GLenum query, GLfloat *v ),0,0);
DLLFUNCTION( OGL, void , glGetMapiv,( GLenum target, GLenum query, GLint *v ),0,0);

DLLFUNCTION( OGL, void , glEvalCoord1d,( GLdouble u ),0,0);
DLLFUNCTION( OGL, void , glEvalCoord1f,( GLfloat u ),0,0);

DLLFUNCTION( OGL, void , glEvalCoord1dv,( const GLdouble *u ),0,0);
DLLFUNCTION( OGL, void , glEvalCoord1fv,( const GLfloat *u ),0,0);

DLLFUNCTION( OGL, void , glEvalCoord2d,( GLdouble u, GLdouble v ),0,0);
DLLFUNCTION( OGL, void , glEvalCoord2f,( GLfloat u, GLfloat v ),0,0);

DLLFUNCTION( OGL, void , glEvalCoord2dv,( const GLdouble *u ),0,0);
DLLFUNCTION( OGL, void , glEvalCoord2fv,( const GLfloat *u ),0,0);

DLLFUNCTION( OGL, void , glMapGrid1d,( GLint un, GLdouble u1, GLdouble u2 ),0,0);
DLLFUNCTION( OGL, void , glMapGrid1f,( GLint un, GLfloat u1, GLfloat u2 ),0,0);

DLLFUNCTION( OGL, void , glMapGrid2d,( GLint un, GLdouble u1, GLdouble u2,
                                     GLint vn, GLdouble v1, GLdouble v2 ),0,0);
DLLFUNCTION( OGL, void , glMapGrid2f,( GLint un, GLfloat u1, GLfloat u2,
                                     GLint vn, GLfloat v1, GLfloat v2 ),0,0);

DLLFUNCTION( OGL, void , glEvalPoint1,( GLint i ),0,0);

DLLFUNCTION( OGL, void , glEvalPoint2,( GLint i, GLint j ),0,0);

DLLFUNCTION( OGL, void , glEvalMesh1,( GLenum mode, GLint i1, GLint i2 ),0,0);

DLLFUNCTION( OGL, void , glEvalMesh2,( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 ),0,0);



/*
 * Fog
 */

DLLFUNCTION( OGL, void , glFogf,( GLenum pname, GLfloat param ),0,0);

DLLFUNCTION( OGL, void , glFogi,( GLenum pname, GLint param ),0,0);

DLLFUNCTION( OGL, void , glFogfv,( GLenum pname, const GLfloat *params ),0,0);

DLLFUNCTION( OGL, void , glFogiv,( GLenum pname, const GLint *params ),0,0);



/*
 * Selection and Feedback
 */

DLLFUNCTION( OGL, void , glFeedbackBuffer,( GLsizei size, GLenum type, GLfloat *buffer ),0,0);

DLLFUNCTION( OGL, void , glPassThrough,( GLfloat token ),0,0);

DLLFUNCTION( OGL, void , glSelectBuffer,( GLsizei size, GLuint *buffer ),0,0);

DLLFUNCTION( OGL, void , glInitNames,( void ),0,0);

DLLFUNCTION( OGL, void , glLoadName,( GLuint name ),0,0);

DLLFUNCTION( OGL, void , glPushName,( GLuint name ),0,0);

DLLFUNCTION( OGL, void , glPopName,( void ),0,0);



/*
 * 1.0 Extensions
 */

/* GL_EXT_blend_minmax */
DLLFUNCTION( OGL, void , glBlendEquationEXT,( GLenum mode ),0,0);



/* GL_EXT_blend_color */
DLLFUNCTION( OGL, void , glBlendColorEXT,( GLclampf red, GLclampf green,
                                         GLclampf blue, GLclampf alpha ),0,0);



/* GL_EXT_polygon_offset */
DLLFUNCTION( OGL, void , glPolygonOffsetEXT,( GLfloat factor, GLfloat bias ),0,0);



/* GL_EXT_vertex_array */

DLLFUNCTION( OGL, void , glVertexPointerEXT,( GLint size, GLenum type,
                                            GLsizei stride,
                                            GLsizei count, const GLvoid *ptr ),0,0);

DLLFUNCTION( OGL, void , glNormalPointerEXT,( GLenum type, GLsizei stride,
                                            GLsizei count, const GLvoid *ptr ),0,0);

DLLFUNCTION( OGL, void , glColorPointerEXT,( GLint size, GLenum type,
                                           GLsizei stride,
                                           GLsizei count, const GLvoid *ptr ),0,0);

DLLFUNCTION( OGL, void , glIndexPointerEXT,( GLenum type, GLsizei stride,
                                           GLsizei count, const GLvoid *ptr ),0,0);

DLLFUNCTION( OGL, void , glTexCoordPointerEXT,( GLint size, GLenum type,
                                              GLsizei stride, GLsizei count,
                                              const GLvoid *ptr ),0,0);

DLLFUNCTION( OGL, void , glEdgeFlagPointerEXT,( GLsizei stride, GLsizei count,
                                              const GLboolean *ptr ),0,0);

DLLFUNCTION( OGL, void , glGetPointervEXT,( GLenum pname, void **params ),0,0);

DLLFUNCTION( OGL, void , glArrayElementEXT,( GLint i ),0,0);

DLLFUNCTION( OGL, void , glDrawArraysEXT,( GLenum mode, GLint first,
                                         GLsizei count ),0,0);



/* GL_EXT_texture_object */

DLLFUNCTION( OGL, void , glGenTexturesEXT,( GLsizei n, GLuint *textures ),0,0);

DLLFUNCTION( OGL, void , glDeleteTexturesEXT,( GLsizei n,
                                             const GLuint *textures),0,0);

DLLFUNCTION( OGL, void , glBindTextureEXT,( GLenum target, GLuint texture ),0,0);

DLLFUNCTION( OGL, void , glPrioritizeTexturesEXT,( GLsizei n,
                                                 const GLuint *textures,
                                                 const GLclampf *priorities ),0,0);

DLLFUNCTION( OGL, GLboolean , glAreTexturesResidentEXT,( GLsizei n,
                                                       const GLuint *textures,
                                                       GLboolean *residences ),0,0);

DLLFUNCTION( OGL, GLboolean , glIsTextureEXT,( GLuint texture ),0,0);



/* GL_EXT_texture3D */

DLLFUNCTION( OGL, void , glTexImage3DEXT,( GLenum target, GLint level,
                                         GLenum internalFormat,
                                         GLsizei width, GLsizei height,
                                         GLsizei depth, GLint border,
                                         GLenum format, GLenum type,
                                         const GLvoid *pixels ),0,0);

DLLFUNCTION( OGL, void , glTexSubImage3DEXT,( GLenum target, GLint level,
                                            GLint xoffset, GLint yoffset,
                                            GLint zoffset, GLsizei width,
                                            GLsizei height, GLsizei depth,
                                            GLenum format,
                                            GLenum type, const GLvoid *pixels),0,0);

DLLFUNCTION( OGL, void , glCopyTexSubImage3DEXT,( GLenum target, GLint level,
                                                GLint xoffset, GLint yoffset,
                                                GLint zoffset, GLint x,
                                                GLint y, GLsizei width,
                                                GLsizei height ),0,0);



/* GL_EXT_color_table */

DLLFUNCTION( OGL, void , glColorTableEXT,( GLenum target, GLenum internalformat,
                                         GLsizei width, GLenum format,
                                         GLenum type, const GLvoid *table ),0,0);

DLLFUNCTION( OGL, void , glColorSubTableEXT,( GLenum target,
                                            GLsizei start, GLsizei count,
                                            GLenum format, GLenum type,
                                            const GLvoid *data ),0,0);

DLLFUNCTION( OGL, void , glGetColorTableEXT,( GLenum target, GLenum format,
                                            GLenum type, GLvoid *table ),0,0);

DLLFUNCTION( OGL, void , glGetColorTableParameterfvEXT,( GLenum target,
                                                       GLenum pname,
                                                       GLfloat *params ),0,0);

DLLFUNCTION( OGL, void , glGetColorTableParameterivEXT,( GLenum target,
                                                       GLenum pname,
                                                       GLint *params ),0,0);


/* GL_SGIS_multitexture */

DLLFUNCTION( OGL, void , glMultiTexCoord1dSGIS,(GLenum target, GLdouble s),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord1dvSGIS,(GLenum target, const GLdouble *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord1fSGIS,(GLenum target, GLfloat s),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord1fvSGIS,(GLenum target, const GLfloat *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord1iSGIS,(GLenum target, GLint s),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord1ivSGIS,(GLenum target, const GLint *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord1sSGIS,(GLenum target, GLshort s),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord1svSGIS,(GLenum target, const GLshort *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord2dSGIS,(GLenum target, GLdouble s, GLdouble t),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord2dvSGIS,(GLenum target, const GLdouble *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord2fSGIS,(GLenum target, GLfloat s, GLfloat t),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord2fvSGIS,(GLenum target, const GLfloat *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord2iSGIS,(GLenum target, GLint s, GLint t),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord2ivSGIS,(GLenum target, const GLint *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord2sSGIS,(GLenum target, GLshort s, GLshort t),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord2svSGIS,(GLenum target, const GLshort *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord3dSGIS,(GLenum target, GLdouble s, GLdouble t, GLdouble r),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord3dvSGIS,(GLenum target, const GLdouble *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord3fSGIS,(GLenum target, GLfloat s, GLfloat t, GLfloat r),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord3fvSGIS,(GLenum target, const GLfloat *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord3iSGIS,(GLenum target, GLint s, GLint t, GLint r),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord3ivSGIS,(GLenum target, const GLint *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord3sSGIS,(GLenum target, GLshort s, GLshort t, GLshort r),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord3svSGIS,(GLenum target, const GLshort *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord4dSGIS,(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord4dvSGIS,(GLenum target, const GLdouble *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord4fSGIS,(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord4fvSGIS,(GLenum target, const GLfloat *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord4iSGIS,(GLenum target, GLint s, GLint t, GLint r, GLint q),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord4ivSGIS,(GLenum target, const GLint *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord4sSGIS,(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord4svSGIS,(GLenum target, const GLshort *v),0,0);

DLLFUNCTION( OGL, void , glMultiTexCoordPointerSGIS,(GLenum target, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer),0,0);

DLLFUNCTION( OGL, void , glSelectTextureSGIS,(GLenum target),0,0);

DLLFUNCTION( OGL, void , glSelectTextureCoordSetSGIS,(GLenum target),0,0);


/* GL_EXT_multitexture */

DLLFUNCTION( OGL, void , glMultiTexCoord1dEXT,(GLenum target, GLdouble s),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord1dvEXT,(GLenum target, const GLdouble *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord1fEXT,(GLenum target, GLfloat s),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord1fvEXT,(GLenum target, const GLfloat *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord1iEXT,(GLenum target, GLint s),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord1ivEXT,(GLenum target, const GLint *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord1sEXT,(GLenum target, GLshort s),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord1svEXT,(GLenum target, const GLshort *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord2dEXT,(GLenum target, GLdouble s, GLdouble t),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord2dvEXT,(GLenum target, const GLdouble *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord2fEXT,(GLenum target, GLfloat s, GLfloat t),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord2fvEXT,(GLenum target, const GLfloat *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord2iEXT,(GLenum target, GLint s, GLint t),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord2ivEXT,(GLenum target, const GLint *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord2sEXT,(GLenum target, GLshort s, GLshort t),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord2svEXT,(GLenum target, const GLshort *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord3dEXT,(GLenum target, GLdouble s, GLdouble t, GLdouble r),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord3dvEXT,(GLenum target, const GLdouble *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord3fEXT,(GLenum target, GLfloat s, GLfloat t, GLfloat r),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord3fvEXT,(GLenum target, const GLfloat *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord3iEXT,(GLenum target, GLint s, GLint t, GLint r),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord3ivEXT,(GLenum target, const GLint *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord3sEXT,(GLenum target, GLshort s, GLshort t, GLshort r),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord3svEXT,(GLenum target, const GLshort *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord4dEXT,(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord4dvEXT,(GLenum target, const GLdouble *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord4fEXT,(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord4fvEXT,(GLenum target, const GLfloat *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord4iEXT,(GLenum target, GLint s, GLint t, GLint r, GLint q),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord4ivEXT,(GLenum target, const GLint *v),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord4sEXT,(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q),0,0);
DLLFUNCTION( OGL, void , glMultiTexCoord4svEXT,(GLenum target, const GLshort *v),0,0);

DLLFUNCTION( OGL, void , glInterleavedTextureCoordSetsEXT,( GLint factor ),0,0);

DLLFUNCTION( OGL, void , glSelectTextureEXT,( GLenum target ),0,0);

DLLFUNCTION( OGL, void , glSelectTextureCoordSetEXT,( GLenum target ),0,0);

DLLFUNCTION( OGL, void , glSelectTextureTransformEXT,( GLenum target ),0,0);






/* GL_EXT_point_parameters */
DLLFUNCTION( OGL, void , glPointParameterfEXT,( GLenum pname, GLfloat param ),0,0);
DLLFUNCTION( OGL, void , glPointParameterfvEXT,( GLenum pname,
                                               const GLfloat *params ),0,0);


/* 1.2 functions */
DLLFUNCTION( OGL, void , glDrawRangeElements,( GLenum mode, GLuint start,
	GLuint end, GLsizei count, GLenum type, const GLvoid *indices ),0,0);

DLLFUNCTION( OGL, void , glTexImage3D,( GLenum target, GLint level,
                                      GLenum internalFormat,
                                      GLsizei width, GLsizei height,
                                      GLsizei depth, GLint border,
                                      GLenum format, GLenum type,
                                      const GLvoid *pixels ),0,0);

DLLFUNCTION( OGL, void , glTexSubImage3D,( GLenum target, GLint level,
                                         GLint xoffset, GLint yoffset,
                                         GLint zoffset, GLsizei width,
                                         GLsizei height, GLsizei depth,
                                         GLenum format,
                                         GLenum type, const GLvoid *pixels),0,0);

DLLFUNCTION( OGL, void , glCopyTexSubImage3D,( GLenum target, GLint level,
                                             GLint xoffset, GLint yoffset,
                                             GLint zoffset, GLint x,
                                             GLint y, GLsizei width,
                                             GLsizei height ),0,0);


/* !!! FIXME: This needs to move to a GL context abstraction layer. */
#ifdef PLATFORM_WIN32
// gdi functions
DLLFUNCTION( OGL, BOOL , wglCopyContext,(HGLRC, HGLRC, UINT),0,0);
DLLFUNCTION( OGL, HGLRC, wglCreateContext,(HDC),4,1);
DLLFUNCTION( OGL, HGLRC, wglCreateLayerContext,(HDC, int),0,0);
DLLFUNCTION( OGL, BOOL , wglDeleteContext,(HGLRC),4,1);
DLLFUNCTION( OGL, HGLRC, wglGetCurrentContext,(VOID),0,1);
DLLFUNCTION( OGL, HDC  , wglGetCurrentDC,(VOID),0,1);
DLLFUNCTION( OGL, PROC , wglGetProcAddress,(LPCSTR),4,1);
DLLFUNCTION( OGL, BOOL , wglMakeCurrent,(HDC, HGLRC),8,1);
DLLFUNCTION( OGL, BOOL , wglShareLists,(HGLRC, HGLRC),0,0);
DLLFUNCTION( OGL, BOOL , wglUseFontBitmapsA,(HDC, DWORD, DWORD, DWORD),0,0);
DLLFUNCTION( OGL, BOOL , wglUseFontBitmapsW,(HDC, DWORD, DWORD, DWORD),0,0);

DLLFUNCTION( OGL, BOOL, wglSwapBuffers, (HDC), 4,1);
DLLFUNCTION( OGL, BOOL, wglSetPixelFormat, (HDC, int, CONST PIXELFORMATDESCRIPTOR*),12,1);
DLLFUNCTION( OGL, int,  wglChoosePixelFormat, (HDC, CONST PIXELFORMATDESCRIPTOR*),8,1);
DLLFUNCTION( OGL, int,  wglDescribePixelFormat, (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR),16,1);
#endif

