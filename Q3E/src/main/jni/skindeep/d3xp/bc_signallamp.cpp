

//this was some work on the carryable version of the signal lamp, but let's just use the weapon version instead

/*
#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

//#include "ai/AI.h"
#include "SmokeParticles.h"

#include "bc_signallamp.h"

#define LIGHT_RADIUS 64

CLASS_DECLARATION(idMoveableItem, idSignallamp)
END_CLASS

idSignallamp::idSignallamp(void)
{
    memset(&headlight, 0, sizeof(headlight));
    headlightHandle = -1;
}

idSignallamp::~idSignallamp(void)
{
    if (headlightHandle != -1)
        gameRenderWorld->FreeLightDef(headlightHandle);

    if (animatedEnt != nullptr)
        delete animatedEnt;

    BecomeInactive(TH_UPDATEPARTICLES);
}

void idSignallamp::Spawn(void)
{
	BecomeActive(TH_THINK);	
	thinkTimer = 0;

    //The animated display model.
    idDict args;
    args.Clear();
    args.SetVector("origin", GetPhysics()->GetOrigin());
    args.SetMatrix("rotation", GetPhysics()->GetAxis());
    args.Set("model", spawnArgs.GetString("model_display"));
    args.Set("bind", GetName());
    args.Set("anim", "idle_off");
    animatedEnt = (idAnimatedEntity*)gameLocal.SpawnEntityType(idAnimatedEntity::Type, &args);    
}

void idSignallamp::Save(idSaveGame *savefile) const
{
}

void idSignallamp::Restore(idRestoreGame *savefile)
{
}

void idSignallamp::Think(void)
{    
	idMoveableItem::Think();

    //Update light position.
    if (headlightHandle != -1)
    {
        idVec3 up, right;
        GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, &right, &up);
        headlight.origin = GetPhysics()->GetOrigin() + (up * 3.2f) + (right * -.7f);
        gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
    }



}

void idSignallamp::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
    KillLight();
    idMoveableItem::Killed(inflictor, attacker, damage, dir, location);
}

void idSignallamp::KillLight()
{
    if (headlightHandle != -1)
    {
        gameRenderWorld->FreeLightDef(headlightHandle);
        headlightHandle = -1;
    }
}

bool idSignallamp::DoFrob(int index, idEntity * frobber)
{
    common->Printf("tap %d\n", gameLocal.time);

    if (index == CARRYFROB_INDEX)
    {
        //player frob held lighter.

        return true;
    }

    KillLight();
    return idMoveableItem::DoFrob(index, frobber);
}

bool idSignallamp::DoFrobHold(int index, idEntity* frobber)
{
    common->Printf("hold %d\n", gameLocal.time);

    return idMoveableItem::DoFrobHold(index, frobber);
}

void idSignallamp::SetLightState(bool activate)
{
    if (activate)
    {
        animatedEnt->Event_PlayAnim("flick_on", 1);

        if (headlightHandle == -1)
        {
            //spawn light.
            headlight.shader = declManager->FindMaterial("lights/defaultProjectedLight", false);
            headlight.pointLight = true;
            headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = LIGHT_RADIUS;
            headlight.shaderParms[0] = .3f;
            headlight.shaderParms[1] = .15f;
            headlight.shaderParms[2] = 0;
            headlight.shaderParms[3] = 1.0f;
            headlight.noShadows = true;
            headlight.isAmbient = true;
            headlight.axis = mat3_identity;
            headlightHandle = gameRenderWorld->AddLightDef(&headlight);
        }
    }
    else
    {
        animatedEnt->Event_PlayAnim("flick_off", 1);

        KillLight();
    }
}




void idSignallamp::Hide(void)
{
    animatedEnt->Hide();
    idMoveableItem::Hide();
}

void idSignallamp::Show(void)
{
    animatedEnt->Show();
    idMoveableItem::Show();
}

*/