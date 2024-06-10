// EditorLight.h
//

//
// dnEditorLight
//
class dnEditorLight : public dnEditorEntity {
public:
	dnEditorLight(idRenderWorld* editorRenderWorld);
	~dnEditorLight();

	// Renders the light.
	virtual void				Render(idDict& spawnArgs, bool isSelected, const renderView_t& renderView) override;

private:
	void						DrawProjectedLight(void);

	qhandle_t	renderLightHandle;
	renderLight_t renderLightParams;
};