#include "../Gamelib/Game_local.h"

/***********************************************************************

	idDeclModelDef

***********************************************************************/

/*
=====================
idDeclModelDef::idDeclModelDef
=====================
*/
idDeclModelDef::idDeclModelDef() {
	modelHandle = NULL;
	skin = NULL;
	offset.Zero();
	for (int i = 0; i < ANIM_NumAnimChannels; i++) {
		channelJoints[i].Clear();
	}
}

/*
=====================
idDeclModelDef::~idDeclModelDef
=====================
*/
idDeclModelDef::~idDeclModelDef() {
	FreeData();
}

/*
=================
idDeclModelDef::Size
=================
*/
size_t idDeclModelDef::Size(void) const {
	return sizeof(idDeclModelDef);
}

/*
=====================
idDeclModelDef::CopyDecl
=====================
*/
void idDeclModelDef::CopyDecl(const idDeclModelDef* decl) {
	int i;

	FreeData();

	offset = decl->offset;
	modelHandle = decl->modelHandle;
	skin = decl->skin;

	anims.SetNum(decl->anims.Num());
	for (i = 0; i < anims.Num(); i++) {
		anims[i] = new idAnim(this, decl->anims[i]);
	}

	joints.SetNum(decl->joints.Num());
	memcpy(joints.Ptr(), decl->joints.Ptr(), decl->joints.Num() * sizeof(joints[0]));
	jointParents.SetNum(decl->jointParents.Num());
	memcpy(jointParents.Ptr(), decl->jointParents.Ptr(), decl->jointParents.Num() * sizeof(jointParents[0]));
	for (i = 0; i < ANIM_NumAnimChannels; i++) {
		channelJoints[i] = decl->channelJoints[i];
	}
}

/*
=====================
idDeclModelDef::FreeData
=====================
*/
void idDeclModelDef::FreeData(void) {
	anims.DeleteContents(true);
	joints.Clear();
	jointParents.Clear();
	modelHandle = NULL;
	skin = NULL;
	offset.Zero();
	for (int i = 0; i < ANIM_NumAnimChannels; i++) {
		channelJoints[i].Clear();
	}
}

/*
================
idDeclModelDef::DefaultDefinition
================
*/
const char* idDeclModelDef::DefaultDefinition(void) const {
	return "{ }";
}

/*
====================
idDeclModelDef::FindJoint
====================
*/
const jointInfo_t* idDeclModelDef::FindJoint(const char* name) const {
	int					i;
	const idMD5Joint* joint;

	if (!modelHandle) {
		return NULL;
	}

	joint = modelHandle->GetJoints();
	for (i = 0; i < joints.Num(); i++, joint++) {
		if (!joint->name.Icmp(name)) {
			return &joints[i];
		}
	}

	return NULL;
}

/*
=====================
idDeclModelDef::ModelHandle
=====================
*/
idRenderModel* idDeclModelDef::ModelHandle(void) const {
	return (idRenderModel*)modelHandle;
}

