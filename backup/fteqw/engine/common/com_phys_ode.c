/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/*
	ODE physics engine code
	This code is ported from DarkPlaces svn commit 9370
	Originally written by LordHavoc.
*/

//if we're not building as an fte-specific plugin, we must be being built as part of the fte engine itself.
//(no, we don't want to act as a plugin for ezquake...)
#ifndef FTEPLUGIN
#define FTEPLUGIN
#define Plug_Init Plug_ODE_Init
#endif

#include "../../plugins/plugin.h"
#include "../../plugins/engine.h"

#define DEG2RAD(d) (d * M_PI * (1/180.0f))
#define RAD2DEG(d) ((d*180) / M_PI)

#ifdef USERBE

#include "pr_common.h"

#ifndef FTEENGINE
#define BZ_Malloc malloc
#define BZ_Free free
#define Z_Free BZ_Free
#define VectorCompare VectorComparestatic
static int VectorCompare (const vec3_t v1, const vec3_t v2)
{
	int		i;
	for (i=0 ; i<3 ; i++)
		if (v1[i] != v2[i])
			return 0;
	return 1;
}

cvar_t *cvar_r_meshpitch;
cvar_t *cvar_r_meshroll;
#endif

static rbeplugfuncs_t *rbefuncs;

//============================================================================
// physics engine support
//============================================================================

//#ifndef ODE_STATIC
//#define ODE_DYNAMIC 1
//#endif

//ODE's headers provide version info only as a string, so we don' know when things are deprecated or not.
//this then fucks us over when we try using -Werror
//so until ODE changes its ways, we'll just have to make assumptions and ignore those warnings.
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#define ODEVERSION MAKE2VER(0,15)
#define MAKE2VER(maj,min) (((maj)<<8)|(min))

// LordHavoc: this large chunk of definitions comes from the ODE library
// include files.
#ifdef ODE_STATIC
#undef ODE_DYNAMIC
#define dDOUBLE
#include "ode/ode.h"
#else
#ifndef ODE_DYNAMIC
#define ODE_DYNAMIC
#endif
#ifdef WINAPI
// ODE does not use WINAPI
#define ODE_API VARGS /*vargs because fte likes to be compiled fastcall (vargs is defined as cdecl...)*/
#else
#define ODE_API VARGS
#endif

#define DEG2RAD(d) (d * M_PI * (1/180.0f))
#define RAD2DEG(d) ((d*180) / M_PI)

// note: dynamic builds of ODE tend to be double precision, this is not used
// for static builds
typedef double dReal;

typedef dReal dVector3[4];
typedef dReal dVector4[4];
typedef dReal dMatrix3[4*3];
typedef dReal dMatrix4[4*4];
typedef dReal dMatrix6[8*6];
typedef dReal dQuaternion[4];

struct dxWorld;		/* dynamics world */
struct dxSpace;		/* collision space */
struct dxBody;		/* rigid body (dynamics object) */
struct dxGeom;		/* geometry (collision object) */
struct dxJoint;
struct dxJointNode;
struct dxJointGroup;
struct dxTriMeshData;

#define dInfinity 3.402823466e+38f

typedef struct dxWorld *dWorldID;
typedef struct dxSpace *dSpaceID;
typedef struct dxBody *dBodyID;
typedef struct dxGeom *dGeomID;
typedef struct dxJoint *dJointID;
typedef struct dxJointGroup *dJointGroupID;
typedef struct dxTriMeshData *dTriMeshDataID;

typedef struct dJointFeedback
{
	dVector3 f1;		/* force applied to body 1 */
	dVector3 t1;		/* torque applied to body 1 */
	dVector3 f2;		/* force applied to body 2 */
	dVector3 t2;		/* torque applied to body 2 */
}
dJointFeedback;

typedef enum dJointType
{
	dJointTypeNone = 0,
	dJointTypeBall,
	dJointTypeHinge,
	dJointTypeSlider,
	dJointTypeContact,
	dJointTypeUniversal,
	dJointTypeHinge2,
	dJointTypeFixed,
	dJointTypeNull,
	dJointTypeAMotor,
	dJointTypeLMotor,
	dJointTypePlane2D,
	dJointTypePR,
	dJointTypePU,
	dJointTypePiston
}
dJointType;

#define D_ALL_PARAM_NAMES(start) \
  /* parameters for limits and motors */ \
  dParamLoStop = start, \
  dParamHiStop, \
  dParamVel, \
  dParamFMax, \
  dParamFudgeFactor, \
  dParamBounce, \
  dParamCFM, \
  dParamStopERP, \
  dParamStopCFM, \
  /* parameters for suspension */ \
  dParamSuspensionERP, \
  dParamSuspensionCFM, \
  dParamERP, \

#define D_ALL_PARAM_NAMES_X(start,x) \
  /* parameters for limits and motors */ \
  dParamLoStop ## x = start, \
  dParamHiStop ## x, \
  dParamVel ## x, \
  dParamFMax ## x, \
  dParamFudgeFactor ## x, \
  dParamBounce ## x, \
  dParamCFM ## x, \
  dParamStopERP ## x, \
  dParamStopCFM ## x, \
  /* parameters for suspension */ \
  dParamSuspensionERP ## x, \
  dParamSuspensionCFM ## x, \
  dParamERP ## x,

enum {
  D_ALL_PARAM_NAMES(0)
  D_ALL_PARAM_NAMES_X(0x100,2)
  D_ALL_PARAM_NAMES_X(0x200,3)

  /* add a multiple of this constant to the basic parameter numbers to get
   * the parameters for the second, third etc axes.
   */
  dParamGroup=0x100
};

typedef struct dMass
{
	dReal mass;
	dVector3 c;
	dMatrix3 I;
}
dMass;

enum
{
	dContactMu2			= 0x001,
	dContactFDir1		= 0x002,
	dContactBounce		= 0x004,
	dContactSoftERP		= 0x008,
	dContactSoftCFM		= 0x010,
	dContactMotion1		= 0x020,
	dContactMotion2		= 0x040,
	dContactMotionN		= 0x080,
	dContactSlip1		= 0x100,
	dContactSlip2		= 0x200,
	
	dContactApprox0		= 0x0000,
	dContactApprox1_1	= 0x1000,
	dContactApprox1_2	= 0x2000,
	dContactApprox1		= 0x3000
};

typedef struct dSurfaceParameters
{
	/* must always be defined */
	int mode;
	dReal mu;

	/* only defined if the corresponding flag is set in mode */
	dReal mu2;
	dReal bounce;
	dReal bounce_vel;
	dReal soft_erp;
	dReal soft_cfm;
	dReal motion1,motion2,motionN;
	dReal slip1,slip2;
} dSurfaceParameters;

typedef struct dContactGeom
{
	dVector3 pos;          ///< contact position
	dVector3 normal;       ///< normal vector
	dReal depth;           ///< penetration depth
	dGeomID g1,g2;         ///< the colliding geoms
	int side1,side2;       ///< (to be documented)
}
dContactGeom;

typedef struct dContact
{
	dSurfaceParameters surface;
	dContactGeom geom;
	dVector3 fdir1;
}
dContact;

typedef void VARGS dNearCallback (void *data, dGeomID o1, dGeomID o2);

// SAP
// Order XZY or ZXY usually works best, if your Y is up.
#define dSAP_AXES_XYZ  ((0)|(1<<2)|(2<<4))
#define dSAP_AXES_XZY  ((0)|(2<<2)|(1<<4))
#define dSAP_AXES_YXZ  ((1)|(0<<2)|(2<<4))
#define dSAP_AXES_YZX  ((1)|(2<<2)|(0<<4))
#define dSAP_AXES_ZXY  ((2)|(0<<2)|(1<<4))
#define dSAP_AXES_ZYX  ((2)|(1<<2)|(0<<4))

//const char*     (ODE_API *dGetConfiguration)(void);
int             (ODE_API *dCheckConfiguration)( const char* token );
int             (ODE_API *dInitODE)(void);
//int             (ODE_API *dInitODE2)(unsigned int uiInitFlags);
//int             (ODE_API *dAllocateODEDataForThread)(unsigned int uiAllocateFlags);
//void            (ODE_API *dCleanupODEAllDataForThread)(void);
void            (ODE_API *dCloseODE)(void);

