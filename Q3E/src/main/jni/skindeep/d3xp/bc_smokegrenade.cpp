#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"

#include "WorldSpawn.h"
#include "bc_smokegrenade.h"

#define SMOKE_SPEWDELAY         2000    //delay before first smoke spew.
#define SMOKE_SPEWINTERVAL      500     //how long before smoke spews.
#define SMOKE_SPEWTOTALTIME     15000   //total time that smoke spews out.

#define SPEW_RADIUS             80      //how far do we try to spawn smoke clouds.

#define INFLICT_INTERVAL        300
#define INFLICT_DISTANCE        120

CLASS_DECLARATION(idMoveableItem, idSmokegrenade)
END_CLASS

void idSmokegrenade::Save(idSaveGame *savefile) const
{
    savefile->WriteObject( nozzleParticle ); // idFuncEmitter * nozzleParticle
    savefile->WriteInt( spewtimer ); // int spewtimer
    savefile->WriteInt( state ); // int state
    savefile->WriteInt( stateTimer ); // int stateTimer
    savefile->WriteInt( intervalTimer ); // int intervalTimer
    savefile->WriteInt( inflictTimer ); // int inflictTimer
}

void idSmokegrenade::Restore(idRestoreGame *savefile)
{
    savefile->ReadObject( CastClassPtrRef(nozzleParticle) ); // idFuncEmitter * nozzleParticle
    savefile->ReadInt( spewtimer ); // int spewtimer
    savefile->ReadInt( state ); // int state
    savefile->ReadInt( stateTimer ); // int stateTimer
    savefile->ReadInt( intervalTimer ); // int intervalTimer
    savefile->ReadInt( inflictTimer ); // int inflictTimer
}

void idSmokegrenade::Spawn(void)
{
	//Allow player to clip through it.
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL | CONTENTS_TRIGGER);
	physicsObj.SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);
	showItemLine = false;    
    
    idVec3 emitterPosition = GetPhysics()->GetOrigin() + (GetPhysics()->GetAxis().ToAngles().ToForward() * 6);
    idAngles particleAngle;
    particleAngle = GetPhysics()->GetAxis().ToAngles();
    particleAngle.pitch += 90;
    idDict args;
    args.Clear();
    args.Set("model", spawnArgs.GetString("model_nozzleprt"));
    args.SetVector("origin", emitterPosition);
    args.SetMatrix("rotation", particleAngle.ToMat3());
    //args.SetBool("start_off", true);
    nozzleParticle = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
    nozzleParticle->Bind(this, true);
    nozzleParticle->SetActive(false);
    nozzleParticle->Hide();

    

    state = SG_SMOKEDELAY;
    stateTimer = gameLocal.time + SMOKE_SPEWDELAY;    
    intervalTimer = 0;
    inflictTimer = 0;

    BecomeActive(TH_THINK);
}

void idSmokegrenade::Think(void)
{
    idMoveableItem::Think();

    if (state == SG_SMOKEDELAY)
    {
        if (gameLocal.time > stateTimer)
        {
            //start spewing smoke.
            state = SG_SMOKING;
            nozzleParticle->Show();
            nozzleParticle->SetActive(true);
            stateTimer = gameLocal.time + SMOKE_SPEWTOTALTIME;
            StartSound("snd_hiss", SND_CHANNEL_ANY);

			//When it starts spewing, do a little jump.
			idVec3 forwardDir = GetPhysics()->GetAxis().ToAngles().ToForward();
			GetPhysics()->ApplyImpulse(0, GetPhysics()->GetOrigin(), (forwardDir * -128) + idVec3(0, 0, 64));

			GetPhysics()->SetAngularVelocity(idVec3(0, -64, 0));
        }
    }
    else if (state == SG_SMOKING)
    {
        if (gameLocal.time > intervalTimer)
        {
            intervalTimer = gameLocal.time + SMOKE_SPEWINTERVAL;
            DoRandomSmokeSpew();
        }


        if (gameLocal.time > stateTimer)
        {
			displayName = spawnArgs.GetString("displayname_empty");

            state = SG_DONE;
            nozzleParticle->SetActive(false);
            StopSound(SND_CHANNEL_ANY);

            //Can create interestpoints now.
            spawnArgs.Set("interest_multibounce", "interest_itembounce");
            spawnArgs.Set("interest_multibounce2", "interest_itembounce_visual");
        }

        if (gameLocal.time > inflictTimer)
        {
            inflictTimer = gameLocal.time + INFLICT_INTERVAL;

            //see if player is near the smoke grenade.

            
            //Do cough effect for player.
            if (gameLocal.GetLocalPlayer()->health > 0
                /*&& !gameLocal.GetLocalPlayer()->fl.notarget*/
                && !gameLocal.GetLocalPlayer()->noclip
                /*&& !gameLocal.GetLocalPlayer()->cond_gascloud*/)
            {
                float distance = (gameLocal.GetLocalPlayer()->GetEyePosition() - this->GetPhysics()->GetOrigin()).Length();

                if (distance < INFLICT_DISTANCE)
                {
                    //player is close to a smoke grenade.
                    gameLocal.GetLocalPlayer()->SetGascloudState(true);
                }
            }
        }
    }
}

void idSmokegrenade::DoRandomSmokeSpew()
{
    idVec3 randomPos = GetPhysics()->GetOrigin();
    randomPos.x += gameLocal.random.RandomInt(-SPEW_RADIUS, SPEW_RADIUS);
    randomPos.y += gameLocal.random.RandomInt(-SPEW_RADIUS, SPEW_RADIUS);
    randomPos.z += gameLocal.random.RandomInt((int)(SPEW_RADIUS * .1f), SPEW_RADIUS);

    trace_t tr;
    gameLocal.clip.TracePoint(tr, GetPhysics()->GetOrigin(), randomPos, MASK_SOLID, NULL);

    idVec3 smokePos;
    if (tr.fraction >= 1)
    {
        smokePos = tr.endpos;
    }
    else
    {
        float distToEndpos = (randomPos - GetPhysics()->GetOrigin()).Length();

        idVec3 dirToEndpos = randomPos - GetPhysics()->GetOrigin();
        dirToEndpos.Normalize();
        smokePos = tr.endpos + (dirToEndpos * (distToEndpos/2));
    }

    gameLocal.DoParticle(spawnArgs.GetString("model_cloudprt"), smokePos);
}

void idSmokegrenade::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	idMoveableItem::Killed(inflictor, attacker, damage, dir, location);
}


bool idSmokegrenade::DoFrob(int index, idEntity * frobber)
{
    return idMoveableItem::DoFrob(index, frobber);
}
