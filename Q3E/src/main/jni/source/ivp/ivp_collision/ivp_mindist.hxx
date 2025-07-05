// Copyright (C) Ipion Software GmbH 1999-2000. All rights reserved.

//IVP_EXPORT_PUBLIC

#ifndef _IVP_MINDIST_INCLUDED
#define _IVP_MINDIST_INCLUDED


class IVP_Mindist_Base;
class IVP_Compact_Edge;

#ifndef IVP_TIME_EVENT_INCLUDED
#    include <ivp_time_event.hxx>
#endif

#ifndef _IVP_LISTENER_HULL_INCLUDED
#	include <ivp_listener_hull.hxx>
#endif
#ifndef _IVP_COLLISION
#	include <ivp_collision.hxx>
#endif

//#include "ivp_mindist_intern.hxx"

//#define IVP_MINDIST_BEHAVIOUR_DEBUG	// do core as soon as possible


/* if IVP_HALFSPACE_OPTIMIZATION_ENABLED is defined, the mindist remembers a virtual plane
 * seperating both ledges. It tries to use that halfspace to optimize the hull */
#define IVP_HALFSPACE_OPTIMIZATION_ENABLED
//#define IVP_PHANTOM_FULL_COLLISION //@@CB - not default behaviour



/********************************************************************************
 *	Name:	       	IVP_SYNAPSE_POLYGON_STATUS
 *	Description:	How the synapse is connected to a geometry
 ********************************************************************************/
enum IVP_SYNAPSE_POLYGON_STATUS {
    IVP_ST_POINT = 0,
    IVP_ST_EDGE  = 1,
    IVP_ST_TRIANGLE =2,
    IVP_ST_BALL = 3,
    IVP_ST_MAX_LEGAL = 4,	// max legal status, should be 2**x
    IVP_ST_BACKSIDE = 5	        // unknown, intrusion
};



/********************************************************************************
 *	Name:	       	IVP_Synapse
 *	Description:	Synapses are attachments to objects
 ********************************************************************************/
class IVP_Synapse: public IVP_Listener_Hull {  // sizeof() == 32
public:
  IVP_Synapse 	 *next, *prev;		       // per object, only for exact/invalid synapses
  IVP_Real_Object  *l_obj;                     // back link to object
  const IVP_Compact_Edge 	*edge;		// Note: all balls share one dummy edge

protected:
    short mindist_offset;             // back link to my controlling mindist
    short status;                     // IVP_SYNAPSE_POLYGON_STATUS point, edge, tri, ball ....
public:
   
protected:
    // hull manager
  IVP_HULL_ELEM_TYPE get_type(){ return IVP_HULL_ELEM_POLYGON; };
  virtual void hull_limit_exceeded_event(IVP_Hull_Manager *hull_manager, IVP_HTIME hull_intrusion_value);
  virtual void hull_manager_is_going_to_be_deleted_event(IVP_Hull_Manager *hull_manager);
  virtual   void hull_manager_is_reset(IVP_FLOAT dt,IVP_FLOAT center_dt);
public:

    IVP_Real_Object *get_object(){ return l_obj; };
    IVP_SYNAPSE_POLYGON_STATUS get_status()const{ return (IVP_SYNAPSE_POLYGON_STATUS) status; };

    virtual ~IVP_Synapse(){;};			// dummy, do not call
    const IVP_Compact_Ledge *get_ledge() const;
    const IVP_Compact_Edge *get_edge() const { return edge; };

    IVP_Mindist_Base *get_synapse_mindist()const{ return  (IVP_Mindist_Base *)(mindist_offset + (char *)this);} ;
    void set_synapse_mindist( IVP_Mindist_Base *md ) { mindist_offset = ((char *)md) - (char *)this; };

    void init_synapse_real( IVP_Mindist_Base *min, IVP_Real_Object *object_to_link ){
	set_synapse_mindist(min);
	l_obj = object_to_link;
	IVP_IF(1){      next = prev = this;    }
    }
};

#endif













