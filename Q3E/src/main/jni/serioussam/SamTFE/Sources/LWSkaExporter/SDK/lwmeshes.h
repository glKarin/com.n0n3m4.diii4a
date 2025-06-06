/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWMESHES.H -- LightWave Geometry Services
 *
 * This header contains the declarations for globals for accessing
 * common geometric information.
 */
#ifndef LWSDK_MESHES_H
#define LWSDK_MESHES_H

#include <lwtypes.h>


/*
 * Meshes are composed of points and polygons given by their internal
 * ID reference.
 */
typedef struct st_GCoreVertex *  LWPntID;
typedef struct st_GCorePolygon * LWPolID;

/*
 * Polygon types are an extensible set of ID codes.  Some common ones
 * are included here.
 */
#define LWPOLTYPE_FACE  LWID_('F','A','C','E')
#define LWPOLTYPE_CURV  LWID_('C','U','R','V')
#define LWPOLTYPE_PTCH  LWID_('P','T','C','H')
#define LWPOLTYPE_MBAL  LWID_('M','B','A','L')
#define LWPOLTYPE_BONE  LWID_('B','O','N','E')

/*
 * Polygon tags are indexed by an extensible set of ID codes.  Some
 * common ones are included here.
 */
#define LWPTAG_SURF     LWID_('S','U','R','F')
#define LWPTAG_PART     LWID_('P','A','R','T')
#define LWPTAG_TXUV     LWID_('T','X','U','V')

/*
 * VMAPs are identifed by an extensible set of ID codes.  Some common
 * ones are included here.
 */
#define LWVMAP_PICK     LWID_('P','I','C','K')
#define LWVMAP_WGHT     LWID_('W','G','H','T')
#define LWVMAP_MNVW     LWID_('M','N','V','W')
#define LWVMAP_TXUV     LWID_('T','X','U','V')
#define LWVMAP_MORF     LWID_('M','O','R','F')
#define LWVMAP_SPOT     LWID_('S','P','O','T')
#define LWVMAP_RGB      LWID_('R','G','B',' ')
#define LWVMAP_RGBA     LWID_('R','G','B','A')

/*
 * Scan callbacks allow the client to traverse the elements of the
 * mesh and get information about each one.
 */
typedef int             LWPntScanFunc (void *, LWPntID);
typedef int             LWPolScanFunc (void *, LWPolID);


/*
 * A LWMeshInfo is an allocated object that allows a mesh to be examined
 * through a set of query callbacks.
 *
 * priv         private data for the mesh info implementation.  Hands off!
 *
 * destroy      destroy the mesh when done.  A mesh should only be freed
 *              by the client that created it.
 *
 * numPoints    these functions return the size of the mesh as a count of
 * numPolygons  its points and polygons.
 *
 * scanPoints   iterate through elements in the mesh.  The scan function
 * scanPolygons callback is called for each element in the mesh.  If the
 *              callback returns non-zero, the iteration stops and the
 *              value is returned.
 *
 * pntBasePos   return the base position of a point.  This is the position
 *              of the point at rest in the basic object coordinate system.
 *
 * pntOtherPos  return an alternate position for the point.  This many be
 *              the same as the base position or it may be the position of
 *              the point after some transformation.  The nature of the
 *              alternate position depends on how the mesh info was created.
 *
 * pntVLookup   selects a vmap for reading vectors.  The vmap is given by
 *              an ID code for the type and a name string.  The function
 *              returns a pointer that may be used to select this same vmap
 *              again quickly, or null for none.
 *
 * pntVSelect   selects a vmap for reading vectors.  The vmap is given by
 *              the pointer returned from pntVLookup above.  The function
 *              returns the dimension of the vmap.
 *
 * pntVGet      reads the vector value of the selected vmap for the given
 *              point.  The vmap can be set using either of the two
 *              functions above.  The vector must at least be as large as
 *              the dimension of the vmap.  The function returns true if
 *              the point is mapped.
 *
 * pntVPGet     just like the above, but reads the vector value of the 
 *              selected vmap for the given point and polygon.
 *
 * polType      returns the type of a polygon as an LWPOLTYPE_* code.
 *
 * polSize      returns the number of vertices that the polygon has.
 *
 * polVertex    returns the point ID for the given vertex of the polygon.
 *              Vertices are indexed starting from zero.
 *
 * polTag       returns the tag string of the given type associated with the
 *              polygon.  A null string pointer means that the polygon does
 *              not have a tag of that type.
 *
 * polFlags             returns the polygon flags, matching those used by the MeshEdit 
 *              polygin info structure in modeler.

 */