/*
=====================
idDeclModelDef::GetJointList
=====================
*/
void idDeclModelDef::GetJointList(const char* jointnames, idList<jointHandle_t>& jointList) const {
	const char* pos;
	idStr				jointname;
	const jointInfo_t* joint;
	const jointInfo_t* child;
	int					i;
	int					num;
	bool				getChildren;
	bool				subtract;

	if (!modelHandle) {
		return;
	}

	jointList.Clear();

	num = modelHandle->NumJoints();

	// scan through list of joints and add each to the joint list
	pos = jointnames;
	while (*pos) {
		// skip over whitespace
		while ((*pos != 0) && isspace(*pos)) {
			pos++;
		}

		if (!*pos) {
			// no more names
			break;
		}

		// copy joint name
		jointname = "";

		if (*pos == '-') {
			subtract = true;
			pos++;
		}
		else {
			subtract = false;
		}

		if (*pos == '*') {
			getChildren = true;
			pos++;
		}
		else {
			getChildren = false;
		}

		while ((*pos != 0) && !isspace(*pos)) {
			jointname += *pos;
			pos++;
		}

		joint = FindJoint(jointname);
		if (!joint) {
			gameLocal.Warning("Unknown joint '%s' in '%s' for model '%s'", jointname.c_str(), jointnames, GetName());
			continue;
		}

		if (!subtract) {
			jointList.AddUnique(joint->num);
		}
		else {
			jointList.Remove(joint->num);
		}

		if (getChildren) {
			// include all joint's children
			child = joint + 1;
			for (i = joint->num + 1; i < num; i++, child++) {
				// all children of the joint should follow it in the list.
				// once we reach a joint without a parent or with a parent
				// who is earlier in the list than the specified joint, then
				// we've gone through all it's children.
				if (child->parentNum < joint->num) {
					break;
				}

				if (!subtract) {
					jointList.AddUnique(child->num);
				}
				else {
					jointList.Remove(child->num);
				}
			}
		}
	}
}

/*
=====================
idDeclModelDef::Touch
=====================
*/
void idDeclModelDef::Touch(void) const {
	if (modelHandle) {
		renderModelManager->FindModel(modelHandle->Name());
	}
}

/*
=====================
idDeclModelDef::GetDefaultSkin
=====================
*/
const idDeclSkin* idDeclModelDef::GetDefaultSkin(void) const {
	return skin;
}

/*
=====================
idDeclModelDef::GetDefaultPose
=====================
*/
const idJointQuat* idDeclModelDef::GetDefaultPose(void) const {
	return modelHandle->GetDefaultPose();
}

/*
=====================
idDeclModelDef::SetupJoints
=====================
*/
void idDeclModelDef::SetupJoints(int* numJoints, idJointMat** jointList, idBounds& frameBounds, bool removeOriginOffset) const {
	int					num;
	const idJointQuat* pose;
	idJointMat* list;

	if (!modelHandle || modelHandle->IsDefaultModel()) {
		Mem_Free16((*jointList));
		(*jointList) = NULL;
		frameBounds.Clear();
		return;
	}

	// get the number of joints
	num = modelHandle->NumJoints();

	if (!num) {
		gameLocal.Error("model '%s' has no joints", modelHandle->Name());
	}

	// set up initial pose for model (with no pose, model is just a jumbled mess)
	list = (idJointMat*)Mem_Alloc16(num * sizeof(list[0]));
	pose = GetDefaultPose();

	// convert the joint quaternions to joint matrices
	SIMDProcessor->ConvertJointQuatsToJointMats(list, pose, joints.Num());

	// check if we offset the model by the origin joint
	if (removeOriginOffset) {
#ifdef VELOCITY_MOVE
		list[0].SetTranslation(idVec3(offset.x, offset.y + pose[0].t.y, offset.z + pose[0].t.z));
#else
		list[0].SetTranslation(offset);
#endif
	}
	else {
		list[0].SetTranslation(pose[0].t + offset);
	}

	// transform the joint hierarchy
	SIMDProcessor->TransformJoints(list, jointParents.Ptr(), 1, joints.Num() - 1);

	*numJoints = num;
	*jointList = list;

	// get the bounds of the default pose
	frameBounds = modelHandle->Bounds(NULL);
}