//int             (ODE_API *dMassCheck)(const dMass *m);
//void            (ODE_API *dMassSetZero)(dMass *);
//void            (ODE_API *dMassSetParameters)(dMass *, dReal themass, dReal cgx, dReal cgy, dReal cgz, dReal I11, dReal I22, dReal I33, dReal I12, dReal I13, dReal I23);
//void            (ODE_API *dMassSetSphere)(dMass *, dReal density, dReal radius);
void            (ODE_API *dMassSetSphereTotal)(dMass *, dReal total_mass, dReal radius);
//void            (ODE_API *dMassSetCapsule)(dMass *, dReal density, int direction, dReal radius, dReal length);
void            (ODE_API *dMassSetCapsuleTotal)(dMass *, dReal total_mass, int direction, dReal radius, dReal length);
//void            (ODE_API *dMassSetCylinder)(dMass *, dReal density, int direction, dReal radius, dReal length);
void            (ODE_API *dMassSetCylinderTotal)(dMass *, dReal total_mass, int direction, dReal radius, dReal length);
//void            (ODE_API *dMassSetBox)(dMass *, dReal density, dReal lx, dReal ly, dReal lz);
void            (ODE_API *dMassSetBoxTotal)(dMass *, dReal total_mass, dReal lx, dReal ly, dReal lz);
//void            (ODE_API *dMassSetTrimesh)(dMass *, dReal density, dGeomID g);
//void            (ODE_API *dMassSetTrimeshTotal)(dMass *m, dReal total_mass, dGeomID g);
//void            (ODE_API *dMassAdjust)(dMass *, dReal newmass);
//void            (ODE_API *dMassTranslate)(dMass *, dReal x, dReal y, dReal z);
//void            (ODE_API *dMassRotate)(dMass *, const dMatrix3 R);
//void            (ODE_API *dMassAdd)(dMass *a, const dMass *b);
//
dWorldID        (ODE_API *dWorldCreate)(void);
void            (ODE_API *dWorldDestroy)(dWorldID world);
void            (ODE_API *dWorldSetGravity)(dWorldID, dReal x, dReal y, dReal z);
void            (ODE_API *dWorldGetGravity)(dWorldID, dVector3 gravity);
void            (ODE_API *dWorldSetERP)(dWorldID, dReal erp);
//dReal           (ODE_API *dWorldGetERP)(dWorldID);
void            (ODE_API *dWorldSetCFM)(dWorldID, dReal cfm);
//dReal           (ODE_API *dWorldGetCFM)(dWorldID);
void            (ODE_API *dWorldStep)(dWorldID, dReal stepsize);
//void            (ODE_API *dWorldImpulseToForce)(dWorldID, dReal stepsize, dReal ix, dReal iy, dReal iz, dVector3 force);
void            (ODE_API *dWorldQuickStep)(dWorldID w, dReal stepsize);
void            (ODE_API *dWorldSetQuickStepNumIterations)(dWorldID, int num);
//int             (ODE_API *dWorldGetQuickStepNumIterations)(dWorldID);
//void            (ODE_API *dWorldSetQuickStepW)(dWorldID, dReal over_relaxation);
//dReal           (ODE_API *dWorldGetQuickStepW)(dWorldID);
//void            (ODE_API *dWorldSetContactMaxCorrectingVel)(dWorldID, dReal vel);
//dReal           (ODE_API *dWorldGetContactMaxCorrectingVel)(dWorldID);
void            (ODE_API *dWorldSetContactSurfaceLayer)(dWorldID, dReal depth);
//dReal           (ODE_API *dWorldGetContactSurfaceLayer)(dWorldID);
//void            (ODE_API *dWorldStepFast1)(dWorldID, dReal stepsize, int maxiterations);
//void            (ODE_API *dWorldSetAutoEnableDepthSF1)(dWorldID, int autoEnableDepth);
//int             (ODE_API *dWorldGetAutoEnableDepthSF1)(dWorldID);
//dReal           (ODE_API *dWorldGetAutoDisableLinearThreshold)(dWorldID);
void            (ODE_API *dWorldSetAutoDisableLinearThreshold)(dWorldID, dReal linear_threshold);
//dReal           (ODE_API *dWorldGetAutoDisableAngularThreshold)(dWorldID);
void            (ODE_API *dWorldSetAutoDisableAngularThreshold)(dWorldID, dReal angular_threshold);
//dReal           (ODE_API *dWorldGetAutoDisableLinearAverageThreshold)(dWorldID);
//void            (ODE_API *dWorldSetAutoDisableLinearAverageThreshold)(dWorldID, dReal linear_average_threshold);
//dReal           (ODE_API *dWorldGetAutoDisableAngularAverageThreshold)(dWorldID);
//void            (ODE_API *dWorldSetAutoDisableAngularAverageThreshold)(dWorldID, dReal angular_average_threshold);
//int             (ODE_API *dWorldGetAutoDisableAverageSamplesCount)(dWorldID);
void            (ODE_API *dWorldSetAutoDisableAverageSamplesCount)(dWorldID, unsigned int average_samples_count );
//int             (ODE_API *dWorldGetAutoDisableSteps)(dWorldID);
void            (ODE_API *dWorldSetAutoDisableSteps)(dWorldID, int steps);
//dReal           (ODE_API *dWorldGetAutoDisableTime)(dWorldID);
void            (ODE_API *dWorldSetAutoDisableTime)(dWorldID, dReal time);
//int             (ODE_API *dWorldGetAutoDisableFlag)(dWorldID);
void            (ODE_API *dWorldSetAutoDisableFlag)(dWorldID, int do_auto_disable);
//dReal           (ODE_API *dWorldGetLinearDampingThreshold)(dWorldID w);
void            (ODE_API *dWorldSetLinearDampingThreshold)(dWorldID w, dReal threshold);
//dReal           (ODE_API *dWorldGetAngularDampingThreshold)(dWorldID w);
void            (ODE_API *dWorldSetAngularDampingThreshold)(dWorldID w, dReal threshold);
//dReal           (ODE_API *dWorldGetLinearDamping)(dWorldID w);
void            (ODE_API *dWorldSetLinearDamping)(dWorldID w, dReal scale);
//dReal           (ODE_API *dWorldGetAngularDamping)(dWorldID w);
void            (ODE_API *dWorldSetAngularDamping)(dWorldID w, dReal scale);
//void            (ODE_API *dWorldSetDamping)(dWorldID w, dReal linear_scale, dReal angular_scale);
//dReal           (ODE_API *dWorldGetMaxAngularSpeed)(dWorldID w);
//void            (ODE_API *dWorldSetMaxAngularSpeed)(dWorldID w, dReal max_speed);
//dReal           (ODE_API *dBodyGetAutoDisableLinearThreshold)(dBodyID);
//void            (ODE_API *dBodySetAutoDisableLinearThreshold)(dBodyID, dReal linear_average_threshold);
//dReal           (ODE_API *dBodyGetAutoDisableAngularThreshold)(dBodyID);
//void            (ODE_API *dBodySetAutoDisableAngularThreshold)(dBodyID, dReal angular_average_threshold);
//int             (ODE_API *dBodyGetAutoDisableAverageSamplesCount)(dBodyID);
//void            (ODE_API *dBodySetAutoDisableAverageSamplesCount)(dBodyID, unsigned int average_samples_count);
//int             (ODE_API *dBodyGetAutoDisableSteps)(dBodyID);
//void            (ODE_API *dBodySetAutoDisableSteps)(dBodyID, int steps);
//dReal           (ODE_API *dBodyGetAutoDisableTime)(dBodyID);
//void            (ODE_API *dBodySetAutoDisableTime)(dBodyID, dReal time);
//int             (ODE_API *dBodyGetAutoDisableFlag)(dBodyID);
//void            (ODE_API *dBodySetAutoDisableFlag)(dBodyID, int do_auto_disable);
//void            (ODE_API *dBodySetAutoDisableDefaults)(dBodyID);
//dWorldID        (ODE_API *dBodyGetWorld)(dBodyID);
dBodyID         (ODE_API *dBodyCreate)(dWorldID);
void            (ODE_API *dBodyDestroy)(dBodyID);
void            (ODE_API *dBodySetData)(dBodyID, void *data);
void *          (ODE_API *dBodyGetData)(dBodyID);
void            (ODE_API *dBodySetPosition)(dBodyID, dReal x, dReal y, dReal z);
void            (ODE_API *dBodySetRotation)(dBodyID, const dMatrix3 R);
//void            (ODE_API *dBodySetQuaternion)(dBodyID, const dQuaternion q);
void            (ODE_API *dBodySetLinearVel)(dBodyID, dReal x, dReal y, dReal z);
void            (ODE_API *dBodySetAngularVel)(dBodyID, dReal x, dReal y, dReal z);
const dReal *   (ODE_API *dBodyGetPosition)(dBodyID);
//void            (ODE_API *dBodyCopyPosition)(dBodyID body, dVector3 pos);
const dReal *   (ODE_API *dBodyGetRotation)(dBodyID);
//void            (ODE_API *dBodyCopyRotation)(dBodyID, dMatrix3 R);
//const dReal *   (ODE_API *dBodyGetQuaternion)(dBodyID);
//void            (ODE_API *dBodyCopyQuaternion)(dBodyID body, dQuaternion quat);
const dReal *   (ODE_API *dBodyGetLinearVel)(dBodyID);
const dReal *   (ODE_API *dBodyGetAngularVel)(dBodyID);
void            (ODE_API *dBodySetMass)(dBodyID, const dMass *mass);
//void            (ODE_API *dBodyGetMass)(dBodyID, dMass *mass);
//void            (ODE_API *dBodyAddForce)(dBodyID, dReal fx, dReal fy, dReal fz);
void            (ODE_API *dBodyAddTorque)(dBodyID, dReal fx, dReal fy, dReal fz);
//void            (ODE_API *dBodyAddRelForce)(dBodyID, dReal fx, dReal fy, dReal fz);
//void            (ODE_API *dBodyAddRelTorque)(dBodyID, dReal fx, dReal fy, dReal fz);
void            (ODE_API *dBodyAddForceAtPos)(dBodyID, dReal fx, dReal fy, dReal fz, dReal px, dReal py, dReal pz);
//void            (ODE_API *dBodyAddForceAtRelPos)(dBodyID, dReal fx, dReal fy, dReal fz, dReal px, dReal py, dReal pz);
//void            (ODE_API *dBodyAddRelForceAtPos)(dBodyID, dReal fx, dReal fy, dReal fz, dReal px, dReal py, dReal pz);
//void            (ODE_API *dBodyAddRelForceAtRelPos)(dBodyID, dReal fx, dReal fy, dReal fz, dReal px, dReal py, dReal pz);
//const dReal *   (ODE_API *dBodyGetForce)(dBodyID);
//const dReal *   (ODE_API *dBodyGetTorque)(dBodyID);
//void            (ODE_API *dBodySetForce)(dBodyID b, dReal x, dReal y, dReal z);
//void            (ODE_API *dBodySetTorque)(dBodyID b, dReal x, dReal y, dReal z);
//void            (ODE_API *dBodyGetRelPointPos)(dBodyID, dReal px, dReal py, dReal pz, dVector3 result);
//void            (ODE_API *dBodyGetRelPointVel)(dBodyID, dReal px, dReal py, dReal pz, dVector3 result);
//void            (ODE_API *dBodyGetPointVel)(dBodyID, dReal px, dReal py, dReal pz, dVector3 result);
//void            (ODE_API *dBodyGetPosRelPoint)(dBodyID, dReal px, dReal py, dReal pz, dVector3 result);
//void            (ODE_API *dBodyVectorToWorld)(dBodyID, dReal px, dReal py, dReal pz, dVector3 result);
//void            (ODE_API *dBodyVectorFromWorld)(dBodyID, dReal px, dReal py, dReal pz, dVector3 result);
//void            (ODE_API *dBodySetFiniteRotationMode)(dBodyID, int mode);
//void            (ODE_API *dBodySetFiniteRotationAxis)(dBodyID, dReal x, dReal y, dReal z);
//int             (ODE_API *dBodyGetFiniteRotationMode)(dBodyID);
//void            (ODE_API *dBodyGetFiniteRotationAxis)(dBodyID, dVector3 result);
int             (ODE_API *dBodyGetNumJoints)(dBodyID b);
dJointID        (ODE_API *dBodyGetJoint)(dBodyID, int index);
//void            (ODE_API *dBodySetDynamic)(dBodyID);
//void            (ODE_API *dBodySetKinematic)(dBodyID);
//int             (ODE_API *dBodyIsKinematic)(dBodyID);
void            (ODE_API *dBodyEnable)(dBodyID);
void            (ODE_API *dBodyDisable)(dBodyID);
//int             (ODE_API *dBodyIsEnabled)(dBodyID);
void            (ODE_API *dBodySetGravityMode)(dBodyID b, int mode);
int             (ODE_API *dBodyGetGravityMode)(dBodyID b);
//void            (*dBodySetMovedCallback)(dBodyID b, void(ODE_API *callback)(dBodyID));
//dGeomID         (ODE_API *dBodyGetFirstGeom)(dBodyID b);
//dGeomID         (ODE_API *dBodyGetNextGeom)(dGeomID g);
//void            (ODE_API *dBodySetDampingDefaults)(dBodyID b);
//dReal           (ODE_API *dBodyGetLinearDamping)(dBodyID b);
void            (ODE_API *dBodySetLinearDamping)(dBodyID b, dReal scale);
//dReal           (ODE_API *dBodyGetAngularDamping)(dBodyID b);
void            (ODE_API *dBodySetAngularDamping)(dBodyID b, dReal scale);
//void            (ODE_API *dBodySetDamping)(dBodyID b, dReal linear_scale, dReal angular_scale);
//dReal           (ODE_API *dBodyGetLinearDampingThreshold)(dBodyID b);
//void            (ODE_API *dBodySetLinearDampingThreshold)(dBodyID b, dReal threshold);
//dReal           (ODE_API *dBodyGetAngularDampingThreshold)(dBodyID b);
//void            (ODE_API *dBodySetAngularDampingThreshold)(dBodyID b, dReal threshold);
//dReal           (ODE_API *dBodyGetMaxAngularSpeed)(dBodyID b);
void            (ODE_API *dBodySetMaxAngularSpeed)(dBodyID b, dReal max_speed);
//int             (ODE_API *dBodyGetGyroscopicMode)(dBodyID b);
//void            (ODE_API *dBodySetGyroscopicMode)(dBodyID b, int enabled);
dJointID        (ODE_API *dJointCreateBall)(dWorldID, dJointGroupID);
dJointID        (ODE_API *dJointCreateHinge)(dWorldID, dJointGroupID);
dJointID        (ODE_API *dJointCreateSlider)(dWorldID, dJointGroupID);
dJointID        (ODE_API *dJointCreateContact)(dWorldID, dJointGroupID, const dContact *);
dJointID        (ODE_API *dJointCreateHinge2)(dWorldID, dJointGroupID);
dJointID        (ODE_API *dJointCreateUniversal)(dWorldID, dJointGroupID);
//dJointID        (ODE_API *dJointCreatePR)(dWorldID, dJointGroupID);
//dJointID        (ODE_API *dJointCreatePU)(dWorldID, dJointGroupID);
//dJointID        (ODE_API *dJointCreatePiston)(dWorldID, dJointGroupID);
dJointID        (ODE_API *dJointCreateFixed)(dWorldID, dJointGroupID);
//dJointID        (ODE_API *dJointCreateNull)(dWorldID, dJointGroupID);
//dJointID        (ODE_API *dJointCreateAMotor)(dWorldID, dJointGroupID);
//dJointID        (ODE_API *dJointCreateLMotor)(dWorldID, dJointGroupID);
//dJointID        (ODE_API *dJointCreatePlane2D)(dWorldID, dJointGroupID);
void            (ODE_API *dJointDestroy)(dJointID);
dJointGroupID   (ODE_API *dJointGroupCreate)(int max_size);
void            (ODE_API *dJointGroupDestroy)(dJointGroupID);
void            (ODE_API *dJointGroupEmpty)(dJointGroupID);
//int             (ODE_API *dJointGetNumBodies)(dJointID);
void            (ODE_API *dJointAttach)(dJointID, dBodyID body1, dBodyID body2);
void            (ODE_API *dJointEnable)(dJointID);
void            (ODE_API *dJointDisable)(dJointID);
//int             (ODE_API *dJointIsEnabled)(dJointID);
void            (ODE_API *dJointSetData)(dJointID, void *data);
void *          (ODE_API *dJointGetData)(dJointID);
//dJointType      (ODE_API *dJointGetType)(dJointID);
dBodyID         (ODE_API *dJointGetBody)(dJointID, int index);
//void            (ODE_API *dJointSetFeedback)(dJointID, dJointFeedback *);
//dJointFeedback *(ODE_API *dJointGetFeedback)(dJointID);
void            (ODE_API *dJointSetBallAnchor)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetBallAnchor2)(dJointID, dReal x, dReal y, dReal z);
void            (ODE_API *dJointSetBallParam)(dJointID, int parameter, dReal value);
void            (ODE_API *dJointSetHingeAnchor)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetHingeAnchorDelta)(dJointID, dReal x, dReal y, dReal z, dReal ax, dReal ay, dReal az);
void            (ODE_API *dJointSetHingeAxis)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetHingeAxisOffset)(dJointID j, dReal x, dReal y, dReal z, dReal angle);
void            (ODE_API *dJointSetHingeParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointAddHingeTorque)(dJointID joint, dReal torque);
void            (ODE_API *dJointSetSliderAxis)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetSliderAxisDelta)(dJointID, dReal x, dReal y, dReal z, dReal ax, dReal ay, dReal az);
void            (ODE_API *dJointSetSliderParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointAddSliderForce)(dJointID joint, dReal force);
void            (ODE_API *dJointSetHinge2Anchor)(dJointID, dReal x, dReal y, dReal z);
void            (ODE_API *dJointSetHinge2Axis1)(dJointID, dReal x, dReal y, dReal z);
void            (ODE_API *dJointSetHinge2Axis2)(dJointID, dReal x, dReal y, dReal z);
void            (ODE_API *dJointSetHinge2Param)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointAddHinge2Torques)(dJointID joint, dReal torque1, dReal torque2);
void            (ODE_API *dJointSetUniversalAnchor)(dJointID, dReal x, dReal y, dReal z);
void            (ODE_API *dJointSetUniversalAxis1)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetUniversalAxis1Offset)(dJointID, dReal x, dReal y, dReal z, dReal offset1, dReal offset2);
void            (ODE_API *dJointSetUniversalAxis2)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetUniversalAxis2Offset)(dJointID, dReal x, dReal y, dReal z, dReal offset1, dReal offset2);
void            (ODE_API *dJointSetUniversalParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointAddUniversalTorques)(dJointID joint, dReal torque1, dReal torque2);
//void            (ODE_API *dJointSetPRAnchor)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPRAxis1)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPRAxis2)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPRParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointAddPRTorque)(dJointID j, dReal torque);
//void            (ODE_API *dJointSetPUAnchor)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPUAnchorOffset)(dJointID, dReal x, dReal y, dReal z, dReal dx, dReal dy, dReal dz);
//void            (ODE_API *dJointSetPUAxis1)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPUAxis2)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPUAxis3)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPUAxisP)(dJointID id, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPUParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointAddPUTorque)(dJointID j, dReal torque);
//void            (ODE_API *dJointSetPistonAnchor)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPistonAnchorOffset)(dJointID j, dReal x, dReal y, dReal z, dReal dx, dReal dy, dReal dz);
//void            (ODE_API *dJointSetPistonParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointAddPistonForce)(dJointID joint, dReal force);
void            (ODE_API *dJointSetFixed)(dJointID);
//void            (ODE_API *dJointSetFixedParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointSetAMotorNumAxes)(dJointID, int num);
//void            (ODE_API *dJointSetAMotorAxis)(dJointID, int anum, int rel, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetAMotorAngle)(dJointID, int anum, dReal angle);
//void            (ODE_API *dJointSetAMotorParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointSetAMotorMode)(dJointID, int mode);
//void            (ODE_API *dJointAddAMotorTorques)(dJointID, dReal torque1, dReal torque2, dReal torque3);
//void            (ODE_API *dJointSetLMotorNumAxes)(dJointID, int num);
//void            (ODE_API *dJointSetLMotorAxis)(dJointID, int anum, int rel, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetLMotorParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointSetPlane2DXParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointSetPlane2DYParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointSetPlane2DAngleParam)(dJointID, int parameter, dReal value);
void            (ODE_API *dJointGetBallAnchor)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetBallAnchor2)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetBallParam)(dJointID, int parameter);
void            (ODE_API *dJointGetHingeAnchor)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetHingeAnchor2)(dJointID, dVector3 result);
void            (ODE_API *dJointGetHingeAxis)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetHingeParam)(dJointID, int parameter);
//dReal           (ODE_API *dJointGetHingeAngle)(dJointID);
//dReal           (ODE_API *dJointGetHingeAngleRate)(dJointID);
//dReal           (ODE_API *dJointGetSliderPosition)(dJointID);
//dReal           (ODE_API *dJointGetSliderPositionRate)(dJointID);
void            (ODE_API *dJointGetSliderAxis)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetSliderParam)(dJointID, int parameter);
void            (ODE_API *dJointGetHinge2Anchor)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetHinge2Anchor2)(dJointID, dVector3 result);
void            (ODE_API *dJointGetHinge2Axis1)(dJointID, dVector3 result);
void            (ODE_API *dJointGetHinge2Axis2)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetHinge2Param)(dJointID, int parameter);
//dReal           (ODE_API *dJointGetHinge2Angle1)(dJointID);
//dReal           (ODE_API *dJointGetHinge2Angle1Rate)(dJointID);
//dReal           (ODE_API *dJointGetHinge2Angle2Rate)(dJointID);
void            (ODE_API *dJointGetUniversalAnchor)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetUniversalAnchor2)(dJointID, dVector3 result);
void            (ODE_API *dJointGetUniversalAxis1)(dJointID, dVector3 result);
void            (ODE_API *dJointGetUniversalAxis2)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetUniversalParam)(dJointID, int parameter);
//void            (ODE_API *dJointGetUniversalAngles)(dJointID, dReal *angle1, dReal *angle2);
//dReal           (ODE_API *dJointGetUniversalAngle1)(dJointID);
//dReal           (ODE_API *dJointGetUniversalAngle2)(dJointID);
//dReal           (ODE_API *dJointGetUniversalAngle1Rate)(dJointID);
//dReal           (ODE_API *dJointGetUniversalAngle2Rate)(dJointID);
//void            (ODE_API *dJointGetPRAnchor)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetPRPosition)(dJointID);
//dReal           (ODE_API *dJointGetPRPositionRate)(dJointID);
//dReal           (ODE_API *dJointGetPRAngle)(dJointID);
//dReal           (ODE_API *dJointGetPRAngleRate)(dJointID);
//void            (ODE_API *dJointGetPRAxis1)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetPRAxis2)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetPRParam)(dJointID, int parameter);
//void            (ODE_API *dJointGetPUAnchor)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetPUPosition)(dJointID);
//dReal           (ODE_API *dJointGetPUPositionRate)(dJointID);
//void            (ODE_API *dJointGetPUAxis1)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetPUAxis2)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetPUAxis3)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetPUAxisP)(dJointID id, dVector3 result);
//void            (ODE_API *dJointGetPUAngles)(dJointID, dReal *angle1, dReal *angle2);
//dReal           (ODE_API *dJointGetPUAngle1)(dJointID);
//dReal           (ODE_API *dJointGetPUAngle1Rate)(dJointID);
//dReal           (ODE_API *dJointGetPUAngle2)(dJointID);
//dReal           (ODE_API *dJointGetPUAngle2Rate)(dJointID);
//dReal           (ODE_API *dJointGetPUParam)(dJointID, int parameter);
//dReal           (ODE_API *dJointGetPistonPosition)(dJointID);
//dReal           (ODE_API *dJointGetPistonPositionRate)(dJointID);
//dReal           (ODE_API *dJointGetPistonAngle)(dJointID);
//dReal           (ODE_API *dJointGetPistonAngleRate)(dJointID);
//void            (ODE_API *dJointGetPistonAnchor)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetPistonAnchor2)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetPistonAxis)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetPistonParam)(dJointID, int parameter);
//int             (ODE_API *dJointGetAMotorNumAxes)(dJointID);
//void            (ODE_API *dJointGetAMotorAxis)(dJointID, int anum, dVector3 result);
//int             (ODE_API *dJointGetAMotorAxisRel)(dJointID, int anum);
//dReal           (ODE_API *dJointGetAMotorAngle)(dJointID, int anum);
//dReal           (ODE_API *dJointGetAMotorAngleRate)(dJointID, int anum);
//dReal           (ODE_API *dJointGetAMotorParam)(dJointID, int parameter);
//int             (ODE_API *dJointGetAMotorMode)(dJointID);
//int             (ODE_API *dJointGetLMotorNumAxes)(dJointID);
//void            (ODE_API *dJointGetLMotorAxis)(dJointID, int anum, dVector3 result);
//dReal           (ODE_API *dJointGetLMotorParam)(dJointID, int parameter);
//dReal           (ODE_API *dJointGetFixedParam)(dJointID, int parameter);
//dJointID        (ODE_API *dConnectingJoint)(dBodyID, dBodyID);
//int             (ODE_API *dConnectingJointList)(dBodyID, dBodyID, dJointID*);
int             (ODE_API *dAreConnected)(dBodyID, dBodyID);
int             (ODE_API *dAreConnectedExcluding)(dBodyID body1, dBodyID body2, int joint_type);
//
dSpaceID        (ODE_API *dSimpleSpaceCreate)(dSpaceID space);
dSpaceID        (ODE_API *dHashSpaceCreate)(dSpaceID space);
dSpaceID        (ODE_API *dQuadTreeSpaceCreate)(dSpaceID space, const dVector3 Center, const dVector3 Extents, int Depth);
//dSpaceID        (ODE_API *dSweepAndPruneSpaceCreate)( dSpaceID space, int axisorder );
void            (ODE_API *dSpaceDestroy)(dSpaceID);
//void            (ODE_API *dHashSpaceSetLevels)(dSpaceID space, int minlevel, int maxlevel);
//void            (ODE_API *dHashSpaceGetLevels)(dSpaceID space, int *minlevel, int *maxlevel);
//void            (ODE_API *dSpaceSetCleanup)(dSpaceID space, int mode);
//int             (ODE_API *dSpaceGetCleanup)(dSpaceID space);
//void            (ODE_API *dSpaceSetSublevel)(dSpaceID space, int sublevel);
//int             (ODE_API *dSpaceGetSublevel)(dSpaceID space);
//void            (ODE_API *dSpaceSetManualCleanup)(dSpaceID space, int mode);
//int             (ODE_API *dSpaceGetManualCleanup)(dSpaceID space);
//void            (ODE_API *dSpaceAdd)(dSpaceID, dGeomID);
//void            (ODE_API *dSpaceRemove)(dSpaceID, dGeomID);
//int             (ODE_API *dSpaceQuery)(dSpaceID, dGeomID);
//void            (ODE_API *dSpaceClean)(dSpaceID);
//int             (ODE_API *dSpaceGetNumGeoms)(dSpaceID);
//dGeomID         (ODE_API *dSpaceGetGeom)(dSpaceID, int i);
//int             (ODE_API *dSpaceGetClass)(dSpaceID space);
//
void            (ODE_API *dGeomDestroy)(dGeomID geom);
void            (ODE_API *dGeomSetData)(dGeomID geom, void* data);
void *          (ODE_API *dGeomGetData)(dGeomID geom);
void            (ODE_API *dGeomSetBody)(dGeomID geom, dBodyID body);
dBodyID         (ODE_API *dGeomGetBody)(dGeomID geom);
void            (ODE_API *dGeomSetPosition)(dGeomID geom, dReal x, dReal y, dReal z);
void            (ODE_API *dGeomSetRotation)(dGeomID geom, const dMatrix3 R);
//void            (ODE_API *dGeomSetQuaternion)(dGeomID geom, const dQuaternion Q);
//const dReal *   (ODE_API *dGeomGetPosition)(dGeomID geom);
//void            (ODE_API *dGeomCopyPosition)(dGeomID geom, dVector3 pos);
//const dReal *   (ODE_API *dGeomGetRotation)(dGeomID geom);
//void            (ODE_API *dGeomCopyRotation)(dGeomID geom, dMatrix3 R);
//void            (ODE_API *dGeomGetQuaternion)(dGeomID geom, dQuaternion result);
//void            (ODE_API *dGeomGetAABB)(dGeomID geom, dReal aabb[6]);
int             (ODE_API *dGeomIsSpace)(dGeomID geom);
//dSpaceID        (ODE_API *dGeomGetSpace)(dGeomID);
//int             (ODE_API *dGeomGetClass)(dGeomID geom);
//void            (ODE_API *dGeomSetCategoryBits)(dGeomID geom, unsigned long bits);
//void            (ODE_API *dGeomSetCollideBits)(dGeomID geom, unsigned long bits);
//unsigned long   (ODE_API *dGeomGetCategoryBits)(dGeomID);
//unsigned long   (ODE_API *dGeomGetCollideBits)(dGeomID);
//void            (ODE_API *dGeomEnable)(dGeomID geom);
//void            (ODE_API *dGeomDisable)(dGeomID geom);
//int             (ODE_API *dGeomIsEnabled)(dGeomID geom);
//void            (ODE_API *dGeomSetOffsetPosition)(dGeomID geom, dReal x, dReal y, dReal z);
//void            (ODE_API *dGeomSetOffsetRotation)(dGeomID geom, const dMatrix3 R);
//void            (ODE_API *dGeomSetOffsetQuaternion)(dGeomID geom, const dQuaternion Q);
//void            (ODE_API *dGeomSetOffsetWorldPosition)(dGeomID geom, dReal x, dReal y, dReal z);
//void            (ODE_API *dGeomSetOffsetWorldRotation)(dGeomID geom, const dMatrix3 R);
//void            (ODE_API *dGeomSetOffsetWorldQuaternion)(dGeomID geom, const dQuaternion);
//void            (ODE_API *dGeomClearOffset)(dGeomID geom);
//int             (ODE_API *dGeomIsOffset)(dGeomID geom);
//const dReal *   (ODE_API *dGeomGetOffsetPosition)(dGeomID geom);
//void            (ODE_API *dGeomCopyOffsetPosition)(dGeomID geom, dVector3 pos);
//const dReal *   (ODE_API *dGeomGetOffsetRotation)(dGeomID geom);
//void            (ODE_API *dGeomCopyOffsetRotation)(dGeomID geom, dMatrix3 R);
//void            (ODE_API *dGeomGetOffsetQuaternion)(dGeomID geom, dQuaternion result);
int             (ODE_API *dCollide)(dGeomID o1, dGeomID o2, int flags, dContactGeom *contact, int skip);
//
void            (ODE_API *dSpaceCollide)(dSpaceID space, void *data, dNearCallback *callback);
void            (ODE_API *dSpaceCollide2)(dGeomID space1, dGeomID space2, void *data, dNearCallback *callback);
//
dGeomID         (ODE_API *dCreateSphere)(dSpaceID space, dReal radius);
//void            (ODE_API *dGeomSphereSetRadius)(dGeomID sphere, dReal radius);
//dReal           (ODE_API *dGeomSphereGetRadius)(dGeomID sphere);
//dReal           (ODE_API *dGeomSpherePointDepth)(dGeomID sphere, dReal x, dReal y, dReal z);
//
//dGeomID         (ODE_API *dCreateConvex)(dSpaceID space, dReal *_planes, unsigned int _planecount, dReal *_points, unsigned int _pointcount,unsigned int *_polygons);
//void            (ODE_API *dGeomSetConvex)(dGeomID g, dReal *_planes, unsigned int _count, dReal *_points, unsigned int _pointcount,unsigned int *_polygons);
//
dGeomID         (ODE_API *dCreateBox)(dSpaceID space, dReal lx, dReal ly, dReal lz);
//void            (ODE_API *dGeomBoxSetLengths)(dGeomID box, dReal lx, dReal ly, dReal lz);
//void            (ODE_API *dGeomBoxGetLengths)(dGeomID box, dVector3 result);
//dReal           (ODE_API *dGeomBoxPointDepth)(dGeomID box, dReal x, dReal y, dReal z);
//dReal           (ODE_API *dGeomBoxPointDepth)(dGeomID box, dReal x, dReal y, dReal z);
//
//dGeomID         (ODE_API *dCreatePlane)(dSpaceID space, dReal a, dReal b, dReal c, dReal d);
//void            (ODE_API *dGeomPlaneSetParams)(dGeomID plane, dReal a, dReal b, dReal c, dReal d);
//void            (ODE_API *dGeomPlaneGetParams)(dGeomID plane, dVector4 result);
//dReal           (ODE_API *dGeomPlanePointDepth)(dGeomID plane, dReal x, dReal y, dReal z);
//
dGeomID         (ODE_API *dCreateCapsule)(dSpaceID space, dReal radius, dReal length);
//void            (ODE_API *dGeomCapsuleSetParams)(dGeomID ccylinder, dReal radius, dReal length);
//void            (ODE_API *dGeomCapsuleGetParams)(dGeomID ccylinder, dReal *radius, dReal *length);
//dReal           (ODE_API *dGeomCapsulePointDepth)(dGeomID ccylinder, dReal x, dReal y, dReal z);
//
dGeomID         (ODE_API *dCreateCylinder)(dSpaceID space, dReal radius, dReal length);
//void            (ODE_API *dGeomCylinderSetParams)(dGeomID cylinder, dReal radius, dReal length);
//void            (ODE_API *dGeomCylinderGetParams)(dGeomID cylinder, dReal *radius, dReal *length);
//
//dGeomID         (ODE_API *dCreateRay)(dSpaceID space, dReal length);
//void            (ODE_API *dGeomRaySetLength)(dGeomID ray, dReal length);
//dReal           (ODE_API *dGeomRayGetLength)(dGeomID ray);
//void            (ODE_API *dGeomRaySet)(dGeomID ray, dReal px, dReal py, dReal pz, dReal dx, dReal dy, dReal dz);
//void            (ODE_API *dGeomRayGet)(dGeomID ray, dVector3 start, dVector3 dir);
//
dGeomID         (ODE_API *dCreateGeomTransform)(dSpaceID space);
void            (ODE_API *dGeomTransformSetGeom)(dGeomID g, dGeomID obj);
//dGeomID         (ODE_API *dGeomTransformGetGeom)(dGeomID g);
void            (ODE_API *dGeomTransformSetCleanup)(dGeomID g, int mode);
//int             (ODE_API *dGeomTransformGetCleanup)(dGeomID g);
//void            (ODE_API *dGeomTransformSetInfo)(dGeomID g, int mode);
//int             (ODE_API *dGeomTransformGetInfo)(dGeomID g);

