#include "PrecompCommon.h"
#include "RenderOverlay.h"

#ifdef ENABLE_DEBUG_WINDOW

#include "detours.h"
#include <DebugDraw.h>

#pragma comment(lib,"opengl32.lib")
//#pragma comment(lib,"freetype.lib")

#include <guichan/sfml.hpp>
#include <guichan/opengl.hpp>

//////////////////////////////////////////////////////////////////////////

bool RenderOverlayOpenGLUseViewer = false;

class RenderOverlayOpenGL : public RenderOverlay
{
public:
	void StartFrame();

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
	
	~RenderOverlayOpenGL();

private:
};

//////////////////////////////////////////////////////////////////////////
// Hooked functions

static BOOL (WINAPI * trueSwapBuffers)(HDC) = SwapBuffers;
static void (WINAPI * truegluPerspective)(GLdouble,GLdouble,GLdouble,GLdouble) = gluPerspective;
static void (WINAPI * trueglFrustum)(GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble) = glFrustum;
static void (WINAPI * trueglViewPort)(GLint x, GLint y, GLsizei width, GLsizei height) = glViewport;
static void (WINAPI * trueglLoadMatrixf)(const GLfloat *m) = glLoadMatrixf;
static void (WINAPI * trueglVertex3f)(GLfloat x, GLfloat y, GLfloat z) = glVertex3f;
static void (WINAPI * trueglVertex3fv)(const GLfloat *v) = glVertex3fv;

struct  
{
	GLdouble left;
	GLdouble right;
	GLdouble bottom;
	GLdouble top;
	GLdouble zNear;
	GLdouble zFar;
} Frustum;

struct
{
	GLdouble fovy;
	GLdouble aspect;
	GLdouble zNear;
	GLdouble zFar;
} Perspective;

struct
{
	GLint x;
	GLint y;
	GLsizei width;
	GLsizei height;
} ViewPort;

struct
{
	GLfloat Projection[16];
	GLfloat ModelView[16];
	GLfloat ModelViewLast[16];
} Matrices;

GLint lastMatrixMode = 0;

int CountProjection = 0;
int CountModelView = 0;

int RecordProjection = 0;
int RecordModelView = 0;

namespace Hooked
{
	void WINAPI gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
	{
		Perspective.fovy = fovy;
		Perspective.aspect = aspect;
		Perspective.zNear = zNear;
		Perspective.zFar = zFar;	
		truegluPerspective(fovy,aspect,zNear,zFar);
	}
	void WINAPI glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
	{
		Frustum.left = left;
		Frustum.right = right;
		Frustum.bottom = bottom;
		Frustum.top = top;
		Frustum.zNear = zNear;
		Frustum.zFar = zFar;
		trueglFrustum(left,right,bottom,top,zNear,zFar);
	}
	void WINAPI glViewPort(GLint x, GLint y, GLsizei width, GLsizei height)
	{
		// did the resolution change?
		if(ViewPort.width != width || ViewPort.height != height)
		{
			gcn::OpenGLGraphics *glGraphics = static_cast<gcn::OpenGLGraphics*>(DW.Core.mGraphics);
			glGraphics->setTargetPlane(width,height);

			//DW.Core.mMainWindow->setSize(width,height);
			DW.Core.mGui->getTop()->setSize(width,height);
		}

		ViewPort.x = x;
		ViewPort.y = y;
		ViewPort.width = width;
		ViewPort.height = height;
		trueglViewPort(x,y,width,height);
	}
	void WINAPI glLoadMatrixf(const GLfloat *m)
	{
		GLint param;
		glGetIntegerv(GL_MATRIX_MODE,&param);
		switch(param)
		{
		case GL_PROJECTION:
			{
				if(CountProjection==RecordProjection)
				{
					memcpy(Matrices.Projection,m,sizeof(GLfloat)*16);
				}
				++CountProjection;
				break;
			}
		case GL_MODELVIEW:
			{
				if(CountModelView==RecordModelView)
				{
					memcpy(Matrices.ModelView,m,sizeof(GLfloat)*16);
				}
				memcpy(Matrices.ModelViewLast,m,sizeof(GLfloat)*16);
				++CountModelView;
				break;
			}
		}
		lastMatrixMode = param;
		trueglLoadMatrixf(m);
	}
	BOOL WINAPI SwapBuffers(HDC dc)
	{
		static_cast<RenderOverlayOpenGL*>(gOverlay)->Render();
		return trueSwapBuffers(dc);
	}

