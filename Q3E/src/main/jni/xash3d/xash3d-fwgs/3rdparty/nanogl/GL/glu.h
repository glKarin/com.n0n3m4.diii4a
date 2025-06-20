/***************************************************************************
                          glu_rip.h  -  description

                A few convenience routines ripped from MesaGL
 ***************************************************************************/

#ifndef _GLU_RIP_H_
#define _GLU_RIP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <GL/gl.h>

#include <malloc.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void gluLookAt( GLdouble eyex, GLdouble eyey, GLdouble eyez,
                GLdouble centerx, GLdouble centery, GLdouble centerz,
                GLdouble upx, GLdouble upy, GLdouble upz );

void gluPerspective( GLdouble fovy, GLdouble aspect,
                     GLdouble zNear, GLdouble zFar );

GLint gluScaleImage( GLenum format,
                     GLint widthin, GLint heightin,
                     GLenum typein, const void *datain,
                     GLint widthout, GLint heightout,
                     GLenum typeout, void *dataout );

GLint gluBuild2DMipmaps( GLenum target, GLint components,
                         GLint width, GLint height, GLenum format,
                         GLenum type, const void *data );

#ifdef __cplusplus
}
#endif

#endif
