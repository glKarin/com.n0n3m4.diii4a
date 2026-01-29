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

//if we're not building as an fte-specific plugin, we must be being built as part of the fte engine itself.
//(no, we don't want to act as a plugin for ezquake...)
#ifndef FTEPLUGIN
#define FTEENGINE
#define FTEPLUGIN
#define pCvar_Register Cvar_Get
#define pCvar_GetNVFDG Cvar_Get2
#define pCvar_GetFloat(x) Cvar_FindVar(x)->value
#define pSys_Error Sys_Error
#define Plug_Init Plug_Bullet_Init
#pragma comment(lib,"../../plugins/bullet/libs/bullet_dbg.lib")
#endif
#include "quakedef.h"
#include "../plugin.h"
#include "../engine.h"

#include "pr_common.h"
#include "com_mesh.h"

#ifndef FTEENGINE
#define BZ_Malloc malloc
#define BZ_Free free
#define Z_Free BZ_Free
//#define vec3_origin vec3_origin_
//static vec3_t vec3_origin;
#define VectorCompare VectorCompare_
static int VectorCompare (const pvec3_t v1, const pvec3_t v2)
{
	int		i;
	for (i=0 ; i<3 ; i++)
		if (v1[i] != v2[i])
			return 0;
	return 1;
}

#endif
static rbeplugfuncs_t *rbefuncs;



//============================================================================
// physics engine support
//============================================================================

#define DEG2RAD(d) (d * M_PI * (1/180.0f))
#define RAD2DEG(d) ((d*180) / M_PI)

#include "btBulletDynamicsCommon.h"

//not sure where these are going. seems to be an issue only on windows.
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

static void World_Bullet_RunCmd(world_t *world, rbecommandqueue_t *cmd);

static cvar_t *physics_bullet_maxiterationsperframe;
static cvar_t *physics_bullet_framerate;
static cvar_t *pr_meshpitch;

void World_Bullet_Init(void)
{
	physics_bullet_maxiterationsperframe	= cvarfuncs->GetNVFDG("physics_bullet_maxiterationsperframe",	"10",	0, "FIXME: should be 1 when CCD is working properly.", "Bullet");
	physics_bullet_framerate				= cvarfuncs->GetNVFDG("physics_bullet_framerate",				"60",	0, "Bullet physics run at a fixed framerate in order to preserve numerical stability (interpolation is used to smooth out the result). Higher framerates are of course more demanding.", "Bullet");
	pr_meshpitch							= cvarfuncs->GetNVFDG("r_meshpitch",								"-1",	0, "", "Bullet");
}

typedef struct bulletcontext_s
{
	rigidbodyengine_t funcs;

	bool hasextraobjs;
//	void *ode_space;
//	void *ode_contactgroup;
	// number of constraint solver iterations to use (for dWorldStepFast)
//	int ode_iterations;
	// actual step (server frametime / ode_iterations)
//	vec_t ode_step;
	// max velocity for a 1-unit radius object at current step to prevent
	// missed collisions
//	vec_t ode_movelimit;
	rbecommandqueue_t *cmdqueuehead;
	rbecommandqueue_t *cmdqueuetail;


	world_t *gworld;


	btBroadphaseInterface *broadphase;
	btDefaultCollisionConfiguration *collisionconfig;
	btCollisionDispatcher *collisiondispatcher;
	btSequentialImpulseConstraintSolver *solver;
	btDiscreteDynamicsWorld *dworld;
	btOverlapFilterCallback *ownerfilter;
} bulletcontext_t;

class QCFilterCallback : public btOverlapFilterCallback
{
	// return true when pairs need collision
	virtual bool	needBroadphaseCollision(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1) const
	{
		//dimensions don't collide
		bool collides = (proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask) != 0;
		collides = collides && (proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask);

		//owners don't collide (unless one is world, obviouslyish)
		if (collides)
		{
			auto b1 = (btRigidBody*)proxy0->m_clientObject;
			auto b2 = (btRigidBody*)proxy1->m_clientObject;
			//don't let two qc-controlled entities collide in Bullet, that's the job of quake.
			if (b1->isStaticOrKinematicObject() && b2->isStaticOrKinematicObject())
				return false;
			auto e1 = (wedict_t*)b1->getUserPointer();
			auto e2 = (wedict_t*)b2->getUserPointer();
			if (e1&&e2)
			{
				if ((e1->v->solid == SOLID_TRIGGER && e2->v->solid != SOLID_BSP) ||
					(e2->v->solid == SOLID_TRIGGER && e1->v->solid != SOLID_BSP))
					return false;	//triggers only collide with bsp objects.
				if (e1->entnum && e2->entnum)
					collides = e1->v->owner != e2->entnum && e2->v->owner != e1->entnum;
			}
		}

		return collides;
	}
};

static void QDECL World_Bullet_End(world_t *world)
{
	auto ctx = reinterpret_cast<bulletcontext_t*>(world->rbe);
	world->rbe = nullptr;
	delete ctx->dworld;
	delete ctx->solver;
	delete ctx->collisionconfig;
	delete ctx->collisiondispatcher;
	delete ctx->broadphase;
	delete ctx->ownerfilter;
	Z_Free(ctx);
}

static void QDECL World_Bullet_RemoveJointFromEntity(world_t *world, wedict_t *ed)
{
	ed->rbe.joint_type = 0;
//	if(ed->rbe.joint)
//		dJointDestroy((dJointID)ed->rbe.joint);
	ed->rbe.joint.joint = NULL;
}

static void QDECL World_Bullet_RemoveFromEntity(world_t *world, wedict_t *ed)
{
	auto ctx = reinterpret_cast<bulletcontext_t*>(world->rbe);
	btRigidBody *body;
	btCollisionShape *geom;
	if (!ed->rbe.physics)
		return;

	// entity is not physics controlled, free any physics data
	ed->rbe.physics = qfalse;

	body = (btRigidBody*)ed->rbe.body.body;
	ed->rbe.body.body = NULL;
	if (body)
		ctx->dworld->removeRigidBody (body);

	geom = (btCollisionShape*)ed->rbe.body.geom;
	ed->rbe.body.geom = NULL;
	if (ed->rbe.body.geom)
		delete geom;

	//FIXME: joints
	rbefuncs->ReleaseCollisionMesh(ed);
	if(ed->rbe.massbuf)
		BZ_Free(ed->rbe.massbuf);
	ed->rbe.massbuf = NULL;
}

static bool NegativeMeshPitch(world_t *world, wedict_t *ent)
{
	if (ent->v->modelindex)
	{
		model_t *model = world->Get_CModel(world, ent->v->modelindex);
		if (model && (model->type == mod_alias || model->type == mod_halflife))
			return pr_meshpitch->value < 0;
		return false;
	}
	return false;
}

static btTransform transformFromQuake(world_t *world, wedict_t *ent)
{
	vec3_t forward, left, up;
	if (NegativeMeshPitch(world, ent))
	{
		pvec3_t iangles = {-ent->v->angles[0], ent->v->angles[1], ent->v->angles[2]};
		rbefuncs->AngleVectors(iangles, forward, left, up);
	}
	else
		rbefuncs->AngleVectors(ent->v->angles, forward, left, up);
	VectorNegate(left, left);

	return btTransform(btMatrix3x3(forward[0], forward[1], forward[2], left[0], left[1], left[2], up[0], up[1], up[2]), btVector3(ent->v->origin[0], ent->v->origin[1], ent->v->origin[2]));
}