	/*void WINAPI glVertex3f (GLfloat x, GLfloat y, GLfloat z)
	{
		switch(gOverlay->GetXForm())
		{
		case RenderOverlay::XFORM_RECAST:
			trueglVertex3f(x,z,y);
			break;
		case RenderOverlay::XFORM_NONE:
		default:
			trueglVertex3f(x,y,z);
		}
	}
	void WINAPI glVertex3fv (const GLfloat *v)
	{
		switch(gOverlay->GetXForm())
		{
		case RenderOverlay::XFORM_RECAST:
			trueglVertex3f(v[0],v[2],v[1]);
			break;
		case RenderOverlay::XFORM_NONE:
		default:
			trueglVertex3fv(v);
		}
	}*/
};

//////////////////////////////////////////////////////////////////////////
// recast uses a different coordinate system than omni-bot, so we need to
// turn xyz into xzy
struct duDebugDrawOpenGlOmni : public duDebugDraw
{
	void depthMask(bool state)
	{
	}
	void begin(duDebugDrawPrimitives prim,float size)
	{
		switch (prim)
		{
		case DU_DRAW_POINTS:
			glPointSize(size);
			glBegin(GL_POINTS);
			break;
		case DU_DRAW_LINES:
			glLineWidth(size);
			glBegin(GL_LINES);
			break;
		case DU_DRAW_TRIS:
			glBegin(GL_TRIANGLES);
			break;
		case DU_DRAW_QUADS:
			glBegin(GL_QUADS);
			break;
		};
	}
	void vertex(const float* pos, unsigned int color)
	{
		glColor4ubv((GLubyte*)&color);
		glVertex3f(pos[0],pos[2],pos[1]);
	}
	void vertex(const float x, const float y, const float z, unsigned int color)
	{
		glColor4ubv((GLubyte*)&color);
		glVertex3f(x,z,y);
	}
	void end()
	{
		glEnd();
		glLineWidth(1.0f);
		glPointSize(1.0f);
	}
};

duDebugDrawOpenGlOmni gOpenGLRenderer;

void RenderOverlayOpenGL::StartFrame()
{
}

bool RenderOverlayOpenGL::Initialize(int Width, int Height)
{
	DetourRestoreAfterWith();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourAttach(&(PVOID&)trueSwapBuffers, Hooked::SwapBuffers);
	DetourAttach(&(PVOID&)truegluPerspective, Hooked::gluPerspective);
	DetourAttach(&(PVOID&)trueglFrustum, Hooked::glFrustum);
	DetourAttach(&(PVOID&)trueglViewPort, Hooked::glViewPort);
	DetourAttach(&(PVOID&)trueglLoadMatrixf, Hooked::glLoadMatrixf);
	//DetourAttach(&(PVOID&)trueglVertex3f, Hooked::glVertex3f);
	//DetourAttach(&(PVOID&)trueglVertex3fv, Hooked::glVertex3fv);

	const LONG error = DetourTransactionCommit();
	OBASSERT(error == NO_ERROR,"DetourTransactionCommit: %d",error);

	//////////////////////////////////////////////////////////////////////////
	HWND wnd = GetForegroundWindow();
	DW.Core.mMainWindow = new sf::RenderWindow(wnd, sf::WindowSettings(), true);

	DW.Core.mImageLoader = new gcn::SFMLImageLoader();
	DW.Core.mGraphics = new gcn::OpenGLGraphics(Width, Height);

	gcn::Image::setImageLoader(DW.Core.mImageLoader);

	DW.Core.mInput = new gcn::SFMLInput();

	extern duDebugDraw * gDDraw;
	gDDraw = &gOpenGLRenderer;

	return error==NO_ERROR;
}

