/***********************************************************************
	hhModelToggle

  Usage:
	Set target to a hhViewedModel entity that will change model.

  operations:
	next/prev model
	next/prev anim
	toggle rotation
	toggle translation
	toggle cycle

***********************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


// hhViewedModel -----------------------------------------------------------

CLASS_DECLARATION(hhAnimatedEntity, hhViewedModel)
END_CLASS

void hhViewedModel::Spawn(void) {

	physicsObj.SetSelf(this);
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	SetPhysics( &physicsObj );

	rotationAmount = 0;
	BecomeActive(TH_THINK|TH_TICKER);
}

void hhViewedModel::SetRotationAmount(float amount) {
	rotationAmount = amount;
}

void hhViewedModel::Ticker() {
	idAngles ang;

	// update rotation
	physicsObj.GetAngles( ang );
	physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_LINEAR|EXTRAPOLATION_NOSTOP), gameLocal.time, gameLocal.msec, ang, idAngles( 0, rotationAmount * 360.0f / 60.0f, 0 ), ang_zero );

	// update visuals so that the skeleton is drawn for non-rotating models
	UpdateVisuals();
}


// hhModelToggle ------------------------------------------------------------

const idEventDef EV_SetInitialAnims("<setanims>", NULL);
const idEventDef EV_RequireTargets("<requiretargets>", NULL);

CLASS_DECLARATION(hhConsole, hhModelToggle)
	EVENT( EV_SetInitialAnims,		hhModelToggle::Event_SetInitialAnims )
	EVENT( EV_RequireTargets,		hhModelToggle::Event_RequireTargets )
END_CLASS


void hhModelToggle::Spawn(void) {
	const idKeyValue *arg;
	int i, num;

	bTranslation = false;
	bRotation = false;
	bCycle = true;

	// Retrieve list of definitions
	defList.Clear();
	num = spawnArgs.GetNumKeyVals();
	for( i = 0; i < num; i++ ) {
		arg = spawnArgs.GetKeyVal( i );
		if ( !arg->GetKey().Icmpn( "defentity", 9 ) ) {
			defList.Append(arg->GetValue());
		}
	}

	FillResourceList();
	BecomeActive(TH_THINK);

	PostEventMS(&EV_RequireTargets, 0);		// Target list not built until after Spawn()
	PostEventMS(&EV_SetInitialAnims, 0);		// Can't set anims in Spawn()
}

void hhModelToggle::FillResourceList() {
	const idDict *dict;
	ResourceSet set;
	int i;
	const idKeyValue *kv;

	for (i=0; i<defList.Num(); i++) {
		dict = gameLocal.FindEntityDefDict( defList[i].c_str(), false );
		if (!dict) {
			gameLocal.Warning("Unknown entity definition in model viewer: %s", defList[i].c_str());
			continue;
		}

		kv = dict->FindKey("model_view");
		if (!kv) {
			kv = dict->FindKey("model");
			if (!kv) {
				gameLocal.Warning("No model for %s", defList[i].c_str());
				continue;
			}
		}

		set.model = kv->GetValue();
		set.animList.Clear();
		set.animFileList.Clear();
		set.args = dict;

		const idDeclModelDef *modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, kv->GetValue(), false ) );
		if (modelDef) {
			int num = modelDef->NumAnims();
			for (int i=1; i<num; i++) {
				const idAnim *anim = modelDef->GetAnim(i);
				idStr animName = anim->FullName();
				set.animList.Append(animName);
				idStr animFileName = anim->MD5Anim(0)->Name();
				set.animFileList.Append(animFileName);
			}
		}
		else {
			kv = NULL;
			while (1) {
				kv = dict->MatchPrefix("anim ", kv);
				if (!kv) {
					break;
				}
				idStr animName = kv->GetKey().c_str() + 5;
				set.animList.Append(animName);
				set.animFileList.Append(kv->GetValue());
			}
		}

		if (set.animList.Num() == 0) {
			gameLocal.Warning("No anims for %s", defList[i].c_str());
			continue;
		}

		resources.Append(set);
	}
}

void hhModelToggle::UpdateGUIValues() {
	if (renderEntity.gui[0]) {
		renderEntity.gui[0]->SetStateString("modelname", resources[currentModel].model.c_str());
		if (resources[currentModel].animList.Num() > 0) {
			renderEntity.gui[0]->SetStateString("animname", resources[currentModel].animList[currentAnim].c_str());
			renderEntity.gui[0]->SetStateString("animfile", resources[currentModel].animFileList[currentAnim].c_str());
		}
		renderEntity.gui[0]->SetStateInt("translation", bTranslation);
		renderEntity.gui[0]->SetStateInt("rotation", bRotation);
		renderEntity.gui[0]->SetStateInt("cycle", bCycle);
	}
}

void hhModelToggle::SetTargetsToModel(const char *modelname) {
	for (int t=0; t<targets.Num(); t++) {
		idEntity *ent = targets[t].GetEntity();
		if (ent) {
			ent->SetModel(modelname);
		}
	}

	UpdateGUIValues();
}

void hhModelToggle::SetTargetsToAnim(const char *animname) {
	for (int t=0; t<targets.Num(); t++) {
		idEntity *ent = targets[t].GetEntity();
		if (ent) {
			int anim = ent->GetAnimator()->GetAnim(animname);

			ent->GetAnimator()->ClearAllAnims( gameLocal.time, 0 );

			if (bCycle) {
				ent->GetAnimator()->CycleAnim(ANIMCHANNEL_ALL, anim, gameLocal.time, 0);
			}
			else {
				ent->GetAnimator()->PlayAnim(ANIMCHANNEL_ALL, anim, gameLocal.time, 0);
			}

			ent->GetAnimator()->RemoveOriginOffset(!bTranslation);
		}
	}

	UpdateGUIValues();
}

void hhModelToggle::SetTargetsToRotate(bool bRotate) {
	for (int t=0; t<targets.Num(); t++) {
		idEntity *ent = targets[t].GetEntity();
		if (ent) {
			// Cast to viewedModel and set rotation
			if (ent->IsType(hhViewedModel::Type)) {
				static_cast<hhViewedModel*>(ent)->SetRotationAmount(bRotate?5:0);
			}
		}
	}

	UpdateGUIValues();
}


void hhModelToggle::NextModel(void) {
	if (resources.Num() > 0) {
		currentModel = (currentModel + 1) % resources.Num();
		currentAnim = 0;

		SetTargetsToModel(resources[currentModel].model.c_str());
		SetTargetsToAnim(resources[currentModel].animList[currentAnim].c_str());
	}
}

void hhModelToggle::PrevModel(void) {
	if (resources.Num() > 0) {
		currentModel = (currentModel - 1 + resources.Num()) % resources.Num();
		currentAnim = 0;

		SetTargetsToModel(resources[currentModel].model.c_str());
		SetTargetsToAnim(resources[currentModel].animList[currentAnim].c_str());
	}
}

void hhModelToggle::NextAnim(void) {
	int numanims = resources[currentModel].animList.Num();
	if (numanims > 0) {
		currentAnim = (currentAnim + 1) % numanims;
		SetTargetsToAnim(resources[currentModel].animList[currentAnim].c_str());
	}
}

void hhModelToggle::PrevAnim(void) {
	int numanims = resources[currentModel].animList.Num();
	if (numanims > 0) {
		currentAnim = (currentAnim - 1 + numanims) % numanims;
		SetTargetsToAnim(resources[currentModel].animList[currentAnim].c_str());
	}
}

bool hhModelToggle::HandleSingleGuiCommand(idEntity *entityGui, idLexer *src) {

	idToken token;

	if (!src->ReadToken(&token)) {
		return false;
	}

	if (token == ";") {
		return false;
	}

	if (token.Icmp("nextmodel") == 0) {
		NextModel();
		return true;
	}
	else if (token.Icmp("prevmodel") == 0) {
		PrevModel();
		return true;
	}
	else if (token.Icmp("nextanim") == 0) {
		NextAnim();
		return true;
	}
	else if (token.Icmp("prevanim") == 0) {
		PrevAnim();
		return true;
	}
	else if (token.Icmp("togglerotation") == 0) {
		// Instead, have gui call a script function and handle rotation with a mover
		bRotation ^= 1;
		SetTargetsToRotate(bRotation);
		return true;
	}
	else if (token.Icmp("toggletranslation") == 0) {
		bTranslation ^= 1;
		SetTargetsToModel(resources[currentModel].model.c_str());
		SetTargetsToAnim(resources[currentModel].animList[currentAnim].c_str());
		return true;
	}
	else if (token.Icmp("togglecycle") == 0) {
		bCycle ^= 1;
		SetTargetsToModel(resources[currentModel].model.c_str());
		SetTargetsToAnim(resources[currentModel].animList[currentAnim].c_str());
		return true;
	}

	src->UnreadToken(&token);
	return false;
}

void hhModelToggle::Event_SetInitialAnims() {
	// Set to currentModel, currentAnim
	currentModel = currentAnim = 0;
	if (resources.Num() > 0) {
		SetTargetsToModel(resources[currentModel].model.c_str());
		if (resources[currentModel].animList.Num() > 0) {
			SetTargetsToAnim(resources[currentModel].animList[currentAnim].c_str());
		}
	}
}

void hhModelToggle::Event_RequireTargets() {
	// Require a valid target: must be delayed after spawn because targets aren't build until then
	if (!targets.Num()) {
		gameLocal.Error( "ModelViewer requires a valid target." );
		PostEventMS(&EV_Remove, 0);
	}
}

