#include "PrecompCommon.h"
#include "RenderOverlay.h"

#ifdef ENABLE_DEBUG_WINDOW

#include <DebugDraw.h>

#include <guichan/sfml.hpp>
#include <guichan/opengl.hpp>

//////////////////////////////////////////////////////////////////////////

namespace GameContext
{
	HGLRC mContext = 0;
	HDC mDC = 0;

	HGLRC mGuiContext = 0;
	HDC mGuiDC = 0;

	void SaveGame()
	{	
		mContext = wglGetCurrentContext();
		mDC = wglGetCurrentDC();
	}

	void RestoreGame()
	{
		if(mContext && mDC)
			wglMakeCurrent(mDC,mContext);
	}

	void SaveGui()
	{
		mGuiContext = wglGetCurrentContext();
		mGuiDC = wglGetCurrentDC();
	}

	void RestoreGui()
	{
		if(mGuiContext && mGuiDC)
			wglMakeCurrent(mGuiDC,mGuiContext);
	}
}

//////////////////////////////////////////////////////////////////////////

struct duDebugDrawGame : public duDebugDraw
{
	void depthMask(bool state)
	{
	}
	void begin(duDebugDrawPrimitives prim, float size = 1.0f)
	{
		curPrimType = prim;
		currentVert = 0;
	}
	void vertex(const float* pos, unsigned int color)
	{
		vertex(pos[0],pos[1],pos[2],color);
	}
	void vertex(const float x, const float y, const float z, unsigned int color)
	{
		switch(curPrimType)
		{
		case DU_DRAW_TRIS:
			{
				const obVec3 v = {x,z,y};
				verts[currentVert++] = v;

				if(currentVert==3)
				{
					g_EngineFuncs->DebugPolygon(
						verts,
						3,
						color,
						IGame::GetDeltaTimeSecs()*2.f,
						0);
					currentVert = 0;
				}
				break;
			}
		case DU_DRAW_POINTS:
			{
				const obVec3 v = {x,z,y};
				verts[currentVert++] = v;

				if(currentVert==1)
				{					
					currentVert = 0;
				}
				break;
			}
		case DU_DRAW_LINES:
			{
				const obVec3 v = {x,y,z};
				verts[currentVert++] = v;

				if(currentVert==2)
				{
					const float start[3] = { verts[0].x, verts[0].z, verts[0].y };
					const float end[3] = { verts[1].x, verts[1].z, verts[1].y };
					g_EngineFuncs->DebugLine(
						start,
						end,
						color,
						IGame::GetDeltaTimeSecs()*2.f);

					currentVert = 0;
				}
				break;
			}
		case DU_DRAW_QUADS:
			{
				const obVec3 v = {x,z,y};
				verts[currentVert++] = v;

				if(currentVert==4)
				{
					/*g_EngineFuncs->DebugPolygon(
					verts,
					3,
					currentColor,
					IGame::GetDeltaTimeSecs()*2.f,
					0);*/
					currentVert = 0;
				}
				break;
			}
		default:
			currentVert = 0;
			break;
		}
	}
	void end()
	{
	}

	duDebugDrawPrimitives	curPrimType;
	obVec3					verts[4];
	int						currentVert;
};

//class RenderInterfaceGame : public RenderInterface
//{
//public:
//	PrimType	curPrimType;
//	obColor		currentColor;
//	obVec3		verts[4];
//	int			currentVert;
//
//	void StartFrame()
//	{
//	}
//
//	void SetColor(float r,float g,float b,float a)
//	{
//		currentColor.FromFloat(r,g,b,a);
//	}
//	void StartPrimitives(PrimType t)
//	{
//		curPrimType = t;
//		currentVert = 0;
//	}
//
//	void EndPrimitives()
//	{
//		// close the loop
//		if(curPrimType == R_LINE_LOOP)
//		{
//			const float start[3] = 
//			{ 
//				verts[0].x, 
//				verts[0].z,
//				verts[0].y 
//			};
//			const float end[3] = 
//			{ 
//				verts[currentVert-1].x, 
//				verts[currentVert-1].z,
//				verts[currentVert-1].y 
//			};
//			g_EngineFuncs->DebugLine(
//				start,
//				end,
//				currentColor,
//				IGame::GetDeltaTimeSecs()*2.f);
//		}
//
//		curPrimType = R_NONE;
//	}
//
//	void LineWidth(float width)
//	{
//		//glLineWidth(width);
//	}
//	void PointSize(float size)
//	{
//		//glPointSize(size);
//	}
//
//	void Vertex(float x,float y,float z)
//	{		
//		switch(curPrimType)
//		{
//		case R_TRIANGLES:
//			{
//				const obVec3 v = {x,z,y};
//				verts[currentVert++] = v;
//
//				if(currentVert==3)
//				{
//					g_EngineFuncs->DebugPolygon(
//						verts,
//						3,
//						currentColor,
//						IGame::GetDeltaTimeSecs()*2.f,
//						0);
//					currentVert = 0;
//				}
//				break;
//			}
//		case R_POINTS:
//			{
//				const obVec3 v = {x,z,y};
//				verts[currentVert++] = v;
//
//				if(currentVert==1)
//				{					
//					currentVert = 0;
//				}
//				break;
//			}
//		case R_LINES:
//			{
//				const obVec3 v = {x,y,z};
//				verts[currentVert++] = v;
//
//				if(currentVert==2)
//				{
//					const float start[3] = { verts[0].x, verts[0].z, verts[0].y };
//					const float end[3] = { verts[1].x, verts[1].z, verts[1].y };
//					g_EngineFuncs->DebugLine(
//						start,
//						end,
//						currentColor,
//						IGame::GetDeltaTimeSecs()*2.f);
//
//					currentVert = 0;
//				}
//				break;
//			}
//		case R_LINE_LOOP:
//			{
//				const obVec3 v = {x,z,y};
//				verts[currentVert++] = v;
//				if(currentVert==2||currentVert==3)
//				{
//					const float start[3] = 
//					{ 
//						verts[currentVert-2].x, 
//						verts[currentVert-2].z,
//						verts[currentVert-2].y 
//					};
//					const float end[3] = 
//					{ 
//						verts[currentVert-1].x, 
//						verts[currentVert-1].z,
//						verts[currentVert-1].y 
//					};
//					g_EngineFuncs->DebugLine(
//						start,
//						end,
//						currentColor,
//						IGame::GetDeltaTimeSecs()*2.f);
//
//					if(currentVert==3)
//					{
//						verts[1] = verts[2];
//						currentVert = 1;
//					}
//				}
//				break;
//			}
//		case R_QUADS:
//			{
//				const obVec3 v = {x,z,y};
//				verts[currentVert++] = v;
//
//				if(currentVert==4)
//				{
//					/*g_EngineFuncs->DebugPolygon(
//						verts,
//						3,
//						currentColor,
//						IGame::GetDeltaTimeSecs()*2.f,
//						0);*/
//					currentVert = 0;
//				}
//				break;
//			}
//		default:
//			currentVert = 0;
//			break;
//		}
//	}
//};

