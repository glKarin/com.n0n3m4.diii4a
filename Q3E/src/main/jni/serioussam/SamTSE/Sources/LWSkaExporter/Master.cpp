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

#include "base.h"

extern LWMessageFuncs *_msg = NULL;
extern LWItemInfo *_iti = NULL;
extern LWObjectFuncs *_obf = NULL;
extern LWObjectInfo *_obi = NULL;
extern LWSceneInfo *_sci = NULL;
extern LWInterfaceInfo *_ifi = NULL;
extern LWItemID _objid = NULL;
extern LWMeshInfo *_pmesh = NULL;
extern LWChannelInfo *_chi = NULL;
extern LWSurfaceFuncs *_srf = NULL;
extern LWBoneInfo *_pbi = NULL;
extern GlobalFunc *_global = NULL;
extern const LWMasterAccess *_local = NULL;
extern void *_serverData = NULL;
extern int (*_evaluate) (void *, const char *command) = NULL;
extern LWXPanelFuncs *_xpanf = NULL;
extern LWColorActivateFunc *_colorpick = NULL;
extern LWInstUpdate *_lwupdate = NULL;
extern EDStateRef _state;

// modeler globals
extern MeshEditOp *_meshEditOperations = NULL;
extern LWObjectFuncs *_objfunc = NULL;
extern LWStateQueryFuncs *_statequery = NULL;

LWPanelID _ModelerPanel = 0;

MCData *_pmcMaster = NULL;  // the master settings for the scene

typedef int CommandFunc(const char *);

LWMessageFuncs  *message;

const char      *describe(LWInstance);
LWInstance      create(void *,void *,LWError *);
void            destroy(LWInstance);
LWError         copy(LWInstance,LWInstance);
LWError         load(LWInstance,const LWLoadState *);
LWError         save(LWInstance,const LWSaveState *);
double          process(LWInstance,const LWMasterAccess *);
unsigned int    flags(LWInstance);
bool            bExportAbsPositions = true;  // export first bone absolute position
int            _bExportAnimBackward = 0; // export animations from last to first

int ReloadGlobalObjects()
{
  // realise _msg
  _global(LWMESSAGEFUNCS_GLOBAL, GFUSE_RELEASE);
  // acquire _msg again(restart)
  _msg = (LWMessageFuncs *)_global(LWMESSAGEFUNCS_GLOBAL, GFUSE_ACQUIRE);
  if (_msg==NULL) {
   return AFUNC_BADGLOBAL;
  }
  // realise _iti
  _global(LWITEMINFO_GLOBAL, GFUSE_RELEASE);
  // acquire _iti again(restart)
  _iti = (LWItemInfo *)_global(LWITEMINFO_GLOBAL, GFUSE_ACQUIRE);
  if (_iti==NULL) {
    return AFUNC_BADGLOBAL;
  }
  // realise _obi
  _global(LWOBJECTINFO_GLOBAL, GFUSE_RELEASE);
  // acquire _obi again(restart)
  _obi = (LWObjectInfo *)_global(LWOBJECTINFO_GLOBAL, GFUSE_ACQUIRE);
  if (_obi==NULL) {
    return AFUNC_BADGLOBAL;
  }
  // realise _sci
  _global(LWSCENEINFO_GLOBAL, GFUSE_RELEASE);
  // acquire _sci again(restart)
  _sci = (LWSceneInfo *)_global(LWSCENEINFO_GLOBAL, GFUSE_ACQUIRE);
  if (_sci==NULL) {
    return AFUNC_BADGLOBAL;
  }
  // realise _ifi
  _global(LWINTERFACEINFO_GLOBAL, GFUSE_RELEASE);
  // acquire _ifi again(restart)
  _ifi = (LWInterfaceInfo *)_global(LWINTERFACEINFO_GLOBAL, GFUSE_ACQUIRE);
  if (_ifi==NULL) {
    return AFUNC_BADGLOBAL;
  }
  // realise _obf
  _global(LWOBJECTFUNCS_GLOBAL, GFUSE_RELEASE);
  // acquire _obf again(restart)
  _obf = (LWObjectFuncs *)_global(LWOBJECTFUNCS_GLOBAL, GFUSE_ACQUIRE);
  if (_obf==NULL) {
    return AFUNC_BADGLOBAL;
  }
  // realise _chi
  _global(LWCHANNELINFO_GLOBAL, GFUSE_RELEASE);
  // acquire _chi again(restart)
  _chi = (LWChannelInfo *)_global(LWCHANNELINFO_GLOBAL, GFUSE_ACQUIRE);
  if (_chi==NULL) {
    return AFUNC_BADGLOBAL;
  }
  // realise _srf
  _global(LWSURFACEFUNCS_GLOBAL, GFUSE_RELEASE);
  // acquire _srf again(restart)
  _srf = (LWSurfaceFuncs *)_global(LWSURFACEFUNCS_GLOBAL, GFUSE_ACQUIRE);
  if (_srf==NULL) {
    return AFUNC_BADGLOBAL;
  }
  // realise _pbi
  _global(LWBONEINFO_GLOBAL, GFUSE_RELEASE);
  // acquire _pbi again(restart)
  _pbi = (LWBoneInfo *)_global(LWBONEINFO_GLOBAL, GFUSE_ACQUIRE);
  if (_pbi==NULL) {
    return AFUNC_BADGLOBAL;
  }

  return(AFUNC_OK);
}

