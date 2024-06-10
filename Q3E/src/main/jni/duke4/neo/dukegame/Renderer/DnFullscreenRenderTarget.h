// DnRender.h
//

//
// DnFullscreenRenderTarget
//
class DnFullscreenRenderTarget {
public:
	DnFullscreenRenderTarget(const char *name, bool hasAlbedo, bool hasDepth, bool hasMSAA, textureFormat_t albedoFormat2 = FMT_NONE, textureFormat_t albedoFormat3 = FMT_NONE, textureFormat_t albedoFormat4 = FMT_NONE);

	void Bind(void);
	void ResolveMSAA(DnFullscreenRenderTarget* destTarget);
	void Resize(int width, int height);
	void Clear(void);

	static void BindNull(void);
private:
	idImage* albedoImage[4];
	idImage* depthImage;
	idRenderTexture* renderTexture;
	int numMultiSamples;
};
