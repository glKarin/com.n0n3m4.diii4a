/*
Copyright (C) 2006-2007 Mark Olsen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/*
This file is currently used only by bigfoot's MorphOS port.

I should explain this file to any Unix/Windows programmers...
Over on amiga-like operating systems, each system function to dynamic libraries is defined as a macro to a jmp statement.

along the lines of ((myfunc_t)((char*)library + offset))(parameters);
Obviously, in an engine that likes function pointers to all the gl functions, this is somewhat problematic.

You can see the state of his opengl library be seeing which functions are actually implemented. :)
*/

/*
 * PS: It is Kiero's library and everything _IS_ implemented ;)
 *   - bigfoot
 *
 * Oh, and just FYI, the offsets are negative, that is, the jump table is below
 * the library base pointer.
 */

void stub_glAlphaFunc(GLenum func, GLclampf ref)
{
	glAlphaFunc(func, ref);
}

void stub_glBegin(GLenum mode)
{
	glBegin(mode);
}

void stub_glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	glBlendFunc(sfactor, dfactor);
}

void stub_glBindTexture(GLenum target, GLuint texture)
{
	glBindTexture(target, texture);
}

void stub_glClear(GLbitfield mask)
{
	glClear(mask);
}

void stub_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	glClearColor(red, green, blue, alpha);
}

void stub_glClearDepth(GLclampd depth)
{
	glClearDepth(depth);
}

void stub_glClearStencil(GLint s)
{
	glClearStencil(s);
}

void stub_glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
	glColor3f(red, green, blue);
}

void stub_glColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
	glColor3ub(red, green, blue);
}

void stub_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	glColor4f(red, green, blue, alpha);
}

void stub_glColor4fv(const GLfloat *v)
{
	glColor4fv(v);
}

void stub_glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	glColor4ub(red, green, blue, alpha);
}

void stub_glColor4ubv(const GLubyte *v)
{
	glColor4ubv(v);
}

void stub_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	glColorMask(red, green, blue, alpha);
}

void stub_glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	glCopyTexImage2D(target, level, internalFormat, x, y, width, height, border);
}

void stub_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

void stub_glCullFace(GLenum mode)
{
	glCullFace(mode);
}

void stub_glDepthFunc(GLenum func)
{
	glDepthFunc(func);
}

void stub_glDepthMask(GLboolean flag)
{
	glDepthMask(flag);
}

void stub_glDepthRange(GLclampd zNear, GLclampd zFar)
{
	glDepthRange(zNear, zFar);
}

void stub_glDisable(GLenum cap)
{
	glDisable(cap);
}

void stub_glDrawBuffer(GLenum mode)
{
	glDrawBuffer(mode);
}

void stub_glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	glDrawPixels(width, height, format, type, pixels);
}

void stub_glEnable(GLenum cap)
{
	glEnable(cap);
}

void stub_glEnd(void)
{
	glEnd();
}

void stub_glFinish(void)
{
	glFinish();
}

void stub_glFlush(void)
{
	glFlush();
}

void stub_glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	glFrustum(left, right, bottom, top, zNear, zFar);
}

void stub_glGetFloatv(GLenum pname, GLfloat *params)
{
	glGetFloatv(pname, params);
}

void stub_glGetIntegerv(GLenum pname, GLint *params)
{
	glGetIntegerv(pname, params);
}

const GLubyte *stub_glGetString(GLenum name)
{
	return glGetString(name);
}

void stub_glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
	glGetTexLevelParameteriv(target, level, pname, params);
}

void stub_glHint(GLenum target, GLenum mode)
{
	glHint(target, mode);
}

void stub_glLoadIdentity(void)
{
	glLoadIdentity();
}

void stub_glLoadMatrixf(const GLfloat *m)
{
	glLoadMatrixf(m);
}

void stub_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
	glNormal3f(nx, ny, nz);
}

void stub_glNormal3fv(const GLfloat *v)
{
	glNormal3fv(v);
}

