#ifndef __RENDER_OVERLAY_H__
#define __RENDER_OVERLAY_H__

struct ReferencePoint 
{
	Vector3f	EyePos;
	Vector3f	Facing;
};

class RenderOverlay
{
public:

	virtual bool Initialize(int Width, int Height) = 0;
	virtual void PostInitialize() = 0;
	virtual void Update() = 0;
	//virtual void Render() = 0;

	virtual void SetColor(const obColor &col) = 0;

	void UpdateViewer();

	virtual void PushMatrix() = 0;
	virtual void PopMatrix() = 0;
	virtual void Translate(const Vector3f &v) = 0;

	// render functions.
	virtual void DrawLine(const Vector3f &p1, const Vector3f &p2) = 0;
	virtual void DrawPolygon(const Vector3List &verts) = 0;
	virtual void DrawRadius(const Vector3f p, float radius) = 0;
	virtual void DrawAABB(const AABB &aabb) = 0;
	virtual void DrawAABB(const AABB &aabb, const Vector3f &pos, const Matrix3f &orientation) = 0;

	virtual ~RenderOverlay() {}
};

class RenderOverlayUser
{
public:

	// allow all users to render
	static void OverlayRenderAll(RenderOverlay *overlay, const ReferencePoint &viewer);

	RenderOverlayUser();
	virtual ~RenderOverlayUser();
private:
	// subclasses should implement this.
	virtual void OverlayRender(RenderOverlay *overlay, const ReferencePoint &viewer) {}
};

extern RenderOverlay *gOverlay;
extern ReferencePoint Viewer;

#endif
