// MeshComponent.h
//

//
// DnMeshComponent
//
class DnMeshComponent : public DnComponent
{
public:
	DnMeshComponent();
	~DnMeshComponent();

	void							Init(idEntity *parent, idRenderModel *componentMesh);
	void							BindToJoint(const char* jointName);

	virtual void					Think();

	void							Destroy(void);
private:
	qhandle_t	renderEntityHandle;
	renderEntity_t renderEntityParams;

	idEntity* parentEntity;
	jointHandle_t bindJoint;
};