void stub_glMatrixMode(GLenum mode)
{
	glMatrixMode(mode);
}

void stub_glMultMatrixf(const GLfloat *m)
{
	glMultMatrixf(m);
}

void stub_glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	glOrtho(left, right, bottom, top, zNear, zFar);
}

void stub_glPolygonMode(GLenum face, GLenum mode)
{
	glPolygonMode(face, mode);
}

void stub_glPopMatrix(void)
{
	glPopMatrix();
}

void stub_glPushMatrix(void)
{
	glPushMatrix();
}

void stub_glReadBuffer(GLenum mode)
{
	glReadBuffer(mode);
}

void stub_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
	glReadPixels(x, y, width, height, format, type, pixels);
}

void stub_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	glRotatef(angle, x, y, z);
}

void stub_glScalef(GLfloat x, GLfloat y, GLfloat z)
{
	glScalef(x, y, z);
}

void stub_glShadeModel(GLenum mode)
{
	glShadeModel(mode);
}

void stub_glTexCoord1f(GLfloat s)
{
	glTexCoord1f(s);
}

void stub_glTexCoord2f(GLfloat s, GLfloat t)
{
	glTexCoord2f(s, t);
}

void stub_glTexCoord2fv(const GLfloat *v)
{
	glTexCoord2fv(v);
}

void stub_glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
	glTexEnvf(target, pname, param);
}

void stub_glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
	glTexEnvfv(target, pname, params);
}

void stub_glTexEnvi(GLenum target, GLenum pname, GLint param)
{
	glTexEnvi(target, pname, param);
}

void stub_glTexGeni(GLenum coord, GLenum pname, GLint param)
{
	glTexGeni(coord, pname, param);
}

void stub_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

void stub_glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	glTexParameteri(target, pname, param);
}

void stub_glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
	glTexParameterf(target, pname, param);
}

void stub_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void stub_glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	glTranslatef(x, y, z);
}

void stub_glVertex2f(GLfloat x, GLfloat y)
{
	glVertex2f(x, y);
}

void stub_glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	glVertex3f(x, y, z);
}

void stub_glVertex3fv(const GLfloat *v)
{
	glVertex3fv(v);
}

void stub_glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	glViewport(x, y, width, height);
}

GLenum stub_glGetError(void)
{
	return glGetError();
}

void stub_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
	glDrawElements(mode, count, type, indices);
}

void stub_glArrayElement(GLint i)
{
	glArrayElement(i);
}

void stub_glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	glVertexPointer(size, type, stride, pointer);
}

void stub_glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	glNormalPointer(type, stride, pointer);
}

void stub_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	glTexCoordPointer(size, type, stride, pointer);
}

void stub_glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	glColorPointer(size, type, stride, pointer);
}

void stub_glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	glDrawArrays(mode, first, count);
}

void stub_glEnableClientState(GLenum array)
{
	glEnableClientState(array);
}

void stub_glDisableClientState(GLenum array)
{
	glDisableClientState(array);
}

void stub_glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
	glStencilOp(fail, zfail, zpass);
}

void stub_glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
	glStencilFunc(func, ref, mask);
}

void stub_glPushAttrib(GLbitfield mask)
{
	glPushAttrib(mask);
}

void stub_glPopAttrib(void)
{
	glPopAttrib();
}

void stub_glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	glScissor(x, y, width, height);
}

void stub_glMultiTexCoord2fARB(GLenum unit, GLfloat s, GLfloat t)
{
	glMultiTexCoord2fARB(unit, s, t);
}

void stub_glMultiTexCoord3fARB(GLenum unit, GLfloat s, GLfloat t, GLfloat r)
{
	glMultiTexCoord3fARB(unit, s, t, r);
}

void stub_glActiveTextureARB(GLenum unit)
{
	glActiveTextureARB(unit);
}

void stub_glClientActiveTextureARB(GLenum unit)
{
	glClientActiveTextureARB(unit);
}