enum { TRIMESH_FACE_NORMALS };
typedef int dTriCallback(dGeomID TriMesh, dGeomID RefObject, int TriangleIndex);
typedef void dTriArrayCallback(dGeomID TriMesh, dGeomID RefObject, const int* TriIndices, int TriCount);
typedef int dTriRayCallback(dGeomID TriMesh, dGeomID Ray, int TriangleIndex, dReal u, dReal v);
typedef int dTriTriMergeCallback(dGeomID TriMesh, int FirstTriangleIndex, int SecondTriangleIndex);

dTriMeshDataID  (ODE_API *dGeomTriMeshDataCreate)(void);
void            (ODE_API *dGeomTriMeshDataDestroy)(dTriMeshDataID g);
//void            (ODE_API *dGeomTriMeshDataSet)(dTriMeshDataID g, int data_id, void* in_data);
//void*           (ODE_API *dGeomTriMeshDataGet)(dTriMeshDataID g, int data_id);
//void            (*dGeomTriMeshSetLastTransform)( (ODE_API *dGeomID g, dMatrix4 last_trans );
//dReal*          (*dGeomTriMeshGetLastTransform)( (ODE_API *dGeomID g );
void            (ODE_API *dGeomTriMeshDataBuildSingle)(dTriMeshDataID g, const void* Vertices, int VertexStride, int VertexCount,  const void* Indices, int IndexCount, int TriStride);
//void            (ODE_API *dGeomTriMeshDataBuildSingle1)(dTriMeshDataID g, const void* Vertices, int VertexStride, int VertexCount,  const void* Indices, int IndexCount, int TriStride, const void* Normals);
//void            (ODE_API *dGeomTriMeshDataBuildDouble)(dTriMeshDataID g,  const void* Vertices,  int VertexStride, int VertexCount,  const void* Indices, int IndexCount, int TriStride);
//void            (ODE_API *dGeomTriMeshDataBuildDouble1)(dTriMeshDataID g,  const void* Vertices,  int VertexStride, int VertexCount,  const void* Indices, int IndexCount, int TriStride, const void* Normals);
//void            (ODE_API *dGeomTriMeshDataBuildSimple)(dTriMeshDataID g, const dReal* Vertices, int VertexCount, const dTriIndex* Indices, int IndexCount);
//void            (ODE_API *dGeomTriMeshDataBuildSimple1)(dTriMeshDataID g, const dReal* Vertices, int VertexCount, const dTriIndex* Indices, int IndexCount, const int* Normals);
//void            (ODE_API *dGeomTriMeshDataPreprocess)(dTriMeshDataID g);
//void            (ODE_API *dGeomTriMeshDataGetBuffer)(dTriMeshDataID g, unsigned char** buf, int* bufLen);
//void            (ODE_API *dGeomTriMeshDataSetBuffer)(dTriMeshDataID g, unsigned char* buf);
//void            (ODE_API *dGeomTriMeshSetCallback)(dGeomID g, dTriCallback* Callback);
//dTriCallback*   (ODE_API *dGeomTriMeshGetCallback)(dGeomID g);
//void            (ODE_API *dGeomTriMeshSetArrayCallback)(dGeomID g, dTriArrayCallback* ArrayCallback);
//dTriArrayCallback* (ODE_API *dGeomTriMeshGetArrayCallback)(dGeomID g);
//void            (ODE_API *dGeomTriMeshSetRayCallback)(dGeomID g, dTriRayCallback* Callback);
//dTriRayCallback* (ODE_API *dGeomTriMeshGetRayCallback)(dGeomID g);
//void            (ODE_API *dGeomTriMeshSetTriMergeCallback)(dGeomID g, dTriTriMergeCallback* Callback);
//dTriTriMergeCallback* (ODE_API *dGeomTriMeshGetTriMergeCallback)(dGeomID g);
dGeomID         (ODE_API *dCreateTriMesh)(dSpaceID space, dTriMeshDataID Data, dTriCallback* Callback, dTriArrayCallback* ArrayCallback, dTriRayCallback* RayCallback);
//void            (ODE_API *dGeomTriMeshSetData)(dGeomID g, dTriMeshDataID Data);
//dTriMeshDataID  (ODE_API *dGeomTriMeshGetData)(dGeomID g);
//void            (ODE_API *dGeomTriMeshEnableTC)(dGeomID g, int geomClass, int enable);
//int             (ODE_API *dGeomTriMeshIsTCEnabled)(dGeomID g, int geomClass);
//void            (ODE_API *dGeomTriMeshClearTCCache)(dGeomID g);
//dTriMeshDataID  (ODE_API *dGeomTriMeshGetTriMeshDataID)(dGeomID g);
//void            (ODE_API *dGeomTriMeshGetTriangle)(dGeomID g, int Index, dVector3* v0, dVector3* v1, dVector3* v2);
//void            (ODE_API *dGeomTriMeshGetPoint)(dGeomID g, int Index, dReal u, dReal v, dVector3 Out);
//int             (ODE_API *dGeomTriMeshGetTriangleCount )(dGeomID g);
//void            (ODE_API *dGeomTriMeshDataUpdate)(dTriMeshDataID g);

typedef void dMessageFunction (int errnum, const char *msg, va_list ap);
void (ODE_API  *dSetErrorHandler) (dMessageFunction *fn);
void (ODE_API  *dSetDebugHandler) (dMessageFunction *fn);
void (ODE_API  *dSetMessageHandler) (dMessageFunction *fn);

