#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

//#include "ai/AI.h"
#include "SmokeParticles.h"

#include "bc_lighter.h"

const int LINGER_ON_TIME = 3000; //after lighter hits floor, how long to stay on before flaming out.
const float LIGHT_RADIUS = 32;

CLASS_DECLARATION(idMoveableItem, idLighter)
END_CLASS

idLighter::idLighter(void)
{
    memset(&headlight, 0, sizeof(headlight));
    headlightHandle = -1;
}

idLighter::~idLighter(void)
{
    if (headlightHandle != -1)
        gameRenderWorld->FreeLightDef(headlightHandle);

    if (animatedEnt != nullptr)
        delete animatedEnt;

    BecomeInactive(TH_UPDATEPARTICLES);
}

void idLighter::Spawn(void)
{
	BecomeActive(TH_THINK);	
	thinkTimer = 0;

    lighterState = LTR_OFF;

    trailParticles = NULL;
    trailParticlesFlyTime = 0;


    idDict args;
    args.Clear();
    args.SetVector("origin", GetPhysics()->GetOrigin());
    args.SetMatrix("rotation", GetPhysics()->GetAxis());
    args.Set("model", spawnArgs.GetString("model_display"));
    //args.Set("bind", GetName()); //BC 3-6-2025: removed this, see note a few lines below...
    args.Set("anim", "idle_off");
    animatedEnt = (idAnimatedEntity*)gameLocal.SpawnEntityType(idAnimatedEntity::Type, &args);    
    if (animatedEnt)
    {
        animatedEnt->Bind(this, true); //BC 3-6-2025: bind the object via bind() instead of using the spawnarg, as the lost and found makes object invisible if we don't do this.
    }
    
}

void idLighter::Save(idSaveGame *savefile) const
{
    savefile->WriteRenderLight( headlight ); // renderLight_t headlight
    savefile->WriteInt( headlightHandle ); // int headlightHandle

    savefile->WriteParticle( trailParticles ); // const idDeclParticle * trailParticles
    savefile->WriteInt( trailParticlesFlyTime ); // int trailParticlesFlyTime

    savefile->WriteInt( thinkTimer ); // int thinkTimer

    savefile->WriteInt( lighterState ); // int lighterState

    savefile->WriteObject( animatedEnt ); // idAnimatedEntity* animatedEnt
    savefile->WriteVec3( lastPosition ); // idVec3 lastPosition
}

void idLighter::Restore(idRestoreGame *savefile)
{
    savefile->ReadRenderLight( headlight ); // renderLight_t headlight
    savefile->ReadInt( headlightHandle ); // int headlightHandle
    if ( headlightHandle != - 1 ) {
        gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
    }

    savefile->ReadParticle( trailParticles ); // const idDeclParticle * trailParticles
    savefile->ReadInt( trailParticlesFlyTime ); // int trailParticlesFlyTime

    savefile->ReadInt( thinkTimer ); // int thinkTimer

    savefile->ReadInt( lighterState ); // int lighterState

    savefile->ReadObject( CastClassPtrRef(animatedEnt) ); // idAnimatedEntity* animatedEnt
    savefile->ReadVec3( lastPosition ); // idVec3 lastPosition
}

void idLighter::Think(void)
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

	//Don't do proximity check if I'm being held by the player.
	if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
	{
		if (gameLocal.GetLocalPlayer()->GetCarryable() == this)
		{
			return;
		}
	}

    if (lighterState == LTR_ON)
    {
        if (GetPhysics()->IsAtRest())
        {
            //short delay before I turn off my flame.
            lighterState = LTR_ON_LINGER;
            thinkTimer = gameLocal.time + LINGER_ON_TIME;

            StopSound(SND_CHANNEL_BODY3);
        }
    }
    else if (lighterState == LTR_ON_LINGER)
    {
        if (gameLocal.time > thinkTimer)
        {
            //lighter has rested on floor for a while. turn off flame.
            SetLightState(false);

			spawnArgs.SetBool("isfire", false);
        }
    }



    //Update particle trail.
    if (trailParticles != NULL && trailParticlesFlyTime && (lighterState == LTR_ON || lighterState == LTR_ON_LINGER) && lastPosition != GetPhysics()->GetOrigin())
    {
        lastPosition = GetPhysics()->GetOrigin();

        idVec3 dir = idVec3(0, 1, 0);
        idVec3 moveDir = GetPhysics()->GetLinearVelocity();
        moveDir.NormalizeFast();

        idVec3 up;
        GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &up);
        idVec3 particlePos = GetPhysics()->GetOrigin() + (up * 2);
        if (!gameLocal.smokeParticles->EmitSmoke(trailParticles, trailParticlesFlyTime, gameLocal.random.RandomFloat(),  particlePos, moveDir.ToMat3(), timeGroup))
        {
            trailParticlesFlyTime = gameLocal.time;
        }
    }
}

void idLighter::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
    KillLight();
    idMoveableItem::Killed(inflictor, attacker, damage, dir, location);
}

void idLighter::KillLight()
{
    if (headlightHandle != -1)
    {
        gameRenderWorld->FreeLightDef(headlightHandle);
        headlightHandle = -1;
    }

    trailParticles = NULL;
    trailParticlesFlyTime = 0;
    BecomeInactive(TH_UPDATEPARTICLES);

	//stop the rocket sound.
	StopSound(SND_CHANNEL_BODY3);
}

bool idLighter::DoFrob(int index, idEntity * frobber)
{
    if (index == CARRYFROB_INDEX)
    {
        //player frob held lighter.        
        SetLightState((lighterState == LTR_OFF)); //flick the lighter.        

        return true;
    }
    else
    {
        //picking up lighter in the world.
        spawnArgs.SetBool("isfire", false); //BC 5-8-2025: Force its "fire" state to be off, so that the player isn't igniting things by just carrying the lighter around.
    }

    KillLight();
    return idMoveableItem::DoFrob(index, frobber);
}

void idLighter::SetLightState(bool activate)
{
    if (activate)
    {
        lighterState = LTR_ON;
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
        lighterState = LTR_OFF;
        animatedEnt->Event_PlayAnim("flick_off", 1);

        KillLight();
    }
}

void idLighter::JustThrown()
{
    SetLightState(true);
    

    //particle trail
    const char *smokeName = spawnArgs.GetString("smoke_fly");
    trailParticlesFlyTime = 0;
    if (*smokeName != '\0')
    {
        trailParticles = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE, smokeName));
        trailParticlesFlyTime = gameLocal.time;
    }

	StartSound("snd_fly", SND_CHANNEL_BODY3);

	spawnArgs.SetBool("isfire", true);
}


void idLighter::Hide(void)
{
    animatedEnt->Hide();
    idMoveableItem::Hide();
}

void idLighter::Show(void)
{
    animatedEnt->Show();
    idMoveableItem::Show();
}