/*
=====================
idDeclModelDef::ParseAnim
=====================
*/
bool idDeclModelDef::ParseAnim(idLexer& src, int numDefaultAnims) {
	int				i;
	int				len;
	idAnim* anim;
	const idMD5Anim* md5anims[ANIM_MaxSyncedAnims];
	const idMD5Anim* md5anim;
	idStr			alias;
	idToken			realname;
	idToken			token;
	int				numAnims;
	animFlags_t		flags;

	numAnims = 0;
	memset(md5anims, 0, sizeof(md5anims));

	if (!src.ReadToken(&realname)) {
		src.Warning("Unexpected end of file");
		MakeDefault();
		return false;
	}
	alias = realname;

	for (i = 0; i < anims.Num(); i++) {
		if (!strcmp(anims[i]->FullName(), realname)) {
			break;
		}
	}

	if ((i < anims.Num()) && (i >= numDefaultAnims)) {
		src.Warning("Duplicate anim '%s'", realname.c_str());
		MakeDefault();
		return false;
	}

	if (i < numDefaultAnims) {
		anim = anims[i];
	}
	else {
		// create the alias associated with this animation
		anim = new idAnim();
		anims.Append(anim);
	}

	// random anims end with a number.  find the numeric suffix of the animation.
	len = alias.Length();
	for (i = len - 1; i > 0; i--) {
		if (!isdigit(alias[i])) {
			break;
		}
	}

	// check for zero length name, or a purely numeric name
	if (i <= 0) {
		src.Warning("Invalid animation name '%s'", alias.c_str());
		MakeDefault();
		return false;
	}

	// remove the numeric suffix
	alias.CapLength(i + 1);

	// parse the anims from the string
	do {
		if (!src.ReadToken(&token)) {
			src.Warning("Unexpected end of file");
			MakeDefault();
			return false;
		}

		// lookup the animation
		md5anim = animationLib.GetAnim(token, ModelHandle());
		if (!md5anim) {
			src.Warning("Couldn't load anim '%s'", token.c_str());
			MakeDefault();
			return false;
		}

		md5anim->CheckModelHierarchy(modelHandle);

		if (numAnims > 0) {
			// make sure it's the same length as the other anims
			if (md5anim->Length() != md5anims[0]->Length()) {
				src.Warning("Anim '%s' does not match length of anim '%s'", md5anim->Name(), md5anims[0]->Name());
				MakeDefault();
				return false;
			}
		}

		if (numAnims >= ANIM_MaxSyncedAnims) {
			src.Warning("Exceeded max synced anims (%d)", ANIM_MaxSyncedAnims);
			MakeDefault();
			return false;
		}

		// add it to our list
		md5anims[numAnims] = md5anim;
		numAnims++;
	} while (src.CheckTokenString(","));

	if (!numAnims) {
		src.Warning("No animation specified");
		MakeDefault();
		return false;
	}

	anim->SetAnim(this, realname, alias, numAnims, md5anims);
	memset(&flags, 0, sizeof(flags));

	// parse any frame commands or animflags
	if (src.CheckTokenString("{")) {
		while (1) {
			if (!src.ReadToken(&token)) {
				src.Warning("Unexpected end of file");
				MakeDefault();
				return false;
			}
			if (token == "}") {
				break;
			}
			else if (token == "prevent_idle_override") {
				flags.prevent_idle_override = true;
			}
			else if (token == "random_cycle_start") {
				flags.random_cycle_start = true;
			}
			else if (token == "ai_no_turn") {
				flags.ai_no_turn = true;
			}
			else if (token == "anim_turn") {
				flags.anim_turn = true;
			}
			else if (token == "frame") {
				// create a frame command
				int			framenum;
				const char* err;

				// make sure we don't have any line breaks while reading the frame command so the error line # will be correct
				if (!src.ReadTokenOnLine(&token)) {
					src.Warning("Missing frame # after 'frame'");
					MakeDefault();
					return false;
				}
				if (token.type == TT_PUNCTUATION && token == "-") {
					src.Warning("Invalid frame # after 'frame'");
					MakeDefault();
					return false;
				}
				else if (token.type != TT_NUMBER || token.subtype == TT_FLOAT) {
					src.Error("expected integer value, found '%s'", token.c_str());
				}

				// get the frame number
				framenum = token.GetIntValue();

				// put the command on the specified frame of the animation
				err = anim->AddFrameCommand(this, framenum, src, NULL);
				if (err) {
					src.Warning("%s", err);
					MakeDefault();
					return false;
				}
			}
			else {
				src.Warning("Unknown command '%s'", token.c_str());
				MakeDefault();
				return false;
			}
		}
	}

	// set the flags
	anim->SetAnimFlags(flags);
	return true;
}

