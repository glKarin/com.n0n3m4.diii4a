// Copyright (C) Ipion Software GmbH 1999-2000. All rights reserved.

//IVP_EXPORT_PUBLIC

/********************************************************************************
 *	File:	       	ivp_polygon.hxx	
 *	Description:	???
 ********************************************************************************/

#ifndef _IVP_POLYGON_INCLUDED
#define _IVP_POLYGON_INCLUDED

#include "ivp_core.hxx"

#include <hk_physics/physics.h>

/********************************************************************************
 *	Name:	   	IVP_Polygon    	
 *	Description:	A polygon, made up of triangles.
 *		        For convinience, the constructor handles all kinds of
 *		       	surfaces with more than three edges.
 *	Attention:	The polygon has to be a polyhedron (== object which is
 *			entirely covered by surfaces).
 *	Note:		To create a polygon, see IVP_Environment::create_polygon(...)
 ********************************************************************************/

class IVP_Polygon: public hk_Entity
{
protected:
    friend class IVP_Environment;
    IVP_Polygon(IVP_Cluster *father, IVP_SurfaceManager *surface_manager, const IVP_Template_Real_Object *real, const IVP_U_Quat *q_world_f_obj, const IVP_U_Point *position);
public:
};

#endif