int Activate_Master(long version, GlobalFunc *global, LWMasterHandler *local, void *serverData)
{
  _global     = global    ;
  _serverData = serverData;

  _msg = (LWMessageFuncs *)_global(LWMESSAGEFUNCS_GLOBAL, GFUSE_ACQUIRE);
  if (_msg==NULL) {
   return AFUNC_BADGLOBAL;
  }
  _iti = (LWItemInfo *)_global(LWITEMINFO_GLOBAL, GFUSE_ACQUIRE);
  if (_iti==NULL) {
    return AFUNC_BADGLOBAL;
  }
  _obi = (LWObjectInfo *)_global(LWOBJECTINFO_GLOBAL, GFUSE_ACQUIRE);
  if (_obi==NULL) {
    return AFUNC_BADGLOBAL;
  }
  _sci = (LWSceneInfo *)_global(LWSCENEINFO_GLOBAL, GFUSE_ACQUIRE);
  if (_sci==NULL) {
    return AFUNC_BADGLOBAL;
  }
  _ifi = (LWInterfaceInfo *)_global(LWINTERFACEINFO_GLOBAL, GFUSE_ACQUIRE);
  if (_ifi==NULL) {
    return AFUNC_BADGLOBAL;
  }
  _obf = (LWObjectFuncs *)_global(LWOBJECTFUNCS_GLOBAL, GFUSE_ACQUIRE);
  if (_obf==NULL) {
    return AFUNC_BADGLOBAL;
  }
  _chi = (LWChannelInfo *)_global(LWCHANNELINFO_GLOBAL, GFUSE_ACQUIRE);
  if (_chi==NULL) {
    return AFUNC_BADGLOBAL;
  }
  _srf = (LWSurfaceFuncs *)_global(LWSURFACEFUNCS_GLOBAL, GFUSE_ACQUIRE);
  if (_srf==NULL) {
    return AFUNC_BADGLOBAL;
  }
  _pbi = (LWBoneInfo *)_global(LWBONEINFO_GLOBAL, GFUSE_ACQUIRE);
  if (_pbi==NULL) {
    return AFUNC_BADGLOBAL;
  }

  if(version != LWMASTER_VERSION)
        return(AFUNC_BADVERSION);

  if(local->inst)
  {
      local->inst->create     = create;
      local->inst->destroy    = destroy;
      local->inst->load       = load;
      local->inst->save       = save;
      local->inst->copy       = copy;
      local->inst->descln     = describe;
  }

  if(local->item)
  {
      local->item->useItems   = NULL;
      local->item->changeID   = NULL;
  }

  local->event            = process;
  local->flags            = flags;

  return(AFUNC_OK);
}

