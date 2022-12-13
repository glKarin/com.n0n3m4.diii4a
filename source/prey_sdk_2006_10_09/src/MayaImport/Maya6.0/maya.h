#ifdef _WIN32

#define _BOOL

#include <maya/MStatus.h>
#include <maya/MString.h> 
#include <maya/MFileIO.h>
#include <maya/MLibrary.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>
#include <maya/MMatrix.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MEulerRotation.h>
#include <maya/MObject.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MItDag.h>
#include <maya/MTime.h>
#include <maya/MAnimControl.h>
#include <maya/MFnGeometryFilter.h>
#include <maya/MFnSet.h>
#include <maya/MSelectionList.h>
#include <maya/MFloatArray.h>
#include <maya/MFnWeightGeometryFilter.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MFnMesh.h>
#include <maya/MDagPathArray.h>
#include <maya/MItGeometry.h>
#include <maya/MPlugArray.h>
#include <maya/MPlug.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnMatrixData.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MFnTransform.h>
#include <maya/MQuaternion.h>
#include <maya/MFnCamera.h>
#include <maya/MFloatMatrix.h>
#include <maya/MFnEnumAttribute.h>

#undef _BOOL

#endif // _WIN32
