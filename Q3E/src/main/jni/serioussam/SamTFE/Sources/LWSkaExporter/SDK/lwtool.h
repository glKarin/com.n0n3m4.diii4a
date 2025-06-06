/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWTOOL.H -- LightWave Mouse Tools
 */
#ifndef LWSDK_TOOL_H
#define LWSDK_TOOL_H

#include <lwtypes.h>
#include <lwxpanel.h>


/*
 * The functions in a wire draw access struct allows a tool to draw a
 * wireframe representation of itself.  The 'moveTo' function selects
 * the line type, and the drawing functions can provide coordinates that
 * are absolute or relative in model or screen space.
 */
typedef struct st_LWWireDrawAccess {
        void             *data;
        void            (*moveTo) (void *, LWFVector, int);
        void            (*lineTo) (void *, LWFVector, int);
        void            (*spline) (void *, LWFVector, LWFVector, LWFVector, int);
        void            (*circle) (void *, double, int);
        int               axis;
        void            (*text)   (void *, const char *, int);
        double            pxScale;
} LWWireDrawAccess;

#define LWWIRE_SOLID     0
#define LWWIRE_DASH      1

#define LWWIRE_ABSOLUTE  0
#define LWWIRE_RELATIVE  100
#define LWWIRE_SCREEN    101

#define LWWIRE_TEXT_L    0
#define LWWIRE_TEXT_C    1
#define LWWIRE_TEXT_R    2


/*
 * Mouse events in LightWave viewports occur in 3D space.
 *
 * posRaw       event position in 3D space.  Naturally this occurs in
 * posSnap      some view plane.  One value is snapped to the nearest
 *              grid intersection in 3D while the other is not.
 *
 * deltaRaw     vector from the initial mouse-down event to the current
 * deltaSnap    event location.  These are the difference between the
 *              current 'pos' values are the first one.
 *
 * dx, dy       screen movement in pixels.  This is the total raw mouse
 *              offset from the starting position.
 *
 * pxRaw/Snap   parametric translation values.  These are the mouse
 * pyRaw/Snap   offsets converted to values in model-space.  They provide
 *              a method for computing abstract distance measures from
 *              left/right and up/down mouse movement roughly scaled to
 *              the magnification of the viewport.
 *
 * axis         event axis.  All the points under the mouse location
 *              are along this axis through 'pos'.
 *
 * ax           screen coordinate system.  The ax vector points to the
 * ay           right on the screen, the ay vector points up the screen
 * az           and the az vector points into the screen.  Movement along
 *              each vector corresponds to approximately one pixel of
 *              screen-space movement.
 *
 * portAxis     view axis which is 0, 1 or 2 for orthogonal views.  It
 *              is -1 for perspective views.
 */
typedef struct st_LWToolEvent {
        LWDVector        posRaw,   posSnap;
        LWDVector        deltaRaw, deltaSnap;
        LWDVector        axis;
        LWDVector        ax, ay, az;
        double           pxRaw, pxSnap;
        double           pyRaw, pySnap;
        int              dx, dy;
        int              portAxis;
        int              flags;
} LWToolEvent;

/*
 * The 'flags' field can have a number of bits set to indicate the type of
 * mouse event.
 *
 * CONSTRAIN    constrained action, activated by a standard key or mouse
 *              combination.
 *
 * CONS_X       direction of constraint for orthogonal moves.  Initially
 * CONS_Y       neither bit is set, but as the user moves enough to select a
 *              primary direction, one or the other will be set.
 *
 * ALT_BUTTON   alternate mouse button event, usually the right button.
 *
 * MULTICLICK   multiclick sequence event.
 */
#define LWTOOLF_CONSTRAIN       (1<<0)
#define LWTOOLF_CONS_X          (1<<1)
#define LWTOOLF_CONS_Y          (1<<2)
#define LWTOOLF_ALT_BUTTON      (1<<3)
#define LWTOOLF_MULTICLICK      (1<<4)


/*
 * A LightWave viewport tool has the following handler functions.
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
typedef struct st_LWToolFuncs {
        void            (*done)   (LWInstance);
        void            (*draw)   (LWInstance, LWWireDrawAccess *);
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
} LWToolFuncs;

#define LWT_DIRTY_WIREFRAME     (1<<0)
#define LWT_DIRTY_HELPTEXT      (1<<1)

#define LWT_EVENT_DROP           0
#define LWT_EVENT_RESET          1
#define LWT_EVENT_ACTIVATE       2

typedef struct st_LWTool {
        LWInstance        instance;
        LWToolFuncs      *tool;
} LWTool;


#endif