/*
================
idDeclModelDef::Parse
================
*/
bool idDeclModelDef::Parse(const char* text, const int textLength) {
	int					i;
	int					num;
	idStr				filename;
	idStr				extension;
	const idMD5Joint* md5joint;
	const idMD5Joint* md5joints;
	idLexer				src;
	idToken				token;
	idToken				token2;
	idStr				jointnames;
	int					channel;
	jointHandle_t		jointnum;
	idList<jointHandle_t> jointList;
	int					numDefaultAnims;

	src.LoadMemory(text, textLength, GetFileName(), GetLineNum());
	src.SetFlags(DECL_LEXER_FLAGS);
	src.SkipUntilString("{");

	numDefaultAnims = 0;
	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (token == "inherit") {
			if (!src.ReadToken(&token2)) {
				src.Warning("Unexpected end of file");
				MakeDefault();
				return false;
			}

			const idDeclModelDef* copy = static_cast<const idDeclModelDef*>(declManager->FindType(DECL_MODELDEF, token2, false));
			if (!copy) {
				common->Warning("Unknown model definition '%s'", token2.c_str());
			}
			else if (copy->GetState() == DS_DEFAULTED) {
				common->Warning("inherited model definition '%s' defaulted", token2.c_str());
				MakeDefault();
				return false;
			}
			else {
				CopyDecl(copy);
				numDefaultAnims = anims.Num();
			}
		}
		else if (token == "skin") {
			if (!src.ReadToken(&token2)) {
				src.Warning("Unexpected end of file");
				MakeDefault();
				return false;
			}
			skin = declManager->FindSkin(token2);
			if (!skin) {
				src.Warning("Skin '%s' not found", token2.c_str());
				MakeDefault();
				return false;
			}
		}
		else if (token == "mesh") {
			if (!src.ReadToken(&token2)) {
				src.Warning("Unexpected end of file");
				MakeDefault();
				return false;
			}
			filename = token2;
			filename.ExtractFileExtension(extension);
			if (extension != MD5_MESH_EXT) {
				src.Warning("Invalid model for MD5 mesh");
				MakeDefault();
				return false;
			}
			modelHandle = renderModelManager->FindModel(filename);
			if (!modelHandle) {
				src.Warning("Model '%s' not found", filename.c_str());
				MakeDefault();
				return false;
			}

			if (modelHandle->IsDefaultModel()) {
				src.Warning("Model '%s' defaulted", filename.c_str());
				MakeDefault();
				return false;
			}

			// get the number of joints
			num = modelHandle->NumJoints();
			if (!num) {
				src.Warning("Model '%s' has no joints", filename.c_str());
			}

			// set up the joint hierarchy
			joints.SetGranularity(1);
			joints.SetNum(num);
			jointParents.SetNum(num);
			channelJoints[0].SetNum(num);
			md5joints = modelHandle->GetJoints();
			md5joint = md5joints;
			for (i = 0; i < num; i++, md5joint++) {
				joints[i].channel = ANIMCHANNEL_ALL;
				joints[i].num = static_cast<jointHandle_t>(i);
				if (md5joint->parent) {
					joints[i].parentNum = static_cast<jointHandle_t>(md5joint->parent - md5joints);
				}
				else {
					joints[i].parentNum = INVALID_JOINT;
				}
				jointParents[i] = joints[i].parentNum;
				channelJoints[0][i] = i;
			}
		}
		else if (token == "remove") {
			// removes any anims whos name matches
			if (!src.ReadToken(&token2)) {
				src.Warning("Unexpected end of file");
				MakeDefault();
				return false;
			}
			num = 0;
			for (i = 0; i < anims.Num(); i++) {
				if ((token2 == anims[i]->Name()) || (token2 == anims[i]->FullName())) {
					delete anims[i];
					anims.RemoveIndex(i);
					if (i >= numDefaultAnims) {
						src.Warning("Anim '%s' was not inherited.  Anim should be removed from the model def.", token2.c_str());
						MakeDefault();
						return false;
					}
					i--;
					numDefaultAnims--;
					num++;
					continue;
				}
			}
			if (!num) {
				src.Warning("Couldn't find anim '%s' to remove", token2.c_str());
				MakeDefault();
				return false;
			}
		}
		else if (token == "anim") {
			if (!modelHandle) {
				src.Warning("Must specify mesh before defining anims");
				MakeDefault();
				return false;
			}
			if (!ParseAnim(src, numDefaultAnims)) {
				MakeDefault();
				return false;
			}
		}
		else if (token == "offset") {
			if (!src.Parse1DMatrix(3, offset.ToFloatPtr())) {
				src.Warning("Expected vector following 'offset'");
				MakeDefault();
				return false;
			}
		}
		else if (token == "channel") {
			if (!modelHandle) {
				src.Warning("Must specify mesh before defining channels");
				MakeDefault();
				return false;
			}

			// set the channel for a group of joints
			if (!src.ReadToken(&token2)) {
				src.Warning("Unexpected end of file");
				MakeDefault();
				return false;
			}
			if (!src.CheckTokenString("(")) {
				src.Warning("Expected { after '%s'\n", token2.c_str());
				MakeDefault();
				return false;
			}

			for (i = ANIMCHANNEL_ALL + 1; i < ANIM_NumAnimChannels; i++) {
				if (!idStr::Icmp(channelNames[i], token2)) {
					break;
				}
			}

			if (i >= ANIM_NumAnimChannels) {
				src.Warning("Unknown channel '%s'", token2.c_str());
				MakeDefault();
				return false;
			}

			channel = i;
			jointnames = "";

			while (!src.CheckTokenString(")")) {
				if (!src.ReadToken(&token2)) {
					src.Warning("Unexpected end of file");
					MakeDefault();
					return false;
				}
				jointnames += token2;
				if ((token2 != "*") && (token2 != "-")) {
					jointnames += " ";
				}
			}

			GetJointList(jointnames, jointList);

			channelJoints[channel].SetNum(jointList.Num());
			for (num = i = 0; i < jointList.Num(); i++) {
				jointnum = jointList[i];
				if (joints[jointnum].channel != ANIMCHANNEL_ALL) {
					src.Warning("Joint '%s' assigned to multiple channels", modelHandle->GetJointName(jointnum));
					continue;
				}
				joints[jointnum].channel = channel;
				channelJoints[channel][num++] = jointnum;
			}
			channelJoints[channel].SetNum(num);
		}
		else {
			src.Warning("unknown token '%s'", token.c_str());
			MakeDefault();
			return false;
		}
	}

	// shrink the anim list down to save space
	anims.SetGranularity(1);
	anims.SetNum(anims.Num());

	return true;
}