static void World_Bullet_Frame_JointFromEntity(world_t *world, wedict_t *ed)
{
	auto rbe = reinterpret_cast<bulletcontext_t*>(world->rbe);
	btTypedConstraint *j = nullptr;
	btRigidBody *b1 = nullptr;
	btRigidBody *b2 = nullptr;
	int movetype = 0;
	int jointtype = 0;
	int enemy = 0, aiment = 0;
	wedict_t *e1, *e2;
//	vec_t CFM, ERP, FMax;
	vec_t Stop;
//	vec_t Vel;
	vec3_t forward;
	movetype = ed->v->movetype;
	jointtype = ed->xv->jointtype;
	enemy = ed->v->enemy;
	aiment = ed->v->aiment;
	btVector3 origin(ed->v->origin[0], ed->v->origin[1], ed->v->origin[2]);
	btVector3 velocity(ed->v->velocity[0], ed->v->velocity[1], ed->v->velocity[2]);
	btVector3 movedir(ed->v->movedir[0], ed->v->movedir[1], ed->v->movedir[2]);
	if(movetype == MOVETYPE_PHYSICS)
		jointtype = 0; // can't have both

	e1 = (wedict_t*)PROG_TO_EDICT(world->progs, enemy);
	b1 = (btRigidBody*)e1->rbe.body.body;
	if(ED_ISFREE(e1) || !b1)
		enemy = 0;
	e2 = (wedict_t*)PROG_TO_EDICT(world->progs, aiment);
	b2 = (btRigidBody*)e2->rbe.body.body;
	if(ED_ISFREE(e2) || !b2)
		aiment = 0;
	// see http://www.ode.org/old_list_archives/2006-January/017614.html
	// we want to set ERP? make it fps independent and work like a spring constant
	// note: if movedir[2] is 0, it becomes ERP = 1, CFM = 1.0 / (H * K)
	if(movedir[0] > 0 && movedir[1] > 0)
	{
//		float K = movedir[0];
//		float D = movedir[1];
//		float R = 2.0 * D * sqrt(K); // we assume D is premultiplied by sqrt(sprungMass)
//		CFM = 1.0 / (rbe->ode_step * K + R); // always > 0
//		ERP = rbe->ode_step * K * CFM;
//		Vel = 0;
//		FMax = 0;
		Stop = movedir[2];
	}
	else if(movedir[1] < 0)
	{
//		CFM = 0;
//		ERP = 0;
//		Vel = movedir[0];
//		FMax = -movedir[1]; // TODO do we need to multiply with world.physics.ode_step?
		Stop = movedir[2] > 0 ? movedir[2] : BT_INFINITY;
	}
	else // movedir[0] > 0, movedir[1] == 0 or movedir[0] < 0, movedir[1] >= 0
	{
//		CFM = 0;
//		ERP = 0;
//		Vel = 0;
//		FMax = 0;
		Stop = BT_INFINITY;
	}
	if(jointtype == ed->rbe.joint_type && VectorCompare(origin, ed->rbe.joint_origin) && VectorCompare(velocity, ed->rbe.joint_velocity) && VectorCompare(ed->v->angles, ed->rbe.joint_angles) && enemy == ed->rbe.joint_enemy && aiment == ed->rbe.joint_aiment && VectorCompare(movedir, ed->rbe.joint_movedir))
		return; // nothing to do

	if(ed->rbe.joint.joint)
	{
		j = (btTypedConstraint*)ed->rbe.joint.joint;
		rbe->dworld->removeConstraint(j);
		ed->rbe.joint.joint = nullptr;
		delete j;
	}
	if (!jointtype)
		return;

	btVector3 b1org(0,0,0), b2org(0,0,0);
	if(enemy)
		b1org.setValue(e1->v->origin[0], e1->v->origin[1], e1->v->origin[2]);
	if(aiment)
		b2org.setValue(e2->v->origin[0], e2->v->origin[1], e2->v->origin[2]);

	ed->rbe.joint_type = jointtype;
	ed->rbe.joint_enemy = enemy;
	ed->rbe.joint_aiment = aiment;
	VectorCopy(origin, ed->rbe.joint_origin);
	VectorCopy(velocity, ed->rbe.joint_velocity);
	VectorCopy(ed->v->angles, ed->rbe.joint_angles);
	VectorCopy(movedir, ed->rbe.joint_movedir);

	rbefuncs->AngleVectors(ed->v->angles, forward, nullptr, nullptr);

	//Con_Printf("making new joint %i\n", (int) (ed - prog->edicts));
	switch(jointtype)
	{
	case JOINTTYPE_POINT:
		j = new btPoint2PointConstraint(*b1, *b2, btVector3(b1org - origin), btVector3(b2org - origin));
		break;
/*	case JOINTTYPE_HINGE:
		btHingeConstraint *h = new btHingeConstraint(*b1, *b2, btVector3(b1org - origin), btVector3(b2org - origin), aa, ab, ref);
		j = h;
		if (h)
		{
			h->setLimit(-Stop, Stop, softness, bias, relaxation);
			h->setAxis(btVector3(forward[0], forward[1], forward[2]));
//			h->dJointSetHingeParam(j, dParamFMax, FMax);
//			h->dJointSetHingeParam(j, dParamHiStop, Stop);	
//			h->dJointSetHingeParam(j, dParamLoStop, -Stop);
//			h->dJointSetHingeParam(j, dParamStopCFM, CFM);
//			h->dJointSetHingeParam(j, dParamStopERP, ERP);

//			h->setMotorTarget(vel);
		}
		break;*/
	case JOINTTYPE_SLIDER:
		{
			btTransform jointtransform = transformFromQuake(world, ed);
			btTransform b1transform = transformFromQuake(world, e1).inverseTimes(jointtransform);
			btTransform b2transform = transformFromQuake(world, e2).inverseTimes(jointtransform);

			btSliderConstraint *s = new btSliderConstraint(*b1, *b2, b1transform, b2transform, false);
			j = s;
			if (s)
			{
//				s->dJointSetSliderAxis(j, forward[0], forward[1], forward[2]);
//				s->dJointSetSliderParam(j, dParamFMax, FMax);
				s->setLowerLinLimit(-Stop);
				s->setUpperLinLimit(Stop);
				s->setLowerAngLimit(0);
				s->setUpperAngLimit(0);
//				s->dJointSetSliderParam(j, dParamHiStop, Stop);
//				s->dJointSetSliderParam(j, dParamLoStop, -Stop);
//				s->dJointSetSliderParam(j, dParamStopCFM, CFM);
//				s->dJointSetSliderParam(j, dParamStopERP, ERP);
//				s->setTargetLinMotorVelocity(vel);
//				s->setPoweredLinMotor(true);
			}
		}
		break;
/*	case JOINTTYPE_UNIVERSAL:
		btGeneric6DofConstraint
		j = dJointCreateUniversal(rbe->ode_world, 0);
		if (j)
		{
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
		}
		break;*/
/*	case JOINTTYPE_HINGE2:
		j = dJointCreateHinge2(rbe->ode_world, 0);
		if (j)
		{
			dJointSetHinge2Anchor(j, origin[0], origin[1], origin[2]);
			dJointSetHinge2Axis1(j, forward[0], forward[1], forward[2]);
			dJointSetHinge2Axis2(j, velocity[0], velocity[1], velocity[2]);
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
		}
		break;*/
	case JOINTTYPE_FIXED:
		{
			btTransform jointtransform = transformFromQuake(world, ed);
			btTransform b1transform = transformFromQuake(world, e1).inverseTimes(jointtransform);
			btTransform b2transform = transformFromQuake(world, e2).inverseTimes(jointtransform);

			j = new btFixedConstraint(*b1, *b2, b1transform, b2transform);
		}
		break;
	case 0:
	default:
		j = nullptr;
		break;
	}

	ed->rbe.joint.joint = (void *) j;
	if (j)
	{
		j->setUserConstraintPtr((void *) ed);
		rbe->dworld->addConstraint(j, false);
	}
}

