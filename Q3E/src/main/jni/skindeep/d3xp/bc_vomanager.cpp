
#include "Entity.h"
#include "Actor.h"
#include "Player.h"
#include "bc_vomanager.h"


//The VOManager handles when someone says a vo line.
idVOManager::idVOManager()
{
}

idVOManager::~idVOManager()
{
}

//lineCategory: see bc_vomanager.h
//#define VO_CATEGORY_DEATH		4
//#define VO_CATEGORY_HITREACTION	3
//#define VO_CATEGORY_NARRATIVE	2
//#define VO_CATEGORY_BARK		1 //gameplay voice bark "I see them!" "moving in!" etc
//#define VO_CATEGORY_GRUNT		0 //just a mouth sound, effort noise, breathing, etc
//returns time duration of VO.
int idVOManager::SayVO(idEntity *speaker, const char *soundname, int lineCategory)
{
	// if systemic VO is disabled for this entity and the line is *NOT* an important narrative line, skip it
	if (!speaker->systemicVoEnabled && lineCategory != VO_CATEGORY_NARRATIVE) 
		return 0;

	if (speaker->IsType(idActor::Type))
	{
		int lastSpeakTime = static_cast<idActor *>(speaker)->vo_lastSpeakTime;
		int lastCategory = static_cast<idActor *>(speaker)->vo_lastCategory;

		if (lastSpeakTime + 1000 >= gameLocal.time && lineCategory <= lastCategory)
			return 0;
	}

	bool playedVO = false;
	int len = 0;
	idStr soundStr = soundname;
	if (soundStr.IcmpPrefix("snd_") == 0)
	{
		playedVO = speaker->StartSound(soundname, SND_CHANNEL_VOICE, 0, false, &len);
	}
	else
	{
		playedVO = speaker->StartSoundShader(declManager->FindSound(soundname), SND_CHANNEL_VOICE, 0, false, &len);
	}

	if (len > 0 && playedVO)
	{
		//successfully said vo.
		if (cvarSystem->GetCVarBool("s_voDebug"))
		{
			//Debug.
			common->Printf("VO [%d][%s]: '%s'\n", gameLocal.time, speaker->GetName(), soundname);

			if (cvarSystem->GetCVarInteger("s_voDebug") >= 2)
			{
				#define		POS_RANDVARIANCE 3
				idVec3 drawPos = idVec3(speaker->GetPhysics()->GetOrigin().x, speaker->GetPhysics()->GetOrigin().y, speaker->GetPhysics()->GetAbsBounds()[1].z);				
				drawPos.x += gameLocal.random.RandomInt(-POS_RANDVARIANCE, POS_RANDVARIANCE);
				drawPos.y += gameLocal.random.RandomInt(-POS_RANDVARIANCE, POS_RANDVARIANCE);
				drawPos.z += gameLocal.random.RandomInt(-POS_RANDVARIANCE, POS_RANDVARIANCE);
				gameRenderWorld->DrawText(soundname, drawPos, .2f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 10000);
			}
		}

		if (speaker->IsType(idActor::Type))
		{
			static_cast<idActor *>(speaker)->vo_lastSpeakTime = gameLocal.time;
			static_cast<idActor *>(speaker)->vo_lastCategory = lineCategory;

			idStr speakParticle = speaker->spawnArgs.GetString("model_sound");
			if (speakParticle.Length() > 0)
			{
				idStr soundwavejoint = speaker->spawnArgs.GetString("soundwave_joint", "head");
				jointHandle_t headJoint = static_cast<idActor*>(speaker)->GetAnimator()->GetJointHandle(soundwavejoint.c_str());
				if (headJoint != INVALID_JOINT)
				{
					idVec3 headPos;
					idMat3 headAxis;
					speaker->GetAnimator()->GetJointTransform(headJoint, gameLocal.time, headPos, headAxis);
					headPos = speaker->GetRenderEntity()->origin + headPos * speaker->GetRenderEntity()->axis;

					idEntity* particle = gameLocal.DoParticle(speakParticle, headPos);					
					if (particle)
					{
						particle->BindToJoint(speaker, headJoint, false);
					}
				}
			}
		}
	}
	else
	{
		gameLocal.Warning("'%s' failed to play VO '%s'\n", speaker->GetName(), soundname);
	}

	return len;
}