typedef struct st_LWMeshInfo *  LWMeshInfoID;

typedef struct st_LWMeshInfo {
        void             *priv;
        void            (*destroy)     (LWMeshInfoID);

        int             (*numPoints)   (LWMeshInfoID);
        int             (*numPolygons) (LWMeshInfoID);
        int             (*scanPoints)  (LWMeshInfoID, LWPntScanFunc *, void *);
        int             (*scanPolys)   (LWMeshInfoID, LWPolScanFunc *, void *);

        void            (*pntBasePos)  (LWMeshInfoID, LWPntID, LWFVector pos);
        void            (*pntOtherPos) (LWMeshInfoID, LWPntID, LWFVector pos);
        void *          (*pntVLookup)  (LWMeshInfoID, LWID, const char *);
        int             (*pntVSelect)  (LWMeshInfoID, void *);
        int             (*pntVGet)     (LWMeshInfoID, LWPntID, float *vector);

        LWID            (*polType)     (LWMeshInfoID, LWPolID);
        int             (*polSize)     (LWMeshInfoID, LWPolID);
        LWPntID         (*polVertex)   (LWMeshInfoID, LWPolID, int);
        const char *    (*polTag)      (LWMeshInfoID, LWPolID, LWID);

        int             (*pntVPGet)    (LWMeshInfoID, LWPntID, LWPolID, float *vector);
        unsigned int    (*polFlags)     (LWMeshInfoID, LWPolID);

        int             (*pntVIDGet)     (LWMeshInfoID, LWPntID, float *vector, void *);
        int             (*pntVPIDGet)    (LWMeshInfoID, LWPntID, LWPolID, float *vector, void *);
} LWMeshInfo;


/*
 * LightWave maintains a database which is essentially an image of
 * all the LWO2 object files loaded into the application.  Each object
 * contains some number of layers which can be scanned.  The functions
 * for accessing this information is the "Scene Objects" global.
 *
 * numObjects   returns the total number of objects in the scene.
 *              These are the unique object files which are loaded,
 *              which may have nothing to do with the animatable
 *              items in the scene.
 *
 * filename     returns the name of the source object file.  Objects 
 *              are indexed from 0 to numObjects - 1.
 *
 * userName     returns the name of the object as seen by the user.
 *              This is typically the base filename without path or
 *              extension, or "Unnamed N" for unsaved objects.  These
 *              are not guaranteed to be unique.
 *
 * refName      returns an internal reference name for this object.
 *              The reference name is guaranteed to be unique and
 *              unchanging for the lifetime of the object.  This
 *              string contains control characters so it cannot be
 *              confused with a filename.
 *
 * maxLayers    returns a value one greater than the largest index of
 *              any existing layer.
 *
 * layerExists  returns true if the layer with the given index exists.  
 *              Layers are indexed from 0 to maxLayers - 1.
 *
 * pivotPoint   fills in the postion vector with the pivot point for 
 *              the given layer.
 *
 * layerMesh    returns a mesh info structure for the given layer.  The
 *              mesh will reference vertices in their rest positions.
 *
 * layerName    returns the name string assigned to the layer, or null
 *              if none.
 *
 * layerVis     returns true if the layer is marked as visible.
 *
 * numVMaps     returns the total number of vmaps in the scene with the 
 *              given type code.  A code of zero selects all vmaps of
 *              every type.
 *
 * vmapName     returns the name of the vmap with the given index, from 
 *              0 to numVMaps - 1.
 *
 * vmapDim      returns the dimension of the vmap vector.
 *
 * vmapType     returns the type code of the vmap.  The index is into the
 *              list of all vmaps given by numVMap with type code zero.
 */
#define LWOBJECTFUNCS_GLOBAL    "Scene Objects 4"

typedef struct st_LWObjectFuncs {
        int             (*numObjects)  (void);
        const char *    (*filename)    (int obj);
        int             (*maxLayers)   (int obj);
        int             (*layerExists) (int obj, int lnum);
        void            (*pivotPoint)  (int obj, int lnum, LWFVector pos);
        LWMeshInfo *    (*layerMesh)   (int obj, int lnum);

        int             (*numVMaps)    (LWID);
        const char *    (*vmapName)    (LWID, int index);
        int             (*vmapDim)     (LWID, int index);
        LWID            (*vmapType)    (int index);

        const char *    (*layerName)   (int obj, int lnum);
        int             (*layerVis)    (int obj, int lnum);

        const char *    (*userName)    (int obj);
        const char *    (*refName)     (int obj);
} LWObjectFuncs;


#endif