static void MatToTransform(const float *mat, btTransform &tr)
{
	tr.setBasis(btMatrix3x3(
		mat[0], mat[1], mat[2],
		mat[4], mat[5], mat[6],
		mat[8], mat[9], mat[10]));
	tr.setOrigin(btVector3(mat[3], mat[7], mat[11]));
}
static void MatFromTransform(float *mat, const btTransform &tr)
{
	const btMatrix3x3 &m = tr.getBasis();
	const btVector3 &o = tr.getOrigin();
	const btVector3 &r0 = m.getRow(0);
	const btVector3 &r1 = m.getRow(1);
	const btVector3 &r2 = m.getRow(2);
	mat[0] = r0[0];
	mat[1] = r0[1];
	mat[2] = r0[2];
	mat[3] =o[0];
	mat[4] = r1[0];
	mat[5] = r1[1];
	mat[6] = r1[2];
	mat[7] =o[1];
	mat[8] = r2[0];
	mat[9] = r2[1];
	mat[10]= r2[2];
	mat[11]=o[2];
}
static qboolean QDECL World_Bullet_RagMatrixToBody(rbebody_t *bodyptr, float *mat)
{	//mat is a 4*3 matrix
	btTransform tr;
	auto body = reinterpret_cast<btRigidBody*>(bodyptr->body);
	MatToTransform(mat, tr);
	body->setWorldTransform(tr);
	return qtrue;
}
static qboolean QDECL World_Bullet_RagCreateBody(world_t *world, rbebody_t *bodyptr, rbebodyinfo_t *bodyinfo, float *mat, wedict_t *ent)
{
	btRigidBody *body = nullptr;
	btCollisionShape *geom = nullptr;
	float radius;
	float threshold;
//	float length;
	auto ctx = reinterpret_cast<bulletcontext_t*>(world->rbe);
//	int axisindex;
	ctx->hasextraobjs = true;

	switch(bodyinfo->geomshape)
	{
/*
	case GEOMTYPE_TRIMESH:
//		foo Matrix4x4_Identity(ed->rbe.offsetmatrix);
		geom = NULL;
		if (!model)
		{
			Con_Printf("entity %i (classname %s) has no model\n", NUM_FOR_EDICT(world->progs, (edict_t*)ed), PR_GetString(world->progs, ed->v->classname));
			if (ed->rbe.physics)
				World_Bullet_RemoveFromEntity(world, ed);
			return;
		}
		if (!rbefuncs->GenerateCollisionMesh(world, model, ed, geomcenter))
		{
			if (ed->rbe.physics)
				World_Bullet_RemoveFromEntity(world, ed);
			return;
		}

//		foo Matrix4x4_RM_CreateTranslate(ed->rbe.offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2]);

		{
			btTriangleIndexVertexArray *tiva = new btTriangleIndexVertexArray();
			btIndexedMesh mesh;
			mesh.m_vertexType = PHY_FLOAT;
			mesh.m_indexType = PHY_INTEGER;
			mesh.m_numTriangles = ed->rbe.numtriangles;
			mesh.m_numVertices = ed->rbe.numvertices;
			mesh.m_triangleIndexBase = (const unsigned char*)ed->rbe.element3i;
			mesh.m_triangleIndexStride = sizeof(*ed->rbe.element3i)*3;
			mesh.m_vertexBase = (const unsigned char*)ed->rbe.vertex3f;
			mesh.m_vertexStride = sizeof(*ed->rbe.vertex3f)*3;
			tiva->addIndexedMesh(mesh);
			geom = new btBvhTriangleMeshShape(tiva, true);
		}
		break;
*/
	default:
		Con_DPrintf("World_Bullet_RagCreateBody: unsupported geomshape %i\n", bodyinfo->geomshape);
	case GEOMTYPE_BOX:
		geom = new btBoxShape(btVector3(bodyinfo->dimensions[0], bodyinfo->dimensions[1], bodyinfo->dimensions[2]) * 0.5);
		radius = sqrt(DotProduct(bodyinfo->dimensions,bodyinfo->dimensions));
		threshold = min(bodyinfo->dimensions[0], min(bodyinfo->dimensions[1], bodyinfo->dimensions[2]));
		break;

	case GEOMTYPE_SPHERE:
		threshold = radius = bodyinfo->dimensions[0] * 0.5f;
		geom = new btSphereShape(radius);
		break;

	case GEOMTYPE_CAPSULE:
//	case GEOMTYPE_CAPSULE_X:
//	case GEOMTYPE_CAPSULE_Y:
	case GEOMTYPE_CAPSULE_Z:
		radius = (bodyinfo->dimensions[0]+bodyinfo->dimensions[1]) * 0.5f;
		geom = new btCapsuleShapeZ(radius, bodyinfo->dimensions[2]);
		threshold = min(radius, bodyinfo->dimensions[2]*0.5);
		radius = max(radius, bodyinfo->dimensions[2]*0.5);
		break;

	case GEOMTYPE_CYLINDER:
//	case GEOMTYPE_CYLINDER_X:
//	case GEOMTYPE_CYLINDER_Y:
	case GEOMTYPE_CYLINDER_Z:
		radius = (bodyinfo->dimensions[0] + bodyinfo->dimensions[1]) * 0.5;
		geom = new btCylinderShapeZ(btVector3(radius, radius, bodyinfo->dimensions[2])*0.5);
		threshold = min(radius, bodyinfo->dimensions[2]*0.5);
		radius = max(radius, bodyinfo->dimensions[2]*0.5);
		break;
	}
	bodyptr->geom = geom;

	//now create the body too

	btVector3 fallInertia(0, 0, 0);
	geom->calculateLocalInertia(bodyinfo->mass, fallInertia);
	btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(bodyinfo->mass, nullptr, geom, fallInertia);
	MatToTransform(mat, fallRigidBodyCI.m_startWorldTransform);
	body = new btRigidBody(fallRigidBodyCI);
	body->setUserPointer(ent);
	bodyptr->body = reinterpret_cast<void*>(body);

	//motion threshhold should be speed/physicsframerate.
	//FIXME: recalculate...
	body->setCcdMotionThreshold(threshold/64);
	//radius should be the body's radius

	body->setCcdSweptSphereRadius(radius);

	ctx->dworld->addRigidBody(body, ent?ent->xv->dimension_solid:~0, ent?ent->xv->dimension_hit:~0);

	return qtrue;
}

static void QDECL World_Bullet_RagMatrixFromJoint(rbejoint_t *joint, rbejointinfo_t *info, float *mat)
{
/*
	dVector3 dr3;
	switch(info->type)
	{
	case JOINTTYPE_POINT:
		dJointGetBallAnchor(joint->ode_joint, dr3);
		mat[3] = dr3[0];
		mat[7] = dr3[1];
		mat[11] = dr3[2];
		VectorClear(mat+4);
		VectorClear(mat+8);
		break;

	case JOINTTYPE_HINGE:
		dJointGetHingeAnchor(joint->ode_joint, dr3);
		mat[3] = dr3[0];
		mat[7] = dr3[1];
		mat[11] = dr3[2];

		dJointGetHingeAxis(joint->ode_joint, dr3);
		VectorCopy(dr3, mat+4);
		VectorClear(mat+8);

		CrossProduct(mat+4, mat+8, mat+0);
		return;
		break;
	case JOINTTYPE_HINGE2:
		dJointGetHinge2Anchor(joint->ode_joint, dr3);
		mat[3] = dr3[0];
		mat[7] = dr3[1];
		mat[11] = dr3[2];

		dJointGetHinge2Axis1(joint->ode_joint, dr3);
		VectorCopy(dr3, mat+4);
		dJointGetHinge2Axis2(joint->ode_joint, dr3);
		VectorCopy(dr3, mat+8);
		break;

	case JOINTTYPE_SLIDER:
		//no anchor point...
		//get the two bodies and average their origin for a somewhat usable representation of an anchor.
		{
			const dReal *p1, *p2;
			dReal n[3];
			dBodyID b1 = dJointGetBody(joint->ode_joint, 0), b2 = dJointGetBody(joint->ode_joint, 1);
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
			dJointGetSliderAxis(joint->ode_joint, dr3 + 0);
			VectorInterpolate(p1, 0.5, p2, dr3);
			mat[3] = dr3[0];
			mat[7] = dr3[1];
			mat[11] = dr3[2];

			VectorClear(mat+4);
			VectorClear(mat+8);
		}
		break;

	case JOINTTYPE_UNIVERSAL:
		dJointGetUniversalAnchor(joint->ode_joint, dr3);
		mat[3] = dr3[0];
		mat[7] = dr3[1];
		mat[11] = dr3[2];

		dJointGetUniversalAxis1(joint->ode_joint, dr3);
		VectorCopy(dr3, mat+4);
		dJointGetUniversalAxis2(joint->ode_joint, dr3);
		VectorCopy(dr3, mat+8);

		CrossProduct(mat+4, mat+8, mat+0);
		return;
		break;
	}
	rbefuncs->AngleVectors(vec3_origin, mat+0, mat+4, mat+8);
	VectorNegate((mat+4), (mat+4));
*/
}