/*
=====================
idDeclModelDef::HasAnim
=====================
*/
bool idDeclModelDef::HasAnim(const char* name) const {
	int	i;

	// find any animations with same name
	for (i = 0; i < anims.Num(); i++) {
		if (!strcmp(anims[i]->Name(), name)) {
			return true;
		}
	}

	return false;
}

/*
=====================
idDeclModelDef::NumAnims
=====================
*/
int idDeclModelDef::NumAnims(void) const {
	return anims.Num() + 1;
}

/*
=====================
idDeclModelDef::GetSpecificAnim

Gets the exact anim for the name, without randomization.
=====================
*/
int idDeclModelDef::GetSpecificAnim(const char* name) const {
	int	i;

	// find a specific animation
	for (i = 0; i < anims.Num(); i++) {
		if (!strcmp(anims[i]->FullName(), name)) {
			return i + 1;
		}
	}

	// didn't find it
	return 0;
}

/*
=====================
idDeclModelDef::GetAnim
=====================
*/
const idAnim* idDeclModelDef::GetAnim(int index) const {
	if ((index < 1) || (index > anims.Num())) {
		return NULL;
	}

	return anims[index - 1];
}

/*
=====================
idDeclModelDef::GetAnim
=====================
*/
int idDeclModelDef::GetAnim(const char* name) const {
	int				i;
	int				which;
	const int		MAX_ANIMS = 64;
	int				animList[MAX_ANIMS];
	int				numAnims;
	int				len;

	len = strlen(name);
	if (len && idStr::CharIsNumeric(name[len - 1])) {
		// find a specific animation
		return GetSpecificAnim(name);
	}

	// find all animations with same name
	numAnims = 0;
	for (i = 0; i < anims.Num(); i++) {
		if (!strcmp(anims[i]->Name(), name)) {
			animList[numAnims++] = i;
			if (numAnims >= MAX_ANIMS) {
				break;
			}
		}
	}

	if (!numAnims) {
		return 0;
	}

	// get a random anim
	//FIXME: don't access gameLocal here?
	which = gameLocal.random.RandomInt(numAnims);
	return animList[which] + 1;
}

