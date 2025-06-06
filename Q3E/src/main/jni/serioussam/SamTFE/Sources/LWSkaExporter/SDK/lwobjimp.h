/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWOBJIMP.H -- LightWave Object Importers
 *
 * When LightWave encounters a foreign object file which it cannot parse, 
 * it will call an "ObjectLoader" class server to import it.  All the 
 * loaders defined for the host will be activated in sequence, and the 
 * first one to recognize the file will load it.
 */
#ifndef LWSDK_OBJIMP_H
#define LWSDK_OBJIMP_H

#include <lwmonitor.h>
#include <lwmeshes.h>

#define LWOBJECTIMPORT_CLASS    "ObjectLoader"
#define LWOBJECTIMPORT_VERSION  3


/*
 * The activation function of the server is passed an LWObjectImport 
 * structure as its local data which includes the filename of the object
 * to load.  The loader attempts to parse the input file and calls the
 * embedded callbacks to insert the data into LightWave.  It indicates 
 * its success or failure by setting the 'result' field, and the optional
 * failedBuf.
 *
 * result       set by the server to one of the LWOBJIM values below.
 *
 * filename     the filename of the object to load.
 *
 * monitor      progress monitor that can be used to track the import
 *              process.
 *
 * failedBuf    string buffer for the server to store a human-readable
 *              error message if the result code is LWOBJIM_FAILED.
 *
 * data         private data pointer to be passed to all the embedded
 *              function callbacks.
 *
 * done         called when object import is complete.
 *
 * layer        start a new layer in the import data.  The layer is
 *              defined by an index number and a name string.
 *
 * pivot        set the pivot point for the current layer.
 *
 * parent       set the index of the parent layer for the current layer.
 *
 * lFlags       set the flag bits for the current layer.  The low-order bit
 *              is set if this is a hidden layer.
 *
 * point        add a new point to the current layer.  The point is given
 *              by an XYZ position and the function returns a void pointer
 *              as point identifier.
 *
 * vmap         select a VMAP for assigning data to points, creating a new
 *              VMAP if it does not yet exist.  The VMAP is defined by a
 *              type, dimension and name.
 *
 * vmapVal      set a vector for a point into the currently selected VMAP.
 *              The vector should have the same dimension as the VMAP.
 *
 * vmapPDV      set a vector for a point with reference to a specific polygon.
 *              This is the "polygon discontinuous value" for the point.
 *
 * polygon      add a new polygon to the current layer.  The polygon is
 *              defined by a type code, type-specific flags and a list of
 *              point identifiers.
 *
 * polTag       associate a tag string to the given polygon.  The tag
 *              string has a type, the most common being 'SURF'.
 *
 * surface      add a new surface to this object.  The surface is defined by
 *              the base name, the reference name, and a block of raw surface
 *              data.
 */
typedef struct st_LWObjectImport {
        int               result;
        const char       *filename;
        LWMonitor        *monitor;
        char             *failedBuf;
        int               failedLen;

        void             *data;

        void            (*done)    (void *);
        void            (*layer)   (void *, int lNum, const char *name);
        void            (*pivot)   (void *, const LWFVector pivot);
        void            (*parent)  (void *, int lNum);
        void            (*lFlags)  (void *, int flags);
        LWPntID         (*point)   (void *, const LWFVector xyz);
        void            (*vmap)    (void *, LWID type, int dim, const char *name);
        void            (*vmapVal) (void *, LWPntID point, const float *val);
        LWPolID         (*polygon) (void *, LWID type, int flags, int numPts, const LWPntID *);
        void            (*polTag)  (void *, LWPolID polygon, LWID type, const char *tag);
        void            (*surface) (void *, const char *, const char *, int, void *);
        void            (*vmapPDV) (void *, LWPntID point, LWPolID polygon, const float *val);
} LWObjectImport;


/*
 * The server must set the 'result' field to one of these following values
 * before it returns.
 *
 * OK           indicates successful parsing of the object file.
 *
 * BADFILE      indicates that the loader could not open the file.
 *
 * NOREC        indicates that the loader could open the file but could
 *              not recognize the format.
 *
 * ABORTED      indicates the that the user manually aborted the load.
 *
 * Any other failure is indicated by the generic FAILED value.  In this case,
 * the loader may also place a human-readable error message into the buffer
 * pointed to by `failedBuf,' provided that `failedLen' is non-zero.
 */
#define LWOBJIM_OK       0
#define LWOBJIM_NOREC    1
#define LWOBJIM_BADFILE  2
#define LWOBJIM_ABORTED  3
#define LWOBJIM_FAILED   99


#endif