static void QDECL World_Bullet_RagMatrixFromBody(world_t *world, rbebody_t *bodyptr, float *mat)
{
	//auto ctx = reinterpret_cast<bulletcontext_t*>(world->rbe);
	auto body = reinterpret_cast<btRigidBody*>(bodyptr->body);
	MatFromTransform(mat, body->getCenterOfMassTransform());
}
static void QDECL World_Bullet_RagEnableJoint(rbejoint_t *joint, qboolean enabled)
{
	auto j = reinterpret_cast<btTypedConstraint*>(joint->joint);
	j->setEnabled(enabled);
}
static void QDECL World_Bullet_RagCreateJoint(world_t *world, rbejoint_t *joint, rbejointinfo_t *info, rbebody_t *body1, rbebody_t *body2, vec3_t aaa2[3])
{
	auto ctx = reinterpret_cast<bulletcontext_t*>(world->rbe);
	btTypedConstraint *j;
	btVector3 org(aaa2[0][0], aaa2[0][1], aaa2[0][2]);
	btVector3 axis1(aaa2[1][0], aaa2[1][1], aaa2[1][2]);
	btVector3 axis2(aaa2[2][0], aaa2[2][1], aaa2[2][2]);
	auto rb1 = reinterpret_cast<btRigidBody*>(body1->body);
	auto rb2 = reinterpret_cast<btRigidBody*>(body2->body);

	switch(info->type)
	{
	case JOINTTYPE_POINT:
		{
			auto point = new btPoint2PointConstraint(*rb1, *rb2, rb1->getWorldTransform().getOrigin()-org, rb2->getWorldTransform().getOrigin()-org);
//			point->setParam(BT_P2P_FLAGS_ERP, info->ERP);
//			point->setParam(BT_P2P_FLAGS_CFM, info->CFM);
			j = point;
		}
		break;
	case JOINTTYPE_HINGE:
		{
			auto hinge = new btHingeConstraint(*rb1, *rb2, org, org, axis1, axis2);
			hinge->setLimit(info->LoStop, info->HiStop /*other stuff!*/);
//			hinge->setParam(BT_HINGE_FLAGS_CFM_NORM, info->CFM);
//			hinge->setParam(BT_HINGE_FLAGS_CFM_STOP, info->CFM2);
//			hinge->setParam(BT_HINGE_FLAGS_ERP_NORM, info->ERP);
//			hinge->setParam(BT_HINGE_FLAGS_ERP_STOP, info->ERP2);
			j = hinge;
		}
		break;
//	case JOINTTYPE_SLIDER:
//		j = btSliderConstraint(rb1, rb2, tr1, tr2, false);
//		break;
	case JOINTTYPE_UNIVERSAL:
		{
			auto uni = new btUniversalConstraint(*rb1, *rb2, org, axis1, axis2);
//			uni->setParam(BT_6DOF_FLAGS_CFM_NORM, info->CFM);
//			uni->setParam(BT_6DOF_FLAGS_CFM_STOP, info->CFM2);
//			uni->setParam(BT_6DOF_FLAGS_ERP_STOP, info->ERP);
			uni->setUpperLimit(info->HiStop, info->HiStop2);
			uni->setLowerLimit(info->LoStop, info->LoStop2);
			j = uni;
		}
		break;
	case JOINTTYPE_HINGE2:
		{
			auto hinge2 = new btHinge2Constraint(*rb1, *rb2, org, axis1, axis2);
			hinge2->setUpperLimit(info->HiStop);
			hinge2->setLowerLimit(info->LoStop);
			j = hinge2;
		}
		break;
//	case JOINTTYPE_FIXED:
//		j = btFixedConstraint(rb1, rb2, tr1, tr2);
//		break;
	default:
		Con_Printf("Bullet: joint type %i not supported\n", info->type);
		j = nullptr;
		break;
	}
/*	if (joint->ode_joint)
	{
		//Con_Printf("made new joint %i\n", (int) (ed - prog->edicts));
//		dJointSetData(joint->ode_joint, NULL);
		dJointAttach(joint->ode_joint, body1?body1->ode_body:NULL, body2?body2->ode_body:NULL);

		switch(info->type)
		{
			case JOINTTYPE_POINT:
				dJointSetBallAnchor(joint->ode_joint, aaa2[0][0], aaa2[0][1], aaa2[0][2]);
				break;
			case JOINTTYPE_HINGE:
				dJointSetHingeAnchor(joint->ode_joint, aaa2[0][0], aaa2[0][1], aaa2[0][2]);
				dJointSetHingeAxis(joint->ode_joint, aaa2[1][0], aaa2[1][1], aaa2[1][2]);
				dJointSetHingeParam(joint->ode_joint, dParamFMax, info->FMax);
				dJointSetHingeParam(joint->ode_joint, dParamHiStop, info->HiStop);	
				dJointSetHingeParam(joint->ode_joint, dParamLoStop, info->LoStop);
				dJointSetHingeParam(joint->ode_joint, dParamStopCFM, info->CFM);
				dJointSetHingeParam(joint->ode_joint, dParamStopERP, info->ERP);
				dJointSetHingeParam(joint->ode_joint, dParamVel, info->Vel);
				break;
			case JOINTTYPE_SLIDER:
				dJointSetSliderAxis(joint->ode_joint, aaa2[1][0], aaa2[1][1], aaa2[1][2]);
				dJointSetSliderParam(joint->ode_joint, dParamFMax, info->FMax);
				dJointSetSliderParam(joint->ode_joint, dParamHiStop, info->HiStop);
				dJointSetSliderParam(joint->ode_joint, dParamLoStop, info->LoStop);
				dJointSetSliderParam(joint->ode_joint, dParamStopCFM, info->CFM);
				dJointSetSliderParam(joint->ode_joint, dParamStopERP, info->ERP);
				dJointSetSliderParam(joint->ode_joint, dParamVel, info->Vel);
				break;
			case JOINTTYPE_UNIVERSAL:
				dJointSetUniversalAnchor(joint->ode_joint, aaa2[0][0], aaa2[0][1], aaa2[0][2]);
				dJointSetUniversalAxis1(joint->ode_joint, aaa2[1][0], aaa2[1][1], aaa2[1][2]);
				dJointSetUniversalAxis2(joint->ode_joint, aaa2[2][0], aaa2[2][1], aaa2[2][2]);
				dJointSetUniversalParam(joint->ode_joint, dParamFMax, info->FMax);
				dJointSetUniversalParam(joint->ode_joint, dParamHiStop, info->HiStop);
				dJointSetUniversalParam(joint->ode_joint, dParamLoStop, info->LoStop);
				dJointSetUniversalParam(joint->ode_joint, dParamStopCFM, info->CFM);
				dJointSetUniversalParam(joint->ode_joint, dParamStopERP, info->ERP);
				dJointSetUniversalParam(joint->ode_joint, dParamVel, info->Vel);
				dJointSetUniversalParam(joint->ode_joint, dParamFMax2, info->FMax2);
				dJointSetUniversalParam(joint->ode_joint, dParamHiStop2, info->HiStop2);
				dJointSetUniversalParam(joint->ode_joint, dParamLoStop2, info->LoStop2);
				dJointSetUniversalParam(joint->ode_joint, dParamStopCFM2, info->CFM2);
				dJointSetUniversalParam(joint->ode_joint, dParamStopERP2, info->ERP2);
				dJointSetUniversalParam(joint->ode_joint, dParamVel2, info->Vel2);
				break;
			case JOINTTYPE_HINGE2:
				dJointSetHinge2Anchor(joint->ode_joint, aaa2[0][0], aaa2[0][1], aaa2[0][2]);
				dJointSetHinge2Axis1(joint->ode_joint, aaa2[1][0], aaa2[1][1], aaa2[1][2]);
				dJointSetHinge2Axis2(joint->ode_joint, aaa2[2][0], aaa2[2][1], aaa2[2][2]);
				dJointSetHinge2Param(joint->ode_joint, dParamFMax, info->FMax);
				dJointSetHinge2Param(joint->ode_joint, dParamHiStop, info->HiStop);
				dJointSetHinge2Param(joint->ode_joint, dParamLoStop, info->LoStop);
				dJointSetHinge2Param(joint->ode_joint, dParamStopCFM, info->CFM);
				dJointSetHinge2Param(joint->ode_joint, dParamStopERP, info->ERP);
				dJointSetHinge2Param(joint->ode_joint, dParamVel, info->Vel);
				dJointSetHinge2Param(joint->ode_joint, dParamFMax2, info->FMax2);
				dJointSetHinge2Param(joint->ode_joint, dParamHiStop2, info->HiStop2);
				dJointSetHinge2Param(joint->ode_joint, dParamLoStop2, info->LoStop2);
				dJointSetHinge2Param(joint->ode_joint, dParamStopCFM2, info->CFM2);
				dJointSetHinge2Param(joint->ode_joint, dParamStopERP2, info->ERP2);
				dJointSetHinge2Param(joint->ode_joint, dParamVel2, info->Vel2);
				break;
			case JOINTTYPE_FIXED:
				dJointSetFixed(joint->ode_joint);
				break;
		}
	}
*/
	if (j)
		ctx->dworld->addConstraint(j, true);
	joint->joint = reinterpret_cast<void*>(j);
}

