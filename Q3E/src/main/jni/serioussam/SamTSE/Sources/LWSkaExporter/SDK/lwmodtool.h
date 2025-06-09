/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWMODTOOL.H -- Modeler Interactive Tools
 */
#ifndef LWSDK_MODTOOL_H
#define LWSDK_MODTOOL_H

#include <lwtool.h>
#include <lwmeshedt.h>



#define LWMESHEDITTOOL_CLASS    "MeshEditTool"
#define LWMESHEDITTOOL_VERSION  4

/*
 * The MeshEdit tool is a LightWave viewport tool that performs its
 * action through a MeshEditOp.  The extra functions are:
 *
 * test         return a code for the edit action that needs to be
 *              performed.  Actions given below.
 *
 * build        perform the mesh edit operation to reflect the current
 *              tool settings.
 *
 * end          clear the state when the last edit action is completed.
 *              This can be a result of the 'test' update code or it can
 *              be triggered by an external action.
 */
typedef struct st_LWMeshEditTool {
        LWInstance        instance;
        LWToolFuncs      *tool;
        int             (*test)  (LWInstance);
        LWError         (*build) (LWInstance, MeshEditOp *);
        void            (*end)   (LWInstance, int keep);
} LWMeshEditTool;

/*
 * The test function can return code for the next action to be performed
 * to modify the edit state.
 *
 * NOTHING      do nothing.  The edit state remains unchanged.
 *
 * UPDATE       reapply the operation with new settings.  The 'build' func
 *              is called.
 *
 * ACCEPT       keep the last operation.  The 'end' callback is called with
 *              the 'keep' argument true.
 *
 * REJECT       discard the last operation.  The 'end' callback is called
 *              with the 'keep' argument false.
 *
 * CLONE        keep the last operation and begin a new one.  'End' is
 *              called with a true 'keep' parameter, and then 'build' is 
 *              called again.
 */
#define LWT_TEST_NOTHING        0
#define LWT_TEST_UPDATE         1
#define LWT_TEST_ACCEPT         2
#define LWT_TEST_REJECT         3
#define LWT_TEST_CLONE          4

#endif