RenderOverlayOpenGL::~RenderOverlayOpenGL()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourDetach(&(PVOID&)trueSwapBuffers, Hooked::SwapBuffers);
	DetourDetach(&(PVOID&)truegluPerspective, Hooked::gluPerspective);
	DetourDetach(&(PVOID&)trueglFrustum, Hooked::glFrustum);
	DetourDetach(&(PVOID&)trueglViewPort, Hooked::glViewPort);
	DetourDetach(&(PVOID&)trueglLoadMatrixf, Hooked::glLoadMatrixf);
	//DetourDetach(&(PVOID&)trueglVertex3f, Hooked::glVertex3f);
	//DetourDetach(&(PVOID&)trueglVertex3fv, Hooked::glVertex3fv);

	const LONG error = DetourTransactionCommit();
	OBASSERT(error == NO_ERROR,"DetourTransactionCommit: %d",error);
}

void RenderOverlayOpenGL::PostInitialize()
{
	// overlays are see through
	DW.mTop->setOpaque(false);
	DW.Hud.mContainer->setOpaque(false);
	DW.Goals.mContainer->setOpaque(false);
	DW.Script.mContainer->setOpaque(false);
	DW.Log.mContainer->setOpaque(false);
	DW.StateTree.mContainer->setOpaque(false);
	DW.Profiler.mContainer->setOpaque(false);
}

void RenderOverlayOpenGL::PushMatrix()
{
	glPushMatrix();
}

void RenderOverlayOpenGL::PopMatrix()
{
	glPopMatrix();
}

void RenderOverlayOpenGL::Translate(const Vector3f &v)
{
	glTranslatef(v.x,v.y,v.z);
}

void RenderOverlayOpenGL::Render()
{
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);

	glEnable(GL_SCISSOR_TEST);
	glPointSize(2.0);
	glLineWidth(2.0);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	trueglLoadMatrixf(Matrices.Projection);

	/*static float fov = 90.f;
	GLdouble aspect = (GLdouble)ViewPort.width / (GLdouble)ViewPort.height;
	gluPerspective(fov,aspect,1,5000);*/

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	if(RenderOverlayOpenGLUseViewer)
	{
		Vector3f p0 = Viewer.EyePos;
		Vector3f p1 = Viewer.EyePos + Viewer.Facing;
		gluLookAt(
			p0.x,
			p0.y,
			p0.z,
			p1.x,
			p1.y,
			p1.z,
			0.f,
			0.f,
			1.f);
	}
	else
	{
		trueglLoadMatrixf(Matrices.ModelView);
	}

	RenderOverlayUser::OverlayRenderAll(this,Viewer);

	//////////////////////////////////////////////////////////////////////////

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_TEXTURE);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();

	DW.Core.mGui->draw();

	CountProjection = 0;
	CountModelView = 0;
}

void RenderOverlayOpenGL::Update()
{
	UpdateViewer();
	//Render();
}

void RenderOverlayOpenGL::SetColor(const obColor &col)
{
	glColor4f(col.rF(),col.gF(),col.bF(),col.aF());
}

void RenderOverlayOpenGL::DrawLine(const Vector3f &p1, const Vector3f &p2)
{
	glBegin(GL_LINES);
		glVertex3fv(p1);
		glVertex3fv(p2);
	glEnd();
}

void RenderOverlayOpenGL::DrawPolygon(const Vector3List &verts)
{
	if(!verts.empty())
	{
		glBegin(GL_TRIANGLE_FAN);
			for(obuint32 i = 0; i < verts.size(); ++i)
			{
				glVertex3fv(verts[i]);
			}
		glEnd();
	}	
}

void RenderOverlayOpenGL::DrawRadius(const Vector3f p, float radius)
{
	glPushMatrix();
	glTranslatef(p.x, p.y, p.z);

	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	
	GLUquadric *q = gluNewQuadric();
		gluQuadricOrientation(q,GLU_OUTSIDE);
		gluDisk(q,radius,radius,16,1);
		//gluQuadricOrientation(q,GLU_INSIDE);
		//gluCylinder(q,32,32,72,20,1);
	gluDeleteQuadric(q);

	glPopMatrix();
}

