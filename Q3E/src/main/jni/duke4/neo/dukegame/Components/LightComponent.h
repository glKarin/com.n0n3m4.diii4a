// LightComponent.h
//

//
// DnLightComponent
//
class DnLightComponent : public DnComponent
{
public:
	DnLightComponent();
	~DnLightComponent();

	void							Init(idEntity* parent, idVec3 radius, idVec3 color, bool noShadows);
	void							BindToJoint(const char* jointName);
	void							SetForwardOffset(float forwardOffset) {
		this->forwardOffset = forwardOffset;
	}

	virtual void					Think();


	renderLight_t renderLightParams;
private:
	qhandle_t	renderLightHandle;	

	idEntity* parentEntity;
	jointHandle_t bindJoint;
	float forwardOffset;
};