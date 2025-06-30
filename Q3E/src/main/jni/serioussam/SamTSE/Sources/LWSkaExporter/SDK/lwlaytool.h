/*
 * LWSDK Header File
 * Copyright 2001, NewTek, Inc.
 *
 * LWLAYTOOL.H -- Layout Interactive Tools
 */
#ifndef LWSDK_LAYTOOL_H
#define LWSDK_LAYTOOL_H

#include <lwtool.h>
#include <lwcustobj.h>


#define LWLAYOUTTOOL_CLASS      "LayoutTool"
#define LWLAYOUTTOOL_VERSION    1

/*
 * A Layout tool is a LightWave viewport tool whose draw() function
 * takes a LWCustomObjAccess instead of a LWWireDrawAccess structure.
 * It has the following handler functions.
 *
 * done         destroy the instance when the user discards the tool.
 *
 * draw         display a wireframe representation of the tool in a 3D
 *              viewport.  Typically this draws the handles.
 *
 * help         return a text string to be displayed as a help tip for
 *              this tool.
 *
 * dirty        return flag bit if either the wireframe or help string
 *              need to be refreshed.
 *
 * count        return the number of handles.  If zero, then 'start' is
 *              used to set the initial handle point.
 *
 * handle       return the 3D location and priority of handle 'i', or zero
 *              if the handle is currently invalid.
 *
 * start        take an initial mouse-down position and return the index
 *              of the handle that should be dragged.
 *
 * adjust       drag the given handle to a new location and return the
 *              index of the handle that should continue being dragged
 *              (often the same as the input).
 *
 * down         process a mouse-down event.  If this function returns
 *              false, handle processing will be done instead of raw
 *              mouse event processing.
 *
 * move         process a mouse-move event.  This is only called if the
 *              down function returned true.
 *
 * up           process a mouse-up event.  This is only called if the down
 *              function returned true.
 *
 * event        process a general event: DROP, RESET or ACTIVATE
 *
 * panel        create and return a view-type xPanel for the tool instance.
 */
typedef struct st_LWLayoutToolFuncs {
        void            (*done)   (LWInstance);
        void            (*draw)   (LWInstance, LWCustomObjAccess *);
        const char *    (*help)   (LWInstance, LWToolEvent *);
        int             (*dirty)  (LWInstance);
        int             (*count)  (LWInstance, LWToolEvent *);
        int             (*handle) (LWInstance, LWToolEvent *, int i, LWDVector pos);
        int             (*start)  (LWInstance, LWToolEvent *);
        int             (*adjust) (LWInstance, LWToolEvent *, int i);
        int             (*down)   (LWInstance, LWToolEvent *);
        void            (*move)   (LWInstance, LWToolEvent *);
        void            (*up)     (LWInstance, LWToolEvent *);
        void            (*event)  (LWInstance, int code);
        LWXPanelID      (*panel)  (LWInstance);
} LWLayoutToolFuncs;

typedef struct st_LWLayoutTool {
        LWInstance        instance;
        LWLayoutToolFuncs *tool;
} LWLayoutTool;


#endif