static dllfunction_t odefuncs[] =
{
//	{"dGetConfiguration",							(void **) &dGetConfiguration},
	{(void **) &dCheckConfiguration,				"dCheckConfiguration"},
	{(void **) &dInitODE,							"dInitODE"},
//	{"dInitODE2",									(void **) &dInitODE2},
//	{"dAllocateODEDataForThread",					(void **) &dAllocateODEDataForThread},
//	{"dCleanupODEAllDataForThread",					(void **) &dCleanupODEAllDataForThread},
	{(void **) &dCloseODE,							"dCloseODE"},
//	{"dMassCheck",									(void **) &dMassCheck},
//	{"dMassSetZero",								(void **) &dMassSetZero},
//	{"dMassSetParameters",							(void **) &dMassSetParameters},
//	{"dMassSetSphere",								(void **) &dMassSetSphere},
	{(void **) &dMassSetSphereTotal,				"dMassSetSphereTotal"},
//	{"dMassSetCapsule",								(void **) &dMassSetCapsule},
	{(void **) &dMassSetCapsuleTotal,				"dMassSetCapsuleTotal"},
//	{"dMassSetCylinder",							(void **) &dMassSetCylinder},
	{(void **) &dMassSetCylinderTotal,				"dMassSetCylinderTotal"},
//	{"dMassSetBox",									(void **) &dMassSetBox},
	{(void **) &dMassSetBoxTotal,					"dMassSetBoxTotal"},
//	{"dMassSetTrimesh",								(void **) &dMassSetTrimesh},
//	{"dMassSetTrimeshTotal",						(void **) &dMassSetTrimeshTotal},
//	{"dMassAdjust",									(void **) &dMassAdjust},
//	{"dMassTranslate",								(void **) &dMassTranslate},
//	{"dMassRotate",									(void **) &dMassRotate},
//	{"dMassAdd",									(void **) &dMassAdd},

	{(void **) &dWorldCreate,						"dWorldCreate"},
	{(void **) &dWorldDestroy,						"dWorldDestroy"},
	{(void **) &dWorldSetGravity,					"dWorldSetGravity"},
	{(void **) &dWorldGetGravity,					"dWorldGetGravity"},
	{(void **) &dWorldSetERP,						"dWorldSetERP"},
//	{"dWorldGetERP",								(void **) &dWorldGetERP},
	{(void **) &dWorldSetCFM,						"dWorldSetCFM"},
//	{"dWorldGetCFM",								(void **) &dWorldGetCFM},
	{(void **) &dWorldStep,							"dWorldStep"},
//	{"dWorldImpulseToForce",						(void **) &dWorldImpulseToForce},
	{(void **) &dWorldQuickStep,					"dWorldQuickStep"},
	{(void **) &dWorldSetQuickStepNumIterations,	"dWorldSetQuickStepNumIterations"},
//	{"dWorldGetQuickStepNumIterations",				(void **) &dWorldGetQuickStepNumIterations},
//	{"dWorldSetQuickStepW",							(void **) &dWorldSetQuickStepW},
//	{"dWorldGetQuickStepW",							(void **) &dWorldGetQuickStepW},
//	{"dWorldSetContactMaxCorrectingVel",			(void **) &dWorldSetContactMaxCorrectingVel},
//	{"dWorldGetContactMaxCorrectingVel",			(void **) &dWorldGetContactMaxCorrectingVel},
	{(void **) &dWorldSetContactSurfaceLayer,		"dWorldSetContactSurfaceLayer"},
//	{"dWorldGetContactSurfaceLayer",				(void **) &dWorldGetContactSurfaceLayer},
//	{(void **) &dWorldStepFast1,					"dWorldStepFast1"},
//	{"dWorldSetAutoEnableDepthSF1",					(void **) &dWorldSetAutoEnableDepthSF1},
//	{"dWorldGetAutoEnableDepthSF1",					(void **) &dWorldGetAutoEnableDepthSF1},
//	{"dWorldGetAutoDisableLinearThreshold",			(void **) &dWorldGetAutoDisableLinearThreshold},
	{(void **) &dWorldSetAutoDisableLinearThreshold,"dWorldSetAutoDisableLinearThreshold"},
//	{"dWorldGetAutoDisableAngularThreshold",		(void **) &dWorldGetAutoDisableAngularThreshold},
	{(void **) &dWorldSetAutoDisableAngularThreshold,"dWorldSetAutoDisableAngularThreshold"},
//	{"dWorldGetAutoDisableLinearAverageThreshold",	(void **) &dWorldGetAutoDisableLinearAverageThreshold},
//	{"dWorldSetAutoDisableLinearAverageThreshold",	(void **) &dWorldSetAutoDisableLinearAverageThreshold},
//	{"dWorldGetAutoDisableAngularAverageThreshold",	(void **) &dWorldGetAutoDisableAngularAverageThreshold},
//	{"dWorldSetAutoDisableAngularAverageThreshold",	(void **) &dWorldSetAutoDisableAngularAverageThreshold},
//	{"dWorldGetAutoDisableAverageSamplesCount",		(void **) &dWorldGetAutoDisableAverageSamplesCount},
	{(void **)&dWorldSetAutoDisableAverageSamplesCount,		"dWorldSetAutoDisableAverageSamplesCount"},
//	{"dWorldGetAutoDisableSteps",					(void **) &dWorldGetAutoDisableSteps},
	{(void **) &dWorldSetAutoDisableSteps,			"dWorldSetAutoDisableSteps"},
//	{"dWorldGetAutoDisableTime",					(void **) &dWorldGetAutoDisableTime},
	{(void **) &dWorldSetAutoDisableTime,			"dWorldSetAutoDisableTime"},
//	{"dWorldGetAutoDisableFlag",					(void **) &dWorldGetAutoDisableFlag},
	{(void **) &dWorldSetAutoDisableFlag,			"dWorldSetAutoDisableFlag"},
//	{"dWorldGetLinearDampingThreshold",				(void **) &dWorldGetLinearDampingThreshold},
	{(void **) &dWorldSetLinearDampingThreshold,	"dWorldSetLinearDampingThreshold"},
//	{"dWorldGetAngularDampingThreshold",			(void **) &dWorldGetAngularDampingThreshold},
	{(void **) &dWorldSetAngularDampingThreshold,	"dWorldSetAngularDampingThreshold"},
//	{"dWorldGetLinearDamping",						(void **) &dWorldGetLinearDamping},
	{(void **) &dWorldSetLinearDamping,				"dWorldSetLinearDamping"},
//	{"dWorldGetAngularDamping",						(void **) &dWorldGetAngularDamping},
	{(void **) &dWorldSetAngularDamping,			"dWorldSetAngularDamping"},
//	{(void **) &dWorldSetDamping,					"dWorldSetDamping"},
//	{"dWorldGetMaxAngularSpeed",					(void **) &dWorldGetMaxAngularSpeed},
//	{"dWorldSetMaxAngularSpeed",					(void **) &dWorldSetMaxAngularSpeed},
//	{"dBodyGetAutoDisableLinearThreshold",			(void **) &dBodyGetAutoDisableLinearThreshold},
//	{"dBodySetAutoDisableLinearThreshold",			(void **) &dBodySetAutoDisableLinearThreshold},
//	{"dBodyGetAutoDisableAngularThreshold",			(void **) &dBodyGetAutoDisableAngularThreshold},
//	{"dBodySetAutoDisableAngularThreshold",			(void **) &dBodySetAutoDisableAngularThreshold},
//	{"dBodyGetAutoDisableAverageSamplesCount",		(void **) &dBodyGetAutoDisableAverageSamplesCount},
//	{"dBodySetAutoDisableAverageSamplesCount",		(void **) &dBodySetAutoDisableAverageSamplesCount},
//	{"dBodyGetAutoDisableSteps",					(void **) &dBodyGetAutoDisableSteps},
//	{"dBodySetAutoDisableSteps",					(void **) &dBodySetAutoDisableSteps},
//	{"dBodyGetAutoDisableTime",						(void **) &dBodyGetAutoDisableTime},
//	{"dBodySetAutoDisableTime",						(void **) &dBodySetAutoDisableTime},
//	{"dBodyGetAutoDisableFlag",						(void **) &dBodyGetAutoDisableFlag},
//	{"dBodySetAutoDisableFlag",						(void **) &dBodySetAutoDisableFlag},
//	{"dBodySetAutoDisableDefaults",					(void **) &dBodySetAutoDisableDefaults},
//	{"dBodyGetWorld",								(void **) &dBodyGetWorld},
	{(void **) &dBodyCreate,						"dBodyCreate"},
	{(void **) &dBodyDestroy,						"dBodyDestroy"},
	{(void **) &dBodySetData,						"dBodySetData"},
	{(void **) &dBodyGetData,						"dBodyGetData"},
	{(void **) &dBodySetPosition,					"dBodySetPosition"},
	{(void **) &dBodySetRotation,					"dBodySetRotation"},
//	{"dBodySetQuaternion",							(void **) &dBodySetQuaternion},
	{(void **) &dBodySetLinearVel,					"dBodySetLinearVel"},
	{(void **) &dBodySetAngularVel,					"dBodySetAngularVel"},
	{(void **) &dBodyGetPosition,					"dBodyGetPosition"},
//	{"dBodyCopyPosition",							(void **) &dBodyCopyPosition},
	{(void **) &dBodyGetRotation,					"dBodyGetRotation"},
//	{"dBodyCopyRotation",							(void **) &dBodyCopyRotation},
//	{"dBodyGetQuaternion",							(void **) &dBodyGetQuaternion},
//	{"dBodyCopyQuaternion",							(void **) &dBodyCopyQuaternion},
	{(void **) &dBodyGetLinearVel,					"dBodyGetLinearVel"},
	{(void **) &dBodyGetAngularVel,					"dBodyGetAngularVel"},
	{(void **) &dBodySetMass,						"dBodySetMass"},
//	{"dBodyGetMass",								(void **) &dBodyGetMass},
//	{"dBodyAddForce",								(void **) &dBodyAddForce},
	{(void **) &dBodyAddTorque,						"dBodyAddTorque"},
//	{"dBodyAddRelForce",							(void **) &dBodyAddRelForce},
//	{"dBodyAddRelTorque",							(void **) &dBodyAddRelTorque},
	{(void **) &dBodyAddForceAtPos,					"dBodyAddForceAtPos"},
//	{"dBodyAddForceAtRelPos",						(void **) &dBodyAddForceAtRelPos},
//	{"dBodyAddRelForceAtPos",						(void **) &dBodyAddRelForceAtPos},
//	{"dBodyAddRelForceAtRelPos",					(void **) &dBodyAddRelForceAtRelPos},
//	{"dBodyGetForce",								(void **) &dBodyGetForce},
//	{"dBodyGetTorque",								(void **) &dBodyGetTorque},
//	{"dBodySetForce",								(void **) &dBodySetForce},
//	{"dBodySetTorque",								(void **) &dBodySetTorque},
//	{"dBodyGetRelPointPos",							(void **) &dBodyGetRelPointPos},
//	{"dBodyGetRelPointVel",							(void **) &dBodyGetRelPointVel},
//	{"dBodyGetPointVel",							(void **) &dBodyGetPointVel},
//	{"dBodyGetPosRelPoint",							(void **) &dBodyGetPosRelPoint},
//	{"dBodyVectorToWorld",							(void **) &dBodyVectorToWorld},
//	{"dBodyVectorFromWorld",						(void **) &dBodyVectorFromWorld},
//	{"dBodySetFiniteRotationMode",					(void **) &dBodySetFiniteRotationMode},
//	{"dBodySetFiniteRotationAxis",					(void **) &dBodySetFiniteRotationAxis},
//	{"dBodyGetFiniteRotationMode",					(void **) &dBodyGetFiniteRotationMode},
//	{"dBodyGetFiniteRotationAxis",					(void **) &dBodyGetFiniteRotationAxis},
	{(void **) &dBodyGetNumJoints,					"dBodyGetNumJoints"},
	{(void **) &dBodyGetJoint,						"dBodyGetJoint"},
//	{"dBodySetDynamic",								(void **) &dBodySetDynamic},
//	{"dBodySetKinematic",							(void **) &dBodySetKinematic},
//	{"dBodyIsKinematic",							(void **) &dBodyIsKinematic},
	{(void **) &dBodyEnable,						"dBodyEnable"},
	{(void **) &dBodyDisable,						"dBodyDisable"},
//	{"dBodyIsEnabled",								(void **) &dBodyIsEnabled},
	{(void **) &dBodySetGravityMode,				"dBodySetGravityMode"},
	{(void **) &dBodyGetGravityMode,				"dBodyGetGravityMode"},
//	{"dBodySetMovedCallback",						(void **) &dBodySetMovedCallback},
//	{"dBodyGetFirstGeom",							(void **) &dBodyGetFirstGeom},
//	{"dBodyGetNextGeom",							(void **) &dBodyGetNextGeom},
//	{"dBodySetDampingDefaults",						(void **) &dBodySetDampingDefaults},
//	{"dBodyGetLinearDamping",						(void **) &dBodyGetLinearDamping},
	{"dBodySetLinearDamping",						(void **) &dBodySetLinearDamping},
//	{"dBodyGetAngularDamping",						(void **) &dBodyGetAngularDamping},
	{"dBodySetAngularDamping",						(void **) &dBodySetAngularDamping},
//	{"dBodySetDamping",								(void **) &dBodySetDamping},
//	{"dBodyGetLinearDampingThreshold",				(void **) &dBodyGetLinearDampingThreshold},
//	{"dBodySetLinearDampingThreshold",				(void **) &dBodySetLinearDampingThreshold},
//	{"dBodyGetAngularDampingThreshold",				(void **) &dBodyGetAngularDampingThreshold},
//	{"dBodySetAngularDampingThreshold",				(void **) &dBodySetAngularDampingThreshold},
//	{"dBodyGetMaxAngularSpeed",						(void **) &dBodyGetMaxAngularSpeed},
	{"dBodySetMaxAngularSpeed",						(void **) &dBodySetMaxAngularSpeed},
//	{"dBodyGetGyroscopicMode",						(void **) &dBodyGetGyroscopicMode},
//	{"dBodySetGyroscopicMode",						(void **) &dBodySetGyroscopicMode},
	{(void **) &dJointCreateBall,					"dJointCreateBall"},
	{(void **) &dJointCreateHinge,					"dJointCreateHinge"},
	{(void **) &dJointCreateSlider,					"dJointCreateSlider"},
	{(void **) &dJointCreateContact,				"dJointCreateContact"},
	{(void **) &dJointCreateHinge2,					"dJointCreateHinge2"},
	{(void **) &dJointCreateUniversal,				"dJointCreateUniversal"},
//	{"dJointCreatePR",								(void **) &dJointCreatePR},
//	{"dJointCreatePU",								(void **) &dJointCreatePU},
//	{"dJointCreatePiston",							(void **) &dJointCreatePiston},
	{(void **) &dJointCreateFixed,					"dJointCreateFixed"},
//	{"dJointCreateNull",							(void **) &dJointCreateNull},
//	{"dJointCreateAMotor",							(void **) &dJointCreateAMotor},
//	{"dJointCreateLMotor",							(void **) &dJointCreateLMotor},
//	{"dJointCreatePlane2D",							(void **) &dJointCreatePlane2D},
	{(void **) &dJointDestroy,						"dJointDestroy"},
	{(void **) &dJointGroupCreate,					"dJointGroupCreate"},
	{(void **) &dJointGroupDestroy,					"dJointGroupDestroy"},
	{(void **) &dJointGroupEmpty,					"dJointGroupEmpty"},
//	{"dJointGetNumBodies",							(void **) &dJointGetNumBodies},
	{(void **) &dJointAttach,						"dJointAttach"},
	{(void **) &dJointEnable,						"dJointEnable"},
	{(void **) &dJointDisable,						"dJointDisable"},
//	{"dJointIsEnabled",								(void **) &dJointIsEnabled},
	{(void **) &dJointSetData,						"dJointSetData"},
	{(void **) &dJointGetData,						"dJointGetData"},
//	{"dJointGetType",								(void **) &dJointGetType},
	{(void **) &dJointGetBody,						"dJointGetBody"},
//	{"dJointSetFeedback",							(void **) &dJointSetFeedback},
//	{"dJointGetFeedback",							(void **) &dJointGetFeedback},
	{(void **) &dJointSetBallAnchor,				"dJointSetBallAnchor"},
//	{"dJointSetBallAnchor2",						(void **) &dJointSetBallAnchor2},
	{(void **) &dJointSetBallParam,					"dJointSetBallParam"},
	{(void **) &dJointSetHingeAnchor,				"dJointSetHingeAnchor"},
//	{"dJointSetHingeAnchorDelta",					(void **) &dJointSetHingeAnchorDelta},
	{(void **) &dJointSetHingeAxis,					"dJointSetHingeAxis"},
//	{"dJointSetHingeAxisOffset",					(void **) &dJointSetHingeAxisOffset},
	{(void **) &dJointSetHingeParam,				"dJointSetHingeParam"},
//	{"dJointAddHingeTorque",						(void **) &dJointAddHingeTorque},
	{(void **) &dJointSetSliderAxis,				"dJointSetSliderAxis"},
//	{"dJointSetSliderAxisDelta",					(void **) &dJointSetSliderAxisDelta},
	{(void **) &dJointSetSliderParam,				"dJointSetSliderParam"},
//	{"dJointAddSliderForce",						(void **) &dJointAddSliderForce},
	{(void **) &dJointSetHinge2Anchor,				"dJointSetHinge2Anchor"},
	{(void **) &dJointSetHinge2Axis1,				"dJointSetHinge2Axis1"},
	{(void **) &dJointSetHinge2Axis2,				"dJointSetHinge2Axis2"},
	{(void **) &dJointSetHinge2Param,				"dJointSetHinge2Param"},
//	{"dJointAddHinge2Torques",						(void **) &dJointAddHinge2Torques},
	{(void **) &dJointSetUniversalAnchor,			"dJointSetUniversalAnchor"},
	{(void **) &dJointSetUniversalAxis1,			"dJointSetUniversalAxis1"},
//	{"dJointSetUniversalAxis1Offset",				(void **) &dJointSetUniversalAxis1Offset},
	{(void **) &dJointSetUniversalAxis2,			"dJointSetUniversalAxis2"},
//	{"dJointSetUniversalAxis2Offset",				(void **) &dJointSetUniversalAxis2Offset},
	{(void **) &dJointSetUniversalParam,			"dJointSetUniversalParam"},
//	{"dJointAddUniversalTorques",					(void **) &dJointAddUniversalTorques},
//	{"dJointSetPRAnchor",							(void **) &dJointSetPRAnchor},
//	{"dJointSetPRAxis1",							(void **) &dJointSetPRAxis1},
//	{"dJointSetPRAxis2",							(void **) &dJointSetPRAxis2},
//	{"dJointSetPRParam",							(void **) &dJointSetPRParam},
//	{"dJointAddPRTorque",							(void **) &dJointAddPRTorque},
//	{"dJointSetPUAnchor",							(void **) &dJointSetPUAnchor},
//	{"dJointSetPUAnchorOffset",						(void **) &dJointSetPUAnchorOffset},
//	{"dJointSetPUAxis1",							(void **) &dJointSetPUAxis1},
//	{"dJointSetPUAxis2",							(void **) &dJointSetPUAxis2},
//	{"dJointSetPUAxis3",							(void **) &dJointSetPUAxis3},
//	{"dJointSetPUAxisP",							(void **) &dJointSetPUAxisP},
//	{"dJointSetPUParam",							(void **) &dJointSetPUParam},
//	{"dJointAddPUTorque",							(void **) &dJointAddPUTorque},
//	{"dJointSetPistonAnchor",						(void **) &dJointSetPistonAnchor},
//	{"dJointSetPistonAnchorOffset",					(void **) &dJointSetPistonAnchorOffset},
//	{"dJointSetPistonParam",						(void **) &dJointSetPistonParam},
//	{"dJointAddPistonForce",						(void **) &dJointAddPistonForce},
	{(void **) &dJointSetFixed,						"dJointSetFixed"},
//	{"dJointSetFixedParam",							(void **) &dJointSetFixedParam},
//	{"dJointSetAMotorNumAxes",						(void **) &dJointSetAMotorNumAxes},
//	{"dJointSetAMotorAxis",							(void **) &dJointSetAMotorAxis},
//	{"dJointSetAMotorAngle",						(void **) &dJointSetAMotorAngle},
//	{"dJointSetAMotorParam",						(void **) &dJointSetAMotorParam},
//	{"dJointSetAMotorMode",							(void **) &dJointSetAMotorMode},
//	{"dJointAddAMotorTorques",						(void **) &dJointAddAMotorTorques},
//	{"dJointSetLMotorNumAxes",						(void **) &dJointSetLMotorNumAxes},
//	{"dJointSetLMotorAxis",							(void **) &dJointSetLMotorAxis},
//	{"dJointSetLMotorParam",						(void **) &dJointSetLMotorParam},
//	{"dJointSetPlane2DXParam",						(void **) &dJointSetPlane2DXParam},
//	{"dJointSetPlane2DYParam",						(void **) &dJointSetPlane2DYParam},
//	{"dJointSetPlane2DAngleParam",					(void **) &dJointSetPlane2DAngleParam},
	{(void **) &dJointGetBallAnchor,				"dJointGetBallAnchor"},
//	{"dJointGetBallAnchor2",						(void **) &dJointGetBallAnchor2},
//	{"dJointGetBallParam",							(void **) &dJointGetBallParam},
	{(void **) &dJointGetHingeAnchor,				"dJointGetHingeAnchor"},
//	{"dJointGetHingeAnchor2",						(void **) &dJointGetHingeAnchor2},
	{(void **) &dJointGetHingeAxis,					"dJointGetHingeAxis"},
//	{"dJointGetHingeParam",							(void **) &dJointGetHingeParam},
//	{"dJointGetHingeAngle",							(void **) &dJointGetHingeAngle},
//	{"dJointGetHingeAngleRate",						(void **) &dJointGetHingeAngleRate},
//	{"dJointGetSliderPosition",						(void **) &dJointGetSliderPosition},
//	{"dJointGetSliderPositionRate",					(void **) &dJointGetSliderPositionRate},
	{(void **) &dJointGetSliderAxis,				"dJointGetSliderAxis"},
//	{"dJointGetSliderParam",						(void **) &dJointGetSliderParam},
//	{"dJointGetHinge2Anchor",						(void **) &dJointGetHinge2Anchor},
//	{"dJointGetHinge2Anchor2",						(void **) &dJointGetHinge2Anchor2},
	{(void **) &dJointGetHinge2Axis1,				"dJointGetHinge2Axis1"},
	{(void **) &dJointGetHinge2Axis2,				"dJointGetHinge2Axis2"},
//	{"dJointGetHinge2Param",						(void **) &dJointGetHinge2Param},
//	{"dJointGetHinge2Angle1",						(void **) &dJointGetHinge2Angle1},
//	{"dJointGetHinge2Angle1Rate",					(void **) &dJointGetHinge2Angle1Rate},
//	{"dJointGetHinge2Angle2Rate",					(void **) &dJointGetHinge2Angle2Rate},
	{(void **) &dJointGetUniversalAnchor,			"dJointGetUniversalAnchor"},
//	{"dJointGetUniversalAnchor2",					(void **) &dJointGetUniversalAnchor2},
	{(void **) &dJointGetUniversalAxis1,			"dJointGetUniversalAxis1"},
	{(void **) &dJointGetUniversalAxis2,			"dJointGetUniversalAxis2"},
//	{"dJointGetUniversalParam",						(void **) &dJointGetUniversalParam},
//	{"dJointGetUniversalAngles",					(void **) &dJointGetUniversalAngles},
//	{"dJointGetUniversalAngle1",					(void **) &dJointGetUniversalAngle1},
//	{"dJointGetUniversalAngle2",					(void **) &dJointGetUniversalAngle2},
//	{"dJointGetUniversalAngle1Rate",				(void **) &dJointGetUniversalAngle1Rate},
//	{"dJointGetUniversalAngle2Rate",				(void **) &dJointGetUniversalAngle2Rate},
//	{"dJointGetPRAnchor",							(void **) &dJointGetPRAnchor},
//	{"dJointGetPRPosition",							(void **) &dJointGetPRPosition},
//	{"dJointGetPRPositionRate",						(void **) &dJointGetPRPositionRate},
//	{"dJointGetPRAngle",							(void **) &dJointGetPRAngle},
//	{"dJointGetPRAngleRate",						(void **) &dJointGetPRAngleRate},
//	{"dJointGetPRAxis1",							(void **) &dJointGetPRAxis1},
//	{"dJointGetPRAxis2",							(void **) &dJointGetPRAxis2},
//	{"dJointGetPRParam",							(void **) &dJointGetPRParam},
//	{"dJointGetPUAnchor",							(void **) &dJointGetPUAnchor},
//	{"dJointGetPUPosition",							(void **) &dJointGetPUPosition},
//	{"dJointGetPUPositionRate",						(void **) &dJointGetPUPositionRate},
//	{"dJointGetPUAxis1",							(void **) &dJointGetPUAxis1},
//	{"dJointGetPUAxis2",							(void **) &dJointGetPUAxis2},
//	{"dJointGetPUAxis3",							(void **) &dJointGetPUAxis3},
//	{"dJointGetPUAxisP",							(void **) &dJointGetPUAxisP},
//	{"dJointGetPUAngles",							(void **) &dJointGetPUAngles},
//	{"dJointGetPUAngle1",							(void **) &dJointGetPUAngle1},
//	{"dJointGetPUAngle1Rate",						(void **) &dJointGetPUAngle1Rate},
//	{"dJointGetPUAngle2",							(void **) &dJointGetPUAngle2},
//	{"dJointGetPUAngle2Rate",						(void **) &dJointGetPUAngle2Rate},
//	{"dJointGetPUParam",							(void **) &dJointGetPUParam},
//	{"dJointGetPistonPosition",						(void **) &dJointGetPistonPosition},
//	{"dJointGetPistonPositionRate",					(void **) &dJointGetPistonPositionRate},
//	{"dJointGetPistonAngle",						(void **) &dJointGetPistonAngle},
//	{"dJointGetPistonAngleRate",					(void **) &dJointGetPistonAngleRate},
//	{"dJointGetPistonAnchor",						(void **) &dJointGetPistonAnchor},
//	{"dJointGetPistonAnchor2",						(void **) &dJointGetPistonAnchor2},
//	{"dJointGetPistonAxis",							(void **) &dJointGetPistonAxis},
//	{"dJointGetPistonParam",						(void **) &dJointGetPistonParam},
//	{"dJointGetAMotorNumAxes",						(void **) &dJointGetAMotorNumAxes},
//	{"dJointGetAMotorAxis",							(void **) &dJointGetAMotorAxis},
//	{"dJointGetAMotorAxisRel",						(void **) &dJointGetAMotorAxisRel},
//	{"dJointGetAMotorAngle",						(void **) &dJointGetAMotorAngle},
//	{"dJointGetAMotorAngleRate",					(void **) &dJointGetAMotorAngleRate},
//	{"dJointGetAMotorParam",						(void **) &dJointGetAMotorParam},
//	{"dJointGetAMotorMode",							(void **) &dJointGetAMotorMode},
//	{"dJointGetLMotorNumAxes",						(void **) &dJointGetLMotorNumAxes},
//	{"dJointGetLMotorAxis",							(void **) &dJointGetLMotorAxis},
//	{"dJointGetLMotorParam",						(void **) &dJointGetLMotorParam},
//	{"dJointGetFixedParam",							(void **) &dJointGetFixedParam},
//	{"dConnectingJoint",							(void **) &dConnectingJoint},
//	{"dConnectingJointList",						(void **) &dConnectingJointList},
	{(void **) &dAreConnected,						"dAreConnected"},
	{(void **) &dAreConnectedExcluding,				"dAreConnectedExcluding"},
	{(void **) &dSimpleSpaceCreate,					"dSimpleSpaceCreate"},
	{(void **) &dHashSpaceCreate,					"dHashSpaceCreate"},
	{(void **) &dQuadTreeSpaceCreate,				"dQuadTreeSpaceCreate"},
//	{"dSweepAndPruneSpaceCreate",					(void **) &dSweepAndPruneSpaceCreate},
	{(void **) &dSpaceDestroy,						"dSpaceDestroy"},
//	{"dHashSpaceSetLevels",							(void **) &dHashSpaceSetLevels},
//	{"dHashSpaceGetLevels",							(void **) &dHashSpaceGetLevels},
//	{"dSpaceSetCleanup",							(void **) &dSpaceSetCleanup},
//	{"dSpaceGetCleanup",							(void **) &dSpaceGetCleanup},
//	{"dSpaceSetSublevel",							(void **) &dSpaceSetSublevel},
//	{"dSpaceGetSublevel",							(void **) &dSpaceGetSublevel},
//	{"dSpaceSetManualCleanup",						(void **) &dSpaceSetManualCleanup},
//	{"dSpaceGetManualCleanup",						(void **) &dSpaceGetManualCleanup},
//	{"dSpaceAdd",									(void **) &dSpaceAdd},
//	{"dSpaceRemove",								(void **) &dSpaceRemove},
//	{"dSpaceQuery",									(void **) &dSpaceQuery},
//	{"dSpaceClean",									(void **) &dSpaceClean},
//	{"dSpaceGetNumGeoms",							(void **) &dSpaceGetNumGeoms},
//	{"dSpaceGetGeom",								(void **) &dSpaceGetGeom},
//	{"dSpaceGetClass",								(void **) &dSpaceGetClass},
	{(void **) &dGeomDestroy,						"dGeomDestroy"},
	{(void **) &dGeomSetData,						"dGeomSetData"},
	{(void **) &dGeomGetData,						"dGeomGetData"},
	{(void **) &dGeomSetBody,						"dGeomSetBody"},
	{(void **) &dGeomGetBody,						"dGeomGetBody"},
	{(void **) &dGeomSetPosition,					"dGeomSetPosition"},
	{(void **) &dGeomSetRotation,					"dGeomSetRotation"},
//	{"dGeomSetQuaternion",							(void **) &dGeomSetQuaternion},
//	{"dGeomGetPosition",							(void **) &dGeomGetPosition},
//	{"dGeomCopyPosition",							(void **) &dGeomCopyPosition},
//	{"dGeomGetRotation",							(void **) &dGeomGetRotation},
//	{"dGeomCopyRotation",							(void **) &dGeomCopyRotation},
//	{"dGeomGetQuaternion",							(void **) &dGeomGetQuaternion},
//	{"dGeomGetAABB",								(void **) &dGeomGetAABB},
	{(void **) &dGeomIsSpace,						"dGeomIsSpace"},
//	{"dGeomGetSpace",								(void **) &dGeomGetSpace},
//	{"dGeomGetClass",								(void **) &dGeomGetClass},
//	{"dGeomSetCategoryBits",						(void **) &dGeomSetCategoryBits},
//	{"dGeomSetCollideBits",							(void **) &dGeomSetCollideBits},
//	{"dGeomGetCategoryBits",						(void **) &dGeomGetCategoryBits},
//	{"dGeomGetCollideBits",							(void **) &dGeomGetCollideBits},
//	{"dGeomEnable",									(void **) &dGeomEnable},
//	{"dGeomDisable",								(void **) &dGeomDisable},
//	{"dGeomIsEnabled",								(void **) &dGeomIsEnabled},
//	{"dGeomSetOffsetPosition",						(void **) &dGeomSetOffsetPosition},
//	{"dGeomSetOffsetRotation",						(void **) &dGeomSetOffsetRotation},
//	{"dGeomSetOffsetQuaternion",					(void **) &dGeomSetOffsetQuaternion},
//	{"dGeomSetOffsetWorldPosition",					(void **) &dGeomSetOffsetWorldPosition},
//	{"dGeomSetOffsetWorldRotation",					(void **) &dGeomSetOffsetWorldRotation},
//	{"dGeomSetOffsetWorldQuaternion",				(void **) &dGeomSetOffsetWorldQuaternion},
//	{"dGeomClearOffset",							(void **) &dGeomClearOffset},
//	{"dGeomIsOffset",								(void **) &dGeomIsOffset},
//	{"dGeomGetOffsetPosition",						(void **) &dGeomGetOffsetPosition},
//	{"dGeomCopyOffsetPosition",						(void **) &dGeomCopyOffsetPosition},
//	{"dGeomGetOffsetRotation",						(void **) &dGeomGetOffsetRotation},
//	{"dGeomCopyOffsetRotation",						(void **) &dGeomCopyOffsetRotation},
//	{"dGeomGetOffsetQuaternion",					(void **) &dGeomGetOffsetQuaternion},
	{(void **) &dCollide,							"dCollide"},
	{(void **) &dSpaceCollide,						"dSpaceCollide"},
	{(void **) &dSpaceCollide2,						"dSpaceCollide2"},
	{(void **) &dCreateSphere,						"dCreateSphere"},
//	{"dGeomSphereSetRadius",						(void **) &dGeomSphereSetRadius},
//	{"dGeomSphereGetRadius",						(void **) &dGeomSphereGetRadius},
//	{"dGeomSpherePointDepth",						(void **) &dGeomSpherePointDepth},
//	{"dCreateConvex",								(void **) &dCreateConvex},
//	{"dGeomSetConvex",								(void **) &dGeomSetConvex},
	{(void **) &dCreateBox,							"dCreateBox"},
//	{"dGeomBoxSetLengths",							(void **) &dGeomBoxSetLengths},
//	{"dGeomBoxGetLengths",							(void **) &dGeomBoxGetLengths},
//	{"dGeomBoxPointDepth",							(void **) &dGeomBoxPointDepth},
//	{"dGeomBoxPointDepth",							(void **) &dGeomBoxPointDepth},
//	{"dCreatePlane",								(void **) &dCreatePlane},
//	{"dGeomPlaneSetParams",							(void **) &dGeomPlaneSetParams},
//	{"dGeomPlaneGetParams",							(void **) &dGeomPlaneGetParams},
//	{"dGeomPlanePointDepth",						(void **) &dGeomPlanePointDepth},
	{(void **) &dCreateCapsule,						"dCreateCapsule"},
//	{"dGeomCapsuleSetParams",						(void **) &dGeomCapsuleSetParams},
//	{"dGeomCapsuleGetParams",						(void **) &dGeomCapsuleGetParams},
//	{"dGeomCapsulePointDepth",						(void **) &dGeomCapsulePointDepth},
	{(void **) &dCreateCylinder,					"dCreateCylinder"},
//	{"dGeomCylinderSetParams",						(void **) &dGeomCylinderSetParams},
//	{"dGeomCylinderGetParams",						(void **) &dGeomCylinderGetParams},
//	{"dCreateRay",									(void **) &dCreateRay},
//	{"dGeomRaySetLength",							(void **) &dGeomRaySetLength},
//	{"dGeomRayGetLength",							(void **) &dGeomRayGetLength},
//	{"dGeomRaySet",									(void **) &dGeomRaySet},
//	{"dGeomRayGet",									(void **) &dGeomRayGet},
	{(void **) &dCreateGeomTransform,				"dCreateGeomTransform"},
	{(void **) &dGeomTransformSetGeom,				"dGeomTransformSetGeom"},
//	{"dGeomTransformGetGeom",						(void **) &dGeomTransformGetGeom},
	{(void **) &dGeomTransformSetCleanup,			"dGeomTransformSetCleanup"},
//	{"dGeomTransformGetCleanup",					(void **) &dGeomTransformGetCleanup},
//	{"dGeomTransformSetInfo",						(void **) &dGeomTransformSetInfo},
//	{"dGeomTransformGetInfo",						(void **) &dGeomTransformGetInfo},
	{(void **) &dGeomTriMeshDataCreate,				"dGeomTriMeshDataCreate"},
	{(void **) &dGeomTriMeshDataDestroy,			"dGeomTriMeshDataDestroy"},
//	{"dGeomTriMeshDataSet",                         (void **) &dGeomTriMeshDataSet},
//	{"dGeomTriMeshDataGet",                         (void **) &dGeomTriMeshDataGet},
//	{"dGeomTriMeshSetLastTransform",                (void **) &dGeomTriMeshSetLastTransform},
//	{"dGeomTriMeshGetLastTransform",                (void **) &dGeomTriMeshGetLastTransform},
	{(void **) &dGeomTriMeshDataBuildSingle,		"dGeomTriMeshDataBuildSingle"},
//	{"dGeomTriMeshDataBuildSingle1",                (void **) &dGeomTriMeshDataBuildSingle1},
//	{"dGeomTriMeshDataBuildDouble",                 (void **) &dGeomTriMeshDataBuildDouble},
//	{"dGeomTriMeshDataBuildDouble1",                (void **) &dGeomTriMeshDataBuildDouble1},
//	{"dGeomTriMeshDataBuildSimple",                 (void **) &dGeomTriMeshDataBuildSimple},
//	{"dGeomTriMeshDataBuildSimple1",                (void **) &dGeomTriMeshDataBuildSimple1},
//	{"dGeomTriMeshDataPreprocess",                  (void **) &dGeomTriMeshDataPreprocess},
//	{"dGeomTriMeshDataGetBuffer",                   (void **) &dGeomTriMeshDataGetBuffer},
//	{"dGeomTriMeshDataSetBuffer",                   (void **) &dGeomTriMeshDataSetBuffer},
//	{"dGeomTriMeshSetCallback",                     (void **) &dGeomTriMeshSetCallback},
//	{"dGeomTriMeshGetCallback",                     (void **) &dGeomTriMeshGetCallback},
//	{"dGeomTriMeshSetArrayCallback",                (void **) &dGeomTriMeshSetArrayCallback},
//	{"dGeomTriMeshGetArrayCallback",                (void **) &dGeomTriMeshGetArrayCallback},
//	{"dGeomTriMeshSetRayCallback",                  (void **) &dGeomTriMeshSetRayCallback},
//	{"dGeomTriMeshGetRayCallback",                  (void **) &dGeomTriMeshGetRayCallback},
//	{"dGeomTriMeshSetTriMergeCallback",             (void **) &dGeomTriMeshSetTriMergeCallback},
//	{"dGeomTriMeshGetTriMergeCallback",             (void **) &dGeomTriMeshGetTriMergeCallback},
	{(void **) &dCreateTriMesh,						"dCreateTriMesh"},
//	{"dGeomTriMeshSetData",                         (void **) &dGeomTriMeshSetData},
//	{"dGeomTriMeshGetData",                         (void **) &dGeomTriMeshGetData},
//	{"dGeomTriMeshEnableTC",                        (void **) &dGeomTriMeshEnableTC},
//	{"dGeomTriMeshIsTCEnabled",                     (void **) &dGeomTriMeshIsTCEnabled},
//	{"dGeomTriMeshClearTCCache",                    (void **) &dGeomTriMeshClearTCCache},
//	{"dGeomTriMeshGetTriMeshDataID",                (void **) &dGeomTriMeshGetTriMeshDataID},
//	{"dGeomTriMeshGetTriangle",                     (void **) &dGeomTriMeshGetTriangle},
//	{"dGeomTriMeshGetPoint",                        (void **) &dGeomTriMeshGetPoint},
//	{"dGeomTriMeshGetTriangleCount",                (void **) &dGeomTriMeshGetTriangleCount},
//	{"dGeomTriMeshDataUpdate",                      (void **) &dGeomTriMeshDataUpdate},

	{(void **) &dSetErrorHandler,					"dSetErrorHandler"},
	{(void **) &dSetDebugHandler,					"dSetDebugHandler"},
	{(void **) &dSetMessageHandler,					"dSetMessageHandler"},
	{NULL, NULL}
};