XCALL_(unsigned int) flags(LWInstance inst)
{
    return(0);
}

XCALL_(const char *) describe(LWInstance inst)
{
  return("SE Exporter master");
}

XCALL_(LWInstance) create(void *priv,void *context,LWError *err)
{
  if (_pmcMaster!=NULL) {
    *err = "Only one SE master plugin per scene is allowed.";
    return NULL;
  }

  _pmcMaster = (MCData *)malloc(sizeof(MCData));
  memset(_pmcMaster,0,sizeof(MCData));
  return _pmcMaster;
}

XCALL_(void) destroy(LWInstance inst)
{

  assert(_pmcMaster == (MCData *)inst);

  free(inst);
  _pmcMaster = NULL;
}

XCALL_(LWError) copy(LWInstance d1,LWInstance d2)
{
    XCALL_INIT;
    return(NULL);
}

XCALL_(LWError) load(LWInstance inst,const LWLoadState *state)
{
  LWLOAD_I4( state, (long*)&_pmcMaster->mc_iFaceForward, 1);
  // LWLOAD_I4( state, (long*)&_pmcMaster->mc_iAnimOrder, 1);

  return NULL;
}

XCALL_(LWError) save(LWInstance inst,const LWSaveState *state)
{
  LWSAVE_I4( state, (long*)&_pmcMaster->mc_iFaceForward, 1);
  // LWSAVE_I4( state, (long*)&_pmcMaster->mc_iAnimOrder, 1);
  return NULL;
}

XCALL_(double) process(LWInstance inst,const LWMasterAccess *ma)
{
  _local = ma;
  _evaluate = _local->evaluate;
  return((double)0.0);
}

/*
======================================================================
ui_get()

Xpanels callback for LWXP_VIEW panels.  Returns a pointer to the data
for a given control value.
====================================================================== */

void *ui_get( MCData *inst, unsigned long vid )
{
   switch ( vid ) {
      case ID_FACEFORWARD:    return &inst->mc_iFaceForward;
      case ID_ANIM_ORDER:     return &inst->mc_iAnimOrder;
      default:                return NULL;
   }
}


/*
======================================================================
ui_set()

Xpanels callback for LWXP_VIEW panels.  Store a value in our instance
data.
====================================================================== */

int ui_set( MCData *inst, unsigned long vid, void *value )
{
   double *d = ( double * ) value;

   switch ( vid ) {
      case ID_FACEFORWARD:
         inst->mc_iFaceForward = *(int*)value;
         break;
      case ID_ANIM_ORDER:
         inst->mc_iAnimOrder   = *(int*)value;

      default:
         return 0;
   }

   return 1;
}

char *astrFaceForward[] = {
  "not",
  "half-faceforward",
  "full-faceforward",
	NULL
};

char *astrAnimOrder[] = {
  "Normal",
  "Reversed",
	NULL
};

void Click_ExportAnim(LWXPanelID pan, int cid)
{
  ExportAnim(pan);
}
void Click_ExportSkeleton(LWXPanelID pan, int cid)
{
  ExportSkeleton();
  //_msg->error("Not implemented yet.", NULL);
}
void Click_ExportMesh(LWXPanelID pan, int cid)
{
  ExportMesh(pan);
}
void Click_ExportBones(LWXPanelID pan, int cid)
{
  ExportBones();
}

void Click_ExportSecAnim(LWXPanelID pan, int cid)
{
  ExportSecAnim(pan);
}


