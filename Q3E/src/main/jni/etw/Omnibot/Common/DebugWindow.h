#ifndef __DEBUGWINDOW_H__
#define __DEBUGWINDOW_H__

enum RenderOverlayType
{
	OVERLAY_OPENGL,
	OVERLAY_GAME,
};

void SetRenderOverlayType(RenderOverlayType _t);

#ifdef ENABLE_DEBUG_WINDOW

// Include all necessary headers.
// Sometimes windows.h defines DELETE which causes a compilation
// error in a Guichan header.
#if defined (DELETE)
#undef DELETE 
#endif

#include <iostream>
#include <guichan.hpp>

//////////////////////////////////////////////////////////////////////////

#include "RenderOverlay.h"

extern RenderOverlay *gOverlay;

//////////////////////////////////////////////////////////////////////////

struct WindowProps
{
	int	mWidth;
	int mHeight;
};

namespace sf
{
	class RenderWindow;
};

namespace gcn
{
	class SFMLInput;
	struct MouseCache
	{
		enum MouseButton
		{
			Left,
			Mid,
			Right,
			Num,
		};

		int				X;
		int				Y;

		bool			MouseClicked[Num];

		obint8			WheelMoved;

		void Reset()
		{
			for(int i = 0; i < MouseCache::Num; ++i)
				MouseClicked[i] = false;

			WheelMoved = 0;
		}

		MouseCache()
		{
			X = Y = 0;

			for(int i = 0; i < Num; ++i)
				MouseClicked[i] = false;
		}
	};

	struct DrawInfo
	{
		gcn::Widget		*m_Widget;
		gcn::Graphics	*m_Graphics;
		int				&m_Line;
		int				m_Indent;

		MouseCache		Mouse;		

		DrawInfo indent()
		{
			DrawInfo di = *this;
			di.m_Indent++;
			return di;
		}

		DrawInfo(gcn::Widget *w, gcn::Graphics *g, int &l, int id)
			: m_Widget(w)
			, m_Graphics(g)
			, m_Line(l)
			, m_Indent(id)
		{
		}
	};

	class StateTree : public gcn::Widget, public gcn::MouseListener, public gcn::KeyListener
	{
	public:
		StateTree();

		void logic();

		void draw(gcn::Graphics* graphics);
		void drawBorder(gcn::Graphics* graphics);

		void mouseEntered(MouseEvent& mouseEvent);
		void mouseExited(MouseEvent& mouseEvent);
		void mousePressed(MouseEvent& mouseEvent);
		void mouseReleased(MouseEvent& mouseEvent);
		void mouseClicked(MouseEvent& mouseEvent);
		void mouseWheelMovedUp(MouseEvent& mouseEvent);
		void mouseWheelMovedDown(MouseEvent& mouseEvent);
		void mouseMoved(MouseEvent& mouseEvent);
		void mouseDragged(MouseEvent& mouseEvent);

		MouseCache mMouse;
	};
};

struct DebugWindow_s
{
	//////////////////////////////////////////////////////////////////////////
	// Script Console.
	struct Console_s
	{
		gcn::Window			*mContainer;
		gcn::TextBox		*mOutput;
		gcn::TextBox		*mInput;
		gcn::ScrollArea		*mOutputScroll;
		gcn::ScrollArea		*mInputScroll;

		gcn::ListBox		*mAutoComplete;

		void Init(const WindowProps &_mainwindow);
		void Shutdown();
		void AddLine(const String &_s);
		void AddLine(const char* _msg, ...);
		void ClearInput();
		void ClearOutput();

		void UpdateWordAtCaret();

		void Update();

		void DumpConsoleToFile(const char *filename);
	} Console;

	struct Hud_s
	{
		gcn::Window								*mContainer;
		gcn::ScrollArea							*mScrollArea;
		gcn::contrib::PropertySheet				*mPropertySheet;
		
		gmGCRoot<gmUserObject>					mUserObject;

		void SetActiveMapGoal(MapGoalPtr mg);

		void Init(const WindowProps &_mainwindow);
		void Shutdown();

		void Update();
	} Hud;

	struct Nav_s
	{
		gcn::contrib::AdjustingContainer		*mAdjCtr;

		void Init(const WindowProps &_mainwindow);
		void Shutdown();

		void Update();
	} Nav;

	struct Profiler_s
	{
		gcn::Window			*mContainer;
		gcn::Widget			*mProfiler;

		void Init(const WindowProps &_mainwindow);
		void Shutdown();

		void Update() {}
	} Profiler;

	struct Log_s
	{
		gcn::Window			*mContainer;
		gcn::TextBox		*mOutput;
		gcn::ScrollArea		*mOutputScroll;