static void QDECL World_Bullet_RagDestroyBody(world_t *world, rbebody_t *bodyptr)
{
	auto ctx = reinterpret_cast<bulletcontext_t*>(world->rbe);
	auto body = reinterpret_cast<btRigidBody*>(bodyptr->body);
	auto geom = reinterpret_cast<btCollisionShape*>(bodyptr->geom);

	bodyptr->body = nullptr;
	bodyptr->geom = nullptr;

	if (body)
	{
		ctx->dworld->removeRigidBody(body);
		delete body;
	}
	if (geom)
		delete geom;
}

static void QDECL World_Bullet_RagDestroyJoint(world_t *world, rbejoint_t *joint)
{
	auto ctx = reinterpret_cast<bulletcontext_t*>(world->rbe);
	auto j = reinterpret_cast<btTypedConstraint*>(joint->joint);
	if (j)
	{
		ctx->dworld->removeConstraint(j);
		delete j;
	}
	joint->joint = nullptr;
}

//bullet gives us a handy way to get/set motion states. we can cheesily update entity fields this way.
class QCMotionState : public btMotionState
{
	wedict_t *edict;
	qboolean dirty;
	btTransform trans;
	world_t *world;


public:
	void ReloadMotionState(void)
	{
		vec3_t offset;
		vec3_t axis[3];
		btVector3 org;
		rbefuncs->AngleVectors(edict->v->angles, axis[0], axis[1], axis[2]);
		VectorNegate(axis[1], axis[1]);
		VectorAvg(edict->rbe.mins, edict->rbe.maxs, offset);
		VectorMA(edict->v->origin, offset[0]*1, axis[0], org);
		org[3] = 0;//for sse.
		VectorMA(org, offset[1]*1, axis[1], org);
		VectorMA(org, offset[2]*1, axis[2], org);

		trans.setBasis(btMatrix3x3(axis[0][0], axis[1][0], axis[2][0],
								axis[0][1],	axis[1][1],	axis[2][1],
								axis[0][2],	axis[1][2],	axis[2][2]));
		trans.setOrigin(org);
	}
	QCMotionState(wedict_t *ed, world_t *w)
	{
		dirty = qtrue;
		edict = ed;
		world = w;

		ReloadMotionState();
	}
	virtual ~QCMotionState()
	{
	}

	virtual void getWorldTransform(btTransform &worldTrans) const
	{
		worldTrans = trans;
	}

	virtual void setWorldTransform(const btTransform &worldTrans)
	{
		vec3_t fwd, left, up, offset;
		trans = worldTrans;

		btVector3 pos = worldTrans.getOrigin();
		VectorCopy(worldTrans.getBasis().getColumn(0), fwd);
		VectorCopy(worldTrans.getBasis().getColumn(1), left);
		VectorCopy(worldTrans.getBasis().getColumn(2), up);
		VectorAvg(edict->rbe.mins, edict->rbe.maxs, offset);
		VectorMA(pos, offset[0]*-1, fwd, pos);
		VectorMA(pos, offset[1]*-1, left, pos);
		VectorMA(pos, offset[2]*-1, up, edict->v->origin);

		rbefuncs->VectorAngles(fwd, up, edict->v->angles, (qboolean)NegativeMeshPitch(world, edict));

		const btVector3 &vel = ((btRigidBody*)edict->rbe.body.body)->getLinearVelocity();
		VectorCopy(vel.m_floats, edict->v->velocity);

		//so it doesn't get rebuilt
		VectorCopy(edict->v->origin, edict->rbe.origin);
		VectorCopy(edict->v->angles, edict->rbe.angles);
		VectorCopy(edict->v->velocity, edict->rbe.velocity);

		//FIXME: relink the ent into the areagrid
	}
};

