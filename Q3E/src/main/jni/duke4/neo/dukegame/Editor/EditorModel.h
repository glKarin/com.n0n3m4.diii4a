// EditorModel.h
//

//
// dnEditorModel
//
class dnEditorModel : public dnEditorEntity {
public:
	dnEditorModel(idRenderWorld* editorRenderWorld);
	~dnEditorModel();

	virtual void				Render(idDict& spawnArgs, bool isSelected, const renderView_t& renderView) override;

private:
	qhandle_t	renderEntityHandle;
	renderEntity_t renderEntityParams;
};