/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "dmap.h"

#ifdef WIN32
#include <windows.h>
//#include <GL/gl.h>
//#include <GL/glu.h>
//#include <GL/glaux.h>
#include "renderer/backend/GLSLProgram.h"

#define	WIN_SIZE	1024

void Draw_ClearWindow( void ) {

	if ( !dmapGlobals.drawflag ) {
		return;
	}

	qglDrawBuffer( GL_FRONT );

	RB_SetGL2D();

	qglClearColor( 0.5, 0.5, 0.5, 0 );
	qglClear( GL_COLOR_BUFFER_BIT );
	
	//return back to fixed function pipeline
	GLSLProgram::Deactivate();

#if 0
	int		w, h, g;
	float	mx, my;

	w = (dmapGlobals.drawBounds.b[1][0] - dmapGlobals.drawBounds.b[0][0]);
	h = (dmapGlobals.drawBounds.b[1][1] - dmapGlobals.drawBounds.b[0][1]);

	mx = dmapGlobals.drawBounds.b[0][0] + w/2;
	my = dmapGlobals.drawBounds.b[1][1] + h/2;

	g = w > h ? w : h;

	glLoadIdentity ();
    gluPerspective (90,  1,  2,  16384);
	gluLookAt (mx, my, draw_maxs[2] + g/2, mx , my, draw_maxs[2], 0, 1, 0);
#else
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity ();
	qglOrtho( dmapGlobals.drawBounds[0][0], dmapGlobals.drawBounds[1][0], 
		dmapGlobals.drawBounds[0][1], dmapGlobals.drawBounds[1][1],
		-1, 1 );
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity();
#endif
	qglColor3f (0,0,0);
//	qglPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglDisable (GL_DEPTH_TEST);
//	qglEnable (GL_BLEND);
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#if 0
//	qglColor4f (1,0,0,0.5);
//	qglBegin( GL_LINE_LOOP );
	qglBegin( GL_QUADS );

	qglVertex2f( dmapGlobals.drawBounds.b[0][0] + 20, dmapGlobals.drawBounds.b[0][1] + 20 );
	qglVertex2f( dmapGlobals.drawBounds.b[1][0] - 20, dmapGlobals.drawBounds.b[0][1] + 20 );
	qglVertex2f( dmapGlobals.drawBounds.b[1][0] - 20, dmapGlobals.drawBounds.b[1][1] - 20 );
	qglVertex2f( dmapGlobals.drawBounds.b[0][0] + 20, dmapGlobals.drawBounds.b[1][1] - 20 );

	qglEnd ();
#endif

	qglFlush ();

}

void Draw_SetRed (void)
{
	if (!dmapGlobals.drawflag)
		return;

	qglColor3f (1,0,0);
}

void Draw_SetGrey (void)
{
	if (!dmapGlobals.drawflag)
		return;

	qglColor3f( 0.5f, 0.5f, 0.5f);
}

void Draw_SetBlack (void)
{
	if (!dmapGlobals.drawflag)
		return;

	qglColor3f( 0.0f, 0.0f, 0.0f );
}

void DrawWinding ( const idWinding *w )
{
	int		i;

	if (!dmapGlobals.drawflag)
		return;

	qglColor3f( 0.3f, 0.0f, 0.0f );
	qglBegin (GL_POLYGON);
	for ( i = 0; i < w->GetNumPoints(); i++ )
		qglVertex3f( (*w)[i][0], (*w)[i][1], (*w)[i][2] );
	qglEnd ();

	qglColor3f( 1, 0, 0 );
	qglBegin (GL_LINE_LOOP);
	for ( i = 0; i < w->GetNumPoints(); i++ )
		qglVertex3f( (*w)[i][0], (*w)[i][1], (*w)[i][2] );
	qglEnd ();

	qglFlush ();
}

void DrawAuxWinding ( const idWinding *w)
{
	int		i;

	if (!dmapGlobals.drawflag)
		return;

	qglColor3f( 0.0f, 0.3f, 0.0f );
	qglBegin (GL_POLYGON);
	for ( i = 0; i < w->GetNumPoints(); i++ )
		qglVertex3f( (*w)[i][0], (*w)[i][1], (*w)[i][2] );
	qglEnd ();

	qglColor3f( 0.0f, 1.0f, 0.0f );
	qglBegin (GL_LINE_LOOP);
	for ( i = 0; i < w->GetNumPoints(); i++ )
		qglVertex3f( (*w)[i][0], (*w)[i][1], (*w)[i][2] );
	qglEnd ();

	qglFlush ();
}

void DrawLine( idVec3 v1, idVec3 v2, int color ) {
	if (!dmapGlobals.drawflag)
		return;

	switch( color ) {
	case 0: qglColor3f( 0, 0, 0 ); break;
	case 1: qglColor3f( 0, 0, 1 ); break;
	case 2: qglColor3f( 0, 1, 0 ); break;
	case 3: qglColor3f( 0, 1, 1 ); break;
	case 4: qglColor3f( 1, 0, 0 ); break;
	case 5: qglColor3f( 1, 0, 1 ); break;
	case 6: qglColor3f( 1, 1, 0 ); break;
	case 7: qglColor3f( 1, 1, 1 ); break;
	}
	

	qglBegin( GL_LINES );

	qglVertex3fv( v1.ToFloatPtr() );
	qglVertex3fv( v2.ToFloatPtr() );

	qglEnd();
	qglFlush();
}

//============================================================

#define	GLSERV_PORT	25001

bool	wins_init;
int			draw_socket;

void GLS_BeginScene (void)
{
	WSADATA	winsockdata;
	WORD	wVersionRequested; 
	struct sockaddr_in	address;
	int		r;

	if (!wins_init)
	{
		wins_init = true;

		wVersionRequested = MAKEWORD(1, 1); 

		r = WSAStartup (MAKEWORD(1, 1), &winsockdata);

		if (r)
			common->Error( "Winsock initialization failed.");

	}

	// connect a socket to the server

	draw_socket = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (draw_socket == -1)
		common->Error( "draw_socket failed");

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = GLSERV_PORT;
	r = connect (draw_socket, (struct sockaddr *)&address, sizeof(address));
	if (r == -1)
	{
		closesocket (draw_socket);
		draw_socket = 0;
	}
}

void GLS_Winding( const idWinding *w, int code )
{
	byte	buf[1024];
	int		i, j;

	if (!draw_socket)
		return;

	((int *)buf)[0] = w->GetNumPoints();
	((int *)buf)[1] = code;
	for ( i = 0; i < w->GetNumPoints(); i++ )
		for (j=0 ; j<3 ; j++)
			((float *)buf)[2+i*3+j] = (*w)[i][j];

	send (draw_socket, (const char *)buf, w->GetNumPoints() * 12 + 8, 0);
}

void GLS_Triangle( const mapTri_t *tri, int code ) {
	idWinding w;

	w.SetNumPoints( 3 );
	VectorCopy( tri->v[0].xyz, w[0] );
	VectorCopy( tri->v[1].xyz, w[1] );
	VectorCopy( tri->v[2].xyz, w[2] );
	GLS_Winding( &w, code );
}

void GLS_EndScene (void)
{
	closesocket (draw_socket);
	draw_socket = 0;
}
#else
void Draw_ClearWindow( void ) {
}

void DrawWinding( const idWinding *w) {
}

void DrawAuxWinding ( const idWinding *w) {
}

void GLS_Winding( const idWinding *w, int code ) {
}

void GLS_BeginScene (void) {
}

void GLS_EndScene (void)
{
}

#endif