static void World_Bullet_Frame_BodyFromEntity(world_t *world, wedict_t *ed)
{
	auto ctx = reinterpret_cast<bulletcontext_t*>(world->rbe);
	btRigidBody *body = nullptr;
//	btScalar mass;
	float test;
//	void *dataID;
	model_t *model;
	int axisindex;
	int modelindex = 0;
	int movetype = MOVETYPE_NONE;
	int solid = SOLID_NOT;
	int geomtype = GEOMTYPE_SOLID;
	qboolean modified = qfalse;
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
//	vec_t f;
	vec_t length;
	vec_t massval = 1.0f;
//	vec_t movelimit;
	vec_t radius;
	vec_t scale;
//	vec_t spinlimit;
	qboolean gravity;

	geomtype = ed->xv->geomtype;
	solid = ed->v->solid;
	movetype = ed->v->movetype;
	scale = ed->xv->scale?ed->xv->scale:1;
	modelindex = 0;
	model = nullptr;

	if (!geomtype)
	{
		switch(solid)
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
		modelindex = ed->v->modelindex;
		model = world->Get_CModel(world, modelindex);
		if (!model || model->loadstate != MLS_LOADED)
		{
			model = nullptr;
			modelindex = 0;
		}
		if (model)
		{
			VectorScale(model->mins, scale, entmins);
			VectorScale(model->maxs, scale, entmaxs);
			if (ed->xv->mass)
				massval = ed->xv->mass;
		}
		else
		{
			modelindex = 0;
			massval = 1.0f;
		}
		massval = 0;	//bullet does not support trisoup moving through the world.
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
			World_Bullet_RemoveFromEntity(world, ed);
		return;
	}

	VectorSubtract(entmaxs, entmins, geomsize);
	if (DotProduct(geomsize,geomsize) == 0)
	{
		// we don't allow point-size physics objects...
		if (ed->rbe.physics)
			World_Bullet_RemoveFromEntity(world, ed);
		return;
	}

	// check if we need to create or replace the geom
	if (!ed->rbe.physics
	 || !VectorCompare(ed->rbe.mins, entmins)
	 || !VectorCompare(ed->rbe.maxs, entmaxs)
	 || ed->rbe.modelindex != modelindex)
	{
		btCollisionShape *geom;

		modified = qtrue;
		World_Bullet_RemoveFromEntity(world, ed);
		ed->rbe.physics = qtrue;
		VectorCopy(entmins, ed->rbe.mins);
		VectorCopy(entmaxs, ed->rbe.maxs);
		ed->rbe.modelindex = modelindex;
		VectorAvg(entmins, entmaxs, geomcenter);
		ed->rbe.movelimit = min(geomsize[0], min(geomsize[1], geomsize[2]));

/*		memset(ed->rbe.offsetmatrix, 0, sizeof(ed->rbe.offsetmatrix));
		ed->rbe.offsetmatrix[0] = 1;
		ed->rbe.offsetmatrix[5] = 1;
		ed->rbe.offsetmatrix[10] = 1;
		ed->rbe.offsetmatrix[3] = -geomcenter[0];
		ed->rbe.offsetmatrix[7] = -geomcenter[1];
		ed->rbe.offsetmatrix[11] = -geomcenter[2];
*/
		ed->rbe.mass = massval;

		switch(geomtype)
		{
		case GEOMTYPE_TRIMESH:
//			foo Matrix4x4_Identity(ed->rbe.offsetmatrix);
			geom = nullptr;
			if (!model)
			{
				Con_Printf("entity %i (classname %s) has no model\n", NUM_FOR_EDICT(world->progs, (edict_t*)ed), PR_GetString(world->progs, ed->v->classname));
				if (ed->rbe.physics)
					World_Bullet_RemoveFromEntity(world, ed);
				return;
			}
			if (!rbefuncs->GenerateCollisionMesh(world, model, ed, geomcenter))
			{
				if (ed->rbe.physics)
					World_Bullet_RemoveFromEntity(world, ed);
				return;
			}

//			foo Matrix4x4_RM_CreateTranslate(ed->rbe.offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2]);

			{
				btTriangleIndexVertexArray *tiva = new btTriangleIndexVertexArray();
				btIndexedMesh mesh;
				mesh.m_vertexType = PHY_FLOAT;
				mesh.m_indexType = PHY_INTEGER;
				mesh.m_numTriangles = ed->rbe.numtriangles;
				mesh.m_numVertices = ed->rbe.numvertices;
				mesh.m_triangleIndexBase = (const unsigned char*)ed->rbe.element3i;
				mesh.m_triangleIndexStride = sizeof(*ed->rbe.element3i)*3;
				mesh.m_vertexBase = (const unsigned char*)ed->rbe.vertex3f;
				mesh.m_vertexStride = sizeof(*ed->rbe.vertex3f)*3;
				tiva->addIndexedMesh(mesh);
				geom = new btBvhTriangleMeshShape(tiva, true);
			}
			break;

		case GEOMTYPE_BOX:
			geom = new btBoxShape(btVector3(geomsize[0], geomsize[1], geomsize[2]) * 0.5);
			break;

		case GEOMTYPE_SPHERE:
			geom = new btSphereShape(geomsize[0] * 0.5f);
			break;

		case GEOMTYPE_CAPSULE:
		case GEOMTYPE_CAPSULE_X:
		case GEOMTYPE_CAPSULE_Y:
		case GEOMTYPE_CAPSULE_Z:
			if (geomtype == GEOMTYPE_CAPSULE)
			{
				// the qc gives us 3 axis radius, the longest axis is the capsule axis
				axisindex = 0;
				if (geomsize[axisindex] < geomsize[1])
					axisindex = 1;
				if (geomsize[axisindex] < geomsize[2])
					axisindex = 2;
			}
			else
				axisindex = geomtype-GEOMTYPE_CAPSULE_X;
			if (axisindex == 0)
				radius = min(geomsize[1], geomsize[2]) * 0.5f;
			else if (axisindex == 1)
				radius = min(geomsize[0], geomsize[2]) * 0.5f;
			else
				radius = min(geomsize[0], geomsize[1]) * 0.5f;
			length = geomsize[axisindex] - radius*2;
			if (length <= 0)
			{
				radius -= (1 - length)*0.5;
				length = 1;
			}
			if (axisindex == 0)
				geom = new btCapsuleShapeX(radius, length);
			else if (axisindex == 1)
				geom = new btCapsuleShape(radius, length);
			else
				geom = new btCapsuleShapeZ(radius, length);
			break;

		case GEOMTYPE_CYLINDER:
		case GEOMTYPE_CYLINDER_X:
		case GEOMTYPE_CYLINDER_Y:
		case GEOMTYPE_CYLINDER_Z:
			if (geomtype == GEOMTYPE_CYLINDER)
			{
				// the qc gives us 3 axis radius, the longest axis is the capsule axis
				axisindex = 0;
				if (geomsize[axisindex] < geomsize[1])
					axisindex = 1;
				if (geomsize[axisindex] < geomsize[2])
					axisindex = 2;
			}
			else
				axisindex = geomtype-GEOMTYPE_CYLINDER_X;
			if (axisindex == 0)
				geom = new btCylinderShapeX(btVector3(geomsize[0], geomsize[1], geomsize[2])*0.5);
			else if (axisindex == 1)
				geom = new btCylinderShape(btVector3(geomsize[0], geomsize[1], geomsize[2])*0.5);
			else
				geom = new btCylinderShapeZ(btVector3(geomsize[0], geomsize[1], geomsize[2])*0.5);
			break;

		default:
//			Con_Printf("World_Bullet_BodyFromEntity: unrecognized solid value %i was accepted by filter\n", solid);
			if (ed->rbe.physics)
				World_Bullet_RemoveFromEntity(world, ed);
			return;
		}
//		Matrix3x4_InvertTo4x4_Simple(ed->rbe.offsetmatrix, ed->rbe.offsetimatrix);
//		ed->rbe.massbuf = BZ_Malloc(sizeof(dMass));
//		memcpy(ed->rbe.massbuf, &mass, sizeof(dMass));

		ed->rbe.body.geom = (void *)geom;
	}

	//non-moving objects need to be static objects (and thus need 0 mass)
	if (movetype != MOVETYPE_PHYSICS && movetype != MOVETYPE_WALK) //enabling kinematic objects for everything else destroys framerates (!movetype || movetype == MOVETYPE_PUSH)
		massval = 0;

	//if the mass changes, we'll need to create a new body (but not the shape, so invalidate the current one)
	if (ed->rbe.mass != massval)
	{
		ed->rbe.mass = massval;
		body = (btRigidBody*)ed->rbe.body.body;
		if (body)
			ctx->dworld->removeRigidBody(body);
		ed->rbe.body.body = NULL;
	}

//	if(ed->rbe.body.geom)
//		dGeomSetData(ed->rbe.body.geom, (void*)ed);
	if (movetype == MOVETYPE_PHYSICS && ed->rbe.mass)
	{
		if (ed->rbe.body.body == NULL)
		{
//			ed->rbe.body.body = (void *)(body = dBodyCreate(world->rbe.world));
//			dGeomSetBody(ed->rbe.body.geom, body);
//			dBodySetData(body, (void*)ed);
//			dBodySetMass(body, (dMass *) ed->rbe.massbuf);

			btVector3 fallInertia(0, 0, 0);
			((btCollisionShape*)ed->rbe.body.geom)->calculateLocalInertia(ed->rbe.mass, fallInertia);
			btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(ed->rbe.mass, new QCMotionState(ed,world), (btCollisionShape*)ed->rbe.body.geom, fallInertia);
			body = new btRigidBody(fallRigidBodyCI);
			body->setUserPointer(ed);
//			btTransform trans;
//			trans.setFromOpenGLMatrix(ed->rbe.offsetmatrix);
//			body->setCenterOfMassTransform(trans);
			ed->rbe.body.body = (void*)body;

			//motion threshhold should be speed/physicsframerate.
			//Threshhold enables CCD when the object moves faster than X
			//FIXME: recalculate...
			body->setCcdMotionThreshold((geomsize[0]+geomsize[1]+geomsize[2])*(4/3));
			//radius should be the body's radius, or smaller.
			body->setCcdSweptSphereRadius((geomsize[0]+geomsize[1]+geomsize[2])*(0.5/3));

			ctx->dworld->addRigidBody(body, ed->xv->dimension_solid, ed->xv->dimension_hit);

			modified = qtrue;
		}
	}
	else
	{
		if (ed->rbe.body.body == nullptr)
		{
			btRigidBody::btRigidBodyConstructionInfo rbci(ed->rbe.mass, new QCMotionState(ed,world), (btCollisionShape*)ed->rbe.body.geom, btVector3(0, 0, 0));
			body = new btRigidBody(rbci);
			body->setUserPointer(ed);
//			btTransform trans;
//			trans.setFromOpenGLMatrix(ed->rbe.offsetmatrix);
//			body->setCenterOfMassTransform(trans);
			ed->rbe.body.body = (void*)body;
			if (ed->rbe.mass)
				body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			else
				body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
			ctx->dworld->addRigidBody(body, ed->xv->dimension_solid, ed->xv->dimension_hit);

			modified = qtrue;
		}
	}

	body = (btRigidBody*)ed->rbe.body.body;

	// get current data from entity
	gravity = qtrue;
	VectorCopy(ed->v->origin, origin);
	VectorCopy(ed->v->velocity, velocity);
	VectorCopy(ed->v->angles, angles);
	VectorCopy(ed->v->avelocity, avelocity);
	if (ed == world->edicts || (ed->xv->gravity && ed->xv->gravity <= 0.01))
		gravity = qfalse;

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
				qangles[PITCH] *= -1;
				qavelocity[PITCH] *= -1;
			}
		}

		rbefuncs->AngleVectors(qangles, forward, left, up);
		VectorNegate(left,left);
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
		modified = qtrue;
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
	if (
		0//!VectorCompare(origin, ed->rbe.origin)
	 || !VectorCompare(velocity, ed->rbe.velocity)
	 //|| !VectorCompare(angles, ed->rbe.angles)
	 || !VectorCompare(avelocity, ed->rbe.avelocity)
	 || gravity != ed->rbe.gravity)
		modified = qtrue;

	// store the qc values into the physics engine
	body = (btRigidBody*)ed->rbe.body.body;
	if (modified && body)
	{
//		dVector3 r[3];
//		float entitymatrix[16];
//		float bodymatrix[16];

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

//		foo Matrix4x4_RM_FromVectors(entitymatrix, forward, left, up, origin);
//		foo Matrix4_Multiply(ed->rbe.offsetmatrix, entitymatrix, bodymatrix);
//		foo Matrix3x4_RM_ToVectors(bodymatrix, forward, left, up, origin);

//		r[0][0] = forward[0];
//		r[1][0] = forward[1];
//		r[2][0] = forward[2];
//		r[0][1] = left[0];
//		r[1][1] = left[1];
//		r[2][1] = left[2];
//		r[0][2] = up[0];
//		r[1][2] = up[1];
//		r[2][2] = up[2];

		auto ms = (QCMotionState*)body->getMotionState();
		ms->ReloadMotionState();
		body->setMotionState(ms);
		body->setLinearVelocity(btVector3(velocity[0], velocity[1], velocity[2]));
		body->setAngularVelocity(btVector3(spinvelocity[0], spinvelocity[1], spinvelocity[2]));
//		body->setGravity(btVector3(ed->xv->gravitydir[0], ed->xv->gravitydir[1], ed->xv->gravitydir[2]) * ed->xv->gravity);

		//something changed. make sure it still falls over appropriately
		body->setActivationState(1);
	}