LWXPanelID get_panel(LWInstance inst)
{
   static LWXPanelControl xctl[] = {
      { ID_FACEFORWARD,     "FaceForward",					"iPopChoice",  },
      { ID_ANIM_ORDER,			"Animation order",			"iPopChoice", },
      { ID_EXPORTMESH,			"Export Mesh",					"vButton",  },
      { ID_EXPORTSKELETON,	"Export Skeleton",			"vButton",  },
      { ID_EXPORTANIM,			"Export Animation",			"vButton",  },
			{ ID_EXPORTBONES,			"Export Bones",					"vButton", },
			{ ID_EXPORTSECANIM,		"Export Sections Anim",	"vButton", },
      { ID_ABSPOSITIONS,		"Absolute positions",		"iBoolean", },
      { 0 }
   };
   static LWXPanelDataDesc xdata[] = {
      { ID_FACEFORWARD,    "FaceForward",  "integer",  },
      { ID_ANIM_ORDER,     "Animation order",  "integer",  },
      { 0 }
   };
   static LWXPanelHint xhint[] = {
      XpLABEL( 0, "SE Exporter master controls" ),
      XpSTRLIST(ID_FACEFORWARD, astrFaceForward),
      XpSTRLIST(ID_ANIM_ORDER , astrAnimOrder),
      XpBUTNOTIFY( ID_EXPORTMESH, Click_ExportMesh ),
      XpBUTNOTIFY( ID_EXPORTSKELETON, Click_ExportSkeleton),
      XpBUTNOTIFY( ID_EXPORTANIM, Click_ExportAnim),
			XpBUTNOTIFY( ID_EXPORTBONES, Click_ExportBones),
			XpBUTNOTIFY( ID_EXPORTSECANIM, Click_ExportSecAnim),
      //XpCHGNOTIFY( /*ID_ABSPOSITIONS, */Click_AbsPositions),
      //XpCHGNOTIFY( /*ID_ANIM_BACKWARD , */Click_AnimBackward),
      //XpBUTNOTIFY( ID_COLOR, handle_color),
      //XpCHGNOTIFY( ui_chgnotify),
      XpEND
   };

   LWXPanelID panel;

   if ( panel = _xpanf->create( LWXP_VIEW, xctl )) {
      _xpanf->hint( panel, 0, xhint );
      _xpanf->describe( panel, xdata,
        (void *(__cdecl *)(void *,unsigned long))ui_get, 
        (enum en_LWXPRefreshCodes (__cdecl *)(void *,unsigned long,void *))ui_set );
      _xpanf->viewInst( panel, inst );
      _xpanf->setData( panel, 0, inst );
   }

   return panel;
}

XCALL_(int) Interface_Master(long version, GlobalFunc *global, LWInterface *local, void *serverData)
{
  _global     = global    ;
  _serverData = serverData;


  if ( version != LWINTERFACE_VERSION ) return AFUNC_BADVERSION;

  (void*&)_colorpick = global( LWCOLORACTIVATEFUNC_GLOBAL, GFUSE_TRANSIENT );
  (void*&)_lwupdate  = global( LWINSTUPDATE_GLOBAL,        GFUSE_TRANSIENT );
  (void*&)_xpanf     = global( LWXPANELFUNCS_GLOBAL,       GFUSE_TRANSIENT );
  if ( !_colorpick || !_lwupdate || !_xpanf ) return AFUNC_BADGLOBAL;

  local->panel   = get_panel(local->inst);
  local->options = NULL;
  local->command = NULL;

  if (local->panel==NULL) {
  return AFUNC_BADGLOBAL;
  }

//  for(SurfaceInstance *psi = _psiFirst; psi!=NULL; psi = psi->si_psiNext) {
//    (*message->info)("surface: ", _srf->name(psi->si_idSurface));
//  }
  return(AFUNC_OK);
}




/*======================================================================
Modeler Exporter

	The next part of the CPP is for the modeler exporter plugin
====================================================================== */




int msgbox_modeler( LWXPanelFuncs *xpanf, const char* msg ) 
{

  LWXPanelID panel;
  int ok = 0;

	enum { ID_MSG = 0x8001 };
	LWXPanelControl ctl[] = {
		{ ID_MSG,    "Message: ",       "string",  },
		{ 0 }
	};
  LWXPanelDataDesc cdata[] = {
    { ID_MSG,    "Message: ",  "string",  },
    { 0 }
	};
  LWXPanelHint hint[] = {
    XpLABEL( 0, "SE Mesh Exporter master controls" ),
    XpLABEL(ID_MSG, msg),
    XpEND
  };


	panel = xpanf->create( LWXP_FORM, ctl );
	if ( !panel ) return 0;

	xpanf->describe( panel, cdata, NULL, NULL );
	xpanf->hint( panel, 0, hint );
	xpanf->formSet( panel, ID_MSG, (void*) msg );
	
	
	ok = xpanf->post( panel );

  xpanf->destroy( panel );
  return ok;

}