		gcn::CheckBox		*mCbNormal;
		gcn::CheckBox		*mCbInfo;
		gcn::CheckBox		*mCbWarning;
		gcn::CheckBox		*mCbError;
		gcn::CheckBox		*mCbDebug;
		gcn::CheckBox		*mCbScript;

		void Init(const WindowProps &_mainwindow);
		void Shutdown();
		void AddLine(eMessageType _type, const String &_s);
		void AddLine(eMessageType _type, CHECK_PRINTF_ARGS const char* _msg, ...);

		void Update() {}
	} Log;

	struct Map_s
	{
		gcn::Window			*mContainer;
		gcn::Widget			*mMap;

		void Init(const WindowProps &_mainwindow);
		void Shutdown();

		void Update() {}
	} Map;

	struct Goals_s
	{
		gcn::Window								*mContainer;
		gcn::ListBox							*mGoalList;
		gcn::ScrollArea							*mInfoScrollArea;
		gcn::contrib::AdjustingContainer		*mGoalInfoContainer;

		void Init(const WindowProps &_mainwindow);
		void Shutdown();

		void Update();
	} Goals;

	struct StateTree_s
	{
		gcn::Window			*mContainer;
		gcn::ListBox		*mClientList;

		gcn::ScrollArea		*mClientScroll;
		gcn::ScrollArea		*mStateScroll;

		gcn::StateTree		*mStateTree;

		void Init(const WindowProps &_mainwindow);
		void Shutdown();

		void Update() {}
	} StateTree;

	struct Script_s
	{
		gcn::Window			*mContainer;
		gcn::ListBox		*mThreadList;
		gcn::ScrollArea		*mOutputScroll;

		void Init(const WindowProps &_mainwindow);
		void Shutdown();

		void Update() {}
	} Script;

	//////////////////////////////////////////////////////////////////////////
	// Core Variables
	struct Core_s
	{
		sf::RenderWindow			*mMainWindow;
		gcn::SFMLInput				*mInput;
		gcn::Graphics				*mGraphics;
		gcn::ImageLoader			*mImageLoader;

		gcn::Gui					*mGui;
		gcn::ImageFont				*mFont;
		gcn::Label					*mLabel;
	} Core;
	
	//////////////////////////////////////////////////////////////////////////
	gcn::Container		*mTop;

	gcn::Button			*mHudBtn;
	gcn::Button			*mProfilerBtn;
	gcn::Button			*mStateTreeBtn;
	gcn::Button			*mMapBtn;
	gcn::Button			*mLogBtn;
	gcn::Button			*mConsoleBtn;
	gcn::Button			*mScriptBtn;
	gcn::Button			*mGoalsBtn;	
	gcn::Button			*mNavBtn;

	obint32				mButtonXOffset;
	//////////////////////////////////////////////////////////////////////////

	gcn::Button *AddPageButton(const String &_btntext);
	gcn::Window *AddPageWindow(const String &_wintext, gcn::Button *togglebutton);

	void Init(const WindowProps &_mainwindow);
	void Shutdown();

	void Update();
};

extern DebugWindow_s DW;

namespace DebugWindow
{
	void Create(int _width = 800, int _height = 600, int _bpp = 32);
	void Destroy();
	//void Render();
	void Update();
};

//////////////////////////////////////////////////////////////////////////
void ShowStateWindow(obuint32 name);
//////////////////////////////////////////////////////////////////////////
class WeaponSystem_DW : public gcn::Window
{
public:
	void logic();
	WeaponSystem_DW();
	~WeaponSystem_DW();
private:
	gcn::ListBox	*mWeaponList;
	gcn::ScrollArea	*mWeaponScroll;
};
//////////////////////////////////////////////////////////////////////////
class SensoryMemory_DW : public gcn::Window
{
public:
	void logic();
	SensoryMemory_DW();
	~SensoryMemory_DW();
private:
	gcn::ListBox	*mSensoryList;
	gcn::ScrollArea	*mSensoryScroll;
	gcn::CheckBox	*mShowPerception;
	gcn::CheckBox	*mShowRecords;
};
//////////////////////////////////////////////////////////////////////////
class Aimer_DW : public gcn::Window
{
public:
	void logic();
	Aimer_DW();
	~Aimer_DW();
private:
	gcn::ListBox	*mAimerList;
	gcn::ScrollArea	*mAimerScroll;
};
//////////////////////////////////////////////////////////////////////////
class ScriptGoal_DW : public gcn::Window
{
public:
	void logic();
	ScriptGoal_DW();
	~ScriptGoal_DW();
private:
};
//////////////////////////////////////////////////////////////////////////

#endif
#endif