/* FIXME: check if we actually need this insanity with bullet (ode sucks).
	if(body)
	{
		// limit movement speed to prevent missed collisions at high speed
		btVector3 ovelocity = body->getLinearVelocity();
		btVector3 ospinvelocity = body->getAngularVelocity();
		movelimit = ed->rbe.movelimit * world->rbe.movelimit;
		test = DotProduct(ovelocity,ovelocity);
		if (test > movelimit*movelimit)
		{
			// scale down linear velocity to the movelimit
			// scale down angular velocity the same amount for consistency
			f = movelimit / sqrt(test);
			VectorScale(ovelocity, f, velocity);
			VectorScale(ospinvelocity, f, spinvelocity);
			body->setLinearVelocity(btVector3(velocity[0], velocity[1], velocity[2]));
			body->setAngularVelocity(btVector3(spinvelocity[0], spinvelocity[1], spinvelocity[2]));
		}

		// make sure the angular velocity is not exploding
		spinlimit = physics_bullet_spinlimit.value;
		test = DotProduct(ospinvelocity,ospinvelocity);
		if (test > spinlimit)
		{
			body->setAngularVelocity(btVector3(0, 0, 0));
		}
	}
*/
}

/*
#define MAX_CONTACTS 16
static void VARGS nearCallback (void *data, dGeomID o1, dGeomID o2)
{
	world_t *world = (world_t *)data;
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
		if (ed1->rbe.body.body == b1 || ed2->rbe.body == b2)
			return;
	}
	if(!ed1 || ed1->isfree)
		ed1 = world->edicts;
	if(!ed2 || ed2->isfree)
		ed2 = world->edicts;

	// generate contact points between the two non-space geoms
	numcontacts = dCollide(o1, o2, MAX_CONTACTS, &(contact[0].geom), sizeof(contact[0]));
	if (numcontacts)
	{
		if(ed1 && ed1->v->touch)
		{
			world->Event_Touch(world, ed1, ed2);
		}
		if(ed2 && ed2->v->touch)
		{
			world->Event_Touch(world, ed2, ed1);
		}

		// if either ent killed itself, don't collide 
		if ((ed1&&ed1->isfree) || (ed2&&ed2->isfree))
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
	dWorldGetGravity(world->rbe.world, grav);
	bouncestop1 *= fabs(grav[2]);

	erp = (DotProduct(ed1->v->velocity, ed1->v->velocity) > DotProduct(ed2->v->velocity, ed2->v->velocity)) ? ed1->xv->erp : ed2->xv->erp;

	// add these contact points to the simulation
	for (i = 0;i < numcontacts;i++)
	{
		contact[i].surface.mode =	(physics_bullet_contact_mu.value != -1 ? dContactApprox1 : 0) |
									(physics_bullet_contact_erp.value != -1 ? dContactSoftERP : 0) |
									(physics_bullet_contact_cfm.value != -1 ? dContactSoftCFM : 0) |
									(bouncefactor1 > 0 ? dContactBounce : 0);
		contact[i].surface.mu = physics_bullet_contact_mu.value;
		if (ed1->xv->friction)
			contact[i].surface.mu *= ed1->xv->friction;
		if (ed2->xv->friction)
			contact[i].surface.mu *= ed2->xv->friction;
		contact[i].surface.mu2 = 0;
		contact[i].surface.soft_erp = physics_bullet_contact_erp.value + erp;
		contact[i].surface.soft_cfm = physics_bullet_contact_cfm.value;
		contact[i].surface.bounce = bouncefactor1;
		contact[i].surface.bounce_vel = bouncestop1;
		c = dJointCreateContact(world->rbe.world, world->rbe.contactgroup, contact + i);
		dJointAttach(c, b1, b2);
	}
}
*/

static void QDECL World_Bullet_Frame(world_t *world, double frametime, double gravity)
{
	auto ctx = reinterpret_cast<bulletcontext_t*>(world->rbe);
	if (world->rbe_hasphysicsents || ctx->hasextraobjs)
	{
		int iters;
		unsigned int i;
		wedict_t *ed;

//		world->rbe.iterations = bound(1, physics_bullet_iterationsperframe.ival, 1000);
//		world->rbe.step = frametime / world->rbe.iterations;
//		world->rbe.movelimit = physics_bullet_movelimit.value / world->rbe.step;


		// copy physics properties from entities to physics engine
		for (i = 0;i < world->num_edicts;i++)
		{
			ed = WEDICT_NUM_PB(world->progs, i);
			if (!ED_ISFREE(ed))
				World_Bullet_Frame_BodyFromEntity(world, ed);
		}
		// oh, and it must be called after all bodies were created
		for (i = 0;i < world->num_edicts;i++)
		{
			ed = WEDICT_NUM_PB(world->progs, i);
			if (!ED_ISFREE(ed))
				World_Bullet_Frame_JointFromEntity(world, ed);
		}
		while(ctx->cmdqueuehead)
		{
			auto cmd = ctx->cmdqueuehead;
			ctx->cmdqueuehead = cmd->next;
			if (!cmd->next)
				ctx->cmdqueuetail = nullptr;
			World_Bullet_RunCmd(world, cmd);
			Z_Free(cmd);
		}

		ctx->dworld->setGravity(btVector3(0, 0, -gravity));

		iters=physics_bullet_maxiterationsperframe->value;
		if (iters < 0)
			iters = 0;
		ctx->dworld->stepSimulation(frametime, iters, 1/bound(1, physics_bullet_framerate->value, 500));

		// set the tolerance for closeness of objects
//		dWorldSetContactSurfaceLayer(world->rbe.world, max(0, physics_bullet_contactsurfacelayer.value));

		// run collisions for the current world state, creating JointGroup
//		dSpaceCollide(world->rbe.space, (void *)world, nearCallback);

		// run physics (move objects, calculate new velocities)
//		if (physics_bullet_worldquickstep.ival)
//		{
//			dWorldSetQuickStepNumIterations(world->rbe.world, bound(1, physics_bullet_worldquickstep_iterations.ival, 200));
//			dWorldQuickStep(world->rbe.world, world->rbe.step);
//		}
//		else
//			dWorldStep(world->rbe.world, world->rbe.step);

		// clear the JointGroup now that we're done with it
//		dJointGroupEmpty(world->rbe.contactgroup);
	}
}

static void World_Bullet_RunCmd(world_t *world, rbecommandqueue_t *cmd)
{
	auto body = (btRigidBody*)(cmd->edict->rbe.body.body);
	switch(cmd->command)
	{
	case RBECMD_ENABLE:
		if (body)
			body->setActivationState(1);
		break;
	case RBECMD_DISABLE:
		if (body)
			body->setActivationState(0);
		break;
	case RBECMD_FORCE:
		if (body)
		{
			btVector3 relativepos;
			const btVector3 &center = body->getCenterOfMassPosition();
			VectorSubtract(cmd->v2, center, relativepos);
			body->setActivationState(1);
			body->applyImpulse(btVector3(cmd->v1[0], cmd->v1[1], cmd->v1[2]), relativepos);
		}
		break;
	case RBECMD_TORQUE:
		if (cmd->edict->rbe.body.body)
		{
			body->setActivationState(1);
			body->applyTorque(btVector3(cmd->v1[0], cmd->v1[1], cmd->v1[2]));
		}
		break;
	}
}