// Handle for ODE DLL
static dllhandle_t *ode_dll = NULL;
#endif

static cvar_t *physics_ode_quadtree_depth;
static cvar_t *physics_ode_contactsurfacelayer;
static cvar_t *physics_ode_worldquickstep;
static cvar_t *physics_ode_worldquickstep_iterations;
static cvar_t *physics_ode_contact_mu;
static cvar_t *physics_ode_contact_erp;
static cvar_t *physics_ode_contact_cfm;
static cvar_t *physics_ode_world_damping;
static cvar_t *physics_ode_world_damping_linear;
static cvar_t *physics_ode_world_damping_linear_threshold;
static cvar_t *physics_ode_world_damping_angular;
static cvar_t *physics_ode_world_damping_angular_threshold;
static cvar_t *physics_ode_world_erp;
static cvar_t *physics_ode_world_cfm;
static cvar_t *physics_ode_iterationsperframe;
static cvar_t *physics_ode_movelimit;
static cvar_t *physics_ode_spinlimit;
static cvar_t *physics_ode_autodisable;
static cvar_t *physics_ode_autodisable_steps;
static cvar_t *physics_ode_autodisable_time;
static cvar_t *physics_ode_autodisable_threshold_linear;
static cvar_t *physics_ode_autodisable_threshold_angular;
static cvar_t *physics_ode_autodisable_threshold_samples;
static cvar_t *physics_ode_maxspeed;

struct odectx_s
{
	rigidbodyengine_t pub;

	qboolean hasextraobjs;
	dWorldID dworld;
	void *space;
	void *contactgroup;
	// number of constraint solver iterations to use (for dWorldStepFast)
	int iterations;
	// actual step (server frametime / ode_iterations)
	vec_t step;
	// max velocity for a 1-unit radius object at current step to prevent
	// missed collisions
	vec_t movelimit;
	rbecommandqueue_t *cmdqueuehead;
	rbecommandqueue_t *cmdqueuetail;
};


static void World_ODE_RunCmd(world_t *world, rbecommandqueue_t *cmd);

#ifdef _WIN32
	#undef vsnprintf
	#undef _vsnprintf
	#define vsnprintf(s,l,f,a) _vsnprintf(s,l,f,a);string[sizeof(string)-1] = 0;
#endif

static void MyODEErrorHandler (int errnum, const char *msg, va_list ap)
{
	char		string[1024];
	vsnprintf (string,sizeof(string), msg, ap);
	string[sizeof(string)-1] = 0;
	Sys_Errorf("ODE ERROR %i: %s", errnum, string);
}
static void MyODEMessageHandler (int errnum, const char *msg, va_list ap)
{
	char		string[1024];
	vsnprintf (string,sizeof(string), msg, ap);
	string[sizeof(string)-1] = 0;
	Con_Printf("ODE Message %i: %s\n", errnum, string);
}

static qboolean World_ODE_Init(void)
{
#ifdef ODE_DYNAMIC
	const char* dllname =
	{
# if defined(_WIN64)
		"libode1"
# elif defined(_WIN32)
		"ode_double"
# elif defined(MACOSX)
		"libode.1.dylib"
# else
		"libode.so.1"
# endif
	};
#endif

	physics_ode_quadtree_depth					= cvarfuncs->GetNVFDG("physics_ode_quadtree_depth",					"5",	0,	"desired subdivision level of quadtree culling space",				"ODE Physics Library");
	physics_ode_contactsurfacelayer				= cvarfuncs->GetNVFDG("physics_ode_contactsurfacelayer",				"0",	0,	"allows objects to overlap this many units to reduce jitter",		"ODE Physics Library");
	physics_ode_worldquickstep					= cvarfuncs->GetNVFDG("physics_ode_worldquickstep",					"1",	0,	"use dWorldQuickStep rather than dWorldStep",						"ODE Physics Library");
	physics_ode_worldquickstep_iterations		= cvarfuncs->GetNVFDG("physics_ode_worldquickstep_iterations",		"20",	0,	"parameter to dWorldQuickStep",										"ODE Physics Library");
	physics_ode_contact_mu						= cvarfuncs->GetNVFDG("physics_ode_contact_mu",						"1",	0,	"contact solver mu parameter - friction pyramid approximation 1 (see ODE User Guide)",	"ODE Physics Library");
	physics_ode_contact_erp						= cvarfuncs->GetNVFDG("physics_ode_contact_erp",						"0.96",	0,	"contact solver erp parameter - Error Restitution Percent (see ODE User Guide)",		"ODE Physics Library");
	physics_ode_contact_cfm						= cvarfuncs->GetNVFDG("physics_ode_contact_cfm",						"0",	0,	"contact solver cfm parameter - Constraint Force Mixing (see ODE User Guide)",			"ODE Physics Library");
	physics_ode_world_damping					= cvarfuncs->GetNVFDG("physics_ode_world_damping",					"1",	0,	"enabled damping scale (see ODE User Guide), this scales all damping values, be aware that behavior depends of step type",	"ODE Physics Library");
	physics_ode_world_damping_linear			= cvarfuncs->GetNVFDG("physics_ode_world_damping_linear",			"-1",0,	"world linear damping scale (see ODE User Guide); use defaults when set to -1",			"ODE Physics Library");
	physics_ode_world_damping_linear_threshold	= cvarfuncs->GetNVFDG("physics_ode_world_damping_linear_threshold",	"-1",	0,	"world linear damping threshold (see ODE User Guide); use defaults when set to -1",		"ODE Physics Library");
	physics_ode_world_damping_angular			= cvarfuncs->GetNVFDG("physics_ode_world_damping_angular",			"-1",0,	"world angular damping scale (see ODE User Guide); use defaults when set to -1",		"ODE Physics Library");
	physics_ode_world_damping_angular_threshold	= cvarfuncs->GetNVFDG("physics_ode_world_damping_angular_threshold",	"-1",	0,	"world angular damping threshold (see ODE User Guide); use defaults when set to -1",	"ODE Physics Library");
	physics_ode_world_erp						= cvarfuncs->GetNVFDG("physics_ode_world_erp",						"-1",	0,	"world solver erp parameter - Error Restitution Percent (see ODE User Guide); use defaults when set to -1",			"ODE Physics Library");
	physics_ode_world_cfm						= cvarfuncs->GetNVFDG("physics_ode_world_cfm",						"-1",	0,	"world solver cfm parameter - Constraint Force Mixing (see ODE User Guide); not touched when -1",					"ODE Physics Library");
	physics_ode_iterationsperframe				= cvarfuncs->GetNVFDG("physics_ode_iterationsperframe",				"4",	0,	"divisor for time step, runs multiple physics steps per frame",														"ODE Physics Library");
	physics_ode_movelimit						= cvarfuncs->GetNVFDG("physics_ode_movelimit",						"0.5",	0,	"clamp velocity if a single move would exceed this percentage of object thickness, to prevent flying through walls","ODE Physics Library");
	physics_ode_spinlimit						= cvarfuncs->GetNVFDG("physics_ode_spinlimit",						"10000",0,	"reset spin velocity if it gets too large",																			"ODE Physics Library");
	physics_ode_maxspeed						= cvarfuncs->GetNVFDG("physics_ode_maxspeed",						"0",	0,	"clamp absolute velocity","ODE Physics Library");
	physics_ode_autodisable						= cvarfuncs->GetNVFDG("physics_ode_autodisable",						"1",	0,	"automatic disabling of objects which dont move for long period of time, makes object stacking a lot faster",		"ODE Physics Library");
	physics_ode_autodisable_steps				= cvarfuncs->GetNVFDG("physics_ode_autodisable_steps",				"10",	0,	"how many steps object should be dormant to be autodisabled",		"ODE Physics Library");
	physics_ode_autodisable_time				= cvarfuncs->GetNVFDG("physics_ode_autodisable_time",					"0",	0,	"how many seconds object should be dormant to be autodisabled",		"ODE Physics Library");
	physics_ode_autodisable_threshold_linear	= cvarfuncs->GetNVFDG("physics_ode_autodisable_threshold_linear",		"0.2",	0,	"body will be disabled if it's linear move below this value",		"ODE Physics Library");
	physics_ode_autodisable_threshold_angular	= cvarfuncs->GetNVFDG("physics_ode_autodisable_threshold_angular",	"0.3",	0,	"body will be disabled if it's angular move below this value",		"ODE Physics Library");
	physics_ode_autodisable_threshold_samples	= cvarfuncs->GetNVFDG("physics_ode_autodisable_threshold_samples",	"5",	0,	"average threshold with this number of samples",					"ODE Physics Library");

#ifdef ODE_DYNAMIC
	// Load the DLL
	ode_dll = plugfuncs->LoadDLL(dllname, odefuncs);
	if (ode_dll)
#endif
	{
		dInitODE();
//		dInitODE2(0);
#ifdef ODE_DYNAMIC
# ifdef dSINGLE
		if (!dCheckConfiguration("ODE_single_precision"))
# else
		if (!dCheckConfiguration("ODE_double_precision"))
# endif
		{
# ifdef dSINGLE
			Con_Printf("ode library not compiled for single precision - incompatible!  Not using ODE physics.\n");
# else
			Con_Printf("ode library not compiled for double precision - incompatible!  Not using ODE physics.\n");
# endif
			plugfuncs->CloseDLL(ode_dll);
			ode_dll = NULL;
		}
#endif
	}

#ifdef ODE_DYNAMIC
	if (!ode_dll)
	{
		Con_Printf("ODE plugin failed: \"%s\" library missing.\n", dllname);
		return false;
	}
#endif

	dSetErrorHandler(MyODEErrorHandler);	//ode will display a messagebox (which probably won't have focus/grabs) and then crash, which messes up all sorts of things like gamma.
	dSetDebugHandler(MyODEErrorHandler);	//both are fatal.
	dSetMessageHandler(MyODEMessageHandler);//merely a print.
	return true;
}

static void World_ODE_Shutdown(void)
{
#ifdef ODE_DYNAMIC
	if (ode_dll)
#endif
	{
		dCloseODE();
#ifdef ODE_DYNAMIC
		plugfuncs->CloseDLL(ode_dll);
		ode_dll = NULL;
#endif
	}
}

static void QDECL World_ODE_End(world_t *world)
{
	struct odectx_s *ctx = (struct odectx_s*)world->rbe;
	world->rbe = NULL;
	dWorldDestroy(ctx->dworld);
	dSpaceDestroy(ctx->space);
	dJointGroupDestroy(ctx->contactgroup);
	Z_Free(ctx);
}

static void QDECL World_ODE_RemoveJointFromEntity(world_t *world, wedict_t *ed)
{
	ed->rbe.joint_type = 0;
	if(ed->rbe.joint.joint)
		dJointDestroy((dJointID)ed->rbe.joint.joint);
	ed->rbe.joint.joint = NULL;
}

static void QDECL World_ODE_RemoveFromEntity(world_t *world, wedict_t *ed)
{
	if (!ed->rbe.physics)
		return;

	// entity is not physics controlled, free any physics data
	ed->rbe.physics = false;
	if (ed->rbe.body.geom)
		dGeomDestroy((dGeomID)ed->rbe.body.geom);
	ed->rbe.body.geom = NULL;
	if (ed->rbe.body.body)
	{
		dJointID j;
		dBodyID b1, b2;
		wedict_t *ed2;
		while(dBodyGetNumJoints((dBodyID)ed->rbe.body.body))
		{
			j = dBodyGetJoint((dBodyID)ed->rbe.body.body, 0);
			ed2 = (wedict_t *) dJointGetData(j);
			b1 = dJointGetBody(j, 0);
			b2 = dJointGetBody(j, 1);
			if(b1 == (dBodyID)ed->rbe.body.body)
			{
				b1 = 0;
				ed2->rbe.joint_enemy = 0;
			}
			if(b2 == (dBodyID)ed->rbe.body.body)
			{
				b2 = 0;
				ed2->rbe.joint_aiment = 0;
			}
			dJointAttach(j, b1, b2);
		}
		dBodyDestroy((dBodyID)ed->rbe.body.body);
	}
	ed->rbe.body.body = NULL;

	rbefuncs->ReleaseCollisionMesh(ed);
	if(ed->rbe.massbuf)
		BZ_Free(ed->rbe.massbuf);
	ed->rbe.massbuf = NULL;
}

static void World_ODE_Frame_BodyToEntity(world_t *world, wedict_t *ed)
{
	model_t *model;
	const dReal *avel;
	const dReal *o;
	const dReal *r; // for some reason dBodyGetRotation returns a [3][4] matrix
	const dReal *vel;
	dBodyID body = (dBodyID)ed->rbe.body.body;
	int movetype;
	float bodymatrix[16];
	float entitymatrix[16];
	vec3_t angles;
	vec3_t avelocity;
	vec3_t forward, left, up;
	vec3_t origin;
	vec3_t spinvelocity;
	vec3_t velocity;
	if (!body)
		return;

	movetype = (int)ed->v->movetype;
	if (movetype != MOVETYPE_PHYSICS)
	{
		switch((int)ed->xv->jointtype)
		{
			// TODO feed back data from physics
			case JOINTTYPE_POINT:
				break;
			case JOINTTYPE_HINGE:
				break;
			case JOINTTYPE_SLIDER:
				break;
			case JOINTTYPE_UNIVERSAL:
				break;
			case JOINTTYPE_HINGE2:
				break;
			case JOINTTYPE_FIXED:
				break;
		}
		return;
	}
	// store the physics engine data into the entity
	o = dBodyGetPosition(body);
	r = dBodyGetRotation(body);
	vel = dBodyGetLinearVel(body);
	avel = dBodyGetAngularVel(body);
	VectorCopy(o, origin);
	forward[0] = r[0];
	forward[1] = r[4];
	forward[2] = r[8];
	left[0] = r[1];
	left[1] = r[5];
	left[2] = r[9];
	up[0] = r[2];
	up[1] = r[6];
	up[2] = r[10];
	VectorCopy(vel, velocity);
	VectorCopy(avel, spinvelocity);
	Matrix4x4_RM_FromVectors(bodymatrix, forward, left, up, origin);
	Matrix4_Multiply(ed->rbe.offsetimatrix, bodymatrix, entitymatrix);
	Matrix3x4_RM_ToVectors(entitymatrix, forward, left, up, origin);

	VectorAngles(forward, up, angles, false);
	avelocity[PITCH] = RAD2DEG(spinvelocity[PITCH]);
	avelocity[YAW] = RAD2DEG(spinvelocity[ROLL]);
	avelocity[ROLL] = RAD2DEG(spinvelocity[YAW]);

	if (ed->v->modelindex)
	{
		model = world->Get_CModel(world, ed->v->modelindex);
		if (!model || model->type == mod_alias)
		{
			angles[PITCH] *= r_meshpitch.value;
			avelocity[PITCH] *= r_meshpitch.value;
		}
	}

	VectorCopy(origin, ed->v->origin);
	VectorCopy(velocity, ed->v->velocity);
	//vVectorCopy(forward, ed->xv->axis_forward);
	//VectorCopy(left, ed->xv->axis_left);
	//VectorCopy(up, ed->xv->axis_up);
	//VectorCopy(spinvelocity, ed->xv->spinvelocity);
	VectorCopy(angles, ed->v->angles);
	VectorCopy(avelocity, ed->v->avelocity);

	// values for BodyFromEntity to check if the qc modified anything later
	VectorCopy(origin, ed->rbe.origin);
	VectorCopy(velocity, ed->rbe.velocity);
	VectorCopy(angles, ed->rbe.angles);
	VectorCopy(avelocity, ed->rbe.avelocity);
	ed->rbe.gravity = dBodyGetGravityMode(body);

	rbefuncs->LinkEdict(world, ed, true);
}