/*
=====================
idDeclModelDef::GetSkin
=====================
*/
const idDeclSkin* idDeclModelDef::GetSkin(void) const {
	return skin;
}

/*
=====================
idDeclModelDef::GetModelName
=====================
*/
const char* idDeclModelDef::GetModelName(void) const {
	if (modelHandle) {
		return modelHandle->Name();
	}
	else {
		return "";
	}
}

/*
=====================
idDeclModelDef::Joints
=====================
*/
const idList<jointInfo_t>& idDeclModelDef::Joints(void) const {
	return joints;
}

/*
=====================
idDeclModelDef::JointParents
=====================
*/
const int* idDeclModelDef::JointParents(void) const {
	return jointParents.Ptr();
}

/*
=====================
idDeclModelDef::NumJoints
=====================
*/
int idDeclModelDef::NumJoints(void) const {
	return joints.Num();
}

/*
=====================
idDeclModelDef::GetJoint
=====================
*/
const jointInfo_t* idDeclModelDef::GetJoint(int jointHandle) const {
	if ((jointHandle < 0) || (jointHandle > joints.Num())) {
		gameLocal.Error("idDeclModelDef::GetJoint : joint handle out of range");
	}
	return &joints[jointHandle];
}

/*
====================
idDeclModelDef::GetJointName
====================
*/
const char* idDeclModelDef::GetJointName(int jointHandle) const {
	const idMD5Joint* joint;

	if (!modelHandle) {
		return NULL;
	}

	if ((jointHandle < 0) || (jointHandle > joints.Num())) {
		gameLocal.Error("idDeclModelDef::GetJointName : joint handle out of range");
	}

	joint = modelHandle->GetJoints();
	return joint[jointHandle].name.c_str();
}

/*
=====================
idDeclModelDef::NumJointsOnChannel
=====================
*/
int idDeclModelDef::NumJointsOnChannel(int channel) const {
	if ((channel < 0) || (channel >= ANIM_NumAnimChannels)) {
		gameLocal.Error("idDeclModelDef::NumJointsOnChannel : channel out of range");
	}
	return channelJoints[channel].Num();
}

/*
=====================
idDeclModelDef::GetChannelJoints
=====================
*/
const int* idDeclModelDef::GetChannelJoints(int channel) const {
	if ((channel < 0) || (channel >= ANIM_NumAnimChannels)) {
		gameLocal.Error("idDeclModelDef::GetChannelJoints : channel out of range");
	}
	return channelJoints[channel].Ptr();
}

/*
=====================
idDeclModelDef::GetVisualOffset
=====================
*/
const idVec3& idDeclModelDef::GetVisualOffset(void) const {
	return offset;
}
