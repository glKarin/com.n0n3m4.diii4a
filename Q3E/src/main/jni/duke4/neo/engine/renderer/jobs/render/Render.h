// Render.h
//

//
// idRender
//
class idRender {
public:
	void			RenderSingleView(const void* data);
private:
	void			RenderShadowMaps(void);
	void			FillDepthBuffer(drawSurf_t** drawSurfs, int numDrawSurfs);
	void			DrawForwardLit(void);
	int				DrawShaderPasses(drawSurf_t** drawSurfs, int numDrawSurfs);
	void			DrawTestOcclusion(void);
private:
	static void		DepthBufferPass(const drawSurf_t* surf);
	static void		PrepareStageTexturing(const shaderStage_t* pStage, const drawSurf_t* surf, idDrawVert* ac);
	static void		FinishStageTexturing(const shaderStage_t* pStage, const drawSurf_t* surf, idDrawVert* ac);
};

void RB_BeginDrawingView(void);

extern idRender render;