int get_user_modeler( LWXPanelFuncs *xpanf ) 
{

  LWXPanelID panel;
  int ok = 0;

	enum { ID_FACEFORWARD = 0x8001, ID_ANIM_ORDER};
	LWXPanelControl ctl[] = {
		{ ID_FACEFORWARD,    "FaceForward",       "iPopChoice",  },
		{ 0 }
	};
  LWXPanelDataDesc cdata[] = {
    { ID_FACEFORWARD,    "FaceForward",  "integer",  },
    { 0 }
	};
  LWXPanelHint hint[] = {
    XpLABEL( 0, "SE Mesh Exporter master controls" ),
    XpSTRLIST(ID_FACEFORWARD, astrFaceForward),
    XpEND
  };


	panel = xpanf->create( LWXP_FORM, ctl );
	if ( !panel ) return 0;

	xpanf->describe( panel, cdata, NULL, NULL );
	xpanf->hint( panel, 0, hint );
	xpanf->formSet( panel, ID_FACEFORWARD, 0 );
	
	
	ok = xpanf->post( panel );

  int iFaceForward = *(int*)_xpanf->formGet( panel, ID_FACEFORWARD);
  if ( ok ) {
		ExportMesh_modeler(iFaceForward);
	}

  xpanf->destroy( panel );
  return ok;

}



XCALL_ ( int ) Activate_ModelerMeshExporter( long version, GlobalFunc *global, MeshEditBegin *local, void *serverData ) {

	_global     = global    ;
  _serverData = serverData;

  if ( version != LWINTERFACE_VERSION ) return AFUNC_BADVERSION;


  _meshEditOperations = local(1,1,OPSEL_GLOBAL);	

	_xpanf = (LWXPanelFuncs *) _global( LWXPANELFUNCS_GLOBAL, GFUSE_TRANSIENT );
  if (_xpanf==NULL) {
    return AFUNC_BADGLOBAL;
  }

	_msg = (LWMessageFuncs *) _global(LWMESSAGEFUNCS_GLOBAL, GFUSE_ACQUIRE );
  if (_msg==NULL) {
    return AFUNC_BADGLOBAL;
  }

	
	_objfunc = (LWObjectFuncs *) _global( LWOBJECTFUNCS_GLOBAL, GFUSE_TRANSIENT );
	if (_objfunc==NULL) {
    return AFUNC_BADGLOBAL;
  }

	_statequery = (LWStateQueryFuncs *) _global( LWSTATEQUERYFUNCS_GLOBAL, GFUSE_TRANSIENT );
	if (_statequery==NULL) {
    return AFUNC_BADGLOBAL;
  }


	_srf = (LWSurfaceFuncs *) _global(LWSURFACEFUNCS_GLOBAL, GFUSE_ACQUIRE);
  if (_srf==NULL) {
    return AFUNC_BADGLOBAL;
  }

  get_user_modeler(_xpanf);
	global( LWXPANELFUNCS_GLOBAL, GFUSE_RELEASE );
	global( LWMESSAGEFUNCS_GLOBAL, GFUSE_RELEASE );
	global( LWSURFACEFUNCS_GLOBAL, GFUSE_RELEASE );
	
	_meshEditOperations->done( _meshEditOperations->state, EDERR_NONE, 0 );

  return(AFUNC_OK);
};




/*===================================================================================
Surface to weights

	The next part of the CPP is for the surface->weights modeler converter plugin
================================================================================== */