static int ragGroups = 0;
static void World_ODE_Frame_JointFromEntity(world_t *world, wedict_t *ed)
{
	struct odectx_s *ctx = (struct odectx_s*)world->rbe;
	dJointID j = 0;
	dBodyID b1 = 0;
	dBodyID b2 = 0;
	dJointGroupID jgid = 0;
	int movetype = 0;
	int jointgroup = 0;
	int jointtype = 0;
	int enemy = 0, aiment = 0;
	wedict_t *o;
	vec3_t origin, velocity, angles, forward, left, up, movedir;
	vec_t CFM, ERP, FMax, Stop, Vel;
	VectorClear(origin);
	VectorClear(velocity);
	VectorClear(angles);
	VectorClear(movedir);
	movetype = (int)ed->v->movetype;
	jointtype = (int)ed->xv->jointtype;
	enemy = ed->v->enemy;
	aiment = ed->v->aiment;
	jointgroup = ed->xv->jointgroup;
	VectorCopy(ed->v->origin, origin);
	VectorCopy(ed->v->velocity, velocity);
	VectorCopy(ed->v->angles, angles);
	VectorCopy(ed->v->movedir, movedir);
	if(movetype == MOVETYPE_PHYSICS)
		jointtype = 0; // can't have both

	o = (wedict_t*)PROG_TO_EDICT(world->progs, enemy);
	if(ED_ISFREE(o) || o->rbe.body.body == 0)
		enemy = 0;
	o = (wedict_t*)PROG_TO_EDICT(world->progs, aiment);
	if(ED_ISFREE(o) || o->rbe.body.body == 0)
		aiment = 0;
	// see http://www.ode.org/old_list_archives/2006-January/017614.html
	// we want to set ERP? make it fps independent and work like a spring constant
	// note: if movedir[2] is 0, it becomes ERP = 1, CFM = 1.0 / (H * K)
	if(movedir[0] > 0 && movedir[1] > 0)
	{
		float K = movedir[0];
		float D = movedir[1];
		float R = 2.0 * D * sqrt(K); // we assume D is premultiplied by sqrt(sprungMass)
		CFM = 1.0 / (ctx->step * K + R); // always > 0
		ERP = ctx->step * K * CFM;
		Vel = 0;
		FMax = 0;
		Stop = movedir[2];
	}
	else if(movedir[1] < 0)
	{
		CFM = 0;
		ERP = 0;
		Vel = movedir[0];
		FMax = -movedir[1]; // TODO do we need to multiply with world.physics.ode_step?
		Stop = movedir[2] > 0 ? movedir[2] : dInfinity;
	}
	else // movedir[0] > 0, movedir[1] == 0 or movedir[0] < 0, movedir[1] >= 0
	{
		CFM = 0;
		ERP = 0;
		Vel = 0;
		FMax = 0;
		Stop = dInfinity;
	}
	if(jointtype == ed->rbe.joint_type && VectorCompare(origin, ed->rbe.joint_origin) && VectorCompare(velocity, ed->rbe.joint_velocity) && VectorCompare(angles, ed->rbe.joint_angles) && enemy == ed->rbe.joint_enemy && aiment == ed->rbe.joint_aiment && VectorCompare(movedir, ed->rbe.joint_movedir))
		return; // nothing to do

	/* we're part of a joint group */
	if (jointgroup > 0) {
		/* we're unaware of it, let's create it */
		if (jointgroup > ragGroups) {
			jgid = dJointGroupCreate(0);
			ragGroups = jointgroup;
		} else {
			jgid = (dJointGroupID)jointgroup;
		}

		ed->rbe.jointgroup = (int)jgid;
	}

	AngleVectorsFLU(angles, forward, left, up);
	switch(jointtype)
	{
		case JOINTTYPE_POINT:
			j = dJointCreateBall(ctx->dworld, jgid);
			break;
		case JOINTTYPE_HINGE:
			j = dJointCreateHinge(ctx->dworld, jgid);
			break;
		case JOINTTYPE_SLIDER:
			j = dJointCreateSlider(ctx->dworld, jgid);
			break;
		case JOINTTYPE_UNIVERSAL:
			j = dJointCreateUniversal(ctx->dworld, jgid);
			break;
		case JOINTTYPE_HINGE2:
			j = dJointCreateHinge2(ctx->dworld, jgid);
			break;
		case JOINTTYPE_FIXED:
			j = dJointCreateFixed(ctx->dworld, jgid);
			break;
		case 0:
		default:
			// no joint
			j = 0;
			break;
	}
	if(ed->rbe.joint.joint)
	{
		//Con_Printf("deleted old joint %i\n", (int) (ed - prog->edicts));
		dJointAttach(ed->rbe.joint.joint, 0, 0);
		dJointDestroy(ed->rbe.joint.joint);
	}
	ed->rbe.joint.joint = (void *) j;
	ed->rbe.joint_type = jointtype;
	ed->rbe.joint_enemy = enemy;
	ed->rbe.joint_aiment = aiment;
	VectorCopy(origin, ed->rbe.joint_origin);
	VectorCopy(velocity, ed->rbe.joint_velocity);
	VectorCopy(angles, ed->rbe.joint_angles);
	VectorCopy(movedir, ed->rbe.joint_movedir);
	if(j)
	{
		//Con_Printf("made new joint %i\n", (int) (ed - prog->edicts));
		dJointSetData(j, (void *) ed);
		if(enemy)
			b1 = (dBodyID)((WEDICT_NUM_UB(world->progs, enemy))->rbe.body.body);
		if(aiment)
			b2 = (dBodyID)((WEDICT_NUM_UB(world->progs, aiment))->rbe.body.body);
		dJointAttach(j, b1, b2);

		switch(jointtype)
		{
			case JOINTTYPE_POINT:
				dJointSetBallAnchor(j, origin[0], origin[1], origin[2]);
				dJointSetBallAnchor2(j, velocity[0], velocity[1], velocity[2]);
				break;
			case JOINTTYPE_HINGE:
				dJointSetHingeAnchor(j, origin[0], origin[1], origin[2]);
				dJointSetHingeAxis(j, forward[0], forward[1], forward[2]);
				dJointSetHingeParam(j, dParamFMax, FMax);
				dJointSetHingeParam(j, dParamHiStop, Stop);	
				dJointSetHingeParam(j, dParamLoStop, -Stop);
				dJointSetHingeParam(j, dParamStopCFM, CFM);
				dJointSetHingeParam(j, dParamStopERP, ERP);
				dJointSetHingeParam(j, dParamVel, Vel);
				break;
			case JOINTTYPE_SLIDER:
				dJointSetSliderAxis(j, forward[0], forward[1], forward[2]);
				dJointSetSliderParam(j, dParamFMax, FMax);
				dJointSetSliderParam(j, dParamHiStop, Stop);
				dJointSetSliderParam(j, dParamLoStop, -Stop);
				dJointSetSliderParam(j, dParamStopCFM, CFM);
				dJointSetSliderParam(j, dParamStopERP, ERP);
				dJointSetSliderParam(j, dParamVel, Vel);
				break;
			case JOINTTYPE_UNIVERSAL:
				dJointSetUniversalAnchor(j, origin[0], origin[1], origin[2]);
				dJointSetUniversalAxis1(j, forward[0], forward[1], forward[2]);
				dJointSetUniversalAxis2(j, up[0], up[1], up[2]);
				dJointSetUniversalParam(j, dParamFMax, FMax);
				dJointSetUniversalParam(j, dParamHiStop, Stop);
				dJointSetUniversalParam(j, dParamLoStop, -Stop);
				dJointSetUniversalParam(j, dParamStopCFM, CFM);
				dJointSetUniversalParam(j, dParamStopERP, ERP);
				dJointSetUniversalParam(j, dParamVel, Vel);
				dJointSetUniversalParam(j, dParamFMax2, FMax);
				dJointSetUniversalParam(j, dParamHiStop2, Stop);
				dJointSetUniversalParam(j, dParamLoStop2, -Stop);
				dJointSetUniversalParam(j, dParamStopCFM2, CFM);
				dJointSetUniversalParam(j, dParamStopERP2, ERP);
				dJointSetUniversalParam(j, dParamVel2, Vel);
				break;
			case JOINTTYPE_HINGE2:
				dJointSetHinge2Anchor(j, origin[0], origin[1], origin[2]);			
#if ODEVERSION>=MAKE2VER(0,16)
				{
					dReal a1[]={forward[0], forward[1], forward[2]}, a2[]={velocity[0], velocity[1], velocity[2]};
					dJointSetHinge2Axes(j, a1, a2);
				}
#else
				dJointSetHinge2Axis1(j, forward[0], forward[1], forward[2]);
				dJointSetHinge2Axis2(j, velocity[0], velocity[1], velocity[2]);
#endif

				dJointSetHinge2Param(j, dParamFMax, FMax);
				dJointSetHinge2Param(j, dParamHiStop, Stop);
				dJointSetHinge2Param(j, dParamLoStop, -Stop);
				dJointSetHinge2Param(j, dParamStopCFM, CFM);
				dJointSetHinge2Param(j, dParamStopERP, ERP);
				dJointSetHinge2Param(j, dParamVel, Vel);
				dJointSetHinge2Param(j, dParamFMax2, FMax);
				dJointSetHinge2Param(j, dParamHiStop2, Stop);
				dJointSetHinge2Param(j, dParamLoStop2, -Stop);
				dJointSetHinge2Param(j, dParamStopCFM2, CFM);
				dJointSetHinge2Param(j, dParamStopERP2, ERP);
				dJointSetHinge2Param(j, dParamVel2, Vel);
				break;
			case JOINTTYPE_FIXED:
				dJointSetFixed(j);
				break;
			case 0:
			default:
				break;
		}
	}
}

static qboolean QDECL World_ODE_RagMatrixToBody(rbebody_t *bodyptr, float *mat)
{
	dVector3 r[3];

	r[0][0] = mat[0];
	r[0][1] = mat[1];
	r[0][2] = mat[2];
	r[1][0] = mat[4];
	r[1][1] = mat[5];
	r[1][2] = mat[6];
	r[2][0] = mat[8];
	r[2][1] = mat[9];
	r[2][2] = mat[10];

	dBodySetPosition(bodyptr->body, mat[3], mat[7], mat[11]);
	dBodySetRotation(bodyptr->body, r[0]);
	dBodySetLinearVel(bodyptr->body, 0, 0, 0);
	dBodySetAngularVel(bodyptr->body, 0, 0, 0);

	return true;
}
static qboolean QDECL World_ODE_RagCreateBody(world_t *world, rbebody_t *bodyptr, rbebodyinfo_t *bodyinfo, float *mat, wedict_t *ent)
{
	struct odectx_s *ctx = (struct odectx_s*)world->rbe;
	dMass mass;
	float radius;
	ctx->hasextraobjs = true;
	
	switch(bodyinfo->geomshape)
	{
	case GEOMTYPE_CAPSULE:
		radius = (bodyinfo->dimensions[0] + bodyinfo->dimensions[1]) * 0.5;
		bodyptr->geom = (void *)dCreateCapsule(ctx->space, radius, bodyinfo->dimensions[2]);
		dMassSetCapsuleTotal(&mass, bodyinfo->mass, 3, radius, bodyinfo->dimensions[2]);
		//aligned along the geom's local z axis
		break;
	case GEOMTYPE_SPHERE:
		//radius
		radius = (bodyinfo->dimensions[0] + bodyinfo->dimensions[1] + bodyinfo->dimensions[2]) / 3;
		bodyptr->geom = dCreateSphere(ctx->space, radius);
		dMassSetSphereTotal(&mass, bodyinfo->mass, radius);
		//aligned along the geom's local z axis
		break;
	case GEOMTYPE_CYLINDER:
		//radius, length
		radius = (bodyinfo->dimensions[0] + bodyinfo->dimensions[1]) * 0.5;
		bodyptr->geom = dCreateCylinder(ctx->space, radius, bodyinfo->dimensions[2]);
		dMassSetCylinderTotal(&mass, bodyinfo->mass, 3, radius, bodyinfo->dimensions[2]);
		//alignment is irreleevnt, thouse I suppose it might be scaled wierdly.
		break;
	default:
	case GEOMTYPE_BOX:
		//diameter
		bodyptr->geom = dCreateBox(ctx->space, bodyinfo->dimensions[0], bodyinfo->dimensions[1], bodyinfo->dimensions[2]);
		dMassSetBoxTotal(&mass, bodyinfo->mass, bodyinfo->dimensions[0], bodyinfo->dimensions[1], bodyinfo->dimensions[2]);
		//monkey
		break;
	}
	bodyptr->body = dBodyCreate(ctx->dworld);
	dBodySetMass(bodyptr->body, &mass);
	dGeomSetBody(bodyptr->geom, bodyptr->body);
	dGeomSetData(bodyptr->geom, (void*)ent);

	return World_ODE_RagMatrixToBody(bodyptr, mat);
}

static void QDECL World_ODE_RagMatrixFromJoint(rbejoint_t *joint, rbejointinfo_t *info, float *mat)
{
	dVector3 dr3;
	switch(info->type)
	{
	case JOINTTYPE_POINT:
		dJointGetBallAnchor(joint->joint, dr3);
		mat[3] = dr3[0];
		mat[7] = dr3[1];
		mat[11] = dr3[2];
		VectorClear(mat+4);
		VectorClear(mat+8);
		break;

	case JOINTTYPE_HINGE:
		dJointGetHingeAnchor(joint->joint, dr3);
		mat[3] = dr3[0];
		mat[7] = dr3[1];
		mat[11] = dr3[2];

		dJointGetHingeAxis(joint->joint, dr3);
		VectorCopy(dr3, mat+4);
		VectorClear(mat+8);

		CrossProduct(mat+4, mat+8, mat+0);
		return;
		break;
	case JOINTTYPE_HINGE2:
		dJointGetHinge2Anchor(joint->joint, dr3);
		mat[3] = dr3[0];
		mat[7] = dr3[1];
		mat[11] = dr3[2];

		dJointGetHinge2Axis1(joint->joint, dr3);
		VectorCopy(dr3, mat+4);
		dJointGetHinge2Axis2(joint->joint, dr3);
		VectorCopy(dr3, mat+8);
		break;

	case JOINTTYPE_SLIDER:
		//no anchor point...
		//get the two bodies and average their origin for a somewhat usable representation of an anchor.
		{
			const dReal *p1, *p2;
			dReal n[3];
			dBodyID b1 = dJointGetBody(joint->joint, 0), b2 = dJointGetBody(joint->joint, 1);
			if (b1)
				p1 = dBodyGetPosition(b1);
			else
			{
				p1 = n;
				VectorClear(n);
			}
			if (b2)
				p2 = dBodyGetPosition(b2);
			else
				p2 = p1;
			dJointGetSliderAxis(joint->joint, dr3 + 0);
			VectorInterpolate(p1, 0.5, p2, dr3);
			mat[3] = dr3[0];
			mat[7] = dr3[1];
			mat[11] = dr3[2];

			VectorClear(mat+4);
			VectorClear(mat+8);
		}
		break;

	case JOINTTYPE_UNIVERSAL:
		dJointGetUniversalAnchor(joint->joint, dr3);
		mat[3] = dr3[0];
		mat[7] = dr3[1];
		mat[11] = dr3[2];

		dJointGetUniversalAxis1(joint->joint, dr3);
		VectorCopy(dr3, mat+4);
		dJointGetUniversalAxis2(joint->joint, dr3);
		VectorCopy(dr3, mat+8);

		CrossProduct(mat+4, mat+8, mat+0);
		return;
		break;
	}
	AngleVectorsFLU(vec3_origin, mat+0, mat+4, mat+8);
}

static void QDECL World_ODE_RagMatrixFromBody(world_t *world, rbebody_t *bodyptr, float *mat)
{
	const dReal *o = dBodyGetPosition(bodyptr->body);
	const dReal *r = dBodyGetRotation(bodyptr->body);
	mat[0] = r[0];
	mat[1] = r[1];
	mat[2] = r[2];
	mat[3] = o[0];

	mat[4] = r[4];
	mat[5] = r[5];
	mat[6] = r[6];
	mat[7] = o[1];

	mat[8] = r[8];
	mat[9] = r[9];
	mat[10] = r[10];
	mat[11] = o[2];
}
static void QDECL World_ODE_RagEnableJoint(rbejoint_t *joint, qboolean enabled)
{
	if (enabled)
		dJointEnable(joint->joint);
	else
		dJointDisable(joint->joint);
}
static void QDECL World_ODE_RagCreateJoint(world_t *world, rbejoint_t *joint, rbejointinfo_t *info, rbebody_t *body1, rbebody_t *body2, vec3_t aaa2[3])
{
	struct odectx_s *ctx = (struct odectx_s*)world->rbe;
	switch(info->type)
	{
		case JOINTTYPE_POINT:
			joint->joint = dJointCreateBall(ctx->dworld, 0);
			break;
		case JOINTTYPE_HINGE:
			joint->joint = dJointCreateHinge(ctx->dworld, 0);
			break;
		case JOINTTYPE_SLIDER:
			joint->joint = dJointCreateSlider(ctx->dworld, 0);
			break;
		case JOINTTYPE_UNIVERSAL:
			joint->joint = dJointCreateUniversal(ctx->dworld, 0);
			break;
		case JOINTTYPE_HINGE2:
			joint->joint = dJointCreateHinge2(ctx->dworld, 0);
			break;
		case JOINTTYPE_FIXED:
			joint->joint = dJointCreateFixed(ctx->dworld, 0);
			break;
		default:
			joint->joint = NULL;
			break;
	}
	if (joint->joint)
	{
		//Con_Printf("made new joint %i\n", (int) (ed - prog->edicts));
//		dJointSetData(joint->ode_joint, NULL);
		dJointAttach(joint->joint, body1?body1->body:NULL, body2?body2->body:NULL);

		switch(info->type)
		{
			case JOINTTYPE_POINT:
				dJointSetBallAnchor(joint->joint, aaa2[0][0], aaa2[0][1], aaa2[0][2]);
				break;
			case JOINTTYPE_HINGE:
				dJointSetHingeAnchor(joint->joint, aaa2[0][0], aaa2[0][1], aaa2[0][2]);
				dJointSetHingeAxis(joint->joint, aaa2[1][0], aaa2[1][1], aaa2[1][2]);
				dJointSetHingeParam(joint->joint, dParamFMax, info->FMax);
				dJointSetHingeParam(joint->joint, dParamHiStop, info->HiStop);	
				dJointSetHingeParam(joint->joint, dParamLoStop, info->LoStop);
				dJointSetHingeParam(joint->joint, dParamStopCFM, info->CFM);
				dJointSetHingeParam(joint->joint, dParamStopERP, info->ERP);
				dJointSetHingeParam(joint->joint, dParamVel, info->Vel);
				break;
			case JOINTTYPE_SLIDER:
				dJointSetSliderAxis(joint->joint, aaa2[1][0], aaa2[1][1], aaa2[1][2]);
				dJointSetSliderParam(joint->joint, dParamFMax, info->FMax);
				dJointSetSliderParam(joint->joint, dParamHiStop, info->HiStop);
				dJointSetSliderParam(joint->joint, dParamLoStop, info->LoStop);
				dJointSetSliderParam(joint->joint, dParamStopCFM, info->CFM);
				dJointSetSliderParam(joint->joint, dParamStopERP, info->ERP);
				dJointSetSliderParam(joint->joint, dParamVel, info->Vel);
				break;
			case JOINTTYPE_UNIVERSAL:
				dJointSetUniversalAnchor(joint->joint, aaa2[0][0], aaa2[0][1], aaa2[0][2]);
				dJointSetUniversalAxis1(joint->joint, aaa2[1][0], aaa2[1][1], aaa2[1][2]);
				dJointSetUniversalAxis2(joint->joint, aaa2[2][0], aaa2[2][1], aaa2[2][2]);
				dJointSetUniversalParam(joint->joint, dParamFMax, info->FMax);
				dJointSetUniversalParam(joint->joint, dParamHiStop, info->HiStop);
				dJointSetUniversalParam(joint->joint, dParamLoStop, info->LoStop);
				dJointSetUniversalParam(joint->joint, dParamStopCFM, info->CFM);
				dJointSetUniversalParam(joint->joint, dParamStopERP, info->ERP);
				dJointSetUniversalParam(joint->joint, dParamVel, info->Vel);
				dJointSetUniversalParam(joint->joint, dParamFMax2, info->FMax2);
				dJointSetUniversalParam(joint->joint, dParamHiStop2, info->HiStop2);
				dJointSetUniversalParam(joint->joint, dParamLoStop2, info->LoStop2);
				dJointSetUniversalParam(joint->joint, dParamStopCFM2, info->CFM2);
				dJointSetUniversalParam(joint->joint, dParamStopERP2, info->ERP2);
				dJointSetUniversalParam(joint->joint, dParamVel2, info->Vel2);
				break;
			case JOINTTYPE_HINGE2:
				dJointSetHinge2Anchor(joint->joint, aaa2[0][0], aaa2[0][1], aaa2[0][2]);
#if ODEVERSION>=MAKE2VER(0,16)
				{
					dReal a1[]={aaa2[1][0], aaa2[1][1], aaa2[1][2]}, a2[]={aaa2[2][0], aaa2[2][1], aaa2[2][2]};
					dJointSetHinge2Axes(joint->joint, a1, a2);
				}
#else
				dJointSetHinge2Axis1(joint->joint, aaa2[1][0], aaa2[1][1], aaa2[1][2]);
				dJointSetHinge2Axis2(joint->joint, aaa2[2][0], aaa2[2][1], aaa2[2][2]);
#endif
				dJointSetHinge2Param(joint->joint, dParamFMax, info->FMax);
				dJointSetHinge2Param(joint->joint, dParamHiStop, info->HiStop);
				dJointSetHinge2Param(joint->joint, dParamLoStop, info->LoStop);
				dJointSetHinge2Param(joint->joint, dParamStopCFM, info->CFM);
				dJointSetHinge2Param(joint->joint, dParamStopERP, info->ERP);
				dJointSetHinge2Param(joint->joint, dParamVel, info->Vel);
				dJointSetHinge2Param(joint->joint, dParamFMax2, info->FMax2);
				dJointSetHinge2Param(joint->joint, dParamHiStop2, info->HiStop2);
				dJointSetHinge2Param(joint->joint, dParamLoStop2, info->LoStop2);
				dJointSetHinge2Param(joint->joint, dParamStopCFM2, info->CFM2);
				dJointSetHinge2Param(joint->joint, dParamStopERP2, info->ERP2);
				dJointSetHinge2Param(joint->joint, dParamVel2, info->Vel2);
				break;
			case JOINTTYPE_FIXED:
				dJointSetFixed(joint->joint);
				break;
		}
	}
}