duDebugDrawGame	gGameRenderInterface;

//////////////////////////////////////////////////////////////////////////

class RenderOverlayGame : public RenderOverlay
{
public:
	bool Initialize(int Width, int Height);
	void PostInitialize();
	void Update();
	void Render();

	void SetColor(const obColor &col);

	void PushMatrix();
	void PopMatrix();
	void Translate(const Vector3f &v);

	void DrawLine(const Vector3f &p1, const Vector3f &p2);
	void DrawPolygon(const Vector3List &verts);
	void DrawRadius(const Vector3f p, float radius);
	void DrawAABB(const AABB &aabb);
	void DrawAABB(const AABB &aabb, const Vector3f &pos, const Matrix3f &orientation);

	~RenderOverlayGame();
};

//////////////////////////////////////////////////////////////////////////

bool RenderOverlayGame::Initialize(int Width, int Height)
{
	GameContext::SaveGame();

	DW.Core.mMainWindow = new sf::RenderWindow(
		sf::VideoMode(Width,Height),
		"Omni-bot",
		sf::Style::Titlebar);

	GameContext::SaveGui();
	GameContext::RestoreGame();

	DW.Core.mImageLoader = new gcn::SFMLImageLoader();
	DW.Core.mGraphics = new gcn::OpenGLGraphics(Width, Height);

	gcn::Image::setImageLoader(DW.Core.mImageLoader);

	DW.Core.mInput = new gcn::SFMLInput();

	extern duDebugDraw * gDDraw;
	gDDraw = &gGameRenderInterface;

	return true;
}

RenderOverlayGame::~RenderOverlayGame()
{
}

void RenderOverlayGame::PostInitialize()
{
}

void RenderOverlayGame::PushMatrix()
{
}

void RenderOverlayGame::PopMatrix()
{
}

void RenderOverlayGame::Translate(const Vector3f &v)
{
}

void RenderOverlayGame::Render()
{
	RenderOverlayUser::OverlayRenderAll(this,Viewer);

	GameContext::RestoreGui();
	DW.Core.mGui->draw();
	DW.Core.mMainWindow->Display();
	GameContext::RestoreGame();
}

void RenderOverlayGame::Update()
{
	UpdateViewer();
	Render();
}

obColor gCurrentColor = COLOR::WHITE;

void RenderOverlayGame::SetColor(const obColor &col)
{
	gCurrentColor = col;
}

void RenderOverlayGame::DrawLine(const Vector3f &p1, const Vector3f &p2)
{
	Utils::DrawLine(p1,p2,gCurrentColor,IGame::GetDeltaTimeSecs());
}

void RenderOverlayGame::DrawPolygon(const Vector3List &verts)
{
	Utils::DrawPolygon(verts,gCurrentColor,IGame::GetDeltaTimeSecs());
}

void RenderOverlayGame::DrawRadius(const Vector3f p, float radius)
{
	Utils::DrawRadius(p,radius,gCurrentColor,IGame::GetDeltaTimeSecs());
}

void RenderOverlayGame::DrawAABB(const AABB_t &aabb)
{
	Utils::OutlineAABB(aabb,gCurrentColor,IGame::GetDeltaTimeSecs());
}

void RenderOverlayGame::DrawAABB(const AABB &aabb, const Vector3f &pos, const Matrix3f &orientation)
{
}

//////////////////////////////////////////////////////////////////////////

RenderOverlay *CreateGameRenderOverlay()
{
	return new RenderOverlayGame;
}

#endif
