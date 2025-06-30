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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <lwsurf.h>
#include <lwhost.h>
#include <lwserver.h>
#include <lwgeneric.h>
#include <crtdbg.h>
#include <stdarg.h>
#include "vecmat.h"

#include <lwmonitor.h>
#include <lwrender.h>
#include <lwio.h>
#include <lwdyna.h>
#include <lwshader.h>
#include <lwserver.h>
#include <lwmotion.h>
#include <lwpanel.h>
#include <lwxpanel.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "base.h"

#ifndef PI
#define PI 3.1415926535897932384
#endif

extern SurfaceInstance *_psiFirst = NULL;  // linked list of surface instances

/*
======================================================================
Create()

Handler callback.  Allocate and initialize instance data.

The create function allocates a blotch struct and returns the pointer
as the instance.  Note that "Blotch *" is used throughout instead of
"LWInstance".  This works since a LWInstance type is a generic pointer
and can safely be replaced with any specific pointer type.  Instance
variables are initialized to some default values.
====================================================================== */

XCALL_( static LWInstance )
Create( void *priv, LWSurfaceID surf, LWError *err )
{
  SurfaceInstance *psi;

  psi = (SurfaceInstance*)calloc( 1, sizeof( SurfaceInstance ));
  if ( !psi ) {
   *err = "Couldn't allocate memory for instance.";
   return NULL;
  }

  psi->si_idSurface = surf;

  psi->si_psiNext = _psiFirst;
  _psiFirst = psi;

  return psi;
}


/*
======================================================================
Destroy()

Handler callback.  Free resources allocated by Create().
====================================================================== */

XCALL_( static void )
Destroy( SurfaceInstance *inst )
{
  SurfaceInstance **ppsi = &_psiFirst;

  while(*ppsi!=NULL) {
    if (*ppsi == inst) {
      *ppsi = (*ppsi)->si_psiNext;
      break;
    }
    ppsi = &((*ppsi)->si_psiNext);
  }

  free(inst);
}


/*
======================================================================
Copy()

Handler callback.  Copy instance data.
====================================================================== */

XCALL_( static LWError )
Copy( SurfaceInstance *to, SurfaceInstance *from )
{
   XCALL_INIT;

   *to = *from;
   return NULL;
}


/*
======================================================================
Load()

Handler callback.  Read instance data.  Shader instance data is stored
in the SURF chunks of object files, but it isn't necessary to know
that to read and write the data.
====================================================================== */

XCALL_( static LWError )
Load( SurfaceInstance *inst, const LWLoadState *ls )
{
  float af[3];

  LWLOAD_I1( ls, inst->si_strShader, sizeof(inst->si_strShader));
  LWLOAD_I1( ls, (char*)&inst->si_astrTextures, sizeof(inst->si_astrTextures));

  for (int i=0; i<sizeof(inst->si_adColors)/sizeof(inst->si_adColors[0]); i++) {
    LWLOAD_FP( ls, af, 3);
    inst->si_adColors[i][0] = af[0];
    inst->si_adColors[i][1] = af[1];
    inst->si_adColors[i][2] = af[2];
  }

  return NULL;
}


/*
======================================================================
Save()

Handler callback.  Write instance data.  The I/O functions in lwio.h
include one for reading and writing floats, but not doubles.  We just
transfer our double-precision data to a float variable before calling
the LWSAVE_FP() macro.
====================================================================== */

XCALL_( static LWError )
Save( SurfaceInstance *inst, const LWSaveState *ss )
{
  float af[3];
  LWSAVE_I1( ss, inst->si_strShader, sizeof(inst->si_strShader));
  LWSAVE_I1( ss, (char*)&inst->si_astrTextures, sizeof(inst->si_astrTextures));

  for (int i=0; i<sizeof(inst->si_adColors)/sizeof(inst->si_adColors[0]); i++) {
    af[0] = (float)inst->si_adColors[i][0];
    af[1] = (float)inst->si_adColors[i][1];
    af[2] = (float)inst->si_adColors[i][2];
    LWSAVE_FP( ss, af, 3);
  }

  return NULL;
}


/*
======================================================================
DescLn()

Handler callback.  Write a one-line text description of the instance
data.  Since the string must persist after this is called, it's part
of the instance.
====================================================================== */

XCALL_( static const char * )
DescLn( SurfaceInstance *inst )
{
   sprintf(inst->desc, "SE_Shader  (%s)", inst->si_strShader);
   return inst->desc;
}