EDError AddToWeightmap (void *strSurf, const EDPolygonInfo *ppliPolyInfo)
{
	static float fWeightOne[] = {1.0f};

	if (strcmp(ppliPolyInfo->surface, (char*)strSurf)==0) {
		for (int i=0;i<ppliPolyInfo->numPnts;i++) {
			_meshEditOperations->pntVMap(_state,ppliPolyInfo->points[i],LWVMAP_WGHT,(char*)strSurf,1,fWeightOne);
		}
	}
	return EDERR_NONE;
};


XCALL_(int) Activate_ModelerSurfaceToWeights( long version, GlobalFunc *global, MeshEditBegin *local, void *serverData )
{
	LWSurfaceID *asurSurfaces;
	int ctSurfs;
	char *strFileName;

	_global     = global;
  _serverData = serverData;

  if ( version != LWINTERFACE_VERSION ) return AFUNC_BADVERSION;


  _meshEditOperations = local(1,1,OPSEL_GLOBAL);
	_state = _meshEditOperations->state; 

	_srf = (LWSurfaceFuncs *) _global(LWSURFACEFUNCS_GLOBAL, GFUSE_ACQUIRE);
  if (_srf==NULL) {
    return AFUNC_BADGLOBAL;
  }

	_statequery = (LWStateQueryFuncs *) _global( LWSTATEQUERYFUNCS_GLOBAL, GFUSE_TRANSIENT );
	if (_statequery==NULL) {
    return AFUNC_BADGLOBAL;
  }

	strFileName = strdup(_statequery->object());

	// get surfaces
  asurSurfaces = _srf->byObject(strFileName);

  // count the surfaces
  ctSurfs = 0;
  while(asurSurfaces[ctSurfs]!=NULL) {
    ctSurfs++;
  }

	{for(int iSurf=0; iSurf<ctSurfs; iSurf++) {

    const char *strSurf = _srf->name(asurSurfaces[iSurf]);
    // for each polygon in the surface, add weight map values for all it's points
		// this does the job a few times for each point, but it's the simplest way to do it
		_meshEditOperations->polyScan(_state,AddToWeightmap,(void*)strSurf,OPLYR_FG);
  }}


	_meshEditOperations->done( _meshEditOperations->state, EDERR_NONE, 0 );

  return(AFUNC_OK);

};


















/*===================================================================================
Copy weight maps

	The next part of the CPP is for the CopyWeightMaps plugin that copies weight maps 
	from the vertices in the background layers to the corresponding vertices in the
	foreground layers. Vertices are matched by position.
================================================================================== */


XCALL_(int) Activate_CopyWeightMaps( long version, GlobalFunc *global, MeshEditBegin *local, void *serverData )
{
	char *strFileName;

	_global     = global;
  _serverData = serverData;

  if ( version != LWINTERFACE_VERSION ) return AFUNC_BADVERSION;


  _meshEditOperations = local(1,1,OPSEL_GLOBAL);
	_state = _meshEditOperations->state; 

	_srf = (LWSurfaceFuncs *) _global(LWSURFACEFUNCS_GLOBAL, GFUSE_ACQUIRE);
  if (_srf==NULL) {
    return AFUNC_BADGLOBAL;
  }

	_statequery = (LWStateQueryFuncs *) _global( LWSTATEQUERYFUNCS_GLOBAL, GFUSE_TRANSIENT );
	if (_statequery==NULL) {
    return AFUNC_BADGLOBAL;
  }

	_objfunc = (LWObjectFuncs *) _global( LWOBJECTFUNCS_GLOBAL, GFUSE_TRANSIENT );
	if (_objfunc==NULL) {
    return AFUNC_BADGLOBAL;
  }

	strFileName = strdup(_statequery->object());

	

	ListWeightMaps();
	ScanBackground();

	// for each point in the foreground layers, look for it's match in the 
	// background layer and copy the weight map value
	_meshEditOperations->pointScan(_state,CopyWeightMaps,NULL,OPLYR_FG);
	

	_meshEditOperations->done( _meshEditOperations->state, EDERR_NONE, 0 );


	FreeMem();

  return(AFUNC_OK);

};