static void QDECL World_ODE_RagDestroyBody(world_t *world, rbebody_t *bodyptr)
{
	if (bodyptr->geom)
		dGeomDestroy(bodyptr->geom);
	bodyptr->geom = NULL;
	if (bodyptr->body)
		dBodyDestroy(bodyptr->body);
	bodyptr->body = NULL;
}

static void QDECL World_ODE_RagDestroyJoint(world_t *world, rbejoint_t *joint)
{
	if (joint->joint)
		dJointDestroy(joint->joint);
	joint->joint = NULL;
}

static void World_ODE_Frame_BodyFromEntity(world_t *world, wedict_t *ed)
{
	struct odectx_s *ctx = (struct odectx_s*)world->rbe;
	dBodyID body = (dBodyID)ed->rbe.body.body;
	dMass mass;
	float test;
	void *dataID;
	model_t *model;
	int axisindex;
	int modelindex = 0;
	int movetype = MOVETYPE_NONE;
	int solid = SOLID_NOT;
	int geomtype = GEOMTYPE_SOLID;
	qboolean modified = false;
	vec3_t angles;
	vec3_t avelocity;
	vec3_t entmaxs;
	vec3_t entmins;
	vec3_t forward;
	vec3_t geomcenter;
	vec3_t geomsize;
	vec3_t left;
	vec3_t origin;
	vec3_t spinvelocity;
	vec3_t up;
	vec3_t velocity;
	vec_t f;
	vec_t length;
	vec_t massval = 1.0f;
	dReal dampLinear;
	dReal dampAngular;
	dReal maxAngularSpeed;
	vec_t movelimit;
	vec_t radius;
	vec_t scale;
	vec_t spinlimit;
	qboolean gravity;
#ifdef ODE_DYNAMIC
	if (!ode_dll)
		return;
#endif
	geomtype = (int)ed->xv->geomtype;
	solid = (int)ed->v->solid;
	movetype = (int)ed->v->movetype;
	scale = ed->xv->scale?ed->xv->scale:1;
	dampLinear = (ed->xv->damp_linear >= 0.0f) ? ed->xv->damp_linear : 0.0f;
	dampAngular = (ed->xv->damp_angular >= 0.0f) ? ed->xv->damp_angular : 0.0f;
	maxAngularSpeed = (ed->xv->max_angular > 0.0f) ? ed->xv->max_angular : dInfinity;
	modelindex = 0;
	model = NULL;

	if (!geomtype)
	{
		switch((int)ed->v->solid)
		{
		case SOLID_NOT:				geomtype = GEOMTYPE_NONE;		break;
		case SOLID_TRIGGER:			geomtype = GEOMTYPE_NONE;		break;
		case SOLID_BSP:				geomtype = GEOMTYPE_TRIMESH;	break;
		case SOLID_PHYSICS_TRIMESH:	geomtype = GEOMTYPE_TRIMESH;	break;
		case SOLID_PHYSICS_BOX:		geomtype = GEOMTYPE_BOX;		break;
		case SOLID_PHYSICS_SPHERE:	geomtype = GEOMTYPE_SPHERE;		break;
		case SOLID_PHYSICS_CAPSULE:	geomtype = GEOMTYPE_CAPSULE;	break;
		case SOLID_PHYSICS_CYLINDER:geomtype = GEOMTYPE_CYLINDER;	break;
		default:					geomtype = GEOMTYPE_BOX;		break;
		}
	}

	switch(geomtype)
	{
	case GEOMTYPE_TRIMESH:
		modelindex = (int)ed->v->modelindex;
		model = world->Get_CModel(world, modelindex);
		if (model)
		{
			VectorScale(model->mins, scale, entmins);
			VectorScale(model->maxs, scale, entmaxs);
			if (ed->xv->mass)
				massval = ed->xv->mass;
		}
		else
		{
			VectorClear(entmins);
			VectorClear(entmaxs);
			modelindex = 0;
			massval = 1.0f;
		}
		break;
	case GEOMTYPE_BOX:
	case GEOMTYPE_SPHERE:
	case GEOMTYPE_CAPSULE:
	case GEOMTYPE_CAPSULE_X:
	case GEOMTYPE_CAPSULE_Y:
	case GEOMTYPE_CAPSULE_Z:
	case GEOMTYPE_CYLINDER:
	case GEOMTYPE_CYLINDER_X:
	case GEOMTYPE_CYLINDER_Y:
	case GEOMTYPE_CYLINDER_Z:
		VectorCopy(ed->v->mins, entmins);
		VectorCopy(ed->v->maxs, entmaxs);
		if (ed->xv->mass)
			massval = ed->xv->mass;
		break;
	default:
//	case GEOMTYPE_NONE:
		if (ed->rbe.physics)
			World_ODE_RemoveFromEntity(world, ed);
		return;
	}

	VectorSubtract(entmaxs, entmins, geomsize);
	if (DotProduct(geomsize,geomsize) == 0)
	{
		// we don't allow point-size physics objects...
		if (ed->rbe.physics)
			World_ODE_RemoveFromEntity(world, ed);
		return;
	}

	if (movetype != MOVETYPE_PHYSICS)
		massval = 1.0f;

	// check if we need to create or replace the geom
	if (!ed->rbe.physics
	 || !VectorCompare(ed->rbe.mins, entmins)
	 || !VectorCompare(ed->rbe.maxs, entmaxs)
	 || ed->rbe.mass != massval
	 || ed->rbe.modelindex != modelindex)
	{
		modified = true;
		World_ODE_RemoveFromEntity(world, ed);
		ed->rbe.physics = true;
		VectorCopy(entmins, ed->rbe.mins);
		VectorCopy(entmaxs, ed->rbe.maxs);
		ed->rbe.mass = massval;
		ed->rbe.modelindex = modelindex;
		VectorAvg(entmins, entmaxs, geomcenter);
		ed->rbe.movelimit = min(geomsize[0], min(geomsize[1], geomsize[2]));

		if (massval * geomsize[0] * geomsize[1] * geomsize[2] == 0)
		{
			if (movetype == MOVETYPE_PHYSICS)
				Con_Printf("entity %i (classname %s) .mass * .size_x * .size_y * .size_z == 0\n", NUM_FOR_EDICT(world->progs, (edict_t*)ed), PR_GetString(world->progs, ed->v->classname));
			massval = 1.0f;
			VectorSet(geomsize, 1.0f, 1.0f, 1.0f);
		}

		switch(geomtype)
		{
		case GEOMTYPE_TRIMESH:
			Matrix4x4_Identity(ed->rbe.offsetmatrix);
			ed->rbe.body.geom = NULL;
			if (!model)
			{
				Con_Printf("entity %i (classname %s) has no model\n", NUM_FOR_EDICT(world->progs, (edict_t*)ed), PR_GetString(world->progs, ed->v->classname));
				if (ed->rbe.physics)
					World_ODE_RemoveFromEntity(world, ed);
				return;
			}
			if (!rbefuncs->GenerateCollisionMesh(world, model, ed, geomcenter))
			{
				if (ed->rbe.physics)
					World_ODE_RemoveFromEntity(world, ed);
				return;
			}

			Matrix4x4_RM_CreateTranslate(ed->rbe.offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2]);
			// now create the geom
			dataID = dGeomTriMeshDataCreate();
			dGeomTriMeshDataBuildSingle(dataID, (void*)ed->rbe.vertex3f, sizeof(float[3]), ed->rbe.numvertices, ed->rbe.element3i, ed->rbe.numtriangles*3, sizeof(int[3]));
			ed->rbe.body.geom = (void *)dCreateTriMesh(ctx->space, dataID, NULL, NULL, NULL);
			dMassSetBoxTotal(&mass, massval, geomsize[0], geomsize[1], geomsize[2]);
			break;
		case GEOMTYPE_BOX:
			Matrix4x4_RM_CreateTranslate(ed->rbe.offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2]);
			ed->rbe.body.geom = (void *)dCreateBox(ctx->space, geomsize[0], geomsize[1], geomsize[2]);
			dMassSetBoxTotal(&mass, massval, geomsize[0], geomsize[1], geomsize[2]);
			break;
		case GEOMTYPE_SPHERE:
			Matrix4x4_RM_CreateTranslate(ed->rbe.offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2]);
			ed->rbe.body.geom = (void *)dCreateSphere(ctx->space, geomsize[0] * 0.5f);
			dMassSetSphereTotal(&mass, massval, geomsize[0] * 0.5f);
			break;
		case GEOMTYPE_CAPSULE:
		case GEOMTYPE_CAPSULE_X:
		case GEOMTYPE_CAPSULE_Y:
		case GEOMTYPE_CAPSULE_Z:
			if (geomtype == GEOMTYPE_CAPSULE)
			{
				axisindex = 0;
				if (geomsize[axisindex] < geomsize[1])
					axisindex = 1;
				if (geomsize[axisindex] < geomsize[2])
					axisindex = 2;
			}
			else
				axisindex = geomtype-GEOMTYPE_CAPSULE_X;
			// the qc gives us 3 axis radius, the longest axis is the capsule
			// axis, since ODE doesn't like this idea we have to create a
			// capsule which uses the standard orientation, and apply a
			// transform to it
			if (axisindex == 0)
			{
				Matrix4x4_CM_ModelMatrix(ed->rbe.offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 90, 1);
				radius = min(geomsize[1], geomsize[2]) * 0.5f;
			}
			else if (axisindex == 1)
			{
				Matrix4x4_CM_ModelMatrix(ed->rbe.offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 90, 0, 0, 1);
				radius = min(geomsize[0], geomsize[2]) * 0.5f;
			}
			else
			{
				Matrix4x4_CM_ModelMatrix(ed->rbe.offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 0, 1);
				radius = min(geomsize[0], geomsize[1]) * 0.5f;
			}
			length = geomsize[axisindex] - radius*2;
			if (length <= 0)
			{
				radius -= (1 - length)*0.5;
				length = 1;
			}
			// because we want to support more than one axisindex, we have to
			// create a transform, and turn on its cleanup setting (which will
			// cause the child to be destroyed when it is destroyed)
			ed->rbe.body.geom = (void *)dCreateCapsule(ctx->space, radius, length);
			dMassSetCapsuleTotal(&mass, massval, axisindex+1, radius, length);
			break;
		case GEOMTYPE_CYLINDER:
		case GEOMTYPE_CYLINDER_X:
		case GEOMTYPE_CYLINDER_Y:
		case GEOMTYPE_CYLINDER_Z:
			if (geomtype == GEOMTYPE_CYLINDER)
			{
				axisindex = 0;
				if (geomsize[axisindex] < geomsize[1])
					axisindex = 1;
				if (geomsize[axisindex] < geomsize[2])
					axisindex = 2;
			}
			else
				axisindex = geomtype-GEOMTYPE_CYLINDER_X;
			// the qc gives us 3 axis radius, the longest axis is the capsule
			// axis, since ODE doesn't like this idea we have to create a
			// capsule which uses the standard orientation, and apply a
			// transform to it
			if (axisindex == 0)
			{
				Matrix4x4_CM_ModelMatrix(ed->rbe.offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 90, 1);
				radius = min(geomsize[1], geomsize[2]) * 0.5f;
			}
			else if (axisindex == 1)
			{
				Matrix4x4_CM_ModelMatrix(ed->rbe.offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 90, 0, 0, 1);
				radius = min(geomsize[0], geomsize[2]) * 0.5f;
			}
			else
			{
				Matrix4x4_CM_ModelMatrix(ed->rbe.offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 0, 1);
				radius = min(geomsize[0], geomsize[1]) * 0.5f;
			}
			length = geomsize[axisindex] - radius*2;
			if (length <= 0)
			{
				radius -= (1 - length)*0.5;
				length = 1;
			}
			// because we want to support more than one axisindex, we have to
			// create a transform, and turn on its cleanup setting (which will
			// cause the child to be destroyed when it is destroyed)
			ed->rbe.body.geom = (void *)dCreateCylinder(ctx->space, radius, length);
			dMassSetCylinderTotal(&mass, massval, axisindex+1, radius, length);
			break;
		default:
			Sys_Errorf("World_ODE_BodyFromEntity: unrecognized solid value %i was accepted by filter\n", solid);
		}
		Matrix3x4_InvertTo4x4_Simple(ed->rbe.offsetmatrix, ed->rbe.offsetimatrix);
		ed->rbe.massbuf = BZ_Malloc(sizeof(dMass));
		memcpy(ed->rbe.massbuf, &mass, sizeof(dMass));
	}

	if(ed->rbe.body.geom)
		dGeomSetData(ed->rbe.body.geom, (void*)ed);
	if (movetype == MOVETYPE_PHYSICS && ed->rbe.body.geom)
	{
		if (ed->rbe.body.body == NULL)
		{
			ed->rbe.body.body = (void *)(body = dBodyCreate(ctx->dworld));
			dGeomSetBody(ed->rbe.body.geom, body);
			dBodySetData(body, (void*)ed);
			dBodySetMass(body, (dMass *) ed->rbe.massbuf);
			modified = true;
		}
	}
	else
	{
		if (ed->rbe.body.body != NULL)
		{
			if(ed->rbe.body.geom)
				dGeomSetBody(ed->rbe.body.geom, 0);
			dBodyDestroy((dBodyID) ed->rbe.body.body);
			ed->rbe.body.body = NULL;
			modified = true;
		}
	}

	// get current data from entity
	gravity = true;
	VectorCopy(ed->v->origin, origin);
	VectorCopy(ed->v->velocity, velocity);
	//val = PRVM_EDICTFIELDVALUE(ed, prog->fieldoffsets.axis_forward);if (val) VectorCopy(val->vector, forward); else VectorClear(forward);
	//val = PRVM_EDICTFIELDVALUE(ed, prog->fieldoffsets.axis_left);if (val) VectorCopy(val->vector, left); else VectorClear(left);
	//val = PRVM_EDICTFIELDVALUE(ed, prog->fieldoffsets.axis_up);if (val) VectorCopy(val->vector, up); else VectorClear(up);
	//val = PRVM_EDICTFIELDVALUE(ed, prog->fieldoffsets.spinvelocity);if (val) VectorCopy(val->vector, spinvelocity); else VectorClear(spinvelocity);
	VectorCopy(ed->v->angles, angles);
	VectorCopy(ed->v->avelocity, avelocity);
	if (ed == world->edicts || (ed->xv->gravity && ed->xv->gravity <= 0.01))
		gravity = false;

	// compatibility for legacy entities
//	if (!DotProduct(forward,forward) || solid == SOLID_BSP)
	{
		vec3_t qangles, qavelocity;
		VectorCopy(angles, qangles);
		VectorCopy(avelocity, qavelocity);
	
		if (ed->v->modelindex)
		{
			model = world->Get_CModel(world, ed->v->modelindex);
			if (!model || model->type == mod_alias)
			{
				qangles[PITCH] *= r_meshpitch.value;
				qavelocity[PITCH] *= r_meshpitch.value;
			}
		}

		AngleVectorsFLU(qangles, forward, left, up);
		// convert single-axis rotations in avelocity to spinvelocity
		// FIXME: untested math - check signs
		VectorSet(spinvelocity, DEG2RAD(qavelocity[PITCH]), DEG2RAD(qavelocity[ROLL]), DEG2RAD(qavelocity[YAW]));
	}

	// compatibility for legacy entities
	switch (solid)
	{
	case SOLID_BBOX:
	case SOLID_SLIDEBOX:
	case SOLID_CORPSE:
		VectorSet(forward, 1, 0, 0);
		VectorSet(left, 0, 1, 0);
		VectorSet(up, 0, 0, 1);
		VectorSet(spinvelocity, 0, 0, 0);
		break;
	}


	// we must prevent NANs...
	test = DotProduct(origin,origin) + DotProduct(forward,forward) + DotProduct(left,left) + DotProduct(up,up) + DotProduct(velocity,velocity) + DotProduct(spinvelocity,spinvelocity);
	if (IS_NAN(test))
	{
		modified = true;
		//Con_Printf("Fixing NAN values on entity %i : .classname = \"%s\" .origin = '%f %f %f' .velocity = '%f %f %f' .axis_forward = '%f %f %f' .axis_left = '%f %f %f' .axis_up = %f %f %f' .spinvelocity = '%f %f %f'\n", PRVM_NUM_FOR_EDICT(ed), PRVM_GetString(PRVM_EDICTFIELDVALUE(ed, prog->fieldoffsets.classname)->string), origin[0], origin[1], origin[2], velocity[0], velocity[1], velocity[2], forward[0], forward[1], forward[2], left[0], left[1], left[2], up[0], up[1], up[2], spinvelocity[0], spinvelocity[1], spinvelocity[2]);
		Con_Printf("Fixing NAN values on entity %i : .classname = \"%s\" .origin = '%f %f %f' .velocity = '%f %f %f' .angles = '%f %f %f' .avelocity = '%f %f %f'\n", NUM_FOR_EDICT(world->progs, (edict_t*)ed), PR_GetString(world->progs, ed->v->classname), origin[0], origin[1], origin[2], velocity[0], velocity[1], velocity[2], angles[0], angles[1], angles[2], avelocity[0], avelocity[1], avelocity[2]);
		test = DotProduct(origin,origin);
		if (IS_NAN(test))
			VectorClear(origin);
		test = DotProduct(forward,forward) * DotProduct(left,left) * DotProduct(up,up);
		if (IS_NAN(test))
		{
			VectorSet(angles, 0, 0, 0);
			VectorSet(forward, 1, 0, 0);
			VectorSet(left, 0, 1, 0);
			VectorSet(up, 0, 0, 1);
		}
		test = DotProduct(velocity,velocity);
		if (IS_NAN(test))
			VectorClear(velocity);
		test = DotProduct(spinvelocity,spinvelocity);
		if (IS_NAN(test))
		{
			VectorClear(avelocity);
			VectorClear(spinvelocity);
		}
	}

	// check if the qc edited any position data
	if (!VectorCompare(origin, ed->rbe.origin)
	 || !VectorCompare(velocity, ed->rbe.velocity)
	 || !VectorCompare(angles, ed->rbe.angles)
	 || !VectorCompare(avelocity, ed->rbe.avelocity)
	 || gravity != ed->rbe.gravity)
		modified = true;

	// store the qc values into the physics engine
	body = ed->rbe.body.body;
	if (modified && ed->rbe.body.geom)
	{
		dVector3 r[3];
		float entitymatrix[16];
		float bodymatrix[16];

#if 0
		Con_Printf("entity %i got changed by QC\n", (int) (ed - prog->edicts));
		if(!VectorCompare(origin, ed->rbe.origin))
			Con_Printf("  origin: %f %f %f -> %f %f %f\n", ed->rbe.origin[0], ed->rbe.origin[1], ed->rbe.origin[2], origin[0], origin[1], origin[2]);
		if(!VectorCompare(velocity, ed->rbe.velocity))
			Con_Printf("  velocity: %f %f %f -> %f %f %f\n", ed->rbe.velocity[0], ed->rbe.velocity[1], ed->rbe.velocity[2], velocity[0], velocity[1], velocity[2]);
		if(!VectorCompare(angles, ed->rbe.angles))
			Con_Printf("  angles: %f %f %f -> %f %f %f\n", ed->rbe.angles[0], ed->rbe.angles[1], ed->rbe.angles[2], angles[0], angles[1], angles[2]);
		if(!VectorCompare(avelocity, ed->rbe.avelocity))
			Con_Printf("  avelocity: %f %f %f -> %f %f %f\n", ed->rbe.avelocity[0], ed->rbe.avelocity[1], ed->rbe.avelocity[2], avelocity[0], avelocity[1], avelocity[2]);
		if(gravity != ed->rbe.gravity)
			Con_Printf("  gravity: %i -> %i\n", ed->ide.ode_gravity, gravity);
#endif

		// values for BodyFromEntity to check if the qc modified anything later
		VectorCopy(origin, ed->rbe.origin);
		VectorCopy(velocity, ed->rbe.velocity);
		VectorCopy(angles, ed->rbe.angles);
		VectorCopy(avelocity, ed->rbe.avelocity);
		ed->rbe.gravity = gravity;

		Matrix4x4_RM_FromVectors(entitymatrix, forward, left, up, origin);
		Matrix4_Multiply(ed->rbe.offsetmatrix, entitymatrix, bodymatrix);
		Matrix3x4_RM_ToVectors(bodymatrix, forward, left, up, origin);
		r[0][0] = forward[0];
		r[1][0] = forward[1];
		r[2][0] = forward[2];
		r[0][1] = left[0];
		r[1][1] = left[1];
		r[2][1] = left[2];
		r[0][2] = up[0];
		r[1][2] = up[1];
		r[2][2] = up[2];
		if(body)
		{
			if(movetype == MOVETYPE_PHYSICS)
			{
				dGeomSetBody(ed->rbe.body.geom, body);
				dBodySetPosition(body, origin[0], origin[1], origin[2]);
				dBodySetRotation(body, r[0]);
				dBodySetLinearVel(body, velocity[0], velocity[1], velocity[2]);
				dBodySetAngularVel(body, spinvelocity[0], spinvelocity[1], spinvelocity[2]);
				dBodySetGravityMode(body, gravity);
				dBodySetLinearDamping(body, dampLinear);
				dBodySetAngularDamping(body, dampAngular);
				dBodySetMaxAngularSpeed(body, maxAngularSpeed);
			}
			else
			{
				dGeomSetBody(ed->rbe.body.geom, body);
				dBodySetPosition(body, origin[0], origin[1], origin[2]);
				dBodySetRotation(body, r[0]);
				dBodySetLinearVel(body, velocity[0], velocity[1], velocity[2]);
				dBodySetAngularVel(body, spinvelocity[0], spinvelocity[1], spinvelocity[2]);
				dBodySetGravityMode(body, gravity);
				dBodySetLinearDamping(body, dampLinear);
				dBodySetAngularDamping(body, dampAngular);
				dBodySetMaxAngularSpeed(body, maxAngularSpeed);
				dGeomSetBody(ed->rbe.body.geom, 0);
			}
		}
		else
		{
			// no body... then let's adjust the parameters of the geom directly
			dGeomSetBody(ed->rbe.body.geom, 0); // just in case we previously HAD a body (which should never happen)
			dGeomSetPosition(ed->rbe.body.geom, origin[0], origin[1], origin[2]);
			dGeomSetRotation(ed->rbe.body.geom, r[0]);
		}
	}

	if(body)
	{
		// limit movement speed to prevent missed collisions at high speed
		const dReal *ovelocity = dBodyGetLinearVel(body);
		const dReal *ospinvelocity = dBodyGetAngularVel(body);

		// simpler clamp as a last resort
		if (physics_ode_maxspeed->value > 0)
		{
			float highestAxis = physics_ode_maxspeed->value;
			float scaleDiff = 1.0;

			for (int i = 0; i < 3; i++)
			{
				if (velocity[i] > highestAxis)
					highestAxis = velocity[i];

				if (velocity[i] < -highestAxis)
					highestAxis = -velocity[i];
			}

			scaleDiff = (physics_ode_maxspeed->value / highestAxis);

			// if we should scale it down...
			if (scaleDiff < 1.0f) {
				VectorScale(ovelocity, scaleDiff, velocity);
				dBodySetLinearVel(body, velocity[0], velocity[1], velocity[2]);
			}
		} else {
			// this is probably more expensive
			movelimit = ctx->movelimit * ctx->movelimit;

			test = DotProduct(ovelocity,ovelocity);
			if (test > movelimit*movelimit)
			{
				// scale down linear velocity to the movelimit
				// scale down angular velocity the same amount for consistency
				f = movelimit / sqrt(test);
				VectorScale(ovelocity, f, velocity);
				VectorScale(ospinvelocity, f, spinvelocity);
				dBodySetLinearVel(body, velocity[0], velocity[1], velocity[2]);
				dBodySetAngularVel(body, spinvelocity[0], spinvelocity[1], spinvelocity[2]);
			}
		}

		// make sure the angular velocity is not exploding
		spinlimit = physics_ode_spinlimit->value;
		test = DotProduct(ospinvelocity,ospinvelocity);
		if (test > spinlimit)
		{
			dBodySetAngularVel(body, 0, 0, 0);
		}
	}
}

