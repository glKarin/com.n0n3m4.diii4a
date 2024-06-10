// MeshComponent.cpp
//

#include "../Gamelib/Game_local.h"

DnMeshComponent::DnMeshComponent()
{
	parentEntity = nullptr;
	renderEntityHandle = -1;
	bindJoint = (jointHandle_t)-1;
	memset(&renderEntityParams, 0, sizeof(renderEntityParams));
}

DnMeshComponent::~DnMeshComponent()
{
	Destroy();
}

void DnMeshComponent::Destroy(void)
{
	if (renderEntityHandle != -1)
	{
		gameRenderWorld->FreeEntityDef(renderEntityHandle);
		renderEntityHandle = -1;
	}
}

void DnMeshComponent::Init(idEntity* parent, idRenderModel* componentMesh)
{
	renderEntityParams.hModel = componentMesh;
	renderEntityHandle = gameRenderWorld->AddEntityDef(&renderEntityParams);
	parentEntity = parent;
}

void DnMeshComponent::BindToJoint(const char* jointName)
{
	idAnimatedEntity* animatedEntity = parentEntity->Cast<idAnimatedEntity>();
	if (animatedEntity == nullptr)
	{
		gameLocal.Error("You can't bind to a joint without the parent deriving from idAnimatedEntity!");
	}

	bindJoint = animatedEntity->GetAnimator()->GetJointHandle(jointName);
	if (bindJoint == -1)
	{
		gameLocal.Error("Failed to bind to joint %s", jointName);
	}
}

void DnMeshComponent::Think()
{
	if (renderEntityHandle == -1)
	{
		return;
	}

	if (bindJoint != -1)
	{
		idAnimatedEntity* animatedEntity = parentEntity->Cast<idAnimatedEntity>();
		if (animatedEntity == nullptr)
		{
			gameLocal.Error("You can't bind to a joint without the parent deriving from idAnimatedEntity!");
		}


		animatedEntity->GetJointWorldTransform(bindJoint, gameLocal.time, renderEntityParams.origin, renderEntityParams.axis);
	}
	else
	{
		renderEntityParams.origin = parentEntity->GetOrigin();
		renderEntityParams.axis = parentEntity->GetAxis();
	}

	gameRenderWorld->UpdateEntityDef(renderEntityHandle, &renderEntityParams);
}