static void QDECL World_Bullet_PushCommand(world_t *world, rbecommandqueue_t *val)
{
	auto ctx = reinterpret_cast<bulletcontext_t*>(world->rbe);
	auto cmd = (rbecommandqueue_t*)BZ_Malloc(sizeof(rbecommandqueue_t));
	world->rbe_hasphysicsents = qtrue;	//just in case.
	memcpy(cmd, val, sizeof(*cmd));
	cmd->next = nullptr;
	//add on the end of the queue, so that order is preserved.
	if (ctx->cmdqueuehead)
	{
		auto ot = ctx->cmdqueuetail;
		ot->next = ctx->cmdqueuetail = cmd;
	}
	else
		ctx->cmdqueuetail = ctx->cmdqueuehead = cmd;
}

static void QDECL World_Bullet_TraceEntity(world_t *world, wedict_t *ed, vec3_t start, vec3_t end, trace_t *trace)
{
	auto ctx = reinterpret_cast<bulletcontext_t*>(world->rbe);
	auto shape = (btCollisionShape*)ed->rbe.body.geom;

//btCollisionAlgorithm
	class myConvexResultCallback : public btCollisionWorld::ConvexResultCallback
	{
	public:
		void *m_impactent;
		btVector3 m_impactpos;
		btVector3 m_impactnorm;
		virtual	btScalar	addSingleResult(btCollisionWorld::LocalConvexResult& convexResult,bool normalInWorldSpace)
		{
			if (m_closestHitFraction > convexResult.m_hitFraction)
			{
				m_closestHitFraction = convexResult.m_hitFraction;
				m_impactpos = convexResult.m_hitPointLocal;
				m_impactnorm = convexResult.m_hitNormalLocal;
				m_impactent = convexResult.m_hitCollisionObject->getUserPointer();
			}
			return 0;
		}
	} result;
	result.m_impactent = nullptr;
	result.m_closestHitFraction = trace->fraction;

	btTransform from(btMatrix3x3(1, 0, 0, 0, 1, 0, 0, 0, 1), btVector3(start[0], start[1], start[2]));
	btTransform to(btMatrix3x3(1, 0, 0, 0, 1, 0, 0, 0, 1), btVector3(end[0], end[1], end[2]));
	ctx->dworld->convexSweepTest((btConvexShape*)shape, from, to, result, 1);

	if (result.m_impactent)
	{
		memset(trace, 0, sizeof(*trace));
		trace->fraction = trace->truefraction = result.m_closestHitFraction;
		VectorInterpolate(start, result.m_closestHitFraction, end, trace->endpos);
//		VectorCopy(result.m_impactpos, trace->endpos);
		VectorCopy(result.m_impactnorm, trace->plane.normal);
		trace->ent = result.m_impactent;
		trace->startsolid = qfalse; //FIXME: we don't really know
	}
}

static void QDECL World_Bullet_Start(world_t *world)
{
	bulletcontext_t *ctx;
	if (world->rbe)
		return;	//no thanks, we already have one. somehow.

	ctx = reinterpret_cast<bulletcontext_t*>(BZ_Malloc(sizeof(*ctx)));
	memset(ctx, 0, sizeof(*ctx));
	ctx->gworld = world;
	ctx->funcs.End						= World_Bullet_End;
	ctx->funcs.RemoveJointFromEntity	= World_Bullet_RemoveJointFromEntity;
	ctx->funcs.RemoveFromEntity			= World_Bullet_RemoveFromEntity;
	ctx->funcs.RagMatrixToBody			= World_Bullet_RagMatrixToBody;
	ctx->funcs.RagCreateBody			= World_Bullet_RagCreateBody;
	ctx->funcs.RagMatrixFromJoint		= World_Bullet_RagMatrixFromJoint;
	ctx->funcs.RagMatrixFromBody		= World_Bullet_RagMatrixFromBody;
	ctx->funcs.RagEnableJoint			= World_Bullet_RagEnableJoint;
	ctx->funcs.RagCreateJoint			= World_Bullet_RagCreateJoint;
	ctx->funcs.RagDestroyBody			= World_Bullet_RagDestroyBody;
	ctx->funcs.RagDestroyJoint			= World_Bullet_RagDestroyJoint;
	ctx->funcs.RunFrame					= World_Bullet_Frame;
	ctx->funcs.PushCommand				= World_Bullet_PushCommand;
	ctx->funcs.Trace					= World_Bullet_TraceEntity;
	world->rbe = &ctx->funcs;


	ctx->broadphase = new btDbvtBroadphase();
	ctx->collisionconfig = new btDefaultCollisionConfiguration();
	ctx->collisiondispatcher = new btCollisionDispatcher(ctx->collisionconfig);
	ctx->solver = new btSequentialImpulseConstraintSolver;
	ctx->dworld = new btDiscreteDynamicsWorld(ctx->collisiondispatcher, ctx->broadphase, ctx->solver, ctx->collisionconfig);

	ctx->ownerfilter = new QCFilterCallback();
	ctx->dworld->getPairCache()->setOverlapFilterCallback(ctx->ownerfilter);



	ctx->dworld->setGravity(btVector3(0, -10, 0));

/*
	if(physics_bullet_world_erp.value >= 0)
		dWorldSetERP(world->rbe.world, physics_bullet_world_erp.value);
	if(physics_bullet_world_cfm.value >= 0)
		dWorldSetCFM(world->rbe.world, physics_bullet_world_cfm.value);
	if (physics_bullet_world_damping.ival)
	{
		dWorldSetLinearDamping(world->rbe.world, (physics_bullet_world_damping_linear.value >= 0) ? (physics_bullet_world_damping_linear.value * physics_bullet_world_damping.value) : 0);
		dWorldSetLinearDampingThreshold(world->rbe.world, (physics_bullet_world_damping_linear_threshold.value >= 0) ? (physics_bullet_world_damping_linear_threshold.value * physics_bullet_world_damping.value) : 0);
		dWorldSetAngularDamping(world->rbe.world, (physics_bullet_world_damping_angular.value >= 0) ? (physics_bullet_world_damping_angular.value * physics_bullet_world_damping.value) : 0);
		dWorldSetAngularDampingThreshold(world->rbe.world, (physics_bullet_world_damping_angular_threshold.value >= 0) ? (physics_bullet_world_damping_angular_threshold.value * physics_bullet_world_damping.value) : 0);
	}
	else
	{
		dWorldSetLinearDamping(world->rbe.world, 0);
		dWorldSetLinearDampingThreshold(world->rbe.world, 0);
		dWorldSetAngularDamping(world->rbe.world, 0);
		dWorldSetAngularDampingThreshold(world->rbe.world, 0);
	}
	if (physics_bullet_autodisable.ival)
	{
		dWorldSetAutoDisableSteps(world->rbe.world, bound(1, physics_bullet_autodisable_steps.ival, 100)); 
		dWorldSetAutoDisableTime(world->rbe.world, physics_bullet_autodisable_time.value);
		dWorldSetAutoDisableAverageSamplesCount(world->rbe.world, bound(1, physics_bullet_autodisable_threshold_samples.ival, 100));
		dWorldSetAutoDisableLinearThreshold(world->rbe.world, physics_bullet_autodisable_threshold_linear.value); 
		dWorldSetAutoDisableAngularThreshold(world->rbe.world, physics_bullet_autodisable_threshold_angular.value); 
		dWorldSetAutoDisableFlag (world->rbe.world, true);
	}
	else
		dWorldSetAutoDisableFlag (world->rbe.world, false);
	*/
}

static void QDECL World_Bullet_Shutdown(void)
{
	if (rbefuncs)
		rbefuncs->UnregisterPhysicsEngine("Bullet");
}

static bool World_Bullet_DoInit(void)
{
	if (!rbefuncs || !rbefuncs->RegisterPhysicsEngine)
	{
		rbefuncs = nullptr;
		Con_Printf("Bullet plugin failed: Engine is incompatible.\n");
	}
	else if (!rbefuncs->RegisterPhysicsEngine("Bullet", World_Bullet_Start))
		Con_Printf("Bullet plugin failed: Engine already has a physics plugin active.\n");
	else
	{
		World_Bullet_Init();
		return true;
	}
	return false;
}

extern "C" qboolean Plug_Init(void)
{
	rbefuncs = (rbeplugfuncs_t*)plugfuncs->GetEngineInterface("RBE", sizeof(*rbefuncs));
	if (rbefuncs && (	rbefuncs->version < RBEPLUGFUNCS_VERSION ||
						rbefuncs->wedictsize != sizeof(wedict_t)))
		rbefuncs = nullptr;

	plugfuncs->ExportFunction("Shutdown", (funcptr_t)World_Bullet_Shutdown);
	return World_Bullet_DoInit()?qtrue:qfalse;
}