#define MAX_CONTACTS 16
static void VARGS nearCallback (void *data, dGeomID o1, dGeomID o2)
{
	world_t *world = (world_t *)data;
	struct odectx_s *ctx = (struct odectx_s*)world->rbe;
	dContact contact[MAX_CONTACTS]; // max contacts per collision pair
	dBodyID b1;
	dBodyID b2;
	dJointID c;
	int i;
	int numcontacts;

	float bouncefactor1 = 0.0f;
	float bouncestop1 = 60.0f / 800.0f;
	float bouncefactor2 = 0.0f;
	float bouncestop2 = 60.0f / 800.0f;
	float dampLinear1 = 1.0f;
	float dampAngular1 = 1.0f;
	float dampLinear2 = 1.0f;
	float dampAngular2 = 1.0f;
	float erp;
	dVector3 grav;
	wedict_t *ed1, *ed2;

	if (dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		// colliding a space with something
		dSpaceCollide2(o1, o2, data, &nearCallback);
		// Note we do not want to test intersections within a space,
		// only between spaces.
		//if (dGeomIsSpace(o1)) dSpaceCollide(o1, data, &nearCallback);
		//if (dGeomIsSpace(o2)) dSpaceCollide(o2, data, &nearCallback);
		return;
	}

	b1 = dGeomGetBody(o1);
	b2 = dGeomGetBody(o2);

	// at least one object has to be using MOVETYPE_PHYSICS or we just don't care
	if (!b1 && !b2)
		return;

	// exit without doing anything if the two bodies are connected by a joint
	if (b1 && b2 && dAreConnectedExcluding(b1, b2, dJointTypeContact))
		return;

	ed1 = (wedict_t *) dGeomGetData(o1);
	ed2 = (wedict_t *) dGeomGetData(o2);
	if (ed1 == ed2 && ed1)
	{
		//ragdolls don't make contact with the bbox of the doll entity
		//the origional entity should probably not be solid anyway.
		//these bodies should probably not collide against bboxes of other entities with ragdolls either, but meh.
		if (ed1->rbe.body.body == b1 || ed2->rbe.body.body == b2)
			return;
	}
	if(!ed1 || ED_ISFREE(ed1))
		ed1 = world->edicts;
	if(!ed2 || ED_ISFREE(ed2))
		ed2 = world->edicts;

	//non-solid things can still interact with pushers, but not other stuff.
	if (!ed1->v->solid && ed2->v->solid != SOLID_BSP)
		return;
	if (!ed2->v->solid && ed1->v->solid != SOLID_BSP)
		return;

	// generate contact points between the two non-space geoms
	numcontacts = dCollide(o1, o2, MAX_CONTACTS, &(contact[0].geom), sizeof(contact[0]));
	if (numcontacts)
	{
		trace_t contactTrace;
		memset ( &contactTrace, 0, sizeof ( trace_t ) );
		contactTrace.endpos[0] = contact[0].geom.pos[0];
		contactTrace.endpos[1] = contact[0].geom.pos[1];
		contactTrace.endpos[2] = contact[0].geom.pos[2];
		contactTrace.plane.normal[0] = contact[0].geom.normal[0];
		contactTrace.plane.normal[1] = contact[0].geom.normal[1];
		contactTrace.plane.normal[2] = contact[0].geom.normal[2];

		if(ed1 && ed1->v->touch)
		{
			world->Event_Touch(world, ed1, ed2, &contactTrace);
		}
		if(ed2 && ed2->v->touch)
		{
			world->Event_Touch(world, ed2, ed1, &contactTrace);
		}

		/* if either ent killed itself, don't collide */
		if ((ed1&&ED_ISFREE(ed1)) || (ed2&&ED_ISFREE(ed2)))
			return;
	}

	if(ed1)
	{
		if (ed1->xv->bouncefactor)
			bouncefactor1 = ed1->xv->bouncefactor;

		if (ed1->xv->bouncestop)
			bouncestop1 = ed1->xv->bouncestop;
	}

	if(ed2)
	{
		if (ed2->xv->bouncefactor)
			bouncefactor2 = ed2->xv->bouncefactor;

		if (ed2->xv->bouncestop)
			bouncestop2 = ed2->xv->bouncestop;
	}

	if ((ed2->entnum&&ed1->v->owner == ed2->entnum) || (ed1->entnum&&ed2->v->owner == ed1->entnum))
		return;

	// merge bounce factors and bounce stop
	if(bouncefactor2 > 0)
	{
		if(bouncefactor1 > 0)
		{
			// TODO possibly better logic to merge bounce factor data?
			if(bouncestop2 < bouncestop1)
				bouncestop1 = bouncestop2;
			if(bouncefactor2 > bouncefactor1)
				bouncefactor1 = bouncefactor2;
		}
		else
		{
			bouncestop1 = bouncestop2;
			bouncefactor1 = bouncefactor2;
		}
	}
	dWorldGetGravity(ctx->dworld, grav);
	bouncestop1 *= fabs(grav[2]);

	erp = (DotProduct(ed1->v->velocity, ed1->v->velocity) > DotProduct(ed2->v->velocity, ed2->v->velocity)) ? ed1->xv->erp : ed2->xv->erp;

	// add these contact points to the simulation
	for (i = 0;i < numcontacts;i++)
	{
		contact[i].surface.mode =	dContactApprox1 |
									(physics_ode_contact_erp->value != -1 ? dContactSoftERP : 0) |
									(physics_ode_contact_cfm->value != -1 ? dContactSoftCFM : 0) |
									(bouncefactor1 > 0 ? dContactBounce : 0);

		if (physics_ode_contact_mu->value != -1) {
			contact[i].surface.mu = physics_ode_contact_mu->value;

			if (ed1->xv->friction)
				contact[i].surface.mu *= ed1->xv->friction;
			if (ed2->xv->friction)
				contact[i].surface.mu *= ed2->xv->friction;
		} else {
			contact[i].surface.mu = dInfinity;
		}

		contact[i].surface.mu2 = 0;
		contact[i].surface.soft_erp = physics_ode_contact_erp->value + erp;
		contact[i].surface.soft_cfm = physics_ode_contact_cfm->value;
		contact[i].surface.bounce = bouncefactor1;
		contact[i].surface.bounce_vel = bouncestop1;

		c = dJointCreateContact(ctx->dworld, ctx->contactgroup, contact + i);
		dJointAttach(c, b1, b2);
	}
}

static void QDECL World_ODE_Frame(world_t *world, double frametime, double gravity)
{
	struct odectx_s *ctx = (struct odectx_s*)world->rbe;
	if (world->rbe_hasphysicsents || ctx->hasextraobjs)
	{
		int i;
		wedict_t *ed;

		ctx->iterations = bound(1, physics_ode_iterationsperframe->ival, 1000);
		ctx->step = frametime / ctx->iterations;
		ctx->movelimit = physics_ode_movelimit->value / ctx->step;

		if (world->rbe_hasphysicsents || ctx->hasextraobjs)
		{
			// copy physics properties from entities to physics engine
			for (i = 0;i < world->num_edicts;i++)
			{
				ed = (wedict_t*)EDICT_NUM_PB(world->progs, i);
				if (!ED_ISFREE(ed))
					World_ODE_Frame_BodyFromEntity(world, ed);
			}
			// oh, and it must be called after all bodies were created
			for (i = 0;i < world->num_edicts;i++)
			{
				ed = (wedict_t*)EDICT_NUM_PB(world->progs, i);
				if (!ED_ISFREE(ed))
					World_ODE_Frame_JointFromEntity(world, ed);
			}
			while(ctx->cmdqueuehead)
			{
				rbecommandqueue_t *cmd = ctx->cmdqueuehead;
				ctx->cmdqueuehead = cmd->next;
				if (!cmd->next)
					ctx->cmdqueuetail = NULL;
				World_ODE_RunCmd(world, cmd);
				Z_Free(cmd);
			}
		}

		for (i = 0;i < ctx->iterations;i++)
		{
			// set the gravity
			dWorldSetGravity(ctx->dworld, 0, 0, -gravity);
			// set the tolerance for closeness of objects
			dWorldSetContactSurfaceLayer(ctx->dworld, max(0, physics_ode_contactsurfacelayer->value));

			// run collisions for the current world state, creating JointGroup
			dSpaceCollide(ctx->space, (void *)world, nearCallback);

			// run physics (move objects, calculate new velocities)
			if (physics_ode_worldquickstep->ival)
			{
				dWorldSetQuickStepNumIterations(ctx->dworld, bound(1, physics_ode_worldquickstep_iterations->ival, 200));
				dWorldQuickStep(ctx->dworld, ctx->step);
			}
			else
				dWorldStep(ctx->dworld, ctx->step);

			// clear the JointGroup now that we're done with it
			dJointGroupEmpty(ctx->contactgroup);
		}

		if (world->rbe_hasphysicsents)
		{
			// copy physics properties from physics engine to entities
			for (i = 1;i < world->num_edicts;i++)
			{
				ed = (wedict_t*)EDICT_NUM_PB(world->progs, i);
				if (!ED_ISFREE(ed))
					World_ODE_Frame_BodyToEntity(world, ed);
			}
		}
	}
}

static void QDECL World_ODE_PushCommand(world_t *world, rbecommandqueue_t *val)
{
	struct odectx_s *ctx = (struct odectx_s*)world->rbe;
	rbecommandqueue_t *cmd = (rbecommandqueue_t*)BZ_Malloc(sizeof(*cmd));
	world->rbe_hasphysicsents = qtrue;	//just in case.
	memcpy(cmd, val, sizeof(*cmd));
	cmd->next = NULL;
	//add on the end of the queue, so that order is preserved.
	if (ctx->cmdqueuehead)
	{
		rbecommandqueue_t *ot = ctx->cmdqueuetail;
		ot->next = ctx->cmdqueuetail = cmd;
	}
	else
		ctx->cmdqueuetail = ctx->cmdqueuehead = cmd;
}

static void QDECL World_ODE_Start(world_t *world)
{
	struct odectx_s *ctx;
	dVector3 center, extents;
	if (world->rbe)
		return;

#ifdef ODE_DYNAMIC
	if (!ode_dll)
		return;
#endif

	ctx = BZ_Malloc(sizeof(*ctx));
	memset(ctx, 0, sizeof(*ctx));
	world->rbe = &ctx->pub;

#ifndef FTEENGINE
	r_meshpitch.value = cvarfuncs->GetFloat("r_meshpitch");
	r_meshroll.value = cvarfuncs->GetFloat("r_meshroll");
#endif

	VectorAvg(world->worldmodel->mins, world->worldmodel->maxs, center);
	VectorSubtract(world->worldmodel->maxs, center, extents);
	ctx->dworld = dWorldCreate();
	ctx->space = dQuadTreeSpaceCreate(NULL, center, extents, bound(1, cvarfuncs->GetFloat("physics_ode_quadtree_depth"), 10));
	ctx->contactgroup = dJointGroupCreate(0);

	ctx->pub.End					= World_ODE_End;
	ctx->pub.RemoveJointFromEntity	= World_ODE_RemoveJointFromEntity;
	ctx->pub.RemoveFromEntity		= World_ODE_RemoveFromEntity;
	ctx->pub.RagMatrixToBody		= World_ODE_RagMatrixToBody;
	ctx->pub.RagCreateBody			= World_ODE_RagCreateBody;
	ctx->pub.RagMatrixFromJoint		= World_ODE_RagMatrixFromJoint;
	ctx->pub.RagMatrixFromBody		= World_ODE_RagMatrixFromBody;
	ctx->pub.RagEnableJoint			= World_ODE_RagEnableJoint;
	ctx->pub.RagCreateJoint			= World_ODE_RagCreateJoint;
	ctx->pub.RagDestroyBody			= World_ODE_RagDestroyBody;
	ctx->pub.RagDestroyJoint		= World_ODE_RagDestroyJoint;
	ctx->pub.RunFrame				= World_ODE_Frame;
	ctx->pub.PushCommand			= World_ODE_PushCommand;

	if(physics_ode_world_erp->value >= 0)
		dWorldSetERP(ctx->dworld, physics_ode_world_erp->value);
	if(physics_ode_world_cfm->value >= 0)
		dWorldSetCFM(ctx->dworld, physics_ode_world_cfm->value);
	if (physics_ode_world_damping->value)
	{
		dWorldSetLinearDamping(ctx->dworld, (physics_ode_world_damping_linear->value >= 0) ? (physics_ode_world_damping_linear->value * physics_ode_world_damping->value) : 0);
		dWorldSetLinearDampingThreshold(ctx->dworld, (physics_ode_world_damping_linear_threshold->value >= 0) ? (physics_ode_world_damping_linear_threshold->value * physics_ode_world_damping->value) : 0);
		dWorldSetAngularDamping(ctx->dworld, (physics_ode_world_damping_angular->value >= 0) ? (physics_ode_world_damping_angular->value * physics_ode_world_damping->value) : 0);
		dWorldSetAngularDampingThreshold(ctx->dworld, (physics_ode_world_damping_angular_threshold->value >= 0) ? (physics_ode_world_damping_angular_threshold->value * physics_ode_world_damping->value) : 0);
	}
	else
	{
		dWorldSetLinearDamping(ctx->dworld, 0);
		dWorldSetLinearDampingThreshold(ctx->dworld, 0);
		dWorldSetAngularDamping(ctx->dworld, 0);
		dWorldSetAngularDampingThreshold(ctx->dworld, 0);
	}
	if (physics_ode_autodisable->ival)
	{
		dWorldSetAutoDisableSteps(ctx->dworld, bound(1, physics_ode_autodisable_steps->ival, 100)); 
		dWorldSetAutoDisableTime(ctx->dworld, physics_ode_autodisable_time->value);
		dWorldSetAutoDisableAverageSamplesCount(ctx->dworld, bound(1, physics_ode_autodisable_threshold_samples->ival, 100));
		dWorldSetAutoDisableLinearThreshold(ctx->dworld, physics_ode_autodisable_threshold_linear->value); 
		dWorldSetAutoDisableAngularThreshold(ctx->dworld, physics_ode_autodisable_threshold_angular->value); 
		dWorldSetAutoDisableFlag (ctx->dworld, true);
	}
	else
		dWorldSetAutoDisableFlag (ctx->dworld, false);
}


static void World_ODE_RunCmd(world_t *world, rbecommandqueue_t *cmd)
{
	switch(cmd->command)
	{
	case RBECMD_ENABLE:
		if (cmd->edict->rbe.body.body)
			dBodyEnable(cmd->edict->rbe.body.body);
		break;
	case RBECMD_DISABLE:
		if (cmd->edict->rbe.body.body)
			dBodyDisable(cmd->edict->rbe.body.body);
		break;
	case RBECMD_FORCE:
		if (cmd->edict->rbe.body.body)
		{
			dBodyEnable(cmd->edict->rbe.body.body);
			dBodyAddForceAtPos(cmd->edict->rbe.body.body, cmd->v1[0], cmd->v1[1], cmd->v1[2], cmd->v2[0], cmd->v2[1], cmd->v2[2]);
		}
		break;
	case RBECMD_TORQUE:
		if (cmd->edict->rbe.body.body)
		{
			dBodyEnable(cmd->edict->rbe.body.body);
			dBodyAddTorque(cmd->edict->rbe.body.body, cmd->v1[0], cmd->v1[1], cmd->v1[2]);
		}
		break;
	}
}

static void QDECL Plug_ODE_Shutdown(void)
{
	if (rbefuncs)
		rbefuncs->UnregisterPhysicsEngine("ODE");
	World_ODE_Shutdown();
}

qboolean Plug_Init(void)
{
	rbefuncs = plugfuncs->GetEngineInterface("RBE", sizeof(*rbefuncs));
#ifndef FTEENGINE
	cvar_r_meshpitch = cvarfuncs->GetNVFDG("r_meshpitch", "1", 0, NULL, NULL);
	cvar_r_meshroll = cvarfuncs->GetNVFDG("r_meshroll", "1", 0, NULL, NULL);
#endif
	if (rbefuncs && (	rbefuncs->version < RBEPLUGFUNCS_VERSION ||
						rbefuncs->wedictsize != sizeof(wedict_t)))
		rbefuncs = NULL;
	if (!rbefuncs)
	{
		Con_Printf("ODE plugin failed: Engine is incompatible.\n");
		return false;
	}

	if (!rbefuncs || !rbefuncs->RegisterPhysicsEngine)
		Con_Printf("ODE plugin failed: Engine doesn't support physics engine plugins.\n");
	else if (!rbefuncs->RegisterPhysicsEngine("ODE", World_ODE_Start))
		Con_Printf("ODE plugin failed: Engine already has a physics plugin active.\n");
	else
	{
		if (!World_ODE_Init())
		{
			rbefuncs->UnregisterPhysicsEngine("ODE");
			return false;
		}
		plugfuncs->ExportFunction("Shutdown", Plug_ODE_Shutdown);
		return true;
	}
	return false;
}
#endif