XCALL_( static LWError )
Init( SurfaceInstance *inst, int mode )
{

   return NULL;
}
XCALL_( static void )
Cleanup( SurfaceInstance *inst )
{
   return;
}
XCALL_( static LWError )
NewTime( SurfaceInstance *inst, LWFrame f, LWTime t )
{
   return NULL;
}
XCALL_( static unsigned int )
Flags( SurfaceInstance *inst )
{
   return 0;  // we don't actually do any rendering in LW
}
XCALL_( static void )
Evaluate( SurfaceInstance *inst, LWShaderAccess *sa )
{
}


/*
======================================================================
Handler()

Handler activation function.  Check the version and fill in the
callback fields of the handler structure.
====================================================================== */

XCALL_(int) Handler_SurfaceParameters( long version, GlobalFunc *global, LWShaderHandler *local, void *serverData)
{
   if ( version != LWSHADER_VERSION ) return AFUNC_BADVERSION;

   (void*&)local->inst->create   = Create;
   (void*&)local->inst->destroy  = Destroy;
   (void*&)local->inst->load     = Load;
   (void*&)local->inst->save     = Save;
   (void*&)local->inst->copy     = Copy;
   (void*&)local->inst->descln   = DescLn;
   (void*&)local->rend->init     = Init;
   (void*&)local->rend->cleanup  = Cleanup;
   (void*&)local->rend->newTime  = NewTime;
   (void*&)local->evaluate       = Evaluate;
   (void*&)local->flags          = Flags;

   return AFUNC_OK;
}


/* interface stuff ----- */

static LWXPanelFuncs *xpanf;
static LWColorActivateFunc *colorpick;
static LWInstUpdate *lwupdate;

enum { 
  ID_SHADER = 0x8001, 
  ID_TEXTURE0, ID_TEXTURE1, ID_TEXTURE2, ID_TEXTURE3,
  ID_TEXTURE4, ID_TEXTURE5, ID_TEXTURE6, ID_TEXTURE7,
  ID_COLOR0, ID_COLOR1, ID_COLOR2, ID_COLOR3,
  ID_COLOR4, ID_COLOR5, ID_COLOR6, ID_COLOR7
};


/*
======================================================================
handle_color()

Event callback for the color button.  Called by LWXPanels.  Opens the
user's installed color picker.
====================================================================== */

static void handle_color( LWXPanelID panel, int cid )
{
/*   LWColorPickLocal local;
   SurfaceInstance *inst;
   int result;

   inst = (SurfaceInstance *)xpanf->getData( panel, 0 );

   local.result  = 0;
   local.title   = "Surface Color";
   local.red     = ( float ) inst->color[ 0 ];
   local.green   = ( float ) inst->color[ 1 ];
   local.blue    = ( float ) inst->color[ 2 ];
   local.data    = NULL;
   local.hotFunc = NULL;

   result = colorpick( LWCOLORPICK_VERSION, &local );
   if ( result == AFUNC_OK && local.result > 0 ) {
      inst->color[ 0 ] = local.red;
      inst->color[ 1 ] = local.green;
      inst->color[ 2 ] = local.blue;
      lwupdate( LWSHADER_HCLASS, inst );
   }*/
}


/*
======================================================================
ui_get()

Xpanels callback for LWXP_VIEW panels.  Returns a pointer to the data
for a given control value.
====================================================================== */

void *ui_get( SurfaceInstance *inst, unsigned long vid )
{
  if (vid == ID_SHADER) {
    return inst->si_strShader;
  } else if (vid>=ID_COLOR0 && vid<=ID_COLOR7) {
    return &inst->si_adColors[vid-ID_COLOR0];
  } else if (vid>=ID_TEXTURE0 && vid<=ID_TEXTURE7) {
    return inst->si_astrTextures[vid-ID_TEXTURE0];
  } else {
    return NULL;
  }
}


/*
======================================================================
ui_set()

Xpanels callback for LWXP_VIEW panels.  Store a value in our instance
data.
====================================================================== */

int ui_set( SurfaceInstance *inst, unsigned long vid, void *value )
{
  if (vid == ID_SHADER) {
    strcpy(inst->si_strShader, (char*)value);
    return 1;
  } else if (vid>=ID_TEXTURE0 && vid<=ID_TEXTURE7) {
    strcpy(inst->si_astrTextures[vid - ID_TEXTURE0], (char*)value);
    return 1;
  } else if (vid>=ID_COLOR0 && vid<=ID_COLOR7) {
    double *ad = (double *)value;
    inst->si_adColors[vid - ID_COLOR0][0] = ad[0];
    inst->si_adColors[vid - ID_COLOR0][1] = ad[1];
    inst->si_adColors[vid - ID_COLOR0][2] = ad[2];
    return 1;
  } else {
    return 0;
  }
}