void RenderOverlayOpenGL::DrawAABB(const AABB_t &aabb)
{
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

	float verts[8*3] =
	{
		aabb.m_Mins[0], aabb.m_Mins[1], aabb.m_Mins[2],
		aabb.m_Maxs[0], aabb.m_Mins[1], aabb.m_Mins[2],
		aabb.m_Maxs[0], aabb.m_Mins[1], aabb.m_Maxs[2],
		aabb.m_Mins[0], aabb.m_Mins[1], aabb.m_Maxs[2],
		aabb.m_Mins[0], aabb.m_Maxs[1], aabb.m_Mins[2],
		aabb.m_Maxs[0], aabb.m_Maxs[1], aabb.m_Mins[2],
		aabb.m_Maxs[0], aabb.m_Maxs[1], aabb.m_Maxs[2],
		aabb.m_Mins[0], aabb.m_Maxs[1], aabb.m_Maxs[2],
	};
	static const unsigned char inds[6*5] =
	{
		7, 6, 5, 4,
		0, 1, 2, 3,
		1, 5, 6, 2,
		3, 7, 4, 0,
		2, 6, 7, 3,
		0, 4, 5, 1,
	};

	glBegin(GL_QUADS);
	const unsigned char* in = inds;
	for (int i = 0; i < 6; ++i)
	{
		glVertex3fv(&verts[*in*3]); in++;
		glVertex3fv(&verts[*in*3]); in++;
		glVertex3fv(&verts[*in*3]); in++;
		glVertex3fv(&verts[*in*3]); in++;
	}
	glEnd();
}

void RenderOverlayOpenGL::DrawAABB(const AABB &aabb, const Vector3f &pos, const Matrix3f &orientation)
{
	glPushMatrix();
	glTranslatef(pos.x,pos.y,pos.z);
	
	/*const Vector3f fwd = orientation.GetRow(0);
	const Vector3f right = orientation.GetRow(1);
	const Vector3f fwd = orientation.GetRow(2);

	const float length = aabb.GetAxisLength(0);
	const float width = aabb.GetAxisLength(1);
	const float height = aabb.GetAxisLength(2);*/

	float angle;
	Vector3f axis;
	orientation.ToAxisAngle(axis,angle);
	static int mode = 0;
	static float anglFlip = -1.f;
	if(mode==0)
		glRotatef(Mathf::RadToDeg(anglFlip*angle),axis.x,axis.y,axis.z);
	else if(mode==1)
		glRotatef(Mathf::RadToDeg(anglFlip*angle),-axis.x,axis.y,axis.z);
	else if(mode==2)
		glRotatef(Mathf::RadToDeg(anglFlip*angle),-axis.z,axis.x,axis.y);

	glMultMatrixf(orientation);

	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

	const float verts[8*3] =
	{
		aabb.m_Mins[0], aabb.m_Mins[1], aabb.m_Mins[2],
		aabb.m_Maxs[0], aabb.m_Mins[1], aabb.m_Mins[2],
		aabb.m_Maxs[0], aabb.m_Mins[1], aabb.m_Maxs[2],
		aabb.m_Mins[0], aabb.m_Mins[1], aabb.m_Maxs[2],
		aabb.m_Mins[0], aabb.m_Maxs[1], aabb.m_Mins[2],
		aabb.m_Maxs[0], aabb.m_Maxs[1], aabb.m_Mins[2],
		aabb.m_Maxs[0], aabb.m_Maxs[1], aabb.m_Maxs[2],
		aabb.m_Mins[0], aabb.m_Maxs[1], aabb.m_Maxs[2],
	};
	static const unsigned char inds[6*4] =
	{
		7, 6, 5, 4,
		0, 1, 2, 3,
		1, 5, 6, 2,
		3, 7, 4, 0,
		2, 6, 7, 3,
		0, 4, 5, 1,
	};

	glBegin(GL_QUADS);
	const unsigned char* in = inds;
	for (int i = 0; i < 6; ++i)
	{
		glVertex3fv(&verts[*in*3]); in++;
		glVertex3fv(&verts[*in*3]); in++;
		glVertex3fv(&verts[*in*3]); in++;
		glVertex3fv(&verts[*in*3]); in++;
	}
	glEnd();

	glPopMatrix();
}

//////////////////////////////////////////////////////////////////////////

RenderOverlay *CreateOpenGLRenderOverlay()
{
	return new RenderOverlayOpenGL;
}

#endif //#ifdef ENABLE_DEBUG_WINDOW