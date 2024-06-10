// DeclModelDef.h
//


/*
==============================================================================================

	idDeclModelDef

==============================================================================================
*/

class idDeclModelDef : public idDecl {
public:
	idDeclModelDef();
	~idDeclModelDef();

	virtual size_t				Size(void) const;
	virtual const char* DefaultDefinition(void) const;
	virtual bool				Parse(const char* text, const int textLength);
	virtual void				FreeData(void);

	void						Touch(void) const;

	const idDeclSkin* GetDefaultSkin(void) const;
	const idJointQuat* GetDefaultPose(void) const;
	void						SetupJoints(int* numJoints, idJointMat** jointList, idBounds& frameBounds, bool removeOriginOffset) const;
	idRenderModel* ModelHandle(void) const;
	void						GetJointList(const char* jointnames, idList<jointHandle_t>& jointList) const;
	const jointInfo_t* FindJoint(const char* name) const;

	int							NumAnims(void) const;
	const idAnim* GetAnim(int index) const;
	int							GetSpecificAnim(const char* name) const;
	int							GetAnim(const char* name) const;
	bool						HasAnim(const char* name) const;
	const idDeclSkin* GetSkin(void) const;
	const char* GetModelName(void) const;
	const idList<jointInfo_t>& Joints(void) const;
	const int* JointParents(void) const;
	int							NumJoints(void) const;
	const jointInfo_t* GetJoint(int jointHandle) const;
	const char* GetJointName(int jointHandle) const;
	int							NumJointsOnChannel(int channel) const;
	const int* GetChannelJoints(int channel) const;

	const idVec3& GetVisualOffset(void) const;

private:
	void						CopyDecl(const idDeclModelDef* decl);
	bool						ParseAnim(idLexer& src, int numDefaultAnims);

private:
	idVec3						offset;
	idList<jointInfo_t>			joints;
	idList<int>					jointParents;
	idList<int>					channelJoints[ANIM_NumAnimChannels];
	idRenderModel* modelHandle;
	idList<idAnim*>			anims;
	const idDeclSkin* skin;
};