/*
======================================================================
ui_chgnotify()

XPanel callback.  XPanels calls this when an event occurs that affects
the value of one of your controls.  We use the instance update global
to tell Layout that our instance data has changed.
====================================================================== */

void ui_chgnotify( LWXPanelID panel, unsigned long cid, unsigned long vid,
   int event )
{
   void *dat;

   if ( event == LWXPEVENT_VALUE )
      if ( dat = xpanf->getData( panel, 0 ))
         lwupdate( LWSHADER_HCLASS, dat );
}


/*
======================================================================
get_panel()

Create and initialize an LWXP_VIEW panel.  Called by Interface().
====================================================================== */

LWXPanelID get_panel( SurfaceInstance *inst )
{
   static LWXPanelControl xctl[] = {
      { ID_SHADER,   "Shader",   "string",  },
      { ID_TEXTURE0,   "Texture0",   "string" },
      { ID_TEXTURE1,   "Texture1",   "string" },
      { ID_TEXTURE2,   "Texture2",   "string" },
      { ID_TEXTURE3,   "Texture3",   "string" },
      { ID_TEXTURE4,   "Texture4",   "string" },
      { ID_TEXTURE5,   "Texture5",   "string" },
      { ID_TEXTURE6,   "Texture6",   "string" },
      { ID_TEXTURE7,   "Texture7",   "string" },
      { ID_COLOR0,   "Color0",   "color" },
      { ID_COLOR1,   "Color1",   "color" },
      { ID_COLOR2,   "Color2",   "color" },
      { ID_COLOR3,   "Color3",   "color" },
      { ID_COLOR4,   "Color4",   "color" },
      { ID_COLOR5,   "Color5",   "color" },
      { ID_COLOR6,   "Color6",   "color" },
      { ID_COLOR7,   "Color7",   "color" },
      { 0 }
   };
   static LWXPanelDataDesc xdata[] = {
      { ID_SHADER,   "Shader",   "string" },
      { ID_COLOR0,   "Color0",   "float" },
      { ID_COLOR1,   "Color1",   "float" },
      { ID_COLOR2,   "Color2",   "float" },
      { ID_COLOR3,   "Color3",   "float" },
      { ID_COLOR4,   "Color4",   "float" },
      { ID_COLOR5,   "Color5",   "float" },
      { ID_COLOR6,   "Color6",   "float" },
      { ID_COLOR7,   "Color7",   "float" },
      { 0 }
   };
   static LWXPanelHint xhint[] = {
      XpLABEL( 0, "SE Shader" ),
      XpBUTNOTIFY( ID_COLOR0, handle_color ),
      XpCHGNOTIFY( ui_chgnotify ),
      XpEND
   };

   LWXPanelID panel;

   if ( panel = xpanf->create( LWXP_VIEW, xctl )) {
      xpanf->hint( panel, 0, xhint );
      xpanf->describe( panel, xdata, (void *(__cdecl *)(void *,unsigned long))ui_get, (enum en_LWXPRefreshCodes (__cdecl *)(void *,unsigned long,void *))ui_set );
      xpanf->viewInst( panel, inst );
      xpanf->setData( panel, 0, inst );
   }

   return panel;
}


/*
======================================================================
Interface()

The interface activation function.
====================================================================== */

XCALL_(int) Interface_SurfaceParameters( long version, GlobalFunc *global, LWInterface *local, void *serverData)
{
  _global     = global    ;
  _serverData = serverData;

  if ( version != LWINTERFACE_VERSION ) return AFUNC_BADVERSION;

 (void*&)colorpick = global( LWCOLORACTIVATEFUNC_GLOBAL, GFUSE_TRANSIENT );
 (void*&)lwupdate  = global( LWINSTUPDATE_GLOBAL,        GFUSE_TRANSIENT );
 (void*&)xpanf     = global( LWXPANELFUNCS_GLOBAL,       GFUSE_TRANSIENT );
 if ( !colorpick || !lwupdate || !xpanf ) return AFUNC_BADGLOBAL;

 local->panel   = get_panel( (SurfaceInstance*)local->inst );
 local->options = NULL;
 local->command = NULL;

 return local->panel ? AFUNC_OK : AFUNC_BADGLOBAL;
}
