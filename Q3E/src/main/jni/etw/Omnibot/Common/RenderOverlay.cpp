#include "PrecompCommon.h"
#include "RenderOverlay.h"

//////////////////////////////////////////////////////////////////////////

ReferencePoint Viewer;

void RenderOverlay::UpdateViewer()
{
	Utils::GetLocalEyePosition(Viewer.EyePos);
	Utils::GetLocalFacing(Viewer.Facing);
}

//////////////////////////////////////////////////////////////////////////
typedef std::list<RenderOverlayUser*> OverlayUsers;

static OverlayUsers RenderOverlayUsers;

RenderOverlayUser::RenderOverlayUser()
{
	RenderOverlayUsers.push_back(this);
}
RenderOverlayUser::~RenderOverlayUser()
{
	RenderOverlayUsers.remove(this);
}
void RenderOverlayUser::OverlayRenderAll(RenderOverlay *overlay, const ReferencePoint &viewer)
{
	for(OverlayUsers::iterator it = RenderOverlayUsers.begin();
		it != RenderOverlayUsers.end();
		++it)
	{
		(*it)->OverlayRender(overlay,viewer);
	}
}
