/*
 * This file is part of the D3wasm project (http://www.continuation-labs.com/projects/d3wasm)
 * Copyright (c) 2019 Gabriel Cuvillier.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef QGLCOMPATPROC
#error "you must define QGLCOMPATPROC before including this file"
#endif

// OpenGL ES 1.1/OpenGL 1.5 compat
QGLCOMPATPROC(glPushAttrib, void, (GLint mask))
QGLCOMPATPROC(glPopAttrib, void, (void))
QGLCOMPATPROC(glPushMatrix, void, (void))
QGLCOMPATPROC(glPopMatrix, void, (void))
QGLCOMPATPROC(glMatrixMode, void, (GLenum mode))
QGLCOMPATPROC(glLoadIdentity, void, (void))
QGLCOMPATPROC(glOrtho, void, (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearZ, GLfloat farZ))
QGLCOMPATPROC(glRotatef, void, (GLfloat angle, GLfloat x, GLfloat y, GLfloat z))
QGLCOMPATPROC(glTranslatef, void, (GLfloat x, GLfloat y, GLfloat z))
QGLCOMPATPROC(glScalef, void, (GLfloat x, GLfloat y, GLfloat z))
QGLCOMPATPROC(glRectf, void, (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2))
QGLCOMPATPROC(glPolygonMode, void, (GLenum face, GLenum mode))
QGLCOMPATPROC(glVertex3f, void, (GLfloat x, GLfloat y, GLfloat z))
QGLCOMPATPROC(glNormal3f, void, (GLfloat x, GLfloat y, GLfloat z))
QGLCOMPATPROC(glVertex2f, void, (GLfloat x, GLfloat y))
QGLCOMPATPROC(glVertex2fv, void, (const GLfloat v[2]))
QGLCOMPATPROC(glVertex3fv, void, (const GLfloat v[3]))
QGLCOMPATPROC(glTexCoord2f, void, (GLfloat s, GLfloat t))
QGLCOMPATPROC(glTexCoord2fv, void, (const GLfloat st[2]))
QGLCOMPATPROC(glColor4f, void, (GLfloat r, GLfloat g, GLfloat b, GLfloat a))
QGLCOMPATPROC(glColor3f, void, (GLfloat r, GLfloat g, GLfloat b))
QGLCOMPATPROC(glColor3fv, void, (const GLfloat v[3]))
QGLCOMPATPROC(glColor4fv, void, (const GLfloat v[4]))
QGLCOMPATPROC(glColor4ubv, void, (const GLubyte v[4]))
QGLCOMPATPROC(glArrayElement, void, (GLint index))
QGLCOMPATPROC(glDisableClientState, void, (GLenum e))
QGLCOMPATPROC(glEnableClientState, void, (GLenum e))
QGLCOMPATPROC(glLoadMatrixf, void, (const GLfloat matrix[16]))
QGLCOMPATPROC(glMultMatrixf, void, (const GLfloat matrix[16]))
QGLCOMPATPROC(glVertexPointer, void, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer))
QGLCOMPATPROC(glBegin, void, (GLenum t))
QGLCOMPATPROC(glEnd, void, (void))
QGLCOMPATPROC(glRasterPos2f, void, (GLfloat x, GLfloat y))
QGLCOMPATPROC(glRasterPos3f, void, (GLfloat x, GLfloat y, GLfloat z))
QGLCOMPATPROC(glRasterPos3fv, void, (const GLfloat p[3]))
QGLCOMPATPROC(glCallLists, void, (GLsizei n, GLenum type, const GLvoid *lists))
QGLCOMPATPROC(glDrawPixels, void, (GLint width, GLint height, GLenum format, GLenum dataType, const void *data))

// GLU
QGLCOMPATPROC(gluSphere, void, (GLUquadricObj *, float r, int lats, int longs))

#undef QGLCOMPATPROC
